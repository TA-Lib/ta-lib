# DOSC

## Summary

Derivative Oscillator: Wilder's RSI, smoothed by two exponential averages in series, with a simple average of that smoothed line subtracted from it. Constance Brown's reading is that the RSI's own swings are too noisy to time with, so she smooths it twice and then plots the distance from its own average as a histogram — MACD's histogram construction, applied to a smoothed RSI rather than to price. The result is in RSI points, centred on zero: crossings of the zero line mark the turn, and the height measures how far the smoothed RSI has run from its mean.

## Formula

RSI = RSI(inReal, timePeriod)   (Wilder, SMA-seeded)

S1 = EMA(RSI, firstPeriod)

DS = EMA(S1, secondPeriod)

DOSC = DS - SMA(DS, signalPeriod)

## Notes

- Every stage is a call to a function TA-Lib already ships, so the output is identical, bit for bit, to `TA_RSI` followed by two `TA_EMA` calls, a `TA_SMA` and a `TA_SUB`. The shortest expression of that chain takes five calls and three intermediate buffers; this computes it in one pass without materialising them.
- Each exponential average is seeded with a simple average of its own first inputs, the same seeding TA-Lib's EMA uses, and the second seeds on what the first publishes. `TA_SetUnstablePeriod` on either `TA_FUNC_UNST_RSI` or `TA_FUNC_UNST_EMA` discards more of that warm-up, and the EMA setting counts twice because there are two exponential stages. Implementations seeding each stage from a single first sample differ over the transient and agree once it decays.
- The degenerate reading is whatever `TA_RSI` answers when neither a gain nor a loss has been seen since the seed. Some other implementations answer 100 there and will disagree over that stretch.
- The periods are independent: the stages commute, so no ordering between the two smoothing periods is required or checked. A signal period of 1 would make the output identically zero, so the minimum is 2.

## Inputs

- `inReal` — Input series, usually the close

## Outputs

- `outReal` — Derivative Oscillator, in RSI points, centred on zero

## Parameters

- `optInTimePeriod` — Period of the RSI
- `optInFirstPeriod` — Period of the first smoothing, applied to the RSI
- `optInSecondPeriod` — Period of the second smoothing, applied to the first
- `optInSignalPeriod` — Period of the simple average subtracted from the smoothed line

## Aliases

derivative oscillator

## See Also

RSI · STOCHRSI · MACD · AC

## References

- Constance M. Brown, "The Derivative Oscillator: A New Approach to an Old Problem", *Journal of Technical Analysis* (MTA Journal), Issue 45, Winter-Spring 1994, pp. 45-50
