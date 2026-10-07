# PSO

## Summary

Premier Stochastic Oscillator: a short-period Fast %K, recentred on zero and rescaled, double-smoothed and then squashed into the range -1 to +1. Leibfarth's reading is that the plain stochastic spends most of its life pinned at one end or the other, so the extremes stop meaning anything; the two exponential passes strip the bar-to-bar noise out of it, and the squash gives back a scale on which the extremes are rare again. Readings beyond ±0.9 are the extremes, and ±0.2 the band Leibfarth watches for the crossing back toward the middle.

## Formula

K = STOCHF(high, low, close, fastKPeriod)   (Fast-K only)

NSK = 0.1 * (K - 50)

SS = EMA(EMA(NSK, emaPeriod), emaPeriod)

PSO = (e^SS - 1) / (e^SS + 1) = tanh(SS / 2)

## Notes

- The affine step comes before the smoothing, as the author's listing spells it. Smoothing first and recentring after is the same value in real arithmetic and differs in the last bit or two in doubles.
- The squash is computed as `tanh(SS/2)`, which is the published quotient rewritten. The quotient form is `inf/inf` once `SS` exceeds 709.78, which a bar whose close lies outside its own high/low range can reach; `tanh` saturates at ±1 instead.
- A Fast-K window whose range is zero reads 50, the midpoint, so a flat market reads PSO 0 rather than the near-extreme the Fast-K convention of 0 would give it. The flatness test is the one STOCHF applies, against the window's own extremes rather than a fixed band.
- Each exponential pass is seeded with a simple average of its own first inputs, the same seeding TA-Lib's EMA uses, and the second pass seeds on what the first publishes. `TA_SetUnstablePeriod(TA_FUNC_UNST_EMA, ...)` discards more of that warm-up, through both passes. Implementations seeding each pass from a single first sample differ over the transient and agree once it decays.
- Leibfarth parameterises the smoothing as the square root of a longer period, 25 in his article. The length is taken here directly, as an integer, because the sources that follow the square root disagree over how to round it.

## Inputs

- `inHigh` — High price series
- `inLow` — Low price series
- `inClose` — Close price series

## Outputs

- `outReal` — Premier Stochastic Oscillator, -1 to +1

## Parameters

- `optInFastK_Period` — Time period for building the Fast-K line
- `optInEMAPeriod` — Period of each of the two smoothing passes

## Aliases

premier stochastic oscillator, premier stochastic

## See Also

STOCHF · STOCH · SMI · WILLR

## References

- Lee Leibfarth, "Premier Stochastic Oscillator", *Technical Analysis of Stocks & Commodities*, v26:8 (August 2008), pp. 30-36
