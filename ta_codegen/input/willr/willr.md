# WILLR

## Summary

Williams' %R momentum oscillator over a rolling period, bounded in [-100, 0]. Measures where the current close sits relative to the high-low range of the last N bars. Near 0 = close at period high (overbought); near -100 = close at period low (oversold).

## Formula

%R = ((highestHigh - close) / (highestHigh - lowestLow)) * -100 over the trailing optInTimePeriod bars, clamped to [-100, 0]; if highestHigh == lowestLow, output 0.

## Inputs

- `inHigh` — High price of each bar
- `inLow` — Low price of each bar
- `inClose` — Close price of each bar

## Outputs

- `outReal` — Williams' %R value in [-100, 0]

## Parameters

- `optInTimePeriod` — Lookback bars for the high/low range

## Aliases

Williams %R, Williams Percent R, %R

## See Also

STOCH · STOCHF · MINMAX
