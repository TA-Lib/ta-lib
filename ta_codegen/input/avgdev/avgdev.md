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

- `optInTimePeriod` — Window length

## Aliases

Average Deviation, Mean Absolute Deviation, MAD

## See Also

STDDEV · VAR · SMA
