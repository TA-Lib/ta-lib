# SKEW

## Summary

An estimate, from the trailing window, of the skewness of the distribution its values come from: the third standardised moment. An asymmetry measure: above 0 the distribution has a longer tail above its mean, below 0 a longer tail below it, and a symmetric window reads 0. The companion to [`KURTOSIS`](/functions/kurtosis), [`VAR`](/functions/var) and [`STDDEV`](/functions/stddev) in the same group.

## Formula

Adjusted Fisher-Pearson coefficient of skewness, the estimator usually written `G1`. With window size `n`, window mean `x̄` and population moments `mₖ = Σ(x−x̄)ᵏ / n`:

    g1 = m₃ / m₂^(3/2)

    G1 = g1 · √(n(n−1)) / (n−2)

The result is 0 when `m₂ = 0`, that is when every value in the window is equal: a point mass is symmetric.

`optInTimePeriod` must be at least 3: the `n−2` denominator is undefined below it. The bound is enforced by the parameter range, so a shorter period is refused rather than silently degraded.

## Notes

- This is `G1`, the form Excel `SKEW`, `scipy.stats.skew(bias=False)` and R `e1071::skewness(type = 2)` compute. Two other estimators are in circulation, each a constant multiple of `g1` rather than a rounding convention: `g1` itself (`scipy.stats.skew(bias=True)`) and `b1 = g1 · ((n−1)/n)^(3/2)` (`type = 3`).
- One reading apart from `n−1` equal ones gives exactly `√n`, above the rest when the odd one is above and `−√n` when it is below. That is the largest magnitude a window can reach, so a window dominated by one outlier is legitimately far from 0.

## Inputs

- `inReal` — The series to measure

## Outputs

- `outReal` — Skewness of the trailing window, or 0 where the window has no spread

## Parameters

- `optInTimePeriod` — Number of trailing values in the window

## Aliases

Skewness, Sample Skewness, Fisher-Pearson Skewness

## See Also

KURTOSIS · VAR · STDDEV

## References

- Joanes, D. N. and Gill, C. A. "Comparing measures of sample skewness and kurtosis." *Journal of the Royal Statistical Society: Series D*, 47(1), 1998, 183-189 — the `g1`, `G1`, `b1` family and which is unbiased under which assumption.
- NIST/SEMATECH *e-Handbook of Statistical Methods*, §1.3.5.11 — the Fisher-Pearson coefficient and its adjusted form.
- Microsoft, *SKEW function* — the `G1` form.
