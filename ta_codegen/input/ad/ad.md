# AD

## Summary

Chaikin Accumulation/Distribution Line, a cumulative volume-flow indicator. Sums a volume-weighted money-flow multiplier per bar to gauge buying vs. selling pressure. Rising line = accumulation (buying pressure); falling = distribution.

## Formula

MFM = ((close-low) - (high-close)) / (high-low); AD_t = AD_{t-1} + MFM_t * volume_t (running sum, seeded at 0)

## Inputs

- `inHigh` — High price of each bar
- `inLow` — Low price of each bar
- `inClose` — Close price of each bar
- `inVolume` — Volume of each bar

## Outputs

- `outReal` — Cumulative A/D line value per bar

## Aliases

Chaikin A/D Line, Accumulation/Distribution Line, Accumulation Distribution

## See Also

ADOSC · OBV
