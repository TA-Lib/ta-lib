//! Candlestick pattern (CDL*) arms for the talib-rs JSON-RPC server.
//!
//! All 61 talib-rs pattern functions share one signature:
//! `fn(&[f64], &[f64], &[f64], &[f64]) -> TaResult<Vec<i32>>` over
//! (open, high, low, close), returning a full-length Vec<i32> with values
//! 0 / +-100 (and +-200 for HIKKAKE/HIKKAKEMOD confirmations), matching
//! TA-Lib's output convention.
//!
//! Integer outputs carry no NaN lookback marker, so each entry in the table
//! below hardcodes talib-rs's effective lookback (the index where its loop
//! starts writing real values), derived from the `lookback` expressions in
//! talib-rs 0.1.2 `src/pattern/mod.rs` with its hardcoded candle settings
//! (avg periods: BodyLong/VeryLong/Short/Doji=10, ShadowLong/VeryLong=0,
//! ShadowShort/VeryShort=10, Near/Far=5, Equal=5 — identical to TA-Lib's
//! ta_common.c defaults).
//!
//! Known semantic caveats (the talib-rs behavior is wrapped as-is, not
//! adjusted, so benchmark results describe what the crate actually computes):
//! - talib-rs hardcodes the penetration factor for the 7 penetration
//!   patterns (0.3 for ABANDONEDBABY, EVENINGDOJISTAR, EVENINGSTAR,
//!   MORNINGDOJISTAR, MORNINGSTAR; 0.5 for DARKCLOUDCOVER, MATHOLD). Those
//!   values equal TA-Lib's defaults, so the protocol's `optInPenetration`
//!   param is accepted but IGNORED — results only match TA-Lib when the
//!   caller sends the default penetration.
//! - CDLENGULFING: talib-rs computes from index 1; TA-Lib's lookback is 2.
//! - CDL3OUTSIDE: talib-rs computes from index 2; TA-Lib's lookback is 3.

use crate::{call_ctx, get_input, respond_error_shaped, respond_ints, RefData};
use serde_json::Value;
use talib_rs::pattern as pat;

/// Common signature of every talib-rs candlestick pattern function.
type CdlFn = fn(&[f64], &[f64], &[f64], &[f64]) -> Result<Vec<i32>, talib_rs::TaError>;

