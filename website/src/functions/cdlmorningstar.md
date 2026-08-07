---
title: "Morning Star (CDLMORNINGSTAR)"
description: "A three-candle bottom-reversal pattern: a long black candle, a small-bodied star gapping down, then a white candle closing well up into the first…"
---

## Summary

A three-candle bottom-reversal pattern: a long black candle, a small-bodied star gapping down, then a white candle closing well up into the first candle's body. Bullish reversal signal. A hit signals a bullish reversal (most meaningful after a downtrend, which the code does not check).

## Notes

- The gap-down is measured between the candles' real bodies, not between their high/low ranges.
- A prior downtrend is not verified.

## Inputs

- `inOpen` — Open price of each bar
- `inHigh` — High price of each bar
- `inLow` — Low price of each bar
- `inClose` — Close price of each bar

## Outputs

- `outInteger` — +100 when the morning star is detected, 0 otherwise. Never negative (pattern is exclusively bullish)

## Parameters

| Parameter | Type | Default | Accepted values | Description |
| --- | --- | --- | --- | --- |
| `optInPenetration` | real | 0.3 | ≥ 0 | Fraction of the 1st candle's body the 3rd close must exceed above the 1st close; larger = deeper penetration required |

## Properties

**Numerical Stability:** [Start-Independent](/functions/stability.md#start-independent)

| Display<br>Flags |
| :-- |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Overlap Input</span> |
| <span class="flag-box">✅</span> **Independent Y-Axis** <span class="flag-tip" tabindex="0" role="note" aria-label="Output is on its own scale, drawn in a separate pane below the price chart." data-tip="Output is on its own scale, drawn in a separate pane below the price chart.">i</span> |
| <span class="flag-box">✅</span> **Candlestick** <span class="flag-tip" tabindex="0" role="note" aria-label="Output is an integer candlestick-pattern signal (e.g. -100 / 0 / +100)." data-tip="Output is an integer candlestick-pattern signal (e.g. -100 / 0 / +100).">i</span> |

## Implementation

TA-Lib Definition: [`cdlmorningstar.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/cdlmorningstar/cdlmorningstar.c) · [`cdlmorningstar.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/cdlmorningstar/cdlmorningstar.yaml)

| Native | File |
|--------|------|
| C | [`ta_CDLMORNINGSTAR.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_CDLMORNINGSTAR.c) |
| Rust | [`cdlmorningstar.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/cdlmorningstar.rs) |
| Java | [`Core_CDLMORNINGSTAR.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/fragments/Core_CDLMORNINGSTAR.java) |

TA-Lib is also available for Python, R and more using a [wrapper](/install/#wrappers).

## Aliases

Morning Star

## See Also

[CDLMORNINGDOJISTAR](/functions/cdlmorningdojistar.md) · [CDLEVENINGSTAR](/functions/cdleveningstar.md) · [CDLABANDONEDBABY](/functions/cdlabandonedbaby.md) · [CDLDOJISTAR](/functions/cdldojistar.md)
