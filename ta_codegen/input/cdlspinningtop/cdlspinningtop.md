# CDLSPINNINGTOP

## Summary

Single-candle pattern: a small real body with both an upper and a lower shadow longer than the body. Signals indecision; the code does not classify it as bullish or bearish.

## Formula

One candle where: upper shadow > real body AND lower shadow > real body AND real body < the BodyShort average. The BodyShort average is the factor-scaled mean body over the prior avgPeriod candles.

## Inputs

- `inOpen` — Open price of each bar
- `inHigh` — High price of each bar
- `inLow` — Low price of each bar
- `inClose` — Close price of each bar

## Outputs

- `outInteger` — +100 when the candle is white (close>=open), -100 when black (close<open), 0 when no pattern. Sign is candle color, NOT bullish/bearish

## Output Values

| Value | Meaning |
|-------|---------|
| -100 | Matching black spinning top (indecision; color only — not a bearish call) |
| 0 | No pattern |
| 100 | Matching white spinning top (indecision; color only — not a bullish call) |

## Implementation

TA-Lib Definition: [`cdlspinningtop.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/cdlspinningtop/cdlspinningtop.c) · [`cdlspinningtop.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/cdlspinningtop/cdlspinningtop.yaml)

| Native | File |
|--------|------|
| C | [`ta_CDLSPINNINGTOP.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_CDLSPINNINGTOP.c) |
| Rust | [`cdlspinningtop.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/cdlspinningtop.rs) |
| Java | [`Core_CDLSPINNINGTOP.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/fragments/Core_CDLSPINNINGTOP.java) |

TA-Lib is also available for Python, R and more using a [wrapper](/install/#wrappers).

## Aliases

Spinning Top

## See Also

CDLDOJI · CDLHIGHWAVE · CDLLONGLEGGEDDOJI
