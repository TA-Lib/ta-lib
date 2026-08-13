---
title: "Abandoned Baby (CDLABANDONEDBABY)"
description: "A three-candle reversal pattern: a long body, then a gapped-away doji, then a body of opposite color that gaps back the other way and closes deep into…"
---

## Summary

A three-candle reversal pattern: a long body, then a gapped-away doji, then a body of opposite color that gaps back the other way and closes deep into the first body. Bullish (bottom) or bearish (top) reversal signal. Nonzero hit signals a reversal: +100 abandoned baby bottom (bullish), -100 abandoned baby top (bearish).

## Notes

- Does not verify the prior trend the pattern classically assumes for significance.

## Inputs

- `inOpen` — Open price of each bar
- `inHigh` — High price of each bar
- `inLow` — Low price of each bar
- `inClose` — Close price of each bar

## Outputs

- `outInteger` — +100 at a bullish abandoned baby bottom (3rd candle white), -100 at a bearish abandoned baby top (3rd candle black), 0 otherwise; sign = color of the 3rd candle

## Parameters

| Parameter | Type | Default | Accepted values | Description |
| --- | --- | --- | --- | --- |
| `optInPenetration` | real | 0.3 | ≥ 0 | Fraction of the 1st candle's real body the 3rd close must penetrate |

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

TA-Lib Definition: [`cdlabandonedbaby.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/cdlabandonedbaby/cdlabandonedbaby.c) · [`cdlabandonedbaby.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/cdlabandonedbaby/cdlabandonedbaby.yaml)

| Native | File |
|--------|------|
| C | [`ta_CDLABANDONEDBABY.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_CDLABANDONEDBABY.c) |
| Rust | [`cdlabandonedbaby.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/cdlabandonedbaby.rs) |
| Java | [`Core_CDLABANDONEDBABY.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/fragments/Core_CDLABANDONEDBABY.java) |

TA-Lib is also available for Python, R and more using a [wrapper](/install/#wrappers).

## Aliases

Abandoned Baby

## See Also

[CDLEVENINGDOJISTAR](/functions/cdleveningdojistar.md) · [CDLMORNINGDOJISTAR](/functions/cdlmorningdojistar.md) · [CDLEVENINGSTAR](/functions/cdleveningstar.md) · [CDLMORNINGSTAR](/functions/cdlmorningstar.md)
