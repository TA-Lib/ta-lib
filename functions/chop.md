---
url: 'https://ta-lib.org/functions/chop.md'
description: >-
  Choppiness Index: a trend-versus-consolidation gauge, the true range travelled
  over a window against the height of the box its bars span.
---
# Choppiness Index (CHOP)

## Summary

Choppiness Index: a trend-versus-consolidation gauge, the true range travelled over a window against the height of the box its bars span. Devised by Bill Dreiss. Log-scaled so that a straight run reads 0 and bars that each fill the whole box read 100. Like ADX and VHF it measures whether the market is trending, not in which direction: low values mean a directional run, high values sideways chop.

## Formula

TR\[j] = max(High\[j], Close\[j-1]) - min(Low\[j], Close\[j-1]), the true range.

S = SUM( TR\[j] ) and R = MAX( High\[j] ) - MIN( Low\[j] ), both over j = t-optInTimePeriod+1 .. t.

CHOP = 100 \* log10( S / R ) / log10( optInTimePeriod ).

The first true range reads the close one bar before the window, so the first value needs `optInTimePeriod` + 1 bars.

## Notes

* The box is the window's highest high minus its lowest low. The 1993 publication instead spans the highest true high and the lowest true low, which also reach the close just before each bar; that form is CHOPTR. The two differ only on bars where the close before the window lies outside the window's high-low range: a gap into the window.
* Because a gap into the window adds to the true range but not to the box, CHOP has no upper bound and can exceed 100.
* A window with no box height, or no true range, reports 100.
* Dreiss's 3-bar smoothing of the index is not built in; apply a moving average to `outReal` to obtain it.

## Inputs

* `inHigh` — High price of each bar
* `inLow` — Low price of each bar
* `inClose` — Close price of each bar

## Outputs

* `outReal` — Choppiness Index value

## Parameters

| Parameter | Type | Default | Accepted values | Description |
| --- | --- | --- | --- | --- |
| `optInTimePeriod` | integer | 14 | 2–100000 | Number of bars in the window |

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

</div>

## Implementation

TA-Lib Definition: [`chop.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/chop/chop.c) · [`chop.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/chop/chop.yaml)

| Native | File |
|--------|------|
| C | [`ta_CHOP.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_CHOP.c) |
| Rust | [`chop.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/chop.rs) |
| Java | [`Core_CHOP.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/fragments/Core_CHOP.java) |
| C# | [`Core_CHOP.cs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/csharp/library/src/Core_CHOP.cs) |

TA-Lib is also available for Python, R and more using a [wrapper](/install/#wrappers).

## Aliases

Choppiness Index, Dreiss Choppiness Index

## See Also

[CHOPTR](/functions/choptr.md) · [VHF](/functions/vhf.md) · [ADX](/functions/adx.md) · [TRANGE](/functions/trange.md)

## References

* Gibbons Burke, "Measuring market choppiness with chaos", *Futures*, October 1993, pp. 52-53
* [TradingView: Choppiness Index (CHOP)](https://www.tradingview.com/support/solutions/43000501980-choppiness-index-chop/)
