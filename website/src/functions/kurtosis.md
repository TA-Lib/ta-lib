---
title: "Rolling Excess Kurtosis (KURTOSIS)"
description: "An estimate, from the trailing window, of the excess kurtosis of the distribution its values come from: the fourth standardised moment minus 3, so a…"
---

## Summary

An estimate, from the trailing window, of the excess kurtosis of the distribution its values come from: the fourth standardised moment minus 3, so a normal distribution reads 0. A tail-weight measure: above 0 the distribution has heavier tails than a normal one with the same variance, below 0 lighter ones. The companion to the shipped [`VAR`](/functions/var.md) and [`STDDEV`](/functions/stddev.md) in the same group.

## Formula

Sample-adjusted Fisher excess kurtosis, the estimator usually written `G2`. With window size `n`, window mean `x̄`, and sample variance `s² = Σ(x−x̄)²/(n−1)`:

    G2 = [ n(n+1) / ((n−1)(n−2)(n−3)) ] · [ Σ(x−x̄)⁴ / s⁴ ]  −  3(n−1)² / ((n−2)(n−3))

The result is NaN when `s² = 0`, that is when every value in the window is equal.

`optInTimePeriod` must be at least 4: the `(n−2)(n−3)` denominators are undefined below it. The bound is enforced by the parameter range, so a shorter period is refused rather than silently degraded.

## Notes

- This is `G2`, the form Excel `KURT` and `scipy.stats.kurtosis(bias=False)` compute. The other form in circulation is the biased `g2 = m₄/m₂² − 3` over population moments, a different estimator rather than a rounding convention: `G2 = (n−1)((n+1)·g2 + 6) / ((n−2)(n−3))`.
- A point mass has no defensible excess kurtosis, so the degenerate window is NaN rather than a number: `0` would assert normality and `−1.2` uniformity. This is deliberately unlike `VAR`, which floors to zero — a variance of zero says something true about the window.
- The result is at most `n`, the value of one reading apart from `n−1` equal ones, so a window dominated by one outlier is legitimately far above 0.

## Inputs

- `inReal` — The series to measure

## Outputs

- `outReal` — Excess kurtosis of the trailing window, or NaN where the window has no spread

## Parameters

| Parameter | Type | Default | Accepted values | Description |
| --- | --- | --- | --- | --- |
| `optInTimePeriod` | integer | 30 | 4–100000 | Number of trailing values in the window, at least 4 |

## Properties

**Numerical Stability:** [Start-Independent](/functions/stability.md#start-independent)

<div class="flag-table">

|  |
| :-- |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Overlap Input</span> |
| <span class="flag-box">✅</span> **Independent Y-Axis** <span class="flag-tip" tabindex="0" role="note" aria-label="Output is on its own scale, drawn in a separate pane below the price chart." data-tip="Output is on its own scale, drawn in a separate pane below the price chart.">i</span> |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Candlestick</span> |
| <span class="flag-box">✅</span> **Can Output NaN or ±Inf** <span class="flag-tip" tabindex="0" role="note" aria-label="Some inputs have no finite result, so a successful call can return NaN or ±Inf — a gap with nothing to plot. See Notes for when." data-tip="Some inputs have no finite result, so a successful call can return NaN or ±Inf — a gap with nothing to plot. See Notes for when.">i</span> |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Identity at Period 1</span> |

</div>

## Implementation

TA-Lib Definition: [`kurtosis.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/kurtosis/kurtosis.c) · [`kurtosis.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/kurtosis/kurtosis.yaml)

| Native | File |
|--------|------|
| C | [`ta_KURTOSIS.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_KURTOSIS.c) |
| Rust | [`kurtosis.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/kurtosis.rs) |
| Java | [`Core_KURTOSIS.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/fragments/Core_KURTOSIS.java) |

TA-Lib is also available for Python, R and more using a [wrapper](/install/#wrappers).

## Aliases

Excess Kurtosis, Sample Excess Kurtosis, KURT

## See Also

[VAR](/functions/var.md) · [STDDEV](/functions/stddev.md) · [BBANDS](/functions/bbands.md)

## References

- Joanes, D. N. and Gill, C. A. "Comparing measures of sample skewness and kurtosis." *Journal of the Royal Statistical Society: Series D*, 47(1), 1998, 183-189 — the `g1/g2`, `G1/G2`, `b1/b2` families and which is unbiased under which assumption.
- NIST/SEMATECH *e-Handbook of Statistical Methods*, §1.3.5.11 — the biased population form.
- Microsoft, *KURT function* — the `G2` form, and `#DIV/0!` for fewer than four points or zero standard deviation.
