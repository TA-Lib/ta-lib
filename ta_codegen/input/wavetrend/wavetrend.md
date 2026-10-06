# WAVETREND

## Summary

WaveTrend Oscillator: how far the typical price sits from its own exponential average, divided by an exponential average of that distance and by Lambert's CCI constant, then smoothed. The reading is the plain stochastic's complaint answered a different way — rather than bounding the oscillator by construction, it scales it by how far price has recently been travelling, so the same numeric level means the same thing in a quiet market and a fast one. The oscillator line and its short simple average cross; the crossings that matter are the ones beyond the extremes, which the author draws at ±53 and ±60.

## Formula

ap = (high + low + close) / 3

esa = EMA(ap, channelPeriod)

d = EMA(|ap - esa|, channelPeriod)

ci = (ap - esa) / (0.015 * d)

WT1 = EMA(ci, averagePeriod)

WT2 = SMA(WT1, signalPeriod)

## Notes

- The middle stage is an exponential CCI, not `TA_CCI`: CCI averages with a simple moving average and takes the mean deviation around that window's own average, where this uses two exponential averages.
- On a market that has stopped moving, the exponential average stops moving too — once its step falls under half an ulp it freezes, a few ulps away from the price. That frozen gap is a real non-zero distance, so dividing by its own average walks the oscillator to ±66.67, an extreme reading produced by nothing but rounding. This answers 0 instead, by taking the distance as zero whenever the price and its average both repeat. Implementations without that test drift to the extreme on flat data, at a bar that depends on their arithmetic.
- A zero divisor is tested on the scaled deviation, the quantity the division actually uses, and answers the neutral 0 rather than dividing.
- Each exponential average is seeded with a simple average of its own first inputs, the same seeding TA-Lib's EMA uses, and each seeds on what the stage before it publishes. `TA_SetUnstablePeriod(TA_FUNC_UNST_EMA, ...)` discards more of that warm-up, and it counts three times because there are three exponential stages.
- The difference the author also plots is `TA_SUB(outWT1, outWT2)`.

## Inputs

- `inHigh` — High price series
- `inLow` — Low price series
- `inClose` — Close price series

## Outputs

- `outWT1` — WaveTrend oscillator line
- `outWT2` — Simple average of the oscillator line

## Parameters

- `optInChannelPeriod` — Period of the price channel, used by both the average and the deviation
- `optInAveragePeriod` — Smoothing for the oscillator line
- `optInSignalPeriod` — Period of the simple average making the signal line

## Aliases

wavetrend, wavetrend oscillator, WT

## See Also

CCI · SMI · STOCHRSI · TSI

## References

- LazyBear, "WaveTrend Oscillator [WT]", TradingView published script, 2014
