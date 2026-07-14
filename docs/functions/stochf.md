---
title: STOCHF
description: "Fast Stochastic Oscillator: the raw %K line and its moving-average-smoothed %D line. Unlike STOCH (which slows both lines), STOCHF returns the unsmoothed FastK and FastD. Oscillates 0-100; >80 overbought, <20 oversold."
---

# STOCHF

## Summary

Fast Stochastic Oscillator: the raw %K line and its moving-average-smoothed %D line. Unlike STOCH (which slows both lines), STOCHF returns the unsmoothed FastK and FastD. Oscillates 0-100; >80 overbought, <20 oversold.

## Formula

FastK = 100 * (Close - LowestLow) / (HighestHigh - LowestLow), over the last FastK_Period bars (incl. today)
FastD = MA(FastK, FastD_Period, FastD_MAType)

## Notes

- When the high-low range over the window is zero, %K is set to 0 instead of being undefined.

## Inputs

- `inPriceHLC` — High/Low/Close price series

## Outputs

- `outFastK` — Raw %K stochastic line
- `outFastD` — MA-smoothed %K (signal line)

## Parameters

- `optInFastK_Period` — Lookback window for the highest-high/lowest-low of Fast-K
- `optInFastD_Period` — Smoothing period for the Fast-D line
- `optInFastD_MAType` — Moving-average type used to smooth Fast-D

## Implementation

TA-Lib Definition: [`stochf.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/stochf/stochf.c) · [`stochf.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/stochf/stochf.yaml)

| Native | File |
|--------|------|
| C | [`ta_STOCHF.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_STOCHF.c) |
| Rust | [`stochf.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/src/ta_func/stochf.rs) |
| Java | [`Core_STOCHF.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/Core_STOCHF.java) |

TA-Lib is also available for Python, R and more using a [wrapper](https://ta-lib.org/wrappers/).

## Aliases

Stochastic Fast, Fast Stochastic Oscillator

## See Also

[STOCH](/functions/stoch.md) · [STOCHRSI](/functions/stochrsi.md) · [MA](/functions/ma.md)
