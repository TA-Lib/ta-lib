---
title: CDLSTALLEDPATTERN
description: "A three-candle pattern of three white candles with consecutively higher closes where the third loses momentum (a small body riding on the shoulder of the second's long body). It is a bearish reversal signal of a stalling advance. A hit (-100) is bearish: the uptrend is stalling and may reverse."
---

# CDLSTALLEDPATTERN

## Summary

A three-candle pattern of three white candles with consecutively higher closes where the third loses momentum (a small body riding on the shoulder of the second's long body). It is a bearish reversal signal of a stalling advance. A hit (-100) is bearish: the uptrend is stalling and may reverse.

## Notes

- The pattern classically appears in an uptrend, but this function does not verify a prior uptrend; the caller must confirm it.

## Inputs

- `inOpen` — Open price of each bar
- `inHigh` — High price of each bar
- `inLow` — Low price of each bar
- `inClose` — Close price of each bar

## Outputs

- `outInteger` — -100 when the pattern is detected (always bearish), 0 otherwise. Never emits +100

## Properties

| Numerical<br>Stability | Display<br>Flags |
| :-- | :-- |
| <span class="flag-box">✅</span> **Start-Independent** <span class="flag-tip" tabindex="0" role="note" aria-label="The value at a bar does not depend on where your data starts — safe to compare across different-length windows." data-tip="The value at a bar does not depend on where your data starts — safe to compare across different-length windows.">i</span> | <span class="flag-box">☐</span> <span style="opacity:0.5">Overlap Input</span> <span class="flag-tip" tabindex="0" role="note" aria-label="Output is on the same scale as the input price, so it is drawn over the price chart." data-tip="Output is on the same scale as the input price, so it is drawn over the price chart.">i</span> |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Initial Unstable Period</span> <span class="flag-tip" tabindex="0" role="note" aria-label="Early values depend on how much history precedes them but converge as more bars are supplied; tunable via the unstable period." data-tip="Early values depend on how much history precedes them but converge as more bars are supplied; tunable via the unstable period.">i</span> | <span class="flag-box">✅</span> **Independent Y-Axis** <span class="flag-tip" tabindex="0" role="note" aria-label="Output is on its own scale, drawn in a separate pane below the price chart." data-tip="Output is on its own scale, drawn in a separate pane below the price chart.">i</span> |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Path-Dependent</span> <span class="flag-tip" tabindex="0" role="note" aria-label="Built up from the first bar (a running accumulation or path-tracking state machine): the value depends on where your data begins and never converges — don't compare across different windows." data-tip="Built up from the first bar (a running accumulation or path-tracking state machine): the value depends on where your data begins and never converges — don't compare across different windows.">i</span> | <span class="flag-box">✅</span> **Candlestick** <span class="flag-tip" tabindex="0" role="note" aria-label="Output is an integer candlestick-pattern signal (e.g. -100 / 0 / +100)." data-tip="Output is an integer candlestick-pattern signal (e.g. -100 / 0 / +100).">i</span> |

## Implementation

TA-Lib Definition: [`cdlstalledpattern.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/cdlstalledpattern/cdlstalledpattern.c) · [`cdlstalledpattern.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/cdlstalledpattern/cdlstalledpattern.yaml)

| Native | File |
|--------|------|
| C | [`ta_CDLSTALLEDPATTERN.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_CDLSTALLEDPATTERN.c) |
| Rust | [`cdlstalledpattern.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/cdlstalledpattern.rs) |
| Java | [`Core_CDLSTALLEDPATTERN.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/library/fragments/Core_CDLSTALLEDPATTERN.java) |

TA-Lib is also available for Python, R and more using a [wrapper](https://ta-lib.org/wrappers/).

## Aliases

Stalled Pattern, Deliberation Pattern

## See Also

[CDLADVANCEBLOCK](/functions/cdladvanceblock) · [CDL3WHITESOLDIERS](/functions/cdl3whitesoldiers) · [CDLXSIDEGAP3METHODS](/functions/cdlxsidegap3methods)
