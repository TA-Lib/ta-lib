#!/usr/bin/env python3
"""Instruction-count regression gate for every C entry point (issue #412).

Runs `bin/ta_bench_icount` under callgrind and compares each entry point's
retired-instruction count against `.github/perf/icount-baseline-<arch>.tsv`.

    scripts/bench_icount.py                     # build, measure, compare
    scripts/bench_icount.py --update-baseline   # ... and rewrite the baseline
    scripts/bench_icount.py --no-build --function=RSI,SMA

The counts are EXACT, not sampled: the same binary on the same input returns the
same number on a loaded runner and an idle one. That is the only reason a 10%
threshold means anything here, and the reason nothing in this file averages,
repeats or discards a measurement.

The number is not a time. Valgrind charges every instruction the same, so
out-of-order execution, port pressure and dependency-chain latency are all
invisible, and it over-charges branchy code: a branchless rewrite can read here
as a regression. Use it for the algorithmic class (a lost fast path, an extra
pass over the window, an un-inlined call); never quote a percentage from it.

A baseline is only comparable within one (architecture, compiler) pair. On a
mismatch this refuses to compare, says so, and asks to be re-baselined rather
than reporting a thousand false regressions.

Only three of the five tiers are exact at any run width. `Open` and
`OpenAndFill` allocate, so a few hundred of their instructions are the
allocator's response to everything that ran before them; under 0.3%, absorbed by
any useful threshold, but it means `--function` narrows the run into something
the baseline cannot judge, and a run so narrowed only reports.
"""

import argparse
import datetime
import os
import platform
import re
import shutil
import subprocess
import sys

# Per-architecture, so adding an ARM runner later cannot overwrite the x86
# baseline with counts that were never comparable to it.
BASELINE_REL = os.path.join(".github", "perf",
                            f"icount-baseline-{platform.machine()}.tsv")

# Floor on how many entry points an unfiltered run must measure before its
# result is allowed to mean anything. 1005 today (201 functions x 5 tiers) and
# the corpus only grows; a suite that measures nothing must not exit 0.
MIN_ROWS = 900

DEFAULT_THRESHOLD = 0.10

# Listed but not failed. Above the ~0.3% the allocating tiers carry from heap
# history, so an unchanged corpus lists nothing.
REPORT_THRESHOLD = 0.01

VALGRIND_ARGS = [
    "--tool=callgrind",
    # One file for every region, in execution order, so a dump is joined to its
    # marker by position rather than by a second pass over N files.
    "--combine-dumps=yes",
    # Only the per-part `summary:` line is read; the per-line and per-instruction
    # detail is a 30x larger file nothing here parses.
    "--dump-line=no",
    "--dump-instr=no",
    "--collect-jumps=no",
]


def find_repo_root():
    script_dir = os.path.dirname(os.path.abspath(__file__))
    out = subprocess.run(["git", "rev-parse", "--show-toplevel"],
                         check=True, capture_output=True, text=True, cwd=script_dir)
    return out.stdout.strip()


def toolchain_id(root):
    """`<arch>/<target triple>/<compiler version>`: the axis a count is only
    comparable along. Reads the compiler CMake actually used, not `cc`."""
    cc = None
    cache = os.path.join(root, "cmake-build", "CMakeCache.txt")
    if os.path.exists(cache):
        with open(cache) as f:
            for line in f:
                if line.startswith("CMAKE_C_COMPILER:"):
                    cc = line.split("=", 1)[1].strip()
                    break
    if not cc:
        cc = shutil.which("cc") or "cc"

    def ask(flag):
        try:
            r = subprocess.run([cc, flag], capture_output=True, text=True)
            return r.stdout.strip() if r.returncode == 0 else "?"
        except OSError:
            return "?"

    return f"{platform.machine()}/{ask('-dumpmachine')}/{ask('-dumpfullversion')}"


# ---------------------------------------------------------------- measuring

def build(root):
    subprocess.run([sys.executable, os.path.join(root, "scripts", "build.py"),
                    "ta_bench_icount"], check=True, cwd=root)


