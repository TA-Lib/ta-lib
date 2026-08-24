//! DIV's documented zero-divisor result (issue #249).
//!
//! Hand-written, not generated: this file lives in
//! `ta_codegen/generator/templates/rust/div_zero.rs` and is copied verbatim into
//! the crate by `generate` (the Rust backend's `clean_keep` holds it, so
//! `generate` never deletes it). It is declared `#[cfg(test)]` in `mod.rs`, so
//! nothing here ships in a release build. Run it with
//! `cargo test --lib -p ta-lib`.
//!
//! DIV's published Notes say: "Zero divided by zero gives NaN; anything else
//! divided by zero gives positive or negative infinity. Neither is reported as
//! an error." That was true by construction in every backend and asserted
//! nowhere — no test in any language had ever handed DIV a zero divisor. The
//! generated doctest is the near miss: it divides two sine series and asserts
//! every output `is_finite()`, which passes only because those inputs never
//! divide by zero.
//!
//! One table covers every sign combination IEEE-754 division distinguishes,
//! plus two controls a wrongly-placed guard would break: a zero NUMERATOR over
//! a non-zero divisor stays a signed zero, and ordinary quotients are
//! untouched.
//!
//! The streaming tier takes the same table twice, because it emits two loops.
//! `DIV_OpenImpl` carries its own transcription of the batch body (the warm-up
//! fill) and `DIV_step_impl` carries the per-bar one, so a guard added to one
//! is invisible to the other — measured on the C side: an `_OpenImpl`-only
//! guard on a zero divisor survived every other assertion in the group. So the
//! whole table goes through `DIV_OpenAndFill`, and again through `peek`/
//! `update` on a second handle. Neither entry point may reject the bar: both
//! guard their INPUTS with `is_finite`, and a zero divisor is finite.
//!
//! The C group `test_div_zero.c` makes the same assertions against the shipped
//! C library and re-issues each batch call to every language server, which
//! compares this crate's output to C's bit for bit.

use crate::ta_func::types::{Core, RetCode};

/// `(numerator, denominator, expected)`. `None` means "the only defined answer
/// is NaN"; a `Some` value carries its SIGN as part of the claim. Every literal
/// is exactly representable, so nothing here depends on rounding.
const CASES: [(f64, f64, Option<f64>); 12] = [
    (0.0, 0.0, None),                        // +0 / +0 -> NaN
    (0.0, -0.0, None),                       // +0 / -0 -> NaN
    (-0.0, 0.0, None),                       // -0 / +0 -> NaN
    (-0.0, -0.0, None),                      // -0 / -0 -> NaN
    (1.5, 0.0, Some(f64::INFINITY)),         // +x / +0 -> +Inf
    (-1.5, 0.0, Some(f64::NEG_INFINITY)),    // -x / +0 -> -Inf
    (1.5, -0.0, Some(f64::NEG_INFINITY)),    // +x / -0 -> -Inf
    (-1.5, -0.0, Some(f64::INFINITY)),       // -x / -0 -> +Inf
    (0.0, 4.0, Some(0.0)),                   // control: +0 / +x -> +0
    (-0.0, 4.0, Some(-0.0)),                 // control: -0 / +x -> -0
    (6.0, 3.0, Some(2.0)),                   // control: ordinary quotient
    (-6.0, 3.0, Some(-2.0)),                 // control: ordinary quotient
];

/// Six rows need more than `==`: the four NaN rows, which are not comparable at
/// all, and the two signed-zero rows, where `+0.0 == -0.0`. (`==` separates
/// `+Inf` from `-Inf` on its own; the sign test is there for the zeros.)
/// Comparing bits instead would over-assert: the NaN PAYLOAD is the host's, not
/// the language's.
fn matches(got: f64, expected: Option<f64>) -> bool {
    match expected {
        None => got.is_nan(),
        Some(want) => got == want && got.is_sign_negative() == want.is_sign_negative(),
    }
}

fn inputs() -> (Vec<f64>, Vec<f64>) {
    (
        CASES.iter().map(|c| c.0).collect(),
        CASES.iter().map(|c| c.1).collect(),
    )
}

