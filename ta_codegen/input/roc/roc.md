# ROC

## Summary

Rate-of-change momentum oscillator: the percent change of price versus the price optInTimePeriod bars earlier. Centered at zero with positive and negative values. Positive when price rose over the period, negative when it fell; magnitude scales the move.

## Formula

ROC = ((price / prevPrice) - 1) * 100, where prevPrice = inReal[i - optInTimePeriod]

## Inputs

- `inReal` — Input price series

## Outputs

- `outReal` — Percent rate of change

## Parameters

- `optInTimePeriod` — Lookback distance to the prior price

## Aliases

Rate of Change, Price Rate of Change

## See Also

MOM · ROCP · ROCR · ROCR100
