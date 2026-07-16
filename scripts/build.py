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
#   scripts/build.py generate       Generate per-function source for all backends (cargo)
#   scripts/build.py servers        Generate + compile JSON-RPC language servers (cargo)
#   scripts/build.py test           C reference regression tests
#   scripts/build.py regtest        Full cross-language regression tests
#   scripts/build.py regtest-only   Codegen verification only (skip C tests)
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
    PREREQS_CMAKE, PREREQS_GCC, PREREQS_JAVAC, PREREQS_JAVA,
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
    except (subprocess.CalledProcessError, FileNotFoundError):
        print("Error: Must be run from within a TA-Lib Git repository.")
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
        cmd = ['cmake', root_dir, f'-DCMAKE_BUILD_TYPE={build_type}']
        if cmake_args:
            cmd.extend(shlex.split(cmake_args))
        subprocess.run(cmd, check=True, cwd=build_dir)

def cmake_build(build_dir: str, target: str = None, jobs: int = DEFAULT_JOBS):
    """Run cmake --build, optionally for a specific target."""
    cmd = ['cmake', '--build', '.', '-j', str(jobs)]
    if target:
        cmd.extend(['--target', target])
    subprocess.run(cmd, check=True, cwd=build_dir)

def show_help():
    print("""TA-Lib Build Targets

  Building (C, via CMake):
    (default)           Build library + all C tools
    ta_regtest          Build the regression test runner

  Building (Rust ta_codegen, via cargo — CMake never invokes cargo):
    ta_codegen          Build the Rust codegen tool
    generate            Generate per-function source for all backends
    servers             Generate all source + compile JSON-RPC language servers
    format              Re-indent the ta_codegen/input/ C source of truth
    format-check        Verify inputs are formatted (fails if not); writes nothing

  Testing:
    test                C reference regression tests
    check-source-lists  Verify the CMake and autotools ta_regtest source
                        lists agree (no build; pure text check)
    regtest             Full pipeline: servers (cargo) + C tests + codegen verification
    regtest-only        Codegen verification only (skip C tests)
    fuzz-064            Bit-exact differential fuzz of the current library vs the
                        frozen released v0.6.4 (opt-in; builds ta_064_serve then
                        runs ta_regtest --fuzz-064). C-only; needs the v0.6.4 tag.
    xlang-hash          Cross-language BITWISE parity gate (opt-in; issue #113):
                        builds the Rust + Java servers + ta_regtest, then runs
                        ta_regtest --xlang-hash — diffs each language server vs the
                        in-process C library (Rust via seed inputs, Java via
                        lossless hex inputs) with no tolerance (except Java's
                        transcendental calls: fdlibm != libm). Needs the JDK.

  Other:
    clean               Remove cmake-build/ and cmake-build-asan/
    help                Show this message

  Options:
    --build-type=Debug  Set cmake build type (default: Release)
    --jobs=8            Parallel jobs (default: number of CPUs)
    --cmake-args="..."  Extra arguments passed to cmake configure
    --sanitize          Build with AddressSanitizer + UBSan into cmake-build-asan
                        (Debug, static-only; e.g. build.py ta_regtest --sanitize)
""")

def run_codegen(root_dir: str, *cargo_args: str):
    """Run a cargo command in the ta_codegen generator crate.

    This is how ta_codegen (Rust) is built/run — directly via cargo, never through
    CMake. Keeps the C build systems free of any Rust/cargo dependency.
    """
    codegen_dir = os.path.join(root_dir, "ta_codegen", "generator")
    subprocess.run(['cargo', *cargo_args], check=True, cwd=codegen_dir)

def build_servers(root_dir: str):
    """Generate the JSON-RPC language servers and compile them (cargo)."""
    run_codegen(root_dir, 'run', '--release', '--', 'generate-servers')
    run_codegen(root_dir, 'run', '--release', '--', 'build')

def build_fuzz064(root_dir: str, build_dir: str, jobs: int) -> int:
    """Opt-in bit-exact differential fuzz of the current library vs the frozen
    released v0.6.4. Builds bin/ta_064_serve (v0.6.4 worktree + shadow-patched
    transport) and ta_regtest, then runs `ta_regtest --fuzz-064`. C-only — no
    cargo/JVM/.NET. Returns ta_regtest's exit code (non-zero on real divergence).
    """
    # 1. Frozen v0.6.4 oracle server (creates ../ta-lib-064 worktree + its lib).
    subprocess.run([sys.executable,
                    os.path.join(root_dir, "scripts", "build_064_serve.py")],
                   check=True)
    # 2. The C test runner (staged into bin/).
    cmake_build(build_dir, target='ensure_ta_regtest_in_bin', jobs=jobs)
    # 3. Run the fuzz (argv is relative "./ta_064_serve", so cwd must be bin/).
    print("=== Running ta_regtest --fuzz-064 ===")
    return subprocess.run([os.path.join(root_dir, "bin", "ta_regtest"), "--fuzz-064"],
                          cwd=os.path.join(root_dir, "bin")).returncode

