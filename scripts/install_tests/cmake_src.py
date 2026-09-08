import os
import subprocess
import sys
import tarfile

from install_tests.python import run_or_die

# The release tarball, built the way vcpkg builds it. Nothing else covers this:
# the autotools leg passes whether or not the CMake build system reached
# `make dist`, so a file CMakeLists.txt reads and Makefile.am's EXTRA_DIST omits
# surfaces only at the next port update, when the published asset can no longer
# change. Keep every such file listed in EXTRA_DIST.
#
# Any problem => sys.exit(1). Intermediate files stay in temp_dir for debugging.

# What ports/talib/portfile.cmake reaches for after vcpkg_cmake_install(): the
# header its wrapper find_path()s, and one library per triplet linkage.
EXPECTED_INSTALLED = (
    "include/ta-lib/ta_libc.h",
    "lib/libta-lib.a",
    "lib/libta-lib.so",
)

CONSUMER_C = r'''
#include <ta-lib/ta_libc.h>
#include <stdio.h>
#include <string.h>

int main( int argc, char **argv )
{
   double in[10] = {1,2,3,4,5,6,7,8,9,10}, out[10];
   int begIdx, nbElement;
   const char *version;

   if( TA_Initialize() != TA_SUCCESS ) { printf("FAIL: TA_Initialize\n"); return 1; }
   if( TA_SMA(0,9,in,3,&begIdx,&nbElement,out) != TA_SUCCESS ) { printf("FAIL: TA_SMA\n"); return 1; }
   if( begIdx != 2 || nbElement != 8 || out[0] != 2.0 )
   {
      printf("FAIL: SMA begIdx=%d nbElement=%d out[0]=%f\n", begIdx, nbElement, out[0]);
      return 1;
   }

   version = TA_GetVersionString();
   if( argc > 1 && strncmp(version, argv[1], strlen(argv[1])) != 0 )
   {
      printf("FAIL: linked TA-Lib %s, expected %s*\n", version, argv[1]);
      return 1;
   }
   printf("consumer ok: %s\n", version);

   TA_Shutdown();
   return 0;
}
'''


def _run_consumer(exe: str, version: str, step: str):
    print(f"  [{step}] {exe} {version}")
    result = subprocess.run([exe, version], capture_output=True, text=True)
    print(result.stdout.strip() or result.stderr.strip())
    if result.returncode != 0:
        print(f"FAIL: {step} exited with {result.returncode}")
        sys.exit(1)


def test_cmake_src_linux(package_file_path: str, temp_dir: str, version: str):
    print("Testing the vcpkg build path (CMake) on Linux")
    print(f"  package_file_path={package_file_path}")
    print(f"  temp_dir={temp_dir}")
    print(f"  version={version}")

    src_dir = os.path.join(temp_dir, "cmake_src")
    os.makedirs(src_dir, exist_ok=True)
    with tarfile.open(package_file_path) as tf:
        tf.extractall(src_dir)

    entries = [d for d in os.listdir(src_dir) if os.path.isdir(os.path.join(src_dir, d))]
    if len(entries) != 1:
        print(f"FAIL: unexpected tarball layout: {entries}")
        sys.exit(1)
    source_dir = os.path.join(src_dir, entries[0])

    # vcpkg_install_copyright() reads this straight out of the extracted source.
    if not os.path.isfile(os.path.join(source_dir, "LICENSE")):
        print("FAIL: the tarball has no LICENSE; the vcpkg port installs it as the copyright file")
        sys.exit(1)

    build_dir = os.path.join(temp_dir, "cmake_build")
    prefix_dir = os.path.join(temp_dir, "cmake_prefix")

    # -DBUILD_DEV_TOOLS=OFF is the port's own option. The dev tools are covered
    # by the autotools leg, which builds and runs ta_regtest from this tarball.
    run_or_die(["cmake", "-S", source_dir, "-B", build_dir,
                "-DCMAKE_BUILD_TYPE=Release",
                "-DBUILD_DEV_TOOLS=OFF",
                f"-DCMAKE_INSTALL_PREFIX={prefix_dir}"], "cmake configure")
    run_or_die(["cmake", "--build", build_dir, "-j", str(os.cpu_count() or 2)], "cmake build")
    run_or_die(["cmake", "--install", build_dir], "cmake install")

    missing = [p for p in EXPECTED_INSTALLED
               if not os.path.exists(os.path.join(prefix_dir, p))]
    if missing:
        print(f"FAIL: the CMake install is missing what the vcpkg port expects: {missing}")
        sys.exit(1)

    # Compile against the staged prefix the way each triplet linkage does. The
    # explicit -I/-L keep this off any system-wide TA-Lib the other legs install.
    consumer_c = os.path.join(temp_dir, "cmake_consumer.c")
    with open(consumer_c, "w") as f:
        f.write(CONSUMER_C)

    include_dir = os.path.join(prefix_dir, "include")
    lib_dir = os.path.join(prefix_dir, "lib")

    shared_exe = os.path.join(temp_dir, "cmake_consumer_shared")
    run_or_die(["gcc", consumer_c, "-I", include_dir, "-L", lib_dir, "-lta-lib",
                f"-Wl,-rpath,{lib_dir}", "-o", shared_exe], "compile consumer (shared)")
    _run_consumer(shared_exe, version, "run consumer (shared)")

    static_exe = os.path.join(temp_dir, "cmake_consumer_static")
    run_or_die(["gcc", consumer_c, "-I", include_dir,
                os.path.join(lib_dir, "libta-lib.a"), "-lm", "-o", static_exe],
               "compile consumer (static)")
    _run_consumer(static_exe, version, "run consumer (static)")
    return
