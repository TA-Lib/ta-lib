---
title: "Gravestone Doji (CDLGRAVESTONEDOJI)"
description: "Single-candle doji whose open and close sit at the low of the day, leaving a long upper shadow and no lower shadow."
---

## Summary

Single-candle doji whose open and close sit at the low of the day, leaving a long upper shadow and no lower shadow. A doji variant whose bullish/bearish meaning depends on the surrounding trend, which the code does not judge. A hit marks a gravestone doji; its bullish vs bearish reversal meaning must be read against the prevailing trend, which this function does not check.

## Formula

One candle. Detected when all hold: (1) doji body: realbody |close-open| <= BodyDoji average; (2) very short/absent lower shadow: lowerShadow < ShadowVeryShort average; (3) non-short upper shadow: upperShadow > ShadowVeryShort average (open/close at the low with an upper shadow).

## Notes

- Does not verify the prior trend that determines the pattern's bullish/bearish meaning.

## Inputs

- `inOpen` — Open price of each bar
- `inHigh` — High price of each bar
- `inLow` — Low price of each bar
- `inClose` — Close price of each bar

## Outputs

- `outInteger` — +100 on a detected gravestone doji, 0 otherwise. Never negative; the positive sign is not a directional signal (evaluate relative to the trend)

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

TA-Lib Definition: [`cdlgravestonedoji.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/cdlgravestonedoji/cdlgravestonedoji.c) · [`cdlgravestonedoji.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/cdlgravestonedoji/cdlgravestonedoji.yaml)

| Native | File |
|--------|------|
| C | [`ta_CDLGRAVESTONEDOJI.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_CDLGRAVESTONEDOJI.c) |
| Rust | [`cdlgravestonedoji.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/cdlgravestonedoji.rs) |
| Java | [`Core_CDLGRAVESTONEDOJI.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/fragments/Core_CDLGRAVESTONEDOJI.java) |

TA-Lib is also available for Python, R and more using a [wrapper](/install/#wrappers).

## Aliases

Gravestone Doji

## See Also

[CDLDOJI](/functions/cdldoji.md) · [CDLDRAGONFLYDOJI](/functions/cdldragonflydoji.md) · [CDLLONGLEGGEDDOJI](/functions/cdllongleggeddoji.md) · [CDLDOJISTAR](/functions/cdldojistar.md)
