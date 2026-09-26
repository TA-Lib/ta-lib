# SYNTH21

## Summary

Synthetic gate function: each output is the current bar plus a seed summed over the warm-up window, which is empty at period 2. It exists only to verify the code generator end to end across all backends; it is never shipped (see `ta_codegen/generator/input_synth/README.md`).

## Formula

outReal[i] = inReal[i] + seed, where seed = 0 when n = 2, else the sum of inReal[j+(n-1)/(n-2)-1] over the n-1 bars j before the first output bar s, and n is the period.

## Notes

- Covers a read made only when a `?:` selects it, whose index divides by a value the test rules out as zero (#442). The Rust backend cuts a window for such a read before the loop runs, so the window's start must not evaluate that division. The read sits in the warm-up loop because the streaming analysis refuses such an index in the main loop; every shipped read of that kind indexes by names and literals.
- Coverage trap: the golden row must keep `optInTimePeriod` at 2, the only period where the divisor is zero; at any other period the fault is not reachable and every leg stays green.

## Inputs

- `inReal` — Input series

## Outputs

- `outReal` — The computed series

## Parameters

- `optInTimePeriod` — Time period
