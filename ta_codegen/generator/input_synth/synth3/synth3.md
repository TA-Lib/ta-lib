# SYNTH3

## Summary

Synthetic gate function: regression driver for issues #158 (integer locals typed by declaration, not by name), #159/#163 (an int-array element compared against the unsigned index domain) and #165 (a signed local inside an expression rather than standing alone). Each bar folds its value into an integer, runs it through a wrap-by-one-period bookkeeping step whose locals are deliberately named so that every naming heuristic in the code generator gets them wrong, then packs the results of eight comparisons between a small int-array element and the index domain. It exists only to verify the code generator end to end across all backends; it is never shipped (see `ta_codegen/generator/input_synth/README.md`).

## Formula

outInteger[i] = ((2 × optInTimePeriod + fold(inReal[i])) & 65535) | (hits << 16), where fold(x) = (int)x for 0 < x < 1e6 and 0 otherwise, and hits packs eight comparisons of ring[fold & 3] = fold & 7 against 2 × optInTimePeriod.

## Notes

- Deterministic and integer-only, so cross-language comparisons are exact.
- Every value is bar-local, so the function is not path-dependent and batch and streaming agree bar for bar.
- The lookback body carries the same compound-assignment shape as the main body: that context used to be rendered with no type information at all.
- Every int-array intermediate is held non-negative on purpose. C compares in the signed domain while Rust widens to the unsigned one, so a negative intermediate would wrap and the languages would disagree by construction — a documented limitation of the index-domain convention, and not something this fixture may depend on.

## Inputs

- `inReal` — Input series whose bars are folded into the integer domain

## Outputs

- `outInteger` — Packed bookkeeping result per bar: comparison bits in the high half, wrap-by-period result in the low 16 bits

## Parameters

- `optInTimePeriod` — Period the negative index is wrapped by

## Implementation

TA-Lib Definition: [`synth3.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/generator/input_synth/synth3/synth3.c) · [`synth3.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/generator/input_synth/synth3/synth3.yaml)
