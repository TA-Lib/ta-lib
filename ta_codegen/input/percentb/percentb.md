# PERCENTB

## Summary

Bollinger Bands %B: where the input sits relative to its Bollinger Bands, 0 at the lower band and 1 at the upper band. Values below 0 or above 1 mean the input is outside the bands.

## Formula

$$
\%B_t = \frac{X_t - \mathrm{lower}_t}{\mathrm{upper}_t - \mathrm{lower}_t}, \text{ or } 0.5 \text{ when } \mathrm{upper}_t = \mathrm{lower}_t
$$

where $\mathrm{upper}_t$ and $\mathrm{lower}_t$ are the [`BBANDS`](/functions/bbands) bands of $X$ with period $n$, moving-average type $\text{matype}$ and deviation multipliers $k_{\text{up}}$, $k_{\text{dn}}$.

## Notes

- An input on the middle band reads `optInNbDevDn / (optInNbDevUp + optInNbDevDn)`: 0.5 with equal multipliers.
- With a simple moving average and both multipliers equal to k, %B = 0.5 + z / (2k), where z is the z-score of the input in its window.
- The result is a ratio; multiply by 100 to read it as a percentage.
- Any `optInMAType` other than SMA is a TA-Lib generalisation, as it is for BBANDS: the deviation stays the population standard deviation about the simple mean.
- PERCENTB is bit for bit `(inReal - lower) / (upper - lower)` computed from BBANDS' own outputs, and 0.5 wherever those two bands are equal.

## Inputs

- `inReal` — Input data series

## Outputs

- `outReal` — Position of the input between the lower band (0) and the upper band (1)

## Parameters

- `optInTimePeriod` — Periods for the MA and standard deviation
- `optInNbDevUp` — Standard-deviation multiplier for the upper band
- `optInNbDevDn` — Standard-deviation multiplier for the lower band
- `optInMAType` — Moving-average type for the middle band

## Aliases

Bollinger %b, %b, %B, Percent B, BB %B, BBP, Bollinger Bands %B

## See Also

BBANDS · BBW · STOCHF

## References

- John A. Bollinger, *Bollinger on Bollinger Bands*, McGraw-Hill 2001 (ISBN 0071373683)
- John Bollinger, [Bollinger Band Rules](https://www.bollingerbands.com/bollinger-band-rules), rules 15-17
