---
url: 'https://ta-lib.org/functions/cdlhikkakemod.md'
description: >-
  A four-candle pattern: two successively narrower inside bars, then a breakout
  bar, with the second candle closing near one extreme of its range.
---
# Modified Hikkake Pattern (CDLHIKKAKEMOD)

## Summary

A four-candle pattern: two successively narrower inside bars, then a breakout bar, with the second candle closing near one extreme of its range. Bullish or bearish reversal signal. Bullish (+) or bearish (-) reversal; per the code's note it is significant in a downtrend (bull) or uptrend (bear), context the code does not verify.

## Notes

* Does not verify the prior trend (downtrend for bullish, uptrend for bearish) that this reversal pattern assumes.
* A bar that is both a new setup and the confirmation of an earlier one reports the new setup; the earlier one ends unconfirmed.

## Inputs

* `inOpen` — Open price of each bar
* `inHigh` — High price of each bar
* `inLow` — Low price of each bar
* `inClose` — Close price of each bar

## Outputs

* `outInteger` — +100 bullish hikkake bar, -100 bearish; +200 confirmed bullish, -200 confirmed bearish; 0 otherwise

## Output Values

How to read these values from the output's flags: [rW8](/spec/inputs-outputs/#rw8).

| Value | Meaning |
|-------|---------|
| -200 | Bearish Modified Hikkake confirmed: within 3 bars, a close below the last inside bar's low confirms the reversal lower |
| -100 | Bearish Modified Hikkake — a false upside breakout traps buyers, warning that the uptrend may be topping out |
| 0 | No pattern on this bar; a setup may still be awaiting confirmation |
| 100 | Bullish Modified Hikkake — a false downside breakout traps sellers, warning that the downtrend may be bottoming out |
| 200 | Bullish Modified Hikkake confirmed: within 3 bars, a close above the last inside bar's high confirms the reversal higher |

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

TA-Lib Definition: [`cdlhikkakemod.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/cdlhikkakemod/cdlhikkakemod.c) · [`cdlhikkakemod.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/cdlhikkakemod/cdlhikkakemod.yaml)

| Native | File |
|--------|------|
| C | [`ta_CDLHIKKAKEMOD.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_CDLHIKKAKEMOD.c) |
| Rust | [`cdlhikkakemod.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/cdlhikkakemod.rs) |
| Java | [`Core_CDLHIKKAKEMOD.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/fragments/Core_CDLHIKKAKEMOD.java) |
| C# | [`Core_CDLHIKKAKEMOD.cs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/csharp/library/src/Core_CDLHIKKAKEMOD.cs) |

TA-Lib is also available for Python, R and more using a [wrapper](/install/#wrappers).

## Aliases

Modified Hikkake, Modified Hikkake Pattern

## See Also

[CDLHIKKAKE](/functions/cdlhikkake.md)
