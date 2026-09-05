# RVOL

## Summary

Relative Volume: today's volume as a ratio to the average volume of the bars that came before it. A value of 1 means the bar traded exactly its recent average, above 1 means unusual participation, below 1 means the move is thin. Because the current bar is excluded from its own baseline, a volume spike shows up at full size instead of being diluted by the average it is compared against.

Read it as a confirmation filter rather than a direction signal: it says how much conviction is behind a price move, not which way. Breakouts on a high ratio are the ones that tend to follow through; the same breakout near 1 is the one to distrust.

## Formula

RVOL_t = Volume_t / ( (1/N) * sum_{i=t-N}^{t-1} Volume_i ), N = optInTimePeriod

## Notes

- The baseline is the mean of the N bars *preceding* the current one, so RVOL needs one bar more than a moving average of the same period before it emits a value.
- A window in which every bar traded nothing has a baseline of zero and no defined ratio: that element is ±Inf, or NaN when the current bar is also zero. Real volume is non-negative, so this only happens on a dead window — an instrument that did not trade at all, or a series carrying no volume, such as a cash-index feed.

## Inputs

- `inVolume` — Volume of each bar

## Outputs

- `outReal` — Ratio of the current bar's volume to the average of the preceding window

## Parameters

- `optInTimePeriod` — Number of preceding bars averaged to form the baseline

## Implementation

TA-Lib Definition: [`rvol.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/rvol/rvol.c) · [`rvol.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/rvol/rvol.yaml)

| Native | File |
|--------|------|
| C | [`ta_RVOL.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_RVOL.c) |
| Rust | [`rvol.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/rvol.rs) |
| Java | [`Core_RVOL.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/fragments/Core_RVOL.java) |

TA-Lib is also available for Python, R and more using a [wrapper](/install/#wrappers).

## Aliases

Relative Volume, RVol

## See Also

OBV · PVO · VWMA · SMA

## References

- RVOL has no attributable originator; it is retail scanner convention, and every published definition agrees on the ratio and on excluding the current bar from its own baseline.
- Reference implementation used as the independent oracle: [trading-signals](https://www.npmjs.com/package/trading-signals), `volume/RVOL`.
