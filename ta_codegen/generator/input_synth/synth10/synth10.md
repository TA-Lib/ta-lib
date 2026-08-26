# SYNTH10

## Summary

Synthetic gate function: exercises **two nullable outputs in one function**, end to end through every backend. MAMA's `outFAMA` is the only nullable output in the shipped corpus and it sits beside a single required one, so three of the four arms the output-distinctness guard branches on were unreachable. It exists only to verify the code generator; it is never shipped (see `ta_codegen/generator/input_synth/README.md`).

## Formula

outRequired[i] = real_i; outFirstOptional[i] = real_i / 2; outSecondOptional[i] = real_i / 4

## Notes

- The declaration order in the `.yaml` is load-bearing. The output-distinctness guard (issue #108) walks output pairs `(i, j)` with `i < j` and branches on which of a pair is nullable, and this order is the only one of three outputs that produces all three combinations: `(nullable, required)`, `(nullable, nullable)` and `(required, nullable)`.
- Two `NULL`s compare **equal**. An unguarded pair test rejects a caller who declined both outputs — the one call the feature exists to allow — so C and Java guard each nullable operand non-null before comparing. Three pairs also make those terms need parentheses (`&&` inside `||`), which one nullable output never showed. C# needs no such arm: an empty span is how it spells "declined", and `Overlaps` already answers false for one.
- `outRequired` owns the `outIdx` advance and the two declinable stores carry none. That is what makes guarding a store *complete*: the write is skipped and the cursor still moves. `mama.c` was reordered for the same reason (issue #125).
- Real outputs rather than this family's usual integer ones. MAMA's nullable output is real, so this mirrors the shipped case — and scaling by a power of two is exact, so every value is bitwise identical in all four backends with no `(int)` cast, the conversion the family's rules warn is defined differently in each of them.
- Rule B6a of `docs/error-handling-spec.md`; issue #262.

## Inputs

- `inReal` — Price series to pass through

## Outputs

- `outFirstOptional` — Half the bar, declinable
- `outRequired` — The bar itself, always written
- `outSecondOptional` — A quarter of the bar, declinable

## Implementation

TA-Lib Definition: [`synth10.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/generator/input_synth/synth10/synth10.c) · [`synth10.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/generator/input_synth/synth10/synth10.yaml)
