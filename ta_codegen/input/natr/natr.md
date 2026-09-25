# NATR

## Summary

Average True Range expressed as a percentage of the current close, making volatility comparable across price levels and securities. Same computation as ATR, then normalized by close. Higher values mean greater relative volatility; unit is percent of price.

## Formula

NATR = (ATR / Close) * 100
ATR: first value = SMA of TRANGE over period; then Wilder smoothing ATR_t = (ATR_{t-1}*(period-1) + TR_t) / period

## Inputs

- `inHigh` — High price of each bar
- `inLow` — Low price of each bar
- `inClose` — Close price of each bar

## Outputs

- `outReal` — ATR as a percentage of the close

## Parameters

- `optInTimePeriod` — Smoothing period for the true range average

## Aliases

Normalized Average True Range

## See Also

ATR · TRANGE · SMA

## References

- John Forman
