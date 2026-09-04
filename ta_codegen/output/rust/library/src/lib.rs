//! # TA-Lib: Technical Analysis Library
//!
//! 180 technical-analysis indicators — moving averages, momentum oscillators,
//! volatility bands, volume studies, Hilbert Transform cycle analysis, statistics,
//! price transforms, and 61 candlestick-pattern recognizers — as a pure-Rust crate.
//!
//! This is the official Rust port of [TA-Lib](https://ta-lib.org): every function is
//! generated from the same canonical definitions as the C library and verified
//! against the C reference implementation.
//!
//! # Quick start
//!
//! ```
//! use ta_lib::{Core, RetCode};
//!
//! let close = [11.0, 12.0, 13.0, 14.0, 15.0, 16.0, 17.0, 18.0, 19.0, 20.0];
//! let core = Core::new();
//! let mut sma = vec![0.0; close.len()];
//!
//! let out = core.SMA(0, close.len() - 1, &close, 3, &mut sma)?;
//!
//! // The first 3-period average lands at input index 2 (the lookback):
//! assert_eq!((out.beg_idx, out.count), (2, 8));
//! assert_eq!(sma[0], 12.0); // (11 + 12 + 13) / 3
//! # Ok::<(), ta_lib::RetCode>(())
//! ```
//!
//! # API shape
//!
//! Every indicator is a method on [`Core`] and follows the same pattern:
//!
//! * Inputs are `&[f64]` slices, computed over the range `startIdx..=endIdx`.
//! * Outputs are written into caller-provided `&mut` slices. An indicator consumes a
//!   number of leading values (its *lookback*) before producing output — query it with
//!   the matching `*_Lookback` method (e.g. [`Core::SMA_Lookback`]).
//! * Integer parameters accept [`Core::INTEGER_DEFAULT`], and real parameters
//!   [`Core::REAL_DEFAULT`], to select their default value; a moving-average type takes
//!   [`MAType::DEFAULT`] instead, the sentinel being unrepresentable at a typed enum.
//! * Every call returns [`Result`]`<`[`OutRange`]`, `[`RetCode`]`>`, so it composes with
//!   `?`. [`OutRange`] says where the values start ([`beg_idx`](OutRange::beg_idx), in the
//!   input series' coordinates) and how many there are ([`count`](OutRange::count)).
//!   A range shorter than the lookback is a **success with no values**, not an error.
//!
//! [`Core`] is immutable after construction: its per-instance settings — unstable
//! period and candlestick thresholds — are chosen up front with
//! [`Core::builder()`] and then frozen, so a `Core` is `Send + Sync` and
//! can be shared read-only across threads (e.g. via `Arc`) with no locking:
//!
//! ```
//! use ta_lib::{Core, FuncUnstId};
//!
//! let core = Core::builder()
//!     .unstable_period(FuncUnstId::EMA, 10)
//!     .build()?;
//!
//! assert_eq!(core.get_unstable_period(FuncUnstId::EMA)?, 10);
//! # Ok::<(), ta_lib::RetCode>(())
//! ```
//!
//! The setters are infallible so that they chain; a rejected argument is
//! reported once, by `build()`, as [`RetCode::BadParam`].
//!
//! To change a setting, build a new `Core` (cloning is cheap); [`Core::to_builder()`]
//! seeds a builder from an existing instance.
//!
//! The crate is `#![forbid(unsafe_code)]`: a bounds violation panics, it never
//! triggers undefined behavior. On x86-64, the batch entry
//! points of indicators built on fused multiply-adds are compiled twice and the
//! hardware-FMA clone is selected at runtime (the same dispatch the C library
//! performs via `target_clones`); both paths are correctly rounded, so results
//! are bit-identical either way. Calling that clone is the one `unsafe` in the
//! crate's shipped dependency graph: it lives in `ta-lib-dispatch`, inside the
//! `is_x86_feature_detected!("fma")` test that has just proved it sound, and
//! `forbid` here does not see it because it expands from another crate's macro.
//! The streaming tier stays single-path.
//!
//! # Live data
//!
//! The calls above take a whole series at once. For a feed that arrives one bar
//! at a time, each indicator also has a *streaming* form: an `*_open` method
//! ([`Core::sma_open`], [`Core::rsi_open`], …) warms a handle up on the history
//! you already have, and from then on one bar in gives that bar's value out,
//! with no re-scan of the series and no allocation per bar.
//!
//! ```
//! use ta_lib::Core;
//!
//! let history = [11.0, 12.0, 13.0, 14.0, 15.0];
//! let core = Core::new();
//! let (mut sma, last) = core.sma_open(&history, 3)?;
//!
//! assert_eq!(last, 14.0); // (13 + 14 + 15) / 3, the last history bar
//! assert_eq!(sma.out_range().count, 3);
//!
//! // A bar that has not closed yet: ask without committing it.
//! assert_eq!(sma.peek(16.0)?, 15.0);
//! assert_eq!(sma.out_range().count, 3);
//!
//! // Once it closes, commit it — same value, and the range advances.
//! assert_eq!(sma.update(16.0)?, 15.0);
//! assert_eq!(sma.out_range().count, 4);
//!
//! // A non-finite bar is rejected, and still counted: the handle's output for
//! // it is the previous one, held, and its state is untouched.
//! assert!(sma.update(f64::NAN).is_err());
//! assert_eq!(sma.out_range().count, 5);
//! # Ok::<(), ta_lib::RetCode>(())
//! ```
//!
//! The handle's value at every bar is bit-identical to what the batch call
//! reports for that bar. [`SmaStream::out_range`] carries the same
//! [`OutRange`] the batch tier returns — the bars the handle has an output for
//! — and every bar handed to [`SmaStream::update`] advances it by one, a bar
//! rejected as non-finite included: its output is the previous one, held.
//! [`SmaStream::peek`] leaves it alone; cloning a handle forks an independent
//! stream, and dropping it closes the stream.
//!
//! The full function reference, grouped by category, is at
//! [ta-lib.org/functions](https://ta-lib.org/functions/); the guides are at
//! [ta-lib.org/api/rust](https://ta-lib.org/api/rust/) and, for the streaming
//! tier, [ta-lib.org/api/rust/stream](https://ta-lib.org/api/rust/stream/).
//!
//! # Indicators by category
//!
//! Every indicator is a method on [`Core`], and the methods are one flat
//! alphabetical list — so this is where the grouping lives. It is the same
//! grouping the registry answers at run time ([`abstract_api::Group`], reported
//! per function as [`FuncInfo::group`](abstract_api::FuncInfo::group)), and each
//! entry carries that row's own one-line hint. Follow a link for the function's
//! formula, arguments, ranges and a runnable example.
//!
//! ## Cycle Indicators (5)
//!
//! * [`HT_DCPERIOD`](Core::HT_DCPERIOD) — Hilbert Transform - Dominant Cycle Period
//! * [`HT_DCPHASE`](Core::HT_DCPHASE) — Hilbert Transform - Dominant Cycle Phase
//! * [`HT_PHASOR`](Core::HT_PHASOR) — Hilbert Transform - Phasor Components
//! * [`HT_SINE`](Core::HT_SINE) — Hilbert Transform - SineWave
//! * [`HT_TRENDMODE`](Core::HT_TRENDMODE) — Hilbert Transform - Trend vs Cycle Mode
//!
//! ## Math Operators (11)
//!
//! * [`ADD`](Core::ADD) — Vector Arithmetic Add
//! * [`DIV`](Core::DIV) — Vector Arithmetic Div
//! * [`MAX`](Core::MAX) — Highest value over a specified period
//! * [`MAXINDEX`](Core::MAXINDEX) — Index of highest value over a specified period
//! * [`MIN`](Core::MIN) — Lowest value over a specified period
//! * [`MININDEX`](Core::MININDEX) — Index of lowest value over a specified period
//! * [`MINMAX`](Core::MINMAX) — Lowest and highest values over a specified period
//! * [`MINMAXINDEX`](Core::MINMAXINDEX) — Indexes of lowest and highest values over a specified period
//! * [`MULT`](Core::MULT) — Vector Arithmetic Mult
//! * [`SUB`](Core::SUB) — Vector Arithmetic Subtraction
//! * [`SUM`](Core::SUM) — Summation
//!
//! ## Math Transform (15)
//!
//! * [`ACOS`](Core::ACOS) — Vector Trigonometric ACos
//! * [`ASIN`](Core::ASIN) — Vector Trigonometric ASin
//! * [`ATAN`](Core::ATAN) — Vector Trigonometric ATan
//! * [`CEIL`](Core::CEIL) — Vector Ceil
//! * [`COS`](Core::COS) — Vector Trigonometric Cos
//! * [`COSH`](Core::COSH) — Vector Trigonometric Cosh
//! * [`EXP`](Core::EXP) — Vector Arithmetic Exp
//! * [`FLOOR`](Core::FLOOR) — Vector Floor
//! * [`LN`](Core::LN) — Vector Log Natural
//! * [`LOG10`](Core::LOG10) — Vector Log10
//! * [`SIN`](Core::SIN) — Vector Trigonometric Sin
//! * [`SINH`](Core::SINH) — Vector Trigonometric Sinh
//! * [`SQRT`](Core::SQRT) — Vector Square Root
//! * [`TAN`](Core::TAN) — Vector Trigonometric Tan
//! * [`TANH`](Core::TANH) — Vector Trigonometric Tanh
//!
//! ## Momentum Indicators (37)
//!
//! * [`AC`](Core::AC) — Accelerator/Decelerator Oscillator
//! * [`ADX`](Core::ADX) — Average Directional Movement Index
//! * [`ADXR`](Core::ADXR) — Average Directional Movement Index Rating
//! * [`AO`](Core::AO) — Awesome Oscillator
//! * [`APO`](Core::APO) — Absolute Price Oscillator
//! * [`AROON`](Core::AROON) — Aroon
//! * [`AROONOSC`](Core::AROONOSC) — Aroon Oscillator
//! * [`BOP`](Core::BOP) — Balance Of Power
//! * [`CCI`](Core::CCI) — Commodity Channel Index
//! * [`CMO`](Core::CMO) — Chande Momentum Oscillator
//! * [`CMOU`](Core::CMOU) — Chande Momentum Oscillator (Unsmoothed)
//! * [`DX`](Core::DX) — Directional Movement Index
//! * [`IMI`](Core::IMI) — Intraday Momentum Index
//! * [`MACD`](Core::MACD) — Moving Average Convergence/Divergence
//! * [`MACDEXT`](Core::MACDEXT) — MACD with controllable MA type
//! * [`MACDFIX`](Core::MACDFIX) — Moving Average Convergence/Divergence Fix 12/26
//! * [`MFI`](Core::MFI) — Money Flow Index
//! * [`MINUS_DI`](Core::MINUS_DI) — Minus Directional Indicator
//! * [`MINUS_DM`](Core::MINUS_DM) — Minus Directional Movement
//! * [`MOM`](Core::MOM) — Momentum
//! * [`PLUS_DI`](Core::PLUS_DI) — Plus Directional Indicator
//! * [`PLUS_DM`](Core::PLUS_DM) — Plus Directional Movement
//! * [`PPO`](Core::PPO) — Percentage Price Oscillator
//! * [`QSTICK`](Core::QSTICK) — Qstick
//! * [`ROC`](Core::ROC) — Rate of change : ((price/prevPrice)-1)*100
//! * [`ROCP`](Core::ROCP) — Rate of change Percentage: (price-prevPrice)/prevPrice
//! * [`ROCR`](Core::ROCR) — Rate of change ratio: (price/prevPrice)
//! * [`ROCR100`](Core::ROCR100) — Rate of change ratio 100 scale: (price/prevPrice)*100
//! * [`RSI`](Core::RSI) — Relative Strength Index
//! * [`SMI`](Core::SMI) — Stochastic Momentum Index
//! * [`STOCH`](Core::STOCH) — Stochastic
//! * [`STOCHF`](Core::STOCHF) — Stochastic Fast
//! * [`STOCHRSI`](Core::STOCHRSI) — Stochastic Relative Strength Index
//! * [`TRIX`](Core::TRIX) — 1-day Rate-Of-Change (ROC) of a Triple Smooth EMA
//! * [`ULTOSC`](Core::ULTOSC) — Ultimate Oscillator
//! * [`WAD`](Core::WAD) — Williams' Accumulation/Distribution
//! * [`WILLR`](Core::WILLR) — Williams' %R
//!
//! ## Overlap Studies (24)
//!
//! * [`ACCBANDS`](Core::ACCBANDS) — Acceleration Bands
//! * [`BBANDS`](Core::BBANDS) — Bollinger Bands
//! * [`DEMA`](Core::DEMA) — Double Exponential Moving Average
//! * [`DONCHIAN`](Core::DONCHIAN) — Donchian Channels
//! * [`EMA`](Core::EMA) — Exponential Moving Average
//! * [`HMA`](Core::HMA) — Hull Moving Average
//! * [`HT_TRENDLINE`](Core::HT_TRENDLINE) — Hilbert Transform - Instantaneous Trendline
//! * [`KAMA`](Core::KAMA) — Kaufman Adaptive Moving Average
//! * [`KC`](Core::KC) — Keltner Channels
//! * [`MA`](Core::MA) — Moving average
//! * [`MAMA`](Core::MAMA) — MESA Adaptive Moving Average
//! * [`MAVP`](Core::MAVP) — Moving average with variable period
//! * [`MIDPOINT`](Core::MIDPOINT) — MidPoint over period
//! * [`MIDPRICE`](Core::MIDPRICE) — Midpoint Price over period
//! * [`RMA`](Core::RMA) — Wilder's Smoothed Moving Average
//! * [`SAR`](Core::SAR) — Parabolic SAR
//! * [`SAREXT`](Core::SAREXT) — Parabolic SAR - Extended
//! * [`SMA`](Core::SMA) — Simple Moving Average
//! * [`SUPERTREND`](Core::SUPERTREND) — SuperTrend
//! * [`T3`](Core::T3) — Triple Exponential Moving Average (T3)
//! * [`TEMA`](Core::TEMA) — Triple Exponential Moving Average
//! * [`TRIMA`](Core::TRIMA) — Triangular Moving Average
//! * [`VWMA`](Core::VWMA) — Volume Weighted Moving Average
//! * [`WMA`](Core::WMA) — Weighted Moving Average
//!
//! ## Pattern Recognition (61)
//!
//! * [`CDL2CROWS`](Core::CDL2CROWS) — Two Crows
//! * [`CDL3BLACKCROWS`](Core::CDL3BLACKCROWS) — Three Black Crows
//! * [`CDL3INSIDE`](Core::CDL3INSIDE) — Three Inside Up/Down
//! * [`CDL3LINESTRIKE`](Core::CDL3LINESTRIKE) — Three-Line Strike
//! * [`CDL3OUTSIDE`](Core::CDL3OUTSIDE) — Three Outside Up/Down
//! * [`CDL3STARSINSOUTH`](Core::CDL3STARSINSOUTH) — Three Stars In The South
//! * [`CDL3WHITESOLDIERS`](Core::CDL3WHITESOLDIERS) — Three Advancing White Soldiers
//! * [`CDLABANDONEDBABY`](Core::CDLABANDONEDBABY) — Abandoned Baby
//! * [`CDLADVANCEBLOCK`](Core::CDLADVANCEBLOCK) — Advance Block
//! * [`CDLBELTHOLD`](Core::CDLBELTHOLD) — Belt-hold
//! * [`CDLBREAKAWAY`](Core::CDLBREAKAWAY) — Breakaway
//! * [`CDLCLOSINGMARUBOZU`](Core::CDLCLOSINGMARUBOZU) — Closing Marubozu
//! * [`CDLCONCEALBABYSWALL`](Core::CDLCONCEALBABYSWALL) — Concealing Baby Swallow
//! * [`CDLCOUNTERATTACK`](Core::CDLCOUNTERATTACK) — Counterattack
//! * [`CDLDARKCLOUDCOVER`](Core::CDLDARKCLOUDCOVER) — Dark Cloud Cover
//! * [`CDLDOJI`](Core::CDLDOJI) — Doji
//! * [`CDLDOJISTAR`](Core::CDLDOJISTAR) — Doji Star
//! * [`CDLDRAGONFLYDOJI`](Core::CDLDRAGONFLYDOJI) — Dragonfly Doji
//! * [`CDLENGULFING`](Core::CDLENGULFING) — Engulfing Pattern
//! * [`CDLEVENINGDOJISTAR`](Core::CDLEVENINGDOJISTAR) — Evening Doji Star
//! * [`CDLEVENINGSTAR`](Core::CDLEVENINGSTAR) — Evening Star
//! * [`CDLGAPSIDESIDEWHITE`](Core::CDLGAPSIDESIDEWHITE) — Up/Down-gap side-by-side white lines
//! * [`CDLGRAVESTONEDOJI`](Core::CDLGRAVESTONEDOJI) — Gravestone Doji
//! * [`CDLHAMMER`](Core::CDLHAMMER) — Hammer
//! * [`CDLHANGINGMAN`](Core::CDLHANGINGMAN) — Hanging Man
//! * [`CDLHARAMI`](Core::CDLHARAMI) — Harami Pattern
//! * [`CDLHARAMICROSS`](Core::CDLHARAMICROSS) — Harami Cross Pattern
//! * [`CDLHIGHWAVE`](Core::CDLHIGHWAVE) — High-Wave Candle
//! * [`CDLHIKKAKE`](Core::CDLHIKKAKE) — Hikkake Pattern
//! * [`CDLHIKKAKEMOD`](Core::CDLHIKKAKEMOD) — Modified Hikkake Pattern
//! * [`CDLHOMINGPIGEON`](Core::CDLHOMINGPIGEON) — Homing Pigeon
//! * [`CDLIDENTICAL3CROWS`](Core::CDLIDENTICAL3CROWS) — Identical Three Crows
//! * [`CDLINNECK`](Core::CDLINNECK) — In-Neck Pattern
//! * [`CDLINVERTEDHAMMER`](Core::CDLINVERTEDHAMMER) — Inverted Hammer
//! * [`CDLKICKING`](Core::CDLKICKING) — Kicking
//! * [`CDLKICKINGBYLENGTH`](Core::CDLKICKINGBYLENGTH) — Kicking - bull/bear determined by the longer marubozu
//! * [`CDLLADDERBOTTOM`](Core::CDLLADDERBOTTOM) — Ladder Bottom
//! * [`CDLLONGLEGGEDDOJI`](Core::CDLLONGLEGGEDDOJI) — Long Legged Doji
//! * [`CDLLONGLINE`](Core::CDLLONGLINE) — Long Line Candle
//! * [`CDLMARUBOZU`](Core::CDLMARUBOZU) — Marubozu
//! * [`CDLMATCHINGLOW`](Core::CDLMATCHINGLOW) — Matching Low
//! * [`CDLMATHOLD`](Core::CDLMATHOLD) — Mat Hold
//! * [`CDLMORNINGDOJISTAR`](Core::CDLMORNINGDOJISTAR) — Morning Doji Star
//! * [`CDLMORNINGSTAR`](Core::CDLMORNINGSTAR) — Morning Star
//! * [`CDLONNECK`](Core::CDLONNECK) — On-Neck Pattern
//! * [`CDLPIERCING`](Core::CDLPIERCING) — Piercing Pattern
//! * [`CDLRICKSHAWMAN`](Core::CDLRICKSHAWMAN) — Rickshaw Man
//! * [`CDLRISEFALL3METHODS`](Core::CDLRISEFALL3METHODS) — Rising/Falling Three Methods
//! * [`CDLSEPARATINGLINES`](Core::CDLSEPARATINGLINES) — Separating Lines
//! * [`CDLSHOOTINGSTAR`](Core::CDLSHOOTINGSTAR) — Shooting Star
//! * [`CDLSHORTLINE`](Core::CDLSHORTLINE) — Short Line Candle
//! * [`CDLSPINNINGTOP`](Core::CDLSPINNINGTOP) — Spinning Top
//! * [`CDLSTALLEDPATTERN`](Core::CDLSTALLEDPATTERN) — Stalled Pattern
//! * [`CDLSTICKSANDWICH`](Core::CDLSTICKSANDWICH) — Stick Sandwich
//! * [`CDLTAKURI`](Core::CDLTAKURI) — Takuri (Dragonfly Doji with very long lower shadow)
//! * [`CDLTASUKIGAP`](Core::CDLTASUKIGAP) — Tasuki Gap
//! * [`CDLTHRUSTING`](Core::CDLTHRUSTING) — Thrusting Pattern
//! * [`CDLTRISTAR`](Core::CDLTRISTAR) — Tristar Pattern
//! * [`CDLUNIQUE3RIVER`](Core::CDLUNIQUE3RIVER) — Unique 3 River
//! * [`CDLUPSIDEGAP2CROWS`](Core::CDLUPSIDEGAP2CROWS) — Upside Gap Two Crows
//! * [`CDLXSIDEGAP3METHODS`](Core::CDLXSIDEGAP3METHODS) — Upside/Downside Gap Three Methods
//!
//! ## Price Transform (5)
//!
//! * [`AVGDEV`](Core::AVGDEV) — Average Deviation
//! * [`AVGPRICE`](Core::AVGPRICE) — Average Price
//! * [`MEDPRICE`](Core::MEDPRICE) — Median Price
//! * [`TYPPRICE`](Core::TYPPRICE) — Typical Price
//! * [`WCLPRICE`](Core::WCLPRICE) — Weighted Close Price
//!
//! ## Statistic Functions (9)
//!
//! * [`BETA`](Core::BETA) — Beta
//! * [`CORREL`](Core::CORREL) — Pearson's Correlation Coefficient (r)
//! * [`LINEARREG`](Core::LINEARREG) — Linear Regression
//! * [`LINEARREG_ANGLE`](Core::LINEARREG_ANGLE) — Linear Regression Angle
//! * [`LINEARREG_INTERCEPT`](Core::LINEARREG_INTERCEPT) — Linear Regression Intercept
//! * [`LINEARREG_SLOPE`](Core::LINEARREG_SLOPE) — Linear Regression Slope
//! * [`STDDEV`](Core::STDDEV) — Standard Deviation
//! * [`TSF`](Core::TSF) — Time Series Forecast
//! * [`VAR`](Core::VAR) — Variance
//!
//! ## Volatility Indicators (3)
//!
//! * [`ATR`](Core::ATR) — Average True Range
//! * [`NATR`](Core::NATR) — Normalized Average True Range
//! * [`TRANGE`](Core::TRANGE) — True Range
//!
//! ## Volume Indicators (10)
//!
//! * [`AD`](Core::AD) — Chaikin A/D Line
//! * [`ADOSC`](Core::ADOSC) — Chaikin A/D Oscillator
//! * [`CMF`](Core::CMF) — Chaikin Money Flow
//! * [`EFI`](Core::EFI) — Elder's Force Index
//! * [`MARKETFI`](Core::MARKETFI) — Market Facilitation Index
//! * [`NVI`](Core::NVI) — Negative Volume Index
//! * [`OBV`](Core::OBV) — On Balance Volume
//! * [`PVI`](Core::PVI) — Positive Volume Index
//! * [`PVO`](Core::PVO) — Percentage Volume Oscillator
//! * [`VWAP`](Core::VWAP) — Volume Weighted Average Price

