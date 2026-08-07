---
title: "Evening Doji Star (CDLEVENINGDOJISTAR)"
description: "A three-candle bearish reversal pattern: a long white candle, a doji that gaps up (the star), then a black candle closing well down into the first…"
---

## Summary

A three-candle bearish reversal pattern: a long white candle, a doji that gaps up (the star), then a black candle closing well down into the first candle's body. A stricter Evening Star whose middle candle must be a doji. Hit (-100) signals a bearish top reversal.

## Notes

- Does not verify the preceding uptrend the bearish reversal classically assumes.

## Inputs

- `inOpen` — Open price of each bar
- `inHigh` — High price of each bar
- `inLow` — Low price of each bar
- `inClose` — Close price of each bar

## Outputs

- `outInteger` — -100 when the pattern is detected, 0 otherwise. Always bearish; never emits +100

## Parameters

| Parameter | Type | Default | Accepted values | Description |
| --- | --- | --- | --- | --- |
| `optInPenetration` | real | 0.3 | ≥ 0 | Fraction of the 1st real body the 3rd candle's close must penetrate; larger demands a deeper close into the first body |

## Properties

**Numerical Stability:** [Start-Independent](/functions/stability.md#start-independent)

| Display<br>Flags |
| :-- |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Overlap Input</span> |
| <span class="flag-box">✅</span> **Independent Y-Axis** <span class="flag-tip" tabindex="0" role="note" aria-label="Output is on its own scale, drawn in a separate pane below the price chart." data-tip="Output is on its own scale, drawn in a separate pane below the price chart.">i</span> |
| <span class="flag-box">✅</span> **Candlestick** <span class="flag-tip" tabindex="0" role="note" aria-label="Output is an integer candlestick-pattern signal (e.g. -100 / 0 / +100)." data-tip="Output is an integer candlestick-pattern signal (e.g. -100 / 0 / +100).">i</span> |

## Implementation

TA-Lib Definition: [`cdleveningdojistar.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/cdleveningdojistar/cdleveningdojistar.c) · [`cdleveningdojistar.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/cdleveningdojistar/cdleveningdojistar.yaml)

| Native | File |
|--------|------|
| C | [`ta_CDLEVENINGDOJISTAR.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_CDLEVENINGDOJISTAR.c) |
| Rust | [`cdleveningdojistar.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/cdleveningdojistar.rs) |
| Java | [`Core_CDLEVENINGDOJISTAR.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/fragments/Core_CDLEVENINGDOJISTAR.java) |

TA-Lib is also available for Python, R and more using a [wrapper](/install/#wrappers).

## Aliases

Evening Doji Star

## See Also

[CDLEVENINGSTAR](/functions/cdleveningstar.md) · [CDLMORNINGDOJISTAR](/functions/cdlmorningdojistar.md) · [CDLDOJISTAR](/functions/cdldojistar.md) · [CDLABANDONEDBABY](/functions/cdlabandonedbaby.md)
