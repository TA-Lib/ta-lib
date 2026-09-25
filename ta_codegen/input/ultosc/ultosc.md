# ULTOSC

## Summary

Ultimate Oscillator: momentum indicator combining buying-pressure/true-range ratios over three time periods into one 0-100 weighted average. Blends short-, medium-, and long-term momentum to damp single-period noise. Ranges 0-100; conventionally >70 overbought, <30 oversold.

## Formula

trueLow = min(low, prevClose);  BP = close - trueLow
TR = max(high-low, |prevClose-high|, |prevClose-low|)
avg_n = (sum BP over n bars) / (sum TR over n bars)
ULTOSC = 100 * (4*avg_short + 2*avg_mid + avg_long) / 7

## Notes

- The three periods are sorted internally, so the 4/2/1 weighting always applies to the shortest, middle, and longest period regardless of the order in which you pass them.

## Inputs

- `inHigh` — High price of each bar
- `inLow` — Low price of each bar
- `inClose` — Close price of each bar

## Outputs

- `outReal` — Ultimate Oscillator value

## Parameters

- `optInTimePeriod1` — Bars for one averaging window
- `optInTimePeriod2` — Bars for another averaging window
- `optInTimePeriod3` — Bars for another averaging window

## Aliases

Ultimate Oscillator, UO

## See Also

ATR · TRANGE · RSI

## References

- Larry Williams, *The Ultimate Oscillator*, Technical Analysis of Stocks & Commodities, V.3:4 (1985)
