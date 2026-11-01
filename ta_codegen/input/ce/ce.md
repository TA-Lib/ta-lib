# CE

## Summary

Le Beau's Chandelier Exit, a two-line volatility stop. The highest high of the window is offset down, and the lowest low up, by a multiple of the Average True Range, so each line hangs from the recent extreme at a distance that widens as the true range grows. The long line is the exit for a long position and hangs below the recent highs; the short line is the mirror for a short position, above the recent lows.

## Formula

Long = MAX(High, TimePeriod) - Multiplier * ATR(TimePeriod)
Short = MIN(Low, TimePeriod) + Multiplier * ATR(TimePeriod)

## Notes

- The extremes are taken over the last TimePeriod bars. Le Beau anchors the highest high at the entry of the trade, which a function over a price series cannot know; the rolling window is what charting platforms plot under this name.
- The Average True Range is this library's, whose first value is the average of the first full period of true ranges that have a previous close. Platforms that start the true range on the very first bar give slightly different values on early bars; the difference decays as the average warms up.
- Both lines are emitted on every bar, each from the highs or the lows. A popular variant takes the extremes of the closes and keeps a single stop that only moves toward price until the trend flips; that variant is not this function.
- Neither line is always on its side of the other: with a small multiplier the long line sits above the short one.
- Both lines inherit the Average True Range's warm-up, so a caller who wants them converged sets `TA_FUNC_UNST_ATR`, exactly as when calling that function directly.
- The two lines are the Chande Kroll Stop with a stop period of 1, `outRealLong` being CKSP's `outHighStop`.

## Inputs

- `inHigh` — High price of each bar
- `inLow` — Low price of each bar
- `inClose` — Close price of each bar

## Outputs

- `outRealLong` — Highest high less the multiple of the Average True Range; the stop for a long position
- `outRealShort` — Lowest low plus the multiple of the Average True Range; the stop for a short position

## Parameters

- `optInTimePeriod` — Window of the highest high and lowest low, and smoothing period of the Average True Range
- `optInMultiplier` — Multiplier applied to the Average True Range to offset the lines

## Aliases

Chandelier Exit, Chandelier Stop, Le Beau Chandelier Exit

## See Also

ATR · MAX · MIN · CKSP · SUPERTREND · KC · SAR

## References

- Alexander Elder, *Come Into My Trading Room*, John Wiley & Sons, 2002, where Charles Le Beau's method is described
- [Chandelier Exit (StockCharts ChartSchool)](https://chartschool.stockcharts.com/table-of-contents/technical-indicators-and-overlays/technical-overlays/chandelier-exit)
