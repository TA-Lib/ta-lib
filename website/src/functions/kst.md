---
title: "Know Sure Thing (Pring) (KST)"
description: "Know Sure Thing: Martin J. Pring's momentum oscillator, a weighted sum of four smoothed rates of change with a moving-average signal line."
---

## Summary

Know Sure Thing: Martin J. Pring's momentum oscillator, a weighted sum of four smoothed rates of change with a moving-average signal line.

Each leg smooths a rate of change over a different span, and the longer legs carry the larger weights, so the line follows the dominant trend while the short legs make it turn early. Readings above zero mean the combined momentum is positive. The usual signals are the line crossing its signal line and the line changing direction. The level is unbounded, and the usual comparison is against the line's own history.

## Formula

ROC_k[t] = (price[t] / price[t - X_k] - 1) × 100

RCMA_k = SMA(ROC_k, A_k), for k = 1..4

KST = 1 × RCMA_1 + 2 × RCMA_2 + 3 × RCMA_3 + 4 × RCMA_4

Signal = SMA(KST, S)

X_k = optInROC{k}Period, A_k = optInSMA{k}Period, S = optInSignalPeriod

## Notes

- The weights are 1 to 4 and nothing divides by their total. Pring's daily table prints each leg's SMA period times its weight (a total of 120) only to show how much the longest leg dominates.
- Both outputs start at the first bar where the signal line exists, so the first few bars where the KST line alone is defined are not emitted. Setting the signal period to 1 returns the line from its own first bar, and the signal output is then a copy of it.
- Each rate of change follows `ROC`: a zero price in the denominator makes that term 0.
- The legs can be in any order. The weights go with leg position, not with the length of the rate of change.
- Pring's daily page uses a 10-day signal. The default signal period follows the charting platforms instead.

## Inputs

- `inReal` — Source price series (canonically the close)

## Outputs

- `outKST` — Know Sure Thing line
- `outKSTSignal` — Simple moving average of the line

## Parameters

| Parameter | Type | Default | Accepted values | Description |
| --- | --- | --- | --- | --- |
| `optInROC1Period` | integer | 10 | 1–100000 | Rate-of-change period of leg 1 (weight 1) |
| `optInROC2Period` | integer | 15 | 1–100000 | Rate-of-change period of leg 2 (weight 2) |
| `optInROC3Period` | integer | 20 | 1–100000 | Rate-of-change period of leg 3 (weight 3) |
| `optInROC4Period` | integer | 30 | 1–100000 | Rate-of-change period of leg 4 (weight 4) |
| `optInSMA1Period` | integer | 10 | 1–100000 | Simple-moving-average period smoothing leg 1 |
| `optInSMA2Period` | integer | 10 | 1–100000 | Simple-moving-average period smoothing leg 2 |
| `optInSMA3Period` | integer | 10 | 1–100000 | Simple-moving-average period smoothing leg 3 |
| `optInSMA4Period` | integer | 15 | 1–100000 | Simple-moving-average period smoothing leg 4 |
| `optInSignalPeriod` | integer | 9 | 1–100000 | Simple-moving-average period of the signal line |

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

</div>

## Implementation

TA-Lib Definition: [`kst.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/kst/kst.c) · [`kst.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/kst/kst.yaml)

| Native | File |
|--------|------|
| C | [`ta_KST.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_KST.c) |
| Rust | [`kst.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/kst.rs) |
| Java | [`Core_KST.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/fragments/Core_KST.java) |
| C# | [`Core_KST.cs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/csharp/library/src/Core_KST.cs) |

TA-Lib is also available for Python, R and more using a [wrapper](/install/#wrappers).

## Aliases

Know Sure Thing, Pring KST, Summed Rate of Change

## See Also

[KSTEXT](/functions/kstext.md) · [ROC](/functions/roc.md) · [SMA](/functions/sma.md) · [COPPOCK](/functions/coppock.md) · [MACD](/functions/macd.md)

## References

- Martin J. Pring, "Summed Rate Of Change (KST)", *Technical Analysis of Stocks & Commodities*, V.10:9, September 1992, pp. 365-369.
- Martin J. Pring, "Identifying Trends With The KST Indicator", *Technical Analysis of Stocks & Commodities*, V.10:10, October 1992, pp. 420-424.
- Pring Research, [Introducing the Daily KST](https://web.archive.org/web/20100505073742/www.pring.com/movieweb/daily_kst.htm).
- StockCharts ChartSchool, [Pring's Know Sure Thing (KST)](https://chartschool.stockcharts.com/table-of-contents/technical-indicators-and-overlays/technical-indicators/prings-know-sure-thing-kst).
