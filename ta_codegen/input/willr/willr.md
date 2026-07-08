# WILLR

## Summary

Williams' %R momentum oscillator over a rolling period, bounded in [-100, 0]. Measures where the current close sits relative to the high-low range of the last N bars. Near 0 = close at period high (overbought); near -100 = close at period low (oversold).

## Formula

%R = -100 * (highestHigh - close) / (highestHigh - lowestLow) over the trailing optInTimePeriod bars; if highestHigh == lowestLow, output 0.

## Inputs

- `inPriceHLC` — High, low, and close price series

## Outputs

- `outReal` — Williams' %R value in [-100, 0]

## Parameters

- `optInTimePeriod` — Lookback bars for the high/low range

## Implementation

TA-Lib Definition: [`willr.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/willr/willr.c) · [`willr.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/willr/willr.yaml)

| Native | File |
|--------|------|
| C | [`ta_WILLR.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_WILLR.c) |
| Rust | [`willr.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/src/ta_func/willr.rs) |
| Java | [`Core_WILLR.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/Core_WILLR.java) |

TA-Lib is also available for Python, R and more using a [wrapper](https://ta-lib.org/wrappers/).

## Aliases

Williams %R, Williams Percent R, %R

## See Also

STOCH · STOCHF · MINMAX
