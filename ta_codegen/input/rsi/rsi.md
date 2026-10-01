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

- While the input has not changed since the first bar the call reads, there is neither a gain nor a loss and RSI is 0/0: the output is the neutral 50. Up to 0.8.1 it was 0, which read as oversold. Input that has only risen gives 100 and input that has only fallen gives 0.
- After a move, an unchanged input holds the last value until the two averages decay to rounding residue: about a thousand unchanged bars at period 2, about ten thousand at period 14. The output then drifts and settles on 50. Input that had only risen or only fallen stays at 100 or 0, except at period 2.


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
