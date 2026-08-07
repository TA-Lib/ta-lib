---
title: "Separating Lines (CDLSEPARATINGLINES)"
description: "A two-candle continuation pattern: the second candle opposes the first in color, opens at the same price as the first, and is a long-bodied belt hold."
---

## Summary

A two-candle continuation pattern: the second candle opposes the first in color, opens at the same price as the first, and is a long-bodied belt hold. Bullish (white second candle) or bearish (black second candle) continuation signal. Trend continuation: +100 = bullish (white belt hold), -100 = bearish (black belt hold).

## Formula

Two consecutive candles i-1, i: (1) opposite colors: color(i-1) == -color(i); (2) same open: open[i-1]-Equal_avg <= open[i] <= open[i-1]+Equal_avg; (3) long body: realbody(i) > BodyLong_avg; (4) belt hold: if i is white, lowershadow(i) < ShadowVeryShort_avg; if i is black, uppershadow(i) < ShadowVeryShort_avg.

## Notes

- A prior trend is not verified, nor that the pattern aligns with it.

## Inputs

- `inOpen` — Open price of each bar
- `inHigh` — High price of each bar
- `inLow` — Low price of each bar
- `inClose` — Close price of each bar

## Outputs

- `outInteger` — +100 for a bullish (white second candle) hit, -100 for a bearish (black second candle) hit, 0 otherwise

## Properties

**Numerical Stability:** [Start-Independent](/functions/stability.md#start-independent)

| Display<br>Flags |
| :-- |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Overlap Input</span> |
| <span class="flag-box">✅</span> **Independent Y-Axis** <span class="flag-tip" tabindex="0" role="note" aria-label="Output is on its own scale, drawn in a separate pane below the price chart." data-tip="Output is on its own scale, drawn in a separate pane below the price chart.">i</span> |
| <span class="flag-box">✅</span> **Candlestick** <span class="flag-tip" tabindex="0" role="note" aria-label="Output is an integer candlestick-pattern signal (e.g. -100 / 0 / +100)." data-tip="Output is an integer candlestick-pattern signal (e.g. -100 / 0 / +100).">i</span> |

## Implementation

TA-Lib Definition: [`cdlseparatinglines.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/cdlseparatinglines/cdlseparatinglines.c) · [`cdlseparatinglines.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/cdlseparatinglines/cdlseparatinglines.yaml)

| Native | File |
|--------|------|
| C | [`ta_CDLSEPARATINGLINES.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_CDLSEPARATINGLINES.c) |
| Rust | [`cdlseparatinglines.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/cdlseparatinglines.rs) |
| Java | [`Core_CDLSEPARATINGLINES.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/fragments/Core_CDLSEPARATINGLINES.java) |

TA-Lib is also available for Python, R and more using a [wrapper](/install/#wrappers).

## Aliases

Separating Lines

## See Also

[CDLBELTHOLD](/functions/cdlbelthold.md) · CDLMEETINGLINES
