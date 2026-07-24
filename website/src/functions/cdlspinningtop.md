---
title: CDLSPINNINGTOP
description: "Single-candle pattern: a small real body with both an upper and a lower shadow longer than the body. Signals indecision; the code does not classify it as bullish or bearish. A hit marks indecision (small body, both shadows long); the sign only reports candle color, not direction."
---

# CDLSPINNINGTOP

## Summary

Single-candle pattern: a small real body with both an upper and a lower shadow longer than the body. Signals indecision; the code does not classify it as bullish or bearish. A hit marks indecision (small body, both shadows long); the sign only reports candle color, not direction.

## Formula

One candle where: upper shadow > real body AND lower shadow > real body AND real body < the BodyShort average. The BodyShort average is the factor-scaled mean body over the prior avgPeriod candles.

## Inputs

- `inOpen` — Open price of each bar
- `inHigh` — High price of each bar
- `inLow` — Low price of each bar
- `inClose` — Close price of each bar

## Outputs

- `outInteger` — +100 when the candle is white (close>=open), -100 when black (close<open), 0 when no pattern. Sign is candle color, NOT bullish/bearish

## Properties

| Numerical<br>Stability | Display<br>Flags |
| :-- | :-- |
| <span class="flag-box">✅</span> **Start-Independent** <span class="flag-tip" tabindex="0" role="note" aria-label="The value at a bar does not depend on where your data starts — safe to compare across different-length windows." data-tip="The value at a bar does not depend on where your data starts — safe to compare across different-length windows.">i</span> | <span class="flag-box">☐</span> <span style="opacity:0.5">Overlap Input</span> <span class="flag-tip" tabindex="0" role="note" aria-label="Output is on the same scale as the input price, so it is drawn over the price chart." data-tip="Output is on the same scale as the input price, so it is drawn over the price chart.">i</span> |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Initial Unstable Period</span> <span class="flag-tip" tabindex="0" role="note" aria-label="Early values depend on how much history precedes them but converge as more bars are supplied; tunable via the unstable period." data-tip="Early values depend on how much history precedes them but converge as more bars are supplied; tunable via the unstable period.">i</span> | <span class="flag-box">✅</span> **Independent Y-Axis** <span class="flag-tip" tabindex="0" role="note" aria-label="Output is on its own scale, drawn in a separate pane below the price chart." data-tip="Output is on its own scale, drawn in a separate pane below the price chart.">i</span> |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Path-Dependent</span> <span class="flag-tip" tabindex="0" role="note" aria-label="Built up from the first bar (a running accumulation or path-tracking state machine): the value depends on where your data begins and never converges — don't compare across different windows." data-tip="Built up from the first bar (a running accumulation or path-tracking state machine): the value depends on where your data begins and never converges — don't compare across different windows.">i</span> | <span class="flag-box">✅</span> **Candlestick** <span class="flag-tip" tabindex="0" role="note" aria-label="Output is an integer candlestick-pattern signal (e.g. -100 / 0 / +100)." data-tip="Output is an integer candlestick-pattern signal (e.g. -100 / 0 / +100).">i</span> |

## Implementation

TA-Lib Definition: [`cdlspinningtop.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/cdlspinningtop/cdlspinningtop.c) · [`cdlspinningtop.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/cdlspinningtop/cdlspinningtop.yaml)

| Native | File |
|--------|------|
| C | [`ta_CDLSPINNINGTOP.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_CDLSPINNINGTOP.c) |
| Rust | [`cdlspinningtop.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/cdlspinningtop.rs) |
| Java | [`Core_CDLSPINNINGTOP.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/library/fragments/Core_CDLSPINNINGTOP.java) |

TA-Lib is also available for Python, R and more using a [wrapper](https://ta-lib.org/wrappers/).

## Aliases

Spinning Top

## See Also

[CDLDOJI](/functions/cdldoji) · [CDLHIGHWAVE](/functions/cdlhighwave) · [CDLLONGLEGGEDDOJI](/functions/cdllongleggeddoji)
