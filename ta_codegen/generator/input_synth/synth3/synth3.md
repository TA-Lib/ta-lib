# SYNTH3

## Summary

Synthetic gate function: regression driver for issues #158 (integer locals typed by declaration, not by name), #159/#163 (an int-array element compared against the unsigned index domain) and #165 (a signed local inside an expression rather than standing alone). Its locals are deliberately named so that every naming heuristic in the code generator gets them wrong. It exists only to verify the code generator end to end across all backends; it is never shipped (see `ta_codegen/generator/input_synth/README.md`).

## Formula

outInteger[i] = ((2 × optInTimePeriod + fold(inReal[i])) & 65535) | (hits << 16), where fold(x) = (int)x for 0 < x < 1e6 and 0 otherwise, and hits packs eight comparisons of ring[fold & 3] = fold & 7 against 2 × optInTimePeriod.

## Notes

- Covers integer locals typed by their DECLARATION rather than their name (#158), an int-array element in mixed arithmetic against the unsigned index domain (#159, #163), and a signed local inside an expression rather than standing alone (#165) — in the main body and again in the lookback body, which is a separate rendering context. Verified against `ta_codegen/input/`: ULTOSC holds the corpus's only local int arrays and never puts one in mixed arithmetic.
- Coverage trap: the locals are named so that a name-based type heuristic gets them wrong — `k` is EMA's smoothing factor, and `slot`, `lag` and `barVal` are on no list at all. Rename them to something a heuristic would classify correctly and the fixture passes without testing anything.
- Coverage trap: the empty comment and the bare-asterisk comment in the body are fixtures too — a comment whose content reduces to nothing used to abort `generate`. Tidying either away removes that coverage silently.
- Coverage trap: every int-array intermediate is held non-negative. C compares in the signed domain and Rust widens to the unsigned one, so a negative intermediate makes this red for the index-domain convention's documented limitation instead of for what it tests.
- Issues #158, #159, #163, #165.

## Inputs

- `inReal` — Input series whose bars are folded into the integer domain

## Outputs

- `outInteger` — Packed bookkeeping result per bar: comparison bits in the high half, wrap-by-period result in the low 16 bits

## Parameters

- `optInTimePeriod` — Period the negative index is wrapped by

## Implementation

TA-Lib Definition: [`synth3.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/generator/input_synth/synth3/synth3.c) · [`synth3.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/generator/input_synth/synth3/synth3.yaml)
