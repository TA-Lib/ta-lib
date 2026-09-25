# WCLPRICE

## Summary

Weighted Close Price: a per-bar price average giving the close double weight relative to high and low.

## Formula

$\text{WCLPRICE} = \dfrac{\text{High} + \text{Low} + 2\cdot\text{Close}}{4}$

## Inputs

- `inHigh` — High price of each bar
- `inLow` — Low price of each bar
- `inClose` — Close price of each bar

## Outputs

- `outReal` — Weighted close price per bar

## Aliases

Weighted Close Price, Weighted Close

## See Also

TYPPRICE · MEDPRICE · AVGPRICE
