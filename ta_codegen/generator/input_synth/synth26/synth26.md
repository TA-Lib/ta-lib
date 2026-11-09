# SYNTH26

## Summary

Synthetic gate function: **a produced series read at four fixed depths, the input read one bar back, and a sub-call result read through a copied pointer**. It exists only to verify the code generator end to end across all backends; it is never shipped (see `ta_codegen/generator/input_synth/README.md`).

## Formula

x = inReal / 2. s[i] = (x[i] + x[i-1]) + (x[i-2] + x[i-3]). outSum[i] = 1, -1 or 0 as s[i] is above, below or equal to MIDPOINT(x, n)[i]. outRise[i] = 1 when inReal[i-1] < inReal[i], else 0.

## Notes

- Covers two tap shapes and a must-alias read the shipped corpus lacks: a delay line on a producer-loop series with three taps at fixed depths, a tap on a bar input, and a map read through an unconditional pointer copy, which must resolve to the series the copy names (#583).

## Inputs

- `inReal` — Input series

## Outputs

- `outSum` — Sign of the four-bar sum of half-bars minus their midpoint
- `outRise` — 1 when the bar rose from the previous one

## Parameters

- `optInTimePeriod` — Period of the midpoint
