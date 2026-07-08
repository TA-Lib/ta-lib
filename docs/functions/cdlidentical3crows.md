---
title: CDLIDENTICAL3CROWS
description: "A three-candle bearish reversal pattern: three consecutive declining black candles, each with a very short (or no) lower shadow, where each candle after the first opens at or very near the prior candle's close. A hit signals a bearish reversal (pattern is always bearish)."
---

# CDLIDENTICAL3CROWS

## Summary

A three-candle bearish reversal pattern: three consecutive declining black candles, each with a very short (or no) lower shadow, where each candle after the first opens at or very near the prior candle's close. A hit signals a bearish reversal (pattern is always bearish).

## Notes

- Does not verify the preceding uptrend that the bearish reversal classically assumes.
- Does not require the three bodies to be equal in size; 'identical' refers only to each candle opening at or near the previous candle's close.

## Inputs

- `inPriceOHLC` — Open, High, Low, Close price series

## Outputs

- `outInteger` — -100 when the pattern is detected (always bearish), 0 otherwise. Never emits +100

## Implementation

TA-Lib Definition: [`cdlidentical3crows.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/cdlidentical3crows/cdlidentical3crows.c) · [`cdlidentical3crows.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/cdlidentical3crows/cdlidentical3crows.yaml)

| Native | File |
|--------|------|
| C | [`ta_CDLIDENTICAL3CROWS.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_CDLIDENTICAL3CROWS.c) |
| Rust | [`cdlidentical3crows.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/src/ta_func/cdlidentical3crows.rs) |
| Java | [`Core_CDLIDENTICAL3CROWS.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/Core_CDLIDENTICAL3CROWS.java) |

TA-Lib is also available for Python, R and more using a [wrapper](https://ta-lib.org/wrappers/).

## Aliases

Identical Three Crows

## See Also

[CDL3BLACKCROWS](cdl3blackcrows.md) · [CDL2CROWS](cdl2crows.md)
