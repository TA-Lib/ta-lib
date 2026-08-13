---
title: "Dark Cloud Cover (CDLDARKCLOUDCOVER)"
description: "A two-candle bearish reversal pattern: a long white candle followed by a black candle that opens above the prior high and closes deep into the prior…"
---

## Summary

A two-candle bearish reversal pattern: a long white candle followed by a black candle that opens above the prior high and closes deep into the prior white body past a penetration threshold. Signals a potential top. A hit (-100) is a bearish reversal signal, most meaningful after an uptrend.

## Notes

- Does not verify the preceding uptrend the bearish reversal classically assumes.

## Inputs

- `inOpen` — Open price of each bar
- `inHigh` — High price of each bar
- `inLow` — Low price of each bar
- `inClose` — Close price of each bar

## Outputs

- `outInteger` — -100 when the pattern is detected (always bearish), 0 otherwise; never emits +100

## Parameters

| Parameter | Type | Default | Accepted values | Description |
| --- | --- | --- | --- | --- |
| `optInPenetration` | real | 0.5 | ≥ 0 | Fraction of candle 1's real body that candle 2's close must penetrate below close[i-1]; larger values require deeper penetration |

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

TA-Lib Definition: [`cdldarkcloudcover.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/cdldarkcloudcover/cdldarkcloudcover.c) · [`cdldarkcloudcover.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/cdldarkcloudcover/cdldarkcloudcover.yaml)

| Native | File |
|--------|------|
| C | [`ta_CDLDARKCLOUDCOVER.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_CDLDARKCLOUDCOVER.c) |
| Rust | [`cdldarkcloudcover.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/cdldarkcloudcover.rs) |
| Java | [`Core_CDLDARKCLOUDCOVER.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/fragments/Core_CDLDARKCLOUDCOVER.java) |

TA-Lib is also available for Python, R and more using a [wrapper](/install/#wrappers).

## Aliases

Dark Cloud Cover

## See Also

[CDLPIERCING](/functions/cdlpiercing.md) · [CDLENGULFING](/functions/cdlengulfing.md) · [CDLONNECK](/functions/cdlonneck.md)
