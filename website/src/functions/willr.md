---
title: "Williams' %R (WILLR)"
description: "Williams' %R momentum oscillator over a rolling period, bounded in [-100, 0]."
---

## Summary

Williams' %R momentum oscillator over a rolling period, bounded in [-100, 0]. Measures where the current close sits relative to the high-low range of the last N bars. Near 0 = close at period high (overbought); near -100 = close at period low (oversold).

## Formula

%R = -100 * (highestHigh - close) / (highestHigh - lowestLow) over the trailing optInTimePeriod bars; if highestHigh == lowestLow, output 0.

## Inputs

- `inHigh` — High price of each bar
- `inLow` — Low price of each bar
- `inClose` — Close price of each bar

## Outputs

- `outReal` — Williams' %R value in [-100, 0]

## Parameters

| Parameter | Type | Default | Accepted values | Description |
| --- | --- | --- | --- | --- |
| `optInTimePeriod` | integer | 14 | 2–100000 | Lookback bars for the high/low range |

## Properties

**Numerical Stability:** [Start-Independent](/functions/stability.md#start-independent)

| Display<br>Flags |
| :-- |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Overlap Input</span> |
| <span class="flag-box">✅</span> **Independent Y-Axis** <span class="flag-tip" tabindex="0" role="note" aria-label="Output is on its own scale, drawn in a separate pane below the price chart." data-tip="Output is on its own scale, drawn in a separate pane below the price chart.">i</span> |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Candlestick</span> |

## Implementation

TA-Lib Definition: [`willr.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/willr/willr.c) · [`willr.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/willr/willr.yaml)

| Native | File |
|--------|------|
| C | [`ta_WILLR.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_WILLR.c) |
| Rust | [`willr.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/willr.rs) |
| Java | [`Core_WILLR.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/fragments/Core_WILLR.java) |

TA-Lib is also available for Python, R and more using a [wrapper](/install/#wrappers).

## Aliases

Williams %R, Williams Percent R, %R

## See Also

[STOCH](/functions/stoch.md) · [STOCHF](/functions/stochf.md) · [MINMAX](/functions/minmax.md)
