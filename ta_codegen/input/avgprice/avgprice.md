# AVGPRICE

## Summary

Average Price: the arithmetic mean of each bar's open, high, low, and close. A price-transform overlap condensing OHLC into a single representative price.

## Formula

outReal[i] = (High[i] + Low[i] + Close[i] + Open[i]) / 4

## Inputs

- `inOpen` — Open price of each bar
- `inHigh` — High price of each bar
- `inLow` — Low price of each bar
- `inClose` — Close price of each bar

## Outputs

- `outReal` — Per-bar average of the four OHLC prices

## Aliases

Average Price

## See Also

MEDPRICE · TYPPRICE · WCLPRICE
