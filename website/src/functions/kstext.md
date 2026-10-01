---
title: "Know Sure Thing with controllable MA type (KSTEXT)"
description: "Know Sure Thing with selectable moving averages: Pring's weighted sum of four smoothed rates of change, with a signal line."
---

## Summary

Know Sure Thing with selectable moving averages: Pring's weighted sum of four smoothed rates of change, with a signal line. One MA type smooths the four legs and another the signal line.

`KST` is Pring's definition, with simple averages throughout. Formulas published since then smooth the legs exponentially, and Pring allows a simple or an exponential signal line. The reading is the same as `KST`'s: above zero the combined momentum is positive, and the usual signals are the line crossing its signal line and the line changing direction.

## Formula

ROC_k[t] = (price[t] / price[t - X_k] - 1) × 100

RCMA_k = MA_roc(ROC_k, A_k), for k = 1..4

KST = 1 × RCMA_1 + 2 × RCMA_2 + 3 × RCMA_3 + 4 × RCMA_4

Signal = MA_signal(KST, S)

X_k = optInROC{k}Period, A_k = optInMA{k}Period, S = optInSignalPeriod

MA_roc uses optInROCMAType, MA_signal uses optInSignalMAType

## Notes

- With both MA types set to `TA_MAType_SMA` the outputs are those of `KST`.
- Both outputs start at the first bar where the signal line exists. A signal period of 1 disables signal-line smoothing for every signal MAType: the signal is then a copy of the line.
- The unstable period of a selected MA type lengthens the lookback, for the legs and for the signal line separately.
- `TA_MAType_MAMA` ignores its period argument, so where it is selected every period above 1 gives the same average.
- Each rate of change follows `ROC`: a zero price in the denominator makes that term 0.
- The weights go with leg position, not with the length of the rate of change.

## Inputs

- `inReal` — Source price series (canonically the close)

## Outputs

- `outKST` — Know Sure Thing line
- `outKSTSignal` — Signal line: MA of the line

## Parameters

| Parameter | Type | Default | Accepted values | Description |
| --- | --- | --- | --- | --- |
| `optInROC1Period` | integer | 10 | 1–100000 | Rate-of-change period of leg 1 (weight 1) |
| `optInROC2Period` | integer | 15 | 1–100000 | Rate-of-change period of leg 2 (weight 2) |
| `optInROC3Period` | integer | 20 | 1–100000 | Rate-of-change period of leg 3 (weight 3) |
| `optInROC4Period` | integer | 30 | 1–100000 | Rate-of-change period of leg 4 (weight 4) |
| `optInMA1Period` | integer | 10 | 1–100000 | Period of the MA smoothing leg 1 |
| `optInMA2Period` | integer | 10 | 1–100000 | Period of the MA smoothing leg 2 |
| `optInMA3Period` | integer | 10 | 1–100000 | Period of the MA smoothing leg 3 |
| `optInMA4Period` | integer | 15 | 1–100000 | Period of the MA smoothing leg 4 |
| `optInSignalPeriod` | integer | 9 | 1–100000 | Period of the signal-line MA |
| `optInROCMAType` | MAType | SMA (0) | any MAType | MA type smoothing the four legs |
| `optInSignalMAType` | MAType | SMA (0) | any MAType | MA type for the signal line |

*`MAType` values: 0 SMA · 1 EMA · 2 WMA · 3 DEMA · 4 TEMA · 5 TRIMA · 6 KAMA · 7 MAMA · 8 T3 · 9 HMA · 10 DISABLED · 11 DEFAULT · 12 ZLEMA · 13 RMA · 14 VIDYA · 15 ALMA*

## Properties

**Numerical Stability:** [Depends on MA Type](/functions/stability.md#depends-on-ma-type) — This function's default, SMA, is start-independent.

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

TA-Lib Definition: [`kstext.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/kstext/kstext.c) · [`kstext.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/kstext/kstext.yaml)

| Native | File |
|--------|------|
| C | [`ta_KSTEXT.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_KSTEXT.c) |
| Rust | [`kstext.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/kstext.rs) |
| Java | [`Core_KSTEXT.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/fragments/Core_KSTEXT.java) |
| C# | [`Core_KSTEXT.cs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/csharp/library/src/Core_KSTEXT.cs) |

TA-Lib is also available for Python, R and more using a [wrapper](/install/#wrappers).

## Aliases

Know Sure Thing Extended, KST with controllable MA type

## See Also

[KST](/functions/kst.md) · [ROC](/functions/roc.md) · [MA](/functions/ma.md) · [MACDEXT](/functions/macdext.md) · [COPPOCK](/functions/coppock.md)

## References

- Martin J. Pring, "Summed Rate Of Change (KST)", *Technical Analysis of Stocks & Commodities*, V.10:9, September 1992, pp. 365-369.
- Martin J. Pring, "Identifying Trends With The KST Indicator", *Technical Analysis of Stocks & Commodities*, V.10:10, October 1992, pp. 420-424.
