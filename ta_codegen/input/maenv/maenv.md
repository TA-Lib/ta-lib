# MAENV

## Summary

Moving Average Envelope: a moving average with one band a fixed percentage above it and one a fixed percentage below. The bands keep a constant proportional distance from the average, so price reaching a band reads as stretched relative to its recent mean, and a move beyond one as unusual strength or weakness.

## Formula

$$
\begin{aligned}
\text{middle}_t &= \operatorname{MA}(X, n, \text{matype})_t \\
\text{upper}_t &= \text{middle}_t \left(1 + \frac{p_{\text{up}}}{100}\right) \\
\text{lower}_t &= \text{middle}_t \left(1 - \frac{p_{\text{dn}}}{100}\right)
\end{aligned}
$$

where $X$ is the input series, $n$ the period, $\text{matype}$ the moving-average type,
and $p_{\text{up}}$, $p_{\text{dn}}$ the upper and lower percentages.

## Notes

- The percentages are in percent: 10, not 0.1, places a band 10% from the moving average.
- Each band has its own percentage; the usual single-percentage envelope is the case where both are equal. A negative percentage puts its band on the other side of the average.
- **The input is assumed positive.** Each band sits a percent of the moving average away from it. Where the average is 0 both bands equal it, and where it is negative the two bands trade places; no error is returned. A series that reaches or crosses zero, such as an oscillator or a spread, has no meaningful result here.
- A displaced envelope is a shift of the plot by a number of bars. The outputs are not displaced.

## Inputs

- `inReal` — Input data series

## Outputs

- `outRealUpperBand` — The moving average plus the upper percentage of it
- `outRealMiddleBand` — The moving average
- `outRealLowerBand` — The moving average minus the lower percentage of it

## Parameters

- `optInTimePeriod` — Periods of the moving average
- `optInPercentUp` — Percent of the moving average added to form the upper band
- `optInPercentDn` — Percent of the moving average subtracted to form the lower band
- `optInMAType` — Moving-average type for the middle band

## Aliases

Envelope, Envelopes, MA Envelopes, Moving Average Envelopes

## See Also

MA · BBANDS · KC · ACCBANDS

## References

- [Moving Average Envelopes — StockCharts ChartSchool](https://chartschool.stockcharts.com/table-of-contents/technical-indicators-and-overlays/technical-overlays/moving-average-envelopes)
- [Envelope (ENV) — TradingView Help Center](https://www.tradingview.com/support/solutions/43000502260-envelope-env/)
