# BOP

## Summary

Balance Of Power compares where the close sits relative to the open, normalized by the bar's high-low range. A per-bar oscillator with no smoothing. Positive: close above open (buyers dominated); negative: sellers dominated.

## Formula

BOP = (Close - Open) / (High - Low)

## Inputs

- `inOpen` — Open price of each bar
- `inHigh` — High price of each bar
- `inLow` — Low price of each bar
- `inClose` — Close price of each bar

## Outputs

- `outReal` — Balance of Power value per bar

## Aliases

Balance Of Power, Balance of Power
