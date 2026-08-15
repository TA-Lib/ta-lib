# CDLLADDERBOTTOM

## Summary

Five-candle bullish reversal pattern: three consecutively lower black candles, a fourth black candle with a non-very-short upper shadow, then a white candle that opens above the prior open and closes above the prior high. A hit is a bullish reversal signal, most meaningful after a downtrend.

## Notes

- Does not verify the preceding downtrend that this bullish reversal classically assumes.
- Bulkowski's testing found this reverses a downtrend only 56% of the time — "near random" — and it is extremely rare (451 occurrences out of 4.7 million candle lines), ranking 41st of 103 patterns for overall performance. ([thepatternsite.com](https://thepatternsite.com/LadderBottom.html))

## Inputs

- `inOpen` — Open price of each bar
- `inHigh` — High price of each bar
- `inLow` — Low price of each bar
- `inClose` — Close price of each bar

## Outputs

- `outInteger` — +100 on a detected ladder bottom, 0 otherwise. Only ever emits +100 (never -100); inherently bullish

## Output Values

| Value | Meaning |
|-------|---------|
| 0 | No pattern |
| 100 | Ladder Bottom detected — a bullish reversal signal, most meaningful after a downtrend (unverified by the function) |

## Implementation

TA-Lib Definition: [`cdlladderbottom.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/cdlladderbottom/cdlladderbottom.c) · [`cdlladderbottom.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/cdlladderbottom/cdlladderbottom.yaml)

| Native | File |
|--------|------|
| C | [`ta_CDLLADDERBOTTOM.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_CDLLADDERBOTTOM.c) |
| Rust | [`cdlladderbottom.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/cdlladderbottom.rs) |
| Java | [`Core_CDLLADDERBOTTOM.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/fragments/Core_CDLLADDERBOTTOM.java) |

TA-Lib is also available for Python, R and more using a [wrapper](/install/#wrappers).

## Aliases

Ladder Bottom

## See Also

CDL3BLACKCROWS · CDLMATCHINGLOW · CDLBREAKAWAY
