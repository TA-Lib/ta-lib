---
title: "Percentile (nearest rank) (PERCENTILE)"
description: "Rolling percentile by the nearest-rank method: sort the trailing window ascending and report the value whose 1-based ordinal rank is the P-th percentile…"
---

## Summary

Rolling percentile by the nearest-rank method: sort the trailing window ascending and report the value whose 1-based ordinal rank is the P-th percentile of the window size. The result is always a value that actually occurred in the window, never an interpolation, so it stays on the price scale and never invents a level the series never traded at. At P = 50 with an odd window it is the rolling median; at the extremes it degenerates to the rolling minimum and maximum.

## Formula

$W_t = \operatorname{sort}(x_{t-N+1}, \dots, x_t)$; $k = \left\lceil \frac{P \cdot N}{100} \right\rceil$ clamped to $[1, N]$; $PERCENTILE_t = W_t[k]$ (N = optInTimePeriod, P = optInPercentile, $W_t[1]$ the smallest)

## Notes

- The nearest-rank method is one of several incompatible percentile conventions. The linear-interpolation family (Hyndman & Fan type 7, the default of most statistical packages, and TradingView's `ta.percentile_linear_interpolation`) reports a weighted blend of two neighbouring order statistics and can emit a value that never occurred. That is a different indicator, not a mode of this one: PERCENTILE's parameter list is fixed at a window and a percentage, and a method selector cannot be appended to it later without changing the function's arity.
- Every input value in the window must be finite. A NaN makes every comparison against it false, which breaks the ordering the rank index is read from.

## Inputs

- `inReal` — Source series to take the percentile of

## Outputs

- `outReal` — The value at the requested rank within the trailing window

## Parameters

| Parameter | Type | Default | Accepted values | Description |
| --- | --- | --- | --- | --- |
| `optInTimePeriod` | integer | 30 | 2–100000 | Number of bars in the trailing window |
| `optInPercentile` | real | 50 | 0–100 | Percentage position within the sorted window |

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

## Implementation

TA-Lib Definition: [`percentile.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/percentile/percentile.c) · [`percentile.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/percentile/percentile.yaml)

| Native | File |
|--------|------|
| C | [`ta_PERCENTILE.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_PERCENTILE.c) |
| Rust | [`percentile.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/percentile.rs) |
| Java | [`Core_PERCENTILE.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/fragments/Core_PERCENTILE.java) |

TA-Lib is also available for Python, R and more using a [wrapper](/install/#wrappers).

## Aliases

Percentile Nearest Rank, Rolling Percentile, Rolling Quantile, Rolling Median

## See Also

[MIN](/functions/min.md) · [MAX](/functions/max.md) · [MEDPRICE](/functions/medprice.md) · [STDDEV](/functions/stddev.md)

## References

- [Percentile — nearest-rank method](https://en.wikipedia.org/wiki/Percentile), the ordinal rank definition this function implements.
- Rob J. Hyndman and Yanan Fan, "Sample Quantiles in Statistical Packages", *The American Statistician* 50(4), 1996 — the nearest-rank rule is their type 1.
