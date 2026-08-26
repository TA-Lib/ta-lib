# SYNTH2

## Summary

Synthetic gate function: regression driver for issue #160 (negative double→int casts). Each bar's value is cast to int twice — once clamped as a magnitude (the MAVP pattern), once masked for its two's-complement low bits — and both results are packed into the integer output. It exists only to verify the code generator end to end across all backends; it is never shipped (see `ta_codegen/generator/input_synth/README.md`).

## Formula

outInteger[i] = (clamp((int)inReal[i], 2, optInTimePeriod+2) << 12) | (bits((int)(inReal[i]*4)) & 4095), where bits(q) = (q & 255) | 1024 for negative q, else q.

## Notes

- Covers a `(int)` cast of a NEGATIVE double, in both use classes #160 named: as a magnitude (clamped between a floor and a ceiling, the MAVP shape) and as a bit pattern (two's-complement low bits). Verified against `ta_codegen/input/`: the bit-pattern class is unreachable there at all, since no shipped body contains a bitwise `&`; and MAVP's clamped period cast, the closest shipped analogue, is guarded non-negative.
- Coverage trap: the input guard here is deliberately `(-1e6, 1e6)`, not the `[0, 1e6)` the rest of this family uses. Fold negatives away, as SYNTH1 does, and both classes stop being exercised while the gate stays green. Nor widen it: `1e6` keeps `barVal * 4` far inside `i32` even after the scaling, so overflow — a different per-language divergence, the one SYNTH1 excludes — stays out of this fixture's result.
- Issue #160.

## Inputs

- `inReal` — Input series whose bars are cast to int

## Outputs

- `outInteger` — Packed clamp-and-mask result per bar

## Parameters

- `optInTimePeriod` — Ceiling for the magnitude clamp

## Implementation

TA-Lib Definition: [`synth2.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/generator/input_synth/synth2/synth2.c) · [`synth2.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/generator/input_synth/synth2/synth2.yaml)
