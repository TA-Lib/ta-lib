---
title: "Stochastic (STOCH)"
description: "Slow Stochastic oscillator: locates the close within the high-low range over a lookback period, then double-smooths it."
---

## Summary

Slow Stochastic oscillator: locates the close within the high-low range over a lookback period, then double-smooths it. Returns the Slow-%K and Slow-%D lines. SlowK/SlowD > 80 overbought, < 20 oversold; %K crossing %D signals momentum shifts.

## Formula

FastK = 100*(Close - LL_n)/(HH_n - LL_n), n = FastK_Period (LL/HH = lowest low / highest high over n)
SlowK = MA(FastK, SlowK_Period, SlowK_MAType)
SlowD = MA(SlowK, SlowD_Period, SlowD_MAType)

## Notes

- When the high-low range over the window is zero, the raw stochastic is set to 0 instead of being undefined.

## Inputs

- `inHigh` — High price of each bar
- `inLow` — Low price of each bar
- `inClose` — Close price of each bar

## Outputs

- `outSlowK` — Raw FastK smoothed by SlowK_Period MA
- `outSlowD` — Signal line: SlowK smoothed by SlowD_Period MA

## Parameters

| Parameter | Type | Default | Accepted values | Description |
| --- | --- | --- | --- | --- |
| `optInFastK_Period` | integer | 5 | 1–100000 | Lookback window for the raw %K high-low range |
| `optInSlowK_Period` | integer | 3 | 1–100000 | Smoothing period turning FastK into SlowK |
| `optInSlowK_MAType` | MAType | SMA (0) | any MAType | MA type used to smooth into SlowK |
| `optInSlowD_Period` | integer | 3 | 1–100000 | Smoothing period for the SlowD signal line |
| `optInSlowD_MAType` | MAType | SMA (0) | any MAType | MA type used for the SlowD line |

*`MAType` values: 0 SMA · 1 EMA · 2 WMA · 3 DEMA · 4 TEMA · 5 TRIMA · 6 KAMA · 7 MAMA · 8 T3 · 9 HMA · 10 DISABLED · 11 DEFAULT · 12 ZLEMA*

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

TA-Lib Definition: [`stoch.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/stoch/stoch.c) · [`stoch.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/stoch/stoch.yaml)

| Native | File |
|--------|------|
| C | [`ta_STOCH.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_STOCH.c) |
| Rust | [`stoch.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/stoch.rs) |
| Java | [`Core_STOCH.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/fragments/Core_STOCH.java) |

TA-Lib is also available for Python, R and more using a [wrapper](/install/#wrappers).

## Aliases

Stochastic, Stochastic Oscillator, Slow Stochastic

## See Also

[STOCHF](/functions/stochf.md) · [STOCHRSI](/functions/stochrsi.md) · [MA](/functions/ma.md)
