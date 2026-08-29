#!/usr/bin/env python3
"""A/B the Rust and Java streaming tier between two revisions of this tree.

`ta_bench_stream` measures the C streaming tier only. This measures the other
two: it times `update` (or `peek`) per bar for every streaming function -- or
`open`, which times a whole warm-up pass rather than one bar -- in two
arms — the working tree and a git revision — and reports the per-function
change with the untouched functions as the control.

    scripts/stream_ab.py --base=origin/dev
    scripts/stream_ab.py --base=HEAD~1 --lang=rust --call=peek --mark=MIN,MAX

The two arms are the SAME harness source compiled against two copies of the
library, so a row is a library difference, not a harness difference. Rounds are
interleaved and the arm order alternates: a fixed order biases every row the
same way when the box drifts busier mid-run.

Read the control split before any row. These binaries are single-CGU LTO (Rust)
and one giant class (Java), so changing any function moves the layout of the
rest; only a claim that clears the control noise is a claim.

The change column is the output; the ns columns are not. Each invocation builds
into its own directory, so a function's absolute ns is comparable only against
the other arm of the SAME run — never against another invocation, whatever was
varied between them (`--period`, `--call`, the revision). Sweeping a parameter
one invocation per point and reading the result as a curve reports build layout
as if it were the parameter.

Inputs: the generated `ta_codegen/output/rust/library` crate and
`ta_codegen/output/java/fragments`. Nothing else — no ta_abstract, no C library,
no servers, no ta_regtest. Needs `cargo` for --lang=rust, `javac`/`java` for
--lang=java. Both revisions must expose the same stream API surface; if they do
not, the base arm fails to compile and says so.

The harness calls every function through its generated signature with the
default-value sentinels (i32::MIN / -4e37 / MAType.DEFAULT), so it needs no
per-function knowledge and nothing here needs editing when a function is added.
A function whose Open signature it cannot render is REPORTED, never dropped
silently.

`--funcs` is for iterating, not for the number you quote: a short set is a
different binary and a different JIT state (Java reads ~3x faster over 7
functions than over all 168), and it leaves too few controls to bound the
layout noise. Sweep everything for the claim.
"""

import argparse
import json
import os
import re
import shutil
import statistics as st
import subprocess
import sys
import tempfile

HIST = 4096          # bars of history handed to Open
FEED = 4096          # bars cycled through update/peek (power of two: index & mask)


# --------------------------------------------------------------------------
# Signature parsing — over the generated sources, one regex per language.
# --------------------------------------------------------------------------

def parse_rust(root):
    """{NAME: {open: [(kind, argname)], call: [argname]}} from the Rust crate."""
    d = os.path.join(root, "ta_codegen/output/rust/library/src/ta_func")
    funcs, declined = {}, {}
    for fn in sorted(os.listdir(d)):
        if not fn.endswith(".rs"):
            continue
        src = open(os.path.join(d, fn)).read()
        # #278 recased the Rust stream API: `SMA_Open` -> `sma_open`,
        # `SMA_Stream` -> `SmaStream`. The report still keys on the uppercase
        # function name, which is what `--funcs`/`--mark` and every other tool
        # here spell, so the emitted call carries its own spelling.
        m = re.search(r"pub fn (\w+)_open\(&self, (.*?)\) -> Result<\((\w+Stream)\b", src)
        if not m:
            continue
        name = m.group(1).upper()
        open_fn = f"{m.group(1)}_open"
        handle = m.group(3)
        args, why = [], None
        for a in [x.strip() for x in m.group(2).split(",") if x.strip()]:
            an, at = [x.strip() for x in a.split(":", 1)]
            kind = {"&[f64]": "slice", "i32": "int", "f64": "real", "MAType": "matype"}.get(at)
            if kind is None:
                why = f"Open parameter {an}: {at}"
                break
            args.append((kind, an))
        if why:
            declined[name] = why
            continue
        # Every `impl <N>_Stream` block, not just the first: the handle carries
        # more than one (the scratch restore lives in its own).
        m2 = None
        for impl in re.finditer(r"impl %s \{(.*?)\n\}" % handle, src, re.S):
            m2 = re.search(r"pub fn update\(&mut self(.*?)\) -> ", impl.group(1))
            if m2:
                break
        if not m2:
            declined[name] = "no update method"
            continue
        call = [x.strip().split(":")[0].strip() for x in m2.group(1).split(",") if x.strip()]
        funcs[name] = {"open": args, "call": call, "openfn": open_fn}
    return funcs, declined


