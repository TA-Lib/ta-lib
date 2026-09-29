# EMV

## Summary

Ease of Movement: the bar-to-bar move of the high-low midpoint divided by a box ratio of volume to range, averaged over a trailing window. It is the numeric form of Richard W. Arms, Jr.'s Equivolume box.

The box ratio is positive whenever the bar traded and has a range, so the sign follows the midpoint move. A large positive value means price rose easily, on light volume relative to its range; a large negative value means it fell easily. Values near zero mean volume was heavy for the distance travelled, or price hardly moved.

The output scales with the instrument's volume and with the volume divisor, so its level is comparable only within one instrument at one divisor. Traders mostly watch its sign and its zero crossings.

## Formula

mid[i] = (high[i] + low[i]) / 2

box[i] = (volume[i] / D) / (high[i] - low[i])

raw[i] = (mid[i] - mid[i-1]) / box[i]

EMV[i] = ( sum_{k=i-N+1..i} raw[k] ) / N, N = optInTimePeriod, D = optInVolumeDivisor

A bar with no range or no volume has no box ratio and contributes 0. The next bar still measures its midpoint move from that bar.

## Notes

- The range is in price points. Achelis's text gives it in eighths of a point, the pre-decimal US quote unit; that reading is reached by multiplying the divisor by 8.
- The divisor is a pure output scale: doubling it doubles every value. Pick one that suits the instrument's volume.
- A period of 1 returns the unsmoothed one-bar values. Smoothing is a simple moving average; for an exponential one, apply `EMA` to this function's output at a period of 1.

## Inputs

- `inHigh` — High price of each bar
- `inLow` — Low price of each bar
- `inVolume` — Volume of each bar

## Outputs

- `outReal` — Ease of Movement, averaged over the window

## Parameters

- `optInTimePeriod` — Number of one-bar values in the simple moving average
- `optInVolumeDivisor` — Volume is divided by this before it forms the box ratio

## Aliases

Ease of Movement, EOM, Arms Ease of Movement

## See Also

MEDPRICE · MARKETFI · EFI · SMA · EMA

## References

- Richard W. Arms, Jr., *Volume Cycles in the Stock Market: Market Timing Through Equivolume Charting*, Dow Jones-Irwin, 1983.
- Steven B. Achelis, *Technical Analysis from A to Z*, 2nd edition, McGraw-Hill, 2000, "Ease of Movement".
- W. A. Thorp, [Arms' Ease of Movement: Adding Volume to the Equation](https://www.aaii.com/journal/article/arms-ease-of-movement-adding-volume-to-the-equation), *AAII Journal*, October 2001.
- StockCharts ChartSchool, [Ease of Movement (EMV)](https://chartschool.stockcharts.com/table-of-contents/technical-indicators-and-overlays/technical-indicators/ease-of-movement-emv).
