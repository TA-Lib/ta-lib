# BBW

## Summary

Bollinger BandWidth: the distance between the upper and lower Bollinger Bands as a percentage of the middle band. Low values mark contracting volatility, the setup John Bollinger calls the Squeeze; high values mark expanding volatility.

## Formula

$$
\mathrm{BBW}_t = 100 \cdot \frac{\mathrm{upper}_t - \mathrm{lower}_t}{\mathrm{middle}_t}, \text{ or } 0 \text{ when } \mathrm{middle}_t = 0
$$

where $\mathrm{upper}_t$, $\mathrm{middle}_t$ and $\mathrm{lower}_t$ are the [`BBANDS`](/functions/bbands) bands of $X$ with period $n$, moving-average type $\text{matype}$ and deviation multipliers $k_{\text{up}}$, $k_{\text{dn}}$. Since both bands sit on the middle band, $\mathrm{BBW}_t = 100\,(k_{\text{up}} + k_{\text{dn}})\,\sigma_t / \mathrm{middle}_t$, with $\sigma_t$ the population standard deviation of the last $n$ values of $X$.

## Notes

- With Bollinger's settings (a simple moving average and two deviations on each side) BBW is 400 times the window's coefficient of variation: its standard deviation divided by its mean.
- The two deviation multipliers enter only through their sum.
- The result is in percent: 10 means the bands are 10% of the middle band apart.
- Any `optInMAType` other than SMA is a TA-Lib generalisation, as it is for BBANDS: the deviation stays the population standard deviation about the simple mean.
- Wherever the middle band is not 0, BBW is bit for bit `((upper - lower) / middle) * 100` computed from BBANDS' own outputs.

## Inputs

- `inReal` — Input data series

## Outputs

- `outReal` — Width of the bands as a percentage of the middle band

## Parameters

- `optInTimePeriod` — Periods for the MA and standard deviation
- `optInNbDevUp` — Standard-deviation multiplier for the upper band
- `optInNbDevDn` — Standard-deviation multiplier for the lower band
- `optInMAType` — Moving-average type for the middle band

## Aliases

Bollinger BandWidth, Bollinger Band Width, Bollinger Bands Width, BandWidth

## See Also

BBANDS · PERCENTB · STDDEV · NATR

## References

- John A. Bollinger, *Bollinger on Bollinger Bands*, McGraw-Hill 2001 (ISBN 0071373683)
- John Bollinger, [Bollinger Band Rules](https://www.bollingerbands.com/bollinger-band-rules), rule 18
