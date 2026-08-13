---
title: "Stochastic Fast (STOCHF)"
description: "Fast Stochastic Oscillator: the raw %K line and its moving-average-smoothed %D line."
---

## Summary

Fast Stochastic Oscillator: the raw %K line and its moving-average-smoothed %D line. Unlike STOCH (which slows both lines), STOCHF returns the unsmoothed FastK and FastD. Oscillates 0-100; >80 overbought, <20 oversold.

## Formula

FastK = 100 * (Close - LowestLow) / (HighestHigh - LowestLow), over the last FastK_Period bars (incl. today)
FastD = MA(FastK, FastD_Period, FastD_MAType)

## Notes

- When the high-low range over the window is zero, %K is set to 0 instead of being undefined.

## Inputs

- `inHigh` — High price of each bar
- `inLow` — Low price of each bar
- `inClose` — Close price of each bar

## Outputs

- `outFastK` — Raw %K stochastic line
- `outFastD` — MA-smoothed %K (signal line)

## Parameters

| Parameter | Type | Default | Accepted values | Description |
| --- | --- | --- | --- | --- |
| `optInFastK_Period` | integer | 5 | 1–100000 | Lookback window for the highest-high/lowest-low of Fast-K |
| `optInFastD_Period` | integer | 3 | 1–100000 | Smoothing period for the Fast-D line |
| `optInFastD_MAType` | MAType | SMA (0) | any MAType | Moving-average type used to smooth Fast-D |

*`MAType` values: 0 SMA · 1 EMA · 2 WMA · 3 DEMA · 4 TEMA · 5 TRIMA · 6 KAMA · 7 MAMA · 8 T3 · 9 HMA · 10 DISABLED · 11 DEFAULT*

## Properties

**Numerical Stability:** [Depends on MA Type](/functions/stability.md#depends-on-ma-type) — This function's default, SMA, is start-independent.

<div class="flag-table">

|  |
| :-- |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Overlap Input</span> |
| <span class="flag-box">✅</span> **Independent Y-Axis** <span class="flag-tip" tabindex="0" role="note" aria-label="Output is on its own scale, drawn in a separate pane below the price chart." data-tip="Output is on its own scale, drawn in a separate pane below the price chart.">i</span> |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Candlestick</span> |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Can Output NaN or ±Inf</span> |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Identity at Period 1</span> |

</div>

## Implementation

TA-Lib Definition: [`stochf.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/stochf/stochf.c) · [`stochf.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/stochf/stochf.yaml)

| Native | File |
|--------|------|
| C | [`ta_STOCHF.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_STOCHF.c) |
| Rust | [`stochf.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/stochf.rs) |
| Java | [`Core_STOCHF.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/fragments/Core_STOCHF.java) |

TA-Lib is also available for Python, R and more using a [wrapper](/install/#wrappers).

## Aliases

Stochastic Fast, Fast Stochastic Oscillator

## See Also

[STOCH](/functions/stoch.md) · [STOCHRSI](/functions/stochrsi.md) · [MA](/functions/ma.md)
