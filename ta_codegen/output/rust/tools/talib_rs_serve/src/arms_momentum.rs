//! Momentum-group arms: TA_ADX .. TA_WILLR mapped onto `talib_rs::momentum`.
//!
//! Param names, defaults, input-array keys / preloaded-column mapping and
//! output key order mirror the corresponding arms in the generated
//! `ta_codegen_serve.rs` server.

use crate::{call_ctx, get_input, ma_type_param, respond_error, respond_error_shaped, respond_reals, RefData};
use serde_json::Value;
use talib_rs::momentum as mo;

pub const FUNCTIONS: &[&str] = &[
    "TA_ADX",
    "TA_ADXR",
    "TA_APO",
    "TA_AROON",
    "TA_AROONOSC",
    "TA_BOP",
    "TA_CCI",
    "TA_CMO",
    "TA_DX",
    "TA_MACD",
    "TA_MACDEXT",
    "TA_MACDFIX",
    "TA_MFI",
    "TA_MINUS_DI",
    "TA_MINUS_DM",
    "TA_MOM",
    "TA_PLUS_DI",
    "TA_PLUS_DM",
    "TA_PPO",
    "TA_ROC",
    "TA_ROCP",
    "TA_ROCR",
    "TA_ROCR100",
    "TA_RSI",
    "TA_STOCH",
    "TA_STOCHF",
    "TA_STOCHRSI",
    "TA_TRIX",
    "TA_ULTOSC",
    "TA_WILLR",
];

/// Integer optional param → usize, with the ta_codegen default and negatives
/// clamped to that default (protocol sends i64; talib-rs takes usize).
fn usize_param(params: &Value, key: &str, default: i64) -> usize {
    let v = params[key].as_i64().unwrap_or(default);
    (if v < 0 { default } else { v }) as usize
}

/// Run `f` `iters` times (iters >= 1), returning the last result and the
/// per-iteration timing in nanoseconds.
fn timed<T>(iters: u64, mut f: impl FnMut() -> T) -> (T, u64) {
    let t0 = std::time::Instant::now();
    let mut result = f();
    for _ in 1..iters {
        result = f();
    }
    ((result), (t0.elapsed().as_nanos() as u64) / iters)
}

