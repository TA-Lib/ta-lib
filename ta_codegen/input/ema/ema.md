# EMA

## Summary

Exponential moving average that weights recent prices more heavily via a recursive smoothing factor. A core building block seeding or composing many other indicators. Reacts faster than SMA; price above/below EMA suggests up/down trend.

## Formula

k = 2 / (period + 1); EMA_t = (price_t - EMA_{t-1}) * k + EMA_{t-1}. Seed: EMA = SMA of first `period` bars.

## Notes

- A period of 1 performs no smoothing: the output is a copy of the input. Allowed since 0.6.5 (issues #48/#59).

## Inputs

- `inReal` — price/data series to smooth

## Outputs

- `outReal` — the exponential moving average

## Parameters

- `optInTimePeriod` — Number of bars in the average; sets smoothing k = 2/(period+1)

## Aliases

Exponential Moving Average, Exponentially Weighted Moving Average, EWMA

## See Also

SMA · DEMA · TEMA · MA · MACD · T3
