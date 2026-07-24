---
title: TRIMA
description: "Triangular Moving Average: a double-smoothed moving average that weights prices toward the middle of the window most heavily. Equivalent to an SMA of an SMA, computed here via an incremental triangular-weighted running numerator."
---

# TRIMA

## Summary

Triangular Moving Average: a double-smoothed moving average that weights prices toward the middle of the window most heavily. Equivalent to an SMA of an SMA, computed here via an incremental triangular-weighted running numerator.

## Formula

Weights rise then fall (4-period: (1a+2b+2c+1d)/6; 5-period: (1a+2b+3c+2d+1e)/9). With n = period>>1: odd divides by (n+1)^2, even by n(n+1). Equivalent to odd: SMA(SMA(x,(period+1)/2),(period+1)/2); even: SMA(SMA(x,period/2),period/2+1).

## Notes

- Follows the generally accepted (Metastock) definition rather than the TradeStation variant.
- A period of 1 performs no smoothing: the output is a copy of the input. Allowed since 0.6.5 (issues #48/#59).

## Inputs

- `inReal` — Source price series

## Outputs

- `outReal` — Triangular moving average

## Parameters

| Parameter | Type | Default | Accepted values | Description |
| --- | --- | --- | --- | --- |
| `optInTimePeriod` | integer | 30 | 1–100000 | Number of bars in the averaging window |

## Properties

| Numerical<br>Stability | Display<br>Flags |
| :-- | :-- |
| <span class="flag-box">✅</span> **Start-Independent** <span class="flag-tip" tabindex="0" role="note" aria-label="The value at a bar does not depend on where your data starts — safe to compare across different-length windows." data-tip="The value at a bar does not depend on where your data starts — safe to compare across different-length windows.">i</span> | <span class="flag-box">✅</span> **Overlap Input** <span class="flag-tip" tabindex="0" role="note" aria-label="Output is on the same scale as the input price, so it is drawn over the price chart." data-tip="Output is on the same scale as the input price, so it is drawn over the price chart.">i</span> |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Initial Unstable Period</span> <span class="flag-tip" tabindex="0" role="note" aria-label="Early values depend on how much history precedes them but converge as more bars are supplied; tunable via the unstable period." data-tip="Early values depend on how much history precedes them but converge as more bars are supplied; tunable via the unstable period.">i</span> | <span class="flag-box">☐</span> <span style="opacity:0.5">Independent Y-Axis</span> <span class="flag-tip" tabindex="0" role="note" aria-label="Output is on its own scale, drawn in a separate pane below the price chart." data-tip="Output is on its own scale, drawn in a separate pane below the price chart.">i</span> |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Path-Dependent</span> <span class="flag-tip" tabindex="0" role="note" aria-label="Built up from the first bar (a running accumulation or path-tracking state machine): the value depends on where your data begins and never converges — don't compare across different windows." data-tip="Built up from the first bar (a running accumulation or path-tracking state machine): the value depends on where your data begins and never converges — don't compare across different windows.">i</span> | <span class="flag-box">☐</span> <span style="opacity:0.5">Candlestick</span> <span class="flag-tip" tabindex="0" role="note" aria-label="Output is an integer candlestick-pattern signal (e.g. -100 / 0 / +100)." data-tip="Output is an integer candlestick-pattern signal (e.g. -100 / 0 / +100).">i</span> |

## Implementation

TA-Lib Definition: [`trima.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/trima/trima.c) · [`trima.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/trima/trima.yaml)

| Native | File |
|--------|------|
| C | [`ta_TRIMA.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_TRIMA.c) |
| Rust | [`trima.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/trima.rs) |
| Java | [`Core_TRIMA.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/library/fragments/Core_TRIMA.java) |

TA-Lib is also available for Python, R and more using a [wrapper](https://ta-lib.org/wrappers/).

## Aliases

Triangular Moving Average

## See Also

[SMA](/functions/sma) · [WMA](/functions/wma) · [MA](/functions/ma)
