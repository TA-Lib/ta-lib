---
title: "Matching Low (CDLMATCHINGLOW)"
description: "A two-candle pattern of two consecutive black (bearish) candles with equal closes (within a tolerance). Treated as a bullish reversal signal."
---

## Summary

A two-candle pattern of two consecutive black (bearish) candles with equal closes (within a tolerance). Treated as a bullish reversal signal. A hit signals a potential bullish reversal (shared support close after two down candles).

## Formula

Two candles i-1, i. Candle i-1: black (close<open). Candle i: black (close<open). Equal closes: close[i-1]-E <= close[i] <= close[i-1]+E, where E = the Equal average. No shadow, body-size, or gap conditions are checked.

## Notes

- The bullish-reversal reading assumes a prior downtrend, which is not verified.
- Although classically read as a bullish reversal (and TA-Lib only emits +100), Bulkowski's testing found it actually acts as a bearish continuation 61% of the time — even so, it still ranks a strong 8th of 103 patterns for overall performance. ([thepatternsite.com](https://thepatternsite.com/MatchingLow.html))

## Inputs

- `inOpen` — Open price of each bar
- `inHigh` — High price of each bar
- `inLow` — Low price of each bar
- `inClose` — Close price of each bar

## Outputs

- `outInteger` — +100 when the pattern is present, 0 otherwise. Only +100 is ever emitted (matching low is always bullish); never -100

## Output Values

| Value | Meaning |
|-------|---------|
| 0 | No pattern |
| 100 | Matching Low detected — bullish reversal signal (shared support close after two black candles) |

## Properties

**Numerical Stability:** [Start-Independent](/functions/stability.md#start-independent)

<div class="flag-table">

|  |
| :-- |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Overlap Input</span> |
| <span class="flag-box">✅</span> **Independent Y-Axis** <span class="flag-tip" tabindex="0" role="note" aria-label="Output is on its own scale, drawn in a separate pane below the price chart." data-tip="Output is on its own scale, drawn in a separate pane below the price chart.">i</span> |
| <span class="flag-box">✅</span> **Candlestick** <span class="flag-tip" tabindex="0" role="note" aria-label="Output is an integer candlestick-pattern signal (e.g. -100 / 0 / +100)." data-tip="Output is an integer candlestick-pattern signal (e.g. -100 / 0 / +100).">i</span> |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Can Output NaN or ±Inf</span> |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Identity at Period 1</span> |

</div>

## Implementation

TA-Lib Definition: [`cdlmatchinglow.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/cdlmatchinglow/cdlmatchinglow.c) · [`cdlmatchinglow.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/cdlmatchinglow/cdlmatchinglow.yaml)

| Native | File |
|--------|------|
| C | [`ta_CDLMATCHINGLOW.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_CDLMATCHINGLOW.c) |
| Rust | [`cdlmatchinglow.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/cdlmatchinglow.rs) |
| Java | [`Core_CDLMATCHINGLOW.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/fragments/Core_CDLMATCHINGLOW.java) |

TA-Lib is also available for Python, R and more using a [wrapper](/install/#wrappers).

## Aliases

Matching Low

## See Also

CDLMATCHINGHIGH · [CDLHOMINGPIGEON](/functions/cdlhomingpigeon.md)
