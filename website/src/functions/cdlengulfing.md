---
title: CDLENGULFING
description: "A two-candle reversal pattern where the second candle's real body engulfs the first candle's opposite-colored real body. Bullish (white engulfs black) or bearish (black engulfs white) reversal signal. Bullish reversal at +100/+80, bearish at -100/-80; ideally after a downtrend (bullish) or uptrend (bearish), which the code does not verify."
---

# CDLENGULFING

## Summary

A two-candle reversal pattern where the second candle's real body engulfs the first candle's opposite-colored real body. Bullish (white engulfs black) or bearish (black engulfs white) reversal signal. Bullish reversal at +100/+80, bearish at -100/-80; ideally after a downtrend (bullish) or uptrend (bearish), which the code does not verify.

## Notes

- Does not verify the prior trend (down for bullish, up for bearish) the reversal classically assumes.

## Inputs

- `inPriceOHLC` — OHLC price bars (open, high, low, close)

## Outputs

- `outInteger` — +100/+80 (bullish, white engulfs black), -100/-80 (bearish, black engulfs white), 0 otherwise. Magnitude 100 when the second body strictly engulfs both ends; 80 when the bodies share an exact endpoint (open[i]==close[i-1] or close[i]==open[i-1])

## Implementation

TA-Lib Definition: [`cdlengulfing.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/cdlengulfing/cdlengulfing.c) · [`cdlengulfing.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/cdlengulfing/cdlengulfing.yaml)

| Native | File |
|--------|------|
| C | [`ta_CDLENGULFING.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_CDLENGULFING.c) |
| Rust | [`cdlengulfing.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/src/ta_func/cdlengulfing.rs) |
| Java | [`Core_CDLENGULFING.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/Core_CDLENGULFING.java) |

TA-Lib is also available for Python, R and more using a [wrapper](https://ta-lib.org/wrappers/).

## Aliases

Engulfing Pattern, Engulfing, Bullish/Bearish Engulfing

## See Also

[CDLHARAMI](/functions/cdlharami) · [CDLCOUNTERATTACK](/functions/cdlcounterattack) · [CDLHARAMICROSS](/functions/cdlharamicross)
