# FMA Readiness Audit — evidence that fusion is safe to adopt

Status: **implemented** (PR #96, 2026-07-14). Fusion is on in all four backends. The
one-time re-baselining gate is **still armed** — `FMA_TRANSITION_TOLERANCE` is `1` and the
v0.6.4 oracle has not been re-frozen; see [Where the mechanism
lives](#where-the-mechanism-lives).

This document holds the **pre-adoption evidence**: the measurement that justified the
numerical contract, and the SAR analysis. The mechanism — the tolerance formula, the
re-freeze procedure, the operand canonicalization, the enumeration oracle — is documented
beside the code that implements it and is not repeated here.

## The contract

TA-Lib guarantees each function **faithfully implements its algorithm within a
documented numerical tolerance** — not bit-for-bit reproducibility. Concretely:
every result stays within **1e-9 relative** of the historical value (in practice
the last bit or two, ~1e-16). This frees TA-Lib to use fused multiply-add (and
modern hardware generally), and — as a bonus — makes results bit-identical
across x86 and ARM.

## What fusion actually changes (measured)

Fused vs unfused Rust, **27.4 million output values**, all 26 fusion-candidate
functions as of PR #96 × 7 data shapes (realistic + adversarial: pattern-rich
candlesticks, constant, tie-heavy).

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

The harness was a scratch tree, never committed, and no longer exists. These numbers are
the record; re-deriving them means rebuilding it against the `EMIT_FMA` oracle below.

## The one function to watch: SAR / SAREXT

SAR is a path-dependent trailing stop: a flipped reversal amplifies into a
whole-trajectory divergence. Measured drift was ≤2.2e-16 with **no reversal
flip** in 27.4M bars, so it is within contract — but it is the only place a
ULP-boundary flip would be user-visible (a stop-out one bar early). If we ever
want SAR bit-stable regardless, an explicit non-fused path with
`-ffp-contract=off` keeps it both historical *and* cross-platform.

## Where the mechanism lives

Each of these is documented in full next to the code that implements it. Read them there,
not here — a second copy is a second thing to keep true.

| What | Where |
|---|---|
| The transition tolerance — the formula, the input-scale floor, why integer outputs get none, and the **RE-FREEZE** procedure that retires it | `FMA_TRANSITION_TOLERANCE` in `src/tools/ta_regtest/test_codegen.c` |
| Which `a*b + c` sites fuse, and why one shared detector rather than four | the module doc of `ta_codegen/generator/src/backends/fma.rs` |
| Accumulator canonicalization — why the accumulator product is the fused one, and why that leaves the Hilbert family byte-for-byte unchanged | `canonicalize_accumulator_add` in the same file |
| The enumeration oracle — flip the master gate, regenerate, diff to see every fused site | `EMIT_FMA` in the same file |

## What landed (PR #96)

- **A single shared detector** (`fma::fuse_operands`) plus the accumulator
  canonicalization, used by **all four** backends — C, Rust, Java and C# — so every
  backend fuses the identical sites. (An earlier draft of this document said .NET
  inherited fusion from the C library it wraps; that has not been true since C# became a
  native backend, which generates its own `Math.FusedMultiplyAdd` calls.)
- **Runtimes:** C99 `fma` (double even in the `TA_S_` single-precision path — it
  computes in double), Rust `f64::mul_add`, Java `Math.fma` (JDK 9+), C#
  `Math.FusedMultiplyAdd`. All four are IEEE-754 correctly-rounded, hence
  bit-identical for equal operands (verified directly: C `fma` == Rust `mul_add`
  across hardware/software and baseline/native builds).
- **Streaming:** the batch and per-bar stream paths fuse the same sites (a
  state-field name-alias feeds the detector the `sp->`-qualified operands), so
  `stream_verify` stays bit-exact.
- **Verification, as measured at the PR:** `fuzz-064` passed with 0 failures, max FMA
  divergence **2.99e-11** (33× under the 1e-9 contract), integer outputs bit-exact;
  cross-language regtest and the doctest and generator suites green. A sabotage probe (a
  1e-7 coefficient error injected into MACD) is caught by the gate — the tolerance does
  not mask real bugs. The C library and the generated Rust crate are bit-identical for
  the same inputs.
