---
title: CDLRISEFALL3METHODS
description: "A five-candle continuation pattern: a long candle, three small counter-color candles that stay partly within the first candle's high-low range, then a long same-color candle that resumes the trend. Bullish (rising) or bearish (falling) continuation signal. A hit signals trend continuation: +100 = bullish (rising three methods), -100 = bearish (falling three methods)."
---

# CDLRISEFALL3METHODS

## Summary

A five-candle continuation pattern: a long candle, three small counter-color candles that stay partly within the first candle's high-low range, then a long same-color candle that resumes the trend. Bullish (rising) or bearish (falling) continuation signal. A hit signals trend continuation: +100 = bullish (rising three methods), -100 = bearish (falling three methods).

## Notes

- Only the three-small-candle variant is detected; the classic pattern allowing two or more small candles is not supported.
- The middle candles need only partially overlap the first candle's range, not be fully contained within it.
- The prior trend the continuation reading assumes is not verified.

## Inputs

- `inOpen` — Open price of each bar
- `inHigh` — High price of each bar
- `inLow` — Low price of each bar
- `inClose` — Close price of each bar

## Outputs

- `outInteger` — +100 when candle 1 is white (rising/bullish continuation), -100 when candle 1 is black (falling/bearish continuation), 0 otherwise. Sign = 100 * color of candle 1

## Properties

**Numerical Stability:** [Start-Independent](/functions/stability#start-independent)

| Display<br>Flags |
| :-- |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Overlap Input</span> |
| <span class="flag-box">✅</span> **Independent Y-Axis** <span class="flag-tip" tabindex="0" role="note" aria-label="Output is on its own scale, drawn in a separate pane below the price chart." data-tip="Output is on its own scale, drawn in a separate pane below the price chart.">i</span> |
| <span class="flag-box">✅</span> **Candlestick** <span class="flag-tip" tabindex="0" role="note" aria-label="Output is an integer candlestick-pattern signal (e.g. -100 / 0 / +100)." data-tip="Output is an integer candlestick-pattern signal (e.g. -100 / 0 / +100).">i</span> |

## Implementation

TA-Lib Definition: [`cdlrisefall3methods.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/cdlrisefall3methods/cdlrisefall3methods.c) · [`cdlrisefall3methods.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/cdlrisefall3methods/cdlrisefall3methods.yaml)

| Native | File |
|--------|------|
| C | [`ta_CDLRISEFALL3METHODS.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_CDLRISEFALL3METHODS.c) |
| Rust | [`cdlrisefall3methods.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/cdlrisefall3methods.rs) |
| Java | [`Core_CDLRISEFALL3METHODS.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/fragments/Core_CDLRISEFALL3METHODS.java) |

TA-Lib is also available for Python, R and more using a [wrapper](https://ta-lib.org/wrappers/).

## Aliases

Rising/Falling Three Methods, Rising Three Methods, Falling Three Methods

## See Also

[CDLXSIDEGAP3METHODS](/functions/cdlxsidegap3methods) · [CDL3INSIDE](/functions/cdl3inside) · [CDL3OUTSIDE](/functions/cdl3outside)
