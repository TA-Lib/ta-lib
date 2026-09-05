---
title: "Percent Rank (PERCENTRANK)"
description: "Percent Rank: where the current value sits inside the distribution of the values that came before it, as a percentage."
---

## Summary

Percent Rank: where the current value sits inside the distribution of the values that came before it, as a percentage.

Each bar is compared against the `optInTimePeriod` values immediately preceding it — the current bar is not part of its own comparison set — and the output is the share of them it exceeds. 0 means the current value is at or below every one of them, 100 means it is above all of them, 50 means it is above half.

It is a distribution-relative reading rather than a price-relative one: unlike an oscillator built from ranges or averages, it says nothing about how far the value moved, only how many of its recent predecessors it overtook. That makes it flat-scaled across instruments and directly comparable between them. It is best known as the third leg of Connors' ConnorsRSI, applied there to one-day returns rather than to price.

## Formula

For each bar t, over the previous optInTimePeriod values:

count = number of j in [t-optInTimePeriod, t-1] with inReal[j] < inReal[t]

PERCENTRANK[t] = ( count / optInTimePeriod ) * 100

The comparison is strictly less-than, so a value tied with a predecessor does not count that predecessor.

## Notes

- Ties are strict: a predecessor equal to the current value is not counted. A constant series therefore reports 0 on every bar. TradingView's `ta.percentrank` counts ties as well (less-than-or-equal), which reports 100 on that same series; Pine parity is a different function, not a variant of this one, and the two agree only on windows with no repeated values.
- +0.0 and -0.0 compare equal, so a sign-only difference never contributes to the count.
- Finite input is a precondition. A NaN in the window fails every comparison, so it silently lowers the rank instead of propagating: the output stays finite and no error is reported.

## Inputs

- `inReal` — Source price/value series

## Outputs

- `outReal` — Percentage of the preceding window strictly below the current value, 0 to 100

## Parameters

| Parameter | Type | Default | Accepted values | Description |
| --- | --- | --- | --- | --- |
| `optInTimePeriod` | integer | 100 | 2–100000 | Number of preceding values the current value is ranked against |

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

TA-Lib Definition: [`percentrank.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/percentrank/percentrank.c) · [`percentrank.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/percentrank/percentrank.yaml)

| Native | File |
|--------|------|
| C | [`ta_PERCENTRANK.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_PERCENTRANK.c) |
| Rust | [`percentrank.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/percentrank.rs) |
| Java | [`Core_PERCENTRANK.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/fragments/Core_PERCENTRANK.java) |

TA-Lib is also available for Python, R and more using a [wrapper](/install/#wrappers).

## See Also

[RSI](/functions/rsi.md) · [WILLR](/functions/willr.md) · [STDDEV](/functions/stddev.md)

## References

- [Connors Research, *An Introduction to ConnorsRSI* (2012), p. 8](https://www.qmatix.com/ConnorsRSI-Pullbacks-Guidebook.pdf)
- [ta4j `PercentRankIndicator`](https://github.com/ta4j/ta4j)
