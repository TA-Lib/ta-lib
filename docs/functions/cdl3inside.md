---
title: CDL3INSIDE
description: "A three-candle reversal pattern: a long real body, then a short real body totally engulfed by it (a harami), then a third candle of opposite color to the first that closes past the first candle's open. Signals a bullish (three inside up) or bearish (three inside down) reversal. A hit is a reversal signal: +100 = three inside up (bullish, significant in a downtrend); -100 = three inside down (bearish, significant in an uptrend)."
---

# CDL3INSIDE

## Summary

A three-candle reversal pattern: a long real body, then a short real body totally engulfed by it (a harami), then a third candle of opposite color to the first that closes past the first candle's open. Signals a bullish (three inside up) or bearish (three inside down) reversal. A hit is a reversal signal: +100 = three inside up (bullish, significant in a downtrend); -100 = three inside down (bearish, significant in an uptrend).

## Notes

- Does not verify the prior trend the pattern classically assumes (three inside up is meaningful in a downtrend, three inside down in an uptrend).

## Inputs

- `inPriceOHLC` — Open, High, Low, Close price series

## Outputs

- `outInteger` — +100 for three inside up (bullish reversal, first candle black), -100 for three inside down (bearish reversal, first candle white), 0 when no pattern. Computed as -candlecolor(1st)*100

## Implementation

TA-Lib Definition: [`cdl3inside.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/cdl3inside/cdl3inside.c) · [`cdl3inside.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/cdl3inside/cdl3inside.yaml)

| Native | File |
|--------|------|
| C | [`ta_CDL3INSIDE.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_CDL3INSIDE.c) |
| Rust | [`cdl3inside.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/src/ta_func/cdl3inside.rs) |
| Java | [`Core_CDL3INSIDE.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/Core_CDL3INSIDE.java) |

TA-Lib is also available for Python, R and more using a [wrapper](https://ta-lib.org/wrappers/).

## Aliases

Three Inside Up/Down, Three Inside, Three Inside Up, Three Inside Down

## See Also

[CDLHARAMI](/functions/cdlharami.md) · [CDL3OUTSIDE](/functions/cdl3outside.md) · [CDLENGULFING](/functions/cdlengulfing.md)
