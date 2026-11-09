# SYNTH27

## Summary

Synthetic gate function: **one series read at parameter lags in both arms of a parameter-selected mode switch**. It exists only to verify the code generator end to end across all backends; it is never shipped (see `ta_codegen/generator/input_synth/README.md`).

## Formula

a = MIDPOINT(inReal, n), b = MIDPOINT(inReal, n). Mode 0: outUp[i] = 1 when a[i-lag] > b[i], else 0. Mode 1: outUp[i] = 1 when a[i] > a[i-lag-1], else 2 when a[i-lag] > b[i], else 0.

## Notes

- Covers a delay line whose taps sit in both arms of a variant `if`: Open captures each arm's depths under the same test, and a tap of the arm not taken stays unread. The shipped corpus's one variant `if` with a composed read, PERCENTB's, reads nothing at a lag (#583).
- Coverage trap: keep the default mode at 1. Only then does a vector run mode 1 at lag 0, where its depth-`lag` tap takes the depth-0 select.
- Coverage trap: keep a tap on `tempA` in each arm. With taps in one arm only, the line takes fixed backs and the per-handle backs across arms go unexercised.

## Inputs

- `inReal` — Input series

## Outputs

- `outUp` — The mode's comparison: 1 or 2 for the arm that held, 0 for neither

## Parameters

- `optInTimePeriod` — Period of both midpoints
- `optInLag` — Bars the lagged leg is read back
- `optInMode` — Which comparison the output takes
