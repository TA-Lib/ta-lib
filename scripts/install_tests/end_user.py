import glob
import os
import subprocess
import sys
import tarfile
import zipfile

from install_tests.consumer import consumer_linux, consumer_windows, run_or_die

# End-user simulation: install the release-candidate C package the documented
# way, then build a C program against the installed files.
#
# Any problem => sys.exit(1). Intermediate files stay in temp_dir for
# debugging.


def log_params(host: str, package_file_path: str, temp_dir: str, version: str, sudo_pwd: str):
    print(f"Testing the installed package on {host}")
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


def test_zip_windows(root_dir: str, package_file_path: str, temp_dir: str, version: str, sudo_pwd: str):
    # package_file_path is the ta-lib-{version}-windows-x86_64.zip; a user
    # extracts it anywhere and points the compiler at include/ and lib/.
    log_params("Windows", package_file_path, temp_dir, version, sudo_pwd)

    pkg_dir = os.path.join(temp_dir, "pkg")
    with zipfile.ZipFile(package_file_path) as zf:
        zf.extractall(pkg_dir)

    include_dir = os.path.join(pkg_dir, "include")
    if not glob.glob(os.path.join(include_dir, "ta-lib", "*.h")):
        print("FAIL: the zip has no include/ta-lib/*.h")
        sys.exit(1)

    consumer_windows(root_dir, temp_dir, version, "zip",
                     include_dir=include_dir,
                     lib_dir=os.path.join(pkg_dir, "lib"),
                     bin_dir=os.path.join(pkg_dir, "bin"))
    return


def test_tarball_linux(package_file_path: str, temp_dir: str, version: str, sudo_pwd: str):
    # package_file_path is the ta-lib-{version}-src.tar.gz, installed the
    # end-user way: ./configure && make && sudo make install (then ldconfig).
    log_params("Linux", package_file_path, temp_dir, version, sudo_pwd)

    src_dir = os.path.join(temp_dir, "src")
    os.makedirs(src_dir, exist_ok=True)
    with tarfile.open(package_file_path) as tf:
        tf.extractall(src_dir)

    # The tarball extracts into a single ta-lib-* directory.
    entries = [d for d in os.listdir(src_dir) if os.path.isdir(os.path.join(src_dir, d))]
    if len(entries) != 1:
        print(f"FAIL: unexpected tarball layout: {entries}")
        sys.exit(1)
    configure_dir = os.path.join(src_dir, entries[0])

    run_or_die(["./configure"], "configure", cwd=configure_dir)
    run_or_die(["make", f"-j{os.cpu_count() or 2}"], "make", cwd=configure_dir)

    def sudo(cmd_tail, step):
        if sudo_pwd:
            full = ["sudo", "-S"] + cmd_tail
            print(f"  [{step}] sudo -S {' '.join(cmd_tail)}")
            result = subprocess.run(full, cwd=configure_dir, input=sudo_pwd + "\n", text=True)
        else:
            full = ["sudo"] + cmd_tail
            print(f"  [{step}] {' '.join(full)}")
            result = subprocess.run(full, cwd=configure_dir)
        if result.returncode != 0:
            print(f"FAIL: {step} exited with {result.returncode}")
            sys.exit(1)

    sudo(["make", "install"], "sudo make install")
    sudo(["ldconfig"], "ldconfig")

    consumer_linux(temp_dir, version, "make_install",
                   include_dir="/usr/local/include", lib_dir="/usr/local/lib",
                   search_paths=False)
    return
