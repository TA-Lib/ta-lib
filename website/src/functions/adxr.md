---
title: ADXR
description: "Smoothed variant of ADX: the average of the current ADX value and the ADX value from (period-1) bars earlier. Further damps ADX to gauge trend strength. Higher values mean a stronger trend; smoother and more lagging than ADX."
---

# ADXR

## Summary

Smoothed variant of ADX: the average of the current ADX value and the ADX value from (period-1) bars earlier. Further damps ADX to gauge trend strength. Higher values mean a stronger trend; smoother and more lagging than ADX.

## Formula

ADXR[i] = (ADX[i] + ADX[i-(period-1)]) / 2

## Notes

- Wilder's original integer rounding is not applied (unreliable when values are near 1).

## Inputs

- `inHigh` — High price of each bar
- `inLow` — Low price of each bar
- `inClose` — Close price of each bar

## Outputs

- `outReal` — ADXR line (averaged ADX)

## Parameters

| Parameter | Type | Default | Accepted values | Description |
| --- | --- | --- | --- | --- |
| `optInTimePeriod` | integer | 14 | 2–100000 | Smoothing period, also the bar gap between the two averaged ADX values |

## Properties

| Numerical<br>Stability | Display<br>Flags |
| :-- | :-- |
| <span class="flag-box">✅</span> **Start-Independent** <span class="flag-tip" tabindex="0" role="note" aria-label="The value at a bar does not depend on where your data starts — safe to compare across different-length windows." data-tip="The value at a bar does not depend on where your data starts — safe to compare across different-length windows.">i</span> | <span class="flag-box">☐</span> <span style="opacity:0.5">Overlap Input</span> <span class="flag-tip" tabindex="0" role="note" aria-label="Output is on the same scale as the input price, so it is drawn over the price chart." data-tip="Output is on the same scale as the input price, so it is drawn over the price chart.">i</span> |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Initial Unstable Period</span> <span class="flag-tip" tabindex="0" role="note" aria-label="Early values depend on how much history precedes them but converge as more bars are supplied; tunable via the unstable period." data-tip="Early values depend on how much history precedes them but converge as more bars are supplied; tunable via the unstable period.">i</span> | <span class="flag-box">✅</span> **Independent Y-Axis** <span class="flag-tip" tabindex="0" role="note" aria-label="Output is on its own scale, drawn in a separate pane below the price chart." data-tip="Output is on its own scale, drawn in a separate pane below the price chart.">i</span> |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Path-Dependent</span> <span class="flag-tip" tabindex="0" role="note" aria-label="Built up from the first bar (a running accumulation or path-tracking state machine): the value depends on where your data begins and never converges — don't compare across different windows." data-tip="Built up from the first bar (a running accumulation or path-tracking state machine): the value depends on where your data begins and never converges — don't compare across different windows.">i</span> | <span class="flag-box">☐</span> <span style="opacity:0.5">Candlestick</span> <span class="flag-tip" tabindex="0" role="note" aria-label="Output is an integer candlestick-pattern signal (e.g. -100 / 0 / +100)." data-tip="Output is an integer candlestick-pattern signal (e.g. -100 / 0 / +100).">i</span> |

## Implementation

TA-Lib Definition: [`adxr.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/adxr/adxr.c) · [`adxr.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/adxr/adxr.yaml)

| Native | File |
|--------|------|
| C | [`ta_ADXR.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_ADXR.c) |
| Rust | [`adxr.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/adxr.rs) |
| Java | [`Core_ADXR.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/library/fragments/Core_ADXR.java) |

TA-Lib is also available for Python, R and more using a [wrapper](https://ta-lib.org/wrappers/).

## Aliases

Average Directional Movement Index Rating

## See Also

[ADX](/functions/adx) · [DX](/functions/dx) · [PLUS_DI](/functions/plus_di) · [MINUS_DI](/functions/minus_di)

## References

- J. Welles Wilder, *New Concepts in Technical Trading Systems*, Trend Research (ISBN 0894590278)