#[test]
fn batch_div_by_zero_succeeds_with_the_ieee_754_value() {
    let core = Core::new();
    let (num, den) = inputs();
    let mut out = vec![0.0_f64; CASES.len()];

    let range = core
        .DIV(0, CASES.len() - 1, &num, &den, &mut out)
        .expect("a zero divisor is not an error");
    assert_eq!(range.beg_idx, 0);
    assert_eq!(range.count, CASES.len(), "a zero divisor must not truncate the output");

    for (i, &(a, b, want)) in CASES.iter().enumerate() {
        assert!(matches(out[i], want), "DIV({a}, {b}) = {} at case {i}", out[i]);
    }
}

#[test]
fn a_sub_range_shifts_rather_than_recomputing_from_zero() {
    let core = Core::new();
    let (num, den) = inputs();
    let mut out = vec![0.0_f64; CASES.len()];

    // 4..=7 is the +/-Inf block, so a range that silently restarted at 0 would
    // land on the NaN rows and fail rather than looking right.
    let range = core.DIV(4, 7, &num, &den, &mut out).expect("a zero divisor is not an error");
    assert_eq!(range.beg_idx, 4);
    assert_eq!(range.count, 4);
    for i in 0..range.count {
        let (a, b, want) = CASES[4 + i];
        assert!(matches(out[i], want), "DIV({a}, {b}) = {} at case {}", out[i], 4 + i);
    }
}

#[test]
fn the_fill_loop_agrees_on_every_row() {
    // DIV_OpenImpl, not DIV_step_impl: a separate transcription of the batch
    // body, and the only assertion that reaches it.
    let core = Core::new();
    let (num, den) = inputs();
    let mut out = vec![0.0_f64; CASES.len()];

    let (stream, range) = core
        .DIV_OpenAndFill(&num, &den, &mut out)
        .expect("a zero divisor is not an error");
    assert_eq!(range.beg_idx, 0);
    assert_eq!(range.count, CASES.len());
    for (i, &(a, b, want)) in CASES.iter().enumerate() {
        assert!(matches(out[i], want), "fill DIV({a}, {b}) = {} at case {i}", out[i]);
    }
    assert_eq!(stream.out_range().count, CASES.len());

    // The same loop at stride 0: only the last row survives the sink, and it
    // must be that row rather than whatever the sink held.
    let (_, last) = core.DIV_Open(&num, &den).expect("a zero divisor is not an error");
    assert!(matches(last, CASES[CASES.len() - 1].2), "DIV_Open gave {last}");
}

#[test]
fn the_streaming_tier_agrees_and_does_not_reject_the_bar() {
    let core = Core::new();
    let (num, den) = inputs();

    let (mut stream, first) = core
        .DIV_Open(&num[..1], &den[..1])
        .expect("a zero divisor is not an error");
    assert!(matches(first, CASES[0].2), "DIV_Open gave {first}");

    for (i, &(a, b, want)) in CASES.iter().enumerate().skip(1) {
        // A zero divisor is a FINITE input. The tier rejects non-finite INPUTS;
        // a guard widened to the OUTPUT would surface here and nowhere else.
        let peeked = stream.peek(a, b).expect("a zero divisor is a finite bar");
        assert!(matches(peeked, want), "peek DIV({a}, {b}) = {peeked} at case {i}");
        let updated = stream.update(a, b).expect("a zero divisor is a finite bar");
        assert!(matches(updated, want), "update DIV({a}, {b}) = {updated} at case {i}");
        assert_eq!(peeked.to_bits(), updated.to_bits(), "peek != update at case {i}");
    }
    assert_eq!(stream.out_range().count, CASES.len());
}

#[test]
fn a_non_finite_bar_is_still_rejected() {
    // The control for the test above: the tier's own contract is unchanged, so
    // "does not reject a zero divisor" cannot be read as "rejects nothing".
    let core = Core::new();
    let (num, den) = inputs();
    let (mut stream, _) = core.DIV_Open(&num[..1], &den[..1]).unwrap();
    for bad in [f64::NAN, f64::INFINITY, f64::NEG_INFINITY] {
        // peek as well as update: they carry the guard separately, so a control
        // on one of them leaves the other's removable.
        assert!(matches!(stream.peek(bad, 1.0), Err(RetCode::BadParam)));
        assert!(matches!(stream.peek(1.0, bad), Err(RetCode::BadParam)));
        assert!(matches!(stream.update(bad, 1.0), Err(RetCode::BadParam)));
        assert!(matches!(stream.update(1.0, bad), Err(RetCode::BadParam)));
    }
}