/// (method name, talib-rs function, talib-rs effective lookback).
const TABLE: &[(&str, CdlFn, usize)] = &[
    ("TA_CDL2CROWS", pat::cdl_2crows, 12),            // BodyLong(10) + 2
    ("TA_CDL3BLACKCROWS", pat::cdl_3blackcrows, 13),  // ShadowVeryShort(10) + 3
    ("TA_CDL3INSIDE", pat::cdl_3inside, 12),          // max(BodyShort,BodyLong)(10) + 2
    ("TA_CDL3LINESTRIKE", pat::cdl_3linestrike, 8),   // Near(5) + 3
    ("TA_CDL3OUTSIDE", pat::cdl_3outside, 2),         // loop from 2 (TA-Lib: 3)
    ("TA_CDL3STARSINSOUTH", pat::cdl_3starsinsouth, 12), // max(10,0,10,10) + 2
    ("TA_CDL3WHITESOLDIERS", pat::cdl_3whitesoldiers, 12), // max(10,10,5,5) + 2
    ("TA_CDLABANDONEDBABY", pat::cdl_abandonedbaby, 12), // max(10,10,10) + 2
    ("TA_CDLADVANCEBLOCK", pat::cdl_advanceblock, 12), // max(0,10,5,5,10) + 2
    ("TA_CDLBELTHOLD", pat::cdl_belthold, 10),        // max(BodyLong,ShadowVeryShort)
    ("TA_CDLBREAKAWAY", pat::cdl_breakaway, 14),      // BodyLong(10) + 4
    ("TA_CDLCLOSINGMARUBOZU", pat::cdl_closingmarubozu, 10), // max(10,10)
    ("TA_CDLCONCEALBABYSWALL", pat::cdl_concealbabyswall, 13), // ShadowVeryShort(10) + 3
    ("TA_CDLCOUNTERATTACK", pat::cdl_counterattack, 11), // max(Equal,BodyLong)(10) + 1
    ("TA_CDLDARKCLOUDCOVER", pat::cdl_darkcloudcover, 11), // BodyLong(10) + 1
    ("TA_CDLDOJI", pat::cdl_doji, 10),                // BodyDoji(10)
    ("TA_CDLDOJISTAR", pat::cdl_dojistar, 11),        // max(BodyDoji,BodyLong)(10) + 1
    ("TA_CDLDRAGONFLYDOJI", pat::cdl_dragonflydoji, 10), // max(10,10)
    ("TA_CDLENGULFING", pat::cdl_engulfing, 1),       // loop from 1 (TA-Lib: 2)
    ("TA_CDLEVENINGDOJISTAR", pat::cdl_eveningdojistar, 12), // max(10,10,10) + 2
    ("TA_CDLEVENINGSTAR", pat::cdl_eveningstar, 12),  // max(BodyShort,BodyLong)(10) + 2
    ("TA_CDLGAPSIDESIDEWHITE", pat::cdl_gapsidesidewhite, 7), // max(Near,Equal)(5) + 2
    ("TA_CDLGRAVESTONEDOJI", pat::cdl_gravestonedoji, 10), // max(10,10)
    ("TA_CDLHAMMER", pat::cdl_hammer, 11),            // max(10,0,10,5) + 1
    ("TA_CDLHANGINGMAN", pat::cdl_hangingman, 11),    // max(10,0,10,5) + 1
    ("TA_CDLHARAMI", pat::cdl_harami, 11),            // max(10,10) + 1
    ("TA_CDLHARAMICROSS", pat::cdl_haramicross, 11),  // max(10,10) + 1
    ("TA_CDLHIGHWAVE", pat::cdl_highwave, 10),        // max(BodyShort,ShadowVeryLong)=max(10,0)
    ("TA_CDLHIKKAKE", pat::cdl_hikkake, 5),           // hardcoded 5
    ("TA_CDLHIKKAKEMOD", pat::cdl_hikkakemod, 10),    // max(1,Near(5)) + 5
    ("TA_CDLHOMINGPIGEON", pat::cdl_homingpigeon, 11), // max(10,10) + 1
    ("TA_CDLIDENTICAL3CROWS", pat::cdl_identical3crows, 12), // max(10,5) + 2
    ("TA_CDLINNECK", pat::cdl_inneck, 11),            // max(Equal,BodyLong)(10) + 1
    ("TA_CDLINVERTEDHAMMER", pat::cdl_invertedhammer, 11), // max(10,0,10) + 1
    ("TA_CDLKICKING", pat::cdl_kicking, 11),          // max(10,10) + 1
    ("TA_CDLKICKINGBYLENGTH", pat::cdl_kickingbylength, 11), // max(10,10) + 1
    ("TA_CDLLADDERBOTTOM", pat::cdl_ladderbottom, 14), // ShadowVeryShort(10) + 4
    ("TA_CDLLONGLEGGEDDOJI", pat::cdl_longleggeddoji, 10), // max(10,0)
    ("TA_CDLLONGLINE", pat::cdl_longline, 10),        // max(10,10)
    ("TA_CDLMARUBOZU", pat::cdl_marubozu, 10),        // max(10,10)
    ("TA_CDLMATCHINGLOW", pat::cdl_matchinglow, 6),   // Equal(5) + 1
    ("TA_CDLMATHOLD", pat::cdl_mathold, 14),          // max(10,10) + 4
    ("TA_CDLMORNINGDOJISTAR", pat::cdl_morningdojistar, 12), // max(10,10,10) + 2
    ("TA_CDLMORNINGSTAR", pat::cdl_morningstar, 12),  // max(10,10) + 2
    ("TA_CDLONNECK", pat::cdl_onneck, 11),            // max(Equal,BodyLong)(10) + 1
    ("TA_CDLPIERCING", pat::cdl_piercing, 11),        // BodyLong(10) + 1
    ("TA_CDLRICKSHAWMAN", pat::cdl_rickshawman, 10),  // max(10,0,5)
    ("TA_CDLRISEFALL3METHODS", pat::cdl_risefall3methods, 14), // max(10,10) + 4
    ("TA_CDLSEPARATINGLINES", pat::cdl_separatinglines, 11), // max(10,10,5) + 1
    ("TA_CDLSHOOTINGSTAR", pat::cdl_shootingstar, 11), // max(10,0,10) + 1
    ("TA_CDLSHORTLINE", pat::cdl_shortline, 10),      // max(10,10)
    ("TA_CDLSPINNINGTOP", pat::cdl_spinningtop, 10),  // BodyShort(10)
    ("TA_CDLSTALLEDPATTERN", pat::cdl_stalledpattern, 12), // max(10,10,10,5) + 2
    ("TA_CDLSTICKSANDWICH", pat::cdl_sticksandwich, 7), // Equal(5) + 2
    ("TA_CDLTAKURI", pat::cdl_takuri, 10),            // max(10,10,0)
    ("TA_CDLTASUKIGAP", pat::cdl_tasukigap, 7),       // Near(5) + 2
    ("TA_CDLTHRUSTING", pat::cdl_thrusting, 11),      // max(Equal,BodyLong)(10) + 1
    ("TA_CDLTRISTAR", pat::cdl_tristar, 12),          // BodyDoji(10) + 2
    ("TA_CDLUNIQUE3RIVER", pat::cdl_unique3river, 12), // max(10,10) + 2
    ("TA_CDLUPSIDEGAP2CROWS", pat::cdl_upsidegap2crows, 12), // max(10,10) + 2
    ("TA_CDLXSIDEGAP3METHODS", pat::cdl_xsidegap3methods, 2), // loop from 2
];

