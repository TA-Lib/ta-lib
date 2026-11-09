#!/usr/bin/env python3
"""Assert every candlestick reads each trailing total at the bar it belongs to.

A candlestick that compares a bar against a settings average carries one running
total per bar offset it needs -- `BodyLongPeriodTotal[1]` is the window ending at
`i-1`, `[0]` the one ending at `i` -- and the pattern is only correct if each
read pairs the total with its own bar. Cross two of them and the code still
compiles, still returns 0/+-100, and still computes an average of the right
setting over the right number of bars; it is simply the wrong ten bars.

Nothing else in the tree can see that, which is why this exists:

  - MC/DC cannot. `pb_primer` lays down k IDENTICAL bars, so every trailing
    window holds the same content by construction and the choice between them is
    unobservable in a builder's scenarios. Crossing the totals in CDLPIERCING,
    CDL3BLACKCROWS or CDL3WHITESOLDIERS -- three patterns with full MC/DC
    coverage, two of them from the adversarially reviewed foundation -- leaves
    the whole tier green.
  - The frozen-release fuzz (`ta_regtest --ref`) is only a partial net.
    Firing rate is why: over 600k bars of gapped random walk, CDLPIERCING fires
    999 times and CDL3BLACKCROWS fires ONCE, so a threshold that moves by one
    bar of window almost never moves a 3-valued output.

Both of those are properties of the input data, and no choice of scenario or
seed fixes them in general. The wiring, though, is a property of the SOURCE, so
check it there and the data stops mattering:

    a total is read at exactly one bar offset, and where the total is an array
    its subscript IS that offset.

That holds for every candlestick in the tree today -- 16 of the 61 read a single
setting at more than one window, and none of them violates it -- so this is a
pin on something already true, not a new convention being imposed.

Both tiers are covered, because both wire the same totals:

    batch/dispatch  TA_CANDLEAVERAGE_OF(BodyLong_rangeType,BodyLong_avgPeriod,
                                        BodyLong_factor,BodyLongPeriodTotal[1],i - 1)
    streaming       TA_STREAM_CANDLEAVERAGE(BodyLong,sp->BodyLongPeriodTotal[1],
                                            sp->lag1_inOpen,...)

The streaming tier names its bar with a `lagN_` prefix instead of an `i` offset
(and no prefix means the current bar, lag 0), so it needs its own pattern but
the same rule.

Scope: the generated C in src/ta_func, which is the shipped library and the
golden every cross-language gate compares against. A generator defect reaches
all four backends identically -- that is the blindness #219 exists for -- so one
backend is enough to catch it. A per-backend emission defect is not covered
here; that is --xlang-hash's territory.

Run standalone, or via `scripts/build.py check-candle-windows`.
"""

import glob
import os
import re
import sys

FUNC_DIR = os.path.join("src", "ta_func")

# TA_CANDLEAVERAGE(SET, TOTAL[k], i - m), or the settings-hoisted
# TA_CANDLEAVERAGE_OF(SET_rangeType, SET_avgPeriod, SET_factor, TOTAL[k], i - m)
# -- k and m both optional. The hoisted form names the setting three times, and
# all three must agree: a total averaged over one setting's period and scaled by
# another's factor is the same silent miswiring as a crossed bar.
BATCH = re.compile(r"TA_CANDLEAVERAGE(?:_OF)?\(\s*(\w+?)(?:_rangeType)?\s*,"
                   r"(?:\s*(\w+?)_avgPeriod\s*,\s*(\w+?)_factor\s*,)?"
                   r"\s*(\w+?)(?:\[(\d+)\])?\s*,\s*i(?:\s*-\s*(\d+))?\s*\)")
_RANGE = r"TA_CANDLERANGE(?:_OF)?\(\s*(\w+?)(?:_rangeType)?\s*,"
# TA_STREAM_CANDLEAVERAGE(SET, sp->TOTAL[k], [sp->lagN_]inOpen, ...)
STREAM = re.compile(r"TA_STREAM_CANDLEAVERAGE\(\s*(\w+)\s*,\s*sp->(\w+?)"
                    r"(?:\[(\d+)\])?\s*,\s*(?:sp->lag(\d+)_)?inOpen")


# ---------------------------------------------------------------------------
# The WRITE side (issue #240)
# ---------------------------------------------------------------------------
#
# Everything above checks which bar a total is COMPARED to. Which bar goes INTO
# it is the other half, and nothing checked it. The #229 window fold rewrote
# exactly that -- the per-bar add and the trailing subtract both became reads of
# a derived ring -- and this gate reported OK for a one-bar rotation in either
# direction, while `stream_verify` caught it on 3 of the 14 functions and the
# other 11 stayed green in every language.
#
# A trailing total is a sliding window of `avgPeriod` bars ending one bar before
# the bar it is compared to. Three statements set it up, and each pins one
# property of the window, so the three check each other with no data at all:
#
#     TR = startIdx - c - <Set>_avgPeriod;          declares the window's start
#     while( i < startIdx - t )  TOT += RANGE(Set, i - w);      fills it
#     TOT += RANGE(Set, i - a) - RANGE(Set, TR - b);            slides it
#
#   LENGTH   the fill runs `(startIdx - t) - TR` times, which is `avgPeriod`
#            exactly when `t == c`.
#   DROP     the first slide must drop the first bar the fill added, which is
#            `TR - w`; so `b == w`.
#   END      the slide adds the bar the total is compared to, so `a` is the
#            read offset -- for an array total that is its subscript, which the
#            read side above already pairs.
#
# The streaming tier repeats the last two against the same numbers: `ringLag`
# IS `c + avgPeriod`, so a trailing read `ringLag + b` slots back names the same
# bar the batch drops, and the per-bar push is at lag `a`.
#
# What is deliberately NOT checked here: the bar the sliding loop STARTS at.
# Getting it wrong shifts the whole output range, which every differential gate
# sees on the first bar of every series -- unlike a window's contents, which
# only a threshold crossing reveals.

# `BodyLongTrailingIdx = startIdx - 2 - BodyLong_avgPeriod;` (the `- 2` optional)
TRAIL = re.compile(
    r"\b(\w+TrailingIdx)\s*=\s*startIdx\s*-\s*(?:(\d+)\s*-\s*)?(\w+)_avgPeriod\s*;")

# The fill loop, whole: `i = TR; while( i < startIdx - t ) { ... }`. Its body
# holds one add per total the loop fills, so it is parsed as a block.
FILL_LOOP = re.compile(
    r"i\s*=\s*(\w+TrailingIdx)\s*;\s*while\(\s*i\s*<\s*startIdx\s*(?:([-+])\s*(\d+))?\s*\)\s*\{"
    r"(.*?)\n\s*\}", re.S)

_OFF = r"(?:\s*([-+])\s*(\w+))?"

