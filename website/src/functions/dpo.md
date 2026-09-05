---
title: "Detrended Price Oscillator (DPO)"
description: "Detrended Price Oscillator: the price a half-cycle back, less the moving average that spans the cycle."
---

## Summary

Detrended Price Oscillator: the price a half-cycle back, less the moving average that spans the cycle. Removing the average removes the trend, leaving the shorter oscillation that the trend was hiding.

It crosses zero as price crosses its own average, so peaks and troughs mark the cycle rather than the direction of the market. The distance between successive peaks estimates the cycle length, and the amplitude is in price units, so it is comparable across time only for one instrument.

## Formula

Let `t = optInTimePeriod / 2 + 1`, an integer division, so a period and its odd successor share the same displacement.

    DPO[i] = P[i - t] - SMA(P, optInTimePeriod)[i]

## Notes

- The value is emitted at the bar whose moving average produced it. Charting packages usually draw it `t` bars to the left instead, which is a plotting convention rather than a different series; a caller wanting that view shifts `outReal` itself.
- A causal variant, `P[i] - SMA(P, optInTimePeriod)[i - t]`, displaces the average instead of the price. It is a genuinely different series, not a re-indexing of this one, and is not implemented here.

## Inputs

- `inReal` — Source price/value series, canonically the close

## Outputs

- `outReal` — Detrended Price Oscillator value, in the units of the input

## Parameters

| Parameter | Type | Default | Accepted values | Description |
| --- | --- | --- | --- | --- |
| `optInTimePeriod` | integer | 20 | 2–100000 | Number of bars spanned by the moving average being removed; the displacement is derived from it |

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

TA-Lib Definition: [`dpo.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/dpo/dpo.c) · [`dpo.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/dpo/dpo.yaml)

| Native | File |
|--------|------|
| C | [`ta_DPO.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_DPO.c) |
| Rust | [`dpo.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/dpo.rs) |
| Java | [`Core_DPO.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/fragments/Core_DPO.java) |

TA-Lib is also available for Python, R and more using a [wrapper](/install/#wrappers).

## Aliases

Detrended Price Oscillator

## See Also

[SMA](/functions/sma.md) · [MOM](/functions/mom.md) · [APO](/functions/apo.md)

## References

- Steven B. Achelis, *Technical Analysis from A to Z*, McGraw-Hill (p. 119)
- [StockCharts ChartSchool: Detrended Price Oscillator](https://chartschool.stockcharts.com/table-of-contents/technical-indicators-and-overlays/technical-indicators/detrended-price-oscillator-dpo)
