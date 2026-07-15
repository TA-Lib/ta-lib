//! Overlap-studies arms: moving averages, BBANDS, MIDPOINT/MIDPRICE, SAR
//! family, HT_TRENDLINE — backed by `talib_rs::overlap`.
//!
//! Param names / defaults / input-column mappings mirror the generated
//! ta_codegen server arms (ta_codegen/output/rust/tools/src/bin/ta_codegen_serve.rs).

use crate::{call_ctx, get_input, ma_type_param, respond_error, respond_error_shaped, respond_reals, RefData};
use serde_json::Value;

pub const FUNCTIONS: &[&str] = &[
    "TA_SMA",
    "TA_EMA",
    "TA_WMA",
    "TA_DEMA",
    "TA_TEMA",
    "TA_TRIMA",
    "TA_KAMA",
    "TA_MAMA",
    "TA_T3",
    "TA_MA",
    "TA_MAVP",
    "TA_BBANDS",
    "TA_MIDPOINT",
    "TA_MIDPRICE",
    "TA_SAR",
    "TA_SAREXT",
    "TA_HT_TRENDLINE",
];

/// Read an i64 param with a default, clamping negatives back to the default
/// before casting to `usize` (the protocol sends i64; talib-rs takes usize).
fn usize_param(params: &Value, key: &str, default: i64) -> usize {
    let v = params[key].as_i64().unwrap_or(default);
    (if v < 0 { default } else { v }) as usize
}

fn f64_param(params: &Value, key: &str, default: f64) -> f64 {
    params[key].as_f64().unwrap_or(default)
}

/// Shared shape for the single-input, single-period MAs
/// (SMA/EMA/WMA/DEMA/TEMA/TRIMA/KAMA and MIDPOINT).
fn single_period_arm(
    params: &Value,
    ref_data: &RefData,
    default_period: i64,
    f: fn(&[f64], usize) -> talib_rs::TaResult<Vec<f64>>,
) -> String {
    let ctx = call_ctx(params);
    let mut buf = Vec::new();
    let input = get_input(params, ref_data, &mut buf, "inReal", |r| &r.close);
    let period = usize_param(params, "optInTimePeriod", default_period);
    let t0 = std::time::Instant::now();
    let mut result = None;
    for _ in 0..ctx.iters {
        result = Some(f(input, period));
    }
    let timing = (t0.elapsed().as_nanos() as u64) / ctx.iters;
    match result.unwrap() {
        Ok(out) => respond_reals(&[&out], ctx.start_idx, ctx.end_idx, timing),
        Err(e) => respond_error(&e),
    }
}

