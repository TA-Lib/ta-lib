---
title: CDLXSIDEGAP3METHODS
description: "A three-candle continuation pattern: two same-color candles separated by a real-body gap, followed by an opposite-color candle that fills into the gap. Bullish (upside) when the first two candles are white, bearish (downside) when they are black. A hit signals trend continuation: +100 bullish (uptrend resumes), -100 bearish (downtrend resumes)."
---

# CDLXSIDEGAP3METHODS

## Summary

A three-candle continuation pattern: two same-color candles separated by a real-body gap, followed by an opposite-color candle that fills into the gap. Bullish (upside) when the first two candles are white, bearish (downside) when they are black. A hit signals trend continuation: +100 bullish (uptrend resumes), -100 bearish (downtrend resumes).

## Notes

- This continuation pattern does not verify the prior trend it classically assumes; the caller must confirm the trend.

## Inputs

- `inPriceOHLC` — Open, High, Low, Close price series

## Outputs

- `outInteger` — +100 when the two same-color candles are white (bullish/upside continuation), -100 when black (bearish/downside continuation), 0 otherwise. Equals candlecolor(1st candle) * 100

## Implementation

TA-Lib Definition: [`cdlxsidegap3methods.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/cdlxsidegap3methods/cdlxsidegap3methods.c) · [`cdlxsidegap3methods.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/cdlxsidegap3methods/cdlxsidegap3methods.yaml)

| Native | File |
|--------|------|
| C | [`ta_CDLXSIDEGAP3METHODS.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_CDLXSIDEGAP3METHODS.c) |
| Rust | [`cdlxsidegap3methods.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/cdlxsidegap3methods.rs) |
| Java | [`Core_CDLXSIDEGAP3METHODS.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/library/fragments/Core_CDLXSIDEGAP3METHODS.java) |

TA-Lib is also available for Python, R and more using a [wrapper](https://ta-lib.org/wrappers/).

## Aliases

Upside/Downside Gap Three Methods, Upside Gap Three Methods, Downside Gap Three Methods

## See Also

[CDLGAPSIDESIDEWHITE](/functions/cdlgapsidesidewhite) · [CDLTASUKIGAP](/functions/cdltasukigap) · [CDLRISEFALL3METHODS](/functions/cdlrisefall3methods)
