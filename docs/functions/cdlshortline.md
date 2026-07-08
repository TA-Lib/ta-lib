---
title: CDLSHORTLINE
description: "Single-candle pattern: a short real body with short upper and lower shadows (a small-range candle). Not a directional signal — the output sign encodes candle color, not bullish/bearish sentiment. A hit only flags a small-range candle; the +/- sign is the candle's color (white/black), not a reversal or continuation call."
---

# CDLSHORTLINE

## Summary

Single-candle pattern: a short real body with short upper and lower shadows (a small-range candle). Not a directional signal — the output sign encodes candle color, not bullish/bearish sentiment. A hit only flags a small-range candle; the +/- sign is the candle's color (white/black), not a reversal or continuation call.

## Formula

One candle at i, all three:
- short real body: real body < the BodyShort average
- short upper shadow: upper shadow < the ShadowShort average
- short lower shadow: lower shadow < the ShadowShort average
If matched: output = candle color * 100 (+100 white, -100 black); else 0.

## Inputs

- `inPriceOHLC` — OHLC price series (open, high, low, close)

## Outputs

- `outInteger` — +100 for a matching white candle (close>=open), -100 for a matching black candle (close<open), 0 when no pattern. Sign is candle color, NOT bullish/bearish

## Implementation

TA-Lib Definition: [`cdlshortline.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/cdlshortline/cdlshortline.c) · [`cdlshortline.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/cdlshortline/cdlshortline.yaml)

| Native | File |
|--------|------|
| C | [`ta_CDLSHORTLINE.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_CDLSHORTLINE.c) |
| Rust | [`cdlshortline.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/src/ta_func/cdlshortline.rs) |
| Java | [`Core_CDLSHORTLINE.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/Core_CDLSHORTLINE.java) |

TA-Lib is also available for Python, R and more using a [wrapper](https://ta-lib.org/wrappers/).

## Aliases

Short Line Candle, Short Line

## See Also

[CDLLONGLINE](cdllongline.md) · [CDLSPINNINGTOP](cdlspinningtop.md) · [CDLDOJI](cdldoji.md)
