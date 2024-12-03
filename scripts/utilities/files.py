import os
import zipfile
import tempfile
import filecmp
import zlib
import tarfile

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

        dir_comparison = filecmp.dircmp(temp_extract_path1, temp_extract_path2)
        return not dir_comparison.diff_files and not dir_comparison.left_only and not dir_comparison.right_only

def create_tar_gz_file(source_dir, tar_gz_file):
    with tarfile.open(tar_gz_file, 'w:gz') as tarf:
        for root, dirs, files in os.walk(source_dir):
            for file in files:
                file_path = os.path.join(root, file)
                arcname = os.path.relpath(file_path, start=source_dir)
                tarf.add(file_path, arcname)

def compare_deb_files(deb_file1, deb_file2) -> bool:
    # Does a binary comparison of the contents of the two .deb files.
    # Ignores file creation time.
    with tempfile.TemporaryDirectory() as temp_extract_dir:
        temp_extract_path1 = os.path.join(temp_extract_dir, 'temp1')
        temp_extract_path2 = os.path.join(temp_extract_dir, 'temp2')
        os.makedirs(temp_extract_path1, exist_ok=True)
        os.makedirs(temp_extract_path2, exist_ok=True)

        os.system(f"dpkg-deb -x {deb_file1} {temp_extract_path1}")
        os.system(f"dpkg-deb -x {deb_file2} {temp_extract_path2}")

        dir_comparison = filecmp.dircmp(temp_extract_path1, temp_extract_path2)
        return not dir_comparison.diff_files and not dir_comparison.left_only and not dir_comparison.right_only