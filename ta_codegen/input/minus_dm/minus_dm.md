# MINUS_DM

## Summary

Minus Directional Movement, the downward component of Wilder's directional movement system. Measures Wilder-smoothed downward price motion over the period. Higher -DM indicates stronger downward directional movement.

## Formula

diffP = high - prevHigh; diffM = prevLow - low
-DM1 = diffM if (diffM > 0 and diffP < diffM) else 0
period<=1: output raw -DM1 per bar.
period>1: seed = sum of first (period-1) -DM1; then Wilder smooth each bar:
-DM = prevMinusDM - prevMinusDM/period (+ -DM1 when the bar qualifies)

## Inputs

- `inHigh` — High price of each bar
- `inLow` — Low price of each bar

## Outputs

- `outReal` — Smoothed minus directional movement

## Parameters

- `optInTimePeriod` — Wilder smoothing period

## Aliases

Minus Directional Movement, -DM

## See Also

PLUS_DM · MINUS_DI · PLUS_DI · DX · ADX · ADXR

## References

- J. Welles Wilder, *New Concepts in Technical Trading Systems*, Trend Research (ISBN 0894590278)
