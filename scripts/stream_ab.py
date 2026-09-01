#!/usr/bin/env python3
"""A/B the Rust, Java and C# streaming tier between two revisions of this tree.

`ta_bench_stream` measures the C streaming tier only. This measures the other
three: it times `update` (or `peek`) per bar for every streaming function -- or
`open`, which times a whole warm-up pass rather than one bar -- in two
arms — the working tree and a git revision — and reports the per-function
change with the untouched functions as the control.

    scripts/stream_ab.py --base=origin/dev
    scripts/stream_ab.py --base=HEAD~1 --lang=rust --call=peek --mark=MIN,MAX
    scripts/stream_ab.py --base=origin/dev --lang=csharp --call=peek

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

Inputs: the generated `ta_codegen/output/rust/library` crate,
`ta_codegen/output/java/fragments` and `ta_codegen/output/csharp/library`.
Nothing else — no ta_abstract, no C library, no servers, no ta_regtest. Needs
`cargo` for --lang=rust, `javac`/`java` for --lang=java, `dotnet` for
--lang=csharp. Both revisions must expose the same stream API surface; if they
do not, the base arm fails to compile and says so. For C# that bar is higher
than for the other two: `TALib.csproj` sets `TreatWarningsAsErrors`, so a base
arm from an older revision must still compile *warning-clean* under whatever SDK
is installed today, not merely compile.

The harness calls every function through its generated signature with the
default-value sentinels (i32::MIN / -4e37 / MAType.DEFAULT), so it needs no
per-function knowledge and nothing here needs editing when a function is added.
A function whose Open signature it cannot render is REPORTED, never dropped
silently.

Every arm takes the function name off the FILENAME, never off the method, and
asserts a floor (FUNC_FLOOR) on how many it parsed. #278 recased the Rust and
Java stream APIs and each arm's signature regex still spelled the old names, so
both parsed zero functions; Rust's exited 0 and Java's reported every requested
function as "not benchable" for a day. Neither breakage was caught by anything
except someone trying to use the tool. A benchmark that measures nothing must
not be able to exit 0.

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

# Floor on how many functions an arm must parse before it is allowed to measure
# anything. There are 176 streaming functions today, in each of the three
# languages, and the number only ever grows. This is not a style check: #278
# recased the Rust and Java stream APIs, both arms' Open regexes still spelled
# the old names, and each silently parsed ZERO -- Rust's until c308e789, Java's
# until this commit. Neither was found by CI; both were found by someone trying
# to use the tool. A benchmark that measures nothing must fail, loudly, saying
# what moved.
# ONE harness, compiled against BOTH arms, so this tool A/Bs an implementation
# and NOT a signature: if the API itself moved between the revisions, the arm
# whose library does not have today's shape fails to compile and the run dies.
# #310 is the worked example — `update` grew a caller-owned sink, so a base from
# before it cannot build the harness this parses out of the working tree. There
# is no silent-wrong mode here, only a loud one; measuring across such a change
# needs a throwaway per-arm harness instead.
FUNC_FLOOR = 170


# --------------------------------------------------------------------------
# Signature parsing — over the generated sources, one regex per language.
#
# Every arm takes the function NAME off the filename and decides indicator-ness
# on something outside the stream API, so a recasing of that API turns a
# function into a reported decline rather than a silent disappearance. The
# emitted call still carries the spelling read out of the source, because that
# is what has to compile; only the report key is filename-derived, which is what
# `--funcs`/`--mark` and every other tool in this tree spell.
# --------------------------------------------------------------------------

def parse_rust(root):
    """{NAME: {open: [(kind, argname)], call: [argname]}} from the Rust crate."""
    d = os.path.join(root, "ta_codegen/output/rust/library/src/ta_func")
    funcs, declined = {}, {}
    for fn in sorted(os.listdir(d)):
        if not fn.endswith(".rs"):
            continue
        name = fn[:-3].upper()
        src = open(os.path.join(d, fn)).read()
        # The batch entry point, not the stream API: it is what separates an
        # indicator from types.rs / mod.rs / the shared stream helpers, and it
        # does not move when the stream API is recased.
        if not re.search(r"pub fn %s_Lookback\(" % re.escape(name), src):
            continue
        m = re.search(r"pub fn (\w+_open)\(&self, ([^)]*?)\) -> Result<\((\w+Stream)\b", src)
        if not m:
            declined[name] = "no Open signature matched"
            continue
        open_fn, handle = m.group(1), m.group(3)
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
        # Every `impl <handle>` block, not just the first: the handle carries
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
        if not fn.startswith("Core_") or not fn.endswith(".java"):
            continue
        name = fn[len("Core_"):-len(".java")]
        src = open(os.path.join(d, fn)).read()
        # #278 recased the Java stream API: `SMA_Open` -> `smaOpen`,
        # `SMA_Stream` -> `SmaStream`. The report keys on the uppercase function
        # name, which is what `--funcs`/`--mark` and every other tool here
        # spell, so it comes off the FILENAME and the emitted call carries its
        # own spelling. `OpenAndFill` cannot collide: the `(` is required.
        m = re.search(r"public (\w+Stream) (\w+Open)\( (.*?) \)\n", src)
        if not m:
            declined[name] = "no Open signature matched"
            continue
        cls, open_fn = m.group(1), m.group(2)
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
        blk = re.search(r"public static final class %s\b(.*?)\n   \}" % cls, src, re.S)
        m2 = re.search(r"public (\w+) update\( (.*?) \)", blk.group(1) if blk else src)
        if not m2:
            declined[name] = "no update method"
            continue
        call = [x.split()[1] for x in m2.group(2).split(",") if x.strip()]
        # Since #310 a MULTI-OUTPUT tier returns void and takes a caller-owned
        # `<N>Out` as its last argument, so the bars are the call list minus
        # that one. Detected off the emitted TYPE, not a name list: the sink is
        # the only parameter whose type is not a bar. A harness that missed this
        # would emit `void r = st.update(bar, out)` against an undeclared `out`
        # -- loud, but only once someone runs the tool.
        sinkty = sinkfield = None
        if m2.group(1) == "void":
            sinkty = m2.group(2).rsplit(",", 1)[-1].strip().split()[0]
            call = call[:-1]
            fm = re.search(r"public static final class %s\b.*?public (?:double|int) (\w+);"
                           % re.escape(sinkty), src, re.S)
            if not fm:
                declined[name] = f"no field found on sink type {sinkty}"
                continue
            sinkfield = fm.group(1)
        funcs[name] = {"open": args, "call": call, "ret": m2.group(1),
                       "cls": cls, "openfn": open_fn,
                       "sinkty": sinkty, "sinkfield": sinkfield}
    return funcs, declined


def parse_csharp(root):
    """Same, from the generated C# per-function `partial class Core` files."""
    d = os.path.join(root, "ta_codegen/output/csharp/library/src")
    funcs, declined = {}, {}
    for fn in sorted(os.listdir(d)):
        if not (fn.startswith("Core_") and fn.endswith(".cs")):
            continue                       # MAType.cs, FuncUnstId.cs
        name = fn[len("Core_"):-len(".cs")]
        src = open(os.path.join(d, fn)).read()
        m = re.search(r"^   public (\w+Stream) (\w+Open)\( ([^)]*?) \)$", src, re.M)
        if not m:
            declined[name] = "no Open signature matched"
            continue
        handle, open_fn = m.group(1), m.group(2)
        args, why = [], None
        for a in [x.strip() for x in m.group(3).split(",") if x.strip()]:
            ty, an = a.rsplit(" ", 1)
            kind = {"ReadOnlySpan<double>": "slice", "int": "int",
                    "double": "real", "MAType": "matype"}.get(ty)
            if kind is None:
                why = f"Open parameter {an}: {ty}"
                break
            args.append((kind, an))
        if why:
            declined[name] = why
            continue
        m2 = re.search(r"^      public (\S+) Update\( ([^)]*?) \)$", src, re.M)
        if not m2:
            declined[name] = "no Update method"
            continue
        ret = m2.group(1)
        # A multi-output Update returns a `readonly record struct` declared in
        # this same file. Consuming its first component keeps the sink one
        # add — GetHashCode() would put a different amount of work in the timed
        # loop for the multi-output functions than for the single-output ones.
        member = None
        if ret not in ("double", "int"):
            mv = re.search(r"public readonly record struct %s\( \w+ (\w+)" % re.escape(ret), src)
            if not mv:
                declined[name] = f"Update returns {ret}, whose shape did not parse"
                continue
            member = mv.group(1)
        call = [x.rsplit(" ", 1)[1] for x in m2.group(2).split(",") if x.strip()]
        funcs[name] = {"open": args, "call": call, "ret": ret, "member": member,
                       "openfn": open_fn}
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
            for (int i = 0; i < 200; i++) { sink += core.%(openfn)s(%(oargs)s) != null ? 1 : 0; }
            long t0 = System.nanoTime();
            for (int i = 0; i < iters; i++) { sink += core.%(openfn)s(%(oargs)s) != null ? 1 : 0; }
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
         %(decl)s
         for (int p = 0; p < passes + 2; p++) {
            Core.%(cls)s st = core.%(openfn)s(%(oargs)s);
            for (int i = 0; i < 20000; i++) { %(body)s }
            long t0 = System.nanoTime();
            for (int i = 0; i < iters; i++) { %(body)s }
            if (p >= 2) all.add((System.nanoTime() - t0) / (double) iters);
         }
         java.util.Collections.sort(all);
         System.out.printf("RESULT %(name)s %%.4f%%n", all.get(0));
      } catch (Throwable t) {
         System.out.println("OPENFAIL %(name)s " + t.getClass().getSimpleName());
      }"""


# The .NET arm. Two differences from Java, both deliberate:
#
# `TieredCompilation` is pinned OFF in the harness csproj. With tiering on, a
# method is first jitted at tier 0 and only re-jitted after it is called enough,
# so the FIRST timed block in the process reads systematically slower than the
# later ones purely from block order -- the hand-rolled bench that preceded this
# arm showed exactly that. Discarding warm-up passes does not fix it, because
# the effect is per-process and per-position, not per-pass. Off means every
# method is fully optimised on its first call. The cost, stated rather than
# hidden: this also disables dynamic PGO, so what is measured is the static-opt
# code, not what a long-lived production process would eventually run. Both arms
# get identical treatment, so the CHANGE column -- the output -- is unaffected;
# the absolute ns are not production numbers, which was already true of the ns
# columns in every arm here.
#
# Timing is `Stopwatch.GetTimestamp()` scaled by `Stopwatch.Frequency`, not
# `DateTime`, and every number is formatted with the invariant culture so a
# box with a comma decimal separator does not emit rows this script cannot parse.
CSHARP_HEAD = """// Generated by scripts/stream_ab.py. Do not edit; do not commit.
using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Globalization;
using TALib;

