# SYNTH9

## Summary

Synthetic gate function: exercises scientific-notation float literals (`1e-13`, `2.5E-3`, `1.0e+6`, `3e2`, `.5e1`, `1e300`) in indicator code, end to end through every backend. No shipped indicator writes a float this way — the `1e-14` occurrences in the input tree are all inside comments — so before this fixture the lexer had never been handed the token and rejected it outright. It exists only to verify the code generator; it is never shipped (see `ta_codegen/generator/input_synth/README.md`).

## Formula

outNegExp[i] = close_i × 1e-13 + 2.5E-3; outPosExp[i] = ( high_i − low_i ) / 1.0e+6 + 3e2 + .5e1; outBigExp[i] = | close_i | / 1e300

## Notes

- Every mantissa/exponent shape the lexer distinguishes appears: a negative exponent, an explicit `+`, an upper-case `E`, an exponent with no decimal point, and a leading-dot mantissa carrying one.
- An exponent makes a constant a float even with no decimal point. Read as an integer, `3e2` would change the type of the expression around it.
- `1e300` is there for the emitter rather than the lexer: it is whole but too large for the `.0` suffix, and the naive rendering is a run of digits carrying neither `.` nor `e` — an integer constant in C, and past `LLONG_MAX` an ill-formed one.
- Real outputs rather than this family's usual integer ones, for the same reason as SYNTH7: the value of the literal is the thing under test, and an integer output would quantize away the difference the gate exists to see.
- Each output depends on its own literal. `1e300 / (1e300 + x)` would be bounded and vacuous — it answers 1.0 whether the exponent is 300 or 301.
- Magnitudes stay small deliberately. The bitwise gates would not care, but the plain cross-language sweep crosses JSON at `%.15g` and compares at an epsilon, so a near-1e291 output would fail on transport rounding and say nothing about literals.

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
