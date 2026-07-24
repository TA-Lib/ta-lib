---
title: STDDEV
description: "Rolling standard deviation of a series over a window, scaled by a deviations multiplier. Delegates to VAR, then takes the square root."
---

# STDDEV

## Summary

Rolling standard deviation of a series over a window, scaled by a deviations multiplier. Delegates to VAR, then takes the square root.

## Formula

$\sigma_i = \sqrt{\mathrm{VAR}_i}\cdot nbDev$, where $\mathrm{VAR}_i = \frac{1}{N}\sum x^2 - \left(\frac{1}{N}\sum x\right)^2$ (population variance, $N=$ timePeriod)

## Notes

- Uses population variance (divides by the period, not period minus one), so results differ slightly from the sample standard deviation used by some tools.

## Inputs

- `inReal` — Series to measure dispersion of

## Outputs

- `outReal` — Standard deviation at each bar, scaled by optInNbDev

## Parameters

| Parameter | Type | Default | Accepted values | Description |
| --- | --- | --- | --- | --- |
| `optInTimePeriod` | integer | 5 | 2–100000 | Window length |
| `optInNbDev` | real | 1 | any real | Multiplier applied to the standard deviation |

## Properties

| Numerical<br>Stability | Display<br>Flags |
| :-- | :-- |
| <span class="flag-box">✅</span> **Start-Independent** <span class="flag-tip" tabindex="0" role="note" aria-label="The value at a bar does not depend on where your data starts — safe to compare across different-length windows." data-tip="The value at a bar does not depend on where your data starts — safe to compare across different-length windows.">i</span> | <span class="flag-box">☐</span> <span style="opacity:0.5">Overlap Input</span> <span class="flag-tip" tabindex="0" role="note" aria-label="Output is on the same scale as the input price, so it is drawn over the price chart." data-tip="Output is on the same scale as the input price, so it is drawn over the price chart.">i</span> |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Initial Unstable Period</span> <span class="flag-tip" tabindex="0" role="note" aria-label="Early values depend on how much history precedes them but converge as more bars are supplied; tunable via the unstable period." data-tip="Early values depend on how much history precedes them but converge as more bars are supplied; tunable via the unstable period.">i</span> | <span class="flag-box">✅</span> **Independent Y-Axis** <span class="flag-tip" tabindex="0" role="note" aria-label="Output is on its own scale, drawn in a separate pane below the price chart." data-tip="Output is on its own scale, drawn in a separate pane below the price chart.">i</span> |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Path-Dependent</span> <span class="flag-tip" tabindex="0" role="note" aria-label="Built up from the first bar (a running accumulation or path-tracking state machine): the value depends on where your data begins and never converges — don't compare across different windows." data-tip="Built up from the first bar (a running accumulation or path-tracking state machine): the value depends on where your data begins and never converges — don't compare across different windows.">i</span> | <span class="flag-box">☐</span> <span style="opacity:0.5">Candlestick</span> <span class="flag-tip" tabindex="0" role="note" aria-label="Output is an integer candlestick-pattern signal (e.g. -100 / 0 / +100)." data-tip="Output is an integer candlestick-pattern signal (e.g. -100 / 0 / +100).">i</span> |

## Implementation

TA-Lib Definition: [`stddev.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/stddev/stddev.c) · [`stddev.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/stddev/stddev.yaml)

| Native | File |
|--------|------|
| C | [`ta_STDDEV.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_STDDEV.c) |
| Rust | [`stddev.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/stddev.rs) |
| Java | [`Core_STDDEV.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/library/fragments/Core_STDDEV.java) |

TA-Lib is also available for Python, R and more using a [wrapper](https://ta-lib.org/wrappers/).

## Aliases

Standard Deviation, SD, sigma

## See Also

[VAR](/functions/var) · [BBANDS](/functions/bbands) · [SMA](/functions/sma)
