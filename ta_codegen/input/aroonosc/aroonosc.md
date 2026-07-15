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

- `inPriceHL` — High and low price series

## Outputs

- `outReal` — Aroon oscillator value (AroonUp - AroonDown)

## Parameters

- `optInTimePeriod` — Lookback window for locating the highest high and lowest low

## Implementation

TA-Lib Definition: [`aroonosc.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/aroonosc/aroonosc.c) · [`aroonosc.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/aroonosc/aroonosc.yaml)

| Native | File |
|--------|------|
| C | [`ta_AROONOSC.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_AROONOSC.c) |
| Rust | [`aroonosc.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/aroonosc.rs) |
| Java | [`Core_AROONOSC.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/library/fragments/Core_AROONOSC.java) |

TA-Lib is also available for Python, R and more using a [wrapper](https://ta-lib.org/wrappers/).

## Aliases

Aroon Oscillator

## See Also

AROON · MINMAX

## References

- Tushar S. Chande
