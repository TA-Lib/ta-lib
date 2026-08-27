#!/usr/bin/env python3
"""Every history-length arm, in every backend, must report the same code.

Two rules, one scan each.

Rule S7 (docs/error-handling-spec.md): a streaming `Open`/`OpenAndFill` given
fewer than `lookback + 1` bars reports `TA_INSUFFICIENT_HISTORY`. It is the
library's one RECOVERABLE condition, so it must be distinguishable from
`TA_BAD_PARAM`, which always means the call itself is wrong.

Rule S1: an EMPTY history reports `TA_OUT_OF_RANGE_START_INDEX`. An opener is a
batch call over `[0, historyLen - 1]`, so the implied `startIdx` of 0 has to name
a bar, and the fault answers B1's code rather than the catch-all (#268).

Why a structural check and not a probe. The condition is emitted from four
independent backends at half a dozen sites each -- a transcribed early return, an
explicit `historyLen < lookback + 1` guard, the composed capture guard, the
identity fast path, the period bank -- and a probe reaches only the shapes
someone thought to write one for. When TA_INSUFFICIENT_HISTORY was introduced,
two C sites were missed (the identity fast path and the period-bank OpenAndFill);
the hand-written probe covered six shapes and neither of those was among them,
and no value gate can see a return code that no test provokes. This reads the
generated output instead, so it covers every function and every shape at once,
and a new indicator or a new emission site is covered the day it lands.

The count of arms inspected is printed and floored per backend, so a pattern that
stops matching fails loudly rather than passing over nothing.
"""

import os
import re
import sys

# One row per backend. Two independent arm shapes are matched, because the
# generator emits the condition two ways and the miss that motivated this check
# was in the first kind while the bulk of the arms are the second:
#
#   EXPLICIT  an `historyLen < <lookback> + 1` precheck, or the composed capture
#             guard (`dummyNBElement < 1` / `outNBElement < 1`). This is where the
#             identity fast path and the period bank live -- the two C sites that
#             were missed.
#   EARLY     the transcribed batch early return, under `startIdx > endIdx`. In
#             the BATCH tier that arm answers success; in a stream it means the
#             history is short. Either way it is never the catch-all, which is
#             what makes this a non-circular invariant rather than a restatement
#             of whatever the emitter happens to produce.
#
# The `historyLen < 1` arm is a different condition -- rule S1, an EMPTY history
# -- and is scanned separately below, against its own code.
#
# The floors are LITERAL, and per backend and per shape. A floor derived from
# what the scan finds moves with it, and would let every arm vanish while still
# "passing its floor". They are minimums, so adding an indicator only raises the
# real count; a DROP means the emitted shape changed and this check is now
# looking at less than it was written to look at.
BACKENDS = [
    ("C", os.path.join("src", "ta_func"), ".c",
     r"TA_BAD_PARAM", r"TA_INSUFFICIENT_HISTORY",
     re.compile(r"(?:historyLen\s*<\s*(?!1\s*\))[^;\n]*?\+\s*1|dummyNBElement\s*<\s*1)\s*\)"
                r"[^\n]*?return\s+(TA_BAD_PARAM|TA_INSUFFICIENT_HISTORY)\s*;"), 11,
     re.compile(r"if\(\s*startIdx\s*>\s*endIdx\s*\)[\s\S]{0,220}?return\s+(TA_\w+)\s*;"), 400),

    ("Rust", os.path.join("ta_codegen", "output", "rust", "library", "src", "ta_func"), ".rs",
     r"BadParam", r"InsufficientHistory",
     re.compile(r"(?:historyLen\s*<\s*(?!1\s*\{)[^\n{]*?\+\s*1|\*outNBElement\s*<\s*1)\s*\{\s*\n\s*"
                r"return\s+Err\(RetCode::(BadParam|InsufficientHistory)\)\s*;"), 11,
     re.compile(r"if\s+startIdx\s*>\s*endIdx\s*\{[\s\S]{0,220}?return\s+(\w+(?:\(RetCode::\w+\))?)"), 100),

    ("Java", os.path.join("ta_codegen", "output", "java", "fragments"), ".java",
     r"RetCode.BadParam", r"RetCode.InsufficientHistory",
     re.compile(r"(?:historyLen\s*<\s*(?!1\s*\))[^\n)]*?\+\s*1|outNBElement\.value\s*<\s*1)\s*\)\s*\{\s*\n\s*"
                r"return\s+RetCode\.(BadParam|InsufficientHistory)\s*;"), 11,
     None, 0),

    ("C#", os.path.join("ta_codegen", "output", "csharp", "library", "src"), ".cs",
     r"RetCode.BadParam", r"RetCode.InsufficientHistory",
     re.compile(r"(?:historyLen\s*<\s*(?!1\s*\))[^\n)]*?\+\s*1|outNBElement\s*<\s*1)\s*\)\s*\{\s*\n\s*"
                r"return\s+RetCode\.(BadParam|InsufficientHistory)\s*;"), 11,
     None, 0),
]