# `TOT[k] = TOT[k] + (RANGE(Set,i - a) - RANGE(Set,TR - b));`, plus the `TOT +=`
# form, the fill form with no subtract, and every offset omitted.
BATCH_WRITE = re.compile(
    r"\b(\w*PeriodTotal\d?)(?:\[(\w+)\])?\s*(?:\+=|=\s*\1(?:\[(\w+)\])?\s*\+)\s*\(?\s*"
    + _RANGE + r"\s*i" + _OFF + r"\s*\)"
    r"(?:\s*-\s*" + _RANGE + r"\s*(\w+)" + _OFF + r"\s*\))?")

# The derived ring's own per-bar push -- what the ring actually holds.
RING_PUSH = re.compile(
    r"sp->ring_(\w+)_derived\[sp->ringPos_\1\]\s*=\s*TA_STREAM_CANDLERANGE\(\s*(\w+)\s*,")

# `sp->TOT[k] += TA_STREAM_CANDLERANGE(Set,[sp->lagL_]inOpen,...) - sp->ring_TR_derived[slot];`
# Update frames only: a peek stops at its last output store and this write is
# below it, so no peek frame carries one.
STREAM_WRITE = re.compile(
    r"sp->(\w*PeriodTotal\d?)(?:\[(\w+)\])?\s*(?:\+=|=\s*sp->\1(?:\[(\w+)\])?\s*\+)\s*\(?\s*"
    r"TA_STREAM_CANDLERANGE\(\s*(\w+)\s*,\s*(?:sp->lag(\d+)_)?inOpen[^)]*\)"
    r"\s*-\s*sp->ring_(\w+)_derived\[([^\]]*)\]")

# The #229 folded form: BOTH terms read one derived ring -- the add
# cursor-relative, the subtract through the runtime lag. The generator emits it
# only for a body that rescans a window every bar, which no candlestick does
# today, so it has no floor; it stays parsed so the form is checked if it returns.
STREAM_FOLD = re.compile(
    r"sp->(\w*PeriodTotal\d?)\[(?:sp->)?(\w+)\]\s*=\s*sp->\1\[(?:sp->)?\2\]\s*\+\s*\(\s*"
    r"sp->ring_(\w+)_derived\[([^\]]*)\]\s*-\s*sp->ring_(\w+)_derived\[([^\]]*)\]\s*\)")

# The CARRIED form: each bar computes one new range per total and carries the
# others in variables rotated once per bar, so the slide names no bar:
#
#     r2 = RANGE(Set, i - 2);  t2 = RANGE(Set, TR - 2);    before the loop
#     r0 = RANGE(Set, i);      t0 = RANGE(Set, TR);         each bar
#     TOT[2] = TOT[2] + (r2 - t2);
#     r2 = r1;  r1 = r0;  t2 = t1;  t1 = t0;                the rotation
#
# Which bar a carried name holds at the slide is resolved by walking the bar's
# statements in order from its value at loop entry; the rotation must then hand
# the next bar the same offsets, which is what makes the walk an invariant. The
# streaming tier carries the same names in `sp->` fields, entered with what the
# open captured from the batch loop, and computes the fresh ones from lags and
# the trailing ring.
_N = r"((?:sp->)?\w+)"
_LHS = r"(?<![\w>.])" + _N + r"\s*=(?!=)"
CARRIED_SLIDE = re.compile(
    r"(?<![\w>.])((?:sp->)?\w*PeriodTotal\d?)(?:\[(\w+)\])?\s*(?:\+=|=\s*\1(?:\[(\w+)\])?\s*\+)"
    r"\s*\(?\s*" + _N + r"\s*-\s*" + _N + r"\s*\)?\s*;")
CARRIED_DEF = re.compile(_LHS + r"\s*" + _RANGE + r"\s*(\w+)" + _OFF + r"\s*\)\s*;")
STREAM_DEF = re.compile(_LHS + r"\s*TA_STREAM_CANDLERANGE\(\s*(\w+)\s*,"
                        r"\s*(?:sp->lag(\d+)_)?inOpen[^)]*\)\s*;")
RING_DEF = re.compile(_LHS + r"\s*sp->ring_(\w+)_derived\[([^\]]*)\]\s*;")
COPY = re.compile(_LHS + r"\s*" + _N + r"\s*;")
_STEP = re.compile(r"_StepImpl\(")
ASSIGN = re.compile(r"(?<![\w>.])" + _N + r"\s*[-+*/]?=(?!=)")

# Any write to a total: the line scan in main() fails on one that is neither a
# form above nor a reset or capture.
TOTAL_WRITE = re.compile(r"(?<![\w>.])(?:sp->)?\w*PeriodTotal\d?(?:\[\w+\])?\s*"
                         r"(?:[-+*/]?=(?!=)|\+\+|--)")
TOTAL_RESET = re.compile(r"(?:double\s+)?\w*PeriodTotal\d?(?:\[\w+\])?\s*=\s*"
                         r"(?:0|0\.0|\{\s*0\s*\})\s*;")
TOTAL_RESET_AT = re.compile(r"(?<![\w>.])(double\s+)?(\w*PeriodTotal\d?)(?:\[(\w+)\])?"
                            r"\s*=\s*(?:0|0\.0|\{\s*0\s*\})\s*;")
TOTAL_CAPTURE = re.compile(r"sp->(\w*PeriodTotal\d?)\s*=\s*\1\s*;")

# ---------------------------------------------------------------------------
# Range-type copies
# ---------------------------------------------------------------------------
#
# A body may be emitted once per range type under `if( Set_rangeType ==
# TA_RangeType_X && ... )`, its macros naming the range type instead of the
# setting. Inside such a block the literal is mapped back to the settings the
# condition pins to it: a range macro names only a range type, so it may stand
# for any setting pinned to that type (they compute the same value), while an
# average must still name its own setting through `_avgPeriod`, and is mapped
# only when the block pins that setting to the literal. A literal the block
# does not pin stays as it is, and fails every setting comparison below.
_PIN_TERM = r"\w+_rangeType\s*==\s*TA_RangeType_\w+"
_PIN_IF = re.compile(
    rf"\bif\(\s*({_PIN_TERM}(?:\s*&&\s*{_PIN_TERM})*)\s*\)\s*\{{")
_PIN = re.compile(r"(\w+)_rangeType\s*==\s*(TA_RangeType_\w+)")
_LITERAL = re.compile(r"(TA_CANDLE(?:RANGE|AVERAGE)_OF\(\s*)(TA_RangeType_\w+)"
                      r"(\s*,\s*(?:(\w+?)_avgPeriod\b)?)")
_OR = "__or__"


