//! The empty series and the sub-lookback range, on the Rust batch API's **public**
//! tier — #179 C8, and the one sentence three backends carry about how Rust bounds
//! its inputs.
//!
//! C8 says the only natural call for an empty slice, `(0, 0)`, "hits
//! `assert!(_assertStart > endIdx || endIdx < inReal0.len())` → **panic**". That
//! reaches the assertion preamble in `<N>_Impl`, which is `pub(crate)`. The public
//! wrapper never gets there: `rust_lang::gen_argument_checks` runs its own bounds
//! first and answers `BadParam`. C8's remaining half is untouched by that and still
//! stands — there is no `(startIdx, endIdx)` that means "no bars", so an empty
//! series has no successful spelling at all — but the panic is not the shape of it.
//!
//! The second claim is the input bound. `java::gen_argument_checks`, `Core.java`'s
//! `clampedStart` and `Core.cs`'s `ClampedStart` each say Rust applies its
//! `_assertStart > endIdx ||` escape to BOTH bounds, so that the input bound is
//! "the one place Java/C# check more than C and Rust do", `Core.cs` naming
//! `SMA(0, 5, &[], 30, &mut [])` as `Ok(count 0)` in Rust where C# throws. True of
//! the `_Impl` asserts, which is what the JSON-RPC servers exercise; not true of
//! the public tier, where `rust_lang.rs` takes the escape on the output bound only
//! and the two backends agree. Nothing pinned that, which is how the sentence
//! outlived it.
//!
//! An integration test on purpose, like `nullable_outputs.rs`: the subject is the
//! public tier, and the in-crate sweeps cannot see it. None of this is reachable
//! from the cross-language gates either — the servers hand every input the full
//! series, so they never present a slice that stops short of `endIdx`.
//!
//! Every call below returns rather than panics; a panic fails the test outright,
//! which is the C8 check.

#![allow(non_snake_case)]

use ta_lib::{Core, OutRange, RetCode};

fn series(n: usize) -> Vec<f64> {
    (0..n).map(|i| 100.0 + 10.0 * (0.1 * i as f64).sin()).collect()
}

/// C8's case: an empty series, asked for over the only range that spells "the
/// whole of it". One function per input arity that the zero-lookback family
/// covers — `SQRT` reads one series, `ADD` two, `AVGPRICE` and `AD` four — plus a
/// period-taking function at `period = 1`, which is how the other 148 functions
/// join the family.
///
/// `BadParam` on every one, from the wrapper, with the preamble unreached.
///
/// The output buffer holds one value, which is the row that isolates the input
/// bound: `(0, 0)` produces one value at a zero lookback, so an output of length
/// zero is rejected by the OUTPUT bound and would answer `BadParam` whatever the
/// input bound did. Given the room it asks for, only the input bound is left
/// between the call and the assertion preamble — which is what C8 describes
/// reaching. The empty-output rows follow, for the caller who sizes the output
/// to the series.
#[test]
fn the_empty_series_is_bad_param_and_does_not_panic() {
    let core = Core::new();
    let none: [f64; 0] = [];
    let mut one = [0.0_f64; 1];

    assert_eq!(core.SQRT(0, 0, &none, &mut one), Err(RetCode::BadParam));
    assert_eq!(core.ADD(0, 0, &none, &none, &mut one), Err(RetCode::BadParam));
    assert_eq!(
        core.AVGPRICE(0, 0, &none, &none, &none, &none, &mut one),
        Err(RetCode::BadParam)
    );
    assert_eq!(
        core.AD(0, 0, &none, &none, &none, &none, &mut one),
        Err(RetCode::BadParam)
    );
    assert_eq!(core.SMA(0, 0, &none, 1, &mut one), Err(RetCode::BadParam));

    // Same calls with the output sized to the (empty) series. Here either bound
    // is enough to answer, so these rows say only that the answer is a value.
    let mut out: [f64; 0] = [];
    assert_eq!(core.SQRT(0, 0, &none, &mut out), Err(RetCode::BadParam));
    assert_eq!(core.SMA(0, 0, &none, 1, &mut out), Err(RetCode::BadParam));
}

/// The other spelling a caller reaches for — `endIdx` one below `startIdx`, so
/// that the range holds nothing — is rejected before any buffer is looked at.
///
/// Together with the test above this is C8's surviving half: an empty range is
/// not expressible. Both answers are errors, so a caller with no bars yet cannot
/// distinguish "nothing to do" from a mistake without checking the length itself.
/// Which of the two answers is right is the C8 decision and is not taken here.
#[test]
fn an_inverted_range_is_an_end_index_error() {
    let core = Core::new();
    let data = series(8);
    let mut out = vec![0.0; 8];

    assert_eq!(
        core.SQRT(1, 0, &data, &mut out),
        Err(RetCode::OutOfRangeEndIndex)
    );
    // With no series at all it is still the range that is answered, not the
    // length: B1/B2 sit ahead of the buffer bounds.
    let none: [f64; 0] = [];
    let mut no_out: [f64; 0] = [];
    assert_eq!(
        core.SQRT(1, 0, &none, &mut no_out),
        Err(RetCode::OutOfRangeEndIndex)
    );
}

/// A range shorter than the lookback frees the OUTPUT bound and not the input
/// bound — the asymmetry `rust_lang::gen_argument_checks` documents, and the one
/// the three sibling comments say Rust does not have.
///
/// `SMA(0, 5, .., 30, ..)` is `Core.cs`'s own example: lookback 29, range 6 bars,
/// so nothing is produced. The four rows vary only the two lengths.
#[test]
fn a_sub_lookback_range_frees_the_output_bound_and_not_the_input_bound() {
    let core = Core::new();
    let six = series(6);
    let five = series(5);
    let none: [f64; 0] = [];
    let mut out_six = vec![0.0; 6];
    let mut out_none: [f64; 0] = [];

    // Output bound off: no values are produced, so no output space is owed.
    assert_eq!(
        core.SMA(0, 5, &six, 30, &mut out_six),
        Ok(OutRange { beg_idx: 0, count: 0 })
    );
    assert_eq!(
        core.SMA(0, 5, &six, 30, &mut out_none),
        Ok(OutRange { beg_idx: 0, count: 0 })
    );

    // Input bound on: the series must still reach `endIdx`, sub-lookback or not.
    assert_eq!(
        core.SMA(0, 5, &five, 30, &mut out_none),
        Err(RetCode::BadParam)
    );
    // The row `Core.cs` names as `Ok(count 0)` here and a throw there. It is
    // `BadParam` here, which is the same verdict C# reaches by throwing.
    assert_eq!(
        core.SMA(0, 5, &none, 30, &mut out_none),
        Err(RetCode::BadParam)
    );
}

/// The input bound is `endIdx + 1` exactly, and it does not move with the
/// lookback — the boundary that tells an off-by-one from a bound that is merely
/// present.
#[test]
fn the_input_bound_is_end_idx_plus_one_at_any_lookback() {
    let core = Core::new();
    let mut out = vec![0.0; 8];

    for period in [1, 2, 30] {
        let exact = series(6);
        let short = series(5);
        // Six bars reach `endIdx = 5`.
        assert!(
            core.SMA(0, 5, &exact, period, &mut out).is_ok(),
            "period {period}: a series reaching endIdx was rejected"
        );
        // Five do not, whatever the period says about how many values come out.
        assert_eq!(
            core.SMA(0, 5, &short, period, &mut out),
            Err(RetCode::BadParam),
            "period {period}: a series one short of endIdx was accepted"
        );
    }
}
