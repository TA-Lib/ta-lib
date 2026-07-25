---
title: CDLUNIQUE3RIVER
description: "A three-candle bullish reversal pattern: a long black candle, then a black harami candle that makes a lower low, then a small white candle. Signals a potential bullish reversal, ideally in a downtrend (trend not checked by the code). A hit (+100) marks a bullish reversal; significant in a downtrend, which the function does not verify."
---

# CDLUNIQUE3RIVER

## Summary

A three-candle bullish reversal pattern: a long black candle, then a black harami candle that makes a lower low, then a small white candle. Signals a potential bullish reversal, ideally in a downtrend (trend not checked by the code). A hit (+100) marks a bullish reversal; significant in a downtrend, which the function does not verify.

## Inputs

- `inOpen` — Open price of each bar
- `inHigh` — High price of each bar
- `inLow` — Low price of each bar
- `inClose` — Close price of each bar

## Outputs

- `outInteger` — +100 when the pattern is present, 0 otherwise. Bullish-only: never emits -100

## Properties

**Numerical Stability:** [Start-Independent](/functions/stability#start-independent)

| Display<br>Flags |
| :-- |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Overlap Input</span> |
| <span class="flag-box">✅</span> **Independent Y-Axis** <span class="flag-tip" tabindex="0" role="note" aria-label="Output is on its own scale, drawn in a separate pane below the price chart." data-tip="Output is on its own scale, drawn in a separate pane below the price chart.">i</span> |
| <span class="flag-box">✅</span> **Candlestick** <span class="flag-tip" tabindex="0" role="note" aria-label="Output is an integer candlestick-pattern signal (e.g. -100 / 0 / +100)." data-tip="Output is an integer candlestick-pattern signal (e.g. -100 / 0 / +100).">i</span> |

## Implementation

TA-Lib Definition: [`cdlunique3river.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/cdlunique3river/cdlunique3river.c) · [`cdlunique3river.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/cdlunique3river/cdlunique3river.yaml)

| Native | File |
|--------|------|
| C | [`ta_CDLUNIQUE3RIVER.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_CDLUNIQUE3RIVER.c) |
| Rust | [`cdlunique3river.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/cdlunique3river.rs) |
| Java | [`Core_CDLUNIQUE3RIVER.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/library/fragments/Core_CDLUNIQUE3RIVER.java) |

TA-Lib is also available for Python, R and more using a [wrapper](https://ta-lib.org/wrappers/).

## Aliases

Unique 3 River, Unique Three River Bottom

## See Also

[CDLHARAMI](/functions/cdlharami) · [CDLHOMINGPIGEON](/functions/cdlhomingpigeon) · [CDL3INSIDE](/functions/cdl3inside)
