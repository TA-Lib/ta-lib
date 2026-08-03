# SYNTH2

## Summary

Synthetic gate function: regression driver for issue #160 (negative double→int casts). Each bar's value is cast to int twice — once clamped as a magnitude (the MAVP pattern), once masked for its two's-complement low bits — and both results are packed into the integer output. It exists only to verify the code generator end to end across all backends; it is never shipped (see `ta_codegen/generator/input_synth/README.md`).

## Formula

outInteger[i] = (clamp((int)inReal[i], 2, optInTimePeriod+2) << 12) | (bits((int)(inReal[i]*4)) & 4095), where bits(q) = (q & 255) | 1024 for negative q, else q.

## Notes

- Deterministic and integer-only, so cross-language comparisons are exact.
- The input guard folds bars outside (-1e6, 1e6) — far inside i32 range even after scaling; negative bars flow into the casts by design.

## Inputs

- `inReal` — Input series whose bars are cast to int

## Outputs

- `outInteger` — Packed clamp-and-mask result per bar

## Parameters

- `optInTimePeriod` — Ceiling for the magnitude clamp

## Implementation

TA-Lib Definition: [`synth2.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/generator/input_synth/synth2/synth2.c) · [`synth2.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/generator/input_synth/synth2/synth2.yaml)
