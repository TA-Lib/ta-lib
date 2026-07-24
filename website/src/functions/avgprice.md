---
title: AVGPRICE
description: "Average Price: the arithmetic mean of each bar's open, high, low, and close. A price-transform overlap condensing OHLC into a single representative price."
---

# AVGPRICE

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

| Numerical<br>Stability | Display<br>Flags |
| :-- | :-- |
| <span class="flag-box">✅</span> **Start-Independent** <span class="flag-tip" tabindex="0" role="note" aria-label="The value at a bar does not depend on where your data starts — safe to compare across different-length windows." data-tip="The value at a bar does not depend on where your data starts — safe to compare across different-length windows.">i</span> | <span class="flag-box">✅</span> **Overlap Input** <span class="flag-tip" tabindex="0" role="note" aria-label="Output is on the same scale as the input price, so it is drawn over the price chart." data-tip="Output is on the same scale as the input price, so it is drawn over the price chart.">i</span> |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Initial Unstable Period</span> <span class="flag-tip" tabindex="0" role="note" aria-label="Early values depend on how much history precedes them but converge as more bars are supplied; tunable via the unstable period." data-tip="Early values depend on how much history precedes them but converge as more bars are supplied; tunable via the unstable period.">i</span> | <span class="flag-box">☐</span> <span style="opacity:0.5">Independent Y-Axis</span> <span class="flag-tip" tabindex="0" role="note" aria-label="Output is on its own scale, drawn in a separate pane below the price chart." data-tip="Output is on its own scale, drawn in a separate pane below the price chart.">i</span> |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Path-Dependent</span> <span class="flag-tip" tabindex="0" role="note" aria-label="Built up from the first bar (a running accumulation or path-tracking state machine): the value depends on where your data begins and never converges — don't compare across different windows." data-tip="Built up from the first bar (a running accumulation or path-tracking state machine): the value depends on where your data begins and never converges — don't compare across different windows.">i</span> | <span class="flag-box">☐</span> <span style="opacity:0.5">Candlestick</span> <span class="flag-tip" tabindex="0" role="note" aria-label="Output is an integer candlestick-pattern signal (e.g. -100 / 0 / +100)." data-tip="Output is an integer candlestick-pattern signal (e.g. -100 / 0 / +100).">i</span> |

## Implementation

TA-Lib Definition: [`avgprice.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/avgprice/avgprice.c) · [`avgprice.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/avgprice/avgprice.yaml)

| Native | File |
|--------|------|
| C | [`ta_AVGPRICE.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_AVGPRICE.c) |
| Rust | [`avgprice.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/avgprice.rs) |
| Java | [`Core_AVGPRICE.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/library/fragments/Core_AVGPRICE.java) |

TA-Lib is also available for Python, R and more using a [wrapper](https://ta-lib.org/wrappers/).

## Aliases

Average Price

## See Also

[MEDPRICE](/functions/medprice) · [TYPPRICE](/functions/typprice) · [WCLPRICE](/functions/wclprice)
