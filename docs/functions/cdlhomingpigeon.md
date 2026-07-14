---
title: CDLHOMINGPIGEON
description: "Two-candle pattern: a long black candle followed by a small black candle whose real body sits inside the prior body. Bullish reversal signal. A hit signals a bullish reversal (meaningful in a downtrend, which the code does not verify)."
---

# CDLHOMINGPIGEON

## Summary

Two-candle pattern: a long black candle followed by a small black candle whose real body sits inside the prior body. Bullish reversal signal. A hit signals a bullish reversal (meaningful in a downtrend, which the code does not verify).

## Formula

Two candles at i-1 and i. Both black: close[i-1] < open[i-1] and close[i] < open[i]. First body long: realbody[i-1] > BodyLong average. Second body short: realbody[i] <= BodyShort average. Second body contained by first: open[i] < open[i-1] and close[i] > close[i-1].

## Notes

- Does not verify the preceding downtrend that the bullish reversal classically assumes.

## Inputs

- `inPriceOHLC` — OHLC price series (open, high, low, close)

## Outputs

- `outInteger` — +100 when the pattern is detected, 0 otherwise. Never emits -100 (always bullish)

## Implementation

TA-Lib Definition: [`cdlhomingpigeon.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/cdlhomingpigeon/cdlhomingpigeon.c) · [`cdlhomingpigeon.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/cdlhomingpigeon/cdlhomingpigeon.yaml)

| Native | File |
|--------|------|
| C | [`ta_CDLHOMINGPIGEON.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_CDLHOMINGPIGEON.c) |
| Rust | [`cdlhomingpigeon.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/src/ta_func/cdlhomingpigeon.rs) |
| Java | [`Core_CDLHOMINGPIGEON.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/Core_CDLHOMINGPIGEON.java) |

TA-Lib is also available for Python, R and more using a [wrapper](https://ta-lib.org/wrappers/).

## Aliases

Homing Pigeon

## See Also

[CDLHARAMI](/functions/cdlharami.md) · [CDLMATCHINGLOW](/functions/cdlmatchinglow.md)
