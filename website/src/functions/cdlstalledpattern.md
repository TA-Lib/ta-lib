---
title: CDLSTALLEDPATTERN
description: "A three-candle pattern of three white candles with consecutively higher closes where the third loses momentum (a small body riding on the shoulder of the second's long body). It is a bearish reversal signal of a stalling advance. A hit (-100) is bearish: the uptrend is stalling and may reverse."
---

# CDLSTALLEDPATTERN

## Summary

A three-candle pattern of three white candles with consecutively higher closes where the third loses momentum (a small body riding on the shoulder of the second's long body). It is a bearish reversal signal of a stalling advance. A hit (-100) is bearish: the uptrend is stalling and may reverse.

## Notes

- The pattern classically appears in an uptrend, but this function does not verify a prior uptrend; the caller must confirm it.

## Inputs

- `inPriceOHLC` — OHLC price series (open, high, low, close)

## Outputs

- `outInteger` — -100 when the pattern is detected (always bearish), 0 otherwise. Never emits +100

## Implementation

TA-Lib Definition: [`cdlstalledpattern.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/cdlstalledpattern/cdlstalledpattern.c) · [`cdlstalledpattern.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/cdlstalledpattern/cdlstalledpattern.yaml)

| Native | File |
|--------|------|
| C | [`ta_CDLSTALLEDPATTERN.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_CDLSTALLEDPATTERN.c) |
| Rust | [`cdlstalledpattern.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/cdlstalledpattern.rs) |
| Java | [`Core_CDLSTALLEDPATTERN.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/library/fragments/Core_CDLSTALLEDPATTERN.java) |

TA-Lib is also available for Python, R and more using a [wrapper](https://ta-lib.org/wrappers/).

## Aliases

Stalled Pattern, Deliberation Pattern

## See Also

[CDLADVANCEBLOCK](/functions/cdladvanceblock) · [CDL3WHITESOLDIERS](/functions/cdl3whitesoldiers) · [CDLXSIDEGAP3METHODS](/functions/cdlxsidegap3methods)
