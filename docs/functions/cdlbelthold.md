---
title: CDLBELTHOLD
description: "Single-candle pattern with a long real body that opens at (or near) its extreme. A bullish belt-hold is a long white candle with no/very short lower shadow; a bearish belt-hold is a long black candle with no/very short upper shadow. A white hit is bullish (opens at the low, closes strong); a black hit is bearish (opens at the high, closes weak)."
---

# CDLBELTHOLD

## Summary

Single-candle pattern with a long real body that opens at (or near) its extreme. A bullish belt-hold is a long white candle with no/very short lower shadow; a bearish belt-hold is a long black candle with no/very short upper shadow. A white hit is bullish (opens at the low, closes strong); a black hit is bearish (opens at the high, closes weak).

## Formula

One candle. Requires real body > BodyLong average (long body), then either: white body (close>=open) AND lower shadow < ShadowVeryShort average -> bullish; OR black body (close<open) AND upper shadow < ShadowVeryShort average -> bearish. No prior-trend or gap conditions are checked.

## Notes

- Does not verify the prior trend that the pattern's bullish/bearish reading classically assumes.

## Inputs

- `inPriceOHLC` — OHLC price series (open, high, low, close)

## Outputs

- `outInteger` — +100 for a bullish (white) belt-hold, -100 for a bearish (black) belt-hold, 0 otherwise

## Implementation

TA-Lib Definition: [`cdlbelthold.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/cdlbelthold/cdlbelthold.c) · [`cdlbelthold.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/cdlbelthold/cdlbelthold.yaml)

| Native | File |
|--------|------|
| C | [`ta_CDLBELTHOLD.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_CDLBELTHOLD.c) |
| Rust | [`cdlbelthold.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/src/ta_func/cdlbelthold.rs) |
| Java | [`Core_CDLBELTHOLD.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/Core_CDLBELTHOLD.java) |

TA-Lib is also available for Python, R and more using a [wrapper](https://ta-lib.org/wrappers/).

## Aliases

Belt-hold, Belt Hold Line

## See Also

[CDLCLOSINGMARUBOZU](cdlclosingmarubozu.md) · [CDLMARUBOZU](cdlmarubozu.md) · [CDLLONGLINE](cdllongline.md)
