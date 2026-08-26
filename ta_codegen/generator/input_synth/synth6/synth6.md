# SYNTH6

## Summary

Synthetic gate function: the mirror image of SYNTH5. The same window-endpoint sum, but the streamable trailing-cursor walk is the base and the batch-only form indexed by the window's start is a `PRAGMA TA_ALT={BATCH,ALL_LANGUAGES}` alternate. It exists only to verify the code generator end to end across all backends; it is never shipped (see `ta_codegen/generator/input_synth/README.md`).

## Formula

outInteger[i] = ((int)(x(i - period + 1) + x(i))) mod 1024

## Notes

- Covers `PRAGMA TA_ALT={BATCH,ALL_LANGUAGES}`. Verified against `ta_codegen/input/`: all six shipped pragmas claim STREAM, so this orientation is reached nowhere else — and it is the only shape in which a resolver that leaked the batch body into the stream tier fails.
- Named by `tests/alt_suite.rs` together with SYNTH5, the same algorithm with the tiers swapped.
- Coverage trap: the base must stay the streamable trailing-cursor walk and the alternate the window-start form, because the test tells the tiers apart by the indexing each carries. Swap them and this silently becomes a second copy of SYNTH5.
- Coverage trap: both bodies add the same two bars in the same order, so they are bit-identical rather than merely equal.
- Issue #190.

## Inputs

- `inReal` — Input series

## Outputs

- `outInteger` — The quantized 10-bit window-endpoint sum

## Parameters

- `optInTimePeriod` — Window width; the two summed bars are its endpoints

## Implementation

TA-Lib Definition: [`synth6.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/generator/input_synth/synth6/synth6.c) · [`synth6.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/generator/input_synth/synth6/synth6.yaml)
