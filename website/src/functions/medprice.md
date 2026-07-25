---
title: MEDPRICE
description: "Median Price: the midpoint of each bar's high and low. A price-transform overlay."
---

# MEDPRICE

## Summary

Median Price: the midpoint of each bar's high and low. A price-transform overlay.

## Formula

$MEDPRICE_i = (High_i + Low_i) / 2$

## Inputs

- `inHigh` — High price of each bar
- `inLow` — Low price of each bar

## Outputs

- `outReal` — Midpoint of each bar's high and low

## Properties

**Numerical Stability:** [Start-Independent](/functions/stability#start-independent)

| Display<br>Flags |
| :-- |
| <span class="flag-box">✅</span> **Overlap Input** <span class="flag-tip" tabindex="0" role="note" aria-label="Output is on the same scale as the input price, so it is drawn over the price chart." data-tip="Output is on the same scale as the input price, so it is drawn over the price chart.">i</span> |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Independent Y-Axis</span> |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Candlestick</span> |

## Implementation

TA-Lib Definition: [`medprice.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/medprice/medprice.c) · [`medprice.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/medprice/medprice.yaml)

| Native | File |
|--------|------|
| C | [`ta_MEDPRICE.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_MEDPRICE.c) |
| Rust | [`medprice.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/medprice.rs) |
| Java | [`Core_MEDPRICE.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/library/fragments/Core_MEDPRICE.java) |

TA-Lib is also available for Python, R and more using a [wrapper](https://ta-lib.org/wrappers/).

## Aliases

Median Price

## See Also

[MIDPRICE](/functions/midprice) · [AVGPRICE](/functions/avgprice) · [TYPPRICE](/functions/typprice) · [WCLPRICE](/functions/wclprice)
