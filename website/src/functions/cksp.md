---
title: "Chande Kroll Stop (CKSP)"
description: "Chande Kroll Stop places a pair of trailing stops a multiple of the Average True Range away from the recent extremes, then takes the extreme of those…"
---

## Summary

Chande Kroll Stop places a pair of trailing stops a multiple of the Average True Range away from the recent extremes, then takes the extreme of those stops over a second, usually longer, window. The result is a stop that follows price but only ratchets after the shorter stop has held for a while.

The high stop sits below price and is the level a long position would give up at; the low stop sits above price and is the short side's. Neither line is always above the other: when the multiplier is large enough the two cross, which is the signal that the range has widened past what the stops can straddle.

## Formula

ATR[i] = Wilder Average True Range of period p at bar i

FHS[i] = MAX( high[i-p+1 .. i] ) - x * ATR[i]

FLS[i] = MIN( low[i-p+1 .. i] ) + x * ATR[i]

outHighStop[i] = MAX( FHS[i-q+1 .. i] )

outLowStop[i] = MIN( FLS[i-q+1 .. i] )

p = optInTimePeriod, x = optInMultiplier, q = optInStopPeriod

## Notes

The Average True Range is this library's, seeded the way `ATR` seeds it. Implementations that start their range at the first bar instead differ over the early bars and converge afterwards; the difference is a seeding convention, not a different indicator.

Each stage is anchored on its own bars. The second stage reads the q-1 first-stage bars before the first output, and those in turn are computed from their own Average True Range and their own extreme windows, so a call started late gives the same values as the tail of a call started at the beginning.

At a multiplier of 0 the two outputs collapse to the plain extremes of the whole p+q-1 span: the highest high and the lowest low.

At a stop period of 1 the second stage is the identity and the outputs are the first-stage stops, which is the Chandelier Exit form.

## Inputs

- `inHigh` — High price of each bar
- `inLow` — Low price of each bar
- `inClose` — Close price of each bar, read only by the True Range

## Outputs

- `outHighStop` — Trailing stop below price, the high side
- `outLowStop` — Trailing stop above price, the low side

## Parameters

| Parameter | Type | Default | Accepted values | Description |
| --- | --- | --- | --- | --- |
| `optInTimePeriod` | integer | 10 | 2–100000 | ATR and extreme window |
| `optInMultiplier` | real | 1 | ≥ 0 | ATR multiplier |
| `optInStopPeriod` | integer | 9 | 1–100000 | Stop window |

## Properties

**Numerical Stability:** [Initial Unstable Period](/functions/stability.md#initial-unstable-period) — Inherited from ATR, which CKSP computes internally; tunable via ATR's unstable period.

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

TA-Lib Definition: [`cksp.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/cksp/cksp.c) · [`cksp.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/cksp/cksp.yaml)

| Native | File |
|--------|------|
| C | [`ta_CKSP.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_CKSP.c) |
| Rust | [`cksp.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/cksp.rs) |
| Java | [`Core_CKSP.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/fragments/Core_CKSP.java) |
| C# | [`Core_CKSP.cs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/csharp/library/src/Core_CKSP.cs) |

TA-Lib is also available for Python, R and more using a [wrapper](/install/#wrappers).

## Aliases

Chande Kroll Stop

## See Also

- [ATR](atr.md) — the range measure the stop distance is built from
- [SUPERTREND](supertrend.md) — the other Overlap Study that offsets a band by a multiple of the ATR
- [MAX](max.md), [MIN](min.md) — the rolling extremes each stage takes

## References

- Tushar Chande and Stanley Kroll, *The New Technical Trader*, Wiley, 1994.
