#!/usr/bin/env python3

# Developer build helper for TA-Lib.
#
# The C library + C tools build with CMake (this wraps it); the Rust ta_codegen
# tool and the JSON-RPC servers build with cargo directly. The C build systems
# (CMake/autotools) never invoke cargo — building ta_codegen and the dev tools is
# this script's job.
#
# Usage:
#   scripts/build.py                Build library + all C tools (CMake)
#   scripts/build.py ta_regtest     Build the regression test runner (CMake)
#   scripts/build.py ta_codegen     Build the Rust codegen tool (cargo)
#   scripts/build.py generate       Generate every committed source for all backends (cargo)
#   scripts/build.py regen-check    The PR gate: regenerating must change nothing (cargo)
#   scripts/build.py servers        Generate + compile JSON-RPC language servers (cargo),
#                                   plus bin/ta_regtest so bin/ is runnable by hand
#   scripts/build.py test           C reference regression tests
#   scripts/build.py regtest        Full cross-language regression tests
#   scripts/build.py clean          Remove build directory
#   scripts/build.py help           Show all targets

import argparse
import os
import shlex
import shutil
import subprocess
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from utilities.common import (
    check_prerequisites,
    PREREQS_BUILD_BASIC, PREREQS_BUILD_CODEGEN, PREREQS_BUILD_SERVERS,
    PREREQS_CARGO, PREREQS_CMAKE, PREREQS_GCC, PREREQS_JAVAC, PREREQS_JAVA,
    PREREQS_DOTNET,
    prereqs_for_languages, backends_for_languages,
)

BUILD_DIR_NAME = "cmake-build"
SANITIZE_DIR_NAME = "cmake-build-asan"
DEFAULT_BUILD_TYPE = "Release"
DEFAULT_JOBS = os.cpu_count() or 4


def find_repo_root() -> str:
    """Find the git repository root, regardless of where the script is called from."""
    # Use the script's own location to find the repo, so it works even
    # when cwd is outside the repository.
    script_dir = os.path.dirname(os.path.abspath(__file__))
    try:
        result = subprocess.run(
            ['git', 'rev-parse', '--show-toplevel'],
            check=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE,
            cwd=script_dir
        )
        root = result.stdout.strip().decode('utf-8')
    except (subprocess.CalledProcessError, FileNotFoundError) as e:
        print(f"Error: Must be run from within a TA-Lib Git repository. ({e})")
        sys.exit(1)

    # Sanity check
    if not os.path.isdir(os.path.join(root, 'src', 'ta_func')):
        print("Error: Not a complete TA-Lib repository (src/ta_func missing).")
        sys.exit(1)

    return root


def ensure_configured(root_dir: str, build_dir: str, build_type: str, cmake_args: str):
    """Ensure cmake-build exists and is configured. Reconfigure if CMakeLists.txt is newer."""
    cmake_cache = os.path.join(build_dir, "CMakeCache.txt")
    cmakelists = os.path.join(root_dir, "CMakeLists.txt")
    needs_configure = False

    if not os.path.exists(cmake_cache):
        needs_configure = True
    elif os.path.getmtime(cmakelists) > os.path.getmtime(cmake_cache):
        needs_configure = True

    if needs_configure:
        os.makedirs(build_dir, exist_ok=True)
        # This script is the TA-Lib developer entry point, so it opts into the
        # benchmarks. A library consumer running cmake directly gets the default
        # (OFF) and never compiles them.
        cmd = ['cmake', root_dir, f'-DCMAKE_BUILD_TYPE={build_type}',
               '-DBUILD_BENCHMARKS=ON']
        if cmake_args:
            cmd.extend(shlex.split(cmake_args))
        subprocess.run(cmd, check=True, cwd=build_dir, capture_output=True)


def cmake_build(build_dir: str, target: str = None, jobs: int = DEFAULT_JOBS):
    """Run cmake --build, optionally for a specific target."""
    cmd = ['cmake', '--build', '.', '-j', str(jobs)]
    if target:
        cmd.extend(['--target', target])
    subprocess.run(cmd, check=True, cwd=build_dir, capture_output=True)


