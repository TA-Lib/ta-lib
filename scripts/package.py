#!/usr/bin/env python3

# Create/update the ta-lib-<version>-src.tar.gz for unix systems.

# This package is for users wanting to rebuild on their machine
# with the typical "./configure; make"

import os
import subprocess
import sys
import glob

from common import verify_git_repo, get_version_string, verify_src_package

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

    if sys.platform == "linux":
        root_dir = verify_git_repo()
        version = get_version_string(root_dir)
        package_linux(root_dir,version)
    else:
        print("For now, this script is only for Linux systems.")
        sys.exit(1)
