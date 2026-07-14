---
title: AROON
description: "Aroon reports how recently the highest high and lowest low occurred within a rolling window of length optInTimePeriod, as two 0-100 oscillators. Indicates trend strength and direction. Up near 100 = a very recent new high (strong uptrend); Down near 100 = a very recent new low. Up/Down crossovers signal trend shifts."
---

# AROON

## Summary

Aroon reports how recently the highest high and lowest low occurred within a rolling window of length optInTimePeriod, as two 0-100 oscillators. Indicates trend strength and direction. Up near 100 = a very recent new high (strong uptrend); Down near 100 = a very recent new low. Up/Down crossovers signal trend shifts.

## Formula

Up = 100*(period-(today-highestIdx))/period; Down = 100*(period-(today-lowestIdx))/period, where highestIdx/lowestIdx index the highest high / lowest low over the window [today-period .. today].

## Inputs

- `inPriceHL` — High and low price series

## Outputs

- `outAroonDown` — Recency of the lowest low (100 = it is the current bar, decaying as it ages)
- `outAroonUp` — Recency of the highest high (100 = it is the current bar, decaying as it ages)

## Parameters

- `optInTimePeriod` — Lookback window length

## Implementation

TA-Lib Definition: [`aroon.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/aroon/aroon.c) · [`aroon.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/aroon/aroon.yaml)

| Native | File |
|--------|------|
| C | [`ta_AROON.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_AROON.c) |
| Rust | [`aroon.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/src/ta_func/aroon.rs) |
| Java | [`Core_AROON.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/Core_AROON.java) |

TA-Lib is also available for Python, R and more using a [wrapper](https://ta-lib.org/wrappers/).

## See Also

[AROONOSC](/functions/aroonosc.md) · [MINMAXINDEX](/functions/minmaxindex.md) · [MIN](/functions/min.md) · [MAX](/functions/max.md)

## References

- Tushar S. Chande
