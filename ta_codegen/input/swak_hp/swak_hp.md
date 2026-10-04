# SWAK_HP

## Summary

The high-pass row of John Ehlers' Swiss Army Knife filter: a one-pole detrender that removes what is slower than the cutoff period and keeps what is faster.

Read it as an oscillator, not as price. Its DC gain is 0, so a flat market returns zero and a trending one returns the trend's departure from itself rather than its level. That is the point of a detrender: what remains is the cyclic part of the series, centred on zero, which can then be measured or compared across instruments whose price levels differ by orders of magnitude.

The alpha is the cutoff-period one. Ehlers notes that it "is computed exactly the same as it is for the EMA", meaning the same construction, not `TA_EMA`'s `2/(n+1)`.

## Formula

```
w   = 2 * pi / optInTimePeriod
a1p = (cos(w) + sin(w) - 1) / cos(w)

c0  = 1 - a1p / 2
a1  = 1 - a1p

y[i] = c0 * (x[i] - x[i-1]) + a1 * y[i-1]
```

The filter is seeded in the steady state of a constant input equal to its first bar: the input slot starts at that bar and the output slot at zero, which is what a gain-0 row answers for a constant.

## Notes

The period range starts at 5 because of arithmetic rather than taste. At a period of 4, `cos(w)` is 6.1e-17 and the numerator `cos(w) + sin(w) - 1` rounds to exactly zero in doubles, so `a1p` is 0, both `c0` and `a1` become 1, and the filter degenerates into the integrator `x[i] - x[s]`. At a period of 2, `cos(w)` is -1 and `c0` is 0, a filter that answers nothing at all. No contiguous range below 5 avoids both.

Every term of the recurrence exists at the first bar, so there is no structural lookback. What the first bars carry is the seed, which decays rather than ending: set `TA_FUNC_UNST_SWAK_HP` to discard bars until that transient is below whatever matters for the caller.

The output may alias the input. This row reads `x[i-1]`, which an aliased write would already have overwritten, so it is carried in a local and never re-read from the input array.

## Inputs

- `inReal` — The series to detrend; Ehlers' default is the bar midpoint `(H+L)/2`, which the caller passes as `TA_MEDPRICE` output

## Outputs

- `outReal` — The detrended line, centred on zero

## Parameters

- `optInTimePeriod` — Cutoff period; cycles longer than it are removed, cycles shorter pass

## Aliases

Swiss Army Knife High-Pass Filter, SWAK HP, Ehlers High-Pass Filter

## See Also

SWAK_2PHP · SWAK_BP · MEDPRICE

## References

- Ehlers, John F. "The Swiss Army Knife Indicator." *Stocks & Commodities*, January 2006.
