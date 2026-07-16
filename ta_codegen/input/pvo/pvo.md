# PVO

## Summary

Percentage Volume Oscillator: a variation of the [Percentage Price Oscillator](/functions/ppo) (PPO, created by Gerald Appel) applied to the **volume** series instead of price. It is the difference between a fast and slow moving average of volume, expressed as a percentage of the slow MA. Positive when short-term volume is above its longer-term average (rising participation), negative when below. The default periods (12, 26) match MACD and PPO.

## Formula

PVO = ((fastMA(inVolume) - slowMA(inVolume)) / slowMA(inVolume)) * 100, both MAs of type optInMAType; output = 0 when slowMA == 0

The standard form is exponential with periods 12 and 26 — ((12-day EMA of Volume - 26-day EMA of Volume) / 26-day EMA of Volume) * 100, i.e. the PPO/MACD oscillator computed on volume. `optInMAType` therefore **defaults to EMA** — the moving average Gerald Appel used for the original PPO/MACD; pass another type (e.g. `TA_MAType_SMA`) to override.

## Inputs

- `inVolume` — Volume series

## Outputs

- `outReal` — PVO value in percent

## Parameters

- `optInFastPeriod` — Period of the fast MA
- `optInSlowPeriod` — Period of the slow MA
- `optInMAType` — Moving average type used for both MAs

## Implementation

TA-Lib Definition: [`pvo.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/pvo/pvo.c) · [`pvo.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/pvo/pvo.yaml)

| Native | File |
|--------|------|
| C | [`ta_PVO.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_PVO.c) |
| Rust | [`pvo.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/pvo.rs) |
| Java | [`Core_PVO.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/library/fragments/Core_PVO.java) |

TA-Lib is also available for Python, R and more using a [wrapper](https://ta-lib.org/wrappers/).

## Aliases

Percentage Volume Oscillator

## See Also

PPO · OBV · MACD

## References

- PVO has no separately documented originator; it applies the PPO/MACD oscillator (Gerald Appel) to the volume series.
- Formula and standard (12, 26, 9) parameters: [Percentage Volume Oscillator (PVO)](https://chartschool.stockcharts.com/table-of-contents/technical-indicators-and-overlays/technical-indicators/percentage-volume-oscillator-pvo), StockCharts ChartSchool; also documented by [TradingView](https://www.tradingview.com/support/solutions/43000591350-percentage-volume-oscillator-pvo/).
