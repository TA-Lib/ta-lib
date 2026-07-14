---
title: CDLEVENINGDOJISTAR
description: "A three-candle bearish reversal pattern: a long white candle, a doji that gaps up (the star), then a black candle closing well down into the first candle's body. A stricter Evening Star whose middle candle must be a doji. Hit (-100) signals a bearish top reversal."
---

# CDLEVENINGDOJISTAR

## Summary

A three-candle bearish reversal pattern: a long white candle, a doji that gaps up (the star), then a black candle closing well down into the first candle's body. A stricter Evening Star whose middle candle must be a doji. Hit (-100) signals a bearish top reversal.

## Notes

- Does not verify the preceding uptrend the bearish reversal classically assumes.

## Inputs

- `inPriceOHLC` — Open, High, Low, Close price series

## Outputs

- `outInteger` — -100 when the pattern is detected, 0 otherwise. Always bearish; never emits +100

## Parameters

- `optInPenetration` — Fraction of the 1st real body the 3rd candle's close must penetrate (default 0.3); larger demands a deeper close into the first body

## Implementation

TA-Lib Definition: [`cdleveningdojistar.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/cdleveningdojistar/cdleveningdojistar.c) · [`cdleveningdojistar.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/cdleveningdojistar/cdleveningdojistar.yaml)

| Native | File |
|--------|------|
| C | [`ta_CDLEVENINGDOJISTAR.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_CDLEVENINGDOJISTAR.c) |
| Rust | [`cdleveningdojistar.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/src/ta_func/cdleveningdojistar.rs) |
| Java | [`Core_CDLEVENINGDOJISTAR.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/Core_CDLEVENINGDOJISTAR.java) |

TA-Lib is also available for Python, R and more using a [wrapper](https://ta-lib.org/wrappers/).

## Aliases

Evening Doji Star

## See Also

[CDLEVENINGSTAR](/functions/cdleveningstar) · [CDLMORNINGDOJISTAR](/functions/cdlmorningdojistar) · [CDLDOJISTAR](/functions/cdldojistar) · [CDLABANDONEDBABY](/functions/cdlabandonedbaby)
