---
title: SUM
description: "Rolling sum of the input over a fixed period. Each output is the sum of the most recent optInTimePeriod input values."
---

# SUM

## Summary

Rolling sum of the input over a fixed period. Each output is the sum of the most recent optInTimePeriod input values.

## Formula

$out_i = \sum_{j=i-(N-1)}^{i} inReal_j$, N = optInTimePeriod

## Inputs

- `inReal` — Values to sum

## Outputs

- `outReal` — Windowed sum over the period

## Parameters

| Parameter | Type | Default | Accepted values | Description |
| --- | --- | --- | --- | --- |
| `optInTimePeriod` | integer | 30 | 2–100000 | Window length summed |

## Properties

**Numerical Stability:** [Start-Independent](/functions/stability#start-independent)

| Display<br>Flags |
| :-- |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Overlap Input</span> |
| <span class="flag-box">✅</span> **Independent Y-Axis** <span class="flag-tip" tabindex="0" role="note" aria-label="Output is on its own scale, drawn in a separate pane below the price chart." data-tip="Output is on its own scale, drawn in a separate pane below the price chart.">i</span> |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Candlestick</span> |

## Implementation

TA-Lib Definition: [`sum.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/sum/sum.c) · [`sum.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/sum/sum.yaml)

| Native | File |
|--------|------|
| C | [`ta_SUM.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_SUM.c) |
| Rust | [`sum.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/sum.rs) |
| Java | [`Core_SUM.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/fragments/Core_SUM.java) |

TA-Lib is also available for Python, R and more using a [wrapper](https://ta-lib.org/wrappers/).

## Aliases

Summation, Rolling Sum, Moving Sum

## See Also

[SMA](/functions/sma)
