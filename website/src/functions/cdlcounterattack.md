---
title: CDLCOUNTERATTACK
description: "A two-candle pattern of two long, opposite-colored real bodies whose closing prices are (nearly) equal. Emits a bullish signal when the second candle is white and a bearish signal when it is black (a reversal signal, though its trend context is not checked). A hit signals a reversal: +100 (white 2nd candle) bullish, -100 (black 2nd candle) bearish; significance depends on a prior trend the code does not check."
---

# CDLCOUNTERATTACK

## Summary

A two-candle pattern of two long, opposite-colored real bodies whose closing prices are (nearly) equal. Emits a bullish signal when the second candle is white and a bearish signal when it is black (a reversal signal, though its trend context is not checked). A hit signals a reversal: +100 (white 2nd candle) bullish, -100 (black 2nd candle) bearish; significance depends on a prior trend the code does not check.

## Notes

- Does not verify the prior trend the reversal signal classically assumes.

## Inputs

- `inOpen` — Open price of each bar
- `inHigh` — High price of each bar
- `inLow` — Low price of each bar
- `inClose` — Close price of each bar

## Outputs

- `outInteger` — +100 when the second candle is white (bullish), -100 when it is black (bearish), 0 when no pattern

## Properties

| Numerical<br>Stability | Display<br>Flags |
| :-- | :-- |
| <span class="flag-box">✅</span> **Start-Independent** <span class="flag-tip" tabindex="0" role="note" aria-label="The value at a bar does not depend on where your data starts — safe to compare across different-length windows." data-tip="The value at a bar does not depend on where your data starts — safe to compare across different-length windows.">i</span> | <span class="flag-box">☐</span> <span style="opacity:0.5">Overlap Input</span> <span class="flag-tip" tabindex="0" role="note" aria-label="Output is on the same scale as the input price, so it is drawn over the price chart." data-tip="Output is on the same scale as the input price, so it is drawn over the price chart.">i</span> |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Initial Unstable Period</span> <span class="flag-tip" tabindex="0" role="note" aria-label="Early values depend on how much history precedes them but converge as more bars are supplied; tunable via the unstable period." data-tip="Early values depend on how much history precedes them but converge as more bars are supplied; tunable via the unstable period.">i</span> | <span class="flag-box">✅</span> **Independent Y-Axis** <span class="flag-tip" tabindex="0" role="note" aria-label="Output is on its own scale, drawn in a separate pane below the price chart." data-tip="Output is on its own scale, drawn in a separate pane below the price chart.">i</span> |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Path-Dependent</span> <span class="flag-tip" tabindex="0" role="note" aria-label="Built up from the first bar (a running accumulation or path-tracking state machine): the value depends on where your data begins and never converges — don't compare across different windows." data-tip="Built up from the first bar (a running accumulation or path-tracking state machine): the value depends on where your data begins and never converges — don't compare across different windows.">i</span> | <span class="flag-box">✅</span> **Candlestick** <span class="flag-tip" tabindex="0" role="note" aria-label="Output is an integer candlestick-pattern signal (e.g. -100 / 0 / +100)." data-tip="Output is an integer candlestick-pattern signal (e.g. -100 / 0 / +100).">i</span> |

## Implementation

TA-Lib Definition: [`cdlcounterattack.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/cdlcounterattack/cdlcounterattack.c) · [`cdlcounterattack.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/cdlcounterattack/cdlcounterattack.yaml)

| Native | File |
|--------|------|
| C | [`ta_CDLCOUNTERATTACK.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_CDLCOUNTERATTACK.c) |
| Rust | [`cdlcounterattack.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/cdlcounterattack.rs) |
| Java | [`Core_CDLCOUNTERATTACK.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/library/fragments/Core_CDLCOUNTERATTACK.java) |

TA-Lib is also available for Python, R and more using a [wrapper](https://ta-lib.org/wrappers/).

## Aliases

Counterattack, Counterattack Lines, Meeting Lines

## See Also

[CDLPIERCING](/functions/cdlpiercing) · [CDLDARKCLOUDCOVER](/functions/cdldarkcloudcover) · [CDLGAPSIDESIDEWHITE](/functions/cdlgapsidesidewhite)
