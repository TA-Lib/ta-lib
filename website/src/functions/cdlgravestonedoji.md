---
title: CDLGRAVESTONEDOJI
description: "Single-candle doji whose open and close sit at the low of the day, leaving a long upper shadow and no lower shadow. A doji variant whose bullish/bearish meaning depends on the surrounding trend, which the code does not judge. A hit marks a gravestone doji; its bullish vs bearish reversal meaning must be read against the prevailing trend, which this function does not check."
---

# CDLGRAVESTONEDOJI

## Summary

Single-candle doji whose open and close sit at the low of the day, leaving a long upper shadow and no lower shadow. A doji variant whose bullish/bearish meaning depends on the surrounding trend, which the code does not judge. A hit marks a gravestone doji; its bullish vs bearish reversal meaning must be read against the prevailing trend, which this function does not check.

## Formula

One candle. Detected when all hold: (1) doji body: realbody |close-open| <= BodyDoji average; (2) very short/absent lower shadow: lowerShadow < ShadowVeryShort average; (3) non-short upper shadow: upperShadow > ShadowVeryShort average (open/close at the low with an upper shadow).

## Notes

- Does not verify the prior trend that determines the pattern's bullish/bearish meaning.

## Inputs

- `inPriceOHLC` — OHLC price series (open, high, low, close)

## Outputs

- `outInteger` — +100 on a detected gravestone doji, 0 otherwise. Never negative; the positive sign is not a directional signal (evaluate relative to the trend)

## Implementation

TA-Lib Definition: [`cdlgravestonedoji.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/cdlgravestonedoji/cdlgravestonedoji.c) · [`cdlgravestonedoji.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/cdlgravestonedoji/cdlgravestonedoji.yaml)

| Native | File |
|--------|------|
| C | [`ta_CDLGRAVESTONEDOJI.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_CDLGRAVESTONEDOJI.c) |
| Rust | [`cdlgravestonedoji.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/cdlgravestonedoji.rs) |
| Java | [`Core_CDLGRAVESTONEDOJI.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/library/fragments/Core_CDLGRAVESTONEDOJI.java) |

TA-Lib is also available for Python, R and more using a [wrapper](https://ta-lib.org/wrappers/).

## Aliases

Gravestone Doji

## See Also

[CDLDOJI](/functions/cdldoji) · [CDLDRAGONFLYDOJI](/functions/cdldragonflydoji) · [CDLLONGLEGGEDDOJI](/functions/cdllongleggeddoji) · [CDLDOJISTAR](/functions/cdldojistar)
