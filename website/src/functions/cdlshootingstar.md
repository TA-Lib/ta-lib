---
title: CDLSHOOTINGSTAR
description: "Single-candle pattern: a small real body with a long upper shadow and little-to-no lower shadow that gaps up from the prior candle's real body. Bearish reversal signal. A hit (-100) flags a bearish reversal at the top of an uptrend."
---

# CDLSHOOTINGSTAR

## Summary

Single-candle pattern: a small real body with a long upper shadow and little-to-no lower shadow that gaps up from the prior candle's real body. Bearish reversal signal. A hit (-100) flags a bearish reversal at the top of an uptrend.

## Notes

- A preceding uptrend is not verified.

## Inputs

- `inPriceOHLC` — Open, High, Low, Close price series

## Outputs

- `outInteger` — -100 when the shooting star is detected, 0 otherwise. Only ever emits negative (bearish); never +100

## Implementation

TA-Lib Definition: [`cdlshootingstar.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/cdlshootingstar/cdlshootingstar.c) · [`cdlshootingstar.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/cdlshootingstar/cdlshootingstar.yaml)

| Native | File |
|--------|------|
| C | [`ta_CDLSHOOTINGSTAR.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_CDLSHOOTINGSTAR.c) |
| Rust | [`cdlshootingstar.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/cdlshootingstar.rs) |
| Java | [`Core_CDLSHOOTINGSTAR.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/library/fragments/Core_CDLSHOOTINGSTAR.java) |

TA-Lib is also available for Python, R and more using a [wrapper](https://ta-lib.org/wrappers/).

## Aliases

Shooting Star

## See Also

[CDLINVERTEDHAMMER](/functions/cdlinvertedhammer) · [CDLHANGINGMAN](/functions/cdlhangingman) · [CDLHAMMER](/functions/cdlhammer) · [CDLGRAVESTONEDOJI](/functions/cdlgravestonedoji)
