#!/usr/bin/env python3
"""Check the TypeScript library's software fma bit for bit against libm.

    scripts/ts_fma_check.py --random=100000000 --seed=1

Compiles ts_fma_check/golden.c, which generates every hard-case family plus
--random cases (half random bit patterns, half realistic values) and answers
each with libm's fma, then streams them through ts_fma_check/check.mjs on the
first `node` on PATH. NaN results compare by NaN-ness, everything else by bits.
Exits non-zero on any mismatch, or if a family checked nothing.

The `naive-differs` column counts the cases where a twice-rounded a*b+c is
wrong, which is how many of a family's cases can catch a broken fma at all.
"""

import argparse
import json
import os
import re
import shutil
import struct
import subprocess
import sys
import tempfile
import time

HERE = os.path.dirname(os.path.abspath(__file__))
CHECK_DIR = os.path.join(HERE, "ts_fma_check")
MIN_NODE = (22, 18)


def fail(msg):
    print(f"ts_fma_check: {msg}", file=sys.stderr)
    sys.exit(1)


def find_node():
    node = shutil.which("node")
    if node is None:
        fail(f"node not found on PATH (needs Node >= {MIN_NODE[0]}.{MIN_NODE[1]})")
    out = subprocess.run([node, "--version"], capture_output=True, text=True).stdout.strip()
    m = re.match(r"v(\d+)\.(\d+)", out)
    if not m or (int(m.group(1)), int(m.group(2))) < MIN_NODE:
        fail(f"{node} is {out or 'unknown'}; needs >= {MIN_NODE[0]}.{MIN_NODE[1]} to run .ts sources")
    return node


def find_cc():
    for cc in (os.environ.get("CC"), "gcc", "cc", "clang"):
        if cc and shutil.which(cc):
            return cc
    fail("no C compiler found (set CC)")


def show(bits_hex):
    bits = int(bits_hex, 16)
    value = struct.unpack("<d", struct.pack("<Q", bits))[0]
    return f"{value.hex()} ({bits_hex})"


def main():
    ap = argparse.ArgumentParser(description=__doc__.split("\n")[1])
    ap.add_argument("--random", type=int, default=1_000_000, help="random cases (default 1000000)")
    ap.add_argument("--seed", type=int, default=1)
    ap.add_argument("--hard", type=int, default=1_000_000, help="cases per hard-case family (default 1000000)")
    ap.add_argument("--jobs", type=int, default=4, help="parallel generator/checker pairs (default 4)")
    ap.add_argument("--show", type=int, default=10, help="mismatches to print (default 10)")
    args = ap.parse_args()
    if args.jobs < 1 or args.random < 0 or args.hard < 0 or args.seed < 0:
        fail("--jobs must be >= 1; --random, --hard and --seed >= 0")

    node = find_node()
    cc = find_cc()
    with tempfile.TemporaryDirectory(prefix="ts_fma_check.") as tmp:
        golden = os.path.join(tmp, "golden")
        cmd = [cc, "-O2", "-ffp-contract=off", "-o", golden, os.path.join(CHECK_DIR, "golden.c"), "-lm"]
        if subprocess.run(cmd).returncode != 0:
            fail("compiling golden.c failed: " + " ".join(cmd))
        names = {}
        for line in subprocess.run([golden, "--families"], capture_output=True, text=True, check=True).stdout.splitlines():
            fid, name = line.split()
            names[fid] = name

        start = time.monotonic()
        pairs = []
        for k in range(args.jobs):
            gen = subprocess.Popen([golden, str(args.seed), str(k), str(args.jobs), str(args.hard), str(args.random)],
                                   stdout=subprocess.PIPE)
            chk = subprocess.Popen([node, os.path.join(CHECK_DIR, "check.mjs"), str(args.show)],
                                   stdin=gen.stdout, stdout=subprocess.PIPE)
            gen.stdout.close()
            pairs.append((gen, chk))
        reports = []
        for gen, chk in pairs:
            out = chk.communicate()[0]
            if gen.wait() != 0 or chk.returncode != 0:
                fail(f"worker failed (golden exit {gen.returncode}, node exit {chk.returncode})")
            reports.append(json.loads(out))
        elapsed = time.monotonic() - start

    totals = {fid: [0, 0, 0] for fid in names}
    examples = []
    for rep in reports:
        for fid, counts in rep["families"].items():
            for i in range(3):
                totals[fid][i] += counts[i]
        examples.extend(rep["examples"])

    print(f"seed {args.seed}, {args.hard} per hard family, {args.random} random, {args.jobs} jobs")
    print(f"{'family':<14} {'checked':>12} {'naive-differs':>14} {'mismatches':>11}")
    checked = mismatches = 0
    empty = []
    for fid in sorted(names, key=int):
        n, naive, bad = totals[fid]
        checked += n
        mismatches += bad
        print(f"{names[fid]:<14} {n:>12} {naive:>14} {bad:>11}")
        expected = {"random-bits": args.random // 2,
                    "random-real": args.random - args.random // 2}.get(names[fid], args.hard)
        if n == 0 and expected > 0:
            empty.append(names[fid])
    print(f"{'total':<14} {checked:>12} {'':>14} {mismatches:>11}")
    print(f"{elapsed:.1f} s, {checked / elapsed / 1e6:.1f} M cases/s" if elapsed > 0 else "")

    for fam, a, b, c, want, got in examples[: args.show]:
        print(f"MISMATCH [{names[str(fam)]}] fma({show(a)}, {show(b)}, {show(c)})")
        print(f"    libm {show(want)}")
        print(f"    ts   {show(got)}")

    if checked == 0:
        fail("no case was checked")
    if empty:
        fail("families checked nothing: " + ", ".join(empty))
    if mismatches:
        fail(f"{mismatches} mismatches")


if __name__ == "__main__":
    main()
