# Proposal: Adopt Fused Multiply-Add (FMA) as TA-Lib's Numerical Contract

Status: **request for comments** — no implementation. Written 2026-07-05.

## The tradeoff, up front

TA-Lib would switch its floating-point arithmetic to explicit fused
multiply-add (`fma(a,b,c)`) in the value-path of its indicators. The cost is a
**one-time change in output values**, bounded by a variation margin we will
test mechanically: every real-valued output stays **numerically equal to
today's result within `|new − old| ≤ 1e-12 × max(1, |old|)`** per element, and
every integer output (candlestick patterns, MINMAXINDEX, HT_TRENDMODE, ...)
stays **bit-identical**. In exchange, recursive indicators get **~25% faster
on every x86 CPU made since 2013 and every ARM64 CPU ever made**, and TA-Lib's
results become — for the first time — **bit-identical across x86-64, ARM64,
and every other platform**, because `fma` is a single IEEE-754
correctly-rounded operation with one defined answer everywhere. The margin
applies once, at the transition; afterwards results are frozen again under our
bit-exact regression gates, on a stronger cross-platform footing than today.

## Why now

- **Hardware:** FMA has been in every x86 CPU since 2013 (Intel Haswell, AMD
  Piledriver) and is part of the ARMv8 base ISA (every Apple Silicon,
  Graviton, and phone CPU). The hardware transition finished a decade ago.
- **Ecosystem:** RHEL 10 builds everything for x86-64-v3 (FMA required);
  Windows 11's CPU floor is FMA-capable; other distros are following. Only
  *compiler defaults* still target 2003-era baseline x86-64, and they exist to
  protect a disappearing tail.
- **Determinism today is weaker than it looks:** with default compiler flags
  (`-ffp-contract=fast`), whether a compiler fuses `a*b+c` is target- and
  expression-shape-dependent. Explicit `fma()` in the generated source, with
  contraction pinned off, replaces that gray zone with one portable contract.

## What changes, and what does not

`ta_codegen` (the generator producing all backends from one canonical source)
would emit explicit `fma()` / `mul_add()` at mechanically enumerated
**value-path** sites: the smoothing recursions (`(x − prev)·k + prev` in the
EMA family, Wilder smoothing, adaptive averages) and similar multiply-add
accumulations. Affected outputs shift by 1–2 ulp per fused operation; the
propagation bound through the deepest cascades (TEMA, TRIX, T3) stays orders
of magnitude below the 1e-12 margin on realistic data.

**Never fused: decision paths.** Any expression consumed by a comparison that
selects a branch or an integer output is excluded from fusion — for example
the candlestick penetration tests (`close > prior_close + realbody ×
penetration` in CDLMORNINGSTAR, CDLPIERCING, ...) and SAR's reversal
arithmetic. With compiler contraction pinned off, this is a structural
guarantee, not a hope: **all pattern/signal outputs remain bit-identical**,
and no knife-edge signal can flip.

## Validation plan (the margin, mechanically enforced)

TA-Lib already has a bit-exact differential fuzz harness that replays
deterministic seeded inputs (7 data shapes × seeds × sizes × parameter
vectors × subranges, ≈118k comparisons across all 161 functions) against a
frozen reference build. The transition reuses it:

1. **One-time transition gate:** old build vs FMA build over the full matrix.
   Real outputs must satisfy `|new − old| ≤ 1e-12 × max(1, |old|)`; integer
   outputs must match bit-for-bit. Any element outside the margin fails the
   transition.
2. **After the transition:** the FMA build becomes the frozen reference and
   all gates return to **bitwise** (memcmp) comparison — including a
   cross-platform leg, since explicit `fma` makes x86-64-v3 and ARM64 outputs
   bit-identical rather than "hopefully close".

The 1e-12 margin is ~50× looser than the analytic worst case for the deepest
EMA cascades at long periods, and one million times tighter than the 1e-6
epsilon used for cross-language spot checks — tight enough that any real bug
still fails loudly.

## Measured performance (200,000 bars per call, x86-64)

| Configuration | DEMA-class EMA recursion |
|---|---:|
| baseline x86-64 (no FMA possible) | 427 µs |
| FMA hardware, fused | **311 µs (−27%)** |
| FMA hardware, fusion disabled | 438 µs |

The win applies to the recursive/smoothing families (EMA, DEMA, TEMA, TRIX,
T3, MACD variants, KAMA, Wilder-smoothed indicators, the Hilbert Transform
family); pure comparison/scan indicators are unaffected.

## Distribution

- **C binaries:** ship x86-64-v3 as the primary artifact with a baseline
  x86-64 fallback artifact (the same policy RHEL 10 adopted). ARM64 artifacts
  get FMA with no action. Building from source on a non-FMA CPU still works:
  `fma()` falls back to a slower software path with **identical bits**.
- **Rust crate (crates.io):** dependencies compile on the user's machine with
  the user's flags; default x86-64 flags route `mul_add` through a slower
  correctly-rounded fallback (same bits, less speed). Users get full speed
  with `RUSTFLAGS="-C target-cpu=x86-64-v3"` (or `native`); ARM64 users get
  it by default. If demand warrants, per-function runtime dispatch
  (`is_x86_feature_detected!`) can deliver FMA speed on default builds at the
  cost of extra generated code.

## Timing

Independently of this proposal, an ongoing optimization pass (buffer-free
"lockstep" rewrites of the EMA cascades) is landing under the *current*
bit-exact contract. The FMA switch would happen **after** that pass completes,
as a single versioned event — so each rewrite is proven bit-exact in
isolation first, and the numerical contract changes exactly once.

## Feedback wanted

1. Is a one-time shift bounded by `1e-12 × max(1, |old|)` acceptable for your
   use (stored expected values, golden files, chart comparisons)?
2. Artifact strategy: is x86-64-v3-primary + baseline-fallback right, or
   should baseline remain primary for another release cycle?
3. Should a build-time opt-out (`TA_NO_FMA`, reproducing today's unfused
   arithmetic) be maintained, and for how long?
4. Anything in your pipeline that requires bit-compatibility with pre-FMA
   TA-Lib outputs indefinitely?
