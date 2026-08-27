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
//! Rule S5 — the fill output's capacity — is here too, for the same reason:
//! the servers hand every opener an output the length of the whole history, so
//! nothing cross-language reaches a short one.
//!
//! Rules S4 (an absent argument) and S2 (a history past `MAX_INDEX`) are not
//! here: a slice cannot be absent, and provoking S2 needs a 100 000 001-element
//! allocation.
//!
//! Rule U6a is here too — a declined output at `update_and_fill` — because it is
//! the same declination S6a makes at the opener, read again per call, and the
//! two are only meaningful side by side.

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

/// Rule S5, from both sides. The bound is `historyLen - lookback` — the count
/// the fill actually writes, not the width of the history — so an
/// exactly-sized output has to be ACCEPTED and one element shorter REJECTED.
/// Only the pair pins the arithmetic: a bound of `historyLen` would reject the
/// first, and no bound at all would accept the second.
#[test]
fn the_fill_output_bound_from_both_sides() {
    let core = Core::new();
    let data = series(252);
    let lb = core.SMA_Lookback(30).expect("a valid period");
    let produced = data.len() - lb;

    assert_eq!(lb, 29, "the probe needs a lookback it can be one short of");
    assert!(produced < data.len(), "the produced count is shorter than the history");

    let mut exact = vec![0.0_f64; produced];
    let (_h, range) = core.SMA_OpenAndFill(&data, 30, &mut exact).expect("an exactly-sized output");
    assert_eq!(range.beg_idx, lb);
    assert_eq!(range.count, produced, "the fill wrote exactly the bound");

    let mut short = vec![0.0_f64; produced - 1];
    assert_eq!(
        core.SMA_OpenAndFill(&data, 30, &mut short).err(),
        Some(RetCode::BadParam),
        "one element short of the produced count is rejected"
    );

    // ...and rejected rather than panicked, which is the whole point: the
    // undersized slice used to fault inside the fill with the buffer already
    // partly written.
    let mut short2 = vec![0.0_f64; produced - 1];
    assert!(
        !panics(|| {
            let _ = core.SMA_OpenAndFill(&data, 30, &mut short2);
        }),
        "the public frame answers before the bounds assert"
    );
    assert!(short2.iter().all(|v| *v == 0.0), "a rejected fill wrote nothing");

    // An oversized output is legal and produces the identical values: the bound
    // is a minimum, and this is what says the exact case above was not luck.
    let mut roomy = vec![0.0_f64; data.len()];
    let (_h2, r2) = core.SMA_OpenAndFill(&data, 30, &mut roomy).expect("an oversized output");
    assert_eq!(r2.count, produced);
    assert!(
        exact.iter().zip(roomy.iter()).all(|(a, b)| a.to_bits() == b.to_bits()),
        "the exactly-sized fill is bit-identical to the roomy one"
    );
}

