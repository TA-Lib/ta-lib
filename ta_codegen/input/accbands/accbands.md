# ACCBANDS

## Summary

Acceleration Bands: three overlap lines around price. The middle band is an SMA of the close; the upper/lower bands are SMAs of the high/low scaled by an intraday-range factor.

## Formula

factor = 4*(H-L)/(H+L)
upperRaw = H*(1+factor), lowerRaw = L*(1-factor)
Upper = SMA(upperRaw, N), Middle = SMA(Close, N), Lower = SMA(lowerRaw, N)

## Inputs

- `inHigh` — High price of each bar
- `inLow` — Low price of each bar
- `inClose` — Close price of each bar

## Outputs

- `outRealUpperBand` — SMA of the range-scaled high band
- `outRealMiddleBand` — SMA of the close
- `outRealLowerBand` — SMA of the range-scaled low band

## Parameters

- `optInTimePeriod` — SMA smoothing period for all three bands

## Aliases

Acceleration Bands

## See Also

SMA · BBANDS
