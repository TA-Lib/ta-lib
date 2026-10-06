# FISHER

## Summary

Ehlers' Fisher Transform: an oscillator that reshapes the midpoint's position in its rolling channel into a near-normal distribution. Extreme values become rare and turning points sharp, where a plain channel position spends much of its time pinned near the edges.

The output is unbounded and centred on zero. A peak or trough marks a likely turn, and the Fisher line crossing its Trigger, the same line one bar later, is the author's entry signal.

## Formula

With `P = (high + low) / 2` and `MaxP`, `MinP` its highest and lowest value over the last `optInTimePeriod` bars:

    r       = ( P - MinP ) / ( MaxP - MinP ), or 0.5 when MaxP = MinP
    v       = 0.66 * ( r - 0.5 ) + 0.67 * v[prev]
    v       = 0.999 if v > 0.99, -0.999 if v < -0.99
    Fisher  = 0.5 * ln( ( 1 + v ) / ( 1 - v ) ) + 0.5 * Fisher[prev]
    Trigger = Fisher[prev]

`v` and `Fisher` start from 0, so the first Trigger is 0, and the limited `v` is the one the next bar reads.

## Notes

- A window whose midpoints are all equal takes the neutral position, so a market that does not move decays toward 0. The original divides by the zero range there.

## Inputs

- `inHigh` — High price of each bar
- `inLow` — Low price of each bar

## Outputs

- `outFisher` — Fisher Transform value
- `outTrigger` — Fisher value of the previous bar

## Parameters

- `optInTimePeriod` — Number of bars in the channel the midpoint is located in

## Aliases

Fisher Transform, Ehlers Fisher Transform

## See Also

STOCHF · WILLR · MIDPRICE · IBS

## References

- John F. Ehlers, "Using The Fisher Transform", *Technical Analysis of Stocks & Commodities*, V.20:11 (November 2002), 40-42
- John F. Ehlers, *Cybernetic Analysis for Stocks and Futures*, Wiley, 2004