def run_bench(root, out_file, bench_args):
    binary = os.path.join(root, "bin", "ta_bench_icount")
    if not os.path.exists(binary):
        sys.exit(f"bench_icount: {binary} not found; run without --no-build")
    if not shutil.which("valgrind"):
        sys.exit("bench_icount: valgrind not found (apt install valgrind). It "
                 "supplies BOTH the tool and <valgrind/callgrind.h>, which the "
                 "binary needs at COMPILE time, so installing it now means "
                 "rebuilding ta_bench_icount.")
    cmd = (["valgrind"] + VALGRIND_ARGS + [f"--callgrind-out-file={out_file}"]
           + [binary] + bench_args)
    proc = subprocess.run(cmd, capture_output=True, text=True, cwd=root)
    if proc.returncode != 0:
        sys.stderr.write(proc.stdout)
        sys.stderr.write(proc.stderr)
        sys.exit(f"bench_icount: ta_bench_icount exited {proc.returncode}")
    return proc.stdout


def parse_rows(stdout):
    """`NAME KIND measured|skipped RETCODE` lines, plus the `#` header fields."""
    meta, rows = {}, []
    for line in stdout.splitlines():
        if line.startswith("# ta_bench_icount "):
            for tok in line[len("# ta_bench_icount "):].split():
                if "=" in tok:
                    k, v = tok.split("=", 1)
                    meta[k] = v
            continue
        if line.startswith("#") or not line.strip():
            continue
        parts = line.split()
        if len(parts) != 4:
            sys.exit(f"bench_icount: unparseable row {line!r}")
        rows.append((parts[0], parts[1], parts[2] == "measured", int(parts[3])))
    return meta, rows


TRIGGER_RE = re.compile(r"^desc: Trigger: Client Request: (.+)$")
SUMMARY_RE = re.compile(r"^summary:\s+(\d+)")


def parse_dumps(path):
    """marker -> Ir, pairing each client-request trigger with the `summary:` of
    the part it opened. The unnamed trailing part (program termination) has no
    trigger and is dropped."""
    dumps, pending = {}, None
    with open(path) as f:
        for line in f:
            m = TRIGGER_RE.match(line)
            if m:
                if pending is not None:
                    sys.exit(f"bench_icount: dump {pending!r} has no summary")
                pending = m.group(1).strip()
                continue
            m = SUMMARY_RE.match(line)
            if m and pending is not None:
                if pending in dumps:
                    sys.exit(f"bench_icount: duplicate dump marker {pending!r}")
                dumps[pending] = int(m.group(1))
                pending = None
    if pending is not None:
        sys.exit(f"bench_icount: dump {pending!r} has no summary")
    return dumps


def join(rows, dumps, filtered):
    """One measurement per measured row. Any disagreement between what the
    binary says it measured and what callgrind dumped is a harness fault, not a
    result: a region that silently stopped dumping would otherwise read green."""
    measured, unmatched = [], []
    for name, kind, is_measured, rc in rows:
        marker = f"{name}/{kind}"
        if is_measured:
            if marker not in dumps:
                unmatched.append(f"no dump for measured region {marker}")
            else:
                measured.append((name, kind, rc, dumps.pop(marker)))
        elif marker in dumps:
            unmatched.append(f"dump present for skipped region {marker}")
            dumps.pop(marker)
    for extra in dumps:
        unmatched.append(f"dump {extra} matches no row")
    if unmatched:
        for u in unmatched[:20]:
            print(f"  {u}", file=sys.stderr)
        sys.exit(f"bench_icount: {len(unmatched)} region(s) out of step "
                 "between stdout and the callgrind dumps")
    if not filtered and len(measured) < MIN_ROWS:
        sys.exit(f"bench_icount: only {len(measured)} entry points measured, "
                 f"floor is {MIN_ROWS}")
    return measured


# ---------------------------------------------------------------- baseline

def write_baseline(path, measured, meta, toolchain, commit):
    os.makedirs(os.path.dirname(path), exist_ok=True)
    corpus = " ".join(f"{k}={meta[k]}" for k in sorted(meta))
    with open(path, "w") as f:
        f.write("# Instruction counts per C entry point, from scripts/bench_icount.py.\n")
        f.write("# Regenerate with `scripts/bench_icount.py --update-baseline`;\n")
        f.write("# a count is comparable only against the same toolchain line.\n")
        f.write(f"#toolchain\t{toolchain}\n")
        f.write(f"#corpus\t{corpus}\n")
        f.write(f"#commit\t{commit}\n")
        f.write(f"#generated\t{datetime.datetime.now(datetime.timezone.utc):%Y-%m-%dT%H:%M:%SZ}\n")
        for name, kind, rc, ir in sorted(measured):
            f.write(f"{name}\t{kind}\t{rc}\t{ir}\n")


