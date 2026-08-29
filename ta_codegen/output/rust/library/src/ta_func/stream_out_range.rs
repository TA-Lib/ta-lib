//! The range a stream handle carries: `out_range()` against the batch tier.
//!
//! Hand-written, not generated: this file lives in
//! `ta_codegen/generator/templates/rust/stream_out_range.rs` and is copied
//! verbatim into the crate by `generate` (the Rust backend's `clean_keep` holds
//! it, so `generate` never deletes it). It is declared `#[cfg(test)]` in
//! `mod.rs`, so nothing here ships in a release build. Run it with
//! `cargo test --lib -p ta-lib`.
//!
//! The invariant (issue #241):
//!
//! > Feed a stream `N` bars, by any mixture of openers and updates, and its
//! > `OutRange` equals the batch `OutRange` over the same `N` bars.
//!
//! Unconditional — no dependence on which opener was used or on the warm-up
//! length. That is what ties the streaming tier to the batch tier through one
//! number pair, which the value gates do not do: they compare outputs, and an
//! output is the same whether or not the handle knows how many it has produced.
//!
//! Coverage is by stream TIER, not by function count. The seeding is emitted
//! from one place per tier and the tiers do not share it — three read the range
//! back off the transcribed body, dispatch inherits its arm's, the period bank
//! derives it from the anchor the whole bank was opened at — so the functions
//! here are chosen to reach every one:
//!
//! | function   | tier                                                          |
//! |------------|---------------------------------------------------------------|
//! | `SMA`      | loop                                                          |
//! | `MINUS_DI` | dual-mode                                                     |
//! | `MA`       | dispatch — including the identity arm, which opens no sub      |
//! | `MAVP`     | period-bank                                                   |
//! | `BBANDS`   | composed, multi-output                                        |
//!
//! The corpus-wide form of this lives in `stream_verify`'s range leg, which
//! runs it for every function in all four language servers. What is here is the
//! part that leg cannot see: `peek` leaving the count alone, and a clone's
//! updates extending only the clone.

use crate::ta_func::types::{Core, OutRange, RetCode};
use crate::MAType;

const WARM: usize = 60;
const N: usize = 90;

fn series(n: usize) -> (Vec<f64>, Vec<f64>, Vec<f64>, Vec<f64>, Vec<f64>) {
    let mut high = Vec::with_capacity(n);
    let mut low = Vec::with_capacity(n);
    let mut close = Vec::with_capacity(n);
    let mut volume = Vec::with_capacity(n);
    let mut periods = Vec::with_capacity(n);
    for i in 0..n {
        let c = 100.0 + 10.0 * (0.1 * i as f64).sin() + 0.013 * i as f64;
        close.push(c);
        high.push(c + (1.3 * i as f64).sin().abs());
        low.push(c - (1.7 * i as f64).sin().abs());
        volume.push(1000.0 + 7.0 * i as f64);
        periods.push(5.0 + ((i % 11) as f64));
    }
    (high, low, close, volume, periods)
}

/// One tier's check: open over `warm` bars, drive to `N` by `update`, and
/// compare against what the batch call reports for the same `N`.
///
/// `open` and `step` are closures so the same body serves every signature.
/// `lookback` comes from the function's own `_Lookback`, not from a literal:
/// a wrong lookback would otherwise be written into both sides of the compare.
fn range_tracks_batch<H>(
    what: &str,
    lookback: usize,
    batch: OutRange,
    open: impl Fn(usize) -> Result<H, RetCode>,
    range: impl Fn(&H) -> OutRange,
    peek: impl Fn(&H, usize),
    step: impl Fn(&mut H, usize),
) {
    for warm in [lookback + 1, lookback + 7, N / 2, N] {
        assert!(warm <= N, "{what}: warm-up {warm} past the series");
        let mut h = open(warm).unwrap_or_else(|e| panic!("{what}: open({warm}) failed: {e:?}"));
        assert_eq!(
            range(&h),
            OutRange { beg_idx: lookback, count: warm - lookback },
            "{what}: Open({warm}) must report (lookback, {warm} - lookback)"
        );
        for t in warm..N {
            let before = range(&h);
            peek(&h, t);
            assert_eq!(range(&h), before, "{what}: peek committed a bar at {t}");
            step(&mut h, t);
            assert_eq!(
                range(&h),
                OutRange { beg_idx: before.beg_idx, count: before.count + 1 },
                "{what}: update must add exactly one to count at bar {t}"
            );
        }
        assert_eq!(
            range(&h),
            batch,
            "{what}: Open({warm}) + updates to {N} bars != the batch range over {N} bars"
        );
    }
}

