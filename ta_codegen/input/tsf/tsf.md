# TSF

## Summary

Time Series Forecast: fits a least-squares linear regression line over the last N bars and projects it one x-step beyond LINEARREG. Same regression as LINEARREG but evaluated at x=period instead of x=period-1.

## Formula

Fit y=b+m*x over window (x=0..N-1): m = (N*SumXY - SumX*SumY)/(SumX^2 - N*SumXSqr), b = (SumY - m*SumX)/N; output = b + m*N. With SumX=N(N-1)/2, SumXSqr=N(N-1)(2N-1)/6.

## Inputs

- `inReal` — Input series to regress and forecast

## Outputs

- `outReal` — Regression line value projected to x=period (one step past LINEARREG)

## Parameters

- `optInTimePeriod` — Number of bars in the regression window

## Aliases

Time Series Forecast

## See Also

LINEARREG · LINEARREG_SLOPE · LINEARREG_INTERCEPT · LINEARREG_ANGLE
