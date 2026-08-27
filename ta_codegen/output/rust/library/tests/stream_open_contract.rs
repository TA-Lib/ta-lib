//! Rule S1 for the Rust streaming openers — `docs/error-handling-spec.md` 2.3,
//! issue #268.
//!
//! An opener is a batch call over `[0, historyLen - 1]`, so an empty history is
//! B1's condition read on that range: the implied `startIdx` of 0 names no bar,
//! and it answers B1's code rather than the catch-all.
//!
//! No cross-language gate reaches it. The JSON-RPC servers hand every opener the
//! full series, so a backend that went back to `BadParam` here stays green in
//! `--codegen` and `--xlang-hash` alike — and the four backends have to agree,
//! because that is the whole point of giving the fault its own code.
//!
//! Rules S4 (an absent argument) and S2 (a history past `MAX_INDEX`) are not
//! here: a slice cannot be absent, and provoking S2 needs a 100 000 001-element
//! allocation.

#![allow(non_snake_case)]

use ta_lib::{Core, MAType, RetCode};

fn series(n: usize) -> Vec<f64> {
    (0..n).map(|i| 100.0 + 10.0 * (0.1 * i as f64).sin()).collect()
}

#[test]
fn an_empty_history_is_an_index_fault() {
    let core = Core::new();
    let empty: [f64; 0] = [];
    let mut out = [0.0_f64; 8];

    assert_eq!(core.SMA_Open(&empty, 30).err(), Some(RetCode::OutOfRangeStartIndex));
    assert_eq!(
        core.SMA_OpenAndFill(&empty, 30, &mut out).err(),
        Some(RetCode::OutOfRangeStartIndex)
    );
    // A candlestick reaches it through four legs, and the dispatch tier and the
    // period bank hand-roll their own prologue.
    assert_eq!(
        core.CDLDOJI_Open(&empty, &empty, &empty, &empty).err(),
        Some(RetCode::OutOfRangeStartIndex)
    );
    assert_eq!(
        core.MA_Open(&empty, 30, MAType::EMA).err(),
        Some(RetCode::OutOfRangeStartIndex)
    );
    assert_eq!(
        core.MAVP_Open(&empty, &empty, 2, 30, MAType::SMA).err(),
        Some(RetCode::OutOfRangeStartIndex)
    );
}

/// The controls, without which the assertions above would pass against an
/// opener that answered `OutOfRangeStartIndex` for everything.
#[test]
fn the_other_rejections_keep_their_own_codes() {
    let core = Core::new();
    let data = series(252);

    // A one-bar history is inside the index domain: that is S7's business.
    assert_eq!(
        core.SMA_Open(&data[..1], 30).err(),
        Some(RetCode::InsufficientHistory)
    );
    // A parameter outside its range is still the catch-all.
    assert_eq!(core.SMA_Open(&data, 0).err(), Some(RetCode::BadParam));
    // So is a second input of a different length — `historyLen` is the FIRST
    // input's, so a later one disagreeing is an argument fault, not an empty
    // history. This is the half the emptiness check used to be fused with.
    assert_eq!(
        core.STOCH_Open(&data, &data[..100], &data, 5, 3, MAType::SMA, 3, MAType::SMA).err(),
        Some(RetCode::BadParam)
    );
    // ...including when the disagreeing one is the empty slice.
    assert_eq!(
        core.STOCH_Open(&data, &[], &data, 5, 3, MAType::SMA, 3, MAType::SMA).err(),
        Some(RetCode::BadParam)
    );
    // And a full history still opens.
    assert!(core.SMA_Open(&data, 30).is_ok());
}
