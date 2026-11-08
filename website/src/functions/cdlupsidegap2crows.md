---
title: "Upside Gap Two Crows (CDLUPSIDEGAP2CROWS)"
description: "A three-candle bearish reversal pattern: a long white candle, then a small black candle gapping up (a gap between the real bodies), then a black candle…"
---

## Summary

A three-candle bearish reversal pattern: a long white candle, then a small black candle gapping up (a gap between the real bodies), then a black candle that engulfs the second candle's real body but still closes above the first candle's close. Signals a bearish reversal. A hit (-100) is a bearish reversal signal, most meaningful in an uptrend.

## Chart

<ta-chart src="/functions/cdlupsidegap2crows.svg" ratio="720:274" alt="Drawn on the price pane over candles, with outInteger as markers on the candles, ▲ below the low for a bullish call and ▼ above the high for a bearish one (no occurrence in the reference data)."></ta-chart>

Drawn on the price pane over candles, with outInteger as markers on the candles, ▲ below the low for a bullish call and ▼ above the high for a bearish one (no occurrence in the reference data). [Reference data (CSV)](/functions/cdlupsidegap2crows.csv) · [Chart (SVG)](/functions/cdlupsidegap2crows.svg)

## Notes

- The pattern classically assumes a prior uptrend, but this function does not verify any trend.
- Although classically a bearish reversal, Bulkowski's testing found this actually acts as a bullish continuation 60% of the time, and even when it does work "the price move is often lousy." ([thepatternsite.com](https://www.thepatternsite.com/UpGapTwoCrows.html))

## Inputs

- `inOpen` — Open price of each bar
- `inHigh` — High price of each bar
- `inLow` — Low price of each bar
- `inClose` — Close price of each bar

## Outputs

- `outInteger` — -100 on a pattern bar, 0 otherwise. Bearish-only: this pattern never emits +100

## Output Values

What candlestick values mean: [Candlestick Outputs](/api/candlestick-outputs/). The function's flags declare which values it can return: [rW7](/spec/inputs-outputs/#rw7).

| Value | Meaning |
|-------|---------|
| -100 | Upside Gap Two Crows pattern detected: bearish |
| 0 | No pattern |

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

TA-Lib Definition: [`cdlupsidegap2crows.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/cdlupsidegap2crows/cdlupsidegap2crows.c) · [`cdlupsidegap2crows.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/cdlupsidegap2crows/cdlupsidegap2crows.yaml)

| Native | File |
|--------|------|
| C | [`ta_CDLUPSIDEGAP2CROWS.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_CDLUPSIDEGAP2CROWS.c) |
| Rust | [`cdlupsidegap2crows.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/cdlupsidegap2crows.rs) |
| Java | [`Core_CDLUPSIDEGAP2CROWS.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/fragments/Core_CDLUPSIDEGAP2CROWS.java) |
| C# | [`Core_CDLUPSIDEGAP2CROWS.cs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/csharp/library/src/Core_CDLUPSIDEGAP2CROWS.cs) |
| TypeScript | [`cdlupsidegap2crows.ts`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/ts/library/src/func/cdlupsidegap2crows.ts) |

Python: `talib.CDLUPSIDEGAP2CROWS`, see [ta-lib-python](https://ta-lib.github.io/ta-lib-python/func_groups/pattern_recognition.html).

TA-Lib is also available for R and more using a [wrapper](/install/#wrappers).

## Aliases

Upside Gap Two Crows

## See Also

[CDL2CROWS](/functions/cdl2crows.md) · [CDLGAPSIDESIDEWHITE](/functions/cdlgapsidesidewhite.md)
