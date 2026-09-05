---
title: "Coppock Curve (COPPOCK)"
description: "Coppock Curve: Edwin S. \"Sedge\" Coppock's long-term momentum oscillator (Barron's, originally published as the \"Trendex Model\"), computed as a weighted…"
---

## Summary

Coppock Curve: Edwin S. "Sedge" Coppock's long-term momentum oscillator (*Barron's*, originally published as the "Trendex Model"), computed as a weighted moving average of the **sum** of two rates of change.

Unbounded; positive turns from below zero are the signal the indicator was designed for (long-term buying opportunities on monthly index data).

## Formula

`COPPOCK = WMA(ROC(optInROC1Period) + ROC(optInROC2Period), optInWMAPeriod)`

Each ROC carries [`ROC`](/functions/roc.md)'s own zero guard — a zero price `optInROC*Period` bars back yields 0.0 for that term, never an infinity. The two ROCs are **summed**, not averaged: every published definition sums them. (Tulip's `copp` averages, so it reads at exactly half this amplitude — a clean 2.0x ratio against Tulip is Tulip's variant, not a defect.)

The formula is symmetric in the two ROC periods and the lookback keys off their max, so `optInROC1Period > optInROC2Period` is accepted rather than rejected.

The classic defaults are 11/14/10 on monthly data. Wikipedia's daily-scale variant (231/294-bar ROC, 210-bar WMA) is a parameter choice reachable through this API, not a competing formula.

## Inputs

- `inReal` — Source price/value series (canonically a monthly close)

## Outputs

- `outReal` — Coppock Curve value

## Parameters

| Parameter | Type | Default | Accepted values | Description |
| --- | --- | --- | --- | --- |
| `optInWMAPeriod` | integer | 10 | 1–100000 | Smoothing period for the ROC sum |
| `optInROC1Period` | integer | 11 | 1–100000 | Short rate-of-change period |
| `optInROC2Period` | integer | 14 | 1–100000 | Long rate-of-change period |

## Notes

- The single fused pass is bit-identical to running `ROC + ROC` into [`WMA`](/functions/wma.md).
- First output at `max(optInROC1Period, optInROC2Period) + optInWMAPeriod - 1`. Not start-dependent: each output depends only on its finite trailing window.

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

TA-Lib Definition: [`coppock.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/coppock/coppock.c) · [`coppock.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/coppock/coppock.yaml)

| Native | File |
|--------|------|
| C | [`ta_COPPOCK.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_COPPOCK.c) |
| Rust | [`coppock.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/coppock.rs) |
| Java | [`Core_COPPOCK.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/fragments/Core_COPPOCK.java) |

TA-Lib is also available for Python, R and more using a [wrapper](/install/#wrappers).
