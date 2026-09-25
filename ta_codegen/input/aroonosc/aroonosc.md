# AROONOSC

## Summary

Aroon Oscillator: AroonUp minus AroonDown over a lookback window. Measures trend direction and strength on a -100..+100 scale. Positive when the high is more recent than the low (up-trend); negative when the low is more recent (down-trend).

## Formula

factor = 100 / optInTimePeriod
AroonUp   = factor * (period - (today - highestIdx))
AroonDown = factor * (period - (today - lowestIdx))
AroonOsc  = AroonUp - AroonDown = factor * (highestIdx - lowestIdx)
highestIdx/lowestIdx = bar index of the highest high / lowest low in the last (period+1) bars.

## Inputs

- `inHigh` — High price of each bar
- `inLow` — Low price of each bar

## Outputs

- `outReal` — Aroon oscillator value (AroonUp - AroonDown)

## Parameters

- `optInTimePeriod` — Lookback window for locating the highest high and lowest low

## Aliases

Aroon Oscillator

## See Also

AROON · MINMAX

## References

- Tushar S. Chande
