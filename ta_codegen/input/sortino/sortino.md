# SORTINO

## Summary

The rolling Sortino ratio: the mean return in excess of a minimum acceptable return (MAR), divided by the downside deviation, over a trailing window, scaled to a year. It is the Sharpe ratio with upside volatility taken out of the denominator: only returns below the MAR count as risk. Above 0 the window's returns beat the MAR on average; the larger the value, the more return per unit of downside.

## Formula

With simple returns $r_i = (x_i - x_{i-1}) / x_{i-1}$, per-bar minimum acceptable return $\text{MAR}$, excess returns $d_i = r_i - \text{MAR}$ and annualization $A$, over the trailing $n$ returns:

$$\text{SORTINO} = \sqrt{A}\,\frac{\bar{d}}{DD}, \qquad \bar{d} = \frac{1}{n}\sum d_i, \qquad DD = \sqrt{\frac{1}{n}\sum \min(d_i, 0)^2}$$

The result is $+\infty$ when no return in the window is below the MAR, and NaN when every return equals it.

## Notes

- The downside deviation divides by the full window `n`: a return above the MAR counts in `n` and adds zero to the sum. This is the definition of Sortino and Price (1994) and the divisor of Bacon's worked example; dividing by `n−1`, or by the number of returns below the MAR, gives a different value.
- The mean is arithmetic, not compounded.
- A window with no return below the MAR has no downside: the result is +Inf, and NaN when every return equals the MAR. In a rising market this is common. With an annualization of 0 it is NaN there and 0 elsewhere.
- The MAR is per bar, in the same unit as the returns. For an annual rate `R`, pass `(1+R)^(1/A) − 1`, where `A` is the annualization; it is not de-annualized for you.
- An annualization of 1 gives the raw per-bar ratio.
- A zero price leaves no return to measure from it: the return of the bar after it counts as 0.

## Inputs

- `inReal` — The price series; its returns are computed internally

## Outputs

- `outReal` — Annualized Sortino ratio of the trailing window, or +Inf or NaN where the window has no return below the MAR

## Parameters

- `optInTimePeriod` — Number of returns in the window; one price more is read
- `optInMAR` — Minimum acceptable return per bar; returns below it are the downside
- `optInAnnualization` — Bars per year; the per-bar ratio is multiplied by its square root

## Aliases

Sortino Ratio, Rolling Sortino, Downside Risk-Adjusted Return

## See Also

SHARPE · STDDEV · VAR · ROCP

## References

- Sortino, F. A. and Price, L. N. "Performance Measurement in a Downside Risk Framework." *The Journal of Investing*, 3(3), Fall 1994, 59-64.
- Bacon, C. R. *Practical Portfolio Performance Measurement and Attribution*, 2nd ed. Wiley, 2008.
