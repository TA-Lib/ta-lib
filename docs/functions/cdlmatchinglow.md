---
title: CDLMATCHINGLOW
description: "A two-candle pattern of two consecutive black (bearish) candles with equal closes (within a tolerance). Treated as a bullish reversal signal. A hit signals a potential bullish reversal (shared support close after two down candles)."
---

# CDLMATCHINGLOW

## Summary

A two-candle pattern of two consecutive black (bearish) candles with equal closes (within a tolerance). Treated as a bullish reversal signal. A hit signals a potential bullish reversal (shared support close after two down candles).

## Formula

Two candles i-1, i. Candle i-1: black (close<open). Candle i: black (close<open). Equal closes: close[i-1]-E <= close[i] <= close[i-1]+E, where E = the Equal average. No shadow, body-size, or gap conditions are checked.

## Notes

- The bullish-reversal reading assumes a prior downtrend, which is not verified.

## Inputs

- `inPriceOHLC` — OHLC price series (open, high, low, close)

## Outputs

- `outInteger` — +100 when the pattern is present, 0 otherwise. Only +100 is ever emitted (matching low is always bullish); never -100

## Implementation

TA-Lib Definition: [`cdlmatchinglow.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/cdlmatchinglow/cdlmatchinglow.c) · [`cdlmatchinglow.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/cdlmatchinglow/cdlmatchinglow.yaml)

| Native | File |
|--------|------|
| C | [`ta_CDLMATCHINGLOW.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_CDLMATCHINGLOW.c) |
| Rust | [`cdlmatchinglow.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/src/ta_func/cdlmatchinglow.rs) |
| Java | [`Core_CDLMATCHINGLOW.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/Core_CDLMATCHINGLOW.java) |

TA-Lib is also available for Python, R and more using a [wrapper](https://ta-lib.org/wrappers/).

## Aliases

Matching Low

## See Also

CDLMATCHINGHIGH · [CDLHOMINGPIGEON](/functions/cdlhomingpigeon)
