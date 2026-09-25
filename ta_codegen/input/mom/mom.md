# MOM

## Summary

Momentum: current price minus the price optInTimePeriod bars ago. The absolute (unnormalized) rate of change. Positive = price rose over the period, negative = fell; centered at zero.

## Formula

MOM[i] = inReal[i] - inReal[i - optInTimePeriod]

## Inputs

- `inReal` — Input price series

## Outputs

- `outReal` — Momentum (current minus value optInTimePeriod bars ago)

## Parameters

- `optInTimePeriod` — Lookback distance in bars

## Aliases

Momentum

## See Also

ROC · ROCP · ROCR · ROCR100
