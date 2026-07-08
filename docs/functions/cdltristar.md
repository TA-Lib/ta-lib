---
title: CDLTRISTAR
description: "A three-candle pattern of three consecutive doji where the middle doji is a star (its body gaps away from the first). Bullish or bearish reversal signal. +100 = bullish reversal (middle doji gapped down), -100 = bearish reversal (middle doji gapped up)."
---

# CDLTRISTAR

## Summary

A three-candle pattern of three consecutive doji where the middle doji is a star (its body gaps away from the first). Bullish or bearish reversal signal. +100 = bullish reversal (middle doji gapped down), -100 = bearish reversal (middle doji gapped up).

## Notes

- This reversal pattern does not verify the prior trend it classically assumes.

## Inputs

- `inPriceOHLC` — Open, High, Low, Close price series

## Outputs

- `outInteger` — +100 (bullish, star gapped down), -100 (bearish, star gapped up), or 0 when no pattern. Both signs are emitted

## Implementation

TA-Lib Definition: [`cdltristar.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/cdltristar/cdltristar.c) · [`cdltristar.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/cdltristar/cdltristar.yaml)

| Native | File |
|--------|------|
| C | [`ta_CDLTRISTAR.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_CDLTRISTAR.c) |
| Rust | [`cdltristar.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/src/ta_func/cdltristar.rs) |
| Java | [`Core_CDLTRISTAR.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/Core_CDLTRISTAR.java) |

TA-Lib is also available for Python, R and more using a [wrapper](https://ta-lib.org/wrappers/).

## Aliases

Tristar Pattern, Tri-Star

## See Also

[CDLDOJI](cdldoji.md) · [CDLDOJISTAR](cdldojistar.md) · [CDLMORNINGDOJISTAR](cdlmorningdojistar.md) · [CDLEVENINGDOJISTAR](cdleveningdojistar.md)