static class BenchStream
{
   const int HIST = %d;
   const int FEED = %d;
   static double sink = 0.0;
   static double[][] c = new double[7][];
   static double[] h_opn, h_high, h_low, h_close, h_vol, h_real1, h_periods;
   static double[] f_opn, f_high, f_low, f_close, f_vol, f_real1, f_periods;
   static long seed = 42L;

   static double nxt()
   {
      seed = unchecked(seed * 6364136223846793005L + 1442695040888963407L);
      return ((double)(seed >>> 33)) / 2147483648.0 - 0.5;
   }

   static void corpus(int n)
   {
      // Deterministic random walk. Timing-only: never hashed, unrelated to
      // fuzz_data.h and to bench_corpus.h. Bit-for-bit the Rust and Java walk.
      for (int k = 0; k < 7; k++) c[k] = new double[n];
      double px = 100.0;
      for (int i = 0; i < n; i++)
      {
         double op = px;
         px = Math.Max(px + nxt(), 1.0);
         c[0][i] = op;
         c[1][i] = Math.Max(op, px) + 0.5 * Math.Abs(nxt());
         c[2][i] = Math.Max(Math.Min(op, px) - 0.5 * Math.Abs(nxt()), 0.5);
         c[3][i] = px;
         c[4][i] = 1000.0 + 100.0 * nxt();
         c[5][i] = px * 0.99 + 0.3;
         c[6][i] = 14.0;
      }
   }

