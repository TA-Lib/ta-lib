---
title: CDLCOUNTERATTACK
description: "A two-candle pattern of two long, opposite-colored real bodies whose closing prices are (nearly) equal. Emits a bullish signal when the second candle is white and a bearish signal when it is black (a reversal signal, though its trend context is not checked). A hit signals a reversal: +100 (white 2nd candle) bullish, -100 (black 2nd candle) bearish; significance depends on a prior trend the code does not check."
---

# CDLCOUNTERATTACK

## Summary

A two-candle pattern of two long, opposite-colored real bodies whose closing prices are (nearly) equal. Emits a bullish signal when the second candle is white and a bearish signal when it is black (a reversal signal, though its trend context is not checked). A hit signals a reversal: +100 (white 2nd candle) bullish, -100 (black 2nd candle) bearish; significance depends on a prior trend the code does not check.

## Notes

- Does not verify the prior trend the reversal signal classically assumes.

## Inputs

- `inPriceOHLC` — OHLC price series (open, high, low, close)

## Outputs

- `outInteger` — +100 when the second candle is white (bullish), -100 when it is black (bearish), 0 when no pattern

## Implementation

TA-Lib Definition: [`cdlcounterattack.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/cdlcounterattack/cdlcounterattack.c) · [`cdlcounterattack.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/cdlcounterattack/cdlcounterattack.yaml)

| Native | File |
|--------|------|
| C | [`ta_CDLCOUNTERATTACK.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_CDLCOUNTERATTACK.c) |
| Rust | [`cdlcounterattack.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/src/ta_func/cdlcounterattack.rs) |
| Java | [`Core_CDLCOUNTERATTACK.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/Core_CDLCOUNTERATTACK.java) |

TA-Lib is also available for Python, R and more using a [wrapper](https://ta-lib.org/wrappers/).

## Aliases

Counterattack, Counterattack Lines, Meeting Lines

## See Also

[CDLPIERCING](cdlpiercing.md) · [CDLDARKCLOUDCOVER](cdldarkcloudcover.md) · [CDLGAPSIDESIDEWHITE](cdlgapsidesidewhite.md)
