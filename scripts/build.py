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
    PREREQS_CMAKE, PREREQS_GCC, PREREQS_JAVAC, PREREQS_JAVA, PREREQS_DOTNET,
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
        # This script is the TA-Lib developer entry point, so it opts into the
        # benchmarks. A library consumer running cmake directly gets the default
        # (OFF) and never compiles them.
        cmd = ['cmake', root_dir, f'-DCMAKE_BUILD_TYPE={build_type}',
               '-DBUILD_BENCHMARKS=ON']
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
    servers             Generate all source + compile JSON-RPC language servers,
                        and bring bin/ta_regtest — the only thing that drives
                        them — up to date, so bin/ can be run by hand
    format              Re-indent the ta_codegen/input/ C source of truth
    format-check        Verify inputs are formatted (fails if not); writes nothing
    clippy              Lint both Rust crates as the dev nightly does
                        (--all-targets -D warnings). Dev profile, so it shares
                        no artifacts with the --release targets and does not
                        trigger rebuilds.

  Testing:
    test                C reference regression tests
    check-source-lists  Verify the CMake and autotools ta_regtest source
                        lists agree (no build; pure text check)
    regtest             Full pipeline: servers (cargo) + C tests + codegen verification
    fuzz-064            Bit-exact differential fuzz of the current library vs the
                        frozen released v0.6.4 (opt-in; builds ta_064_serve then
                        runs ta_regtest --fuzz-064). C-only; needs the v0.6.4 tag.
    xlang-hash          Cross-language BITWISE parity gate (opt-in; issue #113):
                        builds the Rust + Java + C# servers + ta_regtest, then runs
                        ta_regtest --xlang-hash — diffs each language server vs the
                        in-process C library (Rust via seed inputs, Java and C# via
                        lossless hex inputs) with no tolerance (except Java's
                        transcendental calls: fdlibm != libm). Needs the JDK and
                        the .NET SDK.

  Other:
    clean               Remove cmake-build/ and cmake-build-asan/
    help                Show this message

  Options:
    --build-type=Debug  Set cmake build type (default: Release)
    --jobs=8            Parallel jobs (default: number of CPUs)
    --cmake-args="..."  Extra arguments passed to cmake configure
    --language=c,rust   For servers/regtest/xlang-hash: build only
                        these backends, and require only their toolchains. A
                        machine with no JDK or .NET SDK can still run
                        `servers --language=c,rust`. Omit it and every backend
                        is built, which does need both.
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

def run_clippy(root_dir: str):
    """Lint both Rust crates exactly as the dev nightly's clippy job does.

    `-D warnings` is not optional here. The generator already denies
    clippy::pedantic on itself, but the remaining lints are warn-level: without
    the flag a local run prints them and still exits 0, so it would not be the
    same gate CI applies and a "clean" local run could still fail the nightly.

    Deliberately the dev profile, not --release. Clippy fingerprints differ from
    a plain build, so `cargo clippy --release` would share target/release with
    `build.py generate` and force a full rebuild on every alternation between
    the two. On the dev profile they use separate trees and neither invalidates
    the other (measured: build/clippy/build alternates in well under a second
    when nothing changed).
    """
    for manifest in ('ta_codegen/generator/Cargo.toml',
                     'ta_codegen/output/rust/Cargo.toml'):
        print(f"  Clippy {manifest} ...")
        subprocess.run(
            ['cargo', 'clippy', '--all-targets', '--manifest-path', manifest,
             '--', '-D', 'warnings'],
            check=True, cwd=root_dir)
    print("  Clippy clean (generator + generated crate).")


def build_servers(root_dir: str, lang_filter=None):
    """Generate the JSON-RPC language servers and compile them (cargo).

    With a --language filter only those backends are generated and built, so a
    machine without the JDK or the .NET SDK is not forced to have them just to
    exercise C and Rust (issue #150, extended from regtest.py to build.py)."""
    backends = backends_for_languages(lang_filter)
    extra = [f'--backend={backends}'] if backends else []
    run_codegen(root_dir, 'run', '--release', '--', 'generate-servers', *extra)
    run_codegen(root_dir, 'run', '--release', '--', 'build', *extra)

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

def build_xlanghash(root_dir: str, build_dir: str, jobs: int, lang_filter=None) -> int:
    """Cross-language BITWISE parity gate (issue #113). Diffs each generated
    language server against the shipped in-process C library, comparing
    full-precision output hashes with NO tolerance. Builds the Rust + Java + C#
    servers + ta_regtest, then runs `ta_regtest --xlang-hash`. Returns
    ta_regtest's exit code (non-zero on any divergence). Rust crosses the JSON
    boundary with a seed (gen_present); Java and C# cross it with lossless
    hex-bits inputs (#114). Java and C# both relax their transcendental-using
    calls to a 1e-9 tolerance (fdlibm != the C libm; .NET does not guarantee
    Math.* reaches the platform libm). Everything else stays bitwise. Needs the
    JDK for the Java server and the .NET SDK for the C# server.

    --language narrows the gate to a subset of those three, so a machine with no
    JDK or .NET SDK can still run the Rust leg. C is never a server row here —
    it is the in-process golden every row is compared against — so it is dropped
    from the filter rather than silently building a C server nobody consults.
    The filter is ALSO forwarded to ta_regtest: without it the runner would try
    to spawn the servers we deliberately did not build and merely print that
    they are unavailable, turning a narrowed run into a quietly vacuous one.
    CI passes no filter, so the nightly always runs all three rows.
    """
    rows = [b for b in (backends_for_languages(lang_filter) or 'rust,java,csharp').split(',')
            if b in ('rust', 'java', 'csharp')]
    if not rows:
        print(f"Error: --language={lang_filter} selects no xlang-hash row "
              f"(valid: rust, java, csharp; C is the in-process golden).", flush=True)
        return 2
    backends = ','.join(rows)
    if lang_filter:
        # flush: this script's stdout is block-buffered when CI captures it,
        # while the cargo/ta_regtest subprocesses write straight to the pipe —
        # without it these lines land after the output they are labelling.
        print(f"=== xlang-hash limited to: {backends} ===", flush=True)
    # 1. Generate + compile the language servers into bin/.
    run_codegen(root_dir, 'run', '--release', '--', 'generate-servers', f'--backend={backends}')
    run_codegen(root_dir, 'run', '--release', '--', 'build', f'--backend={backends}')
    # 2. The C test runner links the in-process C golden; stage it into bin/.
    cmake_build(build_dir, target='ensure_ta_regtest_in_bin', jobs=jobs)
    # 3. Run the gate (server argv is relative "./", so cwd must be bin/).
    print("=== Running ta_regtest --xlang-hash ===", flush=True)
    cmd = [os.path.join(root_dir, "bin", "ta_regtest"), "--xlang-hash"]
    if lang_filter:
        cmd.append(f"--language={backends}")
    return subprocess.run(cmd, cwd=os.path.join(root_dir, "bin")).returncode

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
CARGO_TARGETS = {'ta_codegen', 'generate', 'format', 'format-check', 'clippy'}

# C targets map to a cmake target.
#
# `servers` is here rather than in CARGO_TARGETS because it stages bin/: it
# compiles the language servers with cargo AND brings bin/ta_regtest — the only
# thing that ever drives them — up to date, so bin/ is never left holding fresh
# servers next to a runner built before the function list changed. `xlang-hash`
# and `fuzz-064` already pair the two the same way. CMake decides whether there
# is anything to rebuild, so the added step is a no-op on an unchanged tree, and
# cmake was already a prerequisite of `servers` for every --language filter.
SIMPLE_TARGETS = {
    'ta_regtest':  'ensure_ta_regtest_in_bin',
    'servers':     'ensure_ta_regtest_in_bin',
    'test':        'test',
    'regtest':     'regtest',
}

# Targets that build language servers and therefore honour --language, both for
# which backends get built and for which toolchains must be present.
LANG_FILTERED_TARGETS = ('servers', 'regtest', 'xlang-hash')

# Map each target to the prerequisite set it requires. These are the
# no---language defaults; see LANG_FILTERED_TARGETS above for the narrowed path.
TARGET_PREREQS = {
    'all':          PREREQS_BUILD_BASIC,
    'ta_regtest':   PREREQS_BUILD_BASIC,
    'ta_codegen':   PREREQS_BUILD_CODEGEN,
    'generate':     PREREQS_BUILD_CODEGEN,
    'format':       PREREQS_BUILD_CODEGEN,
    'format-check': PREREQS_BUILD_CODEGEN,
    'clippy':       PREREQS_BUILD_CODEGEN,
    'servers':      PREREQS_BUILD_SERVERS,
    'test':         PREREQS_BUILD_BASIC,
    'regtest':      PREREQS_BUILD_SERVERS,
    'fuzz-064':     [PREREQS_CMAKE, PREREQS_GCC],
    # build_xlanghash builds --backend=rust,java,csharp, so the .NET SDK is as
    # required here as the JDK. Without it the C# server silently never builds.
    'xlang-hash':   PREREQS_BUILD_CODEGEN + [PREREQS_GCC, PREREQS_JAVAC, PREREQS_JAVA,
                                             PREREQS_DOTNET],
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
    # Narrows BOTH the prerequisite check and the backends actually built, so a
    # machine without a JDK or the .NET SDK can still do `servers`/`regtest`
    # for the backends it does have. Same tokens as regtest.py / ta_regtest.
    parser.add_argument('--language', default=None,
                        help='c,rust,java,csharp — limit which servers are built')
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

    # Targets that build language servers narrow their prerequisites to the
    # backends actually requested: `servers --language=c,rust` must not demand a
    # JDK or the .NET SDK.
    #
    # The prerequisites are derived from the RESOLVED backend list, not from the
    # raw filter, so the check can never disagree with what gets built. A filter
    # naming no real backend (`--language=cref`, or a typo) resolves to None,
    # meaning "build everything" — and then the full toolchain is required,
    # rather than promising a short tool list and building all four anyway.
    _backends = backends_for_languages(args.language)
    if args.target in LANG_FILTERED_TARGETS and _backends:
        check_prerequisites(prereqs_for_languages(_backends))
    else:
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
        elif args.target == 'clippy':
            run_clippy(root_dir)
        return

    ensure_configured(root_dir, build_dir, args.build_type, args.cmake_args)

    # Bit-exact differential fuzz vs frozen v0.6.4 (opt-in; C-only composite —
    # not a single cmake/cargo target). Propagates ta_regtest's exit code.
    if args.target == 'fuzz-064':
        sys.exit(build_fuzz064(root_dir, build_dir, args.jobs))

    # Cross-language BITWISE parity gate (opt-in; issue #113). Composite: build the
    # Rust server + ta_regtest, then diff each server vs the in-process C golden.
    if args.target == 'xlang-hash':
        sys.exit(build_xlanghash(root_dir, build_dir, args.jobs, args.language))

    # The cross-language tests run the C ta_regtest binary against the language
    # servers, so build the servers (cargo) first — the CMake regtest target no
    # longer does it. `servers` runs the same cargo step and then falls through
    # to ensure_ta_regtest_in_bin, so what it leaves in bin/ can be run by hand.
    if args.target in ('servers', 'regtest'):
        build_servers(root_dir, args.language)

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
