# CDLSPINNINGTOP

## Summary

Single-candle pattern: a small real body with both an upper and a lower shadow longer than the body. Signals indecision; the code does not classify it as bullish or bearish. A hit marks indecision (small body, both shadows long); the sign only reports candle color, not direction.

## Formula

One candle where: upper shadow > real body AND lower shadow > real body AND real body < the BodyShort average. The BodyShort average is the factor-scaled mean body over the prior avgPeriod candles.

## Inputs

- `inPriceOHLC` — Open, High, Low, Close price series

## Outputs

- `outInteger` — +100 when the candle is white (close>=open), -100 when black (close<open), 0 when no pattern. Sign is candle color, NOT bullish/bearish

## Implementation

TA-Lib Definition: [`cdlspinningtop.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/cdlspinningtop/cdlspinningtop.c) · [`cdlspinningtop.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/cdlspinningtop/cdlspinningtop.yaml)

| Native | File |
|--------|------|
| C | [`ta_CDLSPINNINGTOP.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_CDLSPINNINGTOP.c) |
| Rust | [`cdlspinningtop.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/cdlspinningtop.rs) |
| Java | [`Core_CDLSPINNINGTOP.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/library/fragments/Core_CDLSPINNINGTOP.java) |

TA-Lib is also available for Python, R and more using a [wrapper](https://ta-lib.org/wrappers/).

## Aliases

Spinning Top

## See Also

CDLDOJI · CDLHIGHWAVE · CDLLONGLEGGEDDOJI
