---
title: "Wilder Accumulative Swing Index (ASI)"
description: "Wilder's Accumulative Swing Index: the running total of the Swing Index (SI), a line meant to trace the market's real swings through the noise of the…"
---

## Summary

Wilder's Accumulative Swing Index: the running total of the Swing Index (SI), a line meant to trace the market's real swings through the noise of the daily closes. Wilder draws trendlines and breakout points on it as on a price chart: a breakout of the ASI that the price has not confirmed yet is his signal. J. Welles Wilder Jr. introduced both in 1978. The line starts at 0 on the first bar of the requested range, so only its shape carries meaning, not its level.

## Formula

$$
\mathrm{ASI}_s = 0 \qquad \mathrm{ASI}_t = \mathrm{ASI}_{t-1} + \mathrm{SI}_t \quad (t > s)
$$

where $s$ is the first bar of the requested range and $\mathrm{SI}_t$ is the Swing Index of bar $t$ against bar $t-1$ at the same `optInLimitMove`.

## Notes

- Wilder accumulates Swing Index values rounded to whole numbers; the running total here is of the unrounded values.

## Inputs

- `inOpen` — Open price of each bar
- `inHigh` — High price of each bar
- `inLow` — Low price of each bar
- `inClose` — Close price of each bar

## Outputs

- `outReal` — Running total of the swing index from the first bar of the range

## Parameters

| Parameter | Type | Default | Accepted values | Description |
| --- | --- | --- | --- | --- |
| `optInLimitMove` | real | 3 | ≥ 0.00000001 | Limit move, the largest one-bar price move the index is scaled against, in price units |

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

TA-Lib Definition: [`asi.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/asi/asi.c) · [`asi.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/asi/asi.yaml)

| Native | File |
|--------|------|
| C | [`ta_ASI.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_ASI.c) |
| Rust | [`asi.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/asi.rs) |
| Java | [`Core_ASI.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/fragments/Core_ASI.java) |
| C# | [`Core_ASI.cs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/csharp/library/src/Core_ASI.cs) |

TA-Lib is also available for Python, R and more using a [wrapper](/install/#wrappers).

## Aliases

Accumulation Swing Index

## See Also

[SI](/functions/si.md) · [WAD](/functions/wad.md) · [CUMSUM](/functions/cumsum.md)

## References

- J. Welles Wilder Jr., *New Concepts in Technical Trading Systems*, Trend Research, 1978, section VIII "The Swing Index", pp.87-106
