---
title: CDLRICKSHAWMAN
description: "Single-candle doji with two long shadows whose body sits near the midpoint of the high-low range. It is a neutral indecision signal, not a directional (bullish/bearish) reversal. A hit marks market indecision/uncertainty; neutral, neither bullish nor bearish."
---

# CDLRICKSHAWMAN

## Summary

Single-candle doji with two long shadows whose body sits near the midpoint of the high-low range. It is a neutral indecision signal, not a directional (bullish/bearish) reversal. A hit marks market indecision/uncertainty; neutral, neither bullish nor bearish.

## Inputs

- `inOpen` — Open price of each bar
- `inHigh` — High price of each bar
- `inLow` — Low price of each bar
- `inClose` — Close price of each bar

## Outputs

- `outInteger` — +100 when the pattern is present, 0 otherwise. Never -100; the code notes the positive value does NOT imply bullish, it signals uncertainty

## Properties

| Numerical<br>Stability | Display<br>Flags |
| :-- | :-- |
| <span class="flag-box">✅</span> **Start-Independent** <span class="flag-tip" tabindex="0" role="note" aria-label="The value at a bar does not depend on where your data starts — safe to compare across different-length windows." data-tip="The value at a bar does not depend on where your data starts — safe to compare across different-length windows.">i</span> | <span class="flag-box">☐</span> <span style="opacity:0.5">Overlap Input</span> <span class="flag-tip" tabindex="0" role="note" aria-label="Output is on the same scale as the input price, so it is drawn over the price chart." data-tip="Output is on the same scale as the input price, so it is drawn over the price chart.">i</span> |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Initial Unstable Period</span> <span class="flag-tip" tabindex="0" role="note" aria-label="Early values depend on how much history precedes them but converge as more bars are supplied; tunable via the unstable period." data-tip="Early values depend on how much history precedes them but converge as more bars are supplied; tunable via the unstable period.">i</span> | <span class="flag-box">✅</span> **Independent Y-Axis** <span class="flag-tip" tabindex="0" role="note" aria-label="Output is on its own scale, drawn in a separate pane below the price chart." data-tip="Output is on its own scale, drawn in a separate pane below the price chart.">i</span> |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Path-Dependent</span> <span class="flag-tip" tabindex="0" role="note" aria-label="Built up from the first bar (a running accumulation or path-tracking state machine): the value depends on where your data begins and never converges — don't compare across different windows." data-tip="Built up from the first bar (a running accumulation or path-tracking state machine): the value depends on where your data begins and never converges — don't compare across different windows.">i</span> | <span class="flag-box">✅</span> **Candlestick** <span class="flag-tip" tabindex="0" role="note" aria-label="Output is an integer candlestick-pattern signal (e.g. -100 / 0 / +100)." data-tip="Output is an integer candlestick-pattern signal (e.g. -100 / 0 / +100).">i</span> |

## Implementation

TA-Lib Definition: [`cdlrickshawman.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/cdlrickshawman/cdlrickshawman.c) · [`cdlrickshawman.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/cdlrickshawman/cdlrickshawman.yaml)

| Native | File |
|--------|------|
| C | [`ta_CDLRICKSHAWMAN.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_CDLRICKSHAWMAN.c) |
| Rust | [`cdlrickshawman.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/cdlrickshawman.rs) |
| Java | [`Core_CDLRICKSHAWMAN.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/library/fragments/Core_CDLRICKSHAWMAN.java) |

TA-Lib is also available for Python, R and more using a [wrapper](https://ta-lib.org/wrappers/).

## Aliases

Rickshaw Man

## See Also

[CDLLONGLEGGEDDOJI](/functions/cdllongleggeddoji) · [CDLDOJI](/functions/cdldoji) · [CDLHIGHWAVE](/functions/cdlhighwave)
