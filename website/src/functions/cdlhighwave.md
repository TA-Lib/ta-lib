---
title: CDLHIGHWAVE
description: "Single-candle pattern: a short real body with both a very long upper and a very long lower shadow. Signals market indecision; the output sign reports only candle color, not a bullish/bearish direction. A hit marks indecision (long-legged candle); not directional - sign encodes only the candle's color."
---

# CDLHIGHWAVE

## Summary

Single-candle pattern: a short real body with both a very long upper and a very long lower shadow. Signals market indecision; the output sign reports only candle color, not a bullish/bearish direction. A hit marks indecision (long-legged candle); not directional - sign encodes only the candle's color.

## Formula

One candle at index i. Hit when all hold: (1) short real body: real body < the BodyShort average; (2) very long upper shadow: upper shadow > the ShadowVeryLong average; (3) very long lower shadow: lower shadow > the ShadowVeryLong average. No color, gap, or trend condition.

## Inputs

- `inOpen` — Open price of each bar
- `inHigh` — High price of each bar
- `inLow` — Low price of each bar
- `inClose` — Close price of each bar

## Outputs

- `outInteger` — On a hit, +100 when the candle is white (close >= open) or -100 when black (close < open); 0 otherwise. Sign denotes color, NOT bull/bear

## Properties

**Numerical Stability:** [Start-Independent](/functions/stability#start-independent)

| Display<br>Flags |
| :-- |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Overlap Input</span> |
| <span class="flag-box">✅</span> **Independent Y-Axis** <span class="flag-tip" tabindex="0" role="note" aria-label="Output is on its own scale, drawn in a separate pane below the price chart." data-tip="Output is on its own scale, drawn in a separate pane below the price chart.">i</span> |
| <span class="flag-box">✅</span> **Candlestick** <span class="flag-tip" tabindex="0" role="note" aria-label="Output is an integer candlestick-pattern signal (e.g. -100 / 0 / +100)." data-tip="Output is an integer candlestick-pattern signal (e.g. -100 / 0 / +100).">i</span> |

## Implementation

TA-Lib Definition: [`cdlhighwave.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/cdlhighwave/cdlhighwave.c) · [`cdlhighwave.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/cdlhighwave/cdlhighwave.yaml)

| Native | File |
|--------|------|
| C | [`ta_CDLHIGHWAVE.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_CDLHIGHWAVE.c) |
| Rust | [`cdlhighwave.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/cdlhighwave.rs) |
| Java | [`Core_CDLHIGHWAVE.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/library/fragments/Core_CDLHIGHWAVE.java) |

TA-Lib is also available for Python, R and more using a [wrapper](https://ta-lib.org/wrappers/).

## Aliases

High-Wave Candle, High Wave

## See Also

[CDLLONGLEGGEDDOJI](/functions/cdllongleggeddoji) · [CDLSPINNINGTOP](/functions/cdlspinningtop) · [CDLRICKSHAWMAN](/functions/cdlrickshawman) · [CDLDOJI](/functions/cdldoji)
