# MAXINDEX

## Summary

Returns the index of the highest input value within a rolling window of optInTimePeriod bars. Same as MAX but outputs the location instead of the value.

## Formula

outInteger[i] = index of max(inReal[i-optInTimePeriod+1 .. i])

## Notes

- When several bars in a window share the highest value, the index of the most recent of them is returned.

## Inputs

- `inReal` — Input series to scan

## Outputs

- `outInteger` — Absolute index (into inReal) of the highest value in each window

## Parameters

- `optInTimePeriod` — Window length over which the max is located

## Aliases

Index of Highest Value, Highest Value Index, argmax

## See Also

MAX · MININDEX · MIN · MINMAXINDEX
