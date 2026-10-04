---
title: "Swiss Army Knife - Gaussian Filter (SWAK_GAUSS)"
description: "The Gaussian row of John Ehlers' Swiss Army Knife filter: a two-pole low-pass smoother whose impulse response approximates a Gaussian, so it lags price…"
---

## Summary

The Gaussian row of John Ehlers' Swiss Army Knife filter: a two-pole low-pass smoother whose impulse response approximates a Gaussian, so it lags price without the overshoot a sharper filter leaves behind a turn.

Read it as a smoothed price line. Its DC gain is 1, so a flat market returns the price itself and the line sits on the chart with price rather than oscillating around zero. The cutoff period sets how much is removed: cycles far shorter than it are attenuated smoothly, cycles far longer pass essentially untouched, and there is no band in between where the filter rings.

The alpha is the one the Gaussian construction asks for, not the exponential-moving-average alpha. Ehlers notes that the same shape can be had by taking an EMA of an EMA, but that doing so "leaves the computation of the correct alpha to be a little nebulous", and gives these coefficients instead. `TA_EMA` cannot stand in: its alpha is `2/(n+1)` for whole `n`, and at a cutoff period of 20 this filter's alpha is 0.38217, which would ask for `n = 4.233`.

## Formula

```
w   = 2 * pi / optInTimePeriod
b2p = 2.415 * (1 - cos(w))
a2p = -b2p + sqrt(b2p^2 + 2 * b2p)

c0  = a2p^2
a1  = 2 * (1 - a2p)
a2  = -(1 - a2p)^2

y[i] = c0 * x[i] + a1 * y[i-1] + a2 * y[i-2]
```

The filter is seeded in the steady state of a constant input equal to its first bar: both output slots start at that bar's value, which is what a gain-1 row answers for a constant.

## Notes

`SWAK` is the abbreviation the trading platforms use for this filter, and the suffix is Ehlers' own label for the row. The prefix is load-bearing rather than decorative: the Swiss Army Knife's Butterworth row keeps this same double real pole and adds two zeros at Nyquist, while the Gaussian paper contrasts it with the Butterworth filters of a separate article, and the SuperSmoother is a third two-pole low-pass with complex poles. A bare `GAUSS` would claim all of them.

Every term of the recurrence exists at the first bar, so there is no structural lookback and nothing is dropped from the front of the request. What the first bars do carry is the seed, which decays rather than ending: set `TA_FUNC_UNST_SWAK_GAUSS` to discard bars until that transient is below whatever matters for the caller.

The output may alias the input. This row's `b1` and `b2` are zero, so it never re-reads an earlier input slot that an aliased write would already have overwritten.

## Inputs

- `inReal` — The series to filter; Ehlers' default is the bar midpoint `(H+L)/2`, which the caller passes as `TA_MEDPRICE` output

## Outputs

- `outReal` — The filtered line, on the same scale as the input

## Parameters

| Parameter | Type | Default | Accepted values | Description |
| --- | --- | --- | --- | --- |
| `optInTimePeriod` | integer | 20 | 2–10000 | Cutoff period; shorter keeps more of the fast motion, longer smooths harder and lags more |

## Properties

**Numerical Stability:** [Initial Unstable Period](/functions/stability.md#initial-unstable-period)

<div class="flag-table">

|  |
| :-- |
| <span class="flag-box">✅</span> **Overlap Input** <span class="flag-tip" tabindex="0" role="note" aria-label="Output is on the same scale as the input price, so it is drawn over the price chart." data-tip="Output is on the same scale as the input price, so it is drawn over the price chart.">i</span> |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Independent Y-Axis</span> |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Candlestick</span> |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Can Output NaN or ±Inf</span> |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Identity at Period 1</span> |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Display Shift</span> |

</div>

## Implementation

TA-Lib Definition: [`swak_gauss.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/swak_gauss/swak_gauss.c) · [`swak_gauss.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/swak_gauss/swak_gauss.yaml)

| Native | File |
|--------|------|
| C | [`ta_SWAK_GAUSS.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_SWAK_GAUSS.c) |
| Rust | [`swak_gauss.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/swak_gauss.rs) |
| Java | [`Core_SWAK_GAUSS.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/fragments/Core_SWAK_GAUSS.java) |
| C# | [`Core_SWAK_GAUSS.cs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/csharp/library/src/Core_SWAK_GAUSS.cs) |

TA-Lib is also available for Python, R and more using a [wrapper](/install/#wrappers).

## Aliases

Swiss Army Knife Gaussian Filter, SWAK Gauss, Ehlers Gaussian Filter

## See Also

[SWAK_BUTTER](/functions/swak_butter.md) · [EMA](/functions/ema.md) · [MEDPRICE](/functions/medprice.md)

## References

- Ehlers, John F. "Swiss Army Knife Indicator." *Technical Analysis of Stocks & Commodities* V.24:1 (January 2006), pp. 28-31, 50-53.
