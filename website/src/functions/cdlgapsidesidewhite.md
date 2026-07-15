---
title: CDLGAPSIDESIDEWHITE
description: "A three-candle pattern: a first candle followed by two white candles of similar body size that both gap the same direction (up or down) from the first candle's real body and open at about the same level. It is a continuation signal whose sign reports the gap direction; the code does not verify a prior trend. A hit signals continuation in the gap's direction: +100 with an upside gap is bullish, -100 with a downside gap is bearish."
---

# CDLGAPSIDESIDEWHITE

## Summary

A three-candle pattern: a first candle followed by two white candles of similar body size that both gap the same direction (up or down) from the first candle's real body and open at about the same level. It is a continuation signal whose sign reports the gap direction; the code does not verify a prior trend. A hit signals continuation in the gap's direction: +100 with an upside gap is bullish, -100 with a downside gap is bearish.

## Notes

- Does not verify the prior trend the continuation signal classically assumes.

## Inputs

- `inPriceOHLC` — Open, High, Low, Close price series

## Outputs

- `outInteger` — +100 for an up-gap (bullish continuation), -100 for a down-gap (bearish continuation), 0 when no pattern. Sign is set solely by the C2-vs-C1 gap direction (realbodygapup ? 100 : -100)

## Implementation

TA-Lib Definition: [`cdlgapsidesidewhite.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/cdlgapsidesidewhite/cdlgapsidesidewhite.c) · [`cdlgapsidesidewhite.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/cdlgapsidesidewhite/cdlgapsidesidewhite.yaml)

| Native | File |
|--------|------|
| C | [`ta_CDLGAPSIDESIDEWHITE.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_CDLGAPSIDESIDEWHITE.c) |
| Rust | [`cdlgapsidesidewhite.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/cdlgapsidesidewhite.rs) |
| Java | [`Core_CDLGAPSIDESIDEWHITE.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/library/fragments/Core_CDLGAPSIDESIDEWHITE.java) |

TA-Lib is also available for Python, R and more using a [wrapper](https://ta-lib.org/wrappers/).

## Aliases

Up/Down-gap side-by-side white lines, Gapping side-by-side white lines

## See Also

[CDLTASUKIGAP](/functions/cdltasukigap) · [CDLXSIDEGAP3METHODS](/functions/cdlxsidegap3methods)