def build_xlanghash(root_dir: str, build_dir: str, jobs: int) -> int:
    """Cross-language BITWISE parity gate (issue #113). Diffs each generated
    language server against the shipped in-process C library, comparing
    full-precision output hashes with NO tolerance. Builds the Rust + Java servers
    + ta_regtest, then runs `ta_regtest --xlang-hash`. Returns ta_regtest's exit
    code (non-zero on any divergence). Rust crosses the JSON boundary with a seed
    (gen_present); Java crosses it with lossless hex-bits inputs (#114) and relaxes
    its transcendental-using calls to a tolerance (fdlibm != the C libm). Needs the
    JDK for the Java server; .NET P/Invokes C == C by construction, so no .NET SDK.
    """
    # 1. Generate + compile the language servers into bin/ (Rust + Java).
    run_codegen(root_dir, 'run', '--release', '--', 'generate-servers', '--backend=rust,java')
    run_codegen(root_dir, 'run', '--release', '--', 'build', '--backend=rust,java')
    # 2. The C test runner links the in-process C golden; stage it into bin/.
    cmake_build(build_dir, target='ensure_ta_regtest_in_bin', jobs=jobs)
    # 3. Run the gate (server argv is relative "./", so cwd must be bin/).
    print("=== Running ta_regtest --xlang-hash ===")
    return subprocess.run([os.path.join(root_dir, "bin", "ta_regtest"), "--xlang-hash"],
                          cwd=os.path.join(root_dir, "bin")).returncode

def check_regtest_source_lists(root_dir: str) -> bool:
    """Verify the two hand-maintained ta_regtest source lists agree.

    ta_regtest is built by BOTH build systems: CMake (TA_REGTEST_SOURCES in
    CMakeLists.txt, used by the local dev loop and the cross-language CI job)
    and autotools (ta_regtest_SOURCES in src/tools/ta_regtest/Makefile.am,
    used by the dist "end-user simulation" nightly). The lists are maintained
    by hand in parallel: a file added to only one of them compiles fine under
    CMake locally and then breaks the autotools nightly. Returns True when
    the lists are identical.
    """
    import re

    cmake_path = os.path.join(root_dir, 'CMakeLists.txt')
    with open(cmake_path, encoding='utf-8') as f:
        cmake_text = f.read()
    m = re.search(r'set\(TA_REGTEST_SOURCES\n(.*?)\n\s*\)', cmake_text, re.S)
    if not m:
        print("Error: TA_REGTEST_SOURCES block not found in CMakeLists.txt")
        return False
    cmake_set = set()
    for line in m.group(1).splitlines():
        entry = line.strip().strip('"')
        if not entry:
            continue
        cmake_set.add(entry.replace(
            '${CMAKE_CURRENT_SOURCE_DIR}/src/tools/ta_regtest/', ''))

    am_path = os.path.join(root_dir, 'src', 'tools', 'ta_regtest', 'Makefile.am')
    am_set = set()
    with open(am_path, encoding='utf-8') as f:
        lines = f.readlines()
    i = 0
    while i < len(lines):
        if lines[i].lstrip().startswith('ta_regtest_SOURCES'):
            block = lines[i].split('=', 1)[1]
            while block.rstrip().endswith('\\') and i + 1 < len(lines):
                block = block.rstrip().rstrip('\\') + ' '
                i += 1
                block += lines[i]
            am_set.update(block.rstrip().rstrip('\\').split())
            break
        i += 1
    if not am_set:
        print(f"Error: ta_regtest_SOURCES block not found in {am_path}")
        return False

    ok = True
    lists = [('CMakeLists.txt TA_REGTEST_SOURCES', cmake_set),
             ('src/tools/ta_regtest/Makefile.am ta_regtest_SOURCES', am_set)]
    union = set()
    for _, entries in lists:
        union |= entries
    for name, entries in lists:
        missing = sorted(union - entries)
        for entry in missing:
            print(f"Error: missing from {name}: {entry}")
            ok = False
    if not ok:
        print("Add the missing entries so all build systems compile the same files.")
        return False

    print(f"ta_regtest source lists agree across CMake and autotools "
          f"({len(cmake_set)} files). OK.")
    return True

# Rust targets run cargo directly (no CMake).
CARGO_TARGETS = {'ta_codegen', 'generate', 'servers', 'format', 'format-check'}

# C targets map to a cmake target.
SIMPLE_TARGETS = {
    'ta_regtest':  'ensure_ta_regtest_in_bin',
    'test':        'test',
    'regtest':     'regtest',
    'regtest-only':'regtest-only',
}

