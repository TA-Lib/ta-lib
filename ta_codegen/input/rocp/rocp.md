# ROCP

## Summary

Rate of change expressed as a fraction of the price optInTimePeriod bars ago. Normalized and centered at zero (positive or negative). >0 rising vs N bars ago, <0 falling; equals ROC/100.

## Formula

ROCP = (price - prevPrice) / prevPrice, prevPrice = inReal[i - optInTimePeriod]

## Inputs

- `inReal` — Source price series

## Outputs

- `outReal` — Fractional rate of change vs the value optInTimePeriod bars earlier

## Parameters

- `optInTimePeriod` — Lookback distance to the previous price

## Aliases

Rate of Change Percentage, Percent Change

## See Also

ROC · ROCR · ROCR100 · MOM
