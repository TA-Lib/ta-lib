# FMA Readiness Audit — evidence that fusion is safe to adopt

Status: **implemented** (PR #96). Fusion is enabled in all backends; the sections
below are the pre-adoption analysis, followed by an [implementation
note](#implemented) recording what landed and how it verified. 2026-07-14.

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
(`ta_regtest --fuzz-064`, current build vs frozen v0.6.4). While
`FMA_TRANSITION_TOLERANCE` is on (`test_codegen.c`), a per-element diff not
covered by an explicit manifest entry is tolerated when it is within the contract
— `|current − v0.6.4| ≤ 1e-9 × max(|current|, |v0.6.4|, inScale)`. It is
output-relative (so volume-scaled outputs like ADOSC and bounded oscillators
alike are sized right), floored at the input scale for the functions that
difference two large near-equal quantities (DEMA/MACDFIX/MACDEXT/HT_PHASOR),
where output-relative is ill-posed near a zero-crossing. These cases are counted
in their own `fma-rebaseline` line with the largest ratio observed, so a genuine
`>1e-9` regression still fails loudly; integer outputs get no tolerance.
**Re-freeze:** once an FMA-enabled release is tagged, point
`scripts/build_064_serve.py`'s `REF_TAG` at it, rebuild the oracle, and set
`FMA_TRANSITION_TOLERANCE` to 0 — every function returns to strict hash-exact
comparison against the FMA baseline.

## Determinism: pin the fused product (canonicalization)

`a*b + c*d` (both operands products — the `k·x + (1−k)·prev` EMA form) is
order-ambiguous under FMA: fusing the left product vs the right gives different
bits, and the streaming transition rewrite can reorder such a statement (`+`
commutes, so pre-FMA this was invisible). The generator therefore canonicalizes
`target = P1 + P2` so the product referencing `target` — the accumulator — is
always the fused (left) one, in every backend and both the batch and stream
paths. This makes fusion independent of incidental operand order, so batch ==
stream and C == Rust == Java bit-for-bit. (Accumulator-first was chosen so the
Hilbert family — whose `HT_TRENDMODE` integer output the audit certified
flip-free — is left byte-for-byte unchanged.)

## Reproducing the fusion-site enumeration

`ta_codegen/generator/src/backends/fma.rs` holds the shared detector
(`fuse_operands`) and the master gate (`const EMIT_FMA`, now `true`). Set it
`false`, `cargo run -- generate`, diff the generated code to see every fused site
(and confirm the pre-FMA output is byte-identical), then revert. The three
independent backends (C `fma`, Rust `mul_add`, Java `Math.fma`) call the one
detector, so they fuse the identical sites; `.NET` P/Invokes the generated C and
inherits it.

## Implemented

Landed on `rfc/fma-numerical-contract` (PR #96):

- **Single shared detector** (`fma::fuse_operands`) + the accumulator
  canonicalization, used by the C, Rust, and Java backends so all three fuse
  identical sites. `.NET` inherits fusion from the C library it wraps.
- **Runtimes:** C99 `fma` (double even in the `TA_S_` single-precision path — it
  computes in double), Rust `f64::mul_add`, Java `Math.fma` (JDK 9+; the CI/build
  toolchain is JDK 21). All three are IEEE-754 correctly-rounded, hence
  bit-identical for equal operands (verified directly: C `fma` == Rust `mul_add`
  across hardware/software and baseline/native builds).
- **Streaming:** the batch and per-bar stream paths fuse the same sites (a
  state-field name-alias feeds the detector the `sp->`-qualified operands), so
  `stream_verify` stays bit-exact (161/161 functions, 14271 legs).
- **Verification:** `fuzz-064` passes with 0 failures, max FMA divergence
  **2.99e-11** (33× under the 1e-9 contract), integer outputs bit-exact;
  cross-language regtest 161×4; 165 doctests; 551 generator tests (incl. a
  fusion-site seed-invariance guard). A sabotage probe (a 1e-7 coefficient error
  injected into MACD) is caught by the gate — the tolerance does not mask real
  bugs. The C library and the generated Rust crate are bit-identical for the same
  inputs.
