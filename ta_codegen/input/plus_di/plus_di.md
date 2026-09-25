# PLUS_DI

## Summary

Plus Directional Indicator: the Wilder-smoothed positive directional movement expressed as a percentage of the true range. Measures the strength of upward price movement. Rising +DI signals strengthening upward direction; compared against MINUS_DI to judge trend direction.

## Formula

+DM1 = (H-Hprev) if (H-Hprev) > 0 and (H-Hprev) > (Lprev-L), else 0.
TR1 = true range = max(H-L, |H-Cprev|, |L-Cprev|).
Seed +DM/TR = sum of first (period-1) one-period values; then Wilder smooth: X = X - X/period + X1.
+DI = 100 * (+DM / TR); if TR = 0, +DI = 0.
When period <= 1: +DI = +DM1 / TR1 (no *100).

## Notes

- Wilder's original integer rounding of intermediate values is not applied (it was unreliable when values are near 1).

## Inputs

- `inHigh` — High price of each bar
- `inLow` — Low price of each bar
- `inClose` — Close price of each bar

## Outputs

- `outReal` — Plus Directional Indicator

## Parameters

- `optInTimePeriod` — Wilder smoothing period

## Aliases

+DI, Plus Directional Indicator, PDI

## See Also

MINUS_DI · DX · ADX · ADXR · PLUS_DM · TRANGE

## References

- J. Welles Wilder, *New Concepts in Technical Trading Systems*, Trend Research (ISBN 0894590278)
