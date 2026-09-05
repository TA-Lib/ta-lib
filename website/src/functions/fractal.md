---
title: "Williams Fractal (FRACTAL)"
description: "Williams Fractal: a causal swing-pivot detector."
---

## Summary

Williams Fractal: a causal swing-pivot detector. A bar is a swing high when its high strictly exceeds the highs of the `optInLeftBars` bars before it and the `optInRightBars` bars after it; a swing low is the mirror on the lows. Bill Williams' original is the symmetric five-candle case; independent left and right arms generalise it.

The right arm cannot be known until it has closed, so the verdict is reported on the confirmation bar, `optInRightBars` bars after the pivot itself. Each output value therefore describes the bar `optInRightBars` back, not the bar it is written at: a flag at output index `k` names input bar `outBegIdx + k - optInRightBars`, whose price is `inHigh[...]` / `inLow[...]` at that index.

The two outputs are independent flags rather than one signed value, because an outside bar can be a swing high and a swing low at once.

## Formula

With `L = optInLeftBars`, `R = optInRightBars` and pivot `c = i - R`:

swingHigh(i) = 100 if High[c] > High[j] for every j in [c-L, c+R] other than c, else 0.

swingLow(i) = 100 if Low[c] < Low[j] for every j in [c-L, c+R] other than c, else 0.

## Notes

- Strict on both sides: a bar tied with any other bar of its window is not a pivot. TradingView's Pine runtime differs — its `ta.pivothigh` / `ta.pivotlow` let a tie with an older bar stand and let a tie with a newer bar cancel, i.e. non-strict left and strict right — so a plateau Pine reports as a pivot is not one here.
- Each output is decided on its own side: a high tied with any other high in the window forces `outSwingHigh` to 0 while leaving `outSwingLow` free to fire 100, and the mirror holds. Only a window flat in both series emits 0 on both.

## Inputs

- `inHigh` — High price series
- `inLow` — Low price series

## Outputs

- `outSwingHigh` — 100 when the bar `optInRightBars` back is a strict swing high, 0 otherwise
- `outSwingLow` — 100 when the bar `optInRightBars` back is a strict swing low, 0 otherwise

## Parameters

| Parameter | Type | Default | Accepted values | Description |
| --- | --- | --- | --- | --- |
| `optInLeftBars` | integer | 2 | 1–100000 | Bars before the pivot that it must strictly dominate |
| `optInRightBars` | integer | 2 | 1–100000 | Bars after the pivot that it must strictly dominate, and the delay before the verdict is reported |

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

TA-Lib Definition: [`fractal.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/fractal/fractal.c) · [`fractal.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/fractal/fractal.yaml)

| Native | File |
|--------|------|
| C | [`ta_FRACTAL.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_FRACTAL.c) |
| Rust | [`fractal.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/fractal.rs) |
| Java | [`Core_FRACTAL.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/fragments/Core_FRACTAL.java) |

TA-Lib is also available for Python, R and more using a [wrapper](/install/#wrappers).

## Aliases

Williams Fractal, Fractals, Swing High, Swing Low, Pivot High, Pivot Low

## See Also

[AROON](/functions/aroon.md) · [MAXINDEX](/functions/maxindex.md) · [MININDEX](/functions/minindex.md) · [MINMAXINDEX](/functions/minmaxindex.md)

## References

- Bill Williams, *Trading Chaos*, John Wiley & Sons (1995)
- [TradingView: Williams Fractal](https://www.tradingview.com/support/solutions/43000591663-williams-fractal/)
