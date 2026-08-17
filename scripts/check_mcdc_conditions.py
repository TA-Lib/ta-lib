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

Declared exemption: CDLHIKKAKE and CDLHIKKAKEMOD are never wired to
pb_check_mcdc and this script never looks at them -- their `patternResult` is
carried in a local across loop iterations (an initial +/-100 on one bar, a
+/-200 confirmation within a few bars of it, tracked by a countdown counter),
not a single per-bar boolean expression, so the firing-arm model this script
counts does not apply. Both have MC/DC-shaped coverage already, just not
through this gate: `test_hikkake_predicate_coverage` in test_candlestick.c
exercises every structural predicate (P1/P2/P34/P56) via near-miss scenarios,
on the legacy pb_check/pb_expect shape rather than pb_conditions/pb_flip/
pb_control. #219's "12 do (10 true MC/DC, 2 Hikkake predicate gates)" already
counted these two as covered on that basis; nothing here proposes converting
them.
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


def _directly_nested(src, if_pos):
    """True if the if-statement starting at `if_pos` is the SOLE content of
    another if's block: `if( X ) { if( Y ) ... }` with nothing else between
    the braces. That shape means the firing decision spans more than one
    if-level, and a single-level conjunct count cannot describe it -- it can
    only land on whichever level happens to be nearest the assignment and
    silently omit every outer guard. CDLENGULFING, CDLHARAMI and
    CDLHARAMICROSS are exactly this shape: the counter used to return 2, 2
    and 1 respectively, each measuring only the innermost level and each a
    plausible-looking number that omitted the rest of the decision.

    A regular flat pattern's innermost `if(` is preceded by other statements
    (a loop's `do {`, an index bump, an accumulator update) rather than a
    bare `{` that closes straight back to another `if(...)`, so this cannot
    trip on the 55 flat patterns -- it only fires when there is genuinely
    nothing else in the enclosing block.
    """
    j = if_pos
    while j > 0 and src[j - 1] in " \t\n\r":
        j -= 1
    if j == 0 or src[j - 1] != "{":
        return False
    j -= 1
    while j > 0 and src[j - 1] in " \t\n\r":
        j -= 1
    if j == 0 or src[j - 1] != ")":
        return False
    depth, k = 1, j - 2
    while k >= 0 and depth > 0:
        if src[k] == ")":
            depth += 1
        elif src[k] == "(":
            depth -= 1
        k -= 1
    if depth != 0:
        return False
    return src[:k + 1].rstrip().endswith("if")


def _enclosing_if(src, if_pos):
    """Position of the `if` whose block contains ONLY the if at `if_pos`, or -1."""
    j = if_pos
    while j > 0 and src[j - 1] in " \t\n\r":
        j -= 1
    if j == 0 or src[j - 1] != "{":
        return -1
    j -= 1
    while j > 0 and src[j - 1] in " \t\n\r":
        j -= 1
    if j == 0 or src[j - 1] != ")":
        return -1
    depth, k = 1, j - 2
    while k >= 0 and depth > 0:
        if src[k] == ")":
            depth += 1
        elif src[k] == "(":
            depth -= 1
        k -= 1
    if depth != 0:
        return -1
    head = src[:k + 1].rstrip()
    return len(head) - 2 if head.endswith("if") else -1


def _block_after(src, pos):
    """The `{...}` block following the parenthesised condition starting at pos."""
    o = src.index("(", pos)
    d = 0
    for j in range(o, len(src)):
        if src[j] == "(":
            d += 1
        elif src[j] == ")":
            d -= 1
            if d == 0:
                b = src.index("{", j)
                break
    d = 0
    for j in range(b, len(src)):
        if src[j] == "{":
            d += 1
        elif src[j] == "}":
            d -= 1
            if d == 0:
                return src[b:j + 1]
    return src[b:]


def _guard_if(src, m_start):
    """The `if` that decides whether this pattern FIRES.

    Normally the innermost one guarding the assignment. Where that if is the sole
    content of another -- CDLENGULFING, CDLHARAMI, CDLHARAMICROSS -- the inner one
    is choosing between two NON-ZERO values, which is the output-class axis
    pb_signs() covers, and the OUTER one is what decides firing. So walk out, but
    only when every assignment in the outer block is non-zero. CDLTRISTAR looks
    the same and is not: its outer block assigns 0 before the inner ifs, so the
    outer if does not decide firing and this still declines.
    """
    head = src[:m_start]
    k = head.rfind("if(")
    if k < 0:
        k = head.rfind("if (")
    if k < 0:
        return -1, "no enclosing if( found"
    outer = _enclosing_if(src, k)
    if outer < 0:
        return k, None
    for a in re.finditer(r"outInteger\s*\[\s*outIdx\+*\s*\]\s*=\s*([^;]+);",
                         _block_after(src, outer)):
        if a.group(1).strip().rstrip("0").strip() in ("", "+", "-"):
            return -1, ("assignment is guarded by nested if-statements and the "
                        "outer block also assigns zero, so the outer if does not "
                        "decide firing; needs a hand-derived condition model")
    if _enclosing_if(src, outer) >= 0:
        return -1, "more than two nested if-levels; needs a hand-derived model"
    return outer, None


