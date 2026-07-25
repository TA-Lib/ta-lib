---
title: MAMA
description: "MESA Adaptive Moving Average: an adaptive EMA whose smoothing factor is driven by the dominant-cycle phase rate measured with a Hilbert transform. Emits two lines, MAMA and its slower follower FAMA. MAMA crossing above FAMA is bullish; crossing below is bearish."
---

# MAMA

## Summary

MESA Adaptive Moving Average: an adaptive EMA whose smoothing factor is driven by the dominant-cycle phase rate measured with a Hilbert transform. Emits two lines, MAMA and its slower follower FAMA. MAMA crossing above FAMA is bullish; crossing below is bearish.

## Formula

phase = atan(Q1/I1) in degrees; deltaPhase = max(1, prevPhase - phase)
alpha = max(fastLimit/deltaPhase, slowLimit) if deltaPhase>1 else fastLimit
MAMA = alpha*price + (1-alpha)*MAMA_prev
FAMA = (alpha/2)*MAMA + (1-alpha/2)*FAMA_prev

## Inputs

- `inReal` — Price series to smooth

## Outputs

- `outMAMA` — Adaptive moving average (fast line)
- `outFAMA` — Following adaptive moving average, using half the alpha (slow line)

## Parameters

| Parameter | Type | Default | Accepted values | Description |
| --- | --- | --- | --- | --- |
| `optInFastLimit` | real | 0.5 | 0.01–0.99 | Upper bound on the adaptive smoothing factor |
| `optInSlowLimit` | real | 0.05 | 0.01–0.99 | Lower bound on the adaptive smoothing factor |

## Properties

**Numerical Stability:** [Initial Unstable Period](/functions/stability#initial-unstable-period)

| Display<br>Flags |
| :-- |
| <span class="flag-box">✅</span> **Overlap Input** <span class="flag-tip" tabindex="0" role="note" aria-label="Output is on the same scale as the input price, so it is drawn over the price chart." data-tip="Output is on the same scale as the input price, so it is drawn over the price chart.">i</span> |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Independent Y-Axis</span> |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Candlestick</span> |

## Implementation

TA-Lib Definition: [`mama.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/mama/mama.c) · [`mama.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/mama/mama.yaml)

| Native | File |
|--------|------|
| C | [`ta_MAMA.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_MAMA.c) |
| Rust | [`mama.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/mama.rs) |
| Java | [`Core_MAMA.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/library/fragments/Core_MAMA.java) |

TA-Lib is also available for Python, R and more using a [wrapper](https://ta-lib.org/wrappers/).

## Aliases

MESA Adaptive Moving Average, Ehlers MAMA

## See Also

[MA](/functions/ma) · [WMA](/functions/wma) · [HT_DCPERIOD](/functions/ht_dcperiod)

## References

- John F. Ehlers, *Rocket Science for Traders: Digital Signal Processing Applications*, John Wiley & Sons (ISBN 0471405671)
