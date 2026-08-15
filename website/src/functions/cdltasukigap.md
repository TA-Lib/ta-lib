---
title: "Tasuki Gap (CDLTASUKIGAP)"
description: "A three-candle pattern: a real-body-gapping candle followed by an opposite-color candle that opens inside its body and closes back into the gap without…"
---

## Summary

A three-candle pattern: a real-body-gapping candle followed by an opposite-color candle that opens inside its body and closes back into the gap without filling it. An upside gap is a bullish continuation signal; a downside gap is a bearish continuation signal.

## Notes

- This continuation pattern does not verify the prior trend it classically assumes; the caller must confirm the trend.
- Bulkowski's testing found the downside Tasuki Gap actually acts as a bullish REVERSAL 54% of the time — opposite its textbook bearish-continuation label — while the upside variant does continue as labeled, but only 57% of the time ("near random"). ([thepatternsite.com](https://thepatternsite.com/DownsideTasukiGap.html))

## Inputs

- `inOpen` — Open price of each bar
- `inHigh` — High price of each bar
- `inLow` — Low price of each bar
- `inClose` — Close price of each bar

## Outputs

- `outInteger` — +100 on a bullish (upside-gap) tasuki gap, -100 on a bearish (downside-gap) tasuki gap, 0 otherwise. Sign equals the color of the gap candle i-1 (candlecolor(i-1)*100)

## Output Values

| Value | Meaning |
|-------|---------|
| -100 | Bearish (downside-gap) Tasuki Gap |
| 0 | No pattern |
| 100 | Bullish (upside-gap) Tasuki Gap |

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

TA-Lib Definition: [`cdltasukigap.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/cdltasukigap/cdltasukigap.c) · [`cdltasukigap.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/cdltasukigap/cdltasukigap.yaml)

| Native | File |
|--------|------|
| C | [`ta_CDLTASUKIGAP.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_CDLTASUKIGAP.c) |
| Rust | [`cdltasukigap.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/cdltasukigap.rs) |
| Java | [`Core_CDLTASUKIGAP.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/fragments/Core_CDLTASUKIGAP.java) |

TA-Lib is also available for Python, R and more using a [wrapper](/install/#wrappers).

## Aliases

Tasuki Gap, Upside/Downside Tasuki Gap

## See Also

[CDLGAPSIDESIDEWHITE](/functions/cdlgapsidesidewhite.md) · [CDLXSIDEGAP3METHODS](/functions/cdlxsidegap3methods.md)
