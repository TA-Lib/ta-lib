#!/usr/bin/env python3

# Produces and tests the assets to be released.
#
# The output depends of the host system.
#
#    For linux/ubuntu: ta-lib-<version>-src.tar.gz
#         with contents for doing "./configure; make; sudo make install"
#
#    For windows: ta-lib-<version>-win64.zip
#         with contents:
#            lib/ta_lib.dll        (dynamic library)
#            lib/ta_lib.lib        (import library)
#            lib/ta_lib_static.lib (static library)
#            include/*.h           (all necessary headers)
#
# How to run it?
#   Do './scripts/package.py' from root of your ta-lib repository.
#
#   Windows Specific:
#    - You must have Visual Studio installed (free community version works).
#    - Host machine must be x64
#    - Scripts must be run in a "VS Development Command Shell" (for the convenience
#      of CMake and Ninja be on the Path and already be configured).
#
#    (FYI, you can optionally do all this in a Windows VM)
#
# How to change the version?
#   Edit MAJOR, MINOR, BUILD in src/ta_common/ta_version.c
#   You do not need to modify other files (this script will update all needed files).

import filecmp
import os
import subprocess
import sys
import glob
import platform
import shutil
import tempfile
import zipfile
import zlib

from common import verify_git_repo, get_version_string, verify_src_package

def compare_zip_files(zip_file1, zip_file2):
    # Does a binary comparison of the contents of the two zip files.
    # Ignores file creation time.
    with tempfile.TemporaryDirectory() as temp_extract_dir:
        temp_extract_path1 = os.path.join(temp_extract_dir, 'temp1')
        temp_extract_path2 = os.path.join(temp_extract_dir, 'temp2')
        os.makedirs(temp_extract_path1, exist_ok=True)
        os.makedirs(temp_extract_path2, exist_ok=True)
        
        with zipfile.ZipFile(zip_file1, 'r') as zip_ref:
            zip_ref.extractall(temp_extract_path1)
        
        with zipfile.ZipFile(zip_file2, 'r') as zip_ref:
            zip_ref.extractall(temp_extract_path2)
        
        dir_comparison = filecmp.dircmp(temp_extract_path1, temp_extract_path2)
        return not dir_comparison.diff_files and not dir_comparison.left_only and not dir_comparison.right_only

def create_zip_file(source_dir, zip_file):
    with zipfile.ZipFile(zip_file, 'w', compression=zipfile.ZIP_DEFLATED, compresslevel=zlib.Z_BEST_COMPRESSION) as zipf:
        for root, dirs, files in os.walk(source_dir):
            for file in files:
                file_path = os.path.join(root, file)
                arcname = os.path.relpath(file_path, start=source_dir)
                zipf.write(file_path, arcname)

