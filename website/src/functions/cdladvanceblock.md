---
title: CDLADVANCEBLOCK
description: "Three-candle bearish reversal pattern: three white candles with consecutively higher closes whose advance weakens (progressively smaller bodies and/or lengthening upper shadows). Signals that an uptrend's advance is being blocked. A hit (-100) is bearish: the advance is stalling/blocked; meaningful mainly within an existing uptrend."
---

# CDLADVANCEBLOCK

## Summary

Three-candle bearish reversal pattern: three white candles with consecutively higher closes whose advance weakens (progressively smaller bodies and/or lengthening upper shadows). Signals that an uptrend's advance is being blocked. A hit (-100) is bearish: the advance is stalling/blocked; meaningful mainly within an existing uptrend.

## Notes

- Does not verify the prior uptrend the pattern classically assumes for significance.

## Inputs

- `inOpen` — Open price of each bar
- `inHigh` — High price of each bar
- `inLow` — Low price of each bar
- `inClose` — Close price of each bar

## Outputs

- `outInteger` — -100 on a detected pattern (always bearish), 0 otherwise; never emits +100

## Properties

| Numerical<br>Stability | Display<br>Flags |
| :-- | :-- |
| <span class="flag-box">✅</span> **Start-Independent** <span class="flag-tip" tabindex="0" role="note" aria-label="The value at a bar does not depend on where your data starts — safe to compare across different-length windows." data-tip="The value at a bar does not depend on where your data starts — safe to compare across different-length windows.">i</span> | <span class="flag-box">☐</span> <span style="opacity:0.5">Overlap Input</span> <span class="flag-tip" tabindex="0" role="note" aria-label="Output is on the same scale as the input price, so it is drawn over the price chart." data-tip="Output is on the same scale as the input price, so it is drawn over the price chart.">i</span> |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Initial Unstable Period</span> <span class="flag-tip" tabindex="0" role="note" aria-label="Early values depend on how much history precedes them but converge as more bars are supplied; tunable via the unstable period." data-tip="Early values depend on how much history precedes them but converge as more bars are supplied; tunable via the unstable period.">i</span> | <span class="flag-box">✅</span> **Independent Y-Axis** <span class="flag-tip" tabindex="0" role="note" aria-label="Output is on its own scale, drawn in a separate pane below the price chart." data-tip="Output is on its own scale, drawn in a separate pane below the price chart.">i</span> |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Path-Dependent</span> <span class="flag-tip" tabindex="0" role="note" aria-label="Built up from the first bar (a running accumulation or path-tracking state machine): the value depends on where your data begins and never converges — don't compare across different windows." data-tip="Built up from the first bar (a running accumulation or path-tracking state machine): the value depends on where your data begins and never converges — don't compare across different windows.">i</span> | <span class="flag-box">✅</span> **Candlestick** <span class="flag-tip" tabindex="0" role="note" aria-label="Output is an integer candlestick-pattern signal (e.g. -100 / 0 / +100)." data-tip="Output is an integer candlestick-pattern signal (e.g. -100 / 0 / +100).">i</span> |

## Implementation

TA-Lib Definition: [`cdladvanceblock.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/cdladvanceblock/cdladvanceblock.c) · [`cdladvanceblock.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/cdladvanceblock/cdladvanceblock.yaml)

| Native | File |
|--------|------|
| C | [`ta_CDLADVANCEBLOCK.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_CDLADVANCEBLOCK.c) |
| Rust | [`cdladvanceblock.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/cdladvanceblock.rs) |
| Java | [`Core_CDLADVANCEBLOCK.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/library/fragments/Core_CDLADVANCEBLOCK.java) |

TA-Lib is also available for Python, R and more using a [wrapper](https://ta-lib.org/wrappers/).

## Aliases

Advance Block

## See Also

[CDL3WHITESOLDIERS](/functions/cdl3whitesoldiers) · CDLDELIBERATION · [CDLSTALLEDPATTERN](/functions/cdlstalledpattern)
