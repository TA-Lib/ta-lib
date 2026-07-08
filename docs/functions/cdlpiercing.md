---
title: CDLPIERCING
description: "Two-candle pattern: a long black candle followed by a long white candle that opens below the prior low and closes back above the midpoint of the prior black body. Bullish reversal signal. A hit (+100) is a bullish reversal signal."
---

# CDLPIERCING

## Summary

Two-candle pattern: a long black candle followed by a long white candle that opens below the prior low and closes back above the midpoint of the prior black body. Bullish reversal signal. A hit (+100) is a bullish reversal signal.

## Notes

- A prior downtrend is not verified.

## Inputs

- `inPriceOHLC` — OHLC price series (open, high, low, close)

## Outputs

- `outInteger` — +100 when the piercing pattern is detected; 0 otherwise. Always bullish, never emits -100

## Implementation

TA-Lib Definition: [`cdlpiercing.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/cdlpiercing/cdlpiercing.c) · [`cdlpiercing.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/cdlpiercing/cdlpiercing.yaml)

| Native | File |
|--------|------|
| C | [`ta_CDLPIERCING.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_CDLPIERCING.c) |
| Rust | [`cdlpiercing.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/src/ta_func/cdlpiercing.rs) |
| Java | [`Core_CDLPIERCING.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/Core_CDLPIERCING.java) |

TA-Lib is also available for Python, R and more using a [wrapper](https://ta-lib.org/wrappers/).

## Aliases

Piercing Pattern, Piercing Line

## See Also

[CDLDARKCLOUDCOVER](cdldarkcloudcover.md) · [CDLENGULFING](cdlengulfing.md) · [CDLMORNINGSTAR](cdlmorningstar.md)
