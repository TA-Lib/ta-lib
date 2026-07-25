---
title: CDLSTICKSANDWICH
description: "A three-candle bullish reversal pattern: two black candles (1st and 3rd) sandwiching a white candle, where the 3rd black candle closes at the same level as the 1st (the \"bread\"). A hit signals a bullish reversal (code comment notes it is significant in a downtrend, which the function does not verify)."
---

# CDLSTICKSANDWICH

## Summary

A three-candle bullish reversal pattern: two black candles (1st and 3rd) sandwiching a white candle, where the 3rd black candle closes at the same level as the 1st (the "bread"). A hit signals a bullish reversal (code comment notes it is significant in a downtrend, which the function does not verify).

## Inputs

- `inOpen` — Open price of each bar
- `inHigh` — High price of each bar
- `inLow` — Low price of each bar
- `inClose` — Close price of each bar

## Outputs

- `outInteger` — +100 when the pattern is present, 0 otherwise. Never -100 — Stick Sandwich is always bullish

## Properties

**Numerical Stability:** [Start-Independent](/functions/stability#start-independent)

| Display<br>Flags |
| :-- |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Overlap Input</span> |
| <span class="flag-box">✅</span> **Independent Y-Axis** <span class="flag-tip" tabindex="0" role="note" aria-label="Output is on its own scale, drawn in a separate pane below the price chart." data-tip="Output is on its own scale, drawn in a separate pane below the price chart.">i</span> |
| <span class="flag-box">✅</span> **Candlestick** <span class="flag-tip" tabindex="0" role="note" aria-label="Output is an integer candlestick-pattern signal (e.g. -100 / 0 / +100)." data-tip="Output is an integer candlestick-pattern signal (e.g. -100 / 0 / +100).">i</span> |

## Implementation

TA-Lib Definition: [`cdlsticksandwich.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/cdlsticksandwich/cdlsticksandwich.c) · [`cdlsticksandwich.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/cdlsticksandwich/cdlsticksandwich.yaml)

| Native | File |
|--------|------|
| C | [`ta_CDLSTICKSANDWICH.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_CDLSTICKSANDWICH.c) |
| Rust | [`cdlsticksandwich.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/cdlsticksandwich.rs) |
| Java | [`Core_CDLSTICKSANDWICH.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/library/fragments/Core_CDLSTICKSANDWICH.java) |

TA-Lib is also available for Python, R and more using a [wrapper](https://ta-lib.org/wrappers/).

## Aliases

Stick Sandwich

## See Also

[CDLMATCHINGLOW](/functions/cdlmatchinglow) · [CDLHOMINGPIGEON](/functions/cdlhomingpigeon)
