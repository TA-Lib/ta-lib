---
title: MACD
description: "Moving Average Convergence/Divergence: the difference between a fast and a slow EMA of the input, plus an EMA-smoothed signal line and their histogram. MACD crossing its signal line and histogram sign changes flag momentum shifts."
---

# MACD

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

| Numerical<br>Stability | Display<br>Flags |
| :-- | :-- |
| <span class="flag-box">✅</span> **Start-Independent** <span class="flag-tip" tabindex="0" role="note" aria-label="The value at a bar does not depend on where your data starts — safe to compare across different-length windows." data-tip="The value at a bar does not depend on where your data starts — safe to compare across different-length windows.">i</span> | <span class="flag-box">☐</span> <span style="opacity:0.5">Overlap Input</span> <span class="flag-tip" tabindex="0" role="note" aria-label="Output is on the same scale as the input price, so it is drawn over the price chart." data-tip="Output is on the same scale as the input price, so it is drawn over the price chart.">i</span> |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Initial Unstable Period</span> <span class="flag-tip" tabindex="0" role="note" aria-label="Early values depend on how much history precedes them but converge as more bars are supplied; tunable via the unstable period." data-tip="Early values depend on how much history precedes them but converge as more bars are supplied; tunable via the unstable period.">i</span> | <span class="flag-box">✅</span> **Independent Y-Axis** <span class="flag-tip" tabindex="0" role="note" aria-label="Output is on its own scale, drawn in a separate pane below the price chart." data-tip="Output is on its own scale, drawn in a separate pane below the price chart.">i</span> |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Path-Dependent</span> <span class="flag-tip" tabindex="0" role="note" aria-label="Built up from the first bar (a running accumulation or path-tracking state machine): the value depends on where your data begins and never converges — don't compare across different windows." data-tip="Built up from the first bar (a running accumulation or path-tracking state machine): the value depends on where your data begins and never converges — don't compare across different windows.">i</span> | <span class="flag-box">☐</span> <span style="opacity:0.5">Candlestick</span> <span class="flag-tip" tabindex="0" role="note" aria-label="Output is an integer candlestick-pattern signal (e.g. -100 / 0 / +100)." data-tip="Output is an integer candlestick-pattern signal (e.g. -100 / 0 / +100).">i</span> |

## Implementation

TA-Lib Definition: [`macd.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/macd/macd.c) · [`macd.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/macd/macd.yaml)

| Native | File |
|--------|------|
| C | [`ta_MACD.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_MACD.c) |
| Rust | [`macd.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/macd.rs) |
| Java | [`Core_MACD.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/library/fragments/Core_MACD.java) |

TA-Lib is also available for Python, R and more using a [wrapper](https://ta-lib.org/wrappers/).

## Aliases

moving average convergence divergence, moving average convergence/divergence

## See Also

[MACDEXT](/functions/macdext) · [MACDFIX](/functions/macdfix) · [EMA](/functions/ema) · [APO](/functions/apo)

## References

- Gerald Appel, *Stock Market Trading Systems*, Traders Pr (ISBN 0934380163)
