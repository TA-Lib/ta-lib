---
title: "Premier Stochastic Oscillator (PSO)"
description: "Premier Stochastic Oscillator: a short-period Fast %K, recentred on zero and rescaled, double-smoothed and then squashed into the range -1 to +1."
---

## Summary

Premier Stochastic Oscillator: a short-period Fast %K, recentred on zero and rescaled, double-smoothed and then squashed into the range -1 to +1. Leibfarth's reading is that the plain stochastic spends most of its life pinned at one end or the other, so the extremes stop meaning anything; the two exponential passes strip the bar-to-bar noise out of it, and the squash gives back a scale on which the extremes are rare again. Readings beyond ±0.9 are the extremes, and ±0.2 the band Leibfarth watches for the crossing back toward the middle.

## Formula

K = STOCHF(high, low, close, fastKPeriod)   (Fast-K only)

NSK = 0.1 * (K - 50)

SS = EMA(EMA(NSK, emaPeriod), emaPeriod)

PSO = (e^SS - 1) / (e^SS + 1) = tanh(SS / 2)

## Notes

- The affine step comes before the smoothing, as the author's listing spells it. Smoothing first and recentring after is the same value in real arithmetic and differs in the last bit or two in doubles.
- The squash is computed as `tanh(SS/2)`, which is the published quotient rewritten. The quotient form is `inf/inf` once `SS` exceeds 709.78, which a bar whose close lies outside its own high/low range can reach; `tanh` saturates at ±1 instead.
- A Fast-K window whose range is zero reads 50, the midpoint, so a flat market reads PSO 0 rather than the near-extreme the Fast-K convention of 0 would give it. The flatness test is the one STOCHF applies, against the window's own extremes rather than a fixed band.
- Each exponential pass is seeded with a simple average of its own first inputs, the same seeding TA-Lib's EMA uses, and the second pass seeds on what the first publishes. `TA_SetUnstablePeriod(TA_FUNC_UNST_EMA, ...)` discards more of that warm-up, through both passes. Implementations seeding each pass from a single first sample differ over the transient and agree once it decays.
- Leibfarth parameterises the smoothing as the square root of a longer period, 25 in his article. The length is taken here directly, as an integer, because the sources that follow the square root disagree over how to round it.

## Inputs

- `inHigh` — High price series
- `inLow` — Low price series
- `inClose` — Close price series

## Outputs

- `outReal` — Premier Stochastic Oscillator, -1 to +1

## Parameters

| Parameter | Type | Default | Accepted values | Description |
| --- | --- | --- | --- | --- |
| `optInFastK_Period` | integer | 8 | 1–100000 | Time period for building the Fast-K line |
| `optInEMAPeriod` | integer | 5 | 1–100000 | Period of each of the two smoothing passes |

## Properties

**Numerical Stability:** [Initial Unstable Period](/functions/stability.md#initial-unstable-period) — Inherited from EMA, which PSO computes internally; tunable via EMA's unstable period.

<div class="flag-table">

|  |
| :-- |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Overlap Input</span> |
| <span class="flag-box">✅</span> **Independent Y-Axis** <span class="flag-tip" tabindex="0" role="note" aria-label="Output is on its own scale, drawn in a separate pane below the price chart." data-tip="Output is on its own scale, drawn in a separate pane below the price chart.">i</span> |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Candlestick</span> |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Can Output NaN or ±Inf</span> |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Identity at Period 1</span> |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Display Shift</span> |
| <span class="flag-box">✅</span> **Uses Transcendental** <span class="flag-tip" tabindex="0" role="note" aria-label="Calls a transcendental math function (exp, log, log10, trigonometric, inverse trigonometric or hyperbolic), so a value may differ slightly between languages, between a stream and a batch call, and between machines. TA-Lib targets a difference below 1e-9." data-tip="Calls a transcendental math function (exp, log, log10, trigonometric, inverse trigonometric or hyperbolic), so a value may differ slightly between languages, between a stream and a batch call, and between machines. TA-Lib targets a difference below 1e-9.">i</span> |

</div>

## Implementation

TA-Lib Definition: [`pso.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/pso/pso.c) · [`pso.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/pso/pso.yaml)

| Native | File |
|--------|------|
| C | [`ta_PSO.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_PSO.c) |
| Rust | [`pso.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/pso.rs) |
| Java | [`Core_PSO.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/fragments/Core_PSO.java) |
| C# | [`Core_PSO.cs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/csharp/library/src/Core_PSO.cs) |

TA-Lib is also available for Python, R and more using a [wrapper](/install/#wrappers).

## Aliases

premier stochastic oscillator, premier stochastic

## See Also

[STOCHF](/functions/stochf.md) · [STOCH](/functions/stoch.md) · [SMI](/functions/smi.md) · [WILLR](/functions/willr.md)

## References

- Lee Leibfarth, "Premier Stochastic Oscillator", *Technical Analysis of Stocks & Commodities*, v26:8 (August 2008), pp. 30-36