#[test]
fn every_tier_reports_the_batch_range() {
    let core = Core::new();
    let (high, low, close, volume, periods) = series(N);
    let _ = volume;
    let mut out0 = vec![0.0_f64; N];
    let mut out1 = vec![0.0_f64; N];
    let mut out2 = vec![0.0_f64; N];

    // Loop tier.
    let batch = core.SMA(0, N - 1, &close, 14, &mut out0).expect("batch SMA");
    range_tracks_batch(
        "SMA (loop)",
        core.SMA_Lookback(14).expect("valid params"),
        batch,
        |w| core.sma_open(&close[..w], 14).map(|(h, _)| h),
        |h| h.out_range(),
        |h, t| {
            h.peek(close[t]).expect("finite bar");
        },
        |h, t| {
            h.update(close[t]).expect("finite bar");
        },
    );

    // Dual-mode tier.
    let batch = core.MINUS_DI(0, N - 1, &high, &low, &close, 14, &mut out0).expect("batch MINUS_DI");
    range_tracks_batch(
        "MINUS_DI (dual-mode)",
        core.MINUS_DI_Lookback(14).expect("valid params"),
        batch,
        |w| core.minus_di_open(&high[..w], &low[..w], &close[..w], 14).map(|(h, _)| h),
        |h| h.out_range(),
        |h, t| {
            h.peek(high[t], low[t], close[t]).expect("finite bar");
        },
        |h, t| {
            h.update(high[t], low[t], close[t]).expect("finite bar");
        },
    );

    // Dispatch tier, an arm that opens a sub-stream.
    let batch = core.MA(0, N - 1, &close, 14, MAType::EMA, &mut out0).expect("batch MA");
    range_tracks_batch(
        "MA (dispatch, EMA arm)",
        core.MA_Lookback(14, MAType::EMA).expect("valid params"),
        batch,
        |w| core.ma_open(&close[..w], 14, MAType::EMA).map(|(h, _)| h),
        |h| h.out_range(),
        |h, t| {
            h.peek(close[t]).expect("finite bar");
        },
        |h, t| {
            h.update(close[t]).expect("finite bar");
        },
    );

    // Dispatch tier, the identity arm: period 1 opens no sub-stream at all and
    // copies the bar straight out, so it seeds the range from its own lookback.
    let batch = core.MA(0, N - 1, &close, 1, MAType::SMA, &mut out0).expect("batch MA(1)");
    range_tracks_batch(
        "MA (dispatch, identity arm)",
        core.MA_Lookback(1, MAType::SMA).expect("valid params"),
        batch,
        |w| core.ma_open(&close[..w], 1, MAType::SMA).map(|(h, _)| h),
        |h| h.out_range(),
        |h, t| {
            h.peek(close[t]).expect("finite bar");
        },
        |h, t| {
            h.update(close[t]).expect("finite bar");
        },
    );

    // Period-bank tier.
    let batch = core
        .MAVP(0, N - 1, &close, &periods, 2, 30, MAType::SMA, &mut out0)
        .expect("batch MAVP");
    range_tracks_batch(
        "MAVP (period bank)",
        core.MAVP_Lookback(2, 30, MAType::SMA).expect("valid params"),
        batch,
        |w| core.mavp_open(&close[..w], &periods[..w], 2, 30, MAType::SMA).map(|(h, _)| h),
        |h| h.out_range(),
        |h, t| {
            h.peek(close[t], periods[t]).expect("finite bar");
        },
        |h, t| {
            h.update(close[t], periods[t]).expect("finite bar");
        },
    );

    // Composed tier, multi-output.
    let batch = core
        .BBANDS(0, N - 1, &close, 20, 2.0, 2.0, MAType::SMA, &mut out0, &mut out1, &mut out2)
        .expect("batch BBANDS");
    range_tracks_batch(
        "BBANDS (composed)",
        core.BBANDS_Lookback(20, 2.0, 2.0, MAType::SMA).expect("valid params"),
        batch,
        |w| core.bbands_open(&close[..w], 20, 2.0, 2.0, MAType::SMA).map(|(h, _)| h),
        |h| h.out_range(),
        |h, t| {
            h.peek(close[t]).expect("finite bar");
        },
        |h, t| {
            h.update(close[t]).expect("finite bar");
        },
    );
}