def parse_java(root):
    """Same, from the Java per-function fragments (Core.java is one big file)."""
    d = os.path.join(root, "ta_codegen/output/java/fragments")
    funcs, declined = {}, {}
    for fn in sorted(os.listdir(d)):
        if not fn.endswith(".java"):
            continue
        src = open(os.path.join(d, fn)).read()
        m = re.search(r"public (\w+)_Stream (\w+)_Open\( (.*?) \)\n", src)
        if not m:
            continue
        name = m.group(2)
        args, why = [], None
        for a in [x.strip() for x in m.group(3).split(",") if x.strip()]:
            parts = a.split()
            if len(parts) != 2:
                why = f"Open parameter {a!r}"
                break
            ty, an = parts
            if an.endswith("[]"):          # `double inReal[]`
                an, ty = an[:-2], ty + "[]"
            kind = {"double[]": "slice", "int": "int", "double": "real",
                    "MAType": "matype"}.get(ty)
            if kind is None:
                why = f"Open parameter {an}: {ty}"
                break
            args.append((kind, an))
        if why:
            declined[name] = why
            continue
        cls = re.search(r"public static final class %s_Stream\b(.*?)\n   \}" % name, src, re.S)
        m2 = re.search(r"public (\w+) update\( (.*?) \)", cls.group(1) if cls else src)
        if not m2:
            declined[name] = "no update method"
            continue
        call = [x.split()[1] for x in m2.group(2).split(",") if x.strip()]
        funcs[name] = {"open": args, "call": call, "ret": m2.group(1)}
    return funcs, declined


def series(argname):
    """Which corpus series backs an input, by the generated argument name."""
    n = argname.lower()
    for key, s in (("high", "high"), ("low", "low"), ("close", "close"),
                   ("open", "opn"), ("volume", "vol"), ("periods", "periods")):
        if key in n:
            return s
    return "real1" if n.endswith("1") else "close"


# --------------------------------------------------------------------------
# Harness emission
# --------------------------------------------------------------------------

RUST_HEAD = """// Generated by scripts/stream_ab.py. Do not edit; do not commit.
#![allow(non_snake_case)]
use std::hint::black_box;
use std::time::Instant;
use ta_lib::{Core, MAType};

const HIST: usize = %d;
const FEED: usize = %d;

fn corpus(n: usize) -> Vec<Vec<f64>> {
    // Deterministic random walk. Timing-only: never hashed, unrelated to
    // fuzz_data.h and to bench_corpus.h.
    let mut seed: u64 = 42;
    let mut next = move || {
        seed = seed.wrapping_mul(6364136223846793005).wrapping_add(1442695040888963407);
        ((seed >> 33) as f64) / 2147483648.0_f64 - 0.5
    };
    let mut s = vec![Vec::with_capacity(n); 7];
    let mut px = 100.0_f64;
    for _ in 0..n {
        let op = px;
        px = (px + next()).max(1.0);
        s[0].push(op);
        s[1].push(op.max(px) + 0.5 * next().abs());
        s[2].push((op.min(px) - 0.5 * next().abs()).max(0.5));
        s[3].push(px);
        s[4].push(1000.0 + 100.0 * next());
        s[5].push(px * 0.99 + 0.3);
        s[6].push(14.0);
    }
    s
}

fn main() {
    let iters: usize = std::env::args().nth(1).and_then(|s| s.parse().ok()).unwrap_or(500_000);
    let passes: usize = std::env::args().nth(2).and_then(|s| s.parse().ok()).unwrap_or(5);
    let c = corpus(HIST + FEED);
    let (h_opn, h_high, h_low, h_close, h_vol, h_real1, h_periods) =
        (&c[0][..HIST], &c[1][..HIST], &c[2][..HIST], &c[3][..HIST], &c[4][..HIST], &c[5][..HIST], &c[6][..HIST]);
    let (f_opn, f_high, f_low, f_close, f_vol, f_real1, f_periods) =
        (&c[0][HIST..], &c[1][HIST..], &c[2][HIST..], &c[3][HIST..], &c[4][HIST..], &c[5][HIST..], &c[6][HIST..]);
    let _ = (h_opn, h_high, h_low, h_close, h_vol, h_real1, h_periods,
             f_opn, f_high, f_low, f_close, f_vol, f_real1, f_periods);
    let core = Core::new();
"""

