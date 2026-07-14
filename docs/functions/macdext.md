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

- `optInFastPeriod` — Period of the fast MA
- `optInFastMAType` — MA type for the fast MA
- `optInSlowPeriod` — Period of the slow MA
- `optInSlowMAType` — MA type for the slow MA
- `optInSignalPeriod` — Period of the signal-line MA
- `optInSignalMAType` — MA type for the signal line

## Implementation

TA-Lib Definition: [`macdext.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/macdext/macdext.c) · [`macdext.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/macdext/macdext.yaml)

| Native | File |
|--------|------|
| C | [`ta_MACDEXT.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_MACDEXT.c) |
| Rust | [`macdext.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/src/ta_func/macdext.rs) |
| Java | [`Core_MACDEXT.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/Core_MACDEXT.java) |

TA-Lib is also available for Python, R and more using a [wrapper](https://ta-lib.org/wrappers/).

## Aliases

MACD Extended, MACD with controllable MA type

## See Also

[MACD](/functions/macd.md) · [MACDFIX](/functions/macdfix.md) · [MA](/functions/ma.md) · [EMA](/functions/ema.md) · [APO](/functions/apo.md) · [PPO](/functions/ppo.md)
