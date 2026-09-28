---
title: "Fractal Adaptive Moving Average (FRAMA)"
description: "Fractal Adaptive Moving Average (John Ehlers): an EMA whose smoothing factor adapts each bar to the fractal dimension of the window, estimated from the…"
---

## Summary

Fractal Adaptive Moving Average (John Ehlers): an EMA whose smoothing factor adapts each bar to the fractal dimension of the window, estimated from the high-low ranges of the window and of its two halves. A straight run gives dimension 1 and the output follows the price; dense congestion gives dimension 2 and very slow smoothing.

## Formula

P[t] = (High[t] + Low[t]) / 2
R1 = range of the newer half, R2 = range of the older half, R = range of the whole window
D = 1 + log2((R1 + R2) / R)
alpha = exp(-4.6 * (D - 1)), at most 1; alpha = 1 when R1 or R2 is 0
FRAMA[t] = alpha * P[t] + (1 - alpha) * FRAMA[t-1], seeded with P on the bar before the first output

## Notes

- Where either half of the window is flat, alpha is 1 and the output is the price; Ehlers' listing keeps the previous bar's dimension there instead.
- The period must be even; an odd period is rejected.

## Inputs

- `inHigh` — High price of each bar
- `inLow` — Low price of each bar

## Outputs

- `outReal` — Adaptive moving average line

## Parameters

| Parameter | Type | Default | Accepted values | Description |
| --- | --- | --- | --- | --- |
| `optInTimePeriod` | integer | 16 | 2–100000 | Number of bars in the window, split into two equal halves |

## Properties

**Numerical Stability:** [Initial Unstable Period](/functions/stability.md#initial-unstable-period)

<div class="flag-table">

|  |
| :-- |
| <span class="flag-box">✅</span> **Overlap Input** <span class="flag-tip" tabindex="0" role="note" aria-label="Output is on the same scale as the input price, so it is drawn over the price chart." data-tip="Output is on the same scale as the input price, so it is drawn over the price chart.">i</span> |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Independent Y-Axis</span> |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Candlestick</span> |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Can Output NaN or ±Inf</span> |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Identity at Period 1</span> |

</div>

## Implementation

TA-Lib Definition: [`frama.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/frama/frama.c) · [`frama.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/frama/frama.yaml)

| Native | File |
|--------|------|
| C | [`ta_FRAMA.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_FRAMA.c) |
| Rust | [`frama.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/frama.rs) |
| Java | [`Core_FRAMA.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/fragments/Core_FRAMA.java) |
| C# | [`Core_FRAMA.cs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/csharp/library/src/Core_FRAMA.cs) |

TA-Lib is also available for Python, R and more using a [wrapper](/install/#wrappers).

## Aliases

Fractal Adaptive Moving Average, FrAMA

## See Also

[KAMA](/functions/kama.md) · [MAMA](/functions/mama.md) · [MEDPRICE](/functions/medprice.md)

## References

- John F. Ehlers, "Fractal Adaptive Moving Averages", Technical Analysis of Stocks & Commodities V.23:10 (October 2005)
