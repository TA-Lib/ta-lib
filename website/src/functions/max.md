---
title: MAX
description: "Highest input value over a rolling window of the last optInTimePeriod bars. A moving-window maximum."
---

# MAX

## Summary

Highest input value over a rolling window of the last optInTimePeriod bars. A moving-window maximum.

## Formula

outReal[i] = max(inReal[i-optInTimePeriod+1 .. i])

## Inputs

- `inReal` — Series to take the rolling maximum of

## Outputs

- `outReal` — Highest value within each trailing window

## Parameters

| Parameter | Type | Default | Accepted values | Description |
| --- | --- | --- | --- | --- |
| `optInTimePeriod` | integer | 30 | 2–100000 | Window length in bars |

## Properties

| Numerical<br>Stability | Display<br>Flags |
| :-- | :-- |
| <span class="flag-box">✅</span> **Start-Independent** <span class="flag-tip" tabindex="0" role="note" aria-label="The value at a bar does not depend on where your data starts — safe to compare across different-length windows." data-tip="The value at a bar does not depend on where your data starts — safe to compare across different-length windows.">i</span> | <span class="flag-box">✅</span> **Overlap Input** <span class="flag-tip" tabindex="0" role="note" aria-label="Output is on the same scale as the input price, so it is drawn over the price chart." data-tip="Output is on the same scale as the input price, so it is drawn over the price chart.">i</span> |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Initial Unstable Period</span> <span class="flag-tip" tabindex="0" role="note" aria-label="Early values depend on how much history precedes them but converge as more bars are supplied; tunable via the unstable period." data-tip="Early values depend on how much history precedes them but converge as more bars are supplied; tunable via the unstable period.">i</span> | <span class="flag-box">☐</span> <span style="opacity:0.5">Independent Y-Axis</span> <span class="flag-tip" tabindex="0" role="note" aria-label="Output is on its own scale, drawn in a separate pane below the price chart." data-tip="Output is on its own scale, drawn in a separate pane below the price chart.">i</span> |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Path-Dependent</span> <span class="flag-tip" tabindex="0" role="note" aria-label="Built up from the first bar (a running accumulation or path-tracking state machine): the value depends on where your data begins and never converges — don't compare across different windows." data-tip="Built up from the first bar (a running accumulation or path-tracking state machine): the value depends on where your data begins and never converges — don't compare across different windows.">i</span> | <span class="flag-box">☐</span> <span style="opacity:0.5">Candlestick</span> <span class="flag-tip" tabindex="0" role="note" aria-label="Output is an integer candlestick-pattern signal (e.g. -100 / 0 / +100)." data-tip="Output is an integer candlestick-pattern signal (e.g. -100 / 0 / +100).">i</span> |

## Implementation

TA-Lib Definition: [`max.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/max/max.c) · [`max.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/max/max.yaml)

| Native | File |
|--------|------|
| C | [`ta_MAX.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_MAX.c) |
| Rust | [`max.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/max.rs) |
| Java | [`Core_MAX.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/library/fragments/Core_MAX.java) |

TA-Lib is also available for Python, R and more using a [wrapper](https://ta-lib.org/wrappers/).

## Aliases

Highest, Highest High, Rolling Maximum

## See Also

[MIN](/functions/min) · [MAXINDEX](/functions/maxindex) · [MINMAX](/functions/minmax)
