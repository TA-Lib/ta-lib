---
title: "Two Crows (CDL2CROWS)"
description: "Three-candle bearish reversal pattern: a long white candle, then a black candle gapping up, then a black candle that opens inside the second body and…"
---

## Summary

Three-candle bearish reversal pattern: a long white candle, then a black candle gapping up, then a black candle that opens inside the second body and closes down inside the first white body. A hit (-100) signals a bearish reversal; significant in an uptrend, which this function does not verify.

## Chart

<ta-chart src="/functions/cdl2crows.svg" ratio="720:274" alt="Drawn on the price pane over candles, with outInteger as markers on the candles, ▲ below the low for a bullish call and ▼ above the high for a bearish one (no occurrence in the reference data)."></ta-chart>

Drawn on the price pane over candles, with outInteger as markers on the candles, ▲ below the low for a bullish call and ▼ above the high for a bearish one (no occurrence in the reference data). [Reference data (CSV)](/functions/cdl2crows.csv) · [Chart (SVG)](/functions/cdl2crows.svg)

## Notes

- Does not verify the prior uptrend the pattern classically assumes for significance.
- Bulkowski's testing found this reverses bearishly only 54% of the time — "near random" — despite the pattern's classic always-bearish label; the breakout direction cannot be predicted with any real accuracy. ([thepatternsite.com](https://thepatternsite.com/TwoCrows.html))

## Inputs

- `inOpen` — Open price of each bar
- `inHigh` — High price of each bar
- `inLow` — Low price of each bar
- `inClose` — Close price of each bar

## Outputs

- `outInteger` — -100 on a detected pattern (always bearish), 0 otherwise. Never emits +100

## Output Values

What candlestick values mean: [Candlestick Outputs](/api/candlestick-outputs/). The function's flags declare which values it can return: [rW7](/spec/inputs-outputs/#rw7).

| Value | Meaning |
|-------|---------|
| -100 | Two Crows pattern detected: bearish |
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

TA-Lib Definition: [`cdl2crows.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/cdl2crows/cdl2crows.c) · [`cdl2crows.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/cdl2crows/cdl2crows.yaml)

| Native | File |
|--------|------|
| C | [`ta_CDL2CROWS.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_CDL2CROWS.c) |
| Rust | [`cdl2crows.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/cdl2crows.rs) |
| Java | [`Core_CDL2CROWS.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/fragments/Core_CDL2CROWS.java) |
| C# | [`Core_CDL2CROWS.cs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/csharp/library/src/Core_CDL2CROWS.cs) |
| TypeScript | [`cdl2crows.ts`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/ts/library/src/func/cdl2crows.ts) |

Python: `talib.CDL2CROWS`, see [ta-lib-python](https://ta-lib.github.io/ta-lib-python/func_groups/pattern_recognition.html).

TA-Lib is also available for R and more using a [wrapper](/install/#wrappers).

## Aliases

Two Crows

## See Also

[CDLUPSIDEGAP2CROWS](/functions/cdlupsidegap2crows.md) · [CDLIDENTICAL3CROWS](/functions/cdlidentical3crows.md)
