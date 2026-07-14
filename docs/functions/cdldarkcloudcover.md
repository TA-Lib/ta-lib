---
title: CDLDARKCLOUDCOVER
description: "A two-candle bearish reversal pattern: a long white candle followed by a black candle that opens above the prior high and closes deep into the prior white body past a penetration threshold. Signals a potential top. A hit (-100) is a bearish reversal signal, most meaningful after an uptrend."
---

# CDLDARKCLOUDCOVER

## Summary

A two-candle bearish reversal pattern: a long white candle followed by a black candle that opens above the prior high and closes deep into the prior white body past a penetration threshold. Signals a potential top. A hit (-100) is a bearish reversal signal, most meaningful after an uptrend.

## Notes

- Does not verify the preceding uptrend the bearish reversal classically assumes.

## Inputs

- `inPriceOHLC` — OHLC price series (open, high, low, close)

## Outputs

- `outInteger` — -100 when the pattern is detected (always bearish), 0 otherwise; never emits +100

## Parameters

- `optInPenetration` — Fraction of candle 1's real body that candle 2's close must penetrate below close[i-1] (default 0.5); larger values require deeper penetration

## Implementation

TA-Lib Definition: [`cdldarkcloudcover.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/cdldarkcloudcover/cdldarkcloudcover.c) · [`cdldarkcloudcover.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/cdldarkcloudcover/cdldarkcloudcover.yaml)

| Native | File |
|--------|------|
| C | [`ta_CDLDARKCLOUDCOVER.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_CDLDARKCLOUDCOVER.c) |
| Rust | [`cdldarkcloudcover.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/src/ta_func/cdldarkcloudcover.rs) |
| Java | [`Core_CDLDARKCLOUDCOVER.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/Core_CDLDARKCLOUDCOVER.java) |

TA-Lib is also available for Python, R and more using a [wrapper](https://ta-lib.org/wrappers/).

## Aliases

Dark Cloud Cover

## See Also

[CDLPIERCING](/functions/cdlpiercing) · [CDLENGULFING](/functions/cdlengulfing) · [CDLONNECK](/functions/cdlonneck)