   static double[] cut(int k, int from, int len)
   {
      double[] r = new double[len];
      Array.Copy(c[k], from, r, 0, len);
      return r;
   }

   static readonly double TICK_NS = 1e9 / Stopwatch.Frequency;
   static double NsPer(long t0, int iters) => (Stopwatch.GetTimestamp() - t0) * TICK_NS / iters;
   static void Say(string name, double ns) =>
      Console.WriteLine("RESULT " + name + " " + ns.ToString("F4", CultureInfo.InvariantCulture));

   static void Main(string[] argv)
   {
      CultureInfo.DefaultThreadCurrentCulture = CultureInfo.InvariantCulture;
      int iters = argv.Length > 0 ? int.Parse(argv[0], CultureInfo.InvariantCulture) : 500000;
      int passes = argv.Length > 1 ? int.Parse(argv[1], CultureInfo.InvariantCulture) : 5;
      corpus(HIST + FEED);
      h_opn = cut(0, 0, HIST); h_high = cut(1, 0, HIST); h_low = cut(2, 0, HIST);
      h_close = cut(3, 0, HIST); h_vol = cut(4, 0, HIST); h_real1 = cut(5, 0, HIST);
      h_periods = cut(6, 0, HIST);
      f_opn = cut(0, HIST, FEED); f_high = cut(1, HIST, FEED); f_low = cut(2, HIST, FEED);
      f_close = cut(3, HIST, FEED); f_vol = cut(4, HIST, FEED); f_real1 = cut(5, HIST, FEED);
      f_periods = cut(6, HIST, FEED);
      Core core = new Core();
"""

CSHARP_BLOCK = """
      try
      {
         var all = new List<double>();
         for (int p = 0; p < passes + 2; p++)
         {
            var st = core.%(openfn)s(%(oargs)s);
            for (int i = 0; i < 20000; i++) { var r = st.%(call)s(%(cargs)s); %(consume)s }
            long t0 = Stopwatch.GetTimestamp();
            for (int i = 0; i < iters; i++) { var r = st.%(call)s(%(cargs)s); %(consume)s }
            if (p >= 2) all.Add(NsPer(t0, iters));
         }
         all.Sort();
         Say("%(name)s", all[0]);
      }
      catch (Exception e) { Console.WriteLine("OPENFAIL %(name)s " + e.GetType().Name); }"""

CSHARP_OPEN_BLOCK = """
      try
      {
         var all = new List<double>();
         for (int p = 0; p < passes + 2; p++)
         {
            for (int i = 0; i < 200; i++) { sink += core.%(openfn)s(%(oargs)s) != null ? 1 : 0; }
            long t0 = Stopwatch.GetTimestamp();
            for (int i = 0; i < iters; i++) { sink += core.%(openfn)s(%(oargs)s) != null ? 1 : 0; }
            if (p >= 2) all.Add(NsPer(t0, iters));
         }
         all.Sort();
         Say("%(name)s", all[0]);
      }
      catch (Exception e) { Console.WriteLine("OPENFAIL %(name)s " + e.GetType().Name); }"""


# How each language spells "use the documented default for this parameter".
SENTINEL = {
    "rust":   {"int": "i32::MIN", "real": "-4e37", "matype": "MAType::DEFAULT"},
    "java":   {"int": "Integer.MIN_VALUE", "real": "-4e37", "matype": "MAType.DEFAULT"},
    "csharp": {"int": "int.MinValue", "real": "-4e37", "matype": "MAType.DEFAULT"},
}


def open_args(f, lang, name, period, marked):
    """Open's arguments: history slices, and the default sentinel for each param."""
    out = []
    for kind, an in f["open"]:
        if kind == "slice":
            out.append("h_" + series(an))
        elif kind == "int" and period and name in marked:
            out.append(str(period))
        else:
            out.append(SENTINEL[lang][kind])
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
                "name": name, "openfn": f["openfn"],
                "oargs": open_args(f, "java", name, period, marked),
            })
            continue
        cargs = ", ".join(f"f_{series(a)}[i & {FEED - 1}]" for a in f["call"])
        if f.get("sinkty"):
            # Allocated once and reused, which is the usage the sink's own
            # javadoc prescribes and the one worth timing -- allocating per
            # iteration would measure the shape #310 removed.
            decl = "Core.%s o = new Core.%s();" % (f["sinkty"], f["sinkty"])
            body = "st.%s(%s, o); sink += o.%s;" % (call, cargs, f["sinkfield"])
        else:
            decl = ""
            consume = "sink += r;" if ret in ("double", "int") else "sink += (r != null ? 1.0 : 0.0);"
            body = "%s r = st.%s(%s); %s" % (
                ret if ret in ("double", "int") else "var", call, cargs, consume)
        s.append(JAVA_BLOCK % {
            "name": name, "cls": f["cls"], "openfn": f["openfn"],
            "decl": decl, "body": body,
            "oargs": open_args(f, "java", name, period, marked),
        })
    s.append('\n      System.err.println("sink " + sink);\n   }\n}\n')
    return "".join(s)


