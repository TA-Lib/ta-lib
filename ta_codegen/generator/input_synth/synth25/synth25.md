# SYNTH25

## Summary

Synthetic gate function: **one intermediate series read by three legs at three parameter shifts**, one leg starting early and two ending early. It exists only to verify the code generator end to end across all backends; it is never shipped (see `ta_codegen/generator/input_synth/README.md`).

## Formula

m = MOM(inReal, 1). Each leg is MIDPOINT(m, n): leg A read optInShiftA bars back, leg B optInShiftB, leg C optInShiftC. outAB[i] = 1, -1 or 0 as leg A is above, below or equal to leg B; outAC[i] the same against leg C.

## Notes

- Covers a composed stream whose sub-calls read an intermediate at parameter depths: the two legs that end early tap one delay line on `m` with two per-handle backs, and the leg that starts early is read through a tap whose depth Open captures because its begin is only presumed. The shipped corpus has no tap on a sub-call source and no line with more than one tap (#583).
- Coverage trap: keep the yaml default shifts pairwise distinct and nonzero. Equal shifts make the per-handle backs indistinguishable from fixed ones, and a zero shift takes the depth-0 select at the defaults.
- Coverage trap: every shift's range minimum is 0. That is what puts a tap at depth 0 in the range-minimum vectors.

## Inputs

- `inReal` — Input series

## Outputs

- `outAB` — Sign of leg A minus leg B
- `outAC` — Sign of leg A minus leg C

## Parameters

- `optInTimePeriod` — Period of every leg's midpoint
- `optInShiftA` — Bars leg A is read back
- `optInShiftB` — Bars leg B is read back
- `optInShiftC` — Bars leg C is read back
