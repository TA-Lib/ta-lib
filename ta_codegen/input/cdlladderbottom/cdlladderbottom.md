# CDLLADDERBOTTOM

## Summary

Five-candle bullish reversal pattern: three consecutively lower black candles, a fourth black candle with a non-very-short upper shadow, then a white candle that opens above the prior open and closes above the prior high. Signals a potential bottom reversal. A hit (+100) is a bullish reversal signal, most meaningful after a downtrend.

## Notes

- Does not verify the preceding downtrend that this bullish reversal classically assumes.

## Inputs

- `inPriceOHLC` — OHLC price series (open, high, low, close)

## Outputs

- `outInteger` — +100 on a detected ladder bottom, 0 otherwise. Only ever emits +100 (never -100); inherently bullish

## Implementation

TA-Lib Definition: [`cdlladderbottom.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/cdlladderbottom/cdlladderbottom.c) · [`cdlladderbottom.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/cdlladderbottom/cdlladderbottom.yaml)

| Native | File |
|--------|------|
| C | [`ta_CDLLADDERBOTTOM.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_CDLLADDERBOTTOM.c) |
| Rust | [`cdlladderbottom.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/src/ta_func/cdlladderbottom.rs) |
| Java | [`Core_CDLLADDERBOTTOM.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/Core_CDLLADDERBOTTOM.java) |

TA-Lib is also available for Python, R and more using a [wrapper](https://ta-lib.org/wrappers/).

## Aliases

Ladder Bottom

## See Also

CDL3BLACKCROWS · CDLMATCHINGLOW · CDLBREAKAWAY