# Map each target to the prerequisite set it requires.
TARGET_PREREQS = {
    'all':          PREREQS_BUILD_BASIC,
    'ta_regtest':   PREREQS_BUILD_BASIC,
    'ta_codegen':   PREREQS_BUILD_CODEGEN,
    'generate':     PREREQS_BUILD_CODEGEN,
    'format':       PREREQS_BUILD_CODEGEN,
    'format-check': PREREQS_BUILD_CODEGEN,
    'servers':      PREREQS_BUILD_SERVERS,
    'test':         PREREQS_BUILD_BASIC,
    'regtest':      PREREQS_BUILD_SERVERS,
    'regtest-only': PREREQS_BUILD_SERVERS,
    'fuzz-064':     [PREREQS_CMAKE, PREREQS_GCC],
    'xlang-hash':   PREREQS_BUILD_CODEGEN + [PREREQS_GCC, PREREQS_JAVAC, PREREQS_JAVA],
}

def main():
    # Refuse to run as root/sudo (Unix only).
    if hasattr(os, 'getuid') and os.getuid() == 0:
        print("Error: Do not run this script as root or with sudo.")
        sys.exit(1)

    parser = argparse.ArgumentParser(
        description="TA-Lib developer build helper",
        add_help=False,
    )
    parser.add_argument('target', nargs='?', default='all')
    parser.add_argument('--build-type', default=DEFAULT_BUILD_TYPE)
    parser.add_argument('--jobs', '-j', type=int, default=DEFAULT_JOBS)
    parser.add_argument('--cmake-args', default='')
    parser.add_argument('--sanitize', action='store_true',
                        help='Build with AddressSanitizer + UBSan into cmake-build-asan (issue #94)')
    parser.add_argument('--help', '-h', action='store_true')
    args = parser.parse_args()

    if args.help or args.target == 'help':
        show_help()
        return

    root_dir = find_repo_root()
    build_dir = os.path.join(root_dir, BUILD_DIR_NAME)

    # ASan/UBSan builds go to a separate directory (Debug, static-only) so they
    # never share object files or the CMake cache with the Release tree.
    if args.sanitize:
        build_dir = os.path.join(root_dir, SANITIZE_DIR_NAME)
        if args.build_type == DEFAULT_BUILD_TYPE:
            args.build_type = 'Debug'
        args.cmake_args = (
            args.cmake_args + ' -DENABLE_SANITIZERS=ON -DBUILD_SHARED_LIBS=OFF'
        ).strip()

    if args.target == 'clean':
        removed = False
        for d in (os.path.join(root_dir, BUILD_DIR_NAME),
                  os.path.join(root_dir, SANITIZE_DIR_NAME)):
            try:
                shutil.rmtree(d)
                print(f"Removed {d}")
                removed = True
            except FileNotFoundError:
                pass
        if not removed:
            print("Nothing to clean.")
        return

    # Pure text check — no build prerequisites.
    if args.target == 'check-source-lists':
        sys.exit(0 if check_regtest_source_lists(root_dir) else 1)

    check_prerequisites(TARGET_PREREQS.get(args.target, PREREQS_BUILD_BASIC))

    # Rust ta_codegen targets build with cargo directly — no CMake involved.
    if args.target in CARGO_TARGETS:
        if args.target == 'ta_codegen':
            run_codegen(root_dir, 'build', '--release')
        elif args.target == 'generate':
            run_codegen(root_dir, 'run', '--release', '--', 'generate')
        elif args.target == 'format':
            run_codegen(root_dir, 'run', '--release', '--', 'format')
        elif args.target == 'format-check':
            run_codegen(root_dir, 'run', '--release', '--', 'format', '--check')
        else:  # servers
            build_servers(root_dir)
        return

    ensure_configured(root_dir, build_dir, args.build_type, args.cmake_args)

    # Bit-exact differential fuzz vs frozen v0.6.4 (opt-in; C-only composite —
    # not a single cmake/cargo target). Propagates ta_regtest's exit code.
    if args.target == 'fuzz-064':
        sys.exit(build_fuzz064(root_dir, build_dir, args.jobs))

    # Cross-language BITWISE parity gate (opt-in; issue #113). Composite: build the
    # Rust server + ta_regtest, then diff each server vs the in-process C golden.
    if args.target == 'xlang-hash':
        sys.exit(build_xlanghash(root_dir, build_dir, args.jobs))

    # The cross-language tests run the C ta_regtest binary against the language
    # servers, so build the servers (cargo) first — the CMake regtest target no
    # longer does it.
    if args.target in ('regtest', 'regtest-only'):
        build_servers(root_dir)

    if args.target == 'all':
        cmake_build(build_dir, jobs=args.jobs)
    elif args.target in SIMPLE_TARGETS:
        cmake_target = SIMPLE_TARGETS[args.target]
        # Under --sanitize the binary lives only in cmake-build-asan/bin/. Build
        # the raw target rather than ensure_ta_regtest_in_bin, whose ALL-target
        # copy would overwrite the Release bin/ta_regtest with the ASan build.
        if args.sanitize and cmake_target == 'ensure_ta_regtest_in_bin':
            cmake_target = 'ta_regtest'
        cmake_build(build_dir, target=cmake_target, jobs=args.jobs)
    else:
        print(f"Error: Unknown target '{args.target}'. Run with 'help' to see available targets.")
        sys.exit(1)

if __name__ == '__main__':
    main()
