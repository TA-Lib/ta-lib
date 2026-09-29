# VIDYA

## Summary

Variable Index Dynamic Average (Tushar Chande): an EMA whose smoothing factor is scaled every bar by the absolute value of the Chande Momentum Oscillator. It follows the price like an EMA in a one-way move and stops moving when up and down moves balance, so it flattens out in consolidations.

## Formula

n = optInTimePeriod, m = optInCMOPeriod, alpha = 2 / (n + 1)

CMO[t] = the unsmoothed Chande Momentum Oscillator over the last m price changes (`CMOU`)

k[t] = alpha * |CMO[t]| / 100

VIDYA[t] = VIDYA[t-1] + k[t] * (x[t] - VIDYA[t-1])

The recursion starts from the first price. Until m price changes are available, the CMO is taken over the changes seen so far. The first output is at bar m.

## Notes

- A period of 1 performs no smoothing: the output is a copy of the input, consistent with `MA(period=1)` for every MAType.
- Chande's 1992 article drives the same step with a ratio of standard deviations; this is his 1995 form, driven by the CMO.
- The CMO is Chande's unsmoothed one. An implementation driven by a Wilder-smoothed CMO (`CMO`) computes a different line that does not converge to this one.
- Being recursive, an output depends on how much history precedes it, and the seed's influence decays more slowly the closer the CMO stays to 0. Implementations that seed differently agree with this one only once that influence has decayed.
- As an `MA` type, the one period is n and the CMO period is (3n + 2) / 4 in integer division, Chande's 12:9 ratio.

## Inputs

- `inReal` — Data on which to compute the average

## Outputs

- `outReal` — Variable Index Dynamic Average line

## Parameters

- `optInTimePeriod` — The EMA length whose alpha the CMO scales
- `optInCMOPeriod` — Number of trailing price changes in the CMO

## Aliases

Variable Index Dynamic Average, VIDyA, Chande's Variable Index Dynamic Average

## See Also

CMOU · EMA · KAMA · MA

## References

- Tushar S. Chande, "Adapting Moving Averages To Market Volatility", *Technical Analysis of Stocks & Commodities* V.10:3 (March 1992). The adaptive EMA and its volatility index.
- Tushar S. Chande, "Identifying Powerful Breakouts Early", *Technical Analysis of Stocks & Commodities* V.13:10 (October 1995). The CMO as the volatility index, with the 12 and 9 periods.