def emit_csharp(funcs, names, call, period, marked):
    s = [CSHARP_HEAD % (HIST, FEED)]
    for name in names:
        f = funcs[name]
        if call == "open":
            s.append(CSHARP_OPEN_BLOCK % {
                "name": name, "openfn": f["openfn"],
                "oargs": open_args(f, "csharp", name, period, marked),
            })
            continue
        s.append(CSHARP_BLOCK % {
            "name": name, "openfn": f["openfn"],
            "call": call.capitalize(),          # C# spells them Update / Peek
            "consume": "sink += r;" if f["member"] is None else f"sink += r.{f['member']};",
            "oargs": open_args(f, "csharp", name, period, marked),
            "cargs": ", ".join(f"f_{series(a)}[i & {FEED - 1}]" for a in f["call"]),
        })
    s.append('\n      Console.Error.WriteLine("sink " + sink);\n   }\n}\n')
    return "".join(s)


# --------------------------------------------------------------------------
# Arm staging and build
# --------------------------------------------------------------------------

def run(cmd, cwd=None, check=True, env=None):
    p = subprocess.run(cmd, cwd=cwd, capture_output=True, text=True, env=env)
    if check and p.returncode != 0:
        sys.exit(f"FAILED: {' '.join(cmd)}\n{p.stdout[-4000:]}{p.stderr[-4000:]}")
    return p


