---
title: "Williams' Accumulation/Distribution (no volume) (WAD)"
description: "Williams' Accumulation/Distribution: a cumulative line that measures each bar's close against the true range extreme — the previous close, whenever it…"
---

## Summary

Williams' Accumulation/Distribution: a cumulative line that measures each bar's close against the *true range* extreme — the previous close, whenever it lies outside the current bar — rather than against the bar's own high and low. A close above the previous one accumulates the distance up from the true low; a close below it distributes the distance down from the true high; an unchanged close contributes nothing.

**It consumes no volume.** Larry Williams' original multiplies each move by that bar's volume; Steven Achelis published the modification that drops the multiplier (*Technical Analysis from A to Z*, 2nd ed., p.368), the industry kept Williams' name on it, and that no-volume form is what Tulip Indicators, pandas-ta-classic, cTrader, TC2000, WealthCharts and MultiCharts all ship — so it is what TA-Lib ships under this name. The volume-weighted original is a different series. WAD is grouped with the volume indicators for discoverability next to AD, ADOSC and OBV, not because it reads volume.

## Formula

TRH_t = max(close_{t-1}, high_t); TRL_t = min(close_{t-1}, low_t); AD_t = close_t - TRL_t if close_t > close_{t-1}, close_t - TRH_t if close_t < close_{t-1}, otherwise 0; WAD_t = WAD_{t-1} + AD_t

The first bar of the requested range has no previous close, so it contributes 0 and the line starts there — the same convention as AD, OBV, NVI and PVI. The accumulator restarts wherever the caller starts, so a different `startIdx` shifts the whole line by a constant.

## Inputs

- `inHigh` — High price of each bar
- `inLow` — Low price of each bar
- `inClose` — Close price of each bar

## Outputs

- `outReal` — Cumulative accumulation/distribution

## Properties

**Numerical Stability:** [Path-Dependent](/functions/stability.md#path-dependent)

| Display<br>Flags |
| :-- |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Overlap Input</span> |
| <span class="flag-box">✅</span> **Independent Y-Axis** <span class="flag-tip" tabindex="0" role="note" aria-label="Output is on its own scale, drawn in a separate pane below the price chart." data-tip="Output is on its own scale, drawn in a separate pane below the price chart.">i</span> |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Candlestick</span> |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Can Output NaN or ±Inf</span> |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Identity at Period 1</span> |

## Implementation

TA-Lib Definition: [`wad.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/wad/wad.c) · [`wad.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/wad/wad.yaml)

| Native | File |
|--------|------|
| C | [`ta_WAD.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_WAD.c) |
| Rust | [`wad.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/wad.rs) |
| Java | [`Core_WAD.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/fragments/Core_WAD.java) |
| C# | [`Core_WAD.cs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/csharp/library/src/Core_WAD.cs) |

TA-Lib is also available for Python, R and more using a [wrapper](https://ta-lib.org/wrappers/).
