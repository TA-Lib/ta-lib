# WMA

## Summary

Linearly weighted moving average: each of the last N prices is weighted by its position, oldest getting weight 1 and newest weight N. Smooths price while emphasizing recent bars.

## Formula

WMA = ( sum_{k=1..N} k * P_k ) / (N(N+1)/2), where P_N is the most recent bar

## Notes

- A period of 1 performs no smoothing: the output is a copy of the input. Allowed since 0.6.5 (issues #48/#59).

## Inputs

- `inReal` — Source price/data series

## Outputs

- `outReal` — Weighted moving average series

## Parameters

- `optInTimePeriod` — Number of bars in the weighting window

## Aliases

Weighted Moving Average, Linearly Weighted Moving Average, LWMA

## See Also

SMA · EMA · MA · DEMA · TEMA
