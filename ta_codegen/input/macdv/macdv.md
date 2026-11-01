# MACDV

## Summary

MACD-V, Alex Spiroglou's volatility-normalised MACD: the spread of a fast and a slow EMA of the close over the Average True Range, times 100. Like MACD it comes with an EMA-smoothed signal line and their histogram. Dividing by the ATR expresses momentum in units of the instrument's own volatility, so one reading means the same thing across instruments, timeframes and volatility regimes, where the raw MACD spread does not. Spiroglou reads the line in ranges: beyond 150 or -150 is an overextended move, between -50 and 50 a ranging market, and the bands in between a trend.

## Formula

MACDV = (EMA(Close, fastPeriod) - EMA(Close, slowPeriod)) / ATR(slowPeriod) * 100
Signal = EMA(MACDV, signalPeriod)
Hist = MACDV - Signal

## Notes

- The Average True Range is Wilder's, over the slow period: the publication's `ATR(26)` beside the 26-bar slow EMA, so one parameter sets both. Without price movement the line is 0, its neutral point.
- The two price EMAs and the Average True Range each warm up as when calling `EMA` and `ATR` directly, so a caller who wants the line converged sets `TA_FUNC_UNST_EMA` and `TA_FUNC_UNST_ATR`, as for those functions.
- If the slow period is set smaller than the fast period, the two are swapped so the slow EMA is always the longer one, as `MACD` does; the ATR period follows the slow period after the swap.
- A signal period of 1 disables signal-line smoothing: the signal equals the MACD-V line and the histogram is zero.
- The publication's MACD section writes the histogram as the signal line minus the MACD line; its reading of the histogram ("above 40" is stretched to the upside) holds only for MACD-V minus the signal, which is what is emitted here, as for `MACD`.

## Inputs

- `inHigh` — High price of each bar
- `inLow` — Low price of each bar
- `inClose` — Close price of each bar

## Outputs

- `outMACDV` — Fast EMA minus slow EMA, divided by the Average True Range, times 100
- `outMACDVSignal` — EMA of the MACD-V line
- `outMACDVHist` — MACD-V minus signal line

## Parameters

- `optInFastPeriod` — Period of the fast EMA
- `optInSlowPeriod` — Period of the slow EMA and of the Average True Range
- `optInSignalPeriod` — Smoothing period of the signal line

## Aliases

MACD-V, volatility normalised MACD, volatility normalized MACD, volatility normalised momentum

## See Also

MACD · ATR · EMA · APO · PPO

## References

- Alex Spiroglou, *MACD-V: Volatility Normalised Momentum*, 2022, [NAAIM Founders Award paper](https://www.naaim.org/wp-content/uploads/2022/05/MACD-V-Alex-Spiroglou-WEB.pdf)
- [MACD-V (StockCharts ChartSchool)](https://chartschool.stockcharts.com/table-of-contents/technical-indicators-and-overlays/technical-indicators/macd-v)
