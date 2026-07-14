---
title: CDLKICKINGBYLENGTH
description: "A two-candle pattern of two opposite-color marubozu (long body, very short shadows on both ends) separated by a gap. A strong directional/reversal signal whose bull/bear bias is set by the longer of the two marubozu. A hit signals a strong directional move; +100 bullish / -100 bearish per the color of the longer marubozu."
---

# CDLKICKINGBYLENGTH

## Summary

A two-candle pattern of two opposite-color marubozu (long body, very short shadows on both ends) separated by a gap. A strong directional/reversal signal whose bull/bear bias is set by the longer of the two marubozu. A hit signals a strong directional move; +100 bullish / -100 bearish per the color of the longer marubozu.

## Inputs

- `inPriceOHLC` — Open, High, Low, Close price series

## Outputs

- `outInteger` — +100 or -100 on a hit, 0 otherwise. Sign = candlecolor of the candle with the larger realbody (i if realbody(i) > realbody(i-1), else i-1; tie goes to i-1): +100 if that marubozu is white, -100 if black

## Implementation

TA-Lib Definition: [`cdlkickingbylength.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/cdlkickingbylength/cdlkickingbylength.c) · [`cdlkickingbylength.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/cdlkickingbylength/cdlkickingbylength.yaml)

| Native | File |
|--------|------|
| C | [`ta_CDLKICKINGBYLENGTH.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_CDLKICKINGBYLENGTH.c) |
| Rust | [`cdlkickingbylength.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/src/ta_func/cdlkickingbylength.rs) |
| Java | [`Core_CDLKICKINGBYLENGTH.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/Core_CDLKICKINGBYLENGTH.java) |

TA-Lib is also available for Python, R and more using a [wrapper](https://ta-lib.org/wrappers/).

## Aliases

Kicking by Length, Kicking - bull/bear decided by the longer marubozu

## See Also

[CDLKICKING](/functions/cdlkicking.md) · [CDLMARUBOZU](/functions/cdlmarubozu.md) · [CDLGAPSIDESIDEWHITE](/functions/cdlgapsidesidewhite.md)
