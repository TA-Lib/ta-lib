# CG

## Summary

John Ehlers' Center of Gravity oscillator: the balance point of the last `optInTimePeriod` values, each weighted by its value and placed at its position counting back from the current bar, which is position 1, negated so that it rises with price. It is smooth and has essentially no lag. Ehlers reads turning points from it and trades its crossings with a copy of itself delayed by one bar. For a positive series it stays between -optInTimePeriod and -1, and a flat window sits at the midpoint, -(optInTimePeriod+1)/2.

## Formula

$$
\mathrm{CG}_t = -\frac{\sum_{i=0}^{n-1} (i+1)\,X_{t-i}}{\sum_{i=0}^{n-1} X_{t-i}}, \text{ or } -\frac{n+1}{2} \text{ when } \sum_{i=0}^{n-1} X_{t-i} = 0
$$

where $X$ is the input series, $n$ is `optInTimePeriod` and $i$ counts bars back from the current bar, which carries the smallest weight.

## Notes

- Ehlers' default input is the median price (H+L)/2: pass the output of MEDPRICE to reproduce it.
- His 2004 book presents the same oscillator shifted up by (optInTimePeriod+1)/2, so that a flat window reads 0: add (optInTimePeriod+1)/2 to the output to obtain that form.
- Where the window sums to exactly zero the author's listing keeps its previous value; TA-Lib returns -(optInTimePeriod+1)/2, so every value depends on its own window alone.

## Inputs

- `inReal` — The series to measure

## Outputs

- `outReal` — Negated center of gravity of the window

## Parameters

- `optInTimePeriod` — Number of bars in the window

## Aliases

Center of Gravity Oscillator, COG

## See Also

WMA · MEDPRICE · CTI

## References

- John F. Ehlers, "The CG Oscillator", MESA Software, [author's PDF](https://mesasoftware.com/papers/TheCGOscillator.pdf); published as "The Center Of Gravity Oscillator", *Technical Analysis of Stocks & Commodities*, May 2002
- John F. Ehlers, *Cybernetic Analysis for Stocks and Futures*, Wiley, 2004, chapter 5
