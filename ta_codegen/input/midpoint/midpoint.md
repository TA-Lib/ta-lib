# MIDPOINT

## Summary

Midpoint over a period: the average of the highest and lowest input values within the lookback window. A single-series overlap smoother (use MIDPRICE for separate high/low price bars).

## Formula

MIDPOINT = (Highest(inReal, period) + Lowest(inReal, period)) / 2

## Inputs

- `inReal` — Series to compute the midpoint over

## Outputs

- `outReal` — Midpoint of the period's high/low range

## Parameters

- `optInTimePeriod` — Lookback window length

## See Also

MIDPRICE · MAX · MIN
