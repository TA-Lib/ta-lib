# PLUS_DM

## Summary

Plus Directional Movement: the Wilder-smoothed accumulation of upward directional movement (+DM1). A component of the Directional Movement System used to build +DI/DX/ADX.

## Formula

+DM1 = (high - prevHigh) if (high-prevHigh) > 0 and > (prevLow-low), else 0.
period<=1: output = +DM1 per bar.
period>1: seed = sum of first (period-1) +DM1; then Wilder smoothing:
+DM = prevPlusDM - prevPlusDM/period + +DM1(today)

## Inputs

- `inHigh` — High price of each bar
- `inLow` — Low price of each bar

## Outputs

- `outReal` — Smoothed plus directional movement

## Parameters

- `optInTimePeriod` — Wilder smoothing period

## Aliases

+DM, Plus Directional Movement

## See Also

MINUS_DM · PLUS_DI · MINUS_DI · DX · ADX · ADXR

## References

- J. Welles Wilder, *New Concepts in Technical Trading Systems*, Trend Research (ISBN 0894590278)
