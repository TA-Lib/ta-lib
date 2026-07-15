//! Arms for the volume (AD, ADOSC, OBV), volatility (ATR, NATR, TRANGE) and
//! price-transform (AVGPRICE, MEDPRICE, TYPPRICE, WCLPRICE) groups.
//!
//! Input-name → preloaded-column mapping mirrors the generated ta_codegen
//! server arms: inHigh/inLow/inClose/inOpen/inVolume map to their columns,
//! bare inReal (OBV) maps to close.
//!
//! Semantic caveats (talib-rs computes from index 0 regardless of startIdx):
//! - OBV: TA-Lib restarts accumulation at startIdx (prevOBV = volume[startIdx]);
//!   talib-rs accumulates from bar 0, so values diverge whenever startIdx > 0.
//! - ADOSC / ATR / NATR: TA-Lib seeds the EMA / Wilder smoothing at
//!   startIdx - lookback; talib-rs always seeds at bar 0, so values diverge
//!   when startIdx > lookback (and for ATR/NATR whenever a nonzero unstable
//!   period is set — this server ignores set_unstable_period).

use crate::{call_ctx, get_input, respond_error, respond_reals, RefData};
use serde_json::Value;

pub const FUNCTIONS: &[&str] = &[
    "TA_AD",
    "TA_ADOSC",
    "TA_OBV",
    "TA_ATR",
    "TA_NATR",
    "TA_TRANGE",
    "TA_AVGPRICE",
    "TA_MEDPRICE",
    "TA_TYPPRICE",
    "TA_WCLPRICE",
];

/// Read an optional integer period param; negatives fall back to the default
/// before the cast to usize (the protocol sends i64, talib-rs takes usize).
fn period_param(params: &Value, key: &str, default: i64) -> usize {
    let v = params[key].as_i64().unwrap_or(default);
    (if v < 0 { default } else { v }) as usize
}

