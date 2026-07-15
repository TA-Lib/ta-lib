//! Math operator + math transform arms (talib-rs `math_operator` and
//! `math_transform` modules).
//!
//! Semantics notes (vs the ta_codegen servers / TA-Lib C):
//! - Math transforms (ACOS..TANH) have lookback 0; talib-rs returns a plain
//!   `Vec<f64>` with no NaN lookback prefix. Any NaNs in the output are data
//!   (out-of-domain inputs, e.g. ACOS of a price > 1), NOT a lookback marker,
//!   so these arms pass `first_valid = 0` explicitly instead of relying on
//!   `respond_reals`' NaN-prefix detection.
//! - ADD/SUB/MULT/DIV likewise have lookback 0. talib-rs errors with
//!   LengthMismatch when the two inputs differ in length (mapped to
//!   retCode 2); TA-Lib itself never length-checks.
//! - MAX/MIN/SUM/MAXINDEX/MININDEX/MINMAX/MINMAXINDEX have lookback
//!   `period - 1`, passed explicitly (the index variants 0-fill their
//!   lookback region, so there is no NaN prefix to detect).
//! - MAXINDEX/MININDEX/MINMAXINDEX: talib-rs outputs ABSOLUTE input indices
//!   (same convention as TA-Lib C), but as `Vec<f64>`; the arms cast the
//!   exact-integer values to i32 for the outInteger response keys.
//! - talib-rs accepts `timeperiod >= 1` where TA-Lib requires `>= 2`; a
//!   period of 1 succeeds here but would be TA_BAD_PARAM in TA-Lib.

use crate::{call_ctx, get_input, respond_error, respond_error_shaped, respond_ints, respond_reals_from, RefData};
use serde_json::Value;
use talib_rs::TaResult;

pub const FUNCTIONS: &[&str] = &[
    "TA_ACOS",
    "TA_ADD",
    "TA_ASIN",
    "TA_ATAN",
    "TA_CEIL",
    "TA_COS",
    "TA_COSH",
    "TA_DIV",
    "TA_EXP",
    "TA_FLOOR",
    "TA_LN",
    "TA_LOG10",
    "TA_MAX",
    "TA_MAXINDEX",
    "TA_MIN",
    "TA_MININDEX",
    "TA_MINMAX",
    "TA_MINMAXINDEX",
    "TA_MULT",
    "TA_SIN",
    "TA_SINH",
    "TA_SQRT",
    "TA_SUB",
    "TA_SUM",
    "TA_TAN",
    "TA_TANH",
];

/// optInTimePeriod with the generated-server default of 30; negative protocol
/// values are clamped to the default before the usize cast.
fn opt_time_period(params: &Value) -> usize {
    let v = params["optInTimePeriod"].as_i64().unwrap_or(30);
    if v < 0 {
        30
    } else {
        v as usize
    }
}

/// One-input, no-param element-wise transform (ACOS, SIN, SQRT, ...).
/// Lookback is 0; NaNs in the output are data, not a lookback prefix.
fn transform_arm(params: &Value, ref_data: &RefData, f: fn(&[f64]) -> Vec<f64>) -> String {
    let ctx = call_ctx(params);
    let mut buf = Vec::new();
    let input = get_input(params, ref_data, &mut buf, "inReal", |r| &r.close);
    let t0 = std::time::Instant::now();
    let mut result = Vec::new();
    for _ in 0..ctx.iters {
        result = f(input);
    }
    let timing = (t0.elapsed().as_nanos() as u64) / ctx.iters;
    respond_reals_from(&[&result], 0, ctx.start_idx, ctx.end_idx, timing)
}

/// Two-input element-wise operator (ADD/SUB/MULT/DIV). Preloaded mapping
/// mirrors the generated server: inReal0 -> close, inReal1 -> high.
fn pair_arm(
    params: &Value,
    ref_data: &RefData,
    f: fn(&[f64], &[f64]) -> TaResult<Vec<f64>>,
) -> String {
    let ctx = call_ctx(params);
    let mut buf0 = Vec::new();
    let mut buf1 = Vec::new();
    let in0 = get_input(params, ref_data, &mut buf0, "inReal0", |r| &r.close);
    let in1 = get_input(params, ref_data, &mut buf1, "inReal1", |r| &r.high);
    let t0 = std::time::Instant::now();
    let mut result = None;
    for _ in 0..ctx.iters {
        result = Some(f(in0, in1));
    }
    let timing = (t0.elapsed().as_nanos() as u64) / ctx.iters;
    match result.unwrap() {
        Ok(out) => respond_reals_from(&[&out], 0, ctx.start_idx, ctx.end_idx, timing),
        Err(e) => respond_error(&e),
    }
}

/// Windowed real-output operator (MAX/MIN/SUM). Lookback = period - 1.
fn period_real_arm(
    params: &Value,
    ref_data: &RefData,
    f: fn(&[f64], usize) -> TaResult<Vec<f64>>,
) -> String {
    let ctx = call_ctx(params);
    let mut buf = Vec::new();
    let input = get_input(params, ref_data, &mut buf, "inReal", |r| &r.close);
    let period = opt_time_period(params);
    let t0 = std::time::Instant::now();
    let mut result = None;
    for _ in 0..ctx.iters {
        result = Some(f(input, period));
    }
    let timing = (t0.elapsed().as_nanos() as u64) / ctx.iters;
    match result.unwrap() {
        Ok(out) => respond_reals_from(
            &[&out],
            period.saturating_sub(1),
            ctx.start_idx,
            ctx.end_idx,
            timing,
        ),
        Err(e) => respond_error(&e),
    }
}

