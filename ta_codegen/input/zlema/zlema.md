# ZLEMA

## Summary

Zero-Lag Exponential Moving Average: an [`EMA`](/functions/ema) applied not to price but to a de-lagged series that extrapolates price forward by the EMA's own lag, cancelling that lag to first order. It tracks a trend far more closely than an EMA of the same length, at the cost of overshooting sharp reversals — the extrapolation keeps pushing in the old direction for a bar or two. Read it as an EMA that turns sooner: crossings of price and average, and changes in its slope, arrive earlier than the equivalent EMA signal, and its overshoot after a spike is a property of the filter rather than a move in the market.

ZLEMA is also selectable as a moving-average type (`TA_MAType_ZLEMA`) wherever an `optInMAType` parameter is accepted ([`MA`](/functions/ma), [`BBANDS`](/functions/bbands), [`STOCH`](/functions/stoch), [`MACDEXT`](/functions/macdext), ...).

## Formula

lag = Integer( (n - 1) / 2 )
d = 2 * Price - Price[lag bars ago]
ZLEMA(n) = EMA( d, n )

The inner average is the standard TA-Lib EMA: smoothing factor 2 / (n + 1), seeded with the simple average of the first n de-lagged values.

## Notes

- **The paper this indicator is usually credited to describes a different filter.** Ehlers and Way's *Zero Lag (Well, Almost)* specifies an error-correcting EMA with a per-bar gain search; neither the de-lagged series nor the `(n-1)/2` lag appears anywhere in it. What TA-Lib ships here is the de-lagged-EMA construction published under the "zero lag" name by Tulip Indicators, pandas-ta, TradingView Pine and others, for which no primary source is traceable.
- `lag` **truncates**: `Integer((n-1)/2)`. For an even period that is one bar shorter than the round-to-nearest convention some descriptions use, which moves the whole line, not just its warm-up. Tulip Indicators, pandas-ta and Pine all truncate.
- The de-lag is computed as `2 * Price - Price[lag]` in one rounding, rather than the algebraically equal `Price + (Price - Price[lag])` that Tulip Indicators, TradingView Pine and the Wikipedia statement use. The second form's extra rounding is one unit in the last place of the larger price — negligible against the de-lagged value, except where that value nearly cancels. When price is near double its value `lag` bars ago the two forms differ by about 5e-12 relative, so expect that much disagreement against those implementations on a strongly trending series, and do not attribute it to the seed or the smoothing factor.
- Implementations disagree on how the inner EMA is seeded — TA-Lib uses its own EMA convention (the simple average of the first `n` de-lagged values), where Tulip Indicators seeds from a single raw price and so emits its first value earlier and converges to these values only after many bars.
- ZLEMA inherits EMA's unstable period rather than owning one: `TA_SetUnstablePeriod(TA_FUNC_UNST_EMA, ...)` moves ZLEMA's first output too.
- A period of 1 performs no smoothing: the output is a copy of the input.

## Inputs

- `inReal` — Source price series, close by convention

## Outputs

- `outReal` — Zero-lag exponential moving average of the input

## Parameters

- `optInTimePeriod` — Number of bars in the exponential average; the de-lag distance derives from it

## Implementation

TA-Lib Definition: [`zlema.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/zlema/zlema.c) · [`zlema.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/zlema/zlema.yaml)

| Native | File |
|--------|------|
| C | [`ta_ZLEMA.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_ZLEMA.c) |
| Rust | [`zlema.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/zlema.rs) |
| Java | [`Core_ZLEMA.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/fragments/Core_ZLEMA.java) |

TA-Lib is also available for Python, R and more using a [wrapper](/install/#wrappers).

## Aliases

Zero Lag Exponential Moving Average, Zero-Lag EMA, ZLMA

## See Also

EMA · DEMA · TEMA · HMA · MA

## References

- John Ehlers and Ric Way, *Zero Lag (Well, Almost)*, Technical Analysis of Stocks & Commodities, November 2010 — the paper the name comes from, describing a different, error-correcting filter: [mesasoftware.com/papers/ZeroLag.pdf](https://www.mesasoftware.com/papers/ZeroLag.pdf)
- Tulip Indicators, *zlema* — an independent implementation of the de-lagged form, differing in its seeding: [tulipindicators.org/zlema](https://tulipindicators.org/zlema)
