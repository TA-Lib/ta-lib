# STC

## Summary

Schaff Trend Cycle (Doug Schaff): a MACD line passed twice through a stochastic, each pass smoothed by half. Bounded 0 to 100, read against 25 and 75: turning up from below 25 is bullish, turning down from above 75 bearish.

## Formula

MACD[t] = EMA(x, fast)[t] - EMA(x, slow)[t]

Frac1[t] = 100 * (MACD[t] - min(MACD, cycle)) / (max(MACD, cycle) - min(MACD, cycle))

PF[t] = PF[t-1] + 0.5 * (Frac1[t] - PF[t-1])

Frac2[t] = 100 * (PF[t] - min(PF, cycle)) / (max(PF, cycle) - min(PF, cycle))

STC[t] = PFF[t] = PFF[t-1] + 0.5 * (Frac2[t] - PFF[t-1])

min and max are taken over the last cycle values, current one included. When a range is zero, the fraction keeps its previous value (0 before any). Each smoother starts on its first input.

## Notes

- The smoothing factor is fixed at 0.5, as in Schaff's published code.
- The MACD line is TA-Lib's: both EMAs are seeded with a simple average over windows ending on the same bar. The published code runs the EMAs from the first bar of the data.
- If the slow period is set smaller than the fast period, the two are swapped, as in `MACD`.
- Being recursive, an output depends on how much history precedes it. The unstable period warms the two smoothers; the EMA unstable period warms the MACD line.

## Inputs

- `inReal` — Input series (typically close)

## Outputs

- `outReal` — Schaff Trend Cycle, from 0 to 100

## Parameters

- `optInFastPeriod` — Period of the fast EMA
- `optInSlowPeriod` — Period of the slow EMA
- `optInCyclePeriod` — Window of both stochastic stages

## Aliases

Schaff Trend Cycle, Schaff TC

## See Also

MACD · STOCHF · STOCHRSI

## References

- Doug Schaff, ["Releasing the Code to the Schaff Trend Cycle"](https://web.archive.org/web/20090418215759/mediaserver.fxstreet.com/Reports/99afdb5f-d41d-4a2c-802c-f5d787df886c/ebfbf387-4b27-4a0f-848c-039f4ab77c00.pdf), FX-Strategy.com, published on FXStreet.com, February 15, 2008. The EasyLanguage source, and the 23/50/10 defaults.
- Doug Schaff, "Catching Currency Moves with The Schaff Trend Cycle Indicator", *Chartpoint*, July/August 2002.
