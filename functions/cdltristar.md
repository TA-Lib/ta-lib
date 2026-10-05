---
url: 'https://ta-lib.org/functions/cdltristar.md'
description: >-
  A three-candle pattern of three consecutive doji where the middle doji is a
  star (its body gaps away from the first). Bullish or bearish reversal signal.
---
# Tristar Pattern (CDLTRISTAR)

## Summary

A three-candle pattern of three consecutive doji where the middle doji is a star (its body gaps away from the first). Bullish or bearish reversal signal.

## Notes

* This reversal pattern does not verify the prior trend it classically assumes.
* Bulkowski's testing found both Tristar variants reverse only marginally better than chance — bullish 60% of the time (rank 28/103 overall, but rare: frequency rank 79/103) and bearish just 52% of the time (rank 76/103) — despite the "exhaustion signal" framing, one of the weaker reversal signals in his candlestick set. ([thepatternsite.com](https://thepatternsite.com/TriStarBull.html))

## Inputs

* `inOpen` — Open price of each bar
* `inHigh` — High price of each bar
* `inLow` — Low price of each bar
* `inClose` — Close price of each bar

## Outputs

* `outInteger` — +100 (bullish, star gapped down), -100 (bearish, star gapped up), or 0 when no pattern. Both signs are emitted

## Output Values

How to read these values from the output's flags: [rW8](/spec/inputs-outputs/#rw8).

| Value | Meaning |
|-------|---------|
| -100 | Bearish Tristar: the middle doji ("star") gaps up from the first, and the top of the third's body stays below the star's; an exhaustion signal warning an uptrend may be topping out |
| 0 | No pattern (or a doji trio without a qualifying star gap) |
| 100 | Bullish Tristar: the middle doji ("star") gaps down from the first, and the bottom of the third's body stays above the star's; an exhaustion signal warning a downtrend may be bottoming out |

## Properties

**Numerical Stability:** [Start-Independent](/functions/stability.md#start-independent)

<div class="flag-table">

|  |
| :-- |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Overlap Input</span> |
| <span class="flag-box">✅</span> **Independent Y-Axis** <span class="flag-tip" tabindex="0" role="note" aria-label="Output is on its own scale, drawn in a separate pane below the price chart." data-tip="Output is on its own scale, drawn in a separate pane below the price chart.">i</span> |
| <span class="flag-box">✅</span> **Candlestick** <span class="flag-tip" tabindex="0" role="note" aria-label="A candlestick pattern; its values are listed under Output Values." data-tip="A candlestick pattern; its values are listed under Output Values.">i</span> |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Can Output NaN or ±Inf</span> |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Identity at Period 1</span> |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Display Shift</span> |

</div>

## Implementation

TA-Lib Definition: [`cdltristar.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/cdltristar/cdltristar.c) · [`cdltristar.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/cdltristar/cdltristar.yaml)

| Native | File |
|--------|------|
| C | [`ta_CDLTRISTAR.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_CDLTRISTAR.c) |
| Rust | [`cdltristar.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/cdltristar.rs) |
| Java | [`Core_CDLTRISTAR.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/fragments/Core_CDLTRISTAR.java) |
| C# | [`Core_CDLTRISTAR.cs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/csharp/library/src/Core_CDLTRISTAR.cs) |

TA-Lib is also available for Python, R and more using a [wrapper](/install/#wrappers).

## Aliases

Tristar Pattern, Tri-Star

## See Also

[CDLDOJI](/functions/cdldoji.md) · [CDLDOJISTAR](/functions/cdldojistar.md) · [CDLMORNINGDOJISTAR](/functions/cdlmorningdojistar.md) · [CDLEVENINGDOJISTAR](/functions/cdleveningdojistar.md)
