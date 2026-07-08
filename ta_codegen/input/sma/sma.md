# SMA

## Summary

Simple Moving Average: the unweighted arithmetic mean of the last N input values. Used to smooth a series.

## Formula

SMA_t = (1/N) * sum_{i=t-N+1}^{t} inReal_i

## Notes

- A period of 1 performs no smoothing: the output is a copy of the input. Allowed since 0.6.5 (issues #48/#59).

## Inputs

- `inReal` — Source series to average

## Outputs

- `outReal` — Simple moving average of the input

## Parameters

- `optInTimePeriod` — Number of bars in the averaging window

## Implementation

TA-Lib Definition: [`sma.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/sma/sma.c) · [`sma.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/sma/sma.yaml)

| Native | File |
|--------|------|
| C | [`ta_SMA.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_SMA.c) |
| Rust | [`sma.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/src/ta_func/sma.rs) |
| Java | [`Core_SMA.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/Core_SMA.java) |

TA-Lib is also available for Python, R and more using a [wrapper](https://ta-lib.org/wrappers/).

## Aliases

simple moving average

## See Also

EMA · WMA · MA · DEMA · TEMA
