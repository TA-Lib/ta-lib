---
title: WCLPRICE
description: "Weighted Close Price: a per-bar price average giving the close double weight relative to high and low."
---

# WCLPRICE

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

| Numerical<br>Stability | Display<br>Flags |
| :-- | :-- |
| <span class="flag-box">✅</span> **Start-Independent** <span class="flag-tip" tabindex="0" role="note" aria-label="The value at a bar does not depend on where your data starts — safe to compare across different-length windows." data-tip="The value at a bar does not depend on where your data starts — safe to compare across different-length windows.">i</span> | <span class="flag-box">✅</span> **Overlap Input** <span class="flag-tip" tabindex="0" role="note" aria-label="Output is on the same scale as the input price, so it is drawn over the price chart." data-tip="Output is on the same scale as the input price, so it is drawn over the price chart.">i</span> |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Initial Unstable Period</span> <span class="flag-tip" tabindex="0" role="note" aria-label="Early values depend on how much history precedes them but converge as more bars are supplied; tunable via the unstable period." data-tip="Early values depend on how much history precedes them but converge as more bars are supplied; tunable via the unstable period.">i</span> | <span class="flag-box">☐</span> <span style="opacity:0.5">Independent Y-Axis</span> <span class="flag-tip" tabindex="0" role="note" aria-label="Output is on its own scale, drawn in a separate pane below the price chart." data-tip="Output is on its own scale, drawn in a separate pane below the price chart.">i</span> |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Path-Dependent</span> <span class="flag-tip" tabindex="0" role="note" aria-label="Built up from the first bar (a running accumulation or path-tracking state machine): the value depends on where your data begins and never converges — don't compare across different windows." data-tip="Built up from the first bar (a running accumulation or path-tracking state machine): the value depends on where your data begins and never converges — don't compare across different windows.">i</span> | <span class="flag-box">☐</span> <span style="opacity:0.5">Candlestick</span> <span class="flag-tip" tabindex="0" role="note" aria-label="Output is an integer candlestick-pattern signal (e.g. -100 / 0 / +100)." data-tip="Output is an integer candlestick-pattern signal (e.g. -100 / 0 / +100).">i</span> |

## Implementation

TA-Lib Definition: [`wclprice.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/wclprice/wclprice.c) · [`wclprice.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/wclprice/wclprice.yaml)

| Native | File |
|--------|------|
| C | [`ta_WCLPRICE.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_WCLPRICE.c) |
| Rust | [`wclprice.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/wclprice.rs) |
| Java | [`Core_WCLPRICE.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/library/fragments/Core_WCLPRICE.java) |

TA-Lib is also available for Python, R and more using a [wrapper](https://ta-lib.org/wrappers/).

## Aliases

Weighted Close Price, Weighted Close

## See Also

[TYPPRICE](/functions/typprice) · [MEDPRICE](/functions/medprice) · [AVGPRICE](/functions/avgprice)
