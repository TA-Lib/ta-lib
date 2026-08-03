# SYNTH1

## Summary

Synthetic gate function: folds the input series through a 10-bit integer state machine built from every bitwise operator (`&`, `|`, `^`, `~`, `<<`, `>>` and their compound forms), integer truthiness, do-while, and switch-on-expression. It exists only to verify the code generator end to end across all backends; it is never shipped (see `ta_codegen/generator/input_synth/README.md`).

## Formula

outInteger[i] = S(i), where S is a path-dependent 10-bit state: rotate by 3, XOR with the quantized bar value ((int)(inReal[i] * 8) mod 1024), then a bar-dependent mixing step selected by the low two bits.

## Notes

- Deterministic and integer-only, so cross-language comparisons are exact.
- State stays in [0, 1023]; shift operands are never negative.

## Inputs

- `inReal` — Input series to fold into the state

## Outputs

- `outInteger` — The 10-bit state after each bar

## Parameters

- `optInTimePeriod` — Warm-up window folded into the initial state

## Implementation

TA-Lib Definition: [`synth1.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/generator/input_synth/synth1/synth1.c) · [`synth1.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/generator/input_synth/synth1/synth1.yaml)
