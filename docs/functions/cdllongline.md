---
title: CDLLONGLINE
description: "A single-candle pattern: a long real body with short upper and short lower shadow. The signal direction follows the candle color (bullish if white, bearish if black). Signals strong directional conviction on the bar: +100 white/bullish, -100 black/bearish. Not intrinsically a reversal or continuation signal."
---

# CDLLONGLINE

## Summary

A single-candle pattern: a long real body with short upper and short lower shadow. The signal direction follows the candle color (bullish if white, bearish if black). Signals strong directional conviction on the bar: +100 white/bullish, -100 black/bearish. Not intrinsically a reversal or continuation signal.

## Inputs

- `inPriceOHLC` — OHLC price series (open, high, low, close)

## Outputs

- `outInteger` — +100 on a white (close>=open) long line, -100 on a black long line, 0 when no pattern

## Implementation

TA-Lib Definition: [`cdllongline.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/cdllongline/cdllongline.c) · [`cdllongline.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/cdllongline/cdllongline.yaml)

| Native | File |
|--------|------|
| C | [`ta_CDLLONGLINE.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_CDLLONGLINE.c) |
| Rust | [`cdllongline.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/src/ta_func/cdllongline.rs) |
| Java | [`Core_CDLLONGLINE.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/Core_CDLLONGLINE.java) |

TA-Lib is also available for Python, R and more using a [wrapper](https://ta-lib.org/wrappers/).

## Aliases

Long Line Candle, Long Line

## See Also

[CDLSHORTLINE](cdlshortline.md) · [CDLCLOSINGMARUBOZU](cdlclosingmarubozu.md) · [CDLMARUBOZU](cdlmarubozu.md) · [CDLLONGLEGGEDDOJI](cdllongleggeddoji.md)
