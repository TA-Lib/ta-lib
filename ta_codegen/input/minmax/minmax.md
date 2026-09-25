# MINMAX

## Summary

Returns both the lowest and highest values of the input over a rolling window of the last optInTimePeriod bars. An overlap-study companion to MIN and MAX that computes both extrema in one pass.

## Formula

outMin[i] = min(inReal[i-optInTimePeriod+1 .. i])  
outMax[i] = max(inReal[i-optInTimePeriod+1 .. i])

## Inputs

- `inReal` — Values scanned for the window min and max

## Outputs

- `outMin` — Lowest value in each rolling window
- `outMax` — Highest value in each rolling window

## Parameters

- `optInTimePeriod` — Rolling window length

## Aliases

Highest Lowest

## See Also

MIN · MAX · MINMAXINDEX · MININDEX · MAXINDEX
