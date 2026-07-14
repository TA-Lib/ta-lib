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
- Under Metastock compatibility mode the EMAs are seeded from the first value instead of a simple moving average, which changes all outputs.
- A signal period of 1 disables signal-line smoothing: the signal equals the MACD line and the histogram is zero. Before 0.6.5 this parameter value produced misaligned output (issues #48/#59).

## Inputs

- `inReal` — Input series (typically close)

## Outputs

- `outMACD` — Fast EMA minus slow EMA
- `outMACDSignal` — EMA of the MACD line
- `outMACDHist` — MACD minus signal line

## Parameters

- `optInFastPeriod` — Period of the fast EMA
- `optInSlowPeriod` — Period of the slow EMA
- `optInSignalPeriod` — Smoothing period of the signal line

## Implementation

TA-Lib Definition: [`macd.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/macd/macd.c) · [`macd.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/macd/macd.yaml)

| Native | File |
|--------|------|
| C | [`ta_MACD.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_MACD.c) |
| Rust | [`macd.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/src/ta_func/macd.rs) |
| Java | [`Core_MACD.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/Core_MACD.java) |

TA-Lib is also available for Python, R and more using a [wrapper](https://ta-lib.org/wrappers/).

## Aliases

moving average convergence divergence, moving average convergence/divergence

## See Also

[MACDEXT](/functions/macdext) · [MACDFIX](/functions/macdfix) · [EMA](/functions/ema) · [APO](/functions/apo)

## References

- Gerald Appel, *Stock Market Trading Systems*, Traders Pr (ISBN 0934380163)
