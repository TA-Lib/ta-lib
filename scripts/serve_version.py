"""Shared helpers for building a "serve of another version" — a JSON-RPC server
whose TRANSPORT is the current tree's generated `ta_codegen_serve.c` but whose
ORACLE is a frozen older libta-lib.a (a pinned worktree: ../ta-lib-064 @ v0.6.4,
../ta-lib-ref @ reference-pre-cutover, ...).

The current transport enumerates every CURRENT function (list_functions) and its
frame dispatch references every CURRENT indicator symbol (TA_<NAME>, TA_S_<NAME>,
TA_<NAME>_Lookback). A frozen library predates functions added since — so those
symbols are undefined at link, and advertising them in list_functions would make
a differential gate try (and fail) to diff a function the frozen version cannot
compute.

Both problems are handled the SAME way for every version serve (this is the
--fuzz-064 subset mechanism, generalized):

  * post_version_funcs(): functions the current tree has but the frozen version
    does not, from a plain ta_func_list.txt diff.
  * filter_list_functions(): drop those from the serve's list_functions payload,
    so a driver's subset gate (strstr on list_functions) skips them.
  * stub_definitions(): define the missing symbols with a `{ return TA_BAD_PARAM; }`
    body reusing their EXACT ta_func.h prototype, injected into the serve TU, so
    the serve links. Pure C (no linker-specific flag) => works on gcc/clang and
    GNU ld / lld / Apple ld64 alike. A skipped function is never dispatched, so the
    stub is never actually called; if it were, it returns an error, not a crash.
"""

import os
import re
import shutil
import subprocess
import sys


# Floating-point contraction setting for everything a differential gate compares
# against (issue #150). Mirrors CMakeLists.txt / configure.ac: explicit fma() is
# the ONLY fusion (PR #96). The pinned tags predate #96, so a frozen worktree has
# nothing of its own to inherit — without this, a compiler whose target has
# baseline FMA (any aarch64) fuses inside the REFERENCE, and the suites then
# measure a compiler difference rather than a code difference.
FP_CONTRACT_FLAG = "-ffp-contract=off"

# -fno-math-errno (issue #192). Same reasoning as above, for the same reason the
# frozen worktrees cannot inherit it: an oracle must differ from the tree in its
# CODE, never in how its compiler was told to behave. This flag cannot change a
# value -- it only frees sqrt() from the obligation to set errno -- so unlike
# FP_CONTRACT_FLAG it is inert for the differential CORRECTNESS gates. (It is not
# inert for FP exception state; see the FE_INVALID note in CMakeLists.txt.) It matters
# to `ta_bench --language=cref,c`, whose cref arm would otherwise carry a scalar
# sqrt the tree no longer has, reading as an algorithm difference on SQRT and
# STDDEV when it is purely a build one.
MATH_ERRNO_FLAG = "-fno-math-errno"

# What every frozen worktree is configured with, and what a cached build tree is
# checked against before it is trusted.
FROZEN_CFLAGS = f"{FP_CONTRACT_FLAG} {MATH_ERRNO_FLAG}"


def build_frozen_lib(ref_root, ref_build, label):
    """Configure + build a frozen worktree's libta-lib.a. Returns its path.

    Deliberately unconditional (issue #150). Gating the configure on a missing
    CMakeCache.txt lets a worktree built by an older revision keep its flagless
    cache forever, and gating the build on a missing libta-lib.a does not help
    either: deleting the archive re-archives the same stale objects without
    recompiling. cmake's own dependency tracking is the correct "build once"
    mechanism — a re-configure with changed flags invalidates every object, and
    the no-op path costs ~0.2s.

    That invalidation is mtime-based, though, so it is only reliable above the
    filesystem's 1-second granularity: a flagless build finishing in the SAME
    wall-clock second as the re-configure recompiles only SOME objects and still
    exits 0, leaving a partially fused oracle that fails as a confusing subset of
    #150's 549 rather than the whole set. Which objects fall inside that second is
    what varies: 21 of 198 recompiled (412 fused instructions left) as reported,
    0 of 198 (all 893 left) when reproduced here. So a flagless cache is discarded
    outright rather than upgraded in place, which removes mtime from the picture
    entirely — an upgrade always starts from a clean tree."""
    if _stale_cache(ref_build):
        print(f"  {label}: discarding a build tree configured without "
              f"{FROZEN_CFLAGS}")
        shutil.rmtree(ref_build)
    os.makedirs(ref_build, exist_ok=True)
    _run_quiet(["cmake", ref_root, "-DCMAKE_BUILD_TYPE=Release",
                f"-DCMAKE_C_FLAGS={FROZEN_CFLAGS}"],
               ref_build, f"{label}: cmake configure")
    out = _run_quiet(["cmake", "--build", ".", "--target", "ta-lib-static",
                      "-j", str(os.cpu_count() or 4)],
                     ref_build, f"{label}: cmake build")
    if "Building C object" in out:
        print(f"  {label}: rebuilt frozen libta-lib.a ({FROZEN_CFLAGS})")
    return os.path.join(ref_build, "libta-lib.a")


def _stale_cache(ref_build):
    """True iff ref_build holds a cmake cache that was NOT configured with
    every flag in FROZEN_CFLAGS. A directory with no cache at all is not stale —
    there is nothing to discard, and the configure below is already correct."""
    cache = os.path.join(ref_build, "CMakeCache.txt")
    if not os.path.exists(cache):
        return False
    with open(cache) as f:
        for line in f:
            # CMAKE_C_FLAGS_RELEASE et al. are distinct entries; the colon pins
            # this to the one cmake actually applies to every C TU.
            if line.startswith("CMAKE_C_FLAGS:"):
                return any(f not in line for f in FROZEN_CFLAGS.split())
    return True


