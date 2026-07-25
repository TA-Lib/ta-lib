---
title: TYPPRICE
description: "Typical Price: the average of the high, low, and close of each bar. A single representative price per period."
---

# TYPPRICE

## Summary

Typical Price: the average of the high, low, and close of each bar. A single representative price per period.

## Formula

out[i] = (High[i] + Low[i] + Close[i]) / 3

## Inputs

- `inHigh` — High price of each bar
- `inLow` — Low price of each bar
- `inClose` — Close price of each bar

## Outputs

- `outReal` — typical price per bar

## Properties

**Numerical Stability:** [Start-Independent](/functions/stability#start-independent)

| Display<br>Flags |
| :-- |
| <span class="flag-box">✅</span> **Overlap Input** <span class="flag-tip" tabindex="0" role="note" aria-label="Output is on the same scale as the input price, so it is drawn over the price chart." data-tip="Output is on the same scale as the input price, so it is drawn over the price chart.">i</span> |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Independent Y-Axis</span> |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Candlestick</span> |

## Implementation

TA-Lib Definition: [`typprice.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/typprice/typprice.c) · [`typprice.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/typprice/typprice.yaml)

| Native | File |
|--------|------|
| C | [`ta_TYPPRICE.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_TYPPRICE.c) |
| Rust | [`typprice.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/typprice.rs) |
| Java | [`Core_TYPPRICE.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/library/fragments/Core_TYPPRICE.java) |

TA-Lib is also available for Python, R and more using a [wrapper](https://ta-lib.org/wrappers/).

## Aliases

Typical Price

## See Also

[MEDPRICE](/functions/medprice) · [WCLPRICE](/functions/wclprice) · [AVGPRICE](/functions/avgprice)
