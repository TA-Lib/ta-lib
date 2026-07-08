---
title: CDLEVENINGSTAR
description: "A three-candle bearish reversal pattern: a long white candle, a short-bodied star gapping up, then a black candle closing well down into the first candle's body. A hit signals a bearish reversal (most significant in an uptrend)."
---

# CDLEVENINGSTAR

## Summary

A three-candle bearish reversal pattern: a long white candle, a short-bodied star gapping up, then a black candle closing well down into the first candle's body. A hit signals a bearish reversal (most significant in an uptrend).

## Notes

- Does not verify the preceding uptrend the bearish reversal classically assumes.
- The third candle only needs a body longer than short, not the full long body some definitions require.

## Inputs

- `inPriceOHLC` — OHLC price series (open, high, low, close)

## Outputs

- `outInteger` — -100 when detected (always bearish), 0 otherwise. Never emits +100

## Parameters

- `optInPenetration` — Fraction of the 1st candle's real body the 3rd close must penetrate below the 1st close (default 0.3); larger requires deeper penetration

## Implementation

TA-Lib Definition: [`cdleveningstar.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/cdleveningstar/cdleveningstar.c) · [`cdleveningstar.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/cdleveningstar/cdleveningstar.yaml)

| Native | File |
|--------|------|
| C | [`ta_CDLEVENINGSTAR.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_CDLEVENINGSTAR.c) |
| Rust | [`cdleveningstar.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/src/ta_func/cdleveningstar.rs) |
| Java | [`Core_CDLEVENINGSTAR.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/Core_CDLEVENINGSTAR.java) |

TA-Lib is also available for Python, R and more using a [wrapper](https://ta-lib.org/wrappers/).

## Aliases

Evening Star

## See Also

[CDLEVENINGDOJISTAR](cdleveningdojistar.md) · [CDLMORNINGSTAR](cdlmorningstar.md) · [CDLMORNINGDOJISTAR](cdlmorningdojistar.md) · CDLSTARSINSOUTH
