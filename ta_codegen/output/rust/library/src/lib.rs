//! # TA-Lib: Technical Analysis Library
//!
//! 161 technical-analysis indicators — moving averages, momentum oscillators,
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
//! let (mut out_beg, mut out_nb) = (0, 0);
//!
//! let ret = core.sma(0, close.len() - 1, &close, 3, &mut out_beg, &mut out_nb, &mut sma);
//! assert_eq!(ret, RetCode::Success);
//!
//! // The first 3-period average lands at input index 2 (the lookback):
//! assert_eq!((out_beg, out_nb), (2, 8));
//! assert_eq!(sma[0], 12.0); // (11 + 12 + 13) / 3
//! ```
//!
//! # API shape
//!
//! Every indicator is a method on [`Core`] and follows the same pattern:
//!
//! * Inputs are `&[f64]` slices, computed over the range `startIdx..=endIdx`.
//! * Outputs are written into caller-provided `&mut` slices; `outBegIdx` receives the
//!   input index of the first output value and `outNBElement` the number of values
//!   written. An indicator consumes a number of leading values (its *lookback*)
//!   before producing output — query it with the matching `*_lookback` method
//!   (e.g. [`Core::sma_lookback`]).
//! * Integer parameters accept `i32::MIN` to select their default value.
//! * Every call returns a [`RetCode`]; anything other than [`RetCode::Success`]
//!   means no output was produced.
//!
//! [`Core`] is immutable after construction: its per-instance settings — unstable
//! period, Metastock [`Compatibility`], and candlestick thresholds — are chosen up
//! front with [`Core::builder()`] and then frozen, so a `Core` is `Send + Sync` and
//! can be shared read-only across threads (e.g. via `Arc`) with no locking:
//!
//! ```
//! use ta_lib::{Core, Compatibility, FuncUnstId};
//!
//! let core = Core::builder()
//!     .compatibility(Compatibility::Metastock)
//!     .unstable_period(FuncUnstId::Ema, 10)
//!     .build();
//! ```
//!
//! To change a setting, build a new `Core` (cloning is cheap); [`Core::to_builder()`]
//! seeds a builder from an existing instance.
//!
//! Every indicator also has an `*_unguarded` variant that skips parameter
//! validation for internal cross-indicator calls — prefer the checked methods.
//! The crate is `#![forbid(unsafe_code)]`: misuse of an `*_unguarded` variant
//! panics, it never triggers undefined behavior.
//!
//! The full function reference, grouped by category, is at
//! [ta-lib.org/functions](https://ta-lib.org/functions/).

#![forbid(unsafe_code)]
#![allow(non_snake_case, unused_variables, unused_assignments, unused_mut, unused_parens, arithmetic_overflow)]
// Generated code: Clippy's style/complexity lints are noise on machine output, and
// several "fixes" would change numeric behavior — e.g. `neg_cmp_op_on_partial_ord`
// on C's `!(a < b)` NaN idiom, or De Morgan rewrites under `nonminimal_bool`. The
// crate is verified bit-exact against the C reference, so these are suppressed rather
// than applied. `too_many_arguments` is inherent to the C API arity.
#![allow(clippy::all, clippy::pedantic)]
#![allow(clippy::approx_constant)] // PI (180/3.141592653589793) is copied verbatim from the C source.
pub mod ta_func;
pub mod abstract_api;
pub use ta_func::*;