def read_baseline(path):
    if not os.path.exists(path):
        return None, {}
    head, rows = {}, {}
    with open(path) as f:
        for line in f:
            line = line.rstrip("\n")
            if line.startswith("#"):
                parts = line[1:].split("\t")
                if len(parts) == 2:
                    head[parts[0]] = parts[1]
                continue
            if not line.strip():
                continue
            name, kind, rc, ir = line.split("\t")
            rows[(name, kind)] = (int(rc), int(ir))
    return head, rows


# ---------------------------------------------------------------- reporting

def report(measured, base_rows, threshold, report_threshold, filtered):
    """Returns (failures, moved, structural). `failures` is what fails the gate;
    `moved` is every row past `report_threshold`, failures included, so the
    nightly summary reads as a list of what the day's commits changed.

    `filtered` suppresses the removed-row scan: a narrowed run did not measure
    the rest, which is not the same as the rest having disappeared."""
    failures, moved, structural = [], [], []
    seen = set()
    for name, kind, rc, ir in measured:
        key = (name, kind)
        seen.add(key)
        if key not in base_rows:
            structural.append((name, kind, "new", ""))
            continue
        base_rc, base_ir = base_rows[key]
        if base_rc != rc:
            structural.append((name, kind, "retcode", f"{base_rc} -> {rc}"))
            continue
        if base_ir <= 0:
            structural.append((name, kind, "bad-baseline", str(base_ir)))
            continue
        delta = (ir - base_ir) / base_ir
        row = (name, kind, base_ir, ir, delta)
        if delta > threshold:
            failures.append(row)
        if abs(delta) >= report_threshold:
            moved.append(row)
    if not filtered:
        for key in base_rows:
            if key not in seen:
                structural.append((key[0], key[1], "removed", ""))
    failures.sort(key=lambda r: -r[4])
    moved.sort(key=lambda r: -abs(r[4]))
    structural.sort()
    return failures, moved, structural


def fmt_table(rows, title):
    out = [f"\n{title}", f"{'entry point':<34} {'kind':<9} {'baseline':>12} {'now':>12} {'change':>9}"]
    for name, kind, base_ir, ir, delta in rows:
        out.append(f"{name:<34} {kind:<9} {base_ir:>12,} {ir:>12,} {delta:>+8.1%}")
    return "\n".join(out)


def md_table(rows, title):
    if not rows:
        return ""
    out = [f"\n### {title}\n",
           "| entry point | kind | baseline | now | change |",
           "| --- | --- | ---: | ---: | ---: |"]
    for name, kind, base_ir, ir, delta in rows:
        out.append(f"| `{name}` | {kind} | {base_ir:,} | {ir:,} | {delta:+.1%} |")
    return "\n".join(out) + "\n"


def emit_github_summary(text):
    path = os.environ.get("GITHUB_STEP_SUMMARY")
    if path:
        with open(path, "a") as f:
            f.write(text)


# ---------------------------------------------------------------- main

