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

- `optInFastPeriod` — Period of the fast EMA
- `optInSlowPeriod` — Period of the slow EMA
- `optInSignalPeriod` — Smoothing period of the signal line

## Aliases

moving average convergence divergence, moving average convergence/divergence

## See Also

MACDEXT · MACDFIX · EMA · APO

## References

- Gerald Appel, *Stock Market Trading Systems*, Traders Pr (ISBN 0934380163)