def _loop_locals(src):
    """Identifiers assigned inside the per-bar loop body.

    A guard that reads one of these is not a function of the bars at `i` -- it
    is carried from an earlier iteration. The Hikkakes are the only patterns
    with any: they detect on a per-bar predicate, remember the result, and emit
    a CONFIRMATION up to three bars later off that remembered state. The
    detection decision is ordinary and countable; the confirmation is a state
    machine and is not a per-bar decision at all.
    """
    body = src
    k = body.find("outIdx = 0;")
    if k >= 0:
        body = body[k:]
    out = set()
    for m in re.finditer(r"(?m)^\s*([A-Za-z_][A-Za-z0-9_]*)\s*=[^=]", body):
        out.add(m.group(1))
    out.discard("outIdx")
    out.discard("i")
    return out


def _match_brace(src, pos):
    """Extent of the statement or block that starts at/after `pos`.

    A braceless body -- `if( cd > 0 ) cd--;` -- ends at its semicolon. Treating
    it as a block instead makes the search skip forward to some unrelated `{`
    far below, and every position inside THAT block then looks enclosed by an if
    it has nothing to do with. The Hikkakes are where this bites: they carry a
    warm-up loop that repeats the whole body before the main one, so a backward
    walk crosses a braceless `if( cd > 0 )` on its way and picks it up as a
    guard.
    """
    j = pos
    while j < len(src) and src[j] in " \t\r\n":
        j += 1
    if j >= len(src):
        return pos, len(src) - 1
    if src[j] != "{":
        e = src.find(";", j)
        return j, (len(src) - 1 if e < 0 else e)
    b, d = j, 0
    for k in range(b, len(src)):
        if src[k] == "{":
            d += 1
        elif src[k] == "}":
            d -= 1
            if d == 0:
                return b, k
    return b, len(src) - 1


def _cond_of_if(src, k):
    """(expression, index just past the closing paren) for the if at k."""
    o = src.index("(", k)
    d = 0
    for j in range(o, len(src)):
        if src[j] == "(":
            d += 1
        elif src[j] == ")":
            d -= 1
            if d == 0:
                return src[o + 1:j], j + 1
    return None, len(src)


def _guard_chain(src, pos):
    """Guards enclosing `pos`, outermost first, as (expression, negated) pairs.

    Walks outward. A position inside an `else` block is guarded by the negation
    of that if's condition, which is what lets `if(A) X else Y` be recognised as
    covering both sides of A.
    """
    chain, cur = [], pos
    while True:
        k = src.rfind("if(", 0, cur)
        k2 = src.rfind("if (", 0, cur)
        k = max(k, k2)
        if k < 0:
            break
        expr, after = _cond_of_if(src, k)
        if expr is None:
            break
        tb, te = _match_brace(src, after)
        if tb <= pos <= te:                      # inside the then-block
            chain.append((expr.strip(), False))
            cur = k
            continue
        tail = src[te + 1:].lstrip()
        if tail.startswith("else"):
            eo = te + 1 + (len(src[te + 1:]) - len(tail)) + 4
            rest = src[eo:].lstrip()
            if rest.startswith("if"):            # else-if: handled on its own turn
                eb, ee = eo, _cond_of_if(src, eo + (len(src[eo:]) - len(rest)))[1]
                _, ee = _match_brace(src, ee)
            else:
                eb, ee = _match_brace(src, eo)
            if eb <= pos <= ee or eo <= pos <= ee:
                chain.append((expr.strip(), True))
                cur = k
                continue
        cur = k
    chain.reverse()
    return chain


