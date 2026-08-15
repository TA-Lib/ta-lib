---
title: "Doji Star (CDLDOJISTAR)"
description: "A two-candle reversal pattern: a long real body followed by a doji whose real body gaps away from it (up after a white body, down after a black body)."
---

## Summary

A two-candle reversal pattern: a long real body followed by a doji whose real body gaps away from it (up after a white body, down after a black body). Signals a potential reversal of the prevailing trend. A hit flags a likely trend reversal; true direction depends on the prevailing trend (bullish in a downtrend, bearish in an uptrend), which the code does not itself verify.

## Formula

Two candles. Candle 1: long real body (realbody > BodyLong average). Candle 2: doji (realbody <= BodyDoji average). Gap: either candle 1 white (color==1) AND candle 2 real body gaps up above it (the real bodies gap up), or candle 1 black (color==-1) AND candle 2 real body gaps down below it (the real bodies gap down).

## Notes

- Does not verify the prior trend the reversal signal classically assumes.
- Bulkowski's testing contradicts the classic reading for the bullish case: theory says a bullish Doji Star (gapping down after a black candle) should be a bullish reversal, but he found it instead acts as a bearish CONTINUATION 64% of the time — almost 2 out of 3, the opposite of the textbook signal. ([thepatternsite.com](https://thepatternsite.com/DojiStarBull.html))

## Inputs

- `inOpen` — Open price of each bar
- `inHigh` — High price of each bar
- `inLow` — Low price of each bar
- `inClose` — Close price of each bar

## Outputs

- `outInteger` — Emits +100 or -100 on a hit, 0 otherwise. Value is -candlecolor(candle1)*100: -100 when candle 1 is white (gap up), +100 when candle 1 is black (gap down)

## Output Values

| Value | Meaning |
|-------|---------|
| -100 | Doji Star gapping up after a strong advance — waning momentum, a bearish warning if it appears in an uptrend |
| 0 | No pattern |
| 100 | Doji Star gapping down after a strong decline — waning momentum, a bullish warning if it appears in a downtrend |

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

TA-Lib Definition: [`cdldojistar.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/cdldojistar/cdldojistar.c) · [`cdldojistar.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/cdldojistar/cdldojistar.yaml)

| Native | File |
|--------|------|
| C | [`ta_CDLDOJISTAR.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_CDLDOJISTAR.c) |
| Rust | [`cdldojistar.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/cdldojistar.rs) |
| Java | [`Core_CDLDOJISTAR.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/fragments/Core_CDLDOJISTAR.java) |

TA-Lib is also available for Python, R and more using a [wrapper](/install/#wrappers).

## Aliases

Doji Star

## See Also

[CDLMORNINGDOJISTAR](/functions/cdlmorningdojistar.md) · [CDLEVENINGDOJISTAR](/functions/cdleveningdojistar.md) · [CDLDOJI](/functions/cdldoji.md) · [CDLMORNINGSTAR](/functions/cdlmorningstar.md) · [CDLEVENINGSTAR](/functions/cdleveningstar.md)
