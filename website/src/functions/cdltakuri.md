---
title: CDLTAKURI
description: "Single-candle pattern: a doji whose open and close sit at the high (no/very short upper shadow) with a very long lower shadow, i.e. a dragonfly doji with an exceptionally long lower shadow. Emitted as a positive signal, but its directional meaning depends on the prevailing trend, which the code does not check. A hit marks a takuri (dragonfly-doji) line; a potential reversal only when read against the trend (typically a bottom/bullish reversal after a downtrend), which the code itself does not verify."
---

# CDLTAKURI

## Summary

Single-candle pattern: a doji whose open and close sit at the high (no/very short upper shadow) with a very long lower shadow, i.e. a dragonfly doji with an exceptionally long lower shadow. Emitted as a positive signal, but its directional meaning depends on the prevailing trend, which the code does not check. A hit marks a takuri (dragonfly-doji) line; a potential reversal only when read against the trend (typically a bottom/bullish reversal after a downtrend), which the code itself does not verify.

## Inputs

- `inOpen` — Open price of each bar
- `inHigh` — High price of each bar
- `inLow` — Low price of each bar
- `inClose` — Close price of each bar

## Outputs

- `outInteger` — +100 when the takuri pattern is detected, 0 otherwise. Never negative; the positive sign is a convention and does not by itself imply bullishness

## Properties

| Numerical<br>Stability | Display<br>Flags |
| :-- | :-- |
| <span class="flag-box">✅</span> **Start-Independent** <span class="flag-tip" tabindex="0" role="note" aria-label="The value at a bar does not depend on where your data starts — safe to compare across different-length windows." data-tip="The value at a bar does not depend on where your data starts — safe to compare across different-length windows.">i</span> | <span class="flag-box">☐</span> <span style="opacity:0.5">Overlap Input</span> <span class="flag-tip" tabindex="0" role="note" aria-label="Output is on the same scale as the input price, so it is drawn over the price chart." data-tip="Output is on the same scale as the input price, so it is drawn over the price chart.">i</span> |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Initial Unstable Period</span> <span class="flag-tip" tabindex="0" role="note" aria-label="Early values depend on how much history precedes them but converge as more bars are supplied; tunable via the unstable period." data-tip="Early values depend on how much history precedes them but converge as more bars are supplied; tunable via the unstable period.">i</span> | <span class="flag-box">✅</span> **Independent Y-Axis** <span class="flag-tip" tabindex="0" role="note" aria-label="Output is on its own scale, drawn in a separate pane below the price chart." data-tip="Output is on its own scale, drawn in a separate pane below the price chart.">i</span> |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Path-Dependent</span> <span class="flag-tip" tabindex="0" role="note" aria-label="Built up from the first bar (a running accumulation or path-tracking state machine): the value depends on where your data begins and never converges — don't compare across different windows." data-tip="Built up from the first bar (a running accumulation or path-tracking state machine): the value depends on where your data begins and never converges — don't compare across different windows.">i</span> | <span class="flag-box">✅</span> **Candlestick** <span class="flag-tip" tabindex="0" role="note" aria-label="Output is an integer candlestick-pattern signal (e.g. -100 / 0 / +100)." data-tip="Output is an integer candlestick-pattern signal (e.g. -100 / 0 / +100).">i</span> |

## Implementation

TA-Lib Definition: [`cdltakuri.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/cdltakuri/cdltakuri.c) · [`cdltakuri.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/cdltakuri/cdltakuri.yaml)

| Native | File |
|--------|------|
| C | [`ta_CDLTAKURI.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_CDLTAKURI.c) |
| Rust | [`cdltakuri.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/cdltakuri.rs) |
| Java | [`Core_CDLTAKURI.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/library/fragments/Core_CDLTAKURI.java) |

TA-Lib is also available for Python, R and more using a [wrapper](https://ta-lib.org/wrappers/).

## Aliases

Takuri, Takuri line

## See Also

[CDLDRAGONFLYDOJI](/functions/cdldragonflydoji) · [CDLDOJI](/functions/cdldoji) · [CDLHAMMER](/functions/cdlhammer) · [CDLGRAVESTONEDOJI](/functions/cdlgravestonedoji)
