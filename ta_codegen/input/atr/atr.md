# ATR

## Summary

Wilder-smoothed average of the True Range over a period, measuring price volatility regardless of direction. Higher ATR means greater volatility; no directional bias.

## Formula

TR_t = max(high-low, |prevClose-high|, |prevClose-low|)
ATR seed = simple average of first `period` TR values
ATR_t = (ATR_{t-1} * (period-1) + TR_t) / period

## Inputs

- `inPriceHLC` — High, low, close price series

## Outputs

- `outReal` — Average True Range value

## Parameters

- `optInTimePeriod` — Smoothing period

## Implementation

TA-Lib Definition: [`atr.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/atr/atr.c) · [`atr.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/atr/atr.yaml)

| Native | File |
|--------|------|
| C | [`ta_ATR.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_ATR.c) |
| Rust | [`atr.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/atr.rs) |
| Java | [`Core_ATR.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/library/fragments/Core_ATR.java) |

TA-Lib is also available for Python, R and more using a [wrapper](https://ta-lib.org/wrappers/).

## Aliases

Average True Range

## See Also

TRANGE · NATR · SMA · EMA

## References

- J. Welles Wilder, *New Concepts in Technical Trading Systems*, Trend Research (ISBN 0894590278)
