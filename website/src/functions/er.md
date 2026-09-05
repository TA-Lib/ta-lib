---
title: "Kaufman Efficiency Ratio (ER)"
description: "Kaufman Efficiency Ratio (also searched as \"KER\"): Perry J."
---

## Summary

Kaufman Efficiency Ratio (also searched as "KER"): Perry J. Kaufman's noise measure from *Smarter Trading* (1995) — the net directional movement over the period divided by the total path travelled to get there. 1.0 is a perfectly efficient (straight-line) move; values near 0 are churn.

This is exactly the efficiency ratio [`KAMA`](/functions/kama.md) computes internally to set its adaptive smoothing constant, exposed standalone and kept bit-identical to it.

## Formula

`ER[t] = |close[t] − close[t−P]| / Σ |close[k] − close[k−1]|` over the same `P` bars.

Two guards, both shared with `KAMA`: a ratio that floating point would nudge just above 1.0 on a straight-line advance is pinned to exactly 1.0, and a dead-flat window (0/0) also reports 1.0 — a flat market therefore reads as "perfectly efficient", which is `KAMA`'s own convention and what keeps the two reconstructible from each other.

The clamp compares against the *signed* net move, so it only fires on advances: on sustained declines the output may exceed 1.0 by a few ULP. The range is "0..1, may exceed 1 by a few ULP on sustained declines", not a hard bound.

TC2000 documents a signed ×100 variant (−100..+100); the absolute 0..1 form here is the author's, StockCharts', LEAN's, backtrader's and pandas-ta's.

## Inputs

- `inReal` — Source price/value series (canonically close)

## Outputs

- `outReal` — Efficiency ratio

## Parameters

| Parameter | Type | Default | Accepted values | Description |
| --- | --- | --- | --- | --- |
| `optInTimePeriod` | integer | 10 | 2–100000 | Number of one-bar changes in the path sum (default 10, the author's own; note `KAMA`'s `optInTimePeriod` — the same window — defaults to 30) |

## Notes

- First output at index `P` (`P` one-bar changes need `P+1` prices). No unstable period, not start-dependent.

## Properties

**Numerical Stability:** [Start-Independent](/functions/stability.md#start-independent)

<div class="flag-table">

|  |
| :-- |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Overlap Input</span> |
| <span class="flag-box">✅</span> **Independent Y-Axis** <span class="flag-tip" tabindex="0" role="note" aria-label="Output is on its own scale, drawn in a separate pane below the price chart." data-tip="Output is on its own scale, drawn in a separate pane below the price chart.">i</span> |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Candlestick</span> |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Can Output NaN or ±Inf</span> |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Identity at Period 1</span> |

</div>

