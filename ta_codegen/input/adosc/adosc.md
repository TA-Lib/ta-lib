# ADOSC

## Summary

Chaikin A/D Oscillator: the difference between a fast and a slow EMA of the Accumulation/Distribution line. Highlights momentum in accumulation/distribution volume flow. Positive/rising suggests accumulation; negative/falling suggests distribution.

## Formula

ad += ((close-low)-(high-close))/(high-low) * volume   (only when high>low)
fastEMA = fastk*ad + (1-fastk)*fastEMA,  fastk = 2/(optInFastPeriod+1)
slowEMA = slowk*ad + (1-slowk)*slowEMA,  slowk = 2/(optInSlowPeriod+1)
ADOSC = fastEMA - slowEMA

## Inputs

- `inHigh` — High price of each bar
- `inLow` — Low price of each bar
- `inClose` — Close price of each bar
- `inVolume` — Volume of each bar

## Outputs

- `outReal` — Fast-EMA minus slow-EMA of the A/D line

## Parameters

- `optInFastPeriod` — Period of the fast A/D EMA
- `optInSlowPeriod` — Period of the slow A/D EMA

## Aliases

Chaikin A/D Oscillator, Chaikin Oscillator

## See Also

AD · EMA

## References

- Marc Chaikin