def _firing_expr(src):
    """The decision that makes this pattern FIRE, as one expression string.

    Collects the guard chain of every NON-ZERO assignment, factors the common
    prefix into conjuncts, and ORs the remainders -- simplifying `A | !A` to a
    tautology (which drops out) and `A | (!A & B)` to `A | B`. That one rule
    covers a flat decision, CDLENGULFING's if/else over two non-zero values,
    CDLHARAMI's else-if chain and CDLTRISTAR's sequential ifs alike.
    """
    locals_ = _loop_locals(src)
    chains = []
    for a in re.finditer(r"outInteger\s*\[\s*outIdx\+*\s*\]\s*=\s*([^;]+);", src):
        if a.group(1).strip().rstrip("0").strip() in ("", "+", "-"):
            continue
        c = _guard_chain(src, a.start())
        if not c:
            return None, "a non-zero assignment has no enclosing if"
        if any(re.search(r"\b%s\b" % re.escape(v), e) for e, _ in c for v in locals_):
            continue              # fires off remembered state, not off bar i
        chains.append(c)
    if not chains:
        return None, "no non-zero outInteger assignment found"

    n = 0
    while all(len(c) > n for c in chains) and len(set(c[n] for c in chains)) == 1:
        n += 1
    common = [e for e, neg in chains[0][:n]]
    if any(neg for _, neg in chains[0][:n]):
        return None, "the firing decision negates one of its own guards"
    rest = [c[n:] for c in chains]

    if any(len(r) == 0 for r in rest):
        alts = []                                 # one path is the bare prefix
    else:
        heads = set((r[0][0], r[0][1]) for r in rest)
        pos = set(e for e, neg in heads if not neg)
        neg = set(e for e, neg in heads if neg)
        if pos & neg:
            # A | (!A & ...) -- drop the negated copy, keep what follows it
            alts = []
            for r in rest:
                terms = [e for e, g in r if not (g and e in pos)]
                if terms:
                    alts.append(" && ".join("(%s)" % t for t in terms))
        else:
            alts = [" && ".join("(%s)" % e for e, g in r) for r in rest]
        if len(alts) <= 1 and len(rest) > 1:
            alts = []                             # the remainder is a tautology

    # Splice a guard in RAW unless it carries a top-level `||` of its own.
    # Wrapping everything would hide each guard's internal structure one
    # parenthesis deep, where the conjunct flattening and the arm detection
    # can no longer see it -- which silently emptied the arm map for every
    # pattern that had one.
    parts = [("(%s)" % e if len(_split_top(e, "||")) > 1 else e) for e in common]
    if alts:
        parts.append("(" + " || ".join("(%s)" % a for a in alts) + ")")
    if not parts:
        return None, "empty firing decision"
    return " && ".join(parts), None


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

    A group containing `||` is genuinely NOT flattenable and stays one condition,
    because a disjunct is not falsifiable while its sibling holds. Its interior is
    reached on a different axis: pb_signs() where the arms are output classes,
    and otherwise pb_disjuncts() plus a sole-true case per alternative. See
    count_disjuncts().
    """
    src = strip_comments(open(path, encoding="utf-8").read())


    # Walk back to the `if(` that guards it.
    expr, err = _firing_expr(src)
    if expr is None:
        return None, err

    # A decision that is ITSELF a disjunction is one condition -- the same
    # reading a nested `(B || C)` already gets. This used to decline, on the
    # view that such a pattern needed a whole branch axis; it does not. Its
    # alternatives are conjunctions, which is exactly what pb_arm() declares and
    # pb_flip_in() attributes a case to, and pb_signs() covers the selection
    # where the arms are colour-gated. The only thing that was missing was
    # letting the count through.
    if len(_split_top(expr, "||")) > 1:
        return 1, None

    total = 0
    for conj in _split_top(expr, "&&"):
        c = _peel(conj)
        if len(_split_top(c, "||")) > 1:
            total += 1                       # disjunction: one condition
        else:
            total += len(_split_top(c, "&&"))  # pure conjunction: flatten it
    return total, None


def count_disjuncts(path):
    """Map flattened-condition index -> number of alternatives, for the
    conditions that are disjunctions.

    count_conditions() stops at the top-level conjuncts, so `A && (B || C)` is
    two conditions and B and C are invisible to it. They are invisible to a flip
    as well: falsifying the condition falsifies every alternative at once, and
    the paired control only needs one of them back. An entire alternative can
    therefore be deleted from a pattern with the whole tier green -- which is not
    a hypothetical, it is what CDLLONGLEGGEDDOJI shipped with in unit 1.

    pb_signs() does not reach these. That axis is for a disjunction whose arms
    are output CLASSES and it works by requiring each class to fire; these arms
    emit the same value. The axis that reaches them is a firing case in which
    exactly one alternative holds, and this function is what pins the builder's
    pb_disjuncts() declaration of how many there are to cover.
    """
    src = strip_comments(open(path, encoding="utf-8").read())
    expr, err = _firing_expr(src)
    if expr is None:
        return None, err
    top = _split_top(expr, "||")
    if len(top) > 1:
        cols = [re.findall(r"ta_candlecolor\([^)]*\)\s*==\s*(-?1)", a) for a in top]
        gated = (all(len(x) >= 1 for x in cols) and
                 len(set(x[0] for x in cols)) == len(top))
        return ({} if gated else {0: len(top)}), None

    out, idx = {}, 0
    for conj in _split_top(expr, "&&"):
        c = _peel(conj)
        alts = _split_top(c, "||")
        if len(alts) > 1:
            # A disjunction whose alternatives are each gated on a DISTINCT
            # ta_candlecolor literal is already covered, on the output-class
            # axis: the arms are mutually exclusive (a candle is white or
            # black, never both) and each emits its own class, so pb_signs()
            # requiring every class to fire is exactly the sole-true property
            # this axis would otherwise assert. Demanding pb_disjuncts() there
            # would be a second declaration of one fact.
            #
            # The test is structural rather than "alternatives == signs",
            # which looks equivalent and is not: CDLGAPSIDESIDEWHITE is
            # bi-signed with a two-alternative disjunction that is NOT
            # colour-gated, and the arithmetic version would have exempted it
            # while its arms stayed unreachable.
            cols = [re.findall(r"ta_candlecolor\([^)]*\)\s*==\s*(-?1)", a)
                    for a in alts]
            gated = (all(len(x) >= 1 for x in cols) and
                     len(set(x[0] for x in cols)) == len(alts))
            if not gated:
                out[idx] = len(alts)
            idx += 1
        else:
            idx += len(_split_top(c, "&&"))
    return out, None


def count_arms(path):
    """Map flattened-condition index -> list of conjunct counts, one per
    alternative, for EVERY disjunction -- colour-gated or not.

    count_disjuncts() answers which alternatives need a sole-true case, and
    exempts the colour-gated ones because pb_signs already fires each class.
    That axis reaches arm SELECTION and stops there. An alternative is itself a
    conjunction, and its terms are reached by neither: a flip of the condition
    falsifies every alternative at once and names none of their terms, so an arm
    of eight conjuncts is satisfied by breaking any one and the other seven are
    asked for by nothing. Declaring the sizes is what lets the run-time check
    require a case per term, and pinning them here is what stops a builder from
    quietly not declaring them.
    """
    src = strip_comments(open(path, encoding="utf-8").read())
    expr, err = _firing_expr(src)
    if expr is None:
        return None, err
    top = _split_top(expr, "||")
    if len(top) > 1:
        return {0: [len(_split_top(_peel(a), "&&")) for a in top]}, None

    out, idx = {}, 0
    for conj in _split_top(expr, "&&"):
        c = _peel(conj)
        alts = _split_top(c, "||")
        if len(alts) > 1:
            out[idx] = [len(_split_top(_peel(a), "&&")) for a in alts]
            idx += 1
        else:
            idx += len(_split_top(c, "&&"))
    return out, None


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
    # The Hikkakes emit from remembered state on their confirmation path, which
    # is not a per-bar decision and gets no MC/DC builder -- see _loop_locals.
    # Count only what the per-bar detection can emit, and resolve the one hop
    # through the local it stores the result in: `patternResult` is assigned
    # 100 * (...) in the same block, so the class set is the ordinary one.
    locals_ = _loop_locals(src)
    classes = set()
    for cand in re.finditer(r"outInteger\s*\[\s*outIdx\+*\s*\]\s*=\s*([^;]+);", src):
        arm = " ".join(cand.group(1).split())
        if arm.strip().rstrip("0").strip() in ("", "+", "-"):
            continue                      # the non-firing `= 0` branch
        chain = _guard_chain(src, cand.start())
        if any(re.search(r"\b%s\b" % re.escape(v), e) for e, _ in chain for v in locals_):
            continue                      # fires off remembered state
        if arm in locals_:
            # The assignment that reaches here is the LAST one before this
            # output, not the first in the file -- these locals are initialised
            # to 0 above the loop, and matching that initialiser instead makes
            # the pattern look like it emits nothing.
            hits = list(re.finditer(r"(?m)^\s*%s\s*=\s*([^;]+);" % re.escape(arm),
                                    src[:cand.start()]))
            if not hits:
                return None, "cannot resolve the firing arm local: %s" % arm
            arm = " ".join(hits[-1].group(1).split())
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

    # build_<x>() ... pb_check_mcdc*("CDL<NAME>"  -- the call line pairs them.
    # The entry point is matched by prefix, not exactly: the penetration
    # patterns register through pb_check_mcdc_p, and a pattern demanding a
    # bare `(` silently dropped all five of them.
    pairs = re.findall(
        r"(build_\w+)\s*\(\s*\)\s*;\s*e\s*=\s*pb_check_mcdc\w*\(\s*\"([A-Z0-9_]+)\"",
        src)
    if not pairs:
        sys.exit("check_mcdc_conditions: found no pb_check_mcdc call sites")

    # Then fail CLOSED, and do it against the BUILDERS rather than against the
    # registrations. A builder this pairing never reaches is not reported as
    # skipped -- it simply is not printed, and the run still ends in OK, which
    # is how five functions went unchecked while the gate stayed green. Keying
    # the completeness check on the entry-point name would inherit whatever
    # blind spot the pairing has; keying it on `pb_conditions()` does not,
    # because that call is what the check exists to pin and no renaming of the
    # entry point can hide it.
    unpaired = sorted(
        fn for fn, body in re.findall(
            r"static void (build_\w+)\(\s*void\s*\)\s*\{(.*?)\n\}", src, re.S)
        if re.search(r"pb_conditions\(", body)
        and fn not in set(p[0] for p in pairs))
    if unpaired:
        sys.exit("check_mcdc_conditions: %d builder(s) declare pb_conditions() "
                 "but pair with no pb_check_mcdc* registration, so their "
                 "declared count is unchecked: %s"
                 % (len(unpaired), ", ".join(unpaired)))

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
        dj = dict((int(a), int(b)) for a, b in
                  re.findall(r"pb_disjuncts\(\s*(\d+)\s*,\s*(\d+)\s*\)", m.group(1)))
        am = {}
        for a, b, n in re.findall(
                r"pb_arm\(\s*(\d+)\s*,\s*(\d+)\s*,\s*(\d+)\s*\)", m.group(1)):
            am.setdefault(int(a), {})[int(b)] = int(n)
        am = dict((c2, [v[q] for q in sorted(v)]) for c2, v in am.items())
        declared[fn] = (int(d.group(1)), int(s.group(1)) if s else 1, dj, am)

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
        want, wantSigns, wantDisj, wantArms = declared[fn]
        signs, sErr = count_signs(pat)
        if signs is None:
            print("  %-20s FAIL  cannot parse firing arm: %s" % (name, sErr))
            bad += 1
            continue
        disj, dErr = count_disjuncts(pat)
        # A pattern the disjunct parser declines is one this check cannot speak
        # for; that is already reported by count_conditions above, and a declined
        # parse must not silently read as "no alternatives to cover".
        okDisj = (disj == wantDisj) if disj is not None else (not wantDisj)
        arms, aErr = count_arms(pat)
        okArms = (arms == wantArms) if arms is not None else (not wantArms)
        okCond, okSigns = want == actual, wantSigns == signs
        flag = "ok" if okCond and okSigns and okDisj and okArms else "MISMATCH"
        print("  %-20s %-8s declared %2d, source has %2d   signs: declared %d, "
              "source has %d%s" % (name, flag, want, actual, wantSigns, signs,
              "" if not (disj or wantDisj) else
              "   disjuncts: declared %s, source has %s"
              % (wantDisj or "{}", disj if disj is not None else "declined")))
        # Say out loud which functions the count speaks for only in part. A cap
        # nobody prints reads as full coverage; this one is real, and the two
        # patterns it applies to carry a whole second output class behind it.
        state = sorted(_loop_locals(open(pat).read()))
        if state:
            print("  %-20s          NOTE: an emitting path guarded by "
                  "loop-carried state (%s) is excluded -- it is not a decision "
                  "over bar i, so neither this count nor the MC/DC builder "
                  "covers it. Its gate is the legacy predicate scenarios."
                  % ("", ", ".join(state)))
        if not okArms:
            print("  %-20s          arms: declared %s, source has %s"
                  % ("", wantArms or "{}",
                     arms if arms is not None else "declined"))
        if not okCond or not okSigns or not okDisj or not okArms:
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
              "its sibling holds, so its interior is unreachable by flips. It "
              "is covered on one of two other axes instead -- the output class "
              "(pb_signs) where the arms are colour-gated, and pb_disjuncts() "
              "with a sole-true case per alternative otherwise, which is what a "
              "disjuncts mismatch means." % bad)
        return 1
    print("Every builder's pb_conditions() and pb_signs() matches its pattern "
          "source. OK.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
