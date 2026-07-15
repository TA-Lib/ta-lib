# CDLHIKKAKEMOD

## Summary

A four-candle pattern: two successively narrower inside bars, then a breakout bar, with the second candle closing near one extreme of its range. Bullish or bearish reversal signal. Bullish (+) or bearish (-) reversal; per the code's note it is significant in a downtrend (bull) or uptrend (bear), context the code does not verify.

## Notes

- Does not verify the prior trend (downtrend for bullish, uptrend for bearish) that this reversal pattern assumes.

## Inputs

- `inPriceOHLC` — OHLC price series (open, high, low, close)

## Outputs

- `outInteger` — +100 bullish hikkake bar, -100 bearish; +200 confirmed bullish, -200 confirmed bearish (confirmation adds another +/-100); 0 otherwise

## Implementation

TA-Lib Definition: [`cdlhikkakemod.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/cdlhikkakemod/cdlhikkakemod.c) · [`cdlhikkakemod.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/cdlhikkakemod/cdlhikkakemod.yaml)

| Native | File |
|--------|------|
| C | [`ta_CDLHIKKAKEMOD.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_CDLHIKKAKEMOD.c) |
| Rust | [`cdlhikkakemod.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/cdlhikkakemod.rs) |
| Java | [`Core_CDLHIKKAKEMOD.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/library/fragments/Core_CDLHIKKAKEMOD.java) |

TA-Lib is also available for Python, R and more using a [wrapper](https://ta-lib.org/wrappers/).

## Aliases

Modified Hikkake, Modified Hikkake Pattern

## See Also

CDLHIKKAKE
