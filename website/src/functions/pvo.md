---
title: "Percentage Volume Oscillator (PVO)"
description: "Percentage Volume Oscillator: a variation of the Percentage Price Oscillator (PPO, created by Gerald Appel) applied to the volume series instead of price."
---

## Summary

Percentage Volume Oscillator: a variation of the [Percentage Price Oscillator](/functions/ppo.md) (PPO, created by Gerald Appel) applied to the **volume** series instead of price. It is the difference between a fast and slow moving average of volume, expressed as a percentage of the slow MA. Positive when short-term volume is above its longer-term average (rising participation), negative when below. The default periods (12, 26) match MACD and PPO.

## Formula

PVO = ((fastMA(inVolume) - slowMA(inVolume)) / slowMA(inVolume)) * 100, both MAs of type optInMAType; output = 0 when slowMA == 0

The standard form is exponential with periods 12 and 26 — ((12-day EMA of Volume - 26-day EMA of Volume) / 26-day EMA of Volume) * 100, i.e. the PPO/MACD oscillator computed on volume. `optInMAType` therefore **defaults to EMA** — the moving average Gerald Appel used for the original PPO/MACD; pass another type (e.g. `TA_MAType_SMA`) to override.

## Notes

- `optInMAType` applies to both the fast and slow moving average. `TA_MAType_MAMA` ignores its period argument, so with `optInMAType = TA_MAType_MAMA` the fast and slow MAs are identical, making the numerator — and therefore the output — zero at every bar.

## Inputs

- `inVolume` — Volume of each bar

## Outputs

- `outReal` — PVO value in percent

## Parameters

| Parameter | Type | Default | Accepted values | Description |
| --- | --- | --- | --- | --- |
| `optInFastPeriod` | integer | 12 | 2–100000 | Period of the fast MA |
| `optInSlowPeriod` | integer | 26 | 2–100000 | Period of the slow MA |
| `optInMAType` | MAType | EMA (1) | any MAType | Moving average type used for both MAs |

*`MAType` values: 0 SMA · 1 EMA · 2 WMA · 3 DEMA · 4 TEMA · 5 TRIMA · 6 KAMA · 7 MAMA · 8 T3 · 9 HMA · 10 DISABLED · 11 DEFAULT · 12 ZLEMA · 13 RMA*

## Properties

**Numerical Stability:** [Depends on MA Type](/functions/stability.md#depends-on-ma-type) — This function's default, EMA, has an initial unstable period.

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

TA-Lib Definition: [`pvo.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/pvo/pvo.c) · [`pvo.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/pvo/pvo.yaml)

| Native | File |
|--------|------|
| C | [`ta_PVO.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_PVO.c) |
| Rust | [`pvo.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/pvo.rs) |
| Java | [`Core_PVO.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/fragments/Core_PVO.java) |

TA-Lib is also available for Python, R and more using a [wrapper](/install/#wrappers).

## Aliases

Percentage Volume Oscillator

## See Also

[PPO](/functions/ppo.md) · [OBV](/functions/obv.md) · [MACD](/functions/macd.md)

## References

- PVO has no separately documented originator; it applies the PPO/MACD oscillator (Gerald Appel) to the volume series.
- Formula and standard (12, 26, 9) parameters: [Percentage Volume Oscillator (PVO)](https://chartschool.stockcharts.com/table-of-contents/technical-indicators-and-overlays/technical-indicators/percentage-volume-oscillator-pvo), StockCharts ChartSchool; also documented by [TradingView](https://www.tradingview.com/support/solutions/43000591350-percentage-volume-oscillator-pvo/).
