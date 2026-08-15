---
title: "Concealing Baby Swallow (CDLCONCEALBABYSWALL)"
description: "A four-candle pattern: two black marubozus, then a black candle that gaps down but pokes its upper shadow into the prior body, then a larger black candle…"
---

## Summary

A four-candle pattern: two black marubozus, then a black candle that gaps down but pokes its upper shadow into the prior body, then a larger black candle fully engulfing the third. A hit signals a bullish reversal.

## Notes

- Does not verify the preceding downtrend the pattern classically assumes.
- Despite the bullish-reversal label, Bulkowski's testing found this pattern actually behaves as a bearish continuation 75% of the time — though the finding rests on just 4 occurrences out of 4.7 million candle lines, and it ranks 101st of 103 patterns overall. ([thepatternsite.com](https://thepatternsite.com/ConcealBaby.html))

## Inputs

- `inOpen` — Open price of each bar
- `inHigh` — High price of each bar
- `inLow` — Low price of each bar
- `inClose` — Close price of each bar

## Outputs

- `outInteger` — +100 on a match, 0 otherwise; never emits -100 (pattern is always bullish)

## Output Values

| Value | Meaning |
|-------|---------|
| 0 | No pattern |
| 100 | Concealing Baby Swallow detected — a bullish reversal signal, most meaningful after a downtrend (unverified by the function) |

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

TA-Lib Definition: [`cdlconcealbabyswall.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/cdlconcealbabyswall/cdlconcealbabyswall.c) · [`cdlconcealbabyswall.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/cdlconcealbabyswall/cdlconcealbabyswall.yaml)

| Native | File |
|--------|------|
| C | [`ta_CDLCONCEALBABYSWALL.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_CDLCONCEALBABYSWALL.c) |
| Rust | [`cdlconcealbabyswall.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/cdlconcealbabyswall.rs) |
| Java | [`Core_CDLCONCEALBABYSWALL.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/fragments/Core_CDLCONCEALBABYSWALL.java) |

TA-Lib is also available for Python, R and more using a [wrapper](/install/#wrappers).

## Aliases

Concealing Baby Swallow

## See Also

[CDLMARUBOZU](/functions/cdlmarubozu.md) · [CDLENGULFING](/functions/cdlengulfing.md)
