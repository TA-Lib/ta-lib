import os
import subprocess
import sys

from install_tests.consumer import consumer_windows

# MSI end-user simulation (Windows): silent-install the release-candidate
# MSI, verify the installed layout, build a C program against the INSTALLED
# location, then silent-uninstall and verify removal. Any problem =>
# sys.exit(1).
#
# CPACK_PACKAGE_INSTALL_DIRECTORY is "TA-Lib" (CMakeLists.txt), so the
# payload lands under <Program Files>\TA-Lib.

INSTALL_SUBDIRS = ("include", "lib", "bin")


def _msiexec(args, step: str, log_path: str):
    cmd = ["msiexec"] + args + ["/qn", "/norestart", "/l*v", log_path]
    print(f"  [{step}] {' '.join(cmd)}")
    result = subprocess.run(cmd)
    if result.returncode != 0:
        print(f"FAIL: {step} exited with {result.returncode} (see {log_path})")
        try:
            with open(log_path, encoding="utf-16", errors="replace") as f:
                tail = f.readlines()[-40:]
            print("".join(tail))
        except OSError:
            pass
        sys.exit(1)


def _find_install_dir():
    candidates = []
    for env_name in ("ProgramFiles", "ProgramFiles(x86)"):
        base = os.environ.get(env_name)
        if base:
            candidates.append(os.path.join(base, "TA-Lib"))
    for path in candidates:
        if os.path.isdir(path):
            return path
    return None


def test_msi_windows(root_dir: str, msi_file_path: str, temp_dir: str, version: str):
    print(f"Testing MSI install/uninstall on Windows")
    # msiexec rejects mixed path separators (error 1619) — normalize.
    msi_file_path = os.path.normpath(os.path.abspath(msi_file_path))
    print(f"  msi_file_path={msi_file_path}")
    if not os.path.isfile(msi_file_path):
        print(f"FAIL: MSI not found: {msi_file_path}")
        sys.exit(1)

    install_log = os.path.join(temp_dir, "msi_install.log")
    uninstall_log = os.path.join(temp_dir, "msi_uninstall.log")

    _msiexec(["/i", msi_file_path], "msiexec install", install_log)

    install_dir = _find_install_dir()
    if not install_dir:
        print("FAIL: TA-Lib install directory not found under Program Files")
        sys.exit(1)
    print(f"  installed at: {install_dir}")

    # Verify the installed layout: headers under include\ta-lib (the
    # documented <ta-lib/ta_libc.h> spelling), libs and DLL.
    expected = [
        os.path.join(install_dir, "include", "ta-lib", "ta_libc.h"),
        os.path.join(install_dir, "include", "ta-lib", "ta_func.h"),
        os.path.join(install_dir, "lib", "ta-lib-static.lib"),
        os.path.join(install_dir, "lib", "ta-lib.lib"),
        os.path.join(install_dir, "bin", "ta-lib.dll"),
        os.path.join(install_dir, "lib", "cmake", "ta-lib", "ta-lib-config.cmake"),
        os.path.join(install_dir, "lib", "cmake", "ta-lib", "ta-lib-config-version.cmake"),
    ]
    missing = [p for p in expected if not os.path.isfile(p)]
    if missing:
        print("FAIL: MSI payload incomplete:")
        for p in missing:
            print(f"  missing: {p}")
        sys.exit(1)

    consumer_windows(root_dir, temp_dir, version, "msi",
                     include_dir=os.path.join(install_dir, "include"),
                     lib_dir=os.path.join(install_dir, "lib"),
                     bin_dir=os.path.join(install_dir, "bin"))

    _msiexec(["/x", msi_file_path], "msiexec uninstall", uninstall_log)

    leftovers = [p for p in expected if os.path.isfile(p)]
    if leftovers:
        print("FAIL: MSI uninstall left files behind:")
        for p in leftovers:
            print(f"  leftover: {p}")
        sys.exit(1)

    print("OK: MSI install, layout, C consumer build, and uninstall all verified")
    return
