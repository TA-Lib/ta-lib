---
title: VAR
description: "Rolling population variance of a real series over a given period. Measures dispersion of values around their mean. Higher values indicate greater dispersion; 0 means constant input."
---

# VAR

## Summary

Rolling population variance of a real series over a given period. Measures dispersion of values around their mean. Higher values indicate greater dispersion; 0 means constant input.

## Formula

$\mathrm{VAR} = \frac{1}{n}\sum x_i^2 - \left(\frac{1}{n}\sum x_i\right)^2$, over the last $n$ = optInTimePeriod values (population, divides by $n$).

## Notes

- Computes population variance (divides by the period), not the sample variance (n-1) used by some definitions.
- The deviation-count parameter is accepted but has no effect on the result.

## Inputs

- `inReal` — Source series

## Outputs

- `outReal` — Rolling population variance

## Parameters

| Parameter | Type | Default | Accepted values | Description |
| --- | --- | --- | --- | --- |
| `optInTimePeriod` | integer | 5 | 1–100000 | Window length for the variance |
| `optInNbDev` | real | 1 | any real | Deviation count accepted by the API but never used in the computation |

## Properties

| Numerical<br>Stability | Display<br>Flags |
| :-- | :-- |
| <span class="flag-box">✅</span> **Start-Independent** <span class="flag-tip" tabindex="0" role="note" aria-label="The value at a bar does not depend on where your data starts — safe to compare across different-length windows." data-tip="The value at a bar does not depend on where your data starts — safe to compare across different-length windows.">i</span> | <span class="flag-box">☐</span> <span style="opacity:0.5">Overlap Input</span> <span class="flag-tip" tabindex="0" role="note" aria-label="Output is on the same scale as the input price, so it is drawn over the price chart." data-tip="Output is on the same scale as the input price, so it is drawn over the price chart.">i</span> |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Initial Unstable Period</span> <span class="flag-tip" tabindex="0" role="note" aria-label="Early values depend on how much history precedes them but converge as more bars are supplied; tunable via the unstable period." data-tip="Early values depend on how much history precedes them but converge as more bars are supplied; tunable via the unstable period.">i</span> | <span class="flag-box">✅</span> **Independent Y-Axis** <span class="flag-tip" tabindex="0" role="note" aria-label="Output is on its own scale, drawn in a separate pane below the price chart." data-tip="Output is on its own scale, drawn in a separate pane below the price chart.">i</span> |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Path-Dependent</span> <span class="flag-tip" tabindex="0" role="note" aria-label="Built up from the first bar (a running accumulation or path-tracking state machine): the value depends on where your data begins and never converges — don't compare across different windows." data-tip="Built up from the first bar (a running accumulation or path-tracking state machine): the value depends on where your data begins and never converges — don't compare across different windows.">i</span> | <span class="flag-box">☐</span> <span style="opacity:0.5">Candlestick</span> <span class="flag-tip" tabindex="0" role="note" aria-label="Output is an integer candlestick-pattern signal (e.g. -100 / 0 / +100)." data-tip="Output is an integer candlestick-pattern signal (e.g. -100 / 0 / +100).">i</span> |

## Implementation

TA-Lib Definition: [`var.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/var/var.c) · [`var.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/var/var.yaml)

| Native | File |
|--------|------|
| C | [`ta_VAR.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_VAR.c) |
| Rust | [`var.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/var.rs) |
| Java | [`Core_VAR.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/library/fragments/Core_VAR.java) |

TA-Lib is also available for Python, R and more using a [wrapper](https://ta-lib.org/wrappers/).

## Aliases

Variance

## See Also

[STDDEV](/functions/stddev)
