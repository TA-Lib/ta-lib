# HT_TRENDMODE

## Summary

Hilbert Transform classifier that labels each bar 1 (trending — favor trend-following) or 0 (cycling — favor mean-reversion). Built from the same MAMA dominant-cycle/phase DSP plus a SineWave/trendline test used across the other HT_* functions.

## Interpretation

Mean-reversion is the trading assumption that price will swing back toward its recent average rather than keep moving in one direction — instead of chasing a breakout, it buys near the low end of the range and sells near the high end, betting on a reversal rather than continuation. A 0 reading is HT_TRENDMODE's signal that this bar fits that regime: price is oscillating rather than trending, so fading the extremes is expected to hold up better than following the move.

## Inputs

- `inReal` — Source price series

## Outputs

- `outInteger` — 1 = trend mode, 0 = cycle mode

## Implementation

TA-Lib Definition: [`ht_trendmode.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/ht_trendmode/ht_trendmode.c) · [`ht_trendmode.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/ht_trendmode/ht_trendmode.yaml)

| Native | File |
|--------|------|
| C | [`ta_HT_TRENDMODE.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_HT_TRENDMODE.c) |
| Rust | [`ht_trendmode.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/ht_trendmode.rs) |
| Java | [`Core_HT_TRENDMODE.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/fragments/Core_HT_TRENDMODE.java) |

TA-Lib is also available for Python, R and more using a [wrapper](/install/#wrappers).

## Aliases

Hilbert Transform Trend vs Cycle Mode, Trend Mode

## See Also

HT_TRENDLINE · HT_SINE · HT_DCPHASE · HT_DCPERIOD · MAMA

## References

- John F. Ehlers, *Rocket Science for Traders: Digital Signal Processing Applications*, John Wiley & Sons (ISBN 0471405671)
