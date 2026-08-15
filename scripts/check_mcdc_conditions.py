#!/usr/bin/env python3
"""Assert each MC/DC builder's pb_conditions(N) against the pattern source.

pb_conditions(N) is the number the completeness check counts against, and it was
a hand-typed literal with nothing tying it to the indicator it describes. That is
a silent hole in both directions:

  - Declare fewer conditions than the pattern has and the extra ones are never
    flipped, never waived, and never reported. The completeness loop walks
    [0, N) -- whatever the author typed -- so it agrees with itself.
  - Add a conjunct to a pattern later and no test notices, provided the new
    conjunct happens to hold on the builder's detect bars. That is the common
    case for a refinement, and for the candlesticks that never fire on the
    252-bar series there is no other gate: the hand-written tables and the
    v0.6.4 freeze are all-zero for them and can only see a pattern that STARTS
    firing.

This closes both by counting the top-level `&&` conjuncts in the pattern's own
detection expression and requiring the builder to agree.

Run standalone, or via `scripts/build.py check-mcdc`.
"""

import os
import re
import sys

TEST_FILE = os.path.join("src", "tools", "ta_regtest", "ta_test_func",
                         "test_candlestick.c")
INPUT_DIR = os.path.join("ta_codegen", "input")


def find_repo_root():
    d = os.path.dirname(os.path.abspath(__file__))
    while d != os.path.dirname(d):
        if os.path.isdir(os.path.join(d, "ta_codegen", "input")):
            return d
        d = os.path.dirname(d)
    sys.exit("check_mcdc_conditions: cannot locate repo root")


def strip_comments(src):
    """Remove /* */ and // comments, preserving newlines so offsets stay sane."""
    out = []
    i, n = 0, len(src)
    while i < n:
        if src.startswith("/*", i):
            j = src.find("*/", i + 2)
            j = n if j < 0 else j + 2
            out.append("".join(ch if ch == "\n" else " " for ch in src[i:j]))
            i = j
        elif src.startswith("//", i):
            j = src.find("\n", i)
            j = n if j < 0 else j
            out.append(" " * (j - i))
            i = j
        else:
            out.append(src[i])
            i += 1
    return "".join(out)


def count_conditions(path):
    """Top-level conjuncts of the detection expression in a candlestick source.

    The detection expression is the `if(` guarding the assignment of a non-zero
    value to outInteger. Conjuncts are `&&` at the depth of the if's own
    parenthesis; anything nested inside a call or a sub-expression is part of one
    atomic condition, not a separate one.
    """
    src = strip_comments(open(path, encoding="utf-8").read())

    # The firing branch: outInteger[outIdx++] = <something other than 0>
    m = None
    for cand in re.finditer(r"outInteger\s*\[\s*outIdx\+\+\s*\]\s*=\s*([^;]+);", src):
        if cand.group(1).strip().rstrip("0").strip() not in ("", "+", "-"):
            m = cand
            break
    if m is None:
        return None, "no non-zero outInteger assignment found"

    # Walk back to the `if(` that guards it.
    head = src[:m.start()]
    k = head.rfind("if(")
    if k < 0:
        k = head.rfind("if (")
    if k < 0:
        return None, "no enclosing if( found"

    open_paren = src.index("(", k)
    depth, i, conj = 0, open_paren, 0
    while i < len(src):
        ch = src[i]
        if ch == "(":
            depth += 1
        elif ch == ")":
            depth -= 1
            if depth == 0:
                break
        elif depth == 1 and src.startswith("&&", i):
            conj += 1
            i += 1
        elif depth == 1 and src.startswith("||", i):
            return None, "top-level || present; conjunct counting does not apply"
        i += 1
    else:
        return None, "unterminated if( expression"

    return conj + 1, None


def main():
    root = find_repo_root()
    test_path = os.path.join(root, TEST_FILE)
    src = strip_comments(open(test_path, encoding="utf-8").read())

    # build_<x>() ... pb_check_mcdc("CDL<NAME>"  -- the call line pairs them.
    pairs = re.findall(
        r"(build_\w+)\s*\(\s*\)\s*;\s*e\s*=\s*pb_check_mcdc\(\s*\"([A-Z0-9_]+)\"", src)
    if not pairs:
        sys.exit("check_mcdc_conditions: found no pb_check_mcdc call sites")

    # pb_conditions(N) declared inside each builder body.
    declared = {}
    for fn in set(p[0] for p in pairs):
        m = re.search(r"static void %s\(\s*void\s*\)\s*\{(.*?)\n\}" % re.escape(fn),
                      src, re.S)
        if not m:
            sys.exit("check_mcdc_conditions: cannot find body of %s" % fn)
        d = re.search(r"pb_conditions\(\s*(\d+)\s*\)", m.group(1))
        if not d:
            sys.exit("check_mcdc_conditions: %s never calls pb_conditions()" % fn)
        declared[fn] = int(d.group(1))

    bad = 0
    print("MC/DC declared-condition check (%d builder(s))" % len(pairs))
    for fn, name in sorted(pairs, key=lambda p: p[1]):
        pat = os.path.join(root, INPUT_DIR, name.lower(), name.lower() + ".c")
        if not os.path.isfile(pat):
            print("  %-20s SKIP  no %s" % (name, os.path.relpath(pat, root)))
            continue
        actual, err = count_conditions(pat)
        if actual is None:
            print("  %-20s FAIL  cannot parse: %s" % (name, err))
            bad += 1
            continue
        want = declared[fn]
        flag = "ok" if want == actual else "MISMATCH"
        print("  %-20s %-8s declared %2d, source has %2d" % (name, flag, want, actual))
        if want != actual:
            bad += 1

    if bad:
        print("\ncheck_mcdc_conditions: %d builder(s) disagree with their pattern "
              "source. Either the builder under-declares -- in which case those "
              "conditions are silently untested -- or a conjunct was added to the "
              "indicator and no case covers it." % bad)
        return 1
    print("Every builder's pb_conditions() matches its pattern source. OK.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
