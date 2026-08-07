---
title: "Upside Gap Two Crows (CDLUPSIDEGAP2CROWS)"
description: "A three-candle bearish reversal pattern: a long white candle, then a small black candle gapping up (a gap between the real bodies), then a black candle…"
---

## Summary

A three-candle bearish reversal pattern: a long white candle, then a small black candle gapping up (a gap between the real bodies), then a black candle that engulfs the second candle's real body but still closes above the first candle's close. Signals a bearish reversal. A hit (-100) is a bearish reversal signal, most meaningful in an uptrend.

## Notes

- The pattern classically assumes a prior uptrend, but this function does not verify any trend.

## Inputs

- `inOpen` — Open price of each bar
- `inHigh` — High price of each bar
- `inLow` — Low price of each bar
- `inClose` — Close price of each bar

## Outputs

- `outInteger` — -100 on a pattern bar, 0 otherwise. Bearish-only: this pattern never emits +100

## Properties

**Numerical Stability:** [Start-Independent](/functions/stability.md#start-independent)

| Display<br>Flags |
| :-- |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Overlap Input</span> |
| <span class="flag-box">✅</span> **Independent Y-Axis** <span class="flag-tip" tabindex="0" role="note" aria-label="Output is on its own scale, drawn in a separate pane below the price chart." data-tip="Output is on its own scale, drawn in a separate pane below the price chart.">i</span> |
| <span class="flag-box">✅</span> **Candlestick** <span class="flag-tip" tabindex="0" role="note" aria-label="Output is an integer candlestick-pattern signal (e.g. -100 / 0 / +100)." data-tip="Output is an integer candlestick-pattern signal (e.g. -100 / 0 / +100).">i</span> |

## Implementation

TA-Lib Definition: [`cdlupsidegap2crows.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/cdlupsidegap2crows/cdlupsidegap2crows.c) · [`cdlupsidegap2crows.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/cdlupsidegap2crows/cdlupsidegap2crows.yaml)

| Native | File |
|--------|------|
| C | [`ta_CDLUPSIDEGAP2CROWS.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_CDLUPSIDEGAP2CROWS.c) |
| Rust | [`cdlupsidegap2crows.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/cdlupsidegap2crows.rs) |
| Java | [`Core_CDLUPSIDEGAP2CROWS.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/fragments/Core_CDLUPSIDEGAP2CROWS.java) |

TA-Lib is also available for Python, R and more using a [wrapper](/install/#wrappers).

## Aliases

Upside Gap Two Crows

## See Also

[CDL2CROWS](/functions/cdl2crows.md) · [CDLGAPSIDESIDEWHITE](/functions/cdlgapsidesidewhite.md)
