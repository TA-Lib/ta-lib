---
title: WMA
description: "Linearly weighted moving average: each of the last N prices is weighted by its position, oldest getting weight 1 and newest weight N. Smooths price while emphasizing recent bars."
---

# WMA

## Summary

Linearly weighted moving average: each of the last N prices is weighted by its position, oldest getting weight 1 and newest weight N. Smooths price while emphasizing recent bars.

## Formula

WMA = ( sum_{k=1..N} k * P_k ) / (N(N+1)/2), where P_N is the most recent bar

## Notes

- A period of 1 performs no smoothing: the output is a copy of the input. Allowed since 0.6.5 (issues #48/#59).

## Inputs

- `inReal` — Source price/data series

## Outputs

- `outReal` — Weighted moving average series

## Parameters

| Parameter | Type | Default | Accepted values | Description |
| --- | --- | --- | --- | --- |
| `optInTimePeriod` | integer | 30 | 1–100000 | Number of bars in the weighting window |

## Properties

**Numerical Stability:** [Start-Independent](/functions/stability#start-independent)

| Display<br>Flags |
| :-- |
| <span class="flag-box">✅</span> **Overlap Input** <span class="flag-tip" tabindex="0" role="note" aria-label="Output is on the same scale as the input price, so it is drawn over the price chart." data-tip="Output is on the same scale as the input price, so it is drawn over the price chart.">i</span> |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Independent Y-Axis</span> |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Candlestick</span> |

## Implementation

TA-Lib Definition: [`wma.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/wma/wma.c) · [`wma.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/wma/wma.yaml)

| Native | File |
|--------|------|
| C | [`ta_WMA.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_WMA.c) |
| Rust | [`wma.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/wma.rs) |
| Java | [`Core_WMA.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/fragments/Core_WMA.java) |

TA-Lib is also available for Python, R and more using a [wrapper](https://ta-lib.org/wrappers/).

## Aliases

Weighted Moving Average, Linearly Weighted Moving Average, LWMA

## See Also

[SMA](/functions/sma) · [EMA](/functions/ema) · [MA](/functions/ma) · [DEMA](/functions/dema) · [TEMA](/functions/tema)
