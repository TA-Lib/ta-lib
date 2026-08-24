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
  - The differential gates are only a partial net. `--fuzz-064` catches the
    crossing in CDLMORNINGSTAR (10 divergences) and misses the other two.
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

    batch/dispatch  TA_CANDLEAVERAGE(BodyLong,BodyLongPeriodTotal[1],i - 1)
    streaming       TA_STREAM_CANDLEAVERAGE(BodyLong,sp->BodyLongPeriodTotal[1],
                                            sp->lag1_inOpen,...)

The streaming tier names its bar with a `lagN_` prefix instead of an `i` offset
(and no prefix means the current bar, lag 0), so it needs its own pattern but
the same rule.

Scope: the generated C in src/ta_func, which is the shipped library and the
golden every differential gate compares against. A generator defect reaches all
four backends identically -- that is the blindness #219 exists for -- so one
backend is enough to catch it. A per-backend emission defect is not covered
here; that is --xlang-hash's territory.

Run standalone, or via `scripts/build.py check-candle-windows`.
"""

import glob
import os
import re
import sys

FUNC_DIR = os.path.join("src", "ta_func")

# TA_CANDLEAVERAGE(SET, TOTAL[k], i - m)   -- k and m both optional
BATCH = re.compile(r"TA_CANDLEAVERAGE\(\s*(\w+)\s*,\s*(\w+?)(?:\[(\d+)\])?\s*,"
                   r"\s*i(?:\s*-\s*(\d+))?\s*\)")
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
    r"TA_CANDLERANGE\(\s*(\w+)\s*,\s*i" + _OFF + r"\s*\)"
    r"(?:\s*-\s*TA_CANDLERANGE\(\s*(\w+)\s*,\s*(\w+)" + _OFF + r"\s*\))?")

# The derived ring's own per-bar push -- what the ring actually holds.
RING_PUSH = re.compile(
    r"sp->ring_(\w+)_derived\[sp->ringPos_\1\]\s*=\s*TA_STREAM_CANDLERANGE\(\s*(\w+)\s*,")

# `sp->TOT[k] += TA_STREAM_CANDLERANGE(Set,[sp->lagL_]inOpen,...) - sp->ring_TR_derived[slot];`
STREAM_WRITE = re.compile(
    r"sp->(\w*PeriodTotal\d?)(?:\[(\w+)\])?\s*(?:\+=|=\s*sp->\1(?:\[(\w+)\])?\s*\+)\s*\(?\s*"
    r"TA_STREAM_CANDLERANGE\(\s*(\w+)\s*,\s*(?:sp->lag(\d+)_)?inOpen[^)]*\)"
    r"\s*-\s*sp->ring_(\w+)_derived\[([^\]]*)\]")

# The #229 folded form: BOTH terms read one derived ring -- the add
# cursor-relative, the subtract through the runtime lag.
STREAM_FOLD = re.compile(
    r"sp->(\w*PeriodTotal\d?)\[(?:sp->)?(\w+)\]\s*=\s*sp->\1\[(?:sp->)?\2\]\s*\+\s*\(\s*"
    r"sp->ring_(\w+)_derived\[([^\]]*)\]\s*-\s*sp->ring_(\w+)_derived\[([^\]]*)\]\s*\)")


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


def scan_writes(src, read_sets, read_offs):
    """Return (problems, counts) for the write side of one file."""
    problems = []
    counts = {"batch": 0, "stream": 0, "fold": 0, "fill": 0}

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
            if known and setting not in known:
                problems.append("%s[%s] %s %s, which the total is never read "
                                "against" % (total, sub, where, setting))
            return
        want = read_sets.get((total, sub))
        if want is not None and want != setting:
            problems.append("%s[%s] %s %s but the total is read against %s"
                            % (total, sub, where, setting, want))

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
            fill_c[(tot, sub)] = c
            fill_set[(tot, sub)] = aset
            prev = fill.setdefault((tot, sub), back)
            if prev != back:
                problems.append("%s[%s] is filled from two different bars (%s, %s)"
                                % (tot, sub, prev, back))

    # --- the sliding writes: which bar in, which bar out ---------------------
    slide = {}                      # (total, subscript) -> (add back, drop back)
    align = {}                      # sliding-loop origin -> the totals implying it
    for m in BATCH_WRITE.finditer(src):
        (tot, sub_l, sub_r, aset, asign, atok,
         bset, tr, bsign, btok) = m.groups()
        counts["batch"] += 1
        sub = sub_l or sub_r or ""
        if sub_l and sub_r and sub_l != sub_r:
            problems.append("%s[%s] accumulates into %s[%s]" % (tot, sub_r, tot, sub_l))
        add = _off(asign, atok)
        want_set(tot, sub, aset, "adds")
        if bset is None:
            continue                # a fill add, handled above
        if not _sub_matches(sub, add):
            problems.append("%s[%s] adds the bar %s back" % (tot, sub, add))
        if sub == "" and read_offs.get((tot, "")) not in (None, {add}):
            problems.append("%s adds the bar %s back but is read at %s"
                            % (tot, add, sorted(read_offs[(tot, "")])))
        want_set(tot, sub, bset, "subtracts")
        if tr not in trails:
            problems.append("%s subtracts through %s, which no "
                            "`startIdx - c - <Set>_avgPeriod` declares" % (tot, tr))
            continue
        if trails[tr][0] != aset:
            problems.append("%s (%s) subtracts through %s, which is paced by "
                            "%s_avgPeriod" % (tot, aset, tr, trails[tr][0]))
        drop = _off(bsign, btok)
        slide.setdefault((tot, sub), (add, drop))
        # The first slide must drop the first bar the fill added.
        want = fill.get((tot, sub))
        if want is None and sub != "" and not sub.isdigit():
            # A loop-driven slide (`TOT[totIdx]`) adds and drops at the loop
            # counter, so for every subscript it covers the fill must have used
            # that subscript as its offset -- which the fill scan required of
            # each concrete entry. All that is left is that it covers some.
            covered = [k for (t, k) in fill
                       if t == tot and k.isdigit() and fill_set.get((t, k)) == aset]
            if covered:
                want = drop
                align[fill_c.get((tot, covered[0]), 0)] = \
                    align.get(fill_c.get((tot, covered[0]), 0), []) + \
                    ["%s[%s]" % (tot, sub)]
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
            align[fill_c.get((tot, sub), 0) + want - add] = \
                align.get(fill_c.get((tot, sub), 0) + want - add, []) + \
                ["%s[%s]" % (tot, sub)]

    if len(align) > 1:
        problems.append("the trailing totals are not anchored to one sliding loop: "
                        + "; ".join("startIdx-%d from %s" % (k, ",".join(sorted(set(v))))
                                    for k, v in sorted(align.items())))

    # --- the streaming tier: the same two offsets, against the ring ----------
    for m in STREAM_WRITE.finditer(src):
        tot, sub_l, sub_r, setting, lag, ring, slot = m.groups()
        counts["stream"] += 1
        sub = sub_l or sub_r or ""
        lag = int(lag) if lag else 0
        want_set(tot, sub, setting, "adds")
        if pushes.get(ring) not in (None, setting):
            problems.append("%s adds %s but drops a slot of the %s ring %s"
                            % (tot, setting, pushes[ring], ring))
        if not _sub_matches(sub, lag):
            problems.append("%s[%s] adds the bar at lag %d" % (tot, sub, lag))
        b = _lag_slot(slot, ring)
        if b is None:
            problems.append("%s: unrecognised trailing ring read `%s`"
                            % (tot, " ".join(slot.split())))
            continue
        want = slide.get((tot, sub))
        if want is None:
            problems.append("%s[%s] streams but the batch tier never slides it"
                            % (tot, sub))
            continue
        if (lag, b) != want:
            problems.append("%s[%s] streams add lag %d / drop %s, but batch slides "
                            "add %s / drop %s" % (tot, sub, lag, b, want[0], want[1]))

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

    for setting, total, sub, off in BATCH.findall(src):
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
    nbW = {"batch": 0, "stream": 0, "fold": 0, "fill": 0}
    bad = []
    scanned = 0

    for path in files:
        name = os.path.basename(path)
        src = open(path, encoding="utf-8").read()
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
            if "PeriodTotal" not in t or "CANDLERANGE(" not in t:
                continue
            if (BATCH_WRITE.search(t) or STREAM_WRITE.search(t)
                    or STREAM_FOLD.search(t)):
                continue
            problems.append("trailing-total write in a form this gate cannot "
                            "read -- teach it the form, do not drop the check: %s" % t)
        for p in problems:
            bad.append("%s: %s" % (name, p))

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
          "%d streaming, %d folded" % (scanned, nbBatch, nbStream,
                                       nbW["batch"], nbW["fill"],
                                       nbW["stream"], nbW["fold"]))

    # Non-vacuity. A regex that stops matching -- a renamed macro, a reformat
    # that splits the call across lines -- would otherwise report a clean run
    # having inspected nothing, which is the exact failure this gate was written
    # in response to.
    if scanned < 50 or nbBatch < 400 or nbStream < 100:
        print("\ncheck_candle_windows: only %d file(s) / %d batch / %d stream "
              "read(s) parsed, far below the corpus this was written against "
              "(57 / 567 / 187). The patterns above have stopped matching the "
              "generated code -- fix them rather than the floor." %
              (scanned, nbBatch, nbStream))
        return 1
    if (nbW["batch"] < 600 or nbW["fill"] < 200 or nbW["stream"] < 70
            or nbW["fold"] < 15):
        print("\ncheck_candle_windows: only %d batch (%d filling) / %d streaming "
              "/ %d folded write(s) parsed, far below the corpus this was written "
              "against (831 / 372 / 100 / 23). The write patterns have stopped "
              "matching -- fix them rather than the floor." %
              (nbW["batch"], nbW["fill"], nbW["stream"], nbW["fold"]))
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
