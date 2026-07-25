---
title: CDLPIERCING
description: "Two-candle pattern: a long black candle followed by a long white candle that opens below the prior low and closes back above the midpoint of the prior black body. Bullish reversal signal. A hit (+100) is a bullish reversal signal."
---

# CDLPIERCING

## Summary

Two-candle pattern: a long black candle followed by a long white candle that opens below the prior low and closes back above the midpoint of the prior black body. Bullish reversal signal. A hit (+100) is a bullish reversal signal.

## Notes

- A prior downtrend is not verified.

## Inputs

- `inOpen` — Open price of each bar
- `inHigh` — High price of each bar
- `inLow` — Low price of each bar
- `inClose` — Close price of each bar

## Outputs

- `outInteger` — +100 when the piercing pattern is detected; 0 otherwise. Always bullish, never emits -100

## Properties

**Numerical Stability:** [Start-Independent](/functions/stability#start-independent)

| Display<br>Flags |
| :-- |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Overlap Input</span> |
| <span class="flag-box">✅</span> **Independent Y-Axis** <span class="flag-tip" tabindex="0" role="note" aria-label="Output is on its own scale, drawn in a separate pane below the price chart." data-tip="Output is on its own scale, drawn in a separate pane below the price chart.">i</span> |
| <span class="flag-box">✅</span> **Candlestick** <span class="flag-tip" tabindex="0" role="note" aria-label="Output is an integer candlestick-pattern signal (e.g. -100 / 0 / +100)." data-tip="Output is an integer candlestick-pattern signal (e.g. -100 / 0 / +100).">i</span> |

## Implementation

TA-Lib Definition: [`cdlpiercing.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/cdlpiercing/cdlpiercing.c) · [`cdlpiercing.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/cdlpiercing/cdlpiercing.yaml)

| Native | File |
|--------|------|
| C | [`ta_CDLPIERCING.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_CDLPIERCING.c) |
| Rust | [`cdlpiercing.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/cdlpiercing.rs) |
| Java | [`Core_CDLPIERCING.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/library/fragments/Core_CDLPIERCING.java) |

TA-Lib is also available for Python, R and more using a [wrapper](https://ta-lib.org/wrappers/).

## Aliases

Piercing Pattern, Piercing Line

## See Also

[CDLDARKCLOUDCOVER](/functions/cdldarkcloudcover) · [CDLENGULFING](/functions/cdlengulfing) · [CDLMORNINGSTAR](/functions/cdlmorningstar)
