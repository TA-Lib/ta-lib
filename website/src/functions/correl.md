---
title: CORREL
description: "Pearson's correlation coefficient (r) between two input series over a rolling window of optInTimePeriod bars. Measures how linearly the two series move together. r near +1: strong positive co-movement; near -1: strong inverse; near 0: no linear relationship."
---

# CORREL

## Summary

Pearson's correlation coefficient (r) between two input series over a rolling window of optInTimePeriod bars. Measures how linearly the two series move together. r near +1: strong positive co-movement; near -1: strong inverse; near 0: no linear relationship.

## Formula

r = (sumXY - sumX*sumY/n) / sqrt((sumX2 - sumX^2/n) * (sumY2 - sumY^2/n)),  n = optInTimePeriod, sums over the window

## Notes

- When the correlation is undefined for a window (for example a constant series), the output is 0 rather than an error or NaN.

## Inputs

- `inReal0` — First data series (X)
- `inReal1` — Second data series (Y)

## Outputs

- `outReal` — Correlation coefficient r in [-1, 1]

## Parameters

| Parameter | Type | Default | Accepted values | Description |
| --- | --- | --- | --- | --- |
| `optInTimePeriod` | integer | 30 | 1–100000 | Rolling window length |

## Properties

| Numerical<br>Stability | Display<br>Flags |
| :-- | :-- |
| <span class="flag-box">✅</span> **Start-Independent** <span class="flag-tip" tabindex="0" role="note" aria-label="The value at a bar does not depend on where your data starts — safe to compare across different-length windows." data-tip="The value at a bar does not depend on where your data starts — safe to compare across different-length windows.">i</span> | <span class="flag-box">☐</span> <span style="opacity:0.5">Overlap Input</span> <span class="flag-tip" tabindex="0" role="note" aria-label="Output is on the same scale as the input price, so it is drawn over the price chart." data-tip="Output is on the same scale as the input price, so it is drawn over the price chart.">i</span> |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Initial Unstable Period</span> <span class="flag-tip" tabindex="0" role="note" aria-label="Early values depend on how much history precedes them but converge as more bars are supplied; tunable via the unstable period." data-tip="Early values depend on how much history precedes them but converge as more bars are supplied; tunable via the unstable period.">i</span> | <span class="flag-box">✅</span> **Independent Y-Axis** <span class="flag-tip" tabindex="0" role="note" aria-label="Output is on its own scale, drawn in a separate pane below the price chart." data-tip="Output is on its own scale, drawn in a separate pane below the price chart.">i</span> |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Path-Dependent</span> <span class="flag-tip" tabindex="0" role="note" aria-label="Built up from the first bar (a running accumulation or path-tracking state machine): the value depends on where your data begins and never converges — don't compare across different windows." data-tip="Built up from the first bar (a running accumulation or path-tracking state machine): the value depends on where your data begins and never converges — don't compare across different windows.">i</span> | <span class="flag-box">☐</span> <span style="opacity:0.5">Candlestick</span> <span class="flag-tip" tabindex="0" role="note" aria-label="Output is an integer candlestick-pattern signal (e.g. -100 / 0 / +100)." data-tip="Output is an integer candlestick-pattern signal (e.g. -100 / 0 / +100).">i</span> |

## Implementation

TA-Lib Definition: [`correl.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/correl/correl.c) · [`correl.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/correl/correl.yaml)

| Native | File |
|--------|------|
| C | [`ta_CORREL.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_CORREL.c) |
| Rust | [`correl.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/correl.rs) |
| Java | [`Core_CORREL.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/library/fragments/Core_CORREL.java) |

TA-Lib is also available for Python, R and more using a [wrapper](https://ta-lib.org/wrappers/).

## Aliases

Pearson Correlation, Correlation Coefficient, r

## See Also

[BETA](/functions/beta) · [STDDEV](/functions/stddev) · [VAR](/functions/var)

## References

- Karl Pearson
