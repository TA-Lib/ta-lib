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
from utilities.common import are_generated_files_git_changed, compare_dir, copy_file_list, create_temp_dir, force_delete, get_src_generated_files, is_cmake_installed, is_debian_based, is_redhat_based, is_rpmbuild_installed, is_ubuntu, verify_git_repo, run_command_sudo
from utilities.files import compare_tar_gz_files, compare_zip_files, create_zip_file, compare_deb_files

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

def package_deb(root_dir: str, version: str, sudo_pwd: str) -> dict:
    # Create .deb packaging to be installed with apt or dpkg (Debian-based systems).
    #
    # TA-Lib will install under '/usr/lib' and '/usr/include/ta-lib'.
    #
    # The asset is created/updated into the 'dist' directory only when it
    # pass some tests.
    #
    result: dict = {"success": False}
    dist_dir = os.path.join(root_dir, 'dist')

    # Check dependencies.
    if not is_debian_based():
        print("Error: Debian-based system required for .deb packaging.")
        return result

    if not is_cmake_installed():
        print("Error: CMake not found. It is required to be install for .deb creation")
        return result

    cmake_lists = os.path.join(root_dir, 'CMakeLists.txt')
    if not os.path.isfile(cmake_lists):
        print(f"Error: {cmake_lists} not found. Make sure you are working within a TA-Lib repos")
        return result

    # Delete previous dist packaging
    glob_all_packages = os.path.join(dist_dir, '*.deb')
    for file in glob.glob(glob_all_packages):
        if version not in file:
            force_delete(file)

    # Create an empty CMake build directory
    build_dir = os.path.join(root_dir, 'build')
    force_delete(build_dir, sudo_pwd)
    os.makedirs(build_dir)
    os.chdir(build_dir)

    # Run CMake configuration.
    # Display the output, but also grep the output for the line that has
    # CPACK_PACKAGE_FILE_NAME.
    try:
        subprocess.run(['cmake', '-DCPACK_GENERATOR=DEB', '-DBUILD_DEV_TOOLS=OFF', '..'], check=True)
    except subprocess.CalledProcessError as e:
        print(f"Error running CMake: {e}")
        return result

    # Run CPack to create the .deb package
    try:
        subprocess.run(['cpack', '.'], check=True)
    except subprocess.CalledProcessError as e:
        print(f"Error running CPack: {e}")
        return result

    # Get the asset file name (from the only .deb file expected in the build directory)
    glob_deb = os.path.join(build_dir, '*.deb')
    deb_files = glob.glob(glob_deb)
    if len(deb_files) != 1:
        print(f"Error: Expected one .deb file, found {len(deb_files)}")
        return result

    # Check that the single .deb file has the version as substring
    deb_file = deb_files[0]
    if version not in deb_file:
        print(f"Error: Expected version {version} in {deb_file}")
        return result

    asset_file_name = os.path.basename(deb_file)

    # Sanity check that asset_file_name is correct.
    test_file_name = os.path.join(build_dir, asset_file_name)
    if not os.path.exists(test_file_name):
        print(f"Error: {test_file_name} not found.")
        return result

    # TODO Add here some "end-user installation" testing.

    # Copy the .deb file into dist, but only if it is binary different
    # The creation date will be ignored.
    dist_file = os.path.join(dist_dir, asset_file_name)
    package_existed = os.path.exists(dist_file)
    package_copied = False
    if not package_existed or not compare_deb_files(deb_file, dist_file):
        os.makedirs(dist_dir, exist_ok=True)
        os.rename(deb_file, dist_file)
        package_copied = True

    result["success"] = True
    result["asset_file_name"] = asset_file_name
    result["existed"] = package_existed
    result["copied"] = package_copied
    return result

def package_rpm(root_dir: str, version: str, sudo_pwd: str) -> dict:
    result: dict = {"success": False}
    # Not implemented yet
    return result

