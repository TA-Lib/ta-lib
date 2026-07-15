# CDLHANGINGMAN

## Summary

Single candle with a small real body, a long lower shadow, and little/no upper shadow, sitting at or near the highs of the prior candle. Bearish reversal signal. A hit is a bearish reversal signal (meaningful at the top of an uptrend).

## Notes

- Does not verify the preceding uptrend that the pattern classically assumes; confirm the trend context yourself.

## Inputs

- `inPriceOHLC` — Open, High, Low, Close price series

## Outputs

- `outInteger` — -100 when detected (always bearish), 0 otherwise. Never emits +100

## Implementation

TA-Lib Definition: [`cdlhangingman.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/cdlhangingman/cdlhangingman.c) · [`cdlhangingman.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/cdlhangingman/cdlhangingman.yaml)

| Native | File |
|--------|------|
| C | [`ta_CDLHANGINGMAN.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_CDLHANGINGMAN.c) |
| Rust | [`cdlhangingman.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/cdlhangingman.rs) |
| Java | [`Core_CDLHANGINGMAN.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/library/fragments/Core_CDLHANGINGMAN.java) |

TA-Lib is also available for Python, R and more using a [wrapper](https://ta-lib.org/wrappers/).

## Aliases

Hanging Man

## See Also

CDLHAMMER · CDLINVERTEDHAMMER · CDLSHOOTINGSTAR · CDLTAKURI
