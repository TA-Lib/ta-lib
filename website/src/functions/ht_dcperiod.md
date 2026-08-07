---
title: "Hilbert Transform - Dominant Cycle Period (HT_DCPERIOD)"
description: "Hilbert Transform estimate of the dominant cycle period (in bars) of the price series. Outputs the smoothed instantaneous cycle period."
---

## Summary

Hilbert Transform estimate of the dominant cycle period (in bars) of the price series. Outputs the smoothed instantaneous cycle period. Output is the estimated dominant cycle length in bars (clamped to 6-50).

## Inputs

- `inReal` — Source price/value series

## Outputs

- `outReal` — Smoothed dominant cycle period in bars

## Properties

**Numerical Stability:** [Initial Unstable Period](/functions/stability.md#initial-unstable-period)

| Display<br>Flags |
| :-- |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Overlap Input</span> |
| <span class="flag-box">✅</span> **Independent Y-Axis** <span class="flag-tip" tabindex="0" role="note" aria-label="Output is on its own scale, drawn in a separate pane below the price chart." data-tip="Output is on its own scale, drawn in a separate pane below the price chart.">i</span> |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Candlestick</span> |

## Implementation

TA-Lib Definition: [`ht_dcperiod.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/ht_dcperiod/ht_dcperiod.c) · [`ht_dcperiod.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/ht_dcperiod/ht_dcperiod.yaml)

| Native | File |
|--------|------|
| C | [`ta_HT_DCPERIOD.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_HT_DCPERIOD.c) |
| Rust | [`ht_dcperiod.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/ht_dcperiod.rs) |
| Java | [`Core_HT_DCPERIOD.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/fragments/Core_HT_DCPERIOD.java) |

TA-Lib is also available for Python, R and more using a [wrapper](/install/#wrappers).

## Aliases

Hilbert Transform Dominant Cycle Period, Dominant Cycle Period

## See Also

[HT_DCPHASE](/functions/ht_dcphase.md) · [HT_PHASOR](/functions/ht_phasor.md) · [HT_SINE](/functions/ht_sine.md) · [HT_TRENDMODE](/functions/ht_trendmode.md) · [MAMA](/functions/mama.md) · [WMA](/functions/wma.md)

## References

- John F. Ehlers, *Rocket Science for Traders: Digital Signal Processing Applications*, John Wiley & Sons (ISBN 0471405671)
