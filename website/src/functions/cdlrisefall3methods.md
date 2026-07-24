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

| Numerical<br>Stability | Display<br>Flags |
| :-- | :-- |
| <span class="flag-box">✅</span> **Start-Independent** <span class="flag-tip" tabindex="0" role="note" aria-label="The value at a bar does not depend on where your data starts — safe to compare across different-length windows." data-tip="The value at a bar does not depend on where your data starts — safe to compare across different-length windows.">i</span> | <span class="flag-box">☐</span> <span style="opacity:0.5">Overlap Input</span> <span class="flag-tip" tabindex="0" role="note" aria-label="Output is on the same scale as the input price, so it is drawn over the price chart." data-tip="Output is on the same scale as the input price, so it is drawn over the price chart.">i</span> |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Initial Unstable Period</span> <span class="flag-tip" tabindex="0" role="note" aria-label="Early values depend on how much history precedes them but converge as more bars are supplied; tunable via the unstable period." data-tip="Early values depend on how much history precedes them but converge as more bars are supplied; tunable via the unstable period.">i</span> | <span class="flag-box">✅</span> **Independent Y-Axis** <span class="flag-tip" tabindex="0" role="note" aria-label="Output is on its own scale, drawn in a separate pane below the price chart." data-tip="Output is on its own scale, drawn in a separate pane below the price chart.">i</span> |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Path-Dependent</span> <span class="flag-tip" tabindex="0" role="note" aria-label="Built up from the first bar (a running accumulation or path-tracking state machine): the value depends on where your data begins and never converges — don't compare across different windows." data-tip="Built up from the first bar (a running accumulation or path-tracking state machine): the value depends on where your data begins and never converges — don't compare across different windows.">i</span> | <span class="flag-box">✅</span> **Candlestick** <span class="flag-tip" tabindex="0" role="note" aria-label="Output is an integer candlestick-pattern signal (e.g. -100 / 0 / +100)." data-tip="Output is an integer candlestick-pattern signal (e.g. -100 / 0 / +100).">i</span> |

## Implementation

TA-Lib Definition: [`cdlrisefall3methods.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/cdlrisefall3methods/cdlrisefall3methods.c) · [`cdlrisefall3methods.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/cdlrisefall3methods/cdlrisefall3methods.yaml)

| Native | File |
|--------|------|
| C | [`ta_CDLRISEFALL3METHODS.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_CDLRISEFALL3METHODS.c) |
| Rust | [`cdlrisefall3methods.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/cdlrisefall3methods.rs) |
| Java | [`Core_CDLRISEFALL3METHODS.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/library/fragments/Core_CDLRISEFALL3METHODS.java) |

TA-Lib is also available for Python, R and more using a [wrapper](https://ta-lib.org/wrappers/).

## Aliases

Rising/Falling Three Methods, Rising Three Methods, Falling Three Methods

## See Also

[CDLXSIDEGAP3METHODS](/functions/cdlxsidegap3methods) · [CDL3INSIDE](/functions/cdl3inside) · [CDL3OUTSIDE](/functions/cdl3outside)
