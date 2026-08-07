---
title: "Three Stars In The South (CDL3STARSINSOUTH)"
description: "A three-candle bullish reversal pattern of three consecutive black candles that progressively shrink and stabilize: a long black candle with a long lower…"
---

## Summary

A three-candle bullish reversal pattern of three consecutive black candles that progressively shrink and stabilize: a long black candle with a long lower shadow, a smaller black candle probing lower, then a small black marubozu contained within the second candle's range. A hit (+100) signals a bullish reversal; per the code comment it is meaningful in a downtrend, but the function does not verify prior trend.

## Notes

- Does not verify the prior downtrend the pattern classically assumes for significance.

## Inputs

- `inOpen` — Open price of each bar
- `inHigh` — High price of each bar
- `inLow` — Low price of each bar
- `inClose` — Close price of each bar

## Outputs

- `outInteger` — +100 on the bar where the pattern completes (always bullish), 0 otherwise. Never emits -100

## Properties

**Numerical Stability:** [Start-Independent](/functions/stability.md#start-independent)

| Display<br>Flags |
| :-- |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Overlap Input</span> |
| <span class="flag-box">✅</span> **Independent Y-Axis** <span class="flag-tip" tabindex="0" role="note" aria-label="Output is on its own scale, drawn in a separate pane below the price chart." data-tip="Output is on its own scale, drawn in a separate pane below the price chart.">i</span> |
| <span class="flag-box">✅</span> **Candlestick** <span class="flag-tip" tabindex="0" role="note" aria-label="Output is an integer candlestick-pattern signal (e.g. -100 / 0 / +100)." data-tip="Output is an integer candlestick-pattern signal (e.g. -100 / 0 / +100).">i</span> |

## Implementation

TA-Lib Definition: [`cdl3starsinsouth.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/cdl3starsinsouth/cdl3starsinsouth.c) · [`cdl3starsinsouth.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/cdl3starsinsouth/cdl3starsinsouth.yaml)

| Native | File |
|--------|------|
| C | [`ta_CDL3STARSINSOUTH.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_CDL3STARSINSOUTH.c) |
| Rust | [`cdl3starsinsouth.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/cdl3starsinsouth.rs) |
| Java | [`Core_CDL3STARSINSOUTH.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/fragments/Core_CDL3STARSINSOUTH.java) |

TA-Lib is also available for Python, R and more using a [wrapper](/install/#wrappers).

## Aliases

Three Stars In The South

## See Also

[CDL3BLACKCROWS](/functions/cdl3blackcrows.md) · [CDLIDENTICAL3CROWS](/functions/cdlidentical3crows.md) · [CDL3WHITESOLDIERS](/functions/cdl3whitesoldiers.md)
