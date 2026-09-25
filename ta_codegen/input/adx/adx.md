# ADX

## Summary

Wilder's Average Directional Movement Index, a smoothed measure of trend strength derived from the directional indicators (+DI/-DI). Quantifies how strongly a market is trending, regardless of direction. Higher values indicate a stronger trend (a common convention treats >25 as trending); says nothing about direction.

## Formula

+DI = 100*(+DM_p/TR_p), -DI = 100*(-DM_p/TR_p); DX = 100*|(-DI)-(+DI)| / ((-DI)+(+DI)); first ADX = mean of the first `period` DX; then ADX = (prevADX*(period-1) + DX)/period. +DM_p/-DM_p/TR_p use Wilder smoothing: X = X - X/period + today's one-bar value.

## Notes

- Wilder's original integer rounding is not applied.

## Inputs

- `inHigh` — High price of each bar
- `inLow` — Low price of each bar
- `inClose` — Close price of each bar

## Outputs

- `outReal` — Smoothed directional trend-strength index (0-100)

## Parameters

- `optInTimePeriod` — Smoothing/averaging period for DM, TR, and ADX

## Aliases

Average Directional Movement Index, Average Directional Index

## See Also

ADXR · DX · PLUS_DI · MINUS_DI · PLUS_DM · MINUS_DM · TRANGE

## References

- J. Welles Wilder, *New Concepts in Technical Trading Systems*, Trend Research (ISBN 0894590278)
