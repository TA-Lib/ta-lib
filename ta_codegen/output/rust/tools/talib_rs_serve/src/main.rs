//! JSON-RPC server exposing the third-party `talib-rs` crate (pure-Rust TA-Lib
//! reimplementation, crates.io `talib-rs`, BSD-3-Clause) over the same
//! stdin/stdout protocol as the ta_codegen language servers, so `ta_bench`
//! can benchmark it alongside them (opt-in: `--language=talib_rs`; never part
//! of a default run). Build via `scripts/build.py talib-rs-server`.
//!
//! Protocol notes (differences from the ta_codegen servers):
//! - talib-rs functions take a full input slice and return full-length Vecs
//!   with a NaN-prefixed lookback region (numpy/python-talib convention).
//!   Arms convert that to TA-Lib's outBegIdx/outNBElement compaction:
//!   outBegIdx = max(startIdx, first_valid), values = out[outBegIdx..=endIdx].
//!   The returned timing includes talib-rs's output-Vec allocation — that is
//!   its real per-call API cost.
//! - talib-rs has no unstable-period or compatibility state:
//!   `set_unstable_period` / `set_compatibility` are accepted as no-ops.
//! - talib-rs errors with InsufficientData where TA-Lib returns success with
//!   an empty range; arms map that error to an empty success response.
//! - `timing_ns_unguarded` duplicates `timing_ns` (talib-rs has no unguarded
//!   tier).

use serde_json::Value;
use std::io::{self, BufRead, Write};

mod arms_math;
mod arms_momentum;
mod arms_overlap;
mod arms_pattern;
mod arms_statistic_cycle;
mod arms_volume_volatility_price;

/// OHLCV data preloaded via the `load_data` method (used by ta_bench so each
/// call doesn't re-send megabytes of JSON).
#[derive(Default)]
pub struct RefData {
    pub open: Vec<f64>,
    pub high: Vec<f64>,
    pub low: Vec<f64>,
    pub close: Vec<f64>,
    pub volume: Vec<f64>,
}

impl RefData {
    pub fn n(&self) -> usize {
        self.close.len()
    }
}

pub fn parse_f64_array(v: &Value) -> Vec<f64> {
    v.as_array()
        .map(|a| a.iter().map(|x| x.as_f64().unwrap_or(f64::NAN)).collect())
        .unwrap_or_default()
}

pub fn json_f64_array(data: &[f64]) -> String {
    let mut s = String::with_capacity(data.len() * 8 + 2);
    s.push('[');
    for (i, &v) in data.iter().enumerate() {
        if i > 0 {
            s.push(',');
        }
        match serde_json::Number::from_f64(v) {
            Some(n) => s.push_str(&n.to_string()),
            None => s.push_str(if v.is_nan() {
                if v.is_sign_negative() {
                    "-nan"
                } else {
                    "nan"
                }
            } else if v < 0.0 {
                "-inf"
            } else {
                "inf"
            }),
        }
    }
    s.push(']');
    s
}

pub fn json_i32_array(data: &[i32]) -> String {
    let mut s = String::with_capacity(data.len() * 4 + 2);
    s.push('[');
    for (i, &v) in data.iter().enumerate() {
        if i > 0 {
            s.push(',');
        }
        s.push_str(&v.to_string());
    }
    s.push(']');
    s
}

/// Count of leading NaNs — talib-rs's lookback marker on real outputs.
pub fn nan_prefix_len(out: &[f64]) -> usize {
    out.iter().take_while(|v| v.is_nan()).count()
}

/// Build the standard success response for real-valued outputs.
///
/// `outs` are full-length talib-rs output vectors (NaN-prefixed). The shared
/// first-valid index is the max NaN prefix across outputs (TA-Lib compacts all
/// outputs of one call to a single outBegIdx). Output JSON keys follow the
/// ta_codegen server convention: outReal, outReal1, outReal2.
pub fn respond_reals(outs: &[&[f64]], start_idx: usize, end_idx: usize, timing_ns: u64) -> String {
    let first_valid = outs.iter().map(|o| nan_prefix_len(o)).max().unwrap_or(0);
    respond_reals_from(outs, first_valid, start_idx, end_idx, timing_ns)
}

