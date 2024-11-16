#!/usr/bin/env python3

# Create/update the ta-lib-<version>-src.tar.gz for unix systems.

# This package is for users wanting to rebuild on their machine
# with the typical "./configure; make"

import os
import subprocess
import sys

from common import verify_git_repo, get_version_string

def package_linux(root_dir):
    os.chdir(root_dir)

    # Check if ./configure exists, if not create it.
    if not os.path.isfile('./configure'):
        print("'./configure' not found. Running 'autogen -i'...")
        try:
            subprocess.run(['autogen', '-i'], check=True)
        except subprocess.CalledProcessError as e:
            print(f"Error running 'autogen -i': {e}")
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

if __name__ == "__main__":

    if sys.platform == "linux":
        root_dir = verify_git_repo()
        version = get_version_string(root_dir)
        package_linux(root_dir)
    else:
        print("For now, this script is only for Linux systems.")
        sys.exit(1)
