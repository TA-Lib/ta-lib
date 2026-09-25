---
title: "Correlation Trend Indicator (CTI)"
description: "John F. Ehlers' Correlation Trend Indicator: the Pearson correlation of the last optInTimePeriod closes against a straight line of positive slope."
---

## Summary

John F. Ehlers' Correlation Trend Indicator: the Pearson correlation of the last `optInTimePeriod` closes against a straight line of positive slope. Bounded in -1..+1 by construction — `+1` is a perfectly linear uptrend, `-1` a perfectly linear downtrend, `0` no linear trend. Trend strength and direction in one bounded number, with no smoothing, no recursion and no filter coefficients.

Arithmetically it is [`CORREL`](/functions/correl.md) of the series against a ramp, with the ramp side collapsed to closed-form constants.

## Formula

With `n` = `optInTimePeriod`, `x` the window's closes and `y` a straight line rising with time:

    CTI = ( n·Σxy − Σx·Σy ) / sqrt( ( n·Σx² − (Σx)² ) · ( n·Σy² − (Σy)² ) ), or 0 when the window is flat

A rising series therefore reads positive. The `y` side is data-independent and collapses exactly: `n·Σy² − (Σy)² = n²(n²−1)/12`.

## Notes

- A flat window returns exactly `0.0` rather than holding the previous value as the author's listing does; holding would make the function path-dependent.
- The result is clamped into -1..+1, as `CORREL` is: rounding in three sums can put a coefficient slightly outside its own range.
- `optInTimePeriod` starts at 2, not 1: at `n = 1` the closed form `n²(n²−1)/12` is identically zero and every window is degenerate.

## Inputs

- `inReal` — The series to measure the trend of

## Outputs

- `outReal` — Correlation against the ramp, in -1..+1

## Parameters

| Parameter | Type | Default | Accepted values | Description |
| --- | --- | --- | --- | --- |
| `optInTimePeriod` | integer | 20 | 2–100000 | Number of trailing values correlated against the ramp |

## Properties

**Numerical Stability:** [Start-Independent](/functions/stability.md#start-independent)

<div class="flag-table">

|  |
| :-- |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Overlap Input</span> |
| <span class="flag-box">✅</span> **Independent Y-Axis** <span class="flag-tip" tabindex="0" role="note" aria-label="Output is on its own scale, drawn in a separate pane below the price chart." data-tip="Output is on its own scale, drawn in a separate pane below the price chart.">i</span> |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Candlestick</span> |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Can Output NaN or ±Inf</span> |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Identity at Period 1</span> |

</div>

## Implementation

TA-Lib Definition: [`cti.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/cti/cti.c) · [`cti.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/cti/cti.yaml)

| Native | File |
|--------|------|
| C | [`ta_CTI.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_CTI.c) |
| Rust | [`cti.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/cti.rs) |
| Java | [`Core_CTI.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/fragments/Core_CTI.java) |
| C# | [`Core_CTI.cs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/csharp/library/src/Core_CTI.cs) |

TA-Lib is also available for Python, R and more using a [wrapper](/install/#wrappers).

## Aliases

Correlation Trend Indicator, Ehlers Correlation Trend Indicator, Correlation Trend

## See Also

[CORREL](/functions/correl.md) · [LINEARREG_SLOPE](/functions/linearreg_slope.md) · [VHF](/functions/vhf.md)

## References

- John F. Ehlers, "Correlation As A Trend Indicator", *Technical Analysis of Stocks & Commodities*, V.38:5 (May 2020), Code Listing 1, [author's PDF](https://www.mesasoftware.com/papers/CORRELATION%20AS%20A%20TREND%20INDICATOR.pdf)
