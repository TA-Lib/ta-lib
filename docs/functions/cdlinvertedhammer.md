---
title: CDLINVERTEDHAMMER
description: "Single-candle pattern: a small real body with a long upper shadow and little-to-no lower shadow that gaps down from the prior candle. Bullish reversal signal. A hit (+100) flags a potential bullish reversal."
---

# CDLINVERTEDHAMMER

## Summary

Single-candle pattern: a small real body with a long upper shadow and little-to-no lower shadow that gaps down from the prior candle. Bullish reversal signal. A hit (+100) flags a potential bullish reversal.

## Notes

- Does not verify the preceding downtrend that the pattern classically assumes; it only checks the gap down from the immediately preceding candle.

## Inputs

- `inPriceOHLC` — Open, High, Low, Close price series

## Outputs

- `outInteger` — +100 when the inverted hammer is detected, 0 otherwise. Never emits -100; the pattern is always bullish

## Implementation

TA-Lib Definition: [`cdlinvertedhammer.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/cdlinvertedhammer/cdlinvertedhammer.c) · [`cdlinvertedhammer.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/cdlinvertedhammer/cdlinvertedhammer.yaml)

| Native | File |
|--------|------|
| C | [`ta_CDLINVERTEDHAMMER.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_CDLINVERTEDHAMMER.c) |
| Rust | [`cdlinvertedhammer.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/src/ta_func/cdlinvertedhammer.rs) |
| Java | [`Core_CDLINVERTEDHAMMER.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/Core_CDLINVERTEDHAMMER.java) |

TA-Lib is also available for Python, R and more using a [wrapper](https://ta-lib.org/wrappers/).

## Aliases

Inverted Hammer

## See Also

[CDLHAMMER](/functions/cdlhammer.md) · [CDLSHOOTINGSTAR](/functions/cdlshootingstar.md) · [CDLHANGINGMAN](/functions/cdlhangingman.md)
