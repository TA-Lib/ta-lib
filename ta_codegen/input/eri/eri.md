# ERI

## Summary

Elder Ray Index: Alexander Elder's Bull Power / Bear Power pair from *Trading for a Living* (1993) — how far the bar's high and low sit from an EMA of the close. Bulls strong enough to push the high above the average read as positive Bull Power; bears dragging the low below it read as negative Bear Power.

## Formula

`Bull Power = High − EMA(Close, n)` and `Bear Power = Low − EMA(Close, n)`, both lines against the **same** EMA. Bull ≥ Bear on every bar since high ≥ low. TradingView's built-in *Bull Bear Power* — which its own support page calls "otherwise known as the Elder-Ray Index" — plots only the sum of the two, not the pair; StockCharts, TC2000 and pandas-ta all ship the two lines.

Because the underlying average is an [`EMA`](/functions/ema), ERI inherits its unstable period: the warm-up consumes `TA_GetUnstablePeriod(TA_FUNC_UNST_EMA)` extra bars, exactly as `EMA` itself does.

## Inputs

- `inHigh` — High price series
- `inLow` — Low price series
- `inClose` — Close price series

## Outputs

- `outBullPower` — High minus the EMA of close
- `outBearPower` — Low minus the EMA of close

## Parameters

- `optInTimePeriod` — Number of bars in the EMA of close

## Notes

- ERI is a cancelling difference: near the zero crossings that carry its signal, tiny EMA discrepancies are amplified without bound in relative terms. Compare against external values with an absolute tolerance.
- No MAType parameter: every canonical source fixes the EMA, and a selectable average would invent a variant nobody ships.

## Implementation

TA-Lib Definition: [`eri.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/eri/eri.c) · [`eri.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/eri/eri.yaml)

| Native | File |
|--------|------|
| C | [`ta_ERI.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_ERI.c) |
| Rust | [`eri.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/eri.rs) |
| Java | [`Core_ERI.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/fragments/Core_ERI.java) |

TA-Lib is also available for Python, R and more using a [wrapper](/install/#wrappers).

## Aliases

Elder-Ray Index · Bull Power / Bear Power

## See Also

EMA · EFI · MACD

## References

- Alexander Elder, *Trading for a Living* (Wiley, 1993), the Elder-Ray chapter
