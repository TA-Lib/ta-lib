# CRSI

## Summary

Connors RSI: a 0 to 100 oscillator, the plain average of three short-term momentum readings on that same scale.

The first is Wilder's RSI of the closes over `optInTimePeriod` bars. The second is an RSI over `optInStreakPeriod` bars of the up/down streak, the signed length of the current run of higher or lower closes (+3 after three higher closes in a row, -2 after two lower ones); an unchanged close resets the streak to 0. The third is the percent rank of the bar's one-bar return, (close - previous close) / previous close, among the `optInRankPeriod` returns before it: the percentage of them strictly below it.

High readings mark a short-term overbought market and low readings an oversold one.

## Formula

$$
\begin{aligned}
\mathrm{CRSI}_t &= \frac{\mathrm{RSI}(\mathrm{Close}, m_1)_t + \mathrm{RSI}(\mathrm{Streak}, m_2)_t + \mathrm{PercentRank}(\mathrm{ROC1}, P)_t}{3} \\[6pt]
\mathrm{Streak}_t &= \begin{cases}
    \max(\mathrm{Streak}_{t-1},\ 0) + 1 & \text{if } \mathrm{Close}_t > \mathrm{Close}_{t-1} \\
    \min(\mathrm{Streak}_{t-1},\ 0) - 1 & \text{if } \mathrm{Close}_t < \mathrm{Close}_{t-1} \\
    0                                   & \text{if } \mathrm{Close}_t = \mathrm{Close}_{t-1}
  \end{cases} \\[6pt]
\mathrm{ROC1}_t &= \frac{\mathrm{Close}_t - \mathrm{Close}_{t-1}}{\mathrm{Close}_{t-1}} \\[6pt]
\mathrm{PercentRank}(\mathrm{ROC1}, P)_t &= \frac{\left|\lbrace\, j : t-P \le j \le t-1,\ \mathrm{ROC1}_j < \mathrm{ROC1}_t \,\rbrace\right|}{P} \times 100
\end{aligned}
$$

where $\mathrm{Close}$ is the input series, $m_1$ is `optInTimePeriod`, $m_2$ is `optInStreakPeriod` and $P$ is `optInRankPeriod`. Both RSIs are Wilder's [`RSI`](/functions/rsi). The streak is defined from the second bar, taking the streak before it as 0, and $\mathrm{ROC1}_t$ is 0 when $\mathrm{Close}_{t-1} = 0$.

## Notes

- CRSI has no unstable period of its own: both RSI legs take `TA_FUNC_UNST_RSI`. They start warming up on the first bar the call reads (the streak's RSI one bar later, the streak being a change), so a leg that needs fewer bars than the lookback warms up over the spare ones.
- Finite input is a precondition. A NaN close does not propagate: the output stays finite and no error is reported, but the values that follow it are wrong.

## Inputs

- `inReal` — Close price series

## Outputs

- `outReal` — The averaged reading, 0 to 100

## Parameters

- `optInTimePeriod` — Period of the RSI of the closes
- `optInStreakPeriod` — Period of the RSI of the up/down streak
- `optInRankPeriod` — Number of earlier one-bar returns each return is ranked against

## Aliases

ConnorsRSI, Connors RSI, Connors Relative Strength Index

## See Also

RSI · PERCENTRANK · STOCHRSI

## References

- [Connors Research, *An Introduction to ConnorsRSI* (2012), pp. 7-9](https://www.qmatix.com/ConnorsRSI-Pullbacks-Guidebook.pdf)
- [StockCharts ChartSchool, *ConnorsRSI*](https://chartschool.stockcharts.com/table-of-contents/technical-indicators-and-overlays/technical-indicators/connorsrsi)