/// `OpenAndFill` reports the range twice — beside the handle, as it always has
/// (#179 C15), and now on the handle. The two must be the same pair, or a caller
/// reading one gets a different answer from a caller reading the other.
#[test]
fn open_and_fill_agrees_with_the_handle_it_returns() {
    let core = Core::new();
    let (_, _, close, _, _) = series(N);
    let mut out = vec![0.0_f64; N];
    let mut batch = vec![0.0_f64; N];

    let br = core.SMA(0, N - 1, &close, 14, &mut batch).expect("batch SMA");
    let (h, fr) = core.sma_open_and_fill(&close, 14, &mut out).expect("openAndFill");
    assert_eq!(fr, br, "the returned range is the batch range");
    assert_eq!(h.out_range(), fr, "the handle reports the same pair it returned");

    // And it keeps growing from there, exactly as a plain open's would.
    let mut h = h;
    h.update(close[N - 1]).expect("finite bar");
    assert_eq!(h.out_range(), OutRange { beg_idx: br.beg_idx, count: br.count + 1 });
}

/// A clone forks: its updates extend only itself. Cheap to get wrong — the
/// range lives on the handle, and `restore_from` (peek's scratch path) copies
/// the state field by field.
#[test]
fn a_clone_carries_the_range_and_then_diverges() {
    let core = Core::new();
    let (_, _, close, _, _) = series(N);

    let (a, _) = core.sma_open(&close, 14).expect("open");
    let mut b = a.clone();
    assert_eq!(b.out_range(), a.out_range(), "a clone carries the range verbatim");
    b.update(close[N - 1]).expect("finite bar");
    assert_eq!(
        b.out_range(),
        OutRange { beg_idx: a.out_range().beg_idx, count: a.out_range().count + 1 },
        "the clone's own update extends only the clone"
    );
    assert_eq!(
        a.out_range(),
        OutRange { beg_idx: 13, count: N - 13 },
        "the original is untouched by the clone's update"
    );
}

/// A rejected bar must leave the range where it was — the same property the
/// non-finite gate asserts about the rest of the handle, for the one field that
/// gate cannot see (it compares the next value, and a moved count changes none).
#[test]
fn a_rejected_bar_does_not_advance_the_range() {
    let core = Core::new();
    let (_, _, close, _, _) = series(N);

    let (mut s, _) = core.sma_open(&close[..WARM], 14).expect("open");
    let before = s.out_range();
    for bad in [f64::NAN, f64::INFINITY, f64::NEG_INFINITY] {
        assert!(matches!(s.update(bad), Err(RetCode::BadParam)), "a non-finite bar is rejected");
        assert_eq!(s.out_range(), before, "a rejected bar must not advance the count");
    }
    s.update(close[WARM]).expect("finite bar");
    assert_eq!(s.out_range(), OutRange { beg_idx: before.beg_idx, count: before.count + 1 });
}

/// The anchored openers: `<N>_OpenInternal` begins at `max(startIdx, lookback)`,
/// and an anchor the history does not reach is `InsufficientHistory` — not a
/// handle carrying a nonsense range.
///
/// This is the one surface the corpus-wide range leg cannot reach from outside
/// the crate (`_OpenInternal` is `pub(crate)`), and it is where a real defect
/// lived: the period==1 identity arms clamp their anchor up to `startIdx`, and
/// the two that skipped the matching history re-check computed
/// `historyLen - anchor` anyway. In Rust that is `usize`, so it underflowed —
/// `mavp_open(.., optInMinPeriod = 1, ..)` on a short history PANICKED in a
/// debug build, inside a crate that forbids unsafe, where every other opener
/// returned `InsufficientHistory`. Both sites are covered below, plus the loop
/// tier's identity arm, which had the re-check from the start.
#[test]
fn an_anchor_past_the_history_is_insufficient_history() {
    let core = Core::new();
    let (_, _, close, _, periods) = series(N);

    // Dispatch tier, identity arm (period 1 opens no sub-stream at all).
    assert!(
        matches!(core.ma_open_internal(&close[..10], 30, 1, MAType::SMA), Err(RetCode::InsufficientHistory)),
        "ma_open_internal anchored past the history must reject"
    );
    // The same arm reached the other way: DISABLED has lookback 0 at every period.
    assert!(
        matches!(core.ma_open_internal(&close[..10], 30, 5, MAType::DISABLED), Err(RetCode::InsufficientHistory)),
        "ma_open_internal(DISABLED) anchored past the history must reject"
    );
    // Loop tier, identity arm.
    assert!(
        matches!(core.ema_open_internal(&close[..10], 30, 1), Err(RetCode::InsufficientHistory)),
        "ema_open_internal anchored past the history must reject"
    );
    // Period bank: the bank is opened at `subStart`, and slot period 1 is the
    // dispatch identity arm again — this is the shape that panicked.
    assert!(
        matches!(
            core.mavp_open_internal(&close[..10], &periods[..10], 30, 1, 1, MAType::SMA),
            Err(RetCode::InsufficientHistory)
        ),
        "mavp_open_internal anchored past the history must reject"
    );
    // And through the public opener, which is how the panic was reachable.
    assert!(
        matches!(core.mavp_open(&close[..10], &periods[..10], 1, 30, MAType::SMA), Err(RetCode::InsufficientHistory)),
        "mavp_open on a history shorter than the bank's anchor must reject"
    );

    // The positive half, so this is not just a rejection sweep: a legitimate
    // anchor reports max(startIdx, lookback) and the count that follows from it.
    let lb = core.MA_Lookback(1, MAType::SMA).expect("valid params");
    assert_eq!(lb, 0, "the identity arm's lookback is 0, which is what makes startIdx the anchor");
    let (h, _) = core.ma_open_internal(&close, 5, 1, MAType::SMA).expect("a reachable anchor");
    assert_eq!(
        h.out_range(),
        OutRange { beg_idx: 5, count: N - 5 },
        "an anchored open reports max(startIdx, lookback) and the bars after it"
    );
}