def stage_tree(root, rev, dest, paths, strip, ignore=None):
    os.makedirs(dest, exist_ok=True)
    if rev is None:
        # `ignore` matters for the head arm only: `git archive` never carries
        # untracked build output, but a copytree of the working tree does, and a
        # stale obj/project.assets.json pins absolute paths from the tree it was
        # restored in — which then fails the staged build with a message about a
        # directory the user never asked to build.
        ig = shutil.ignore_patterns(*ignore) if ignore else None
        for p in paths:
            shutil.copytree(os.path.join(root, p), os.path.join(dest, os.path.basename(p)),
                            ignore=ig)
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


# TieredCompilation off: see the CSHARP_HEAD comment. The rest is the minimum
# that compiles — this project is a throwaway, so it does not inherit the
# library's TreatWarningsAsErrors/Nullable, which exist to police shipped code.
CSHARP_BENCH_CSPROJ = """<Project Sdk="Microsoft.NET.Sdk">
  <PropertyGroup>
    <OutputType>Exe</OutputType>
    <TargetFramework>%s</TargetFramework>
    <LangVersion>latest</LangVersion>
    <Nullable>disable</Nullable>
    <AssemblyName>stream_ab</AssemblyName>
    <Optimize>true</Optimize>
    <TieredCompilation>false</TieredCompilation>
    <TieredPGO>false</TieredPGO>
    <InvariantGlobalization>true</InvariantGlobalization>
  </PropertyGroup>
  <ItemGroup>
    <ProjectReference Include="../library/TALib.csproj" />
  </ItemGroup>
</Project>
"""