/// The bound on the tiers that hand-roll their own fill: the dispatch tier, the
/// period bank, and a multi-output composed function, whose sub-calls fill
/// scratch of their own rather than the caller's slices.
#[test]
fn the_fill_output_bound_holds_on_every_tier() {
    let core = Core::new();
    let data = series(252);
    let periods = vec![5.0_f64; 252];

    // Dispatch, including the identity arm, whose lookback is 0.
    for (period, ma) in [(30, MAType::EMA), (1, MAType::SMA)] {
        let lb = core.MA_Lookback(period, ma).expect("a valid period");
        let produced = data.len() - lb;
        let mut exact = vec![0.0_f64; produced];
        assert!(core.MA_OpenAndFill(&data, period, ma, &mut exact).is_ok(), "MA exact");
        let mut short = vec![0.0_f64; produced - 1];
        assert_eq!(
            core.MA_OpenAndFill(&data, period, ma, &mut short).err(),
            Some(RetCode::BadParam),
            "MA one short"
        );
    }

    // Period bank.
    let lb = core.MAVP_Lookback(2, 30, MAType::SMA).expect("a valid window");
    let produced = data.len() - lb;
    let mut exact = vec![0.0_f64; produced];
    assert!(core.MAVP_OpenAndFill(&data, &periods, 2, 30, MAType::SMA, &mut exact).is_ok());
    let mut short = vec![0.0_f64; produced - 1];
    assert_eq!(
        core.MAVP_OpenAndFill(&data, &periods, 2, 30, MAType::SMA, &mut short).err(),
        Some(RetCode::BadParam)
    );

    // Composed, three outputs: each is checked separately, so a short THIRD
    // output is rejected while the first two are exact.
    let lb = core.BBANDS_Lookback(20, 2.0, 2.0, MAType::SMA).expect("valid");
    let produced = data.len() - lb;
    let (mut a, mut b, mut c) = (vec![0.0; produced], vec![0.0; produced], vec![0.0; produced]);
    assert!(core.BBANDS_OpenAndFill(&data, 20, 2.0, 2.0, MAType::SMA, &mut a, &mut b, &mut c).is_ok());
    let (mut a2, mut b2, mut c2) = (vec![0.0; produced], vec![0.0; produced], vec![0.0; produced - 1]);
    assert_eq!(
        core.BBANDS_OpenAndFill(&data, 20, 2.0, 2.0, MAType::SMA, &mut a2, &mut b2, &mut c2).err(),
        Some(RetCode::BadParam),
        "each output is bounded separately"
    );
}

/// A history too short to produce anything must still reach S7. The capacity
/// bound floors at zero for exactly this reason: `historyLen - lookback` is
/// negative there, and the caller's mistake is the history, not the buffer.
#[test]
fn a_short_history_reaches_the_warm_up_check_not_the_capacity_one() {
    let core = Core::new();
    let data = series(252);
    let lb = core.SMA_Lookback(30).expect("valid");
    let mut nothing: [f64; 0] = [];
    assert_eq!(
        core.SMA_OpenAndFill(&data[..lb], 30, &mut nothing).err(),
        Some(RetCode::InsufficientHistory),
        "a history one short of lookback + 1 is S7, whatever the output holds"
    );
}

/// Rule B6a at the opener: `outFAMA` is `Option<&mut [f64]>`, and `None`
/// declines it exactly as it does in the batch tier.
///
/// Non-vacuous in three directions. The supplied run is the oracle, so a fill
/// that stopped computing FAMA when it is declined — the easy way to "support"
/// this — diverges on the FIRST `update` after the open, which reads the
/// handle's state rather than anything that was written out. The declined run
/// must still reject an undersized `outMAMA`, so the conditional bound cannot
/// have been dropped wholesale. And a SUPPLIED but undersized `outFAMA` is
/// still rejected, so "declinable" did not become "unchecked".
#[test]
fn a_declined_fill_output_is_still_computed() {
    let core = Core::new();
    let data = series(252);
    let lb = core.MAMA_Lookback(0.5, 0.05).expect("valid");
    let produced = data.len() - lb;

    let mut ref_mama = vec![0.0_f64; produced];
    let mut ref_fama = vec![0.0_f64; produced];
    let (mut both, both_range) = core
        .MAMA_OpenAndFill(&data, 0.5, 0.05, &mut ref_mama, Some(&mut ref_fama))
        .expect("a fully supplied open");

    let mut solo_mama = vec![0.0_f64; produced];
    let (mut declined, declined_range) = core
        .MAMA_OpenAndFill(&data, 0.5, 0.05, &mut solo_mama, None)
        .expect("declining outFAMA is not a rejection");

    assert_eq!(ref_mama, solo_mama, "declining outFAMA leaves outMAMA bit-identical");
    assert_eq!(both_range.beg_idx, declined_range.beg_idx);
    assert_eq!(both_range.count, declined_range.count);

    // The state, not just the write: FAMA feeds the next bar, so a handle that
    // skipped computing it answers differently here.
    let bar = data[data.len() - 1] + 1.0;
    let (m_both, f_both) = both.update(bar).expect("a finite bar");
    let (m_solo, f_solo) = declined.update(bar).expect("a finite bar");
    assert_eq!(m_both.to_bits(), m_solo.to_bits());
    assert_eq!(
        f_both.to_bits(),
        f_solo.to_bits(),
        "a declined outFAMA is still computed: the handle carries it"
    );

    let mut short_mama = vec![0.0_f64; produced - 1];
    assert_eq!(
        core.MAMA_OpenAndFill(&data, 0.5, 0.05, &mut short_mama, None).err(),
        Some(RetCode::BadParam),
        "an undersized outMAMA is still rejected when outFAMA is declined"
    );
    let mut full_mama = vec![0.0_f64; produced];
    let mut short_fama = vec![0.0_f64; produced - 1];
    assert_eq!(
        core.MAMA_OpenAndFill(&data, 0.5, 0.05, &mut full_mama, Some(&mut short_fama)).err(),
        Some(RetCode::BadParam),
        "a supplied outFAMA is still bounded"
    );
}

