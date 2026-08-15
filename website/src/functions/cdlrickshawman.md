---
title: "Rickshaw Man (CDLRICKSHAWMAN)"
description: "Single-candle doji with two long shadows whose body sits near the midpoint of the high-low range."
---

## Summary

Single-candle doji with two long shadows whose body sits near the midpoint of the high-low range. It is a neutral indecision signal, not a directional (bullish/bearish) reversal. A hit marks market indecision/uncertainty; neutral, neither bullish nor bearish.

## Notes

- Bulkowski's verdict: "The rickshaw man candle may look pretty on the chart but it has no investment implications that I have been able to find" — his testing shows it continues only 51% of the time, statistically random. ([thepatternsite.com](https://thepatternsite.com/RickshawMan.html))

## Inputs

- `inOpen` — Open price of each bar
- `inHigh` — High price of each bar
- `inLow` — Low price of each bar
- `inClose` — Close price of each bar

## Outputs

- `outInteger` — +100 when the pattern is present, 0 otherwise. Never -100; the code notes the positive value does NOT imply bullish, it signals uncertainty

## Output Values

| Value | Meaning |
|-------|---------|
| 0 | No pattern |
| 100 | Rickshaw Man detected — neutral indecision signal, not a directional (bullish/bearish) bias |

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

TA-Lib Definition: [`cdlrickshawman.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/cdlrickshawman/cdlrickshawman.c) · [`cdlrickshawman.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/cdlrickshawman/cdlrickshawman.yaml)

| Native | File |
|--------|------|
| C | [`ta_CDLRICKSHAWMAN.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_CDLRICKSHAWMAN.c) |
| Rust | [`cdlrickshawman.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/cdlrickshawman.rs) |
| Java | [`Core_CDLRICKSHAWMAN.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/fragments/Core_CDLRICKSHAWMAN.java) |

TA-Lib is also available for Python, R and more using a [wrapper](/install/#wrappers).

## Aliases

Rickshaw Man

## See Also

[CDLLONGLEGGEDDOJI](/functions/cdllongleggeddoji.md) · [CDLDOJI](/functions/cdldoji.md) · [CDLHIGHWAVE](/functions/cdlhighwave.md)
