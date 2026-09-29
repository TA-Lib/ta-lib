# MCGD

## Summary

McGinley Dynamic (John R. McGinley, Jr.): a moving average whose speed adjusts to the market. Each bar closes a fraction `1 / (N * (x/MD)^4)` of the gap to the price: less than `RMA`'s `1/N` while the price is above the line, more while it is below, so the line tracks falling prices faster than rising ones.

McGinley suggests a period of about 60% of the simple moving average being emulated: a Dynamic of 12 to follow a 20-bar average.

## Formula

N = optInTimePeriod

MD[0] = x[0]

for i >= 1:  MD[i] = MD[i-1] + ( x[i] - MD[i-1] ) / ( N * ( x[i] / MD[i-1] )^4 )

The first output is at bar N-1.

## Notes

- Where the price is 0, or so small against the line that the step overflows, the step is undefined and the line keeps its previous value. A line at 0 stays at 0.
- Being recursive, an output depends on how much history precedes it. Close to the price the seed's influence decays by a factor of `1 - 1/N` per bar, as in `RMA`; the unstable period is how much of the warm-up to discard.
- The line is scale-equivariant but meant for positive prices: a series crossing zero sends it off to meaningless values, and a single bar far enough below the line can take the line to 0 or below it.
- Some implementations seed with the simple average of the first N bars. They agree with this one only once the seed's influence has decayed.
- Some implementations write the step's denominator as `0.6 * P * (x / MD)^4`. That is this function at a period of `0.6 * P`, when that is an integer.

## Inputs

- `inReal` — Data on which to compute the average

## Outputs

- `outReal` — McGinley Dynamic line

## Parameters

- `optInTimePeriod` — The N of the step's denominator

## Aliases

McGinley Dynamic, MD, MGD

## See Also

RMA · EMA · KAMA · SMA

## References

- **John R. McGinley, Jr., "McGinley Dynamics", MTA Journal (Market Technicians Association), Summer-Fall 1997, pp. 15-18.** The original definition, with the fourth-power adjustment and the 60% rule for choosing N.
