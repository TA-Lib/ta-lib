---
title: "Acceleration Bands (ACCBANDS)"
description: "Acceleration Bands: three overlap lines around price."
---

## Summary

Acceleration Bands: three overlap lines around price. The middle band is an SMA of the close; the upper/lower bands are SMAs of the high/low scaled by an intraday-range factor.

## Chart

<ta-chart src="/functions/accbands.svg" ratio="720:274" alt="Drawn on the price pane over candles, with outRealMiddleBand as a solid line and a band between outRealUpperBand and outRealLowerBand with a soft fill."></ta-chart>

Drawn on the price pane over candles, with outRealMiddleBand as a solid line and a band between outRealUpperBand and outRealLowerBand with a soft fill. [Reference data (CSV)](/functions/accbands.csv) · [Chart (SVG)](/functions/accbands.svg)

## Formula

factor = 4*(H-L)/(H+L)
upperRaw = H*(1+factor), lowerRaw = L*(1-factor)
Upper = SMA(upperRaw, N), Middle = SMA(Close, N), Lower = SMA(lowerRaw, N)

## Inputs

- `inHigh` — High price of each bar
- `inLow` — Low price of each bar
- `inClose` — Close price of each bar

## Outputs

- `outRealUpperBand` — SMA of the range-scaled high band
- `outRealMiddleBand` — SMA of the close
- `outRealLowerBand` — SMA of the range-scaled low band

## Parameters

| Parameter | Type | Default | Accepted values | Description |
| --- | --- | --- | --- | --- |
| `optInTimePeriod` | integer | 20 | 2–100000 | SMA smoothing period for all three bands |

## Properties

**Numerical Stability:** [Start-Independent](/functions/stability.md#start-independent)

<div class="flag-table">

|  |
| :-- |
| <span class="flag-box">✅</span> **Overlap Input** <span class="flag-tip" tabindex="0" role="note" aria-label="Output is on the same scale as the input price, so it is drawn over the price chart." data-tip="Output is on the same scale as the input price, so it is drawn over the price chart.">i</span> |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Independent Y-Axis</span> |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Candlestick</span> |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Can Output NaN or ±Inf</span> |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Identity at Period 1</span> |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Display Shift</span> |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Uses Transcendental</span> |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Input Must Be Positive</span> |

</div>

## Implementation

Added in 0.5.0.

TA-Lib Definition: [`accbands.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/accbands/accbands.c) · [`accbands.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/accbands/accbands.yaml)

| Native | File |
|--------|------|
| C | [`ta_ACCBANDS.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_ACCBANDS.c) |
| Rust | [`accbands.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/accbands.rs) |
| Java | [`Core_ACCBANDS.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/fragments/Core_ACCBANDS.java) |
| C# | [`Core_ACCBANDS.cs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/csharp/library/src/Core_ACCBANDS.cs) |
| TypeScript | [`accbands.ts`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/ts/library/src/func/accbands.ts) |

Python: `talib.ACCBANDS`, see [ta-lib-python](https://ta-lib.github.io/ta-lib-python/func_groups/overlap_studies.html).

TA-Lib is also available for R and more using a [wrapper](/install/#wrappers).

## Aliases

Acceleration Bands

## See Also

[SMA](/functions/sma.md) · [BBANDS](/functions/bbands.md)
