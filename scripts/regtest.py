#!/usr/bin/env python3
"""One-command regression testing and benchmarking.

Build control:
  --no-build                 Skip cmake (ta_regtest, ta_bench)
  --no-generate              Skip indicator AND server regeneration
  --no-generate-indicators   Skip indicator regeneration only
  --no-generate-servers      Skip server regeneration only

Test control:
  --no-regtest               Skip correctness verification
  --no-perftest              Skip performance benchmark
  --no-test                  Skip both regtest and perftest
  --test-only                Skip all build/generate, just run tests
  --direct-bench-only        Skip build/generate/regtest/perftest, just run direct bench

Filters (applied to generate AND test):
  --language=c,rust          Filter languages
  --function=SMA,RSI         Filter indicators
  --codegen-only             Regtest: skip C reference tests

Perftest options:
  --points=5000              Data points (default 5000)
  --iters=20                 Iterations (default 20)

Examples:
  scripts/regtest.py                                             # full pipeline
  scripts/regtest.py --no-build --function=SMA                   # regen SMA + test
  scripts/regtest.py --no-build --no-generate-servers --function=SMA
                                                                 # regen SMA indicator + test
  scripts/regtest.py --no-regtest --language=c --function=STOCH  # build + perftest only
  scripts/regtest.py --test-only --no-regtest --function=STOCH   # just perftest, no build
"""

import os
import shutil
import subprocess
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from utilities.common import check_prerequisites, PREREQS_BUILD_SERVERS

OUR_FLAGS = {
    "--no-build", "--no-generate", "--no-generate-indicators", "--no-generate-servers",
    "--no-regtest", "--no-perftest", "--no-test", "--no-direct-bench",
    "--test-only", "--direct-bench-only", "--rust-debug",
}


def find_repo_root():
    script_dir = os.path.dirname(os.path.abspath(__file__))
    result = subprocess.run(
        ["git", "rev-parse", "--show-toplevel"],
        check=True, capture_output=True, text=True, cwd=script_dir,
    )
    return result.stdout.strip()


def get_filter(args, prefix):
    for a in args:
        if a.startswith(prefix + "="):
            return a.split("=", 1)[1]
    return None


# Canonical cutover task #7 (reference-as-server): the reference C library is
# frozen at this immutable tag and checked out in a sibling git worktree, so
# ta_ref_serve stays a true oracle even after src/ta_func becomes the generated
# code. See docs/canonical-cutover-runbook.md.
REF_TAG = "reference-pre-cutover"


def _ta_ref_serve_paths(src_root, build_dir):
    """(serve_src, lib_a, include_dirs) for building ta_ref_serve from a given
    checkout (src_root) whose static lib lives in build_dir."""
    c_out = os.path.join(src_root, "ta_codegen", "output", "c")
    c_tools = os.path.join(c_out, "tools")
    return (
        os.path.join(c_tools, "ta_codegen_serve.c"),
        os.path.join(build_dir, "libta-lib.a"),
        [
            c_tools,
            os.path.join(src_root, "include"),
            os.path.join(c_out, "ta_common"),
            os.path.join(c_out, "ta_abstract"),
            os.path.join(c_out, "ta_abstract", "frames"),
            # ta_def_ui.h does `#include "ta_frame.h"` (frames/) and pulls in
            # ta_abstract_serve.c, which lives under ta_codegen/generator/templates/c.
            os.path.join(src_root, "ta_codegen", "generator", "templates", "c"),
            # Current-tree layout: ta_memory.h / ta_utility.h live with the
            # library sources, not under ta_codegen/output/c, and the server
            # includes "ta_func/ta_func_private.h" relative to src/.
            os.path.join(src_root, "src", "ta_common"),
            os.path.join(src_root, "src", "ta_func"),
            os.path.join(src_root, "src"),
            os.path.join(src_root, "src", "ta_abstract"),
            os.path.join(src_root, "src", "ta_abstract", "frames"),
        ],
    )


