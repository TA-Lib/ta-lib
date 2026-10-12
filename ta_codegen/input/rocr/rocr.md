# ROCR

## Summary

Rate of Change Ratio: the ratio of the current price to the price optInTimePeriod bars ago. A momentum measure centered at 1. Always positive, centered at 1: >1 rising, <1 falling.

## Formula

ROCR = price / price[t - optInTimePeriod]

## Notes

- **The input is assumed positive.** The price `optInTimePeriod` bars back is the divisor. Where it is exactly 0 the output is 0. Near zero the divisor is small and the output very large; no error is returned. A series that reaches or crosses zero, such as an oscillator or a spread, has no meaningful result here.

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
