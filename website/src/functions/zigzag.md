---
title: "Zig Zag (ZIGZAG)"
description: "The percent-filter swing line."
---

## Summary

The percent-filter swing line. A leg runs from the last confirmed pivot to the most extreme price since it, and it reverses only when price moves at least `optInSensitivity` percent away from that extreme and at least `optInMinTrendLength` bars have passed since the pivot's own bar. Achelis describes the chart form and warns that *"the last 'leg' displayed in a Zig Zag chart can change"*.

This function does not emit the chart line, which is drawn with hindsight. It emits the causal state the line is drawn from, one row per bar and final once emitted: the price of the swing extreme currently being tracked, the absolute bar index of that extreme, and whether the current leg is up or down.

The chart is rebuilt exactly from the outputs. A pivot is confirmed at bar `i > outBegIdx` whenever `outTrend[i]` differs from `outTrend[i-1]`, and that pivot is `(outPivotIdx[i-1], outZigZag[i-1])`. The last, still-open pivot is `(outPivotIdx[last], outZigZag[last])` — the leg Achelis warns about.

## Formula

With `H = inHigh`, `L = inLow`, `m = optInMinTrendLength`, `s = optInSensitivity / 100`:

    up = 1 + s,  dn = 1 - s

    seed at bar j0 = startIdx - m:   trend = -1,  P = L[j0],  p = j0

    for i = j0+1 .. endIdx:
       trend = -1:  if H[i] >= P*up and i - p >= m   then trend = +1, P = H[i], p = i
                    else if L[i] <= P                then P = L[i], p = i
       trend = +1:  if L[i] <= P*dn and i - p >= m   then trend = -1, P = L[i], p = i
                    else if H[i] >= P                then P = H[i], p = i

       outZigZag = P,  outTrend = trend,  outPivotIdx = p

## Notes

- **Reversal is tested before extension.** On an outside bar that both makes a new extreme and clears the threshold, the leg reverses and the old pivot stays where it was. The other order moves the pivot and loses the reversal, which is a whole leg of difference rather than a rounding.
- **Ties extend.** The comparisons are `<=` and `>=`, so at an equal price the pivot moves to the later bar: `outZigZag` does not change and `outPivotIdx` does. That movement is the only thing the index output carries and the price output cannot.
- **The gate counts from the pivot's own bar, the seed included.** No reversal can fire before the first output bar; the first one can fire exactly there.
- **The seed is a low.** A series that only rises therefore reports an up leg from its first reversal and never returns to a down leg.
- The lookback is `optInMinTrendLength` and does not depend on the sensitivity: the threshold decides whether a reversal fires, never how early it may.
- `optInSensitivity` is a percent, as Achelis, StockCharts, TTR, Skender and TradingView take it. 0 and 1 are degenerate but defined: at 0 a reversal fires on almost every bar the gate allows, and at 100 no downward reversal fires on positive prices.
- The value at a bar depends on where the caller started, through the seed; the function is `path_dependent` for that reason, as SAR and SUPERTREND are.

## Inputs

- `inHigh` — High price of each bar
- `inLow` — Low price of each bar

## Outputs

- `outZigZag` — Price of the swing extreme currently being tracked
- `outTrend` — +1 while the leg is up, -1 while it is down
- `outPivotIdx` — Absolute index, into the input, of the bar that extreme sits on

## Parameters

| Parameter | Type | Default | Accepted values | Description |
| --- | --- | --- | --- | --- |
| `optInSensitivity` | real | 5 | 0–100 | Minimum move away from the current extreme that reverses the leg, in percent |
| `optInMinTrendLength` | integer | 1 | 1–100000 | Minimum number of bars between two pivots |

## Properties

**Numerical Stability:** [Path-Dependent](/functions/stability.md#path-dependent)

<div class="flag-table">

|  |
| :-- |
| <span class="flag-box">✅</span> **Overlap Input** <span class="flag-tip" tabindex="0" role="note" aria-label="Output is on the same scale as the input price, so it is drawn over the price chart." data-tip="Output is on the same scale as the input price, so it is drawn over the price chart.">i</span> |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Independent Y-Axis</span> |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Candlestick</span> |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Can Output NaN or ±Inf</span> |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Identity at Period 1</span> |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Display Shift</span> |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Uses Transcendental</span> |

</div>

## Implementation

TA-Lib Definition: [`zigzag.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/zigzag/zigzag.c) · [`zigzag.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/zigzag/zigzag.yaml)

| Native | File |
|--------|------|
| C | [`ta_ZIGZAG.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_ZIGZAG.c) |
| Rust | [`zigzag.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/zigzag.rs) |
| Java | [`Core_ZIGZAG.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/fragments/Core_ZIGZAG.java) |
| C# | [`Core_ZIGZAG.cs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/csharp/library/src/Core_ZIGZAG.cs) |

TA-Lib is also available for Python, R and more using a [wrapper](/install/#wrappers).

## Aliases

Zig Zag, ZigZag, Filtered Waves

## See Also

[SAR](/functions/sar.md) · [SUPERTREND](/functions/supertrend.md) · [MINMAXINDEX](/functions/minmaxindex.md) · [MIDPRICE](/functions/midprice.md)

## References

- Arthur A. Merrill, *Filtered Waves, Basic Theory: A Tool for Stock Market Analysis*, The Analysis Press, 1977
- Steven B. Achelis, *Technical Analysis from A to Z*, "Zig Zag"
- StockCharts ChartSchool, "ZigZag"
