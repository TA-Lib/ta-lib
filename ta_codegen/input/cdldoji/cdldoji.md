# CDLDOJI

## Summary

Single-candle Doji recognizer: fires when the real body (|close-open|) is at or below the BodyDoji threshold. Returns 100 on a match, 0 otherwise. Market indecision; neither bullish nor bearish on its own.

## Formula

match if $|close-open| \le \text{CandleAverage(BodyDoji)}$

## Inputs

- `inPriceOHLC` — OHLC price bars

## Outputs

- `outInteger` — 100 when a doji is detected, else 0

## Implementation

TA-Lib Definition: [`cdldoji.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/cdldoji/cdldoji.c) · [`cdldoji.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/cdldoji/cdldoji.yaml)

| Native | File |
|--------|------|
| C | [`ta_CDLDOJI.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_CDLDOJI.c) |
| Rust | [`cdldoji.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/src/ta_func/cdldoji.rs) |
| Java | [`Core_CDLDOJI.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/Core_CDLDOJI.java) |

TA-Lib is also available for Python, R and more using a [wrapper](https://ta-lib.org/wrappers/).

## Aliases

Doji

## See Also

CDLDOJISTAR · CDLDRAGONFLYDOJI · CDLGRAVESTONEDOJI · CDLLONGLEGGEDDOJI
