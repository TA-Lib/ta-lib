# TRIX

## Summary

1-day Rate-Of-Change of a triple-smoothed EMA of the input. Momentum oscillator that filters out price moves shorter than the chosen period. Oscillates around zero; sign, zero-crossings and slope signal momentum direction.

## Formula

E1 = EMA(inReal, n); E2 = EMA(E1, n); E3 = EMA(E2, n); TRIX = ROC_1(E3) = 100 * (E3_today/E3_yesterday - 1)

## Notes

- The final rate-of-change step yields 0 when the previous smoothed value is exactly zero, rather than being undefined.

## Inputs

- `inReal` — Source series to smooth

## Outputs

- `outReal` — 1-day percent ROC of the triple EMA

## Parameters

- `optInTimePeriod` — EMA period used at each of the three smoothing passes

## Aliases

Triple Exponential Average

## See Also

EMA · ROC · ROCR · TEMA

## References

- Jack K. Hutson, Technical Analysis of Stocks & Commodities (1980s)
