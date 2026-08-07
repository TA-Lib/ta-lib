---
title: "Three Advancing White Soldiers (CDL3WHITESOLDIERS)"
description: "A three-candle pattern of consecutive white candles with progressively higher closes, each opening within/near the prior body and each with a very short…"
---

## Summary

A three-candle pattern of consecutive white candles with progressively higher closes, each opening within/near the prior body and each with a very short upper shadow. It is a bullish reversal signal. A hit (+100) is bullish, signaling a reversal (most meaningful in a downtrend, which the code does not verify).

## Notes

- Does not verify the prior downtrend the pattern classically assumes for significance.

## Inputs

- `inOpen` — Open price of each bar
- `inHigh` — High price of each bar
- `inLow` — Low price of each bar
- `inClose` — Close price of each bar

## Outputs

- `outInteger` — +100 when the pattern is detected, 0 otherwise; never negative (three white soldiers is always bullish)

## Properties

**Numerical Stability:** [Start-Independent](/functions/stability.md#start-independent)

| Display<br>Flags |
| :-- |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Overlap Input</span> |
| <span class="flag-box">✅</span> **Independent Y-Axis** <span class="flag-tip" tabindex="0" role="note" aria-label="Output is on its own scale, drawn in a separate pane below the price chart." data-tip="Output is on its own scale, drawn in a separate pane below the price chart.">i</span> |
| <span class="flag-box">✅</span> **Candlestick** <span class="flag-tip" tabindex="0" role="note" aria-label="Output is an integer candlestick-pattern signal (e.g. -100 / 0 / +100)." data-tip="Output is an integer candlestick-pattern signal (e.g. -100 / 0 / +100).">i</span> |

## Implementation

TA-Lib Definition: [`cdl3whitesoldiers.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/cdl3whitesoldiers/cdl3whitesoldiers.c) · [`cdl3whitesoldiers.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/cdl3whitesoldiers/cdl3whitesoldiers.yaml)

| Native | File |
|--------|------|
| C | [`ta_CDL3WHITESOLDIERS.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_CDL3WHITESOLDIERS.c) |
| Rust | [`cdl3whitesoldiers.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/cdl3whitesoldiers.rs) |
| Java | [`Core_CDL3WHITESOLDIERS.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/fragments/Core_CDL3WHITESOLDIERS.java) |

TA-Lib is also available for Python, R and more using a [wrapper](/install/#wrappers).

## Aliases

Three Advancing White Soldiers, Three White Soldiers

## See Also

[CDL3BLACKCROWS](/functions/cdl3blackcrows.md) · [CDLADVANCEBLOCK](/functions/cdladvanceblock.md) · [CDLIDENTICAL3CROWS](/functions/cdlidentical3crows.md)