def _block_end(src, brace):
    """Index just past the `}` matching the `{` at `brace`."""
    depth = 0
    for k in range(brace, len(src)):
        if src[k] == "{":
            depth += 1
        elif src[k] == "}":
            depth -= 1
            if depth == 0:
                return k + 1
    return len(src)


def unfold_range_types(src):
    out, pos = [], 0
    for m in _PIN_IF.finditer(src):
        if m.start() < pos:
            continue
        pins = set(_PIN.findall(m.group(1)))
        end = _block_end(src, m.end() - 1)

        def name(mm):
            lit, avg = mm.group(2), mm.group(4)
            if avg:
                setting = avg if (avg, lit) in pins else lit
            else:
                setting = _OR.join(sorted(s for s, t in pins if t == lit)) or lit
            return mm.group(1) + setting + mm.group(3)
        out.append(src[pos:m.end()])
        out.append(_LITERAL.sub(name, src[m.end():end]))
        pos = end
    out.append(src[pos:])
    return "".join(out)


def fold_ring_locals(src):
    """A step may advance a ring cursor in a local: load it, use it, wrap it,
    store it last. Spell that local as the field it mirrors, so the rules below
    see one form. Folded only when the field is untouched between the load and
    the store; any other shape is left as written and fails the cursor rule."""
    for name in sorted(set(re.findall(r"\bint\s+ringPosL_(\w+)\s*;", src))):
        local, field = r"\bringPosL_%s\b" % name, "sp->ringPos_%s" % name
        load = re.compile(r"(?<![\w>.])ringPosL_%s\s*=\s*sp->ringPos_%s\s*;" % (name, name))
        store = re.compile(r"sp->ringPos_%s\s*=\s*ringPosL_%s\s*;" % (name, name))
        out, pos = [], 0
        for ld in load.finditer(src):
            st = store.search(src, ld.end())
            if (ld.start() < pos or st is None
                    or re.search(r"sp->ringPos_%s\b" % name, src[ld.end():st.start()])
                    or load.search(src, ld.end(), st.start())):
                continue
            out.append(src[pos:ld.start()])
            out.append(re.sub(local, field, src[ld.end():st.start()]))
            pos = st.end()
        out.append(src[pos:])
        src = re.sub(r"\bint\s+ringPosL_%s\s*;" % name, "", "".join(out))
    return src


def _blank_comments(src):
    """Comments as spaces, newlines kept: code in a comment is not code, and the
    statement and brace scans below must not see one."""
    return re.sub(r"/\*.*?\*/|//[^\n]*",
                  lambda m: re.sub(r"[^\n]", " ", m.group(0)), src, flags=re.S)


def _same(a, b):
    """Two setting names agree; a range-type copy's name may stand for several."""
    return bool(set(a.split(_OR)) & set(b.split(_OR)))


def _off(sign, tok):
    """An offset as BARS BACK: `i - 2` -> 2, `TR + 1` -> -1, bare `i` -> 0. A
    variable offset stays its token, so two of them compare by identity."""
    if not tok:
        return 0
    if tok.isdigit():
        return int(tok) if sign == "-" else -int(tok)
    if sign != "-":
        return "+" + tok            # `i + var` never occurs; keep it distinct
    return tok


def _lag_slot(expr, ring):
    """A trailing ring read -> its offset past the runtime lag, in bars back.

    Two layouts, and which one is emitted is not a free choice: a ring nothing
    reads at a shift keeps the legacy oldest-slot form (`ring[pos]`, capacity ==
    lag), one with a shifted read switches to the absolute-mod form and grows a
    `ringLag` field. Both name bar `cursor - ringLag - b`.
    """
    e = " ".join(expr.split())
    if e == "sp->ringPos_%s" % ring:
        return 0
    head = "(sp->ringPos_{0} + sp->ringCap_{0} - sp->ringLag_{0}".format(ring)
    tail = ") % sp->ringCap_{0}".format(ring)
    if not (e.startswith(head) and e.endswith(tail)):
        return None
    mid = e[len(head):len(e) - len(tail)].strip()
    if mid == "":
        return 0
    m = re.fullmatch(r"([-+])\s*(\S+)", mid)
    if not m:
        return None
    return _off(m.group(1), m.group(2).replace("sp->", ""))


def _cursor_slot(expr, ring):
    """A cursor-relative (de-moduloed) ring read -> the offset it reads back, in
    bars back. Slot `pos` holds the current bar, so the offset IS the lag. This
    is the expression `backend_suite` pins for CDL3BLACKCROWS; here the whole
    corpus is held to the same shape, which is the point of a gate."""
    e = " ".join(expr.split())
    p, c = "sp->ringPos_%s" % ring, "sp->ringCap_%s" % ring
    m = re.fullmatch(r"\(%s \+ %s - (\S+) >= %s\) \? (.*)" % (re.escape(p), re.escape(c),
                                                              re.escape(c)), e)
    if not m:
        return None
    w, rest = m.group(1), m.group(2)
    if rest != "{p} + {c} - {w} - {c} : {p} + {c} - {w}".format(p=p, c=c, w=w):
        return None
    return _off("-", w.replace("sp->", ""))


def _sub_matches(sub, back):
    """A subscripted total accumulates at the bar its subscript names."""
    if sub == "":
        return True
    if sub.isdigit():
        return back == int(sub)
    return back == sub


def _functions(src):
    """(start, end) of each top-level function body: `{` and `}` at column 0."""
    spans, close = [], re.compile(r"^\}$", re.M)
    for m in re.finditer(r"^\{$", src, re.M):
        e = close.search(src, m.end())
        spans.append((m.start(), e.end() if e else len(src)))
    return spans


def _do_loops(src, lo, hi):
    """(do, body start, body end) of each `do { ... }` in [lo, hi)."""
    loops = []
    for m in re.compile(r"\bdo\s*\{").finditer(src, lo, hi):
        loops.append((m.start(), m.end(), _block_end(src, m.end() - 1) - 1))
    return loops


# Every pass runs: no condition beyond the subscript's own bound.
TOTAL_LOOP = re.compile(r"\(\s*totIdx\s*=\s*\d+\s*;\s*totIdx\s*>=\s*0\s*;\s*(?:totIdx\s*-=\s*1|totIdx--|--totIdx)\s*\)")
LEAVES = re.compile(r"\b(?:if|break|continue|return|goto)\b")


def _total_loop(src, lo, brace):
    """The block opened at `brace` is the body of a `for` over `totIdx`."""
    j = len(src[lo:brace].rstrip()) + lo - 1
    if j < lo or src[j] != ")":
        return False
    depth = 0
    while j >= lo:
        depth += {")": 1, "(": -1}.get(src[j], 0)
        if depth == 0:
            break
        j -= 1
    f = re.search(r"\bfor\s*$", src[lo:j])
    if f is None or TOTAL_LOOP.match(src, j) is None:
        return False
    head = src[lo:lo + f.start()].rstrip()
    return head == "" or head[-1] in ";{}"


