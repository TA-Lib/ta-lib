---
title: ULTOSC
description: "Ultimate Oscillator: momentum indicator combining buying-pressure/true-range ratios over three time periods into one 0-100 weighted average. Blends short-, medium-, and long-term momentum to damp single-period noise. Ranges 0-100; conventionally >70 overbought, <30 oversold."
---

# ULTOSC

## Summary

Ultimate Oscillator: momentum indicator combining buying-pressure/true-range ratios over three time periods into one 0-100 weighted average. Blends short-, medium-, and long-term momentum to damp single-period noise. Ranges 0-100; conventionally >70 overbought, <30 oversold.

## Formula

trueLow = min(low, prevClose);  BP = close - trueLow
TR = max(high-low, |prevClose-high|, |prevClose-low|)
avg_n = (sum BP over n bars) / (sum TR over n bars)
ULTOSC = 100 * (4*avg_short + 2*avg_mid + avg_long) / 7

## Notes

- The three periods are sorted internally, so the 4/2/1 weighting always applies to the shortest, middle, and longest period regardless of the order in which you pass them.

## Inputs

- `inPriceHLC` — High/Low/Close price series

## Outputs

- `outReal` — Ultimate Oscillator value

## Parameters

- `optInTimePeriod1` — Bars for one averaging window
- `optInTimePeriod2` — Bars for another averaging window
- `optInTimePeriod3` — Bars for another averaging window

## Implementation

TA-Lib Definition: [`ultosc.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/ultosc/ultosc.c) · [`ultosc.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/ultosc/ultosc.yaml)

| Native | File |
|--------|------|
| C | [`ta_ULTOSC.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_ULTOSC.c) |
| Rust | [`ultosc.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/ultosc.rs) |
| Java | [`Core_ULTOSC.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/library/fragments/Core_ULTOSC.java) |

TA-Lib is also available for Python, R and more using a [wrapper](https://ta-lib.org/wrappers/).

## Aliases

Ultimate Oscillator, UO

## See Also

[ATR](/functions/atr) · [TRANGE](/functions/trange) · [RSI](/functions/rsi)

## References

- Larry Williams, *The Ultimate Oscillator*, Technical Analysis of Stocks & Commodities, V.3:4 (1985)
