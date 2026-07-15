# MFI

## Summary

Money Flow Index: a volume-weighted momentum oscillator (0-100) comparing positive vs negative money flow over a period. A volume-based analog of RSI. >80 overbought, <20 oversold.

## Formula

TP = (High+Low+Close)/3; MF = TP*Volume, classed positive if TP>prevTP, negative if TP<prevTP, neither if equal. MFI = 100 * posSumMF/(posSumMF+negSumMF).

## Notes

- When the typical price is unchanged from the prior bar, that bar's money flow is counted as neither positive nor negative.

## Inputs

- `inPriceHLCV` — High, low, close, and volume series

## Outputs

- `outReal` — Money Flow Index

## Parameters

- `optInTimePeriod` — Lookback window for summing money flow

## Implementation

TA-Lib Definition: [`mfi.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/mfi/mfi.c) · [`mfi.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/mfi/mfi.yaml)

| Native | File |
|--------|------|
| C | [`ta_MFI.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_MFI.c) |
| Rust | [`mfi.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/mfi.rs) |
| Java | [`Core_MFI.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/library/fragments/Core_MFI.java) |

TA-Lib is also available for Python, R and more using a [wrapper](https://ta-lib.org/wrappers/).

## Aliases

Money Flow Index

## See Also

RSI · AD · ADOSC

## References

- Gene Quong & Avrum Soudack, *Volume-Weighted RSI: Money Flow*, Technical Analysis of Stocks & Commodities, V.7:3 (March 1989)
