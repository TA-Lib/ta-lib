# IMI

## Summary

Intraday Momentum Index: an RSI-like 0-100 oscillator built from the open-to-close body of each bar. Over a rolling window it ratios cumulative up-body moves against total up+down body moves.

## Formula

upsum = Σ(close-open) for bars with close>open; downsum = Σ(open-close) for bars with close<=open, over window [i-lookback, i]; IMI = 100 * upsum/(upsum+downsum)

## Inputs

- `inOpen` — Open price of each bar
- `inClose` — Close price of each bar

## Outputs

- `outReal` — IMI oscillator value, 0-100

## Parameters

- `optInTimePeriod` — Rolling window length for the up/down body sums

## Aliases

Intraday Momentum Index

## See Also

RSI
