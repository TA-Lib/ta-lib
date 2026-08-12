# EFI

## Summary

Alexander Elder's Force Index (*Trading for a Living*, 1993): volume-weighted momentum. Each bar's close-to-close move is multiplied by that bar's volume — direction from the price change, conviction from the volume behind it — and the result is smoothed with an exponential moving average. Elder reads a 2-period smoothing as the short-term force and 13 as the intermediate-term one.

## Formula

force_t = ( close_t - close_{t-1} ) * volume_t; EFI = EMA( force, optInTimePeriod )

The EMA is TA-Lib's, seeded with a simple average of the first `optInTimePeriod` force values. A period of 1 leaves the raw one-bar Force Index.

## Inputs

- `inClose` — Close price of each bar
- `inVolume` — Volume of each bar

## Outputs

- `outReal` — Smoothed force

## Parameters

- `optInTimePeriod` — EMA smoothing length. Default 13, Elder's intermediate-term setting; his short-term reading uses 2. Obeys `TA_SetUnstablePeriod(TA_FUNC_UNST_EMA, ...)`.

## Implementation

TA-Lib Definition: [`efi.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/efi/efi.c) · [`efi.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/efi/efi.yaml)

| Native | File |
|--------|------|
| C | [`ta_EFI.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_EFI.c) |
| Rust | [`efi.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/efi.rs) |
| Java | [`Core_EFI.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/fragments/Core_EFI.java) |
| C# | [`Core_EFI.cs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/csharp/library/src/Core_EFI.cs) |

TA-Lib is also available for Python, R and more using a [wrapper](/install/#wrappers).

## See Also

AD · EMA · MFI · OBV · PVO

## References

- Alexander Elder, *Trading for a Living*, Wiley, 1993, introduces the Force Index and the 2-period and 13-period smoothing he reads as short- and intermediate-term.
- StockCharts ChartSchool, *Force Index*: `Force Index(1) = {Close - Close prior} x Volume`, `Force Index(13) = 13-period EMA of Force Index(1)`.
- TradingView's built-in `ta.fi(length)` is `ta.ema(ta.change(close) * volume, length)`; MotiveWave and Investopedia agree. No competing formula was found.
- pandas-ta-classic `efi` computes the same form; its EMA was written to reproduce TA-Lib's seeding, so the two agree on the warm-up by construction rather than independently. Tulip Indicators ships no force index.
