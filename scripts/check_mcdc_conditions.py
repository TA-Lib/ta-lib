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


def _split_top(expr, tok):
    """Split `expr` on every `tok` sitting at parenthesis depth 0."""
    parts, depth, last, i = [], 0, 0, 0
    while i < len(expr):
        ch = expr[i]
        if ch == "(":
            depth += 1
        elif ch == ")":
            depth -= 1
        elif depth == 0 and expr.startswith(tok, i):
            parts.append(expr[last:i])
            last = i + len(tok)
            i += len(tok) - 1
        i += 1
    parts.append(expr[last:])
    return parts


def _peel(expr):
    """Drop redundant outer parentheses: `(( a && b ))` -> `a && b`."""
    c = expr.strip()
    while c.startswith("(") and c.endswith(")"):
        depth, ok = 0, True
        for ch in c[1:-1]:
            if ch == "(":
                depth += 1
            elif ch == ")":
                depth -= 1
                if depth < 0:
                    ok = False
                    break
        if not ok or depth != 0:
            break
        c = c[1:-1].strip()
    return c


def count_conditions(path):
    """Atomic conditions of the detection expression in a candlestick source.

    The detection expression is the `if(` guarding the assignment of a non-zero
    value to outInteger. Its conditions are the `&&` at that expression's own
    depth, PLUS those of any parenthesised group that is itself a pure
    conjunction -- because `A && (B && C)` is `A && B && C` and those parens
    carry no meaning. Counting such a group as one condition is not a
    conservative reading, it is a wrong one: it leaves an interior comparison
    with no flip, no control and nothing in the totals to show it (CDLRICKSHAWMAN
    c3, whose band's lower edge could be relaxed with the whole tier still green).

    A group containing `||` is genuinely NOT flattenable and stays one condition.
    Its interior is reached on a different axis -- the output class, pb_signs() --
    because a disjunct is not falsifiable while its sibling holds.
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
    depth, i, expr = 0, open_paren, None
    while i < len(src):
        ch = src[i]
        if ch == "(":
            depth += 1
        elif ch == ")":
            depth -= 1
            if depth == 0:
                expr = src[open_paren + 1:i]
                break
        i += 1
    if expr is None:
        return None, "unterminated if( expression"

    if len(_split_top(expr, "||")) > 1:
        return None, "top-level || present; conjunct counting does not apply"

    total = 0
    for conj in _split_top(expr, "&&"):
        c = _peel(conj)
        if len(_split_top(c, "||")) > 1:
            total += 1                       # disjunction: one condition
        else:
            total += len(_split_top(c, "&&"))  # pure conjunction: flatten it
    return total, None


def _drop_calls(expr, fname):
    """Remove `fname( ... )` calls, parens balanced, so the multiplier is what's
    left. Needed because the call's own arguments carry digits -- CDLHARAMI's
    arm indexes inClose[i-1], and CDLENGULFING's picks its bar with a ternary
    inside the subscript."""
    out, i = [], 0
    while i < len(expr):
        k = expr.find(fname + "(", i)
        if k < 0:
            out.append(expr[i:])
            break
        out.append(expr[i:k])
        j = expr.index("(", k)
        depth = 0
        while j < len(expr):
            if expr[j] == "(":
                depth += 1
            elif expr[j] == ")":
                depth -= 1
                if depth == 0:
                    j += 1
                    break
            j += 1
        i = j
    return "".join(out)


def arm_classes(arm):
    """The set of non-zero values one firing arm can produce, or None."""
    if "ta_candlecolor" in arm:
        # +/-1 times a magnitude: both signs, whatever the magnitude is.
        rest = _drop_calls(arm, "ta_candlecolor")
        mags = re.findall(r"\d+", rest)
        if len(set(mags)) != 1:
            return None
        m = int(mags[0])
        return {m, -m}
    # A signed literal. The sign may be spelled explicitly: CDLTRISTAR writes
    # `+100` against its sibling `-100`.
    if re.fullmatch(r"[+-]?\s*\d+", arm):
        return {int(arm.replace(" ", ""))}
    # A ternary selecting between two literals, e.g. `cond ? 100 : -100`.
    t = re.fullmatch(r"\(?\s*[^?]*\?\s*([+-]?\s*\d+)\s*:\s*([+-]?\s*\d+)\s*\)?", arm)
    if t:
        return {int(t.group(1).replace(" ", "")), int(t.group(2).replace(" ", ""))}
    return None


def count_signs(path):
    """Distinct non-zero values the pattern's firing arms can emit.

    A pattern that hard-codes 100 or -100 emits one class. One whose arm is
    ta_candlecolor(...)*N emits two, and a builder can cover it entirely on a
    single colour without the completeness check noticing: `condition` there
    means a top-level conjunct, so a (white && x) || (black && y) disjunction is
    ONE condition and its interior is invisible by construction. pb_signs()
    declares the domain and this pins the declaration, exactly as
    count_conditions() pins pb_conditions().

    EVERY firing arm counts, not just the first. CDLENGULFING, CDLHARAMI and
    CDLHARAMICROSS each have two, ta_candlecolor(...)*100 and *80, so they emit
    FOUR classes; stopping at the first arm called them bi-signed and would have
    let a builder declare 2, satisfy this check, and leave the +/-80 arm
    uncovered -- the same hole one level down.
    """
    src = strip_comments(open(path, encoding="utf-8").read())

    # Deliberately looser than count_conditions': `outIdx` with or without the
    # ++. CDLTRISTAR writes its +/-100 through a bare outInteger[outIdx] and
    # post-increments once at the end, so the strict form finds none of its
    # firing arms. Do NOT loosen count_conditions to match: TRISTAR's -100 and
    # +100 come from two decisions NESTED inside the outer one, which the
    # top-level-conjunct model does not describe, and the strict regex failing
    # closed there is what forces a human to look. A class set does not depend
    # on decision structure, so this direction is safe and the other is not.
    classes = set()
    for cand in re.finditer(r"outInteger\s*\[\s*outIdx\+*\s*\]\s*=\s*([^;]+);", src):
        arm = " ".join(cand.group(1).split())
        if arm.strip().rstrip("0").strip() in ("", "+", "-"):
            continue                      # the non-firing `= 0` branch
        got = arm_classes(arm)
        if got is None:
            return None, "unrecognised firing arm: %s" % arm
        classes |= got
    if not classes:
        return None, "no non-zero outInteger assignment found"
    return len(classes), None


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
        s = re.search(r"pb_signs\(\s*(\d+)\s*\)", m.group(1))
        declared[fn] = (int(d.group(1)), int(s.group(1)) if s else 1)

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
        want, wantSigns = declared[fn]
        signs, sErr = count_signs(pat)
        if signs is None:
            print("  %-20s FAIL  cannot parse firing arm: %s" % (name, sErr))
            bad += 1
            continue
        okCond, okSigns = want == actual, wantSigns == signs
        flag = "ok" if okCond and okSigns else "MISMATCH"
        print("  %-20s %-8s declared %2d, source has %2d   signs: declared %d, "
              "source has %d" % (name, flag, want, actual, wantSigns, signs))
        if not okCond or not okSigns:
            bad += 1

    if bad:
        print("\ncheck_mcdc_conditions: %d builder(s) disagree with their pattern "
              "source. Either the builder under-declares -- in which case those "
              "conditions are silently untested -- or a conjunct was added to the "
              "indicator and no case covers it. Note the count FLATTENS a "
              "parenthesised pure conjunction, because `A && (B && C)` is three "
              "conditions and grouping two comparisons cost CDLRICKSHAWMAN a "
              "boundary nothing tested. A DISJUNCTION is the one group that "
              "stays a single condition: a disjunct cannot be falsified while "
              "its sibling holds, so its interior is unreachable by flips and is "
              "covered on the output-class axis (pb_signs) instead -- which is "
              "what a signs mismatch means." % bad)
        return 1
    print("Every builder's pb_conditions() and pb_signs() matches its pattern "
          "source. OK.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
