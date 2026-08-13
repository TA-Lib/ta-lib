---
title: "Hilbert Transform - Dominant Cycle Phase (HT_DCPHASE)"
description: "Hilbert Transform Dominant Cycle Phase: the instantaneous phase (in degrees) of the dominant market cycle, derived from a homodyne discriminator on a…"
---

## Summary

Hilbert Transform Dominant Cycle Phase: the instantaneous phase (in degrees) of the dominant market cycle, derived from a homodyne discriminator on a Hilbert-transformed, smoothed price. One real output per bar. Output is degrees, wrapped so it never exceeds 315 (can go negative).

## Inputs

- `inReal` — Price series to analyze

## Outputs

- `outReal` — Dominant cycle phase in degrees

## Properties

**Numerical Stability:** [Initial Unstable Period](/functions/stability.md#initial-unstable-period)

<div class="flag-table">

|  |
| :-- |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Overlap Input</span> |
| <span class="flag-box">✅</span> **Independent Y-Axis** <span class="flag-tip" tabindex="0" role="note" aria-label="Output is on its own scale, drawn in a separate pane below the price chart." data-tip="Output is on its own scale, drawn in a separate pane below the price chart.">i</span> |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Candlestick</span> |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Can Output NaN or ±Inf</span> |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Identity at Period 1</span> |

</div>

## Implementation

TA-Lib Definition: [`ht_dcphase.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/ht_dcphase/ht_dcphase.c) · [`ht_dcphase.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/ht_dcphase/ht_dcphase.yaml)

| Native | File |
|--------|------|
| C | [`ta_HT_DCPHASE.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_HT_DCPHASE.c) |
| Rust | [`ht_dcphase.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/ht_dcphase.rs) |
| Java | [`Core_HT_DCPHASE.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/fragments/Core_HT_DCPHASE.java) |

TA-Lib is also available for Python, R and more using a [wrapper](/install/#wrappers).

## Aliases

Hilbert Transform Dominant Cycle Phase

## See Also

[HT_DCPERIOD](/functions/ht_dcperiod.md) · [HT_PHASOR](/functions/ht_phasor.md) · [HT_SINE](/functions/ht_sine.md) · [HT_TRENDLINE](/functions/ht_trendline.md) · [HT_TRENDMODE](/functions/ht_trendmode.md) · [MAMA](/functions/mama.md) · [WMA](/functions/wma.md)

## References

- John F. Ehlers, *Rocket Science for Traders: Digital Signal Processing Applications*, John Wiley & Sons (ISBN 0471405671)
