# SYNTH8

## Summary

Synthetic gate function: exposes the value of `ta_candleaverage` as real outputs so the existing bitwise cross-language gates can compare them. SYNTH7 is the same idea for `ta_candlerange`, and carries the rationale for making these intermediates observable at all. It exists only to verify the code generator end to end across all backends; it is never shipped (see `ta_codegen/generator/input_synth/README.md`).

## Formula

outAvgShadows[i] = candleAverage(ShadowShort, sum, bar i); outAvgCurrentBar[i] = candleAverage(ShadowVeryLong, sum, bar i)

## Notes

- Covers the VALUE of `ta_candleaverage` as real outputs, both of the helper's branches and both of its divisors: an averaged window (`avgPeriod != 0`, the halving divisor, the divergent arm) and the `avgPeriod == 0` branch that reads the current bar's range instead of the running sum. SYNTH7 carries why the shipped corpus cannot observe either.
- Coverage trap: the window update stays ONE expression, `sum += new - old`, as every shipped candlestick writes it. Split into `-= old` then `+= new` it rounds twice against the running total instead of once against the difference, and measurably re-rounds the per-bar 1-ULP difference away — green and blind.
- Coverage trap: nothing is clamped, and the helper result goes to a local rather than inline — both for the reasons given in SYNTH7.
- The sum handed to the `avgPeriod == 0` output is ignored by that branch by construction. It is passed anyway because that is what a real caller does, and a backend that wrongly consulted it diverges here.
- The window's per-bar ranges live in a circular buffer, so a bar that has left the window is never re-read from the inputs; with every read of the current bar happening before any store, that is what lets these real outputs alias an input. No shipped candlestick has to satisfy either property — their outputs are `int`, which the alias gate never aliases onto a `double` input.
- Issue #216.

## Inputs

- `inOpen` — Open price of each bar
- `inHigh` — High price of each bar
- `inLow` — Low price of each bar
- `inClose` — Close price of each bar

## Outputs

- `outAvgShadows` — Candle average over a `Shadows` window
- `outAvgCurrentBar` — Candle average with `avgPeriod` zero, reading the current bar

## Implementation

TA-Lib Definition: [`synth8.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/generator/input_synth/synth8/synth8.c) · [`synth8.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/generator/input_synth/synth8/synth8.yaml)
