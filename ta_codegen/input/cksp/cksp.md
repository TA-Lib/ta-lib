# CKSP

## Summary

Chande and Kroll's two-line volatility stop. The highest high is offset down, and the lowest low up, by a multiple of the Average True Range, and each line then takes the extreme of its own recent values, so it moves only when a new extreme enters the stop window or an old one leaves it. Price above both lines reads as an uptrend and price below both as a downtrend.

## Formula

FirstHighStop = MAX(High, TimePeriod) - Multiplier * ATR(TimePeriod)
FirstLowStop = MIN(Low, TimePeriod) + Multiplier * ATR(TimePeriod)

HighStop = MAX(FirstHighStop, StopPeriod)
LowStop = MIN(FirstLowStop, StopPeriod)

## Notes

- The two lines are named for how they are built, not for the position they protect, because published implementations give "long stop" and "short stop" to opposite lines. `outHighStop` is the line most platforms plot as the short stop, `outLowStop` the one they plot as the long stop.
- Neither line is always above the other: a large multiplier pushes the high line below the low one.
- The Average True Range is this library's, whose first value is the average of the first full period of true ranges that have a previous close. Platforms that start the true range on the very first bar give slightly different values on early bars; the difference decays as the average warms up.
- The book's own settings are reported as a multiplier of 3 and a stop period of 20; the defaults here are the ones charting platforms ship.
- A stop period of 1 leaves the first stops unchanged, which is the Chandelier Exit on both sides.
- Both lines inherit the Average True Range's warm-up, so a caller who wants them converged sets `TA_FUNC_UNST_ATR`, exactly as when calling that function directly.

## Inputs

- `inHigh` — High price of each bar
- `inLow` — Low price of each bar
- `inClose` — Close price of each bar

## Outputs

- `outHighStop` — Highest of the recent first high stops; usually plotted as the short stop
- `outLowStop` — Lowest of the recent first low stops; usually plotted as the long stop

## Parameters

- `optInTimePeriod` — Window of the highest high and lowest low, and smoothing period of the Average True Range
- `optInMultiplier` — Multiplier applied to the Average True Range to offset the first stops
- `optInStopPeriod` — Window over which each first stop takes its extreme

## Aliases

Chande Kroll Stop, CKS, Chande-Kroll Stop

## See Also

ATR · MAX · MIN · SUPERTREND · KC · DONCHIAN

## References

- Tushar S. Chande and Stanley Kroll, *The New Technical Trader*, John Wiley & Sons, 1994
- [Chande Kroll Stop (TradingView)](https://www.tradingview.com/support/solutions/43000589105-chande-kroll-stop/)
