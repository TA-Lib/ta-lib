---
title: "Swiss Army Knife - Butterworth Filter (SWAK_BUTTER)"
description: "The Butterworth row of John Ehlers' Swiss Army Knife filter: the Gaussian row's double real pole with two zeros added at Nyquist, giving a two-pole…"
---

## Summary

The Butterworth row of John Ehlers' Swiss Army Knife filter: the Gaussian row's double real pole with two zeros added at Nyquist, giving a two-pole low-pass that cuts the shortest cycles harder than the Gaussian does while keeping the same pole placement.

Read it as a smoothed price line. Its DC gain is 1, so a flat market returns the price itself and the line sits on the chart with price. The two Nyquist zeros are what separate it from `TA_SWAK_GAUSS`: a bar-to-bar alternation, the fastest motion a sampled series can carry, is removed outright rather than merely attenuated, which is why the numerator weights three consecutive bars 1, 2, 1.

The name keeps the `SWAK` prefix on purpose. This is the Swiss Army Knife's Butterworth row, not the Butterworth filters of Ehlers' separate article, and not the SuperSmoother, which is a third two-pole low-pass with complex poles.

## Formula

```
w   = 2 * pi / optInTimePeriod
b2p = 2.415 * (1 - cos(w))
a2p = -b2p + sqrt(b2p^2 + 2 * b2p)

c0  = a2p^2 / 4
a1  = 2 * (1 - a2p)
a2  = -(1 - a2p)^2

y[i] = c0 * (x[i] + 2 * x[i-1] + x[i-2]) + a1 * y[i-1] + a2 * y[i-2]
```

The quarter in `c0` offsets the numerator's total weight of 4, which is what holds the DC gain at 1.

The filter is seeded in the steady state of a constant input equal to its first bar: both input slots and both output slots start at that bar's value.

## Notes

Every term of the recurrence exists at the first bar, so there is no structural lookback and nothing is dropped from the front of the request. What the first bars carry is the seed, which decays rather than ending: set `TA_FUNC_UNST_SWAK_BUTTER` to discard bars until that transient is below whatever matters for the caller.

The output may alias the input. This row reads `x[i-1]` and `x[i-2]`, which an aliased write would already have overwritten, so both are carried in locals and never re-read from the input array.

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
| <span class="flag-box">✅</span> **Uses Transcendental** <span class="flag-tip" tabindex="0" role="note" aria-label="Calls a transcendental math function (exp, log, log10, trigonometric, inverse trigonometric or hyperbolic), so a value may differ slightly between languages, between a stream and a batch call, and between machines. TA-Lib targets a difference within 1e-9." data-tip="Calls a transcendental math function (exp, log, log10, trigonometric, inverse trigonometric or hyperbolic), so a value may differ slightly between languages, between a stream and a batch call, and between machines. TA-Lib targets a difference within 1e-9.">i</span> |

</div>

## Implementation

TA-Lib Definition: [`swak_butter.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/swak_butter/swak_butter.c) · [`swak_butter.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/swak_butter/swak_butter.yaml)

| Native | File |
|--------|------|
| C | [`ta_SWAK_BUTTER.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_SWAK_BUTTER.c) |
| Rust | [`swak_butter.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/swak_butter.rs) |
| Java | [`Core_SWAK_BUTTER.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/fragments/Core_SWAK_BUTTER.java) |
| C# | [`Core_SWAK_BUTTER.cs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/csharp/library/src/Core_SWAK_BUTTER.cs) |

TA-Lib is also available for Python, R and more using a [wrapper](/install/#wrappers).

## Aliases

Swiss Army Knife Butterworth Filter, SWAK Butter, Ehlers Butterworth Filter

## See Also

[SWAK_GAUSS](/functions/swak_gauss.md) · [EMA](/functions/ema.md) · [MEDPRICE](/functions/medprice.md)

## References

- Ehlers, John F. "Swiss Army Knife Indicator." *Technical Analysis of Stocks & Commodities* V.24:1 (January 2006), pp. 28-31, 50-53.
