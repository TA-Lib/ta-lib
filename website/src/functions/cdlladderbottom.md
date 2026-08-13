---
title: "Ladder Bottom (CDLLADDERBOTTOM)"
description: "Five-candle bullish reversal pattern: three consecutively lower black candles, a fourth black candle with a non-very-short upper shadow, then a white…"
---

## Summary

Five-candle bullish reversal pattern: three consecutively lower black candles, a fourth black candle with a non-very-short upper shadow, then a white candle that opens above the prior open and closes above the prior high. Signals a potential bottom reversal. A hit (+100) is a bullish reversal signal, most meaningful after a downtrend.

## Notes

- Does not verify the preceding downtrend that this bullish reversal classically assumes.

## Inputs

- `inOpen` — Open price of each bar
- `inHigh` — High price of each bar
- `inLow` — Low price of each bar
- `inClose` — Close price of each bar

## Outputs

- `outInteger` — +100 on a detected ladder bottom, 0 otherwise. Only ever emits +100 (never -100); inherently bullish

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

TA-Lib Definition: [`cdlladderbottom.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/cdlladderbottom/cdlladderbottom.c) · [`cdlladderbottom.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/cdlladderbottom/cdlladderbottom.yaml)

| Native | File |
|--------|------|
| C | [`ta_CDLLADDERBOTTOM.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_CDLLADDERBOTTOM.c) |
| Rust | [`cdlladderbottom.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/cdlladderbottom.rs) |
| Java | [`Core_CDLLADDERBOTTOM.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/fragments/Core_CDLLADDERBOTTOM.java) |

TA-Lib is also available for Python, R and more using a [wrapper](/install/#wrappers).

## Aliases

Ladder Bottom

## See Also

[CDL3BLACKCROWS](/functions/cdl3blackcrows.md) · [CDLMATCHINGLOW](/functions/cdlmatchinglow.md) · [CDLBREAKAWAY](/functions/cdlbreakaway.md)