RUST_BLOCK = """
    {
        let mut all: Vec<f64> = Vec::new();
        for _p in 0..passes {
            if let Ok((%(mut)s st, _v)) = core.%(openfn)s(%(oargs)s) {
                for i in 0..1000 { black_box(st.%(call)s(%(cargs)s)); }
                let t0 = Instant::now();
                for i in 0..iters { black_box(st.%(call)s(%(cargs)s)); }
                all.push(t0.elapsed().as_nanos() as f64 / iters as f64);
            }
        }
        if all.is_empty() {
            println!("OPENFAIL %(name)s");
        } else {
            all.sort_by(|a, b| a.partial_cmp(b).unwrap());
            println!("RESULT %(name)s {:.4}", all[0]);
        }
    }"""

# `--call=open` times the OPEN itself, not the per-bar step: one whole warm-up
# pass over HIST bars per iteration, so it costs microseconds where update costs
# nanoseconds. `--iters` is scaled down accordingly in main(). This is the only
# mode that measures work proportional to the history rather than to one bar.
#
# The handle is bound and black_boxed rather than reduced to `.is_ok()`. The
# harness builds with `lto = "thin"`, and observing only the Result discriminant
# lets LLVM delete the whole open wherever the handle does not escape: 25 of 169
# functions then time at ~0.3 ns — a 4096-bar warm-up reported as a quarter of a
# nanosecond, silently reading as "unaffected" rather than "unmeasured".
# Accumulator and stateless opens (AD, OBV, NVI, PVI, WAD, BOP, ROCR) are the
# ones that vanish. update/peek never had this exposure: they keep the handle
# live across the loop and black_box each returned value.
RUST_OPEN_BLOCK = """
    {
        let mut all: Vec<f64> = Vec::new();
        for _p in 0..passes {
            for _ in 0..8 { if let Ok(h) = core.%(openfn)s(%(oargs)s) { black_box(&h); } }
            let t0 = Instant::now();
            for _ in 0..iters { if let Ok(h) = core.%(openfn)s(%(oargs)s) { black_box(&h); } }
            all.push(t0.elapsed().as_nanos() as f64 / iters as f64);
        }
        if core.%(openfn)s(%(oargs)s).is_err() {
            println!("OPENFAIL %(name)s");
        } else {
            all.sort_by(|a, b| a.partial_cmp(b).unwrap());
            println!("RESULT %(name)s {:.4}", all[0]);
        }
    }"""

