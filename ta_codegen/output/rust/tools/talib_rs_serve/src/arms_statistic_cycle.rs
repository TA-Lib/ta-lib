//! Statistic + Hilbert Transform cycle function arms.
//!
//! Mirrors the ta_codegen server's JSON contract for each "TA_XXX" method:
//! same param names/defaults, same preloaded-column mapping (inReal -> close;
//! BETA/CORREL: inReal0 -> close, inReal1 -> high), same output key order
//! (HT_PHASOR: outReal=InPhase, outReal1=Quadrature; HT_SINE: outReal=Sine,
//! outReal1=LeadSine; HT_TRENDMODE: outInteger).

use crate::{call_ctx, get_input, respond_error, respond_error_shaped, respond_ints, respond_reals, RefData};
use serde_json::Value;

pub const FUNCTIONS: &[&str] = &[
    "TA_BETA",
    "TA_CORREL",
    "TA_LINEARREG",
    "TA_LINEARREG_ANGLE",
    "TA_LINEARREG_INTERCEPT",
    "TA_LINEARREG_SLOPE",
    "TA_STDDEV",
    "TA_TSF",
    "TA_VAR",
    "TA_HT_DCPERIOD",
    "TA_HT_DCPHASE",
    "TA_HT_PHASOR",
    "TA_HT_SINE",
    "TA_HT_TRENDMODE",
];

/// Read an integer optional param; negatives fall back to the default before
/// the usize cast (the protocol sends i64, talib-rs takes usize).
fn usize_param(params: &Value, key: &str, default: i64) -> usize {
    let v = params[key].as_i64().unwrap_or(default);
    if v < 0 {
        default as usize
    } else {
        v as usize
    }
}

