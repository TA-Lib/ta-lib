# KDJ

## Summary

The stochastic oscillator as it is drawn on Chinese-market platforms: the Slow-%K and Slow-%D lines plus a third line J that amplifies the gap between them. K and D are read like any stochastic — above 80 overbought, below 20 oversold, a K/D crossing signalling a momentum shift — while J is a divergence gauge whose excursions outside the 0-100 band mark the strongest moves. Both smoothing stages default to Wilder's moving average, which is the smoother the original formula language specifies; selecting a simple moving average for both reproduces the classic Slow Stochastic with a J line attached.

## Formula

RSV = 100*(Close - LL_n)/(HH_n - LL_n), n = FastK_Period (LL/HH = lowest low / highest high over n)
K = MA(RSV, SlowK_Period, SlowK_MAType)
D = MA(K, SlowD_Period, SlowD_MAType)
J = 3*K - 2*D

## Notes

- The default smoothing is Wilder's moving average. The originating 通达信 (Tongdaxin) formula language writes each stage as `SMA(X, N, 1)`, a recurrence with weight 1/N on the new value, which is Wilder's smoothing under another name — not a simple average.
- How that recurrence is started is a TA-Lib house convention, not something the originating specification settles: like every other Wilder-smoothed function here, the first value is the simple average of the first N inputs, and callers who want the transient gone set the unstable period. Platforms that seed the recurrence at 50, or at the first raw value, differ for the first several dozen bars.
- J is deliberately unbounded. It leaves the 0-100 band routinely and is never clamped.
- When the high-low range over the window is zero, the raw stochastic is set to 0 instead of being undefined.

## Inputs

- `inHigh` — High price of each bar
- `inLow` — Low price of each bar
- `inClose` — Close price of each bar

## Outputs

- `outK` — Raw stochastic smoothed by SlowK_Period MA
- `outD` — Signal line: K smoothed by SlowD_Period MA
- `outJ` — Divergence line, three parts K less two parts D

## Parameters

- `optInFastK_Period` — Lookback window for the raw stochastic high-low range
- `optInSlowK_Period` — Smoothing period turning the raw stochastic into K
- `optInSlowK_MAType` — MA type used to smooth into K
- `optInSlowD_Period` — Smoothing period for the D signal line
- `optInSlowD_MAType` — MA type used for the D line

## Implementation

TA-Lib Definition: [`kdj.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/kdj/kdj.c) · [`kdj.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/kdj/kdj.yaml)

| Native | File |
|--------|------|
| C | [`ta_KDJ.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_KDJ.c) |
| Rust | [`kdj.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/kdj.rs) |
| Java | [`Core_KDJ.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/fragments/Core_KDJ.java) |

TA-Lib is also available for Python, R and more using a [wrapper](/install/#wrappers).

## Aliases

KDJ Indicator, Random Index, Stochastic KDJ, K D J lines

## See Also

STOCH · STOCHF · RMA · MA

## References

- [KDJ indicator explained — GTCFX](https://www.gtcfx.com/en-intl/knowledge-to-learn/technical-analysis/kdj-indicator-explained/kdj-k-d-and-j-lines-explained-formulas-and-meaning)
- [What is the KDJ indicator — Futu](https://www.futuhk.com/en/blog/detail-kdj-9-241091014)
