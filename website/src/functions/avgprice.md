---
title: "Average Price (AVGPRICE)"
description: "Average Price: the arithmetic mean of each bar's open, high, low, and close. A price-transform overlap condensing OHLC into a single representative price."
---

## Summary

Average Price: the arithmetic mean of each bar's open, high, low, and close. A price-transform overlap condensing OHLC into a single representative price.

## Formula

outReal[i] = (High[i] + Low[i] + Close[i] + Open[i]) / 4

## Inputs

- `inOpen` — Open price of each bar
- `inHigh` — High price of each bar
- `inLow` — Low price of each bar
- `inClose` — Close price of each bar

## Outputs

- `outReal` — Per-bar average of the four OHLC prices

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

</div>

## Implementation

TA-Lib Definition: [`avgprice.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/avgprice/avgprice.c) · [`avgprice.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/avgprice/avgprice.yaml)

| Native | File |
|--------|------|
| C | [`ta_AVGPRICE.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_AVGPRICE.c) |
| Rust | [`avgprice.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/avgprice.rs) |
| Java | [`Core_AVGPRICE.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/fragments/Core_AVGPRICE.java) |

TA-Lib is also available for Python, R and more using a [wrapper](/install/#wrappers).

## Aliases

Average Price

## See Also

[MEDPRICE](/functions/medprice.md) · [TYPPRICE](/functions/typprice.md) · [WCLPRICE](/functions/wclprice.md)
