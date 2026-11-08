---
title: "Stick Sandwich (CDLSTICKSANDWICH)"
description: "A three-candle bullish reversal pattern: two black candles (1st and 3rd) sandwiching a white candle, where the 3rd black candle closes at the same level…"
---

## Summary

A three-candle bullish reversal pattern: two black candles (1st and 3rd) sandwiching a white candle, where the 3rd black candle closes at the same level as the 1st (the "bread"). A hit signals a bullish reversal (significant in a downtrend, which the function does not verify).

## Chart

<ta-chart src="/functions/cdlsticksandwich.svg" ratio="720:274" alt="Drawn on the price pane over candles, with outInteger as markers on the candles, ▲ below the low for a bullish call and ▼ above the high for a bearish one (no occurrence in the reference data)."></ta-chart>

Drawn on the price pane over candles, with outInteger as markers on the candles, ▲ below the low for a bullish call and ▼ above the high for a bearish one (no occurrence in the reference data). [Reference data (CSV)](/functions/cdlsticksandwich.csv) · [Chart (SVG)](/functions/cdlsticksandwich.svg)

## Notes

- Although classically a bullish reversal (and TA-Lib only emits +100), Bulkowski's testing found it actually acts as a bearish continuation 62% of the time — despite that, it still ranks a respectable 14th of 103 patterns for overall performance. ([thepatternsite.com](https://thepatternsite.com/StickSandwich.html))

## Inputs

- `inOpen` — Open price of each bar
- `inHigh` — High price of each bar
- `inLow` — Low price of each bar
- `inClose` — Close price of each bar

## Outputs

- `outInteger` — +100 when the pattern is present, 0 otherwise. Never -100 — Stick Sandwich is always bullish

## Output Values

What candlestick values mean: [Candlestick Outputs](/api/candlestick-outputs/). The function's flags declare which values it can return: [rW7](/spec/inputs-outputs/#rw7).

| Value | Meaning |
|-------|---------|
| 0 | No pattern |
| 100 | Stick Sandwich detected — bullish reversal signal |

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

TA-Lib Definition: [`cdlsticksandwich.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/cdlsticksandwich/cdlsticksandwich.c) · [`cdlsticksandwich.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/cdlsticksandwich/cdlsticksandwich.yaml)

| Native | File |
|--------|------|
| C | [`ta_CDLSTICKSANDWICH.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_CDLSTICKSANDWICH.c) |
| Rust | [`cdlsticksandwich.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/cdlsticksandwich.rs) |
| Java | [`Core_CDLSTICKSANDWICH.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/fragments/Core_CDLSTICKSANDWICH.java) |
| C# | [`Core_CDLSTICKSANDWICH.cs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/csharp/library/src/Core_CDLSTICKSANDWICH.cs) |
| TypeScript | [`cdlsticksandwich.ts`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/ts/library/src/func/cdlsticksandwich.ts) |

Python: `talib.CDLSTICKSANDWICH`, see [ta-lib-python](https://ta-lib.github.io/ta-lib-python/func_groups/pattern_recognition.html).

TA-Lib is also available for R and more using a [wrapper](/install/#wrappers).

## Aliases

Stick Sandwich

## See Also

[CDLMATCHINGLOW](/functions/cdlmatchinglow.md) · [CDLHOMINGPIGEON](/functions/cdlhomingpigeon.md)
