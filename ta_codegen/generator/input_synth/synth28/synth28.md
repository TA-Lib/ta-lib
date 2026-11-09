# SYNTH28

## Summary

Synthetic gate function: **tail copies of an intermediate that is later rewritten in place and partially overwritten, and lagged reads of a series a map defines**. It exists only to verify the code generator end to end across all backends; it is never shipped (see `ta_codegen/generator/input_synth/README.md`).

## Formula

m = MIDPOINT(inReal, n). outMaxNow[i] = m[i]; outMaxLag[i] = m[i-lag]. w[i] = m[i-lag] where m[i-lag] > m[i], else (m[i-lag] + m[i-lag+1]) / 2. outSide[i] = 1 when m[i]/2 > w[i], else 2 when m[i-lag]/2 > w[i], else 0.

## Notes

- Covers four things the shipped corpus lacks: a tail copy whose source is rewritten later, which must copy rather than alias; a tail copy of a lagged element, taken from a delay line; a partial write whose prior value comes from a delay line, seeded before the write; and a delay line on a series a map defines (#583).
- Coverage trap: `optInLag`'s range minimum is 1, where the partial write's prior is read at depth 0, the select path.

## Inputs

- `inReal` — Input series

## Outputs

- `outMaxNow` — The midpoint at the bar
- `outMaxLag` — The midpoint optInLag bars back
- `outSide` — Which half-midpoint lies above the adjusted series

## Parameters

- `optInTimePeriod` — Period of the midpoint
- `optInLag` — Bars the lagged copy is read back
