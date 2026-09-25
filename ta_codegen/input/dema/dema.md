# DEMA

## Summary

Double Exponential Moving Average: an EMA combined with an EMA-of-EMA to reduce lag versus a plain EMA. Overlap Studies overlay on price.

## Formula

EMA1 = EMA(inReal, period); EMA2 = EMA(EMA1, period); DEMA = 2*EMA1 - EMA2

## Notes

- A period of 1 performs no smoothing: the output is a copy of the input. Allowed since 0.6.5 (issues #48/#59).

## Inputs

- `inReal` — Source series (typically price)

## Outputs

- `outReal` — DEMA line

## Parameters

- `optInTimePeriod` — Smoothing period for both EMA passes

## Aliases

Double Exponential Moving Average

## See Also

EMA · TEMA · MA

## References

- Patrick G. Mulloy, *Smoothing Data with Faster Moving Averages*, Technical Analysis of Stocks & Commodities, V.12:1 (January 1994)
