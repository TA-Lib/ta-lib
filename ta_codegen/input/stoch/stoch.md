# STOCH

## Summary

Slow Stochastic oscillator: locates the close within the high-low range over a lookback period, then double-smooths it. Returns the Slow-%K and Slow-%D lines. SlowK/SlowD > 80 overbought, < 20 oversold; %K crossing %D signals momentum shifts.

## Formula

FastK = 100*(Close - LL_n)/(HH_n - LL_n), n = FastK_Period (LL/HH = lowest low / highest high over n)
SlowK = MA(FastK, SlowK_Period, SlowK_MAType)
SlowD = MA(SlowK, SlowD_Period, SlowD_MAType)

## Notes

- When the high-low range over the window is zero, the raw stochastic is set to 0 instead of being undefined.

## Inputs

- `inPriceHLC` — High/Low/Close series; range from High/Low, level from Close

## Outputs

- `outSlowK` — Raw FastK smoothed by SlowK_Period MA
- `outSlowD` — Signal line: SlowK smoothed by SlowD_Period MA

## Parameters

- `optInFastK_Period` — Lookback window for the raw %K high-low range
- `optInSlowK_Period` — Smoothing period turning FastK into SlowK
- `optInSlowK_MAType` — MA type used to smooth into SlowK
- `optInSlowD_Period` — Smoothing period for the SlowD signal line
- `optInSlowD_MAType` — MA type used for the SlowD line

## Implementation

TA-Lib Definition: [`stoch.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/stoch/stoch.c) · [`stoch.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/stoch/stoch.yaml)

| Native | File |
|--------|------|
| C | [`ta_STOCH.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_STOCH.c) |
| Rust | [`stoch.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/src/ta_func/stoch.rs) |
| Java | [`Core_STOCH.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/Core_STOCH.java) |

TA-Lib is also available for Python, R and more using a [wrapper](https://ta-lib.org/wrappers/).

## Aliases

Stochastic, Stochastic Oscillator, Slow Stochastic

## See Also

STOCHF · STOCHRSI · MA
