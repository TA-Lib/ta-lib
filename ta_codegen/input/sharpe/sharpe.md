# SHARPE

## Summary

The rolling Sharpe ratio: the mean return in excess of a risk-free rate, divided by the standard deviation of that excess return, over a trailing window. A drift-over-volatility measure of the series itself, scaled to a year. Above 0 the window's returns beat the risk-free rate on average; the larger the value, the more return per unit of volatility.

## Formula

With simple returns $r_i = (x_i - x_{i-1}) / x_{i-1}$, per-bar risk-free rate $R_f$, excess returns $d_i = r_i - R_f$ and annualization $A$, over the trailing $n$ returns:

$$\text{SHARPE} = \sqrt{A}\,\frac{\bar{d}}{\sigma_d}, \qquad \bar{d} = \frac{1}{n}\sum d_i, \qquad \sigma_d = \sqrt{\frac{1}{n-1}\sum (d_i - \bar{d})^2}$$

The result is $\pm\infty$ by the sign of $\bar{d}$ when every excess return in the window is equal, and NaN when they are also exactly 0.

## Notes

- This is Sharpe's 1994 *ex post* ratio with a constant benchmark, over simple returns and the sample (`n−1`) standard deviation.
- A window whose returns are all equal has no spread: the result is +Inf or -Inf by the sign of the mean excess return, and NaN when that mean is exactly 0. With an annualization of 0 it is NaN there and 0 elsewhere.
- The risk-free rate is per bar, in the same unit as the returns. For an annual rate `R`, pass `(1+R)^(1/A) − 1`, where `A` is the annualization; it is not de-annualized for you.
- An annualization of 1 gives the raw per-bar ratio.
- A zero price leaves no return to measure from it: the return of the bar after it counts as 0.

## Inputs

- `inReal` — The price series; its returns are computed internally

## Outputs

- `outReal` — Annualized Sharpe ratio of the trailing window, or ±Inf or NaN where the window has no spread

## Parameters

- `optInTimePeriod` — Number of returns in the window; one price more is read
- `optInRiskFreeRate` — Risk-free return per bar, subtracted from each return
- `optInAnnualization` — Bars per year; the per-bar ratio is multiplied by its square root

## Aliases

Sharpe Ratio, Reward-to-Variability Ratio, Rolling Sharpe

## See Also

STDDEV · VAR · BETA · ROCP

## References

- Sharpe, W. F. "The Sharpe Ratio." *The Journal of Portfolio Management*, 21(1), Fall 1994, 49-58.
