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
#            lib/ta-lib.dll        (dynamic library)
#            lib/ta-lib.lib        (import library)
#            lib/ta-lib-static.lib (static library)
#            include/*.h           (API headers)
#            VERSION.txt           Version number (e.g. "0.6.0")
#
# How to run it?
#   Do './scripts/package.py' while current directory is the root of the ta-lib repository.
#
#   Windows Specific:
#    - You must have Visual Studio installed (free community version works).
#    - Host machine must be x64
#    - Scripts must be run in a "VS Development Command Shell" (for having
#      CMake and Ninja be on the Path).
#
#    (FYI, all this can optionally be done in a Windows VM)
#
# How to change the version?
#   Edit MAJOR, MINOR, BUILD in src/ta_common/ta_version.c
#   There is no need to modify other files (they will be updated by this script).

import argparse
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

from utilities.versions import sync_versions
from utilities.common import verify_git_repo, test_autotool_src, run_command_sudo
from utilities.files import compare_tar_gz_files, compare_zip_files, create_zip_file

def package_windows(root_dir: str, version: str):
    os.chdir(root_dir)

    # Delete previous dist packaging
    asset_file_name = f'ta-lib-{version}-win64.zip'
    dist_dir = os.path.join(root_dir, 'dist')
    glob_all_packages = os.path.join(dist_dir, 'ta-lib-*-win64.zip')
    for file in glob.glob(glob_all_packages):
        if not file.endswith(asset_file_name):
            os.remove(file)

    temp_dir = os.path.join(root_dir, 'temp')
    glob_all_temp_packages = os.path.join(temp_dir, 'ta-lib-*-win64.zip')
    for file in glob.glob(glob_all_temp_packages ):
        os.remove(file)

    build_dir = os.path.join(root_dir, 'build')
    if os.path.exists(build_dir):
        shutil.rmtree(build_dir)

    glob_all_temp_packages = os.path.join(temp_dir, 'ta-lib-*-win64')
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
    os.makedirs(temp_dir, exist_ok=True)
    package_temp_dir = os.path.join(temp_dir, f'ta-lib-{version}-win64')
    os.makedirs(package_temp_dir, exist_ok=True)
    os.makedirs(os.path.join(package_temp_dir, 'lib'), exist_ok=True)
    os.makedirs(os.path.join(package_temp_dir, 'include'), exist_ok=True)

    # Copy the built files to the temporary package directory
    try:
        shutil.copy('ta-lib.dll', os.path.join(package_temp_dir, 'lib'))
        shutil.copy('ta-lib.lib', os.path.join(package_temp_dir, 'lib'))
        shutil.copy('ta-lib-static.lib', os.path.join(package_temp_dir, 'lib'))
        for header in glob.glob(os.path.join(root_dir, 'include', '*.h')):
            shutil.copy(header, os.path.join(package_temp_dir, 'include'))
        # Create the VERSION.txt file
        with open(os.path.join(package_temp_dir, 'VERSION.txt'), 'w') as f:
            f.write(version)
    except subprocess.CalledProcessError as e:
        print(f"Error copying files: {e}")
        return

    # Compress the package (.zip)
    package_temp_file = os.path.join(temp_dir, asset_file_name)
    try:
        create_zip_file(package_temp_dir, package_temp_file)
    except subprocess.CalledProcessError as e:
        print(f"Error creating zip file: {e}")
        return

    # TODO Add testing of the package here.

    # Copy the zip into dist, but only if its content is binary different.
    os.makedirs(dist_dir, exist_ok=True)
    package_final = os.path.join(dist_dir, asset_file_name)
    if not os.path.exists(package_final) or not compare_zip_files(package_temp_file, package_final):
        print(f"Copying package to dist\\{asset_file_name}")
        shutil.copy(package_temp_file, package_final)
    else:
        print(f"Generated {asset_file_name} identical to one already in dist directory. Skipping copy.")

    # Create the msi file
    #try:
    #    subprocess.run(['cpack', '-G', generator, '-C', 'Release'], cwd=build_dir, check=True)
    #except:
    #    print(f"Error running CPack: {e}")
    print(f"Packaging completed successfully.")

def package_linux(root_dir: str, version: str, sudo_pwd: str):
    os.chdir(root_dir)

    # Delete previous dist packaging
    asset_file_name = f'ta-lib-{version}-src.tar.gz'
    dist_dir = os.path.join(root_dir, 'dist')
    glob_all_packages = os.path.join(dist_dir, 'ta-lib-*-src.tar.gz')
    for file in glob.glob(glob_all_packages):
        if not file.endswith(asset_file_name):
            os.remove(file)

    temp_dir = os.path.join(root_dir, 'temp')
    package_temp_file_prefix = "ta-lib-git"
    package_temp_dir = os.path.join(temp_dir, package_temp_file_prefix)
    package_temp_file_src = os.path.join(root_dir, f"{package_temp_file_prefix}.tar.gz")
    package_temp_file_dest = os.path.join(temp_dir, f"{package_temp_file_prefix}.tar.gz")

    os.system(f"rm -f {package_temp_file_src}")
    os.system(f"rm -f {package_temp_file_dest}")

    try:
        subprocess.run(['rm', '-rf', package_temp_dir], check=True, stderr=subprocess.DEVNULL)
    except subprocess.CalledProcessError as e:
        run_command_sudo(['rm', '-rf', package_temp_dir], sudo_pwd)

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

    # Move ta-lib-git.tar.gz into temp directory.
    if not os.path.isfile(package_temp_file_src):
        print(f"Error: {package_temp_file_src} not found.")
        return
    os.makedirs(temp_dir, exist_ok=True)
    os.rename(package_temp_file_src, package_temp_file_dest)

    # From this point, simulate the "end-user" dev experience
    # who just downloaded the source package.
    os.chdir(temp_dir)
    os.system(f"tar -xzf {package_temp_file_dest}")

    # Verify the source package is OK for autotools users.
    if not test_autotool_src(package_temp_dir, sudo_pwd):
        print("Error: Source package verification failed.")
        return

    # Move ta-lib-git.tar.gz into root_dir/dist (create directory as needed)
    # at same time rename it ta-lib-<version>-src.tar.gz
    # ...
    # but do this only if the archive *content* has changed
    # (ignore metadata such as file creation time).
    package_final = os.path.join(dist_dir, asset_file_name)
    if not os.path.exists(package_final) or not compare_tar_gz_files(package_temp_file_dest, package_final):
        print(f"Moving tested package to dist/{asset_file_name}")
        os.makedirs(dist_dir, exist_ok=True)
        os.rename(package_temp_file_dest, package_final)
    else:
        print(f"Generated {asset_file_name} identical to one already in dist directory. Skipping copy.")

    # Some clean-up (but keep the untarred directory for debugging)
    os.system(f"rm -f {package_temp_file_src}")
    os.system(f"rm -f {package_temp_file_dest}")

    print(f"Packaging completed successfully.")

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Test release candidate assets in 'dist'")
    parser.add_argument('-p', '--pwd', type=str, default="", help="Optional password for sudo commands")
    args = parser.parse_args()

    sudo_pwd = args.pwd
    root_dir = verify_git_repo()
    version = sync_versions(root_dir)
    host_platform = sys.platform
    if host_platform == "linux":
        package_linux(root_dir,version,sudo_pwd)
    elif host_platform == "win32":
        arch = platform.architecture()[0]
        if arch == '64bit':
            package_windows(root_dir, version)
        else:
            print( f"Architecture [{arch}] not yet supported. Only 64 bits supported on windows.")
    else:
        print(f"Unsupported platform [{host_platform}]")
        sys.exit(1)
