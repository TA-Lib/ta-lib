# RVIR

## Summary

Relative Volatility Index, refined form: Donald Dorsey's 1995 revision of his own indicator, which runs the 1993 RVI over the daily highs and again over the daily lows and averages the two indices.

Output is bounded [0..100] and is interpreted like RVI: above 50 the highs and lows have been more volatile while rising than while falling, below 50 the reverse. Dorsey's stated reason for the revision is that a high and a low carry the day's range, so the pair answers the question the close alone can only approximate.

## Formula

    RVIR[i] = 0.5 * ( RVI(high)[i] + RVI(low)[i] )

Both legs use the same `optInTimePeriod` and `optInStdDevPeriod`, so they warm on the same bar.

## Notes

- RVIR is the 1995 revision; [`RVI`](/functions/rvi) is the 1993 version.

## Inputs

- `inHigh` — High price of each bar
- `inLow` — Low price of each bar

## Outputs

- `outReal` — The averaged index, in 0..100

## Parameters

- `optInTimePeriod` — Wilder smoothing period applied to both legs of both indices
- `optInStdDevPeriod` — Number of trailing values each standard deviation spans

## Implementation

TA-Lib Definition: [`rvir.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/rvir/rvir.c) · [`rvir.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/rvir/rvir.yaml)

| Native | File |
|--------|------|
| C | [`ta_RVIR.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_RVIR.c) |
| Rust | [`rvir.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/rvir.rs) |
| Java | [`Core_RVIR.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/fragments/Core_RVIR.java) |

TA-Lib is also available for Python, R and more using a [wrapper](/install/#wrappers).

## Aliases

Relative Volatility Index (1995 revision), RVIr

## See Also

RVI · STDDEV · ATR

## References

- Dorsey, Donald. "Refining the Relative Volatility Index." *Technical Analysis of Stocks & Commodities*, V.13:9 (September 1995), 388-391.
- Dorsey, Donald. "The Relative Volatility Index." *Technical Analysis of Stocks & Commodities*, V.11:6 (June 1993), 253-256.
