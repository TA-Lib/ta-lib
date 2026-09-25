# HT_TRENDMODE

## Summary

Hilbert Transform classifier that labels each bar 1 (trending — favor trend-following) or 0 (cycling — favor mean-reversion). Built from the same MAMA dominant-cycle/phase DSP plus a SineWave/trendline test used across the other HT_* functions.

## Interpretation

Mean-reversion is the trading assumption that price will swing back toward its recent average rather than keep moving in one direction — instead of chasing a breakout, it buys near the low end of the range and sells near the high end, betting on a reversal rather than continuation. A 0 reading is HT_TRENDMODE's signal that this bar fits that regime: price is oscillating rather than trending, so fading the extremes is expected to hold up better than following the move.

## Inputs

- `inReal` — Source price series

## Outputs

- `outInteger` — 1 = trend mode, 0 = cycle mode

## Aliases

Hilbert Transform Trend vs Cycle Mode, Trend Mode

## See Also

HT_TRENDLINE · HT_SINE · HT_DCPHASE · HT_DCPERIOD · MAMA

## References

- John F. Ehlers, *Rocket Science for Traders: Digital Signal Processing Applications*, John Wiley & Sons (ISBN 0471405671)
