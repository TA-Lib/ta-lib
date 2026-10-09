---
url: 'https://ta-lib.org/functions/emv.md'
description: >-
  Ease of Movement: the bar-to-bar move of the high-low midpoint divided by a
  box ratio of volume to range, averaged over a trailing window.
---
# Arms Ease of Movement (EMV)

## Summary

Ease of Movement: the bar-to-bar move of the high-low midpoint divided by a box ratio of volume to range, averaged over a trailing window. It is the numeric form of Richard W. Arms, Jr.'s Equivolume box.

The box ratio is positive whenever the bar traded and has a range, so the sign follows the midpoint move. A large positive value means price rose easily, on light volume relative to its range; a large negative value means it fell easily. Values near zero mean volume was heavy for the distance travelled, or price hardly moved.

The output scales with the instrument's volume and with the volume divisor, so its level is comparable only within one instrument at one divisor. Traders mostly watch its sign and its zero crossings.

## Formula

mid\[i] = (high\[i] + low\[i]) / 2

box\[i] = (volume\[i] / D) / (high\[i] - low\[i])

raw\[i] = (mid\[i] - mid\[i-1]) / box\[i]

EMV\[i] = ( sum\_{k=i-N+1..i} raw\[k] ) / N, N = optInTimePeriod, D = optInVolumeDivisor

A bar with no range or no volume has no box ratio and contributes 0. The next bar still measures its midpoint move from that bar.

## Notes

* The range is in price points. Achelis's text gives it in eighths of a point, the pre-decimal US quote unit; that reading is reached by multiplying the divisor by 8.
* The divisor is a pure output scale: doubling it doubles every value. Pick one that suits the instrument's volume.
* A period of 1 returns the unsmoothed one-bar values. Smoothing is a simple moving average; for an exponential one, apply `EMA` to this function's output at a period of 1.

## Inputs

* `inHigh` — High price of each bar
* `inLow` — Low price of each bar
* `inVolume` — Volume of each bar

## Outputs

* `outReal` — Ease of Movement, averaged over the window

## Parameters

| Parameter | Type | Default | Accepted values | Description |
| --- | --- | --- | --- | --- |
| `optInTimePeriod` | integer | 14 | 1–100000 | Number of one-bar values in the simple moving average |
| `optInVolumeDivisor` | real | 10000 | ≥ 1 | Volume is divided by this before it forms the box ratio |

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
| <span class="flag-box">☐</span> <span style="opacity:0.5">Display Shift</span> |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Uses Transcendental</span> |

</div>

## Implementation

TA-Lib Definition: [`emv.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/emv/emv.c) · [`emv.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/emv/emv.yaml)

| Native | File |
|--------|------|
| C | [`ta_EMV.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_EMV.c) |
| Rust | [`emv.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/emv.rs) |
| Java | [`Core_EMV.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/fragments/Core_EMV.java) |
| C# | [`Core_EMV.cs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/csharp/library/src/Core_EMV.cs) |

TA-Lib is also available for Python, R and more using a [wrapper](/install/#wrappers).

## Aliases

Ease of Movement, EOM, Arms Ease of Movement

## See Also

[MEDPRICE](/functions/medprice.md) · [MARKETFI](/functions/marketfi.md) · [EFI](/functions/efi.md) · [SMA](/functions/sma.md) · [EMA](/functions/ema.md)

## References

* Richard W. Arms, Jr., *Volume Cycles in the Stock Market: Market Timing Through Equivolume Charting*, Dow Jones-Irwin, 1983.
* Steven B. Achelis, *Technical Analysis from A to Z*, 2nd edition, McGraw-Hill, 2000, "Ease of Movement".
* W. A. Thorp, [Arms' Ease of Movement: Adding Volume to the Equation](https://www.aaii.com/journal/article/arms-ease-of-movement-adding-volume-to-the-equation), *AAII Journal*, October 2001.
* StockCharts ChartSchool, [Ease of Movement (EMV)](https://chartschool.stockcharts.com/table-of-contents/technical-indicators-and-overlays/technical-indicators/ease-of-movement-emv).
