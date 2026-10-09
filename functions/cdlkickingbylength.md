---
url: 'https://ta-lib.org/functions/cdlkickingbylength.md'
description: >-
  A two-candle pattern of two opposite-color marubozu (long body, very short
  shadows on both ends) separated by a gap.
---
# Kicking - bull/bear determined by the longer marubozu (CDLKICKINGBYLENGTH)

## Summary

A two-candle pattern of two opposite-color marubozu (long body, very short shadows on both ends) separated by a gap. A strong directional/reversal signal whose bull/bear bias is set by the longer of the two marubozu.

## Inputs

* `inOpen` — Open price of each bar
* `inHigh` — High price of each bar
* `inLow` — Low price of each bar
* `inClose` — Close price of each bar

## Outputs

* `outInteger` — +100 or -100 on a hit, 0 otherwise. Sign = candlecolor of the candle with the larger realbody (i if realbody(i) > realbody(i-1), else i-1; tie goes to i-1): +100 if that marubozu is white, -100 if black

## Output Values

How to read these values from the output's flags: [rW8](/spec/inputs-outputs/#rw8).

| Value | Meaning |
|-------|---------|
| -100 | Bearish Kicking by Length: the longer of the two marubozu (the first, when both are as long) closed black |
| 0 | No pattern |
| 100 | Bullish Kicking by Length: the longer of the two marubozu (the first, when both are as long) closed white |

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

TA-Lib Definition: [`cdlkickingbylength.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/cdlkickingbylength/cdlkickingbylength.c) · [`cdlkickingbylength.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/cdlkickingbylength/cdlkickingbylength.yaml)

| Native | File |
|--------|------|
| C | [`ta_CDLKICKINGBYLENGTH.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_CDLKICKINGBYLENGTH.c) |
| Rust | [`cdlkickingbylength.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/cdlkickingbylength.rs) |
| Java | [`Core_CDLKICKINGBYLENGTH.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/fragments/Core_CDLKICKINGBYLENGTH.java) |
| C# | [`Core_CDLKICKINGBYLENGTH.cs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/csharp/library/src/Core_CDLKICKINGBYLENGTH.cs) |

TA-Lib is also available for Python, R and more using a [wrapper](/install/#wrappers).

## Aliases

Kicking by Length, Kicking - bull/bear decided by the longer marubozu

## See Also

[CDLKICKING](/functions/cdlkicking.md) · [CDLMARUBOZU](/functions/cdlmarubozu.md) · [CDLGAPSIDESIDEWHITE](/functions/cdlgapsidesidewhite.md)
