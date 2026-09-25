# LINEARREG_INTERCEPT

## Summary

Returns the y-intercept (b) of the least-squares regression line fitted over the last optInTimePeriod values. Part of the linear-regression family (LINEARREG, SLOPE, ANGLE, TSF).

## Formula

Fit y = b + m·x over the window with x = bars-ago (x=0 is the current bar, x=period-1 the oldest). With SumX = period(period-1)/2, SumXSqr = period(period-1)(2·period-1)/6, Divisor = SumX² − period·SumXSqr:
m = (period·SumXY − SumX·SumY) / Divisor
b = (SumY − m·SumX) / period   ← output

## Inputs

- `inReal` — Input series to regress

## Outputs

- `outReal` — Intercept b of the fitted line at each bar

## Parameters

- `optInTimePeriod` — Window length of the regression

## Aliases

Linear Regression Intercept

## See Also

LINEARREG · LINEARREG_SLOPE · LINEARREG_ANGLE · TSF
