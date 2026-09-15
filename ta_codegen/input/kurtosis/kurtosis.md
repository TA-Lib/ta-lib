# KURTOSIS

## Summary

The fourth standardised moment of the trailing window, minus 3 so a normal window reads 0. A tail-weight measure: above 0 the window has fatter tails and a sharper peak than a normal distribution of the same variance, below 0 it is flatter. The companion to the shipped [`VAR`](/functions/var) and [`STDDEV`](/functions/stddev) in the same group.

`SKEW`, the third-moment sibling, is deliberately a separate question — it carries its own convention fork and its own references, and two functions that share only a word get independent docs and decisions.

## Formula

Sample-adjusted Fisher excess kurtosis, the estimator usually written `G2`. With window size `n`, window mean `x̄`, and sample variance `s² = Σ(x−x̄)²/(n−1)`:

    G2 = [ n(n+1) / ((n−1)(n−2)(n−3)) ] · [ Σ(x−x̄)⁴ / s⁴ ]  −  3(n−1)² / ((n−2)(n−3))

`optInTimePeriod` must be at least 4: the `(n−2)(n−3)` denominators are undefined below it. The bound is enforced by the parameter range, so a shorter period is refused rather than silently degraded.

## Notes

- **Which estimator.** This is `G2`, what Excel `KURT`, `scipy.stats.kurtosis(bias=False)` and R `e1071::kurtosis(type=2)` all compute, and the one that is unbiased under normality. The other form in circulation is the biased `g2 = m₄/m₂² − 3` over population moments, which NIST/SEMATECH gives. They are not a rounding convention apart: on a 9-point normal sample `G2 = 1.79450407519901` against `g2 = 0.342114639479481`.
- **A window with no spread returns NaN, not a number.** Excess kurtosis has no defensible neutral to fall back on — `0` asserts normality and `−1.2` asserts uniformity, and a point mass supports neither. `scipy.stats.kurtosis` returns `nan` at both bias settings, and Excel `KURT` documents `#DIV/0!` when the sample's standard deviation is zero. This function declares `nan_inf_output` and leaves the division unguarded, so the answer arrives from IEEE rather than from a branch. It is deliberately a *different* answer from `VAR`'s floor-to-zero: a variance of zero says something true about the window, a kurtosis of a point mass does not.
- **Cancellation, and why the rebuild period is not `VAR`'s.** Deviations are taken against a shift near the window and the central moments recovered from the shifted sums, as `var.c` does. What does not carry over is the period: a fourth moment recovered against a stale shift pays `(u/σ)⁴` where a second pays `(u/σ)²`, so the shift goes stale four times faster in the exponent. MEASURED on 1200-bar series at `n = 30`, worst relative error per bar against a 60-digit reference:

  | rebuild every | 32n (`VAR`'s) | 8n | 2n | n | n/4 |
  |---|---|---|---|---|---|
  | random walk around 100 | 1.18e-06 | 9.33e-07 | 1.61e-08 | 1.28e-09 | 4.58e-12 |
  | random walk on 3.1e10 | 4.99e-06 | 4.99e-06 | 6.27e-10 | 1.45e-09 | 1.76e-12 |
  | outlier 1e5 every 200 bars | 2.84e-13 | 2.84e-13 | 2.68e-13 | 1.51e-13 | 4.48e-14 |

  `VAR`'s `32n` leaves 1e-6 on an ordinary random walk, and the collapse trigger cannot catch it — the ratio it tests sits around 0.24 there, six orders from firing. Hence `n/4`.
- **Rebuilding beats rescanning.** Rescanning the whole window every bar measures 5.34e-11 and 4.30e-11 on those two walks — worse than the `n/4` rebuild, at roughly ten times the arithmetic. The rebuild anchors the shift at the window *mean*; a per-bar rescan can only anchor it at a window *value*, which sits further from centre.
- **The result is not bounded.** There is no clamp and no range assertion; a window dominated by one outlier is legitimately far above 0.

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