/// Same as `respond_reals` but with an explicit first-valid index (for outputs
/// where the NaN prefix is not a reliable lookback marker).
pub fn respond_reals_from(
    outs: &[&[f64]],
    first_valid: usize,
    start_idx: usize,
    end_idx: usize,
    timing_ns: u64,
) -> String {
    let beg = start_idx.max(first_valid);
    let (out_beg, nb) = if beg > end_idx || outs.iter().any(|o| end_idx >= o.len()) {
        (0usize, 0usize)
    } else {
        (beg, end_idx - beg + 1)
    };
    let mut resp = format!(
        "{{\"retCode\":0,\"outBegIdx\":{},\"outNBElement\":{},\"lookback\":{},\"timing_ns\":{},\"timing_ns_unguarded\":{}",
        out_beg, nb, first_valid, timing_ns, timing_ns
    );
    for (i, out) in outs.iter().enumerate() {
        let key = if i == 0 {
            "outReal".to_string()
        } else {
            format!("outReal{i}")
        };
        let slice = if nb > 0 { &out[out_beg..out_beg + nb] } else { &[][..] };
        resp.push_str(&format!(",\"{}\":{}", key, json_f64_array(slice)));
    }
    resp.push('}');
    resp
}

/// Build the standard success response for integer-valued outputs (CDL*,
/// MAXINDEX/MININDEX, MINMAXINDEX). Integer outputs carry no NaN marker, so
/// the caller passes the talib-rs lookback explicitly (mirroring the formula
/// in the talib-rs source for that function). Keys: outInteger, outInteger1.
pub fn respond_ints(
    outs: &[&[i32]],
    first_valid: usize,
    start_idx: usize,
    end_idx: usize,
    timing_ns: u64,
) -> String {
    let beg = start_idx.max(first_valid);
    let (out_beg, nb) = if beg > end_idx || outs.iter().any(|o| end_idx >= o.len()) {
        (0usize, 0usize)
    } else {
        (beg, end_idx - beg + 1)
    };
    let mut resp = format!(
        "{{\"retCode\":0,\"outBegIdx\":{},\"outNBElement\":{},\"lookback\":{},\"timing_ns\":{},\"timing_ns_unguarded\":{}",
        out_beg, nb, first_valid, timing_ns, timing_ns
    );
    for (i, out) in outs.iter().enumerate() {
        let key = if i == 0 {
            "outInteger".to_string()
        } else {
            format!("outInteger{i}")
        };
        let slice = if nb > 0 { &out[out_beg..out_beg + nb] } else { &[][..] };
        resp.push_str(&format!(",\"{}\":{}", key, json_i32_array(slice)));
    }
    resp.push('}');
    resp
}

/// Map a talib-rs error to a protocol response. InsufficientData corresponds
/// to TA-Lib's success-with-empty-range; anything else is TA_BAD_PARAM (2).
/// For the empty-success case the response must carry the same output-key
/// layout as the arm's normal path (outReal/outReal1/... then
/// outInteger/outInteger1/...), so multi-output and integer-output arms pass
/// their shape explicitly.
pub fn respond_error_shaped(err: &talib_rs::TaError, n_reals: usize, n_ints: usize) -> String {
    match err {
        talib_rs::TaError::InsufficientData { .. } => {
            let mut resp = String::from(
                "{\"retCode\":0,\"outBegIdx\":0,\"outNBElement\":0,\"lookback\":0,\"timing_ns\":0,\"timing_ns_unguarded\":0",
            );
            for i in 0..n_reals {
                if i == 0 {
                    resp.push_str(",\"outReal\":[]");
                } else {
                    resp.push_str(&format!(",\"outReal{i}\":[]"));
                }
            }
            for i in 0..n_ints {
                if i == 0 {
                    resp.push_str(",\"outInteger\":[]");
                } else {
                    resp.push_str(&format!(",\"outInteger{i}\":[]"));
                }
            }
            resp.push('}');
            resp
        }
        _ => "{\"retCode\":2,\"outBegIdx\":0,\"outNBElement\":0,\"timing_ns\":0}".to_string(),
    }
}

/// Shorthand for the common single-real-output shape.
pub fn respond_error(err: &talib_rs::TaError) -> String {
    respond_error_shaped(err, 1, 0)
}

