# LINEARREG_ANGLE

## Summary

The angle, in degrees, of the least-squares best-fit line over the last N points. It is the LINEARREG_SLOPE value passed through atan and converted to degrees. Positive angle = rising fit line, negative = falling; magnitude reflects steepness.

## Formula

m = (N·SumXY − SumX·SumY) / (SumX² − N·SumXSqr), with SumX=N(N−1)/2, SumXSqr=N(N−1)(2N−1)/6; angle = atan(m)·(180/π)

## Inputs

- `inReal` — Input series to regress

## Outputs

- `outReal` — Regression line slope expressed as an angle in degrees

## Parameters

- `optInTimePeriod` — Number of points in the regression window

## Aliases

Linear Regression Angle, Least Squares Angle

## See Also

LINEARREG · LINEARREG_SLOPE · LINEARREG_INTERCEPT · TSF
