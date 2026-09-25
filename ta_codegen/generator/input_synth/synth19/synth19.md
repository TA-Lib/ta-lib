# SYNTH19

## Summary

Synthetic gate function: fills an allocated `int` buffer and an allocated `double` buffer from the first output's window, clears part of each with `memset`, folds both into one seed, and reports the seed with the low three bits of each bar. It exists only to verify the code generator end to end across all backends; it is never shipped (see `ta_codegen/generator/input_synth/README.md`).

## Formula

outInteger[i] = 1000000 * C + 10 * (int)S + ((int)inReal[i] mod 8), where C and S are the sums of the partly cleared count and value buffers.

## Notes

- Covers `memset` into an allocated `int` and an allocated `double` buffer, each at offset 0 and at a non-zero offset. Verified against `ta_codegen/input/`: no shipped body calls `memset`.
- Coverage trap: each clear is observable only because it overwrites data the golden inputs put there. Move a clear onto slots the inputs leave at zero, or change the inputs, and it can go unobserved with every leg green; re-check by deleting each `memset` in turn.
- Coverage trap: the buffers must stay outside the steady loop. The streaming analyzer refuses an allocated buffer as loop state, and a fixed-size array would not reach the C# span path this fixture exists for.
- Issue #444 (G1).

## Inputs

- `inReal` — Input series

## Outputs

- `outInteger` — `1000000 * C + 10 * S + b`, from the two seeds and the bar's low three bits `b`

## Parameters

- `optInTimePeriod` — Window length
