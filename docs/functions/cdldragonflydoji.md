---
title: CDLDRAGONFLYDOJI
description: "Single-candle pattern: a doji (open and close nearly equal) sitting at the top of the range, with no meaningful upper shadow and a long lower shadow. A reversal signal, but its bullish/bearish meaning depends on the prior trend (the code does not judge direction). A hit marks a dragonfly doji; treated as a potential reversal, but direction (bullish/bearish) must be read from the trend it appears in."
---

# CDLDRAGONFLYDOJI

## Summary

Single-candle pattern: a doji (open and close nearly equal) sitting at the top of the range, with no meaningful upper shadow and a long lower shadow. A reversal signal, but its bullish/bearish meaning depends on the prior trend (the code does not judge direction). A hit marks a dragonfly doji; treated as a potential reversal, but direction (bullish/bearish) must be read from the trend it appears in.

## Formula

Single candle. realbody <= BodyDoji average (doji body) AND upper shadow < ShadowVeryShort average (no/very short upper shadow) AND lower shadow > ShadowVeryShort average (lower shadow present, not very short). No color, gap, or trend test.

## Notes

- Does not verify the prior trend that determines the pattern's bullish/bearish meaning.

## Inputs

- `inPriceOHLC` — OHLC price bars (open, high, low, close)

## Outputs

- `outInteger` — +100 when the pattern is present, 0 otherwise; never -100. The +100 does not itself imply bullishness (must be read against the trend)

## Implementation

TA-Lib Definition: [`cdldragonflydoji.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/cdldragonflydoji/cdldragonflydoji.c) · [`cdldragonflydoji.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/cdldragonflydoji/cdldragonflydoji.yaml)

| Native | File |
|--------|------|
| C | [`ta_CDLDRAGONFLYDOJI.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_CDLDRAGONFLYDOJI.c) |
| Rust | [`cdldragonflydoji.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/src/ta_func/cdldragonflydoji.rs) |
| Java | [`Core_CDLDRAGONFLYDOJI.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/Core_CDLDRAGONFLYDOJI.java) |

TA-Lib is also available for Python, R and more using a [wrapper](https://ta-lib.org/wrappers/).

## Aliases

Dragonfly Doji

## See Also

[CDLDOJI](cdldoji.md) · [CDLGRAVESTONEDOJI](cdlgravestonedoji.md) · [CDLLONGLEGGEDDOJI](cdllongleggeddoji.md) · [CDLTAKURI](cdltakuri.md)
