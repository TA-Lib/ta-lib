---
title: "Squeeze Momentum and Level (SQZMOM)"
description: "John Carter's TTM Squeeze, in the form LazyBear published on TradingView in 2014."
---

## Summary

John Carter's TTM Squeeze, in the form LazyBear published on TradingView in 2014. Volatility compression is read by comparing the Bollinger Bands against a Keltner Channel: when the bands sit inside the channel the market is coiled, and when they push outside it the coil has released. LazyBear replaced Carter's simple momentum with a linear regression of the close against a Donchian/SMA anchor, which is the histogram that StockCharts, pandas, ta4j and trading-signals all compute.

"Squeeze Pro" tests the bands against three Keltner widths instead of one, so compression becomes a level rather than a boolean. `outSqueeze` carries that level: 3, 2 and 1 for the narrow, normal and wide channels, -1 once the bands are outside the wide channel, and 0 in between. The classic squeeze-on flag is `outSqueeze >= 2`, and with `wide == normal` the three states are exactly LazyBear's on / off / none.

## Formula

With `b = optInBBPeriod` and `k = optInKCPeriod`:

    BBU, BBL = SMA(close, b) +/- optInNbDev * STDDEV(close, b)
    KCM      = SMA(close, k)
    BAND     = SMA(TRANGE(high, low, close), k)
    DMID     = MIDPRICE(high, low, k)
    DEV      = close - ( DMID + KCM ) / 2

    outMomentum = LINEARREG(DEV, k)

    inside(m)  = BBL > KCM - m*BAND  and  BBU < KCM + m*BAND

    outSqueeze =  3  if inside(optInFactorNarrow)
                  2  if inside(optInFactorNormal)
                  1  if inside(optInFactorWide)
                 -1  if BBL < KCM - optInFactorWide*BAND and BBU > KCM + optInFactorWide*BAND
                  0  otherwise

## Notes

- `BAND` is the simple mean of the true range over `k` bars, not an ATR: no Wilder smoothing, and so no unstable period anywhere in this function.
- The anchor is written three ways across the sources — `avg(avg(HH,LL), SMA)`, `((HH+LL)/2 + SMA)/2` and `0.25*(HH+LL) + 0.5*SMA`. All three give the same double, because scaling by a power of two is exact.
- The three channels share a centre and `BAND >= 0`, so they nest: inside the narrow one implies inside the normal one implies inside the wide one. The ordinal is therefore monotone, and the factors are required only to be ordered `wide >= normal >= narrow >= 0`, not strictly.
- The momentum window is the Keltner period, as in LazyBear's script, pandas and trading-signals.
- The three chains that feed the outputs do not reach their first value together. The first output bar is the latest of them, so where `b` is much larger than `k` the momentum exists before the state does and is still withheld until both do. pandas emits the earlier momentum; a capture compared against it must start at the later bar.

## Inputs

- `inHigh` — High price of each bar
- `inLow` — Low price of each bar
- `inClose` — Closing price of each bar

## Outputs

- `outMomentum` — Linear regression of the close against the Donchian/SMA anchor
- `outSqueeze` — Compression level: 3 narrow, 2 normal, 1 wide, -1 released, 0 otherwise

## Parameters

| Parameter | Type | Default | Accepted values | Description |
| --- | --- | --- | --- | --- |
| `optInBBPeriod` | integer | 20 | 2–100000 | Number of bars in the Bollinger Band window |
| `optInNbDev` | real | 2 | ≥ 0 | Number of standard deviations the Bollinger Bands are placed at |
| `optInKCPeriod` | integer | 20 | 2–100000 | Number of bars in the Keltner Channel, the true-range mean and the regression |
| `optInFactorWide` | real | 2 | ≥ 0 | Keltner width, in true-range means, of the widest channel |
| `optInFactorNormal` | real | 1.5 | ≥ 0 | Keltner width of the classic channel |
| `optInFactorNarrow` | real | 1 | ≥ 0 | Keltner width of the tightest channel |

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
| <span class="flag-box">☐</span> <span style="opacity:0.5">Display Shift</span> |

</div>

## Implementation

TA-Lib Definition: [`sqzmom.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/sqzmom/sqzmom.c) · [`sqzmom.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/sqzmom/sqzmom.yaml)

| Native | File |
|--------|------|
| C | [`ta_SQZMOM.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_SQZMOM.c) |
| Rust | [`sqzmom.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/sqzmom.rs) |
| Java | [`Core_SQZMOM.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/fragments/Core_SQZMOM.java) |
| C# | [`Core_SQZMOM.cs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/csharp/library/src/Core_SQZMOM.cs) |

TA-Lib is also available for Python, R and more using a [wrapper](/install/#wrappers).

## Aliases

Squeeze Momentum, TTM Squeeze, TTM Squeeze Pro, SQZMOM_LB

## See Also

[BBANDS](/functions/bbands.md) · [KC](/functions/kc.md) · [TRANGE](/functions/trange.md) · [MIDPRICE](/functions/midprice.md) · [LINEARREG](/functions/linearreg.md)

## References

- John F. Carter, *Mastering the Trade*, McGraw-Hill, 2005, chapter 11
- LazyBear, "Squeeze Momentum Indicator [LazyBear]", TradingView, 2014-07-04
- Beardy_Fred, "TTM Squeeze Pro", TradingView, 2021
