---
title: AVGDEV
description: "Rolling average absolute deviation of a series from its own simple moving average over the last N periods. Measures dispersion around the window mean. Higher values indicate greater spread; zero when all values in the window are equal."
---

# AVGDEV

## Summary

Rolling average absolute deviation of a series from its own simple moving average over the last N periods. Measures dispersion around the window mean. Higher values indicate greater spread; zero when all values in the window are equal.

## Formula

$mean_t = \frac{1}{N}\sum_{i=0}^{N-1} x_{t-i}$; $AVGDEV_t = \frac{1}{N}\sum_{i=0}^{N-1} |x_{t-i} - mean_t|$ (N = optInTimePeriod)

## Inputs

- `inReal` — source series

## Outputs

- `outReal` — mean absolute deviation over the window

## Parameters

| Parameter | Type | Default | Accepted values | Description |
| --- | --- | --- | --- | --- |
| `optInTimePeriod` | integer | 14 | 2–100000 | Window length |

## Properties

| Numerical<br>Stability | Display<br>Flags |
| :-- | :-- |
| <span class="flag-box">✅</span> **Start-Independent** <span class="flag-tip" tabindex="0" role="note" aria-label="The value at a bar does not depend on where your data starts — safe to compare across different-length windows." data-tip="The value at a bar does not depend on where your data starts — safe to compare across different-length windows.">i</span> | <span class="flag-box">✅</span> **Overlap Input** <span class="flag-tip" tabindex="0" role="note" aria-label="Output is on the same scale as the input price, so it is drawn over the price chart." data-tip="Output is on the same scale as the input price, so it is drawn over the price chart.">i</span> |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Initial Unstable Period</span> <span class="flag-tip" tabindex="0" role="note" aria-label="Early values depend on how much history precedes them but converge as more bars are supplied; tunable via the unstable period." data-tip="Early values depend on how much history precedes them but converge as more bars are supplied; tunable via the unstable period.">i</span> | <span class="flag-box">☐</span> <span style="opacity:0.5">Independent Y-Axis</span> <span class="flag-tip" tabindex="0" role="note" aria-label="Output is on its own scale, drawn in a separate pane below the price chart." data-tip="Output is on its own scale, drawn in a separate pane below the price chart.">i</span> |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Path-Dependent</span> <span class="flag-tip" tabindex="0" role="note" aria-label="Built up from the first bar (a running accumulation or path-tracking state machine): the value depends on where your data begins and never converges — don't compare across different windows." data-tip="Built up from the first bar (a running accumulation or path-tracking state machine): the value depends on where your data begins and never converges — don't compare across different windows.">i</span> | <span class="flag-box">☐</span> <span style="opacity:0.5">Candlestick</span> <span class="flag-tip" tabindex="0" role="note" aria-label="Output is an integer candlestick-pattern signal (e.g. -100 / 0 / +100)." data-tip="Output is an integer candlestick-pattern signal (e.g. -100 / 0 / +100).">i</span> |

## Implementation

TA-Lib Definition: [`avgdev.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/avgdev/avgdev.c) · [`avgdev.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/avgdev/avgdev.yaml)

| Native | File |
|--------|------|
| C | [`ta_AVGDEV.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_AVGDEV.c) |
| Rust | [`avgdev.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/avgdev.rs) |
| Java | [`Core_AVGDEV.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/library/fragments/Core_AVGDEV.java) |

TA-Lib is also available for Python, R and more using a [wrapper](https://ta-lib.org/wrappers/).

## Aliases

Average Deviation, Mean Absolute Deviation, MAD

## See Also

[STDDEV](/functions/stddev) · [VAR](/functions/var) · [SMA](/functions/sma)
