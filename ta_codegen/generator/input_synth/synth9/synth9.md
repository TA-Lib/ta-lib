# SYNTH9

## Summary

Synthetic gate function: exercises scientific-notation float literals (`1e-13`, `2.5E-3`, `1.0e+6`, `3e2`, `.5e1`, `1e300`) in indicator code, end to end through every backend. No shipped indicator writes a float this way. It exists only to verify the code generator; it is never shipped (see `ta_codegen/generator/input_synth/README.md`).

## Formula

outNegExp[i] = close_i × 1e-13 + 2.5E-3; outPosExp[i] = ( high_i − low_i ) / 1.0e+6 + 3e2 + .5e1; outBigExp[i] = | close_i | / 1e300

## Notes

- Covers scientific-notation float literals. Verified against `ta_codegen/input/`: no shipped body writes one — the `1e-14`, `1e-6` and `2e-10` occurrences in the tree are all inside comments, which the lexer skips — so the number path had never been handed the token.
- Coverage trap: every mantissa/exponent shape the lexer distinguishes has to stay present, because they take different branches — a negative exponent, an explicit `+`, an upper-case `E`, an exponent with no decimal point, and a leading-dot mantissa carrying one.
- Coverage trap: each output must depend on ITS OWN literal. `1e300 / (1e300 + x)` is tidy and vacuous — it answers 1.0 whether the exponent is 300 or 301.
- Coverage trap: magnitudes stay small on purpose. The bitwise gates would not care, but the plain `--codegen` sweep crosses JSON at `%.15g` and compares at an epsilon, so an output near 1e291 fails on transport rounding and says nothing about literals. Dividing BY the large literal rather than multiplying by it is what keeps that true.
- `1e300` is there for the emitter rather than the lexer: it is whole but too large for a `.0` suffix, and the naive rendering carries neither `.` nor `e` — an integer constant in C, and past `LLONG_MAX` an ill-formed one.
- The mantissa multiply and the addition are separate statements, so no `a*b+c` fusion site exists. FMA is covered elsewhere; here it would only put a second variable into any failure.
- No issue; the post-mortem is commit `2526fe42b`.

## Inputs

- `inHigh` — High price of each bar
- `inLow` — Low price of each bar
- `inClose` — Close price of each bar

## Outputs

- `outNegExp` — Bar close scaled by a negative-exponent literal, offset by another
- `outPosExp` — Bar spread scaled by a positive-exponent literal, offset by two more
- `outBigExp` — Bar close divided by a literal too large for the plain `.0` rendering

## Implementation

TA-Lib Definition: [`synth9.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/generator/input_synth/synth9/synth9.c) · [`synth9.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/generator/input_synth/synth9/synth9.yaml)
