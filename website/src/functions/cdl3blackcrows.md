---
title: CDL3BLACKCROWS
description: "A four-bar pattern: a white candle followed by three consecutive black (down) candles with successively lower closes, each opening inside the prior black's real body. It is a bearish reversal signal. A hit (-100) signals a bearish reversal."
---

# CDL3BLACKCROWS

## Summary

A four-bar pattern: a white candle followed by three consecutive black (down) candles with successively lower closes, each opening inside the prior black's real body. It is a bearish reversal signal. A hit (-100) signals a bearish reversal.

## Notes

- Does not verify the prior mature uptrend the pattern classically assumes for significance.

## Inputs

- `inOpen` — Open price of each bar
- `inHigh` — High price of each bar
- `inLow` — Low price of each bar
- `inClose` — Close price of each bar

## Outputs

- `outInteger` — -100 when the bearish pattern is detected, 0 otherwise. Never emits +100

## Properties

**Numerical Stability:** [Start-Independent](/functions/stability#start-independent)

| Display<br>Flags |
| :-- |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Overlap Input</span> |
| <span class="flag-box">✅</span> **Independent Y-Axis** <span class="flag-tip" tabindex="0" role="note" aria-label="Output is on its own scale, drawn in a separate pane below the price chart." data-tip="Output is on its own scale, drawn in a separate pane below the price chart.">i</span> |
| <span class="flag-box">✅</span> **Candlestick** <span class="flag-tip" tabindex="0" role="note" aria-label="Output is an integer candlestick-pattern signal (e.g. -100 / 0 / +100)." data-tip="Output is an integer candlestick-pattern signal (e.g. -100 / 0 / +100).">i</span> |

## Implementation

TA-Lib Definition: [`cdl3blackcrows.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/cdl3blackcrows/cdl3blackcrows.c) · [`cdl3blackcrows.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/cdl3blackcrows/cdl3blackcrows.yaml)

| Native | File |
|--------|------|
| C | [`ta_CDL3BLACKCROWS.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_CDL3BLACKCROWS.c) |
| Rust | [`cdl3blackcrows.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/cdl3blackcrows.rs) |
| Java | [`Core_CDL3BLACKCROWS.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/fragments/Core_CDL3BLACKCROWS.java) |

TA-Lib is also available for Python, R and more using a [wrapper](https://ta-lib.org/wrappers/).

## Aliases

Three Black Crows, 3 Black Crows

## See Also

[CDL3WHITESOLDIERS](/functions/cdl3whitesoldiers) · [CDLIDENTICAL3CROWS](/functions/cdlidentical3crows) · [CDLADVANCEBLOCK](/functions/cdladvanceblock)
