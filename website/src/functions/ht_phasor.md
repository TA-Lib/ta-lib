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

## Properties

| Numerical<br>Stability | Display<br>Flags |
| :-- | :-- |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Start-Independent</span> <span class="flag-tip" tabindex="0" role="note" aria-label="The value at a bar does not depend on where your data starts — safe to compare across different-length windows." data-tip="The value at a bar does not depend on where your data starts — safe to compare across different-length windows.">i</span> | <span class="flag-box">☐</span> <span style="opacity:0.5">Overlap Input</span> <span class="flag-tip" tabindex="0" role="note" aria-label="Output is on the same scale as the input price, so it is drawn over the price chart." data-tip="Output is on the same scale as the input price, so it is drawn over the price chart.">i</span> |
| <span class="flag-box">✅</span> **Initial Unstable Period** <span class="flag-tip" tabindex="0" role="note" aria-label="Early values depend on how much history precedes them but converge as more bars are supplied; tunable via the unstable period." data-tip="Early values depend on how much history precedes them but converge as more bars are supplied; tunable via the unstable period.">i</span> | <span class="flag-box">✅</span> **Independent Y-Axis** <span class="flag-tip" tabindex="0" role="note" aria-label="Output is on its own scale, drawn in a separate pane below the price chart." data-tip="Output is on its own scale, drawn in a separate pane below the price chart.">i</span> |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Path-Dependent</span> <span class="flag-tip" tabindex="0" role="note" aria-label="Built up from the first bar (a running accumulation or path-tracking state machine): the value depends on where your data begins and never converges — don't compare across different windows." data-tip="Built up from the first bar (a running accumulation or path-tracking state machine): the value depends on where your data begins and never converges — don't compare across different windows.">i</span> | <span class="flag-box">☐</span> <span style="opacity:0.5">Candlestick</span> <span class="flag-tip" tabindex="0" role="note" aria-label="Output is an integer candlestick-pattern signal (e.g. -100 / 0 / +100)." data-tip="Output is an integer candlestick-pattern signal (e.g. -100 / 0 / +100).">i</span> |

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
