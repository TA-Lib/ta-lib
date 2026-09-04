---
title: "Bollinger Bands (BBANDS)"
description: "Bollinger Bands: a moving-average middle band with upper and lower bands offset by a multiple of the standard deviation."
---

## Summary

Bollinger Bands: a moving-average middle band with upper and lower bands offset by a multiple of the standard deviation. Used to gauge relative price volatility.

## Formula

$$
\begin{aligned}
\text{middle}_t &= \operatorname{MA}(X, n, \text{matype})_t \\
\sigma_t &= \operatorname{STDDEV}(X, n)_t \\
\text{upper}_t &= \text{middle}_t + k_{\text{up}}\,\sigma_t \\
\text{lower}_t &= \text{middle}_t - k_{\text{dn}}\,\sigma_t
\end{aligned}
$$

where $X$ is the input series, $n$ the period, $\text{matype}$ the moving-average type,
and $k_{\text{up}}$, $k_{\text{dn}}$ the upper and lower deviation multipliers.

## Notes

- The defaults reproduce Bollinger's original definition: a 20-period SMA middle band with
  $k_{\text{up}} = k_{\text{dn}} = 2$. Any other $\text{matype}$ is a TA-Lib generalisation.
- $\text{matype}$ sets where the envelope is centred; $n$ and $k$ set how wide it is. The two are
  independent — $\sigma$ depends only on the price window, so changing the middle band re-centres
  the bands without resizing them.

## Inputs

- `inReal` — Input data series

## Outputs

- `outRealUpperBand` — Middle band plus nbDevUp standard deviations
- `outRealMiddleBand` — The moving average
- `outRealLowerBand` — Middle band minus nbDevDn standard deviations

## Parameters

| Parameter | Type | Default | Accepted values | Description |
| --- | --- | --- | --- | --- |
| `optInTimePeriod` | integer | 20 | 2–100000 | Periods for the MA and standard deviation |
| `optInNbDevUp` | real | 2 | any real | Standard-deviation multiplier for the upper band |
| `optInNbDevDn` | real | 2 | any real | Standard-deviation multiplier for the lower band |
| `optInMAType` | MAType | SMA (0) | any MAType | Moving-average type for the middle band |

*`MAType` values: 0 SMA · 1 EMA · 2 WMA · 3 DEMA · 4 TEMA · 5 TRIMA · 6 KAMA · 7 MAMA · 8 T3 · 9 HMA · 10 DISABLED · 11 DEFAULT · 12 ZLEMA*

## Properties

**Numerical Stability:** [Depends on MA Type](/functions/stability.md#depends-on-ma-type) — This function's default, SMA, is start-independent.

<div class="flag-table">

|  |
| :-- |
| <span class="flag-box">✅</span> **Overlap Input** <span class="flag-tip" tabindex="0" role="note" aria-label="Output is on the same scale as the input price, so it is drawn over the price chart." data-tip="Output is on the same scale as the input price, so it is drawn over the price chart.">i</span> |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Independent Y-Axis</span> |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Candlestick</span> |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Can Output NaN or ±Inf</span> |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Identity at Period 1</span> |

</div>

## Implementation

TA-Lib Definition: [`bbands.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/bbands/bbands.c) · [`bbands.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/bbands/bbands.yaml)

| Native | File |
|--------|------|
| C | [`ta_BBANDS.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_BBANDS.c) |
| Rust | [`bbands.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/bbands.rs) |
| Java | [`Core_BBANDS.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/fragments/Core_BBANDS.java) |

TA-Lib is also available for Python, R and more using a [wrapper](/install/#wrappers).

## Aliases

Bollinger Bands

## See Also

[MA](/functions/ma.md) · [STDDEV](/functions/stddev.md) · [SMA](/functions/sma.md)

## References

- John A. Bollinger, *Bollinger on Bollinger Bands*, McGraw-Hill Trade (ISBN 0071373683)
