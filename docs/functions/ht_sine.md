---
title: HT_SINE
description: "Hilbert Transform SineWave: derives the dominant-cycle phase from price and emits its sine plus a 45-degree-lead sine. The two curves cross near cycle turning points. outSine and outLeadSine crossing marks cycle turning points."
---

# HT_SINE

## Summary

Hilbert Transform SineWave: derives the dominant-cycle phase from price and emits its sine plus a 45-degree-lead sine. The two curves cross near cycle turning points. outSine and outLeadSine crossing marks cycle turning points.

## Inputs

- `inReal` — Source price series

## Outputs

- `outSine` — Sine of the dominant-cycle phase
- `outLeadSine` — Sine of the phase advanced 45 degrees (lead)

## Implementation

TA-Lib Definition: [`ht_sine.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/ht_sine/ht_sine.c) · [`ht_sine.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/ht_sine/ht_sine.yaml)

| Native | File |
|--------|------|
| C | [`ta_HT_SINE.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_HT_SINE.c) |
| Rust | [`ht_sine.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/src/ta_func/ht_sine.rs) |
| Java | [`Core_HT_SINE.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/Core_HT_SINE.java) |

TA-Lib is also available for Python, R and more using a [wrapper](https://ta-lib.org/wrappers/).

## Aliases

Hilbert Transform SineWave, Ehlers SineWave, SineWave Indicator

## See Also

[HT_DCPHASE](/functions/ht_dcphase) · [HT_DCPERIOD](/functions/ht_dcperiod) · [HT_PHASOR](/functions/ht_phasor) · [HT_TRENDMODE](/functions/ht_trendmode) · [MAMA](/functions/mama)

## References

- John F. Ehlers, *Rocket Science for Traders: Digital Signal Processing Applications*, John Wiley & Sons (ISBN 0471405671)
