# CORREL

## Summary

Pearson's correlation coefficient (r) between two input series over a rolling window of optInTimePeriod bars. Measures how linearly the two series move together. r near +1: strong positive co-movement; near -1: strong inverse; near 0: no linear relationship.

## Formula

r = (sumXY - sumX*sumY/n) / sqrt((sumX2 - sumX^2/n) * (sumY2 - sumY^2/n)),  n = optInTimePeriod, sums over the window

## Notes

- When the correlation is undefined for a window (for example a constant series), the output is 0 rather than an error or NaN.

## Inputs

- `inReal0` — First data series (X)
- `inReal1` — Second data series (Y)

## Outputs

- `outReal` — Correlation coefficient r in [-1, 1]

## Parameters

- `optInTimePeriod` — Rolling window length

## Aliases

Pearson Correlation, Correlation Coefficient, r

## See Also

BETA · STDDEV · VAR

## References

- Karl Pearson
