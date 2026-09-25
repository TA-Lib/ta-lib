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

## Aliases

Moving Average Convergence/Divergence Fix

## See Also

MACD · MACDEXT · EMA · APO
