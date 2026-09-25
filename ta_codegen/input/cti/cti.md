# CTI

## Summary

John F. Ehlers' Correlation Trend Indicator: the Pearson correlation of the last `optInTimePeriod` closes against a straight line of positive slope. Bounded in -1..+1 by construction — `+1` is a perfectly linear uptrend, `-1` a perfectly linear downtrend, `0` no linear trend. Trend strength and direction in one bounded number, with no smoothing, no recursion and no filter coefficients.

Arithmetically it is [`CORREL`](/functions/correl) of the series against a ramp, with the ramp side collapsed to closed-form constants.

## Formula

With `n` = `optInTimePeriod`, `x` the window's closes and `y` a straight line rising with time:

    CTI = ( n·Σxy − Σx·Σy ) / sqrt( ( n·Σx² − (Σx)² ) · ( n·Σy² − (Σy)² ) ), or 0 when the window is flat

A rising series therefore reads positive. The `y` side is data-independent and collapses exactly: `n·Σy² − (Σy)² = n²(n²−1)/12`.

## Notes

- A flat window returns exactly `0.0` rather than holding the previous value as the author's listing does; holding would make the function path-dependent.
- The result is clamped into -1..+1, as `CORREL` is: rounding in three sums can put a coefficient slightly outside its own range.
- `optInTimePeriod` starts at 2, not 1: at `n = 1` the closed form `n²(n²−1)/12` is identically zero and every window is degenerate.

## Inputs

- `inReal` — The series to measure the trend of

## Outputs

- `outReal` — Correlation against the ramp, in -1..+1

## Parameters

- `optInTimePeriod` — Number of trailing values correlated against the ramp

## Aliases

Correlation Trend Indicator, Ehlers Correlation Trend Indicator, Correlation Trend

## See Also

CORREL · LINEARREG_SLOPE · VHF

## References

- John F. Ehlers, "Correlation As A Trend Indicator", *Technical Analysis of Stocks & Commodities*, V.38:5 (May 2020), Code Listing 1, [author's PDF](https://www.mesasoftware.com/papers/CORRELATION%20AS%20A%20TREND%20INDICATOR.pdf)
