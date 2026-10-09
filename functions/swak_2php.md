---
url: 'https://ta-lib.org/functions/swak_2php.md'
description: >-
  The two-pole high-pass row of John Ehlers' Swiss Army Knife filter: the same
  double real pole the Gaussian and Butterworth rows use, with a detrending…
---
# Swiss Army Knife - Two-Pole High-Pass Filter (SWAK_2PHP)

## Summary

The two-pole high-pass row of John Ehlers' Swiss Army Knife filter: the same double real pole the Gaussian and Butterworth rows use, with a detrending numerator instead of a smoothing one. It removes what is slower than the cutoff period and rolls off twice as steeply as the one-pole row.

Read it as an oscillator, not as price. Its DC gain is 0, so a flat market returns zero and a trend returns its departure from itself. Against `TA_SWAK_HP` the difference is the slope of the transition: the second pole buys a sharper separation between what is kept and what is removed.

## Formula

```
w   = 2 * pi / optInTimePeriod
b2p = 2.415 * (1 - cos(w))
a2p = -b2p + sqrt(b2p^2 + 2 * b2p)

c0  = (1 - a2p / 2)^2
a1  = 2 * (1 - a2p)
a2  = -(1 - a2p)^2

y[i] = c0 * (x[i] - 2 * x[i-1] + x[i-2]) + a1 * y[i-1] + a2 * y[i-2]
```

The numerator is zero on a constant, which is what makes the DC gain 0. At Nyquist it weighs 4, and `c0` divides that back to exactly 1.

The filter is seeded in the steady state of a constant input equal to its first bar: both input slots start at that bar and both output slots at zero.

## Notes

Every term of the recurrence exists at the first bar, so there is no structural lookback. What the first bars carry is the seed, which decays rather than ending: set `TA_FUNC_UNST_SWAK_2PHP` to discard bars until that transient is below whatever matters for the caller. The pole is a repeated real root, so the transient decays as `k * (1 - a2p)^k` rather than as a plain geometric.

The output may alias the input. This row reads `x[i-1]` and `x[i-2]`, which an aliased write would already have overwritten, so both are carried in locals and never re-read from the input array.

## Inputs

* `inReal` — The series to detrend; Ehlers' default is the bar midpoint `(H+L)/2`, which the caller passes as `TA_MEDPRICE` output

## Outputs

* `outReal` — The detrended line, centred on zero

## Parameters

| Parameter | Type | Default | Accepted values | Description |
| --- | --- | --- | --- | --- |
| `optInTimePeriod` | integer | 20 | 2–10000 | Cutoff period; cycles longer than it are removed, cycles shorter pass |

## Properties

**Numerical Stability:** [Initial Unstable Period](/functions/stability.md#initial-unstable-period)

<div class="flag-table">

|  |
| :-- |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Overlap Input</span> |
| <span class="flag-box">✅</span> **Independent Y-Axis** <span class="flag-tip" tabindex="0" role="note" aria-label="Output is on its own scale, drawn in a separate pane below the price chart." data-tip="Output is on its own scale, drawn in a separate pane below the price chart.">i</span> |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Candlestick</span> |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Can Output NaN or ±Inf</span> |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Identity at Period 1</span> |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Display Shift</span> |
| <span class="flag-box">✅</span> **Uses Transcendental** <span class="flag-tip" tabindex="0" role="note" aria-label="Calls a transcendental math function (exp, log, log10, trigonometric, inverse trigonometric or hyperbolic), so a value may differ slightly between languages, between a stream and a batch call, and between machines. TA-Lib targets a difference below 1e-9." data-tip="Calls a transcendental math function (exp, log, log10, trigonometric, inverse trigonometric or hyperbolic), so a value may differ slightly between languages, between a stream and a batch call, and between machines. TA-Lib targets a difference below 1e-9.">i</span> |

</div>

## Implementation

TA-Lib Definition: [`swak_2php.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/swak_2php/swak_2php.c) · [`swak_2php.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/swak_2php/swak_2php.yaml)

| Native | File |
|--------|------|
| C | [`ta_SWAK_2PHP.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_SWAK_2PHP.c) |
| Rust | [`swak_2php.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/swak_2php.rs) |
| Java | [`Core_SWAK_2PHP.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/fragments/Core_SWAK_2PHP.java) |
| C# | [`Core_SWAK_2PHP.cs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/csharp/library/src/Core_SWAK_2PHP.cs) |

TA-Lib is also available for Python, R and more using a [wrapper](/install/#wrappers).

## Aliases

Swiss Army Knife Two-Pole High-Pass Filter, SWAK 2PHP, Ehlers Two-Pole High-Pass Filter

## See Also

[SWAK_HP](/functions/swak_hp.md) · [SWAK_BP](/functions/swak_bp.md) · [MEDPRICE](/functions/medprice.md)

## References

* Ehlers, John F. "Swiss Army Knife Indicator." *Technical Analysis of Stocks & Commodities* V.24:1 (January 2006), pp. 28-31, 50-53.
