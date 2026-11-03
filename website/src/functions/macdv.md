---
title: "MACD-V, Volatility Normalised Momentum (MACDV)"
description: "MACD-V, Alex Spiroglou's volatility-normalised MACD: the spread of a fast and a slow EMA of the close over the Average True Range, times 100."
---

## Summary

MACD-V, Alex Spiroglou's volatility-normalised MACD: the spread of a fast and a slow EMA of the close over the Average True Range, times 100. Like MACD it comes with an EMA-smoothed signal line and their histogram. Dividing by the ATR expresses momentum in units of the instrument's own volatility, so one reading means the same thing across instruments, timeframes and volatility regimes, where the raw MACD spread does not. Spiroglou reads the line in ranges: beyond 150 or -150 is an overextended move, between -50 and 50 a ranging market, and the bands in between a trend.

## Formula

MACDV = (EMA(Close, fastPeriod) - EMA(Close, slowPeriod)) / ATR(slowPeriod) * 100
Signal = EMA(MACDV, signalPeriod)
Hist = MACDV - Signal

## Notes

- The Average True Range is Wilder's, over the slow period: the publication's `ATR(26)` beside the 26-bar slow EMA, so one parameter sets both. Without price movement the line falls to 0, its neutral point.
- The two price EMAs and the Average True Range each warm up as when calling `EMA` and `ATR` directly, so a caller who wants the line converged sets `TA_FUNC_UNST_EMA` and `TA_FUNC_UNST_ATR`, as for those functions.
- If the slow period is set smaller than the fast period, the two are swapped so the slow EMA is always the longer one, as `MACD` does; the ATR period follows the slow period after the swap.
- A signal period of 1 disables signal-line smoothing: the signal equals the MACD-V line and the histogram is zero.
- The publication's MACD section writes the histogram as the signal line minus the MACD line; its reading of the histogram ("above 40" is stretched to the upside) holds only for MACD-V minus the signal, which is what is emitted here, as for `MACD`.

## Inputs

- `inHigh` — High price of each bar
- `inLow` — Low price of each bar
- `inClose` — Close price of each bar

## Outputs

- `outMACDV` — Fast EMA minus slow EMA, divided by the Average True Range, times 100
- `outMACDVSignal` — EMA of the MACD-V line
- `outMACDVHist` — MACD-V minus signal line

## Parameters

| Parameter | Type | Default | Accepted values | Description |
| --- | --- | --- | --- | --- |
| `optInFastPeriod` | integer | 12 | 2–100000 | Period of the fast EMA |
| `optInSlowPeriod` | integer | 26 | 2–100000 | Period of the slow EMA and of the Average True Range |
| `optInSignalPeriod` | integer | 9 | 1–100000 | Smoothing period of the signal line |

## Properties

**Numerical Stability:** [Initial Unstable Period](/functions/stability.md#initial-unstable-period) — Inherited from ATR and EMA, which MACDV computes internally; tunable via ATR and EMA's unstable period.

<div class="flag-table">

|  |
| :-- |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Overlap Input</span> |
| <span class="flag-box">✅</span> **Independent Y-Axis** <span class="flag-tip" tabindex="0" role="note" aria-label="Output is on its own scale, drawn in a separate pane below the price chart." data-tip="Output is on its own scale, drawn in a separate pane below the price chart.">i</span> |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Candlestick</span> |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Can Output NaN or ±Inf</span> |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Identity at Period 1</span> |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Display Shift</span> |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Uses Transcendental</span> |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Input Must Be Positive</span> |

</div>

## Implementation

TA-Lib Definition: [`macdv.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/macdv/macdv.c) · [`macdv.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/macdv/macdv.yaml)

| Native | File |
|--------|------|
| C | [`ta_MACDV.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_MACDV.c) |
| Rust | [`macdv.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/macdv.rs) |
| Java | [`Core_MACDV.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/fragments/Core_MACDV.java) |
| C# | [`Core_MACDV.cs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/csharp/library/src/Core_MACDV.cs) |
| TypeScript | [`macdv.ts`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/ts/library/src/func/macdv.ts) |

TA-Lib is also available for Python, R and more using a [wrapper](/install/#wrappers).

## Aliases

MACD-V, volatility normalised MACD, volatility normalized MACD, volatility normalised momentum

## See Also

[MACD](/functions/macd.md) · [ATR](/functions/atr.md) · [EMA](/functions/ema.md) · [APO](/functions/apo.md) · [PPO](/functions/ppo.md)

## References

- Alex Spiroglou, *MACD-V: Volatility Normalised Momentum*, 2022, [NAAIM Founders Award paper](https://www.naaim.org/wp-content/uploads/2022/05/MACD-V-Alex-Spiroglou-WEB.pdf)
- [MACD-V (StockCharts ChartSchool)](https://chartschool.stockcharts.com/table-of-contents/technical-indicators-and-overlays/technical-indicators/macd-v)
