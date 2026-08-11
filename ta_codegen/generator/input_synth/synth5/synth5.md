# SYNTH5

## Summary

Synthetic gate function: the sum of the first and last bar of a rolling window, written twice — a base indexed by the window's start (which reads ahead of the emitted bar, so it cannot be streamed) and a `PRAGMA TA_ALT={STREAM,ALL_LANGUAGES}` alternate that walks the same window with a trailing cursor. It exists only to verify the code generator end to end across all backends; it is never shipped (see `ta_codegen/generator/input_synth/README.md`).

## Formula

outInteger[i] = ((int)(x(i - period + 1) + x(i))) mod 1024

## Notes

- The base is deliberately unstreamable: `inReal[i + nbInitialElementNeeded]` reads past the bar being emitted. Delete the alternate and the function stops generating, so the gate cannot pass by accident.
- Both bodies add the two bars in the same order, so they are bit-identical rather than merely equal — the contract an alternate owes its base.
- Bars are clamped to [0, 1000000) before any arithmetic, so the sum stays bounded and the integer cast is identical in every language.

## Inputs

- `inReal` — Input series

## Outputs

- `outInteger` — The quantized 10-bit window-endpoint sum

## Parameters

- `optInTimePeriod` — Window width; the two summed bars are its endpoints

## Implementation

TA-Lib Definition: [`synth5.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/generator/input_synth/synth5/synth5.c) · [`synth5.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/generator/input_synth/synth5/synth5.yaml)