def _guarded(src, lo, pos):
    """The statement at `pos` runs only under a condition: it sits in a nested
    block other than a loop over the totals' subscript, or something (`if( .. )`,
    `else`) precedes it in its own statement."""
    opened = []
    for k in range(lo, pos):
        if src[k] == "{":
            opened.append(k)
        elif src[k] == "}" and opened:
            opened.pop()
    if any(not _total_loop(src, lo, b) or LEAVES.search(src, b, pos) for b in opened):
        return True
    k = max(src.rfind(";", lo, pos), src.rfind("{", lo, pos), src.rfind("}", lo, pos))
    return src[max(k + 1, lo):pos].strip() != ""


def _events(src, lo, hi, tracked, pushes, problems, where):
    """The assignments to `tracked` names and the carried slides in [lo, hi), in
    order, `lo` being the top level of the block. A value is (setting, base,
    back): base `i` for the bar side, else the trailing index (or ring) it reads
    behind; None when the gate cannot say."""
    ev = []
    for m in ASSIGN.finditer(src, lo, hi):
        name = m.group(1)
        if name not in tracked:
            continue
        if _guarded(src, lo, m.start()):
            problems.append("%s: %s is assigned under a condition -- a form this "
                            "gate cannot follow" % (where, name))
            ev.append((m.start(), "set", name, None))
            continue
        stmt = src[m.start():src.find(";", m.end(), hi) + 1]
        d, f = CARRIED_DEF.fullmatch(stmt), STREAM_DEF.fullmatch(stmt)
        r, c = RING_DEF.fullmatch(stmt), COPY.fullmatch(stmt)
        val = None
        if d:
            val = (d.group(2), d.group(3), _off(d.group(4), d.group(5)))
        elif f:
            val = (f.group(2), "i", int(f.group(3) or 0))
        elif r:
            b = _lag_slot(r.group(3), r.group(2))
            if b is not None and r.group(2) in pushes:
                val = (pushes[r.group(2)], r.group(2), b)
        elif c and c.group(2) in tracked:
            ev.append((m.start(), "copy", name, c.group(2)))
            continue
        ev.append((m.start(), "set", name, val))
    for m in CARRIED_SLIDE.finditer(src, lo, hi):
        if _guarded(src, lo, m.start()):
            problems.append("%s: %s[%s] slides under a condition -- a form this "
                            "gate cannot follow"
                            % (where, m.group(1), m.group(2) or m.group(3) or ""))
        ev.append((m.start(), "slide", m))
    return sorted(ev, key=lambda e: e[0])


def _walk(events, env):
    """Run one bar's events from `env`; return the slides resolved against it."""
    slides = []
    for e in events:
        if e[1] == "set":
            env[e[2]] = e[3]
        elif e[1] == "copy":
            env[e[2]] = env.get(e[3])
        else:
            m = e[2]
            slides.append((m, env.get(m.group(4)), env.get(m.group(5))))
    return slides


def _bar(v):
    return v[1] if v[2] == 0 else "%s-%s" % v[1:]


def _rotation_problems(entry, env, where):
    """Each carried name must leave the bar one bar fresher than it entered, so
    the next bar finds it at the same offset."""
    problems = []
    for name, v in sorted(entry.items()):
        got = env.get(name)
        if v is None or not isinstance(v[2], int):
            continue
        if got is None or got[1:] != (v[1], v[2] - 1):
            problems.append("%s: %s enters a bar holding %s and leaves it holding "
                            "%s, not %s" % (where, name, _bar(v),
                                            _bar(got) if got else "nothing",
                                            _bar((None, v[1], v[2] - 1))))
    return problems


