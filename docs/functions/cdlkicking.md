---
title: CDLKICKING
description: "Two-candle pattern of two opposite-color marubozu (long bodies with very short shadows) separated by a price gap. A reversal signal whose direction is set by the second candle's color. Hit signals a reversal in the direction of the second candle: +100 bullish, -100 bearish."
---

# CDLKICKING

## Summary

Two-candle pattern of two opposite-color marubozu (long bodies with very short shadows) separated by a price gap. A reversal signal whose direction is set by the second candle's color. Hit signals a reversal in the direction of the second candle: +100 bullish, -100 bearish.

## Inputs

- `inPriceOHLC` — Open, High, Low, Close price series

## Outputs

- `outInteger` — +100 when the second candle is white (bullish), -100 when it is black (bearish), 0 otherwise

## Implementation

TA-Lib Definition: [`cdlkicking.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/cdlkicking/cdlkicking.c) · [`cdlkicking.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/cdlkicking/cdlkicking.yaml)

| Native | File |
|--------|------|
| C | [`ta_CDLKICKING.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_CDLKICKING.c) |
| Rust | [`cdlkicking.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/src/ta_func/cdlkicking.rs) |
| Java | [`Core_CDLKICKING.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/Core_CDLKICKING.java) |

TA-Lib is also available for Python, R and more using a [wrapper](https://ta-lib.org/wrappers/).

## Aliases

Kicking

## See Also

[CDLKICKINGBYLENGTH](/functions/cdlkickingbylength) · [CDLMARUBOZU](/functions/cdlmarubozu) · [CDLGAPSIDESIDEWHITE](/functions/cdlgapsidesidewhite)
