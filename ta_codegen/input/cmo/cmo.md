# CMO

## Summary

Chande Momentum Oscillator: bounded momentum measure from Wilder-smoothed average up-moves and down-moves. Identical to RSI except the numerator uses (gain-loss) instead of gain. Bounded in [-100,+100]; positive = net upward momentum, negative = net downward.

## Formula

d = P[t]-P[t-1]; over the initial period accumulate gain = sum of positive d, loss = sum of -d for negative d. Wilder-smooth each: prevGain = (prevGain*(period-1) + gain_today)/period (same for loss). CMO = 100 * (prevGain-prevLoss)/(prevGain+prevLoss); 0 when prevGain+prevLoss == 0.

## Notes

- Gains and losses are smoothed with Wilder's method (as in RSI) rather than the simple period sums of Chande's original definition.

## Inputs

- `inReal` — Source price/value series

## Outputs

- `outReal` — CMO oscillator value

## Parameters

- `optInTimePeriod` — Bars over which gains/losses are smoothed

## Aliases

Chande Momentum Oscillator

## See Also

RSI

## References

- Tushar S. Chande, *The New Technical Trader*, John Wiley & Sons (ISBN 0471597805)
