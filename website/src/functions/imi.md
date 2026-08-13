---
title: "Intraday Momentum Index (IMI)"
description: "Intraday Momentum Index: an RSI-like 0-100 oscillator built from the open-to-close body of each bar."
---

## Summary

Intraday Momentum Index: an RSI-like 0-100 oscillator built from the open-to-close body of each bar. Over a rolling window it ratios cumulative up-body moves against total up+down body moves.

## Formula

upsum = Σ(close-open) for bars with close>open; downsum = Σ(open-close) for bars with close<=open, over window [i-lookback, i]; IMI = 100 * upsum/(upsum+downsum)

## Inputs

- `inOpen` — Open price of each bar
- `inClose` — Close price of each bar

## Outputs

- `outReal` — IMI oscillator value, 0-100

## Parameters

| Parameter | Type | Default | Accepted values | Description |
| --- | --- | --- | --- | --- |
| `optInTimePeriod` | integer | 14 | 2–100000 | Rolling window length for the up/down body sums |

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

TA-Lib Definition: [`imi.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/imi/imi.c) · [`imi.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/imi/imi.yaml)

| Native | File |
|--------|------|
| C | [`ta_IMI.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_IMI.c) |
| Rust | [`imi.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/imi.rs) |
| Java | [`Core_IMI.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/fragments/Core_IMI.java) |

TA-Lib is also available for Python, R and more using a [wrapper](/install/#wrappers).

## Aliases

Intraday Momentum Index

## See Also

[RSI](/functions/rsi.md)
