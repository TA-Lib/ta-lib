import os
import subprocess
import sys

from install_tests.python import build_wrapper_and_verify

# MSI end-user simulation (Windows): silent-install the release-candidate
# MSI, verify the installed layout, build ta-lib-python from source against
# the INSTALLED location (the real end-user path), then silent-uninstall and
# verify removal. Any problem => sys.exit(1).
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


def test_msi_windows(msi_file_path: str, temp_dir: str, version: str):
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

    # Verify the installed layout: headers under include\ta-lib (the 0.6.1
    # convention, required by ta-lib-python source builds), libs and DLL.
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

    # The real end-user path: build the python wrapper against the MSI
    # install. ta-lib-python's default search paths include
    # "<Program Files>\TA-Lib\{include,lib}", but pass them explicitly so
    # the test does not depend on which Program Files variant was used.
    msi_venv_dir = os.path.join(temp_dir, "msi_python")
    os.makedirs(msi_venv_dir, exist_ok=True)
    build_wrapper_and_verify(
        msi_venv_dir, version,
        include_path=os.path.join(install_dir, "include"),
        library_path=os.path.join(install_dir, "lib"),
    )

    _msiexec(["/x", msi_file_path], "msiexec uninstall", uninstall_log)

    leftovers = [p for p in expected if os.path.isfile(p)]
    if leftovers:
        print("FAIL: MSI uninstall left files behind:")
        for p in leftovers:
            print(f"  leftover: {p}")
        sys.exit(1)

    print("OK: MSI install, layout, python wrapper build, and uninstall all verified")
    return
