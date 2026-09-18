//! # TA-Lib: Technical Analysis Library
//!
//! 200+ technical-analysis indicators — moving averages, momentum oscillators,
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
//! let out = core.sma(0, close.len() - 1, &close, 3, &mut sma)?;
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
//!   the matching `*_lookback` method (e.g. [`Core::sma_lookback`]).
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
//! // A non-finite bar is rejected, and the rejection costs nothing at all:
//! // no state, no value, no range.
//! assert!(sma.update(f64::NAN).is_err());
//! assert_eq!(sma.out_range().count, 4);
//!
//! // Re-feed the bar when a corrected value arrives, or — when none is coming —
//! // count it and carry on. Its output is the previous one, held.
//! sma.advance()?;
//! assert_eq!(sma.out_range().count, 5);
//! assert_eq!(sma.value(), 15.0);
//! # Ok::<(), ta_lib::RetCode>(())
//! ```
//!
//! The handle's value at every bar is bit-identical to what the batch call
//! reports for that bar. [`SmaStream::out_range`] carries the same
//! [`OutRange`] the batch tier returns — the bars the handle has an output for
//! — and every bar [`SmaStream::update`] accepts advances it by one. A rejected
//! bar advances nothing; [`SmaStream::advance`] is how a caller counts one it
//! decided not to feed. [`SmaStream::peek`] leaves it alone; cloning a handle
//! forks an independent stream, and dropping it closes the stream.
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
//! * [`HT_DCPERIOD`](Core::ht_dcperiod) — Hilbert Transform - Dominant Cycle Period
//! * [`HT_DCPHASE`](Core::ht_dcphase) — Hilbert Transform - Dominant Cycle Phase
//! * [`HT_PHASOR`](Core::ht_phasor) — Hilbert Transform - Phasor Components
//! * [`HT_SINE`](Core::ht_sine) — Hilbert Transform - SineWave
//! * [`HT_TRENDMODE`](Core::ht_trendmode) — Hilbert Transform - Trend vs Cycle Mode
//!
//! ## Math Operators (12)
//!
//! * [`ADD`](Core::add) — Vector Arithmetic Add
//! * [`CUMSUM`](Core::cumsum) — Cumulative Sum
//! * [`DIV`](Core::div) — Vector Arithmetic Div
//! * [`MAX`](Core::max) — Highest value over a specified period
//! * [`MAXINDEX`](Core::maxindex) — Index of highest value over a specified period
//! * [`MIN`](Core::min) — Lowest value over a specified period
//! * [`MININDEX`](Core::minindex) — Index of lowest value over a specified period
//! * [`MINMAX`](Core::minmax) — Lowest and highest values over a specified period
//! * [`MINMAXINDEX`](Core::minmaxindex) — Indexes of lowest and highest values over a specified period
//! * [`MULT`](Core::mult) — Vector Arithmetic Mult
//! * [`SUB`](Core::sub) — Vector Arithmetic Subtraction
//! * [`SUM`](Core::sum) — Summation
//!
//! ## Math Transform (15)
//!
//! * [`ACOS`](Core::acos) — Vector Trigonometric ACos
//! * [`ASIN`](Core::asin) — Vector Trigonometric ASin
//! * [`ATAN`](Core::atan) — Vector Trigonometric ATan
//! * [`CEIL`](Core::ceil) — Vector Ceil
//! * [`COS`](Core::cos) — Vector Trigonometric Cos
//! * [`COSH`](Core::cosh) — Vector Trigonometric Cosh
//! * [`EXP`](Core::exp) — Vector Arithmetic Exp
//! * [`FLOOR`](Core::floor) — Vector Floor
//! * [`LN`](Core::ln) — Vector Log Natural
//! * [`LOG10`](Core::log10) — Vector Log10
//! * [`SIN`](Core::sin) — Vector Trigonometric Sin
//! * [`SINH`](Core::sinh) — Vector Trigonometric Sinh
//! * [`SQRT`](Core::sqrt) — Vector Square Root
//! * [`TAN`](Core::tan) — Vector Trigonometric Tan
//! * [`TANH`](Core::tanh) — Vector Trigonometric Tanh
//!
//! ## Momentum Indicators (47)
//!
//! * [`AC`](Core::ac) — Accelerator/Decelerator Oscillator
//! * [`ADX`](Core::adx) — Average Directional Movement Index
//! * [`ADXR`](Core::adxr) — Average Directional Movement Index Rating
//! * [`AO`](Core::ao) — Awesome Oscillator
//! * [`APO`](Core::apo) — Absolute Price Oscillator
//! * [`AROON`](Core::aroon) — Aroon
//! * [`AROONOSC`](Core::aroonosc) — Aroon Oscillator
//! * [`BOP`](Core::bop) — Balance Of Power
//! * [`CCI`](Core::cci) — Commodity Channel Index
//! * [`CMO`](Core::cmo) — Chande Momentum Oscillator
//! * [`CMOU`](Core::cmou) — Chande Momentum Oscillator (Unsmoothed)
//! * [`COPPOCK`](Core::coppock) — Coppock Curve
//! * [`DPO`](Core::dpo) — Detrended Price Oscillator
//! * [`DX`](Core::dx) — Directional Movement Index
//! * [`ER`](Core::er) — Kaufman Efficiency Ratio
//! * [`ERI`](Core::eri) — Elder Ray Index (Bull Power / Bear Power)
//! * [`FOSC`](Core::fosc) — Forecast Oscillator
//! * [`FRACTAL`](Core::fractal) — Williams Fractal
//! * [`IMI`](Core::imi) — Intraday Momentum Index
//! * [`KDJ`](Core::kdj) — KDJ Stochastic
//! * [`MACD`](Core::macd) — Moving Average Convergence/Divergence
//! * [`MACDEXT`](Core::macdext) — MACD with controllable MA type
//! * [`MACDFIX`](Core::macdfix) — Moving Average Convergence/Divergence Fix 12/26
//! * [`MFI`](Core::mfi) — Money Flow Index
//! * [`MINUS_DI`](Core::minus_di) — Minus Directional Indicator
//! * [`MINUS_DM`](Core::minus_dm) — Minus Directional Movement
//! * [`MOM`](Core::mom) — Momentum
//! * [`PLUS_DI`](Core::plus_di) — Plus Directional Indicator
//! * [`PLUS_DM`](Core::plus_dm) — Plus Directional Movement
//! * [`PPO`](Core::ppo) — Percentage Price Oscillator
//! * [`QSTICK`](Core::qstick) — Qstick
//! * [`ROC`](Core::roc) — Rate of change : ((price/prevPrice)-1)*100
//! * [`ROCP`](Core::rocp) — Rate of change Percentage: (price-prevPrice)/prevPrice
//! * [`ROCR`](Core::rocr) — Rate of change ratio: (price/prevPrice)
//! * [`ROCR100`](Core::rocr100) — Rate of change ratio 100 scale: (price/prevPrice)*100
//! * [`RSI`](Core::rsi) — Relative Strength Index
//! * [`SMI`](Core::smi) — Stochastic Momentum Index
//! * [`STOCH`](Core::stoch) — Stochastic
//! * [`STOCHF`](Core::stochf) — Stochastic Fast
//! * [`STOCHRSI`](Core::stochrsi) — Stochastic Relative Strength Index
//! * [`TRIX`](Core::trix) — 1-day Rate-Of-Change (ROC) of a Triple Smooth EMA
//! * [`TSI`](Core::tsi) — True Strength Index
//! * [`ULTOSC`](Core::ultosc) — Ultimate Oscillator
//! * [`VHF`](Core::vhf) — Vertical Horizontal Filter
//! * [`VORTEX`](Core::vortex) — Vortex Indicator
//! * [`WAD`](Core::wad) — Williams' Accumulation/Distribution
//! * [`WILLR`](Core::willr) — Williams' %R
//!
//! ## Overlap Studies (25)
//!
//! * [`ACCBANDS`](Core::accbands) — Acceleration Bands
//! * [`BBANDS`](Core::bbands) — Bollinger Bands
//! * [`DEMA`](Core::dema) — Double Exponential Moving Average
//! * [`DONCHIAN`](Core::donchian) — Donchian Channels
//! * [`EMA`](Core::ema) — Exponential Moving Average
//! * [`HMA`](Core::hma) — Hull Moving Average
//! * [`HT_TRENDLINE`](Core::ht_trendline) — Hilbert Transform - Instantaneous Trendline
//! * [`KAMA`](Core::kama) — Kaufman Adaptive Moving Average
//! * [`KC`](Core::kc) — Keltner Channels
//! * [`MA`](Core::ma) — Moving average
//! * [`MAMA`](Core::mama) — MESA Adaptive Moving Average
//! * [`MAVP`](Core::mavp) — Moving average with variable period
//! * [`MIDPOINT`](Core::midpoint) — MidPoint over period
//! * [`MIDPRICE`](Core::midprice) — Midpoint Price over period
//! * [`RMA`](Core::rma) — Wilder's Smoothed Moving Average
//! * [`SAR`](Core::sar) — Parabolic SAR
//! * [`SAREXT`](Core::sarext) — Parabolic SAR - Extended
//! * [`SMA`](Core::sma) — Simple Moving Average
//! * [`SUPERTREND`](Core::supertrend) — SuperTrend
//! * [`T3`](Core::t3) — Triple Exponential Moving Average (T3)
//! * [`TEMA`](Core::tema) — Triple Exponential Moving Average
//! * [`TRIMA`](Core::trima) — Triangular Moving Average
//! * [`VWMA`](Core::vwma) — Volume Weighted Moving Average
//! * [`WMA`](Core::wma) — Weighted Moving Average
//! * [`ZLEMA`](Core::zlema) — Zero-Lag Exponential Moving Average
//!
//! ## Pattern Recognition (61)
//!
//! * [`CDL2CROWS`](Core::cdl2crows) — Two Crows
//! * [`CDL3BLACKCROWS`](Core::cdl3blackcrows) — Three Black Crows
//! * [`CDL3INSIDE`](Core::cdl3inside) — Three Inside Up/Down
//! * [`CDL3LINESTRIKE`](Core::cdl3linestrike) — Three-Line Strike
//! * [`CDL3OUTSIDE`](Core::cdl3outside) — Three Outside Up/Down
//! * [`CDL3STARSINSOUTH`](Core::cdl3starsinsouth) — Three Stars In The South
//! * [`CDL3WHITESOLDIERS`](Core::cdl3whitesoldiers) — Three Advancing White Soldiers
//! * [`CDLABANDONEDBABY`](Core::cdlabandonedbaby) — Abandoned Baby
//! * [`CDLADVANCEBLOCK`](Core::cdladvanceblock) — Advance Block
//! * [`CDLBELTHOLD`](Core::cdlbelthold) — Belt-hold
//! * [`CDLBREAKAWAY`](Core::cdlbreakaway) — Breakaway
//! * [`CDLCLOSINGMARUBOZU`](Core::cdlclosingmarubozu) — Closing Marubozu
//! * [`CDLCONCEALBABYSWALL`](Core::cdlconcealbabyswall) — Concealing Baby Swallow
//! * [`CDLCOUNTERATTACK`](Core::cdlcounterattack) — Counterattack
//! * [`CDLDARKCLOUDCOVER`](Core::cdldarkcloudcover) — Dark Cloud Cover
//! * [`CDLDOJI`](Core::cdldoji) — Doji
//! * [`CDLDOJISTAR`](Core::cdldojistar) — Doji Star
//! * [`CDLDRAGONFLYDOJI`](Core::cdldragonflydoji) — Dragonfly Doji
//! * [`CDLENGULFING`](Core::cdlengulfing) — Engulfing Pattern
//! * [`CDLEVENINGDOJISTAR`](Core::cdleveningdojistar) — Evening Doji Star
//! * [`CDLEVENINGSTAR`](Core::cdleveningstar) — Evening Star
//! * [`CDLGAPSIDESIDEWHITE`](Core::cdlgapsidesidewhite) — Up/Down-gap side-by-side white lines
//! * [`CDLGRAVESTONEDOJI`](Core::cdlgravestonedoji) — Gravestone Doji
//! * [`CDLHAMMER`](Core::cdlhammer) — Hammer
//! * [`CDLHANGINGMAN`](Core::cdlhangingman) — Hanging Man
//! * [`CDLHARAMI`](Core::cdlharami) — Harami Pattern
//! * [`CDLHARAMICROSS`](Core::cdlharamicross) — Harami Cross Pattern
//! * [`CDLHIGHWAVE`](Core::cdlhighwave) — High-Wave Candle
//! * [`CDLHIKKAKE`](Core::cdlhikkake) — Hikkake Pattern
//! * [`CDLHIKKAKEMOD`](Core::cdlhikkakemod) — Modified Hikkake Pattern
//! * [`CDLHOMINGPIGEON`](Core::cdlhomingpigeon) — Homing Pigeon
//! * [`CDLIDENTICAL3CROWS`](Core::cdlidentical3crows) — Identical Three Crows
//! * [`CDLINNECK`](Core::cdlinneck) — In-Neck Pattern
//! * [`CDLINVERTEDHAMMER`](Core::cdlinvertedhammer) — Inverted Hammer
//! * [`CDLKICKING`](Core::cdlkicking) — Kicking
//! * [`CDLKICKINGBYLENGTH`](Core::cdlkickingbylength) — Kicking - bull/bear determined by the longer marubozu
//! * [`CDLLADDERBOTTOM`](Core::cdlladderbottom) — Ladder Bottom
//! * [`CDLLONGLEGGEDDOJI`](Core::cdllongleggeddoji) — Long Legged Doji
//! * [`CDLLONGLINE`](Core::cdllongline) — Long Line Candle
//! * [`CDLMARUBOZU`](Core::cdlmarubozu) — Marubozu
//! * [`CDLMATCHINGLOW`](Core::cdlmatchinglow) — Matching Low
//! * [`CDLMATHOLD`](Core::cdlmathold) — Mat Hold
//! * [`CDLMORNINGDOJISTAR`](Core::cdlmorningdojistar) — Morning Doji Star
//! * [`CDLMORNINGSTAR`](Core::cdlmorningstar) — Morning Star
//! * [`CDLONNECK`](Core::cdlonneck) — On-Neck Pattern
//! * [`CDLPIERCING`](Core::cdlpiercing) — Piercing Pattern
//! * [`CDLRICKSHAWMAN`](Core::cdlrickshawman) — Rickshaw Man
//! * [`CDLRISEFALL3METHODS`](Core::cdlrisefall3methods) — Rising/Falling Three Methods
//! * [`CDLSEPARATINGLINES`](Core::cdlseparatinglines) — Separating Lines
//! * [`CDLSHOOTINGSTAR`](Core::cdlshootingstar) — Shooting Star
//! * [`CDLSHORTLINE`](Core::cdlshortline) — Short Line Candle
//! * [`CDLSPINNINGTOP`](Core::cdlspinningtop) — Spinning Top
//! * [`CDLSTALLEDPATTERN`](Core::cdlstalledpattern) — Stalled Pattern
//! * [`CDLSTICKSANDWICH`](Core::cdlsticksandwich) — Stick Sandwich
//! * [`CDLTAKURI`](Core::cdltakuri) — Takuri (Dragonfly Doji with very long lower shadow)
//! * [`CDLTASUKIGAP`](Core::cdltasukigap) — Tasuki Gap
//! * [`CDLTHRUSTING`](Core::cdlthrusting) — Thrusting Pattern
//! * [`CDLTRISTAR`](Core::cdltristar) — Tristar Pattern
//! * [`CDLUNIQUE3RIVER`](Core::cdlunique3river) — Unique 3 River
//! * [`CDLUPSIDEGAP2CROWS`](Core::cdlupsidegap2crows) — Upside Gap Two Crows
//! * [`CDLXSIDEGAP3METHODS`](Core::cdlxsidegap3methods) — Upside/Downside Gap Three Methods
//!
//! ## Price Transform (6)
//!
//! * [`AVGDEV`](Core::avgdev) — Average Deviation
//! * [`AVGPRICE`](Core::avgprice) — Average Price
//! * [`HA`](Core::ha) — Heikin-Ashi Candles
//! * [`MEDPRICE`](Core::medprice) — Median Price
//! * [`TYPPRICE`](Core::typprice) — Typical Price
//! * [`WCLPRICE`](Core::wclprice) — Weighted Close Price
//!
//! ## Statistic Functions (11)
//!
//! * [`BETA`](Core::beta) — Beta
//! * [`CORREL`](Core::correl) — Pearson's Correlation Coefficient (r)
//! * [`LINEARREG`](Core::linearreg) — Linear Regression
//! * [`LINEARREG_ANGLE`](Core::linearreg_angle) — Linear Regression Angle
//! * [`LINEARREG_INTERCEPT`](Core::linearreg_intercept) — Linear Regression Intercept
//! * [`LINEARREG_SLOPE`](Core::linearreg_slope) — Linear Regression Slope
//! * [`PERCENTILE`](Core::percentile) — Percentile (nearest rank)
//! * [`PERCENTRANK`](Core::percentrank) — Percent Rank
//! * [`STDDEV`](Core::stddev) — Standard Deviation
//! * [`TSF`](Core::tsf) — Time Series Forecast
//! * [`VAR`](Core::var) — Variance
//!
//! ## Volatility Indicators (7)
//!
//! * [`ADR`](Core::adr) — Average Day Range
//! * [`ATR`](Core::atr) — Average True Range
//! * [`CVI`](Core::cvi) — Chaikin's Volatility
//! * [`MASSI`](Core::massi) — Mass Index
//! * [`NATR`](Core::natr) — Normalized Average True Range
//! * [`RVI`](Core::rvi) — Relative Volatility Index
//! * [`TRANGE`](Core::trange) — True Range
//!
//! ## Volume Indicators (12)
//!
//! * [`AD`](Core::ad) — Chaikin A/D Line
//! * [`ADOSC`](Core::adosc) — Chaikin A/D Oscillator
//! * [`CMF`](Core::cmf) — Chaikin Money Flow
//! * [`EFI`](Core::efi) — Elder's Force Index
//! * [`MARKETFI`](Core::marketfi) — Market Facilitation Index
//! * [`NVI`](Core::nvi) — Negative Volume Index
//! * [`OBV`](Core::obv) — On Balance Volume
//! * [`PVI`](Core::pvi) — Positive Volume Index
//! * [`PVO`](Core::pvo) — Percentage Volume Oscillator
//! * [`PVT`](Core::pvt) — Price Volume Trend
//! * [`RVOL`](Core::rvol) — Relative Volume
//! * [`VWAP`](Core::vwap) — Volume Weighted Average Price

#![forbid(unsafe_code)]
// Every public item, and every public enum variant and struct field, carries its
// own documentation (#179 D7). `warn` rather than `deny` so that a future rustc
// widening the lint cannot break a downstream build; the nightly's
// `cargo clippy -- -D warnings` is what makes it a gate here.
#![warn(missing_docs)]
#![allow(non_snake_case, non_camel_case_types, unused_variables, unused_assignments, unused_mut, unused_parens)]
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
