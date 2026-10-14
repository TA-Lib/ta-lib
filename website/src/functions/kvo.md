---
title: "Klinger Volume Oscillator (KVO)"
description: "Klinger Volume Oscillator: each bar's volume is signed by the direction of its high-low-close sum and weighted by where the bar's own range sits inside…"
---

## Summary

Klinger Volume Oscillator: each bar's volume is signed by the direction of its high-low-close sum and weighted by where the bar's own range sits inside the cumulative range of the current trend run, then the resulting volume force is smoothed twice and differenced. Klinger's reading is that volume alone says how much trading happened but not what it accomplished, so he scales it by the share of the trend's accumulated range that this bar actually covered. The oscillator crosses zero as the balance of that force changes hands, and crosses its own trigger line earlier.

## Formula

hlc = high + low + close;  dm = high - low

trend = +1 if hlc rose, -1 if hlc fell, unchanged if hlc repeated

cm = previous dm + current dm on a trend change, otherwise previous cm + current dm

vf = volume * |2 * (dm / cm) - 1| * 100 * trend

KVO = EMA(vf, fastPeriod) - EMA(vf, slowPeriod)

Signal = EMA(KVO, signalPeriod)

## Notes

- The trend is **held** when the high-low-close sum repeats, which is the article's own tenet: the existing trend is maintained on equality. A reading that breaks the tie in either direction changes every later bar, because the cumulative range is reset by a trend change.
- The cumulative range is reset by every trend change, so two calls starting at different bars carry different range state only until the first trend change after both; what remains between them is the seed error of the three averages, which fades at the rate of the longest period. KVO converges, but as a small difference of two large averages it needs more bars than one of them would.
- All three exponential averages are seeded from the volume force of the same anchor bar rather than from a simple average of their own first inputs, so one warm-up covers all three. KVO is exactly zero on that seed bar, which falls inside the warm-up and is never output. `TA_SetUnstablePeriod(TA_FUNC_UNST_EMA, ...)` discards more of that warm-up.
- The weight is the 1997 form, `|2*(dm/cm) - 1|`, which is what the December 1997 Traders' Tips print. Some later implementations use `|2*(dm/cm - 1)|`, which is a different series.
- A cumulative range of exactly zero leaves the volume force at zero rather than dividing.

## Inputs

- `inHigh` — High price series
- `inLow` — Low price series
- `inClose` — Close price series
- `inVolume` — Volume series

## Outputs

- `outKVO` — Klinger Volume Oscillator
- `outKVOSignal` — Trigger line, an exponential average of the oscillator

## Parameters

| Parameter | Type | Default | Accepted values | Description |
| --- | --- | --- | --- | --- |
| `optInFastPeriod` | integer | 34 | 2–100000 | Period of the faster smoothing of the volume force |
| `optInSlowPeriod` | integer | 55 | 2–100000 | Period of the slower smoothing of the volume force |
| `optInSignalPeriod` | integer | 13 | 2–100000 | Smoothing period of the trigger line |

## Properties

**Numerical Stability:** [Initial Unstable Period](/functions/stability.md#initial-unstable-period) — Inherited from EMA, which KVO computes internally; tunable via EMA's unstable period.

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
| <span class="flag-box">☐</span> <span style="opacity:0.5">Input Must Be Positive</span> |

</div>

## Implementation

TA-Lib Definition: [`kvo.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/kvo/kvo.c) · [`kvo.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/kvo/kvo.yaml)

| Native | File |
|--------|------|
| C | [`ta_KVO.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_KVO.c) |
| Rust | [`kvo.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/kvo.rs) |
| Java | [`Core_KVO.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/fragments/Core_KVO.java) |
| C# | [`Core_KVO.cs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/csharp/library/src/Core_KVO.cs) |

TA-Lib is also available for Python, R and more using a [wrapper](/install/#wrappers).

## Aliases

klinger, klinger volume oscillator, klinger oscillator

## See Also

[ADOSC](/functions/adosc.md) · [OBV](/functions/obv.md) · [MFI](/functions/mfi.md) · [AD](/functions/ad.md)

## References

- Stephen J. Klinger, "Identifying Trends With Volume Analysis", *Technical Analysis of Stocks & Commodities*, v15:12 (December 1997)