pub fn dispatch(method: &str, params: &Value, ref_data: &RefData) -> Option<String> {
    match method {
        // ------------------------------------------------------------------
        // Two-input statistics (inReal0 -> close, inReal1 -> high preloaded)
        // ------------------------------------------------------------------
        "TA_BETA" => {
            let ctx = call_ctx(params);
            let mut buf0 = Vec::new();
            let mut buf1 = Vec::new();
            let input0 = get_input(params, ref_data, &mut buf0, "inReal0", |r| &r.close);
            let input1 = get_input(params, ref_data, &mut buf1, "inReal1", |r| &r.high);
            let period = usize_param(params, "optInTimePeriod", 5);
            let t0 = std::time::Instant::now();
            let mut result = None;
            for _ in 0..ctx.iters {
                result = Some(talib_rs::statistic::beta(input0, input1, period));
            }
            let timing = (t0.elapsed().as_nanos() as u64) / ctx.iters;
            Some(match result.unwrap() {
                Ok(out) => respond_reals(&[&out], ctx.start_idx, ctx.end_idx, timing),
                Err(e) => respond_error(&e),
            })
        }
        "TA_CORREL" => {
            let ctx = call_ctx(params);
            let mut buf0 = Vec::new();
            let mut buf1 = Vec::new();
            let input0 = get_input(params, ref_data, &mut buf0, "inReal0", |r| &r.close);
            let input1 = get_input(params, ref_data, &mut buf1, "inReal1", |r| &r.high);
            let period = usize_param(params, "optInTimePeriod", 30);
            let t0 = std::time::Instant::now();
            let mut result = None;
            for _ in 0..ctx.iters {
                result = Some(talib_rs::statistic::correl(input0, input1, period));
            }
            let timing = (t0.elapsed().as_nanos() as u64) / ctx.iters;
            Some(match result.unwrap() {
                Ok(out) => respond_reals(&[&out], ctx.start_idx, ctx.end_idx, timing),
                Err(e) => respond_error(&e),
            })
        }
        // ------------------------------------------------------------------
        // Linear regression family + TSF (single input, period only)
        // ------------------------------------------------------------------
        "TA_LINEARREG" => {
            let ctx = call_ctx(params);
            let mut buf = Vec::new();
            let input = get_input(params, ref_data, &mut buf, "inReal", |r| &r.close);
            let period = usize_param(params, "optInTimePeriod", 14);
            let t0 = std::time::Instant::now();
            let mut result = None;
            for _ in 0..ctx.iters {
                result = Some(talib_rs::statistic::linearreg(input, period));
            }
            let timing = (t0.elapsed().as_nanos() as u64) / ctx.iters;
            Some(match result.unwrap() {
                Ok(out) => respond_reals(&[&out], ctx.start_idx, ctx.end_idx, timing),
                Err(e) => respond_error(&e),
            })
        }
        "TA_LINEARREG_ANGLE" => {
            let ctx = call_ctx(params);
            let mut buf = Vec::new();
            let input = get_input(params, ref_data, &mut buf, "inReal", |r| &r.close);
            let period = usize_param(params, "optInTimePeriod", 14);
            let t0 = std::time::Instant::now();
            let mut result = None;
            for _ in 0..ctx.iters {
                result = Some(talib_rs::statistic::linearreg_angle(input, period));
            }
            let timing = (t0.elapsed().as_nanos() as u64) / ctx.iters;
            Some(match result.unwrap() {
                Ok(out) => respond_reals(&[&out], ctx.start_idx, ctx.end_idx, timing),
                Err(e) => respond_error(&e),
            })
        }
        "TA_LINEARREG_INTERCEPT" => {
            let ctx = call_ctx(params);
            let mut buf = Vec::new();
            let input = get_input(params, ref_data, &mut buf, "inReal", |r| &r.close);
            let period = usize_param(params, "optInTimePeriod", 14);
            let t0 = std::time::Instant::now();
            let mut result = None;
            for _ in 0..ctx.iters {
                result = Some(talib_rs::statistic::linearreg_intercept(input, period));
            }
            let timing = (t0.elapsed().as_nanos() as u64) / ctx.iters;
            Some(match result.unwrap() {
                Ok(out) => respond_reals(&[&out], ctx.start_idx, ctx.end_idx, timing),
                Err(e) => respond_error(&e),
            })
        }
        "TA_LINEARREG_SLOPE" => {
            let ctx = call_ctx(params);
            let mut buf = Vec::new();
            let input = get_input(params, ref_data, &mut buf, "inReal", |r| &r.close);
            let period = usize_param(params, "optInTimePeriod", 14);
            let t0 = std::time::Instant::now();
            let mut result = None;
            for _ in 0..ctx.iters {
                result = Some(talib_rs::statistic::linearreg_slope(input, period));
            }
            let timing = (t0.elapsed().as_nanos() as u64) / ctx.iters;
            Some(match result.unwrap() {
                Ok(out) => respond_reals(&[&out], ctx.start_idx, ctx.end_idx, timing),
                Err(e) => respond_error(&e),
            })
        }
        "TA_TSF" => {
            let ctx = call_ctx(params);
            let mut buf = Vec::new();
            let input = get_input(params, ref_data, &mut buf, "inReal", |r| &r.close);
            let period = usize_param(params, "optInTimePeriod", 14);
            let t0 = std::time::Instant::now();
            let mut result = None;
            for _ in 0..ctx.iters {
                result = Some(talib_rs::statistic::tsf(input, period));
            }
            let timing = (t0.elapsed().as_nanos() as u64) / ctx.iters;
            Some(match result.unwrap() {
                Ok(out) => respond_reals(&[&out], ctx.start_idx, ctx.end_idx, timing),
                Err(e) => respond_error(&e),
            })
        }
        // ------------------------------------------------------------------
        // STDDEV / VAR (period + nbdev)
        // ------------------------------------------------------------------
        "TA_STDDEV" => {
            let ctx = call_ctx(params);
            let mut buf = Vec::new();
            let input = get_input(params, ref_data, &mut buf, "inReal", |r| &r.close);
            let period = usize_param(params, "optInTimePeriod", 5);
            let nbdev = params["optInNbDev"].as_f64().unwrap_or(1.0);
            let t0 = std::time::Instant::now();
            let mut result = None;
            for _ in 0..ctx.iters {
                result = Some(talib_rs::statistic::stddev(input, period, nbdev));
            }
            let timing = (t0.elapsed().as_nanos() as u64) / ctx.iters;
            Some(match result.unwrap() {
                Ok(out) => respond_reals(&[&out], ctx.start_idx, ctx.end_idx, timing),
                Err(e) => respond_error(&e),
            })
        }
        "TA_VAR" => {
            let ctx = call_ctx(params);
            let mut buf = Vec::new();
            let input = get_input(params, ref_data, &mut buf, "inReal", |r| &r.close);
            let period = usize_param(params, "optInTimePeriod", 5);
            // talib-rs var() takes nbdev but ignores it (same as TA-Lib's C VAR,
            // whose computation never uses optInNbDev).
            let nbdev = params["optInNbDev"].as_f64().unwrap_or(1.0);
            let t0 = std::time::Instant::now();
            let mut result = None;
            for _ in 0..ctx.iters {
                result = Some(talib_rs::statistic::var(input, period, nbdev));
            }
            let timing = (t0.elapsed().as_nanos() as u64) / ctx.iters;
            Some(match result.unwrap() {
                Ok(out) => respond_reals(&[&out], ctx.start_idx, ctx.end_idx, timing),
                Err(e) => respond_error(&e),
            })
        }
        // ------------------------------------------------------------------
        // Hilbert Transform cycle functions (no params; unstable-period
        // state is a no-op in this server, so these match TA-Lib only at
        // unstablePeriod=0 — see main.rs protocol notes).
        // ------------------------------------------------------------------
        "TA_HT_DCPERIOD" => {
            let ctx = call_ctx(params);
            let mut buf = Vec::new();
            let input = get_input(params, ref_data, &mut buf, "inReal", |r| &r.close);
            let t0 = std::time::Instant::now();
            let mut result = None;
            for _ in 0..ctx.iters {
                result = Some(talib_rs::cycle::ht_dcperiod(input));
            }
            let timing = (t0.elapsed().as_nanos() as u64) / ctx.iters;
            Some(match result.unwrap() {
                Ok(out) => respond_reals(&[&out], ctx.start_idx, ctx.end_idx, timing),
                Err(e) => respond_error(&e),
            })
        }
        "TA_HT_DCPHASE" => {
            let ctx = call_ctx(params);
            let mut buf = Vec::new();
            let input = get_input(params, ref_data, &mut buf, "inReal", |r| &r.close);
            let t0 = std::time::Instant::now();
            let mut result = None;
            for _ in 0..ctx.iters {
                result = Some(talib_rs::cycle::ht_dcphase(input));
            }
            let timing = (t0.elapsed().as_nanos() as u64) / ctx.iters;
            Some(match result.unwrap() {
                Ok(out) => respond_reals(&[&out], ctx.start_idx, ctx.end_idx, timing),
                Err(e) => respond_error(&e),
            })
        }
        "TA_HT_PHASOR" => {
            let ctx = call_ctx(params);
            let mut buf = Vec::new();
            let input = get_input(params, ref_data, &mut buf, "inReal", |r| &r.close);
            let t0 = std::time::Instant::now();
            let mut result = None;
            for _ in 0..ctx.iters {
                result = Some(talib_rs::cycle::ht_phasor(input));
            }
            let timing = (t0.elapsed().as_nanos() as u64) / ctx.iters;
            Some(match result.unwrap() {
                // outReal = InPhase, outReal1 = Quadrature (ta_codegen order)
                Ok((inphase, quadrature)) => respond_reals(
                    &[&inphase, &quadrature],
                    ctx.start_idx,
                    ctx.end_idx,
                    timing,
                ),
                Err(e) => respond_error_shaped(&e, 2, 0),
            })
        }
        "TA_HT_SINE" => {
            let ctx = call_ctx(params);
            let mut buf = Vec::new();
            let input = get_input(params, ref_data, &mut buf, "inReal", |r| &r.close);
            let t0 = std::time::Instant::now();
            let mut result = None;
            for _ in 0..ctx.iters {
                result = Some(talib_rs::cycle::ht_sine(input));
            }
            let timing = (t0.elapsed().as_nanos() as u64) / ctx.iters;
            Some(match result.unwrap() {
                // outReal = Sine, outReal1 = LeadSine (ta_codegen order)
                Ok((sine, leadsine)) => {
                    respond_reals(&[&sine, &leadsine], ctx.start_idx, ctx.end_idx, timing)
                }
                Err(e) => respond_error_shaped(&e, 2, 0),
            })
        }
        "TA_HT_TRENDMODE" => {
            let ctx = call_ctx(params);
            let mut buf = Vec::new();
            let input = get_input(params, ref_data, &mut buf, "inReal", |r| &r.close);
            let t0 = std::time::Instant::now();
            let mut result = None;
            for _ in 0..ctx.iters {
                result = Some(talib_rs::cycle::ht_trendmode(input));
            }
            let timing = (t0.elapsed().as_nanos() as u64) / ctx.iters;
            Some(match result.unwrap() {
                // Integer output: no NaN prefix. talib-rs writes real values
                // from index 63 (its fixed lookback, = TA-Lib's at unstable
                // period 0); earlier entries are 0-filled.
                Ok(out) => respond_ints(&[&out], 63, ctx.start_idx, ctx.end_idx, timing),
                // respond_error's empty-success shape carries an outReal key;
                // for an integer output emit the same empty range with the
                // correct outInteger key instead.
                Err(talib_rs::TaError::InsufficientData { .. }) => {
                    respond_ints(&[&[][..]], 0, ctx.start_idx, ctx.end_idx, timing)
                }
                Err(e) => respond_error(&e),
            })
        }
        _ => None,
    }
}
