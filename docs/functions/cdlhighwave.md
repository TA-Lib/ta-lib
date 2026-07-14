---
title: CDLHIGHWAVE
description: "Single-candle pattern: a short real body with both a very long upper and a very long lower shadow. Signals market indecision; the output sign reports only candle color, not a bullish/bearish direction. A hit marks indecision (long-legged candle); not directional - sign encodes only the candle's color."
---

# CDLHIGHWAVE

## Summary

Single-candle pattern: a short real body with both a very long upper and a very long lower shadow. Signals market indecision; the output sign reports only candle color, not a bullish/bearish direction. A hit marks indecision (long-legged candle); not directional - sign encodes only the candle's color.

## Formula

One candle at index i. Hit when all hold: (1) short real body: real body < the BodyShort average; (2) very long upper shadow: upper shadow > the ShadowVeryLong average; (3) very long lower shadow: lower shadow > the ShadowVeryLong average. No color, gap, or trend condition.

## Inputs

- `inPriceOHLC` — OHLC price series (open, high, low, close)

## Outputs

- `outInteger` — On a hit, +100 when the candle is white (close >= open) or -100 when black (close < open); 0 otherwise. Sign denotes color, NOT bull/bear

## Implementation

TA-Lib Definition: [`cdlhighwave.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/cdlhighwave/cdlhighwave.c) · [`cdlhighwave.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/cdlhighwave/cdlhighwave.yaml)

| Native | File |
|--------|------|
| C | [`ta_CDLHIGHWAVE.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_CDLHIGHWAVE.c) |
| Rust | [`cdlhighwave.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/src/ta_func/cdlhighwave.rs) |
| Java | [`Core_CDLHIGHWAVE.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/Core_CDLHIGHWAVE.java) |

TA-Lib is also available for Python, R and more using a [wrapper](https://ta-lib.org/wrappers/).

## Aliases

High-Wave Candle, High Wave

## See Also

[CDLLONGLEGGEDDOJI](/functions/cdllongleggeddoji.md) · [CDLSPINNINGTOP](/functions/cdlspinningtop.md) · [CDLRICKSHAWMAN](/functions/cdlrickshawman.md) · [CDLDOJI](/functions/cdldoji.md)
