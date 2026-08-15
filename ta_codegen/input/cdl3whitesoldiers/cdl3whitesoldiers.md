# CDL3WHITESOLDIERS

## Summary

A three-candle pattern of consecutive white candles with progressively higher closes, each opening within/near the prior body and each with a very short upper shadow. A hit is a bullish reversal signal, most meaningful in a downtrend, which the code does not verify.

## Notes

- Does not verify the prior downtrend the pattern classically assumes for significance.
- Bulkowski's testing found this reverses a downtrend 82% of the time, but cautions the high rate mostly reflects how rare downward breakouts are afterward — moves following an upward breakout perform poorly. ([thepatternsite.com](https://thepatternsite.com/ThreeWhiteSoldiers.html))

## Inputs

- `inOpen` — Open price of each bar
- `inHigh` — High price of each bar
- `inLow` — Low price of each bar
- `inClose` — Close price of each bar

## Outputs

- `outInteger` — +100 when the pattern is detected, 0 otherwise; never negative (three white soldiers is always bullish)

## Output Values

| Value | Meaning |
|-------|---------|
| 0 | No pattern |
| 100 | Three White Soldiers detected — a bullish reversal signal, most meaningful after a downtrend (unverified by the function) |

## Implementation

TA-Lib Definition: [`cdl3whitesoldiers.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/cdl3whitesoldiers/cdl3whitesoldiers.c) · [`cdl3whitesoldiers.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/cdl3whitesoldiers/cdl3whitesoldiers.yaml)

| Native | File |
|--------|------|
| C | [`ta_CDL3WHITESOLDIERS.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_CDL3WHITESOLDIERS.c) |
| Rust | [`cdl3whitesoldiers.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/cdl3whitesoldiers.rs) |
| Java | [`Core_CDL3WHITESOLDIERS.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/fragments/Core_CDL3WHITESOLDIERS.java) |

TA-Lib is also available for Python, R and more using a [wrapper](/install/#wrappers).

## Aliases

Three Advancing White Soldiers, Three White Soldiers

## See Also

CDL3BLACKCROWS · CDLADVANCEBLOCK · CDLIDENTICAL3CROWS
