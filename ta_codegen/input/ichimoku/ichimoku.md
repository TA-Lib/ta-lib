# ICHIMOKU

## Summary

Ichimoku Kinko Hyo, "one glance equilibrium chart": four lines built from highs and lows alone. Goichi Hosoda's reading is that a market's balance is visible without any smoothing — each line is the midpoint of a window, the mean of its highest high and its lowest low, so it marks the level at which that stretch of trading was evenly divided. The conversion line turns fastest, the base line is the reference, and the two leading spans are drawn ahead of price, where the band between them is read as support or resistance before it is reached.

## Formula

MID(n) = ( highest high over n bars + lowest low over n bars ) / 2

Tenkan-sen = MID(tenkanPeriod)

Kijun-sen = MID(kijunPeriod)

Senkou Span A = ( Tenkan-sen + Kijun-sen ) / 2

Senkou Span B = MID(senkouBPeriod)

## Notes

- Each line is `TA_MIDPRICE` over its own period, and Span A is `TA_MEDPRICE` of the Tenkan and Kijun lines: it halves two midpoints that are already rounded. Averaging the four extremes in one step is the same number in real arithmetic and can differ in the last bit.
- The two spans are drawn `kijunPeriod` bars ahead of the bar that computed them. That is a display shift: `TA_ICHIMOKU_DisplayShift` reports it, and it changes nothing about the values, the lookback or the returned range. Every output is written at the bar that computed it, so the last `kijunPeriod` span values are the cloud a chart draws past the last bar.
- Hosoda's 26 counts the current bar, which is a chart offset of 25. QuantConnect LEAN, pandas-ta and MetaTrader offset by 26, and that is what the display shift reports at the default periods; a caller who wants Hosoda's count draws the spans one bar earlier.
- The lookback is the longest of the three periods less one. Nothing orders the three, so any of them can be the longest.
- The Chikou span is the close drawn `kijunPeriod` bars back. It carries no computation and is not an output. Its usual signal, the close above the close `n` bars earlier, is `TA_MOM(close, n) > 0`.

## Inputs

- `inHigh` — High price series
- `inLow` — Low price series

## Outputs

- `outTenkanSen` — Conversion line
- `outKijunSen` — Base line
- `outSenkouSpanA` — First leading span, drawn ahead by the base period
- `outSenkouSpanB` — Second leading span, drawn ahead by the base period

## Parameters

- `optInTenkanPeriod` — Period of the conversion line
- `optInKijunPeriod` — Period of the base line, and the forward shift of the two spans
- `optInSenkouBPeriod` — Period of the second leading span

## Aliases

ichimoku, ichimoku kinko hyo, ichimoku cloud, kumo

## See Also

MIDPRICE · MEDPRICE · SAR · BBANDS

## References

- Goichi Hosoda, *Ichimoku Kinko Hyo*, 1969
