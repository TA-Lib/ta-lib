---
title: "Moving Average Convergence/Divergence (MACD)"
description: "Moving Average Convergence/Divergence: the difference between a fast and a slow EMA of the input, plus an EMA-smoothed signal line and their histogram."
---

## Summary

Moving Average Convergence/Divergence: the difference between a fast and a slow EMA of the input, plus an EMA-smoothed signal line and their histogram. MACD crossing its signal line and histogram sign changes flag momentum shifts.

## Formula

MACD = EMA_fast - EMA_slow;  Signal = EMA(MACD, signalPeriod);  Hist = MACD - Signal

## Notes

- If the slow period is set smaller than the fast period, the two are swapped so the slow EMA is always the longer one.
- A signal period of 1 disables signal-line smoothing: the signal equals the MACD line and the histogram is zero. Before 0.6.5 this parameter value produced misaligned output (issues #48/#59).

## Inputs

- `inReal` — Input series (typically close)

## Outputs

- `outMACD` — Fast EMA minus slow EMA
- `outMACDSignal` — EMA of the MACD line
- `outMACDHist` — MACD minus signal line

## Parameters

| Parameter | Type | Default | Accepted values | Description |
| --- | --- | --- | --- | --- |
| `optInFastPeriod` | integer | 12 | 2–100000 | Period of the fast EMA |
| `optInSlowPeriod` | integer | 26 | 2–100000 | Period of the slow EMA |
| `optInSignalPeriod` | integer | 9 | 1–100000 | Smoothing period of the signal line |

## Properties

**Numerical Stability:** [Initial Unstable Period](/functions/stability.md#initial-unstable-period) — Inherited from EMA, which MACD computes internally; tunable via EMA's unstable period.

| Display<br>Flags |
| :-- |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Overlap Input</span> |
| <span class="flag-box">✅</span> **Independent Y-Axis** <span class="flag-tip" tabindex="0" role="note" aria-label="Output is on its own scale, drawn in a separate pane below the price chart." data-tip="Output is on its own scale, drawn in a separate pane below the price chart.">i</span> |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Candlestick</span> |

## Implementation

TA-Lib Definition: [`macd.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/macd/macd.c) · [`macd.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/macd/macd.yaml)

| Native | File |
|--------|------|
| C | [`ta_MACD.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_MACD.c) |
| Rust | [`macd.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/macd.rs) |
| Java | [`Core_MACD.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/fragments/Core_MACD.java) |

TA-Lib is also available for Python, R and more using a [wrapper](/install/#wrappers).

## Aliases

moving average convergence divergence, moving average convergence/divergence

## See Also

[MACDEXT](/functions/macdext.md) · [MACDFIX](/functions/macdfix.md) · [EMA](/functions/ema.md) · [APO](/functions/apo.md)

## References

- Gerald Appel, *Stock Market Trading Systems*, Traders Pr (ISBN 0934380163)
