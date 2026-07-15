# MINUS_DM

## Summary

Minus Directional Movement, the downward component of Wilder's directional movement system. Measures Wilder-smoothed downward price motion over the period. Higher -DM indicates stronger downward directional movement.

## Formula

diffP = high - prevHigh; diffM = prevLow - low
-DM1 = diffM if (diffM > 0 and diffP < diffM) else 0
period<=1: output raw -DM1 per bar.
period>1: seed = sum of first (period-1) -DM1; then Wilder smooth each bar:
-DM = prevMinusDM - prevMinusDM/period (+ -DM1 when the bar qualifies)

## Inputs

- `inPriceHL` — High and low price series

## Outputs

- `outReal` — Smoothed minus directional movement

## Parameters

- `optInTimePeriod` — Wilder smoothing period

## Implementation

TA-Lib Definition: [`minus_dm.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/minus_dm/minus_dm.c) · [`minus_dm.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/minus_dm/minus_dm.yaml)

| Native | File |
|--------|------|
| C | [`ta_MINUS_DM.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_MINUS_DM.c) |
| Rust | [`minus_dm.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/minus_dm.rs) |
| Java | [`Core_MINUS_DM.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/library/fragments/Core_MINUS_DM.java) |

TA-Lib is also available for Python, R and more using a [wrapper](https://ta-lib.org/wrappers/).

## Aliases

Minus Directional Movement, -DM

## See Also

PLUS_DM · MINUS_DI · PLUS_DI · DX · ADX · ADXR

## References

- J. Welles Wilder, *New Concepts in Technical Trading Systems*, Trend Research (ISBN 0894590278)
