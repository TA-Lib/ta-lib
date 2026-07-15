---
title: CDLHARAMI
description: "Two-candle pattern: a long real body followed by a short real body contained within the first candle's real body. A reversal signal whose direction is the opposite of the first candle's color. A hit signals a potential reversal opposite the 1st candle: positive (black 1st) is bullish, negative (white 1st) is bearish."
---

# CDLHARAMI

## Summary

Two-candle pattern: a long real body followed by a short real body contained within the first candle's real body. A reversal signal whose direction is the opposite of the first candle's color. A hit signals a potential reversal opposite the 1st candle: positive (black 1st) is bullish, negative (white 1st) is bearish.

## Notes

- Does not verify the prior trend (downtrend for bullish, uptrend for bearish) that the reversal signal assumes.

## Inputs

- `inPriceOHLC` — OHLC price series (open, high, low, close)

## Outputs

- `outInteger` — +100/+80 when the long 1st candle is black (bullish), -100/-80 when it is white (bearish), 0 otherwise; 80 when the two real bodies share an end, 100 when the 1st body strictly overhangs both ends of the 2nd

## Implementation

TA-Lib Definition: [`cdlharami.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/cdlharami/cdlharami.c) · [`cdlharami.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/cdlharami/cdlharami.yaml)

| Native | File |
|--------|------|
| C | [`ta_CDLHARAMI.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_CDLHARAMI.c) |
| Rust | [`cdlharami.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/cdlharami.rs) |
| Java | [`Core_CDLHARAMI.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/library/fragments/Core_CDLHARAMI.java) |

TA-Lib is also available for Python, R and more using a [wrapper](https://ta-lib.org/wrappers/).

## Aliases

Harami, Harami Pattern

## See Also

[CDLHARAMICROSS](/functions/cdlharamicross) · [CDLENGULFING](/functions/cdlengulfing)
