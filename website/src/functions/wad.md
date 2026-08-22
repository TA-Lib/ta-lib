---
title: "Williams' Accumulation/Distribution (WAD)"
description: "Williams' Accumulation/Distribution: a cumulative line meant to expose whether a security is quietly under accumulation (informed buying) or distribution…"
---

## Summary

Williams' Accumulation/Distribution: a cumulative line meant to expose whether a security is quietly under accumulation (informed buying) or distribution (informed selling) beneath the surface of price. Larry Williams built it to catch that shift before price confirms it — traders watch for the line to diverge from price, since a line that keeps rising while price stalls or falls points to accumulation, and one that stalls while price pushes to a new high points to distribution.

**It consumes no volume.** Larry Williams' original multiplies each move by that bar's volume; Steven Achelis published the modification that drops the multiplier (*Technical Analysis from A to Z*, 2nd ed., p.368), and the industry kept Williams' name on that no-volume form. That industry-wide decision is enough for TA-Lib to ship the same form under the same name. What remains once the multiplier is dropped is a signed close-to-close move measured on the true range, so it is grouped as a momentum indicator, not a volume one.

## Formula

For each bar t:

    TRH_t = max(close_{t-1}, high_t)
    TRL_t = min(close_{t-1}, low_t)

    if close_t > close_{t-1} then AD_t = close_t - TRL_t
    if close_t < close_{t-1} then AD_t = close_t - TRH_t
    otherwise                     AD_t = 0

    WAD_t = WAD_{t-1} + AD_t

The first bar of the requested range has no previous close, so the first output is always AD_t = 0. A different `startIdx` shifts WAD's whole line by a constant.

## Inputs

- `inHigh` — High price of each bar
- `inLow` — Low price of each bar
- `inClose` — Close price of each bar

## Outputs

- `outReal` — Cumulative accumulation/distribution

## Properties

**Numerical Stability:** [Path-Dependent](/functions/stability.md#path-dependent)

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

TA-Lib Definition: [`wad.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/wad/wad.c) · [`wad.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/wad/wad.yaml)

| Native | File |
|--------|------|
| C | [`ta_WAD.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_WAD.c) |
| Rust | [`wad.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/wad.rs) |
| Java | [`Core_WAD.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/fragments/Core_WAD.java) |
| C# | [`Core_WAD.cs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/csharp/library/src/Core_WAD.cs) |

TA-Lib is also available for Python, R and more using a [wrapper](/install/#wrappers).

## See Also

[AD](/functions/ad.md) · [ADOSC](/functions/adosc.md) · [NVI](/functions/nvi.md) · [OBV](/functions/obv.md) · [PVI](/functions/pvi.md)

## References

- Larry Williams is the originator; Steven Achelis, *Technical Analysis from A to Z*, 2nd edition, page 368 publishes the no-volume form this ships, with the worked 12-bar example pinned in the test suite.
- IncredibleCharts, [*Williams Accumulation Distribution*](https://www.incrediblecharts.com/indicators/williams_accumulation_distribution.php) — Williams' volume-weighted original, `AD = Price Move × Volume` over the same true-range price move.
- IncredibleCharts, [*Williams Accumulate Distribute*](https://www.incrediblecharts.com/indicators/williams_accumulate_distribute.php) — the Achelis form under its disambiguating name, "not a volume indicator despite the name".
