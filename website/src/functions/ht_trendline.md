---
title: HT_TRENDLINE
description: "Ehlers' Hilbert Transform Instantaneous Trendline: a smoothed, low-lag overlay whose averaging window adapts to the dominant cycle period measured via Hilbert-transform quadrature (I/Q) analysis of price."
---

# HT_TRENDLINE

## Summary

Ehlers' Hilbert Transform Instantaneous Trendline: a smoothed, low-lag overlay whose averaging window adapts to the dominant cycle period measured via Hilbert-transform quadrature (I/Q) analysis of price.

## Inputs

- `inReal` — Source price series

## Outputs

- `outReal` — Instantaneous trendline value

## Properties

**Numerical Stability:** [Initial Unstable Period](/functions/stability#initial-unstable-period)

| Display<br>Flags |
| :-- |
| <span class="flag-box">✅</span> **Overlap Input** <span class="flag-tip" tabindex="0" role="note" aria-label="Output is on the same scale as the input price, so it is drawn over the price chart." data-tip="Output is on the same scale as the input price, so it is drawn over the price chart.">i</span> |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Independent Y-Axis</span> |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Candlestick</span> |

## Implementation

TA-Lib Definition: [`ht_trendline.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/ht_trendline/ht_trendline.c) · [`ht_trendline.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/ht_trendline/ht_trendline.yaml)

| Native | File |
|--------|------|
| C | [`ta_HT_TRENDLINE.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_HT_TRENDLINE.c) |
| Rust | [`ht_trendline.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/ht_trendline.rs) |
| Java | [`Core_HT_TRENDLINE.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/fragments/Core_HT_TRENDLINE.java) |

TA-Lib is also available for Python, R and more using a [wrapper](https://ta-lib.org/wrappers/).

## Aliases

Hilbert Transform Instantaneous Trendline, Instantaneous Trendline

## See Also

[HT_DCPERIOD](/functions/ht_dcperiod) · [HT_PHASOR](/functions/ht_phasor) · [MAMA](/functions/mama) · [WMA](/functions/wma)

## References

- John F. Ehlers, *Rocket Science for Traders: Digital Signal Processing Applications*, John Wiley & Sons (ISBN 0471405671)