def package_src_tar_gz(root_dir: str, version: str, sudo_pwd: str) -> dict:
    # The src.tar.gz is for users wanting to build from source with autotools (./configure).
    #
    # The asset is created/updated into the 'dist' directory only when it
    # pass some tests.
    #
    result: dict = {"success": False}

    asset_file_name = f"ta-lib-{version}-src.tar.gz"
    result["asset_file_name"] = asset_file_name

    dist_dir = os.path.join(root_dir, 'dist')

    # Delete previous dist packaging
    glob_all_packages = os.path.join(dist_dir, '*-src.tar.gz')
    for file in glob.glob(glob_all_packages):
        if not file.endswith(asset_file_name):
            force_delete(file)

    temp_dir = os.path.join(root_dir, 'temp')
    package_temp_file_prefix = "ta-lib-git"
    package_temp_dir = os.path.join(temp_dir, package_temp_file_prefix)
    package_temp_file_src = os.path.join(root_dir, f"{package_temp_file_prefix}.tar.gz")
    package_temp_file_dest = os.path.join(temp_dir, f"{package_temp_file_prefix}.tar.gz")

    force_delete(package_temp_file_src, sudo_pwd)
    force_delete(package_temp_file_dest, sudo_pwd)
    force_delete(package_temp_dir, sudo_pwd)

    # Always autoreconf before re-packaging
    try:
        subprocess.run(['autoreconf', '-fi'], check=True)
    except subprocess.CalledProcessError as e:
        print(f"Error running 'autoreconf -fi': {e}")
        return result

    # Run ./configure
    try:
        subprocess.run(['./configure'], check=True)
    except subprocess.CalledProcessError as e:
        print(f"Error running './configure': {e}")
        return result

    # Run make dist
    try:
        subprocess.run(['make', 'dist'], check=True)
    except subprocess.CalledProcessError as e:
        print(f"Error running 'make dist': {e}")
        return result

    # Move ta-lib-git.tar.gz into temp directory.
    if not os.path.isfile(package_temp_file_src):
        print(f"Error: {package_temp_file_src} not found.")
        return result

    os.makedirs(temp_dir, exist_ok=True)
    os.rename(package_temp_file_src, package_temp_file_dest)

    # From this point, simulate the "end-user" dev experience
    # as if the source package was freshly downloaded.
    os.chdir(temp_dir)
    os.system(f"tar -xzf {package_temp_file_dest}")
    if not test_autotool_src(package_temp_dir, sudo_pwd):
        print("Error: Source package verification failed.")
        return result

    # Move ta-lib-git.tar.gz into root_dir/dist (create directory as needed)
    # at same time rename it ta-lib-<version>-src.tar.gz
    # ...
    # but do this only if the archive *content* has changed
    # (ignore metadata such as file creation time).
    dist_file = os.path.join(dist_dir, asset_file_name)
    package_existed = os.path.exists(dist_file)
    package_copied = False
    if not package_existed or not compare_tar_gz_files(package_temp_file_dest, dist_file):
        os.makedirs(dist_dir, exist_ok=True)
        os.rename(package_temp_file_dest, dist_file)
        package_copied = True

    # Some clean-up (but keep the untarred directory for debugging)
    force_delete(package_temp_file_src, sudo_pwd)
    force_delete(package_temp_file_dest, sudo_pwd)

    result["success"] = True
    result["existed"] = package_existed
    result["copied"] = package_copied
    return result


def display_package_results(results: dict):
    # Display the results returned by most packaging functions.
    asset_built = results.get("built", False)
    asset_built_success = results.get("success", False)
    asset_file_name = results.get("asset_file_name", "unknown")
    asset_copied = results.get("copied", False)
    asset_existed = results.get("existed", False)
    if asset_built:
        if not asset_built_success:
            print(f"Error: Packaging dist/{asset_file_name} failed.")

        if asset_copied:
            if asset_existed:
                print(f"Updated dist/{asset_file_name}")
            else:
                print(f"Created dist/{asset_file_name}")
        else:
            print(f"No changes for dist/{asset_file_name}")
    else:
        print(f"{asset_file_name} build skipped (not supported on this platform)")


def package_linux(root_dir: str, version: str, sudo_pwd: str):
    os.chdir(root_dir)

    # The dist/ta-lib-*-src.tar.gz are created only on Ubuntu
    # but are tested with other Linux distributions.
    src_tar_gz_results = {
        "success": False,
        "built": False,
        "asset_file_name": "src.tar.gz", # Default, will change.
    }

    if is_ubuntu():
        results = package_src_tar_gz(root_dir, version, sudo_pwd)
        src_tar_gz_results.update(results)
        src_tar_gz_results["built"] = True
        if not src_tar_gz_results["success"]:
            print(f'Error: Packaging dist/{src_tar_gz_results["asset_file_name"]} failed.')
            sys.exit(1)

    # When supported by host, build RPM using CMakeLists.txt (CPack)
    rpm_results = {
        "success": False,
        "built": False,
        "asset_file_name": ".rpm", # Default, will change.
    }
    if is_redhat_based():
        if not is_rpmbuild_installed():
            print("Error: rpmbuild not found. RPM not created")
            sys.exit(1)
        results = package_deb(root_dir, version, sudo_pwd)
        rpm_results.update(results)
        rpm_results["built"] = True
        if not rpm_results["success"]:
            print(f'Error: Packaging dist/{rpm_results["asset_file_name"]} failed.')
            sys.exit(1)

    # When supported by host, build DEB using CMakeLists.txt (CPack)
    deb_results = {
        "success": False,
        "built": False,
        "asset_file_name": ".deb", # Default, will change.
    }
    if is_debian_based():
        results = package_deb(root_dir, version, sudo_pwd)
        deb_results.update(results)
        deb_results["built"] = True
        if not deb_results["success"]:
            print(f'Error: Packaging dist/{deb_results["asset_file_name"]} failed.')
            sys.exit(1)

    # A summary of everything that was done
    print(f"\n***********")
    print(f"* Summary *")
    print(f"***********")
    display_package_results(src_tar_gz_results)
    display_package_results(rpm_results)
    display_package_results(deb_results)

    print(f"\nPackaging completed successfully.")


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

    except subprocess.CalledProcessError as e:
        print(f"Error during verification: {e}")
        return False

    finally:
        os.chdir(original_dir)

    return True

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
