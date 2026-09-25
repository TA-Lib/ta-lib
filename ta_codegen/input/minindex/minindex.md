# MININDEX

## Summary

Returns the absolute index of the lowest value within a rolling window of the given period. Same scan as MIN but outputs the position of the minimum rather than its value.

## Formula

outInteger[i] = index of min(inReal[i-optInTimePeriod+1 .. i])

## Notes

- When several bars in a window share the lowest value, the index of one of them is returned — not necessarily the first or the last.

## Inputs

- `inReal` — Series to scan for its minimum

## Outputs

- `outInteger` — Absolute index in inReal of the lowest value in each window

## Parameters

- `optInTimePeriod` — Window length over which the minimum is located

## Aliases

Index of Lowest Value, Lowest Value Index, Rolling Argmin

## See Also

MIN · MAXINDEX · MINMAXINDEX · MINMAX
