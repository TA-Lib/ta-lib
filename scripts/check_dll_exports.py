#!/usr/bin/env python3
"""Link a probe against the Windows DLL's import library and run it.

Nothing else in this project ever links `ta-lib.dll`. The dist tests check that
it and `ta-lib.lib` EXIST (scripts/install_tests/msi.py) and the Python wrapper
static-links `ta-lib-static.lib`, so a prototype that loses `TA_LIB_API` builds
green everywhere and is simply absent from the DLL. That shipped once already:
`TA_GetVersionString` was missing from 2002 until 0.7.1 (#57).

On ELF the same omission is caught by check_abi.py's export gate, which reads
`nm -D`. This is the Windows half, and it is a LINK test rather than a symbol
dump because there is no portable `nm` here -- a missing export is an
unresolved external at link time, which is exactly the failure a consumer sees.

Windows only; run from the repo root.
"""

import glob
import os
import shutil
import subprocess
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from utilities.windows import call_vcvarsall          # noqa: E402


def die(msg: str):
    print("::error::%s" % msg)
    sys.exit(1)


def run(cmd, **kw):
    print("+ %s" % " ".join(cmd), flush=True)
    if subprocess.run(cmd, **kw).returncode != 0:
        die("command failed: %s" % " ".join(cmd))


def one(pattern: str, what: str) -> str:
    hits = glob.glob(pattern, recursive=True)
    if not hits:
        die("no %s matched %s -- nothing to link, so this gate measured nothing."
            % (what, pattern))
    return hits[0]


def main() -> int:
    if sys.platform != "win32":
        die("Windows only -- on ELF this is check_abi.py's export gate.")
    root = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
    os.chdir(root)
    call_vcvarsall(root, ["amd64"])

    # Shared ONLY: the probe must resolve through the import library rather than
    # quietly succeed by pulling objects out of ta-lib-static.lib.
    run(["cmake", "-S", ".", "-B", "build-dll",
         "-DBUILD_SHARED_LIBS=ON", "-DBUILD_STATIC_LIBS=OFF", "-DBUILD_DEV_TOOLS=OFF"])
    run(["cmake", "--build", "build-dll", "--config", "Release"])

    implib = one("build-dll/**/ta-lib.lib", "import library")
    dll = one("build-dll/**/ta-lib.dll", "DLL")
    print("import library: %s\nDLL: %s" % (implib, dll))

    run(["cmake", "-S", "src/tools/dll_probe", "-B", "probe-build",
         "-DTA_INCLUDE_DIR=%s" % os.path.join(root, "include").replace("\\", "/"),
         "-DTA_IMPORT_LIB=%s" % os.path.abspath(implib).replace("\\", "/")])
    run(["cmake", "--build", "probe-build", "--config", "Release"])

    exe = one("probe-build/**/dll_probe.exe", "probe executable")
    shutil.copy(dll, os.path.dirname(os.path.abspath(exe)))
    rc = subprocess.run([os.path.abspath(exe)]).returncode
    if rc != 0:
        die("the probe linked against the DLL but failed at runtime (exit %d)." % rc)
    print("The DLL exports every entry point the probe calls.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
