//! The ride-along: after every eligible batch request, the server replays that
//! request's own inputs through its streaming tiers and bit-compares against a
//! fresh internal `batch(0, m-1)`. One emitter per backend, all four emitting
//! the same field set -- `ride_along_suite.rs` pins that on emitted text,
//! because a runtime floor cannot see a comparison that was never emitted.

use std::fmt::Write as _;

pub(crate) mod c;
pub(crate) mod csharp;
pub(crate) mod java;
pub(crate) mod rust_lang;

/// `["inHigh", "inLow"]` -> `"inHigh, inLow, "`. A fold rather than
/// `map(format!).collect()`, which clippy::pedantic rejects.
fn ride_arg_list(names: &[String]) -> String {
    names.iter().fold(String::new(), |mut acc, n| {
        let _ = write!(acc, "{n}, ");
        acc
    })
}
