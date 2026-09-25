# BBANDS

## Summary

Bollinger Bands: a moving-average middle band with upper and lower bands offset by a multiple of the standard deviation. Used to gauge relative price volatility.

## Formula

$$
\begin{aligned}
\text{middle}_t &= \operatorname{MA}(X, n, \text{matype})_t \\
\sigma_t &= \operatorname{STDDEV}(X, n)_t \\
\text{upper}_t &= \text{middle}_t + k_{\text{up}}\,\sigma_t \\
\text{lower}_t &= \text{middle}_t - k_{\text{dn}}\,\sigma_t
\end{aligned}
$$

where $X$ is the input series, $n$ the period, $\text{matype}$ the moving-average type,
and $k_{\text{up}}$, $k_{\text{dn}}$ the upper and lower deviation multipliers.

## Notes

- The defaults reproduce Bollinger's original definition: a 20-period SMA middle band with
  $k_{\text{up}} = k_{\text{dn}} = 2$. Any other $\text{matype}$ is a TA-Lib generalisation.
- $\text{matype}$ sets where the envelope is centred; $n$ and $k$ set how wide it is. The two are
  independent — $\sigma$ depends only on the price window, so changing the middle band re-centres
  the bands without resizing them.

## Inputs

- `inReal` — Input data series

## Outputs

- `outRealUpperBand` — Middle band plus nbDevUp standard deviations
- `outRealMiddleBand` — The moving average
- `outRealLowerBand` — Middle band minus nbDevDn standard deviations

## Parameters

- `optInTimePeriod` — Periods for the MA and standard deviation
- `optInNbDevUp` — Standard-deviation multiplier for the upper band
- `optInNbDevDn` — Standard-deviation multiplier for the lower band
- `optInMAType` — Moving-average type for the middle band

## Aliases

Bollinger Bands

## See Also

MA · STDDEV · SMA

## References

- John A. Bollinger, *Bollinger on Bollinger Bands*, McGraw-Hill Trade (ISBN 0071373683)
