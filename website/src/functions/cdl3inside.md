---
title: "Three Inside Up/Down (CDL3INSIDE)"
description: "A three-candle reversal pattern: a long real body, then a short real body totally engulfed by it (a harami), then a third candle of opposite color to the…"
---

## Summary

A three-candle reversal pattern: a long real body, then a short real body totally engulfed by it (a harami), then a third candle of opposite color to the first that closes past the first candle's open. Signals a bullish (three inside up) or bearish (three inside down) reversal. A hit is a reversal signal: +100 = three inside up (bullish, significant in a downtrend); -100 = three inside down (bearish, significant in an uptrend).

## Notes

- Does not verify the prior trend the pattern classically assumes (three inside up is meaningful in a downtrend, three inside down in an uptrend).

## Inputs

- `inOpen` — Open price of each bar
- `inHigh` — High price of each bar
- `inLow` — Low price of each bar
- `inClose` — Close price of each bar

## Outputs

- `outInteger` — +100 for three inside up (bullish reversal, first candle black), -100 for three inside down (bearish reversal, first candle white), 0 when no pattern. Computed as -candlecolor(1st)*100

## Properties

**Numerical Stability:** [Start-Independent](/functions/stability.md#start-independent)

| Display<br>Flags |
| :-- |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Overlap Input</span> |
| <span class="flag-box">✅</span> **Independent Y-Axis** <span class="flag-tip" tabindex="0" role="note" aria-label="Output is on its own scale, drawn in a separate pane below the price chart." data-tip="Output is on its own scale, drawn in a separate pane below the price chart.">i</span> |
| <span class="flag-box">✅</span> **Candlestick** <span class="flag-tip" tabindex="0" role="note" aria-label="Output is an integer candlestick-pattern signal (e.g. -100 / 0 / +100)." data-tip="Output is an integer candlestick-pattern signal (e.g. -100 / 0 / +100).">i</span> |

## Implementation

TA-Lib Definition: [`cdl3inside.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/cdl3inside/cdl3inside.c) · [`cdl3inside.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/cdl3inside/cdl3inside.yaml)

| Native | File |
|--------|------|
| C | [`ta_CDL3INSIDE.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_CDL3INSIDE.c) |
| Rust | [`cdl3inside.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/cdl3inside.rs) |
| Java | [`Core_CDL3INSIDE.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/fragments/Core_CDL3INSIDE.java) |

TA-Lib is also available for Python, R and more using a [wrapper](/install/#wrappers).

## Aliases

Three Inside Up/Down, Three Inside, Three Inside Up, Three Inside Down

## See Also

[CDLHARAMI](/functions/cdlharami.md) · [CDL3OUTSIDE](/functions/cdl3outside.md) · [CDLENGULFING](/functions/cdlengulfing.md)
