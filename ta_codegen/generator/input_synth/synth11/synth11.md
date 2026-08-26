# SYNTH11

## Summary

Synthetic gate function: exercises **a third integer output** — one more than the shipped corpus has ever declared — and **a `cond ? 1 : 0` stored into an integer output**. Neither is reachable from the shipped corpus. It exists only to verify the code generator; it is never shipped (see `ta_codegen/generator/input_synth/README.md`).

## Formula

outAbove[i] = 1 if real_i > 0 else 0; outBelow[i] = 1 if real_i < 0 else 0; outLarge[i] = 1 if real_i > 1000 else 0

## Notes

- Covers a THIRD integer output — one more than the corpus has ever declared, MINMAXINDEX's two being the maximum — and a `cond ? 1 : 0` stored into an integer output. Verified against `ta_codegen/input/`: the corpus's only `? 1 : 0` are four helper predicates in `helpers/candlestick.c`, each RETURNED from the helper and consumed in an `if` condition — never stored into an output array, which is the form under test.
- Coverage trap: three integer outputs, not two. The C harnesses size their file-scope output buffers from the corpus's maximum arity, so dropping one output here puts that sizing back under the corpus's own ceiling — the state in which a third integer output failed to compile.
- Not here, deliberately: a real output beside the integer ones. `ta_variant_frame` and `ta_stream_frame` carry one `outIsInteger` flag per FUNCTION and `test_variants.c` branches on it, so mixing output types is a feature with its own assert, not a fixture away.
- No `(int)` cast anywhere: the outputs come from comparisons, which are IEEE-identical in all four languages.
- Each output is a different function of the same bar, so a store that landed in the wrong buffer changes a value the gates compare.
- Issue #262.

## Inputs

- `inReal` — Price series to classify

## Outputs

- `outAbove` — 1 when the bar is above zero, 0 otherwise
- `outBelow` — 1 when the bar is below zero, 0 otherwise
- `outLarge` — 1 when the bar is above 1000, 0 otherwise

## Implementation

TA-Lib Definition: [`synth11.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/generator/input_synth/synth11/synth11.c) · [`synth11.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/generator/input_synth/synth11/synth11.yaml)