pub fn dispatch(method: &str, params: &Value, ref_data: &RefData) -> Option<String> {
    match method {
        "TA_AD" => {
            let ctx = call_ctx(params);
            let mut buf_h = Vec::new();
            let mut buf_l = Vec::new();
            let mut buf_c = Vec::new();
            let mut buf_v = Vec::new();
            let high = get_input(params, ref_data, &mut buf_h, "inHigh", |r| &r.high);
            let low = get_input(params, ref_data, &mut buf_l, "inLow", |r| &r.low);
            let close = get_input(params, ref_data, &mut buf_c, "inClose", |r| &r.close);
            let volume = get_input(params, ref_data, &mut buf_v, "inVolume", |r| &r.volume);
            let t0 = std::time::Instant::now();
            let mut result = None;
            for _ in 0..ctx.iters {
                result = Some(talib_rs::volume::ad(high, low, close, volume));
            }
            let timing = (t0.elapsed().as_nanos() as u64) / ctx.iters;
            Some(match result.unwrap() {
                Ok(out) => respond_reals(&[&out], ctx.start_idx, ctx.end_idx, timing),
                Err(e) => respond_error(&e),
            })
        }
        "TA_ADOSC" => {
            let ctx = call_ctx(params);
            let mut buf_h = Vec::new();
            let mut buf_l = Vec::new();
            let mut buf_c = Vec::new();
            let mut buf_v = Vec::new();
            let high = get_input(params, ref_data, &mut buf_h, "inHigh", |r| &r.high);
            let low = get_input(params, ref_data, &mut buf_l, "inLow", |r| &r.low);
            let close = get_input(params, ref_data, &mut buf_c, "inClose", |r| &r.close);
            let volume = get_input(params, ref_data, &mut buf_v, "inVolume", |r| &r.volume);
            let fast_period = period_param(params, "optInFastPeriod", 3);
            let slow_period = period_param(params, "optInSlowPeriod", 10);
            let t0 = std::time::Instant::now();
            let mut result = None;
            for _ in 0..ctx.iters {
                result = Some(talib_rs::volume::adosc(
                    high,
                    low,
                    close,
                    volume,
                    fast_period,
                    slow_period,
                ));
            }
            let timing = (t0.elapsed().as_nanos() as u64) / ctx.iters;
            Some(match result.unwrap() {
                Ok(out) => respond_reals(&[&out], ctx.start_idx, ctx.end_idx, timing),
                Err(e) => respond_error(&e),
            })
        }
        "TA_OBV" => {
            let ctx = call_ctx(params);
            let mut buf_r = Vec::new();
            let mut buf_v = Vec::new();
            let real = get_input(params, ref_data, &mut buf_r, "inReal", |r| &r.close);
            let volume = get_input(params, ref_data, &mut buf_v, "inVolume", |r| &r.volume);
            let t0 = std::time::Instant::now();
            let mut result = None;
            for _ in 0..ctx.iters {
                result = Some(talib_rs::volume::obv(real, volume));
            }
            let timing = (t0.elapsed().as_nanos() as u64) / ctx.iters;
            Some(match result.unwrap() {
                Ok(out) => respond_reals(&[&out], ctx.start_idx, ctx.end_idx, timing),
                Err(e) => respond_error(&e),
            })
        }
        "TA_ATR" => {
            let ctx = call_ctx(params);
            let mut buf_h = Vec::new();
            let mut buf_l = Vec::new();
            let mut buf_c = Vec::new();
            let high = get_input(params, ref_data, &mut buf_h, "inHigh", |r| &r.high);
            let low = get_input(params, ref_data, &mut buf_l, "inLow", |r| &r.low);
            let close = get_input(params, ref_data, &mut buf_c, "inClose", |r| &r.close);
            let time_period = period_param(params, "optInTimePeriod", 14);
            let t0 = std::time::Instant::now();
            let mut result = None;
            for _ in 0..ctx.iters {
                result = Some(talib_rs::volatility::atr(high, low, close, time_period));
            }
            let timing = (t0.elapsed().as_nanos() as u64) / ctx.iters;
            Some(match result.unwrap() {
                Ok(out) => respond_reals(&[&out], ctx.start_idx, ctx.end_idx, timing),
                Err(e) => respond_error(&e),
            })
        }
        "TA_NATR" => {
            let ctx = call_ctx(params);
            let mut buf_h = Vec::new();
            let mut buf_l = Vec::new();
            let mut buf_c = Vec::new();
            let high = get_input(params, ref_data, &mut buf_h, "inHigh", |r| &r.high);
            let low = get_input(params, ref_data, &mut buf_l, "inLow", |r| &r.low);
            let close = get_input(params, ref_data, &mut buf_c, "inClose", |r| &r.close);
            let time_period = period_param(params, "optInTimePeriod", 14);
            let t0 = std::time::Instant::now();
            let mut result = None;
            for _ in 0..ctx.iters {
                result = Some(talib_rs::volatility::natr(high, low, close, time_period));
            }
            let timing = (t0.elapsed().as_nanos() as u64) / ctx.iters;
            Some(match result.unwrap() {
                Ok(out) => respond_reals(&[&out], ctx.start_idx, ctx.end_idx, timing),
                Err(e) => respond_error(&e),
            })
        }
        "TA_TRANGE" => {
            let ctx = call_ctx(params);
            let mut buf_h = Vec::new();
            let mut buf_l = Vec::new();
            let mut buf_c = Vec::new();
            let high = get_input(params, ref_data, &mut buf_h, "inHigh", |r| &r.high);
            let low = get_input(params, ref_data, &mut buf_l, "inLow", |r| &r.low);
            let close = get_input(params, ref_data, &mut buf_c, "inClose", |r| &r.close);
            let t0 = std::time::Instant::now();
            let mut result = None;
            for _ in 0..ctx.iters {
                result = Some(talib_rs::volatility::trange(high, low, close));
            }
            let timing = (t0.elapsed().as_nanos() as u64) / ctx.iters;
            Some(match result.unwrap() {
                Ok(out) => respond_reals(&[&out], ctx.start_idx, ctx.end_idx, timing),
                Err(e) => respond_error(&e),
            })
        }
        "TA_AVGPRICE" => {
            let ctx = call_ctx(params);
            let mut buf_o = Vec::new();
            let mut buf_h = Vec::new();
            let mut buf_l = Vec::new();
            let mut buf_c = Vec::new();
            let open = get_input(params, ref_data, &mut buf_o, "inOpen", |r| &r.open);
            let high = get_input(params, ref_data, &mut buf_h, "inHigh", |r| &r.high);
            let low = get_input(params, ref_data, &mut buf_l, "inLow", |r| &r.low);
            let close = get_input(params, ref_data, &mut buf_c, "inClose", |r| &r.close);
            let t0 = std::time::Instant::now();
            let mut result = None;
            for _ in 0..ctx.iters {
                result = Some(talib_rs::price_transform::avgprice(open, high, low, close));
            }
            let timing = (t0.elapsed().as_nanos() as u64) / ctx.iters;
            Some(match result.unwrap() {
                Ok(out) => respond_reals(&[&out], ctx.start_idx, ctx.end_idx, timing),
                Err(e) => respond_error(&e),
            })
        }
        "TA_MEDPRICE" => {
            let ctx = call_ctx(params);
            let mut buf_h = Vec::new();
            let mut buf_l = Vec::new();
            let high = get_input(params, ref_data, &mut buf_h, "inHigh", |r| &r.high);
            let low = get_input(params, ref_data, &mut buf_l, "inLow", |r| &r.low);
            let t0 = std::time::Instant::now();
            let mut result = None;
            for _ in 0..ctx.iters {
                result = Some(talib_rs::price_transform::medprice(high, low));
            }
            let timing = (t0.elapsed().as_nanos() as u64) / ctx.iters;
            Some(match result.unwrap() {
                Ok(out) => respond_reals(&[&out], ctx.start_idx, ctx.end_idx, timing),
                Err(e) => respond_error(&e),
            })
        }
        "TA_TYPPRICE" => {
            let ctx = call_ctx(params);
            let mut buf_h = Vec::new();
            let mut buf_l = Vec::new();
            let mut buf_c = Vec::new();
            let high = get_input(params, ref_data, &mut buf_h, "inHigh", |r| &r.high);
            let low = get_input(params, ref_data, &mut buf_l, "inLow", |r| &r.low);
            let close = get_input(params, ref_data, &mut buf_c, "inClose", |r| &r.close);
            let t0 = std::time::Instant::now();
            let mut result = None;
            for _ in 0..ctx.iters {
                result = Some(talib_rs::price_transform::typprice(high, low, close));
            }
            let timing = (t0.elapsed().as_nanos() as u64) / ctx.iters;
            Some(match result.unwrap() {
                Ok(out) => respond_reals(&[&out], ctx.start_idx, ctx.end_idx, timing),
                Err(e) => respond_error(&e),
            })
        }
        "TA_WCLPRICE" => {
            let ctx = call_ctx(params);
            let mut buf_h = Vec::new();
            let mut buf_l = Vec::new();
            let mut buf_c = Vec::new();
            let high = get_input(params, ref_data, &mut buf_h, "inHigh", |r| &r.high);
            let low = get_input(params, ref_data, &mut buf_l, "inLow", |r| &r.low);
            let close = get_input(params, ref_data, &mut buf_c, "inClose", |r| &r.close);
            let t0 = std::time::Instant::now();
            let mut result = None;
            for _ in 0..ctx.iters {
                result = Some(talib_rs::price_transform::wclprice(high, low, close));
            }
            let timing = (t0.elapsed().as_nanos() as u64) / ctx.iters;
            Some(match result.unwrap() {
                Ok(out) => respond_reals(&[&out], ctx.start_idx, ctx.end_idx, timing),
                Err(e) => respond_error(&e),
            })
        }
        _ => None,
    }
}
