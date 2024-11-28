import filecmp
import glob
import os
import shutil
import subprocess
import sys
import tempfile
import time

def run_command_sudo(command, sudo_pwd=''):
    """
    Run a command with sudo, optionally using a password if provided.
    Will exit the script if calling the command fails or exit code != 0.
    """
    try:
        if sudo_pwd:
            # Pipeline the password to sudo
            process = subprocess.Popen(['sudo', '-S'] + command, stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
            stdout, stderr = process.communicate(input=f'{sudo_pwd}\n'.encode())
            if process.returncode != 0:
                print(f"Error during 'sudo {' '.join(command)}': {stderr.decode()}")
                sys.exit(1)
        else:
            subprocess.run(['sudo'] + command, check=True)
    except subprocess.CalledProcessError as e:
        print(f"Error during 'sudo {' '.join(command)}': {e}")
        sys.exit(1)
    except Exception as e:
        print(f"Unexpected error during 'sudo {' '.join(command)}': {e}")
        sys.exit(1)


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

def get_src_generated_files() -> list:
    """
    Return the list of generated files and directories.

    This is only for what is expected in the src.tar.gz package.

    Everything under a directory ('**') and file glob allowed ('*')

    See get_all_generated_files() for more...
    """
    return [
        'include/ta_func.h',
        'include/ta_defs.h',
        'src/ta_func/*.c',
        'src/ta_abstract/*.c',
        'src/ta_abstract/frames/*.c',
        'src/ta_abstract/frames/*.h',
        'src/ta_common/ta_retcode.c',
        'src/ta_abstract/ta_java_defs.h',
    ]

def get_all_generated_files() -> list:
    """
    Returns list of all generated files and directories.
    Everything under a directory ('**') and file glob allowed ('*')
    """
    return [
        'swig/src/interface/ta_func.swg',
        'dotnet/src/Core/TA-Lib-Core.vcproj',
        'dotnet/src/Core/TA-Lib-Core.h',
        'ide/msvc/lib_proj/ta_func/ta_func.dsp',
        'java/src/**',
    ]  + get_src_generated_files()

def expand_globs(root_dir: str, file_list: list) -> list:
    """
    Expand glob patterns in the file list to actual file paths.
    """
    expanded_files = []
    for file in file_list:
        # Use recursive globbing if '**' is in the pattern
        if '**' in file:
            expanded_files.extend(glob.glob(os.path.join(root_dir, file), recursive=True))
        else:
            expanded_files.extend(glob.glob(os.path.join(root_dir, file)))
    return expanded_files

def are_generated_files_git_changed(root_dir: str) -> bool:
    # Using git, verify if any of the generated files have changed.
    #
    # root_dir must be the root of the TA-Lib Git repository.
    original_dir = os.getcwd()
    os.chdir(root_dir)

    try:
        result = subprocess.run(['git', 'diff', '--exit-code', 'HEAD', '--'] + get_all_generated_files(), check=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        return result.returncode != 0

    except subprocess.CalledProcessError as e:
        print(f"Error: {e}")
        return False

    finally:
        os.chdir(original_dir)
    return False

def copy_file_list(src_dir: str, dest_dir: str, file_list: list):
    # Copy the files and directory to dest_dir.
    #
    # The file list can include whole directories ('**') and file glob ('*').
    #
    # 'dest_dir' can then be used later to detect changes
    # with compare_dir.

    # Delete all contents in dest_dir, but not dest_dir itself.
    if os.path.exists(dest_dir):
        for root, dirs, files in os.walk(dest_dir):
            for file in files:
                os.remove(os.path.join(root, file))
            for dir in dirs:
                shutil.rmtree(os.path.join(root, dir))

    os.makedirs(dest_dir, exist_ok=True)

    expanded_files = expand_globs(src_dir, file_list)
    for src_file in expanded_files:
        dest_file = os.path.join(dest_dir, os.path.relpath(src_file, src_dir))
        os.makedirs(os.path.dirname(dest_file), exist_ok=True)
        if os.path.isdir(src_file):
            shutil.copytree(src_file, dest_file, dirs_exist_ok=True)
        else:
            shutil.copy(src_file, dest_file)

def compare_dir(dir1: str, dir2: str) -> bool:
    # Detect any difference in files or directory.
    # For files, also does a binary comparison.
    # Recursively check subdirectories.
    dircmp = filecmp.dircmp(dir1, dir2)

    differences_found = False

    if dircmp.left_only:
        print(f"Files only in {dir1}: {dircmp.left_only}")
        differences_found = True

    if dircmp.right_only:
        print(f"Files only in {dir2}: {dircmp.right_only}")
        differences_found = True

    if dircmp.diff_files:
        print(f"Files that differ: {dircmp.diff_files}")
        differences_found = True

    if dircmp.funny_files:
        print(f"Files that could not be compared: {dircmp.funny_files}")
        differences_found = True

    for subdir in dircmp.common_dirs:
        if not compare_dir(os.path.join(dir1, subdir), os.path.join(dir2, subdir)):
            differences_found = True

    return not differences_found

def test_autotool_src(configure_dir: str, sudo_pwd: str) -> bool:
    # Returns True on success.

    # sudo_pwd is optional.
    #
    # configure_dir is the location where the end-user is expected to
    # do './configure'.
    #
    # For this test, configure_dir must be somwhere within the TA-Lib
    # git repos (will typically be under ta-lib/temp).
    #
    # In specified configure_dir do:
    # - './configure'
    # - 'make' (verify returning zero)
    # - Run './src/tools/ta_regtest/ta_regtest' (verify returning zero)
    # - Run './src/tools/gen_code/gen_code' (verify no unexpected changes)
    # - 'sudo make install' (verify returning zero)

    original_dir = os.getcwd()
    root_dir = verify_git_repo()
    generated_files_temp_copy_1 = create_temp_dir(root_dir)
    generated_files_temp_copy_2 = create_temp_dir(root_dir)
    os.chdir(configure_dir)

    try:
        git_changed = are_generated_files_git_changed(root_dir);
        copy_file_list(configure_dir,
                       generated_files_temp_copy_1,
                       get_src_generated_files())

        # Simulate typical user installation.
        subprocess.run(['./configure'], check=True)
        subprocess.run(['make'], check=True)

        # Run its src/tools/ta_regtest/ta_regtest
        subprocess.run(['src/tools/ta_regtest/ta_regtest'], check=True)

        if not os.path.isfile('src/tools/gen_code/gen_code'):
            print("Error: src/tools/gen_code/gen_code does not exist.")
            return False

        # Re-running gen_code should not cause changes to the root directory.
        # (but do nothing if there was already git changes prior to gen_code).
        # This is just a sanity check that the script is not breaking something
        # unexpected outside of the "end-user simulated" directory.
        if not git_changed and are_generated_files_git_changed(root_dir):
            print("Error: Unexpected changes from gen_code to root_dir. Do 'git diff'")
            return False

        # Now verify if gen_code did change files unexpectably within
        # the "end-user simulated" directory.
        #
        # It should not, because we are at the point of testing the src
        # package, which should have the latest generated files version
        # and re-running gen_code should have no effect.
        copy_file_list(configure_dir,
                       generated_files_temp_copy_2,
                       get_src_generated_files())
        if not compare_dir(generated_files_temp_copy_1, generated_files_temp_copy_2):
            return False


        run_command_sudo(['make', 'install'], sudo_pwd)

        # run 'sudo make install', if not executed within a Github Action.
#        if 'GITHUB_ACTIONS' not in os.environ:
#            run_command_sudo(['make', 'install'], sudo_pwd)
#        else:
#            print("Skipping 'sudo make install' as this is a Github Action.")

    except subprocess.CalledProcessError as e:
        print(f"Error during verification: {e}")
        return False

    finally:
        os.chdir(original_dir)

    return True

