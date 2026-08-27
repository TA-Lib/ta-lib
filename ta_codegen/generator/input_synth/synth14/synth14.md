# SYNTH14

## Summary

Synthetic gate function: a **cross-indicator call whose result feeds mixed real and integer outputs**. SYNTH12 carries the mixed outputs in the loop tier and SYNTH13 carries the cross-call with a single real output; the intersection — composed *and* mixed — is what this one covers, and it is the shape a band-plus-direction indicator takes when its band comes from a sub-call. It exists only to verify the code generator; it is never shipped (see `ta_codegen/generator/input_synth/README.md`).

## Formula

outAvg[i] = SMA(inReal, n)[i] / 2; outSide[i] = 1 if that average is above zero, -1 if below, else 0; outTwice[i] = SMA(inReal, n)[i] * 2

## Notes

- Covers the COMPOSED streaming tier with a non-real output. A cross-call puts a function in that tier, where the emitters allocate an `sc_<out>` scratch per output and carry a `cur_<out>` scalar per output — both of which were sized `double` unconditionally, in all four backends, guarded by an assert per backend rather than by a type. Verified against `ta_codegen/input/`: every shipped composed function is real-only, so nothing in the corpus reaches this.
- Coverage trap: the outputs must not be derivable without the sub-call. Replace `sma()` with an inline loop and this silently becomes a second SYNTH12 — same assertions, none of the composed emitters, and the four asserts it exists to have removed would go untested.
- The `sc_` scratch is what the compilers catch: a `double *` scratch handed to a sub-call that writes `int *` is an incompatible pointer type in C, and the equivalent is a type error in the other three. The `cur_` scalar is not — an integer output's value is integral by construction, so a `double` scalar round-trips it losslessly. Typing it is correctness of representation, not a value fix; MEASURED, by regenerating with `cur_` forced back to `double` and finding the whole gate still green. Do not describe that scalar as a silent-truncation trap on the strength of the pointer case next to it.
- The scratch is a local buffer rather than one of the outputs: two outputs are `double` and one is `int`, so no single output can hold the sub-call's real-valued series for the other two to read afterwards.
- Halving and doubling are exact and the side comes from comparisons, so every value is reproducible bit-for-bit without depending on a rounding mode. The sub-call's own values are not clean decimals — `TA_SMA` carries a rolling total — which is what makes the golden row prove the derivation followed `sma.c` rather than a fresh-window sum.

## Inputs

- `inReal` — Series the sub-call averages

## Outputs

- `outAvg` — Half the sub-call's average
- `outSide` — 1 when that average is above zero, -1 when below, 0 when exactly zero
- `outTwice` — Twice the sub-call's average

## Parameters

- `optInTimePeriod` — Period handed to the sub-call

## Implementation

TA-Lib Definition: [`synth14.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/generator/input_synth/synth14/synth14.c) · [`synth14.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/generator/input_synth/synth14/synth14.yaml)
