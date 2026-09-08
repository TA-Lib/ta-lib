#!/usr/bin/env python3
"""MA's two dispatch frames must stay inside HotSpot C2's inlining budget.

`MaStream.peek` and `Core.maStepImpl` are switches over `MAType`, and they sit
under every handle that holds an MA sub-handle -- APO, BBANDS, MACDEXT, MAVP,
PPO, PVO, STOCH, STOCHF, and KDJ and STOCHRSI through those. C2 refuses to
inline a hot method whose bytecode exceeds `FreqInlineSize`, so crossing that
budget costs those callers roughly a third of their per-bar time, measured. It
costs nothing visible: no test fails, no output changes.

Each new MAType grows peek by 16 bytes and the step frame by 20 (the arm body
plus a tableswitch entry), so this expires on a schedule -- and the enum is
live: HMA, RMA and ZLEMA all arrived in 2026.

325 is C2's DEFAULT, not a law. It moves between JDK versions and any
deployment can override it. This gate says the frames left the budget the
arm-return shape was measured against, not that the code is wrong.

Usage: check_java_inline_budget.py <classes-dir>
"""

import re
import subprocess
import sys

# `-XX:FreqInlineSize`, the HotSpot default for a hot call site.
BUDGET = 325

FRAMES = [
    ("Core$MaStream", re.compile(r"^\s+public double peek\(double\);")),
    ("Core", re.compile(r"^\s+private void maStepImpl\(io\.github\.talib\.Core\$MaStream, double\);")),
]

# A method's last instruction is always a 1-byte return or throw, so the final
# offset plus one is the Code attribute's length -- the number C2 compares.
# Asserted rather than assumed: a multi-byte last instruction would silently
# undercount and this gate would read green while the frame was over.
LAST_OPS = {"return", "ireturn", "lreturn", "freturn", "dreturn", "areturn", "athrow"}


def die(msg: str):
    print("::error::%s" % msg)
    sys.exit(1)


def disassemble(classes: str) -> str:
    """One javap call for both frames -- two would be two JVM startups."""
    names = ["io.github.talib." + c for c, _ in FRAMES]
    try:
        return subprocess.run(["javap", "-p", "-c", "-cp", classes] + names,
                              capture_output=True, text=True, check=True).stdout
    except (subprocess.CalledProcessError, FileNotFoundError) as e:
        die("javap could not read %s from %s: %s" % (", ".join(names), classes, e))


def code_length(out: str, cls: str, sig: re.Pattern) -> int:
    lines = out.splitlines()
    start = next((i for i, l in enumerate(lines) if sig.match(l)), None)
    if start is None:
        die("%s: no method matching %s -- the signature moved, so this gate "
            "measured NOTHING. Fix the pattern in this script rather than "
            "deleting the check." % (cls, sig.pattern))

    last_off, last_op = None, None
    for l in lines[start + 1:]:
        m = re.match(r"\s+(\d+): (\S+)", l)
        if m:
            last_off, last_op = int(m.group(1)), m.group(2)
        elif last_off is not None and not l.strip():
            break
    if last_off is None:
        die("%s: matched the signature but found no bytecode under it." % cls)
    if last_op not in LAST_OPS:
        die("%s: last instruction is %r, not a 1-byte return/throw, so "
            "offset+1 is not the code length. Teach this script the real "
            "length before trusting it." % (cls, last_op))
    return last_off + 1


def main():
    if len(sys.argv) != 2:
        die("usage: check_java_inline_budget.py <classes-dir>")
    classes = sys.argv[1]

    out = disassemble(classes)
    over = []
    for cls, sig in FRAMES:
        n = code_length(out, cls, sig)
        name = "%s.%s" % (cls, "peek" if "peek" in sig.pattern else "maStepImpl")
        print("%-24s %3d bytes (budget %d, %+d)" % (name, n, BUDGET, n - BUDGET))
        if n > BUDGET:
            over.append((name, n))

    if over:
        die("%s over C2's %d-byte FreqInlineSize: %s. Every caller holding an MA "
            "sub-handle just lost inlining on that frame, worth ~a third of its "
            "per-bar time. An N-way switch cannot stay under a fixed budget as N "
            "grows, so the fix is to split it: keep the common MATypes in this "
            "frame and delegate the rest to a second method that may grow freely. "
            "Emitter: build_dispatch_peek_frame / emit_dispatch in "
            "ta_codegen/generator/src/backends/java_stream.rs."
            % ("Frame" if len(over) == 1 else "Frames", BUDGET,
               ", ".join("%s at %d" % (n, b) for n, b in over)))
    print("Both MA dispatch frames are inside the %d-byte budget." % BUDGET)


if __name__ == "__main__":
    main()
