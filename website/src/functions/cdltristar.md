---
title: "Tristar Pattern (CDLTRISTAR)"
description: "A three-candle pattern of three consecutive doji where the middle doji is a star (its body gaps away from the first)."
---

## Summary

A three-candle pattern of three consecutive doji where the middle doji is a star (its body gaps away from the first). Bullish or bearish reversal signal. +100 = bullish reversal (middle doji gapped down), -100 = bearish reversal (middle doji gapped up).

## Notes

- This reversal pattern does not verify the prior trend it classically assumes.

## Inputs

- `inOpen` — Open price of each bar
- `inHigh` — High price of each bar
- `inLow` — Low price of each bar
- `inClose` — Close price of each bar

## Outputs

- `outInteger` — +100 (bullish, star gapped down), -100 (bearish, star gapped up), or 0 when no pattern. Both signs are emitted

## Properties

**Numerical Stability:** [Start-Independent](/functions/stability.md#start-independent)

<div class="flag-table">

|  |
| :-- |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Overlap Input</span> |
| <span class="flag-box">✅</span> **Independent Y-Axis** <span class="flag-tip" tabindex="0" role="note" aria-label="Output is on its own scale, drawn in a separate pane below the price chart." data-tip="Output is on its own scale, drawn in a separate pane below the price chart.">i</span> |
| <span class="flag-box">✅</span> **Candlestick** <span class="flag-tip" tabindex="0" role="note" aria-label="Output is an integer candlestick-pattern signal (e.g. -100 / 0 / +100)." data-tip="Output is an integer candlestick-pattern signal (e.g. -100 / 0 / +100).">i</span> |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Can Output NaN or ±Inf</span> |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Identity at Period 1</span> |

</div>

## Implementation

TA-Lib Definition: [`cdltristar.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/cdltristar/cdltristar.c) · [`cdltristar.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/cdltristar/cdltristar.yaml)

| Native | File |
|--------|------|
| C | [`ta_CDLTRISTAR.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_CDLTRISTAR.c) |
| Rust | [`cdltristar.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/cdltristar.rs) |
| Java | [`Core_CDLTRISTAR.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/fragments/Core_CDLTRISTAR.java) |

TA-Lib is also available for Python, R and more using a [wrapper](/install/#wrappers).

## Aliases

Tristar Pattern, Tri-Star

## See Also

[CDLDOJI](/functions/cdldoji.md) · [CDLDOJISTAR](/functions/cdldojistar.md) · [CDLMORNINGDOJISTAR](/functions/cdlmorningdojistar.md) · [CDLEVENINGDOJISTAR](/functions/cdleveningdojistar.md)
