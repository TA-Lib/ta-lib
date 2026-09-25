# MINMAXINDEX

## Summary

Returns the absolute input indices of the lowest and highest values within each rolling window of optInTimePeriod bars. Index variant of MINMAX.

## Formula

outMinIdx[i] = index of min(inReal[i-optInTimePeriod+1 .. i])  
outMaxIdx[i] = index of max(inReal[i-optInTimePeriod+1 .. i])

## Notes

- When several bars in a window share the extreme value, the index of one of them is returned — not necessarily the first or the last.

## Inputs

- `inReal` — Input series scanned for extremes

## Outputs

- `outMinIdx` — Absolute index (into inReal) of the window minimum
- `outMaxIdx` — Absolute index (into inReal) of the window maximum

## Parameters

- `optInTimePeriod` — Window length in bars

## Aliases

Lowest/Highest Index

## See Also

MINMAX · MIN · MAX · MININDEX · MAXINDEX
