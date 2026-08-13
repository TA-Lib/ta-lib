---
title: "Marubozu (CDLMARUBOZU)"
description: "Single candle with a long real body and no/very-short upper and lower shadows, so open and close sit at the range extremes."
---

## Summary

Single candle with a long real body and no/very-short upper and lower shadows, so open and close sit at the range extremes. Bullish (white) or bearish (black) reversal/strength signal per the body color. +100 = white marubozu (strong buying pressure); -100 = black marubozu (strong selling pressure).

## Formula

One candle at i. Match when: realbody(i) > BodyLong average AND upperShadow(i) < ShadowVeryShort average AND lowerShadow(i) < ShadowVeryShort average. If matched emit candlecolor(i)*100 (+100 white when close>=open, -100 black when close<open); else 0.

## Inputs

- `inOpen` — Open price of each bar
- `inHigh` — High price of each bar
- `inLow` — Low price of each bar
- `inClose` — Close price of each bar

## Outputs

- `outInteger` — +100 on a white (bullish) marubozu, -100 on a black (bearish) marubozu, 0 when no pattern. Sign follows the candle color

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

TA-Lib Definition: [`cdlmarubozu.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/cdlmarubozu/cdlmarubozu.c) · [`cdlmarubozu.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/cdlmarubozu/cdlmarubozu.yaml)

| Native | File |
|--------|------|
| C | [`ta_CDLMARUBOZU.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_CDLMARUBOZU.c) |
| Rust | [`cdlmarubozu.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/cdlmarubozu.rs) |
| Java | [`Core_CDLMARUBOZU.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/fragments/Core_CDLMARUBOZU.java) |

TA-Lib is also available for Python, R and more using a [wrapper](/install/#wrappers).

## Aliases

Marubozu, Shaven Head/Bottom

## See Also

[CDLCLOSINGMARUBOZU](/functions/cdlclosingmarubozu.md) · [CDLLONGLINE](/functions/cdllongline.md) · [CDLBELTHOLD](/functions/cdlbelthold.md)
