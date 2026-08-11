# SYNTH6

## Summary

Synthetic gate function: the mirror image of SYNTH5. The same window-endpoint sum, but the streamable trailing-cursor walk is the base and the batch-only form indexed by the window's start is a `PRAGMA TA_ALT={BATCH,ALL_LANGUAGES}` alternate. It exists only to verify the code generator end to end across all backends; it is never shipped (see `ta_codegen/generator/input_synth/README.md`).

## Formula

outInteger[i] = ((int)(x(i - period + 1) + x(i))) mod 1024

## Notes

- SYNTH5 claims STREAM, SYNTH6 claims BATCH. Between them the resolver is exercised in both directions, so a bug that always returned the base — or always returned the alternate — fails one of the two.
- Both bodies add the two bars in the same order, so they are bit-identical rather than merely equal.
- Bars are clamped to [0, 1000000) before any arithmetic, so the sum stays bounded and the integer cast is identical in every language.

## Inputs

- `inReal` — Input series

## Outputs

- `outInteger` — The quantized 10-bit window-endpoint sum

## Parameters

- `optInTimePeriod` — Window width; the two summed bars are its endpoints

## Implementation

TA-Lib Definition: [`synth6.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/generator/input_synth/synth6/synth6.c) · [`synth6.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/generator/input_synth/synth6/synth6.yaml)
