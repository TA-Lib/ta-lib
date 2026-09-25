# RVI

## Summary

Relative Volatility Index: Donald Dorsey's volatility oscillator, built exactly like RSI except that the quantity routed to the up and down buckets is the rolling standard deviation of price rather than the size of the move. The direction of the close-to-close change still decides which bucket a bar feeds.

Bounded in 0..100. High values mean the recent volatility arrived mostly on up bars, low values that it arrived mostly on down bars. Dorsey proposed it as a confirming filter rather than a stand-alone signal: take a long entry only while RVI is above 50, a short only while it is below.

## Formula

With `S` the standard deviation of the last `optInStdDevPeriod` values of `inReal`, and `C` the input series:

    U[i] = S[i] if C[i] > C[i-1], else 0
    D[i] = S[i] if C[i] < C[i-1], else 0
    Up   = RMA(U, optInTimePeriod)
    Down = RMA(D, optInTimePeriod)
    RVI  = 100 * Up / ( Up + Down ), or 50 when Up + Down = 0

`RMA` is Wilder's smoothed moving average, seeded with the simple average of its first `optInTimePeriod` inputs. A bar whose close equals the previous close feeds neither bucket.

## Notes

- RVI is the 1993 version; [`RVIR`](/functions/rvir) is the 1995 revision.
- A tie contributes to neither bucket, matching RSI's treatment of an unchanged close. Descriptions that write the denominator as a smoothed `S` instead of `Up + Down` are counting ties as down bars, which is a different indicator.
- Unrelated to the Relative Vigor Index, which several platforms also abbreviate RVI.

## Inputs

- `inReal` — Source price/value series, canonically the close

## Outputs

- `outReal` — Relative Volatility Index value

## Parameters

- `optInTimePeriod` — Wilder smoothing period applied to both legs
- `optInStdDevPeriod` — Number of trailing values the standard deviation spans

## Aliases

Relative Volatility Index, RVIorig

## See Also

RSI · RMA · STDDEV · CMO

## References

- Donald Dorsey, "The Relative Volatility Index", *Technical Analysis of Stocks & Commodities*, V.11:6 (June 1993), 253-256
- Donald Dorsey, "Refining the Relative Volatility Index", *Technical Analysis of Stocks & Commodities*, V.13:9 (September 1995), 388-391
- [Chart manual: Relative Volatility Index](https://user42.tuxfamily.org/chart/manual/Relative-Volatility-Index.html)
- [DXcharts: Relative Volatility Index](https://devexperts.com/dxcharts/kb/docs/relative-volatility-index-rvi)
