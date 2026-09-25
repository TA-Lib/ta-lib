# RSI

## Summary

Wilder's Relative Strength Index, a momentum oscillator bounded 0-100 from the ratio of average gains to average losses over the period. Used to gauge overbought/oversold conditions. >70 overbought, <30 oversold.

## Formula

$$
\begin{aligned}
U_t &= \max(X_t - X_{t-1},\ 0)
   &  D_t &= \max(X_{t-1} - X_t,\ 0) \\[4pt]
\overline{U}_t &= \begin{cases}
    \operatorname{SMA}(U, n)_t                 & \text{if } t = n \\[4pt]
    \dfrac{(n-1)\,\overline{U}_{t-1} + U_t}{n} & \text{if } t > n
  \end{cases}
   &  \overline{D}_t &= \begin{cases}
    \operatorname{SMA}(D, n)_t                 & \text{if } t = n \\[4pt]
    \dfrac{(n-1)\,\overline{D}_{t-1} + D_t}{n} & \text{if } t > n
  \end{cases} \\[4pt]
\mathrm{RS}_t &= \frac{\overline{U}_t}{\overline{D}_t}
   &  \mathrm{RSI}_t &= 100 - \frac{100}{1 + \mathrm{RS}_t}
\end{aligned}
$$

where $X$ is the input series and $n$ the period.

## Notes



## Inputs

- `inReal` — Price series (typically close)

## Outputs

- `outReal` — RSI value

## Parameters

- `optInTimePeriod` — Lookback for the gain/loss averaging

## Aliases

relative strength index

## See Also

CMO · STOCHRSI

## References

- J. Welles Wilder, *New Concepts in Technical Trading Systems*, Trend Research (ISBN 0894590278)
