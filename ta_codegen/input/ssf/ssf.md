# SSF

## Summary

John Ehlers' SuperSmoother: a two-pole low-pass smoother with a zero at Nyquist, which he recommends for use instead of an EMA and builds most of his later indicators on. Read it as a smoothed price line with less lag than an EMA of comparable smoothness.

Its DC gain is 1, so a flat market returns the price itself and the line sits on the chart with price. The critical period is the wavelength that divides the pass band from the reject band: cycles longer than it pass, shorter ones fall off at 12 dB per octave, and a two-bar alternation is removed entirely. The poles are a complex pair, which no EMA of an EMA and none of the `SWAK_*` rows can place.

## Formula

```
P  = optInTimePeriod
a1 = exp(-1.414 * 3.14159 / P)
c2 = 2 * a1 * cos(1.414 * pi / P)
c3 = -a1^2
c1 = 1 - c2 - c3

y[i] = c1 * (x[i] + x[i-1]) / 2 + c2 * y[i-1] + c3 * y[i-2]
```

The constants are Ehlers' own: `1.414` and `3.14159` in the radius, and a cosine he takes in degrees as `cos(1.414 * 180 / P)`. The first three bars of the history seed the filter with the price; the first output is a recursion step, at bar `max(P - 1, 3)`.

## Notes

- A period of 1 performs no smoothing: the output is a copy of the input, consistent with `MA(period=1)` for every MAType.
- Ehlers' 2004 form averages no bars in the numerator (`c1 * x[i]`, no zero at Nyquist); he replaced it by 2013, and implementations still computing it differ from this function by up to a few percent.
- Implementations with an exact `sqrt(2)` and `pi` in place of Ehlers' constants differ from this function in about the fifth significant digit.

## Inputs

- `inReal` — The series to smooth; Ehlers' examples use the close

## Outputs

- `outReal` — The smoothed line, on the same scale as the input

## Parameters

- `optInTimePeriod` — Critical period: the wavelength that divides the pass band and the reject band

## Aliases

SuperSmoother, Super Smoother, Ehlers SuperSmoother, SuperSmoother Filter

## See Also

EMA · SWAK_BUTTER · SWAK_GAUSS · MA

## References

- Ehlers, John F. [The Ultimate Smoother](https://www.mesasoftware.com/papers/UltimateSmoother.pdf) (2024), Code Listing 1, `$SuperSmoother`.
- Ehlers, John F. "Predictive and Successful Indicators." *Technical Analysis of Stocks & Commodities* (January 2014).
- Ehlers, John F. *Cybernetic Analysis for Stocks and Futures*, Wiley (2004), chapter 13.
