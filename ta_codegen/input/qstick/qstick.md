# QSTICK

## Summary

Tushar Chande and Stanley Kroll's Qstick (*The New Technical Trader*, 1994): a simple moving average of the candle body, close minus open. It measures how bullish or bearish the bodies have been over the window, independently of the wicks — above zero the bodies closed up on balance, below zero they closed down, and the zero-line crossings are the signal.

## Formula

body_t = close_t - open_t; QSTICK_t = ( Σ body over the last `optInTimePeriod` bars ) / optInTimePeriod

The moving average is a plain SMA, so there is no seeding convention and none of the cross-library divergence that comes with one. `optInTimePeriod` of 1 leaves the raw body.

## Inputs

- `inOpen` — Open price of each bar
- `inClose` — Close price of each bar

## Outputs

- `outReal` — Average candle body over the window

## Parameters

- `optInTimePeriod` — Number of bars averaged. Default 10, matching Tulip Indicators and pandas-ta-classic. Other packages differ: TraderEvolution documents 1, and AmiBroker community code commonly uses 8.

## Implementation

TA-Lib Definition: [`qstick.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/qstick/qstick.c) · [`qstick.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/qstick/qstick.yaml)

| Native | File |
|--------|------|
| C | [`ta_QSTICK.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_QSTICK.c) |
| Rust | [`qstick.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/qstick.rs) |
| Java | [`Core_QSTICK.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/fragments/Core_QSTICK.java) |
| C# | [`Core_QSTICK.cs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/csharp/library/src/Core_QSTICK.cs) |

TA-Lib is also available for Python, R and more using a [wrapper](/install/#wrappers).

## See Also

CMO · IMI · MOM · SMA

## References

- Tushar S. Chande and Stanley Kroll, *The New Technical Trader*, Wiley, 1994, define Qstick as an n-period moving average of `Close - Open`.
- Steven B. Achelis, *Technical Analysis from A to Z*, page 280 carries the worked example pinned in the test suite; it is exact in binary, every price on the page being a sixteenth.
- Tulip Indicators `ti_qstick` and pandas-ta-classic `qstick` compute the same form and both default the period to 10, which is where this default comes from.
- TraderEvolution documents a default period of 1 and offers a choice of moving average; AmiBroker community code commonly cites 8. Neither is verifiable against the book, and this ships the SMA-only form the authors define.
