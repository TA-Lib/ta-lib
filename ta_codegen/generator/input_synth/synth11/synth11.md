# SYNTH11

## Summary

Synthetic gate function: exercises **a third integer output** — one more than the shipped corpus has ever declared, and the count that sizes the C harnesses' file-scope output buffers — and **a `cond ? 1 : 0` stored into an integer output**, which Java and C# used to collapse into code that does not compile. Both were literals or folds the corpus happened never to reach. It exists only to verify the code generator; it is never shipped (see `ta_codegen/generator/input_synth/README.md`).

## Formula

outAbove[i] = 1 if real_i > 0 else 0; outBelow[i] = 1 if real_i < 0 else 0; outLarge[i] = 1 if real_i > 1000 else 0

## Notes

- The C harnesses — `ta_codegen_serve`, `ta_bench`, `ta_bench_stream` and the in-server `stream_verify` — hand every function the same file-scope buffers, and the counts were the literals 3 (double) and 2 (int): what MACD/BBANDS/STOCH and MINMAXINDEX happen to need. A third integer output compiled to `'g_outIntBuf2' undeclared`, and nothing in the tree could reach it. The counts now come from `common::max_output_arity`, and this fixture is what makes them move — and what fails if anyone writes a literal back.
- **A `cond ? 1 : 0` stored into an integer output.** Java and C# collapse that ternary to the bare condition — right where a boolean is wanted, wrong here: C has no booleans, so the destination is an `int`, and `outAbove[outIdx] = bar > 0.0;` does not compile in either language. The corpus writes `? 1 : 0` only inside helper predicates consumed by an `if`, so nothing reached the bad case. The emitters now keep the ternary on an assignment's right-hand side, and these three stores are what prove it.
- **Not here, deliberately: a real output beside the integer ones.** The output-distinctness guard could not compile a cross-typed term in three of the four backends and now skips such pairs (Appendix E of `docs/error-handling-spec.md`), but that is not the only thing in the way — `ta_variant_frame` and `ta_stream_frame` carry one `outIsInteger` flag per *function*, and `test_variants.c` branches on it. Mixing output types is a feature with its own assert, not a fixture away.
- No `(int)` cast anywhere. The outputs come from comparisons, which are IEEE-identical in all four languages, rather than from a conversion the family's rules warn is defined differently in each of them.
- Issue #262.

## Inputs

- `inReal` — Price series to classify

## Outputs

- `outAbove` — 1 when the bar is above zero, 0 otherwise
- `outBelow` — 1 when the bar is below zero, 0 otherwise
- `outLarge` — 1 when the bar is above 1000, 0 otherwise

## Implementation

TA-Lib Definition: [`synth11.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/generator/input_synth/synth11/synth11.c) · [`synth11.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/generator/input_synth/synth11/synth11.yaml)
