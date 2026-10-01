# KSTEXT

## Summary

Know Sure Thing with selectable moving averages: Pring's weighted sum of four smoothed rates of change, with a signal line. One MA type smooths the four legs and another the signal line.

`KST` is Pring's definition, with simple averages throughout. Formulas published since then smooth the legs exponentially, and Pring allows a simple or an exponential signal line. The reading is the same as `KST`'s: above zero the combined momentum is positive, and the usual signals are the line crossing its signal line and the line changing direction.

## Formula

ROC_k[t] = (price[t] / price[t - X_k] - 1) × 100

RCMA_k = MA_roc(ROC_k, A_k), for k = 1..4

KST = 1 × RCMA_1 + 2 × RCMA_2 + 3 × RCMA_3 + 4 × RCMA_4

Signal = MA_signal(KST, S)

X_k = optInROC{k}Period, A_k = optInMA{k}Period, S = optInSignalPeriod

MA_roc uses optInROCMAType, MA_signal uses optInSignalMAType

## Notes

- With both MA types set to `TA_MAType_SMA` the outputs are those of `KST`.
- Both outputs start at the first bar where the signal line exists. A signal period of 1 disables signal-line smoothing for every signal MAType: the signal is then a copy of the line.
- The unstable period of a selected MA type lengthens the lookback, for the legs and for the signal line separately.
- `TA_MAType_MAMA` ignores its period argument, so where it is selected every period above 1 gives the same average.
- Each rate of change follows `ROC`: a zero price in the denominator makes that term 0.
- The weights go with leg position, not with the length of the rate of change.

## Inputs

- `inReal` — Source price series (canonically the close)

## Outputs

- `outKST` — Know Sure Thing line
- `outKSTSignal` — Signal line: MA of the line

## Parameters

- `optInROC1Period` — Rate-of-change period of leg 1 (weight 1)
- `optInROC2Period` — Rate-of-change period of leg 2 (weight 2)
- `optInROC3Period` — Rate-of-change period of leg 3 (weight 3)
- `optInROC4Period` — Rate-of-change period of leg 4 (weight 4)
- `optInMA1Period` — Period of the MA smoothing leg 1
- `optInMA2Period` — Period of the MA smoothing leg 2
- `optInMA3Period` — Period of the MA smoothing leg 3
- `optInMA4Period` — Period of the MA smoothing leg 4
- `optInSignalPeriod` — Period of the signal-line MA
- `optInROCMAType` — MA type smoothing the four legs
- `optInSignalMAType` — MA type for the signal line

## Aliases

Know Sure Thing Extended, KST with controllable MA type

## See Also

KST · ROC · MA · MACDEXT · COPPOCK

## References

- Martin J. Pring, "Summed Rate Of Change (KST)", *Technical Analysis of Stocks & Commodities*, V.10:9, September 1992, pp. 365-369.
- Martin J. Pring, "Identifying Trends With The KST Indicator", *Technical Analysis of Stocks & Commodities*, V.10:10, October 1992, pp. 420-424.