def _run_quiet(cmd, cwd, what):
    """Run cmd, swallowing output unless it fails (these run on every invocation
    now, and a successful no-op should not spam the pipeline log)."""
    r = subprocess.run(cmd, cwd=cwd, capture_output=True, text=True)
    if r.returncode != 0:
        sys.stdout.write(r.stdout)
        sys.stderr.write(r.stderr)
        sys.exit(f"serve_version: {what} failed (exit {r.returncode})")
    return r.stdout


def _func_names(root):
    """The set of function names in a tree's ta_func_list.txt (first column)."""
    path = os.path.join(root, "ta_func_list.txt")
    names = set()
    with open(path) as f:
        for line in f:
            line = line.strip()
            if line:
                names.add(line.split()[0])
    return names


def post_version_funcs(current_root, version_root):
    """Sorted names present in current_root but not in version_root.

    Empty when the frozen version already has every current function (the
    historical case, before any post-freeze function existed)."""
    return sorted(_func_names(current_root) - _func_names(version_root))


# One list_functions entry line, split so the leading comma can be added or
# removed. Group 'comma' is present on every entry except the alphabetically
# first, which opens the JSON array.
_LIST_ENTRY_RE = re.compile(
    r'^(?P<head>[^\n]*json_appendf\([^\n]*?, ")(?P<comma>,?)(?P<tail>\\"TA_[A-Z0-9_]+\\"[^\n]*)$',
    re.M)


def filter_list_functions(src_text, post_funcs):
    """Remove post_funcs' entries from the serve's list_functions payload.

    Each entry is a line like
        pos = json_appendf(resp, resp_size, pos, ",\\"TA_PVO\\"");
    The escaped-quote token \\"TA_<NAME>\\" appears ONLY in list_functions (the
    dispatch chain uses the bare "TA_<NAME>"), so removing the whole line is
    safe.

    Only the alphabetically FIRST entry opens the array and so carries no
    leading comma. Removing any other entry therefore keeps the JSON valid on
    its own, but removing the first one would leave the next entry's comma
    dangling — `{"functions":[,"TA_ACCBANDS",...]}`. AC (#228) is the first
    post-version function to hit that: it sorts ahead of ACCBANDS, which had
    been the first entry since the freeze. So the comma is re-normalized over
    the surviving entries afterwards rather than assumed."""
    for name in post_funcs:
        pattern = r'[^\n]*\\"TA_' + re.escape(name) + r'\\"[^\n]*\n'
        src_text, n = re.subn(pattern, '', src_text)
        assert n == 1, "filter_list_functions: expected 1 %s entry, removed %d" % (name, n)

    # Re-normalize: exactly the first surviving entry opens the array.
    entries = list(_LIST_ENTRY_RE.finditer(src_text))
    assert entries, "filter_list_functions: no list_functions entries left"

    def fixed(m, want_comma):
        return m.group('head') + (',' if want_comma else '') + m.group('tail')

    # Rebuild back-to-front so earlier spans stay valid.
    for i, m in reversed(list(enumerate(entries))):
        repl = fixed(m, i > 0)
        if repl != m.group(0):
            src_text = src_text[:m.start()] + repl + src_text[m.end():]
    return src_text


def stub_definitions(post_funcs, header_path):
    """Portable C stub definitions for the indicator symbols each post-version
    function contributes and the current transport references: TA_<NAME>,
    TA_S_<NAME>, TA_<NAME>_Lookback (whichever ta_func.h declares).

    Each stub reuses the EXACT prototype from ta_func.h with a
    `{ return TA_BAD_PARAM; }` body, so it is a valid definition in the serve TU
    (which already `#include`s ta_func.h) — no cross-TU type mismatch and no
    linker-specific flag (unlike -Wl,--defsym, which Apple's ld64 does not
    implement). TA_BAD_PARAM is a small int, fine for the int-returning
    *_Lookback too. Never reached at runtime: the subset gate skips these
    functions before dispatch."""
    if not post_funcs:
        return ""
    with open(header_path) as f:
        header = f.read()
    defs = []
    for name in post_funcs:
        for sym in ("TA_%s" % name, "TA_S_%s" % name,
                    "TA_%s_Lookback" % name, "TA_S_%s_Lookback" % name):
            # `<sym>\s*\(` (paren-anchored) matches only <sym>'s own declaration,
            # never a longer name that has <sym> as a prefix (e.g. TA_PVO vs
            # TA_PVO_Lookback). A symbol ta_func.h does not declare is not
            # referenced by the transport, so no stub is needed for it.
            m = re.search(r'(TA_LIB_API\s+[\w ]+?' + re.escape(sym) +
                          r'\s*\([^;]*\))\s*;', header)
            if m:
                defs.append(m.group(1).strip() + " { return TA_BAD_PARAM; }")
    if not defs:
        return ""
    return ("\n/* [serve-of-another-version] portable stub definitions for symbols\n"
            " * absent from the frozen library; never dispatched (the subset gate\n"
            " * skips these functions). Exact ta_func.h signatures => valid C, no\n"
            " * linker flag, no cross-TU type mismatch. */\n" + "\n".join(defs) + "\n")


def header_path(include_dirs):
    """Locate ta_func.h among a serve build's -I directories."""
    for d in include_dirs:
        cand = os.path.join(d, "ta_func.h")
        if os.path.exists(cand):
            return cand
    return None
