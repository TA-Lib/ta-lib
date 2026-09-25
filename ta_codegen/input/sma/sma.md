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

## Aliases

simple moving average

## See Also

EMA · WMA · MA · DEMA · TEMA