pub const FUNCTIONS: &[&str] = &[
    "TA_CDL2CROWS",
    "TA_CDL3BLACKCROWS",
    "TA_CDL3INSIDE",
    "TA_CDL3LINESTRIKE",
    "TA_CDL3OUTSIDE",
    "TA_CDL3STARSINSOUTH",
    "TA_CDL3WHITESOLDIERS",
    "TA_CDLABANDONEDBABY",
    "TA_CDLADVANCEBLOCK",
    "TA_CDLBELTHOLD",
    "TA_CDLBREAKAWAY",
    "TA_CDLCLOSINGMARUBOZU",
    "TA_CDLCONCEALBABYSWALL",
    "TA_CDLCOUNTERATTACK",
    "TA_CDLDARKCLOUDCOVER",
    "TA_CDLDOJI",
    "TA_CDLDOJISTAR",
    "TA_CDLDRAGONFLYDOJI",
    "TA_CDLENGULFING",
    "TA_CDLEVENINGDOJISTAR",
    "TA_CDLEVENINGSTAR",
    "TA_CDLGAPSIDESIDEWHITE",
    "TA_CDLGRAVESTONEDOJI",
    "TA_CDLHAMMER",
    "TA_CDLHANGINGMAN",
    "TA_CDLHARAMI",
    "TA_CDLHARAMICROSS",
    "TA_CDLHIGHWAVE",
    "TA_CDLHIKKAKE",
    "TA_CDLHIKKAKEMOD",
    "TA_CDLHOMINGPIGEON",
    "TA_CDLIDENTICAL3CROWS",
    "TA_CDLINNECK",
    "TA_CDLINVERTEDHAMMER",
    "TA_CDLKICKING",
    "TA_CDLKICKINGBYLENGTH",
    "TA_CDLLADDERBOTTOM",
    "TA_CDLLONGLEGGEDDOJI",
    "TA_CDLLONGLINE",
    "TA_CDLMARUBOZU",
    "TA_CDLMATCHINGLOW",
    "TA_CDLMATHOLD",
    "TA_CDLMORNINGDOJISTAR",
    "TA_CDLMORNINGSTAR",
    "TA_CDLONNECK",
    "TA_CDLPIERCING",
    "TA_CDLRICKSHAWMAN",
    "TA_CDLRISEFALL3METHODS",
    "TA_CDLSEPARATINGLINES",
    "TA_CDLSHOOTINGSTAR",
    "TA_CDLSHORTLINE",
    "TA_CDLSPINNINGTOP",
    "TA_CDLSTALLEDPATTERN",
    "TA_CDLSTICKSANDWICH",
    "TA_CDLTAKURI",
    "TA_CDLTASUKIGAP",
    "TA_CDLTHRUSTING",
    "TA_CDLTRISTAR",
    "TA_CDLUNIQUE3RIVER",
    "TA_CDLUPSIDEGAP2CROWS",
    "TA_CDLXSIDEGAP3METHODS",
];

pub fn dispatch(method: &str, params: &Value, ref_data: &RefData) -> Option<String> {
    let &(_, func, first_valid) = TABLE.iter().find(|(name, _, _)| *name == method)?;

    let ctx = call_ctx(params);
    // Same preloaded-column mapping as the ta_codegen server arms:
    // inOpen->open, inHigh->high, inLow->low, inClose->close.
    let mut buf_open = Vec::new();
    let mut buf_high = Vec::new();
    let mut buf_low = Vec::new();
    let mut buf_close = Vec::new();
    let in_open = get_input(params, ref_data, &mut buf_open, "inOpen", |r| &r.open);
    let in_high = get_input(params, ref_data, &mut buf_high, "inHigh", |r| &r.high);
    let in_low = get_input(params, ref_data, &mut buf_low, "inLow", |r| &r.low);
    let in_close = get_input(params, ref_data, &mut buf_close, "inClose", |r| &r.close);

    // Note: the penetration patterns' optInPenetration protocol param is
    // intentionally not read — talib-rs hardcodes the TA-Lib default value
    // (see module docs).
    let t0 = std::time::Instant::now();
    let mut result = None;
    for _ in 0..ctx.iters {
        result = Some(func(in_open, in_high, in_low, in_close));
    }
    let timing = (t0.elapsed().as_nanos() as u64) / ctx.iters;

    Some(match result.unwrap() {
        Ok(out) => respond_ints(&[&out], first_valid, ctx.start_idx, ctx.end_idx, timing),
        Err(e) => respond_error_shaped(&e, 0, 1),
    })
}
