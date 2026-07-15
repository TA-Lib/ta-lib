---
title: HT_DCPHASE
description: "Hilbert Transform Dominant Cycle Phase: the instantaneous phase (in degrees) of the dominant market cycle, derived from a homodyne discriminator on a Hilbert-transformed, smoothed price. One real output per bar. Output is degrees, wrapped so it never exceeds 315 (can go negative)."
---

# HT_DCPHASE

## Summary

Hilbert Transform Dominant Cycle Phase: the instantaneous phase (in degrees) of the dominant market cycle, derived from a homodyne discriminator on a Hilbert-transformed, smoothed price. One real output per bar. Output is degrees, wrapped so it never exceeds 315 (can go negative).

## Inputs

- `inReal` — Price series to analyze

## Outputs

- `outReal` — Dominant cycle phase in degrees

## Implementation

TA-Lib Definition: [`ht_dcphase.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/ht_dcphase/ht_dcphase.c) · [`ht_dcphase.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/ht_dcphase/ht_dcphase.yaml)

| Native | File |
|--------|------|
| C | [`ta_HT_DCPHASE.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_HT_DCPHASE.c) |
| Rust | [`ht_dcphase.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/ht_dcphase.rs) |
| Java | [`Core_HT_DCPHASE.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/library/fragments/Core_HT_DCPHASE.java) |

TA-Lib is also available for Python, R and more using a [wrapper](https://ta-lib.org/wrappers/).

## Aliases

Hilbert Transform Dominant Cycle Phase

## See Also

[HT_DCPERIOD](/functions/ht_dcperiod) · [HT_PHASOR](/functions/ht_phasor) · [HT_SINE](/functions/ht_sine) · [HT_TRENDLINE](/functions/ht_trendline) · [HT_TRENDMODE](/functions/ht_trendmode) · [MAMA](/functions/mama) · [WMA](/functions/wma)

## References

- John F. Ehlers, *Rocket Science for Traders: Digital Signal Processing Applications*, John Wiley & Sons (ISBN 0471405671)
