# HT_SINE

## Summary

Hilbert Transform SineWave: derives the dominant-cycle phase from price and emits its sine plus a 45-degree-lead sine. The two curves cross near cycle turning points. outSine and outLeadSine crossing marks cycle turning points.

## Inputs

- `inReal` — Source price series

## Outputs

- `outSine` — Sine of the dominant-cycle phase
- `outLeadSine` — Sine of the phase advanced 45 degrees (lead)

## Aliases

Hilbert Transform SineWave, Ehlers SineWave, SineWave Indicator

## See Also

HT_DCPHASE · HT_DCPERIOD · HT_PHASOR · HT_TRENDMODE · MAMA

## References

- John F. Ehlers, *Rocket Science for Traders: Digital Signal Processing Applications*, John Wiley & Sons (ISBN 0471405671)
