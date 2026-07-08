# HT_TRENDMODE

## Summary

Hilbert Transform classifier that labels each bar as trending (1) or cycling (0). Reuses the MAMA dominant-cycle/phase DSP plus a SineWave/trendline test to decide the market mode. 1 = trending market (favor trend-following); 0 = cycle/mean-reverting mode.

## Inputs

- `inReal` — Source price series

## Outputs

- `outInteger` — 1 = trend mode, 0 = cycle mode

## Implementation

TA-Lib Definition: [`ht_trendmode.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/ht_trendmode/ht_trendmode.c) · [`ht_trendmode.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/ht_trendmode/ht_trendmode.yaml)

| Native | File |
|--------|------|
| C | [`ta_HT_TRENDMODE.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_HT_TRENDMODE.c) |
| Rust | [`ht_trendmode.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/src/ta_func/ht_trendmode.rs) |
| Java | [`Core_HT_TRENDMODE.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/Core_HT_TRENDMODE.java) |

TA-Lib is also available for Python, R and more using a [wrapper](https://ta-lib.org/wrappers/).

## Aliases

Hilbert Transform Trend vs Cycle Mode, Trend Mode

## See Also

HT_TRENDLINE · HT_SINE · HT_DCPHASE · HT_DCPERIOD · MAMA

## References

- John F. Ehlers, *Rocket Science for Traders: Digital Signal Processing Applications*, John Wiley & Sons (ISBN 0471405671)
