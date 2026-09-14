---
title: "Rolling Median (MEDIAN)"
description: "The middle order statistic of the trailing window: the central value when optInTimePeriod is odd, the mean of the two central values when it is even."
---

## Summary

The middle order statistic of the trailing window: the central value when `optInTimePeriod` is odd, the mean of the two central values when it is even. A robust measure of central tendency — unlike [`SMA`](/functions/sma.md) it is unmoved by a single spike, which is what makes it useful as a filter rather than as a level.

Not to be confused with [`MEDPRICE`](/functions/medprice.md), which is `(High + Low) / 2` of one bar and is not an order statistic.

## Formula

With `W` the window sorted ascending and `W[1]` its smallest value:

    n odd :   MEDIAN = W[(n+1)/2]
    n even:   MEDIAN = ( W[n/2] + W[n/2 + 1] ) / 2

## Notes

- **This is not [`PERCENTILE`](/functions/percentile.md) at 50.** `PERCENTILE` reports the nearest rank, `ceil(P·n/100)` clamped to the window. At odd `n` that ordinal *is* the median and the two functions agree bit for bit. At even `n`, `ceil(n/2) = n/2` selects the **lower** of the two central values, which is not the median: on a 4-bar window of `1, 2, 3, 4` this function returns `2.5` and `PERCENTILE` returns `2`.
- **At even `n` the output can be a value the series never traded at.** That is the deliberate opposite of `PERCENTILE`'s design property, and it is the whole reason the two are separate functions rather than one with a mode selector — `PERCENTILE`'s parameter list is fixed at a window and a percentage, and a method selector cannot be appended to it without changing its arity.
- **The even-`n` mean is not a variant to choose.** NumPy, R's `median()`, scipy and Excel's `MEDIAN` all take it, and there is no original author to arbitrate against. Nor is the spelling a variant: `(lo + hi) / 2.0` and `0.5 * (lo + hi)` are the same double.
- **The odd case is a branch, not `(v + v) / 2`.** The arithmetic form is exact for any value this library is realistically handed, but it overflows above `DBL_MAX/2`, and this function does not declare `nan_inf_output`. The branch is loop-invariant and costs nothing. At even `n` the sum of the two central values can still overflow near `DBL_MAX`, which is what NumPy does with such inputs as well.
- **Equal values keep insertion order.** The window is carried twice, once by age and once by value, and the by-value copy shifts only strictly greater entries on insertion. A run of equal values therefore stays in age order, which is what lets the removal evict the oldest of the run by value alone, with no slot array.
- `optInTimePeriod` is not restricted to odd values: TA-Lib has no odd-only range mechanism, and it would surprise any caller reaching for a 20-bar median.

## Inputs

- `inReal` — The series to take the median of

## Outputs

- `outReal` — Median of the trailing window

## Parameters

| Parameter | Type | Default | Accepted values | Description |
| --- | --- | --- | --- | --- |
| `optInTimePeriod` | integer | 30 | 2–100000 | Number of trailing values in the window |

## See Also

[PERCENTILE](/functions/percentile.md) · [SMA](/functions/sma.md) · [MEDPRICE](/functions/medprice.md)

## References

- NumPy `numpy.median`, R `stats::median`, scipy, Excel `MEDIAN` — four independent implementations of one unambiguous definition.

## Properties

**Numerical Stability:** [Start-Independent](/functions/stability.md#start-independent)

<div class="flag-table">

|  |
| :-- |
| <span class="flag-box">✅</span> **Overlap Input** <span class="flag-tip" tabindex="0" role="note" aria-label="Output is on the same scale as the input price, so it is drawn over the price chart." data-tip="Output is on the same scale as the input price, so it is drawn over the price chart.">i</span> |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Independent Y-Axis</span> |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Candlestick</span> |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Can Output NaN or ±Inf</span> |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Identity at Period 1</span> |

</div>

