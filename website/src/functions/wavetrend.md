---
title: "WaveTrend Oscillator (WAVETREND)"
description: "WaveTrend Oscillator: how far the typical price sits from its own exponential average, divided by an exponential average of that distance and by…"
---

## Summary

WaveTrend Oscillator: how far the typical price sits from its own exponential average, divided by an exponential average of that distance and by Lambert's CCI constant, then smoothed. The reading is the plain stochastic's complaint answered a different way — rather than bounding the oscillator by construction, it scales it by how far price has recently been travelling, so the same numeric level means the same thing in a quiet market and a fast one. The oscillator line and its short simple average cross; the crossings that matter are the ones beyond the extremes, which the author draws at ±53 and ±60.

## Formula

ap = (high + low + close) / 3

esa = EMA(ap, channelPeriod)

d = EMA(|ap - esa|, channelPeriod)

ci = (ap - esa) / (0.015 * d)

WT1 = EMA(ci, averagePeriod)

WT2 = SMA(WT1, signalPeriod)

## Notes

- The middle stage is an exponential CCI, not `TA_CCI`: CCI averages with a simple moving average and takes the mean deviation around that window's own average, where this uses two exponential averages.
- On a market that has stopped moving, the exponential average stops moving too — once its step falls under half an ulp it freezes, a few ulps away from the price. That frozen gap is a real non-zero distance, so dividing by its own average walks the oscillator to ±66.67, an extreme reading produced by nothing but rounding. This answers 0 instead, by taking the distance as zero whenever the price and its average both repeat. Implementations without that test drift to the extreme on flat data, at a bar that depends on their arithmetic.
- A zero divisor is tested on the scaled deviation, the quantity the division actually uses, and answers the neutral 0 rather than dividing.
- Each exponential average is seeded with a simple average of its own first inputs, the same seeding TA-Lib's EMA uses, and each seeds on what the stage before it publishes. `TA_SetUnstablePeriod(TA_FUNC_UNST_EMA, ...)` discards more of that warm-up, and it counts three times because there are three exponential stages.
- The difference the author also plots is `TA_SUB(outWT1, outWT2)`.

## Inputs

- `inHigh` — High price series
- `inLow` — Low price series
- `inClose` — Close price series

## Outputs

- `outWT1` — WaveTrend oscillator line
- `outWT2` — Simple average of the oscillator line

## Parameters

| Parameter | Type | Default | Accepted values | Description |
| --- | --- | --- | --- | --- |
| `optInChannelPeriod` | integer | 10 | 2–100000 | Period of the price channel, used by both the average and the deviation |
| `optInAveragePeriod` | integer | 21 | 1–100000 | Smoothing for the oscillator line |
| `optInSignalPeriod` | integer | 4 | 1–100000 | Period of the simple average making the signal line |

## Properties

**Numerical Stability:** [Initial Unstable Period](/functions/stability.md#initial-unstable-period) — Inherited from EMA, which WAVETREND computes internally; tunable via EMA's unstable period.

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

TA-Lib Definition: [`wavetrend.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/wavetrend/wavetrend.c) · [`wavetrend.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/wavetrend/wavetrend.yaml)

| Native | File |
|--------|------|
| C | [`ta_WAVETREND.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_WAVETREND.c) |
| Rust | [`wavetrend.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/wavetrend.rs) |
| Java | [`Core_WAVETREND.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/fragments/Core_WAVETREND.java) |
| C# | [`Core_WAVETREND.cs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/csharp/library/src/Core_WAVETREND.cs) |

TA-Lib is also available for Python, R and more using a [wrapper](/install/#wrappers).

## Aliases

wavetrend, wavetrend oscillator, WT

## See Also

[CCI](/functions/cci.md) · [SMI](/functions/smi.md) · [STOCHRSI](/functions/stochrsi.md) · [TSI](/functions/tsi.md)

## References

- LazyBear, "WaveTrend Oscillator [WT]", TradingView published script, 2014
