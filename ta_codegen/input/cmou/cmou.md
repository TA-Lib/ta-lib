# CMOU

## Summary

Chande Momentum Oscillator: Tushar Chande's original momentum oscillator, computed from **plain moving-window sums** of the up-moves and down-moves over the period.

Bounded in [-100,+100]; positive = net upward momentum, negative = net downward.

CMOU is the version as defined by Chande in his book *The New Technical Trader* (1994), and is the more common implementation used by TradingView (`ta.cmo`), QuantConnect and pandas-ta's default.

See [`CMO`](/functions/cmo) for a smoothed variant of CMOU.

## Formula

d = P[t]-P[t-1]; over the trailing `optInTimePeriod` changes accumulate Su = sum of the positive d, Sd = sum of -d for negative d. CMOU = 100 * (Su-Sd)/(Su+Sd); 0 when Su+Sd == 0 (an exactly flat window). Unlike CMO, the sums are the plain period totals (a moving-window sum), not Wilder-smoothed averages, so there is no unstable period.

## Inputs

- `inReal` — Source price/value series

## Outputs

- `outReal` — CMOU oscillator value

## Parameters

- `optInTimePeriod` — Number of trailing price changes summed

## Aliases

Chande Momentum Oscillator (Unsmoothed)

## See Also

CMO · RSI

## References

- Tushar S. Chande, *The New Technical Trader*, John Wiley & Sons (ISBN 0471597805)
