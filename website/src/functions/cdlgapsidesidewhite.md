---
title: "Up/Down-gap side-by-side white lines (CDLGAPSIDESIDEWHITE)"
description: "A three-candle pattern: a first candle followed by two white candles of similar body size that both gap the same direction (up or down) from the first…"
---

## Summary

A three-candle pattern: a first candle followed by two white candles of similar body size that both gap the same direction (up or down) from the first candle's real body and open at about the same level. It is a continuation signal whose sign reports the gap direction; the code does not verify a prior trend.

## Notes

- Does not verify the prior trend the continuation signal classically assumes.
- Bulkowski's data shows the bullish form is rare (984 occurrences out of 4.7 million candle lines, frequency rank 73/103) but continues as labeled 66% of the time; the bearish form is rarer still (frequency rank 86/103) and its 56% continuation rate is "near random" — Bulkowski cautions the bearish sample is too thin to trust. ([thepatternsite.com](https://thepatternsite.com/SidebySideWhiteLinesBull.html))

## Inputs

- `inOpen` — Open price of each bar
- `inHigh` — High price of each bar
- `inLow` — Low price of each bar
- `inClose` — Close price of each bar

## Outputs

- `outInteger` — +100 for an up-gap (bullish continuation), -100 for a down-gap (bearish continuation), 0 when no pattern. Sign is set solely by the C2-vs-C1 gap direction (realbodygapup ? 100 : -100)

## Output Values

| Value | Meaning |
|-------|---------|
| -100 | Bearish continuation: two similar white candles gapped down together and held the gap, suggesting the decline will resume |
| 0 | No pattern |
| 100 | Bullish continuation: two similar white candles gapped up together and held the gap, suggesting the advance will resume |

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

TA-Lib Definition: [`cdlgapsidesidewhite.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/cdlgapsidesidewhite/cdlgapsidesidewhite.c) · [`cdlgapsidesidewhite.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/cdlgapsidesidewhite/cdlgapsidesidewhite.yaml)

| Native | File |
|--------|------|
| C | [`ta_CDLGAPSIDESIDEWHITE.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_CDLGAPSIDESIDEWHITE.c) |
| Rust | [`cdlgapsidesidewhite.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/cdlgapsidesidewhite.rs) |
| Java | [`Core_CDLGAPSIDESIDEWHITE.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/fragments/Core_CDLGAPSIDESIDEWHITE.java) |

TA-Lib is also available for Python, R and more using a [wrapper](/install/#wrappers).

## Aliases

Up/Down-gap side-by-side white lines, Gapping side-by-side white lines

## See Also

[CDLTASUKIGAP](/functions/cdltasukigap.md) · [CDLXSIDEGAP3METHODS](/functions/cdlxsidegap3methods.md)
