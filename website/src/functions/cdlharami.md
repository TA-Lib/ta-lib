---
title: "Harami Pattern (CDLHARAMI)"
description: "Two-candle pattern: a long real body followed by a short real body contained within the first candle's real body."
---

## Summary

Two-candle pattern: a long real body followed by a short real body contained within the first candle's real body. A reversal signal whose direction is the opposite of the first candle's color. A hit signals a potential reversal opposite the 1st candle: positive (black 1st) is bullish, negative (white 1st) is bearish.

## Notes

- Does not verify the prior trend (downtrend for bullish, uptrend for bearish) that the reversal signal assumes.

## Inputs

- `inOpen` — Open price of each bar
- `inHigh` — High price of each bar
- `inLow` — Low price of each bar
- `inClose` — Close price of each bar

## Outputs

- `outInteger` — +100/+80 when the long 1st candle is black (bullish), -100/-80 when it is white (bearish), 0 otherwise; 80 when the two real bodies share an end, 100 when the 1st body strictly overhangs both ends of the 2nd

## Properties

**Numerical Stability:** [Start-Independent](/functions/stability.md#start-independent)

| Display<br>Flags |
| :-- |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Overlap Input</span> |
| <span class="flag-box">✅</span> **Independent Y-Axis** <span class="flag-tip" tabindex="0" role="note" aria-label="Output is on its own scale, drawn in a separate pane below the price chart." data-tip="Output is on its own scale, drawn in a separate pane below the price chart.">i</span> |
| <span class="flag-box">✅</span> **Candlestick** <span class="flag-tip" tabindex="0" role="note" aria-label="Output is an integer candlestick-pattern signal (e.g. -100 / 0 / +100)." data-tip="Output is an integer candlestick-pattern signal (e.g. -100 / 0 / +100).">i</span> |

## Implementation

TA-Lib Definition: [`cdlharami.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/cdlharami/cdlharami.c) · [`cdlharami.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/cdlharami/cdlharami.yaml)

| Native | File |
|--------|------|
| C | [`ta_CDLHARAMI.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_CDLHARAMI.c) |
| Rust | [`cdlharami.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/cdlharami.rs) |
| Java | [`Core_CDLHARAMI.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/fragments/Core_CDLHARAMI.java) |

TA-Lib is also available for Python, R and more using a [wrapper](/install/#wrappers).

## Aliases

Harami, Harami Pattern

## See Also

[CDLHARAMICROSS](/functions/cdlharamicross.md) · [CDLENGULFING](/functions/cdlengulfing.md)
