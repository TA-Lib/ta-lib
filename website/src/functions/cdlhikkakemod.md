---
title: CDLHIKKAKEMOD
description: "A four-candle pattern: two successively narrower inside bars, then a breakout bar, with the second candle closing near one extreme of its range. Bullish or bearish reversal signal. Bullish (+) or bearish (-) reversal; per the code's note it is significant in a downtrend (bull) or uptrend (bear), context the code does not verify."
---

# CDLHIKKAKEMOD

## Summary

A four-candle pattern: two successively narrower inside bars, then a breakout bar, with the second candle closing near one extreme of its range. Bullish or bearish reversal signal. Bullish (+) or bearish (-) reversal; per the code's note it is significant in a downtrend (bull) or uptrend (bear), context the code does not verify.

## Notes

- Does not verify the prior trend (downtrend for bullish, uptrend for bearish) that this reversal pattern assumes.

## Inputs

- `inOpen` — Open price of each bar
- `inHigh` — High price of each bar
- `inLow` — Low price of each bar
- `inClose` — Close price of each bar

## Outputs

- `outInteger` — +100 bullish hikkake bar, -100 bearish; +200 confirmed bullish, -200 confirmed bearish (confirmation adds another +/-100); 0 otherwise

## Properties

| Numerical<br>Stability | Display<br>Flags |
| :-- | :-- |
| <span class="flag-box">✅</span> **Start-Independent** <span class="flag-tip" tabindex="0" role="note" aria-label="The value at a bar does not depend on where your data starts — safe to compare across different-length windows." data-tip="The value at a bar does not depend on where your data starts — safe to compare across different-length windows.">i</span> | <span class="flag-box">☐</span> <span style="opacity:0.5">Overlap Input</span> <span class="flag-tip" tabindex="0" role="note" aria-label="Output is on the same scale as the input price, so it is drawn over the price chart." data-tip="Output is on the same scale as the input price, so it is drawn over the price chart.">i</span> |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Initial Unstable Period</span> <span class="flag-tip" tabindex="0" role="note" aria-label="Early values depend on how much history precedes them but converge as more bars are supplied; tunable via the unstable period." data-tip="Early values depend on how much history precedes them but converge as more bars are supplied; tunable via the unstable period.">i</span> | <span class="flag-box">✅</span> **Independent Y-Axis** <span class="flag-tip" tabindex="0" role="note" aria-label="Output is on its own scale, drawn in a separate pane below the price chart." data-tip="Output is on its own scale, drawn in a separate pane below the price chart.">i</span> |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Path-Dependent</span> <span class="flag-tip" tabindex="0" role="note" aria-label="Built up from the first bar (a running accumulation or path-tracking state machine): the value depends on where your data begins and never converges — don't compare across different windows." data-tip="Built up from the first bar (a running accumulation or path-tracking state machine): the value depends on where your data begins and never converges — don't compare across different windows.">i</span> | <span class="flag-box">✅</span> **Candlestick** <span class="flag-tip" tabindex="0" role="note" aria-label="Output is an integer candlestick-pattern signal (e.g. -100 / 0 / +100)." data-tip="Output is an integer candlestick-pattern signal (e.g. -100 / 0 / +100).">i</span> |

## Implementation

TA-Lib Definition: [`cdlhikkakemod.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/cdlhikkakemod/cdlhikkakemod.c) · [`cdlhikkakemod.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/cdlhikkakemod/cdlhikkakemod.yaml)

| Native | File |
|--------|------|
| C | [`ta_CDLHIKKAKEMOD.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_CDLHIKKAKEMOD.c) |
| Rust | [`cdlhikkakemod.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/cdlhikkakemod.rs) |
| Java | [`Core_CDLHIKKAKEMOD.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/library/fragments/Core_CDLHIKKAKEMOD.java) |

TA-Lib is also available for Python, R and more using a [wrapper](https://ta-lib.org/wrappers/).

## Aliases

Modified Hikkake, Modified Hikkake Pattern

## See Also

[CDLHIKKAKE](/functions/cdlhikkake)
