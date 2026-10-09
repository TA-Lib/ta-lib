---
title: "Rogers-Satchell Volatility (ROGERSSATCHELL)"
description: "Rogers-Satchell volatility: a range-based estimator that reads one bar's open, high, low and close as a single unbiased estimate of that bar's variance…"
---

## Summary

Rogers-Satchell volatility: a range-based estimator that reads one bar's open, high, low and close as a single unbiased estimate of that bar's variance, then reports the root of the mean over the last `n` bars, scaled to periods per year.

What separates it from the other range estimators is that it is unbiased **whatever the drift**. A bar that opens at its low and closes at its high has travelled in one direction and dispersed nothing around that path, and this estimator reads it as exactly zero, where Parkinson and Garman-Klass read a wide range as volatility. The price of that is a blind spot of its own: the estimator has no close-to-open term, so overnight gaps are invisible to it.

Read the output as a fraction in log-return units, not price units and not percent. At an `optInAnnualization` of 252 it is an annualised figure for daily bars; pass 1 to leave the per-bar figure, 52 for weekly bars, 12 for monthly.

## Formula

```
S1[i] = ln(High[i]/Open[i])     I1[i] = ln(Low[i]/Open[i])     X1[i] = ln(Close[i]/Open[i])

term[i] = S1[i] * (S1[i] - X1[i]) + I1[i] * (I1[i] - X1[i])
        = ln(High[i]/Close[i]) * ln(High[i]/Open[i])
        + ln(Low[i]/Close[i])  * ln(Low[i]/Open[i])

S[i]    = term[i-n+1] + ... + term[i]

out[i]  = sqrt(A) * sqrt(S[i] / n)      when S[i] > 0
        = 0                             otherwise
```

with `n` = `optInTimePeriod` and `A` = `optInAnnualization`.

## Notes

The estimator is defined on a consistent bar, one whose low is at or below both the open and the close and whose high is at or above both. Such a bar always gives a term at or above zero. A bar whose high or low sits strictly between its open and close can give a negative term; the window sum is then floored at zero rather than rooted, as VAR floors its variance so STDDEV can root it unconditionally.

A bar carrying a price at or below zero contributes a zero term and still counts toward the window's `n`. The test is exact rather than against a fixed band, so an instrument quoted in small units is not zeroed out by the threshold.

The window sum is carried from bar to bar and rebuilt as a fresh sum whenever it collapses relative to the largest it has held, and periodically otherwise. A window of flat bars therefore reads exactly zero; a sum that is only ever added to and subtracted from would leave the rounding of earlier bars behind instead.

`optInAnnualization` is applied as `sqrt(A)` computed once and multiplied in at the end, so the annualised output is exactly `sqrt(A)` times the per-bar one, and `A = 1` is an identity rather than a rounding.

Every term reads only its own bar, so the lookback is `n - 1`: the first output lands on bar `n - 1`. An estimator that measures the bar against the previous close consumes one more bar and starts at `n`.

## Inputs

- `inOpen` — Open price of each bar
- `inHigh` — High price of each bar
- `inLow` — Low price of each bar
- `inClose` — Close price of each bar

## Outputs

- `outReal` — Estimated volatility, in log-return units

## Parameters

| Parameter | Type | Default | Accepted values | Description |
| --- | --- | --- | --- | --- |
| `optInTimePeriod` | integer | 10 | 1–100000 | Number of bars in the window. `n = 1` is the paper's own single-bar estimator. |
| `optInAnnualization` | real | 252 | ≥ 0 | Periods per year. Pass 1 for the per-bar figure. |

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
| <span class="flag-box">✅</span> **Uses Transcendental** <span class="flag-tip" tabindex="0" role="note" aria-label="Calls a transcendental math function (exp, log, log10, trigonometric, inverse trigonometric or hyperbolic), so a value may differ slightly between languages, between a stream and a batch call, and between machines. TA-Lib targets a difference below 1e-9." data-tip="Calls a transcendental math function (exp, log, log10, trigonometric, inverse trigonometric or hyperbolic), so a value may differ slightly between languages, between a stream and a batch call, and between machines. TA-Lib targets a difference below 1e-9.">i</span> |

</div>

## Implementation

TA-Lib Definition: [`rogerssatchell.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/rogerssatchell/rogerssatchell.c) · [`rogerssatchell.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/rogerssatchell/rogerssatchell.yaml)

| Native | File |
|--------|------|
| C | [`ta_ROGERSSATCHELL.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_ROGERSSATCHELL.c) |
| Rust | [`rogerssatchell.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/rogerssatchell.rs) |
| Java | [`Core_ROGERSSATCHELL.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/fragments/Core_ROGERSSATCHELL.java) |
| C# | [`Core_ROGERSSATCHELL.cs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/csharp/library/src/Core_ROGERSSATCHELL.cs) |

TA-Lib is also available for Python, R and more using a [wrapper](/install/#wrappers).

## Aliases

Rogers-Satchell Volatility, Rogers Satchell, RSV

## See Also

[ATR](/functions/atr.md) · [NATR](/functions/natr.md) · [VAR](/functions/var.md)

## References

- Rogers, L. C. G. and Satchell, S. E. "Estimating Variance From High, Low and Closing Prices." *The Annals of Applied Probability* 1(4):504-512, November 1991. doi:10.1214/aoap/1177005835
