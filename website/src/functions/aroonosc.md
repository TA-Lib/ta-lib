---
title: AROONOSC
description: "Aroon Oscillator: AroonUp minus AroonDown over a lookback window. Measures trend direction and strength on a -100..+100 scale. Positive when the high is more recent than the low (up-trend); negative when the low is more recent (down-trend)."
---

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

| Parameter | Type | Default | Accepted values | Description |
| --- | --- | --- | --- | --- |
| `optInTimePeriod` | integer | 14 | 2–100000 | Lookback window for locating the highest high and lowest low |

## Properties

| Numerical<br>Stability | Display<br>Flags |
| :-- | :-- |
| <span class="flag-box">✅</span> **Start-Independent** <span class="flag-tip" tabindex="0" role="note" aria-label="The value at a bar does not depend on where your data starts — safe to compare across different-length windows." data-tip="The value at a bar does not depend on where your data starts — safe to compare across different-length windows.">i</span> | <span class="flag-box">☐</span> <span style="opacity:0.5">Overlap Input</span> <span class="flag-tip" tabindex="0" role="note" aria-label="Output is on the same scale as the input price, so it is drawn over the price chart." data-tip="Output is on the same scale as the input price, so it is drawn over the price chart.">i</span> |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Initial Unstable Period</span> <span class="flag-tip" tabindex="0" role="note" aria-label="Early values depend on how much history precedes them but converge as more bars are supplied; tunable via the unstable period." data-tip="Early values depend on how much history precedes them but converge as more bars are supplied; tunable via the unstable period.">i</span> | <span class="flag-box">✅</span> **Independent Y-Axis** <span class="flag-tip" tabindex="0" role="note" aria-label="Output is on its own scale, drawn in a separate pane below the price chart." data-tip="Output is on its own scale, drawn in a separate pane below the price chart.">i</span> |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Path-Dependent</span> <span class="flag-tip" tabindex="0" role="note" aria-label="Built up from the first bar (a running accumulation or path-tracking state machine): the value depends on where your data begins and never converges — don't compare across different windows." data-tip="Built up from the first bar (a running accumulation or path-tracking state machine): the value depends on where your data begins and never converges — don't compare across different windows.">i</span> | <span class="flag-box">☐</span> <span style="opacity:0.5">Candlestick</span> <span class="flag-tip" tabindex="0" role="note" aria-label="Output is an integer candlestick-pattern signal (e.g. -100 / 0 / +100)." data-tip="Output is an integer candlestick-pattern signal (e.g. -100 / 0 / +100).">i</span> |

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

[AROON](/functions/aroon) · [MINMAX](/functions/minmax)

## References

- Tushar S. Chande
