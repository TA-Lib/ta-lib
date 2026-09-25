# MINUS_DI

## Summary

Wilder's Minus Directional Indicator: the Wilder-smoothed downward directional movement (-DM) normalized by smoothed True Range. Measures the strength of downward price movement. Higher -DI indicates a stronger downtrend; compared against +DI to gauge directional dominance.

## Formula

-DM1 = (prevLow - low) if (prevLow-low)>0 and (high-prevHigh)<(prevLow-low), else 0. Seed -DM/TR = sum of first (period-1) -DM1/TR1, then Wilder-smooth each: X = X - X/period + today. -DI = 100 * (-DM / TR); TR from ta_true_range. If period<=1: -DI1 = -DM1/TR1 (no ×100).

## Notes

- Wilder's original integer rounding is not applied (it was removed as unreliable when values are near 1).

## Inputs

- `inHigh` — High price of each bar
- `inLow` — Low price of each bar
- `inClose` — Close price of each bar

## Outputs

- `outReal` — The Minus Directional Indicator (-DI) line

## Parameters

- `optInTimePeriod` — Smoothing/lookback period for -DM and TR

## Aliases

-DI, Negative Directional Indicator

## See Also

PLUS_DI · MINUS_DM · DX · ADX · ADXR · TRANGE

## References

- J. Welles Wilder, *New Concepts in Technical Trading Systems*, Trend Research (ISBN 0894590278)
