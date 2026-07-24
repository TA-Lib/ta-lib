---
title: MACDFIX
description: "MACD with the fast/slow EMAs fixed to the classic 12/26 periods (with the classic fixed smoothing factors 0.15 and 0.075), exposing only the signal period. Signal-line crossovers and histogram sign flag momentum shifts."
---

# MACDFIX

## Summary

MACD with the fast/slow EMAs fixed to the classic 12/26 periods (with the classic fixed smoothing factors 0.15 and 0.075), exposing only the signal period. Signal-line crossovers and histogram sign flag momentum shifts.

## Formula

MACD = EMA_12 - EMA_26   (fixed k: 0.15 for 12, 0.075 for 26)
Signal = EMA(MACD, signalPeriod),  k = 2/(signalPeriod+1)
Hist = MACD - Signal

## Notes

- A signal period of 1 disables signal-line smoothing: the signal equals the MACD line and the histogram is zero. Before 0.6.5 this parameter value produced misaligned output (issues #48/#59).

## Inputs

- `inReal` — Source series (typically close)

## Outputs

- `outMACD` — Fixed EMA12 minus EMA26
- `outMACDSignal` — EMA of the MACD line
- `outMACDHist` — MACD minus signal

## Parameters

| Parameter | Type | Default | Accepted values | Description |
| --- | --- | --- | --- | --- |
| `optInSignalPeriod` | integer | 9 | 1–100000 | Smoothing period for the signal line |

## Properties

| Numerical<br>Stability | Display<br>Flags |
| :-- | :-- |
| <span class="flag-box">✅</span> **Start-Independent** <span class="flag-tip" tabindex="0" role="note" aria-label="The value at a bar does not depend on where your data starts — safe to compare across different-length windows." data-tip="The value at a bar does not depend on where your data starts — safe to compare across different-length windows.">i</span> | <span class="flag-box">☐</span> <span style="opacity:0.5">Overlap Input</span> <span class="flag-tip" tabindex="0" role="note" aria-label="Output is on the same scale as the input price, so it is drawn over the price chart." data-tip="Output is on the same scale as the input price, so it is drawn over the price chart.">i</span> |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Initial Unstable Period</span> <span class="flag-tip" tabindex="0" role="note" aria-label="Early values depend on how much history precedes them but converge as more bars are supplied; tunable via the unstable period." data-tip="Early values depend on how much history precedes them but converge as more bars are supplied; tunable via the unstable period.">i</span> | <span class="flag-box">✅</span> **Independent Y-Axis** <span class="flag-tip" tabindex="0" role="note" aria-label="Output is on its own scale, drawn in a separate pane below the price chart." data-tip="Output is on its own scale, drawn in a separate pane below the price chart.">i</span> |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Path-Dependent</span> <span class="flag-tip" tabindex="0" role="note" aria-label="Built up from the first bar (a running accumulation or path-tracking state machine): the value depends on where your data begins and never converges — don't compare across different windows." data-tip="Built up from the first bar (a running accumulation or path-tracking state machine): the value depends on where your data begins and never converges — don't compare across different windows.">i</span> | <span class="flag-box">☐</span> <span style="opacity:0.5">Candlestick</span> <span class="flag-tip" tabindex="0" role="note" aria-label="Output is an integer candlestick-pattern signal (e.g. -100 / 0 / +100)." data-tip="Output is an integer candlestick-pattern signal (e.g. -100 / 0 / +100).">i</span> |

## Implementation

TA-Lib Definition: [`macdfix.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/macdfix/macdfix.c) · [`macdfix.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/macdfix/macdfix.yaml)

| Native | File |
|--------|------|
| C | [`ta_MACDFIX.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_MACDFIX.c) |
| Rust | [`macdfix.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/macdfix.rs) |
| Java | [`Core_MACDFIX.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/library/fragments/Core_MACDFIX.java) |

TA-Lib is also available for Python, R and more using a [wrapper](https://ta-lib.org/wrappers/).

## Aliases

Moving Average Convergence/Divergence Fix

## See Also

[MACD](/functions/macd) · [MACDEXT](/functions/macdext) · [EMA](/functions/ema) · [APO](/functions/apo)
