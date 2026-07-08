---
title: HT_DCPERIOD
description: "Hilbert Transform estimate of the dominant cycle period (in bars) of the price series. Outputs the smoothed instantaneous cycle period. Output is the estimated dominant cycle length in bars (clamped to 6-50)."
---

# HT_DCPERIOD

## Summary

Hilbert Transform estimate of the dominant cycle period (in bars) of the price series. Outputs the smoothed instantaneous cycle period. Output is the estimated dominant cycle length in bars (clamped to 6-50).

## Inputs

- `inReal` — Source price/value series

## Outputs

- `outReal` — Smoothed dominant cycle period in bars

## Implementation

TA-Lib Definition: [`ht_dcperiod.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/ht_dcperiod/ht_dcperiod.c) · [`ht_dcperiod.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/ht_dcperiod/ht_dcperiod.yaml)

| Native | File |
|--------|------|
| C | [`ta_HT_DCPERIOD.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_HT_DCPERIOD.c) |
| Rust | [`ht_dcperiod.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/src/ta_func/ht_dcperiod.rs) |
| Java | [`Core_HT_DCPERIOD.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/Core_HT_DCPERIOD.java) |

TA-Lib is also available for Python, R and more using a [wrapper](https://ta-lib.org/wrappers/).

## Aliases

Hilbert Transform Dominant Cycle Period, Dominant Cycle Period

## See Also

[HT_DCPHASE](ht_dcphase.md) · [HT_PHASOR](ht_phasor.md) · [HT_SINE](ht_sine.md) · [HT_TRENDMODE](ht_trendmode.md) · [MAMA](mama.md) · [WMA](wma.md)

## References

- John F. Ehlers, *Rocket Science for Traders: Digital Signal Processing Applications*, John Wiley & Sons (ISBN 0471405671)