pub fn dispatch(method: &str, params: &Value, ref_data: &RefData) -> Option<String> {
    match method {
        "TA_SMA" => Some(single_period_arm(params, ref_data, 30, talib_rs::overlap::sma)),
        "TA_EMA" => Some(single_period_arm(params, ref_data, 30, talib_rs::overlap::ema)),
        "TA_WMA" => Some(single_period_arm(params, ref_data, 30, talib_rs::overlap::wma)),
        "TA_DEMA" => Some(single_period_arm(params, ref_data, 30, talib_rs::overlap::dema)),
        "TA_TEMA" => Some(single_period_arm(params, ref_data, 30, talib_rs::overlap::tema)),
        "TA_TRIMA" => Some(single_period_arm(params, ref_data, 30, talib_rs::overlap::trima)),
        "TA_KAMA" => Some(single_period_arm(params, ref_data, 30, talib_rs::overlap::kama)),
        "TA_MIDPOINT" => Some(single_period_arm(params, ref_data, 14, talib_rs::overlap::midpoint)),
        "TA_MAMA" => {
            let ctx = call_ctx(params);
            let mut buf = Vec::new();
            let input = get_input(params, ref_data, &mut buf, "inReal", |r| &r.close);
            let fast_limit = f64_param(params, "optInFastLimit", 0.5);
            let slow_limit = f64_param(params, "optInSlowLimit", 0.05);
            let t0 = std::time::Instant::now();
            let mut result = None;
            for _ in 0..ctx.iters {
                result = Some(talib_rs::overlap::mama(input, fast_limit, slow_limit));
            }
            let timing = (t0.elapsed().as_nanos() as u64) / ctx.iters;
            Some(match result.unwrap() {
                // outReal = MAMA, outReal1 = FAMA (generated-arm order)
                Ok((mama, fama)) => {
                    respond_reals(&[&mama, &fama], ctx.start_idx, ctx.end_idx, timing)
                }
                Err(e) => respond_error_shaped(&e, 2, 0),
            })
        }
        "TA_T3" => {
            let ctx = call_ctx(params);
            let mut buf = Vec::new();
            let input = get_input(params, ref_data, &mut buf, "inReal", |r| &r.close);
            let period = usize_param(params, "optInTimePeriod", 5);
            let v_factor = f64_param(params, "optInVFactor", 0.7);
            let t0 = std::time::Instant::now();
            let mut result = None;
            for _ in 0..ctx.iters {
                result = Some(talib_rs::overlap::t3(input, period, v_factor));
            }
            let timing = (t0.elapsed().as_nanos() as u64) / ctx.iters;
            Some(match result.unwrap() {
                Ok(out) => respond_reals(&[&out], ctx.start_idx, ctx.end_idx, timing),
                Err(e) => respond_error(&e),
            })
        }
        "TA_MA" => {
            let ctx = call_ctx(params);
            let mut buf = Vec::new();
            let input = get_input(params, ref_data, &mut buf, "inReal", |r| &r.close);
            let period = usize_param(params, "optInTimePeriod", 30);
            let ma_type = match ma_type_param(params, "optInMAType") {
                Ok(m) => m,
                Err(e) => return Some(respond_error(&e)),
            };
            let t0 = std::time::Instant::now();
            let mut result = None;
            for _ in 0..ctx.iters {
                result = Some(talib_rs::overlap::ma(input, period, ma_type));
            }
            let timing = (t0.elapsed().as_nanos() as u64) / ctx.iters;
            Some(match result.unwrap() {
                Ok(out) => respond_reals(&[&out], ctx.start_idx, ctx.end_idx, timing),
                Err(e) => respond_error(&e),
            })
        }
        "TA_MAVP" => {
            let ctx = call_ctx(params);
            // Generated arm: inReal0 (the series) → close, inReal1 (the
            // per-element periods) → high when preloaded.
            let mut buf0 = Vec::new();
            let mut buf1 = Vec::new();
            let input = get_input(params, ref_data, &mut buf0, "inReal0", |r| &r.close);
            let periods = get_input(params, ref_data, &mut buf1, "inReal1", |r| &r.high);
            let min_period = usize_param(params, "optInMinPeriod", 2);
            let max_period = usize_param(params, "optInMaxPeriod", 30);
            let ma_type = match ma_type_param(params, "optInMAType") {
                Ok(m) => m,
                Err(e) => return Some(respond_error(&e)),
            };
            let t0 = std::time::Instant::now();
            let mut result = None;
            for _ in 0..ctx.iters {
                result = Some(talib_rs::overlap::mavp(
                    input, periods, min_period, max_period, ma_type,
                ));
            }
            let timing = (t0.elapsed().as_nanos() as u64) / ctx.iters;
            Some(match result.unwrap() {
                Ok(out) => respond_reals(&[&out], ctx.start_idx, ctx.end_idx, timing),
                Err(e) => respond_error(&e),
            })
        }
        "TA_BBANDS" => {
            let ctx = call_ctx(params);
            let mut buf = Vec::new();
            let input = get_input(params, ref_data, &mut buf, "inReal", |r| &r.close);
            let period = usize_param(params, "optInTimePeriod", 5);
            let nb_dev_up = f64_param(params, "optInNbDevUp", 2.0);
            let nb_dev_dn = f64_param(params, "optInNbDevDn", 2.0);
            let ma_type = match ma_type_param(params, "optInMAType") {
                Ok(m) => m,
                Err(e) => return Some(respond_error(&e)),
            };
            let t0 = std::time::Instant::now();
            let mut result = None;
            for _ in 0..ctx.iters {
                result = Some(talib_rs::overlap::bbands(
                    input, period, nb_dev_up, nb_dev_dn, ma_type,
                ));
            }
            let timing = (t0.elapsed().as_nanos() as u64) / ctx.iters;
            Some(match result.unwrap() {
                // outReal = upper, outReal1 = middle, outReal2 = lower
                Ok((upper, middle, lower)) => {
                    respond_reals(&[&upper, &middle, &lower], ctx.start_idx, ctx.end_idx, timing)
                }
                Err(e) => respond_error_shaped(&e, 3, 0),
            })
        }
        "TA_MIDPRICE" => {
            let ctx = call_ctx(params);
            let mut buf_h = Vec::new();
            let mut buf_l = Vec::new();
            let high = get_input(params, ref_data, &mut buf_h, "inHigh", |r| &r.high);
            let low = get_input(params, ref_data, &mut buf_l, "inLow", |r| &r.low);
            let period = usize_param(params, "optInTimePeriod", 14);
            let t0 = std::time::Instant::now();
            let mut result = None;
            for _ in 0..ctx.iters {
                result = Some(talib_rs::overlap::midprice(high, low, period));
            }
            let timing = (t0.elapsed().as_nanos() as u64) / ctx.iters;
            Some(match result.unwrap() {
                Ok(out) => respond_reals(&[&out], ctx.start_idx, ctx.end_idx, timing),
                Err(e) => respond_error(&e),
            })
        }
        "TA_SAR" => {
            let ctx = call_ctx(params);
            let mut buf_h = Vec::new();
            let mut buf_l = Vec::new();
            let high = get_input(params, ref_data, &mut buf_h, "inHigh", |r| &r.high);
            let low = get_input(params, ref_data, &mut buf_l, "inLow", |r| &r.low);
            let acceleration = f64_param(params, "optInAcceleration", 0.02);
            let maximum = f64_param(params, "optInMaximum", 0.2);
            let t0 = std::time::Instant::now();
            let mut result = None;
            for _ in 0..ctx.iters {
                result = Some(talib_rs::overlap::sar(high, low, acceleration, maximum));
            }
            let timing = (t0.elapsed().as_nanos() as u64) / ctx.iters;
            Some(match result.unwrap() {
                Ok(out) => respond_reals(&[&out], ctx.start_idx, ctx.end_idx, timing),
                Err(e) => respond_error(&e),
            })
        }
        "TA_SAREXT" => {
            let ctx = call_ctx(params);
            let mut buf_h = Vec::new();
            let mut buf_l = Vec::new();
            let high = get_input(params, ref_data, &mut buf_h, "inHigh", |r| &r.high);
            let low = get_input(params, ref_data, &mut buf_l, "inLow", |r| &r.low);
            let start_value = f64_param(params, "optInStartValue", 0.0);
            let offset_on_reverse = f64_param(params, "optInOffsetOnReverse", 0.0);
            let accel_init_long = f64_param(params, "optInAccelerationInitLong", 0.02);
            let accel_long = f64_param(params, "optInAccelerationLong", 0.02);
            let accel_max_long = f64_param(params, "optInAccelerationMaxLong", 0.2);
            let accel_init_short = f64_param(params, "optInAccelerationInitShort", 0.02);
            let accel_short = f64_param(params, "optInAccelerationShort", 0.02);
            let accel_max_short = f64_param(params, "optInAccelerationMaxShort", 0.2);
            let t0 = std::time::Instant::now();
            let mut result = None;
            for _ in 0..ctx.iters {
                result = Some(talib_rs::overlap::sar_ext(
                    high,
                    low,
                    start_value,
                    offset_on_reverse,
                    accel_init_long,
                    accel_long,
                    accel_max_long,
                    accel_init_short,
                    accel_short,
                    accel_max_short,
                ));
            }
            let timing = (t0.elapsed().as_nanos() as u64) / ctx.iters;
            Some(match result.unwrap() {
                Ok(out) => respond_reals(&[&out], ctx.start_idx, ctx.end_idx, timing),
                Err(e) => respond_error(&e),
            })
        }
        "TA_HT_TRENDLINE" => {
            let ctx = call_ctx(params);
            let mut buf = Vec::new();
            let input = get_input(params, ref_data, &mut buf, "inReal", |r| &r.close);
            let t0 = std::time::Instant::now();
            let mut result = None;
            for _ in 0..ctx.iters {
                result = Some(talib_rs::overlap::ht_trendline(input));
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
