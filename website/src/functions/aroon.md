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

| Numerical<br>Stability | Display<br>Flags |
| :-- | :-- |
| <span class="flag-box">✅</span> **Start-Independent** <span class="flag-tip" tabindex="0" role="note" aria-label="The value at a bar does not depend on where your data starts — safe to compare across different-length windows." data-tip="The value at a bar does not depend on where your data starts — safe to compare across different-length windows.">i</span> | <span class="flag-box">☐</span> <span style="opacity:0.5">Overlap Input</span> <span class="flag-tip" tabindex="0" role="note" aria-label="Output is on the same scale as the input price, so it is drawn over the price chart." data-tip="Output is on the same scale as the input price, so it is drawn over the price chart.">i</span> |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Initial Unstable Period</span> <span class="flag-tip" tabindex="0" role="note" aria-label="Early values depend on how much history precedes them but converge as more bars are supplied; tunable via the unstable period." data-tip="Early values depend on how much history precedes them but converge as more bars are supplied; tunable via the unstable period.">i</span> | <span class="flag-box">✅</span> **Independent Y-Axis** <span class="flag-tip" tabindex="0" role="note" aria-label="Output is on its own scale, drawn in a separate pane below the price chart." data-tip="Output is on its own scale, drawn in a separate pane below the price chart.">i</span> |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Path-Dependent</span> <span class="flag-tip" tabindex="0" role="note" aria-label="Built up from the first bar (a running accumulation or path-tracking state machine): the value depends on where your data begins and never converges — don't compare across different windows." data-tip="Built up from the first bar (a running accumulation or path-tracking state machine): the value depends on where your data begins and never converges — don't compare across different windows.">i</span> | <span class="flag-box">☐</span> <span style="opacity:0.5">Candlestick</span> <span class="flag-tip" tabindex="0" role="note" aria-label="Output is an integer candlestick-pattern signal (e.g. -100 / 0 / +100)." data-tip="Output is an integer candlestick-pattern signal (e.g. -100 / 0 / +100).">i</span> |

## Implementation

TA-Lib Definition: [`aroon.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/aroon/aroon.c) · [`aroon.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/aroon/aroon.yaml)

| Native | File |
|--------|------|
| C | [`ta_AROON.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_AROON.c) |
| Rust | [`aroon.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/aroon.rs) |
| Java | [`Core_AROON.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/library/fragments/Core_AROON.java) |

TA-Lib is also available for Python, R and more using a [wrapper](https://ta-lib.org/wrappers/).

## See Also

[AROONOSC](/functions/aroonosc) · [MINMAXINDEX](/functions/minmaxindex) · [MIN](/functions/min) · [MAX](/functions/max)

## References

- Tushar S. Chande
