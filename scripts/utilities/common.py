import filecmp
import glob
import os
import re
import shutil
import subprocess
import sys
import tempfile
import time

# Various bool functions to help identify the host environment
def is_redhat_based() -> bool:
    return os.path.exists('/etc/redhat-release')

def is_debian_based() -> bool:
    return os.path.exists('/etc/debian_version')

def is_arch_linux() -> bool:
    return os.path.exists('/etc/arch-release')

def is_ubuntu() -> bool:
    if not is_debian_based():
        return False
    try:
        with open('/etc/os-release') as f:
            for line in f:
                if line.startswith('ID=ubuntu'):
                    return True
    except Exception:
        pass
    return False

def is_linux() -> bool:
    return is_debian_based() or is_redhat_based() or is_arch_linux()

def is_macos() -> bool:
    return sys.platform == 'darwin'

def is_windows() -> bool:
    return sys.platform == 'win32'

def is_cmake_installed() -> bool:
    try:
        subprocess.run(['cmake', '--version'], check=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    except subprocess.CalledProcessError:
        return False
    return True

def is_rpmbuild_installed() -> bool:
    if not is_redhat_based():
        return False
    try:
        subprocess.run(['rpmbuild', '--version'], check=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    except subprocess.CalledProcessError:
        return False
    return True

def is_dpkg_installed() -> bool:
    if not is_debian_based():
        return False
    try:
        subprocess.run(['dpkg', '--version'], check=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    except subprocess.CalledProcessError:
        return False
    return True

def is_dotnet_installed() -> bool:
    try:
        subprocess.run(['dotnet', '--version'], check=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        return True
    except (subprocess.CalledProcessError, FileNotFoundError):
        return False

def is_wix_installed() -> bool:
    # For installation, see https://cmake.org/cmake/help/latest/cpack_gen/wix.html#wix-net-tools
    try:
        subprocess.run(['wix', '--version'], check=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        return True
    except (subprocess.CalledProcessError, FileNotFoundError):
        return False

def is_msbuild_installed() -> bool:
    if sys.platform == 'Windows':
        try:
            result = subprocess.run(['vswhere', '-latest', '-products', '*', '-requires', 'Microsoft.Component.MSBuild', '-find', 'MSBuild\\**\\Bin\\MSBuild.exe'], check=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
            msbuild_path = result.stdout.decode().strip()
            if msbuild_path:
                subprocess.run([msbuild_path, '-version'], check=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
                return True
        except (subprocess.CalledProcessError, FileNotFoundError):
            return False
    return False

def is_brew_installed() -> bool:
    try:
        subprocess.run(['brew', '--version'], check=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        return True
    except (subprocess.CalledProcessError, FileNotFoundError):
        return False
    return False

def is_arm64_toolchain_installed() -> bool:
    if is_linux():
        try:
            subprocess.run(['aarch64-linux-gnu-gcc', '--version'], check=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
            return True
        except (subprocess.CalledProcessError, FileNotFoundError):
            return False
    elif is_windows:
        if not is_msbuild_installed():
            return False

    # TODO - Add tool specific detection for Windows/MacOS

    return sys.platform.machine().lower() in ['aarch64', 'arm64']

def is_x86_64_toolchain_installed() -> bool:
    if is_linux():
        try:
            subprocess.run(['x86_64-linux-gnu-gcc', '--version'], check=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
            return True
        except (subprocess.CalledProcessError, FileNotFoundError):
            return False
    elif is_windows:
        if not is_msbuild_installed():
            return False

    # TODO - Add more tool specific detection for Windows/MacOS

    return sys.platform.machine().lower() in ['amd64', 'x86_64']

def is_i386_toolchain_installed() -> bool:
    if is_linux():
        try:
            subprocess.run(['i686-linux-gnu-gcc', '--version'], check=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
            return True
        except (subprocess.CalledProcessError, FileNotFoundError):
            return False

    # TODO - Add tool specific detection for Windows/MacOS
    return sys.platform.machine().lower() in ['i386', 'i686']

# Utility function to list all assets expected for a release.
def get_release_assets(version:str) -> list:
    """
    Return the list of assets expected for a release.

    This is used for CI.

    TA-Lib maintainers should modify this list everytime an assets is added/removed
    """
    return [
        f'ta-lib-{version}-src.tar.gz',
        f'ta-lib-{version}-windows-x86_64.msi',
        f'ta-lib-{version}-windows-x86_64.zip',
        f'ta-lib-{version}-windows-x86_32.msi',
        f'ta-lib-{version}-windows-x86_32.zip',
        f'ta-lib_{version}_amd64.deb',
        f'ta-lib_{version}_arm64.deb',
        f'ta-lib_{version}_i386.deb',
    ]

# Utility functions to identify the gen_code generated files.
def get_src_generated_files() -> list:
    """
    Return the list of generated files and directories.

    This is only for what is expected in the src.tar.gz package.

    Everything under a directory ('**') and file glob allowed ('*')

    See get_all_generated_files() for more...

    This is used for CI.

    TA-Lib maintainers should update this list everytime a new file is generated (or not).
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

    This is used for CI.

    TA-Lib maintainers should update this list everytime a new file is generated (or not).
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


def run_command(command: list) -> str:
    """Run a shell command and return the output."""
    try:
        result = subprocess.run(command, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
        if result.returncode != 0:
            print(f"stdout for '{' '.join(command)}': {result.stdout}")
            print(f"stderr for '{' '.join(command)}': {result.stderr}")
            # If result.stderr contains the string "CPack Error: Problem running WiX.", then
            # print the content of the log file at the specified path.
            if "CPack Error: Problem running WiX." in result.stderr:
                log_file_match = re.search(r"'([^']+)'", result.stderr)
                if log_file_match:
                    log_file = log_file_match.group(1)
                    if os.path.exists(log_file):
                        print(f"Contents of {log_file}:")
                        with open(log_file, 'r') as f:
                            print(f.read())
            sys.exit(1)
    except subprocess.CalledProcessError as e:
        print(f"Error during '{' '.join(command)}': {e}")
        sys.exit(1)

    return result.stdout.strip()

def run_command_term(command: list):
    """
    Similar to run_command, but let it print its output to the terminal instead of capturing.
    Also, exit if the command fails.
    """
    try:
        subprocess.run(command, check=True)
    except subprocess.CalledProcessError as e:
        print(f"Error during '{' '.join(command)}': {e}")
        sys.exit(1)

def run_command_sudo(command: list, sudo_pwd: str=''):
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
            return stdout.decode().strip()
        else:
            return run_command(['sudo'] + command)
    except subprocess.CalledProcessError as e:
        print(f"Error during 'sudo {' '.join(command)}': {e}")
        sys.exit(1)
    except Exception as e:
        print(f"Unexpected error during 'sudo {' '.join(command)}': {e}")
        sys.exit(1)

    return f"Unexpected error: {' '.join(command)}"


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
        print("Current directory is not a complete TA-Lib Git repository (src/ta_func missing)")
        sys.exit(1)

def verify_git_repo_original() -> str:
    # Similar to verify_git_repo, but checks additionally if running from
    # the original repos (will return false if a fork).
    #
    # Some operations (e.g. publishing the official TA-Lib on brew) should
    # be attempted to be done exclusively with the original repos.
    #
    root_dir = verify_git_repo()

    try:
        # Get the remote URL of the repository
        result = subprocess.run(
            ["git", "config", "--get", "remote.origin.url"],
            capture_output=True,
            text=True,
            check=True
        )
        remote_url = result.stdout.strip()

        # Normalize the URL to a common format (account for ssh and https differences).
        normalized_url = re.sub(r'^git@github\.com:', 'https://github.com/', remote_url)
        normalized_url = re.sub(r'\.git$', '', normalized_url).lower()

        # print(f"Remote URL: {normalized_url}")
        # Check if the normalized URL matches the original ta-lib repository URL
        if normalized_url.endswith("ta-lib/ta-lib"):
            return root_dir

    except subprocess.CalledProcessError:
        print("Error: Could not determine the remote URL of the repository.")
        sys.exit(1)

    print("Warning: some processing were skipped because running from a fork")
    sys.exit(0)

def get_git_bot_user_name() -> str:
    return "github-actions[bot]"

def get_git_user_name() -> str:
    if os.getenv('GITHUB_ACTIONS') == 'true':
        return get_git_bot_user_name()

    try:
        result = subprocess.run(['git', 'config', 'user.name'], capture_output=True, text=True, check=True)
        user_name = result.stdout.strip()
        if not user_name:
            user_name = "local"
        return user_name
    except subprocess.CalledProcessError as e:
        return "local"

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

