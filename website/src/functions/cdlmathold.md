---
title: "Mat Hold (CDLMATHOLD)"
description: "A five-candle bullish continuation pattern: a long white candle, an upside real-body-gapped small black candle, two more small falling candles that hold…"
---

## Summary

A five-candle bullish continuation pattern: a long white candle, an upside real-body-gapped small black candle, two more small falling candles that hold within the first body, and a final white candle closing above the reaction days' highs. Signals continuation of the prior uptrend. Hit = bullish continuation of the existing uptrend.

## Notes

- The colors of the third and fourth (reaction) candles are not checked, although they are classically black.
- The continuation reading assumes a prior uptrend, which is not verified.
- Bulkowski's own dataset contains only 52 Mat Hold occurrences out of 4.7 million candle lines; he explicitly warns the 78% continuation rate he measured "will likely be wrong or at least subject to large change as additional samples become available." ([thepatternsite.com](https://thepatternsite.com/MatHold.html))

## Inputs

- `inOpen` — Open price of each bar
- `inHigh` — High price of each bar
- `inLow` — Low price of each bar
- `inClose` — Close price of each bar

## Outputs

- `outInteger` — +100 when the bullish Mat Hold is detected, 0 otherwise. Never emits -100

## Output Values

| Value | Meaning |
|-------|---------|
| 0 | No pattern |
| 100 | Mat Hold detected — bullish continuation signal |

## Parameters

| Parameter | Type | Default | Accepted values | Description |
| --- | --- | --- | --- | --- |
| `optInPenetration` | real | 0.5 | ≥ 0 | Max fraction of the 1st white body the reaction days (3rd, 4th) may penetrate |

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

TA-Lib Definition: [`cdlmathold.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/cdlmathold/cdlmathold.c) · [`cdlmathold.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/cdlmathold/cdlmathold.yaml)

| Native | File |
|--------|------|
| C | [`ta_CDLMATHOLD.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_CDLMATHOLD.c) |
| Rust | [`cdlmathold.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/cdlmathold.rs) |
| Java | [`Core_CDLMATHOLD.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/fragments/Core_CDLMATHOLD.java) |

TA-Lib is also available for Python, R and more using a [wrapper](/install/#wrappers).

## Aliases

Mat Hold

## See Also

[CDLRISEFALL3METHODS](/functions/cdlrisefall3methods.md) · [CDLXSIDEGAP3METHODS](/functions/cdlxsidegap3methods.md)
