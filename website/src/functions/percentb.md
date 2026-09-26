---
title: "Bollinger Bands %B (PERCENTB)"
description: "Bollinger Bands %B: where the input sits relative to its Bollinger Bands, 0 at the lower band and 1 at the upper band."
---

## Summary

Bollinger Bands %B: where the input sits relative to its Bollinger Bands, 0 at the lower band and 1 at the upper band. Values below 0 or above 1 mean the input is outside the bands.

## Formula

$$
\%B_t = \frac{X_t - \mathrm{lower}_t}{\mathrm{upper}_t - \mathrm{lower}_t}, \text{ or } 0.5 \text{ when } \mathrm{upper}_t = \mathrm{lower}_t
$$

where $\mathrm{upper}_t$ and $\mathrm{lower}_t$ are the [`BBANDS`](/functions/bbands.md) bands of $X$ with period $n$, moving-average type $\text{matype}$ and deviation multipliers $k_{\text{up}}$, $k_{\text{dn}}$.

## Notes

- An input on the middle band reads `optInNbDevDn / (optInNbDevUp + optInNbDevDn)`: 0.5 with equal multipliers.
- With a simple moving average and both multipliers equal to k, %B = 0.5 + z / (2k), where z is the z-score of the input in its window.
- The result is a ratio; multiply by 100 to read it as a percentage.
- Any `optInMAType` other than SMA is a TA-Lib generalisation, as it is for BBANDS: the deviation stays the population standard deviation about the simple mean.
- PERCENTB is bit for bit `(inReal - lower) / (upper - lower)` computed from BBANDS' own outputs, and 0.5 wherever those two bands are equal.

## Inputs

- `inReal` — Input data series

## Outputs

- `outReal` — Position of the input between the lower band (0) and the upper band (1)

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

TA-Lib Definition: [`percentb.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/percentb/percentb.c) · [`percentb.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/percentb/percentb.yaml)

| Native | File |
|--------|------|
| C | [`ta_PERCENTB.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_PERCENTB.c) |
| Rust | [`percentb.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/percentb.rs) |
| Java | [`Core_PERCENTB.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/fragments/Core_PERCENTB.java) |
| C# | [`Core_PERCENTB.cs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/csharp/library/src/Core_PERCENTB.cs) |

TA-Lib is also available for Python, R and more using a [wrapper](/install/#wrappers).

## Aliases

Bollinger %b, %b, %B, Percent B, BB %B, BBP, Bollinger Bands %B

## See Also

[BBANDS](/functions/bbands.md) · [BBW](/functions/bbw.md) · [STOCHF](/functions/stochf.md)

## References

- John A. Bollinger, *Bollinger on Bollinger Bands*, McGraw-Hill 2001 (ISBN 0071373683)
- John Bollinger, [Bollinger Band Rules](https://www.bollingerbands.com/bollinger-band-rules), rules 15-17
