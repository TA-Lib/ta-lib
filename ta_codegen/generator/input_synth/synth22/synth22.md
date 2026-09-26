# SYNTH22

## Summary

Synthetic gate function: each output is the bar-to-bar change plus half the previous output. It exists only to verify the code generator end to end across all backends; it is never shipped (see `ta_codegen/generator/input_synth/README.md`).

## Formula

outReal[k] = inReal[i] - inReal[i-1] + outReal[k-1] * 0.5, with the second term absent for the first output.

## Notes

- Covers a loop the Rust backend windows with `get`, whose windows never fit, so every call runs the loop as written instead (#442): the previous output is read only when a `?:` selects it, and its window would start one element before the output array. Every shipped function's windows fit on a real call, so no other gate runs that path.
- Coverage trap: the read of `outReal[outIdx-1]` must stay in a `?:` on `havePrev`. Read it unconditionally, or from a bar that always exists, and the windows fit; move it into an `if` arm and it is never windowed. Either way the fallback stops running with every leg green.

## Inputs

- `inReal` — Input series

## Outputs

- `outReal` — The computed series
