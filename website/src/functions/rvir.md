---
title: "Relative Volatility Index, refined high/low form (RVIR)"
description: "Relative Volatility Index, refined form: Donald Dorsey's 1995 revision of his own indicator, which runs the 1993 RVI over the daily highs and again over…"
---

## Summary

Relative Volatility Index, refined form: Donald Dorsey's 1995 revision of his own indicator, which runs the 1993 RVI over the daily highs and again over the daily lows and averages the two indices. Each leg is the shipped [`RVI`](/functions/rvi.md) unchanged — the same rolling standard deviation routed to an up or a down bucket by the direction of the bar, the same Wilder smoothing, the same treatment of a tie.

Bounded in 0..100 and read like the close-only form: above 50 the recent volatility arrived mostly on up bars, below 50 mostly on down bars. Dorsey's stated reason for the revision is that a high and a low carry the day's range, so the pair answers the question the close alone can only approximate.

## Formula

With `RVI(x)` the close-only index computed over the series `x`:

    RVIR[i] = 0.5 * ( RVI(high)[i] + RVI(low)[i] )

Both legs use the same `optInTimePeriod` and `optInStdDevPeriod`, so they warm on the same bar and the lookback is `RVI`'s.

## Notes

- The name is contested, and in the opposite direction from what the abbreviation suggests. Some vendors reserve the bare name RVI for *this* revision and call the 1993 close-only form RVIorig; others default the other way. This library ships the 1993 form as [`RVI`](/functions/rvi.md) and the 1995 revision here.
- The two legs are computed in one pass rather than by calling `RVI` twice, but the arithmetic of each leg is `RVI`'s in `RVI`'s order. The result is the average of two `RVI` calls bit for bit, which is what the regression test asserts.
- A bar whose high equals the previous high feeds neither bucket of the high leg, and likewise for the lows. Descriptions that write a leg's denominator as a smoothed deviation instead of `U + D` are counting ties as down bars, which is a different indicator: on a 252-bar equity series that flip moves this function by up to 4.6 index points.
- Each leg reports 50 when its own smoothed legs are both exactly zero, for the reason `RVI` does. The average is taken after each leg has resolved that, so a tie in one series does not drag the other.
- On a series whose high equals its low at every bar the two legs are the same computation, and this function returns exactly `RVI` of it.
- Sources publishing something else under this name, and how far from this function they land on a 252-bar equity series: a plain exponential smoother instead of Wilder's, up to 12.9 index points; averaging the *prices* and taking one index of the result instead of averaging the two indices, up to 13.6; adding the close as a third leg, up to 6.0; a 9-period deviation, up to 3.2. These are different indicators, not errors.
- This is not a re-parameterisation of `RVI`: against `RVI` of the closes at the shared defaults the two series differ by up to 18.1 index points on the same corpus.

## Inputs

- `inHigh` — High price of each bar
- `inLow` — Low price of each bar

## Outputs

- `outReal` — The averaged index, in 0..100

## Parameters

| Parameter | Type | Default | Accepted values | Description |
| --- | --- | --- | --- | --- |
| `optInTimePeriod` | integer | 14 | 1–100000 | Wilder smoothing period applied to both legs of both indices |
| `optInStdDevPeriod` | integer | 10 | 2–100000 | Number of trailing values each standard deviation spans |

## See Also

[RVI](/functions/rvi.md) · [STDDEV](/functions/stddev.md) · [ATR](/functions/atr.md)

## References

- Dorsey, Donald. "Refining the Relative Volatility Index." *Technical Analysis of Stocks & Commodities*, V.13:9 (September 1995), 388-391.
- Dorsey, Donald. "The Relative Volatility Index." *Technical Analysis of Stocks & Commodities*, V.11:6 (June 1993), 253-256.

## Properties

**Numerical Stability:** [Initial Unstable Period](/functions/stability.md#initial-unstable-period) — Inherited from RVI, which RVIR computes internally; tunable via RVI's unstable period.

<div class="flag-table">

|  |
| :-- |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Overlap Input</span> |
| <span class="flag-box">✅</span> **Independent Y-Axis** <span class="flag-tip" tabindex="0" role="note" aria-label="Output is on its own scale, drawn in a separate pane below the price chart." data-tip="Output is on its own scale, drawn in a separate pane below the price chart.">i</span> |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Candlestick</span> |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Can Output NaN or ±Inf</span> |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Identity at Period 1</span> |

</div>

