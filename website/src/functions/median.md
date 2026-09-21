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

- This is not [`PERCENTILE`](/functions/percentile.md) at 50. `PERCENTILE` reports the nearest rank, which at even `n` selects the **lower** of the two central values: on a 4-bar window of `1, 2, 3, 4` this function returns `2.5` and `PERCENTILE` returns `2`. At odd `n` the two agree bit for bit.
- At even `n` the output can therefore be a value the series never traded at, which is the deliberate opposite of `PERCENTILE`'s design property.
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