def package_windows(root_dir: str, version: str):
    os.chdir(root_dir)

    # Clean-up any previous packaging
    dist_dir = os.path.join(root_dir, 'dist')
    #glob_all_packages = os.path.join(root_dir, 'dist', 'ta-lib-*-win64.zip')
    #for file in glob.glob(glob_all_packages):
    #    os.remove(file)

    glob_all_temp_packages = os.path.join(root_dir, 'temp', 'ta-lib-*-win64.zip')
    for file in glob.glob(glob_all_temp_packages ):
        os.remove(file)

    build_dir = os.path.join(root_dir, 'build')
    if os.path.exists(build_dir):
        shutil.rmtree(build_dir)

    glob_all_temp_packages = os.path.join(root_dir, 'temp', 'ta-lib-*-win64')
    for directory in glob.glob(glob_all_temp_packages):
        shutil.rmtree(directory)

    # Run CMake and build
    try:
        os.makedirs(build_dir)
        os.chdir(build_dir)
        subprocess.run(['cmake', '-G', 'Ninja', '..'], check=True)
        subprocess.run(['cmake', '--build', '.'], check=True)
    except subprocess.CalledProcessError as e:
        print(f"Error running CMake or build: {e}")
        return

    # Create temporary package dirs (including its subdirs)
    temp_dir = os.path.join(root_dir, 'temp')
    os.makedirs(temp_dir, exist_ok=True)
    package_temp_dir = os.path.join(temp_dir, f'ta-lib-{version}-win64')
    os.makedirs(package_temp_dir, exist_ok=True)
    os.makedirs(os.path.join(package_temp_dir, 'lib'), exist_ok=True)
    os.makedirs(os.path.join(package_temp_dir, 'include'), exist_ok=True)

    # Copy the built files to the temporary package directory
    try:
        shutil.copy('ta_lib.dll', os.path.join(package_temp_dir, 'lib'))
        shutil.copy('ta_lib.lib', os.path.join(package_temp_dir, 'lib'))
        shutil.copy('ta_lib_static.lib', os.path.join(package_temp_dir, 'lib'))
        for header in glob.glob(os.path.join(root_dir, 'include', '*.h')):
            shutil.copy(header, os.path.join(package_temp_dir, 'include'))
    except subprocess.CalledProcessError as e:
        print(f"Error copying files: {e}")
        return

    # Create the zip file
    package_temp_file = os.path.join(temp_dir, f'ta-lib-{version}-win64.zip')
    try:
        # subprocess.run(['powershell', 'Compress-Archive', '-Path', package_temp_dir, '-DestinationPath', package_temp_file], check=True)
        create_zip_file(package_temp_dir, package_temp_file)
    except subprocess.CalledProcessError as e:
        print(f"Error creating zip file: {e}")
        return

    # TODO Add testing of the package here.

    # Copy package_temp_file into dist, but only if its content is binary different.
    os.makedirs(dist_dir, exist_ok=True)
    package_final = os.path.join(dist_dir, f'ta-lib-{version}-win64.zip')
    if not os.path.exists(package_final) or not compare_zip_files(package_temp_file, package_final):
        print(f"Copying package to dist\\ta-lib-{version}-win64.zip")
        shutil.copy(package_temp_file, package_final)
    else:
        print(f"Generated ta-lib-{version}-win64.zip identical to one already in dist directory. Skipping copy.")

    print(f"Packaging completed successfully.")

def package_linux(root_dir: str, version: str):
    os.chdir(root_dir)

    # Clean-up any previous packaging
    for file in glob.glob(f'dist/ta-lib-*-src.tar.gz'):
        os.remove(file)

    os.system('rm -f ta-lib-git.tar.gz')

    try:
        subprocess.run(['rm', '-rf', 'ta-lib-git'], check=True, stderr=subprocess.DEVNULL)
    except subprocess.CalledProcessError as e:
        try:
            subprocess.run(['sudo', 'rm', '-rf', 'ta-lib-git'], check=True)
        except subprocess.CalledProcessError as e:
            print(f"Error running 'sudo rm -rf ta-lib-git': {e}")
            return

    # Always autoreconf before re-packaging
    try:
        subprocess.run(['autoreconf', '-fi'], check=True)
    except subprocess.CalledProcessError as e:
        print(f"Error running 'autoreconf -fi': {e}")
        return

    # Run ./configure
    try:
        subprocess.run(['./configure'], check=True)
    except subprocess.CalledProcessError as e:
        print(f"Error running './configure': {e}")
        return

    # Run make dist
    try:
        subprocess.run(['make', 'dist'], check=True)
    except subprocess.CalledProcessError as e:
        print(f"Error running 'make dist': {e}")
        return

    # Decompress ta-lib-git.tar.gz
    if not os.path.isfile('ta-lib-git.tar.gz'):
        print("Error: ta-lib-git.tar.gz not found.")
        return

    os.system('tar -xzf ta-lib-git.tar.gz')

    # Verify the source package is OK.
    if not verify_src_package(f"{root_dir}/ta-lib-git"):
        print("Error: Source package verification failed.")
        return

    # Move ta-lib-git.tar.gz into root_dir/dist (create directory as needed)
    # at same time rename it ta-lib-<version>-src.tar.gz
    os.makedirs('dist', exist_ok=True)
    os.rename('ta-lib-git.tar.gz', f'dist/ta-lib-{version}-src.tar.gz')
    # delete the ta-lib-git directory
    os.system('rm -rf ta-lib-git')

    print(f"Packaging to dist/ta-lib-{version}.tar.gz successful.")

if __name__ == "__main__":
    root_dir = verify_git_repo()
    version = get_version_string(root_dir)

    if sys.platform == "linux":
        package_linux(root_dir,version)
    elif sys.platform == "win32":
        arch = platform.architecture()[0]
        if arch == '64bit':
            package_windows(root_dir, version)
        else:
            print( f"Architecture [{arch}] not yet supported. Only 64 bits supported on windows.")
    else:
        print("For now, this script is only for Linux systems.")
        sys.exit(1)
