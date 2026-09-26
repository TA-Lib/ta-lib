---
title: "Bollinger BandWidth (BBW)"
description: "Bollinger BandWidth: the distance between the upper and lower Bollinger Bands, normalised by the middle band."
---

## Summary

Bollinger BandWidth: the distance between the upper and lower Bollinger Bands, normalised by the middle band. Low values mark contracting volatility, the setup John Bollinger calls the Squeeze; high values mark expanding volatility.

## Formula

$$
\mathrm{BBW}_t = \frac{\mathrm{upper}_t - \mathrm{lower}_t}{\mathrm{middle}_t}, \text{ or } 0 \text{ when } \mathrm{middle}_t = 0
$$

where $\mathrm{upper}_t$, $\mathrm{middle}_t$ and $\mathrm{lower}_t$ are the [`BBANDS`](/functions/bbands.md) bands of $X$ with period $n$, moving-average type $\text{matype}$ and deviation multipliers $k_{\text{up}}$, $k_{\text{dn}}$. Since both bands sit on the middle band, $\mathrm{BBW}_t = (k_{\text{up}} + k_{\text{dn}})\,\sigma_t / \mathrm{middle}_t$, with $\sigma_t$ the population standard deviation of the last $n$ values of $X$.

## Notes

- With Bollinger's settings (a simple moving average and two deviations on each side) BBW is four times the window's coefficient of variation: its standard deviation divided by its mean.
- The two deviation multipliers enter only through their sum.
- The result is a ratio; multiply by 100 to read it as a percentage of the middle band.
- Any `optInMAType` other than SMA is a TA-Lib generalisation, as it is for BBANDS: the deviation stays the population standard deviation about the simple mean.
- Wherever the middle band is not 0, BBW is bit for bit `(upper - lower) / middle` computed from BBANDS' own outputs.

## Inputs

- `inReal` — Input data series

## Outputs

- `outReal` — Width of the bands as a fraction of the middle band

## Parameters

| Parameter | Type | Default | Accepted values | Description |
| --- | --- | --- | --- | --- |
| `optInTimePeriod` | integer | 20 | 2–100000 | Periods for the MA and standard deviation |
| `optInNbDevUp` | real | 2 | any real | Standard-deviation multiplier for the upper band |
| `optInNbDevDn` | real | 2 | any real | Standard-deviation multiplier for the lower band |
| `optInMAType` | MAType | SMA (0) | any MAType | Moving-average type for the middle band |

*`MAType` values: 0 SMA · 1 EMA · 2 WMA · 3 DEMA · 4 TEMA · 5 TRIMA · 6 KAMA · 7 MAMA · 8 T3 · 9 HMA · 10 DISABLED · 11 DEFAULT · 12 ZLEMA · 13 RMA*

## Properties

**Numerical Stability:** [Depends on MA Type](/functions/stability.md#depends-on-ma-type) — This function's default, SMA, is start-independent.

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

TA-Lib Definition: [`bbw.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/bbw/bbw.c) · [`bbw.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/bbw/bbw.yaml)

| Native | File |
|--------|------|
| C | [`ta_BBW.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_BBW.c) |
| Rust | [`bbw.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/bbw.rs) |
| Java | [`Core_BBW.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/fragments/Core_BBW.java) |
| C# | [`Core_BBW.cs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/csharp/library/src/Core_BBW.cs) |

TA-Lib is also available for Python, R and more using a [wrapper](/install/#wrappers).

## Aliases

Bollinger BandWidth, Bollinger Band Width, Bollinger Bands Width, BandWidth

## See Also

[BBANDS](/functions/bbands.md) · [PERCENTB](/functions/percentb.md) · [STDDEV](/functions/stddev.md) · [NATR](/functions/natr.md)

## References

- John A. Bollinger, *Bollinger on Bollinger Bands*, McGraw-Hill 2001 (ISBN 0071373683)
- John Bollinger, [Bollinger Band Rules](https://www.bollingerbands.com/bollinger-band-rules), rule 18
