# MA

## Summary

Generic moving-average dispatcher that forwards the job to the MA implementation selected by optInMAType. Single uniform interface over all TA-Lib moving averages.

## Formula

outReal = MA_of_type(optInMAType)(inReal, optInTimePeriod); default type = SMA

## Notes

- A period of 1 performs no smoothing for every MAType: the output is a copy of the input.
- `TA_MAType_DISABLED` bypasses smoothing explicitly, for any period: the output is a copy of the input with a lookback of 0. Every function that takes an MAType parameter accepts it.
- `TA_MAType_DEFAULT` selects the documented default of the parameter it is passed to — SMA here, EMA for APO, PPO and PVO. Every function that takes an MAType parameter accepts it.

## Inputs

- `inReal` — Series to average

## Outputs

- `outReal` — Selected moving average of the input

## Parameters

- `optInTimePeriod` — Averaging window length
- `optInMAType` — Which moving-average algorithm to dispatch to

## Aliases

Moving Average, MovingAverage

## See Also

SMA · EMA · WMA · DEMA · TEMA · TRIMA · KAMA · MAMA · T3 · HMA · ZLEMA · RMA
