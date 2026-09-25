# VWMA

## Summary

Volume Weighted Moving Average: the mean price over a trailing window of `optInTimePeriod` bars, each bar weighted by its own volume. Heavily traded bars pull the average toward their price; quiet bars barely move it.

Read like any moving average — price above is strength, below is weakness. Against a plain [`SMA`](/functions/sma) of the same window it leads on high-volume moves and lags on low-volume drift, so the gap between the two lines measures how volume-confirmed a move is.

It has no attributable inventor — charting-package folklore — and every published definition agrees, so there is no competing variant.

## Formula

VWMA = ( sum_{k=t-N+1..t} P[k] * V[k] ) / ( sum_{k=t-N+1..t} V[k] ), N = optInTimePeriod

Equivalently, SMA(P * V, N) / SMA(V, N), the composition TradingView documents for `ta.vwma`. In TA-Lib the two are bit-identical for N of 2 or more, up to the first window whose volume is entirely zero. There is no seeding and no recursion, hence no unstable period.

## Notes

- A period of 1 performs no smoothing: the output is a copy of the input, whatever the volume.
- Volume is expected to be non-negative. Individual zero-volume bars are fine: a bar that did not trade simply carries no weight, and the average stays well defined as long as some bar in the window has volume. At a period of 2 or more, a window in which *every* volume is zero has no weights at all; the weighted mean is then undefined and that element is NaN, as it is in every other implementation. Series carrying no volume on any bar, such as cash-index feeds, are outside what a volume-weighted average can describe — use SMA or WMA there.

## Inputs

- `inReal` — Source price series, close by convention
- `inVolume` — Volume of each bar

## Outputs

- `outReal` — Volume weighted moving average of the input

## Parameters

- `optInTimePeriod` — Number of bars in the weighting window

## Aliases

Volume Weighted Moving Average

## See Also

SMA · WMA · MA · OBV

## References

- VWMA has no separately documented originator; its definition is uniform across charting packages.
- MotiveWave, *Volume Weighted Moving Average* study documentation — the closest thing to a primary definition.
- TradingView, *Volume Weighted Moving Average (VWMA)* — documents the equivalence with SMA(price * volume) / SMA(volume).
