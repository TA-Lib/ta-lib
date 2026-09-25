# SYNTH18

## Summary

Synthetic gate function: each output is `(x - cap + 2(n-1) + 3 + 7.25) / 2`, where `cap` is the larger of 1000 and the first bar of the window. It exists only to verify the code generator end to end across all backends; it is never shipped (see `ta_codegen/generator/input_synth/README.md`).

## Formula

outReal[i] = (inReal[i] - max(inReal[s-(n-1)], 1000) + 2(n-1) + 3 + 7.25) * 0.5, where s is the first output bar and n the period.

## Notes

- Covers a local declared with an initializer and read before a later top-level store (#438): `cap` by a strict running extreme, `weight` inside the warm-up loop, `scale` by another local's initializer, `bias` by a compound store. Each one changes every output if its initializer is lost. No shipped function has such a store; every shipped local with a live initializer is either never stored at top level or stored before any read.
- Coverage trap: every store to `weight`, `scale` and `bias` must stay at the top level of the body, after the read. Move one into a block, or ahead of its read, and that case stops being exercised with every leg green.
- Coverage trap: `cap` must be compared strictly (`>`) so it is rewritten as a running maximum, and the window's first bar must stay below 1000 in the golden input, or `cap` never holds its initializer.

## Inputs

- `inReal` — Input series

## Outputs

- `outReal` — The shifted, halved input

## Parameters

- `optInTimePeriod` — Time period