JAVA_OPEN_BLOCK = """
      try {
         java.util.ArrayList<Double> all = new java.util.ArrayList<>();
         for (int p = 0; p < passes + 2; p++) {
            for (int i = 0; i < 200; i++) { sink += core.%(name)s_Open(%(oargs)s) != null ? 1 : 0; }
            long t0 = System.nanoTime();
            for (int i = 0; i < iters; i++) { sink += core.%(name)s_Open(%(oargs)s) != null ? 1 : 0; }
            if (p >= 2) all.add((System.nanoTime() - t0) / (double) iters);
         }
         java.util.Collections.sort(all);
         System.out.printf("RESULT %(name)s %%.4f%%n", all.get(0));
      } catch (Throwable t) {
         System.out.println("OPENFAIL %(name)s " + t.getClass().getSimpleName());
      }"""

JAVA_HEAD = """// Generated by scripts/stream_ab.py. Do not edit; do not commit.
import io.github.talib.*;

public class BenchStream {
   static final int HIST = %d;
   static final int FEED = %d;
   static double sink = 0.0;
   static double[][] c = new double[7][];
   static double[] h_opn, h_high, h_low, h_close, h_vol, h_real1, h_periods;
   static double[] f_opn, f_high, f_low, f_close, f_vol, f_real1, f_periods;
   static long seed = 42L;

   static double nxt() {
      seed = seed * 6364136223846793005L + 1442695040888963407L;
      return ((double)(seed >>> 33)) / 2147483648.0 - 0.5;
   }

   static void corpus(int n) {
      // Deterministic random walk. Timing-only: never hashed, unrelated to
      // fuzz_data.h and to bench_corpus.h.
      for (int k = 0; k < 7; k++) c[k] = new double[n];
      double px = 100.0;
      for (int i = 0; i < n; i++) {
         double op = px;
         px = Math.max(px + nxt(), 1.0);
         c[0][i] = op;
         c[1][i] = Math.max(op, px) + 0.5 * Math.abs(nxt());
         c[2][i] = Math.max(Math.min(op, px) - 0.5 * Math.abs(nxt()), 0.5);
         c[3][i] = px;
         c[4][i] = 1000.0 + 100.0 * nxt();
         c[5][i] = px * 0.99 + 0.3;
         c[6][i] = 14.0;
      }
   }

   static double[] cut(int k, int from, int len) {
      double[] r = new double[len];
      System.arraycopy(c[k], from, r, 0, len);
      return r;
   }

   public static void main(String[] argv) throws Exception {
      int iters = argv.length > 0 ? Integer.parseInt(argv[0]) : 500000;
      int passes = argv.length > 1 ? Integer.parseInt(argv[1]) : 5;
      corpus(HIST + FEED);
      h_opn = cut(0, 0, HIST); h_high = cut(1, 0, HIST); h_low = cut(2, 0, HIST);
      h_close = cut(3, 0, HIST); h_vol = cut(4, 0, HIST); h_real1 = cut(5, 0, HIST);
      h_periods = cut(6, 0, HIST);
      f_opn = cut(0, HIST, FEED); f_high = cut(1, HIST, FEED); f_low = cut(2, HIST, FEED);
      f_close = cut(3, HIST, FEED); f_vol = cut(4, HIST, FEED); f_real1 = cut(5, HIST, FEED);
      f_periods = cut(6, HIST, FEED);
      Core core = new Core();
"""

# The first two passes are discarded: they are the JIT warming up, not the code.
JAVA_BLOCK = """
      try {
         java.util.ArrayList<Double> all = new java.util.ArrayList<>();
         for (int p = 0; p < passes + 2; p++) {
            Core.%(name)s_Stream st = core.%(name)s_Open(%(oargs)s);
            for (int i = 0; i < 20000; i++) { %(ret)s r = st.%(call)s(%(cargs)s); %(consume)s }
            long t0 = System.nanoTime();
            for (int i = 0; i < iters; i++) { %(ret)s r = st.%(call)s(%(cargs)s); %(consume)s }
            if (p >= 2) all.add((System.nanoTime() - t0) / (double) iters);
         }
         java.util.Collections.sort(all);
         System.out.printf("RESULT %(name)s %%.4f%%n", all.get(0));
      } catch (Throwable t) {
         System.out.println("OPENFAIL %(name)s " + t.getClass().getSimpleName());
      }"""