def main():
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--no-build", action="store_true",
                    help="measure bin/ta_bench_icount as it stands")
    ap.add_argument("--update-baseline", action="store_true",
                    help="rewrite the baseline when the run does not regress")
    ap.add_argument("--force-baseline", action="store_true",
                    help="adopt today's counts as the baseline even where they "
                         "regressed, and exit 0. For a perf change that is "
                         "intended; it accepts everything the run measured")
    ap.add_argument("--threshold", type=float, default=DEFAULT_THRESHOLD,
                    help=f"fractional growth that FAILS a row (default {DEFAULT_THRESHOLD})")
    ap.add_argument("--report-threshold", type=float, default=REPORT_THRESHOLD,
                    help=f"absolute change a row must reach to be LISTED "
                         f"(default {REPORT_THRESHOLD})")
    ap.add_argument("--function", default=None,
                    help="comma-separated substrings. Narrows the run, which makes "
                         "it report-only: see the allocator note in the header")
    ap.add_argument("--points", type=int, default=None)
    ap.add_argument("--stream-iters", type=int, default=None)
    ap.add_argument("--shape", default=None)
    ap.add_argument("--out-file", default=None,
                    help="keep the raw callgrind output at this path")
    args = ap.parse_args()

    root = find_repo_root()
    if not args.no_build:
        build(root)

    filtered = bool(args.function)
    bench_args = []
    if args.function:
        bench_args.append(f"--function={args.function}")
    if args.points is not None:
        bench_args.append(f"--points={args.points}")
    if args.stream_iters is not None:
        bench_args.append(f"--stream-iters={args.stream_iters}")
    if args.shape:
        bench_args.append(f"--shape={args.shape}")

    out_file = args.out_file or os.path.join(root, "temp", "callgrind.icount.out")
    os.makedirs(os.path.dirname(out_file), exist_ok=True)

    stdout = run_bench(root, out_file, bench_args)
    meta, rows = parse_rows(stdout)
    measured = join(rows, parse_dumps(out_file), filtered)

    toolchain = toolchain_id(root)
    commit = subprocess.run(["git", "rev-parse", "HEAD"], cwd=root,
                            capture_output=True, text=True).stdout.strip()
    baseline_path = os.path.join(root, BASELINE_REL)
    head, base_rows = read_baseline(baseline_path)
    corpus = " ".join(f"{k}={meta[k]}" for k in sorted(meta))

    print(f"\nmeasured {len(measured)} entry points  ({toolchain})")
    print(f"corpus: {corpus}")

    # No baseline, a different toolchain, or a different corpus: there is
    # nothing to compare against, and pretending otherwise reports a thousand
    # false rows. Say so and re-baseline; do not fail the run.
    incomparable = None
    if head is None:
        incomparable = f"no baseline at {BASELINE_REL}"
    elif head.get("toolchain") != toolchain:
        incomparable = f"baseline toolchain {head.get('toolchain')!r} != {toolchain!r}"
    elif head.get("corpus") != corpus:
        incomparable = f"baseline corpus {head.get('corpus')!r} != {corpus!r}"

    if incomparable:
        print(f"\nNOT COMPARED: {incomparable}.\n"
              "Instruction counts are comparable only within one toolchain and "
              "one input corpus.")
        emit_github_summary("## Instruction-count suite\n\n"
                            f"**Not compared.** {incomparable}. "
                            f"Measured {len(measured)} entry points.\n")
        if (args.update_baseline or args.force_baseline) and not filtered:
            write_baseline(baseline_path, measured, meta, toolchain, commit)
            print(f"baseline written: {BASELINE_REL}")
        return 0

    failures, moved, structural = report(measured, base_rows, args.threshold,
                                         args.report_threshold, filtered)
    others = [r for r in moved if r not in failures]

    if structural:
        print(f"\nstructural changes vs baseline ({len(structural)}):")
        for name, kind, what, detail in structural[:40]:
            print(f"  {name} {kind}: {what} {detail}".rstrip())
        if len(structural) > 40:
            print(f"  ... and {len(structural) - 40} more")
    if others:
        print(fmt_table(others[:30], f"moved >={args.report_threshold:.0%} "
                                     f"({len(others)} total)"))
    if failures:
        print(fmt_table(failures, f"OVER THRESHOLD >{args.threshold:.0%} "
                                  f"({len(failures)} total)"))

    summary = ["## Instruction-count suite\n",
               f"\n`{toolchain}` · {len(measured)} entry points · "
               f"fails above {args.threshold:.0%} · baseline commit "
               f"`{head.get('commit', '?')[:12]}`\n"]
    if filtered:
        summary.append(f"\n_Narrowed to `{args.function}`: report only, "
                       "the allocating tiers are not comparable against a "
                       "whole-corpus baseline._\n")
    if failures:
        summary.append(f"\n**{len(failures)} entry point(s) over threshold.**\n")
    elif not moved and not structural:
        summary.append("\nNothing moved.\n")
    summary.append(md_table(failures, "Over threshold"))
    summary.append(md_table(others[:30], f"Moved ({len(others)})"))
    if structural:
        summary.append("\n### Structural\n\n" + "\n".join(
            f"- `{n}` {k}: {w} {d}".rstrip() for n, k, w, d in structural[:40]) + "\n")
    emit_github_summary("".join(summary))

    if failures and args.force_baseline and not filtered:
        print(f"\nACCEPTED: {len(failures)} entry point(s) over "
              f"{args.threshold:.0%} adopted into the baseline by "
              "--force-baseline.")
        write_baseline(baseline_path, measured, meta, toolchain, commit)
        print(f"baseline written: {BASELINE_REL}")
        return 0

    if failures:
        if filtered:
            print(f"\n{len(failures)} entry point(s) over {args.threshold:.0%}, "
                  "but --function makes this run report-only. Re-run whole-corpus "
                  "to gate on it.")
            return 0
        print(f"\nFAIL: {len(failures)} entry point(s) grew more than "
              f"{args.threshold:.0%}. The baseline is NOT updated.")
        return 1

    print(f"\nPASS: no entry point grew more than {args.threshold:.0%}.")
    if (args.update_baseline or args.force_baseline) and not filtered:
        write_baseline(baseline_path, measured, meta, toolchain, commit)
        print(f"baseline written: {BASELINE_REL}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
