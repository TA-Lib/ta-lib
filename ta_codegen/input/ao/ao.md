# AO

## Summary

Bill Williams' Awesome Oscillator (*New Trading Dimensions*, 1998): market momentum read as the spread between a short and a long simple moving average of the median price. It contrasts what the recent bars have done against a longer stretch of the same market, using the bar midpoint rather than the close so that intrabar range, not the settle, drives the reading.

Above zero the short window sits higher than the long one and momentum is with the bulls; below zero it is with the bears. It is drawn as a zero-centred histogram, and the readings that get traded are the zero-line crossings, the twin-peaks divergence, and the run of consecutive same-side bars — which is why the sign and the bar-to-bar change matter more than the level.

The oscillator is the first leg of Williams' Profitunity system, alongside the Alligator and the Accelerator/Decelerator.

## Formula

median_t = ( high_t + low_t ) / 2; AO_t = SMA(median, fast)_t − SMA(median, slow)_t

Both legs are plain simple moving averages, so there is no seeding convention and none of the cross-library divergence that comes with one. An inverted pair is not swapped: passing a fast period longer than the slow one is well defined and simply yields −AO.

## Inputs

- `inHigh` — High price of each bar
- `inLow` — Low price of each bar

## Outputs

- `outReal` — Spread between the two moving averages, centred on zero

## Parameters

- `optInFastPeriod` — Number of bars in the short moving average. Default 5, the value Williams uses and every surveyed package ships.
- `optInSlowPeriod` — Number of bars in the long moving average. Default 34, likewise universal. MetaTrader, cTrader and Tulip Indicators hardcode the pair; TradingView, pandas-ta-classic and StockSharp expose it, and at the defaults the two agree exactly.

## Implementation

TA-Lib Definition: [`ao.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/ao/ao.c) · [`ao.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/ao/ao.yaml)

| Native | File |
|--------|------|
| C | [`ta_AO.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_AO.c) |
| Rust | [`ao.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/ao.rs) |
| Java | [`Core_AO.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/fragments/Core_AO.java) |
| C# | [`Core_AO.cs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/csharp/library/src/Core_AO.cs) |

TA-Lib is also available for Python, R and more using a [wrapper](/install/#wrappers).

## Aliases

Awesome Oscillator, Bill Williams Awesome Oscillator, BW AO

## See Also

APO · MACD · MEDPRICE · PPO · ULTOSC

## References

- Bill Williams, *New Trading Dimensions*, Wiley, 1998, and *Trading Chaos*, define the Awesome Oscillator as the 5-period less the 34-period simple moving average of the median price.
- Tulip Indicators `ti_ao` and pandas-ta-classic `ao` compute the same form on the same inputs, and both report the first value at the same bar. Tulip hardcodes the periods and multiplies by a precomputed reciprocal; this divides, which is what keeps each leg bit-identical to `TA_SMA`.
- pandas-ta-classic swaps an inverted pair before computing, so it answers AO(slow, fast) where this answers its negation.
- MetaTrader 4 and 5 expose the indicator as `iAO`, cTrader as `AwesomeOscillator`, and TradingView under the short title `AO`; the abbreviation is settled across the industry.
