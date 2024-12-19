import re
from typing import Union

import glob
import os
import subprocess
import sys
import zipfile
import tempfile
import filecmp
import zlib
import tarfile

from .common import is_linux, is_windows, run_command_sudo

# Use path_join to create a new path with proper seperators for the host system.
# (this is to solve a problem on windows with '\' versus '/')
def path_join(*args: Union[str, os.PathLike]) -> str:
    return os.path.normpath(os.path.join(*args))

def compare_dir_recursive(dir1, dir2):
    # Recursively compare subdirectories and files content
    comparison = filecmp.dircmp(dir1, dir2)
    if comparison.diff_files:
        print(f"Differing files: {comparison.diff_files}")
    if comparison.left_only:
        print(f"Files only in {dir1}: {comparison.left_only}")
    if comparison.right_only:
        print(f"Files only in {dir2}: {comparison.right_only}")
    result = not comparison.diff_files and not comparison.left_only and not comparison.right_only
    if not result:
        return False

    for subdir in comparison.common_dirs:
        if not compare_dir_recursive(path_join(dir1, subdir), path_join(dir2, subdir)):
            return False

    return True # Identical

def remove_lib_files_recursive(target_path: str):
    # Comparing libraries is often not deterministic even if the source code was not changed
    # (e.g. because of embedded __DATE__).
    #
    # This function remove all library files at the target_path, leaving
    # presumably only the source files.
    #
    # Note: To further help detect differences in any source files, all
    # packages should includes ta_common.h which provides a source digest.
    #
    # That source digest is necessary because some package includes only the
    # headers... not all the files that were used to build the package (but
    # all are reflected in the digest)
    if is_windows():
        for root, dirs, files in os.walk(target_path):
            for file in files:
                if file.endswith('.msi') or file.endswith('.lib') or file.endswith('.dll'):
                    os.remove(path_join(root, file))
    elif is_linux():
        pattern = r'\.so\.\d+'
        for root, dirs, files in os.walk(target_path):
            for file in files:
                if file.endswith('.a') or file.endswith('.so') or re.search(pattern, file) is not None:
                    os.remove(path_join(root, file))

def compare_zip_files(zip_file1, zip_file2):
    # Does a comparison of the contents of the two zip files.
    # Ignores file creation time and binaries.
    with tempfile.TemporaryDirectory() as temp_extract_dir:
        temp_extract_path1 = path_join(temp_extract_dir, 'temp1')
        temp_extract_path2 = path_join(temp_extract_dir, 'temp2')
        os.makedirs(temp_extract_path1, exist_ok=True)
        os.makedirs(temp_extract_path2, exist_ok=True)

        with zipfile.ZipFile(zip_file1, 'r') as zip_ref:
            zip_ref.extractall(temp_extract_path1)

        with zipfile.ZipFile(zip_file2, 'r') as zip_ref:
            zip_ref.extractall(temp_extract_path2)

        # Remove in both temp directory all library files.
        # Only the remaining source files will be compared.
        remove_lib_files_recursive(temp_extract_path1)
        remove_lib_files_recursive(temp_extract_path2)

        return compare_dir_recursive(temp_extract_path1, temp_extract_path2)

def create_zip_file(source_dir, zip_file):
    with zipfile.ZipFile(zip_file, 'w', compression=zipfile.ZIP_DEFLATED, compresslevel=zlib.Z_BEST_COMPRESSION) as zipf:
        for root, dirs, files in os.walk(source_dir):
            for file in files:
                file_path = path_join(root, file)
                arcname = os.path.relpath(file_path, start=source_dir)
                zipf.write(file_path, arcname)

def compare_tar_gz_files(tar_gz_file1, tar_gz_file2) -> bool:
    # Does a binary comparison of the contents of the two tar.gz files.
    # Ignores file creation time.
    with tempfile.TemporaryDirectory() as temp_extract_dir:
        temp_extract_path1 = os.path.join(temp_extract_dir, 'temp1')
        temp_extract_path2 = os.path.join(temp_extract_dir, 'temp2')
        os.makedirs(temp_extract_path1, exist_ok=True)
        os.makedirs(temp_extract_path2, exist_ok=True)

        with tarfile.open(tar_gz_file1, 'r:gz') as tar_ref:
            tar_ref.extractall(temp_extract_path1)

        with tarfile.open(tar_gz_file2, 'r:gz') as tar_ref:
            tar_ref.extractall(temp_extract_path2)

        # Remove in both temp directory all library files.
        # Only the remaining source files will be compared.
        remove_lib_files_recursive(temp_extract_path1)
        remove_lib_files_recursive(temp_extract_path2)

        # Because of subtle difference between autotools versions,
        # ignore also a few of its generated files. They
        # just "do not matter" as much.
        #
        # Source file differences are what matter the most
        # to trig re-packaging.
        for root, dirs, files in os.walk(temp_extract_path1):
            for file in files:
                if file == 'Makefile.in' or file == 'configure' or file == 'ltmain.sh':
                    os.remove(os.path.join(root, file))

        for root, dirs, files in os.walk(temp_extract_path2):
            for file in files:
                if file == 'Makefile.in' or file == 'configure' or file == 'ltmain.sh':
                    os.remove(os.path.join(root, file))

        return compare_dir_recursive(temp_extract_path1, temp_extract_path2)