#![forbid(unsafe_code)]
// Every public item, and every public enum variant and struct field, carries its
// own documentation (#179 D7). `warn` rather than `deny` so that a future rustc
// widening the lint cannot break a downstream build; the nightly's
// `cargo clippy -- -D warnings` is what makes it a gate here.
#![warn(missing_docs)]
#![allow(non_snake_case, non_camel_case_types, unused_variables, unused_assignments, unused_mut, unused_parens, arithmetic_overflow)]
// Generated code: Clippy's style/complexity lints are noise on machine output, and
// several "fixes" would change numeric behavior — e.g. `neg_cmp_op_on_partial_ord`
// on C's `!(a < b)` NaN idiom, or De Morgan rewrites under `nonminimal_bool`. The
// crate is verified bit-exact against the C reference, so these are suppressed rather
// than applied. `too_many_arguments` is inherent to the C API arity.
#![allow(clippy::all, clippy::pedantic)]
#![allow(clippy::approx_constant)] // PI (180/3.141592653589793) is copied verbatim from the C source.
// Private, so every public type has exactly one path. `ta_func` is the C source
// directory's name, and `ta_lib::ta_func::Core` would stutter; the glob below is
// the only way in (#179 C5).
mod ta_func;
pub mod abstract_api;
pub use ta_func::*;

// The README is the crate's front page on crates.io and on GitHub, and its Rust
// sample is a claim about this API — yet nothing in the tree compiled it:
// `readme = "README.md"` is packaging metadata, and every other doctest here
// comes from a generated per-function page. So the front page was the one piece
// of Rust in this crate that could say anything, and twice it did: the install
// line resolved to no published version (#179 A1) and the indicator count was
// seven stale (#179 A2), both found by reading rather than by a gate. The counts
// and the install requirement are derived now; this covers the code.
//
// `cfg(doctest)` is what keeps it to `cargo test --doc`: the item does not exist
// during `cargo build`, `cargo clippy` or `cargo doc`, so the README's headings
// never appear in the rendered docs and its links are not resolved as intra-doc
// links (they are ordinary Markdown links, and must stay that way to render on
// crates.io).
#[cfg(doctest)]
#[doc = include_str!("../README.md")]
struct ReadmeExamples;
