# SYNTH12

## Summary

Synthetic gate function: declares **a real output and an integer output in the same function**, interleaved as real/integer/real. No shipped function mixes output element types, and two generators asserted the mix was impossible until this fixture existed. It exists only to verify the code generator; it is never shipped (see `ta_codegen/generator/input_synth/README.md`).

## Formula

outHalf[i] = real_i / 2; outSign[i] = 1 if real_i > 0, -1 if real_i < 0, else 0; outQuarter[i] = real_i / 4

## Notes

- Covers a function whose outputs do **not** all share one element type. Verified against `ta_codegen/input/`: of 176 definitions, 65 declare integer outputs and every one of them is integer-only (61 `CDL*`, `HT_TRENDMODE`, `MAXINDEX`, `MININDEX`, `MINMAXINDEX`); the other 111 are real-only. `ta_variant_frame` and `ta_stream_frame` each carried a single per-FUNCTION `outIsInteger` flag and an `assert!` that fired the moment a definition mixed the two.
- Coverage trap: the types are **interleaved** (real, integer, real), not grouped. The frame thunks index `outReal[]` and `outInteger[]` by a per-type running counter, while the harnesses index their buffers by the global output position — two conventions that agree exactly while a function's outputs share a type. Grouping the reals first would leave only one slot disagreeing; interleaving leaves two. Reorder these outputs and the fixture still passes a generator that got the mapping wrong.
- Coverage trap: three outputs, which is the corpus maximum. A fourth would grow every harness's file-scope buffers; a second would drop this below the arity at which the two indexing conventions can be told apart at more than one slot.
- No `(int)` cast anywhere: the sign comes from comparisons, which are IEEE-identical in all four languages, and halving and quartering a double are exact. So every value this fixture emits is reproducible bit-for-bit without relying on a rounding mode.
- Each output is a different function of the same bar, so a store that landed in the wrong buffer changes a value the gates compare.
- Issue #262 established the per-type output arity these buffers are sized from; this fixture is what makes the mixed case reachable.

## Inputs

- `inReal` — Series to transform

## Outputs

- `outHalf` — The bar halved
- `outSign` — 1 when the bar is above zero, -1 when below, 0 when exactly zero
- `outQuarter` — The bar quartered

## Implementation

TA-Lib Definition: [`synth12.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/generator/input_synth/synth12/synth12.c) · [`synth12.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/generator/input_synth/synth12/synth12.yaml)
