# MIDPRICE

## Summary

Midpoint of the price range over a rolling window: the average of the highest high and lowest low across the last optInTimePeriod bars. An overlap-study line plotted on price.

## Formula

MIDPRICE = (Highest(High, N) + Lowest(Low, N)) / 2, over the N=optInTimePeriod bars ending at each index

This is the Donchian Channel centerline: `DONCHIAN` emits this line as its middle output, alongside the two extrema.

## Inputs

- `inHigh` — High price of each bar
- `inLow` — Low price of each bar

## Outputs

- `outReal` — Midpoint of the period's high/low extremes

## Parameters

- `optInTimePeriod` — Window length over which the high/low extremes are taken

## Aliases

Midpoint Price

## See Also

MIDPOINT · MEDPRICE
