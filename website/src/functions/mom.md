---
title: "Momentum (MOM)"
description: "Momentum: current price minus the price optInTimePeriod bars ago. The absolute (unnormalized) rate of change."
---

## Summary

Momentum: current price minus the price optInTimePeriod bars ago. The absolute (unnormalized) rate of change. Positive = price rose over the period, negative = fell; centered at zero.

## Chart

<ta-chart src="/functions/mom.svg" ratio="720:416" alt="Drawn on the price pane over the close as a line; below it, a separate indicator pane with a zero line showing outReal as a solid line."></ta-chart>

Drawn on the price pane over the close as a line; below it, a separate indicator pane with a zero line showing outReal as a solid line. [Reference data (CSV)](/functions/mom.csv) · [Chart (SVG)](/functions/mom.svg)

## Formula

MOM[i] = inReal[i] - inReal[i - optInTimePeriod]

## Inputs

- `inReal` — Input price series

## Outputs

- `outReal` — Momentum (current minus value optInTimePeriod bars ago)

## Parameters

| Parameter | Type | Default | Accepted values | Description |
| --- | --- | --- | --- | --- |
| `optInTimePeriod` | integer | 10 | 1–100000 | Lookback distance in bars |

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
| <span class="flag-box">☐</span> <span style="opacity:0.5">Display Shift</span> |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Uses Transcendental</span> |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Input Must Be Positive</span> |

</div>

## Implementation

Added in 0.4.0.

TA-Lib Definition: [`mom.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/mom/mom.c) · [`mom.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/mom/mom.yaml)

| Native | File |
|--------|------|
| C | [`ta_MOM.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_MOM.c) |
| Rust | [`mom.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/mom.rs) |
| Java | [`Core_MOM.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/fragments/Core_MOM.java) |
| C# | [`Core_MOM.cs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/csharp/library/src/Core_MOM.cs) |
| TypeScript | [`mom.ts`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/ts/library/src/func/mom.ts) |

Python: `talib.MOM`, see [ta-lib-python](https://ta-lib.github.io/ta-lib-python/func_groups/momentum_indicators.html).

TA-Lib is also available for R and more using a [wrapper](/install/#wrappers).

## Aliases

Momentum

## See Also

[ROC](/functions/roc.md) · [ROCP](/functions/rocp.md) · [ROCR](/functions/rocr.md) · [ROCR100](/functions/rocr100.md)
