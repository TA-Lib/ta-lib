# MIN

## Summary

Rolling minimum: the lowest input value over the trailing period.

## Formula

outReal[i] = min(inReal[i-optInTimePeriod+1 .. i])

## Inputs

- `inReal` — Source series to take the minimum of

## Outputs

- `outReal` — Lowest input value over the trailing window

## Parameters

- `optInTimePeriod` — Number of bars in the trailing window

## Aliases

Lowest, Rolling Min, Min Value

## See Also

MAX · MININDEX · MINMAX
