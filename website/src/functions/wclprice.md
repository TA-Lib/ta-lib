---
title: "Weighted Close Price (WCLPRICE)"
description: "Weighted Close Price: a per-bar price average giving the close double weight relative to high and low."
---

## Summary

Weighted Close Price: a per-bar price average giving the close double weight relative to high and low.

## Formula

$\text{WCLPRICE} = \dfrac{\text{High} + \text{Low} + 2\cdot\text{Close}}{4}$

## Inputs

- `inHigh` — High price of each bar
- `inLow` — Low price of each bar
- `inClose` — Close price of each bar

## Outputs

- `outReal` — Weighted close price per bar

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

TA-Lib Definition: [`wclprice.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/wclprice/wclprice.c) · [`wclprice.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/wclprice/wclprice.yaml)

| Native | File |
|--------|------|
| C | [`ta_WCLPRICE.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_WCLPRICE.c) |
| Rust | [`wclprice.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/wclprice.rs) |
| Java | [`Core_WCLPRICE.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/fragments/Core_WCLPRICE.java) |

TA-Lib is also available for Python, R and more using a [wrapper](/install/#wrappers).

## Aliases

Weighted Close Price, Weighted Close

## See Also

[TYPPRICE](/functions/typprice.md) · [MEDPRICE](/functions/medprice.md) · [AVGPRICE](/functions/avgprice.md)
