# ROCR100

## Summary

Rate-of-change ratio scaled by 100: current price as a percentage of the price optInTimePeriod bars ago. Momentum measure centered at 100 and always positive. Above 100 = price rose vs n bars ago; below 100 = price fell.

## Formula

$ROCR100_t = \dfrac{price_t}{price_{t-n}} \times 100$, where $n$ = optInTimePeriod

## Inputs

- `inReal` — Input price/data series

## Outputs

- `outReal` — Rate-of-change ratio times 100

## Parameters

- `optInTimePeriod` — Lookback distance (bars back) for the reference price

## Aliases

Rate of Change Ratio 100 Scale, MO

## See Also

ROCR · ROC · ROCP · MOM
