import os
import shutil
import subprocess
import sys
import tempfile
import time

def create_temp_dir(root_dir) -> str:
    # Create a temporary directory under root_dir/temp, also purge older ones.
    #
    # Return the path of the newly created directory.

    # Delete oldest directories if more than 10 exists and it is more
    # than 1 hour old.
    temp_root_dir = os.path.join(root_dir, "temp")
    os.makedirs(temp_root_dir, exist_ok=True)
    temp_dirs = sorted(os.listdir(temp_root_dir), key=lambda x: os.path.getctime(os.path.join(temp_root_dir, x)))
    if len(temp_dirs) > 10:
        for i in range(len(temp_dirs) - 10):
            temp_dir_path = os.path.join(temp_root_dir, temp_dirs[i])
            if os.path.isdir(temp_dir_path) and (time.time() - os.path.getctime(temp_dir_path)) > 3600:
                shutil.rmtree(temp_dir_path)

    # Create the new temp directory
    return tempfile.mkdtemp(dir=temp_root_dir)

def verify_git_repo() -> str:
    # Verify that the script is called from within a ta-lib git repos, and if yes
    # change the working directory to the root of it.
    #
    # That root path is returned.
    try:
        subprocess.run(['git', '--version'], check=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    except subprocess.CalledProcessError:
        print("Git is not installed. Please install Git and try again.")
        sys.exit(1)

    error = False
    try:
        result = subprocess.run(['git', 'rev-parse', '--is-inside-work-tree'], check=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        if result.stdout.strip() == b'true':
            # Change to the root directory of the Git repository
            root_dir = subprocess.run(['git', 'rev-parse', '--show-toplevel'], check=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE).stdout.strip().decode('utf-8')
            os.chdir(root_dir)
            return root_dir
        else:
            error = True
    except subprocess.CalledProcessError:
        error = True

    if error:
        print("Must run this script while the current directory is in a TA-Lib Git repository.")
        sys.exit(1)

    # Sanity check that src/ta_func exists.
    if not os.path.isdir('src/ta_func'):
        print("Current directory is not a TA-Lib Git repository (src/ta_func missing)")
        sys.exit(1)

def verify_src_package(root_dir: str) -> bool:
    # Returns True on success.

    # In current directory do:
    # - Run ./configure
    # - Run 'make' (verify returning zero)
    # - Run its src/tools/ta_regtest/ta_regtest (verify returning zero)
    # - Verify that src/tools/gen_code/gen_code exists (do not run it).
    #
    # If the host is not a Github Action also do:
    #   - Run 'sudo make install' (verify returning zero)

    original_dir = os.getcwd()
    os.chdir(root_dir)

    try:
        # Simulate typical user installation.
        subprocess.run(['./configure', '--prefix=/usr'], check=True)
        subprocess.run(['make'], check=True)

        # Run its src/tools/ta_regtest/ta_regtest
        subprocess.run(['src/tools/ta_regtest/ta_regtest'], check=True)

        # Verify that src/tools/gen_code/gen_code exists (do not run it)
        if not os.path.isfile('src/tools/gen_code/gen_code'):
            print("Error: src/tools/gen_code/gen_code does not exist.")
            return False

        # run 'sudo make install', if not executed within a Github Action.
        if 'GITHUB_ACTIONS' not in os.environ:
            subprocess.run(['sudo', 'make', 'install'], check=True)
        else:
            print("Skipping 'sudo make install' as this is a Github Action.")

    except subprocess.CalledProcessError as e:
        print(f"Error during verification: {e}")
        return False

    finally:
        os.chdir(original_dir)

    return True

