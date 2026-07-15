---
title: HT_PHASOR
description: "Hilbert Transform indicator that decomposes the price series into its in-phase (I) and quadrature (Q) phasor components. Shares the same detrend/Hilbert machinery as the other HT_* cycle functions."
---

# HT_PHASOR

## Summary

Hilbert Transform indicator that decomposes the price series into its in-phase (I) and quadrature (Q) phasor components. Shares the same detrend/Hilbert machinery as the other HT_* cycle functions.

## Formula

Smooth price with a 4-bar WMA (weights 1,2,3,4 /10). Apply the Hilbert Transform (a=0.0962, b=0.5769, scaled per bar by adjustedPrevPeriod = 0.075*period + 0.54) to get detrender = HT(smoothed) and Q1 = HT(detrender). Output: outInPhase = detrender delayed 3 price bars; outQuadrature = Q1.

## Inputs

- `inReal` — Source price series

## Outputs

- `outInPhase` — In-phase component (detrender delayed 3 bars)
- `outQuadrature` — Quadrature component (Q1 of the Hilbert Transform)

## Implementation

TA-Lib Definition: [`ht_phasor.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/ht_phasor/ht_phasor.c) · [`ht_phasor.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/ht_phasor/ht_phasor.yaml)

| Native | File |
|--------|------|
| C | [`ta_HT_PHASOR.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_HT_PHASOR.c) |
| Rust | [`ht_phasor.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/ht_phasor.rs) |
| Java | [`Core_HT_PHASOR.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/library/fragments/Core_HT_PHASOR.java) |

TA-Lib is also available for Python, R and more using a [wrapper](https://ta-lib.org/wrappers/).

## Aliases

Hilbert Transform Phasor, InPhase Quadrature

## See Also

[HT_DCPERIOD](/functions/ht_dcperiod) · [HT_DCPHASE](/functions/ht_dcphase) · [HT_SINE](/functions/ht_sine) · [HT_TRENDMODE](/functions/ht_trendmode) · [MAMA](/functions/mama) · [WMA](/functions/wma)

## References

- John F. Ehlers, *Rocket Science for Traders: Digital Signal Processing Applications*, John Wiley & Sons (ISBN 0471405671)