/// Windowed index-output operator (MAXINDEX/MININDEX). talib-rs returns
/// absolute input indices as f64 (0-filled lookback); cast to i32 for the
/// outInteger key. Lookback = period - 1.
fn period_index_arm(
    params: &Value,
    ref_data: &RefData,
    f: fn(&[f64], usize) -> TaResult<Vec<f64>>,
) -> String {
    let ctx = call_ctx(params);
    let mut buf = Vec::new();
    let input = get_input(params, ref_data, &mut buf, "inReal", |r| &r.close);
    let period = opt_time_period(params);
    let t0 = std::time::Instant::now();
    let mut result = None;
    for _ in 0..ctx.iters {
        result = Some(f(input, period));
    }
    let timing = (t0.elapsed().as_nanos() as u64) / ctx.iters;
    match result.unwrap() {
        Ok(out) => {
            let ints: Vec<i32> = out.iter().map(|&v| v as i32).collect();
            respond_ints(
                &[&ints],
                period.saturating_sub(1),
                ctx.start_idx,
                ctx.end_idx,
                timing,
            )
        }
        Err(e) => respond_error_shaped(&e, 0, 1),
    }
}

pub fn dispatch(method: &str, params: &Value, ref_data: &RefData) -> Option<String> {
    use talib_rs::math_operator as mo;
    use talib_rs::math_transform as mt;
    Some(match method {
        // --- Math transforms (lookback 0) ---
        "TA_ACOS" => transform_arm(params, ref_data, mt::acos),
        "TA_ASIN" => transform_arm(params, ref_data, mt::asin),
        "TA_ATAN" => transform_arm(params, ref_data, mt::atan),
        "TA_CEIL" => transform_arm(params, ref_data, mt::ceil),
        "TA_COS" => transform_arm(params, ref_data, mt::cos),
        "TA_COSH" => transform_arm(params, ref_data, mt::cosh),
        "TA_EXP" => transform_arm(params, ref_data, mt::exp),
        "TA_FLOOR" => transform_arm(params, ref_data, mt::floor),
        "TA_LN" => transform_arm(params, ref_data, mt::ln),
        "TA_LOG10" => transform_arm(params, ref_data, mt::log10),
        "TA_SIN" => transform_arm(params, ref_data, mt::sin),
        "TA_SINH" => transform_arm(params, ref_data, mt::sinh),
        "TA_SQRT" => transform_arm(params, ref_data, mt::sqrt),
        "TA_TAN" => transform_arm(params, ref_data, mt::tan),
        "TA_TANH" => transform_arm(params, ref_data, mt::tanh),

        // --- Two-input operators (lookback 0) ---
        "TA_ADD" => pair_arm(params, ref_data, mo::add),
        "TA_SUB" => pair_arm(params, ref_data, mo::sub),
        "TA_MULT" => pair_arm(params, ref_data, mo::mult),
        "TA_DIV" => pair_arm(params, ref_data, mo::div),

        // --- Windowed operators (lookback period-1) ---
        "TA_MAX" => period_real_arm(params, ref_data, mo::max),
        "TA_MIN" => period_real_arm(params, ref_data, mo::min),
        "TA_SUM" => period_real_arm(params, ref_data, mo::sum),
        "TA_MAXINDEX" => period_index_arm(params, ref_data, mo::maxindex),
        "TA_MININDEX" => period_index_arm(params, ref_data, mo::minindex),

        "TA_MINMAX" => {
            let ctx = call_ctx(params);
            let mut buf = Vec::new();
            let input = get_input(params, ref_data, &mut buf, "inReal", |r| &r.close);
            let period = opt_time_period(params);
            let t0 = std::time::Instant::now();
            let mut result = None;
            for _ in 0..ctx.iters {
                result = Some(mo::minmax(input, period));
            }
            let timing = (t0.elapsed().as_nanos() as u64) / ctx.iters;
            match result.unwrap() {
                // Tuple is (min, max); TA-Lib order is outMin, outMax so
                // outReal = min, outReal1 = max (matches the generated arm).
                Ok((out_min, out_max)) => respond_reals_from(
                    &[&out_min, &out_max],
                    period.saturating_sub(1),
                    ctx.start_idx,
                    ctx.end_idx,
                    timing,
                ),
                Err(e) => respond_error_shaped(&e, 2, 0),
            }
        }

        "TA_MINMAXINDEX" => {
            let ctx = call_ctx(params);
            let mut buf = Vec::new();
            let input = get_input(params, ref_data, &mut buf, "inReal", |r| &r.close);
            let period = opt_time_period(params);
            let t0 = std::time::Instant::now();
            let mut result = None;
            for _ in 0..ctx.iters {
                result = Some(mo::minmaxindex(input, period));
            }
            let timing = (t0.elapsed().as_nanos() as u64) / ctx.iters;
            match result.unwrap() {
                // Tuple is (minidx, maxidx) as f64 absolute indices; TA-Lib
                // order is outMinIdx, outMaxIdx so outInteger = minidx,
                // outInteger1 = maxidx (matches the generated arm).
                Ok((out_minidx, out_maxidx)) => {
                    let min_ints: Vec<i32> = out_minidx.iter().map(|&v| v as i32).collect();
                    let max_ints: Vec<i32> = out_maxidx.iter().map(|&v| v as i32).collect();
                    respond_ints(
                        &[&min_ints, &max_ints],
                        period.saturating_sub(1),
                        ctx.start_idx,
                        ctx.end_idx,
                        timing,
                    )
                }
                Err(e) => respond_error_shaped(&e, 0, 2),
            }
        }

        _ => return None,
    })
}
