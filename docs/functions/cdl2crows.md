---
title: CDL2CROWS
description: "Three-candle bearish reversal pattern: a long white candle, then a black candle gapping up, then a black candle that opens inside the second body and closes down inside the first white body. A hit (-100) signals a bearish reversal; significant in an uptrend, which this function does not verify."
---

# CDL2CROWS

## Summary

Three-candle bearish reversal pattern: a long white candle, then a black candle gapping up, then a black candle that opens inside the second body and closes down inside the first white body. A hit (-100) signals a bearish reversal; significant in an uptrend, which this function does not verify.

## Notes

- Does not verify the prior uptrend the pattern classically assumes for significance.

## Inputs

- `inPriceOHLC` — OHLC price series (open, high, low, close)

## Outputs

- `outInteger` — -100 on a detected pattern (always bearish), 0 otherwise. Never emits +100

## Implementation

TA-Lib Definition: [`cdl2crows.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/cdl2crows/cdl2crows.c) · [`cdl2crows.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/cdl2crows/cdl2crows.yaml)

| Native | File |
|--------|------|
| C | [`ta_CDL2CROWS.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_CDL2CROWS.c) |
| Rust | [`cdl2crows.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/src/ta_func/cdl2crows.rs) |
| Java | [`Core_CDL2CROWS.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/Core_CDL2CROWS.java) |

TA-Lib is also available for Python, R and more using a [wrapper](https://ta-lib.org/wrappers/).

## Aliases

Two Crows

## See Also

[CDLUPSIDEGAP2CROWS](/functions/cdlupsidegap2crows.md) · [CDLIDENTICAL3CROWS](/functions/cdlidentical3crows.md)
