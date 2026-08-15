# CDL3LINESTRIKE

## Summary

A four-candle pattern: three same-color candles with consecutively higher (or lower) closes, each opening within or near the prior real body, then a fourth opposite-color candle that opens beyond the third close and closes past the first candle's open. TA-Lib emits a signed continuation-style signal keyed to the color of the first three candles, traditionally read as significant only inside a trend matching those three candles.

## Notes

- Does not verify the surrounding trend the pattern classically assumes for significance.
- TA-Lib's sign follows the classic continuation reading. Thomas Bulkowski's statistical study of the pattern (*Encyclopedia of Candlestick Charts*) found the opposite in practice — it acted as a reversal far more often than a continuation — so traders who follow his research read this pattern's signal in the opposite direction from what its sign here suggests.

## Inputs

- `inOpen` — Open price of each bar
- `inHigh` — High price of each bar
- `inLow` — Low price of each bar
- `inClose` — Close price of each bar

## Outputs

- `outInteger` — +100 for a white (rising) three-line strike, -100 for a black (falling) three-line strike, 0 otherwise. Sign is the color of the first three candles: candlecolor(i-1)*100

## Output Values

| Value | Meaning |
|-------|---------|
| -100 | Bearish Three-Line Strike — three falling candles abruptly erased by one strong reversal bar; classically read as a bearish continuation despite the apparent reversal |
| 0 | No pattern |
| 100 | Bullish Three-Line Strike — three rising candles abruptly erased by one strong reversal bar; classically read as a bullish continuation despite the apparent reversal |

## Implementation

TA-Lib Definition: [`cdl3linestrike.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/cdl3linestrike/cdl3linestrike.c) · [`cdl3linestrike.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/cdl3linestrike/cdl3linestrike.yaml)

| Native | File |
|--------|------|
| C | [`ta_CDL3LINESTRIKE.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_CDL3LINESTRIKE.c) |
| Rust | [`cdl3linestrike.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/cdl3linestrike.rs) |
| Java | [`Core_CDL3LINESTRIKE.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/fragments/Core_CDL3LINESTRIKE.java) |

TA-Lib is also available for Python, R and more using a [wrapper](/install/#wrappers).

## Aliases

Three-Line Strike, 3-Line Strike

## See Also

CDL3WHITESOLDIERS · CDL3BLACKCROWS
