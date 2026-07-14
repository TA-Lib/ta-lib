---
title: CDLHARAMICROSS
description: "A two-candle reversal pattern: a long real body followed by a doji whose real body is contained within the first candle's real body (the doji variant of the Harami). Bullish after a black first candle, bearish after a white first candle. A hit signals a potential reversal: +100/+80 bullish (black first candle), -100/-80 bearish (white first candle)."
---

# CDLHARAMICROSS

## Summary

A two-candle reversal pattern: a long real body followed by a doji whose real body is contained within the first candle's real body (the doji variant of the Harami). Bullish after a black first candle, bearish after a white first candle. A hit signals a potential reversal: +100/+80 bullish (black first candle), -100/-80 bearish (white first candle).

## Notes

- Does not verify the prior trend (downtrend for bullish, uptrend for bearish) that the reversal signal assumes.

## Inputs

- `inPriceOHLC` — Open, High, Low, Close price series

## Outputs

- `outInteger` — +100/+80 when the first candle is black (bullish), -100/-80 when the first candle is white (bearish), 0 otherwise. Magnitude 100 for strict containment inside the first body, 80 when one real-body end matches

## Implementation

TA-Lib Definition: [`cdlharamicross.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/cdlharamicross/cdlharamicross.c) · [`cdlharamicross.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/cdlharamicross/cdlharamicross.yaml)

| Native | File |
|--------|------|
| C | [`ta_CDLHARAMICROSS.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_CDLHARAMICROSS.c) |
| Rust | [`cdlharamicross.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/src/ta_func/cdlharamicross.rs) |
| Java | [`Core_CDLHARAMICROSS.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/Core_CDLHARAMICROSS.java) |

TA-Lib is also available for Python, R and more using a [wrapper](https://ta-lib.org/wrappers/).

## Aliases

Harami Cross

## See Also

[CDLHARAMI](/functions/cdlharami.md) · [CDLDOJI](/functions/cdldoji.md)
