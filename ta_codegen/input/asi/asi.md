# ASI

## Summary

Wilder's Accumulative Swing Index: the running total of the Swing Index (SI), a line meant to trace the market's real swings through the noise of the daily closes. Wilder draws trendlines and breakout points on it as on a price chart: a breakout of the ASI that the price has not confirmed yet is his signal. J. Welles Wilder Jr. introduced both in 1978. The line starts at 0 on the first bar of the requested range, so only its shape carries meaning, not its level.

## Formula

$$
\mathrm{ASI}_s = 0 \qquad \mathrm{ASI}_t = \mathrm{ASI}_{t-1} + \mathrm{SI}_t \quad (t > s)
$$

where $s$ is the first bar of the requested range and $\mathrm{SI}_t$ is the Swing Index of bar $t$ against bar $t-1$ at the same `optInLimitMove`.

## Notes

- Wilder accumulates Swing Index values rounded to whole numbers; the running total here is of the unrounded values.

## Inputs

- `inOpen` — Open price of each bar
- `inHigh` — High price of each bar
- `inLow` — Low price of each bar
- `inClose` — Close price of each bar

## Outputs

- `outReal` — Running total of the swing index from the first bar of the range

## Parameters

- `optInLimitMove` — Limit move, the largest one-bar price move the index is scaled against, in price units

## Aliases

Accumulation Swing Index

## See Also

SI · WAD · CUMSUM

## References

- J. Welles Wilder Jr., *New Concepts in Technical Trading Systems*, Trend Research, 1978, section VIII "The Swing Index", pp.87-106
