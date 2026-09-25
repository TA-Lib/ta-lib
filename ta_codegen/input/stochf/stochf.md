# STOCHF

## Summary

Fast Stochastic Oscillator: the raw %K line and its moving-average-smoothed %D line. Unlike STOCH (which slows both lines), STOCHF returns the unsmoothed FastK and FastD. Oscillates 0-100; >80 overbought, <20 oversold.

## Formula

FastK = 100 * (Close - LowestLow) / (HighestHigh - LowestLow), over the last FastK_Period bars (incl. today)
FastD = MA(FastK, FastD_Period, FastD_MAType)

## Notes

- When the high-low range over the window is zero, %K is set to 0 instead of being undefined.

## Inputs

- `inHigh` — High price of each bar
- `inLow` — Low price of each bar
- `inClose` — Close price of each bar

## Outputs

- `outFastK` — Raw %K stochastic line
- `outFastD` — MA-smoothed %K (signal line)

## Parameters

- `optInFastK_Period` — Lookback window for the highest-high/lowest-low of Fast-K
- `optInFastD_Period` — Smoothing period for the Fast-D line
- `optInFastD_MAType` — Moving-average type used to smooth Fast-D

## Aliases

Stochastic Fast, Fast Stochastic Oscillator

## See Also

STOCH · STOCHRSI · MA
