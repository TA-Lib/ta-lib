---
title: CDLDOJISTAR
description: "A two-candle reversal pattern: a long real body followed by a doji whose real body gaps away from it (up after a white body, down after a black body). Signals a potential reversal of the prevailing trend. A hit flags a likely trend reversal; true direction depends on the prevailing trend (bullish in a downtrend, bearish in an uptrend), which the code does not itself verify."
---

# CDLDOJISTAR

## Summary

A two-candle reversal pattern: a long real body followed by a doji whose real body gaps away from it (up after a white body, down after a black body). Signals a potential reversal of the prevailing trend. A hit flags a likely trend reversal; true direction depends on the prevailing trend (bullish in a downtrend, bearish in an uptrend), which the code does not itself verify.

## Formula

Two candles. Candle 1: long real body (realbody > BodyLong average). Candle 2: doji (realbody <= BodyDoji average). Gap: either candle 1 white (color==1) AND candle 2 real body gaps up above it (the real bodies gap up), or candle 1 black (color==-1) AND candle 2 real body gaps down below it (the real bodies gap down).

## Notes

- Does not verify the prior trend the reversal signal classically assumes.

## Inputs

- `inOpen` — Open price of each bar
- `inHigh` — High price of each bar
- `inLow` — Low price of each bar
- `inClose` — Close price of each bar

## Outputs

- `outInteger` — Emits +100 or -100 on a hit, 0 otherwise. Value is -candlecolor(candle1)*100: -100 when candle 1 is white (gap up), +100 when candle 1 is black (gap down)

## Properties

| Numerical<br>Stability | Display<br>Flags |
| :-- | :-- |
| <span class="flag-box">✅</span> **Start-Independent** <span class="flag-tip" tabindex="0" role="note" aria-label="The value at a bar does not depend on where your data starts — safe to compare across different-length windows." data-tip="The value at a bar does not depend on where your data starts — safe to compare across different-length windows.">i</span> | <span class="flag-box">☐</span> <span style="opacity:0.5">Overlap Input</span> <span class="flag-tip" tabindex="0" role="note" aria-label="Output is on the same scale as the input price, so it is drawn over the price chart." data-tip="Output is on the same scale as the input price, so it is drawn over the price chart.">i</span> |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Initial Unstable Period</span> <span class="flag-tip" tabindex="0" role="note" aria-label="Early values depend on how much history precedes them but converge as more bars are supplied; tunable via the unstable period." data-tip="Early values depend on how much history precedes them but converge as more bars are supplied; tunable via the unstable period.">i</span> | <span class="flag-box">✅</span> **Independent Y-Axis** <span class="flag-tip" tabindex="0" role="note" aria-label="Output is on its own scale, drawn in a separate pane below the price chart." data-tip="Output is on its own scale, drawn in a separate pane below the price chart.">i</span> |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Path-Dependent</span> <span class="flag-tip" tabindex="0" role="note" aria-label="Built up from the first bar (a running accumulation or path-tracking state machine): the value depends on where your data begins and never converges — don't compare across different windows." data-tip="Built up from the first bar (a running accumulation or path-tracking state machine): the value depends on where your data begins and never converges — don't compare across different windows.">i</span> | <span class="flag-box">✅</span> **Candlestick** <span class="flag-tip" tabindex="0" role="note" aria-label="Output is an integer candlestick-pattern signal (e.g. -100 / 0 / +100)." data-tip="Output is an integer candlestick-pattern signal (e.g. -100 / 0 / +100).">i</span> |

## Implementation

TA-Lib Definition: [`cdldojistar.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/cdldojistar/cdldojistar.c) · [`cdldojistar.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/cdldojistar/cdldojistar.yaml)

| Native | File |
|--------|------|
| C | [`ta_CDLDOJISTAR.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_CDLDOJISTAR.c) |
| Rust | [`cdldojistar.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/cdldojistar.rs) |
| Java | [`Core_CDLDOJISTAR.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/library/fragments/Core_CDLDOJISTAR.java) |

TA-Lib is also available for Python, R and more using a [wrapper](https://ta-lib.org/wrappers/).

## Aliases

Doji Star

## See Also

[CDLMORNINGDOJISTAR](/functions/cdlmorningdojistar) · [CDLEVENINGDOJISTAR](/functions/cdleveningdojistar) · [CDLDOJI](/functions/cdldoji) · [CDLMORNINGSTAR](/functions/cdlmorningstar) · [CDLEVENINGSTAR](/functions/cdleveningstar)
