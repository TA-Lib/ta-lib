---
title: DIV
description: "Element-wise division of two input series. Computes the quotient of corresponding values from two real inputs."
---

# DIV

## Summary

Element-wise division of two input series. Computes the quotient of corresponding values from two real inputs.

## Formula

outReal[i] = inReal0[i] / inReal1[i]

## Inputs

- `inReal0` — Dividend (numerator) series
- `inReal1` — Divisor (denominator) series

## Outputs

- `outReal` — Per-element quotient inReal0/inReal1

## Properties

| Numerical<br>Stability | Display<br>Flags |
| :-- | :-- |
| <span class="flag-box">✅</span> **Start-Independent** <span class="flag-tip" tabindex="0" role="note" aria-label="The value at a bar does not depend on where your data starts — safe to compare across different-length windows." data-tip="The value at a bar does not depend on where your data starts — safe to compare across different-length windows.">i</span> | <span class="flag-box">☐</span> <span style="opacity:0.5">Overlap Input</span> <span class="flag-tip" tabindex="0" role="note" aria-label="Output is on the same scale as the input price, so it is drawn over the price chart." data-tip="Output is on the same scale as the input price, so it is drawn over the price chart.">i</span> |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Initial Unstable Period</span> <span class="flag-tip" tabindex="0" role="note" aria-label="Early values depend on how much history precedes them but converge as more bars are supplied; tunable via the unstable period." data-tip="Early values depend on how much history precedes them but converge as more bars are supplied; tunable via the unstable period.">i</span> | <span class="flag-box">✅</span> **Independent Y-Axis** <span class="flag-tip" tabindex="0" role="note" aria-label="Output is on its own scale, drawn in a separate pane below the price chart." data-tip="Output is on its own scale, drawn in a separate pane below the price chart.">i</span> |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Path-Dependent</span> <span class="flag-tip" tabindex="0" role="note" aria-label="Built up from the first bar (a running accumulation or path-tracking state machine): the value depends on where your data begins and never converges — don't compare across different windows." data-tip="Built up from the first bar (a running accumulation or path-tracking state machine): the value depends on where your data begins and never converges — don't compare across different windows.">i</span> | <span class="flag-box">☐</span> <span style="opacity:0.5">Candlestick</span> <span class="flag-tip" tabindex="0" role="note" aria-label="Output is an integer candlestick-pattern signal (e.g. -100 / 0 / +100)." data-tip="Output is an integer candlestick-pattern signal (e.g. -100 / 0 / +100).">i</span> |

## Implementation

TA-Lib Definition: [`div.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/div/div.c) · [`div.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/div/div.yaml)

| Native | File |
|--------|------|
| C | [`ta_DIV.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_DIV.c) |
| Rust | [`div.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/div.rs) |
| Java | [`Core_DIV.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/library/fragments/Core_DIV.java) |

TA-Lib is also available for Python, R and more using a [wrapper](https://ta-lib.org/wrappers/).

## Aliases

Vector Arithmetic Divide, Divide

## See Also

[MULT](/functions/mult) · [ADD](/functions/add) · [SUB](/functions/sub)
