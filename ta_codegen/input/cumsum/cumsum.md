# CUMSUM

## Summary

Cumulative Sum: the running total of a series from the anchor bar forward. A math primitive rather than an indicator — it is the one operation between the shipped corpus and three named classical breadth indicators: the A/D Line is `CUMSUM(SUB(advances, declines))`, the A/D Volume Line is `CUMSUM(SUB(advancingVolume, decliningVolume))`, and the McClellan Summation Index is `CUMSUM` of the McClellan Oscillator.

[`SUM`](/functions/sum) is a *rolling window* over `optInTimePeriod` bars; `CUMSUM` has no window — every bar since the anchor contributes.

## Formula

`out[j] = inReal[startIdx] + inReal[startIdx+1] + … + inReal[startIdx+j]`

Left-to-right in one double, no compensation — the same plain `+=` convention the shipped accumulators (`AD`, `OBV`) use.

**The accumulator re-seeds at the anchor.** `CUMSUM(3, 7, x)` starts its total at `x[3]`; it does not warm up from `x[0]`. This is the published contract of the indicators built on it (StockCharts: only the A/D Line's *shape* carries meaning, the first value is "simply Net Advances for one period") and the convention of every shipped path-dependent function. The `path_dependent` flag declares exactly this class.

## Inputs

- `inReal` — Source series (canonically a per-bar net figure, e.g. advances − declines)

## Outputs

- `outReal` — Running total since the anchor bar

## Notes

- Lookback 0: `outBegIdx = startIdx`, one output per input bar. Streaming state is a single accumulator, so a peek commits nothing by construction.
- The sum is uncompensated. A Kahan or Neumaier variant would diverge from `AD`'s own convention, which this follows.

## Implementation

TA-Lib Definition: [`cumsum.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/cumsum/cumsum.c) · [`cumsum.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/cumsum/cumsum.yaml)

| Native | File |
|--------|------|
| C | [`ta_CUMSUM.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_CUMSUM.c) |
| Rust | [`cumsum.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/cumsum.rs) |
| Java | [`Core_CUMSUM.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/fragments/Core_CUMSUM.java) |

TA-Lib is also available for Python, R and more using a [wrapper](/install/#wrappers).
