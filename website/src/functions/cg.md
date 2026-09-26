---
title: "Center of Gravity Oscillator (CG)"
description: "John Ehlers' Center of Gravity oscillator: the balance point of the last optInTimePeriod values, each weighted by its value and placed at its position…"
---

## Summary

John Ehlers' Center of Gravity oscillator: the balance point of the last `optInTimePeriod` values, each weighted by its value and placed at its position counting back from the current bar, which is position 1, negated so that it rises with price. It is smooth and has essentially no lag. Ehlers reads turning points from it and trades its crossings with a copy of itself delayed by one bar. For a positive series it stays between -optInTimePeriod and -1, and a flat window sits at the midpoint, -(optInTimePeriod+1)/2.

## Formula

$$
\mathrm{CG}_t = -\frac{\sum_{i=0}^{n-1} (i+1)\,X_{t-i}}{\sum_{i=0}^{n-1} X_{t-i}}, \text{ or } -\frac{n+1}{2} \text{ when } \sum_{i=0}^{n-1} X_{t-i} = 0
$$

where $X$ is the input series, $n$ is `optInTimePeriod` and $i$ counts bars back from the current bar, which carries the smallest weight.

## Notes

- Ehlers' default input is the median price (H+L)/2: pass the output of MEDPRICE to reproduce it.
- His 2004 book presents the same oscillator shifted up by (optInTimePeriod+1)/2, so that a flat window reads 0: add (optInTimePeriod+1)/2 to the output to obtain that form.
- Where the window sums to exactly zero the author's listing keeps its previous value; TA-Lib returns -(optInTimePeriod+1)/2, so every value depends on its own window alone.

## Inputs

- `inReal` — The series to measure

## Outputs

- `outReal` — Negated center of gravity of the window

## Parameters

| Parameter | Type | Default | Accepted values | Description |
| --- | --- | --- | --- | --- |
| `optInTimePeriod` | integer | 10 | 2–100000 | Number of bars in the window |

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

TA-Lib Definition: [`cg.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/cg/cg.c) · [`cg.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/cg/cg.yaml)

| Native | File |
|--------|------|
| C | [`ta_CG.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_CG.c) |
| Rust | [`cg.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/cg.rs) |
| Java | [`Core_CG.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/fragments/Core_CG.java) |
| C# | [`Core_CG.cs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/csharp/library/src/Core_CG.cs) |

TA-Lib is also available for Python, R and more using a [wrapper](/install/#wrappers).

## Aliases

Center of Gravity Oscillator, COG

## See Also

[WMA](/functions/wma.md) · [MEDPRICE](/functions/medprice.md) · [CTI](/functions/cti.md)

## References

- John F. Ehlers, "The CG Oscillator", MESA Software, [author's PDF](https://mesasoftware.com/papers/TheCGOscillator.pdf); published as "The Center Of Gravity Oscillator", *Technical Analysis of Stocks & Commodities*, May 2002
- John F. Ehlers, *Cybernetic Analysis for Stocks and Futures*, Wiley, 2004, chapter 5
