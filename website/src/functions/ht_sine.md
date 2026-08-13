---
title: "Hilbert Transform - SineWave (HT_SINE)"
description: "Hilbert Transform SineWave: derives the dominant-cycle phase from price and emits its sine plus a 45-degree-lead sine."
---

## Summary

Hilbert Transform SineWave: derives the dominant-cycle phase from price and emits its sine plus a 45-degree-lead sine. The two curves cross near cycle turning points. outSine and outLeadSine crossing marks cycle turning points.

## Inputs

- `inReal` — Source price series

## Outputs

- `outSine` — Sine of the dominant-cycle phase
- `outLeadSine` — Sine of the phase advanced 45 degrees (lead)

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

TA-Lib Definition: [`ht_sine.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/ht_sine/ht_sine.c) · [`ht_sine.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/ht_sine/ht_sine.yaml)

| Native | File |
|--------|------|
| C | [`ta_HT_SINE.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_HT_SINE.c) |
| Rust | [`ht_sine.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/ht_sine.rs) |
| Java | [`Core_HT_SINE.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/fragments/Core_HT_SINE.java) |

TA-Lib is also available for Python, R and more using a [wrapper](/install/#wrappers).

## Aliases

Hilbert Transform SineWave, Ehlers SineWave, SineWave Indicator

## See Also

[HT_DCPHASE](/functions/ht_dcphase.md) · [HT_DCPERIOD](/functions/ht_dcperiod.md) · [HT_PHASOR](/functions/ht_phasor.md) · [HT_TRENDMODE](/functions/ht_trendmode.md) · [MAMA](/functions/mama.md)

## References

- John F. Ehlers, *Rocket Science for Traders: Digital Signal Processing Applications*, John Wiley & Sons (ISBN 0471405671)
