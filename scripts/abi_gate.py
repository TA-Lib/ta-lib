#!/usr/bin/env python3
"""ABI gate: generated C library vs the frozen pre-cutover reference.

Compares the EXPORTED symbol surface of the GENERATED C library — the CMake
archive `cmake-build/libta-lib.a`, built from `src/` (which holds the generated
C) — against the FROZEN reference library built from the pinned-tag worktree
(../ta-lib-ref @ reference-pre-cutover).

Why this is the right check: the build uses no -fvisibility=hidden and no linker
version script, so on ELF every extern symbol of the static archive is exported
by the shared library. The exported-symbol set is therefore source-determined,
and `nm` on the archives is a faithful, build-flag-independent ABI surface.

Pass rule:
  - FAIL if any reference PUBLIC symbol (exported AND named in a public include/
    header) is missing from the generated lib — those are the contract downstream
    binaries link against; removing/renaming one is an ABI break.
  - ADDED exported symbols (e.g. *_Unguarded) are ABI-compatible additions. They
    are reported but do NOT fail the gate (adding symbols never breaks an
    existing consumer). Decision: accept the additions and deliberately do NOT
    add -fvisibility=hidden or a version script — the pre-cutover reference
    exported all its internals too, so export-everything is the status quo.
    Hardening is a possible future cleanup, not an ABI obligation.
  - Type ABI (enums/structs/typedefs) lives in include/, which the cutover keeps
    in place; the gate verifies include/ is unchanged vs the tag.

Exit status: 0 if green, 1 if a public symbol is missing (ABI break), 2 on setup
error (reference lib or worktree absent).

Usage:  python3 scripts/abi_gate.py [--verbose]
"""

import os
import re
import subprocess
import sys

REF_TAG = "reference-pre-cutover"


def repo_root():
    return subprocess.run(["git", "rev-parse", "--show-toplevel"],
                          check=True, capture_output=True, text=True).stdout.strip()


def nm_defined(path):
    """External, defined symbol names from an object file or archive."""
    out = subprocess.run(["nm", "-g", "--defined-only", path],
                         capture_output=True, text=True).stdout
    syms = set()
    for line in out.splitlines():
        parts = line.split()
        if len(parts) >= 2 and len(parts[-2]) == 1 and parts[-2].isalpha():
            syms.add(parts[-1])
    return syms


def generated_symbols(root, verbose):
    """Exported symbols of the generated shipped C library, read from the CMake
    static archive. Post-cutover (option B) the generated C lives in `src/`, so the
    archive CMake builds from `src/` IS the generated lib — nm it directly (no need
    to recompile a parallel tree)."""
    lib = os.path.join(root, "cmake-build", "libta-lib.a")
    if not os.path.exists(lib):
        print(f"  SETUP ERROR — generated lib not built: {lib}\n"
              f"  Build it:  scripts/build.py ta_regtest")
        return None, [("generated lib", lib)]
    syms = nm_defined(lib)
    if verbose:
        print(f"  generated lib: {lib} ({len(syms)} exported symbols)")
    return syms, []


def include_identifiers(root):
    ids = set()
    inc = os.path.join(root, "include")
    tok = re.compile(r"[A-Za-z_][A-Za-z0-9_]*")
    for dirpath, _dirs, files in os.walk(inc):
        for f in files:
            if f.endswith((".h", ".hpp")):
                with open(os.path.join(dirpath, f), errors="replace") as fh:
                    ids |= set(tok.findall(fh.read()))
    return ids


def include_changed_vs_tag(root):
    r = subprocess.run(["git", "diff", "--stat", REF_TAG, "--", "include/"],
                       cwd=root, capture_output=True, text=True)
    return r.stdout.strip()


def main():
    verbose = "--verbose" in sys.argv
    root = repo_root()

    ref_a = os.path.join(os.path.dirname(root), "ta-lib-ref",
                         "cmake-build", "libta-lib.a")
    if not os.path.exists(ref_a):
        print(f"ABI GATE: SETUP ERROR — reference lib not found:\n  {ref_a}\n"
              f"  Build it: scripts/regtest.py builds the pinned-tag worktree lib,\n"
              f"  or:  cd ../ta-lib-ref && cmake -B cmake-build -DCMAKE_BUILD_TYPE=Release"
              f" && cmake --build cmake-build --target ta-lib-static")
        return 2

    print("=== ABI gate: generated lib vs frozen reference (Stage 2) ===")
    ref = nm_defined(ref_a)
    gen, failures = generated_symbols(root, verbose)
    if failures:
        print("ABI GATE: SETUP ERROR — some generated sources failed to compile.")
        return 2

    inc_ids = include_identifiers(root)
    public_ref = ref & inc_ids          # exported AND named in a public header
    removed = ref - gen                 # in reference, gone from generated
    added = gen - ref                   # new in generated
    public_removed = sorted(removed & inc_ids)
    unguarded_added = sorted(s for s in added if s.endswith("_Unguarded"))
    other_added = sorted(s for s in added if not s.endswith("_Unguarded"))
    internal_removed = sorted(removed - inc_ids)

    print(f"\n  reference exported symbols : {len(ref)}")
    print(f"  generated exported symbols : {len(gen)}")
    print(f"  public (exported & in include/): {len(public_ref)}")
    print(f"\n  PUBLIC symbols REMOVED (ABI break) : {len(public_removed)}")
    for s in public_removed:
        print(f"      - {s}")
    print(f"  internal symbols removed (non-ABI) : {len(internal_removed)}")
    if verbose:
        for s in internal_removed:
            print(f"      ~ {s}")
    print(f"  added *_Unguarded (intentional)    : {len(unguarded_added)}")
    print(f"  added other symbols                : {len(other_added)}")
    if verbose or (other_added and len(other_added) <= 60):
        for s in other_added:
            tag = " [in include/]" if s in inc_ids else ""
            print(f"      + {s}{tag}")

    inc_diff = include_changed_vs_tag(root)
    print(f"\n  include/ vs {REF_TAG}: "
          + ("UNCHANGED (type ABI frozen)" if not inc_diff else "CHANGED:\n" + inc_diff))

    print()
    if public_removed:
        print("ABI GATE: ❌ FAIL — public reference symbols missing from the "
              "generated lib (see above). These are ABI breaks.")
        return 1
    print("ABI GATE: ✅ PASS — generated lib exports a superset of the reference "
          "public ABI; differences are additions (and internal-symbol churn).")
    return 0


if __name__ == "__main__":
    sys.exit(main())
