---
title: CDL3BLACKCROWS
description: "A four-bar pattern: a white candle followed by three consecutive black (down) candles with successively lower closes, each opening inside the prior black's real body. It is a bearish reversal signal. A hit (-100) signals a bearish reversal."
---

# CDL3BLACKCROWS

## Summary

A four-bar pattern: a white candle followed by three consecutive black (down) candles with successively lower closes, each opening inside the prior black's real body. It is a bearish reversal signal. A hit (-100) signals a bearish reversal.

## Notes

- Does not verify the prior mature uptrend the pattern classically assumes for significance.

## Inputs

- `inPriceOHLC` — OHLC price series (open, high, low, close)

## Outputs

- `outInteger` — -100 when the bearish pattern is detected, 0 otherwise. Never emits +100

## Implementation

TA-Lib Definition: [`cdl3blackcrows.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/cdl3blackcrows/cdl3blackcrows.c) · [`cdl3blackcrows.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/cdl3blackcrows/cdl3blackcrows.yaml)

| Native | File |
|--------|------|
| C | [`ta_CDL3BLACKCROWS.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_CDL3BLACKCROWS.c) |
| Rust | [`cdl3blackcrows.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/src/ta_func/cdl3blackcrows.rs) |
| Java | [`Core_CDL3BLACKCROWS.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/Core_CDL3BLACKCROWS.java) |

TA-Lib is also available for Python, R and more using a [wrapper](https://ta-lib.org/wrappers/).

## Aliases

Three Black Crows, 3 Black Crows

## See Also

[CDL3WHITESOLDIERS](cdl3whitesoldiers.md) · [CDLIDENTICAL3CROWS](cdlidentical3crows.md) · [CDLADVANCEBLOCK](cdladvanceblock.md)
