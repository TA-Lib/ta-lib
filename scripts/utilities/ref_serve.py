"""Build ta_ref_serve, the frozen pre-cutover reference oracle.

Shared by build.py (which owns the `ta_ref_serve` target) and regtest.py (which
needs the oracle present before it can run). It lives here so the two cannot
drift, and so that BUILDING the oracle is reachable from the tool named build.
"""

import os
import subprocess
import sys

# serve_version lives in scripts/, one level up from this package.
sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
import serve_version


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
            # includes "ta_func/ta_func_stream_private.h" relative to src/.
            os.path.join(src_root, "src", "ta_common"),
            os.path.join(src_root, "src", "ta_func"),
            os.path.join(src_root, "src"),
            os.path.join(src_root, "src", "ta_abstract"),
            os.path.join(src_root, "src", "ta_abstract", "frames"),
        ],
    )


def _compile_ta_ref_serve(serve_src, lib_a, include_dirs, bin_dir, post_funcs=()):
    """Turn the generated C server source into the reference server: strip the
    generated indicator + ta_common .c includes (libta-lib.a provides those),
    add the reference headers + TA_Initialize, and link against lib_a. Returns
    the cc exit code.

    `post_funcs` are functions the current tree has but the frozen reference lib
    does not (see serve_version): they are dropped from list_functions and their
    symbols aliased to a stub so this current-transport serve links against the
    frozen library. See serve_version for the rationale."""
    import re
    with open(serve_src) as f:
        src_text = f.read()
    # VALUES only. ta_func/ and ta_common/ come from the frozen lib_a, but
    # ta_abstract_all.c / ta_func_api.c are NOT stripped and resolve against the
    # CURRENT src/ta_abstract via -I, so this serve's metadata answers are the
    # generator compared against itself. Never build a metadata gate on it (#161).
    src_text = re.sub(r'#include "ta_func/[^"]*\.c"\n', '', src_text)
    src_text = re.sub(r'#include "ta_common/[^"]*\.c"\n', '', src_text)
    src_text = src_text.replace(
        '#include <stdio.h>',
        '#include <stdio.h>\n'
        '#include "ta_func.h"\n'
        '#include "ta_memory.h"\n'
        '#include "ta_utility.h"\n'
    )
    if post_funcs:
        src_text = serve_version.filter_list_functions(src_text, post_funcs)
        stubs = serve_version.stub_definitions(post_funcs,
                                               serve_version.header_path(include_dirs))
        src_text = src_text.replace('int main(void) {', stubs + 'int main(void) {', 1)
    src_text = src_text.replace(
        'int main(void) {',
        'int main(void) { TA_Initialize(); TA_RestoreCandleDefaultSettings(TA_AllCandleSettings);'
    )
    tmp_ref = os.path.join(bin_dir, "_ta_ref_serve.c")
    with open(tmp_ref, "w") as f:
        f.write(src_text)
    cmd = ["cc", "-O3", "-flto", "-DNDEBUG", "-DTA_REF_SERVE", "-Wno-everything",
           serve_version.FP_CONTRACT_FLAG, serve_version.MATH_ERRNO_FLAG]
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
        # frozen library.
        serve_src, _lib_ignored, includes = _ta_ref_serve_paths(root, os.path.join(root, "cmake-build"))
        # Build the frozen reference static lib (the tag is immutable, but the
        # FP-contraction setting must match this tree's — see build_frozen_lib).
        lib_a = serve_version.build_frozen_lib(ref_root, ref_build, "ta_ref_serve")
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
        # Functions added since the pinned tag are absent from the frozen lib;
        # drop them from list_functions + stub their symbols so the current
        # transport links (the codegen sweep's subset gate skips them).
        post_funcs = serve_version.post_version_funcs(root, ref_root)
        if post_funcs:
            print(f"  post-reference functions (skipped by the subset gate): {', '.join(post_funcs)}")
        # A post-reference function with no list_functions entry means the server
        # sources predate it — a brand-new function whose only regenerations so
        # far were `--func`-filtered, which skip the whole-corpus files. Reported,
        # never repaired: a test path that silently regenerates what it is about
        # to measure is how a stale tree reads green, which is the whole of #322.
        with open(serve_src) as f:
            serve_text = f.read()
        stale = [n for n in post_funcs if ('\\"TA_%s\\"' % n) not in serve_text]
        if stale:
            print(f"  ERROR: the generated server sources have no list_functions entry "
                  f"for: {', '.join(stale)}")
            print("  They predate those function(s), so the oracle built from them "
                  "would not carry the transport to reach them.")
            print("  Recover with:  scripts/build.py generate")
            print("  (a `generate --func=<NAME>` run deliberately skips the "
                  "whole-corpus files, which is how this happens.)")
            sys.exit(1)
        rc = _compile_ta_ref_serve(serve_src, lib_a, includes, bin_dir, post_funcs)
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
