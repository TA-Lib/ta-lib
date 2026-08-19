//! The streaming tier's non-finite input rejection.
//!
//! Hand-written, not generated: this file lives in
//! `ta_codegen/generator/templates/rust/stream_finite.rs` and is copied verbatim
//! into the crate by `generate` (the Rust backend's `clean_keep` holds it, so
//! `generate` never deletes it). It is declared `#[cfg(test)]` in `mod.rs`, so
//! nothing here ships in a release build. Run it with
//! `cargo test --lib -p ta-lib`.
//!
//! Non-finite rejection is a property of SINGLE VALUES, never of input arrays.
//!
//! An input ARRAY is never scanned, in either tier — including the warm-up
//! history handed to `_Open` / `_OpenAndFill`. Keeping one free of NaN and
//! infinities is the caller's responsibility; passing a non-finite one is
//! undefined behaviour. See `docs/error-handling-spec.md`, rule N-5.
//!
//! A SINGLE VALUE is always checked, because it is one comparison, and for the
//! streaming tier it earns that cost twice over: batch is handed a series,
//! computes and forgets, so a NaN reaches the outputs depending on that bar and
//! no others, while a handle RETAINS state and one non-finite bar poisons every
//! value it will ever produce afterwards — long after the feed recovers.
//! Rejecting the bar and leaving the handle untouched is strictly more useful
//! than accepting it and going permanently NaN.
//!
//! Two properties, per function:
//!
//! 2. `update` / `peek` reject a non-finite bar in ANY input slot.
//! 3. The handle is UNCHANGED by a rejected call. This is the property that
//!    makes the rejection useful rather than merely safe, and it is checked
//!    against a control stream rather than by inspection: two handles opened on
//!    the same history, one of them offered the bad bar first, must agree BIT
//!    FOR BIT on the next good bar. A rejection that half-advanced the state
//!    would satisfy (2) and fail only here.
//!
//! (The numbering keeps 2 and 3: property 1 was the history scan, withdrawn.)
//!
//! Coverage is by stream TIER, not by function count. The check is emitted from
//! one place in `rust_stream.rs`, but into the entry points of five different
//! tiers, so the functions here are chosen to reach every one:
//!
//! | function   | tier                                                        |
//! |------------|-------------------------------------------------------------|
//! | `SMA`      | loop                                                        |
//! | `MINUS_DI` | dual-mode                                                   |
//! | `MA`       | dispatch — including the identity arm, which copies the bar  |
//! |            | straight out and never reaches a sub-stream at all           |
//! | `MAVP`     | period-bank — and the only streaming input reaching an `as i32` |
//! | `BBANDS`   | composed, plus the real optional parameters                  |
//! | `STOCH`    | composed, multi-output, one sub feeding the next             |
//! | `CDLDOJI`  | integer output over four price inputs                       |

use crate::ta_func::types::{Core, RetCode};
use crate::MAType;

const WARM: usize = 60;

fn series(n: usize) -> (Vec<f64>, Vec<f64>, Vec<f64>, Vec<f64>, Vec<f64>) {
    let mut open = Vec::with_capacity(n);
    let mut high = Vec::with_capacity(n);
    let mut low = Vec::with_capacity(n);
    let mut close = Vec::with_capacity(n);
    let mut periods = Vec::with_capacity(n);
    for i in 0..n {
        let base = 100.0 + 8.0 * (i as f64 * 0.11).sin() + 0.03 * i as f64;
        open.push(base - 0.4);
        high.push(base + 1.25);
        low.push(base - 1.25);
        close.push(base + 0.4 * (i as f64 * 0.71).sin());
        periods.push(5.0 + (i % 11) as f64);
    }
    (open, high, low, close, periods)
}

/// The three values the tier refuses.
const BAD: [f64; 3] = [f64::NAN, f64::INFINITY, f64::NEG_INFINITY];

fn probed(n: &mut usize, ok: bool) -> bool {
    *n += 1;
    ok
}

fn is_bad_param<T>(r: &Result<T, RetCode>) -> bool {
    matches!(r, Err(RetCode::BadParam))
}

