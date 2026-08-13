---
title: "Long Legged Doji (CDLLONGLEGGEDDOJI)"
description: "Single-candle doji (open ~ close) with at least one long shadow. Signals market indecision, not a directional bias."
---

## Summary

Single-candle doji (open ~ close) with at least one long shadow. Signals market indecision, not a directional bias. Marks indecision/uncertainty; not inherently bullish or bearish despite the positive sign.

## Formula

One candle. Hit when: real body <= BodyDoji average (doji body) AND (lower shadow > ShadowLong average OR upper shadow > ShadowLong average), i.e. at least one long shadow.

## Notes

- Only one long shadow (upper or lower) is required, whereas the classic pattern shows both long upper and lower shadows.

## Inputs

- `inOpen` — Open price of each bar
- `inHigh` — High price of each bar
- `inLow` — Low price of each bar
- `inClose` — Close price of each bar

## Outputs

- `outInteger` — +100 when the pattern is present, 0 otherwise. Only +100 is emitted; the code never emits -100, and the positive sign does NOT mean bullish

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

TA-Lib Definition: [`cdllongleggeddoji.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/cdllongleggeddoji/cdllongleggeddoji.c) · [`cdllongleggeddoji.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/cdllongleggeddoji/cdllongleggeddoji.yaml)

| Native | File |
|--------|------|
| C | [`ta_CDLLONGLEGGEDDOJI.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_CDLLONGLEGGEDDOJI.c) |
| Rust | [`cdllongleggeddoji.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/cdllongleggeddoji.rs) |
| Java | [`Core_CDLLONGLEGGEDDOJI.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/fragments/Core_CDLLONGLEGGEDDOJI.java) |

TA-Lib is also available for Python, R and more using a [wrapper](/install/#wrappers).

## Aliases

Long Legged Doji

## See Also

[CDLDOJI](/functions/cdldoji.md) · [CDLGRAVESTONEDOJI](/functions/cdlgravestonedoji.md) · [CDLDRAGONFLYDOJI](/functions/cdldragonflydoji.md) · [CDLRICKSHAWMAN](/functions/cdlrickshawman.md)
