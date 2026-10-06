---
title: "Fisher Transform (FISHER)"
description: "Ehlers' Fisher Transform: an oscillator that reshapes the midpoint's position in its rolling channel into a near-normal distribution."
---

## Summary

Ehlers' Fisher Transform: an oscillator that reshapes the midpoint's position in its rolling channel into a near-normal distribution. Extreme values become rare and turning points sharp, where a plain channel position spends much of its time pinned near the edges.

The output is unbounded and centred on zero. A peak or trough marks a likely turn, and the Fisher line crossing its Trigger, the same line one bar later, is the author's entry signal.

## Formula

With `P = (high + low) / 2` and `MaxP`, `MinP` its highest and lowest value over the last `optInTimePeriod` bars:

    r       = ( P - MinP ) / ( MaxP - MinP ), or 0.5 when MaxP = MinP
    v       = 0.66 * ( r - 0.5 ) + 0.67 * v[prev]
    v       = 0.999 if v > 0.99, -0.999 if v < -0.99
    Fisher  = 0.5 * ln( ( 1 + v ) / ( 1 - v ) ) + 0.5 * Fisher[prev]
    Trigger = Fisher[prev]

`v` and `Fisher` start from 0, so the first Trigger is 0, and the limited `v` is the one the next bar reads.

## Notes

- A window whose midpoints are all equal takes the neutral position, so a market that does not move decays toward 0. The original divides by the zero range there.

## Inputs

- `inHigh` — High price of each bar
- `inLow` — Low price of each bar

## Outputs

- `outFisher` — Fisher Transform value
- `outTrigger` — Fisher value of the previous bar

## Parameters

| Parameter | Type | Default | Accepted values | Description |
| --- | --- | --- | --- | --- |
| `optInTimePeriod` | integer | 10 | 2–100000 | Number of bars in the channel the midpoint is located in |

## Properties

**Numerical Stability:** [Initial Unstable Period](/functions/stability.md#initial-unstable-period)

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

TA-Lib Definition: [`fisher.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/fisher/fisher.c) · [`fisher.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/fisher/fisher.yaml)

| Native | File |
|--------|------|
| C | [`ta_FISHER.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_FISHER.c) |
| Rust | [`fisher.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/fisher.rs) |
| Java | [`Core_FISHER.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/fragments/Core_FISHER.java) |
| C# | [`Core_FISHER.cs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/csharp/library/src/Core_FISHER.cs) |

TA-Lib is also available for Python, R and more using a [wrapper](/install/#wrappers).

## Aliases

Fisher Transform, Ehlers Fisher Transform

## See Also

[STOCHF](/functions/stochf.md) · [WILLR](/functions/willr.md) · [MIDPRICE](/functions/midprice.md) · [IBS](/functions/ibs.md)

## References

- John F. Ehlers, "Using The Fisher Transform", *Technical Analysis of Stocks & Commodities*, V.20:11 (November 2002), 40-42
- John F. Ehlers, *Cybernetic Analysis for Stocks and Futures*, Wiley, 2004
