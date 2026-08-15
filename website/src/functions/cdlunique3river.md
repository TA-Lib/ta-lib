---
title: "Unique 3 River (CDLUNIQUE3RIVER)"
description: "A three-candle bullish reversal pattern: a long black candle, then a black harami candle that makes a lower low, then a small white candle."
---

## Summary

A three-candle bullish reversal pattern: a long black candle, then a black harami candle that makes a lower low, then a small white candle. Signals a potential bullish reversal, ideally in a downtrend (trend not checked by the code). A hit (+100) marks a bullish reversal; significant in a downtrend, which the function does not verify.

## Notes

- Although classically a bullish reversal (and TA-Lib only emits +100), Bulkowski's testing found the opposite: it acts as a bearish continuation 60% of the time, ranking 60th of 103 patterns overall. ([thepatternsite.com](https://thepatternsite.com/Unique3RiverBottom.html))

## Inputs

- `inOpen` — Open price of each bar
- `inHigh` — High price of each bar
- `inLow` — Low price of each bar
- `inClose` — Close price of each bar

## Outputs

- `outInteger` — +100 when the pattern is present, 0 otherwise. Bullish-only: never emits -100

## Output Values

| Value | Meaning |
|-------|---------|
| 0 | No pattern |
| 100 | Unique 3 River detected — bullish reversal signal |

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

TA-Lib Definition: [`cdlunique3river.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/cdlunique3river/cdlunique3river.c) · [`cdlunique3river.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/cdlunique3river/cdlunique3river.yaml)

| Native | File |
|--------|------|
| C | [`ta_CDLUNIQUE3RIVER.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_CDLUNIQUE3RIVER.c) |
| Rust | [`cdlunique3river.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/cdlunique3river.rs) |
| Java | [`Core_CDLUNIQUE3RIVER.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/fragments/Core_CDLUNIQUE3RIVER.java) |

TA-Lib is also available for Python, R and more using a [wrapper](/install/#wrappers).

## Aliases

Unique 3 River, Unique Three River Bottom

## See Also

[CDLHARAMI](/functions/cdlharami.md) · [CDLHOMINGPIGEON](/functions/cdlhomingpigeon.md) · [CDL3INSIDE](/functions/cdl3inside.md)
