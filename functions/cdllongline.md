---
url: 'https://ta-lib.org/functions/cdllongline.md'
description: >-
  A single-candle pattern: a long real body with short upper and short lower
  shadow.
---
# Long Line Candle (CDLLONGLINE)

## Summary

A single-candle pattern: a long real body with short upper and short lower shadow. The signal direction follows the candle color (bullish if white, bearish if black). Signals strong directional conviction on the bar. Not intrinsically a reversal or continuation signal.

## Inputs

* `inOpen` — Open price of each bar
* `inHigh` — High price of each bar
* `inLow` — Low price of each bar
* `inClose` — Close price of each bar

## Outputs

* `outInteger` — +100 on a white (close>=open) long line, -100 on a black long line, 0 when no pattern

## Output Values

How to read these values from the output's flags: [rW8](/spec/inputs-outputs/#rw8).

| Value | Meaning |
|-------|---------|
| -100 | Black long-line candle (long body, short shadows) |
| 0 | No pattern |
| 100 | White long-line candle (long body, short shadows) |

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

TA-Lib Definition: [`cdllongline.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/cdllongline/cdllongline.c) · [`cdllongline.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/cdllongline/cdllongline.yaml)

| Native | File |
|--------|------|
| C | [`ta_CDLLONGLINE.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_CDLLONGLINE.c) |
| Rust | [`cdllongline.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/cdllongline.rs) |
| Java | [`Core_CDLLONGLINE.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/fragments/Core_CDLLONGLINE.java) |
| C# | [`Core_CDLLONGLINE.cs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/csharp/library/src/Core_CDLLONGLINE.cs) |

TA-Lib is also available for Python, R and more using a [wrapper](/install/#wrappers).

## Aliases

Long Line Candle, Long Line

## See Also

[CDLSHORTLINE](/functions/cdlshortline.md) · [CDLCLOSINGMARUBOZU](/functions/cdlclosingmarubozu.md) · [CDLMARUBOZU](/functions/cdlmarubozu.md) · [CDLLONGLEGGEDDOJI](/functions/cdllongleggeddoji.md)
