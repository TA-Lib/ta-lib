---
title: "Relative Volatility Index, refined high/low form (RVIR)"
description: "Relative Volatility Index, refined form: Donald Dorsey's 1995 revision of his own indicator, which runs the 1993 RVI over the daily highs and again over…"
---

## Summary

Relative Volatility Index, refined form: Donald Dorsey's 1995 revision of his own indicator, which runs the 1993 RVI over the daily highs and again over the daily lows and averages the two indices.

Output is bounded [0..100] and is interpreted like RVI: above 50 the highs and lows have been more volatile while rising than while falling, below 50 the reverse. Dorsey's stated reason for the revision is that a high and a low carry the day's range, so the pair answers the question the close alone can only approximate.

## Formula

    RVIR[i] = 0.5 * ( RVI(high)[i] + RVI(low)[i] )

Both legs use the same `optInTimePeriod` and `optInStdDevPeriod`, so they warm on the same bar.

## Notes

- RVIR is the 1995 revision; [`RVI`](/functions/rvi.md) is the 1993 version.

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

## Implementation

TA-Lib Definition: [`rvir.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/rvir/rvir.c) · [`rvir.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/rvir/rvir.yaml)

| Native | File |
|--------|------|
| C | [`ta_RVIR.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_RVIR.c) |
| Rust | [`rvir.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/rvir.rs) |
| Java | [`Core_RVIR.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/fragments/Core_RVIR.java) |

TA-Lib is also available for Python, R and more using a [wrapper](/install/#wrappers).

## Aliases

Relative Volatility Index (1995 revision), RVIr

## See Also

[RVI](/functions/rvi.md) · [STDDEV](/functions/stddev.md) · [ATR](/functions/atr.md)

## References

- Dorsey, Donald. "Refining the Relative Volatility Index." *Technical Analysis of Stocks & Commodities*, V.13:9 (September 1995), 388-391.
- Dorsey, Donald. "The Relative Volatility Index." *Technical Analysis of Stocks & Commodities*, V.11:6 (June 1993), 253-256.
