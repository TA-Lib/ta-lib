---
title: ACCBANDS
description: "Acceleration Bands: three overlap lines around price. The middle band is an SMA of the close; the upper/lower bands are SMAs of the high/low scaled by an intraday-range factor."
---

# ACCBANDS

## Summary

Acceleration Bands: three overlap lines around price. The middle band is an SMA of the close; the upper/lower bands are SMAs of the high/low scaled by an intraday-range factor.

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

**Numerical Stability:** [Start-Independent](/functions/stability#start-independent)

| Display<br>Flags |
| :-- |
| <span class="flag-box">✅</span> **Overlap Input** <span class="flag-tip" tabindex="0" role="note" aria-label="Output is on the same scale as the input price, so it is drawn over the price chart." data-tip="Output is on the same scale as the input price, so it is drawn over the price chart.">i</span> |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Independent Y-Axis</span> |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Candlestick</span> |

## Implementation

TA-Lib Definition: [`accbands.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/accbands/accbands.c) · [`accbands.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/accbands/accbands.yaml)

| Native | File |
|--------|------|
| C | [`ta_ACCBANDS.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_ACCBANDS.c) |
| Rust | [`accbands.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/accbands.rs) |
| Java | [`Core_ACCBANDS.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/library/fragments/Core_ACCBANDS.java) |

TA-Lib is also available for Python, R and more using a [wrapper](https://ta-lib.org/wrappers/).

## Aliases

Acceleration Bands

## See Also

[SMA](/functions/sma) · [BBANDS](/functions/bbands)