def _compile_ta_ref_serve(serve_src, lib_a, include_dirs, bin_dir):
    """Turn the generated C server source into the reference server: strip the
    generated indicator + ta_common .c includes (libta-lib.a provides those),
    add the reference headers + TA_Initialize, and link against lib_a. Returns
    the cc exit code."""
    import re
    with open(serve_src) as f:
        src_text = f.read()
    src_text = re.sub(r'#include "ta_func/[^"]*\.c"\n', '', src_text)
    src_text = re.sub(r'#include "ta_common/[^"]*\.c"\n', '', src_text)
    src_text = src_text.replace(
        '#include <stdio.h>',
        '#include <stdio.h>\n'
        '#include "ta_func.h"\n'
        '#include "ta_memory.h"\n'
        '#include "ta_utility.h"\n'
    )
    src_text = src_text.replace(
        'int main(void) {',
        'int main(void) { TA_Initialize(); TA_RestoreCandleDefaultSettings(TA_AllCandleSettings);'
    )
    tmp_ref = os.path.join(bin_dir, "_ta_ref_serve.c")
    with open(tmp_ref, "w") as f:
        f.write(src_text)
    cmd = ["cc", "-O3", "-flto", "-DNDEBUG", "-DTA_REF_SERVE", "-Wno-everything"]
    cmd += [f"-I{d}" for d in include_dirs]
    cmd += ["-o", os.path.join(bin_dir, "ta_ref_serve"), tmp_ref, lib_a, "-lm"]
    rc = subprocess.run(cmd).returncode
    os.unlink(tmp_ref)
    return rc


def ensure_reference_serve(root, bin_dir):
    """Build bin/ta_ref_serve, the frozen reference oracle for ta_regtest's
    cross-language codegen verification (canonical cutover task #7).

    Builds from the pinned-tag worktree (../ta-lib-ref @ REF_TAG), so the
    oracle is independent of this tree. The tag is immutable, so the reference
    lib + server are built once and reused.

    If the tag is unavailable the script ABORTS: post-cutover there is no
    valid substitute — building the "reference" from the current tree would
    compare the generated code against itself."""
    print("=== Building ta_ref_serve (frozen reference oracle) ===")
    ref_root = os.path.join(os.path.dirname(root), "ta-lib-ref")
    ref_build = os.path.join(ref_root, "cmake-build")
    bin_serve = os.path.join(bin_dir, "ta_ref_serve")

    tag_ok = subprocess.run(
        ["git", "rev-parse", "--verify", "--quiet", f"refs/tags/{REF_TAG}"],
        cwd=root, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL,
    ).returncode == 0

    # Create the worktree once, from the tag.
    if tag_ok and not os.path.isdir(ref_root):
        print(f"  Creating reference worktree {ref_root} @ {REF_TAG}")
        subprocess.run(["git", "worktree", "add", ref_root, REF_TAG],
                       check=True, cwd=root)

    if tag_ok and os.path.isdir(ref_root):
        # Transport (server source + headers) comes from the CURRENT tree so the
        # JSON-RPC protocol never drifts from what ta_regtest speaks (e.g. the
        # use_float leg); the ORACLE property lives in lib_a, which stays the
        # frozen pinned-tag build. The two trees' public C API declarations are
        # identical (audited), so current headers link cleanly against the
        # frozen library. TA_*_Unguarded/TA_S_*_Unguarded calls are compiled
        # out via TA_REF_SERVE (the frozen lib has no unguarded symbols).
        serve_src, _lib_ignored, includes = _ta_ref_serve_paths(root, os.path.join(root, "cmake-build"))
        lib_a = os.path.join(ref_build, "libta-lib.a")
        # Build the frozen reference static lib once (the tag is immutable).
        if not os.path.exists(lib_a):
            print("  Building frozen reference libta-lib.a (one time)...")
            os.makedirs(ref_build, exist_ok=True)
            if not os.path.exists(os.path.join(ref_build, "CMakeCache.txt")):
                subprocess.run(["cmake", ref_root, "-DCMAKE_BUILD_TYPE=Release"],
                               check=True, cwd=ref_build)
            subprocess.run(["cmake", "--build", ".", "--target", "ta-lib-static",
                            "-j", str(os.cpu_count() or 4)], check=True, cwd=ref_build)
        if not os.path.exists(serve_src):
            print(f"  ta_ref_serve: FAILED — {serve_src} missing in worktree")
            return
        # Rebuild if missing, or older than the frozen lib OR the current
        # transport source (the server template evolves with the protocol).
        if (os.path.exists(bin_serve)
                and os.path.getmtime(bin_serve) >= os.path.getmtime(lib_a)
                and os.path.getmtime(bin_serve) >= os.path.getmtime(serve_src)):
            print("  ta_ref_serve: up to date (frozen reference unchanged)")
            return
        rc = _compile_ta_ref_serve(serve_src, lib_a, includes, bin_dir)
        print("  ta_ref_serve:",
              "OK (from pinned-tag worktree)" if rc == 0 else f"FAILED (exit {rc})")
        if rc != 0:
            sys.exit(1)
        return

    # --- No tag: fail loudly. Post-cutover, building the "reference" from the
    # CURRENT tree is circular (the generated code would be compared against
    # itself) and its include layout no longer matches — CI must fetch the
    # pinned tag (actions/checkout fetch-depth: 0 + the tag pushed upstream).
    print(f"  ERROR: tag '{REF_TAG}' unavailable — cannot build the frozen")
    print(f"  ERROR: reference oracle. Fetch tags (git fetch --tags) or push")
    print(f"  ERROR: the tag upstream. Aborting: codegen verification without")
    print(f"  ERROR: the reference oracle would silently self-compare.")
    sys.exit(1)


