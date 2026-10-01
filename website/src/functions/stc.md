---
title: "Schaff Trend Cycle (STC)"
description: "Schaff Trend Cycle (Doug Schaff): a MACD line passed twice through a stochastic, each pass smoothed by half."
---

## Summary

Schaff Trend Cycle (Doug Schaff): a MACD line passed twice through a stochastic, each pass smoothed by half. Bounded 0 to 100, read against 25 and 75: turning up from below 25 is bullish, turning down from above 75 bearish.

## Formula

MACD[t] = EMA(x, fast)[t] - EMA(x, slow)[t]

Frac1[t] = 100 * (MACD[t] - min(MACD, cycle)) / (max(MACD, cycle) - min(MACD, cycle))

PF[t] = PF[t-1] + 0.5 * (Frac1[t] - PF[t-1])

Frac2[t] = 100 * (PF[t] - min(PF, cycle)) / (max(PF, cycle) - min(PF, cycle))

STC[t] = PFF[t] = PFF[t-1] + 0.5 * (Frac2[t] - PFF[t-1])

min and max are taken over the last cycle values, current one included. When a range is zero, the fraction keeps its previous value (0 before any). Each smoother starts on its first input.

## Notes

- The smoothing factor is fixed at 0.5, as in Schaff's published code.
- The MACD line is TA-Lib's: both EMAs are seeded with a simple average over windows ending on the same bar. The published code runs the EMAs from the first bar of the data.
- If the slow period is set smaller than the fast period, the two are swapped, as in `MACD`.
- Being recursive, an output depends on how much history precedes it. The unstable period warms the two smoothers; the EMA unstable period warms the MACD line.

## Inputs

- `inReal` — Input series (typically close)

## Outputs

- `outReal` — Schaff Trend Cycle, from 0 to 100

## Parameters

| Parameter | Type | Default | Accepted values | Description |
| --- | --- | --- | --- | --- |
| `optInFastPeriod` | integer | 23 | 2–100000 | Period of the fast EMA |
| `optInSlowPeriod` | integer | 50 | 2–100000 | Period of the slow EMA |
| `optInCyclePeriod` | integer | 10 | 2–100000 | Window of both stochastic stages |

## Properties

**Numerical Stability:** [Initial Unstable Period](/functions/stability.md#initial-unstable-period) — Tunable via STC's own unstable period and EMA's, which STC computes internally.

<div class="flag-table">

|  |
| :-- |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Overlap Input</span> |
| <span class="flag-box">✅</span> **Independent Y-Axis** <span class="flag-tip" tabindex="0" role="note" aria-label="Output is on its own scale, drawn in a separate pane below the price chart." data-tip="Output is on its own scale, drawn in a separate pane below the price chart.">i</span> |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Candlestick</span> |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Can Output NaN or ±Inf</span> |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Identity at Period 1</span> |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Display Shift</span> |

</div>

## Implementation

TA-Lib Definition: [`stc.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/stc/stc.c) · [`stc.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/stc/stc.yaml)

| Native | File |
|--------|------|
| C | [`ta_STC.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_STC.c) |
| Rust | [`stc.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/stc.rs) |
| Java | [`Core_STC.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/fragments/Core_STC.java) |
| C# | [`Core_STC.cs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/csharp/library/src/Core_STC.cs) |

TA-Lib is also available for Python, R and more using a [wrapper](/install/#wrappers).

## Aliases

Schaff Trend Cycle, Schaff TC

## See Also

[MACD](/functions/macd.md) · [STOCHF](/functions/stochf.md) · [STOCHRSI](/functions/stochrsi.md)

## References

- Doug Schaff, ["Releasing the Code to the Schaff Trend Cycle"](https://web.archive.org/web/20090418215759/mediaserver.fxstreet.com/Reports/99afdb5f-d41d-4a2c-802c-f5d787df886c/ebfbf387-4b27-4a0f-848c-039f4ab77c00.pdf), FX-Strategy.com, published on FXStreet.com, February 15, 2008. The EasyLanguage source, and the 23/50/10 defaults.
- Doug Schaff, "Catching Currency Moves with The Schaff Trend Cycle Indicator", *Chartpoint*, July/August 2002.
