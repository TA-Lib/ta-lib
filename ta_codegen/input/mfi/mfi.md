# MFI

## Summary

Money Flow Index: a volume-weighted momentum oscillator (0-100) comparing positive vs negative money flow over a period. A volume-based analog of RSI. >80 overbought, <20 oversold.

## Formula

TP = (High+Low+Close)/3; MF = TP*Volume, classed positive if TP>prevTP, negative if TP<prevTP, neither if equal. MFI = 100 * posSumMF/(posSumMF+negSumMF).

## Notes

- When the typical price is unchanged from the prior bar, that bar's money flow is counted as neither positive nor negative.
- A window in which no bar contributed any money flow — every typical price unchanged, or no volume traded — leaves the index undefined (0/0); 0 is returned. The result does not otherwise depend on the size of the money flow: scaling every volume, or quoting the instrument in a different unit, leaves the index unchanged.

## Inputs

- `inHigh` — High price of each bar
- `inLow` — Low price of each bar
- `inClose` — Close price of each bar
- `inVolume` — Volume of each bar

## Outputs

- `outReal` — Money Flow Index

## Parameters

- `optInTimePeriod` — Lookback window for summing money flow

## Aliases

Money Flow Index

## See Also

RSI · AD · ADOSC

## References

- Gene Quong & Avrum Soudack, *Volume-Weighted RSI: Money Flow*, Technical Analysis of Stocks & Commodities, V.7:3 (March 1989)
