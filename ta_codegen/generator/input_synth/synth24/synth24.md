# SYNTH24

## Summary

Synthetic gate function: three scaled copies of the input, each reporting a different display shift. It exists only to verify the code generator end to end across all backends; it is never shipped (see `ta_codegen/generator/input_synth/README.md`).

## Formula

outOwnBar[i] = inReal[i], outAhead[i] = 2 × inReal[i], outBehind[i] = 4 × inReal[i].

Display shift: 0 for `outOwnBar`, `optInTimePeriod` for `outAhead`, `-(optInTimePeriod / 2 + 1)` for `outBehind`.

## Notes

- Covers a `<name>_display_shift` that branches on `outputIdx`, keeps a local and answers a positive, a negative and a zero shift from one call (#489). No shipped function answers shifts of both signs from one call.
- Coverage trap: the third shift divides the period by `outputIdx` so that the index meets a parameter in one expression; written as `/ 2` it gives the same numbers and stops covering that.
- Coverage trap: the three shifts must stay pairwise different at the period the golden table uses, and the first output must stay unflagged. Equal shifts pass with the index ignored, and a flag on every output drops the case of a zero answered for an unflagged output beside flagged ones.

## Inputs

- `inReal` — Input series

## Outputs

- `outOwnBar` — The input, drawn at its own bar
- `outAhead` — Twice the input, drawn ahead
- `outBehind` — Four times the input, drawn behind

## Parameters

- `optInTimePeriod` — Sets the two display shifts; the values do not depend on it
