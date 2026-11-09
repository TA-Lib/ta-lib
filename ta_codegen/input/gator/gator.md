# GATOR

## Summary

Bill Williams' Gator Oscillator: how far apart the three Alligator lines are, as two histograms. The upper bars are the gap between Jaw and Teeth, the lower bars the gap between Teeth and Lips drawn below zero. Both growing is the alligator eating, a trend under way; both shrinking toward zero is it falling asleep, the lines converging in a range. One side growing while the other shrinks is it waking or sated, the turn from one state into the other.

It is read under the Alligator ([`ALLIGATOR`](/functions/alligator)), the Profitunity leg it measures.

## Formula

Jaw, Teeth and Lips are the three [`ALLIGATOR`](/functions/alligator) lines, each drawn its own shift ahead. The histograms compare them where a chart draws them:

Upper = | Jaw − Teeth |  
Lower = −| Teeth − Lips |

## Notes

- Each line is drawn ahead by its own shift, so the line drawn further ahead is read back by the difference of the two shifts: at the default shifts Upper_t = | Jaw_t−3 − Teeth_t | and Lower_t = −| Teeth_t−2 − Lips_t |. `TA_GATOR_DisplayShift` reports where a chart draws each histogram, the smaller of its two lines' shifts; it changes nothing about the values or the returned range.
- Each histogram computes its own lines, each seeded over the bars its own lookback reaches back, so the Teeth an output reads can differ from the `TA_ALLIGATOR` Teeth by the seed until it has decayed; setting the `RMA` unstable period closes the gap.
- A zero Lower is `+0.0`.

## Inputs

- `inHigh` — High price of each bar
- `inLow` — Low price of each bar

## Outputs

- `outUpper` — Gap between the Jaw and the Teeth, zero or above
- `outLower` — Gap between the Teeth and the Lips, zero or below

## Parameters

- `optInJawPeriod` — Number of bars in the jaw's smoothed moving average.
- `optInJawShift` — Bars ahead of the computing bar at which a chart draws the jaw.
- `optInTeethPeriod` — Number of bars in the teeth's smoothed moving average.
- `optInTeethShift` — Bars ahead of the computing bar at which a chart draws the teeth.
- `optInLipsPeriod` — Number of bars in the lips' smoothed moving average.
- `optInLipsShift` — Bars ahead of the computing bar at which a chart draws the lips.

## Aliases

gator oscillator, williams gator

## See Also

ALLIGATOR · AO · AC · RMA

## References

- Bill Williams, *New Trading Dimensions*, 1998