def open_args(f, lang, name, period, marked):
    """Open's arguments: history slices, and the default sentinel for each param."""
    out = []
    for kind, an in f["open"]:
        if kind == "slice":
            out.append("h_" + series(an))
        elif kind == "int":
            use = period if (period and name in marked) else None
            out.append(str(use) if use else ("i32::MIN" if lang == "rust" else "Integer.MIN_VALUE"))
        elif kind == "real":
            out.append("-4e37")
        else:
            out.append("MAType::DEFAULT" if lang == "rust" else "MAType.DEFAULT")
    return ", ".join(out)


def emit_rust(funcs, names, call, period, marked):
    s = [RUST_HEAD % (HIST, FEED)]
    for name in names:
        f = funcs[name]
        oargs = open_args(f, "rust", name, period, marked)
        if call == "open":
            s.append(RUST_OPEN_BLOCK % {"name": name, "openfn": f["openfn"], "oargs": oargs})
            continue
        s.append(RUST_BLOCK % {
            "name": name, "openfn": f["openfn"],
            "call": call, "mut": "" if call == "peek" else "mut",
            "oargs": oargs,
            "cargs": ", ".join(f"f_{series(a)}[i & {FEED - 1}]" for a in f["call"]),
        })
    s.append("\n}\n")
    return "".join(s)


def emit_java(funcs, names, call, period, marked):
    s = [JAVA_HEAD % (HIST, FEED)]
    for name in names:
        f = funcs[name]
        ret = f["ret"]
        if call == "open":
            s.append(JAVA_OPEN_BLOCK % {
                "name": name, "oargs": open_args(f, "java", name, period, marked),
            })
            continue
        s.append(JAVA_BLOCK % {
            "name": name, "call": call,
            "ret": ret if ret in ("double", "int") else "var",
            "consume": "sink += r;" if ret in ("double", "int") else "sink += (r != null ? 1.0 : 0.0);",
            "oargs": open_args(f, "java", name, period, marked),
            "cargs": ", ".join(f"f_{series(a)}[i & {FEED - 1}]" for a in f["call"]),
        })
    s.append('\n      System.err.println("sink " + sink);\n   }\n}\n')
    return "".join(s)


# --------------------------------------------------------------------------
# Arm staging and build
# --------------------------------------------------------------------------

def run(cmd, cwd=None, check=True):
    p = subprocess.run(cmd, cwd=cwd, capture_output=True, text=True)
    if check and p.returncode != 0:
        sys.exit(f"FAILED: {' '.join(cmd)}\n{p.stdout[-4000:]}{p.stderr[-4000:]}")
    return p


def stage_tree(root, rev, dest, paths, strip):
    os.makedirs(dest, exist_ok=True)
    if rev is None:
        for p in paths:
            shutil.copytree(os.path.join(root, p), os.path.join(dest, os.path.basename(p)))
        return
    ar = subprocess.run(["git", "archive", rev, "--"] + paths,
                        cwd=root, capture_output=True)
    if ar.returncode != 0:
        sys.exit(f"git archive {rev} failed: {ar.stderr.decode()[:2000]}")
    tar = subprocess.run(["tar", "-x", "-C", dest, f"--strip-components={strip}"],
                         input=ar.stdout, capture_output=True)
    if tar.returncode != 0:
        sys.exit(f"tar failed: {tar.stderr.decode()[:2000]}")


RUST_WS = """[workspace]
members = ["dispatch", "library", "bench"]
resolver = "2"

[profile.release]
lto = "thin"
codegen-units = 1
"""

RUST_BENCH_TOML = """[package]
name = "stream_ab"
version = "0.0.0"
edition = "2021"

[dependencies]
ta-lib = { path = "../library" }

[[bin]]
name = "stream_ab"
path = "src/main.rs"
"""