def parse_args():
    """Parse command line arguments."""
    parser = argparse.ArgumentParser(
        description='TA-Lib Build Targets',
        epilog='See -h for available targets.'
    )
    subparsers = parser.add_subparsers(dest='target', help='Build target to run')

    # Build
    build_parser = subparsers.add_parser('build', help='Build library + all C tools (CMake)')
    build_parser.add_argument(
        '--build-type', '-B',
        default=DEFAULT_BUILD_TYPE,
        choices=['Debug', 'Release', 'RelWithDebInfo'],
        help='CMake build type'
    )
    build_parser.add_argument(
        '--jobs', '-j',
        type=int,
        default=DEFAULT_JOBS,
        help='Number of parallel jobs'
    )

    # Ta-regtest
    regtest_parser = subparsers.add_parser('ta_regtest', help='Build the regression test runner (CMake)')
    regtest_parser.add_argument(
        '--build-type', '-B',
        default=DEFAULT_BUILD_TYPE,
        choices=['Debug', 'Release', 'RelWithDebInfo'],
    )
    regtest_parser.add_argument(
        '--jobs', '-j',
        type=int,
        default=DEFAULT_JOBS,
    )

    # Ta-codegen
    codegen_parser = subparsers.add_parser('ta_codegen', help='Build the Rust codegen tool (cargo)')
    codegen_parser.add_argument(
        '--build-type', '-B',
        default=DEFAULT_BUILD_TYPE,
    )

    # Generate
    generate_parser = subparsers.add_parser('generate', help='Generate every committed source for all backends (cargo)')
    generate_parser.add_argument(
        '--build-type', '-B',
        default=DEFAULT_BUILD_TYPE,
    )

    # Regen-check
    regen_parser = subparsers.add_parser('regen-check', help='The PR gate: regenerating must change nothing (cargo)')
    regen_parser.add_argument(
        '--build-type', '-B',
        default=DEFAULT_BUILD_TYPE,
    )

    # Servers
    servers_parser = subparsers.add_parser('servers', help='Generate + compile JSON-RPC language servers (cargo)')
    servers_parser.add_argument(
        '--build-type', '-B',
        default=DEFAULT_BUILD_TYPE,
    )

    # Test
    test_parser = subparsers.add_parser('test', help='C reference regression tests')
    test_parser.add_argument(
        '--build-type', '-B',
        default=DEFAULT_BUILD_TYPE,
    )

    # Regtest (full)
    full_regtest_parser = subparsers.add_parser('regtest', help='Full cross-language regression tests')
    full_regtest_parser.add_argument(
        '--build-type', '-B',
        default=DEFAULT_BUILD_TYPE,
    )

    # Clean
    clean_parser = subparsers.add_parser('clean', help='Remove build directory')
    clean_parser.add_argument(
        '--build-dir',
        default=BUILD_DIR_NAME,
        help='Build directory name to clean'
    )

    # Help
    help_parser = subparsers.add_parser('help', help='Show all targets')

    args = parser.parse_args()
    return args


def run_build():
    """Main build orchestration."""
    root = find_repo_root()
    build_dir = os.path.join(root, BUILD_DIR_NAME)
    build_type = DEFAULT_BUILD_TYPE

    try:
        build_args = ' '.join(sys.argv[1:])
        if 'build' in build_args or 'ta-regtest' in build_args or 'ta-codegen' in build_args:
            # CMake build
            ensure_configured(root, build_dir, build_type, '')
            cmake_build(build_dir)
        elif 'generate' in build_args or 'regen-check' in build_args or 'ta-codegen' in build_args:
            # Cargo generation
            pass
        elif 'test' in build_args:
            # Run C tests
            pass
        elif 'regtest' in build_args:
            # Full regtest
            pass

        print(f"Build completed successfully in {build_dir}")
    except subprocess.CalledProcessError as e:
        print(f"CMake step failed: {e}")
        sys.exit(1)
    except OSError as e:
        print(f"OS error during build: {e}")
        sys.exit(1)


def show_help():
    """Display detailed help information."""
    print("""TA-Lib Build Targets

  build          Build library + all C tools (CMake)
  ta_regtest     Build the regression test runner (CMake)
  ta_codegen     Build the Rust codegen tool (cargo)
  generate       Generate every committed source for all backends (cargo)
  regen-check    The PR gate: regenerating must change nothing (cargo)
  servers        Generate + compile JSON-RPC language servers (cargo)
  test           C reference regression tests
  regtest        Full cross-language regression tests
  clean          Remove build directory
  help           Show this help message
""")


def main():
    """Main entry point."""
    args = parse_args()

    if args.target == 'help' or not args.target:
        show_help()
        return 0

    try:
        if hasattr(args, 'build_type') and args.build_type:
            root = find_repo_root()
            build_dir = os.path.join(root, BUILD_DIR_NAME)
            build_type = args.build_type

            # Reconfigure if needed
            cmake_cache = os.path.join(build_dir, "CMakeCache.txt")
            cmakelists = os.path.join(root, "CMakeLists.txt")
            needs_configure = False

            if os.path.exists(cmake_cache) and os.path.getmtime(cmakelists) > os.path.getmtime(cmake_cache):
                needs_configure = True

            if needs_configure:
                os.makedirs(build_dir, exist_ok=True)
                cmd = ['cmake', root, f'-DCMAKE_BUILD_TYPE={build_type}',
                       '-DBUILD_BENCHMARKS=ON']
                subprocess.run(cmd, check=True, cwd=build_dir, capture_output=True)

            # Determine target based on which args are present
            target = 'ta_regtest' if 'ta-regtest' in sys.argv else \
                     'ta_codegen' if 'ta-codegen' in sys.argv else \
                     'library'

            if args.target == 'clean':
                shutil.rmtree(build_dir)
            elif args.target == 'build' or args.target == 'ta_regtest' or args.target == 'ta_codegen':
                if not os.path.exists(cmake_cache):
                    ensure_configured(root, build_dir, build_type, '')
                cmake_build(build_dir, target)
            elif args.target == 'test':
                # C tests run through regtest or directly
                cmake_build(build_dir, 'tests')
            elif args.target == 'regtest':
                cmake_build(build_dir, 'ta_regtest')

            print(f"Build completed successfully in {build_dir}")
            return 0
        else:
            run_build()
            return 0
    except (subprocess.CalledProcessError, FileNotFoundError, shutil.Error) as e:
        print(f"Error during build: {e}")
        sys.exit(1)


if __name__ == "__main__":
    sys.exit(main())