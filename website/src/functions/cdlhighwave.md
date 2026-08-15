---
title: "High-Wave Candle (CDLHIGHWAVE)"
description: "Single-candle pattern: a short real body with both a very long upper and a very long lower shadow."
---

## Summary

Single-candle pattern: a short real body with both a very long upper and a very long lower shadow. Signals market indecision; the output sign reports only candle color, not a bullish/bearish direction. A hit marks indecision (long-legged candle); not directional - sign encodes only the candle's color.

## Formula

One candle at index i. Hit when all hold: (1) short real body: real body < the BodyShort average; (2) very long upper shadow: upper shadow > the ShadowVeryLong average; (3) very long lower shadow: lower shadow > the ShadowVeryLong average. No color, gap, or trend condition.

## Notes

- Bulkowski's testing found the High-Wave candle acts as a reversal only 51% of the time — statistically indistinguishable from random — which he notes actually agrees with the pattern's theoretical meaning of pure indecision. ([thepatternsite.com](https://thepatternsite.com/HighWave.html))

## Inputs

- `inOpen` — Open price of each bar
- `inHigh` — High price of each bar
- `inLow` — Low price of each bar
- `inClose` — Close price of each bar

## Outputs

- `outInteger` — On a hit, +100 when the candle is white (close >= open) or -100 when black (close < open); 0 otherwise. Sign denotes color, NOT bull/bear

## Output Values

| Value | Meaning |
|-------|---------|
| -100 | High-Wave candle (black) — sharp indecision after a volatile session; sign marks the candle's color only, not a bullish/bearish call |
| 0 | No pattern |
| 100 | High-Wave candle (white) — sharp indecision after a volatile session; sign marks the candle's color only, not a bullish/bearish call |

## Properties

**Numerical Stability:** [Start-Independent](/functions/stability.md#start-independent)

<div class="flag-table">

|  |
| :-- |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Overlap Input</span> |
| <span class="flag-box">✅</span> **Independent Y-Axis** <span class="flag-tip" tabindex="0" role="note" aria-label="Output is on its own scale, drawn in a separate pane below the price chart." data-tip="Output is on its own scale, drawn in a separate pane below the price chart.">i</span> |
| <span class="flag-box">✅</span> **Candlestick** <span class="flag-tip" tabindex="0" role="note" aria-label="Output is an integer candlestick-pattern signal (e.g. -100 / 0 / +100)." data-tip="Output is an integer candlestick-pattern signal (e.g. -100 / 0 / +100).">i</span> |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Can Output NaN or ±Inf</span> |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Identity at Period 1</span> |

</div>

## Implementation

TA-Lib Definition: [`cdlhighwave.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/cdlhighwave/cdlhighwave.c) · [`cdlhighwave.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/cdlhighwave/cdlhighwave.yaml)

| Native | File |
|--------|------|
| C | [`ta_CDLHIGHWAVE.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_CDLHIGHWAVE.c) |
| Rust | [`cdlhighwave.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/cdlhighwave.rs) |
| Java | [`Core_CDLHIGHWAVE.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/fragments/Core_CDLHIGHWAVE.java) |

TA-Lib is also available for Python, R and more using a [wrapper](/install/#wrappers).

## Aliases

High-Wave Candle, High Wave

## See Also

[CDLLONGLEGGEDDOJI](/functions/cdllongleggeddoji.md) · [CDLSPINNINGTOP](/functions/cdlspinningtop.md) · [CDLRICKSHAWMAN](/functions/cdlrickshawman.md) · [CDLDOJI](/functions/cdldoji.md)
