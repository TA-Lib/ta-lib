---
title: CDLINNECK
description: "A two-candle in-neck pattern: a long black candle followed by a white candle that opens below the prior low and closes just barely into the prior body (near the prior close). It is a bearish continuation signal. A hit signals bearish continuation (the down move is expected to resume)."
---

# CDLINNECK

## Summary

A two-candle in-neck pattern: a long black candle followed by a white candle that opens below the prior low and closes just barely into the prior body (near the prior close). It is a bearish continuation signal. A hit signals bearish continuation (the down move is expected to resume).

## Formula

Two candles. First: black (close1 < open1) with a long real body (realbody > candleaverage(BodyLong)). Second: white (close2 >= open2), opens below the first candle's low (open2 < low1), and closes slightly into the first body: close2 >= close1 AND close2 <= close1 + candleaverage(Equal). No prior-trend check is performed.

## Notes

- Does not verify the preceding downtrend that this bearish continuation pattern assumes.

## Inputs

- `inPriceOHLC` — OHLC price series (open, high, low, close)

## Outputs

- `outInteger` — -100 when the in-neck pattern is detected, 0 otherwise. This pattern only ever emits the negative (bearish) signal; it never emits +100

## Implementation

TA-Lib Definition: [`cdlinneck.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/cdlinneck/cdlinneck.c) · [`cdlinneck.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/cdlinneck/cdlinneck.yaml)

| Native | File |
|--------|------|
| C | [`ta_CDLINNECK.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_CDLINNECK.c) |
| Rust | [`cdlinneck.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/src/ta_func/cdlinneck.rs) |
| Java | [`Core_CDLINNECK.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/Core_CDLINNECK.java) |

TA-Lib is also available for Python, R and more using a [wrapper](https://ta-lib.org/wrappers/).

## Aliases

In-Neck Pattern, In-Neck Line

## See Also

[CDLONNECK](cdlonneck.md) · [CDLTHRUSTING](cdlthrusting.md) · [CDLMATCHINGLOW](cdlmatchinglow.md)
