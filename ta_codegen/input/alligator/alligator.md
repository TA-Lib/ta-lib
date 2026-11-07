# ALLIGATOR

## Summary

Bill Williams' Alligator (*Trading Chaos*, 1995): three smoothed moving averages of the median price. The Jaw, Teeth and Lips are each drawn a few bars ahead of the bar that computed it. When the three lines are intertwined the alligator sleeps and the market has no trend; when they fan out in order, Lips above Teeth above Jaw or the reverse, it is feeding and a trend is under way. Price crossing back into the lines is read as the trend tiring.

The Alligator is a leg of Williams' Profitunity system, alongside the Awesome Oscillator ([`AO`](/functions/ao)) and the Accelerator/Decelerator ([`AC`](/functions/ac)).

## Formula

median_t = ( high_t + low_t ) / 2  
Jaw_t = SMMA(median, jawPeriod)_t  
Teeth_t = SMMA(median, teethPeriod)_t  
Lips_t = SMMA(median, lipsPeriod)_t

SMMA is Wilder's smoothed moving average ([`RMA`](/functions/rma)): seeded with the simple average of its first `n` values, then SMMA_t = SMMA_t−1 + ( x_t − SMMA_t−1 ) / n.

## Notes

- Each line is `TA_RMA` of `TA_MEDPRICE` over its own period. Williams draws the lines ahead of price; that is a display shift: `TA_ALLIGATOR_DisplayShift` reports each line's shift, and it changes nothing about the values, the lookback or the returned range. Every output is written at the bar that computed it, so the last `shift` values of a line are the part a chart draws past the last bar.
- All three lines start at the bar of the longest period, and each shorter line seeds over the bars its own lookback reaches back from there, not at the first bar of the series. A chart that seeds every line at the first bar draws different short lines until the seed has decayed; setting the `RMA` unstable period closes the gap.

## Inputs

- `inHigh` — High price of each bar
- `inLow` — Low price of each bar

## Outputs

- `outJaw` — Jaw: the slowest line, drawn ahead by the jaw shift
- `outTeeth` — Teeth: the middle line, drawn ahead by the teeth shift
- `outLips` — Lips: the fastest line, drawn ahead by the lips shift

## Parameters

- `optInJawPeriod` — Number of bars in the jaw's smoothed moving average.
- `optInJawShift` — Bars ahead of the computing bar at which a chart draws the jaw.
- `optInTeethPeriod` — Number of bars in the teeth's smoothed moving average.
- `optInTeethShift` — Bars ahead of the computing bar at which a chart draws the teeth.
- `optInLipsPeriod` — Number of bars in the lips' smoothed moving average.
- `optInLipsShift` — Bars ahead of the computing bar at which a chart draws the lips.

## Aliases

williams alligator, bill williams alligator, jaw teeth lips

## See Also

RMA · MEDPRICE · AO · AC · ICHIMOKU

## References

- Bill Williams, *Trading Chaos*, 1995
- Bill Williams, *New Trading Dimensions*, 1998
- [MetaTrader 5 — Alligator](https://www.metatrader5.com/en/terminal/help/indicators/bw_indicators/alligator)
