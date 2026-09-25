# MAVP

## Summary

Moving average whose period varies per bar, driven by a companion period series. For each bar it computes an MA of the selected type over the (clamped) period given by inPeriods.

## Formula

p_i = clamp((int)inPeriods[startIdx+i], optInMinPeriod, optInMaxPeriod); outReal[i] = MA(inReal, p_i, optInMAType) at bar startIdx+i

## Notes

- Fractional per-bar periods are truncated to whole numbers before being clamped to the minimum and maximum period.
- Period values of 1 perform no smoothing (the bar's output equals its input); the minimum allowed period is 1 since 0.6.5.

## Inputs

- `inReal` — series to be averaged
- `inPeriods` — per-bar desired MA period

## Outputs

- `outReal` — variable-period moving average

## Parameters

- `optInMinPeriod` — Lower clamp for the per-bar period
- `optInMaxPeriod` — Upper clamp for the per-bar period
- `optInMAType` — Moving-average type applied

## Aliases

Moving Average Variable Period, Variable Period Moving Average

## See Also

MA · SMA · MAMA · T3