/// Run `f` with the panic message suppressed, and say whether it panicked.
fn panics(f: impl FnOnce()) -> bool {
    let prior = std::panic::take_hook();
    std::panic::set_hook(Box::new(|_| {}));
    let outcome = std::panic::catch_unwind(std::panic::AssertUnwindSafe(f));
    std::panic::set_hook(prior);
    outcome.is_err()
}

/// Rule U6a: `update_and_fill` declines a nullable output exactly as the opener
/// does, and the choice is the CALL's — the four open/fill combinations are all
/// accepted and all compute the same numbers.
///
/// Non-vacuous in the same three directions as the opener's probe, plus the one
/// this rule adds. The supplied run is the oracle, and the comparison that
/// cannot be satisfied by a backend that stopped computing FAMA is the handle's
/// own value after the fill, not the arrays. The mixed combinations are the
/// point of the rule: declining at `Open` and supplying here — and the reverse —
/// must be as ordinary as either matching pair. And a declining call must still
/// bound `outMAMA`, and still bound `outFAMA` when it IS supplied.
#[test]
fn a_declined_output_at_update_and_fill_is_a_property_of_the_call() {
    let core = Core::new();
    let data = series(252);
    let lb = core.MAMA_Lookback(0.5, 0.05).expect("valid");
    let produced = data.len() - lb;
    let bars: Vec<f64> = (0..8).map(|i| data[data.len() - 1] + 1.0 + i as f64 * 0.25).collect();
    // The fill buffers start at a canary, not at zero: comparing two arrays the
    // fill never wrote would otherwise pass on their shared initial value, which
    // is exactly the break the supplied/supplied leg below is meant to catch.
    const CANARY: f64 = -1.2345678901234e300;
    let canary = |n: usize| vec![CANARY; n];
    let written = |v: &[f64]| v.iter().all(|x| *x != CANARY);

    // Four handles over the same history — the two opener choices crossed with
    // the two fill choices below.
    let opened = |declined_at_open: bool| {
        let mut mama = vec![0.0_f64; produced];
        let mut fama = vec![0.0_f64; produced];
        let (h, _) = if declined_at_open {
            core.MAMA_OpenAndFill(&data, 0.5, 0.05, &mut mama, None)
        } else {
            core.MAMA_OpenAndFill(&data, 0.5, 0.05, &mut mama, Some(&mut fama))
        }
        .expect("a valid open");
        h
    };

    // The oracle: supplied at open, supplied here.
    let mut h_ss = opened(false);
    let mut ref_mama = canary(bars.len());
    let mut ref_fama = canary(bars.len());
    h_ss.update_and_fill(&bars, &mut ref_mama, Some(&mut ref_fama))
        .expect("the fully supplied fill");
    assert!(written(&ref_mama) && written(&ref_fama), "the oracle fill wrote both outputs");

    for (what, declined_at_open) in [("supplied at open", false), ("declined at open", true)] {
        let mut h = opened(declined_at_open);
        let mut mama = canary(bars.len());
        h.update_and_fill(&bars, &mut mama, None)
            .unwrap_or_else(|e| panic!("{what}, declined here: {e:?}"));
        assert!(written(&mama), "{what}, declined here: the fill wrote outMAMA");
        assert_eq!(mama, ref_mama, "{what}, declined here: outMAMA");
        assert_eq!(h.out_range(), h_ss.out_range(), "{what}, declined here: the range");
        // The state, not the write: FAMA feeds the next bar, so a fill that
        // skipped computing it answers differently here.
        let (m, f) = h.peek(bars[bars.len() - 1] + 1.0).expect("a finite bar");
        let (m_ref, f_ref) = h_ss.peek(bars[bars.len() - 1] + 1.0).expect("a finite bar");
        assert_eq!(m.to_bits(), m_ref.to_bits(), "{what}, declined here: the handle's outMAMA");
        assert_eq!(
            f.to_bits(),
            f_ref.to_bits(),
            "{what}, declined here: a declined outFAMA is still computed"
        );

        // …and supplying it here, whatever the opener was given.
        let mut h2 = opened(declined_at_open);
        let mut mama2 = canary(bars.len());
        let mut fama2 = canary(bars.len());
        h2.update_and_fill(&bars, &mut mama2, Some(&mut fama2))
            .unwrap_or_else(|e| panic!("{what}, supplied here: {e:?}"));
        assert!(
            written(&mama2) && written(&fama2),
            "{what}, supplied here: the fill wrote both outputs"
        );
        assert_eq!(mama2, ref_mama, "{what}, supplied here: outMAMA");
        assert_eq!(fama2, ref_fama, "{what}, supplied here: outFAMA");
    }

    // "May differ again on the NEXT call" — the sentence the whole rule rests on.
    // One handle, three fills, alternating; each has to agree with the oracle
    // driven the same way with everything supplied.
    let mut alt = opened(false);
    let mut oracle = opened(false);
    for (k, decline) in [true, false, true].into_iter().enumerate() {
        let leg: Vec<f64> = bars.iter().map(|b| b + k as f64).collect();
        let mut want_m = canary(leg.len());
        let mut want_f = canary(leg.len());
        oracle
            .update_and_fill(&leg, &mut want_m, Some(&mut want_f))
            .expect("the oracle leg");
        let mut got_m = canary(leg.len());
        let mut got_f = canary(leg.len());
        if decline {
            alt.update_and_fill(&leg, &mut got_m, None).expect("a declining leg");
        } else {
            alt.update_and_fill(&leg, &mut got_m, Some(&mut got_f))
                .expect("a supplying leg");
            assert_eq!(got_f, want_f, "leg {k}: outFAMA");
        }
        assert!(written(&got_m), "leg {k}: the fill wrote outMAMA");
        assert_eq!(got_m, want_m, "leg {k}: outMAMA");
        assert_eq!(alt.out_range(), oracle.out_range(), "leg {k}: the range");
    }
    assert_eq!(
        alt.peek(bars[0]).expect("a finite bar").1.to_bits(),
        oracle.peek(bars[0]).expect("a finite bar").1.to_bits(),
        "alternating the declined set left the handle's FAMA identical"
    );

    // Declining one output disarms neither the other's bound nor its own when
    // it IS supplied.
    let mut h = opened(false);
    let r0 = h.out_range();
    let mut short_mama = vec![0.0_f64; bars.len() - 1];
    assert_eq!(
        h.update_and_fill(&bars, &mut short_mama, None).err(),
        Some(RetCode::BadParam),
        "an undersized outMAMA is still rejected when outFAMA is declined"
    );
    let mut full_mama = vec![0.0_f64; bars.len()];
    let mut short_fama = vec![0.0_f64; bars.len() - 1];
    assert_eq!(
        h.update_and_fill(&bars, &mut full_mama, Some(&mut short_fama)).err(),
        Some(RetCode::BadParam),
        "a supplied outFAMA is still bounded"
    );
    assert_eq!(h.out_range(), r0, "a rejected fill commits nothing");
}
