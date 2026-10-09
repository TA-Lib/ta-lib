---
title: "Doji (CDLDOJI)"
description: "Single-candle Doji recognizer: fires when the real body (|close-open|) is at or below the BodyDoji threshold."
---

## Summary

Single-candle Doji recognizer: fires when the real body (|close-open|) is at or below the BodyDoji threshold. Market indecision; neither bullish nor bearish on its own.

## Formula

match if $|close-open| \le \text{CandleAverage(BodyDoji)}$

## Inputs

- `inOpen` — Open price of each bar
- `inHigh` — High price of each bar
- `inLow` — Low price of each bar
- `inClose` — Close price of each bar

## Outputs

- `outInteger` — 100 when a doji is detected, else 0

## Output Values

How to read these values from the output's flags: [rW8](/spec/inputs-outputs/#rw8).

| Value | Meaning |
|-------|---------|
| 0 | No doji |
| 100 | Doji detected — market indecision; neither bullish nor bearish on its own |

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

</div>

## Implementation

TA-Lib Definition: [`cdldoji.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/cdldoji/cdldoji.c) · [`cdldoji.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/cdldoji/cdldoji.yaml)

| Native | File |
|--------|------|
| C | [`ta_CDLDOJI.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_CDLDOJI.c) |
| Rust | [`cdldoji.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/cdldoji.rs) |
| Java | [`Core_CDLDOJI.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/fragments/Core_CDLDOJI.java) |
| C# | [`Core_CDLDOJI.cs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/csharp/library/src/Core_CDLDOJI.cs) |

TA-Lib is also available for Python, R and more using a [wrapper](/install/#wrappers).

## Aliases

Doji

## See Also

[CDLDOJISTAR](/functions/cdldojistar.md) · [CDLDRAGONFLYDOJI](/functions/cdldragonflydoji.md) · [CDLGRAVESTONEDOJI](/functions/cdlgravestonedoji.md) · [CDLLONGLEGGEDDOJI](/functions/cdllongleggeddoji.md)
