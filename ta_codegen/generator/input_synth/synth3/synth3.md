# SYNTH3

## Summary

Synthetic gate function: regression driver for issue #158 (integer locals typed by declaration, not by name). Each bar folds its value into an integer, then runs it through a wrap-by-one-period bookkeeping step whose locals are deliberately named so that every naming heuristic in the code generator gets them wrong. It exists only to verify the code generator end to end across all backends; it is never shipped (see `ta_codegen/generator/input_synth/README.md`).

## Formula

outInteger[i] = (2 × optInTimePeriod + fold(inReal[i])) & 65535, where fold(x) = (int)x for 0 < x < 1e6 and 0 otherwise.

## Notes

- Deterministic and integer-only, so cross-language comparisons are exact.
- Every value is bar-local, so the function is not path-dependent and batch and streaming agree bar for bar.
- The lookback body carries the same compound-assignment shape as the main body: that context used to be rendered with no type information at all.

## Inputs

- `inReal` — Input series whose bars are folded into the integer domain

## Outputs

- `outInteger` — Packed bookkeeping result per bar

## Parameters

- `optInTimePeriod` — Period the negative index is wrapped by

## Implementation

TA-Lib Definition: [`synth3.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/generator/input_synth/synth3/synth3.c) · [`synth3.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/generator/input_synth/synth3/synth3.yaml)