/// The `_OpenImpl` prologue must reject an anchor past the history for EVERY
/// function — not only the ones whose transcribed body happens to carry
/// TA-Lib's "make sure there is still something to evaluate" preamble.
///
/// 37 of the 174 `_OpenImpl` bodies do not have it, because in the batch tier
/// that case is caught by the prologue's `endIdx < startIdx` guard, which the
/// streaming prologue never got. Their loop is `nbBar = endIdx - startIdx + 1`
/// followed by `while nbBar != 0`, so a negative count never reaches zero: in C
/// it walks off both the input and the output (an ASan stack-buffer-overflow in
/// `TA_AD_OpenImpl`), and here `usize` underflows and panics.
///
/// The functions below are one per shape in that set, all with a lookback of 0
/// or none at all, which is what leaves `startIdx` as the only thing bounding
/// the loop.
#[test]
fn an_anchor_past_the_history_is_rejected_by_bodies_that_do_not_check_it() {
    let core = Core::new();
    let (high, low, close, volume, _) = series(N);
    const H: usize = 10; // ten bars of history ...
    const A: usize = 30; // ... anchored well past the end of it

    // Loop tier, lookback 0, four inputs — the shape that overflowed in C.
    assert!(
        matches!(
            core.ad_open_internal(&high[..H], &low[..H], &close[..H], &volume[..H], A),
            Err(RetCode::InsufficientHistory)
        ),
        "ad_open_internal anchored past the history must reject, not run an unbounded loop"
    );
    // Loop tier, lookback 0, two inputs.
    assert!(
        matches!(
            core.obv_open_internal(&close[..H], &volume[..H], A),
            Err(RetCode::InsufficientHistory)
        ),
        "obv_open_internal anchored past the history must reject"
    );
    // Stateless map: no accumulator to be short of, so nothing but the anchor
    // can stop it.
    assert!(
        matches!(
            core.medprice_open_internal(&high[..H], &low[..H], A),
            Err(RetCode::InsufficientHistory)
        ),
        "medprice_open_internal anchored past the history must reject"
    );
    // Composed, multi-output.
    assert!(
        matches!(
            core.bbands_open_internal(&close[..H], A, 5, 2.0, 2.0, MAType::SMA),
            Err(RetCode::InsufficientHistory)
        ),
        "bbands_open_internal anchored past the history must reject"
    );
    // Composed, single output, with a period whose lookback is below the anchor.
    assert!(
        matches!(
            core.stddev_open_internal(&close[..H], A, 5, 1.0),
            Err(RetCode::InsufficientHistory)
        ),
        "stddev_open_internal anchored past the history must reject"
    );

    // The boundary is exactly `startIdx == historyLen - 1`: the last bar of the
    // history is still a legal anchor, and one past it is not. Without both
    // halves this is a rejection sweep that an always-reject prologue passes.
    assert!(
        core.ad_open_internal(&high[..H], &low[..H], &close[..H], &volume[..H], H - 1).is_ok(),
        "the last bar of the history is a legal anchor"
    );
    assert!(
        matches!(
            core.ad_open_internal(&high[..H], &low[..H], &close[..H], &volume[..H], H),
            Err(RetCode::InsufficientHistory)
        ),
        "one bar past the history is not"
    );
    let (h, _) = core
        .ad_open_internal(&high[..H], &low[..H], &close[..H], &volume[..H], H - 1)
        .expect("a reachable anchor");
    assert_eq!(
        h.out_range(),
        OutRange { beg_idx: H - 1, count: 1 },
        "anchoring on the last bar reports exactly that one bar"
    );
}

