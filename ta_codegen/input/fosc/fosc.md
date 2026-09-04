# FOSC

## Summary

Forecast Oscillator: the percentage by which the close deviates from the Time Series Forecast that was made one bar earlier for the current bar.

Positive means price came in above what the regression projected, negative below; the value oscillates around zero and crosses it whenever price meets its own forecast. Persistent readings far from zero say the trend is running ahead of, or lagging, its regression line.

## Formula

FOSC[t] = 100 * (P[t] - TSF[t-1]) / P[t], where TSF[t-1] is the Time Series Forecast fitted over the N bars ending at t-1 and evaluated one x-step beyond that window — the forecast for bar t made without seeing it.

## Notes

- Several vendors publish a "Chande Forecast Oscillator (CFO)" that compares the close to the regression value of the window *ending at the same bar*, with no lag. FOSC is the lagged form Chande and Achelis describe.
- The default window is Chande's own suggestion, shorter than the one TA-Lib's TSF and LINEARREG default to.

## Inputs

- `inReal` — Source price/value series

## Outputs

- `outReal` — Percentage deviation of the close from the previous bar's forecast

## Parameters

- `optInTimePeriod` — Number of bars in the regression window

## Implementation

TA-Lib Definition: [`fosc.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/fosc/fosc.c) · [`fosc.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/fosc/fosc.yaml)

| Native | File |
|--------|------|
| C | [`ta_FOSC.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_FOSC.c) |
| Rust | [`fosc.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/fosc.rs) |
| Java | [`Core_FOSC.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/fragments/Core_FOSC.java) |

TA-Lib is also available for Python, R and more using a [wrapper](/install/#wrappers).

## Aliases

Forecast Oscillator

## See Also

TSF · LINEARREG · CMOU

## References

- Tushar S. Chande and Stanley Kroll, *The New Technical Trader*, John Wiley & Sons (ISBN 0471597805)
- Steven B. Achelis, *Technical Analysis from A to Z*, page 147
