# TYPPRICE

## Summary

Typical Price: the average of the high, low, and close of each bar. A single representative price per period.

## Formula

out[i] = (High[i] + Low[i] + Close[i]) / 3

## Inputs

- `inHigh` — High price of each bar
- `inLow` — Low price of each bar
- `inClose` — Close price of each bar

## Outputs

- `outReal` — typical price per bar

## Aliases

Typical Price

## See Also

MEDPRICE · WCLPRICE · AVGPRICE
