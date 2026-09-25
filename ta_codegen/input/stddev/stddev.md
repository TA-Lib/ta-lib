# STDDEV

## Summary

Rolling standard deviation of a series over a window, scaled by a deviations multiplier. Delegates to VAR, then takes the square root.

## Formula

$\sigma_i = \sqrt{\mathrm{VAR}_i}\cdot nbDev$, where $\mathrm{VAR}_i = \frac{1}{N}\sum x^2 - \left(\frac{1}{N}\sum x\right)^2$ (population variance, $N=$ timePeriod)

## Notes

- Uses population variance (divides by the period, not period minus one), so results differ slightly from the sample standard deviation used by some tools.

## Inputs

- `inReal` — Series to measure dispersion of

## Outputs

- `outReal` — Standard deviation at each bar, scaled by optInNbDev

## Parameters

- `optInTimePeriod` — Window length
- `optInNbDev` — Multiplier applied to the standard deviation

## Aliases

Standard Deviation, SD, sigma

## See Also

VAR · BBANDS · SMA
