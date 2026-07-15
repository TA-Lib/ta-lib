# AVGDEV

## Summary

Rolling average absolute deviation of a series from its own simple moving average over the last N periods. Measures dispersion around the window mean. Higher values indicate greater spread; zero when all values in the window are equal.

## Formula

$mean_t = \frac{1}{N}\sum_{i=0}^{N-1} x_{t-i}$; $AVGDEV_t = \frac{1}{N}\sum_{i=0}^{N-1} |x_{t-i} - mean_t|$ (N = optInTimePeriod)

## Inputs

- `inReal` — source series

## Outputs

- `outReal` — mean absolute deviation over the window

## Parameters

- `optInTimePeriod` — window length

## Implementation

TA-Lib Definition: [`avgdev.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/avgdev/avgdev.c) · [`avgdev.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/avgdev/avgdev.yaml)

| Native | File |
|--------|------|
| C | [`ta_AVGDEV.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_AVGDEV.c) |
| Rust | [`avgdev.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/avgdev.rs) |
| Java | [`Core_AVGDEV.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/library/fragments/Core_AVGDEV.java) |

TA-Lib is also available for Python, R and more using a [wrapper](https://ta-lib.org/wrappers/).

## Aliases

Average Deviation, Mean Absolute Deviation, MAD

## See Also

STDDEV · VAR · SMA
