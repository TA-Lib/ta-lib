# AROON

## Summary

Aroon reports how recently the highest high and lowest low occurred within a rolling window of length optInTimePeriod, as two 0-100 oscillators. Indicates trend strength and direction. Up near 100 = a very recent new high (strong uptrend); Down near 100 = a very recent new low. Up/Down crossovers signal trend shifts.

## Formula

Up = 100*(period-(today-highestIdx))/period; Down = 100*(period-(today-lowestIdx))/period, where highestIdx/lowestIdx index the highest high / lowest low over the window [today-period .. today].

## Inputs

- `inHigh` — High price of each bar
- `inLow` — Low price of each bar

## Outputs

- `outAroonDown` — Recency of the lowest low (100 = it is the current bar, decaying as it ages)
- `outAroonUp` — Recency of the highest high (100 = it is the current bar, decaying as it ages)

## Parameters

- `optInTimePeriod` — Lookback window length

## See Also

AROONOSC · MINMAXINDEX · MIN · MAX

## References

- Tushar S. Chande
