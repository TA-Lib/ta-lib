---
title: "Variable Index Dynamic Average (VIDYA)"
description: "Variable Index Dynamic Average (Tushar Chande): an EMA whose smoothing factor is scaled every bar by the absolute value of the Chande Momentum Oscillator."
---

## Summary

Variable Index Dynamic Average (Tushar Chande): an EMA whose smoothing factor is scaled every bar by the absolute value of the Chande Momentum Oscillator. It follows the price like an EMA in a one-way move and stops moving when up and down moves balance, so it flattens out in consolidations.

## Formula

n = optInTimePeriod, m = optInCMOPeriod, alpha = 2 / (n + 1)

CMO[t] = the unsmoothed Chande Momentum Oscillator over the last m price changes (`CMOU`)

k[t] = alpha * |CMO[t]| / 100

VIDYA[t] = VIDYA[t-1] + k[t] * (x[t] - VIDYA[t-1])

The recursion starts from the first price. Until m price changes are available, the CMO is taken over the changes seen so far. The first output is at bar m.

## Notes

- A period of 1 performs no smoothing: the output is a copy of the input, consistent with `MA(period=1)` for every MAType.
- Chande's 1992 article drives the same step with a ratio of standard deviations; this is his 1995 form, driven by the CMO.
- The CMO is Chande's unsmoothed one. An implementation driven by a Wilder-smoothed CMO (`CMO`) computes a different line that does not converge to this one.
- Being recursive, an output depends on how much history precedes it, and the seed's influence decays more slowly the closer the CMO stays to 0. Implementations that seed differently agree with this one only once that influence has decayed.
- As an `MA` type, the one period is n and the CMO period is (3n + 2) / 4 in integer division, Chande's 12:9 ratio.

## Inputs

- `inReal` — Data on which to compute the average

## Outputs

- `outReal` — Variable Index Dynamic Average line

## Parameters

| Parameter | Type | Default | Accepted values | Description |
| --- | --- | --- | --- | --- |
| `optInTimePeriod` | integer | 12 | 1–100000 | The EMA length whose alpha the CMO scales |
| `optInCMOPeriod` | integer | 9 | 2–100000 | Number of trailing price changes in the CMO |

## Properties

**Numerical Stability:** [Initial Unstable Period](/functions/stability.md#initial-unstable-period)

<div class="flag-table">

|  |
| :-- |
| <span class="flag-box">✅</span> **Overlap Input** <span class="flag-tip" tabindex="0" role="note" aria-label="Output is on the same scale as the input price, so it is drawn over the price chart." data-tip="Output is on the same scale as the input price, so it is drawn over the price chart.">i</span> |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Independent Y-Axis</span> |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Candlestick</span> |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Can Output NaN or ±Inf</span> |
| <span class="flag-box">✅</span> **Identity at Period 1** <span class="flag-tip" tabindex="0" role="note" aria-label="A period of 1 performs no smoothing: the lookback is 0 and every output value is a bit-exact copy of its input value." data-tip="A period of 1 performs no smoothing: the lookback is 0 and every output value is a bit-exact copy of its input value.">i</span> |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Display Shift</span> |

</div>

## Implementation

TA-Lib Definition: [`vidya.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/vidya/vidya.c) · [`vidya.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/vidya/vidya.yaml)

| Native | File |
|--------|------|
| C | [`ta_VIDYA.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_VIDYA.c) |
| Rust | [`vidya.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/vidya.rs) |
| Java | [`Core_VIDYA.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/fragments/Core_VIDYA.java) |
| C# | [`Core_VIDYA.cs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/csharp/library/src/Core_VIDYA.cs) |

TA-Lib is also available for Python, R and more using a [wrapper](/install/#wrappers).

## Aliases

Variable Index Dynamic Average, VIDyA, Chande's Variable Index Dynamic Average

## See Also

[CMOU](/functions/cmou.md) · [EMA](/functions/ema.md) · [KAMA](/functions/kama.md) · [MA](/functions/ma.md)

## References

- Tushar S. Chande, "Adapting Moving Averages To Market Volatility", *Technical Analysis of Stocks & Commodities* V.10:3 (March 1992). The adaptive EMA and its volatility index.
- Tushar S. Chande, "Identifying Powerful Breakouts Early", *Technical Analysis of Stocks & Commodities* V.13:10 (October 1995). The CMO as the volatility index, with the 12 and 9 periods.