# Rule S1, the same idea over the EMPTY-history arm: it is emitted once per
# entry point in every backend, and answering B1's code rather than the catch-all
# is the whole of what #268 changed, so a site that reverts has to be loud. Same
# literal floors, same reason.
EMPTY_ARMS = [
    ("C", os.path.join("src", "ta_func"), ".c", "TA_OUT_OF_RANGE_START_INDEX",
     re.compile(r"if\(\s*historyLen\s*<\s*1\s*\)\s*return\s+(TA_\w+)\s*;"), 500),

    ("Rust", os.path.join("ta_codegen", "output", "rust", "library", "src", "ta_func"), ".rs",
     "OutOfRangeStartIndex",
     re.compile(r"if\s+\w+\.is_empty\(\)\s*\{\s*\n\s*return\s+Err\(RetCode::(\w+)\)\s*;"), 170),

    ("Java", os.path.join("ta_codegen", "output", "java", "fragments"), ".java",
     "OutOfRangeStartIndex",
     re.compile(r"if\(\s*historyLen\s*<\s*1\s*\)\s*\{\s*\n\s*return\s+RetCode\.(\w+)\s*;"), 170),

    ("C#", os.path.join("ta_codegen", "output", "csharp", "library", "src"), ".cs",
     "OutOfRangeStartIndex",
     re.compile(r"if\(\s*historyLen\s*<\s*1\s*\)\s*\{\s*\n\s*return\s+RetCode\.(\w+)\s*;"), 170),
]


CATCH_ALL = ("TA_BAD_PARAM", "BadParam", "Err(RetCode::BadParam)")


def _scan(root_dir, subdir, suffix, arm_re, is_wrong):
    """(count, [`path:line -> code` for every arm `is_wrong` rejects])."""
    d = os.path.join(root_dir, subdir)
    if not os.path.isdir(d):
        return None, [f"{d} does not exist"]
    found, wrong = 0, []
    for name in sorted(os.listdir(d)):
        if not name.endswith(suffix):
            continue
        path = os.path.join(d, name)
        with open(path, encoding="utf-8") as f:
            text = f.read()
        for m in arm_re.finditer(text):
            found += 1
            code = m.group(m.lastindex) if m.lastindex else ""
            if is_wrong(code):
                wrong.append(f"{os.path.join(subdir, name)}:"
                             f"{text.count(chr(10), 0, m.start()) + 1} -> {code}")
    return found, wrong


def check_stream_retcodes(root_dir: str) -> bool:
    ok = True
    for (label, subdir, suffix, catch_all, expected,
         explicit_re, explicit_floor, early_re, early_floor) in BACKENDS:
        for shape, arm_re, floor in (("explicit", explicit_re, explicit_floor),
                                     ("early-return", early_re, early_floor)):
            if arm_re is None:
                continue
            found, wrong = _scan(root_dir, subdir, suffix, arm_re,
                                 lambda code: code in CATCH_ALL)
            if found is None:
                print(f"Error: {label}: {wrong[0]}")
                return False
            if found < floor:
                print(f"Error: {label} {shape}: {found} arm(s) matched, expected at least "
                      f"{floor}. The emitted shape changed and this check is now looking at "
                      f"less than it was written to look at -- fix the pattern, do not lower "
                      f"the floor.")
                ok = False
                continue
            if wrong:
                print(f"Error: {label} {shape}: {len(wrong)} short-history arm(s) answer "
                      f"{catch_all} instead of {expected} (rule S7):")
                for w in wrong[:12]:
                    print(f"         {w}")
                if len(wrong) > 12:
                    print(f"         ... and {len(wrong) - 12} more")
                ok = False
                continue
            print(f"  {label} {shape}: {found} arm(s), none on {catch_all}. OK.")
    return ok


def check_empty_history_retcodes(root_dir: str) -> bool:
    ok = True
    for label, subdir, suffix, expected, arm_re, floor in EMPTY_ARMS:
        found, wrong = _scan(root_dir, subdir, suffix, arm_re,
                             lambda code, want=expected: code != want)
        if found is None:
            print(f"Error: {label}: {wrong[0]}")
            return False
        if found < floor:
            print(f"Error: {label}: {found} empty-history arm(s) matched, expected at "
                  f"least {floor}. The emitted shape changed and this check is now "
                  f"looking at less than it was written to look at -- fix the pattern, "
                  f"do not lower the floor.")
            ok = False
            continue
        if wrong:
            print(f"Error: {label}: {len(wrong)} empty-history arm(s) do not answer "
                  f"{expected} (rule S1):")
            for w in wrong[:12]:
                print(f"         {w}")
            if len(wrong) > 12:
                print(f"         ... and {len(wrong) - 12} more")
            ok = False
            continue
        print(f"  {label}: {found} arm(s), all on {expected}. OK.")
    return ok


def main() -> int:
    root = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
    print("Short-history return code (rule S7) across all four backends:")
    ok = check_stream_retcodes(root)
    print("Empty-history return code (rule S1) across all four backends:")
    ok = check_empty_history_retcodes(root) and ok
    return 0 if ok else 1


if __name__ == "__main__":
    sys.exit(main())
