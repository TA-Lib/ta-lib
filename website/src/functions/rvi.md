---
title: "Relative Volatility Index (RVI)"
description: "Relative Volatility Index: Donald Dorsey's volatility oscillator, built exactly like RSI except that the quantity routed to the up and down buckets is…"
---

## Summary

Relative Volatility Index: Donald Dorsey's volatility oscillator, built exactly like RSI except that the quantity routed to the up and down buckets is the rolling standard deviation of price rather than the size of the move. The direction of the close-to-close change still decides which bucket a bar feeds.

Bounded in 0..100. High values mean the recent volatility arrived mostly on up bars, low values that it arrived mostly on down bars. Dorsey proposed it as a confirming filter rather than a stand-alone signal: take a long entry only while RVI is above 50, a short only while it is below.

## Formula

With `S` the standard deviation of the last `optInStdDevPeriod` values of `inReal`, and `C` the input series:

    U[i] = S[i] if C[i] > C[i-1], else 0
    D[i] = S[i] if C[i] < C[i-1], else 0
    RVI  = 100 * RMA(U, optInTimePeriod) / ( RMA(U, optInTimePeriod) + RMA(D, optInTimePeriod) )

`RMA` is Wilder's smoothed moving average, seeded with the simple average of its first `optInTimePeriod` inputs. A bar whose close equals the previous close feeds neither bucket.

## Notes

- This is Dorsey's 1993 original, which measures the closes alone. His 1995 revision averages the index of the highs with the index of the lows; some vendors reserve the name RVI for that revision and call this one RVIorig. It is not implemented here.
- A tie contributes to neither bucket, matching RSI's treatment of an unchanged close. Descriptions that write the denominator as a smoothed `S` instead of `U + D` are counting ties as down bars, which is a different indicator.
- Both smoothed legs can be exactly zero at the same bar, which happens whenever the smoothing carries no memory and the bar is a tie. RVI reports its neutral centre, 50, there rather than a non-finite value.
- The standard deviation is the population form. The sample form differs by a constant factor that cancels in the ratio, so it is not a variant.
- Sources publishing something else under this name, and how far from this function they land on a 252-bar equity series: a plain exponential smoother instead of Wilder's, up to 11.6 index points; one shared period for both the deviation and the smoothing, up to 15.6; an RSI taken over the standard-deviation series, up to 35.2; a linear-regression residual, up to 36.0. These are different indicators, not errors.
- Unrelated to the Relative Vigor Index, which several platforms also abbreviate RVI.

## Inputs

- `inReal` — Source price/value series, canonically the close

## Outputs

- `outReal` — Relative Volatility Index value

## Parameters

| Parameter | Type | Default | Accepted values | Description |
| --- | --- | --- | --- | --- |
| `optInTimePeriod` | integer | 14 | 1–100000 | Wilder smoothing period applied to both legs |
| `optInStdDevPeriod` | integer | 10 | 2–100000 | Number of trailing values the standard deviation spans |

## Properties

**Numerical Stability:** [Initial Unstable Period](/functions/stability.md#initial-unstable-period)

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

TA-Lib Definition: [`rvi.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/rvi/rvi.c) · [`rvi.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/rvi/rvi.yaml)

| Native | File |
|--------|------|
| C | [`ta_RVI.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_RVI.c) |
| Rust | [`rvi.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/rvi.rs) |
| Java | [`Core_RVI.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/fragments/Core_RVI.java) |

TA-Lib is also available for Python, R and more using a [wrapper](/install/#wrappers).

## Aliases

Relative Volatility Index, RVIorig

## See Also

[RSI](/functions/rsi.md) · [RMA](/functions/rma.md) · [STDDEV](/functions/stddev.md) · [CMO](/functions/cmo.md)

## References

- Donald Dorsey, "The Relative Volatility Index", *Technical Analysis of Stocks & Commodities*, V.11:6 (June 1993), 253-256
- Donald Dorsey, "Refining the Relative Volatility Index", *Technical Analysis of Stocks & Commodities*, V.13:9 (September 1995), 388-391
- [Chart manual: Relative Volatility Index](https://user42.tuxfamily.org/chart/manual/Relative-Volatility-Index.html)
- [DXcharts: Relative Volatility Index](https://devexperts.com/dxcharts/kb/docs/relative-volatility-index-rvi)
