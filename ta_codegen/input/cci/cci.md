# CCI

## Summary

Commodity Channel Index: measures the current typical price relative to its simple moving average, scaled by mean absolute deviation. Momentum oscillator flagging overbought/oversold extremes. CCI > +100 overbought; CCI < -100 oversold.

## Formula

TP_i = (High_i + Low_i + Close_i)/3
SMA = (1/N) * sum(TP over N bars)
meanDev = (1/N) * sum(|TP - SMA| over N bars)
CCI = (TP_last - SMA) / (0.015 * meanDev)

## Inputs

- `inHigh` — High price of each bar
- `inLow` — Low price of each bar
- `inClose` — Close price of each bar

## Outputs

- `outReal` — CCI value per bar

## Parameters

- `optInTimePeriod` — Number of bars in the averaging/deviation window

## Aliases

Commodity Channel Index

## See Also

TYPPRICE · SMA

## References

- Donald Lambert
