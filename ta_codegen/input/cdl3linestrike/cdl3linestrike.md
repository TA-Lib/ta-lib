# CDL3LINESTRIKE

## Summary

A four-candle pattern: three same-color candles with consecutively higher (or lower) closes, each opening within or near the prior real body, then a fourth opposite-color candle that opens beyond the third close and closes past the first candle's open. TA-Lib emits a signed continuation-style signal keyed to the color of the first three candles. +100 = three-white (bullish) strike, -100 = three-black (bearish) strike; traditionally read as significant only inside a trend matching the first three candles.

## Notes

- Does not verify the surrounding trend the pattern classically assumes for significance.

## Inputs

- `inPriceOHLC` — OHLC price series (open, high, low, close)

## Outputs

- `outInteger` — +100 for a white (rising) three-line strike, -100 for a black (falling) three-line strike, 0 otherwise. Sign is the color of the first three candles: candlecolor(i-1)*100

## Implementation

TA-Lib Definition: [`cdl3linestrike.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/cdl3linestrike/cdl3linestrike.c) · [`cdl3linestrike.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/cdl3linestrike/cdl3linestrike.yaml)

| Native | File |
|--------|------|
| C | [`ta_CDL3LINESTRIKE.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_CDL3LINESTRIKE.c) |
| Rust | [`cdl3linestrike.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/src/ta_func/cdl3linestrike.rs) |
| Java | [`Core_CDL3LINESTRIKE.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/Core_CDL3LINESTRIKE.java) |

TA-Lib is also available for Python, R and more using a [wrapper](https://ta-lib.org/wrappers/).

## Aliases

Three-Line Strike, 3-Line Strike

## See Also

CDL3WHITESOLDIERS · CDL3BLACKCROWS
