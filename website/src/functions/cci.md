---
title: CCI
description: "Commodity Channel Index: measures the current typical price relative to its simple moving average, scaled by mean absolute deviation. Momentum oscillator flagging overbought/oversold extremes. CCI > +100 overbought; CCI < -100 oversold."
---

# CCI

## Summary

Commodity Channel Index: measures the current typical price relative to its simple moving average, scaled by mean absolute deviation. Momentum oscillator flagging overbought/oversold extremes. CCI > +100 overbought; CCI < -100 oversold.

## Formula

TP_i = (High_i + Low_i + Close_i)/3
SMA = (1/N) * sum(TP over N bars)
meanDev = (1/N) * sum(|TP - SMA| over N bars)
CCI = (TP_last - SMA) / (0.015 * meanDev)

## Inputs

- `inHigh` — High price of each bar
- `inLow` — Low price of each bar
- `inClose` — Close price of each bar

## Outputs

- `outReal` — CCI value per bar

## Parameters

| Parameter | Type | Default | Accepted values | Description |
| --- | --- | --- | --- | --- |
| `optInTimePeriod` | integer | 14 | 2–100000 | Number of bars in the averaging/deviation window |

## Properties

| Numerical<br>Stability | Display<br>Flags |
| :-- | :-- |
| <span class="flag-box">✅</span> **Start-Independent** <span class="flag-tip" tabindex="0" role="note" aria-label="The value at a bar does not depend on where your data starts — safe to compare across different-length windows." data-tip="The value at a bar does not depend on where your data starts — safe to compare across different-length windows.">i</span> | <span class="flag-box">☐</span> <span style="opacity:0.5">Overlap Input</span> <span class="flag-tip" tabindex="0" role="note" aria-label="Output is on the same scale as the input price, so it is drawn over the price chart." data-tip="Output is on the same scale as the input price, so it is drawn over the price chart.">i</span> |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Initial Unstable Period</span> <span class="flag-tip" tabindex="0" role="note" aria-label="Early values depend on how much history precedes them but converge as more bars are supplied; tunable via the unstable period." data-tip="Early values depend on how much history precedes them but converge as more bars are supplied; tunable via the unstable period.">i</span> | <span class="flag-box">✅</span> **Independent Y-Axis** <span class="flag-tip" tabindex="0" role="note" aria-label="Output is on its own scale, drawn in a separate pane below the price chart." data-tip="Output is on its own scale, drawn in a separate pane below the price chart.">i</span> |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Path-Dependent</span> <span class="flag-tip" tabindex="0" role="note" aria-label="Built up from the first bar (a running accumulation or path-tracking state machine): the value depends on where your data begins and never converges — don't compare across different windows." data-tip="Built up from the first bar (a running accumulation or path-tracking state machine): the value depends on where your data begins and never converges — don't compare across different windows.">i</span> | <span class="flag-box">☐</span> <span style="opacity:0.5">Candlestick</span> <span class="flag-tip" tabindex="0" role="note" aria-label="Output is an integer candlestick-pattern signal (e.g. -100 / 0 / +100)." data-tip="Output is an integer candlestick-pattern signal (e.g. -100 / 0 / +100).">i</span> |

## Implementation

TA-Lib Definition: [`cci.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/cci/cci.c) · [`cci.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/cci/cci.yaml)

| Native | File |
|--------|------|
| C | [`ta_CCI.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_CCI.c) |
| Rust | [`cci.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/cci.rs) |
| Java | [`Core_CCI.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/library/fragments/Core_CCI.java) |

TA-Lib is also available for Python, R and more using a [wrapper](https://ta-lib.org/wrappers/).

## Aliases

Commodity Channel Index

## See Also

[TYPPRICE](/functions/typprice) · [SMA](/functions/sma)

## References

- Donald Lambert