def build_csharp(root, arm, rev, work, src):
    d = os.path.join(work, "csharp", arm)
    stage_tree(root, rev, d, ["ta_codegen/output/csharp/library"], strip=3,
               ignore=["bin", "obj"])
    csproj = os.path.join(d, "library/TALib.csproj")
    if not os.path.exists(csproj):
        sys.exit(f"[csharp/{arm}] no TALib.csproj in {rev or 'the working tree'}")
    # Read the TFM off the arm's own library rather than pinning one here: the
    # two arms are different revisions and the library has retargeted before.
    m = re.search(r"<TargetFramework>(.*?)</TargetFramework>", open(csproj).read())
    if not m:
        sys.exit(f"[csharp/{arm}] TALib.csproj declares no <TargetFramework>")
    tfm = m.group(1)
    bench = os.path.join(d, "bench")
    os.makedirs(bench, exist_ok=True)
    open(os.path.join(bench, "stream_ab.csproj"), "w").write(CSHARP_BENCH_CSPROJ % tfm)
    open(os.path.join(bench, "Program.cs"), "w").write(src)
    env = dict(os.environ, DOTNET_CLI_TELEMETRY_OPTOUT="1", DOTNET_NOLOGO="1")
    run(["dotnet", "build", "-c", "Release", "--nologo", "stream_ab.csproj"],
        cwd=bench, env=env)
    return ["dotnet", os.path.join(bench, f"bin/Release/{tfm}/stream_ab.dll")]


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


# parse / emit / build, per --lang value.
ARMS = {
    "rust":   (parse_rust, emit_rust, build_rust),
    "java":   (parse_java, emit_java, build_java),
    "csharp": (parse_csharp, emit_csharp, build_csharp),
}

# Checked up front for every requested arm: the rust leg alone is minutes, and
# finding out only then that the next arm has no compiler wastes all of it.
TOOLCHAIN = {"rust": ["cargo"], "java": ["javac", "java"], "csharp": ["dotnet"]}


def main():
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--base", required=True, help="git revision to compare the working tree against")
    ap.add_argument("--lang", default="rust,java,csharp",
                    help="rust,java,csharp (default all three)")
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

    langs = [l for l in args.lang.split(",") if l]
    for lang in langs:
        if lang not in ARMS:
            sys.exit(f"unknown --lang={lang}; known: {', '.join(sorted(ARMS))}")
        for tool in TOOLCHAIN[lang]:
            if shutil.which(tool) is None:
                sys.exit(f"--lang={lang} needs `{tool}` on PATH, and it is not "
                         f"there. Drop it from --lang to run the other arms.")

    root = run(["git", "rev-parse", "--show-toplevel"]).stdout.strip()
    marked = {f for f in args.mark.split(",") if f}
    only = {f for f in args.funcs.split(",") if f}
    work = args.workdir or tempfile.mkdtemp(prefix="ta_stream_ab_")
    keep = bool(args.workdir)

    try:
        for lang in langs:
            parse, emit, build = ARMS[lang]
            funcs, declined = parse(root)
            if declined:
                # Capped: when a rename takes out the whole API, every function
                # declines for the same reason, and 176 of them bury the one
                # line that says what to do about it.
                shown = list(declined.items())[:8]
                print(f"[{lang}] not benchable ({len(declined)}): " +
                      ", ".join(f"{k} ({v})" for k, v in shown) +
                      (f", ... and {len(declined) - len(shown)} more"
                       if len(declined) > len(shown) else ""))
            # The floor is on what was PARSED, not on what `--funcs` selected:
            # it is a claim about the parser, and it must fire whether or not
            # this invocation narrowed the set.
            if len(funcs) < FUNC_FLOOR:
                sys.exit(f"[{lang}] parsed {len(funcs)} streaming functions, "
                         f"below the floor of {FUNC_FLOOR} - the generated API "
                         f"spelling has moved out from under this parser. A run "
                         f"that measures nothing must not report a clean sweep. "
                         f"Fix parse_{lang}() (and raise the floor if the count "
                         f"is genuinely lower now).")
            names = sorted(f for f in funcs if not only or f in only)
            missing = (marked | only) - set(names)
            if missing:
                sys.exit(f"[{lang}] requested but not benchable: {sorted(missing)}")
            print(f"[{lang}] {len(names)} functions, {len(marked)} marked; building both arms")

            src = emit(funcs, names, args.call, args.period, marked)
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
                path = args.json if len(langs) == 1 else f"{args.json}.{lang}"
                json.dump({"lang": lang, "call": args.call, "base": args.base,
                           "rounds": args.rounds, "iters": args.iters,
                           "raw": data, "rows": rows}, open(path, "w"), indent=1)
                print(f"[{lang}] raw samples -> {path}")
    finally:
        if not keep:
            shutil.rmtree(work, ignore_errors=True)


if __name__ == "__main__":
    main()