def build_rust(root, arm, rev, work, src):
    d = os.path.join(work, "rust", arm)
    stage_tree(root, rev, d, ["ta_codegen/output/rust/library",
                                  "ta_codegen/output/rust/dispatch"], strip=3)
    os.makedirs(os.path.join(d, "bench/src"), exist_ok=True)
    open(os.path.join(d, "Cargo.toml"), "w").write(RUST_WS)
    open(os.path.join(d, "bench/Cargo.toml"), "w").write(RUST_BENCH_TOML)
    open(os.path.join(d, "bench/src/main.rs"), "w").write(src)
    run(["cargo", "build", "--release", "--bin", "stream_ab"], cwd=d)
    return [os.path.join(d, "target/release/stream_ab")]


def build_java(root, arm, rev, work, src):
    d = os.path.join(work, "java", arm)
    stage_tree(root, rev, os.path.join(d, "src"),
                   ["ta_codegen/output/java/library/src/main/java"], strip=7)
    classes = os.path.join(d, "classes")
    os.makedirs(classes, exist_ok=True)
    files = [os.path.join(dp, f) for dp, _, fs in os.walk(os.path.join(d, "src"))
             for f in fs if f.endswith(".java")]
    run(["javac", "-nowarn", "-d", classes] + files)
    open(os.path.join(d, "BenchStream.java"), "w").write(src)
    run(["javac", "-nowarn", "-cp", classes, "-d", classes,
         os.path.join(d, "BenchStream.java")])
    return ["java", "-cp", classes, "BenchStream"]


# --------------------------------------------------------------------------
# Run and report
# --------------------------------------------------------------------------

def measure(cmd, iters, passes):
    out = run(cmd + [str(iters), str(passes)]).stdout
    res, failed = {}, []
    for line in out.splitlines():
        p = line.split()
        if p[:1] == ["RESULT"]:
            res[p[1]] = float(p[2])
        elif p[:1] == ["OPENFAIL"]:
            failed.append(p[1])
    return res, failed


def report(lang, call, data, marked, rounds, iters):
    names = sorted(set(data["head"]) & set(data["base"]))
    rows = []
    for n in names:
        a, b = st.median(data["base"][n]), st.median(data["head"][n])
        spread = max((max(v) - min(v)) / st.median(v) * 100 for v in
                     (data["base"][n], data["head"][n]))
        rows.append((n, a, b, (b - a) / a * 100, b - a, spread))
    mark = [r for r in rows if r[0] in marked] if marked else rows
    ctl = [r for r in rows if r[0] not in marked] if marked else []

    print(f"\n=== {lang} {call}: {rounds} interleaved rounds, iters={iters} ===")
    print(f"{'func':<16}{'base ns':>10}{'head ns':>10}{'chg %':>9}{'chg ns':>9}{'spread%':>9}")
    for r in sorted(mark, key=lambda x: x[3])[:40 if not marked else None]:
        print(f"{r[0]:<16}{r[1]:>10.3f}{r[2]:>10.3f}{r[3]:>8.1f}%{r[4]:>9.2f}{r[5]:>8.1f}%")
    if marked:
        print(f"\nmarked   n={len(mark):<4} median {st.median([r[3] for r in mark]):+.1f}%"
              f"  ({st.median([r[4] for r in mark]):+.2f} ns)")
    if ctl:
        print(f"control  n={len(ctl):<4} median {st.median([r[3] for r in ctl]):+.1f}%"
              f"  ({st.median([r[4] for r in ctl]):+.2f} ns)"
              f"  past 10%: {sum(1 for r in ctl if abs(r[3]) > 10)}"
              f"  past 25%: {sum(1 for r in ctl if abs(r[3]) > 25)}")
        print("control outliers: " + ", ".join(
            f"{r[0]} {r[3]:+.0f}%" for r in sorted(ctl, key=lambda x: -abs(x[3]))[:5]))
        print(f"median control spread {st.median([r[5] for r in ctl]):.1f}%"
              "   <- read this before any row above")
    return rows


