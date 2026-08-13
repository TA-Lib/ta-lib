---
title: "Dragonfly Doji (CDLDRAGONFLYDOJI)"
description: "Single-candle pattern: a doji (open and close nearly equal) sitting at the top of the range, with no meaningful upper shadow and a long lower shadow."
---

## Summary

Single-candle pattern: a doji (open and close nearly equal) sitting at the top of the range, with no meaningful upper shadow and a long lower shadow. A reversal signal, but its bullish/bearish meaning depends on the prior trend (the code does not judge direction). A hit marks a dragonfly doji; treated as a potential reversal, but direction (bullish/bearish) must be read from the trend it appears in.

## Formula

Single candle. realbody <= BodyDoji average (doji body) AND upper shadow < ShadowVeryShort average (no/very short upper shadow) AND lower shadow > ShadowVeryShort average (lower shadow present, not very short). No color, gap, or trend test.

## Notes

- Does not verify the prior trend that determines the pattern's bullish/bearish meaning.

## Inputs

- `inOpen` — Open price of each bar
- `inHigh` — High price of each bar
- `inLow` — Low price of each bar
- `inClose` — Close price of each bar

## Outputs

- `outInteger` — +100 when the pattern is present, 0 otherwise; never -100. The +100 does not itself imply bullishness (must be read against the trend)

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

TA-Lib Definition: [`cdldragonflydoji.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/cdldragonflydoji/cdldragonflydoji.c) · [`cdldragonflydoji.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/cdldragonflydoji/cdldragonflydoji.yaml)

| Native | File |
|--------|------|
| C | [`ta_CDLDRAGONFLYDOJI.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_CDLDRAGONFLYDOJI.c) |
| Rust | [`cdldragonflydoji.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/cdldragonflydoji.rs) |
| Java | [`Core_CDLDRAGONFLYDOJI.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/fragments/Core_CDLDRAGONFLYDOJI.java) |

TA-Lib is also available for Python, R and more using a [wrapper](/install/#wrappers).

## Aliases

Dragonfly Doji

## See Also

[CDLDOJI](/functions/cdldoji.md) · [CDLGRAVESTONEDOJI](/functions/cdlgravestonedoji.md) · [CDLLONGLEGGEDDOJI](/functions/cdllongleggeddoji.md) · [CDLTAKURI](/functions/cdltakuri.md)
