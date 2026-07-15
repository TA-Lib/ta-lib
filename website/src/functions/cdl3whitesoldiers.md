---
title: CDL3WHITESOLDIERS
description: "A three-candle pattern of consecutive white candles with progressively higher closes, each opening within/near the prior body and each with a very short upper shadow. It is a bullish reversal signal. A hit (+100) is bullish, signaling a reversal (most meaningful in a downtrend, which the code does not verify)."
---

# CDL3WHITESOLDIERS

## Summary

A three-candle pattern of consecutive white candles with progressively higher closes, each opening within/near the prior body and each with a very short upper shadow. It is a bullish reversal signal. A hit (+100) is bullish, signaling a reversal (most meaningful in a downtrend, which the code does not verify).

## Notes

- Does not verify the prior downtrend the pattern classically assumes for significance.

## Inputs

- `inPriceOHLC` — OHLC price series (open, high, low, close)

## Outputs

- `outInteger` — +100 when the pattern is detected, 0 otherwise; never negative (three white soldiers is always bullish)

## Implementation

TA-Lib Definition: [`cdl3whitesoldiers.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/cdl3whitesoldiers/cdl3whitesoldiers.c) · [`cdl3whitesoldiers.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/cdl3whitesoldiers/cdl3whitesoldiers.yaml)

| Native | File |
|--------|------|
| C | [`ta_CDL3WHITESOLDIERS.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_CDL3WHITESOLDIERS.c) |
| Rust | [`cdl3whitesoldiers.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/cdl3whitesoldiers.rs) |
| Java | [`Core_CDL3WHITESOLDIERS.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/library/fragments/Core_CDL3WHITESOLDIERS.java) |

TA-Lib is also available for Python, R and more using a [wrapper](https://ta-lib.org/wrappers/).

## Aliases

Three Advancing White Soldiers, Three White Soldiers

## See Also

[CDL3BLACKCROWS](/functions/cdl3blackcrows) · [CDLADVANCEBLOCK](/functions/cdladvanceblock) · [CDLIDENTICAL3CROWS](/functions/cdlidentical3crows)
