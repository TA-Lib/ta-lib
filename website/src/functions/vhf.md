---
title: "Vertical Horizontal Filter (VHF)"
description: "Vertical Horizontal Filter: Adam White's trend-versus-range filter, the range a window covered divided by the path it actually travelled. Bounded in [0,1]."
---

## Summary

Vertical Horizontal Filter: Adam White's trend-versus-range filter, the range a window covered divided by the path it actually travelled.

Bounded in [0,1]. Values near 1 mean the market covered most of its path in one direction (trending); values near 0 mean it retraced repeatedly and went nowhere (choppy). Like ADX it measures trend *strength*, not direction, but it uses no smoothing and carries no recursion.

A common use is regime selection: run trend-following logic while VHF is high, oscillator logic while it is low.

## Formula

num = MAX(C[t-optInTimePeriod+1..t]) - MIN(C[t-optInTimePeriod+1..t]), the range spanned by the `optInTimePeriod` most recent closes. den = SUM( |C[j] - C[j-1]| ) for j = t-optInTimePeriod+1 .. t, the total absolute movement over the same number of changes, which therefore reaches one close further back. VHF = num / den.

The two windows are deliberately not co-terminal: the extrema span `optInTimePeriod` closes, the changes consume one more. Because `num` is the distance between two points the changes connect, `num <= den` always, so the result never leaves [0,1].

## Notes

- A window whose closes are all identical has no vertical movement and no horizontal movement. VHF reports 0 there. Other libraries differ: Tulip Indicators leaves the division unguarded and emits NaN, pandas-ta-classic perturbs the numerator and emits +Inf.
- Adam White later described an 18-bar VHF smoothed by a 6-bar moving average. That variant is not implemented here; apply a moving average to `outReal` to obtain it.

## Inputs

- `inReal` — Source price/value series, canonically the close

## Outputs

- `outReal` — Vertical Horizontal Filter value

## Parameters

| Parameter | Type | Default | Accepted values | Description |
| --- | --- | --- | --- | --- |
| `optInTimePeriod` | integer | 28 | 2–100000 | Number of trailing closes spanned by the range |

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

</div>

## Implementation

TA-Lib Definition: [`vhf.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/vhf/vhf.c) · [`vhf.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/vhf/vhf.yaml)

| Native | File |
|--------|------|
| C | [`ta_VHF.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_VHF.c) |
| Rust | [`vhf.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/vhf.rs) |
| Java | [`Core_VHF.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/fragments/Core_VHF.java) |

TA-Lib is also available for Python, R and more using a [wrapper](/install/#wrappers).

## Aliases

Vertical Horizontal Filter

## See Also

[ADX](/functions/adx.md) · [CMO](/functions/cmo.md) · [CMOU](/functions/cmou.md)

## References

- Adam White, "The Vertical Horizontal Filter", *Futures Magazine*, August 1991
- Steven B. Achelis, *Technical Analysis from A to Z*, McGraw-Hill (p. 354)
- [Incredible Charts: Vertical Horizontal Filter](https://www.incrediblecharts.com/indicators/vertical_horizontal_filter.php)
