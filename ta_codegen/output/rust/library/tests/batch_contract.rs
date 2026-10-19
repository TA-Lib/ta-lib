//! Rule rB4's empty output and rB5's two bounds, for the Rust batch API
//! (`https://ta-lib.org/spec/errors/#rb4`), issues #262 and #265.
//!
//! None of it is reachable from the cross-language gates. The JSON-RPC servers
//! supply every declared output and floor its length at one, and hand every
//! input the full series, so a backend that went back to accepting an empty
//! output, or a slice that does not reach `endIdx`, stays green in `--codegen`,
//! `--xlang-hash` and `--ref` alike.
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

/// An undersized output is refused by the public tier as a code (#265). The
/// body's assert states the same bound and is unreachable through here.
#[test]
fn a_short_output_is_refused_rather_than_panicked() {
    let data = series(252);
    let core = Core::new();
    let exact = 252 - core.mama_lookback(0.5, 0.05).expect("a valid lookback");

    let (mut mama, mut fama) = (vec![0.0; exact], vec![0.0; exact]);
    core.mama(0, 251, &data, 0.5, 0.05, &mut mama, &mut fama)
        .expect("outputs sized to the produced count exactly");

    for (m, f) in [(exact - 1, exact), (exact, exact - 1)] {
        let (mut mama, mut fama) = (vec![0.0; m], vec![0.0; f]);
        assert_eq!(
            core.mama(0, 251, &data, 0.5, 0.05, &mut mama, &mut fama),
            Err(RetCode::BadParam),
            "an output one short ({m}, {f})"
        );
        assert!(
            !panics(|| {
                let core = Core::new();
                let (mut mama, mut fama) = (vec![0.0; m], vec![0.0; f]);
                let _ = core.mama(0, 251, &data, 0.5, 0.05, &mut mama, &mut fama);
            }),
            "rejected rather than panicked ({m}, {f})"
        );
    }
}

/// An empty output is absent, and the call is refused before any length is
/// looked at, on a range that produces nothing as on one that produces values.
/// One element is enough for the control, which is what tells this from the
/// length rule.
#[test]
fn an_empty_output_is_an_absent_one() {
    let data = series(252);
    let core = Core::new();
    let period = 253; // longer than the range, so nothing is produced
    assert!(
        core.accbands_lookback(period).expect("a valid lookback") > 251,
        "the probe needs a lookback past the range, or it proves nothing"
    );

    let mut a: Vec<f64> = Vec::new();
    let mut b: Vec<f64> = Vec::new();
    let mut c: Vec<f64> = Vec::new();
    assert_eq!(
        core.accbands(0, 251, &data, &data, &data, period, &mut a, &mut b, &mut c),
        Err(RetCode::BadParam),
        "empty outputs on a range that produces nothing"
    );
    assert_eq!(
        core.accbands(0, 251, &data, &data, &data, 20, &mut a, &mut b, &mut c),
        Err(RetCode::BadParam),
        "and on a range that produces values"
    );

    // Zero-length subslices of a real buffer are empty all the same.
    let mut buf = [0.0_f64; 3];
    let (x, rest) = buf.split_at_mut(1);
    let (y, z) = rest.split_at_mut(1);
    assert_eq!(
        core.accbands(0, 251, &data, &data, &data, period, x, &mut y[..0], z),
        Err(RetCode::BadParam),
        "one empty output among three"
    );
    let r = core
        .accbands(0, 251, &data, &data, &data, period, x, y, z)
        .expect("one element each is enough when nothing is produced");
    assert_eq!(r, OutRange { beg_idx: 0, count: 0 });

    let mama_end = core.mama_lookback(0.5, 0.05).expect("a valid lookback") - 1;
    let mut one = [0.0_f64; 1];
    let mut two = [0.0_f64; 1];
    let mut none: [f64; 0] = [];
    assert_eq!(
        core.mama(0, mama_end, &data, 0.5, 0.05, &mut none, &mut one),
        Err(RetCode::BadParam),
        "an empty outMAMA"
    );
    assert_eq!(
        core.mama(0, mama_end, &data, 0.5, 0.05, &mut one, &mut none),
        Err(RetCode::BadParam),
        "an empty outFAMA"
    );
    let r = core
        .mama(0, mama_end, &data, 0.5, 0.05, &mut one, &mut two)
        .expect("one element each is enough when nothing is produced");
    assert_eq!(r.count, 0);

    assert_eq!(
        core.mama_open_and_fill(&data[..=mama_end], 0.5, 0.05, &mut one, &mut none).err(),
        Some(RetCode::BadParam),
        "the opener refuses an empty outFAMA ahead of the history check"
    );
    assert_eq!(
        core.mama_open_and_fill(&data[..=mama_end], 0.5, 0.05, &mut one, &mut two).err(),
        Some(RetCode::InsufficientHistory)
    );
}

/// rB5's input half, which the public tier states without the sub-lookback escape
/// the body's assert takes — so a short input is refused on a range that
/// produces nothing as readily as on one that does (#265, input length).
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
        core.sma(0, 251, &short, 30, &mut out),
        Err(RetCode::BadParam),
        "an input that does not reach endIdx is a caller bug"
    );

    // And one that does not: startIdx..=endIdx is shorter than the lookback, so
    // no value is produced and one element of output is enough. The input bound
    // holds anyway.
    let mut none = vec![0.0; 1];
    assert!(
        core.sma_lookback(30).expect("a valid lookback") > 10,
        "the probe needs a sub-lookback range, or it repeats the case above"
    );
    assert_eq!(
        core.sma(0, 10, &short[..5], 30, &mut none),
        Err(RetCode::BadParam),
        "the input bound takes no sub-lookback escape"
    );

    // Control: the same call with the input one element longer succeeds, so the
    // rejections above are the length and not the fixture.
    assert_eq!(
        core.sma(0, 10, &short[..11], 30, &mut none),
        Ok(OutRange { beg_idx: 0, count: 0 })
    );
}

/// `MA`'s MAMA arm writes FAMA into a scratch it drops, and answers what a direct
/// `MAMA` does.
#[test]
fn ma_routes_only_the_mama_line() {
    let data = series(252);
    let core = Core::new();

    let mut direct = vec![0.0; 252];
    let mut fama = vec![0.0; 252];
    let r_direct = core
        .mama(0, 251, &data, 0.5, 0.05, &mut direct, &mut fama)
        .expect("MAMA");

    let mut viaMa = vec![0.0; 252];
    let r_ma = core
        .ma(0, 251, &data, 30, ta_lib::MAType::MAMA, &mut viaMa)
        .expect("MA(MAMA)");

    assert_eq!((r_ma.beg_idx, r_ma.count), (r_direct.beg_idx, r_direct.count));
    for i in 0..r_ma.count {
        assert_eq!(viaMa[i].to_bits(), direct[i].to_bits(), "MA(MAMA) at {i}");
    }
    assert_eq!(RetCode::Success.as_c_int(), 0, "the crate's codes are C's");
}