def main():
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--base", required=True, help="git revision to compare the working tree against")
    ap.add_argument("--lang", default="rust,java", help="rust,java (default both)")
    ap.add_argument("--call", default="update", choices=["update", "peek", "open"])
    ap.add_argument("--rounds", type=int, default=6, help="interleaved A/B rounds (default 6)")
    ap.add_argument("--iters", type=int, default=None,
                    help="timed calls per pass (default 500000; 2000 for --call=open, "
                         "which does a whole warm-up pass per call)")
    ap.add_argument("--passes", type=int, default=5, help="passes per round; the best is kept")
    ap.add_argument("--mark", default="", help="CSV of functions under test; the rest are controls")
    ap.add_argument("--funcs", default="", help="CSV: restrict to these functions")
    ap.add_argument("--period", type=int, help="force this period on the marked functions")
    ap.add_argument("--workdir", help="build directory (default: a temp dir, removed at exit)")
    ap.add_argument("--json", help="write the raw per-round samples here")
    args = ap.parse_args()
    if args.iters is None:
        # An open replays the whole HIST-bar warm-up, so it costs microseconds
        # where update/peek cost nanoseconds; 500k of them would run for hours.
        args.iters = 2000 if args.call == "open" else 500000

    root = run(["git", "rev-parse", "--show-toplevel"]).stdout.strip()
    marked = {f for f in args.mark.split(",") if f}
    only = {f for f in args.funcs.split(",") if f}
    work = args.workdir or tempfile.mkdtemp(prefix="ta_stream_ab_")
    keep = bool(args.workdir)

    try:
        for lang in [l for l in args.lang.split(",") if l]:
            funcs, declined = (parse_rust(root) if lang == "rust" else parse_java(root))
            if declined:
                print(f"[{lang}] not benchable ({len(declined)}): " +
                      ", ".join(f"{k} ({v})" for k, v in declined.items()))
            names = sorted(f for f in funcs if not only or f in only)
            missing = (marked | only) - set(names)
            if missing:
                sys.exit(f"[{lang}] requested but not benchable: {sorted(missing)}")
            if not names:
                sys.exit(f"[{lang}] parsed 0 benchable functions - the generated "
                         f"API spelling has moved out from under this parser. A run "
                         f"that measures nothing must not report a clean sweep.")
            print(f"[{lang}] {len(names)} functions, {len(marked)} marked; building both arms")

            emit = emit_rust if lang == "rust" else emit_java
            src = emit(funcs, names, args.call, args.period, marked)
            build = build_rust if lang == "rust" else build_java
            cmds = {"head": build(root, "head", None, work, src),
                    "base": build(root, "base", args.base, work, src)}

            data = {"head": {}, "base": {}}
            for r in range(args.rounds):
                # Alternate which arm goes first, so a box that drifts busier
                # during a round does not bias every row the same way.
                for arm in (("head", "base") if r % 2 == 0 else ("base", "head")):
                    res, failed = measure(cmds[arm], args.iters, args.passes)
                    if failed:
                        print(f"[{lang}/{arm}] Open failed: {', '.join(sorted(failed))}")
                    for k, v in res.items():
                        data[arm].setdefault(k, []).append(v)
                print(f"[{lang}] round {r + 1}/{args.rounds}", file=sys.stderr)

            rows = report(lang, args.call, data, marked, args.rounds, args.iters)
            if args.json:
                path = args.json if len(args.lang.split(",")) == 1 else f"{args.json}.{lang}"
                json.dump({"lang": lang, "call": args.call, "base": args.base,
                           "rounds": args.rounds, "iters": args.iters,
                           "raw": data, "rows": rows}, open(path, "w"), indent=1)
                print(f"[{lang}] raw samples -> {path}")
    finally:
        if not keep:
            shutil.rmtree(work, ignore_errors=True)


if __name__ == "__main__":
    main()
