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

- Each line is `TA_MIDPRICE` over its own period, and Span A is `TA_MEDPRICE` of the other two lines. Span A halves the two midpoints after each has been rounded, rather than averaging the four extremes. The two spellings are the same number in real arithmetic and a different double in the last bit often enough to matter: on the 252-bar regression corpus the rate is 0% at the published 9/26 periods, 5.7% at 26/9, and 12.4% at 2/2, and on a four-decimal series at 3/5 it is 48%. The rate is not a function of the longer period alone: 9/26 and 26/9 share a 26-bar window and read 0% and 5.7%.
- The two spans are drawn `kijunPeriod` bars ahead of the bar that computed them. That is a display shift: it is reported through the display-shift call and changes nothing about the values, the lookback or the returned range. Every output is written at the bar that computed it.
- The lookback is the longest of the three periods less one. It is not the Senkou B period: nothing orders the three, so a base line longer than the second span dominates.
- The Chikou span, the close drawn backward, carries no computation and is not an output here: it is the input series with a display shift.

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
