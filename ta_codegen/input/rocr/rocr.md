# ROCR

## Summary

Rate of Change Ratio: the ratio of the current price to the price optInTimePeriod bars ago. A momentum measure centered at 1. Always positive, centered at 1: >1 rising, <1 falling.

## Formula

ROCR = price / price[t - optInTimePeriod]

## Inputs

- `inReal` — Price series

## Outputs

- `outReal` — Ratio of current price to prior price

## Parameters

- `optInTimePeriod` — Lookback distance in bars for the prior price

## Aliases

Rate of Change Ratio

## See Also

ROC · ROCP · ROCR100 · MOM