#[test]
fn update_and_peek_reject_a_non_finite_bar_without_moving_the_handle() {
    let core = Core::new();
    let (open, high, low, close, periods) = series(WARM + 2);
    let (o, h, l, c, p) = (
        &open[..WARM],
        &high[..WARM],
        &low[..WARM],
        &close[..WARM],
        &periods[..WARM],
    );
    let (no, nh, nl, nc, np) = (open[WARM], high[WARM], low[WARM], close[WARM], periods[WARM]);
    let mut rejects = 0usize;
    let mut states = 0usize;

    for &bad in &BAD {
        // --- loop tier ---
        let (mut sa, _) = core.SMA_Open(c, 14).unwrap();
        let (mut sb, _) = core.SMA_Open(c, 14).unwrap();
        assert!(probed(&mut rejects, is_bad_param(&sa.update(bad))), "SMA.update accepted {bad}");
        assert!(probed(&mut rejects, is_bad_param(&sa.peek(bad))), "SMA.peek accepted {bad}");
        assert!(
            probed(&mut states, (sa.update(nc).unwrap().to_bits() == sb.update(nc).unwrap().to_bits())),
            "SMA: a rejected bar moved the handle"
        );

        // --- dual-mode tier, one input slot at a time ---
        let (mut da, _) = core.MINUS_DI_Open(h, l, c, 14).unwrap();
        let (mut db, _) = core.MINUS_DI_Open(h, l, c, 14).unwrap();
        assert!(probed(&mut rejects, is_bad_param(&da.update(bad, nl, nc))), "MINUS_DI.update(high)");
        assert!(probed(&mut rejects, is_bad_param(&da.update(nh, bad, nc))), "MINUS_DI.update(low)");
        assert!(probed(&mut rejects, is_bad_param(&da.update(nh, nl, bad))), "MINUS_DI.update(close)");
        assert!(probed(&mut rejects, is_bad_param(&da.peek(bad, nl, nc))), "MINUS_DI.peek(high)");
        assert!(
            probed(&mut states, da.update(nh, nl, nc).unwrap().to_bits()
                == db.update(nh, nl, nc).unwrap().to_bits()),
            "MINUS_DI: a rejected bar moved the handle"
        );

        // --- dispatch tier, and its identity arm ---
        let (mut ma, _) = core.MA_Open(c, 14, MAType::EMA).unwrap();
        let (mut mb, _) = core.MA_Open(c, 14, MAType::EMA).unwrap();
        assert!(probed(&mut rejects, is_bad_param(&ma.update(bad))), "MA.update accepted {bad}");
        assert!(probed(&mut rejects, is_bad_param(&ma.peek(bad))), "MA.peek accepted {bad}");
        assert!(
            probed(&mut states, (ma.update(nc).unwrap().to_bits() == mb.update(nc).unwrap().to_bits())),
            "MA: a rejected bar moved the handle"
        );

        // Period 1 copies the bar straight to the output and never reaches a
        // sub-stream, so a check delegated to the sub would miss it.
        let (mut mi, _) = core.MA_Open(c, 1, MAType::SMA).unwrap();
        assert!(probed(&mut rejects, is_bad_param(&mi.update(bad))), "MA(identity).update");
        assert!(probed(&mut rejects, is_bad_param(&mi.peek(bad))), "MA(identity).peek");

        // --- period-bank tier: the period reaches `as i32` ---
        let (mut va, _) = core.MAVP_Open(c, p, 2, 30, MAType::SMA).unwrap();
        let (mut vb, _) = core.MAVP_Open(c, p, 2, 30, MAType::SMA).unwrap();
        assert!(probed(&mut rejects, is_bad_param(&va.update(bad, np))), "MAVP.update(real)");
        assert!(probed(&mut rejects, is_bad_param(&va.update(nc, bad))), "MAVP.update(period)");
        assert!(probed(&mut rejects, is_bad_param(&va.peek(nc, bad))), "MAVP.peek(period)");
        assert!(
            probed(&mut states, va.update(nc, np).unwrap().to_bits()
                == vb.update(nc, np).unwrap().to_bits()),
            "MAVP: a rejected bar moved the handle"
        );

        // --- composed tiers ---
        let (mut ba, _) = core.BBANDS_Open(c, 20, 2.0, 2.0, MAType::SMA).unwrap();
        let (mut bb, _) = core.BBANDS_Open(c, 20, 2.0, 2.0, MAType::SMA).unwrap();
        assert!(probed(&mut rejects, is_bad_param(&ba.update(bad))), "BBANDS.update accepted {bad}");
        assert!(probed(&mut rejects, is_bad_param(&ba.peek(bad))), "BBANDS.peek accepted {bad}");
        let (au, am, al) = ba.update(nc).unwrap();
        let (bu, bm, bl) = bb.update(nc).unwrap();
        assert!(
            probed(&mut states, (au.to_bits(), am.to_bits(), al.to_bits())
                == (bu.to_bits(), bm.to_bits(), bl.to_bits())),
            "BBANDS: a rejected bar moved the handle"
        );

        let (mut ka, _) = core
            .STOCH_Open(h, l, c, 5, 3, MAType::SMA, 3, MAType::SMA)
            .unwrap();
        let (mut kb, _) = core
            .STOCH_Open(h, l, c, 5, 3, MAType::SMA, 3, MAType::SMA)
            .unwrap();
        assert!(probed(&mut rejects, is_bad_param(&ka.update(bad, nl, nc))), "STOCH.update");
        assert!(probed(&mut rejects, is_bad_param(&ka.peek(nh, bad, nc))), "STOCH.peek");
        let (kak, kad) = ka.update(nh, nl, nc).unwrap();
        let (kbk, kbd) = kb.update(nh, nl, nc).unwrap();
        assert!(
            probed(&mut states, (kak.to_bits(), kad.to_bits())
                == (kbk.to_bits(), kbd.to_bits())),
            "STOCH: a rejected bar moved the handle"
        );

        // --- integer output, four price inputs ---
        let (mut ja, _) = core.CDLDOJI_Open(o, h, l, c).unwrap();
        let (mut jb, _) = core.CDLDOJI_Open(o, h, l, c).unwrap();
        assert!(probed(&mut rejects, is_bad_param(&ja.update(bad, nh, nl, nc))), "CDLDOJI.update(open)");
        assert!(probed(&mut rejects, is_bad_param(&ja.peek(no, nh, nl, bad))), "CDLDOJI.peek(close)");
        assert!(
            probed(&mut states, ja.update(no, nh, nl, nc).unwrap()
                == jb.update(no, nh, nl, nc).unwrap()),
            "CDLDOJI: a rejected bar moved the handle"
        );
    }
    // Both floors are literal and both counters live inside their assertions.
    assert!(rejects >= 57, "the reject sweep ran {rejects} probes");
    assert!(states >= 21, "the handle-unchanged sweep ran {states} compares");
}

/// A NaN real PARAMETER is rejected too.
///
/// Not a restatement of the range check: `x < min` and `x > max` are BOTH false
/// for NaN, so the batch tier's range test admits it — which is exactly why the
/// streaming tier spells the same two comparisons inverted,
/// `!(x >= min && x <= max)`. An infinity is already outside every declared
/// bound and both spellings reject it, so NaN is the whole difference.
#[test]
fn a_nan_real_parameter_is_rejected() {
    let core = Core::new();
    let (_, _, _, close, _) = series(WARM + 2);
    let c = &close[..WARM];

    assert!(
        is_bad_param(&core.BBANDS_Open(c, 20, f64::NAN, 2.0, MAType::SMA)),
        "BBANDS_Open accepted a NaN optInNbDevUp"
    );
    assert!(
        is_bad_param(&core.BBANDS_Open(c, 20, 2.0, f64::NAN, MAType::SMA)),
        "BBANDS_Open accepted a NaN optInNbDevDn"
    );
    // Control: the same call with both parameters in range must succeed, or the
    // two assertions above would pass for the wrong reason.
    assert!(
        core.BBANDS_Open(c, 20, 2.0, 2.0, MAType::SMA).is_ok(),
        "the control open must succeed"
    );
}
