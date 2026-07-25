---
title: HMA
description: "Hull Moving Average, published by Alan Hull in 2005: a moving average built to track price with far less lag than an [`SMA`](/functions/sma), [`WMA`](/functions/wma) or [`EMA`](/functions/ema) of the same length while staying smooth. It first removes lag by doubling a half-period [`WMA`](/functions/wma) and subtracting the full-period one — extrapolating the average toward current price — then smooths that de-lagged series with a final WMA over the square root of the period. HMA is also selectable as a moving-average type (`TA_MAType_HMA`) wherever an `optInMAType` parameter is accepted ([`MA`](/functions/ma), [`BBANDS`](/functions/bbands), [`STOCH`](/functions/stoch), [`MACDEXT`](/functions/macdext), ...)."
---

# HMA

## Summary

Hull Moving Average, published by Alan Hull in 2005: a moving average built to track price with far less lag than an [`SMA`](/functions/sma), [`WMA`](/functions/wma) or [`EMA`](/functions/ema) of the same length while staying smooth. It first removes lag by doubling a half-period [`WMA`](/functions/wma) and subtracting the full-period one — extrapolating the average toward current price — then smooths that de-lagged series with a final WMA over the square root of the period.

HMA is also selectable as a moving-average type (`TA_MAType_HMA`) wherever an `optInMAType` parameter is accepted ([`MA`](/functions/ma), [`BBANDS`](/functions/bbands), [`STOCH`](/functions/stoch), [`MACDEXT`](/functions/macdext), ...).

## Formula

HMA(n) = WMA( 2 * WMA(price, Integer(n/2)) - WMA(price, n), Integer(SquareRoot(n)) )

All three averages are the standard linearly-weighted moving average (TA-Lib's WMA). Every output is a closed-form weighted sum of the input window: there is no seeding, no recursion, hence no unstable period.

## Notes

- The two derived periods `n/2` and `sqrt(n)` are **truncated** to integers, exactly as in Alan Hull's own statement of the formula (`Integer()`); Tulip Indicators and pandas-ta do the same. Some other published descriptions round to nearest instead, which changes both the values and, for the square root, the lookback — a visibly different line, not a tolerance-level difference. TA-Lib follows the author.
- The default period of 20 is Alan Hull's own default. It is also a period on which the truncate and round-to-nearest conventions coincide (20/2 is exact; sqrt(20) = 4.47 truncates and rounds to 4), so at the default TA-Lib matches charting platforms regardless of their rounding convention.
- The period range starts at 2: a period of 1 would make the half-period WMA degenerate (`Integer(1/2) = 0`).

## Inputs

- `inReal` — Source price series, close by convention

## Outputs

- `outReal` — Hull moving average of the input

## Parameters

| Parameter | Type | Default | Accepted values | Description |
| --- | --- | --- | --- | --- |
| `optInTimePeriod` | integer | 20 | 2–100000 | Number of bars in the full-period WMA; the half and square-root periods derive from it |

## Properties

**Numerical Stability:** [Start-Independent](/functions/stability#start-independent)

| Display<br>Flags |
| :-- |
| <span class="flag-box">✅</span> **Overlap Input** <span class="flag-tip" tabindex="0" role="note" aria-label="Output is on the same scale as the input price, so it is drawn over the price chart." data-tip="Output is on the same scale as the input price, so it is drawn over the price chart.">i</span> |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Independent Y-Axis</span> |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Candlestick</span> |

## Implementation

TA-Lib Definition: [`hma.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/hma/hma.c) · [`hma.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/hma/hma.yaml)

| Native | File |
|--------|------|
| C | [`ta_HMA.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_HMA.c) |
| Rust | [`hma.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/hma.rs) |
| Java | [`Core_HMA.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/library/fragments/Core_HMA.java) |

TA-Lib is also available for Python, R and more using a [wrapper](https://ta-lib.org/wrappers/).

## Aliases

Hull Moving Average

## See Also

[WMA](/functions/wma) · [MA](/functions/ma) · [SMA](/functions/sma) · [EMA](/functions/ema)

## References

- Alan Hull, *How to reduce lag in a moving average* — the original definition, including the `Integer()` truncation of both derived periods: [alanhull.com/hull-moving-average](https://alanhull.com/hull-moving-average)
