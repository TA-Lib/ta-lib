---
title: "Lowest value over a specified period (MIN)"
description: "Rolling minimum: the lowest input value over the trailing period."
---

## Summary

Rolling minimum: the lowest input value over the trailing period.

## Formula

outReal[i] = min(inReal[i-optInTimePeriod+1 .. i])

## Inputs

- `inReal` — Source series to take the minimum of

## Outputs

- `outReal` — Lowest input value over the trailing window

## Parameters

| Parameter | Type | Default | Accepted values | Description |
| --- | --- | --- | --- | --- |
| `optInTimePeriod` | integer | 30 | 2–100000 | Number of bars in the trailing window |

## Properties

**Numerical Stability:** [Start-Independent](/functions/stability.md#start-independent)

| Display<br>Flags |
| :-- |
| <span class="flag-box">✅</span> **Overlap Input** <span class="flag-tip" tabindex="0" role="note" aria-label="Output is on the same scale as the input price, so it is drawn over the price chart." data-tip="Output is on the same scale as the input price, so it is drawn over the price chart.">i</span> |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Independent Y-Axis</span> |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Candlestick</span> |

## Implementation

TA-Lib Definition: [`min.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/min/min.c) · [`min.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/min/min.yaml)

| Native | File |
|--------|------|
| C | [`ta_MIN.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_MIN.c) |
| Rust | [`min.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/min.rs) |
| Java | [`Core_MIN.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/fragments/Core_MIN.java) |

TA-Lib is also available for Python, R and more using a [wrapper](/install/#wrappers).

## Aliases

Lowest, Rolling Min, Min Value

## See Also

[MAX](/functions/max.md) · [MININDEX](/functions/minindex.md) · [MINMAX](/functions/minmax.md)
