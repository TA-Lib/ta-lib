# DX

## Summary

Wilder's Directional Movement Index: the normalized spread between +DI and -DI. Measures the strength of directional (trending) movement, irrespective of direction. Higher DX = stronger trend (either direction); low DX = ranging market.

## Formula

Seed +DM14, -DM14, TR14 as sums of the first (period-1) one-period values, then Wilder-smooth each: X = X - X/period + today. +DI = 100*(+DM14/TR14), -DI = 100*(-DM14/TR14). DX = 100 * |(-DI) - (+DI)| / ((-DI) + (+DI)).

## Notes

- Wilder's original integer rounding is not applied (it can be unreliable when values are near 1).
- When +DI and -DI sum to zero the value is undefined; the previous bar's DX is carried forward instead (the first such bar outputs zero).

## Inputs

- `inHigh` — High price of each bar
- `inLow` — Low price of each bar
- `inClose` — Close price of each bar

## Outputs

- `outReal` — DX directional movement index value

## Parameters

- `optInTimePeriod` — Smoothing period for the DM and TR sums

## Aliases

Directional Movement Index, DMI

## See Also

ADX · ADXR · PLUS_DI · MINUS_DI · PLUS_DM · MINUS_DM · TRANGE

## References

- J. Welles Wilder, *New Concepts in Technical Trading Systems*, Trend Research (ISBN 0894590278)
