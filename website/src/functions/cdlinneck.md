---
title: "In-Neck Pattern (CDLINNECK)"
description: "A two-candle in-neck pattern: a long black candle followed by a white candle that opens below the prior low and closes just barely into the prior body…"
---

## Summary

A two-candle in-neck pattern: a long black candle followed by a white candle that opens below the prior low and closes just barely into the prior body (near the prior close). It is a bearish continuation signal. A hit signals bearish continuation (the down move is expected to resume).

## Formula

Two candles. First: black (close1 < open1) with a long real body (realbody > candleaverage(BodyLong)). Second: white (close2 >= open2), opens below the first candle's low (open2 < low1), and closes slightly into the first body: close2 >= close1 AND close2 <= close1 + candleaverage(Equal). No prior-trend check is performed.

## Notes

- Does not verify the preceding downtrend that this bearish continuation pattern assumes.

## Inputs

- `inOpen` — Open price of each bar
- `inHigh` — High price of each bar
- `inLow` — Low price of each bar
- `inClose` — Close price of each bar

## Outputs

- `outInteger` — -100 when the in-neck pattern is detected, 0 otherwise. This pattern only ever emits the negative (bearish) signal; it never emits +100

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

TA-Lib Definition: [`cdlinneck.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/cdlinneck/cdlinneck.c) · [`cdlinneck.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/cdlinneck/cdlinneck.yaml)

| Native | File |
|--------|------|
| C | [`ta_CDLINNECK.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_CDLINNECK.c) |
| Rust | [`cdlinneck.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/cdlinneck.rs) |
| Java | [`Core_CDLINNECK.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/fragments/Core_CDLINNECK.java) |

TA-Lib is also available for Python, R and more using a [wrapper](/install/#wrappers).

## Aliases

In-Neck Pattern, In-Neck Line

## See Also

[CDLONNECK](/functions/cdlonneck.md) · [CDLTHRUSTING](/functions/cdlthrusting.md) · [CDLMATCHINGLOW](/functions/cdlmatchinglow.md)
