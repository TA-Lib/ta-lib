---
title: DEMA
description: "Double Exponential Moving Average: an EMA combined with an EMA-of-EMA to reduce lag versus a plain EMA. Overlap Studies overlay on price."
---

# DEMA

## Summary

Double Exponential Moving Average: an EMA combined with an EMA-of-EMA to reduce lag versus a plain EMA. Overlap Studies overlay on price.

## Formula

EMA1 = EMA(inReal, period); EMA2 = EMA(EMA1, period); DEMA = 2*EMA1 - EMA2

## Notes

- A period of 1 performs no smoothing: the output is a copy of the input. Allowed since 0.6.5 (issues #48/#59).

## Inputs

- `inReal` — Source series (typically price)

## Outputs

- `outReal` — DEMA line

## Parameters

| Parameter | Type | Default | Accepted values | Description |
| --- | --- | --- | --- | --- |
| `optInTimePeriod` | integer | 30 | 1–100000 | Smoothing period for both EMA passes |

## Properties

| Numerical<br>Stability | Display<br>Flags |
| :-- | :-- |
| <span class="flag-box">✅</span> **Start-Independent** <span class="flag-tip" tabindex="0" role="note" aria-label="The value at a bar does not depend on where your data starts — safe to compare across different-length windows." data-tip="The value at a bar does not depend on where your data starts — safe to compare across different-length windows.">i</span> | <span class="flag-box">✅</span> **Overlap Input** <span class="flag-tip" tabindex="0" role="note" aria-label="Output is on the same scale as the input price, so it is drawn over the price chart." data-tip="Output is on the same scale as the input price, so it is drawn over the price chart.">i</span> |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Initial Unstable Period</span> <span class="flag-tip" tabindex="0" role="note" aria-label="Early values depend on how much history precedes them but converge as more bars are supplied; tunable via the unstable period." data-tip="Early values depend on how much history precedes them but converge as more bars are supplied; tunable via the unstable period.">i</span> | <span class="flag-box">☐</span> <span style="opacity:0.5">Independent Y-Axis</span> <span class="flag-tip" tabindex="0" role="note" aria-label="Output is on its own scale, drawn in a separate pane below the price chart." data-tip="Output is on its own scale, drawn in a separate pane below the price chart.">i</span> |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Path-Dependent</span> <span class="flag-tip" tabindex="0" role="note" aria-label="Built up from the first bar (a running accumulation or path-tracking state machine): the value depends on where your data begins and never converges — don't compare across different windows." data-tip="Built up from the first bar (a running accumulation or path-tracking state machine): the value depends on where your data begins and never converges — don't compare across different windows.">i</span> | <span class="flag-box">☐</span> <span style="opacity:0.5">Candlestick</span> <span class="flag-tip" tabindex="0" role="note" aria-label="Output is an integer candlestick-pattern signal (e.g. -100 / 0 / +100)." data-tip="Output is an integer candlestick-pattern signal (e.g. -100 / 0 / +100).">i</span> |

## Implementation

TA-Lib Definition: [`dema.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/dema/dema.c) · [`dema.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/dema/dema.yaml)

| Native | File |
|--------|------|
| C | [`ta_DEMA.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_DEMA.c) |
| Rust | [`dema.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/dema.rs) |
| Java | [`Core_DEMA.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/library/fragments/Core_DEMA.java) |

TA-Lib is also available for Python, R and more using a [wrapper](https://ta-lib.org/wrappers/).

## Aliases

Double Exponential Moving Average

## See Also

[EMA](/functions/ema) · [TEMA](/functions/tema) · [MA](/functions/ma)

## References

- Patrick G. Mulloy, *Smoothing Data with Faster Moving Averages*, Technical Analysis of Stocks & Commodities, V.12:1 (January 1994)
