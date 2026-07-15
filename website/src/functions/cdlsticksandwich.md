---
title: CDLSTICKSANDWICH
description: "A three-candle bullish reversal pattern: two black candles (1st and 3rd) sandwiching a white candle, where the 3rd black candle closes at the same level as the 1st (the \"bread\"). A hit signals a bullish reversal (code comment notes it is significant in a downtrend, which the function does not verify)."
---

# CDLSTICKSANDWICH

## Summary

A three-candle bullish reversal pattern: two black candles (1st and 3rd) sandwiching a white candle, where the 3rd black candle closes at the same level as the 1st (the "bread"). A hit signals a bullish reversal (code comment notes it is significant in a downtrend, which the function does not verify).

## Inputs

- `inPriceOHLC` — OHLC price series (open, high, low, close)

## Outputs

- `outInteger` — +100 when the pattern is present, 0 otherwise. Never -100 — Stick Sandwich is always bullish

## Implementation

TA-Lib Definition: [`cdlsticksandwich.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/cdlsticksandwich/cdlsticksandwich.c) · [`cdlsticksandwich.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/cdlsticksandwich/cdlsticksandwich.yaml)

| Native | File |
|--------|------|
| C | [`ta_CDLSTICKSANDWICH.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_CDLSTICKSANDWICH.c) |
| Rust | [`cdlsticksandwich.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/cdlsticksandwich.rs) |
| Java | [`Core_CDLSTICKSANDWICH.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/library/fragments/Core_CDLSTICKSANDWICH.java) |

TA-Lib is also available for Python, R and more using a [wrapper](https://ta-lib.org/wrappers/).

## Aliases

Stick Sandwich

## See Also

[CDLMATCHINGLOW](/functions/cdlmatchinglow) · [CDLHOMINGPIGEON](/functions/cdlhomingpigeon)
