# SUM

## Summary

Rolling sum of the input over a fixed period. Each output is the sum of the most recent optInTimePeriod input values.

## Formula

$out_i = \sum_{j=i-(N-1)}^{i} inReal_j$, N = optInTimePeriod

## Inputs

- `inReal` — Values to sum

## Outputs

- `outReal` — Windowed sum over the period

## Parameters

- `optInTimePeriod` — Window length summed

## Aliases

Summation, Rolling Sum, Moving Sum

## See Also

SMA
