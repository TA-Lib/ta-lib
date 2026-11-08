---
title: "Upside/Downside Gap Three Methods (CDLXSIDEGAP3METHODS)"
description: "A three-candle continuation pattern: two same-color candles separated by a real-body gap, followed by an opposite-color candle that fills into the gap."
---

## Summary

A three-candle continuation pattern: two same-color candles separated by a real-body gap, followed by an opposite-color candle that fills into the gap. Bullish (upside) when the first two candles are white, bearish (downside) when they are black.

## Chart

<ta-chart src="/functions/cdlxsidegap3methods.svg" ratio="720:274" alt="Drawn on the price pane over candles, with outInteger as markers on the candles, ▲ below the low for a bullish call and ▼ above the high for a bearish one."></ta-chart>

Drawn on the price pane over candles, with outInteger as markers on the candles, ▲ below the low for a bullish call and ▼ above the high for a bearish one. [Reference data (CSV)](/functions/cdlxsidegap3methods.csv) · [Chart (SVG)](/functions/cdlxsidegap3methods.svg)

## Notes

- This continuation pattern does not verify the prior trend it classically assumes; the caller must confirm the trend.
- Bulkowski's testing found BOTH directions of this pattern actually act as reversals more often than not, opposite the classic continuation label: the upside variant reverses bearish 59% of the time, the downside variant reverses bullish 62% of the time. ([thepatternsite.com](https://thepatternsite.com/UpGap3Methods.html))

## Inputs

- `inOpen` — Open price of each bar
- `inHigh` — High price of each bar
- `inLow` — Low price of each bar
- `inClose` — Close price of each bar

## Outputs

- `outInteger` — +100 when the two same-color candles are white (bullish/upside continuation), -100 when black (bearish/downside continuation), 0 otherwise. Equals candlecolor(1st candle) * 100

## Output Values

What candlestick values mean: [Candlestick Outputs](/api/candlestick-outputs/). The function's flags declare which values it can return: [rW7](/spec/inputs-outputs/#rw7).

| Value | Meaning |
|-------|---------|
| -100 | Downside Gap Three Methods: the first two candles are black — bearish continuation |
| 0 | No pattern |
| 100 | Upside Gap Three Methods: the first two candles are white — bullish continuation |

## Properties

**Numerical Stability:** [Start-Independent](/functions/stability.md#start-independent)

<div class="flag-table">

|  |
| :-- |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Overlap Input</span> |
| <span class="flag-box">✅</span> **Independent Y-Axis** <span class="flag-tip" tabindex="0" role="note" aria-label="Output is on its own scale, drawn in a separate pane below the price chart." data-tip="Output is on its own scale, drawn in a separate pane below the price chart.">i</span> |
| <span class="flag-box">✅</span> **Candlestick** <span class="flag-tip" tabindex="0" role="note" aria-label="A candlestick pattern; its values are listed under Output Values." data-tip="A candlestick pattern; its values are listed under Output Values.">i</span> |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Can Output NaN or ±Inf</span> |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Identity at Period 1</span> |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Display Shift</span> |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Uses Transcendental</span> |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Input Must Be Positive</span> |

</div>

## Implementation

Added in 0.4.0.

TA-Lib Definition: [`cdlxsidegap3methods.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/cdlxsidegap3methods/cdlxsidegap3methods.c) · [`cdlxsidegap3methods.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/cdlxsidegap3methods/cdlxsidegap3methods.yaml)

| Native | File |
|--------|------|
| C | [`ta_CDLXSIDEGAP3METHODS.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_CDLXSIDEGAP3METHODS.c) |
| Rust | [`cdlxsidegap3methods.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/cdlxsidegap3methods.rs) |
| Java | [`Core_CDLXSIDEGAP3METHODS.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/fragments/Core_CDLXSIDEGAP3METHODS.java) |
| C# | [`Core_CDLXSIDEGAP3METHODS.cs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/csharp/library/src/Core_CDLXSIDEGAP3METHODS.cs) |
| TypeScript | [`cdlxsidegap3methods.ts`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/ts/library/src/func/cdlxsidegap3methods.ts) |

Python: `talib.CDLXSIDEGAP3METHODS`, see [ta-lib-python](https://ta-lib.github.io/ta-lib-python/func_groups/pattern_recognition.html).

TA-Lib is also available for R and more using a [wrapper](/install/#wrappers).

## Aliases

Upside/Downside Gap Three Methods, Upside Gap Three Methods, Downside Gap Three Methods

## See Also

[CDLGAPSIDESIDEWHITE](/functions/cdlgapsidesidewhite.md) · [CDLTASUKIGAP](/functions/cdltasukigap.md) · [CDLRISEFALL3METHODS](/functions/cdlrisefall3methods.md)
