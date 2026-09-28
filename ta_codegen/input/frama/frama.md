# FRAMA

## Summary

Fractal Adaptive Moving Average (John Ehlers): an EMA whose smoothing factor adapts each bar to the fractal dimension of the window, estimated from the high-low ranges of the window and of its two halves. A straight run gives dimension 1 and the output follows the price; dense congestion gives dimension 2 and very slow smoothing.

## Formula

P[t] = (High[t] + Low[t]) / 2
R1 = range of the newer half, R2 = range of the older half, R = range of the whole window
D = 1 + log2((R1 + R2) / R)
alpha = exp(-4.6 * (D - 1)), at most 1; alpha = 1 when R1 or R2 is 0
FRAMA[t] = alpha * P[t] + (1 - alpha) * FRAMA[t-1], seeded with P on the bar before the first output

## Notes

- Where either half of the window is flat, alpha is 1 and the output is the price; Ehlers' listing keeps the previous bar's dimension there instead.
- The period must be even; an odd period is rejected.

## Inputs

- `inHigh` — High price of each bar
- `inLow` — Low price of each bar

## Outputs

- `outReal` — Adaptive moving average line

## Parameters

- `optInTimePeriod` — Number of bars in the window, split into two equal halves

## Aliases

Fractal Adaptive Moving Average, FrAMA

## See Also

KAMA · MAMA · MEDPRICE

## References

- John F. Ehlers, "Fractal Adaptive Moving Averages", Technical Analysis of Stocks & Commodities V.23:10 (October 2005)
