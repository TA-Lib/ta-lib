# SYNTH4

## Summary

Synthetic gate function: a smoothing recursion split across an explicit `_private` variant, so the guarded entry point pre-computes a decoupled real parameter and passes it down. It exists only to verify the code generator end to end across all backends; it is never shipped (see `ta_codegen/generator/input_synth/README.md`).

## Formula

S(i) = (x(i) - S(i-1)) * k + S(i-1), with k = 2 / (period + 1) supplied by the guarded entry point; outInteger[i] = ((int)(S(i) * 8)) mod 1024.

## Notes

- Bars are clamped to [0, 1000000) before any arithmetic, so the state stays bounded and the integer cast is identical in every language.
- The recursion makes the decoupled parameter a float multiply operand, which pins the FMA site selection: a private extra parameter is never a body declaration, so no site here fuses — identically in all four backends.

## Inputs

- `inReal` — Input series to smooth into the state

## Outputs

- `outInteger` — The quantized 10-bit state after each bar

## Parameters

- `optInTimePeriod` — Warm-up window, and the source of the smoothing factor the guarded variant derives

## Implementation

TA-Lib Definition: [`synth4.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/generator/input_synth/synth4/synth4.c) · [`synth4.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/generator/input_synth/synth4/synth4.yaml)
