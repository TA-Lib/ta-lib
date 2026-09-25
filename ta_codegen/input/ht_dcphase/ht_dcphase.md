# HT_DCPHASE

## Summary

Hilbert Transform Dominant Cycle Phase: the instantaneous phase (in degrees) of the dominant market cycle, derived from a homodyne discriminator on a Hilbert-transformed, smoothed price. One real output per bar. Output is degrees, in the range −45 to 315 (a full 360° span).

## Interpretation

DCPhase is a rotating angle: it locates the current bar within the dominant price cycle the Hilbert-transform model has locked onto. 90° is the modeled cycle's high, 270° its low, and 0°/180° are the midpoints where the cycle is moving fastest. `HT_SINE` plots this angle as a sine wave, crossing its 45°-lead companion near those highs and lows. `HT_TRENDMODE` separately flags bars where the market isn't cycling at all.

## Inputs

- `inReal` — Price series to analyze

## Outputs

- `outReal` — Dominant cycle phase in degrees

## Aliases

Hilbert Transform Dominant Cycle Phase

## See Also

HT_DCPERIOD · HT_PHASOR · HT_SINE · HT_TRENDLINE · HT_TRENDMODE · MAMA · WMA

## References

- John F. Ehlers, *Rocket Science for Traders: Digital Signal Processing Applications*, John Wiley & Sons (ISBN 0471405671)
