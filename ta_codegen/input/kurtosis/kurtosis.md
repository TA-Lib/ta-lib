# KURTOSIS

## Summary

The fourth standardised moment of the trailing window, minus 3 so a normal window reads 0. A tail-weight measure: above 0 the window has fatter tails and a sharper peak than a normal distribution of the same variance, below 0 it is flatter. The companion to the shipped [`VAR`](/functions/var) and [`STDDEV`](/functions/stddev) in the same group.

## Formula

Sample-adjusted Fisher excess kurtosis, the estimator usually written `G2`. With window size `n`, window mean `x̄`, and sample variance `s² = Σ(x−x̄)²/(n−1)`:

    G2 = [ n(n+1) / ((n−1)(n−2)(n−3)) ] · [ Σ(x−x̄)⁴ / s⁴ ]  −  3(n−1)² / ((n−2)(n−3))

The result is NaN when `s² = 0`, that is when every value in the window is equal.

`optInTimePeriod` must be at least 4: the `(n−2)(n−3)` denominators are undefined below it. The bound is enforced by the parameter range, so a shorter period is refused rather than silently degraded.

## Notes

- This is `G2`, the form Excel `KURT`, `scipy.stats.kurtosis(bias=False)` and R `e1071::kurtosis(type=2)` compute. The other form in circulation is the biased `g2 = m₄/m₂² − 3` over population moments; the two are far apart, not a rounding convention apart — on a 9-point normal sample `G2 = 1.79450407519901` against `g2 = 0.342114639479481`.
- A point mass has no defensible excess kurtosis, so the degenerate window is NaN rather than a number: `0` would assert normality and `−1.2` uniformity. This is deliberately unlike `VAR`, which floors to zero — a variance of zero says something true about the window.
- The result is not bounded. A window dominated by one outlier is legitimately far above 0.

## Inputs

- `inReal` — The series to measure

## Outputs

- `outReal` — Excess kurtosis of the trailing window, or NaN where the window has no spread

## Parameters

- `optInTimePeriod` — Number of trailing values in the window, at least 4

## See Also

VAR · STDDEV · BBANDS

## References

- Joanes, D. N. and Gill, C. A. "Comparing measures of sample skewness and kurtosis." *Journal of the Royal Statistical Society: Series D*, 47(1), 1998, 183-189 — the `g1/g2`, `G1/G2`, `b1/b2` families and which is unbiased under which assumption.
- NIST/SEMATECH *e-Handbook of Statistical Methods*, §1.3.5.11 — the biased population form.
- Microsoft, *KURT function* — the `G2` form, and `#DIV/0!` for fewer than four points or zero standard deviation.