def main():
    if "--help" in sys.argv or "-h" in sys.argv:
        print(__doc__.strip())
        sys.exit(0)

    check_prerequisites(PREREQS_BUILD_SERVERS)

    argv = sys.argv[1:]
    test_only      = "--test-only" in argv
    direct_only    = "--direct-bench-only" in argv
    no_build       = "--no-build" in argv or test_only or direct_only
    no_gen         = "--no-generate" in argv or test_only or direct_only
    no_gen_ind     = "--no-generate-indicators" in argv or no_gen
    no_gen_srv     = "--no-generate-servers" in argv or no_gen
    no_test        = "--no-test" in argv
    no_regtest     = "--no-regtest" in argv or no_test or direct_only
    no_perftest    = "--no-perftest" in argv or no_test or direct_only
    # Build the Rust server with the debug profile (overflow checks on) so the
    # codegen run traps the IMI/APO/PPO-class arithmetic overflow that a release
    # build wraps away. CI runs this as a Rust-only gate.
    rust_debug     = "--rust-debug" in argv

    # Alias --indicator(s) and --functions to --function
    def normalize_flag(a):
        for prefix in ("--indicators=", "--indicator=", "--functions="):
            if a.startswith(prefix):
                return "--function=" + a.split("=", 1)[1]
        return a
    passthrough = [normalize_flag(a) for a in argv if a not in OUR_FLAGS]
    func_filter = get_filter(passthrough, "--function")
    lang_filter = get_filter(passthrough, "--language")

    root = find_repo_root()
    build_dir = os.path.join(root, "cmake-build")
    bin_dir = os.path.join(root, "bin")
    codegen_dir = os.path.join(root, "ta_codegen", "generator")
    jobs = str(os.cpu_count() or 4)

    # 1. cmake
    if not no_build:
        os.makedirs(build_dir, exist_ok=True)
        if not os.path.exists(os.path.join(build_dir, "CMakeCache.txt")):
            subprocess.run(["cmake", root, "-DCMAKE_BUILD_TYPE=Release"],
                           check=True, cwd=build_dir)
        print("=== Building ta_regtest + ta_bench ===")
        subprocess.run(["cmake", "--build", ".", "--target",
                        "ensure_ta_regtest_in_bin", "ta_bench", "ta_bench_direct",
                        "-j", jobs],
                       check=True, cwd=build_dir)
        for name in ("ta_bench", "ta_bench_direct"):
            src = os.path.join(build_dir, "bin", name)
            dst = os.path.join(bin_dir, name)
            if os.path.exists(src):
                try:
                    shutil.copy2(src, dst)
                except OSError:
                    if os.path.exists(dst):
                        os.remove(dst)
                    shutil.copy2(src, dst)

        # Build ta_ref_serve from the FROZEN pinned-tag reference worktree
        # (canonical cutover task #7, reference-as-server). See ensure_reference_serve.
        ensure_reference_serve(root, bin_dir)

    # 2. generate indicators
    if not no_gen_ind:
        print("\n=== Regenerating indicator files ===")
        cmd = ["cargo", "run", "--release", "--", "generate"]
        if lang_filter:
            cmd.append(f"--backend={lang_filter}")
        if func_filter:
            cmd.append(f"--function={func_filter}")
        subprocess.run(cmd, check=True, cwd=codegen_dir)

    # 3a. generate servers
    if not no_gen_srv:
        print("\n=== Regenerating server files ===")
        cmd = ["cargo", "run", "--release", "--", "generate-servers"]
        if lang_filter:
            cmd.append(f"--backend={lang_filter}")
        subprocess.run(cmd, check=True, cwd=codegen_dir)

    # 3b. generate bench binary source
    if not no_gen_srv:
        print("\n=== Regenerating bench binary ===")
        cmd = ["cargo", "run", "--release", "--", "generate-bench", "--backend=c"]
        subprocess.run(cmd, check=True, cwd=codegen_dir)

    # 4. compile servers (only if something was regenerated)
    did_generate = not no_gen_ind or not no_gen_srv
    if did_generate:
        print("\n=== Compiling servers ===")
        cmd = ["cargo", "run", "--release", "--", "build"]
        if lang_filter:
            cmd.append(f"--backend={lang_filter}")
        subprocess.run(cmd, check=True, cwd=codegen_dir)

        # Debug-profile Rust server: rebuild just the Rust server bin without
        # --release (overflow checks on) and install it over the release one, so
        # the codegen run below crashes on an arithmetic overflow instead of
        # wrapping it away. See run_edge_range_sweep in test_codegen.c.
        if rust_debug:
            print("\n=== Rebuilding Rust server (debug profile) ===")
            rust_dir = os.path.join(root, "ta_codegen", "output", "rust")
            subprocess.run(["cargo", "build", "--bin", "ta_codegen_serve"],
                           check=True, cwd=rust_dir)
            shutil.copy2(
                os.path.join(rust_dir, "target", "debug", "ta_codegen_serve"),
                os.path.join(bin_dir, "ta_codegen_serve_rust"),
            )

    # 5. regtest
    rc = 0
    codegen_only = "--codegen-only" in passthrough
    if not no_regtest:
        # 5a. C reference tests (skip if --codegen-only)
        if not codegen_only:
            print("\n" + "=" * 60)
            print("REGTEST — C reference tests (252 points, all ranges)")
            print("=" * 60)
            # Only pass --function filter, not --language or --codegen flags
            direct_args = [a for a in passthrough
                           if a.startswith("--function=")]
            rc = subprocess.run(
                [os.path.join(bin_dir, "ta_regtest")] + direct_args,
                cwd=bin_dir,
            ).returncode
            if rc != 0:
                print(f"\nC reference regtest FAILED (exit {rc})")
                sys.exit(rc)

        # 5b. Cross-language codegen tests
        print("\n" + "=" * 60)
        print("REGTEST — cross-language codegen verification")
        print("=" * 60)
        codegen_args = list(passthrough)
        if not any(a.startswith("--codegen") for a in codegen_args):
            codegen_args = ["--codegen-only"] + codegen_args
        rc = subprocess.run(
            [os.path.join(bin_dir, "ta_regtest")] + codegen_args,
            cwd=bin_dir,
        ).returncode
        if rc != 0:
            print(f"\nCodegen regtest FAILED (exit {rc})")
            sys.exit(rc)

    # 6. perftest
    if not no_perftest:
        print("\n" + "=" * 60)
        print("PERFTEST — performance (large dataset, averaged)")
        print("=" * 60, flush=True)
        # Always include cref for comparison, even when --language= filters
        bench_args = list(passthrough)
        if lang_filter and "cref" not in lang_filter:
            for i, a in enumerate(bench_args):
                if a.startswith("--language="):
                    bench_args[i] = a + ",cref"
                    break
        bench_rc = subprocess.run(
            [os.path.join(bin_dir, "ta_bench")] + bench_args,
            cwd=bin_dir,
        ).returncode
        if bench_rc != 0 and rc == 0:
            rc = bench_rc

    # 7. direct bench (zero-overhead, no server)
    # Runs unless --no-test; independent of --no-perftest
    if (not no_test or direct_only) and "--no-direct-bench" not in argv:
        bench_direct = os.path.join(bin_dir, "ta_bench_direct")
        bench_cg = os.path.join(bin_dir, "ta_bench_cg")
        missing = []
        if not os.path.exists(bench_direct):
            missing.append("ta_bench_direct")
        if not os.path.exists(bench_cg):
            missing.append("ta_bench_cg")
        if missing:
            print(f"\n  Skipping direct bench (missing: {', '.join(missing)})")
            print("  Run without --direct-bench-only to build them first.")
        else:
            print("\n" + "=" * 60)
            print("DIRECT BENCH — zero-overhead (direct function calls)")
            print("=" * 60, flush=True)
            direct_args = [a for a in passthrough
                           if a.startswith("--function=")
                           or a.startswith("--iters=")
                           or a.startswith("--points=")]
            direct_rc = subprocess.run(
                [bench_direct] + direct_args, cwd=bin_dir,
            ).returncode
            if direct_rc != 0 and rc == 0:
                rc = direct_rc

    sys.exit(rc)


if __name__ == "__main__":
    main()
