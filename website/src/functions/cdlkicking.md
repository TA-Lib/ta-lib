---
title: "Kicking (CDLKICKING)"
description: "Two-candle pattern of two opposite-color marubozu (long bodies with very short shadows) separated by a price gap."
---

## Summary

Two-candle pattern of two opposite-color marubozu (long bodies with very short shadows) separated by a price gap. A reversal signal whose direction is set by the second candle's color.

## Notes

- Bulkowski's testing found Kicking reverses only 53% (bullish) / 54% (bearish) of the time — both "near random" — and it's also one of the rarest patterns he tracked (frequency rank 100/103 bullish, 102/103 bearish). ([thepatternsite.com](https://thepatternsite.com/KickingBull.html))

## Inputs

- `inOpen` — Open price of each bar
- `inHigh` — High price of each bar
- `inLow` — Low price of each bar
- `inClose` — Close price of each bar

## Outputs

- `outInteger` — +100 when the second candle is white (bullish), -100 when it is black (bearish), 0 otherwise

## Output Values

| Value | Meaning |
|-------|---------|
| -100 | Bearish Kicking: the second (gapping) marubozu closed black |
| 0 | No pattern |
| 100 | Bullish Kicking: the second (gapping) marubozu closed white |

## Properties

**Numerical Stability:** [Start-Independent](/functions/stability.md#start-independent)

<div class="flag-table">

|  |
| :-- |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Overlap Input</span> |
| <span class="flag-box">✅</span> **Independent Y-Axis** <span class="flag-tip" tabindex="0" role="note" aria-label="Output is on its own scale, drawn in a separate pane below the price chart." data-tip="Output is on its own scale, drawn in a separate pane below the price chart.">i</span> |
| <span class="flag-box">✅</span> **Candlestick** <span class="flag-tip" tabindex="0" role="note" aria-label="Output is an integer candlestick-pattern signal (e.g. -100 / 0 / +100)." data-tip="Output is an integer candlestick-pattern signal (e.g. -100 / 0 / +100).">i</span> |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Can Output NaN or ±Inf</span> |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Identity at Period 1</span> |

</div>

## Implementation

TA-Lib Definition: [`cdlkicking.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/cdlkicking/cdlkicking.c) · [`cdlkicking.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/cdlkicking/cdlkicking.yaml)

| Native | File |
|--------|------|
| C | [`ta_CDLKICKING.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_CDLKICKING.c) |
| Rust | [`cdlkicking.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/cdlkicking.rs) |
| Java | [`Core_CDLKICKING.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/fragments/Core_CDLKICKING.java) |

TA-Lib is also available for Python, R and more using a [wrapper](/install/#wrappers).

## Aliases

Kicking

## See Also

[CDLKICKINGBYLENGTH](/functions/cdlkickingbylength.md) · [CDLMARUBOZU](/functions/cdlmarubozu.md) · [CDLGAPSIDESIDEWHITE](/functions/cdlgapsidesidewhite.md)
