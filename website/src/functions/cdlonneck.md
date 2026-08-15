---
title: "On-Neck Pattern (CDLONNECK)"
description: "A two-candle on-neck pattern: a long black candle followed by a white candle that opens below the prior candle's low and closes right at that low."
---

## Summary

A two-candle on-neck pattern: a long black candle followed by a white candle that opens below the prior candle's low and closes right at that low. Bearish continuation signal. A hit is bearish (bearish continuation); the code does not verify the assumed prior downtrend.

## Formula

Two candles. 1st: black (close<open) with long real body (realbody > BodyLong average). 2nd: white (close>=open); open < prior low; close within the Equal band of the prior low, i.e. (prior_low - EqualAvg) <= close2 <= (prior_low + EqualAvg).

## Notes

- The bearish-continuation reading assumes a prior downtrend, which is not verified.
- Bulkowski's testing found the bearish continuation holds only 56% of the time, which he explicitly calls "near random." ([thepatternsite.com](https://thepatternsite.com/OnNeck.html))

## Inputs

- `inOpen` — Open price of each bar
- `inHigh` — High price of each bar
- `inLow` — Low price of each bar
- `inClose` — Close price of each bar

## Outputs

- `outInteger` — -100 on a match, 0 otherwise. Only -100 is ever emitted (never +100); on-neck is always bearish

## Output Values

| Value | Meaning |
|-------|---------|
| -100 | On-Neck pattern detected: bearish continuation |
| 0 | No pattern |

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

TA-Lib Definition: [`cdlonneck.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/cdlonneck/cdlonneck.c) · [`cdlonneck.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/cdlonneck/cdlonneck.yaml)

| Native | File |
|--------|------|
| C | [`ta_CDLONNECK.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_CDLONNECK.c) |
| Rust | [`cdlonneck.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/cdlonneck.rs) |
| Java | [`Core_CDLONNECK.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/fragments/Core_CDLONNECK.java) |

TA-Lib is also available for Python, R and more using a [wrapper](/install/#wrappers).

## Aliases

On-Neck Pattern, On-Neck Line

## See Also

[CDLINNECK](/functions/cdlinneck.md) · [CDLTHRUSTING](/functions/cdlthrusting.md) · CDLMEETINGLINES
