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

## Implementation

TA-Lib Definition: [`linearreg_angle.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/linearreg_angle/linearreg_angle.c) · [`linearreg_angle.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/linearreg_angle/linearreg_angle.yaml)

| Native | File |
|--------|------|
| C | [`ta_LINEARREG_ANGLE.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_LINEARREG_ANGLE.c) |
| Rust | [`linearreg_angle.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/src/ta_func/linearreg_angle.rs) |
| Java | [`Core_LINEARREG_ANGLE.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/Core_LINEARREG_ANGLE.java) |

TA-Lib is also available for Python, R and more using a [wrapper](https://ta-lib.org/wrappers/).

## Aliases

Linear Regression Angle, Least Squares Angle

## See Also

LINEARREG · LINEARREG_SLOPE · LINEARREG_INTERCEPT · TSF
