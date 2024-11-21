#!/usr/bin/env python3

# Produces the assets release candidates in 'dist'.
#
# The outputs depend of the host system.
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
#    (FYI, all this can optionally be done in a Windows VM)
#
# How to change the version?
#   Edit MAJOR, MINOR, BUILD in src/ta_common/ta_version.c
#   There is no need to modify other files (they will be updated by this script).

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

from utilities.common import verify_git_repo, get_version_string, verify_src_package
from utilities.files import compare_tar_gz_files, compare_zip_files, create_zip_file

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
    dist_dir = os.path.join(root_dir, 'dist')
    package_temp_file_prefix = "ta-lib-git"
    package_temp_dir = os.path.join(root_dir, package_temp_file_prefix)
    package_temp_file = os.path.join(root_dir, f"{package_temp_file_prefix}.tar.gz")

    #for file in glob.glob(f'dist/ta-lib-*-src.tar.gz'):
    #    os.remove(file)
    os.system(f"rm -rf {package_temp_dir}")
    os.system(f"rm -f {package_temp_file}")

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

    if not os.path.isfile(package_temp_file):
        print(f"Error: {package_temp_file} not found.")
        return

    os.system(f"tar -xzf {package_temp_file}")

    # Verify the source package is OK.

    if not verify_src_package(package_temp_dir):
        print("Error: Source package verification failed.")
        return

    # Move ta-lib-git.tar.gz into root_dir/dist (create directory as needed)
    # at same time rename it ta-lib-<version>-src.tar.gz
    # ...
    # but do this only if the archive *content* has changed
    # (ignore metadata such as file creation time).
    asset_file_name = f'ta-lib-{version}-src.tar.gz'
    package_final = os.path.join(dist_dir, asset_file_name)
    if not os.path.exists(package_final) or not compare_tar_gz_files(package_temp_file, package_final):
        print(f"Copying package to dist/{asset_file_name}")
        os.makedirs(dist_dir, exist_ok=True)
        os.rename(package_temp_file, package_final)
    else:
        print(f"Generated {asset_file_name} identical to one already in dist directory. Skipping copy.")

    # Clean-up
    os.system(f"rm -rf {package_temp_dir}")
    os.system(f"rm -f {package_temp_file}")

    print(f"Packaging completed successfully.")

if __name__ == "__main__":
    root_dir = verify_git_repo()
    version = get_version_string(root_dir)
    host_platform = sys.platform
    if host_platform == "linux":
        package_linux(root_dir,version)
    elif host_platform == "win32":
        arch = platform.architecture()[0]
        if arch == '64bit':
            package_windows(root_dir, version)
        else:
            print( f"Architecture [{arch}] not yet supported. Only 64 bits supported on windows.")
    else:
        print(f"Unsupported platform [{host_platform}]")
        sys.exit(1)
