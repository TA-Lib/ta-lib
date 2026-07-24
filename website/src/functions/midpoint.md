---
title: MIDPOINT
description: "Midpoint over a period: the average of the highest and lowest input values within the lookback window. A single-series overlap smoother (use MIDPRICE for separate high/low price bars)."
---

# MIDPOINT

## Summary

Midpoint over a period: the average of the highest and lowest input values within the lookback window. A single-series overlap smoother (use MIDPRICE for separate high/low price bars).

## Formula

MIDPOINT = (Highest(inReal, period) + Lowest(inReal, period)) / 2

## Inputs

- `inReal` — Series to compute the midpoint over

## Outputs

- `outReal` — Midpoint of the period's high/low range

## Parameters

| Parameter | Type | Default | Accepted values | Description |
| --- | --- | --- | --- | --- |
| `optInTimePeriod` | integer | 14 | 2–100000 | Lookback window length |

## Properties

| Numerical<br>Stability | Display<br>Flags |
| :-- | :-- |
| <span class="flag-box">✅</span> **Start-Independent** <span class="flag-tip" tabindex="0" role="note" aria-label="The value at a bar does not depend on where your data starts — safe to compare across different-length windows." data-tip="The value at a bar does not depend on where your data starts — safe to compare across different-length windows.">i</span> | <span class="flag-box">✅</span> **Overlap Input** <span class="flag-tip" tabindex="0" role="note" aria-label="Output is on the same scale as the input price, so it is drawn over the price chart." data-tip="Output is on the same scale as the input price, so it is drawn over the price chart.">i</span> |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Initial Unstable Period</span> <span class="flag-tip" tabindex="0" role="note" aria-label="Early values depend on how much history precedes them but converge as more bars are supplied; tunable via the unstable period." data-tip="Early values depend on how much history precedes them but converge as more bars are supplied; tunable via the unstable period.">i</span> | <span class="flag-box">☐</span> <span style="opacity:0.5">Independent Y-Axis</span> <span class="flag-tip" tabindex="0" role="note" aria-label="Output is on its own scale, drawn in a separate pane below the price chart." data-tip="Output is on its own scale, drawn in a separate pane below the price chart.">i</span> |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Path-Dependent</span> <span class="flag-tip" tabindex="0" role="note" aria-label="Built up from the first bar (a running accumulation or path-tracking state machine): the value depends on where your data begins and never converges — don't compare across different windows." data-tip="Built up from the first bar (a running accumulation or path-tracking state machine): the value depends on where your data begins and never converges — don't compare across different windows.">i</span> | <span class="flag-box">☐</span> <span style="opacity:0.5">Candlestick</span> <span class="flag-tip" tabindex="0" role="note" aria-label="Output is an integer candlestick-pattern signal (e.g. -100 / 0 / +100)." data-tip="Output is an integer candlestick-pattern signal (e.g. -100 / 0 / +100).">i</span> |

## Implementation

TA-Lib Definition: [`midpoint.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/midpoint/midpoint.c) · [`midpoint.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/midpoint/midpoint.yaml)

| Native | File |
|--------|------|
| C | [`ta_MIDPOINT.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_MIDPOINT.c) |
| Rust | [`midpoint.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/midpoint.rs) |
| Java | [`Core_MIDPOINT.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/library/fragments/Core_MIDPOINT.java) |

TA-Lib is also available for Python, R and more using a [wrapper](https://ta-lib.org/wrappers/).

## See Also

[MIDPRICE](/functions/midprice) · [MAX](/functions/max) · [MIN](/functions/min)
