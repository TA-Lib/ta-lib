---
title: "Chaikin A/D Line (AD)"
description: "Chaikin Accumulation/Distribution Line, a cumulative volume-flow indicator."
---

## Summary

Chaikin Accumulation/Distribution Line, a cumulative volume-flow indicator. Sums a volume-weighted money-flow multiplier per bar to gauge buying vs. selling pressure. Rising line = accumulation (buying pressure); falling = distribution.

## Chart

<ta-chart src="/functions/ad.svg" ratio="720:484" alt="Drawn on the price pane over candles; below it, a separate indicator pane with a zero line showing outReal as a solid line, then a volume pane."></ta-chart>

Drawn on the price pane over candles; below it, a separate indicator pane with a zero line showing outReal as a solid line, then a volume pane. [Reference data (CSV)](/functions/ad.csv) · [Chart (SVG)](/functions/ad.svg)

## Formula

MFM = ((close-low) - (high-close)) / (high-low); AD_t = AD_{t-1} + MFM_t * volume_t (running sum, seeded at 0)

## Inputs

- `inHigh` — High price of each bar
- `inLow` — Low price of each bar
- `inClose` — Close price of each bar
- `inVolume` — Volume of each bar

## Outputs

- `outReal` — Cumulative A/D line value per bar

## Properties

**Numerical Stability:** [Path-Dependent](/functions/stability.md#path-dependent)

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

TA-Lib Definition: [`ad.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/ad/ad.c) · [`ad.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/ad/ad.yaml)

| Native | File |
|--------|------|
| C | [`ta_AD.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_AD.c) |
| Rust | [`ad.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/ad.rs) |
| Java | [`Core_AD.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/fragments/Core_AD.java) |
| C# | [`Core_AD.cs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/csharp/library/src/Core_AD.cs) |
| TypeScript | [`ad.ts`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/ts/library/src/func/ad.ts) |

Python: `talib.AD`, see [ta-lib-python](https://ta-lib.github.io/ta-lib-python/func_groups/volume_indicators.html).

TA-Lib is also available for R and more using a [wrapper](/install/#wrappers).

## Aliases

Chaikin A/D Line, Accumulation/Distribution Line, Accumulation Distribution

## See Also

[ADOSC](/functions/adosc.md) · [OBV](/functions/obv.md)
