---
title: CDL3OUTSIDE
description: "A three-candle pattern: an engulfing pair (candle 2's body fully engulfs candle 1's body) followed by a third candle that confirms in the engulfing direction. Signals a bullish reversal (Three Outside Up) or bearish reversal (Three Outside Down). +100 = bullish reversal (Three Outside Up); -100 = bearish reversal (Three Outside Down)."
---

# CDL3OUTSIDE

## Summary

A three-candle pattern: an engulfing pair (candle 2's body fully engulfs candle 1's body) followed by a third candle that confirms in the engulfing direction. Signals a bullish reversal (Three Outside Up) or bearish reversal (Three Outside Down). +100 = bullish reversal (Three Outside Up); -100 = bearish reversal (Three Outside Down).

## Notes

- Does not verify the prior trend the pattern classically assumes (three outside up is meaningful in a downtrend, three outside down in an uptrend).

## Inputs

- `inOpen` — Open price of each bar
- `inHigh` — High price of each bar
- `inLow` — Low price of each bar
- `inClose` — Close price of each bar

## Outputs

- `outInteger` — +100 for Three Outside Up (bullish), -100 for Three Outside Down (bearish), 0 when no pattern. Emits both signs; value is candle i-1's color * 100

## Properties

**Numerical Stability:** [Start-Independent](/functions/stability#start-independent)

| Display<br>Flags |
| :-- |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Overlap Input</span> |
| <span class="flag-box">✅</span> **Independent Y-Axis** <span class="flag-tip" tabindex="0" role="note" aria-label="Output is on its own scale, drawn in a separate pane below the price chart." data-tip="Output is on its own scale, drawn in a separate pane below the price chart.">i</span> |
| <span class="flag-box">✅</span> **Candlestick** <span class="flag-tip" tabindex="0" role="note" aria-label="Output is an integer candlestick-pattern signal (e.g. -100 / 0 / +100)." data-tip="Output is an integer candlestick-pattern signal (e.g. -100 / 0 / +100).">i</span> |

## Implementation

TA-Lib Definition: [`cdl3outside.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/cdl3outside/cdl3outside.c) · [`cdl3outside.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/cdl3outside/cdl3outside.yaml)

| Native | File |
|--------|------|
| C | [`ta_CDL3OUTSIDE.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_CDL3OUTSIDE.c) |
| Rust | [`cdl3outside.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/cdl3outside.rs) |
| Java | [`Core_CDL3OUTSIDE.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/fragments/Core_CDL3OUTSIDE.java) |

TA-Lib is also available for Python, R and more using a [wrapper](https://ta-lib.org/wrappers/).

## Aliases

Three Outside Up/Down, Three Outside

## See Also

[CDL3INSIDE](/functions/cdl3inside) · [CDLENGULFING](/functions/cdlengulfing) · [CDL3LINESTRIKE](/functions/cdl3linestrike)
