---
title: "True Range (TRANGE)"
description: "True Range: the greatest of today's high-low span and the two gaps between yesterday's close and today's high/low."
---

## Summary

True Range: the greatest of today's high-low span and the two gaps between yesterday's close and today's high/low. Base volatility measure used to build ATR/NATR. Larger values mean wider or gappier bars (higher volatility).

## Chart

<ta-chart src="/functions/trange.svg" ratio="720:416" alt="Drawn on the price pane over candles; below it, a separate indicator pane with a zero line showing outReal as a solid line."></ta-chart>

Drawn on the price pane over candles; below it, a separate indicator pane with a zero line showing outReal as a solid line. [Reference data (CSV)](/functions/trange.csv) · [Chart (SVG)](/functions/trange.svg)

## Formula

TR = max( high - low, |prevClose - high|, |prevClose - low| )

## Notes

- The first bar produces no value because it has no prior close; unlike some definitions, it does not fall back to the high-low range for that bar.

## Inputs

- `inHigh` — High price of each bar
- `inLow` — Low price of each bar
- `inClose` — Close price of each bar

## Outputs

- `outReal` — True Range value per bar

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
| <span class="flag-box">☐</span> <span style="opacity:0.5">Input Must Be Positive</span> |

</div>

## Implementation

Added in 0.4.0.

TA-Lib Definition: [`trange.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/trange/trange.c) · [`trange.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/trange/trange.yaml)

| Native | File |
|--------|------|
| C | [`ta_TRANGE.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_TRANGE.c) |
| Rust | [`trange.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/trange.rs) |
| Java | [`Core_TRANGE.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/fragments/Core_TRANGE.java) |
| C# | [`Core_TRANGE.cs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/csharp/library/src/Core_TRANGE.cs) |
| TypeScript | [`trange.ts`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/ts/library/src/func/trange.ts) |

Python: `talib.TRANGE`, see [ta-lib-python](https://ta-lib.github.io/ta-lib-python/func_groups/volatility_indicators.html).

TA-Lib is also available for R and more using a [wrapper](/install/#wrappers).

## Aliases

True Range, TR

## See Also

[ATR](/functions/atr.md) · [NATR](/functions/natr.md)

## References

- J. Welles Wilder, *New Concepts in Technical Trading Systems*, Trend Research (ISBN 0894590278)
