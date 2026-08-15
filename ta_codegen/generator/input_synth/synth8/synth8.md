# SYNTH8

## Summary

Synthetic gate function: exposes the value of `ta_candleaverage` as real outputs so the existing bitwise cross-language gates can compare them. SYNTH7 is the same idea for `ta_candlerange`, and carries the rationale for making these intermediates observable at all. It exists only to verify the code generator end to end across all backends; it is never shipped (see `ta_codegen/generator/input_synth/README.md`).

## Formula

outAvgShadows[i] = candleAverage(ShadowShort, sum, bar i); outAvgCurrentBar[i] = candleAverage(ShadowVeryLong, sum, bar i)

## Notes

- The two outputs cover both branches of the helper and both of its divisors: an averaged `Shadows` window (`avgPeriod != 0`, the halving divisor, and the divergent arm) and the `avgPeriod == 0` branch that reads the current bar's range instead of the running sum (divisor 1.0).
- The sum passed for `outAvgCurrentBar` is ignored by that branch by construction. It is passed anyway because that is what a real caller does, and a backend that wrongly consulted it would diverge here.
- The window's per-bar ranges live in a circular buffer, so a bar that has left the window is never re-read from the input arrays — the same reason `cmf.c` carries its volume in the buffer. Together with reading the current bar fully before writing any output, that is what makes these outputs safe to alias an input.
- No shipped candlestick has to satisfy either property: their outputs are `int`, which the in-place alias gate never aliases onto a `double` input. SYNTH7 and SYNTH8 are the first OHLC consumers with real outputs, and the gate caught both of them the first time round.
- Nothing is clamped, for the reason given in SYNTH7: the divergent region is bars whose low sits below half their high, and clamping those away would hide what the gate measures.

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
