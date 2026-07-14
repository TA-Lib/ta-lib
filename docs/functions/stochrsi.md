---
title: STOCHRSI
description: "Applies the Fast Stochastic (STOCHF) oscillator to an RSI series instead of price, measuring where RSI sits within its recent min/max range. Oscillates 0-100; high = RSI near its recent top, low = near its recent bottom."
---

# STOCHRSI

## Summary

Applies the Fast Stochastic (STOCHF) oscillator to an RSI series instead of price, measuring where RSI sits within its recent min/max range. Oscillates 0-100; high = RSI near its recent top, low = near its recent bottom.

## Formula

rsi = RSI(inReal, optInTimePeriod)
FastK = 100 * (rsi_t - min(rsi, FastK_Period)) / (max(rsi, FastK_Period) - min(rsi, FastK_Period))
FastD = MA(FastK, FastD_Period, FastD_MAType)

## Notes

- To reproduce the original article's unsmoothed Stochastic RSI, set the RSI period equal to the %K period and read the raw %K output.
- When the RSI's recent range is zero, %K is set to 0 instead of being undefined.

## Inputs

- `inReal` — Source series fed into the RSI calculation

## Outputs

- `outFastK` — Unsmoothed stochastic of the RSI (raw %K)
- `outFastD` — %K smoothed over FastD_Period (signal line)

## Parameters

- `optInTimePeriod` — RSI period
- `optInFastK_Period` — Lookback window for the RSI min/max stochastic
- `optInFastD_Period` — Smoothing period for %D
- `optInFastD_MAType` — MA type used to smooth %D

## Implementation

TA-Lib Definition: [`stochrsi.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/stochrsi/stochrsi.c) · [`stochrsi.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/stochrsi/stochrsi.yaml)

| Native | File |
|--------|------|
| C | [`ta_STOCHRSI.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_STOCHRSI.c) |
| Rust | [`stochrsi.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/src/ta_func/stochrsi.rs) |
| Java | [`Core_STOCHRSI.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/Core_STOCHRSI.java) |

TA-Lib is also available for Python, R and more using a [wrapper](https://ta-lib.org/wrappers/).

## Aliases

Stochastic RSI

## See Also

[RSI](/functions/rsi.md) · [STOCHF](/functions/stochf.md) · [STOCH](/functions/stoch.md) · [MA](/functions/ma.md)

## References

- Tushar S. Chande, Stanley Kroll, *The New Technical Trader*, John Wiley & Sons (ISBN 0471597805)
