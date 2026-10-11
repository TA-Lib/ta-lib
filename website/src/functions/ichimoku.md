---
title: "Ichimoku Kinko Hyo (ICHIMOKU)"
description: "Ichimoku Kinko Hyo, \"one glance equilibrium chart\": four lines built from highs and lows alone."
---

## Summary

Ichimoku Kinko Hyo, "one glance equilibrium chart": four lines built from highs and lows alone. Goichi Hosoda's reading is that a market's balance is visible without any smoothing — each line is the midpoint of a window, the mean of its highest high and its lowest low, so it marks the level at which that stretch of trading was evenly divided. The conversion line turns fastest, the base line is the reference, and the two leading spans are drawn ahead of price, where the band between them is read as support or resistance before it is reached.

## Formula

MID(n) = ( highest high over n bars + lowest low over n bars ) / 2

Tenkan-sen = MID(tenkanPeriod)

Kijun-sen = MID(kijunPeriod)

Senkou Span A = ( Tenkan-sen + Kijun-sen ) / 2

Senkou Span B = MID(senkouBPeriod)

## Notes

- Each line is `TA_MIDPRICE` over its own period, and Span A is `TA_MEDPRICE` of the other two lines. Span A halves the two midpoints after each has been rounded, rather than averaging the four extremes. The two spellings are the same number in real arithmetic and a different double in the last bit often enough to matter: on the 252-bar regression corpus the rate is 0% at the published 9/26 periods, 5.7% at 26/9, and 12.4% at 2/2, and on a four-decimal series at 3/5 it is 48%. The rate is not a function of the longer period alone: 9/26 and 26/9 share a 26-bar window and read 0% and 5.7%.
- The two spans are drawn `kijunPeriod` bars ahead of the bar that computed them. That is a display shift: it is reported through the display-shift call and changes nothing about the values, the lookback or the returned range. Every output is written at the bar that computed it.
- The lookback is the longest of the three periods less one. It is not the Senkou B period: nothing orders the three, so a base line longer than the second span dominates.
- The Chikou span, the close drawn backward, carries no computation and is not an output here: it is the input series with a display shift.

## Inputs

- `inHigh` — High price series
- `inLow` — Low price series

## Outputs

- `outTenkanSen` — Conversion line
- `outKijunSen` — Base line
- `outSenkouSpanA` — First leading span, drawn ahead by the base period
- `outSenkouSpanB` — Second leading span, drawn ahead by the base period

## Parameters

| Parameter | Type | Default | Accepted values | Description |
| --- | --- | --- | --- | --- |
| `optInTenkanPeriod` | integer | 9 | 2–100000 | Period of the conversion line |
| `optInKijunPeriod` | integer | 26 | 2–100000 | Period of the base line, and the forward shift of the two spans |
| `optInSenkouBPeriod` | integer | 52 | 2–100000 | Period of the second leading span |

## Properties

**Numerical Stability:** [Start-Independent](/functions/stability.md#start-independent)

<div class="flag-table">

|  |
| :-- |
| <span class="flag-box">✅</span> **Overlap Input** <span class="flag-tip" tabindex="0" role="note" aria-label="Output is on the same scale as the input price, so it is drawn over the price chart." data-tip="Output is on the same scale as the input price, so it is drawn over the price chart.">i</span> |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Independent Y-Axis</span> |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Candlestick</span> |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Can Output NaN or ±Inf</span> |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Identity at Period 1</span> |
| <span class="flag-box">✅</span> **Display Shift** <span class="flag-tip" tabindex="0" role="note" aria-label="A chart draws at least one output ahead of or behind the bar that computed it. The display-shift query gives the number of bars; the values are not shifted." data-tip="A chart draws at least one output ahead of or behind the bar that computed it. The display-shift query gives the number of bars; the values are not shifted.">i</span> |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Uses Transcendental</span> |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Input Must Be Positive</span> |

</div>

## Implementation

TA-Lib Definition: [`ichimoku.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/ichimoku/ichimoku.c) · [`ichimoku.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/ichimoku/ichimoku.yaml)

| Native | File |
|--------|------|
| C | [`ta_ICHIMOKU.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_ICHIMOKU.c) |
| Rust | [`ichimoku.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/ichimoku.rs) |
| Java | [`Core_ICHIMOKU.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/fragments/Core_ICHIMOKU.java) |
| C# | [`Core_ICHIMOKU.cs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/csharp/library/src/Core_ICHIMOKU.cs) |

TA-Lib is also available for Python, R and more using a [wrapper](/install/#wrappers).

## Aliases

ichimoku, ichimoku kinko hyo, ichimoku cloud, kumo

## See Also

[MIDPRICE](/functions/midprice.md) · [MEDPRICE](/functions/medprice.md) · [SAR](/functions/sar.md) · [BBANDS](/functions/bbands.md)

## References

- Goichi Hosoda, *Ichimoku Kinko Hyo*, 1969
