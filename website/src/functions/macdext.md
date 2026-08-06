---
title: MACDEXT
description: "MACD variant where the fast, slow, and signal moving averages each use a user-selectable MA type. Outputs the MACD line, its signal line, and their difference (histogram). Hist sign change (MACD crossing its signal line) flags momentum shifts."
---

# MACDEXT

## Summary

MACD variant where the fast, slow, and signal moving averages each use a user-selectable MA type. Outputs the MACD line, its signal line, and their difference (histogram). Hist sign change (MACD crossing its signal line) flags momentum shifts.

## Formula

MACD = MA_fast(inReal) - MA_slow(inReal)
Signal = MA_signal(MACD)
Hist = MACD - Signal
(each MA_* uses its own MA type and period)

## Notes

- If the slow period is set smaller than the fast period, the fast and slow periods and their MA types are swapped so the slow moving average is always the longer one.
- A signal period of 1 disables signal-line smoothing for every signal MAType: the signal equals the MACD line and the histogram is zero.

## Inputs

- `inReal` — Source series

## Outputs

- `outMACD` — MACD line: fast MA minus slow MA
- `outMACDSignal` — Signal line: MA of the MACD line
- `outMACDHist` — Histogram: MACD minus signal

## Parameters

| Parameter | Type | Default | Accepted values | Description |
| --- | --- | --- | --- | --- |
| `optInFastPeriod` | integer | 12 | 2–100000 | Period of the fast MA |
| `optInFastMAType` | MAType | SMA (0) | any MAType | MA type for the fast MA |
| `optInSlowPeriod` | integer | 26 | 2–100000 | Period of the slow MA |
| `optInSlowMAType` | MAType | SMA (0) | any MAType | MA type for the slow MA |
| `optInSignalPeriod` | integer | 9 | 1–100000 | Period of the signal-line MA |
| `optInSignalMAType` | MAType | SMA (0) | any MAType | MA type for the signal line |

*`MAType` values: 0 SMA · 1 EMA · 2 WMA · 3 DEMA · 4 TEMA · 5 TRIMA · 6 KAMA · 7 MAMA · 8 T3 · 9 HMA · 10 DISABLED*

## Properties

**Numerical Stability:** [Depends on MA Type](/functions/stability#depends-on-ma-type) — This function's default, SMA, is start-independent.

| Display<br>Flags |
| :-- |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Overlap Input</span> |
| <span class="flag-box">✅</span> **Independent Y-Axis** <span class="flag-tip" tabindex="0" role="note" aria-label="Output is on its own scale, drawn in a separate pane below the price chart." data-tip="Output is on its own scale, drawn in a separate pane below the price chart.">i</span> |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Candlestick</span> |

## Implementation

TA-Lib Definition: [`macdext.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/macdext/macdext.c) · [`macdext.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/macdext/macdext.yaml)

| Native | File |
|--------|------|
| C | [`ta_MACDEXT.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_MACDEXT.c) |
| Rust | [`macdext.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/macdext.rs) |
| Java | [`Core_MACDEXT.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/fragments/Core_MACDEXT.java) |

TA-Lib is also available for Python, R and more using a [wrapper](https://ta-lib.org/wrappers/).

## Aliases

MACD Extended, MACD with controllable MA type

## See Also

[MACD](/functions/macd) · [MACDFIX](/functions/macdfix) · [MA](/functions/ma) · [EMA](/functions/ema) · [APO](/functions/apo) · [PPO](/functions/ppo)
