---
title: CDLMORNINGSTAR
description: "A three-candle bottom-reversal pattern: a long black candle, a small-bodied star gapping down, then a white candle closing well up into the first candle's body. Bullish reversal signal. A hit signals a bullish reversal (most meaningful after a downtrend, which the code does not check)."
---

# CDLMORNINGSTAR

## Summary

A three-candle bottom-reversal pattern: a long black candle, a small-bodied star gapping down, then a white candle closing well up into the first candle's body. Bullish reversal signal. A hit signals a bullish reversal (most meaningful after a downtrend, which the code does not check).

## Notes

- The gap-down is measured between the candles' real bodies, not between their high/low ranges.
- A prior downtrend is not verified.

## Inputs

- `inPriceOHLC` — Open, High, Low, Close price series

## Outputs

- `outInteger` — +100 when the morning star is detected, 0 otherwise. Never negative (pattern is exclusively bullish)

## Parameters

- `optInPenetration` — Fraction of the 1st candle's body the 3rd close must exceed above the 1st close; larger = deeper penetration required (default 0.3)

## Implementation

TA-Lib Definition: [`cdlmorningstar.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/cdlmorningstar/cdlmorningstar.c) · [`cdlmorningstar.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/cdlmorningstar/cdlmorningstar.yaml)

| Native | File |
|--------|------|
| C | [`ta_CDLMORNINGSTAR.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_CDLMORNINGSTAR.c) |
| Rust | [`cdlmorningstar.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/cdlmorningstar.rs) |
| Java | [`Core_CDLMORNINGSTAR.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/library/fragments/Core_CDLMORNINGSTAR.java) |

TA-Lib is also available for Python, R and more using a [wrapper](https://ta-lib.org/wrappers/).

## Aliases

Morning Star

## See Also

[CDLMORNINGDOJISTAR](/functions/cdlmorningdojistar) · [CDLEVENINGSTAR](/functions/cdleveningstar) · [CDLABANDONEDBABY](/functions/cdlabandonedbaby) · [CDLDOJISTAR](/functions/cdldojistar)
