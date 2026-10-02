---
url: 'https://ta-lib.org/functions/alma.md'
description: >-
  Arnaud Legoux Moving Average: the last N inputs weighted by a Gaussian whose
  peak sits a fraction offset of the way from the oldest to the newest bar.
---
# Arnaud Legoux Moving Average (ALMA)

## Summary

Arnaud Legoux Moving Average: the last N inputs weighted by a Gaussian whose peak sits a fraction `offset` of the way from the oldest to the newest bar. Moving the offset toward 1 puts the peak on recent bars and cuts the lag; moving it toward 0 puts the peak on old bars and lags most. Smoothing is greatest with the peak mid-window, near 0.5. Sigma sets the width: the Gaussian's standard deviation is `N / sigma` bars, so a larger sigma concentrates the weight around the peak and smooths less.

The weights are non-negative and sum to one, so the line stays within the range of its window, up to rounding. At the default shape and a period of 5 or more, it lags about half as much as an `SMA` of the same period and passes about twice its noise.

## Formula

N = optInTimePeriod, m = floor( offset \* (N-1) ), s = N / sigma

g\[j] = exp( -(j-m)^2 / (2 s^2) ),  j = 0 .. N-1, j = 0 the oldest bar of the window

ALMA\[t] = sum_j ( g\[j] / sum_k g\[k] ) \* x\[t-N+1+j]

The first output is at bar N-1.

## Notes

* The peak is floored to a whole bar, as in the authors' code. At periods below about 5 this puts it on an older bar: at period 2 the line is almost entirely the previous bar.
* `TA_MAType_ALMA` runs this function at the default sigma and offset, so the line in `MA` and every function taking an MAType has the lag and noise stated above.
* The published paper's summary formula uses a different parameterisation; this is the form of the authors' own implementation.

## Inputs

* `inReal` — Data on which to compute the average

## Outputs

* `outReal` — Arnaud Legoux Moving Average line

## Parameters

| Parameter | Type | Default | Accepted values | Description |
| --- | --- | --- | --- | --- |
| `optInTimePeriod` | integer | 9 | 1–100000 | Number of bars in the window |
| `optInSigma` | real | 6 | ≥ 0.01 | Divides the period to give the Gaussian's width in bars |
| `optInOffset` | real | 0.85 | 0–1 | Position of the peak weight, 0 at the oldest bar and 1 at the newest |

## Properties

**Numerical Stability:** [Start-Independent](/functions/stability.md#start-independent)

<div class="flag-table">

|  |
| :-- |
| <span class="flag-box">✅</span> **Overlap Input** <span class="flag-tip" tabindex="0" role="note" aria-label="Output is on the same scale as the input price, so it is drawn over the price chart." data-tip="Output is on the same scale as the input price, so it is drawn over the price chart.">i</span> |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Independent Y-Axis</span> |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Candlestick</span> |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Can Output NaN or ±Inf</span> |
| <span class="flag-box">✅</span> **Identity at Period 1** <span class="flag-tip" tabindex="0" role="note" aria-label="A period of 1 performs no smoothing: every output value is a bit-exact copy of its input value." data-tip="A period of 1 performs no smoothing: every output value is a bit-exact copy of its input value.">i</span> |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Display Shift</span> |

</div>

## Implementation

TA-Lib Definition: [`alma.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/alma/alma.c) · [`alma.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/alma/alma.yaml)

| Native | File |
|--------|------|
| C | [`ta_ALMA.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_ALMA.c) |
| Rust | [`alma.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/alma.rs) |
| Java | [`Core_ALMA.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/fragments/Core_ALMA.java) |
| C# | [`Core_ALMA.cs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/csharp/library/src/Core_ALMA.cs) |

TA-Lib is also available for Python, R and more using a [wrapper](/install/#wrappers).

## See Also

[WMA](/functions/wma.md) · [SMA](/functions/sma.md) · [TRIMA](/functions/trima.md) · [HMA](/functions/hma.md)

## References

* Arnaud Legoux and Dimitris Kouzis-Loukas, ["ALMA; In search for the perfect Moving Average"](https://web.archive.org/web/20110904091012/www.arnaudlegoux.com/wp-content/uploads/2011/03/ALMA-Arnaud-Legoux-Moving-Average.pdf), November 2009. The authors' NinjaTrader implementation (2010) fixes the operational form above.
