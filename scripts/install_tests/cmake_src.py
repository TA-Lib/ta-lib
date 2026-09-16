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
    # Not the per-config ta-lib-config-<cfg>.cmake: its name follows
    # CMAKE_BUILD_TYPE.
    "lib/cmake/ta-lib/ta-lib-config.cmake",
    "lib/cmake/ta-lib/ta-lib-config-version.cmake",
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


# CONSUMER_C above uses <ta-lib/ta_libc.h>; this is the other supported
# spelling, so the two together compile one include directory each.
CONSUMER_FLAT_C = r"""
#include <ta_libc.h>
void ta_lib_flat_spelling_compiles( void ) {}
"""

# Asserted in CMake, not by a successful compile: a dropped include entry still
# compiles on any host that already has a TA-Lib under /usr or /usr/local,
# resolving against THAT one, so the compile reads green while the target is wrong.
CONSUMER_CMAKE = """
cmake_minimum_required(VERSION 3.18)
project(ta_lib_find_package_consumer C)

find_package(ta-lib {version_req} REQUIRED CONFIG)

string(FIND "${{ta-lib_DIR}}" "{prefix}" _at)
if(NOT _at EQUAL 0)
    message(FATAL_ERROR "resolved ta-lib_DIR=${{ta-lib_DIR}}, outside the staged prefix {prefix}")
endif()

get_target_property(_type ta-lib::ta-lib TYPE)
if(NOT _type STREQUAL "{expected_type}")
    message(FATAL_ERROR "ta-lib::ta-lib is ${{_type}}, expected {expected_type}")
endif()

get_target_property(_inc ta-lib::ta-lib INTERFACE_INCLUDE_DIRECTORIES)
list(LENGTH _inc _n)
if(NOT _n EQUAL 2)
    message(FATAL_ERROR "ta-lib::ta-lib carries ${{_n}} include entries, expected 2: ${{_inc}}")
endif()
list(GET _inc 0 _inc0)
list(GET _inc 1 _inc1)
if(NOT _inc0 MATCHES "/include/ta-lib$" OR NOT _inc1 MATCHES "/include$")
    message(FATAL_ERROR "include interface ${{_inc}}, expected <prefix>/include/ta-lib then <prefix>/include")
endif()

add_executable(consumer consumer.c consumer_flat.c)
target_link_libraries(consumer PRIVATE ta-lib::ta-lib)
"""


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
    _configure_build_install(source_dir, build_dir, prefix_dir, [], "both")

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

    # Nothing above reaches the installed package config.
    _find_package_consumer(temp_dir, prefix_dir, version, "SHARED_LIBRARY", "both")

    # The only mode in which the EXPORT_NAME assignment does anything, and what
    # every vcpkg static triplet configures.
    static_build = os.path.join(temp_dir, "cmake_build_static")
    static_prefix = os.path.join(temp_dir, "cmake_prefix_static")
    _configure_build_install(source_dir, static_build, static_prefix,
                             ["-DBUILD_SHARED_LIBS=OFF"], "static-only")
    _find_package_consumer(temp_dir, static_prefix, version, "STATIC_LIBRARY",
                           "static-only")
    return


def _configure_build_install(source_dir: str, build_dir: str, prefix_dir: str,
                             extra: list, tag: str):
    run_or_die(["cmake", "-S", source_dir, "-B", build_dir,
                "-DCMAKE_BUILD_TYPE=Release",
                "-DBUILD_DEV_TOOLS=OFF",
                f"-DCMAKE_INSTALL_PREFIX={prefix_dir}"] + extra,
               f"cmake configure ({tag})")
    run_or_die(["cmake", "--build", build_dir, "-j", str(os.cpu_count() or 2)],
               f"cmake build ({tag})")
    run_or_die(["cmake", "--install", build_dir], f"cmake install ({tag})")


def _find_package_consumer(temp_dir: str, prefix_dir: str, version: str,
                           expected_type: str, tag: str):
    print(f"  [find_package consumer: {tag}] {prefix_dir}")
    src = os.path.join(temp_dir, f"find_package_{tag}")
    os.makedirs(src, exist_ok=True)
    with open(os.path.join(src, "CMakeLists.txt"), "w") as f:
        f.write(CONSUMER_CMAKE.format(version_req=version, prefix=prefix_dir,
                                      expected_type=expected_type))
    with open(os.path.join(src, "consumer.c"), "w") as f:
        f.write(CONSUMER_C)
    with open(os.path.join(src, "consumer_flat.c"), "w") as f:
        f.write(CONSUMER_FLAT_C)

    build = os.path.join(temp_dir, f"find_package_{tag}_build")
    run_or_die(["cmake", "-S", src, "-B", build,
                f"-DCMAKE_PREFIX_PATH={prefix_dir}"],
               f"find_package configure ({tag})")
    run_or_die(["cmake", "--build", build], f"find_package build ({tag})")
    _run_consumer(os.path.join(build, "consumer"), version,
                  f"run find_package consumer ({tag})")
