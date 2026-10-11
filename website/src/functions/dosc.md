---
title: "Derivative Oscillator (DOSC)"
description: "Derivative Oscillator: Wilder's RSI, smoothed by two exponential averages in series, with a simple average of that smoothed line subtracted from it."
---

## Summary

Derivative Oscillator: Wilder's RSI, smoothed by two exponential averages in series, with a simple average of that smoothed line subtracted from it. Constance Brown's reading is that the RSI's own swings are too noisy to time with, so she smooths it twice and then plots the distance from its own average as a histogram — MACD's histogram construction, applied to a smoothed RSI rather than to price. The result is in RSI points, centred on zero: crossings of the zero line mark the turn, and the height measures how far the smoothed RSI has run from its mean.

## Formula

RSI = RSI(inReal, timePeriod)   (Wilder, SMA-seeded)

S1 = EMA(RSI, firstPeriod)

DS = EMA(S1, secondPeriod)

DOSC = DS - SMA(DS, signalPeriod)

## Notes

- Every stage is a call to a function TA-Lib already ships, so the output is identical, bit for bit, to `TA_RSI` followed by two `TA_EMA` calls, a `TA_SMA` and a `TA_SUB`. The shortest expression of that chain takes five calls and three intermediate buffers; this computes it in one pass without materialising them.
- Each exponential average is seeded with a simple average of its own first inputs, the same seeding TA-Lib's EMA uses, and the second seeds on what the first publishes. `TA_SetUnstablePeriod` on either `TA_FUNC_UNST_RSI` or `TA_FUNC_UNST_EMA` discards more of that warm-up, and the EMA setting counts twice because there are two exponential stages. Implementations seeding each stage from a single first sample differ over the transient and agree once it decays.
- The degenerate reading is whatever `TA_RSI` answers when neither a gain nor a loss has been seen since the seed. Some other implementations answer 100 there and will disagree over that stretch.
- The periods are independent: the stages commute, so no ordering between the two smoothing periods is required or checked. A signal period of 1 would make the output identically zero, so the minimum is 2.

## Inputs

- `inReal` — Input series, usually the close

## Outputs

- `outReal` — Derivative Oscillator, in RSI points, centred on zero

## Parameters

| Parameter | Type | Default | Accepted values | Description |
| --- | --- | --- | --- | --- |
| `optInTimePeriod` | integer | 14 | 2–100000 | Period of the RSI |
| `optInFirstPeriod` | integer | 5 | 2–100000 | Period of the first smoothing, applied to the RSI |
| `optInSecondPeriod` | integer | 3 | 2–100000 | Period of the second smoothing, applied to the first |
| `optInSignalPeriod` | integer | 9 | 2–100000 | Period of the simple average subtracted from the smoothed line |

## Properties

**Numerical Stability:** [Initial Unstable Period](/functions/stability.md#initial-unstable-period) — Inherited from EMA and RSI, which DOSC computes internally; tunable via EMA and RSI's unstable period.

<div class="flag-table">

|  |
| :-- |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Overlap Input</span> |
| <span class="flag-box">✅</span> **Independent Y-Axis** <span class="flag-tip" tabindex="0" role="note" aria-label="Output is on its own scale, drawn in a separate pane below the price chart." data-tip="Output is on its own scale, drawn in a separate pane below the price chart.">i</span> |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Candlestick</span> |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Can Output NaN or ±Inf</span> |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Identity at Period 1</span> |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Display Shift</span> |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Uses Transcendental</span> |

</div>

## Implementation

TA-Lib Definition: [`dosc.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/dosc/dosc.c) · [`dosc.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/dosc/dosc.yaml)

| Native | File |
|--------|------|
| C | [`ta_DOSC.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_DOSC.c) |
| Rust | [`dosc.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/dosc.rs) |
| Java | [`Core_DOSC.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/fragments/Core_DOSC.java) |
| C# | [`Core_DOSC.cs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/csharp/library/src/Core_DOSC.cs) |

TA-Lib is also available for Python, R and more using a [wrapper](/install/#wrappers).

## Aliases

derivative oscillator

## See Also

[RSI](/functions/rsi.md) · [STOCHRSI](/functions/stochrsi.md) · [MACD](/functions/macd.md) · [AC](/functions/ac.md)

## References

- Constance M. Brown, "The Derivative Oscillator: A New Approach to an Old Problem", *Journal of Technical Analysis* (MTA Journal), Issue 45, Winter-Spring 1994, pp. 45-50
