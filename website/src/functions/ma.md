---
title: "Moving average (MA)"
description: "Generic moving-average dispatcher that forwards the job to the MA implementation selected by optInMAType."
---

## Summary

Generic moving-average dispatcher that forwards the job to the MA implementation selected by optInMAType. Single uniform interface over all TA-Lib moving averages.

## Formula

outReal = MA_of_type(optInMAType)(inReal, optInTimePeriod); default type = SMA

## Notes

- A period of 1 performs no smoothing for every MAType: the output is a copy of the input.
- `TA_MAType_DISABLED` bypasses smoothing explicitly, for any period: the output is a copy of the input with a lookback of 0. Every function that takes an MAType parameter accepts it.
- `TA_MAType_DEFAULT` selects the documented default of the parameter it is passed to — SMA here, EMA for APO, PPO and PVO. Every function that takes an MAType parameter accepts it.

## Inputs

- `inReal` — Series to average

## Outputs

- `outReal` — Selected moving average of the input

## Parameters

| Parameter | Type | Default | Accepted values | Description |
| --- | --- | --- | --- | --- |
| `optInTimePeriod` | integer | 30 | 1–100000 | Averaging window length |
| `optInMAType` | MAType | SMA (0) | any MAType | Which moving-average algorithm to dispatch to |

*`MAType` values: 0 SMA · 1 EMA · 2 WMA · 3 DEMA · 4 TEMA · 5 TRIMA · 6 KAMA · 7 MAMA · 8 T3 · 9 HMA · 10 DISABLED · 11 DEFAULT · 12 ZLEMA · 13 RMA*

## Properties

**Numerical Stability:** [Depends on MA Type](/functions/stability.md#depends-on-ma-type) — This function's default, SMA, is start-independent.

<div class="flag-table">

|  |
| :-- |
| <span class="flag-box">✅</span> **Overlap Input** <span class="flag-tip" tabindex="0" role="note" aria-label="Output is on the same scale as the input price, so it is drawn over the price chart." data-tip="Output is on the same scale as the input price, so it is drawn over the price chart.">i</span> |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Independent Y-Axis</span> |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Candlestick</span> |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Can Output NaN or ±Inf</span> |
| <span class="flag-box">✅</span> **Identity at Period 1** <span class="flag-tip" tabindex="0" role="note" aria-label="A period of 1 performs no smoothing: the lookback is 0 and every output value is a bit-exact copy of its input value." data-tip="A period of 1 performs no smoothing: the lookback is 0 and every output value is a bit-exact copy of its input value.">i</span> |

</div>

## Implementation

TA-Lib Definition: [`ma.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/ma/ma.c) · [`ma.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/ma/ma.yaml)

| Native | File |
|--------|------|
| C | [`ta_MA.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_MA.c) |
| Rust | [`ma.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/ma.rs) |
| Java | [`Core_MA.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/fragments/Core_MA.java) |

TA-Lib is also available for Python, R and more using a [wrapper](/install/#wrappers).

## Aliases

Moving Average, MovingAverage

## See Also

[SMA](/functions/sma.md) · [EMA](/functions/ema.md) · [WMA](/functions/wma.md) · [DEMA](/functions/dema.md) · [TEMA](/functions/tema.md) · [TRIMA](/functions/trima.md) · [KAMA](/functions/kama.md) · [MAMA](/functions/mama.md) · [T3](/functions/t3.md) · [HMA](/functions/hma.md) · [ZLEMA](/functions/zlema.md) · [RMA](/functions/rma.md)
