---
title: CDLADVANCEBLOCK
description: "Three-candle bearish reversal pattern: three white candles with consecutively higher closes whose advance weakens (progressively smaller bodies and/or lengthening upper shadows). Signals that an uptrend's advance is being blocked. A hit (-100) is bearish: the advance is stalling/blocked; meaningful mainly within an existing uptrend."
---

# CDLADVANCEBLOCK

## Summary

Three-candle bearish reversal pattern: three white candles with consecutively higher closes whose advance weakens (progressively smaller bodies and/or lengthening upper shadows). Signals that an uptrend's advance is being blocked. A hit (-100) is bearish: the advance is stalling/blocked; meaningful mainly within an existing uptrend.

## Notes

- Does not verify the prior uptrend the pattern classically assumes for significance.

## Inputs

- `inPriceOHLC` — OHLC price series (open, high, low, close)

## Outputs

- `outInteger` — -100 on a detected pattern (always bearish), 0 otherwise; never emits +100

## Implementation

TA-Lib Definition: [`cdladvanceblock.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/cdladvanceblock/cdladvanceblock.c) · [`cdladvanceblock.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/cdladvanceblock/cdladvanceblock.yaml)

| Native | File |
|--------|------|
| C | [`ta_CDLADVANCEBLOCK.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_CDLADVANCEBLOCK.c) |
| Rust | [`cdladvanceblock.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/src/ta_func/cdladvanceblock.rs) |
| Java | [`Core_CDLADVANCEBLOCK.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/Core_CDLADVANCEBLOCK.java) |

TA-Lib is also available for Python, R and more using a [wrapper](https://ta-lib.org/wrappers/).

## Aliases

Advance Block

## See Also

[CDL3WHITESOLDIERS](/functions/cdl3whitesoldiers) · CDLDELIBERATION · [CDLSTALLEDPATTERN](/functions/cdlstalledpattern)
