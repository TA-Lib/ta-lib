---
title: "Midpoint Price over period (MIDPRICE)"
description: "Midpoint of the price range over a rolling window: the average of the highest high and lowest low across the last optInTimePeriod bars."
---

## Summary

Midpoint of the price range over a rolling window: the average of the highest high and lowest low across the last optInTimePeriod bars. An overlap-study line plotted on price.

## Formula

MIDPRICE = (Highest(High, N) + Lowest(Low, N)) / 2, over the N=optInTimePeriod bars ending at each index

## Inputs

- `inHigh` — High price of each bar
- `inLow` — Low price of each bar

## Outputs

- `outReal` — Midpoint of the period's high/low extremes

## Parameters

| Parameter | Type | Default | Accepted values | Description |
| --- | --- | --- | --- | --- |
| `optInTimePeriod` | integer | 14 | 2–100000 | Window length over which the high/low extremes are taken |

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

TA-Lib Definition: [`midprice.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/midprice/midprice.c) · [`midprice.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/midprice/midprice.yaml)

| Native | File |
|--------|------|
| C | [`ta_MIDPRICE.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_MIDPRICE.c) |
| Rust | [`midprice.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/midprice.rs) |
| Java | [`Core_MIDPRICE.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/fragments/Core_MIDPRICE.java) |

TA-Lib is also available for Python, R and more using a [wrapper](/install/#wrappers).

## Aliases

Midpoint Price

## See Also

[MIDPOINT](/functions/midpoint.md) · [MEDPRICE](/functions/medprice.md)
