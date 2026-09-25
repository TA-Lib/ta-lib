# MAMA

## Summary

MESA Adaptive Moving Average: an adaptive EMA whose smoothing factor is driven by the dominant-cycle phase rate measured with a Hilbert transform. Emits two lines, MAMA and its slower follower FAMA. MAMA crossing above FAMA is bullish; crossing below is bearish.

## Formula

phase = atan(Q1/I1) in degrees; deltaPhase = max(1, prevPhase - phase)
alpha = max(fastLimit/deltaPhase, slowLimit) if deltaPhase>1 else fastLimit
MAMA = alpha*price + (1-alpha)*MAMA_prev
FAMA = (alpha/2)*MAMA + (1-alpha/2)*FAMA_prev

## Inputs

- `inReal` — Price series to smooth

## Outputs

- `outMAMA` — Adaptive moving average (fast line)
- `outFAMA` — Following adaptive moving average, using half the alpha (slow line)

## Parameters

- `optInFastLimit` — Upper bound on the adaptive smoothing factor
- `optInSlowLimit` — Lower bound on the adaptive smoothing factor

## Aliases

MESA Adaptive Moving Average, Ehlers MAMA

## See Also

MA · WMA · HT_DCPERIOD

## References

- John F. Ehlers, *Rocket Science for Traders: Digital Signal Processing Applications*, John Wiley & Sons (ISBN 0471405671)
