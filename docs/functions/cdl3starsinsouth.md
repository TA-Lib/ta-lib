---
title: CDL3STARSINSOUTH
description: "A three-candle bullish reversal pattern of three consecutive black candles that progressively shrink and stabilize: a long black candle with a long lower shadow, a smaller black candle probing lower, then a small black marubozu contained within the second candle's range. A hit (+100) signals a bullish reversal; per the code comment it is meaningful in a downtrend, but the function does not verify prior trend."
---

# CDL3STARSINSOUTH

## Summary

A three-candle bullish reversal pattern of three consecutive black candles that progressively shrink and stabilize: a long black candle with a long lower shadow, a smaller black candle probing lower, then a small black marubozu contained within the second candle's range. A hit (+100) signals a bullish reversal; per the code comment it is meaningful in a downtrend, but the function does not verify prior trend.

## Notes

- Does not verify the prior downtrend the pattern classically assumes for significance.

## Inputs

- `inPriceOHLC` — OHLC price series (open, high, low, close)

## Outputs

- `outInteger` — +100 on the bar where the pattern completes (always bullish), 0 otherwise. Never emits -100

## Implementation

TA-Lib Definition: [`cdl3starsinsouth.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/cdl3starsinsouth/cdl3starsinsouth.c) · [`cdl3starsinsouth.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/cdl3starsinsouth/cdl3starsinsouth.yaml)

| Native | File |
|--------|------|
| C | [`ta_CDL3STARSINSOUTH.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_CDL3STARSINSOUTH.c) |
| Rust | [`cdl3starsinsouth.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/src/ta_func/cdl3starsinsouth.rs) |
| Java | [`Core_CDL3STARSINSOUTH.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/Core_CDL3STARSINSOUTH.java) |

TA-Lib is also available for Python, R and more using a [wrapper](https://ta-lib.org/wrappers/).

## Aliases

Three Stars In The South

## See Also

[CDL3BLACKCROWS](cdl3blackcrows.md) · [CDLIDENTICAL3CROWS](cdlidentical3crows.md) · [CDL3WHITESOLDIERS](cdl3whitesoldiers.md)
