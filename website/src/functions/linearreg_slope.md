---
title: LINEARREG_SLOPE
description: "Slope 'm' of the least-squares best-fit line (y = b + m*x) over the last optInTimePeriod bars. Reports the per-bar rate of change of the fitted trend line. Positive slope = rising trend, negative = falling; magnitude is price change per bar."
---

# LINEARREG_SLOPE

## Summary

Slope 'm' of the least-squares best-fit line (y = b + m*x) over the last optInTimePeriod bars. Reports the per-bar rate of change of the fitted trend line. Positive slope = rising trend, negative = falling; magnitude is price change per bar.

## Formula

m = (n·SumXY − SumX·SumY) / Divisor
SumX = n(n−1)/2,  SumXSqr = n(n−1)(2n−1)/6,  Divisor = SumX² − n·SumXSqr
SumXY = Σ i·y[today−i],  SumY = Σ y[today−i],  i=0..n−1,  n=period,  y=inReal

## Inputs

- `inReal` — Data series to fit

## Outputs

- `outReal` — Slope m of the fitted line

## Parameters

| Parameter | Type | Default | Accepted values | Description |
| --- | --- | --- | --- | --- |
| `optInTimePeriod` | integer | 14 | 2–100000 | Number of bars in the regression window |

## Properties

| Numerical<br>Stability | Display<br>Flags |
| :-- | :-- |
| <span class="flag-box">✅</span> **Start-Independent** <span class="flag-tip" tabindex="0" role="note" aria-label="The value at a bar does not depend on where your data starts — safe to compare across different-length windows." data-tip="The value at a bar does not depend on where your data starts — safe to compare across different-length windows.">i</span> | <span class="flag-box">☐</span> <span style="opacity:0.5">Overlap Input</span> <span class="flag-tip" tabindex="0" role="note" aria-label="Output is on the same scale as the input price, so it is drawn over the price chart." data-tip="Output is on the same scale as the input price, so it is drawn over the price chart.">i</span> |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Initial Unstable Period</span> <span class="flag-tip" tabindex="0" role="note" aria-label="Early values depend on how much history precedes them but converge as more bars are supplied; tunable via the unstable period." data-tip="Early values depend on how much history precedes them but converge as more bars are supplied; tunable via the unstable period.">i</span> | <span class="flag-box">✅</span> **Independent Y-Axis** <span class="flag-tip" tabindex="0" role="note" aria-label="Output is on its own scale, drawn in a separate pane below the price chart." data-tip="Output is on its own scale, drawn in a separate pane below the price chart.">i</span> |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Path-Dependent</span> <span class="flag-tip" tabindex="0" role="note" aria-label="Built up from the first bar (a running accumulation or path-tracking state machine): the value depends on where your data begins and never converges — don't compare across different windows." data-tip="Built up from the first bar (a running accumulation or path-tracking state machine): the value depends on where your data begins and never converges — don't compare across different windows.">i</span> | <span class="flag-box">☐</span> <span style="opacity:0.5">Candlestick</span> <span class="flag-tip" tabindex="0" role="note" aria-label="Output is an integer candlestick-pattern signal (e.g. -100 / 0 / +100)." data-tip="Output is an integer candlestick-pattern signal (e.g. -100 / 0 / +100).">i</span> |

## Implementation

TA-Lib Definition: [`linearreg_slope.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/linearreg_slope/linearreg_slope.c) · [`linearreg_slope.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/linearreg_slope/linearreg_slope.yaml)

| Native | File |
|--------|------|
| C | [`ta_LINEARREG_SLOPE.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_LINEARREG_SLOPE.c) |
| Rust | [`linearreg_slope.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/linearreg_slope.rs) |
| Java | [`Core_LINEARREG_SLOPE.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/library/fragments/Core_LINEARREG_SLOPE.java) |

TA-Lib is also available for Python, R and more using a [wrapper](https://ta-lib.org/wrappers/).

## Aliases

Linear Regression Slope, LSMA slope, least squares slope

## See Also

[LINEARREG](/functions/linearreg) · [LINEARREG_INTERCEPT](/functions/linearreg_intercept) · [LINEARREG_ANGLE](/functions/linearreg_angle) · [TSF](/functions/tsf)
