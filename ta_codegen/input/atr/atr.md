# ATR

## Summary

Wilder-smoothed average of the True Range over a period, measuring price volatility regardless of direction. Higher ATR means greater volatility; no directional bias.

## Formula

TR_t = max(high-low, |prevClose-high|, |prevClose-low|)
ATR seed = simple average of first `period` TR values
ATR_t = (ATR_{t-1} * (period-1) + TR_t) / period

## Inputs

- `inHigh` — High price of each bar
- `inLow` — Low price of each bar
- `inClose` — Close price of each bar

## Outputs

- `outReal` — Average True Range value

## Parameters

- `optInTimePeriod` — Smoothing period

## Aliases

Average True Range

## See Also

TRANGE · NATR · SMA · EMA

## References

- J. Welles Wilder, *New Concepts in Technical Trading Systems*, Trend Research (ISBN 0894590278)
