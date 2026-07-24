---
title: MININDEX
description: "Returns the absolute index of the lowest value within a rolling window of the given period. Same scan as MIN but outputs the position of the minimum rather than its value."
---

# MININDEX

## Summary

Returns the absolute index of the lowest value within a rolling window of the given period. Same scan as MIN but outputs the position of the minimum rather than its value.

## Formula

outInteger[t] = argmin_{t-period+1 <= i <= t} inReal[i]  (absolute index into inReal)

## Notes

- When several bars in a window share the lowest value, which bar's index is returned is not guaranteed to be a specific one of the tied bars.

## Inputs

- `inReal` — Series to scan for its minimum

## Outputs

- `outInteger` — Absolute index in inReal of the lowest value in each window

## Parameters

| Parameter | Type | Default | Accepted values | Description |
| --- | --- | --- | --- | --- |
| `optInTimePeriod` | integer | 30 | 2–100000 | Window length over which the minimum is located |

## Properties

| Numerical<br>Stability | Display<br>Flags |
| :-- | :-- |
| <span class="flag-box">✅</span> **Start-Independent** <span class="flag-tip" tabindex="0" role="note" aria-label="The value at a bar does not depend on where your data starts — safe to compare across different-length windows." data-tip="The value at a bar does not depend on where your data starts — safe to compare across different-length windows.">i</span> | <span class="flag-box">☐</span> <span style="opacity:0.5">Overlap Input</span> <span class="flag-tip" tabindex="0" role="note" aria-label="Output is on the same scale as the input price, so it is drawn over the price chart." data-tip="Output is on the same scale as the input price, so it is drawn over the price chart.">i</span> |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Initial Unstable Period</span> <span class="flag-tip" tabindex="0" role="note" aria-label="Early values depend on how much history precedes them but converge as more bars are supplied; tunable via the unstable period." data-tip="Early values depend on how much history precedes them but converge as more bars are supplied; tunable via the unstable period.">i</span> | <span class="flag-box">✅</span> **Independent Y-Axis** <span class="flag-tip" tabindex="0" role="note" aria-label="Output is on its own scale, drawn in a separate pane below the price chart." data-tip="Output is on its own scale, drawn in a separate pane below the price chart.">i</span> |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Path-Dependent</span> <span class="flag-tip" tabindex="0" role="note" aria-label="Built up from the first bar (a running accumulation or path-tracking state machine): the value depends on where your data begins and never converges — don't compare across different windows." data-tip="Built up from the first bar (a running accumulation or path-tracking state machine): the value depends on where your data begins and never converges — don't compare across different windows.">i</span> | <span class="flag-box">☐</span> <span style="opacity:0.5">Candlestick</span> <span class="flag-tip" tabindex="0" role="note" aria-label="Output is an integer candlestick-pattern signal (e.g. -100 / 0 / +100)." data-tip="Output is an integer candlestick-pattern signal (e.g. -100 / 0 / +100).">i</span> |

## Implementation

TA-Lib Definition: [`minindex.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/minindex/minindex.c) · [`minindex.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/minindex/minindex.yaml)

| Native | File |
|--------|------|
| C | [`ta_MININDEX.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_MININDEX.c) |
| Rust | [`minindex.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/minindex.rs) |
| Java | [`Core_MININDEX.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/library/fragments/Core_MININDEX.java) |

TA-Lib is also available for Python, R and more using a [wrapper](https://ta-lib.org/wrappers/).

## Aliases

Index of Lowest Value, Lowest Value Index, Rolling Argmin

## See Also

[MIN](/functions/min) · [MAXINDEX](/functions/maxindex) · [MINMAXINDEX](/functions/minmaxindex) · [MINMAX](/functions/minmax)
