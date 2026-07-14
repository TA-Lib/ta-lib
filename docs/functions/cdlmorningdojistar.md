---
title: CDLMORNINGDOJISTAR
description: "A three-candle bullish reversal pattern: a long black candle, then a doji that gaps down, then a white candle closing well up into the first candle's body. It is the doji-star variant of the morning star. A hit (+100) signals a bullish reversal; most meaningful after a downtrend, which this function does not verify."
---

# CDLMORNINGDOJISTAR

## Summary

A three-candle bullish reversal pattern: a long black candle, then a doji that gaps down, then a white candle closing well up into the first candle's body. It is the doji-star variant of the morning star. A hit (+100) signals a bullish reversal; most meaningful after a downtrend, which this function does not verify.

## Notes

- The gap-down is measured between the candles' real bodies, not between their high/low ranges.
- A prior downtrend is not verified.

## Inputs

- `inPriceOHLC` — Open, High, Low, Close price series

## Outputs

- `outInteger` — +100 when the pattern is detected, 0 otherwise. Always bullish; never emits -100

## Parameters

- `optInPenetration` — Fraction (default 0.3) of the 1st candle's real body the 3rd close must exceed above close[i-2]; larger values demand deeper penetration into the black body

## Implementation

TA-Lib Definition: [`cdlmorningdojistar.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/cdlmorningdojistar/cdlmorningdojistar.c) · [`cdlmorningdojistar.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/cdlmorningdojistar/cdlmorningdojistar.yaml)

| Native | File |
|--------|------|
| C | [`ta_CDLMORNINGDOJISTAR.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_CDLMORNINGDOJISTAR.c) |
| Rust | [`cdlmorningdojistar.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/src/ta_func/cdlmorningdojistar.rs) |
| Java | [`Core_CDLMORNINGDOJISTAR.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/Core_CDLMORNINGDOJISTAR.java) |

TA-Lib is also available for Python, R and more using a [wrapper](https://ta-lib.org/wrappers/).

## Aliases

Morning Doji Star

## See Also

[CDLMORNINGSTAR](/functions/cdlmorningstar.md) · [CDLEVENINGDOJISTAR](/functions/cdleveningdojistar.md) · [CDLEVENINGSTAR](/functions/cdleveningstar.md) · [CDLDOJISTAR](/functions/cdldojistar.md)
