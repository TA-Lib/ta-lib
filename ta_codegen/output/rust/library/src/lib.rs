//! # TA-Lib: Technical Analysis Library
//!
//! 176 technical-analysis indicators — moving averages, momentum oscillators,
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
//! are bit-identical either way. The streaming tier stays single-path.
//!
//! The full function reference, grouped by category, is at
//! [ta-lib.org/functions](https://ta-lib.org/functions/).

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
