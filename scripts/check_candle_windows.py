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
    read whose subscript disagrees with its offset.
    """
    reads, problems = [], []
    for _set, total, sub, off in BATCH.findall(src):
        off = int(off) if off else 0
        reads.append(("batch", total, sub, off))
        if sub != "" and int(sub) != off:
            problems.append("%s[%s] read at i-%d" % (total, sub, off))
    for _set, total, sub, lag in STREAM.findall(src):
        lag = int(lag) if lag else 0
        reads.append(("stream", total, sub, lag))
        if sub != "" and int(sub) != lag:
            problems.append("%s[%s] read at lag %d" % (total, sub, lag))
    return reads, problems


def main():
    root = find_repo_root()
    files = sorted(glob.glob(os.path.join(root, FUNC_DIR, "ta_CDL*.c")))
    if not files:
        sys.exit("check_candle_windows: no ta_CDL*.c under %s" % FUNC_DIR)

    nbBatch = nbStream = 0
    bad = []
    scanned = 0

    for path in files:
        name = os.path.basename(path)
        reads, problems = scan(open(path, encoding="utf-8").read())
        if not reads:
            continue
        scanned += 1
        nbBatch += sum(1 for r in reads if r[0] == "batch")
        nbStream += sum(1 for r in reads if r[0] == "stream")
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
          "%d streaming read(s)" % (scanned, nbBatch, nbStream))

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

    if bad:
        print("\ncheck_candle_windows: %d wiring problem(s)." % len(bad))
        for b in bad:
            print("   %s" % b)
        print("\nA candlestick carries one running total per bar offset it "
              "compares, and each read must pair the total with its own bar. "
              "A crossed pair still computes an average of the right setting "
              "over the right number of bars -- just the wrong ones -- so it "
              "changes the output only where the comparison happens to "
              "straddle. MC/DC cannot see it (pb_primer's bars are identical, "
              "so every trailing window holds the same content) and the "
              "differential gates see it only by luck of the data.")
        return 1

    print("Every trailing total is read at the bar it accumulates. OK.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
