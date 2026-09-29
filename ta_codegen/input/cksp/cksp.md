# CKSP

## Summary

Chande Kroll Stop places a pair of trailing stops a multiple of the Average True Range away from the recent extremes, then takes the extreme of those stops over a second, usually longer, window. The result is a stop that follows price but only ratchets after the shorter stop has held for a while.

The high stop sits below price and is the level a long position would give up at; the low stop sits above price and is the short side's. Neither line is always above the other: when the multiplier is large enough the two cross, which is the signal that the range has widened past what the stops can straddle.

## Formula

ATR[i] = Wilder Average True Range of period p at bar i

FHS[i] = MAX( high[i-p+1 .. i] ) - x * ATR[i]

FLS[i] = MIN( low[i-p+1 .. i] ) + x * ATR[i]

outHighStop[i] = MAX( FHS[i-q+1 .. i] )

outLowStop[i] = MIN( FLS[i-q+1 .. i] )

p = optInTimePeriod, x = optInMultiplier, q = optInStopPeriod

## Notes

The Average True Range is this library's, seeded the way `ATR` seeds it. Implementations that start their range at the first bar instead differ over the early bars and converge afterwards; the difference is a seeding convention, not a different indicator.

Each stage is anchored on its own bars. The second stage reads the q-1 first-stage bars before the first output, and those in turn are computed from their own Average True Range and their own extreme windows, so a call started late gives the same values as the tail of a call started at the beginning.

At a multiplier of 0 the two outputs collapse to the plain extremes of the whole p+q-1 span: the highest high and the lowest low.

At a stop period of 1 the second stage is the identity and the outputs are the first-stage stops, which is the Chandelier Exit form.

## Inputs

- `inHigh` — High price of each bar
- `inLow` — Low price of each bar
- `inClose` — Close price of each bar, read only by the True Range

## Outputs

- `outHighStop` — Trailing stop below price, the high side
- `outLowStop` — Trailing stop above price, the low side

## Parameters

- `optInTimePeriod` — ATR and extreme window
- `optInMultiplier` — ATR multiplier
- `optInStopPeriod` — Stop window

## Aliases

Chande Kroll Stop

## See Also

- [ATR](atr.md) — the range measure the stop distance is built from
- [SUPERTREND](supertrend.md) — the other Overlap Study that offsets a band by a multiple of the ATR
- [MAX](max.md), [MIN](min.md) — the rolling extremes each stage takes

## References

- Tushar Chande and Stanley Kroll, *The New Technical Trader*, Wiley, 1994.
