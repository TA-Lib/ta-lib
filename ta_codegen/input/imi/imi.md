# IMI

## Summary

Intraday Momentum Index: an RSI-like 0-100 oscillator built from the open-to-close body of each bar. Over a rolling window it ratios cumulative up-body moves against total up+down body moves.

## Formula

upsum = Σ(close-open) for bars with close>open; downsum = Σ(open-close) for bars with close<=open, over window [i-lookback, i]; IMI = 100 * upsum/(upsum+downsum)

## Inputs

- `inPriceOC` — Per-bar open and close prices

## Outputs

- `outReal` — IMI oscillator value, 0-100

## Parameters

- `optInTimePeriod` — Rolling window length for the up/down body sums

## Implementation

TA-Lib Definition: [`imi.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/imi/imi.c) · [`imi.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/imi/imi.yaml)

| Native | File |
|--------|------|
| C | [`ta_IMI.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_IMI.c) |
| Rust | [`imi.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/src/ta_func/imi.rs) |
| Java | [`Core_IMI.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/Core_IMI.java) |

TA-Lib is also available for Python, R and more using a [wrapper](https://ta-lib.org/wrappers/).

## Aliases

Intraday Momentum Index

## See Also

RSI
