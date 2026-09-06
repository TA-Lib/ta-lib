---
title: "Kaufman Efficiency Ratio (ER)"
description: "Kaufman Efficiency Ratio (also searched as \"KER\"): Perry Kaufman's noise measure from Smarter Trading (1995) — the net directional movement over the…"
---

## Summary

Kaufman Efficiency Ratio (also searched as "KER"): Perry Kaufman's noise measure from *Smarter Trading* (1995) — the net directional movement over the period divided by the total path travelled to get there. 1.0 is a perfectly efficient (straight-line) move; values near 0 are churn.

This is exactly the efficiency ratio [`KAMA`](/functions/kama.md) computes internally to set its adaptive smoothing constant, exposed standalone and kept bit-identical to it.

## Formula

`ER[t] = |close[t] − close[t−P]| / Σ |close[k] − close[k−1]|` over the same `P` bars.

Two guards, both shared with `KAMA`: a ratio that floating point would nudge just above 1.0 on a straight-line advance is pinned to exactly 1.0, and a dead-flat window (0/0) also reports 1.0 — a flat market therefore reads as "perfectly efficient", which is `KAMA`'s own convention and what keeps the two reconstructible from each other.

The output is a hard 0..1 — the net move can never exceed the path travelled.

TC2000 documents a signed ×100 variant (−100..+100); the absolute 0..1 form here is the author's, StockCharts', LEAN's, backtrader's and pandas-ta's.

## Inputs

- `inReal` — Source price/value series (canonically close)

## Outputs

- `outReal` — Efficiency ratio

## Parameters

| Parameter | Type | Default | Accepted values | Description |
| --- | --- | --- | --- | --- |
| `optInTimePeriod` | integer | 10 | 2–100000 | Number of one-bar changes in the path sum (`KAMA`'s `optInTimePeriod` is the same window, under its own default) |

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

## Implementation

TA-Lib Definition: [`er.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/er/er.c) · [`er.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/er/er.yaml)

| Native | File |
|--------|------|
| C | [`ta_ER.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_ER.c) |
| Rust | [`er.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/er.rs) |
| Java | [`Core_ER.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/fragments/Core_ER.java) |

TA-Lib is also available for Python, R and more using a [wrapper](/install/#wrappers).

## Aliases

Efficiency Ratio · Kaufman Efficiency Ratio · KER

## See Also

[KAMA](/functions/kama.md) · [MAMA](/functions/mama.md) · [STDDEV](/functions/stddev.md) · [VHF](/functions/vhf.md)

## References

- Perry Kaufman, *Smarter Trading: Improving Performance in Changing Markets* (McGraw-Hill, 1995) — the efficiency ratio and the adaptive moving average built on it
- Perry Kaufman, *Trading Systems and Methods*, 6th ed. (Wiley, 2019), the "Efficiency Ratio" section
