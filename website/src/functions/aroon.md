---
title: "Aroon (AROON)"
description: "Aroon reports how recently the highest high and lowest low occurred within a rolling window of length optInTimePeriod, as two 0-100 oscillators."
---

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

| Parameter | Type | Default | Accepted values | Description |
| --- | --- | --- | --- | --- |
| `optInTimePeriod` | integer | 14 | 2–100000 | Lookback window length |

## Properties

**Numerical Stability:** [Start-Independent](/functions/stability.md#start-independent)

<div class="flag-table">

|  |
| :-- |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Overlap Input</span> |
| <span class="flag-box">✅</span> **Independent Y-Axis** <span class="flag-tip" tabindex="0" role="note" aria-label="Output is on its own scale, drawn in a separate pane below the price chart." data-tip="Output is on its own scale, drawn in a separate pane below the price chart.">i</span> |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Candlestick</span> |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Can Output NaN or ±Inf</span> |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Identity at Period 1</span> |

</div>

## Implementation

TA-Lib Definition: [`aroon.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/aroon/aroon.c) · [`aroon.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/aroon/aroon.yaml)

| Native | File |
|--------|------|
| C | [`ta_AROON.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_AROON.c) |
| Rust | [`aroon.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/aroon.rs) |
| Java | [`Core_AROON.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/fragments/Core_AROON.java) |

TA-Lib is also available for Python, R and more using a [wrapper](/install/#wrappers).

## See Also

[AROONOSC](/functions/aroonosc.md) · [MINMAXINDEX](/functions/minmaxindex.md) · [MIN](/functions/min.md) · [MAX](/functions/max.md)

## References

- Tushar S. Chande
