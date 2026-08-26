# SYNTH1

## Summary

Synthetic gate function: folds the input series through a 10-bit integer state machine built from every bitwise operator (`&`, `|`, `^`, `~`, `<<`, `>>` and their compound forms), integer truthiness, do-while, and switch-on-expression. It exists only to verify the code generator end to end across all backends; it is never shipped (see `ta_codegen/generator/input_synth/README.md`).

## Formula

outInteger[i] = S(i), where S is a path-dependent 10-bit state: rotate by 3, XOR with the quantized bar value ((int)(inReal[i] * 8) mod 1024), then a bar-dependent mixing step selected by the low two bits.

## Notes

- Covers every bitwise operator and compound form (`&`, `|`, `^`, `~`, `<<`, `>>`, `&=`, `|=`, `^=`, `<<=`, `>>=`), integer truthiness, and a `switch` on an expression. Verified against `ta_codegen/input/`: none of them appears in a shipped body except `>>` (TRIMA halves its period), and the corpus's three `switch`es (`ma.c` twice, `helpers/candlestick.c` once) are all on a bare variable rather than an expression.
- `do`/`while` is not exclusive to this fixture — 74 shipped bodies use one — so it rides along rather than being covered by this fixture.
- Coverage trap: every intermediate feeds `outInteger`, so a miscompile of any one operator changes a compared value. An operator whose result stops reaching the output is covered by nothing.
- Coverage trap: the state is held in `[0, 1023]` and no shift ever sees a negative operand. Widen either and this goes red for a language difference the README already forbids, not for a generator defect.
- Issue #157.

## Inputs

- `inReal` — Input series to fold into the state

## Outputs

- `outInteger` — The 10-bit state after each bar

## Parameters

- `optInTimePeriod` — Warm-up window folded into the initial state

## Implementation

TA-Lib Definition: [`synth1.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/generator/input_synth/synth1/synth1.c) · [`synth1.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/generator/input_synth/synth1/synth1.yaml)
