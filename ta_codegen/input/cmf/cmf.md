# CMF

## Summary

Chaikin Money Flow: over a trailing window of `optInTimePeriod` bars, the sum of each bar's money flow volume divided by the sum of its volume. The result is a ratio in `[-1, +1]`.

A bar's money flow volume is its volume scaled by where the close sat inside the bar's range: a close at the high contributes the full volume, a close at the low contributes minus the full volume, and a close at the midpoint contributes nothing. Summing that over a window and dividing by the window's volume answers "over these N bars, what share of the traded volume closed near the top of its range?"

Above zero is accumulation, below zero is distribution, and the distance from zero measures conviction. Because the divisor is the window's own volume, the output is comparable across instruments and across time in a way a raw accumulation total is not.

Created by Marc Chaikin, who also authored the [`AD`](/functions/ad) line this shares its per-bar multiplier with. CMF is that same multiplier summed over a fixed window and normalised, where AD accumulates it from the start of the series without bound.

## Formula

t = high[i] - low[i]

mfv[i] = ((close[i] - low[i]) - (high[i] - close[i])) / t * volume[i], or 0 when t is not positive

CMF[i] = ( sum_{k=i-N+1..i} mfv[k] ) / ( sum_{k=i-N+1..i} volume[k] ), N = optInTimePeriod

There is no seeding and no recursion, hence no unstable period. Each output depends only on the N bars in its own window.

## Notes

- The output is the raw ratio in `[-1, +1]`, matching every published definition. Some retail platforms display it multiplied by 100; that is a presentation choice, not a different indicator.
- Each bar's close is expected to lie within its own `[low, high]`, and its volume to be finite and non-negative. A close outside its bar makes the multiplier exceed ±1 and is passed through unclamped, exactly as [`AD`](/functions/ad) does.
- A bar whose high equals its low has no range for the close to sit inside, so it contributes exactly zero money flow volume rather than dividing by zero. Its volume still counts toward the divisor.
- A window whose volume is entirely zero has no money flow to distribute and reports 0.0. Published references are silent here and other implementations divide by zero; TA-Lib does not return NaN from a successful call.
- Bars where the low exceeds the high are malformed rather than degenerate, and also contribute zero.
- The default period of 20 follows the original write-up, which describes 20 or 21 bars.

## Inputs

- `inHigh` — High price of each bar
- `inLow` — Low price of each bar
- `inClose` — Close price of each bar
- `inVolume` — Volume of each bar

## Outputs

- `outReal` — Chaikin money flow, in the range -1 to +1

## Parameters

- `optInTimePeriod` — Number of bars in the window

## Implementation

TA-Lib Definition: [`cmf.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/cmf/cmf.c) · [`cmf.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/cmf/cmf.yaml)

| Native | File |
|--------|------|
| C | [`ta_CMF.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_CMF.c) |
| Rust | [`cmf.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/cmf.rs) |
| Java | [`Core_CMF.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/fragments/Core_CMF.java) |

TA-Lib is also available for Python, R and more using a [wrapper](https://ta-lib.org/wrappers/).

## Aliases

Chaikin Money Flow

## See Also

AD · ADOSC · MFI · OBV

## References

- Marc Chaikin is the originator; StockCharts ChartSchool carries the canonical three-step derivation, "20-period Sum of Money Flow Volume / 20-period Sum of Volume".
- TradingView, *Chaikin Money Flow (CMF)* — the same derivation with a worked 21-period example.
- Kirkpatrick and Dahlquist, *Technical Analysis: The Complete Resource for Financial Market Technicians*, 2nd edition, pages 419 and 421.
