---
title: "Shooting Star (CDLSHOOTINGSTAR)"
description: "Single-candle pattern: a small real body with a long upper shadow and little-to-no lower shadow that gaps up from the prior candle's real body."
---

## Summary

Single-candle pattern: a small real body with a long upper shadow and little-to-no lower shadow that gaps up from the prior candle's real body. Bearish reversal signal. A hit (-100) flags a bearish reversal at the top of an uptrend.

## Notes

- A preceding uptrend is not verified.

## Inputs

- `inOpen` — Open price of each bar
- `inHigh` — High price of each bar
- `inLow` — Low price of each bar
- `inClose` — Close price of each bar

## Outputs

- `outInteger` — -100 when the shooting star is detected, 0 otherwise. Only ever emits negative (bearish); never +100

## Properties

**Numerical Stability:** [Start-Independent](/functions/stability.md#start-independent)

| Display<br>Flags |
| :-- |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Overlap Input</span> |
| <span class="flag-box">✅</span> **Independent Y-Axis** <span class="flag-tip" tabindex="0" role="note" aria-label="Output is on its own scale, drawn in a separate pane below the price chart." data-tip="Output is on its own scale, drawn in a separate pane below the price chart.">i</span> |
| <span class="flag-box">✅</span> **Candlestick** <span class="flag-tip" tabindex="0" role="note" aria-label="Output is an integer candlestick-pattern signal (e.g. -100 / 0 / +100)." data-tip="Output is an integer candlestick-pattern signal (e.g. -100 / 0 / +100).">i</span> |

## Implementation

TA-Lib Definition: [`cdlshootingstar.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/cdlshootingstar/cdlshootingstar.c) · [`cdlshootingstar.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/cdlshootingstar/cdlshootingstar.yaml)

| Native | File |
|--------|------|
| C | [`ta_CDLSHOOTINGSTAR.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_CDLSHOOTINGSTAR.c) |
| Rust | [`cdlshootingstar.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/cdlshootingstar.rs) |
| Java | [`Core_CDLSHOOTINGSTAR.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/fragments/Core_CDLSHOOTINGSTAR.java) |

TA-Lib is also available for Python, R and more using a [wrapper](/install/#wrappers).

## Aliases

Shooting Star

## See Also

[CDLINVERTEDHAMMER](/functions/cdlinvertedhammer.md) · [CDLHANGINGMAN](/functions/cdlhangingman.md) · [CDLHAMMER](/functions/cdlhammer.md) · [CDLGRAVESTONEDOJI](/functions/cdlgravestonedoji.md)
