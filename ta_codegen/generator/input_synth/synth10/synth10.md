# SYNTH10

## Summary

Synthetic gate function: exercises **two nullable outputs in one function**, end to end through every backend — three output pairs, covering every arm the output-distinctness guard branches on. It exists only to verify the code generator; it is never shipped (see `ta_codegen/generator/input_synth/README.md`).

## Formula

outRequired[i] = real_i; outFirstOptional[i] = real_i / 2; outSecondOptional[i] = real_i / 4

## Notes

- Covers two nullable outputs in one function. Verified against `ta_codegen/input/`: MAMA's `outFAMA` is the corpus's only nullable output and it sits beside a single required one, so the output-distinctness guard (#108) had only ever been generated in its simplest arrangement.
- Coverage trap, the sharpest in this family: the output declaration ORDER in the `.yaml` is load-bearing. The guard walks output pairs `(i, j)` with `i < j`, and this order is the only one of three outputs that produces all three combinations — `(nullable, required)`, `(nullable, nullable)` and `(required, nullable)`. Reorder it and coverage silently drops back to what MAMA already gives, with the gate still green.
- Coverage trap: `outRequired` owns the `outIdx` advance and the two declinable stores carry none. That is what makes guarding a store COMPLETE — the write is skipped and the cursor still moves. `mama.c` was reordered for the same reason (#125).
- Real outputs, scaled by powers of two: MAMA's nullable output is real, so this mirrors the shipped case, and halving is exact, so every value is bitwise identical in all four backends with no `(int)` cast — the conversion the README warns is defined differently in each of them.
- Each output is a different function of the same bar, so a store that landed in the wrong buffer — or a guard that skipped the wrong one — changes a value the gates compare. The multiplications are separate statements from any addition, so no `a*b+c` fusion site exists to add a second variable to a failure.
- Issue #262; rule B6a of `docs/error-handling-spec.md`.

## Inputs

- `inReal` — Price series to pass through

## Outputs

- `outFirstOptional` — Half the bar, declinable
- `outRequired` — The bar itself, always written
- `outSecondOptional` — A quarter of the bar, declinable

## Implementation

TA-Lib Definition: [`synth10.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/generator/input_synth/synth10/synth10.c) · [`synth10.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/generator/input_synth/synth10/synth10.yaml)
