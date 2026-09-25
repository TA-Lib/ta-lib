# MAX

## Summary

Highest input value over a rolling window of the last optInTimePeriod bars. A moving-window maximum.

## Formula

outReal[i] = max(inReal[i-optInTimePeriod+1 .. i])

## Inputs

- `inReal` — Series to take the rolling maximum of

## Outputs

- `outReal` — Highest value within each trailing window

## Parameters

- `optInTimePeriod` — Window length in bars

## Aliases

Highest, Highest High, Rolling Maximum

## See Also

MIN · MAXINDEX · MINMAX
