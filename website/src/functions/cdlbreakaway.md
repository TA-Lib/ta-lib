---
title: CDLBREAKAWAY
description: "A five-candle reversal pattern: a long first candle, a same-colored second candle that gaps away from it by its real body, two more candles extending the move, and an opposite-colored fifth candle that closes back inside the gap. Emits a bullish signal (bottom reversal) or bearish signal (top reversal). A hit signals a reversal: +100 bullish (bottom), -100 bearish (top)."
---

# CDLBREAKAWAY

## Summary

A five-candle reversal pattern: a long first candle, a same-colored second candle that gaps away from it by its real body, two more candles extending the move, and an opposite-colored fifth candle that closes back inside the gap. Emits a bullish signal (bottom reversal) or bearish signal (top reversal). A hit signals a reversal: +100 bullish (bottom), -100 bearish (top).

## Notes

- Does not verify the prior trend the pattern classically assumes (a breakaway matters most against a preceding move).

## Inputs

- `inPriceOHLC` — Open, High, Low, Close price series

## Outputs

- `outInteger` — +100 when the fifth candle is white (bullish breakaway), -100 when it is black (bearish breakaway), 0 otherwise

## Implementation

TA-Lib Definition: [`cdlbreakaway.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/cdlbreakaway/cdlbreakaway.c) · [`cdlbreakaway.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/cdlbreakaway/cdlbreakaway.yaml)

| Native | File |
|--------|------|
| C | [`ta_CDLBREAKAWAY.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_CDLBREAKAWAY.c) |
| Rust | [`cdlbreakaway.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/cdlbreakaway.rs) |
| Java | [`Core_CDLBREAKAWAY.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/library/fragments/Core_CDLBREAKAWAY.java) |

TA-Lib is also available for Python, R and more using a [wrapper](https://ta-lib.org/wrappers/).

## Aliases

Breakaway

## See Also

[CDLGAPSIDESIDEWHITE](/functions/cdlgapsidesidewhite) · [CDLRISEFALL3METHODS](/functions/cdlrisefall3methods) · [CDL3LINESTRIKE](/functions/cdl3linestrike)
