# CMO

## Summary

Chande Momentum Oscillator: bounded momentum measure from Wilder-smoothed average up-moves and down-moves. Identical to RSI except the numerator uses (gain-loss) instead of gain. Bounded in [-100,+100]; positive = net upward momentum, negative = net downward.

## Formula

d = P[t]-P[t-1]; over the initial period accumulate gain = sum of positive d, loss = sum of -d for negative d. Wilder-smooth each: prevGain = (prevGain*(period-1) + gain_today)/period (same for loss). CMO = 100 * (prevGain-prevLoss)/(prevGain+prevLoss); 0 when prevGain+prevLoss == 0.

## Notes

- Gains and losses are smoothed with Wilder's method (as in RSI) rather than the simple period sums of Chande's original definition.
- In Metastock-compatibility mode, an extra initial bar is emitted using a simple gain/loss average.

## Inputs

- `inReal` — Source price/value series

## Outputs

- `outReal` — CMO oscillator value

## Parameters

- `optInTimePeriod` — Bars over which gains/losses are smoothed

## Implementation

TA-Lib Definition: [`cmo.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/cmo/cmo.c) · [`cmo.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/cmo/cmo.yaml)

| Native | File |
|--------|------|
| C | [`ta_CMO.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_CMO.c) |
| Rust | [`cmo.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/src/ta_func/cmo.rs) |
| Java | [`Core_CMO.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/Core_CMO.java) |

TA-Lib is also available for Python, R and more using a [wrapper](https://ta-lib.org/wrappers/).

## Aliases

Chande Momentum Oscillator

## See Also

RSI

## References

- Tushar S. Chande, *The New Technical Trader*, John Wiley & Sons (ISBN 0471597805)
