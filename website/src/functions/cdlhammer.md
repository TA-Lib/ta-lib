---
title: "Hammer (CDLHAMMER)"
description: "Single-candle pattern: a small real body at the top of the range with a long lower shadow and little or no upper shadow, sitting at or near the prior…"
---

## Summary

Single-candle pattern: a small real body at the top of the range with a long lower shadow and little or no upper shadow, sitting at or near the prior candle's low. Bullish reversal signal. A hit (+100) flags a potential bullish reversal.

## Notes

- Does not verify the preceding downtrend that the pattern classically assumes; confirm the trend context yourself.

## Inputs

- `inOpen` — Open price of each bar
- `inHigh` — High price of each bar
- `inLow` — Low price of each bar
- `inClose` — Close price of each bar

## Outputs

- `outInteger` — +100 when the hammer is detected, 0 otherwise. Bullish only; never emits -100

## Properties

**Numerical Stability:** [Start-Independent](/functions/stability.md#start-independent)

| Display<br>Flags |
| :-- |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Overlap Input</span> |
| <span class="flag-box">✅</span> **Independent Y-Axis** <span class="flag-tip" tabindex="0" role="note" aria-label="Output is on its own scale, drawn in a separate pane below the price chart." data-tip="Output is on its own scale, drawn in a separate pane below the price chart.">i</span> |
| <span class="flag-box">✅</span> **Candlestick** <span class="flag-tip" tabindex="0" role="note" aria-label="Output is an integer candlestick-pattern signal (e.g. -100 / 0 / +100)." data-tip="Output is an integer candlestick-pattern signal (e.g. -100 / 0 / +100).">i</span> |

## Implementation

TA-Lib Definition: [`cdlhammer.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/cdlhammer/cdlhammer.c) · [`cdlhammer.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/cdlhammer/cdlhammer.yaml)

| Native | File |
|--------|------|
| C | [`ta_CDLHAMMER.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_CDLHAMMER.c) |
| Rust | [`cdlhammer.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/cdlhammer.rs) |
| Java | [`Core_CDLHAMMER.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/fragments/Core_CDLHAMMER.java) |

TA-Lib is also available for Python, R and more using a [wrapper](/install/#wrappers).

## Aliases

Hammer

## See Also

[CDLINVERTEDHAMMER](/functions/cdlinvertedhammer.md) · [CDLHANGINGMAN](/functions/cdlhangingman.md) · [CDLTAKURI](/functions/cdltakuri.md)