/// MaType from the protocol's integer ordinal (0..=8, TA-Lib compatible).
/// Negative and out-of-range ordinals map to InvalidParameter -> retCode 2,
/// matching TA-Lib's TA_BAD_PARAM for a bad optInMAType — one policy for
/// every arm.
pub fn ma_type_param(params: &Value, key: &str) -> Result<talib_rs::MaType, talib_rs::TaError> {
    let v = params[key].as_i64().unwrap_or(0) as i32;
    talib_rs::MaType::try_from(v)
}

/// Fetch a named input array: preloaded column if `use_preloaded`, else the
/// JSON param. `column` selects which preloaded series maps to this input
/// (the ta_codegen servers map bare `inReal` to close).
pub fn get_input<'a>(
    params: &Value,
    ref_data: &'a RefData,
    json_buf: &'a mut Vec<f64>,
    key: &str,
    column: fn(&RefData) -> &Vec<f64>,
) -> &'a [f64] {
    let use_preloaded = params["use_preloaded"].as_i64().unwrap_or(0);
    if use_preloaded != 0 && ref_data.n() > 0 {
        column(ref_data)
    } else {
        *json_buf = parse_f64_array(&params[key]);
        json_buf
    }
}

pub struct CallCtx {
    pub start_idx: usize,
    pub end_idx: usize,
    pub iters: u64,
}

pub fn call_ctx(params: &Value) -> CallCtx {
    CallCtx {
        start_idx: params["startIdx"].as_u64().unwrap_or(0) as usize,
        end_idx: params["endIdx"].as_u64().unwrap_or(0) as usize,
        iters: std::cmp::max(1, params["iters"].as_i64().unwrap_or(1)) as u64,
    }
}

fn handle_line(line: &str, ref_data: &mut RefData) -> String {
    let req: Value = match serde_json::from_str(line) {
        Ok(v) => v,
        Err(e) => return format!("{{\"error\":\"parse error: {e}\"}}"),
    };
    let method = req["method"].as_str().unwrap_or("");
    let params = &req["params"];

    match method {
        "load_data" => {
            ref_data.open = parse_f64_array(&params["open"]);
            ref_data.high = parse_f64_array(&params["high"]);
            ref_data.low = parse_f64_array(&params["low"]);
            ref_data.close = parse_f64_array(&params["close"]);
            ref_data.volume = parse_f64_array(&params["volume"]);
            format!("{{\"status\":\"ok\",\"n\":{}}}", ref_data.n())
        }
        "list_functions" => {
            let mut funcs: Vec<&'static str> = Vec::new();
            funcs.extend_from_slice(arms_math::FUNCTIONS);
            funcs.extend_from_slice(arms_momentum::FUNCTIONS);
            funcs.extend_from_slice(arms_overlap::FUNCTIONS);
            funcs.extend_from_slice(arms_pattern::FUNCTIONS);
            funcs.extend_from_slice(arms_statistic_cycle::FUNCTIONS);
            funcs.extend_from_slice(arms_volume_volatility_price::FUNCTIONS);
            funcs.sort_unstable();
            serde_json::json!({ "functions": funcs }).to_string()
        }
        // talib-rs has no unstable-period / compatibility state. Accept and
        // ignore so a driver's global-state setup doesn't abort the session;
        // talib-rs always behaves as if unstablePeriod were 0.
        "set_unstable_period" | "set_compatibility" => "{\"status\":\"ok\"}".to_string(),
        _ => arms_overlap::dispatch(method, params, ref_data)
            .or_else(|| arms_momentum::dispatch(method, params, ref_data))
            .or_else(|| arms_math::dispatch(method, params, ref_data))
            .or_else(|| arms_statistic_cycle::dispatch(method, params, ref_data))
            .or_else(|| arms_volume_volatility_price::dispatch(method, params, ref_data))
            .or_else(|| arms_pattern::dispatch(method, params, ref_data))
            .unwrap_or_else(|| format!("{{\"error\":\"unknown method: {method}\"}}")),
    }
}

fn main() {
    let stdin = io::stdin();
    let stdout = io::stdout();
    let mut out = stdout.lock();
    let mut ref_data = RefData::default();

    for line in stdin.lock().lines() {
        let line = match line {
            Ok(l) => l,
            Err(_) => break,
        };
        if line.trim().is_empty() {
            continue;
        }
        let resp = handle_line(&line, &mut ref_data);
        if writeln!(out, "{resp}").is_err() {
            break;
        }
        let _ = out.flush();
    }
}
