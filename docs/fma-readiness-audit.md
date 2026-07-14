# FMA Readiness Audit — evidence that fusion is safe to adopt

Status: **preparation** (no fusion enabled yet). Companion to
[`fma-proposal.md`](fma-proposal.md) (PR #96). 2026-07-14.

## The contract

TA-Lib guarantees each function **faithfully implements its algorithm within a
documented numerical tolerance** — not bit-for-bit reproducibility. Concretely:
every result stays within **1e-9 relative** of the historical value (in practice
the last bit or two, ~1e-16). This frees TA-Lib to use fused multiply-add (and
modern hardware generally), and — as a bonus — makes results bit-identical
across x86 and ARM.

## What fusion actually changes (measured)

Fused vs unfused Rust, **27.4 million output values**, all 26 fusion-candidate
functions × 7 data shapes (realistic + adversarial: pattern-rich candlesticks,
constant, tie-heavy). Harness: `scratchpad/fma_measure/`.

- **Continuous outputs:** max change **1.7e-10 relative** (MACDFIX; it differences
  two large EMAs near a zero-crossing), typically ~1e-16. Well inside the 1e-9
  contract. The EMA cascades (DEMA/TEMA/TRIX/MACD) are **bit-identical** — the
  `(x−prev)·k+prev` form is fusion-invariant, so the perf win comes with no
  numeric change at all.
- **Discrete outputs** (candlestick patterns, HT_TRENDMODE, SAR reversals):
  **zero changes in 27.4M values** — patterns fired 23,000+ times and not one
  flipped. A flip is *possible* only when an input lands within ~1 ULP of a
  threshold (constructible by hand, never seen on real or degenerate data), and
  even then neither side is "wrong" — both are faithful at a boundary the
  algorithm never pinned to the 16th digit.

Every one of the 27.4M divergences is ≤2.6e-12, with a clean gap up to the
contract's 1e-9 — nothing borderline.

## The one function to watch: SAR / SAREXT

SAR is a path-dependent trailing stop: a flipped reversal amplifies into a
whole-trajectory divergence. Measured drift was ≤2.2e-16 with **no reversal
flip** in 27.4M bars, so it is within contract — but it is the only place a
ULP-boundary flip would be user-visible (a stop-out one bar early). If we ever
want SAR bit-stable regardless, an explicit non-fused path with
`-ffp-contract=off` keeps it both historical *and* cross-platform.

## How to enforce the tolerance (regression gate)

The one-time transition is verified by the existing differential-fuzz harness
using a **scale-relative** bound (`TOL_REL_IN`, `tol=1e-9 × input scale` — the
same mechanism already used for LINEARREG/TSF). After the transition the FMA
build is the frozen reference and gates return to bitwise comparison.

## Reproducing the fusion-site enumeration

`ta_codegen/generator/src/backends/rust_lang.rs:2763` gates the emitter
(`const EMIT_FMA`). Set it `true`, `cargo run -- generate --backend=rust`, diff
the generated code to see every `a*b+c` → `mul_add` site, then revert. (The C
backend gains an analogous emitter; rustc never contracts `a*b+c` on its own, so
the Rust generator is the enumeration oracle.)