def scan_writes(src, read_sets, read_offs):
    """Return (problems, counts) for the write side of one file."""
    problems = []
    counts = {"batch": 0, "stream": 0, "fold": 0, "fill": 0,
              "carried": 0, "carried_stream": 0}

    trails = {}
    for tr, c, setting in TRAIL.findall(src):
        c = int(c) if c else 0
        prev = trails.setdefault(tr, (setting, c))
        if prev != (setting, c):
            problems.append("%s is declared as both %s-%d and %s-%d"
                            % (tr, prev[0], prev[1], setting, c))
    pushes = dict(RING_PUSH.findall(src))

    def want_set(total, sub, setting, where):
        if sub != "" and not sub.isdigit():
            # A loop-driven slide covers several subscripts at once; require its
            # setting to be one the total is actually read with.
            known = {v for (t, _k), v in read_sets.items() if t == total}
            if known and not any(_same(setting, k) for k in known):
                problems.append("%s[%s] %s %s, which the total is never read "
                                "against" % (total, sub, where, setting))
            return
        want = read_sets.get((total, sub))
        if want is not None and not _same(setting, want):
            problems.append("%s[%s] %s %s but the total is read against %s"
                            % (total, sub, where, setting, want))

    def paced(tot, sub, tr, where):
        # A range-type copy's range macro may name several settings, so the
        # trailing index is held to the setting the total's average names.
        own = read_sets.get((tot, sub))
        if own is not None and trails[tr][0] != own:
            problems.append("%s[%s] is read against %s but %s %s, which is paced "
                            "by %s_avgPeriod" % (tot, sub, own, where, tr,
                                                 trails[tr][0]))

    # --- the fill loops: window LENGTH, and the bar each total starts from ---
    fill = {}                       # (total, subscript) -> bars back
    fill_c = {}                     # (total, subscript) -> its trailing base
    fill_set = {}                   # (total, subscript) -> the setting it holds
    for tr, tsign, ttok, body in FILL_LOOP.findall(src):
        if tr not in trails:
            problems.append("fill loop walks %s, which no "
                            "`startIdx - c - <Set>_avgPeriod` declares" % tr)
            continue
        setting, c = trails[tr]
        t = int(ttok) if ttok else 0
        if tsign == "+":
            t = -t
        if t != c:
            problems.append("the fill loop for %s runs to startIdx-%d but %s starts "
                            "%d before startIdx-avgPeriod, so it accumulates %d bars, "
                            "not avgPeriod" % (tr, t, tr, c, c - t))
        for m in BATCH_WRITE.finditer(body):
            tot, sub_l, sub_r, aset, asign, atok, bset, _tr, _bs, _bt = m.groups()
            if bset is not None:
                continue            # not a fill add
            counts["fill"] += 1
            sub = sub_l or sub_r or ""
            back = _off(asign, atok)
            want_set(tot, sub, aset, "is filled with")
            if sub.isdigit() and back != int(sub):
                problems.append("%s[%s] is filled from the bar %s back"
                                % (tot, sub, back))
            paced(tot, sub, tr, "is filled walking")
            fill_c[(tot, sub)] = c
            fill_set[(tot, sub)] = aset
            prev = fill.setdefault((tot, sub), back)
            if prev != back:
                problems.append("%s[%s] is filled from two different bars (%s, %s)"
                                % (tot, sub, prev, back))

    # --- the sliding writes: which bar in, which bar out ---------------------
    slide = {}                      # (total, subscript) -> (add back, drop back)
    align = {}                      # sliding-loop origin -> the totals implying it

    def check_slide(tot, sub, aset, add, bset, tr, drop):
        if not _sub_matches(sub, add):
            problems.append("%s[%s] adds the bar %s back" % (tot, sub, add))
        if sub == "" and read_offs.get((tot, "")) not in (None, {add}):
            problems.append("%s adds the bar %s back but is read at %s"
                            % (tot, add, sorted(read_offs[(tot, "")])))
        want_set(tot, sub, bset, "subtracts")
        if tr not in trails:
            problems.append("%s subtracts through %s, which no "
                            "`startIdx - c - <Set>_avgPeriod` declares" % (tot, tr))
            return
        if not _same(aset, trails[tr][0]):
            problems.append("%s (%s) subtracts through %s, which is paced by "
                            "%s_avgPeriod" % (tot, aset, tr, trails[tr][0]))
        paced(tot, sub, tr, "drops through")
        slide.setdefault((tot, sub), (add, drop))
        # The first slide must drop the first bar the fill added.
        want = fill.get((tot, sub))
        if want is None and sub != "" and not sub.isdigit():
            # A loop-driven slide (`TOT[totIdx]`) adds and drops at the loop
            # counter, so for every subscript it covers the fill must have used
            # that subscript as its offset -- which the fill scan required of
            # each concrete entry. All that is left is that it covers some.
            covered = [k for (t, k) in fill if t == tot and k.isdigit()
                       and _same(fill_set.get((t, k), ""), aset)]
            if covered:
                want = drop
                align.setdefault(fill_c.get((tot, covered[0]), 0), []).append(
                    "%s[%s]" % (tot, sub))
        if want is None:
            problems.append("%s[%s] slides but nothing fills it" % (tot, sub))
        elif want != drop:
            problems.append("%s[%s] drops the bar %s back but was filled from %s back"
                            % (tot, sub, drop, want))
        # Every window in a file is anchored to the SAME sliding loop, so
        # `startIdx - c - w + a` -- the bar that loop starts at -- must come out
        # the same for all of them. This is what ties the fill offset to the
        # slide offset when the fill walks a shifted `i` range (CDL2CROWS fills
        # at `i` and slides at `i - 2`).
        if want is not None and isinstance(want, int) and isinstance(add, int):
            align.setdefault(fill_c.get((tot, sub), 0) + want - add, []).append(
                "%s[%s]" % (tot, sub))

    for m in BATCH_WRITE.finditer(src):
        (tot, sub_l, sub_r, aset, asign, atok,
         bset, tr, bsign, btok) = m.groups()
        counts["batch"] += 1
        sub = sub_l or sub_r or ""
        if sub_l and sub_r and sub_l != sub_r:
            problems.append("%s[%s] accumulates into %s[%s]" % (tot, sub_r, tot, sub_l))
        want_set(tot, sub, aset, "adds")
        if bset is None:
            continue                # a fill add, handled above
        check_slide(tot, sub, aset, _off(asign, atok), bset, tr, _off(bsign, btok))

    # --- the carried slides: resolve each name to its bar, then check alike ---
    tracked = set()
    for m in CARRIED_SLIDE.finditer(src):
        tracked.update(m.group(4, 5))
    names = "|".join(["i"] + sorted(trails))
    idx = re.compile(r"(?<![\w>.])(?:(%s)\s*(?:[-+*/]?=(?!=)|\+\+|--)|(?:\+\+|--)\s*(%s)\b)"
                     % (names, names))
    batch_slides, stream_slides, steps, bodies = [], [], [], []
    seen = set()
    inits = {}                      # carried name -> its loop-entry value
    split = set()
    captures = []                   # (where, {sp->name: value}) after each loop
    for flo, fhi in _functions(src):
        if _STEP.search(src, src.rfind("\n", 0, flo - 1), flo):
            steps.append((flo, fhi))
            continue
        loops = _do_loops(src, flo, fhi)
        for k, (do, blo, bhi) in enumerate(loops):
            where = "line %d" % (src.count("\n", 0, do) + 1)
            fills = list(FILL_LOOP.finditer(src, loops[k - 1][2] if k else flo, do))
            filled = set()
            for f in fills:
                for w in BATCH_WRITE.finditer(src, f.start(4), f.end(4)):
                    if w.group(7) is None:
                        filled.add((w.group(1), w.group(2) or w.group(3) or ""))
                        if _guarded(src, f.start(4), w.start()):
                            problems.append("%s: %s[%s] is filled under a condition"
                                            % (where, w.group(1),
                                               w.group(2) or w.group(3) or ""))
            lo = max([loops[k - 1][2] if k else flo] + [f.end() for f in fills])
            entry = {}
            for e in _events(src, lo, do, tracked, pushes, problems, where):
                if e[1] != "set":
                    continue
                if idx.search(src, e[0], do):
                    problems.append("%s: %s is initialized before its loop's "
                                    "index is set" % (where, e[2]))
                entry[e[2]] = e[3]
                if e[3] is not None:
                    prev = inits.setdefault(e[2], e[3])
                    if prev[1:] != e[3][1:] and e[2] not in split:
                        split.add(e[2])
                        problems.append("%s is initialized to both %s and %s"
                                        % (e[2], _bar(prev), _bar(e[3])))
            # Every offset in the bar is read against `i` and the trailing
            # indexes as the bar entered: each may move once, by one, after its
            # last read.
            moves = {}
            for x in idx.finditer(src, blo, bhi):
                moves.setdefault(x.group(1) or x.group(2), []).append(x)
            for name, xs in sorted(moves.items()):
                stmt = src[xs[0].start():src.find(";", xs[0].start(), bhi) + 1]
                if len(xs) > 1:
                    problems.append("%s: %s moves %d times per bar, not once"
                                    % (where, name, len(xs)))
                elif _guarded(src, blo, xs[0].start()) or not re.fullmatch(
                        r"\w+\s*(?:\+=\s*1|\+\+)\s*;|\+\+\s*\w+\s*;", stmt):
                    problems.append("%s: %s moves other than unconditionally by one "
                                    "-- a form this gate cannot follow" % (where, name))
                elif re.compile(r"(?<![\w>.])%s\b" % name).search(src, xs[0].end(), bhi):
                    problems.append("%s: %s moves before the bar's last read of it"
                                    % (where, name))
            env = dict(entry)
            bodies.append((blo, bhi))
            slid = {}
            for w in BATCH_WRITE.finditer(src, blo, bhi):
                if w.group(7) is not None:
                    key = (w.group(1), w.group(2) or w.group(3) or "")
                    slid[key] = slid.get(key, 0) + 1
                    if _guarded(src, blo, w.start()):
                        problems.append("%s: %s[%s] slides under a condition" % (
                            (where,) + key))
            for m, a, b in _walk(_events(src, blo, bhi, tracked, pushes, problems,
                                         where), env):
                seen.add(m.start())
                if not m.group(1).startswith("sp->"):
                    batch_slides.append((where, m, a, b))
                    key = (m.group(1), m.group(2) or m.group(3) or "")
                    slid[key] = slid.get(key, 0) + 1
            for (t, s), n in list(slid.items()):
                if s and not s.isdigit():   # a loop-driven slide covers them all
                    for ft, fs in filled:
                        if ft == t:
                            slid[(ft, fs)] = slid.get((ft, fs), 0) + n
                    del slid[(t, s)]
            for key in sorted(filled | set(slid)):
                if key not in filled:
                    problems.append("%s: %s[%s] slides in a loop no fill of its "
                                    "own precedes" % ((where,) + key))
                elif slid.get(key, 0) != 1:
                    problems.append("%s: %s[%s] slides %d time(s) per bar, not once"
                                    % ((where,) + key + (slid.get(key, 0),)))
            problems.extend(_rotation_problems(entry, env, where))
            cap = {}
            for m in COPY.finditer(src, bhi, loops[k + 1][0] if k + 1 < len(loops)
                                   else fhi):
                if m.group(1).startswith("sp->") and m.group(2) in tracked:
                    cap[m.group(1)] = entry.get(m.group(2))
            if cap:
                captures.append((where, cap))

    # The stream step enters with what the open captured after its loop, which
    # the batch rotation check has proved still holds each name's loop-entry
    # offset -- after EVERY copy of the loop, since any of them may open it.
    captured = {}
    for n in sorted({n for _w, cap in captures for n in cap}):
        votes = {}
        for _w, cap in captures:
            v = cap.get(n)
            votes.setdefault(v[1:] if v else None, []).append(v)
        best = max(votes, key=lambda k: (len(votes[k]), k is not None))
        captured[n] = votes[best][0]
        for where, cap in captures:
            v = cap.get(n)
            if n in cap and (v[1:] if v else None) != best:
                problems.append("%s: the open captures %s as %s here but as %s "
                                "after the other loops" % (
                                    where, n, _bar(v) if v else "nothing",
                                    _bar(captured[n]) if best else "nothing"))
    for where, cap in captures:
        for n in sorted(set(captured) - set(cap)):
            problems.append("%s: the open never captures %s after this loop"
                            % (where, n))
    for flo, fhi in steps:
        where = "line %d" % (src.count("\n", 0, flo) + 1)
        entry = {n: v for n, v in captured.items() if n in tracked}
        env = dict(entry)
        for m, a, b in _walk(_events(src, flo + 1, fhi, tracked, pushes, problems,
                                     where), env):
            seen.add(m.start())
            if m.group(1).startswith("sp->"):
                stream_slides.append((where, m, a, b))
        problems.extend(_rotation_problems(entry, env, where))
    for m in CARRIED_SLIDE.finditer(src):
        if m.start() not in seen:
            problems.append("line %d: %s slides by carried names outside any loop "
                            "this gate follows" % (src.count("\n", 0, m.start()) + 1,
                                                   m.group(1)))

    def resolved(where, m, a, b):
        tot, sub_l, sub_r, an, bn = m.groups()
        sub = sub_l or sub_r or ""
        if sub_l and sub_r and sub_l != sub_r:
            problems.append("%s: %s[%s] accumulates into %s[%s]"
                            % (where, tot, sub_r, tot, sub_l))
            return None
        for n, v, want in ((an, a, "the bar side"), (bn, b, "a trailing index")):
            if v is None or (v[1] == "i") != (n == an):
                problems.append("%s: %s[%s] slides by %s, which does not hold %s "
                                "this gate can name" % (where, tot, sub, n, want))
                return None
        return tot.replace("sp->", ""), sub, a, b

    for where, m, a, b in batch_slides:
        counts["carried"] += 1
        r = resolved(where, m, a, b)
        if r:
            tot, sub, a, b = r
            want_set(tot, sub, a[0], "adds")
            check_slide(tot, sub, a[0], a[2], b[0], b[1], b[2])

    if len(align) > 1:
        problems.append("the trailing totals are not anchored to one sliding loop: "
                        + "; ".join("startIdx-%d from %s" % (k, ",".join(sorted(set(v))))
                                    for k, v in sorted(align.items())))

    # --- the streaming tier: the same two offsets, against the ring ----------
    def check_stream(tot, sub, setting, lag, ring, b, ringdesc):
        want_set(tot, sub, setting, "adds")
        if pushes.get(ring) is not None and not _same(pushes[ring], setting):
            problems.append("%s adds %s but drops a slot of the %s ring %s"
                            % (tot, setting, pushes[ring], ring))
        if not _sub_matches(sub, lag):
            problems.append("%s[%s] adds the bar at lag %d" % (tot, sub, lag))
        if b is None:
            problems.append("%s: unrecognised trailing ring read `%s`"
                            % (tot, ringdesc))
            return
        want = slide.get((tot, sub))
        if want is None:
            problems.append("%s[%s] streams but the batch tier never slides it"
                            % (tot, sub))
        elif (lag, b) != want:
            problems.append("%s[%s] streams add lag %s / drop %s, but batch slides "
                            "add %s / drop %s" % (tot, sub, lag, b, want[0], want[1]))

    for m in STREAM_WRITE.finditer(src):
        tot, sub_l, sub_r, setting, lag, ring, slot = m.groups()
        counts["stream"] += 1
        check_stream(tot, sub_l or sub_r or "", setting, int(lag) if lag else 0,
                     ring, _lag_slot(slot, ring), " ".join(slot.split()))

    for where, m, a, b in stream_slides:
        counts["carried_stream"] += 1
        r = resolved(where, m, a, b)
        if r:
            tot, sub, a, b = r
            if b[1] not in pushes:
                problems.append("%s: %s[%s] drops %s, which is no trailing ring"
                                % (where, tot, sub, b[1]))
                continue
            check_stream(tot, sub, a[0], a[2], b[1], b[2], b[1])

    for m in STREAM_FOLD.finditer(src):
        tot, ctr, ring_a, slot_a, ring_b, slot_b = m.groups()
        counts["fold"] += 1
        if ring_a != ring_b:
            problems.append("%s adds from ring %s and drops from ring %s"
                            % (tot, ring_a, ring_b))
            continue
        want_set(tot, ctr, pushes.get(ring_a, "?"), "folds into a ring of")
        add = _cursor_slot(slot_a, ring_a)
        drop = _lag_slot(slot_b, ring_b)
        if add != ctr:
            problems.append("%s[sp->%s] adds `%s`, not the cursor-relative ring "
                            "read at sp->%s" % (tot, ctr, " ".join(slot_a.split()), ctr))
        if drop != ctr:
            problems.append("%s[sp->%s] drops `%s`, not the trailing ring read at "
                            "sp->%s" % (tot, ctr, " ".join(slot_b.split()), ctr))
        want = slide.get((tot, ctr))
        if want is None:
            problems.append("%s[sp->%s] folds but the batch tier never slides it"
                            % (tot, ctr))
        elif want != (ctr, ctr):
            problems.append("%s[sp->%s] folds add/drop at sp->%s, but batch slides "
                            "add %s / drop %s" % (tot, ctr, ctr, want[0], want[1]))

    # Every slide sits in a loop or step the checks above walk; an add without a
    # drop belongs only in a fill loop.
    fill_bodies = [f.span(4) for f in FILL_LOOP.finditer(src)]
    # A warm-up `while` that slides (CDLHIKKAKEMOD) is walked only for this and
    # for conditions, not for the once-per-bar count.
    warmups = [(m.end(), _block_end(src, m.end() - 1) - 1)
               for m in re.finditer(r"\bwhile\(\s*i\s*<[^{;]*\)\s*\{", src)]
    warmups = [b for b in warmups if not any(lo <= b[0] < hi for lo, hi in fill_bodies)
               and b[0] not in [f[0] for f in fill_bodies]]
    for w in BATCH_WRITE.finditer(src):
        if w.group(7) is not None:
            for lo, hi in warmups:
                if lo <= w.start() < hi and _guarded(src, lo, w.start()):
                    problems.append("line %d: %s slides under a condition" % (
                        src.count("\n", 0, w.start()) + 1, w.group(1)))
        spans = fill_bodies if w.group(7) is None else bodies + warmups
        if not any(lo <= w.start() < hi for lo, hi in spans):
            problems.append("line %d: %s[%s] %s outside any %s this gate follows" % (
                src.count("\n", 0, w.start()) + 1, w.group(1),
                w.group(2) or w.group(3) or "",
                "adds" if w.group(7) is None else "slides",
                "fill loop" if w.group(7) is None else "loop"))
    for w in list(STREAM_WRITE.finditer(src)) + list(STREAM_FOLD.finditer(src)):
        if not any(lo <= w.start() < hi for lo, hi in steps):
            problems.append("line %d: sp->%s slides outside any stream step"
                            % (src.count("\n", 0, w.start()) + 1, w.group(1)))
    # A total is reset only to start its fill: the next fill loop filling it
    # follows with no sliding loop in between. A declaration zeroes every
    # element; an element nothing fills is free.
    do_starts = [d for flo, fhi in _functions(src) for d, _b, _e in _do_loops(src, flo, fhi)]
    for r in TOTAL_RESET_AT.finditer(src):
        tot, sub = r.group(2), None if r.group(1) else r.group(3)
        if not any(t == tot and (sub is None or k == sub) for t, k in fill):
            continue
        fill_at = [f.start() for f in FILL_LOOP.finditer(src, r.end())
                   for w in BATCH_WRITE.finditer(src, *f.span(4))
                   if w.group(7) is None and w.group(1) == tot
                   and (sub is None or (w.group(2) or w.group(3) or "") == sub)]
        nxt = min(fill_at) if fill_at else None
        if (nxt is None or any(lo <= r.start() < hi for lo, hi in bodies + fill_bodies)
                or any(r.end() <= d < nxt for d in do_starts)):
            problems.append("line %d: %s%s is reset other than to start its fill"
                            % (src.count("\n", 0, r.start()) + 1, tot,
                               "[%s]" % sub if sub else ""))

    # Each stream step slides every total the batch tier slides, exactly once.
    if "TA_STREAM_CANDLEAVERAGE" in src and not steps:
        problems.append("streams its totals but has no `_StepImpl` this gate can find")
    for flo, fhi in steps:
        where = "line %d" % (src.count("\n", 0, flo) + 1)
        slid = {}
        for m in STREAM_WRITE.finditer(src, flo, fhi):
            key = (m.group(1), m.group(2) or m.group(3) or "")
            slid[key] = slid.get(key, 0) + 1
            if _guarded(src, flo + 1, m.start()):
                problems.append("%s: %s[%s] slides under a condition" % ((where,) + key))
        for m in STREAM_FOLD.finditer(src, flo, fhi):
            slid[m.group(1, 2)] = slid.get(m.group(1, 2), 0) + 1
            if _guarded(src, flo + 1, m.start()):
                problems.append("%s: %s[%s] slides under a condition"
                                % ((where,) + m.group(1, 2)))
        # The trailing ring's cursor is the stream's trailing index: it advances
        # once, unconditionally, after the step's last use of it, and only the
        # wrap may touch it after that.
        for ring in sorted(set(re.findall(r"sp->ringPos_(\w+)", src[flo:fhi]))):
            pos = r"sp->ringPos_%s\b" % ring
            adv = list(re.compile(r"%s\s*=\s*%s\s*\+\s*1\s*;" % (pos, pos))
                       .finditer(src, flo, fhi))
            if len(adv) != 1 or _guarded(src, flo + 1, adv[0].start()):
                problems.append("%s: the %s ring cursor advances %d time(s) per step, "
                                "not once unconditionally" % (where, ring, len(adv)))
                continue
            wrap = re.compile(r"\s*if\(\s*%s\s*>=\s*sp->ringCap_%s\s*\)\s*\{\s*%s\s*="
                              r"\s*0\s*;\s*\}" % (pos, ring, pos)).match(src, adv[0].end())
            if re.compile(pos).search(src, wrap.end() if wrap else adv[0].end(), fhi):
                problems.append("%s: the %s ring cursor advances before the step's "
                                "last use of it" % (where, ring))
        for w, m, _a, _b in stream_slides:
            if w == where:
                key = (m.group(1)[4:], m.group(2) or m.group(3) or "")
                slid[key] = slid.get(key, 0) + 1
        for key in sorted(slide):
            if slid.get(key, 0) != 1:
                problems.append("%s: %s[%s] slides %d time(s) per stream step, not "
                                "once" % ((where,) + key + (slid.get(key, 0),)))

    return problems, counts