def create_tar_gz_file(source_dir, tar_gz_file):
    with tarfile.open(tar_gz_file, 'w:gz') as tarf:
        for root, dirs, files in os.walk(source_dir):
            for file in files:
                file_path = os.path.join(root, file)
                arcname = os.path.relpath(file_path, start=source_dir)
                tarf.add(file_path, arcname)


def compare_deb_files(deb_file1, deb_file2) -> bool:
    # Does a binary comparison of the contents of the two .deb files.
    # Ignores file creation time and binaries.
    with tempfile.TemporaryDirectory() as temp_extract_dir:

        temp_extract_path1 = os.path.join(temp_extract_dir, 'temp1')
        temp_extract_path2 = os.path.join(temp_extract_dir, 'temp2')
        os.makedirs(temp_extract_path1, exist_ok=True)
        os.makedirs(temp_extract_path2, exist_ok=True)

        try:
            subprocess.run(['dpkg-deb', '-x', deb_file1, temp_extract_path1], check=True)
            subprocess.run(['dpkg-deb', '-x', deb_file2, temp_extract_path2], check=True)
        except subprocess.CalledProcessError as e:
            print(f"Error extracting .deb files: {e}")
            return False

        # Remove in both temp directory all library files.
        # Only the remaining source files will be compared.
        remove_lib_files_recursive(temp_extract_path1)
        remove_lib_files_recursive(temp_extract_path2)

        return compare_dir_recursive(temp_extract_path1, temp_extract_path2)

def compare_msi_files(msi_file1, msi_file2) -> bool:
    # Does a binary comparison of the contents of the two .msi files.
    # Ignores file creation time and binaries
    with tempfile.TemporaryDirectory() as temp_extract_dir:
        temp_extract_path1 = path_join(temp_extract_dir, 'temp1')
        temp_extract_path2 = path_join(temp_extract_dir, 'temp2')
        os.makedirs(temp_extract_path1, exist_ok=True)
        os.makedirs(temp_extract_path2, exist_ok=True)
        os.system(f"msiexec /a {msi_file1} /qn TARGETDIR={temp_extract_path1}")
        os.system(f"msiexec /a {msi_file2} /qn TARGETDIR={temp_extract_path2}")

        # Remove in both temp directory all library files.
        # Only the remaining source files will be compared
        remove_lib_files_recursive(temp_extract_path1)
        remove_lib_files_recursive(temp_extract_path2)

        return compare_dir_recursive(temp_extract_path1, temp_extract_path2)

def create_rtf_from_txt(input_file: str, output_file: str):
    """
    Converts a plain text file to an RTF file.

    Args:
        input_file (str): Path to the input plain text file.
        output_file (str): Path to the output RTF file.
    """
    # Create the RTF header
    rtf_header = '{\\rtf1\\ansi\\deff0\n'
    rtf_footer = '}'

    try:
        with open(input_file, 'r') as txt_file, open(output_file, 'w') as rtf_file:
            # Write the RTF header
            rtf_file.write(rtf_header)

            # Add the content of the plain text file, escaping special characters
            for line in txt_file:
                # Escape backslashes, braces, and newlines
                escaped_line = line.replace('\\', '\\\\').replace('{', '\\{').replace('}', '\\}')
                rtf_file.write(escaped_line + '\\par\n')

            # Write the RTF footer
            rtf_file.write(rtf_footer)

        print(f"RTF file created: {output_file}")
    except Exception as e:
        print(f"Error creating RTF file: {e}")

def force_delete(path: str, sudo_pwd: str = ""):
    # Force delete a file or directory.
    #
    # Try first as normal user. On failure, try again as sudo.
    #
    # 'sudo' is sometimes necessary for files that were created as part
    # of the packaging/installation process.
    #
    # Exit on any error.
    try:
        if is_windows():
            if os.path.isdir(path):
                subprocess.run(['cmd', '/c', 'rmdir', '/s', '/q', path], check=True, stderr=subprocess.DEVNULL)
            else:
                subprocess.run(['cmd', '/c', 'del', '/f', '/q', path], check=True, stderr=subprocess.DEVNULL)
        else:
            subprocess.run(['rm', '-rf', path], check=True, stderr=subprocess.DEVNULL)
    except subprocess.CalledProcessError as e:
        if sudo_pwd and os.path.exists(path) and is_linux():
            run_command_sudo(['rm', '-rf', path], sudo_pwd)

    # Verify that the target is indeed deleted.
    if os.path.exists(path):
        print(f"Error: Failed to delete {path}")
        sys.exit(1)

def force_delete_glob(path: str, pattern: str, sudo_pwd: str = ""):
    # Will exit on any error.
    glob_targets = path_join(path, pattern)
    for target in glob.glob(glob_targets):
        force_delete(target, sudo_pwd)
