---
title: MINUS_DM
description: "Minus Directional Movement, the downward component of Wilder's directional movement system. Measures Wilder-smoothed downward price motion over the period. Higher -DM indicates stronger downward directional movement."
---

# MINUS_DM

## Summary

Minus Directional Movement, the downward component of Wilder's directional movement system. Measures Wilder-smoothed downward price motion over the period. Higher -DM indicates stronger downward directional movement.

## Formula

diffP = high - prevHigh; diffM = prevLow - low
-DM1 = diffM if (diffM > 0 and diffP < diffM) else 0
period<=1: output raw -DM1 per bar.
period>1: seed = sum of first (period-1) -DM1; then Wilder smooth each bar:
-DM = prevMinusDM - prevMinusDM/period (+ -DM1 when the bar qualifies)

## Inputs

- `inHigh` — High price of each bar
- `inLow` — Low price of each bar

## Outputs

- `outReal` — Smoothed minus directional movement

## Parameters

| Parameter | Type | Default | Accepted values | Description |
| --- | --- | --- | --- | --- |
| `optInTimePeriod` | integer | 14 | 1–100000 | Wilder smoothing period |

## Properties

| Numerical<br>Stability | Display<br>Flags |
| :-- | :-- |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Start-Independent</span> <span class="flag-tip" tabindex="0" role="note" aria-label="The value at a bar does not depend on where your data starts — safe to compare across different-length windows." data-tip="The value at a bar does not depend on where your data starts — safe to compare across different-length windows.">i</span> | <span class="flag-box">☐</span> <span style="opacity:0.5">Overlap Input</span> <span class="flag-tip" tabindex="0" role="note" aria-label="Output is on the same scale as the input price, so it is drawn over the price chart." data-tip="Output is on the same scale as the input price, so it is drawn over the price chart.">i</span> |
| <span class="flag-box">✅</span> **Initial Unstable Period** <span class="flag-tip" tabindex="0" role="note" aria-label="Early values depend on how much history precedes them but converge as more bars are supplied; tunable via the unstable period." data-tip="Early values depend on how much history precedes them but converge as more bars are supplied; tunable via the unstable period.">i</span> | <span class="flag-box">✅</span> **Independent Y-Axis** <span class="flag-tip" tabindex="0" role="note" aria-label="Output is on its own scale, drawn in a separate pane below the price chart." data-tip="Output is on its own scale, drawn in a separate pane below the price chart.">i</span> |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Path-Dependent</span> <span class="flag-tip" tabindex="0" role="note" aria-label="Built up from the first bar (a running accumulation or path-tracking state machine): the value depends on where your data begins and never converges — don't compare across different windows." data-tip="Built up from the first bar (a running accumulation or path-tracking state machine): the value depends on where your data begins and never converges — don't compare across different windows.">i</span> | <span class="flag-box">☐</span> <span style="opacity:0.5">Candlestick</span> <span class="flag-tip" tabindex="0" role="note" aria-label="Output is an integer candlestick-pattern signal (e.g. -100 / 0 / +100)." data-tip="Output is an integer candlestick-pattern signal (e.g. -100 / 0 / +100).">i</span> |

## Implementation

TA-Lib Definition: [`minus_dm.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/minus_dm/minus_dm.c) · [`minus_dm.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/minus_dm/minus_dm.yaml)

| Native | File |
|--------|------|
| C | [`ta_MINUS_DM.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_MINUS_DM.c) |
| Rust | [`minus_dm.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/minus_dm.rs) |
| Java | [`Core_MINUS_DM.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/library/fragments/Core_MINUS_DM.java) |

TA-Lib is also available for Python, R and more using a [wrapper](https://ta-lib.org/wrappers/).

## Aliases

Minus Directional Movement, -DM

## See Also

[PLUS_DM](/functions/plus_dm) · [MINUS_DI](/functions/minus_di) · [PLUS_DI](/functions/plus_di) · [DX](/functions/dx) · [ADX](/functions/adx) · [ADXR](/functions/adxr)

## References

- J. Welles Wilder, *New Concepts in Technical Trading Systems*, Trend Research (ISBN 0894590278)
