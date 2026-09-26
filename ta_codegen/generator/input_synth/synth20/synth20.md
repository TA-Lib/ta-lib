# SYNTH20

## Summary

Synthetic gate function: each output is twice the previous bar plus twice the current bar. It exists only to verify the code generator end to end across all backends; it is never shipped (see `ta_codegen/generator/input_synth/README.md`).

## Formula

outReal[i] = inReal[i-1] + inReal[i-1] + inReal[i] + inReal[i], summed in that order.

## Notes

- Covers a short countdown nested in another inside a loop the Rust backend windows (#442): both are unrolled into the pass, and every copy of the inner body must carry both counters' values. No shipped function nests one `for` in another inside its main loop.
- Coverage trap: both loops must count down from a literal to a literal (`a = 1; a >= 0; a--`) and the read must use the outer counter, or they stay loops, the pass is not windowed, and every leg stays green.

## Inputs

- `inReal` — Input series

## Outputs

- `outReal` — The computed series
