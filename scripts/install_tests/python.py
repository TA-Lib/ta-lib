import os

def log_params(host: str, package_file_path: str, temp_dir: str, version: str, sudo_pwd: str):
    print(f"Testing ta-lib-python installation on {host}")
    # Never display or log sudo_pwd, but want to know if it was specified.
    if sudo_pwd:
        hidden_sudo_pwd = "(hidden)"
    else:
        hidden_sudo_pwd = "\"\""

    print(f"  package_file_path={package_file_path}")
    print(f"  temp_dir={temp_dir}")
    print(f"  version={version}")
    print(f"  sudo_pwd={hidden_sudo_pwd}")

    # Create a dummy file "PARAMS" into temp_dir to help debugging.
    with open(os.path.join(temp_dir, "PARAMS"), "w") as f:
        f.write(f"package_file_path={package_file_path}\n")
        f.write(f"temp_dir={temp_dir}\n")
        f.write(f"version={version}\n")
        f.write(f"sudo_pwd={hidden_sudo_pwd}\n")

    return

def test_python_windows(package_file_path: str, temp_dir: str, version: str, sudo_pwd: str):
    # Test installation procedure for ta-lib-python and validate
    # that this ta-lib package release candidate is OK.
    #
    # Parameters
    # ----------
    # package_file_path is the ta-lib-{version}-win64.zip
    #
    # temp_dir is an empty directory that can be used
    # for most intermediate file operations.
    #
    # version is the "0.0.0" string expected to be returned
    # by the installed ta-lib package.
    #
    # sudo_pwd can be pipeline into a "sudo -S" command.
    # If sudo_pwd is an empty string, then call "sudo" without
    # "-S" and let it be interactive.
    # More info:
    #    https://stackoverflow.com/questions/60807449/run-github-action-as-sudo
    #
    # Test Behavior
    # -------------
    # Must re-install/upgrade if ta-lib is already installed.
    #
    # Recommended to create a venv in temp_dir to simulate
    # a user setup.
    #
    # Leave all intermediate files in temp_dir after the test
    # (might be useful for debugging).
    #
    # Just sys.exit(1) on any problem discovered.

    log_params("Windows", package_file_path, temp_dir, version, sudo_pwd)

    # TODO - Implement the test !!!

    return

def test_python_linux(package_file_path: str, temp_dir: str, version: str, sudo_pwd: str):
    # Same as test_python_windows except package_file_path is
    # the ta-lib-{version}-src.tar.gz
    #
    # Installation to be done with autotools "./configure"

    log_params("Linux", package_file_path, temp_dir, version, sudo_pwd)

    # TODO - Implement the test !!!

    return
