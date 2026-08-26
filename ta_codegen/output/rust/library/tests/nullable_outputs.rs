//! Rule B6a, the empty-output half of rule B6, and B5's two bounds, for the Rust
//! batch API — `docs/error-handling-spec.md` 2.2 and Appendix F, issues #262 and
//! #265.
//!
//! None of it is reachable from the cross-language gates. The JSON-RPC servers
//! supply every declared output and floor its length at one, and hand every
//! input the full series, so a backend that went back to requiring `outFAMA`, to
//! rejecting distinct empty buffers, or to accepting a slice that does not reach
//! `endIdx`, stays green in `--codegen`, `--xlang-hash` and `--fuzz-064` alike.
//! That is what this file is for.
//!
//! It is an **integration** test on purpose, where the phantom-I/O sweep is
//! in-crate: these are public-API rules, so probing the public tier is the
//! subject rather than a limitation (#265).

#![allow(non_snake_case)]

use ta_lib::{Core, OutRange, RetCode};

fn series(n: usize) -> Vec<f64> {
    (0..n).map(|i| 100.0 + 10.0 * (0.1 * i as f64).sin()).collect()
}

/// Run `f` with the panic message suppressed, and say whether it panicked. The
/// expected panics below are the assertion preamble doing its job; printing
/// their backtraces would read as failures in a passing run.
fn panics(f: impl FnOnce()) -> bool {
    let prior = std::panic::take_hook();
    std::panic::set_hook(Box::new(|_| {}));
    let outcome = std::panic::catch_unwind(std::panic::AssertUnwindSafe(f));
    std::panic::set_hook(prior);
    outcome.is_err()
}

/// A nullable output is `Option<&mut [f64]>`, and declining it changes nothing
/// about the output that was asked for.
///
/// Acceptance alone would not test that: a body that stopped computing FAMA, or
/// took a different path without it, would be accepted here just the same. So
/// the declining call has to reproduce the supplied one bit for bit.
#[test]
fn declining_an_output_changes_nothing_else() {
    let data = series(252);
    let core = Core::new();

    let mut mama_ref = vec![0.0; 252];
    let mut fama_ref = vec![0.0; 252];
    let r_ref = core
        .MAMA(0, 251, &data, 0.5, 0.05, &mut mama_ref, Some(&mut fama_ref))
        .expect("the reference call");
    assert!(r_ref.count > 0, "the reference call must produce values");

    const CANARY: f64 = -1.2345678901234e300;
    let mut mama = vec![CANARY; 252];
    let r = core
        .MAMA(0, 251, &data, 0.5, 0.05, &mut mama, None)
        .expect("declining outFAMA is legal — rule B6a");

    assert_eq!((r.beg_idx, r.count), (r_ref.beg_idx, r_ref.count));
    for i in 0..r.count {
        assert_eq!(
            mama[i].to_bits(),
            mama_ref[i].to_bits(),
            "declining outFAMA changed outMAMA[{i}]"
        );
    }
    // Rule N2: only the reported range is written, declined output or not.
    assert!(
        mama[r.count..].iter().all(|v| v.to_bits() == CANARY.to_bits()),
        "the declining call wrote past its own count"
    );
}

/// The capacity assert (B5) is skipped for a declined output and kept for the
/// one that was supplied — the half a caller actually sees.
#[test]
fn a_declined_output_needs_no_capacity() {
    let data = series(252);
    let core = Core::new();

    // Nothing at all for FAMA, and MAMA sized to the produced count exactly.
    let lookback = core.MAMA_Lookback(0.5, 0.05).expect("a valid lookback");
    let mut mama = vec![0.0; 252 - lookback];
    core.MAMA(0, 251, &data, 0.5, 0.05, &mut mama, None)
        .expect("a declined output imposes no size");

    // Control: the supplied output is still bounded. One short must be rejected
    // — B5, from the public tier, as a code (#265). The body's assert states the
    // same bound and is unreachable through here.
    let mut short = vec![0.0; 252 - lookback - 1];
    assert_eq!(
        core.MAMA(0, 251, &data, 0.5, 0.05, &mut short, None),
        Err(RetCode::BadParam),
        "an undersized supplied output must still be rejected"
    );
    assert!(
        !panics(|| {
            let core = Core::new();
            let mut short = vec![0.0; 252 - lookback - 1];
            let _ = core.MAMA(0, 251, &data, 0.5, 0.05, &mut short, None);
        }),
        "and rejected rather than panicked: the public tier answers before the assert"
    );
}

