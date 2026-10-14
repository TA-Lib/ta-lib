# KVO

## Summary

Klinger Volume Oscillator: each bar's volume is signed by the direction of its high-low-close sum and weighted by where the bar's own range sits inside the cumulative range of the current trend run, then the resulting volume force is smoothed twice and differenced. Klinger's reading is that volume alone says how much trading happened but not what it accomplished, so he scales it by the share of the trend's accumulated range that this bar actually covered. The oscillator crosses zero as the balance of that force changes hands, and crosses its own trigger line earlier.

## Formula

hlc = high + low + close;  dm = high - low

trend = +1 if hlc rose, -1 if hlc fell, unchanged if hlc repeated

cm = previous dm + current dm on a trend change, otherwise previous cm + current dm

vf = volume * |2 * (dm / cm) - 1| * 100 * trend

KVO = EMA(vf, fastPeriod) - EMA(vf, slowPeriod)

Signal = EMA(KVO, signalPeriod)

## Notes

- The trend is **held** when the high-low-close sum repeats, which is the article's own tenet: the existing trend is maintained on equality. A reading that breaks the tie in either direction changes every later bar, because the cumulative range is reset by a trend change.
- The cumulative range is reset by every trend change, so two calls starting at different bars carry different range state only until the first trend change after both; what remains between them is the seed error of the three averages, which fades at the slow average's rate. KVO converges, but as a small difference of two large averages it needs more bars than one of them would.
- All three exponential averages are seeded from the volume force of the same anchor bar rather than from a simple average of their own first inputs, so one warm-up covers all three and the first KVO value is exactly zero. `TA_SetUnstablePeriod(TA_FUNC_UNST_EMA, ...)` discards more of that warm-up.
- The weight is the 1997 form, `|2*(dm/cm) - 1|`, which is what the December 1997 Traders' Tips print. Some later implementations use `|2*(dm/cm - 1)|`, which is a different series.
- A cumulative range of exactly zero leaves the volume force at zero rather than dividing.

## Inputs

- `inHigh` — High price series
- `inLow` — Low price series
- `inClose` — Close price series
- `inVolume` — Volume series

## Outputs

- `outKVO` — Klinger Volume Oscillator
- `outKVOSignal` — Trigger line, an exponential average of the oscillator

## Parameters

- `optInFastPeriod` — Period of the faster smoothing of the volume force
- `optInSlowPeriod` — Period of the slower smoothing of the volume force
- `optInSignalPeriod` — Smoothing period of the trigger line

## Aliases

klinger, klinger volume oscillator, klinger oscillator

## See Also

ADOSC · OBV · MFI · AD

## References

- Stephen J. Klinger, "Identifying Trends With Volume Analysis", *Technical Analysis of Stocks & Commodities*, v15:12 (December 1997)
