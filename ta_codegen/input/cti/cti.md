# CTI

## Summary

John F. Ehlers' Correlation Trend Indicator: the Pearson correlation of the last `optInTimePeriod` closes against a straight line of positive slope. Bounded in -1..+1 by construction — `+1` is a perfectly linear uptrend, `-1` a perfectly linear downtrend, `0` no linear trend. Trend strength and direction in one bounded number, with no smoothing, no recursion and no filter coefficients.

Arithmetically it is [`CORREL`](/functions/correl) of the series against a ramp, with the ramp side collapsed to closed-form constants.

## Formula

With the window's closes `x` and `y` a straight line in time:

    CTI = ( n·Σxy − Σx·Σy ) / sqrt( ( n·Σx² − (Σx)² ) · ( n·Σy² − (Σy)² ) )

The `y` side is data-independent and collapses exactly: `n·Σy² − (Σy)² = n²(n²−1)/12`.

## Notes

- **The ramp's direction is the whole sign of the indicator.** This implementation carries `y` as *bars ago*, which runs backward in time, so a rising series correlates negatively with it and the coefficient is negated once at the output. Ehlers' own listing counts the same way and takes `Y = -count`, which is the same thing. Getting this wrong inverts the indicator completely rather than perturbing it, because Pearson's `r` is odd in either variable — and no magnitude or `|r|` assertion can see it. The bars-ago orientation is kept because the O(1) window slide is written for it.
- **The sums are taken against a shift, not on raw price levels.** Transcribing the published listing literally would compute `n·Σx² − (Σx)²` on the levels themselves, which is the cancellation that made `CORREL` return `0`, `-1` and `-1.73` from perfectly correlated inputs. MEASURED: at a price level of 1e2 with a 1e-5 spread the naive form errs by 5.7e-03 absolute — on an indicator whose entire range is 2 wide — against 1.6e-10 for the shift-and-reseed form.
- **There is a conditioning floor, and it is not a defect.** Once the window's spread falls below roughly 1e-8 of its level, the input doubles no longer carry the answer and no re-anchoring can recover it. That regime is a property of the input, not of this function.
- **A window with no spread returns exactly `0.0`.** Only the price side can degenerate — the ramp's sum of squares is a positive constant for every `n ≥ 2` — and the answer follows `CORREL`'s precedent rather than the author's listing, which holds the previous value. Holding would make this function path-dependent; returning NaN from a successful call is not permitted. Implementations disagree here: the listing holds, `CORREL` gives `0`, pandas gives `NaN`, Pine gives `na`.
- **The result is clamped into -1..+1**, as `CORREL` is: rounding in three sums can put a coefficient a few ulp outside its own range.
- `optInTimePeriod` starts at 2, not 1: at `n = 1` the closed form `n²(n²−1)/12` is identically zero and every window is degenerate.

## Inputs

- `inReal` — The series to measure the trend of

## Outputs

- `outReal` — Correlation against the ramp, in -1..+1

## Parameters

- `optInTimePeriod` — Number of trailing values correlated against the ramp

## See Also

CORREL · LINEARREG_SLOPE · VHF

## References

- Ehlers, John F. "Correlation As A Trend Indicator." *Technical Analysis of Stocks & Commodities*, May 2020 — Code Listing 1, marked (c) 2013-2019 John F. Ehlers.
