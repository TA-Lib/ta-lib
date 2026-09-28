# SYNTH23

## Summary

Synthetic gate function: each output is the negation of the input times zero. It exists only to verify the code generator end to end across all backends; it is never shipped (see `ta_codegen/generator/input_synth/README.md`).

## Formula

outReal[i] = -(inReal[i] * 0.0): `-0.0` for a positive or `+0.0` input, `+0.0` for a negative one.

## Notes

- Covers the sign of a negated zero (#463). A lowering of `-x` that loses it is wrong the same way in every backend, which the two comparative legs cannot see.
- Coverage trap: the input must keep both signs. On an all-positive series every output is `-0.0`, which a lowering that always answers `-0.0` for a zero also produces.

## Inputs

- `inReal` — Input series

## Outputs

- `outReal` — The negated zero
