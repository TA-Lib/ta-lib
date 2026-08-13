# EFI

## Summary

Alexander Elder's Force Index (*Trading for a Living*, 1993): volume-weighted momentum. Each bar's close-to-close move is weighted by that bar's volume, and the result is smoothed with an exponential moving average. The sign is the direction of the move; the size combines how far price travelled with how much volume stood behind it.

Elder reads two settings — 2 for the short term, which he pairs with a 22-period EMA of price to mark corrections against an established trend, and 13 for the intermediate term, the default here. A divergence against price can be confirmed by a zero-line cross. Beyond Elder, much longer settings are also in use, 100 or so, for the longer-term balance between buyers and sellers. Nothing normalises the result, so it scales with the instrument's own volume: read its sign and its shape over time, not its level against another instrument.

## Formula

force_t = ( close_t - close_{t-1} ) * volume_t; EFI = EMA( force, optInTimePeriod )

The EMA is TA-Lib's, seeded with a simple average of the first `optInTimePeriod` force values. A period of 1 leaves the raw one-bar Force Index.

## Inputs

- `inClose` — Close price of each bar
- `inVolume` — Volume of each bar

## Outputs

- `outReal` — Smoothed force

## Parameters

- `optInTimePeriod` — EMA period applied to the force series

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
- MotiveWave, *Elder's Force Index*: `rawForce = vol * (price - prevP)`, smoothed by a moving average whose default method is EMA, at 2 and 13. No competing formula was found.
