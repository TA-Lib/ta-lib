---
title: ASIN
description: "Element-wise arcsine (inverse sine) of each input value. A vector math transform, not a market indicator."
---

# ASIN

## Summary

Element-wise arcsine (inverse sine) of each input value. A vector math transform, not a market indicator.

## Formula

outReal[i] = asin(inReal[i])

## Inputs

- `inReal` — Input values (domain [-1,1] for a real result)

## Outputs

- `outReal` — Arcsine of each input, in radians

## Properties

| Numerical<br>Stability | Display<br>Flags |
| :-- | :-- |
| <span class="flag-box">✅</span> **Start-Independent** <span class="flag-tip" tabindex="0" role="note" aria-label="The value at a bar does not depend on where your data starts — safe to compare across different-length windows." data-tip="The value at a bar does not depend on where your data starts — safe to compare across different-length windows.">i</span> | <span class="flag-box">☐</span> <span style="opacity:0.5">Overlap Input</span> <span class="flag-tip" tabindex="0" role="note" aria-label="Output is on the same scale as the input price, so it is drawn over the price chart." data-tip="Output is on the same scale as the input price, so it is drawn over the price chart.">i</span> |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Initial Unstable Period</span> <span class="flag-tip" tabindex="0" role="note" aria-label="Early values depend on how much history precedes them but converge as more bars are supplied; tunable via the unstable period." data-tip="Early values depend on how much history precedes them but converge as more bars are supplied; tunable via the unstable period.">i</span> | <span class="flag-box">✅</span> **Independent Y-Axis** <span class="flag-tip" tabindex="0" role="note" aria-label="Output is on its own scale, drawn in a separate pane below the price chart." data-tip="Output is on its own scale, drawn in a separate pane below the price chart.">i</span> |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Path-Dependent</span> <span class="flag-tip" tabindex="0" role="note" aria-label="Built up from the first bar (a running accumulation or path-tracking state machine): the value depends on where your data begins and never converges — don't compare across different windows." data-tip="Built up from the first bar (a running accumulation or path-tracking state machine): the value depends on where your data begins and never converges — don't compare across different windows.">i</span> | <span class="flag-box">☐</span> <span style="opacity:0.5">Candlestick</span> <span class="flag-tip" tabindex="0" role="note" aria-label="Output is an integer candlestick-pattern signal (e.g. -100 / 0 / +100)." data-tip="Output is an integer candlestick-pattern signal (e.g. -100 / 0 / +100).">i</span> |

## Implementation

TA-Lib Definition: [`asin.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/asin/asin.c) · [`asin.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/asin/asin.yaml)

| Native | File |
|--------|------|
| C | [`ta_ASIN.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_ASIN.c) |
| Rust | [`asin.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/asin.rs) |
| Java | [`Core_ASIN.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/library/fragments/Core_ASIN.java) |

TA-Lib is also available for Python, R and more using a [wrapper](https://ta-lib.org/wrappers/).

## Aliases

arcsine, inverse sine

## See Also

[ACOS](/functions/acos) · [ATAN](/functions/atan) · [SIN](/functions/sin) · [COS](/functions/cos)
