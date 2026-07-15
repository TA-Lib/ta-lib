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

- `optInSignalPeriod` — Smoothing period for the signal line

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

MACD · MACDEXT · EMA · APO
