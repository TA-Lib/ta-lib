---
url: 'https://ta-lib.org/functions/choptr.md'
description: >-
  Choppiness Index with a true-range box: the true range travelled over a window
  against the span from its highest true high to its lowest true low.
---
# Choppiness Index (True Range Box) (CHOPTR)

## Summary

Choppiness Index with a true-range box: the true range travelled over a window against the span from its highest true high to its lowest true low. Bill Dreiss's form as published in 1993. Log-scaled so that a straight run reads 0 and bars that each fill the whole box read 100. Measures whether the market is trending, not in which direction: low values mean a directional run, high values sideways chop.

## Formula

TH\[j] = max(High\[j], Close\[j-1]) and TL\[j] = min(Low\[j], Close\[j-1]), the true high and true low. TR\[j] = TH\[j] - TL\[j].

S = SUM( TR\[j] ) and R = MAX( TH\[j] ) - MIN( TL\[j] ), both over j = t-optInTimePeriod+1 .. t.

CHOPTR = 100 \* log10( S / R ) / log10( optInTimePeriod ).

The first true range reads the close one bar before the window, so the first value needs `optInTimePeriod` + 1 bars.

## Notes

* The box reaches the close just before each bar. Most charting platforms instead use the window's highest high minus its lowest low; that form is CHOP. The two differ only on bars where the close before the window lies outside the window's high-low range: a gap into the window.
* With every close inside its own bar's high-low range, the value stays within 0 to 100, up to rounding.
* A window with no true range reports 100.
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

TA-Lib Definition: [`choptr.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/choptr/choptr.c) · [`choptr.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/choptr/choptr.yaml)

| Native | File |
|--------|------|
| C | [`ta_CHOPTR.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_CHOPTR.c) |
| Rust | [`choptr.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/choptr.rs) |
| Java | [`Core_CHOPTR.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/fragments/Core_CHOPTR.java) |
| C# | [`Core_CHOPTR.cs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/csharp/library/src/Core_CHOPTR.cs) |

TA-Lib is also available for Python, R and more using a [wrapper](/install/#wrappers).

## Aliases

Choppiness Index, Dreiss Choppiness Index, True Range Choppiness Index

## See Also

[CHOP](/functions/chop.md) · [VHF](/functions/vhf.md) · [ADX](/functions/adx.md) · [TRANGE](/functions/trange.md)

## References

* Gibbons Burke, "Measuring market choppiness with chaos", *Futures*, October 1993, pp. 52-53
* [Incredible Charts: Choppiness Index](https://www.incrediblecharts.com/indicators/choppiness-index.php)
