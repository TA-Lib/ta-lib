# SYNTH19

## Summary

Synthetic gate function: fills an allocated `int` buffer and an allocated `double` buffer from the first output's window, clears part of each with `memset`, sums each buffer into a seed, and reports both seeds with the low three bits of each bar. It exists only to verify the code generator end to end across all backends; it is never shipped (see `ta_codegen/generator/input_synth/README.md`).

## Formula

outInteger[i] = 1000000 * C + 10 * (int)S + b_i, where C and S are the sums of the partly cleared count and value buffers, and b_i is (int)inReal[i] mod 8 after a negative, non-finite or out-of-range bar is folded to 0.

## Notes

- Covers `memset` into an allocated `int` and an allocated `double` buffer, each at offset 0 and at a non-zero offset. Verified against `ta_codegen/input/`: no shipped body calls `memset`.
- Coverage trap: a clear at the wrong offset or of the wrong length changes a seed only because every slot holds a distinct non-zero value. The count buffer is distinct by construction; the value buffer is distinct only for the golden inputs, which other fixtures share, so a change to them can let a misplaced clear of it go unobserved with every leg green.
- Coverage trap: the buffers must stay outside the steady loop. The streaming analyzer refuses an allocated buffer as loop state, and a fixed-size array would not reach the C# span path this fixture exists for.
- Issue #444 (G1).

## Inputs

- `inReal` — Input series

## Outputs

- `outInteger` — `1000000 * C + 10 * (int)S + b`, from the two seeds and the folded bar's low three bits `b`

## Parameters

- `optInTimePeriod` — Window length
