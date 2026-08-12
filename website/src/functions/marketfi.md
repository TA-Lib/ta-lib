---
title: "Market Facilitation Index (MARKETFI)"
description: "Bill Williams' Market Facilitation Index (Trading Chaos, 1995): the price range a bar travelled per unit of volume traded — how much movement the market…"
---

## Summary

Bill Williams' Market Facilitation Index (*Trading Chaos*, 1995): the price range a bar travelled per unit of volume traded — how much movement the market "facilitated" per tick. A rising index on rising volume is read as a move the market is absorbing; a rising index on falling volume as one it is not.

Retail material commonly abbreviates this "MFI" or "BW MFI". TA-Lib already ships `TA_MFI` for the Money Flow Index, so this carries the `MARKETFI` name used by Tulip and pandas-ta-classic.

Charting packages often overlay a four-state colour code (green / fade / fake / squat) derived from the signs of the bar-to-bar change in this index and in volume. That is an interpretive layer on top of the series, not part of it; `outReal` is the scalar only.

## Formula

MARKETFI_t = (high_t - low_t) / volume_t

A bar with zero volume reports 0 rather than dividing: it facilitated no movement, and a successful call never emits NaN or ±Inf.

## Inputs

- `inHigh` — High price of each bar
- `inLow` — Low price of each bar
- `inVolume` — Volume of each bar

## Outputs

- `outReal` — Range travelled per unit of volume, per bar

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

</div>

## Implementation

TA-Lib Definition: [`marketfi.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/marketfi/marketfi.c) · [`marketfi.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/marketfi/marketfi.yaml)

| Native | File |
|--------|------|
| C | [`ta_MARKETFI.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_MARKETFI.c) |
| Rust | [`marketfi.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/marketfi.rs) |
| Java | [`Core_MARKETFI.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/fragments/Core_MARKETFI.java) |
| C# | [`Core_MARKETFI.cs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/csharp/library/src/Core_MARKETFI.cs) |

TA-Lib is also available for Python, R and more using a [wrapper](/install/#wrappers).

## See Also

[AD](/functions/ad.md) · [ADOSC](/functions/adosc.md) · [NVI](/functions/nvi.md) · [OBV](/functions/obv.md) · [PVI](/functions/pvi.md)

## References

- Bill Williams, *Trading Chaos*, 1995, introduces the Market Facilitation Index as the price range a bar travelled per unit of volume.
- Tulip Indicators `ti_marketfi` and pandas-ta-classic `marketfi` compute the same form, but divide unconditionally: both emit ±Inf on a zero-volume bar where this returns 0.
- The four-state green/fade/fake/squat colour code charting packages overlay is derived from the signs of the bar-to-bar change in this index and in volume. It is an interpretive layer, not part of the series; `outReal` is the scalar only.