pub fn dispatch(method: &str, params: &Value, ref_data: &RefData) -> Option<String> {
    match method {
        "TA_ADX" => {
            let ctx = call_ctx(params);
            let (mut bh, mut bl, mut bc) = (Vec::new(), Vec::new(), Vec::new());
            let high = get_input(params, ref_data, &mut bh, "inHigh", |r| &r.high);
            let low = get_input(params, ref_data, &mut bl, "inLow", |r| &r.low);
            let close = get_input(params, ref_data, &mut bc, "inClose", |r| &r.close);
            let period = usize_param(params, "optInTimePeriod", 14);
            let (result, timing) = timed(ctx.iters, || mo::adx(high, low, close, period));
            Some(match result {
                Ok(out) => respond_reals(&[&out], ctx.start_idx, ctx.end_idx, timing),
                Err(e) => respond_error(&e),
            })
        }
        "TA_ADXR" => {
            let ctx = call_ctx(params);
            let (mut bh, mut bl, mut bc) = (Vec::new(), Vec::new(), Vec::new());
            let high = get_input(params, ref_data, &mut bh, "inHigh", |r| &r.high);
            let low = get_input(params, ref_data, &mut bl, "inLow", |r| &r.low);
            let close = get_input(params, ref_data, &mut bc, "inClose", |r| &r.close);
            let period = usize_param(params, "optInTimePeriod", 14);
            let (result, timing) = timed(ctx.iters, || mo::adxr(high, low, close, period));
            Some(match result {
                Ok(out) => respond_reals(&[&out], ctx.start_idx, ctx.end_idx, timing),
                Err(e) => respond_error(&e),
            })
        }
        "TA_APO" => {
            let ctx = call_ctx(params);
            let mut buf = Vec::new();
            let input = get_input(params, ref_data, &mut buf, "inReal", |r| &r.close);
            let fast = usize_param(params, "optInFastPeriod", 12);
            let slow = usize_param(params, "optInSlowPeriod", 26);
            let matype = match ma_type_param(params, "optInMAType") {
                Ok(m) => m,
                Err(e) => return Some(respond_error(&e)),
            };
            let (result, timing) = timed(ctx.iters, || mo::apo(input, fast, slow, matype));
            Some(match result {
                Ok(out) => respond_reals(&[&out], ctx.start_idx, ctx.end_idx, timing),
                Err(e) => respond_error(&e),
            })
        }
        "TA_AROON" => {
            let ctx = call_ctx(params);
            let (mut bh, mut bl) = (Vec::new(), Vec::new());
            let high = get_input(params, ref_data, &mut bh, "inHigh", |r| &r.high);
            let low = get_input(params, ref_data, &mut bl, "inLow", |r| &r.low);
            let period = usize_param(params, "optInTimePeriod", 14);
            let (result, timing) = timed(ctx.iters, || mo::aroon(high, low, period));
            Some(match result {
                // talib-rs returns (aroon_down, aroon_up); TA-Lib output 0 is
                // outAroonDown, output 1 is outAroonUp — same order.
                Ok((down, up)) => {
                    respond_reals(&[&down, &up], ctx.start_idx, ctx.end_idx, timing)
                }
                Err(e) => respond_error_shaped(&e, 2, 0),
            })
        }
        "TA_AROONOSC" => {
            let ctx = call_ctx(params);
            let (mut bh, mut bl) = (Vec::new(), Vec::new());
            let high = get_input(params, ref_data, &mut bh, "inHigh", |r| &r.high);
            let low = get_input(params, ref_data, &mut bl, "inLow", |r| &r.low);
            let period = usize_param(params, "optInTimePeriod", 14);
            let (result, timing) = timed(ctx.iters, || mo::aroon_osc(high, low, period));
            Some(match result {
                Ok(out) => respond_reals(&[&out], ctx.start_idx, ctx.end_idx, timing),
                Err(e) => respond_error(&e),
            })
        }
        "TA_BOP" => {
            let ctx = call_ctx(params);
            let (mut bo, mut bh, mut bl, mut bc) =
                (Vec::new(), Vec::new(), Vec::new(), Vec::new());
            let open = get_input(params, ref_data, &mut bo, "inOpen", |r| &r.open);
            let high = get_input(params, ref_data, &mut bh, "inHigh", |r| &r.high);
            let low = get_input(params, ref_data, &mut bl, "inLow", |r| &r.low);
            let close = get_input(params, ref_data, &mut bc, "inClose", |r| &r.close);
            let (result, timing) = timed(ctx.iters, || mo::bop(open, high, low, close));
            Some(match result {
                Ok(out) => respond_reals(&[&out], ctx.start_idx, ctx.end_idx, timing),
                Err(e) => respond_error(&e),
            })
        }
        "TA_CCI" => {
            let ctx = call_ctx(params);
            let (mut bh, mut bl, mut bc) = (Vec::new(), Vec::new(), Vec::new());
            let high = get_input(params, ref_data, &mut bh, "inHigh", |r| &r.high);
            let low = get_input(params, ref_data, &mut bl, "inLow", |r| &r.low);
            let close = get_input(params, ref_data, &mut bc, "inClose", |r| &r.close);
            let period = usize_param(params, "optInTimePeriod", 14);
            let (result, timing) = timed(ctx.iters, || mo::cci(high, low, close, period));
            Some(match result {
                Ok(out) => respond_reals(&[&out], ctx.start_idx, ctx.end_idx, timing),
                Err(e) => respond_error(&e),
            })
        }
        "TA_CMO" => {
            let ctx = call_ctx(params);
            let mut buf = Vec::new();
            let input = get_input(params, ref_data, &mut buf, "inReal", |r| &r.close);
            let period = usize_param(params, "optInTimePeriod", 14);
            let (result, timing) = timed(ctx.iters, || mo::cmo(input, period));
            Some(match result {
                Ok(out) => respond_reals(&[&out], ctx.start_idx, ctx.end_idx, timing),
                Err(e) => respond_error(&e),
            })
        }
        "TA_DX" => {
            let ctx = call_ctx(params);
            let (mut bh, mut bl, mut bc) = (Vec::new(), Vec::new(), Vec::new());
            let high = get_input(params, ref_data, &mut bh, "inHigh", |r| &r.high);
            let low = get_input(params, ref_data, &mut bl, "inLow", |r| &r.low);
            let close = get_input(params, ref_data, &mut bc, "inClose", |r| &r.close);
            let period = usize_param(params, "optInTimePeriod", 14);
            let (result, timing) = timed(ctx.iters, || mo::dx(high, low, close, period));
            Some(match result {
                Ok(out) => respond_reals(&[&out], ctx.start_idx, ctx.end_idx, timing),
                Err(e) => respond_error(&e),
            })
        }
        "TA_MACD" => {
            let ctx = call_ctx(params);
            let mut buf = Vec::new();
            let input = get_input(params, ref_data, &mut buf, "inReal", |r| &r.close);
            let fast = usize_param(params, "optInFastPeriod", 12);
            let slow = usize_param(params, "optInSlowPeriod", 26);
            let signal = usize_param(params, "optInSignalPeriod", 9);
            let (result, timing) = timed(ctx.iters, || mo::macd(input, fast, slow, signal));
            Some(match result {
                // (macd, signal, hist) → outReal (outMACD), outReal1
                // (outMACDSignal), outReal2 (outMACDHist).
                Ok((m, s, h)) => {
                    respond_reals(&[&m, &s, &h], ctx.start_idx, ctx.end_idx, timing)
                }
                Err(e) => respond_error_shaped(&e, 3, 0),
            })
        }
        "TA_MACDEXT" => {
            let ctx = call_ctx(params);
            let mut buf = Vec::new();
            let input = get_input(params, ref_data, &mut buf, "inReal", |r| &r.close);
            let fast = usize_param(params, "optInFastPeriod", 12);
            let slow = usize_param(params, "optInSlowPeriod", 26);
            let signal = usize_param(params, "optInSignalPeriod", 9);
            let fast_ma = match ma_type_param(params, "optInFastMAType") {
                Ok(m) => m,
                Err(e) => return Some(respond_error_shaped(&e, 3, 0)),
            };
            let slow_ma = match ma_type_param(params, "optInSlowMAType") {
                Ok(m) => m,
                Err(e) => return Some(respond_error_shaped(&e, 3, 0)),
            };
            let signal_ma = match ma_type_param(params, "optInSignalMAType") {
                Ok(m) => m,
                Err(e) => return Some(respond_error_shaped(&e, 3, 0)),
            };
            let (result, timing) = timed(ctx.iters, || {
                mo::macd_ext(input, fast, fast_ma, slow, slow_ma, signal, signal_ma)
            });
            Some(match result {
                Ok((m, s, h)) => {
                    respond_reals(&[&m, &s, &h], ctx.start_idx, ctx.end_idx, timing)
                }
                Err(e) => respond_error_shaped(&e, 3, 0),
            })
        }
        "TA_MACDFIX" => {
            let ctx = call_ctx(params);
            let mut buf = Vec::new();
            let input = get_input(params, ref_data, &mut buf, "inReal", |r| &r.close);
            let signal = usize_param(params, "optInSignalPeriod", 9);
            let (result, timing) = timed(ctx.iters, || mo::macd_fix(input, signal));
            Some(match result {
                Ok((m, s, h)) => {
                    respond_reals(&[&m, &s, &h], ctx.start_idx, ctx.end_idx, timing)
                }
                Err(e) => respond_error_shaped(&e, 3, 0),
            })
        }
        "TA_MFI" => {
            let ctx = call_ctx(params);
            let (mut bh, mut bl, mut bc, mut bv) =
                (Vec::new(), Vec::new(), Vec::new(), Vec::new());
            let high = get_input(params, ref_data, &mut bh, "inHigh", |r| &r.high);
            let low = get_input(params, ref_data, &mut bl, "inLow", |r| &r.low);
            let close = get_input(params, ref_data, &mut bc, "inClose", |r| &r.close);
            let volume = get_input(params, ref_data, &mut bv, "inVolume", |r| &r.volume);
            let period = usize_param(params, "optInTimePeriod", 14);
            let (result, timing) =
                timed(ctx.iters, || mo::mfi(high, low, close, volume, period));
            Some(match result {
                Ok(out) => respond_reals(&[&out], ctx.start_idx, ctx.end_idx, timing),
                Err(e) => respond_error(&e),
            })
        }
        "TA_MINUS_DI" => {
            let ctx = call_ctx(params);
            let (mut bh, mut bl, mut bc) = (Vec::new(), Vec::new(), Vec::new());
            let high = get_input(params, ref_data, &mut bh, "inHigh", |r| &r.high);
            let low = get_input(params, ref_data, &mut bl, "inLow", |r| &r.low);
            let close = get_input(params, ref_data, &mut bc, "inClose", |r| &r.close);
            let period = usize_param(params, "optInTimePeriod", 14);
            let (result, timing) = timed(ctx.iters, || mo::minus_di(high, low, close, period));
            Some(match result {
                Ok(out) => respond_reals(&[&out], ctx.start_idx, ctx.end_idx, timing),
                Err(e) => respond_error(&e),
            })
        }
        "TA_MINUS_DM" => {
            let ctx = call_ctx(params);
            let (mut bh, mut bl) = (Vec::new(), Vec::new());
            let high = get_input(params, ref_data, &mut bh, "inHigh", |r| &r.high);
            let low = get_input(params, ref_data, &mut bl, "inLow", |r| &r.low);
            let period = usize_param(params, "optInTimePeriod", 14);
            let (result, timing) = timed(ctx.iters, || mo::minus_dm(high, low, period));
            Some(match result {
                Ok(out) => respond_reals(&[&out], ctx.start_idx, ctx.end_idx, timing),
                Err(e) => respond_error(&e),
            })
        }
        "TA_MOM" => {
            let ctx = call_ctx(params);
            let mut buf = Vec::new();
            let input = get_input(params, ref_data, &mut buf, "inReal", |r| &r.close);
            let period = usize_param(params, "optInTimePeriod", 10);
            let (result, timing) = timed(ctx.iters, || mo::mom(input, period));
            Some(match result {
                Ok(out) => respond_reals(&[&out], ctx.start_idx, ctx.end_idx, timing),
                Err(e) => respond_error(&e),
            })
        }
        "TA_PLUS_DI" => {
            let ctx = call_ctx(params);
            let (mut bh, mut bl, mut bc) = (Vec::new(), Vec::new(), Vec::new());
            let high = get_input(params, ref_data, &mut bh, "inHigh", |r| &r.high);
            let low = get_input(params, ref_data, &mut bl, "inLow", |r| &r.low);
            let close = get_input(params, ref_data, &mut bc, "inClose", |r| &r.close);
            let period = usize_param(params, "optInTimePeriod", 14);
            let (result, timing) = timed(ctx.iters, || mo::plus_di(high, low, close, period));
            Some(match result {
                Ok(out) => respond_reals(&[&out], ctx.start_idx, ctx.end_idx, timing),
                Err(e) => respond_error(&e),
            })
        }
        "TA_PLUS_DM" => {
            let ctx = call_ctx(params);
            let (mut bh, mut bl) = (Vec::new(), Vec::new());
            let high = get_input(params, ref_data, &mut bh, "inHigh", |r| &r.high);
            let low = get_input(params, ref_data, &mut bl, "inLow", |r| &r.low);
            let period = usize_param(params, "optInTimePeriod", 14);
            let (result, timing) = timed(ctx.iters, || mo::plus_dm(high, low, period));
            Some(match result {
                Ok(out) => respond_reals(&[&out], ctx.start_idx, ctx.end_idx, timing),
                Err(e) => respond_error(&e),
            })
        }
        "TA_PPO" => {
            let ctx = call_ctx(params);
            let mut buf = Vec::new();
            let input = get_input(params, ref_data, &mut buf, "inReal", |r| &r.close);
            let fast = usize_param(params, "optInFastPeriod", 12);
            let slow = usize_param(params, "optInSlowPeriod", 26);
            let matype = match ma_type_param(params, "optInMAType") {
                Ok(m) => m,
                Err(e) => return Some(respond_error(&e)),
            };
            let (result, timing) = timed(ctx.iters, || mo::ppo(input, fast, slow, matype));
            Some(match result {
                Ok(out) => respond_reals(&[&out], ctx.start_idx, ctx.end_idx, timing),
                Err(e) => respond_error(&e),
            })
        }
        "TA_ROC" => {
            let ctx = call_ctx(params);
            let mut buf = Vec::new();
            let input = get_input(params, ref_data, &mut buf, "inReal", |r| &r.close);
            let period = usize_param(params, "optInTimePeriod", 10);
            let (result, timing) = timed(ctx.iters, || mo::roc(input, period));
            Some(match result {
                Ok(out) => respond_reals(&[&out], ctx.start_idx, ctx.end_idx, timing),
                Err(e) => respond_error(&e),
            })
        }
        "TA_ROCP" => {
            let ctx = call_ctx(params);
            let mut buf = Vec::new();
            let input = get_input(params, ref_data, &mut buf, "inReal", |r| &r.close);
            let period = usize_param(params, "optInTimePeriod", 10);
            let (result, timing) = timed(ctx.iters, || mo::rocp(input, period));
            Some(match result {
                Ok(out) => respond_reals(&[&out], ctx.start_idx, ctx.end_idx, timing),
                Err(e) => respond_error(&e),
            })
        }
        "TA_ROCR" => {
            let ctx = call_ctx(params);
            let mut buf = Vec::new();
            let input = get_input(params, ref_data, &mut buf, "inReal", |r| &r.close);
            let period = usize_param(params, "optInTimePeriod", 10);
            let (result, timing) = timed(ctx.iters, || mo::rocr(input, period));
            Some(match result {
                Ok(out) => respond_reals(&[&out], ctx.start_idx, ctx.end_idx, timing),
                Err(e) => respond_error(&e),
            })
        }
        "TA_ROCR100" => {
            let ctx = call_ctx(params);
            let mut buf = Vec::new();
            let input = get_input(params, ref_data, &mut buf, "inReal", |r| &r.close);
            let period = usize_param(params, "optInTimePeriod", 10);
            let (result, timing) = timed(ctx.iters, || mo::rocr100(input, period));
            Some(match result {
                Ok(out) => respond_reals(&[&out], ctx.start_idx, ctx.end_idx, timing),
                Err(e) => respond_error(&e),
            })
        }
        "TA_RSI" => {
            let ctx = call_ctx(params);
            let mut buf = Vec::new();
            let input = get_input(params, ref_data, &mut buf, "inReal", |r| &r.close);
            let period = usize_param(params, "optInTimePeriod", 14);
            let (result, timing) = timed(ctx.iters, || mo::rsi(input, period));
            Some(match result {
                Ok(out) => respond_reals(&[&out], ctx.start_idx, ctx.end_idx, timing),
                Err(e) => respond_error(&e),
            })
        }
        "TA_STOCH" => {
            let ctx = call_ctx(params);
            let (mut bh, mut bl, mut bc) = (Vec::new(), Vec::new(), Vec::new());
            let high = get_input(params, ref_data, &mut bh, "inHigh", |r| &r.high);
            let low = get_input(params, ref_data, &mut bl, "inLow", |r| &r.low);
            let close = get_input(params, ref_data, &mut bc, "inClose", |r| &r.close);
            let fastk = usize_param(params, "optInFastK_Period", 5);
            let slowk = usize_param(params, "optInSlowK_Period", 3);
            let slowk_ma = match ma_type_param(params, "optInSlowK_MAType") {
                Ok(m) => m,
                Err(e) => return Some(respond_error_shaped(&e, 2, 0)),
            };
            let slowd = usize_param(params, "optInSlowD_Period", 3);
            let slowd_ma = match ma_type_param(params, "optInSlowD_MAType") {
                Ok(m) => m,
                Err(e) => return Some(respond_error_shaped(&e, 2, 0)),
            };
            let (result, timing) = timed(ctx.iters, || {
                mo::stoch(high, low, close, fastk, slowk, slowk_ma, slowd, slowd_ma)
            });
            Some(match result {
                // (slowk, slowd) → outReal (outSlowK), outReal1 (outSlowD).
                Ok((k, d)) => respond_reals(&[&k, &d], ctx.start_idx, ctx.end_idx, timing),
                Err(e) => respond_error_shaped(&e, 2, 0),
            })
        }
        "TA_STOCHF" => {
            let ctx = call_ctx(params);
            let (mut bh, mut bl, mut bc) = (Vec::new(), Vec::new(), Vec::new());
            let high = get_input(params, ref_data, &mut bh, "inHigh", |r| &r.high);
            let low = get_input(params, ref_data, &mut bl, "inLow", |r| &r.low);
            let close = get_input(params, ref_data, &mut bc, "inClose", |r| &r.close);
            let fastk = usize_param(params, "optInFastK_Period", 5);
            let fastd = usize_param(params, "optInFastD_Period", 3);
            let fastd_ma = match ma_type_param(params, "optInFastD_MAType") {
                Ok(m) => m,
                Err(e) => return Some(respond_error_shaped(&e, 2, 0)),
            };
            let (result, timing) = timed(ctx.iters, || {
                mo::stochf(high, low, close, fastk, fastd, fastd_ma)
            });
            Some(match result {
                // (fastk, fastd) → outReal (outFastK), outReal1 (outFastD).
                Ok((k, d)) => respond_reals(&[&k, &d], ctx.start_idx, ctx.end_idx, timing),
                Err(e) => respond_error_shaped(&e, 2, 0),
            })
        }
        "TA_STOCHRSI" => {
            let ctx = call_ctx(params);
            let mut buf = Vec::new();
            let input = get_input(params, ref_data, &mut buf, "inReal", |r| &r.close);
            let period = usize_param(params, "optInTimePeriod", 14);
            let fastk = usize_param(params, "optInFastK_Period", 5);
            let fastd = usize_param(params, "optInFastD_Period", 3);
            let fastd_ma = match ma_type_param(params, "optInFastD_MAType") {
                Ok(m) => m,
                Err(e) => return Some(respond_error_shaped(&e, 2, 0)),
            };
            let (result, timing) = timed(ctx.iters, || {
                mo::stochrsi(input, period, fastk, fastd, fastd_ma)
            });
            Some(match result {
                Ok((k, d)) => respond_reals(&[&k, &d], ctx.start_idx, ctx.end_idx, timing),
                Err(e) => respond_error_shaped(&e, 2, 0),
            })
        }
        "TA_TRIX" => {
            let ctx = call_ctx(params);
            let mut buf = Vec::new();
            let input = get_input(params, ref_data, &mut buf, "inReal", |r| &r.close);
            let period = usize_param(params, "optInTimePeriod", 30);
            let (result, timing) = timed(ctx.iters, || mo::trix(input, period));
            Some(match result {
                Ok(out) => respond_reals(&[&out], ctx.start_idx, ctx.end_idx, timing),
                Err(e) => respond_error(&e),
            })
        }
        "TA_ULTOSC" => {
            let ctx = call_ctx(params);
            let (mut bh, mut bl, mut bc) = (Vec::new(), Vec::new(), Vec::new());
            let high = get_input(params, ref_data, &mut bh, "inHigh", |r| &r.high);
            let low = get_input(params, ref_data, &mut bl, "inLow", |r| &r.low);
            let close = get_input(params, ref_data, &mut bc, "inClose", |r| &r.close);
            let p1 = usize_param(params, "optInTimePeriod1", 7);
            let p2 = usize_param(params, "optInTimePeriod2", 14);
            let p3 = usize_param(params, "optInTimePeriod3", 28);
            let (result, timing) =
                timed(ctx.iters, || mo::ultosc(high, low, close, p1, p2, p3));
            Some(match result {
                Ok(out) => respond_reals(&[&out], ctx.start_idx, ctx.end_idx, timing),
                Err(e) => respond_error(&e),
            })
        }
        "TA_WILLR" => {
            let ctx = call_ctx(params);
            let (mut bh, mut bl, mut bc) = (Vec::new(), Vec::new(), Vec::new());
            let high = get_input(params, ref_data, &mut bh, "inHigh", |r| &r.high);
            let low = get_input(params, ref_data, &mut bl, "inLow", |r| &r.low);
            let close = get_input(params, ref_data, &mut bc, "inClose", |r| &r.close);
            let period = usize_param(params, "optInTimePeriod", 14);
            let (result, timing) = timed(ctx.iters, || mo::willr(high, low, close, period));
            Some(match result {
                Ok(out) => respond_reals(&[&out], ctx.start_idx, ctx.end_idx, timing),
                Err(e) => respond_error(&e),
            })
        }
        _ => None,
    }
}