def find_repo_root():
    d = os.path.dirname(os.path.abspath(__file__))
    while d != os.path.dirname(d):
        if os.path.isdir(os.path.join(d, "ta_codegen", "input")):
            return d
        d = os.path.dirname(d)
    sys.exit("check_candle_windows: cannot locate repo root")


def scan(src):
    """Return (reads, problems) for one file.

    `reads` is a list of (tier, total, subscript, offset); `problems` names a
    read whose subscript disagrees with its offset. `sets` maps each total to
    the candle setting it is read with -- the write side pairs against it.
    """
    reads, problems, sets, offs = [], [], {}, {}

    def note_set(total, sub, setting):
        # Keyed by SUBSCRIPT too: CDLMATHOLD packs one BodyLong window and three
        # BodyShort ones into a single `BodyPeriodTotal[5]`.
        prev = sets.setdefault((total, sub), setting)
        if prev != setting:
            problems.append("%s[%s] is read against both %s and %s"
                            % (total, sub, prev, setting))

    for setting, avg, fac, total, sub, off in BATCH.findall(src):
        if (avg or setting) != setting or (fac or setting) != setting:
            problems.append("%s%s averages %s's range over %s_avgPeriod with "
                            "%s_factor" % (total, "[%s]" % sub if sub else "",
                                           setting, avg, fac))
        off = int(off) if off else 0
        reads.append(("batch", total, sub, off))
        note_set(total, sub, setting)
        offs.setdefault((total, sub), set()).add(off)
        if sub != "" and int(sub) != off:
            problems.append("%s[%s] read at i-%d" % (total, sub, off))
    for setting, total, sub, lag in STREAM.findall(src):
        lag = int(lag) if lag else 0
        reads.append(("stream", total, sub, lag))
        note_set(total, sub, setting)
        if sub != "" and int(sub) != lag:
            problems.append("%s[%s] read at lag %d" % (total, sub, lag))
    return reads, problems, sets, offs


