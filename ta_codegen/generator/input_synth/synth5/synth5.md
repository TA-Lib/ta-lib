# SYNTH5

## Summary

Synthetic gate function: the sum of the first and last bar of a rolling window, written twice — a base indexed by the window's start and a `PRAGMA TA_ALT={STREAM,ALL_LANGUAGES}` alternate that walks the same window with a trailing cursor. It exists only to verify the code generator end to end across all backends; it is never shipped (see `ta_codegen/generator/input_synth/README.md`).

## Formula

outInteger[i] = ((int)(x(i - period + 1) + x(i))) mod 1024

## Notes

- Covers `PRAGMA TA_ALT={STREAM,ALL_LANGUAGES}`. That orientation is not unreached — six shipped rolling-extremum functions ship it — so what this fixture adds is a base that CANNOT generate without its alternate: `inReal[i + nbInitialElementNeeded]` reads past the bar being emitted, which no per-bar automaton can express. Delete the alternate and generation fails, so the gate cannot pass by accident.
- Named by `tests/alt_suite.rs`, which asserts on the emitted statements per tier — an alternate is generator input and never becomes a symbol, so nothing else can show which body won. SYNTH6 is the mirror; the pair is what defeats an always-base or always-alternate resolver.
- Coverage trap: both bodies add the same two bars in the SAME order, so base and alternate are bit-identical rather than merely equal. Reorder either sum and this fails on rounding instead of on resolution.
- Issue #190.

## Inputs

- `inReal` — Input series

## Outputs

- `outInteger` — The quantized 10-bit window-endpoint sum

## Parameters

- `optInTimePeriod` — Window width; the two summed bars are its endpoints

## Implementation

TA-Lib Definition: [`synth5.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/generator/input_synth/synth5/synth5.c) · [`synth5.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/generator/input_synth/synth5/synth5.yaml)
