# CDLHAMMER

## Summary

Single-candle pattern: a small real body at the top of the range with a long lower shadow and little or no upper shadow, sitting at or near the prior candle's low. Bullish reversal signal. A hit (+100) flags a potential bullish reversal.

## Notes

- Does not verify the preceding downtrend that the pattern classically assumes; confirm the trend context yourself.

## Inputs

- `inPriceOHLC` — Open, High, Low, Close price series

## Outputs

- `outInteger` — +100 when the hammer is detected, 0 otherwise. Bullish only; never emits -100

## Implementation

TA-Lib Definition: [`cdlhammer.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/cdlhammer/cdlhammer.c) · [`cdlhammer.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/cdlhammer/cdlhammer.yaml)

| Native | File |
|--------|------|
| C | [`ta_CDLHAMMER.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_CDLHAMMER.c) |
| Rust | [`cdlhammer.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/cdlhammer.rs) |
| Java | [`Core_CDLHAMMER.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/library/fragments/Core_CDLHAMMER.java) |

TA-Lib is also available for Python, R and more using a [wrapper](https://ta-lib.org/wrappers/).

## Aliases

Hammer

## See Also

CDLINVERTEDHAMMER · CDLHANGINGMAN · CDLTAKURI