def main():
    root = find_repo_root()
    files = sorted(glob.glob(os.path.join(root, FUNC_DIR, "ta_CDL*.c")))
    if not files:
        sys.exit("check_candle_windows: no ta_CDL*.c under %s" % FUNC_DIR)

    nbBatch = nbStream = 0
    nbW = {"batch": 0, "stream": 0, "fold": 0, "fill": 0,
           "carried": 0, "carried_stream": 0}
    bad = []
    scanned = 0

    for path in files:
        name = os.path.basename(path)
        src = unfold_range_types(fold_ring_locals(_blank_comments(
            open(path, encoding="utf-8").read())))
        reads, problems, read_sets, read_offs = scan(src)
        if not reads:
            continue
        scanned += 1
        nbBatch += sum(1 for r in reads if r[0] == "batch")
        nbStream += sum(1 for r in reads if r[0] == "stream")
        wproblems, wcounts = scan_writes(src, read_sets, read_offs)
        problems = problems + wproblems
        for k in nbW:
            nbW[k] += wcounts[k]
        # Nothing may write a trailing total through a shape this gate does not
        # parse: an unrecognised form is a silent hole exactly where #240 found
        # one, so it fails here instead.
        for line in src.splitlines():
            t = line.strip()
            if TOTAL_WRITE.search(t):
                if (TOTAL_RESET.fullmatch(t) or TOTAL_CAPTURE.fullmatch(t)
                        or BATCH_WRITE.search(t) or STREAM_WRITE.search(t)
                        or STREAM_FOLD.search(t) or CARRIED_SLIDE.search(t)):
                    continue
            elif "PeriodTotal" not in t or "CANDLERANGE" not in t:
                continue
            elif CARRIED_DEF.search(t) or STREAM_DEF.search(t):
                continue
            problems.append("trailing-total write in a form this gate cannot "
                            "read -- teach it the form, do not drop the check: %s" % t)
        # One defect repeats once per range-type copy.
        tally = {}
        for p in problems:
            tally[p] = tally.get(p, 0) + 1
        for p, n in tally.items():
            bad.append("%s: %s%s" % (name, p, " (x%d)" % n if n > 1 else ""))

        # A total read at two different bars is the same defect wearing the
        # other naming convention: the STAR pair spells its two windows
        # BodyShortPeriodTotal / BodyShortPeriodTotal2, so there is no
        # subscript to disagree with the offset and only this catches it.
        seen = {}
        for tier, total, sub, off in reads:
            seen.setdefault((tier, total, sub), set()).add(off)
        for (tier, total, sub), offsets in sorted(seen.items()):
            if len(offsets) > 1:
                bad.append("%s: %s%s (%s) read at bars %s"
                           % (name, total, "[%s]" % sub if sub else "",
                              tier, sorted(offsets)))

    print("Candle trailing-total wiring: %d file(s), %d batch read(s), "
          "%d streaming read(s); writes (#240): %d batch (%d filling), "
          "%d streaming, %d folded, %d carried (%d streaming)"
          % (scanned, nbBatch, nbStream, nbW["batch"], nbW["fill"], nbW["stream"],
             nbW["fold"], nbW["carried"], nbW["carried_stream"]))

    # Non-vacuity. A regex that stops matching -- a renamed macro, a reformat
    # that splits the call across lines -- would otherwise report a clean run
    # having inspected nothing, which is the exact failure this gate was written
    # in response to. The range-type copies multiply every batch count.
    if scanned < 50 or nbBatch < 900 or nbStream < 260:
        print("\ncheck_candle_windows: only %d file(s) / %d batch / %d stream "
              "read(s) parsed, far below the corpus this was written against "
              "(57 / 1321 / 374). The patterns above have stopped matching the "
              "generated code -- fix them rather than the floor." %
              (scanned, nbBatch, nbStream))
        return 1
    if (nbW["batch"] < 1150 or nbW["fill"] < 700 or nbW["stream"] < 70
            or nbW["carried"] < 250 or nbW["carried_stream"] < 35):
        print("\ncheck_candle_windows: only %d batch (%d filling) / %d streaming "
              "/ %d carried (%d streaming) write(s) parsed, far below the corpus "
              "this was written against (1677 / 1017 / 100 / 369 / 53). The write "
              "patterns have stopped matching -- fix them rather than the floor." %
              (nbW["batch"], nbW["fill"], nbW["stream"], nbW["carried"],
               nbW["carried_stream"]))
        return 1

    if bad:
        print("\ncheck_candle_windows: %d wiring problem(s)." % len(bad))
        for b in bad:
            print("   %s" % b)
        print("\nA candlestick carries one running total per bar offset it "
              "compares; each read must pair the total with its own bar, and "
              "each write must add the bar the total names and drop the one "
              "exactly avgPeriod bars before it. "
              "A crossed pair still computes an average of the right setting "
              "over the right number of bars -- just the wrong ones -- so it "
              "changes the output only where the comparison happens to "
              "straddle. MC/DC cannot see it (pb_primer's bars are identical, "
              "so every trailing window holds the same content) and the "
              "differential gates see it only by luck of the data.")
        return 1

    print("Every trailing total accumulates the bar it is read at, over a window "
          "of its own setting's avgPeriod, in both tiers. OK.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
