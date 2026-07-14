---
title: CDLRICKSHAWMAN
description: "Single-candle doji with two long shadows whose body sits near the midpoint of the high-low range. It is a neutral indecision signal, not a directional (bullish/bearish) reversal. A hit marks market indecision/uncertainty; neutral, neither bullish nor bearish."
---

# CDLRICKSHAWMAN

## Summary

Single-candle doji with two long shadows whose body sits near the midpoint of the high-low range. It is a neutral indecision signal, not a directional (bullish/bearish) reversal. A hit marks market indecision/uncertainty; neutral, neither bullish nor bearish.

## Inputs

- `inPriceOHLC` — OHLC price series (open, high, low, close)

## Outputs

- `outInteger` — +100 when the pattern is present, 0 otherwise. Never -100; the code notes the positive value does NOT imply bullish, it signals uncertainty

## Implementation

TA-Lib Definition: [`cdlrickshawman.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/cdlrickshawman/cdlrickshawman.c) · [`cdlrickshawman.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/cdlrickshawman/cdlrickshawman.yaml)

| Native | File |
|--------|------|
| C | [`ta_CDLRICKSHAWMAN.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_CDLRICKSHAWMAN.c) |
| Rust | [`cdlrickshawman.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/src/ta_func/cdlrickshawman.rs) |
| Java | [`Core_CDLRICKSHAWMAN.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/Core_CDLRICKSHAWMAN.java) |

TA-Lib is also available for Python, R and more using a [wrapper](https://ta-lib.org/wrappers/).

## Aliases

Rickshaw Man

## See Also

[CDLLONGLEGGEDDOJI](/functions/cdllongleggeddoji) · [CDLDOJI](/functions/cdldoji) · [CDLHIGHWAVE](/functions/cdlhighwave)
