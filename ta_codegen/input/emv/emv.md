# EMV

## Summary

Richard W. Arms, Jr.'s Ease of Movement: the bar-to-bar move of the high-low midpoint divided by a box ratio of volume to range, smoothed by a simple moving average. It is positive when the midpoint rises, and large when that move came on light volume relative to the bar's range.

## Formula

mid[i] = (high[i] + low[i]) / 2, raw[i] = (mid[i] - mid[i-1]) / ((volume[i] / optInVolumeDivisor) / (high[i] - low[i])), and outReal[i] is the average of the last optInTimePeriod raw values. raw[i] is exactly 0 on a bar with no volume or no range, and mid[i-1] is the midpoint of the bar immediately before i, including such a bar. At optInTimePeriod 1 the output is the raw series itself.

## Notes

- The divisor is a pure output scale: in exact arithmetic EMV is proportional to it, so it selects the units the values are read in rather than a different indicator. 10,000 is the constant Achelis's worked table was computed with; StockCharts and some libraries print 100,000,000 instead.
- The range is in points. Achelis's entry describes it in eighths, which at a divisor D is the same series as points at 8D.
- lookback = optInTimePeriod: one bar forms the first midpoint change, then the average's own warm-up of optInTimePeriod-1 on top. The divisor does not enter it.

## Inputs

- `inHigh` — High price of each bar
- `inLow` — Low price of each bar
- `inVolume` — Volume of each bar

## Outputs

- `outReal` — Ease of Movement

## Parameters

- `optInTimePeriod` — Number of periods for the smoothing average; 1 leaves the raw series
- `optInVolumeDivisor` — Volume scale divisor

## See Also

- MARKETFI — the same high, low and volume bundle, as a per-bar ratio with no smoothing
- EFI — the same "one bar for the difference, then the average's own lookback" shape

## References

- Steven B. Achelis, Technical Analysis from A to Z, p. 132
