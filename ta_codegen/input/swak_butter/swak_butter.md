# SWAK_BUTTER

## Summary

The Butterworth row of John Ehlers' Swiss Army Knife filter: the Gaussian row's double real pole with two zeros added at Nyquist, giving a two-pole low-pass that cuts the shortest cycles harder than the Gaussian does while keeping the same pole placement.

Read it as a smoothed price line. Its DC gain is 1, so a flat market returns the price itself and the line sits on the chart with price. The two Nyquist zeros are what separate it from `TA_SWAK_GAUSS`: a bar-to-bar alternation — the fastest motion a sampled series can carry — is removed outright rather than merely attenuated, which is why the numerator weights three consecutive bars 1, 2, 1.

The name keeps the `SWAK` prefix on purpose. This is the Swiss Army Knife's Butterworth row, not the Butterworth filters of Ehlers' separate article, and not the SuperSmoother, which is a third two-pole low-pass with complex poles.

## Formula

```
w   = 2 * pi / optInTimePeriod
b2p = 2.415 * (1 - cos(w))
a2p = -b2p + sqrt(b2p^2 + 2 * b2p)

c0  = a2p^2 / 4
a1  = 2 * (1 - a2p)
a2  = -(1 - a2p)^2

y[i] = c0 * (x[i] + 2 * x[i-1] + x[i-2]) + a1 * y[i-1] + a2 * y[i-2]
```

The quarter in `c0` offsets the numerator's total weight of 4, which is what holds the DC gain at 1.

The filter is seeded in the steady state of a constant input equal to its first bar: both input slots and both output slots start at that bar's value.

## Notes

Every term of the recurrence exists at the first bar, so there is no structural lookback and nothing is dropped from the front of the request. What the first bars carry is the seed, which decays rather than ending: set `TA_FUNC_UNST_SWAK_BUTTER` to discard bars until that transient is below whatever matters for the caller.

The output may alias the input. This row reads `x[i-1]` and `x[i-2]`, which an aliased write would already have overwritten, so both are carried in locals and never re-read from the input array.

## Inputs

- `inReal` — The series to filter; Ehlers' default is the bar midpoint `(H+L)/2`, which the caller passes as `TA_MEDPRICE` output

## Outputs

- `outReal` — The filtered line, on the same scale as the input

## Parameters

- `optInTimePeriod` — Cutoff period; shorter keeps more of the fast motion, longer smooths harder and lags more

## Aliases

Swiss Army Knife Butterworth Filter, SWAK Butter, Ehlers Butterworth Filter

## See Also

SWAK_GAUSS · EMA · MEDPRICE

## References

- Ehlers, John F. "The Swiss Army Knife Indicator." *Stocks & Commodities*, January 2006.
