---
title: "Commodity Channel Index (CCI)"
description: "Commodity Channel Index: measures the current typical price relative to its simple moving average, scaled by mean absolute deviation."
---

## Summary

Commodity Channel Index: measures the current typical price relative to its simple moving average, scaled by mean absolute deviation. Momentum oscillator flagging overbought/oversold extremes. CCI > +100 overbought; CCI < -100 oversold.

## Chart

<ta-chart src="/functions/cci.svg" ratio="720:416" alt="Drawn on the price pane over candles; below it, a separate indicator pane with a zero line showing outReal as a solid line."></ta-chart>

Drawn on the price pane over candles; below it, a separate indicator pane with a zero line showing outReal as a solid line. [Reference data (CSV)](/functions/cci.csv) · [Chart (SVG)](/functions/cci.svg)

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

**Numerical Stability:** [Start-Independent](/functions/stability.md#start-independent)

<div class="flag-table">

|  |
| :-- |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Overlap Input</span> |
| <span class="flag-box">✅</span> **Independent Y-Axis** <span class="flag-tip" tabindex="0" role="note" aria-label="Output is on its own scale, drawn in a separate pane below the price chart." data-tip="Output is on its own scale, drawn in a separate pane below the price chart.">i</span> |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Candlestick</span> |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Can Output NaN or ±Inf</span> |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Identity at Period 1</span> |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Display Shift</span> |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Uses Transcendental</span> |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Input Must Be Positive</span> |

</div>

## Implementation

Added in 0.4.0.

TA-Lib Definition: [`cci.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/cci/cci.c) · [`cci.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/cci/cci.yaml)

| Native | File |
|--------|------|
| C | [`ta_CCI.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_CCI.c) |
| Rust | [`cci.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/cci.rs) |
| Java | [`Core_CCI.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/fragments/Core_CCI.java) |
| C# | [`Core_CCI.cs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/csharp/library/src/Core_CCI.cs) |
| TypeScript | [`cci.ts`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/ts/library/src/func/cci.ts) |

Python: `talib.CCI`, see [ta-lib-python](https://ta-lib.github.io/ta-lib-python/func_groups/momentum_indicators.html).

TA-Lib is also available for R and more using a [wrapper](/install/#wrappers).

## Aliases

Commodity Channel Index

## See Also

[TYPPRICE](/functions/typprice.md) · [SMA](/functions/sma.md)

## References

- Donald Lambert
