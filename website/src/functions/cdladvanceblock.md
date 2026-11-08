---
title: "Advance Block (CDLADVANCEBLOCK)"
description: "Three-candle bearish reversal pattern: three white candles with consecutively higher closes whose advance weakens (progressively smaller bodies and/or…"
---

## Summary

Three-candle bearish reversal pattern: three white candles with consecutively higher closes whose advance weakens (progressively smaller bodies and/or lengthening upper shadows). Signals that an uptrend's advance is being blocked. A hit (-100) is bearish: the advance is stalling/blocked; meaningful mainly within an existing uptrend.

## Chart

<ta-chart src="/functions/cdladvanceblock.svg" ratio="720:274" alt="Drawn on the price pane over candles, with outInteger as markers on the candles, ▲ below the low for a bullish call and ▼ above the high for a bearish one (no occurrence in the reference data)."></ta-chart>

Drawn on the price pane over candles, with outInteger as markers on the candles, ▲ below the low for a bullish call and ▼ above the high for a bearish one (no occurrence in the reference data). [Reference data (CSV)](/functions/cdladvanceblock.csv) · [Chart (SVG)](/functions/cdladvanceblock.svg)

## Notes

- Does not verify the prior uptrend the pattern classically assumes for significance.
- Although classically read as a bearish reversal, Bulkowski's testing found the Advance Block actually acts as a bullish continuation 64% of the time. ([thepatternsite.com](https://thepatternsite.com/AdvanceBlock.html))

## Inputs

- `inOpen` — Open price of each bar
- `inHigh` — High price of each bar
- `inLow` — Low price of each bar
- `inClose` — Close price of each bar

## Outputs

- `outInteger` — -100 on a detected pattern (always bearish), 0 otherwise; never emits +100

## Output Values

What candlestick values mean: [Candlestick Outputs](/api/candlestick-outputs/). The function's flags declare which values it can return: [rW7](/spec/inputs-outputs/#rw7).

| Value | Meaning |
|-------|---------|
| -100 | Advance Block pattern detected: bearish |
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

TA-Lib Definition: [`cdladvanceblock.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/cdladvanceblock/cdladvanceblock.c) · [`cdladvanceblock.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/cdladvanceblock/cdladvanceblock.yaml)

| Native | File |
|--------|------|
| C | [`ta_CDLADVANCEBLOCK.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_CDLADVANCEBLOCK.c) |
| Rust | [`cdladvanceblock.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/cdladvanceblock.rs) |
| Java | [`Core_CDLADVANCEBLOCK.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/fragments/Core_CDLADVANCEBLOCK.java) |
| C# | [`Core_CDLADVANCEBLOCK.cs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/csharp/library/src/Core_CDLADVANCEBLOCK.cs) |
| TypeScript | [`cdladvanceblock.ts`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/ts/library/src/func/cdladvanceblock.ts) |

Python: `talib.CDLADVANCEBLOCK`, see [ta-lib-python](https://ta-lib.github.io/ta-lib-python/func_groups/pattern_recognition.html).

TA-Lib is also available for R and more using a [wrapper](/install/#wrappers).

## Aliases

Advance Block

## See Also

[CDL3WHITESOLDIERS](/functions/cdl3whitesoldiers.md) · CDLDELIBERATION · [CDLSTALLEDPATTERN](/functions/cdlstalledpattern.md)
