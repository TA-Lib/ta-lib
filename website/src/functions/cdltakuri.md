---
title: "Takuri (Dragonfly Doji with very long lower shadow) (CDLTAKURI)"
description: "Single-candle pattern: a doji whose open and close sit at the high (no/very short upper shadow) with a very long lower shadow, i.e. a dragonfly doji with…"
---

## Summary

Single-candle pattern: a doji whose open and close sit at the high (no/very short upper shadow) with a very long lower shadow, i.e. a dragonfly doji with an exceptionally long lower shadow. Emitted as a positive signal, but its directional meaning depends on the prevailing trend, which the code does not check. A hit marks a takuri (dragonfly-doji) line; a potential reversal only when read against the trend (typically a bottom/bullish reversal after a downtrend), which the code itself does not verify.

## Inputs

- `inOpen` — Open price of each bar
- `inHigh` — High price of each bar
- `inLow` — Low price of each bar
- `inClose` — Close price of each bar

## Outputs

- `outInteger` — +100 when the takuri pattern is detected, 0 otherwise. Never negative; the positive sign is a convention and does not by itself imply bullishness

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

TA-Lib Definition: [`cdltakuri.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/cdltakuri/cdltakuri.c) · [`cdltakuri.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/cdltakuri/cdltakuri.yaml)

| Native | File |
|--------|------|
| C | [`ta_CDLTAKURI.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_CDLTAKURI.c) |
| Rust | [`cdltakuri.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/cdltakuri.rs) |
| Java | [`Core_CDLTAKURI.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/fragments/Core_CDLTAKURI.java) |

TA-Lib is also available for Python, R and more using a [wrapper](/install/#wrappers).

## Aliases

Takuri, Takuri line

## See Also

[CDLDRAGONFLYDOJI](/functions/cdldragonflydoji.md) · [CDLDOJI](/functions/cdldoji.md) · [CDLHAMMER](/functions/cdlhammer.md) · [CDLGRAVESTONEDOJI](/functions/cdlgravestonedoji.md)
