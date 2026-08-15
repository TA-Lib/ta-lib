---
title: "Evening Star (CDLEVENINGSTAR)"
description: "A three-candle bearish reversal pattern: a long white candle, a short-bodied star gapping up, then a black candle closing well down into the first…"
---

## Summary

A three-candle bearish reversal pattern: a long white candle, a short-bodied star gapping up, then a black candle closing well down into the first candle's body. A hit signals a bearish reversal (most significant in an uptrend).

## Notes

- Does not verify the preceding uptrend the bearish reversal classically assumes.
- The third candle only needs a body longer than short, not the full long body some definitions require.

## Inputs

- `inOpen` — Open price of each bar
- `inHigh` — High price of each bar
- `inLow` — Low price of each bar
- `inClose` — Close price of each bar

## Outputs

- `outInteger` — -100 when detected (always bearish), 0 otherwise. Never emits +100

## Output Values

| Value | Meaning |
|-------|---------|
| -100 | Evening Star pattern detected: bearish |
| 0 | No pattern |

## Parameters

| Parameter | Type | Default | Accepted values | Description |
| --- | --- | --- | --- | --- |
| `optInPenetration` | real | 0.3 | ≥ 0 | Fraction of the 1st candle's real body the 3rd close must penetrate below the 1st close; larger requires deeper penetration |

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

TA-Lib Definition: [`cdleveningstar.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/cdleveningstar/cdleveningstar.c) · [`cdleveningstar.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/cdleveningstar/cdleveningstar.yaml)

| Native | File |
|--------|------|
| C | [`ta_CDLEVENINGSTAR.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_CDLEVENINGSTAR.c) |
| Rust | [`cdleveningstar.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/cdleveningstar.rs) |
| Java | [`Core_CDLEVENINGSTAR.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/fragments/Core_CDLEVENINGSTAR.java) |

TA-Lib is also available for Python, R and more using a [wrapper](/install/#wrappers).

## Aliases

Evening Star

## See Also

[CDLEVENINGDOJISTAR](/functions/cdleveningdojistar.md) · [CDLMORNINGSTAR](/functions/cdlmorningstar.md) · [CDLMORNINGDOJISTAR](/functions/cdlmorningdojistar.md) · CDLSTARSINSOUTH
