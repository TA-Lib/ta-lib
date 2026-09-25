# MEDIAN

## Summary

The middle order statistic of the trailing window: the central value when `optInTimePeriod` is odd, the mean of the two central values when it is even. A robust measure of central tendency — unlike [`SMA`](/functions/sma), a single spike moves it by at most one rank however large the spike is, which is what makes it useful as a filter rather than as a level.

Not to be confused with [`MEDPRICE`](/functions/medprice), which is `(High + Low) / 2` of one bar and is not an order statistic.

## Formula

With `W` the window sorted ascending and `W[1]` its smallest value:

    n odd :   MEDIAN = W[(n+1)/2]
    n even:   MEDIAN = ( W[n/2] + W[n/2 + 1] ) / 2

## Notes

- This is not [`PERCENTILE`](/functions/percentile) at 50. `PERCENTILE` reports the nearest rank, which at even `n` selects the **lower** of the two central values: on a 4-bar window of `1, 2, 3, 4` this function returns `2.5` and `PERCENTILE` returns `2`. At odd `n` the two agree bit for bit.
- At even `n` the output can therefore be a value the series never traded at, which is the deliberate opposite of `PERCENTILE`'s design property.
- `optInTimePeriod` is not restricted to odd values: TA-Lib has no odd-only range mechanism, and it would surprise any caller reaching for a 20-bar median.
- Every input value must be finite. A NaN makes every comparison against it false, which breaks the order the window is kept in, and the output can stay wrong long after the NaN has left the window.

## Inputs

- `inReal` — The series to take the median of

## Outputs

- `outReal` — Median of the trailing window

## Parameters

- `optInTimePeriod` — Number of trailing values in the window

## Aliases

Rolling Median, Moving Median, Running Median

## See Also

PERCENTILE · SMA · MEDPRICE

## References

- NumPy `numpy.median`, R `stats::median`, scipy, Excel `MEDIAN` — four independent implementations of one unambiguous definition.
