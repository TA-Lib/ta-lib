# SYNTH15

## Summary

Synthetic gate function: each output is the input less the input `optInTimePeriod-1` bars earlier, truncated to an integer. It exists only to verify the code generator end to end across all backends; it is never shipped (see `ta_codegen/generator/input_synth/README.md`).

## Formula

outReal[i] = inReal[i] - trunc(inReal[i-(n-1)]), where trunc clamps to 0 outside (-1e6, 1e6); n = 1 copies the input.

## Notes

- Covers a Rust stream state that is split (#439): the step stores into a heap buffer inside a loop, so the buffer reaches it as its own `&mut [i32]`. Its element type is `int`, which no shipped buffer has, and the state also carries the period-1 identity path, whose opener builds the split state from defaults. MEDIAN and PERCENTILE are the only shipped split states; both hold `double` buffers and neither has an identity path.
- Coverage trap: the shift must stay a loop that stores into the buffer. Rewritten as a ring advanced by an index, the step stores once per bar outside any loop, the state is not split, and every leg stays green while covering nothing new.
- Coverage trap: the output store stays above every buffer store. The peek frame derives its answer from the reads above the output and drops the rest; moving a store above it changes what the frame has to shadow.
- The truncation guard keeps the `(int)` cast inside the range where C, Rust, Java and C# agree.

## Inputs

- `inReal` — Input series

## Outputs

- `outReal` — The input less the truncated input `n-1` bars earlier

## Parameters

- `optInTimePeriod` — Window length n; 1 takes the identity path

## Implementation

TA-Lib Definition: [`synth15.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/generator/input_synth/synth15/synth15.c) · [`synth15.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/generator/input_synth/synth15/synth15.yaml)