/// Appendix D item 11: three separately allocated empty `Vec`s are three
/// distinct buffers, and a range shorter than the lookback produces nothing, so
/// the call is a success with an empty range (rule N1) — as it always was in C
/// and Java.
///
/// It used to answer `BadParam`: every unallocated `Vec` hands out the same
/// dangling aligned pointer, and the guard compared `as_ptr()`. Zero-length
/// SUBSLICES of one buffer have real addresses and were accepted, so Rust
/// rejected some empty triples and accepted others.
#[test]
fn distinct_empty_outputs_are_not_aliases() {
    let data = series(252);
    let core = Core::new();
    let period = 253; // longer than the range, so nothing is produced
    assert!(
        core.ACCBANDS_Lookback(period).expect("a valid lookback") > 251,
        "the probe needs a lookback past the range, or it proves nothing"
    );

    let mut a: Vec<f64> = Vec::new();
    let mut b: Vec<f64> = Vec::new();
    let mut c: Vec<f64> = Vec::new();
    assert_eq!(
        (a.as_ptr(), b.as_ptr()),
        (b.as_ptr(), c.as_ptr()),
        "the probe is only interesting while empty Vecs share one dangling pointer"
    );
    let r = core
        .ACCBANDS(0, 251, &data, &data, &data, period, &mut a, &mut b, &mut c)
        .expect("a sub-lookback range needs no output space — rules N1 and B5");
    assert_eq!(r, OutRange { beg_idx: 0, count: 0 });

    // Zero-length subslices of one buffer carry real addresses. Same answer.
    let mut buf = [0.0_f64; 3];
    let (x, rest) = buf.split_at_mut(1);
    let (y, z) = rest.split_at_mut(1);
    let r = core
        .ACCBANDS(0, 251, &data, &data, &data, period, &mut x[..0], &mut y[..0], &mut z[..0])
        .expect("distinct empty subslices are distinct buffers too");
    assert_eq!(r, OutRange { beg_idx: 0, count: 0 });
}

/// Control for the test above: empty outputs are accepted because nothing is
/// produced, not because the bound went away. On a range that DOES produce
/// values the same three empty buffers must still be refused.
#[test]
fn empty_outputs_on_a_producing_range_still_fault() {
    let data = series(252);
    let core = Core::new();
    let mut a: Vec<f64> = Vec::new();
    let mut b: Vec<f64> = Vec::new();
    let mut c: Vec<f64> = Vec::new();
    assert_eq!(
        core.ACCBANDS(0, 251, &data, &data, &data, 20, &mut a, &mut b, &mut c),
        Err(RetCode::BadParam),
        "B5 must still bound an output that has to hold values"
    );
}

/// B5's input half, which the public tier states without the sub-lookback escape
/// the body's assert takes — so a short input is refused on a range that
/// produces nothing as readily as on one that does (#265, spec footnote [5]).
///
/// This is the one bound where the crate answers a caller that C cannot: C is
/// handed bare pointers and reads past the end. Java and C# have said it since
/// #260; here it used to be an assert with an escape, so the sub-lookback call
/// below was a silent `Ok`.
#[test]
fn a_short_input_is_refused_on_every_range() {
    let core = Core::new();
    let short = series(100);

    // A range that produces values.
    let mut out = vec![0.0; 252];
    assert_eq!(
        core.SMA(0, 251, &short, 30, &mut out),
        Err(RetCode::BadParam),
        "an input that does not reach endIdx is a caller bug"
    );

    // And one that does not: startIdx..=endIdx is shorter than the lookback, so
    // no value is produced and no output space is owed — the input bound holds
    // anyway.
    let mut none: Vec<f64> = Vec::new();
    assert!(
        core.SMA_Lookback(30).expect("a valid lookback") > 10,
        "the probe needs a sub-lookback range, or it repeats the case above"
    );
    assert_eq!(
        core.SMA(0, 10, &short[..5], 30, &mut none),
        Err(RetCode::BadParam),
        "the input bound takes no sub-lookback escape"
    );

    // Control: the same call with the input one element longer succeeds, so the
    // rejections above are the length and not the fixture.
    assert_eq!(
        core.SMA(0, 10, &short[..11], 30, &mut none),
        Ok(OutRange { beg_idx: 0, count: 0 })
    );
}

/// The cross-call inside `MA`'s MAMA arm declines FAMA rather than allocating a
/// buffer to throw away, and still answers what a direct `MAMA` does.
#[test]
fn ma_routes_only_the_mama_line() {
    let data = series(252);
    let core = Core::new();

    let mut direct = vec![0.0; 252];
    let r_direct = core
        .MAMA(0, 251, &data, 0.5, 0.05, &mut direct, None)
        .expect("MAMA");

    let mut viaMa = vec![0.0; 252];
    let r_ma = core
        .MA(0, 251, &data, 30, ta_lib::MAType::MAMA, &mut viaMa)
        .expect("MA(MAMA)");

    assert_eq!((r_ma.beg_idx, r_ma.count), (r_direct.beg_idx, r_direct.count));
    for i in 0..r_ma.count {
        assert_eq!(viaMa[i].to_bits(), direct[i].to_bits(), "MA(MAMA) at {i}");
    }
    assert_eq!(RetCode::Success.as_c_int(), 0, "the crate's codes are C's");
}
