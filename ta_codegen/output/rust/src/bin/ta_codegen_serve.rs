#![allow(non_snake_case, unused_variables, clippy::all)]

use serde_json::{self, Value};
use std::io::{self, BufRead, Write};
use std::time::Instant;
use ta_lib::{Core, RetCode, FuncUnstId, Compatibility};
use ta_lib::abstract_api::{self, InputType, OutputType, OptDomain};

const MAX_ARRAY_SIZE: usize = 200000;

struct RefData {
    open: Vec<f64>,
    high: Vec<f64>,
    low: Vec<f64>,
    close: Vec<f64>,
    volume: Vec<f64>,
    oi: Vec<f64>,
    n: usize,
}

impl RefData {
    fn new() -> Self {
        RefData {
            open: vec![0.0; MAX_ARRAY_SIZE],
            high: vec![0.0; MAX_ARRAY_SIZE],
            low: vec![0.0; MAX_ARRAY_SIZE],
            close: vec![0.0; MAX_ARRAY_SIZE],
            volume: vec![0.0; MAX_ARRAY_SIZE],
            oi: vec![0.0; MAX_ARRAY_SIZE],
            n: 0,
        }
    }
}

fn parse_f64_array(val: &Value) -> Vec<f64> {
    match val.as_array() {
        Some(arr) => arr.iter().filter_map(|v| v.as_f64()).collect(),
        None => Vec::new(),
    }
}

fn retcode_to_int(rc: RetCode) -> i32 {
    match rc {
        RetCode::Success => 0,
        RetCode::BadParam => 2,
        RetCode::AllocErr => 3,
        RetCode::InternalError => 5000,
        RetCode::OutOfRangeStartIndex => 12,
        RetCode::OutOfRangeEndIndex => 13,
    }
}

fn json_f64_array(data: &[f64]) -> String {
    let mut s = String::with_capacity(data.len() * 8 + 2);
    s.push('[');
    for (i, &v) in data.iter().enumerate() {
        if i > 0 { s.push(','); }
        match serde_json::Number::from_f64(v) {
            Some(n) => s.push_str(&n.to_string()),
            None => s.push_str(
                if v.is_nan() { if v.is_sign_negative() { "-nan" } else { "nan" } }
                else if v < 0.0 { "-inf" } else { "inf" }),
        }
    }
    s.push(']');
    s
}

fn json_i32_array(data: &[i32]) -> String {
    let mut s = String::with_capacity(data.len() * 4 + 2);
    s.push('[');
    for (i, &v) in data.iter().enumerate() {
        if i > 0 { s.push(','); }
        s.push_str(&v.to_string());
    }
    s.push(']');
    s
}

fn func_unst_id_from_int(id: usize) -> Option<FuncUnstId> {
    match id {
        0 => Some(FuncUnstId::Adx),
        1 => Some(FuncUnstId::Adxr),
        2 => Some(FuncUnstId::Atr),
        3 => Some(FuncUnstId::Cmo),
        4 => Some(FuncUnstId::Dx),
        5 => Some(FuncUnstId::Ema),
        6 => Some(FuncUnstId::HtDcPeriod),
        7 => Some(FuncUnstId::HtDcPhase),
        8 => Some(FuncUnstId::HtPhasor),
        9 => Some(FuncUnstId::HtSine),
        10 => Some(FuncUnstId::HtTrendline),
        11 => Some(FuncUnstId::HtTrendMode),
        12 => Some(FuncUnstId::Imi),
        13 => Some(FuncUnstId::Kama),
        14 => Some(FuncUnstId::Mama),
        15 => Some(FuncUnstId::Mfi),
        16 => Some(FuncUnstId::MinusDI),
        17 => Some(FuncUnstId::MinusDM),
        18 => Some(FuncUnstId::Natr),
        19 => Some(FuncUnstId::PlusDI),
        20 => Some(FuncUnstId::PlusDM),
        21 => Some(FuncUnstId::Rsi),
        22 => Some(FuncUnstId::StochRsi),
        23 => Some(FuncUnstId::T3),
        _ => None,
    }
}

fn handle_request(core: &mut Core, ref_data: &mut RefData, line: &str) -> String {
    let req: Value = match serde_json::from_str(line) {
        Ok(v) => v,
        Err(e) => return format!("{{\"error\":\"Parse error: {}\"}}", e),
    };
    let method = match req["method"].as_str() {
        Some(m) => m,
        None => return "{\"error\":\"Missing method field\"}".to_string(),
    };
    let params = &req["params"];

    dispatch(core, ref_data, method, params)
}

fn dispatch(core: &mut Core, ref_data: &mut RefData, method: &str, params: &Value) -> String {
    match method {
        "load_data" => {
            let open = parse_f64_array(&params["open"]);
            ref_data.n = open.len().min(MAX_ARRAY_SIZE);
            ref_data.open[..ref_data.n].copy_from_slice(&open[..ref_data.n]);
            let high = parse_f64_array(&params["high"]);
            ref_data.high[..ref_data.n].copy_from_slice(&high[..ref_data.n]);
            let low = parse_f64_array(&params["low"]);
            ref_data.low[..ref_data.n].copy_from_slice(&low[..ref_data.n]);
            let close = parse_f64_array(&params["close"]);
            ref_data.close[..ref_data.n].copy_from_slice(&close[..ref_data.n]);
            let volume = parse_f64_array(&params["volume"]);
            ref_data.volume[..ref_data.n].copy_from_slice(&volume[..ref_data.n]);
            let oi = parse_f64_array(&params["openInterest"]);
            ref_data.oi[..ref_data.n].copy_from_slice(&oi[..ref_data.n]);
            format!("{{\"status\":\"ok\",\"n\":{}}}", ref_data.n)
        }
        "TA_ACCBANDS" => {
            let startIdx = params["startIdx"].as_u64().unwrap_or(0) as usize;
            let endIdx = params["endIdx"].as_u64().unwrap_or(0) as usize;
            let use_preloaded = params["use_preloaded"].as_i64().unwrap_or(0);
            let bench_iters = std::cmp::max(1, params["iters"].as_i64().unwrap_or(1)) as u64;
            let mut _json_inHigh: Vec<f64> = Vec::new();
            let mut _json_inLow: Vec<f64> = Vec::new();
            let mut _json_inClose: Vec<f64> = Vec::new();
            let inHigh: &[f64];
            let inLow: &[f64];
            let inClose: &[f64];
            if use_preloaded != 0 && ref_data.n > 0 {
                inHigh = &ref_data.high[..ref_data.n];
                inLow = &ref_data.low[..ref_data.n];
                inClose = &ref_data.close[..ref_data.n];
            } else {
                _json_inHigh = parse_f64_array(&params["inHigh"]);
                inHigh = &_json_inHigh;
                _json_inLow = parse_f64_array(&params["inLow"]);
                inLow = &_json_inLow;
                _json_inClose = parse_f64_array(&params["inClose"]);
                inClose = &_json_inClose;
            }
            let optInTimePeriod = params["optInTimePeriod"].as_i64().unwrap_or(20) as i32;
            let out_size = if endIdx >= startIdx { endIdx - startIdx + 1 } else { 0 };
            let mut outBuf0: Vec<f64> = vec![0.0f64; out_size];
            let mut outBuf1: Vec<f64> = vec![0.0f64; out_size];
            let mut outBuf2: Vec<f64> = vec![0.0f64; out_size];
            let mut outBegIdx: usize = 0;
            let mut outNBElement: usize = 0;
            let mut rc = RetCode::Success;
            let start_time = Instant::now();
            for _bi in 0..bench_iters {
            rc = core.accbands(
                startIdx, endIdx,
                &inHigh,
                &inLow,
                &inClose,
                optInTimePeriod,
                &mut outBegIdx, &mut outNBElement, &mut outBuf0, &mut outBuf1, &mut outBuf2,
            );
            }
            let elapsed_ns = start_time.elapsed().as_nanos() as u64 / bench_iters as u64;
            let start_time_ung = Instant::now();
            for _biu in 0..bench_iters {
            rc = core.accbands_unguarded(
                startIdx, endIdx,
                &inHigh,
                &inLow,
                &inClose,
                optInTimePeriod,
                &mut outBegIdx, &mut outNBElement, &mut outBuf0, &mut outBuf1, &mut outBuf2,
            );
            }
            let elapsed_ns_ung = start_time_ung.elapsed().as_nanos() as u64 / bench_iters as u64;
            let lookback = core.accbands_lookback(optInTimePeriod);
            let mut resp = format!("{{\"retCode\":{},\"outBegIdx\":{},\"outNBElement\":{},\"lookback\":{},\"timing_ns\":{},\"timing_ns_unguarded\":{}", retcode_to_int(rc), outBegIdx, outNBElement, lookback, elapsed_ns, elapsed_ns_ung);
            resp.push_str(",\"outReal\":"); resp.push_str(&json_f64_array(&outBuf0[..outNBElement]));
            resp.push_str(",\"outReal1\":"); resp.push_str(&json_f64_array(&outBuf1[..outNBElement]));
            resp.push_str(",\"outReal2\":"); resp.push_str(&json_f64_array(&outBuf2[..outNBElement]));
            resp.push('}');
            resp
        }
        "TA_ACOS" => {
            let startIdx = params["startIdx"].as_u64().unwrap_or(0) as usize;
            let endIdx = params["endIdx"].as_u64().unwrap_or(0) as usize;
            let use_preloaded = params["use_preloaded"].as_i64().unwrap_or(0);
            let bench_iters = std::cmp::max(1, params["iters"].as_i64().unwrap_or(1)) as u64;
            let mut _json_inReal: Vec<f64> = Vec::new();
            let inReal: &[f64];
            if use_preloaded != 0 && ref_data.n > 0 {
                inReal = &ref_data.close[..ref_data.n];
            } else {
                _json_inReal = parse_f64_array(&params["inReal"]);
                inReal = &_json_inReal;
            }
            let out_size = if endIdx >= startIdx { endIdx - startIdx + 1 } else { 0 };
            let mut outBuf0: Vec<f64> = vec![0.0f64; out_size];
            let mut outBegIdx: usize = 0;
            let mut outNBElement: usize = 0;
            let mut rc = RetCode::Success;
            let start_time = Instant::now();
            for _bi in 0..bench_iters {
            rc = core.acos(
                startIdx, endIdx,
                &inReal,
                &mut outBegIdx, &mut outNBElement, &mut outBuf0,
            );
            }
            let elapsed_ns = start_time.elapsed().as_nanos() as u64 / bench_iters as u64;
            let start_time_ung = Instant::now();
            for _biu in 0..bench_iters {
            rc = core.acos_unguarded(
                startIdx, endIdx,
                &inReal,
                &mut outBegIdx, &mut outNBElement, &mut outBuf0,
            );
            }
            let elapsed_ns_ung = start_time_ung.elapsed().as_nanos() as u64 / bench_iters as u64;
            let lookback = core.acos_lookback();
            let mut resp = format!("{{\"retCode\":{},\"outBegIdx\":{},\"outNBElement\":{},\"lookback\":{},\"timing_ns\":{},\"timing_ns_unguarded\":{}", retcode_to_int(rc), outBegIdx, outNBElement, lookback, elapsed_ns, elapsed_ns_ung);
            resp.push_str(",\"outReal\":"); resp.push_str(&json_f64_array(&outBuf0[..outNBElement]));
            resp.push('}');
            resp
        }
        "TA_AD" => {
            let startIdx = params["startIdx"].as_u64().unwrap_or(0) as usize;
            let endIdx = params["endIdx"].as_u64().unwrap_or(0) as usize;
            let use_preloaded = params["use_preloaded"].as_i64().unwrap_or(0);
            let bench_iters = std::cmp::max(1, params["iters"].as_i64().unwrap_or(1)) as u64;
            let mut _json_inHigh: Vec<f64> = Vec::new();
            let mut _json_inLow: Vec<f64> = Vec::new();
            let mut _json_inClose: Vec<f64> = Vec::new();
            let mut _json_inVolume: Vec<f64> = Vec::new();
            let inHigh: &[f64];
            let inLow: &[f64];
            let inClose: &[f64];
            let inVolume: &[f64];
            if use_preloaded != 0 && ref_data.n > 0 {
                inHigh = &ref_data.high[..ref_data.n];
                inLow = &ref_data.low[..ref_data.n];
                inClose = &ref_data.close[..ref_data.n];
                inVolume = &ref_data.volume[..ref_data.n];
            } else {
                _json_inHigh = parse_f64_array(&params["inHigh"]);
                inHigh = &_json_inHigh;
                _json_inLow = parse_f64_array(&params["inLow"]);
                inLow = &_json_inLow;
                _json_inClose = parse_f64_array(&params["inClose"]);
                inClose = &_json_inClose;
                _json_inVolume = parse_f64_array(&params["inVolume"]);
                inVolume = &_json_inVolume;
            }
            let out_size = if endIdx >= startIdx { endIdx - startIdx + 1 } else { 0 };
            let mut outBuf0: Vec<f64> = vec![0.0f64; out_size];
            let mut outBegIdx: usize = 0;
            let mut outNBElement: usize = 0;
            let mut rc = RetCode::Success;
            let start_time = Instant::now();
            for _bi in 0..bench_iters {
            rc = core.ad(
                startIdx, endIdx,
                &inHigh,
                &inLow,
                &inClose,
                &inVolume,
                &mut outBegIdx, &mut outNBElement, &mut outBuf0,
            );
            }
            let elapsed_ns = start_time.elapsed().as_nanos() as u64 / bench_iters as u64;
            let start_time_ung = Instant::now();
            for _biu in 0..bench_iters {
            rc = core.ad_unguarded(
                startIdx, endIdx,
                &inHigh,
                &inLow,
                &inClose,
                &inVolume,
                &mut outBegIdx, &mut outNBElement, &mut outBuf0,
            );
            }
            let elapsed_ns_ung = start_time_ung.elapsed().as_nanos() as u64 / bench_iters as u64;
            let lookback = core.ad_lookback();
            let mut resp = format!("{{\"retCode\":{},\"outBegIdx\":{},\"outNBElement\":{},\"lookback\":{},\"timing_ns\":{},\"timing_ns_unguarded\":{}", retcode_to_int(rc), outBegIdx, outNBElement, lookback, elapsed_ns, elapsed_ns_ung);
            resp.push_str(",\"outReal\":"); resp.push_str(&json_f64_array(&outBuf0[..outNBElement]));
            resp.push('}');
            resp
        }
        "TA_ADD" => {
            let startIdx = params["startIdx"].as_u64().unwrap_or(0) as usize;
            let endIdx = params["endIdx"].as_u64().unwrap_or(0) as usize;
            let use_preloaded = params["use_preloaded"].as_i64().unwrap_or(0);
            let bench_iters = std::cmp::max(1, params["iters"].as_i64().unwrap_or(1)) as u64;
            let mut _json_inReal0: Vec<f64> = Vec::new();
            let mut _json_inReal1: Vec<f64> = Vec::new();
            let inReal0: &[f64];
            let inReal1: &[f64];
            if use_preloaded != 0 && ref_data.n > 0 {
                inReal0 = &ref_data.close[..ref_data.n];
                inReal1 = &ref_data.high[..ref_data.n];
            } else {
                _json_inReal0 = parse_f64_array(&params["inReal0"]);
                inReal0 = &_json_inReal0;
                _json_inReal1 = parse_f64_array(&params["inReal1"]);
                inReal1 = &_json_inReal1;
            }
            let out_size = if endIdx >= startIdx { endIdx - startIdx + 1 } else { 0 };
            let mut outBuf0: Vec<f64> = vec![0.0f64; out_size];
            let mut outBegIdx: usize = 0;
            let mut outNBElement: usize = 0;
            let mut rc = RetCode::Success;
            let start_time = Instant::now();
            for _bi in 0..bench_iters {
            rc = core.add(
                startIdx, endIdx,
                &inReal0,
                &inReal1,
                &mut outBegIdx, &mut outNBElement, &mut outBuf0,
            );
            }
            let elapsed_ns = start_time.elapsed().as_nanos() as u64 / bench_iters as u64;
            let start_time_ung = Instant::now();
            for _biu in 0..bench_iters {
            rc = core.add_unguarded(
                startIdx, endIdx,
                &inReal0,
                &inReal1,
                &mut outBegIdx, &mut outNBElement, &mut outBuf0,
            );
            }
            let elapsed_ns_ung = start_time_ung.elapsed().as_nanos() as u64 / bench_iters as u64;
            let lookback = core.add_lookback();
            let mut resp = format!("{{\"retCode\":{},\"outBegIdx\":{},\"outNBElement\":{},\"lookback\":{},\"timing_ns\":{},\"timing_ns_unguarded\":{}", retcode_to_int(rc), outBegIdx, outNBElement, lookback, elapsed_ns, elapsed_ns_ung);
            resp.push_str(",\"outReal\":"); resp.push_str(&json_f64_array(&outBuf0[..outNBElement]));
            resp.push('}');
            resp
        }
        "TA_ADOSC" => {
            let startIdx = params["startIdx"].as_u64().unwrap_or(0) as usize;
            let endIdx = params["endIdx"].as_u64().unwrap_or(0) as usize;
            let use_preloaded = params["use_preloaded"].as_i64().unwrap_or(0);
            let bench_iters = std::cmp::max(1, params["iters"].as_i64().unwrap_or(1)) as u64;
            let mut _json_inHigh: Vec<f64> = Vec::new();
            let mut _json_inLow: Vec<f64> = Vec::new();
            let mut _json_inClose: Vec<f64> = Vec::new();
            let mut _json_inVolume: Vec<f64> = Vec::new();
            let inHigh: &[f64];
            let inLow: &[f64];
            let inClose: &[f64];
            let inVolume: &[f64];
            if use_preloaded != 0 && ref_data.n > 0 {
                inHigh = &ref_data.high[..ref_data.n];
                inLow = &ref_data.low[..ref_data.n];
                inClose = &ref_data.close[..ref_data.n];
                inVolume = &ref_data.volume[..ref_data.n];
            } else {
                _json_inHigh = parse_f64_array(&params["inHigh"]);
                inHigh = &_json_inHigh;
                _json_inLow = parse_f64_array(&params["inLow"]);
                inLow = &_json_inLow;
                _json_inClose = parse_f64_array(&params["inClose"]);
                inClose = &_json_inClose;
                _json_inVolume = parse_f64_array(&params["inVolume"]);
                inVolume = &_json_inVolume;
            }
            let optInFastPeriod = params["optInFastPeriod"].as_i64().unwrap_or(3) as i32;
            let optInSlowPeriod = params["optInSlowPeriod"].as_i64().unwrap_or(10) as i32;
            let out_size = if endIdx >= startIdx { endIdx - startIdx + 1 } else { 0 };
            let mut outBuf0: Vec<f64> = vec![0.0f64; out_size];
            let mut outBegIdx: usize = 0;
            let mut outNBElement: usize = 0;
            let mut rc = RetCode::Success;
            let start_time = Instant::now();
            for _bi in 0..bench_iters {
            rc = core.adosc(
                startIdx, endIdx,
                &inHigh,
                &inLow,
                &inClose,
                &inVolume,
                optInFastPeriod,
                optInSlowPeriod,
                &mut outBegIdx, &mut outNBElement, &mut outBuf0,
            );
            }
            let elapsed_ns = start_time.elapsed().as_nanos() as u64 / bench_iters as u64;
            let start_time_ung = Instant::now();
            for _biu in 0..bench_iters {
            rc = core.adosc_unguarded(
                startIdx, endIdx,
                &inHigh,
                &inLow,
                &inClose,
                &inVolume,
                optInFastPeriod,
                optInSlowPeriod,
                &mut outBegIdx, &mut outNBElement, &mut outBuf0,
            );
            }
            let elapsed_ns_ung = start_time_ung.elapsed().as_nanos() as u64 / bench_iters as u64;
            let lookback = core.adosc_lookback(optInFastPeriod, optInSlowPeriod);
            let mut resp = format!("{{\"retCode\":{},\"outBegIdx\":{},\"outNBElement\":{},\"lookback\":{},\"timing_ns\":{},\"timing_ns_unguarded\":{}", retcode_to_int(rc), outBegIdx, outNBElement, lookback, elapsed_ns, elapsed_ns_ung);
            resp.push_str(",\"outReal\":"); resp.push_str(&json_f64_array(&outBuf0[..outNBElement]));
            resp.push('}');
            resp
        }
        "TA_ADX" => {
            let startIdx = params["startIdx"].as_u64().unwrap_or(0) as usize;
            let endIdx = params["endIdx"].as_u64().unwrap_or(0) as usize;
            let use_preloaded = params["use_preloaded"].as_i64().unwrap_or(0);
            let bench_iters = std::cmp::max(1, params["iters"].as_i64().unwrap_or(1)) as u64;
            let mut _json_inHigh: Vec<f64> = Vec::new();
            let mut _json_inLow: Vec<f64> = Vec::new();
            let mut _json_inClose: Vec<f64> = Vec::new();
            let inHigh: &[f64];
            let inLow: &[f64];
            let inClose: &[f64];
            if use_preloaded != 0 && ref_data.n > 0 {
                inHigh = &ref_data.high[..ref_data.n];
                inLow = &ref_data.low[..ref_data.n];
                inClose = &ref_data.close[..ref_data.n];
            } else {
                _json_inHigh = parse_f64_array(&params["inHigh"]);
                inHigh = &_json_inHigh;
                _json_inLow = parse_f64_array(&params["inLow"]);
                inLow = &_json_inLow;
                _json_inClose = parse_f64_array(&params["inClose"]);
                inClose = &_json_inClose;
            }
            let optInTimePeriod = params["optInTimePeriod"].as_i64().unwrap_or(14) as i32;
            if let Some(period) = params["unstablePeriod"].as_i64() {
                core.unstable_period[0] = period as i32;
            }
            let out_size = if endIdx >= startIdx { endIdx - startIdx + 1 } else { 0 };
            let mut outBuf0: Vec<f64> = vec![0.0f64; out_size];
            let mut outBegIdx: usize = 0;
            let mut outNBElement: usize = 0;
            let mut rc = RetCode::Success;
            let start_time = Instant::now();
            for _bi in 0..bench_iters {
            rc = core.adx(
                startIdx, endIdx,
                &inHigh,
                &inLow,
                &inClose,
                optInTimePeriod,
                &mut outBegIdx, &mut outNBElement, &mut outBuf0,
            );
            }
            let elapsed_ns = start_time.elapsed().as_nanos() as u64 / bench_iters as u64;
            let start_time_ung = Instant::now();
            for _biu in 0..bench_iters {
            rc = core.adx_unguarded(
                startIdx, endIdx,
                &inHigh,
                &inLow,
                &inClose,
                optInTimePeriod,
                &mut outBegIdx, &mut outNBElement, &mut outBuf0,
            );
            }
            let elapsed_ns_ung = start_time_ung.elapsed().as_nanos() as u64 / bench_iters as u64;
            let lookback = core.adx_lookback(optInTimePeriod);
            let mut resp = format!("{{\"retCode\":{},\"outBegIdx\":{},\"outNBElement\":{},\"lookback\":{},\"timing_ns\":{},\"timing_ns_unguarded\":{}", retcode_to_int(rc), outBegIdx, outNBElement, lookback, elapsed_ns, elapsed_ns_ung);
            resp.push_str(",\"outReal\":"); resp.push_str(&json_f64_array(&outBuf0[..outNBElement]));
            resp.push('}');
            resp
        }
        "TA_ADXR" => {
            let startIdx = params["startIdx"].as_u64().unwrap_or(0) as usize;
            let endIdx = params["endIdx"].as_u64().unwrap_or(0) as usize;
            let use_preloaded = params["use_preloaded"].as_i64().unwrap_or(0);
            let bench_iters = std::cmp::max(1, params["iters"].as_i64().unwrap_or(1)) as u64;
            let mut _json_inHigh: Vec<f64> = Vec::new();
            let mut _json_inLow: Vec<f64> = Vec::new();
            let mut _json_inClose: Vec<f64> = Vec::new();
            let inHigh: &[f64];
            let inLow: &[f64];
            let inClose: &[f64];
            if use_preloaded != 0 && ref_data.n > 0 {
                inHigh = &ref_data.high[..ref_data.n];
                inLow = &ref_data.low[..ref_data.n];
                inClose = &ref_data.close[..ref_data.n];
            } else {
                _json_inHigh = parse_f64_array(&params["inHigh"]);
                inHigh = &_json_inHigh;
                _json_inLow = parse_f64_array(&params["inLow"]);
                inLow = &_json_inLow;
                _json_inClose = parse_f64_array(&params["inClose"]);
                inClose = &_json_inClose;
            }
            let optInTimePeriod = params["optInTimePeriod"].as_i64().unwrap_or(14) as i32;
            if let Some(period) = params["unstablePeriod"].as_i64() {
                core.unstable_period[1] = period as i32;
            }
            let out_size = if endIdx >= startIdx { endIdx - startIdx + 1 } else { 0 };
            let mut outBuf0: Vec<f64> = vec![0.0f64; out_size];
            let mut outBegIdx: usize = 0;
            let mut outNBElement: usize = 0;
            let mut rc = RetCode::Success;
            let start_time = Instant::now();
            for _bi in 0..bench_iters {
            rc = core.adxr(
                startIdx, endIdx,
                &inHigh,
                &inLow,
                &inClose,
                optInTimePeriod,
                &mut outBegIdx, &mut outNBElement, &mut outBuf0,
            );
            }
            let elapsed_ns = start_time.elapsed().as_nanos() as u64 / bench_iters as u64;
            let start_time_ung = Instant::now();
            for _biu in 0..bench_iters {
            rc = core.adxr_unguarded(
                startIdx, endIdx,
                &inHigh,
                &inLow,
                &inClose,
                optInTimePeriod,
                &mut outBegIdx, &mut outNBElement, &mut outBuf0,
            );
            }
            let elapsed_ns_ung = start_time_ung.elapsed().as_nanos() as u64 / bench_iters as u64;
            let lookback = core.adxr_lookback(optInTimePeriod);
            let mut resp = format!("{{\"retCode\":{},\"outBegIdx\":{},\"outNBElement\":{},\"lookback\":{},\"timing_ns\":{},\"timing_ns_unguarded\":{}", retcode_to_int(rc), outBegIdx, outNBElement, lookback, elapsed_ns, elapsed_ns_ung);
            resp.push_str(",\"outReal\":"); resp.push_str(&json_f64_array(&outBuf0[..outNBElement]));
            resp.push('}');
            resp
        }
        "TA_APO" => {
            let startIdx = params["startIdx"].as_u64().unwrap_or(0) as usize;
            let endIdx = params["endIdx"].as_u64().unwrap_or(0) as usize;
            let use_preloaded = params["use_preloaded"].as_i64().unwrap_or(0);
            let bench_iters = std::cmp::max(1, params["iters"].as_i64().unwrap_or(1)) as u64;
            let mut _json_inReal: Vec<f64> = Vec::new();
            let inReal: &[f64];
            if use_preloaded != 0 && ref_data.n > 0 {
                inReal = &ref_data.close[..ref_data.n];
            } else {
                _json_inReal = parse_f64_array(&params["inReal"]);
                inReal = &_json_inReal;
            }
            let optInFastPeriod = params["optInFastPeriod"].as_i64().unwrap_or(12) as i32;
            let optInSlowPeriod = params["optInSlowPeriod"].as_i64().unwrap_or(26) as i32;
            let optInMAType = params["optInMAType"].as_i64().unwrap_or(0) as i32;
            let out_size = if endIdx >= startIdx { endIdx - startIdx + 1 } else { 0 };
            let mut outBuf0: Vec<f64> = vec![0.0f64; out_size];
            let mut outBegIdx: usize = 0;
            let mut outNBElement: usize = 0;
            let mut rc = RetCode::Success;
            let start_time = Instant::now();
            for _bi in 0..bench_iters {
            rc = core.apo(
                startIdx, endIdx,
                &inReal,
                optInFastPeriod,
                optInSlowPeriod,
                optInMAType,
                &mut outBegIdx, &mut outNBElement, &mut outBuf0,
            );
            }
            let elapsed_ns = start_time.elapsed().as_nanos() as u64 / bench_iters as u64;
            let start_time_ung = Instant::now();
            for _biu in 0..bench_iters {
            rc = core.apo_unguarded(
                startIdx, endIdx,
                &inReal,
                optInFastPeriod,
                optInSlowPeriod,
                optInMAType,
                &mut outBegIdx, &mut outNBElement, &mut outBuf0,
            );
            }
            let elapsed_ns_ung = start_time_ung.elapsed().as_nanos() as u64 / bench_iters as u64;
            let lookback = core.apo_lookback(optInFastPeriod, optInSlowPeriod, optInMAType);
            let mut resp = format!("{{\"retCode\":{},\"outBegIdx\":{},\"outNBElement\":{},\"lookback\":{},\"timing_ns\":{},\"timing_ns_unguarded\":{}", retcode_to_int(rc), outBegIdx, outNBElement, lookback, elapsed_ns, elapsed_ns_ung);
            resp.push_str(",\"outReal\":"); resp.push_str(&json_f64_array(&outBuf0[..outNBElement]));
            resp.push('}');
            resp
        }
        "TA_AROON" => {
            let startIdx = params["startIdx"].as_u64().unwrap_or(0) as usize;
            let endIdx = params["endIdx"].as_u64().unwrap_or(0) as usize;
            let use_preloaded = params["use_preloaded"].as_i64().unwrap_or(0);
            let bench_iters = std::cmp::max(1, params["iters"].as_i64().unwrap_or(1)) as u64;
            let mut _json_inHigh: Vec<f64> = Vec::new();
            let mut _json_inLow: Vec<f64> = Vec::new();
            let inHigh: &[f64];
            let inLow: &[f64];
            if use_preloaded != 0 && ref_data.n > 0 {
                inHigh = &ref_data.high[..ref_data.n];
                inLow = &ref_data.low[..ref_data.n];
            } else {
                _json_inHigh = parse_f64_array(&params["inHigh"]);
                inHigh = &_json_inHigh;
                _json_inLow = parse_f64_array(&params["inLow"]);
                inLow = &_json_inLow;
            }
            let optInTimePeriod = params["optInTimePeriod"].as_i64().unwrap_or(14) as i32;
            let out_size = if endIdx >= startIdx { endIdx - startIdx + 1 } else { 0 };
            let mut outBuf0: Vec<f64> = vec![0.0f64; out_size];
            let mut outBuf1: Vec<f64> = vec![0.0f64; out_size];
            let mut outBegIdx: usize = 0;
            let mut outNBElement: usize = 0;
            let mut rc = RetCode::Success;
            let start_time = Instant::now();
            for _bi in 0..bench_iters {
            rc = core.aroon(
                startIdx, endIdx,
                &inHigh,
                &inLow,
                optInTimePeriod,
                &mut outBegIdx, &mut outNBElement, &mut outBuf0, &mut outBuf1,
            );
            }
            let elapsed_ns = start_time.elapsed().as_nanos() as u64 / bench_iters as u64;
            let start_time_ung = Instant::now();
            for _biu in 0..bench_iters {
            rc = core.aroon_unguarded(
                startIdx, endIdx,
                &inHigh,
                &inLow,
                optInTimePeriod,
                &mut outBegIdx, &mut outNBElement, &mut outBuf0, &mut outBuf1,
            );
            }
            let elapsed_ns_ung = start_time_ung.elapsed().as_nanos() as u64 / bench_iters as u64;
            let lookback = core.aroon_lookback(optInTimePeriod);
            let mut resp = format!("{{\"retCode\":{},\"outBegIdx\":{},\"outNBElement\":{},\"lookback\":{},\"timing_ns\":{},\"timing_ns_unguarded\":{}", retcode_to_int(rc), outBegIdx, outNBElement, lookback, elapsed_ns, elapsed_ns_ung);
            resp.push_str(",\"outReal\":"); resp.push_str(&json_f64_array(&outBuf0[..outNBElement]));
            resp.push_str(",\"outReal1\":"); resp.push_str(&json_f64_array(&outBuf1[..outNBElement]));
            resp.push('}');
            resp
        }
        "TA_AROONOSC" => {
            let startIdx = params["startIdx"].as_u64().unwrap_or(0) as usize;
            let endIdx = params["endIdx"].as_u64().unwrap_or(0) as usize;
            let use_preloaded = params["use_preloaded"].as_i64().unwrap_or(0);
            let bench_iters = std::cmp::max(1, params["iters"].as_i64().unwrap_or(1)) as u64;
            let mut _json_inHigh: Vec<f64> = Vec::new();
            let mut _json_inLow: Vec<f64> = Vec::new();
            let inHigh: &[f64];
            let inLow: &[f64];
            if use_preloaded != 0 && ref_data.n > 0 {
                inHigh = &ref_data.high[..ref_data.n];
                inLow = &ref_data.low[..ref_data.n];
            } else {
                _json_inHigh = parse_f64_array(&params["inHigh"]);
                inHigh = &_json_inHigh;
                _json_inLow = parse_f64_array(&params["inLow"]);
                inLow = &_json_inLow;
            }
            let optInTimePeriod = params["optInTimePeriod"].as_i64().unwrap_or(14) as i32;
            let out_size = if endIdx >= startIdx { endIdx - startIdx + 1 } else { 0 };
            let mut outBuf0: Vec<f64> = vec![0.0f64; out_size];
            let mut outBegIdx: usize = 0;
            let mut outNBElement: usize = 0;
            let mut rc = RetCode::Success;
            let start_time = Instant::now();
            for _bi in 0..bench_iters {
            rc = core.aroonosc(
                startIdx, endIdx,
                &inHigh,
                &inLow,
                optInTimePeriod,
                &mut outBegIdx, &mut outNBElement, &mut outBuf0,
            );
            }
            let elapsed_ns = start_time.elapsed().as_nanos() as u64 / bench_iters as u64;
            let start_time_ung = Instant::now();
            for _biu in 0..bench_iters {
            rc = core.aroonosc_unguarded(
                startIdx, endIdx,
                &inHigh,
                &inLow,
                optInTimePeriod,
                &mut outBegIdx, &mut outNBElement, &mut outBuf0,
            );
            }
            let elapsed_ns_ung = start_time_ung.elapsed().as_nanos() as u64 / bench_iters as u64;
            let lookback = core.aroonosc_lookback(optInTimePeriod);
            let mut resp = format!("{{\"retCode\":{},\"outBegIdx\":{},\"outNBElement\":{},\"lookback\":{},\"timing_ns\":{},\"timing_ns_unguarded\":{}", retcode_to_int(rc), outBegIdx, outNBElement, lookback, elapsed_ns, elapsed_ns_ung);
            resp.push_str(",\"outReal\":"); resp.push_str(&json_f64_array(&outBuf0[..outNBElement]));
            resp.push('}');
            resp
        }
        "TA_ASIN" => {
            let startIdx = params["startIdx"].as_u64().unwrap_or(0) as usize;
            let endIdx = params["endIdx"].as_u64().unwrap_or(0) as usize;
            let use_preloaded = params["use_preloaded"].as_i64().unwrap_or(0);
            let bench_iters = std::cmp::max(1, params["iters"].as_i64().unwrap_or(1)) as u64;
            let mut _json_inReal: Vec<f64> = Vec::new();
            let inReal: &[f64];
            if use_preloaded != 0 && ref_data.n > 0 {
                inReal = &ref_data.close[..ref_data.n];
            } else {
                _json_inReal = parse_f64_array(&params["inReal"]);
                inReal = &_json_inReal;
            }
            let out_size = if endIdx >= startIdx { endIdx - startIdx + 1 } else { 0 };
            let mut outBuf0: Vec<f64> = vec![0.0f64; out_size];
            let mut outBegIdx: usize = 0;
            let mut outNBElement: usize = 0;
            let mut rc = RetCode::Success;
            let start_time = Instant::now();
            for _bi in 0..bench_iters {
            rc = core.asin(
                startIdx, endIdx,
                &inReal,
                &mut outBegIdx, &mut outNBElement, &mut outBuf0,
            );
            }
            let elapsed_ns = start_time.elapsed().as_nanos() as u64 / bench_iters as u64;
            let start_time_ung = Instant::now();
            for _biu in 0..bench_iters {
            rc = core.asin_unguarded(
                startIdx, endIdx,
                &inReal,
                &mut outBegIdx, &mut outNBElement, &mut outBuf0,
            );
            }
            let elapsed_ns_ung = start_time_ung.elapsed().as_nanos() as u64 / bench_iters as u64;
            let lookback = core.asin_lookback();
            let mut resp = format!("{{\"retCode\":{},\"outBegIdx\":{},\"outNBElement\":{},\"lookback\":{},\"timing_ns\":{},\"timing_ns_unguarded\":{}", retcode_to_int(rc), outBegIdx, outNBElement, lookback, elapsed_ns, elapsed_ns_ung);
            resp.push_str(",\"outReal\":"); resp.push_str(&json_f64_array(&outBuf0[..outNBElement]));
            resp.push('}');
            resp
        }
        "TA_ATAN" => {
            let startIdx = params["startIdx"].as_u64().unwrap_or(0) as usize;
            let endIdx = params["endIdx"].as_u64().unwrap_or(0) as usize;
            let use_preloaded = params["use_preloaded"].as_i64().unwrap_or(0);
            let bench_iters = std::cmp::max(1, params["iters"].as_i64().unwrap_or(1)) as u64;
            let mut _json_inReal: Vec<f64> = Vec::new();
            let inReal: &[f64];
            if use_preloaded != 0 && ref_data.n > 0 {
                inReal = &ref_data.close[..ref_data.n];
            } else {
                _json_inReal = parse_f64_array(&params["inReal"]);
                inReal = &_json_inReal;
            }
            let out_size = if endIdx >= startIdx { endIdx - startIdx + 1 } else { 0 };
            let mut outBuf0: Vec<f64> = vec![0.0f64; out_size];
            let mut outBegIdx: usize = 0;
            let mut outNBElement: usize = 0;
            let mut rc = RetCode::Success;
            let start_time = Instant::now();
            for _bi in 0..bench_iters {
            rc = core.atan(
                startIdx, endIdx,
                &inReal,
                &mut outBegIdx, &mut outNBElement, &mut outBuf0,
            );
            }
            let elapsed_ns = start_time.elapsed().as_nanos() as u64 / bench_iters as u64;
            let start_time_ung = Instant::now();
            for _biu in 0..bench_iters {
            rc = core.atan_unguarded(
                startIdx, endIdx,
                &inReal,
                &mut outBegIdx, &mut outNBElement, &mut outBuf0,
            );
            }
            let elapsed_ns_ung = start_time_ung.elapsed().as_nanos() as u64 / bench_iters as u64;
            let lookback = core.atan_lookback();
            let mut resp = format!("{{\"retCode\":{},\"outBegIdx\":{},\"outNBElement\":{},\"lookback\":{},\"timing_ns\":{},\"timing_ns_unguarded\":{}", retcode_to_int(rc), outBegIdx, outNBElement, lookback, elapsed_ns, elapsed_ns_ung);
            resp.push_str(",\"outReal\":"); resp.push_str(&json_f64_array(&outBuf0[..outNBElement]));
            resp.push('}');
            resp
        }
        "TA_ATR" => {
            let startIdx = params["startIdx"].as_u64().unwrap_or(0) as usize;
            let endIdx = params["endIdx"].as_u64().unwrap_or(0) as usize;
            let use_preloaded = params["use_preloaded"].as_i64().unwrap_or(0);
            let bench_iters = std::cmp::max(1, params["iters"].as_i64().unwrap_or(1)) as u64;
            let mut _json_inHigh: Vec<f64> = Vec::new();
            let mut _json_inLow: Vec<f64> = Vec::new();
            let mut _json_inClose: Vec<f64> = Vec::new();
            let inHigh: &[f64];
            let inLow: &[f64];
            let inClose: &[f64];
            if use_preloaded != 0 && ref_data.n > 0 {
                inHigh = &ref_data.high[..ref_data.n];
                inLow = &ref_data.low[..ref_data.n];
                inClose = &ref_data.close[..ref_data.n];
            } else {
                _json_inHigh = parse_f64_array(&params["inHigh"]);
                inHigh = &_json_inHigh;
                _json_inLow = parse_f64_array(&params["inLow"]);
                inLow = &_json_inLow;
                _json_inClose = parse_f64_array(&params["inClose"]);
                inClose = &_json_inClose;
            }
            let optInTimePeriod = params["optInTimePeriod"].as_i64().unwrap_or(14) as i32;
            if let Some(period) = params["unstablePeriod"].as_i64() {
                core.unstable_period[2] = period as i32;
            }
            let out_size = if endIdx >= startIdx { endIdx - startIdx + 1 } else { 0 };
            let mut outBuf0: Vec<f64> = vec![0.0f64; out_size];
            let mut outBegIdx: usize = 0;
            let mut outNBElement: usize = 0;
            let mut rc = RetCode::Success;
            let start_time = Instant::now();
            for _bi in 0..bench_iters {
            rc = core.atr(
                startIdx, endIdx,
                &inHigh,
                &inLow,
                &inClose,
                optInTimePeriod,
                &mut outBegIdx, &mut outNBElement, &mut outBuf0,
            );
            }
            let elapsed_ns = start_time.elapsed().as_nanos() as u64 / bench_iters as u64;
            let start_time_ung = Instant::now();
            for _biu in 0..bench_iters {
            rc = core.atr_unguarded(
                startIdx, endIdx,
                &inHigh,
                &inLow,
                &inClose,
                optInTimePeriod,
                &mut outBegIdx, &mut outNBElement, &mut outBuf0,
            );
            }
            let elapsed_ns_ung = start_time_ung.elapsed().as_nanos() as u64 / bench_iters as u64;
            let lookback = core.atr_lookback(optInTimePeriod);
            let mut resp = format!("{{\"retCode\":{},\"outBegIdx\":{},\"outNBElement\":{},\"lookback\":{},\"timing_ns\":{},\"timing_ns_unguarded\":{}", retcode_to_int(rc), outBegIdx, outNBElement, lookback, elapsed_ns, elapsed_ns_ung);
            resp.push_str(",\"outReal\":"); resp.push_str(&json_f64_array(&outBuf0[..outNBElement]));
            resp.push('}');
            resp
        }
        "TA_AVGDEV" => {
            let startIdx = params["startIdx"].as_u64().unwrap_or(0) as usize;
            let endIdx = params["endIdx"].as_u64().unwrap_or(0) as usize;
            let use_preloaded = params["use_preloaded"].as_i64().unwrap_or(0);
            let bench_iters = std::cmp::max(1, params["iters"].as_i64().unwrap_or(1)) as u64;
            let mut _json_inReal: Vec<f64> = Vec::new();
            let inReal: &[f64];
            if use_preloaded != 0 && ref_data.n > 0 {
                inReal = &ref_data.close[..ref_data.n];
            } else {
                _json_inReal = parse_f64_array(&params["inReal"]);
                inReal = &_json_inReal;
            }
            let optInTimePeriod = params["optInTimePeriod"].as_i64().unwrap_or(14) as i32;
            let out_size = if endIdx >= startIdx { endIdx - startIdx + 1 } else { 0 };
            let mut outBuf0: Vec<f64> = vec![0.0f64; out_size];
            let mut outBegIdx: usize = 0;
            let mut outNBElement: usize = 0;
            let mut rc = RetCode::Success;
            let start_time = Instant::now();
            for _bi in 0..bench_iters {
            rc = core.avgdev(
                startIdx, endIdx,
                &inReal,
                optInTimePeriod,
                &mut outBegIdx, &mut outNBElement, &mut outBuf0,
            );
            }
            let elapsed_ns = start_time.elapsed().as_nanos() as u64 / bench_iters as u64;
            let start_time_ung = Instant::now();
            for _biu in 0..bench_iters {
            rc = core.avgdev_unguarded(
                startIdx, endIdx,
                &inReal,
                optInTimePeriod,
                &mut outBegIdx, &mut outNBElement, &mut outBuf0,
            );
            }
            let elapsed_ns_ung = start_time_ung.elapsed().as_nanos() as u64 / bench_iters as u64;
            let lookback = core.avgdev_lookback(optInTimePeriod);
            let mut resp = format!("{{\"retCode\":{},\"outBegIdx\":{},\"outNBElement\":{},\"lookback\":{},\"timing_ns\":{},\"timing_ns_unguarded\":{}", retcode_to_int(rc), outBegIdx, outNBElement, lookback, elapsed_ns, elapsed_ns_ung);
            resp.push_str(",\"outReal\":"); resp.push_str(&json_f64_array(&outBuf0[..outNBElement]));
            resp.push('}');
            resp
        }
        "TA_AVGPRICE" => {
            let startIdx = params["startIdx"].as_u64().unwrap_or(0) as usize;
            let endIdx = params["endIdx"].as_u64().unwrap_or(0) as usize;
            let use_preloaded = params["use_preloaded"].as_i64().unwrap_or(0);
            let bench_iters = std::cmp::max(1, params["iters"].as_i64().unwrap_or(1)) as u64;
            let mut _json_inOpen: Vec<f64> = Vec::new();
            let mut _json_inHigh: Vec<f64> = Vec::new();
            let mut _json_inLow: Vec<f64> = Vec::new();
            let mut _json_inClose: Vec<f64> = Vec::new();
            let inOpen: &[f64];
            let inHigh: &[f64];
            let inLow: &[f64];
            let inClose: &[f64];
            if use_preloaded != 0 && ref_data.n > 0 {
                inOpen = &ref_data.open[..ref_data.n];
                inHigh = &ref_data.high[..ref_data.n];
                inLow = &ref_data.low[..ref_data.n];
                inClose = &ref_data.close[..ref_data.n];
            } else {
                _json_inOpen = parse_f64_array(&params["inOpen"]);
                inOpen = &_json_inOpen;
                _json_inHigh = parse_f64_array(&params["inHigh"]);
                inHigh = &_json_inHigh;
                _json_inLow = parse_f64_array(&params["inLow"]);
                inLow = &_json_inLow;
                _json_inClose = parse_f64_array(&params["inClose"]);
                inClose = &_json_inClose;
            }
            let out_size = if endIdx >= startIdx { endIdx - startIdx + 1 } else { 0 };
            let mut outBuf0: Vec<f64> = vec![0.0f64; out_size];
            let mut outBegIdx: usize = 0;
            let mut outNBElement: usize = 0;
            let mut rc = RetCode::Success;
            let start_time = Instant::now();
            for _bi in 0..bench_iters {
            rc = core.avgprice(
                startIdx, endIdx,
                &inOpen,
                &inHigh,
                &inLow,
                &inClose,
                &mut outBegIdx, &mut outNBElement, &mut outBuf0,
            );
            }
            let elapsed_ns = start_time.elapsed().as_nanos() as u64 / bench_iters as u64;
            let start_time_ung = Instant::now();
            for _biu in 0..bench_iters {
            rc = core.avgprice_unguarded(
                startIdx, endIdx,
                &inOpen,
                &inHigh,
                &inLow,
                &inClose,
                &mut outBegIdx, &mut outNBElement, &mut outBuf0,
            );
            }
            let elapsed_ns_ung = start_time_ung.elapsed().as_nanos() as u64 / bench_iters as u64;
            let lookback = core.avgprice_lookback();
            let mut resp = format!("{{\"retCode\":{},\"outBegIdx\":{},\"outNBElement\":{},\"lookback\":{},\"timing_ns\":{},\"timing_ns_unguarded\":{}", retcode_to_int(rc), outBegIdx, outNBElement, lookback, elapsed_ns, elapsed_ns_ung);
            resp.push_str(",\"outReal\":"); resp.push_str(&json_f64_array(&outBuf0[..outNBElement]));
            resp.push('}');
            resp
        }
        "TA_BBANDS" => {
            let startIdx = params["startIdx"].as_u64().unwrap_or(0) as usize;
            let endIdx = params["endIdx"].as_u64().unwrap_or(0) as usize;
            let use_preloaded = params["use_preloaded"].as_i64().unwrap_or(0);
            let bench_iters = std::cmp::max(1, params["iters"].as_i64().unwrap_or(1)) as u64;
            let mut _json_inReal: Vec<f64> = Vec::new();
            let inReal: &[f64];
            if use_preloaded != 0 && ref_data.n > 0 {
                inReal = &ref_data.close[..ref_data.n];
            } else {
                _json_inReal = parse_f64_array(&params["inReal"]);
                inReal = &_json_inReal;
            }
            let optInTimePeriod = params["optInTimePeriod"].as_i64().unwrap_or(5) as i32;
            let optInNbDevUp = params["optInNbDevUp"].as_f64().unwrap_or(2.0) as f64;
            let optInNbDevDn = params["optInNbDevDn"].as_f64().unwrap_or(2.0) as f64;
            let optInMAType = params["optInMAType"].as_i64().unwrap_or(0) as i32;
            let out_size = if endIdx >= startIdx { endIdx - startIdx + 1 } else { 0 };
            let mut outBuf0: Vec<f64> = vec![0.0f64; out_size];
            let mut outBuf1: Vec<f64> = vec![0.0f64; out_size];
            let mut outBuf2: Vec<f64> = vec![0.0f64; out_size];
            let mut outBegIdx: usize = 0;
            let mut outNBElement: usize = 0;
            let mut rc = RetCode::Success;
            let start_time = Instant::now();
            for _bi in 0..bench_iters {
            rc = core.bbands(
                startIdx, endIdx,
                &inReal,
                optInTimePeriod,
                optInNbDevUp,
                optInNbDevDn,
                optInMAType,
                &mut outBegIdx, &mut outNBElement, &mut outBuf0, &mut outBuf1, &mut outBuf2,
            );
            }
            let elapsed_ns = start_time.elapsed().as_nanos() as u64 / bench_iters as u64;
            let start_time_ung = Instant::now();
            for _biu in 0..bench_iters {
            rc = core.bbands_unguarded(
                startIdx, endIdx,
                &inReal,
                optInTimePeriod,
                optInNbDevUp,
                optInNbDevDn,
                optInMAType,
                &mut outBegIdx, &mut outNBElement, &mut outBuf0, &mut outBuf1, &mut outBuf2,
            );
            }
            let elapsed_ns_ung = start_time_ung.elapsed().as_nanos() as u64 / bench_iters as u64;
            let lookback = core.bbands_lookback(optInTimePeriod, optInNbDevUp, optInNbDevDn, optInMAType);
            let mut resp = format!("{{\"retCode\":{},\"outBegIdx\":{},\"outNBElement\":{},\"lookback\":{},\"timing_ns\":{},\"timing_ns_unguarded\":{}", retcode_to_int(rc), outBegIdx, outNBElement, lookback, elapsed_ns, elapsed_ns_ung);
            resp.push_str(",\"outReal\":"); resp.push_str(&json_f64_array(&outBuf0[..outNBElement]));
            resp.push_str(",\"outReal1\":"); resp.push_str(&json_f64_array(&outBuf1[..outNBElement]));
            resp.push_str(",\"outReal2\":"); resp.push_str(&json_f64_array(&outBuf2[..outNBElement]));
            resp.push('}');
            resp
        }
        "TA_BETA" => {
            let startIdx = params["startIdx"].as_u64().unwrap_or(0) as usize;
            let endIdx = params["endIdx"].as_u64().unwrap_or(0) as usize;
            let use_preloaded = params["use_preloaded"].as_i64().unwrap_or(0);
            let bench_iters = std::cmp::max(1, params["iters"].as_i64().unwrap_or(1)) as u64;
            let mut _json_inReal0: Vec<f64> = Vec::new();
            let mut _json_inReal1: Vec<f64> = Vec::new();
            let inReal0: &[f64];
            let inReal1: &[f64];
            if use_preloaded != 0 && ref_data.n > 0 {
                inReal0 = &ref_data.close[..ref_data.n];
                inReal1 = &ref_data.high[..ref_data.n];
            } else {
                _json_inReal0 = parse_f64_array(&params["inReal0"]);
                inReal0 = &_json_inReal0;
                _json_inReal1 = parse_f64_array(&params["inReal1"]);
                inReal1 = &_json_inReal1;
            }
            let optInTimePeriod = params["optInTimePeriod"].as_i64().unwrap_or(5) as i32;
            let out_size = if endIdx >= startIdx { endIdx - startIdx + 1 } else { 0 };
            let mut outBuf0: Vec<f64> = vec![0.0f64; out_size];
            let mut outBegIdx: usize = 0;
            let mut outNBElement: usize = 0;
            let mut rc = RetCode::Success;
            let start_time = Instant::now();
            for _bi in 0..bench_iters {
            rc = core.beta(
                startIdx, endIdx,
                &inReal0,
                &inReal1,
                optInTimePeriod,
                &mut outBegIdx, &mut outNBElement, &mut outBuf0,
            );
            }
            let elapsed_ns = start_time.elapsed().as_nanos() as u64 / bench_iters as u64;
            let start_time_ung = Instant::now();
            for _biu in 0..bench_iters {
            rc = core.beta_unguarded(
                startIdx, endIdx,
                &inReal0,
                &inReal1,
                optInTimePeriod,
                &mut outBegIdx, &mut outNBElement, &mut outBuf0,
            );
            }
            let elapsed_ns_ung = start_time_ung.elapsed().as_nanos() as u64 / bench_iters as u64;
            let lookback = core.beta_lookback(optInTimePeriod);
            let mut resp = format!("{{\"retCode\":{},\"outBegIdx\":{},\"outNBElement\":{},\"lookback\":{},\"timing_ns\":{},\"timing_ns_unguarded\":{}", retcode_to_int(rc), outBegIdx, outNBElement, lookback, elapsed_ns, elapsed_ns_ung);
            resp.push_str(",\"outReal\":"); resp.push_str(&json_f64_array(&outBuf0[..outNBElement]));
            resp.push('}');
            resp
        }
        "TA_BOP" => {
            let startIdx = params["startIdx"].as_u64().unwrap_or(0) as usize;
            let endIdx = params["endIdx"].as_u64().unwrap_or(0) as usize;
            let use_preloaded = params["use_preloaded"].as_i64().unwrap_or(0);
            let bench_iters = std::cmp::max(1, params["iters"].as_i64().unwrap_or(1)) as u64;
            let mut _json_inOpen: Vec<f64> = Vec::new();
            let mut _json_inHigh: Vec<f64> = Vec::new();
            let mut _json_inLow: Vec<f64> = Vec::new();
            let mut _json_inClose: Vec<f64> = Vec::new();
            let inOpen: &[f64];
            let inHigh: &[f64];
            let inLow: &[f64];
            let inClose: &[f64];
            if use_preloaded != 0 && ref_data.n > 0 {
                inOpen = &ref_data.open[..ref_data.n];
                inHigh = &ref_data.high[..ref_data.n];
                inLow = &ref_data.low[..ref_data.n];
                inClose = &ref_data.close[..ref_data.n];
            } else {
                _json_inOpen = parse_f64_array(&params["inOpen"]);
                inOpen = &_json_inOpen;
                _json_inHigh = parse_f64_array(&params["inHigh"]);
                inHigh = &_json_inHigh;
                _json_inLow = parse_f64_array(&params["inLow"]);
                inLow = &_json_inLow;
                _json_inClose = parse_f64_array(&params["inClose"]);
                inClose = &_json_inClose;
            }
            let out_size = if endIdx >= startIdx { endIdx - startIdx + 1 } else { 0 };
            let mut outBuf0: Vec<f64> = vec![0.0f64; out_size];
            let mut outBegIdx: usize = 0;
            let mut outNBElement: usize = 0;
            let mut rc = RetCode::Success;
            let start_time = Instant::now();
            for _bi in 0..bench_iters {
            rc = core.bop(
                startIdx, endIdx,
                &inOpen,
                &inHigh,
                &inLow,
                &inClose,
                &mut outBegIdx, &mut outNBElement, &mut outBuf0,
            );
            }
            let elapsed_ns = start_time.elapsed().as_nanos() as u64 / bench_iters as u64;
            let start_time_ung = Instant::now();
            for _biu in 0..bench_iters {
            rc = core.bop_unguarded(
                startIdx, endIdx,
                &inOpen,
                &inHigh,
                &inLow,
                &inClose,
                &mut outBegIdx, &mut outNBElement, &mut outBuf0,
            );
            }
            let elapsed_ns_ung = start_time_ung.elapsed().as_nanos() as u64 / bench_iters as u64;
            let lookback = core.bop_lookback();
            let mut resp = format!("{{\"retCode\":{},\"outBegIdx\":{},\"outNBElement\":{},\"lookback\":{},\"timing_ns\":{},\"timing_ns_unguarded\":{}", retcode_to_int(rc), outBegIdx, outNBElement, lookback, elapsed_ns, elapsed_ns_ung);
            resp.push_str(",\"outReal\":"); resp.push_str(&json_f64_array(&outBuf0[..outNBElement]));
            resp.push('}');
            resp
        }
        "TA_CCI" => {
            let startIdx = params["startIdx"].as_u64().unwrap_or(0) as usize;
            let endIdx = params["endIdx"].as_u64().unwrap_or(0) as usize;
            let use_preloaded = params["use_preloaded"].as_i64().unwrap_or(0);
            let bench_iters = std::cmp::max(1, params["iters"].as_i64().unwrap_or(1)) as u64;
            let mut _json_inHigh: Vec<f64> = Vec::new();
            let mut _json_inLow: Vec<f64> = Vec::new();
            let mut _json_inClose: Vec<f64> = Vec::new();
            let inHigh: &[f64];
            let inLow: &[f64];
            let inClose: &[f64];
            if use_preloaded != 0 && ref_data.n > 0 {
                inHigh = &ref_data.high[..ref_data.n];
                inLow = &ref_data.low[..ref_data.n];
                inClose = &ref_data.close[..ref_data.n];
            } else {
                _json_inHigh = parse_f64_array(&params["inHigh"]);
                inHigh = &_json_inHigh;
                _json_inLow = parse_f64_array(&params["inLow"]);
                inLow = &_json_inLow;
                _json_inClose = parse_f64_array(&params["inClose"]);
                inClose = &_json_inClose;
            }
            let optInTimePeriod = params["optInTimePeriod"].as_i64().unwrap_or(14) as i32;
            let out_size = if endIdx >= startIdx { endIdx - startIdx + 1 } else { 0 };
            let mut outBuf0: Vec<f64> = vec![0.0f64; out_size];
            let mut outBegIdx: usize = 0;
            let mut outNBElement: usize = 0;
            let mut rc = RetCode::Success;
            let start_time = Instant::now();
            for _bi in 0..bench_iters {
            rc = core.cci(
                startIdx, endIdx,
                &inHigh,
                &inLow,
                &inClose,
                optInTimePeriod,
                &mut outBegIdx, &mut outNBElement, &mut outBuf0,
            );
            }
            let elapsed_ns = start_time.elapsed().as_nanos() as u64 / bench_iters as u64;
            let start_time_ung = Instant::now();
            for _biu in 0..bench_iters {
            rc = core.cci_unguarded(
                startIdx, endIdx,
                &inHigh,
                &inLow,
                &inClose,
                optInTimePeriod,
                &mut outBegIdx, &mut outNBElement, &mut outBuf0,
            );
            }
            let elapsed_ns_ung = start_time_ung.elapsed().as_nanos() as u64 / bench_iters as u64;
            let lookback = core.cci_lookback(optInTimePeriod);
            let mut resp = format!("{{\"retCode\":{},\"outBegIdx\":{},\"outNBElement\":{},\"lookback\":{},\"timing_ns\":{},\"timing_ns_unguarded\":{}", retcode_to_int(rc), outBegIdx, outNBElement, lookback, elapsed_ns, elapsed_ns_ung);
            resp.push_str(",\"outReal\":"); resp.push_str(&json_f64_array(&outBuf0[..outNBElement]));
            resp.push('}');
            resp
        }
        "TA_CDL2CROWS" => {
            let startIdx = params["startIdx"].as_u64().unwrap_or(0) as usize;
            let endIdx = params["endIdx"].as_u64().unwrap_or(0) as usize;
            let use_preloaded = params["use_preloaded"].as_i64().unwrap_or(0);
            let bench_iters = std::cmp::max(1, params["iters"].as_i64().unwrap_or(1)) as u64;
            let mut _json_inOpen: Vec<f64> = Vec::new();
            let mut _json_inHigh: Vec<f64> = Vec::new();
            let mut _json_inLow: Vec<f64> = Vec::new();
            let mut _json_inClose: Vec<f64> = Vec::new();
            let inOpen: &[f64];
            let inHigh: &[f64];
            let inLow: &[f64];
            let inClose: &[f64];
            if use_preloaded != 0 && ref_data.n > 0 {
                inOpen = &ref_data.open[..ref_data.n];
                inHigh = &ref_data.high[..ref_data.n];
                inLow = &ref_data.low[..ref_data.n];
                inClose = &ref_data.close[..ref_data.n];
            } else {
                _json_inOpen = parse_f64_array(&params["inOpen"]);
                inOpen = &_json_inOpen;
                _json_inHigh = parse_f64_array(&params["inHigh"]);
                inHigh = &_json_inHigh;
                _json_inLow = parse_f64_array(&params["inLow"]);
                inLow = &_json_inLow;
                _json_inClose = parse_f64_array(&params["inClose"]);
                inClose = &_json_inClose;
            }
            let out_size = if endIdx >= startIdx { endIdx - startIdx + 1 } else { 0 };
            let mut outIntBuf0: Vec<i32> = vec![0i32; out_size];
            let mut outBegIdx: usize = 0;
            let mut outNBElement: usize = 0;
            let mut rc = RetCode::Success;
            let start_time = Instant::now();
            for _bi in 0..bench_iters {
            rc = core.cdl2crows(
                startIdx, endIdx,
                &inOpen,
                &inHigh,
                &inLow,
                &inClose,
                &mut outBegIdx, &mut outNBElement, &mut outIntBuf0,
            );
            }
            let elapsed_ns = start_time.elapsed().as_nanos() as u64 / bench_iters as u64;
            let start_time_ung = Instant::now();
            for _biu in 0..bench_iters {
            rc = core.cdl2crows_unguarded(
                startIdx, endIdx,
                &inOpen,
                &inHigh,
                &inLow,
                &inClose,
                &mut outBegIdx, &mut outNBElement, &mut outIntBuf0,
            );
            }
            let elapsed_ns_ung = start_time_ung.elapsed().as_nanos() as u64 / bench_iters as u64;
            let lookback = core.cdl2crows_lookback();
            let mut resp = format!("{{\"retCode\":{},\"outBegIdx\":{},\"outNBElement\":{},\"lookback\":{},\"timing_ns\":{},\"timing_ns_unguarded\":{}", retcode_to_int(rc), outBegIdx, outNBElement, lookback, elapsed_ns, elapsed_ns_ung);
            resp.push_str(",\"outInteger\":"); resp.push_str(&json_i32_array(&outIntBuf0[..outNBElement]));
            resp.push('}');
            resp
        }
        "TA_CDL3BLACKCROWS" => {
            let startIdx = params["startIdx"].as_u64().unwrap_or(0) as usize;
            let endIdx = params["endIdx"].as_u64().unwrap_or(0) as usize;
            let use_preloaded = params["use_preloaded"].as_i64().unwrap_or(0);
            let bench_iters = std::cmp::max(1, params["iters"].as_i64().unwrap_or(1)) as u64;
            let mut _json_inOpen: Vec<f64> = Vec::new();
            let mut _json_inHigh: Vec<f64> = Vec::new();
            let mut _json_inLow: Vec<f64> = Vec::new();
            let mut _json_inClose: Vec<f64> = Vec::new();
            let inOpen: &[f64];
            let inHigh: &[f64];
            let inLow: &[f64];
            let inClose: &[f64];
            if use_preloaded != 0 && ref_data.n > 0 {
                inOpen = &ref_data.open[..ref_data.n];
                inHigh = &ref_data.high[..ref_data.n];
                inLow = &ref_data.low[..ref_data.n];
                inClose = &ref_data.close[..ref_data.n];
            } else {
                _json_inOpen = parse_f64_array(&params["inOpen"]);
                inOpen = &_json_inOpen;
                _json_inHigh = parse_f64_array(&params["inHigh"]);
                inHigh = &_json_inHigh;
                _json_inLow = parse_f64_array(&params["inLow"]);
                inLow = &_json_inLow;
                _json_inClose = parse_f64_array(&params["inClose"]);
                inClose = &_json_inClose;
            }
            let out_size = if endIdx >= startIdx { endIdx - startIdx + 1 } else { 0 };
            let mut outIntBuf0: Vec<i32> = vec![0i32; out_size];
            let mut outBegIdx: usize = 0;
            let mut outNBElement: usize = 0;
            let mut rc = RetCode::Success;
            let start_time = Instant::now();
            for _bi in 0..bench_iters {
            rc = core.cdl3blackcrows(
                startIdx, endIdx,
                &inOpen,
                &inHigh,
                &inLow,
                &inClose,
                &mut outBegIdx, &mut outNBElement, &mut outIntBuf0,
            );
            }
            let elapsed_ns = start_time.elapsed().as_nanos() as u64 / bench_iters as u64;
            let start_time_ung = Instant::now();
            for _biu in 0..bench_iters {
            rc = core.cdl3blackcrows_unguarded(
                startIdx, endIdx,
                &inOpen,
                &inHigh,
                &inLow,
                &inClose,
                &mut outBegIdx, &mut outNBElement, &mut outIntBuf0,
            );
            }
            let elapsed_ns_ung = start_time_ung.elapsed().as_nanos() as u64 / bench_iters as u64;
            let lookback = core.cdl3blackcrows_lookback();
            let mut resp = format!("{{\"retCode\":{},\"outBegIdx\":{},\"outNBElement\":{},\"lookback\":{},\"timing_ns\":{},\"timing_ns_unguarded\":{}", retcode_to_int(rc), outBegIdx, outNBElement, lookback, elapsed_ns, elapsed_ns_ung);
            resp.push_str(",\"outInteger\":"); resp.push_str(&json_i32_array(&outIntBuf0[..outNBElement]));
            resp.push('}');
            resp
        }
        "TA_CDL3INSIDE" => {
            let startIdx = params["startIdx"].as_u64().unwrap_or(0) as usize;
            let endIdx = params["endIdx"].as_u64().unwrap_or(0) as usize;
            let use_preloaded = params["use_preloaded"].as_i64().unwrap_or(0);
            let bench_iters = std::cmp::max(1, params["iters"].as_i64().unwrap_or(1)) as u64;
            let mut _json_inOpen: Vec<f64> = Vec::new();
            let mut _json_inHigh: Vec<f64> = Vec::new();
            let mut _json_inLow: Vec<f64> = Vec::new();
            let mut _json_inClose: Vec<f64> = Vec::new();
            let inOpen: &[f64];
            let inHigh: &[f64];
            let inLow: &[f64];
            let inClose: &[f64];
            if use_preloaded != 0 && ref_data.n > 0 {
                inOpen = &ref_data.open[..ref_data.n];
                inHigh = &ref_data.high[..ref_data.n];
                inLow = &ref_data.low[..ref_data.n];
                inClose = &ref_data.close[..ref_data.n];
            } else {
                _json_inOpen = parse_f64_array(&params["inOpen"]);
                inOpen = &_json_inOpen;
                _json_inHigh = parse_f64_array(&params["inHigh"]);
                inHigh = &_json_inHigh;
                _json_inLow = parse_f64_array(&params["inLow"]);
                inLow = &_json_inLow;
                _json_inClose = parse_f64_array(&params["inClose"]);
                inClose = &_json_inClose;
            }
            let out_size = if endIdx >= startIdx { endIdx - startIdx + 1 } else { 0 };
            let mut outIntBuf0: Vec<i32> = vec![0i32; out_size];
            let mut outBegIdx: usize = 0;
            let mut outNBElement: usize = 0;
            let mut rc = RetCode::Success;
            let start_time = Instant::now();
            for _bi in 0..bench_iters {
            rc = core.cdl3inside(
                startIdx, endIdx,
                &inOpen,
                &inHigh,
                &inLow,
                &inClose,
                &mut outBegIdx, &mut outNBElement, &mut outIntBuf0,
            );
            }
            let elapsed_ns = start_time.elapsed().as_nanos() as u64 / bench_iters as u64;
            let start_time_ung = Instant::now();
            for _biu in 0..bench_iters {
            rc = core.cdl3inside_unguarded(
                startIdx, endIdx,
                &inOpen,
                &inHigh,
                &inLow,
                &inClose,
                &mut outBegIdx, &mut outNBElement, &mut outIntBuf0,
            );
            }
            let elapsed_ns_ung = start_time_ung.elapsed().as_nanos() as u64 / bench_iters as u64;
            let lookback = core.cdl3inside_lookback();
            let mut resp = format!("{{\"retCode\":{},\"outBegIdx\":{},\"outNBElement\":{},\"lookback\":{},\"timing_ns\":{},\"timing_ns_unguarded\":{}", retcode_to_int(rc), outBegIdx, outNBElement, lookback, elapsed_ns, elapsed_ns_ung);
            resp.push_str(",\"outInteger\":"); resp.push_str(&json_i32_array(&outIntBuf0[..outNBElement]));
            resp.push('}');
            resp
        }
        "TA_CDL3LINESTRIKE" => {
            let startIdx = params["startIdx"].as_u64().unwrap_or(0) as usize;
            let endIdx = params["endIdx"].as_u64().unwrap_or(0) as usize;
            let use_preloaded = params["use_preloaded"].as_i64().unwrap_or(0);
            let bench_iters = std::cmp::max(1, params["iters"].as_i64().unwrap_or(1)) as u64;
            let mut _json_inOpen: Vec<f64> = Vec::new();
            let mut _json_inHigh: Vec<f64> = Vec::new();
            let mut _json_inLow: Vec<f64> = Vec::new();
            let mut _json_inClose: Vec<f64> = Vec::new();
            let inOpen: &[f64];
            let inHigh: &[f64];
            let inLow: &[f64];
            let inClose: &[f64];
            if use_preloaded != 0 && ref_data.n > 0 {
                inOpen = &ref_data.open[..ref_data.n];
                inHigh = &ref_data.high[..ref_data.n];
                inLow = &ref_data.low[..ref_data.n];
                inClose = &ref_data.close[..ref_data.n];
            } else {
                _json_inOpen = parse_f64_array(&params["inOpen"]);
                inOpen = &_json_inOpen;
                _json_inHigh = parse_f64_array(&params["inHigh"]);
                inHigh = &_json_inHigh;
                _json_inLow = parse_f64_array(&params["inLow"]);
                inLow = &_json_inLow;
                _json_inClose = parse_f64_array(&params["inClose"]);
                inClose = &_json_inClose;
            }
            let out_size = if endIdx >= startIdx { endIdx - startIdx + 1 } else { 0 };
            let mut outIntBuf0: Vec<i32> = vec![0i32; out_size];
            let mut outBegIdx: usize = 0;
            let mut outNBElement: usize = 0;
            let mut rc = RetCode::Success;
            let start_time = Instant::now();
            for _bi in 0..bench_iters {
            rc = core.cdl3linestrike(
                startIdx, endIdx,
                &inOpen,
                &inHigh,
                &inLow,
                &inClose,
                &mut outBegIdx, &mut outNBElement, &mut outIntBuf0,
            );
            }
            let elapsed_ns = start_time.elapsed().as_nanos() as u64 / bench_iters as u64;
            let start_time_ung = Instant::now();
            for _biu in 0..bench_iters {
            rc = core.cdl3linestrike_unguarded(
                startIdx, endIdx,
                &inOpen,
                &inHigh,
                &inLow,
                &inClose,
                &mut outBegIdx, &mut outNBElement, &mut outIntBuf0,
            );
            }
            let elapsed_ns_ung = start_time_ung.elapsed().as_nanos() as u64 / bench_iters as u64;
            let lookback = core.cdl3linestrike_lookback();
            let mut resp = format!("{{\"retCode\":{},\"outBegIdx\":{},\"outNBElement\":{},\"lookback\":{},\"timing_ns\":{},\"timing_ns_unguarded\":{}", retcode_to_int(rc), outBegIdx, outNBElement, lookback, elapsed_ns, elapsed_ns_ung);
            resp.push_str(",\"outInteger\":"); resp.push_str(&json_i32_array(&outIntBuf0[..outNBElement]));
            resp.push('}');
            resp
        }
        "TA_CDL3OUTSIDE" => {
            let startIdx = params["startIdx"].as_u64().unwrap_or(0) as usize;
            let endIdx = params["endIdx"].as_u64().unwrap_or(0) as usize;
            let use_preloaded = params["use_preloaded"].as_i64().unwrap_or(0);
            let bench_iters = std::cmp::max(1, params["iters"].as_i64().unwrap_or(1)) as u64;
            let mut _json_inOpen: Vec<f64> = Vec::new();
            let mut _json_inHigh: Vec<f64> = Vec::new();
            let mut _json_inLow: Vec<f64> = Vec::new();
            let mut _json_inClose: Vec<f64> = Vec::new();
            let inOpen: &[f64];
            let inHigh: &[f64];
            let inLow: &[f64];
            let inClose: &[f64];
            if use_preloaded != 0 && ref_data.n > 0 {
                inOpen = &ref_data.open[..ref_data.n];
                inHigh = &ref_data.high[..ref_data.n];
                inLow = &ref_data.low[..ref_data.n];
                inClose = &ref_data.close[..ref_data.n];
            } else {
                _json_inOpen = parse_f64_array(&params["inOpen"]);
                inOpen = &_json_inOpen;
                _json_inHigh = parse_f64_array(&params["inHigh"]);
                inHigh = &_json_inHigh;
                _json_inLow = parse_f64_array(&params["inLow"]);
                inLow = &_json_inLow;
                _json_inClose = parse_f64_array(&params["inClose"]);
                inClose = &_json_inClose;
            }
            let out_size = if endIdx >= startIdx { endIdx - startIdx + 1 } else { 0 };
            let mut outIntBuf0: Vec<i32> = vec![0i32; out_size];
            let mut outBegIdx: usize = 0;
            let mut outNBElement: usize = 0;
            let mut rc = RetCode::Success;
            let start_time = Instant::now();
            for _bi in 0..bench_iters {
            rc = core.cdl3outside(
                startIdx, endIdx,
                &inOpen,
                &inHigh,
                &inLow,
                &inClose,
                &mut outBegIdx, &mut outNBElement, &mut outIntBuf0,
            );
            }
            let elapsed_ns = start_time.elapsed().as_nanos() as u64 / bench_iters as u64;
            let start_time_ung = Instant::now();
            for _biu in 0..bench_iters {
            rc = core.cdl3outside_unguarded(
                startIdx, endIdx,
                &inOpen,
                &inHigh,
                &inLow,
                &inClose,
                &mut outBegIdx, &mut outNBElement, &mut outIntBuf0,
            );
            }
            let elapsed_ns_ung = start_time_ung.elapsed().as_nanos() as u64 / bench_iters as u64;
            let lookback = core.cdl3outside_lookback();
            let mut resp = format!("{{\"retCode\":{},\"outBegIdx\":{},\"outNBElement\":{},\"lookback\":{},\"timing_ns\":{},\"timing_ns_unguarded\":{}", retcode_to_int(rc), outBegIdx, outNBElement, lookback, elapsed_ns, elapsed_ns_ung);
            resp.push_str(",\"outInteger\":"); resp.push_str(&json_i32_array(&outIntBuf0[..outNBElement]));
            resp.push('}');
            resp
        }
        "TA_CDL3STARSINSOUTH" => {
            let startIdx = params["startIdx"].as_u64().unwrap_or(0) as usize;
            let endIdx = params["endIdx"].as_u64().unwrap_or(0) as usize;
            let use_preloaded = params["use_preloaded"].as_i64().unwrap_or(0);
            let bench_iters = std::cmp::max(1, params["iters"].as_i64().unwrap_or(1)) as u64;
            let mut _json_inOpen: Vec<f64> = Vec::new();
            let mut _json_inHigh: Vec<f64> = Vec::new();
            let mut _json_inLow: Vec<f64> = Vec::new();
            let mut _json_inClose: Vec<f64> = Vec::new();
            let inOpen: &[f64];
            let inHigh: &[f64];
            let inLow: &[f64];
            let inClose: &[f64];
            if use_preloaded != 0 && ref_data.n > 0 {
                inOpen = &ref_data.open[..ref_data.n];
                inHigh = &ref_data.high[..ref_data.n];
                inLow = &ref_data.low[..ref_data.n];
                inClose = &ref_data.close[..ref_data.n];
            } else {
                _json_inOpen = parse_f64_array(&params["inOpen"]);
                inOpen = &_json_inOpen;
                _json_inHigh = parse_f64_array(&params["inHigh"]);
                inHigh = &_json_inHigh;
                _json_inLow = parse_f64_array(&params["inLow"]);
                inLow = &_json_inLow;
                _json_inClose = parse_f64_array(&params["inClose"]);
                inClose = &_json_inClose;
            }
            let out_size = if endIdx >= startIdx { endIdx - startIdx + 1 } else { 0 };
            let mut outIntBuf0: Vec<i32> = vec![0i32; out_size];
            let mut outBegIdx: usize = 0;
            let mut outNBElement: usize = 0;
            let mut rc = RetCode::Success;
            let start_time = Instant::now();
            for _bi in 0..bench_iters {
            rc = core.cdl3starsinsouth(
                startIdx, endIdx,
                &inOpen,
                &inHigh,
                &inLow,
                &inClose,
                &mut outBegIdx, &mut outNBElement, &mut outIntBuf0,
            );
            }
            let elapsed_ns = start_time.elapsed().as_nanos() as u64 / bench_iters as u64;
            let start_time_ung = Instant::now();
            for _biu in 0..bench_iters {
            rc = core.cdl3starsinsouth_unguarded(
                startIdx, endIdx,
                &inOpen,
                &inHigh,
                &inLow,
                &inClose,
                &mut outBegIdx, &mut outNBElement, &mut outIntBuf0,
            );
            }
            let elapsed_ns_ung = start_time_ung.elapsed().as_nanos() as u64 / bench_iters as u64;
            let lookback = core.cdl3starsinsouth_lookback();
            let mut resp = format!("{{\"retCode\":{},\"outBegIdx\":{},\"outNBElement\":{},\"lookback\":{},\"timing_ns\":{},\"timing_ns_unguarded\":{}", retcode_to_int(rc), outBegIdx, outNBElement, lookback, elapsed_ns, elapsed_ns_ung);
            resp.push_str(",\"outInteger\":"); resp.push_str(&json_i32_array(&outIntBuf0[..outNBElement]));
            resp.push('}');
            resp
        }
        "TA_CDL3WHITESOLDIERS" => {
            let startIdx = params["startIdx"].as_u64().unwrap_or(0) as usize;
            let endIdx = params["endIdx"].as_u64().unwrap_or(0) as usize;
            let use_preloaded = params["use_preloaded"].as_i64().unwrap_or(0);
            let bench_iters = std::cmp::max(1, params["iters"].as_i64().unwrap_or(1)) as u64;
            let mut _json_inOpen: Vec<f64> = Vec::new();
            let mut _json_inHigh: Vec<f64> = Vec::new();
            let mut _json_inLow: Vec<f64> = Vec::new();
            let mut _json_inClose: Vec<f64> = Vec::new();
            let inOpen: &[f64];
            let inHigh: &[f64];
            let inLow: &[f64];
            let inClose: &[f64];
            if use_preloaded != 0 && ref_data.n > 0 {
                inOpen = &ref_data.open[..ref_data.n];
                inHigh = &ref_data.high[..ref_data.n];
                inLow = &ref_data.low[..ref_data.n];
                inClose = &ref_data.close[..ref_data.n];
            } else {
                _json_inOpen = parse_f64_array(&params["inOpen"]);
                inOpen = &_json_inOpen;
                _json_inHigh = parse_f64_array(&params["inHigh"]);
                inHigh = &_json_inHigh;
                _json_inLow = parse_f64_array(&params["inLow"]);
                inLow = &_json_inLow;
                _json_inClose = parse_f64_array(&params["inClose"]);
                inClose = &_json_inClose;
            }
            let out_size = if endIdx >= startIdx { endIdx - startIdx + 1 } else { 0 };
            let mut outIntBuf0: Vec<i32> = vec![0i32; out_size];
            let mut outBegIdx: usize = 0;
            let mut outNBElement: usize = 0;
            let mut rc = RetCode::Success;
            let start_time = Instant::now();
            for _bi in 0..bench_iters {
            rc = core.cdl3whitesoldiers(
                startIdx, endIdx,
                &inOpen,
                &inHigh,
                &inLow,
                &inClose,
                &mut outBegIdx, &mut outNBElement, &mut outIntBuf0,
            );
            }
            let elapsed_ns = start_time.elapsed().as_nanos() as u64 / bench_iters as u64;
            let start_time_ung = Instant::now();
            for _biu in 0..bench_iters {
            rc = core.cdl3whitesoldiers_unguarded(
                startIdx, endIdx,
                &inOpen,
                &inHigh,
                &inLow,
                &inClose,
                &mut outBegIdx, &mut outNBElement, &mut outIntBuf0,
            );
            }
            let elapsed_ns_ung = start_time_ung.elapsed().as_nanos() as u64 / bench_iters as u64;
            let lookback = core.cdl3whitesoldiers_lookback();
            let mut resp = format!("{{\"retCode\":{},\"outBegIdx\":{},\"outNBElement\":{},\"lookback\":{},\"timing_ns\":{},\"timing_ns_unguarded\":{}", retcode_to_int(rc), outBegIdx, outNBElement, lookback, elapsed_ns, elapsed_ns_ung);
            resp.push_str(",\"outInteger\":"); resp.push_str(&json_i32_array(&outIntBuf0[..outNBElement]));
            resp.push('}');
            resp
        }
        "TA_CDLABANDONEDBABY" => {
            let startIdx = params["startIdx"].as_u64().unwrap_or(0) as usize;
            let endIdx = params["endIdx"].as_u64().unwrap_or(0) as usize;
            let use_preloaded = params["use_preloaded"].as_i64().unwrap_or(0);
            let bench_iters = std::cmp::max(1, params["iters"].as_i64().unwrap_or(1)) as u64;
            let mut _json_inOpen: Vec<f64> = Vec::new();
            let mut _json_inHigh: Vec<f64> = Vec::new();
            let mut _json_inLow: Vec<f64> = Vec::new();
            let mut _json_inClose: Vec<f64> = Vec::new();
            let inOpen: &[f64];
            let inHigh: &[f64];
            let inLow: &[f64];
            let inClose: &[f64];
            if use_preloaded != 0 && ref_data.n > 0 {
                inOpen = &ref_data.open[..ref_data.n];
                inHigh = &ref_data.high[..ref_data.n];
                inLow = &ref_data.low[..ref_data.n];
                inClose = &ref_data.close[..ref_data.n];
            } else {
                _json_inOpen = parse_f64_array(&params["inOpen"]);
                inOpen = &_json_inOpen;
                _json_inHigh = parse_f64_array(&params["inHigh"]);
                inHigh = &_json_inHigh;
                _json_inLow = parse_f64_array(&params["inLow"]);
                inLow = &_json_inLow;
                _json_inClose = parse_f64_array(&params["inClose"]);
                inClose = &_json_inClose;
            }
            let optInPenetration = params["optInPenetration"].as_f64().unwrap_or(0.3) as f64;
            let out_size = if endIdx >= startIdx { endIdx - startIdx + 1 } else { 0 };
            let mut outIntBuf0: Vec<i32> = vec![0i32; out_size];
            let mut outBegIdx: usize = 0;
            let mut outNBElement: usize = 0;
            let mut rc = RetCode::Success;
            let start_time = Instant::now();
            for _bi in 0..bench_iters {
            rc = core.cdlabandonedbaby(
                startIdx, endIdx,
                &inOpen,
                &inHigh,
                &inLow,
                &inClose,
                optInPenetration,
                &mut outBegIdx, &mut outNBElement, &mut outIntBuf0,
            );
            }
            let elapsed_ns = start_time.elapsed().as_nanos() as u64 / bench_iters as u64;
            let start_time_ung = Instant::now();
            for _biu in 0..bench_iters {
            rc = core.cdlabandonedbaby_unguarded(
                startIdx, endIdx,
                &inOpen,
                &inHigh,
                &inLow,
                &inClose,
                optInPenetration,
                &mut outBegIdx, &mut outNBElement, &mut outIntBuf0,
            );
            }
            let elapsed_ns_ung = start_time_ung.elapsed().as_nanos() as u64 / bench_iters as u64;
            let lookback = core.cdlabandonedbaby_lookback(optInPenetration);
            let mut resp = format!("{{\"retCode\":{},\"outBegIdx\":{},\"outNBElement\":{},\"lookback\":{},\"timing_ns\":{},\"timing_ns_unguarded\":{}", retcode_to_int(rc), outBegIdx, outNBElement, lookback, elapsed_ns, elapsed_ns_ung);
            resp.push_str(",\"outInteger\":"); resp.push_str(&json_i32_array(&outIntBuf0[..outNBElement]));
            resp.push('}');
            resp
        }
        "TA_CDLADVANCEBLOCK" => {
            let startIdx = params["startIdx"].as_u64().unwrap_or(0) as usize;
            let endIdx = params["endIdx"].as_u64().unwrap_or(0) as usize;
            let use_preloaded = params["use_preloaded"].as_i64().unwrap_or(0);
            let bench_iters = std::cmp::max(1, params["iters"].as_i64().unwrap_or(1)) as u64;
            let mut _json_inOpen: Vec<f64> = Vec::new();
            let mut _json_inHigh: Vec<f64> = Vec::new();
            let mut _json_inLow: Vec<f64> = Vec::new();
            let mut _json_inClose: Vec<f64> = Vec::new();
            let inOpen: &[f64];
            let inHigh: &[f64];
            let inLow: &[f64];
            let inClose: &[f64];
            if use_preloaded != 0 && ref_data.n > 0 {
                inOpen = &ref_data.open[..ref_data.n];
                inHigh = &ref_data.high[..ref_data.n];
                inLow = &ref_data.low[..ref_data.n];
                inClose = &ref_data.close[..ref_data.n];
            } else {
                _json_inOpen = parse_f64_array(&params["inOpen"]);
                inOpen = &_json_inOpen;
                _json_inHigh = parse_f64_array(&params["inHigh"]);
                inHigh = &_json_inHigh;
                _json_inLow = parse_f64_array(&params["inLow"]);
                inLow = &_json_inLow;
                _json_inClose = parse_f64_array(&params["inClose"]);
                inClose = &_json_inClose;
            }
            let out_size = if endIdx >= startIdx { endIdx - startIdx + 1 } else { 0 };
            let mut outIntBuf0: Vec<i32> = vec![0i32; out_size];
            let mut outBegIdx: usize = 0;
            let mut outNBElement: usize = 0;
            let mut rc = RetCode::Success;
            let start_time = Instant::now();
            for _bi in 0..bench_iters {
            rc = core.cdladvanceblock(
                startIdx, endIdx,
                &inOpen,
                &inHigh,
                &inLow,
                &inClose,
                &mut outBegIdx, &mut outNBElement, &mut outIntBuf0,
            );
            }
            let elapsed_ns = start_time.elapsed().as_nanos() as u64 / bench_iters as u64;
            let start_time_ung = Instant::now();
            for _biu in 0..bench_iters {
            rc = core.cdladvanceblock_unguarded(
                startIdx, endIdx,
                &inOpen,
                &inHigh,
                &inLow,
                &inClose,
                &mut outBegIdx, &mut outNBElement, &mut outIntBuf0,
            );
            }
            let elapsed_ns_ung = start_time_ung.elapsed().as_nanos() as u64 / bench_iters as u64;
            let lookback = core.cdladvanceblock_lookback();
            let mut resp = format!("{{\"retCode\":{},\"outBegIdx\":{},\"outNBElement\":{},\"lookback\":{},\"timing_ns\":{},\"timing_ns_unguarded\":{}", retcode_to_int(rc), outBegIdx, outNBElement, lookback, elapsed_ns, elapsed_ns_ung);
            resp.push_str(",\"outInteger\":"); resp.push_str(&json_i32_array(&outIntBuf0[..outNBElement]));
            resp.push('}');
            resp
        }
        "TA_CDLBELTHOLD" => {
            let startIdx = params["startIdx"].as_u64().unwrap_or(0) as usize;
            let endIdx = params["endIdx"].as_u64().unwrap_or(0) as usize;
            let use_preloaded = params["use_preloaded"].as_i64().unwrap_or(0);
            let bench_iters = std::cmp::max(1, params["iters"].as_i64().unwrap_or(1)) as u64;
            let mut _json_inOpen: Vec<f64> = Vec::new();
            let mut _json_inHigh: Vec<f64> = Vec::new();
            let mut _json_inLow: Vec<f64> = Vec::new();
            let mut _json_inClose: Vec<f64> = Vec::new();
            let inOpen: &[f64];
            let inHigh: &[f64];
            let inLow: &[f64];
            let inClose: &[f64];
            if use_preloaded != 0 && ref_data.n > 0 {
                inOpen = &ref_data.open[..ref_data.n];
                inHigh = &ref_data.high[..ref_data.n];
                inLow = &ref_data.low[..ref_data.n];
                inClose = &ref_data.close[..ref_data.n];
            } else {
                _json_inOpen = parse_f64_array(&params["inOpen"]);
                inOpen = &_json_inOpen;
                _json_inHigh = parse_f64_array(&params["inHigh"]);
                inHigh = &_json_inHigh;
                _json_inLow = parse_f64_array(&params["inLow"]);
                inLow = &_json_inLow;
                _json_inClose = parse_f64_array(&params["inClose"]);
                inClose = &_json_inClose;
            }
            let out_size = if endIdx >= startIdx { endIdx - startIdx + 1 } else { 0 };
            let mut outIntBuf0: Vec<i32> = vec![0i32; out_size];
            let mut outBegIdx: usize = 0;
            let mut outNBElement: usize = 0;
            let mut rc = RetCode::Success;
            let start_time = Instant::now();
            for _bi in 0..bench_iters {
            rc = core.cdlbelthold(
                startIdx, endIdx,
                &inOpen,
                &inHigh,
                &inLow,
                &inClose,
                &mut outBegIdx, &mut outNBElement, &mut outIntBuf0,
            );
            }
            let elapsed_ns = start_time.elapsed().as_nanos() as u64 / bench_iters as u64;
            let start_time_ung = Instant::now();
            for _biu in 0..bench_iters {
            rc = core.cdlbelthold_unguarded(
                startIdx, endIdx,
                &inOpen,
                &inHigh,
                &inLow,
                &inClose,
                &mut outBegIdx, &mut outNBElement, &mut outIntBuf0,
            );
            }
            let elapsed_ns_ung = start_time_ung.elapsed().as_nanos() as u64 / bench_iters as u64;
            let lookback = core.cdlbelthold_lookback();
            let mut resp = format!("{{\"retCode\":{},\"outBegIdx\":{},\"outNBElement\":{},\"lookback\":{},\"timing_ns\":{},\"timing_ns_unguarded\":{}", retcode_to_int(rc), outBegIdx, outNBElement, lookback, elapsed_ns, elapsed_ns_ung);
            resp.push_str(",\"outInteger\":"); resp.push_str(&json_i32_array(&outIntBuf0[..outNBElement]));
            resp.push('}');
            resp
        }
        "TA_CDLBREAKAWAY" => {
            let startIdx = params["startIdx"].as_u64().unwrap_or(0) as usize;
            let endIdx = params["endIdx"].as_u64().unwrap_or(0) as usize;
            let use_preloaded = params["use_preloaded"].as_i64().unwrap_or(0);
            let bench_iters = std::cmp::max(1, params["iters"].as_i64().unwrap_or(1)) as u64;
            let mut _json_inOpen: Vec<f64> = Vec::new();
            let mut _json_inHigh: Vec<f64> = Vec::new();
            let mut _json_inLow: Vec<f64> = Vec::new();
            let mut _json_inClose: Vec<f64> = Vec::new();
            let inOpen: &[f64];
            let inHigh: &[f64];
            let inLow: &[f64];
            let inClose: &[f64];
            if use_preloaded != 0 && ref_data.n > 0 {
                inOpen = &ref_data.open[..ref_data.n];
                inHigh = &ref_data.high[..ref_data.n];
                inLow = &ref_data.low[..ref_data.n];
                inClose = &ref_data.close[..ref_data.n];
            } else {
                _json_inOpen = parse_f64_array(&params["inOpen"]);
                inOpen = &_json_inOpen;
                _json_inHigh = parse_f64_array(&params["inHigh"]);
                inHigh = &_json_inHigh;
                _json_inLow = parse_f64_array(&params["inLow"]);
                inLow = &_json_inLow;
                _json_inClose = parse_f64_array(&params["inClose"]);
                inClose = &_json_inClose;
            }
            let out_size = if endIdx >= startIdx { endIdx - startIdx + 1 } else { 0 };
            let mut outIntBuf0: Vec<i32> = vec![0i32; out_size];
            let mut outBegIdx: usize = 0;
            let mut outNBElement: usize = 0;
            let mut rc = RetCode::Success;
            let start_time = Instant::now();
            for _bi in 0..bench_iters {
            rc = core.cdlbreakaway(
                startIdx, endIdx,
                &inOpen,
                &inHigh,
                &inLow,
                &inClose,
                &mut outBegIdx, &mut outNBElement, &mut outIntBuf0,
            );
            }
            let elapsed_ns = start_time.elapsed().as_nanos() as u64 / bench_iters as u64;
            let start_time_ung = Instant::now();
            for _biu in 0..bench_iters {
            rc = core.cdlbreakaway_unguarded(
                startIdx, endIdx,
                &inOpen,
                &inHigh,
                &inLow,
                &inClose,
                &mut outBegIdx, &mut outNBElement, &mut outIntBuf0,
            );
            }
            let elapsed_ns_ung = start_time_ung.elapsed().as_nanos() as u64 / bench_iters as u64;
            let lookback = core.cdlbreakaway_lookback();
            let mut resp = format!("{{\"retCode\":{},\"outBegIdx\":{},\"outNBElement\":{},\"lookback\":{},\"timing_ns\":{},\"timing_ns_unguarded\":{}", retcode_to_int(rc), outBegIdx, outNBElement, lookback, elapsed_ns, elapsed_ns_ung);
            resp.push_str(",\"outInteger\":"); resp.push_str(&json_i32_array(&outIntBuf0[..outNBElement]));
            resp.push('}');
            resp
        }
        "TA_CDLCLOSINGMARUBOZU" => {
            let startIdx = params["startIdx"].as_u64().unwrap_or(0) as usize;
            let endIdx = params["endIdx"].as_u64().unwrap_or(0) as usize;
            let use_preloaded = params["use_preloaded"].as_i64().unwrap_or(0);
            let bench_iters = std::cmp::max(1, params["iters"].as_i64().unwrap_or(1)) as u64;
            let mut _json_inOpen: Vec<f64> = Vec::new();
            let mut _json_inHigh: Vec<f64> = Vec::new();
            let mut _json_inLow: Vec<f64> = Vec::new();
            let mut _json_inClose: Vec<f64> = Vec::new();
            let inOpen: &[f64];
            let inHigh: &[f64];
            let inLow: &[f64];
            let inClose: &[f64];
            if use_preloaded != 0 && ref_data.n > 0 {
                inOpen = &ref_data.open[..ref_data.n];
                inHigh = &ref_data.high[..ref_data.n];
                inLow = &ref_data.low[..ref_data.n];
                inClose = &ref_data.close[..ref_data.n];
            } else {
                _json_inOpen = parse_f64_array(&params["inOpen"]);
                inOpen = &_json_inOpen;
                _json_inHigh = parse_f64_array(&params["inHigh"]);
                inHigh = &_json_inHigh;
                _json_inLow = parse_f64_array(&params["inLow"]);
                inLow = &_json_inLow;
                _json_inClose = parse_f64_array(&params["inClose"]);
                inClose = &_json_inClose;
            }
            let out_size = if endIdx >= startIdx { endIdx - startIdx + 1 } else { 0 };
            let mut outIntBuf0: Vec<i32> = vec![0i32; out_size];
            let mut outBegIdx: usize = 0;
            let mut outNBElement: usize = 0;
            let mut rc = RetCode::Success;
            let start_time = Instant::now();
            for _bi in 0..bench_iters {
            rc = core.cdlclosingmarubozu(
                startIdx, endIdx,
                &inOpen,
                &inHigh,
                &inLow,
                &inClose,
                &mut outBegIdx, &mut outNBElement, &mut outIntBuf0,
            );
            }
            let elapsed_ns = start_time.elapsed().as_nanos() as u64 / bench_iters as u64;
            let start_time_ung = Instant::now();
            for _biu in 0..bench_iters {
            rc = core.cdlclosingmarubozu_unguarded(
                startIdx, endIdx,
                &inOpen,
                &inHigh,
                &inLow,
                &inClose,
                &mut outBegIdx, &mut outNBElement, &mut outIntBuf0,
            );
            }
            let elapsed_ns_ung = start_time_ung.elapsed().as_nanos() as u64 / bench_iters as u64;
            let lookback = core.cdlclosingmarubozu_lookback();
            let mut resp = format!("{{\"retCode\":{},\"outBegIdx\":{},\"outNBElement\":{},\"lookback\":{},\"timing_ns\":{},\"timing_ns_unguarded\":{}", retcode_to_int(rc), outBegIdx, outNBElement, lookback, elapsed_ns, elapsed_ns_ung);
            resp.push_str(",\"outInteger\":"); resp.push_str(&json_i32_array(&outIntBuf0[..outNBElement]));
            resp.push('}');
            resp
        }
        "TA_CDLCONCEALBABYSWALL" => {
            let startIdx = params["startIdx"].as_u64().unwrap_or(0) as usize;
            let endIdx = params["endIdx"].as_u64().unwrap_or(0) as usize;
            let use_preloaded = params["use_preloaded"].as_i64().unwrap_or(0);
            let bench_iters = std::cmp::max(1, params["iters"].as_i64().unwrap_or(1)) as u64;
            let mut _json_inOpen: Vec<f64> = Vec::new();
            let mut _json_inHigh: Vec<f64> = Vec::new();
            let mut _json_inLow: Vec<f64> = Vec::new();
            let mut _json_inClose: Vec<f64> = Vec::new();
            let inOpen: &[f64];
            let inHigh: &[f64];
            let inLow: &[f64];
            let inClose: &[f64];
            if use_preloaded != 0 && ref_data.n > 0 {
                inOpen = &ref_data.open[..ref_data.n];
                inHigh = &ref_data.high[..ref_data.n];
                inLow = &ref_data.low[..ref_data.n];
                inClose = &ref_data.close[..ref_data.n];
            } else {
                _json_inOpen = parse_f64_array(&params["inOpen"]);
                inOpen = &_json_inOpen;
                _json_inHigh = parse_f64_array(&params["inHigh"]);
                inHigh = &_json_inHigh;
                _json_inLow = parse_f64_array(&params["inLow"]);
                inLow = &_json_inLow;
                _json_inClose = parse_f64_array(&params["inClose"]);
                inClose = &_json_inClose;
            }
            let out_size = if endIdx >= startIdx { endIdx - startIdx + 1 } else { 0 };
            let mut outIntBuf0: Vec<i32> = vec![0i32; out_size];
            let mut outBegIdx: usize = 0;
            let mut outNBElement: usize = 0;
            let mut rc = RetCode::Success;
            let start_time = Instant::now();
            for _bi in 0..bench_iters {
            rc = core.cdlconcealbabyswall(
                startIdx, endIdx,
                &inOpen,
                &inHigh,
                &inLow,
                &inClose,
                &mut outBegIdx, &mut outNBElement, &mut outIntBuf0,
            );
            }
            let elapsed_ns = start_time.elapsed().as_nanos() as u64 / bench_iters as u64;
            let start_time_ung = Instant::now();
            for _biu in 0..bench_iters {
            rc = core.cdlconcealbabyswall_unguarded(
                startIdx, endIdx,
                &inOpen,
                &inHigh,
                &inLow,
                &inClose,
                &mut outBegIdx, &mut outNBElement, &mut outIntBuf0,
            );
            }
            let elapsed_ns_ung = start_time_ung.elapsed().as_nanos() as u64 / bench_iters as u64;
            let lookback = core.cdlconcealbabyswall_lookback();
            let mut resp = format!("{{\"retCode\":{},\"outBegIdx\":{},\"outNBElement\":{},\"lookback\":{},\"timing_ns\":{},\"timing_ns_unguarded\":{}", retcode_to_int(rc), outBegIdx, outNBElement, lookback, elapsed_ns, elapsed_ns_ung);
            resp.push_str(",\"outInteger\":"); resp.push_str(&json_i32_array(&outIntBuf0[..outNBElement]));
            resp.push('}');
            resp
        }
        "TA_CDLCOUNTERATTACK" => {
            let startIdx = params["startIdx"].as_u64().unwrap_or(0) as usize;
            let endIdx = params["endIdx"].as_u64().unwrap_or(0) as usize;
            let use_preloaded = params["use_preloaded"].as_i64().unwrap_or(0);
            let bench_iters = std::cmp::max(1, params["iters"].as_i64().unwrap_or(1)) as u64;
            let mut _json_inOpen: Vec<f64> = Vec::new();
            let mut _json_inHigh: Vec<f64> = Vec::new();
            let mut _json_inLow: Vec<f64> = Vec::new();
            let mut _json_inClose: Vec<f64> = Vec::new();
            let inOpen: &[f64];
            let inHigh: &[f64];
            let inLow: &[f64];
            let inClose: &[f64];
            if use_preloaded != 0 && ref_data.n > 0 {
                inOpen = &ref_data.open[..ref_data.n];
                inHigh = &ref_data.high[..ref_data.n];
                inLow = &ref_data.low[..ref_data.n];
                inClose = &ref_data.close[..ref_data.n];
            } else {
                _json_inOpen = parse_f64_array(&params["inOpen"]);
                inOpen = &_json_inOpen;
                _json_inHigh = parse_f64_array(&params["inHigh"]);
                inHigh = &_json_inHigh;
                _json_inLow = parse_f64_array(&params["inLow"]);
                inLow = &_json_inLow;
                _json_inClose = parse_f64_array(&params["inClose"]);
                inClose = &_json_inClose;
            }
            let out_size = if endIdx >= startIdx { endIdx - startIdx + 1 } else { 0 };
            let mut outIntBuf0: Vec<i32> = vec![0i32; out_size];
            let mut outBegIdx: usize = 0;
            let mut outNBElement: usize = 0;
            let mut rc = RetCode::Success;
            let start_time = Instant::now();
            for _bi in 0..bench_iters {
            rc = core.cdlcounterattack(
                startIdx, endIdx,
                &inOpen,
                &inHigh,
                &inLow,
                &inClose,
                &mut outBegIdx, &mut outNBElement, &mut outIntBuf0,
            );
            }
            let elapsed_ns = start_time.elapsed().as_nanos() as u64 / bench_iters as u64;
            let start_time_ung = Instant::now();
            for _biu in 0..bench_iters {
            rc = core.cdlcounterattack_unguarded(
                startIdx, endIdx,
                &inOpen,
                &inHigh,
                &inLow,
                &inClose,
                &mut outBegIdx, &mut outNBElement, &mut outIntBuf0,
            );
            }
            let elapsed_ns_ung = start_time_ung.elapsed().as_nanos() as u64 / bench_iters as u64;
            let lookback = core.cdlcounterattack_lookback();
            let mut resp = format!("{{\"retCode\":{},\"outBegIdx\":{},\"outNBElement\":{},\"lookback\":{},\"timing_ns\":{},\"timing_ns_unguarded\":{}", retcode_to_int(rc), outBegIdx, outNBElement, lookback, elapsed_ns, elapsed_ns_ung);
            resp.push_str(",\"outInteger\":"); resp.push_str(&json_i32_array(&outIntBuf0[..outNBElement]));
            resp.push('}');
            resp
        }
        "TA_CDLDARKCLOUDCOVER" => {
            let startIdx = params["startIdx"].as_u64().unwrap_or(0) as usize;
            let endIdx = params["endIdx"].as_u64().unwrap_or(0) as usize;
            let use_preloaded = params["use_preloaded"].as_i64().unwrap_or(0);
            let bench_iters = std::cmp::max(1, params["iters"].as_i64().unwrap_or(1)) as u64;
            let mut _json_inOpen: Vec<f64> = Vec::new();
            let mut _json_inHigh: Vec<f64> = Vec::new();
            let mut _json_inLow: Vec<f64> = Vec::new();
            let mut _json_inClose: Vec<f64> = Vec::new();
            let inOpen: &[f64];
            let inHigh: &[f64];
            let inLow: &[f64];
            let inClose: &[f64];
            if use_preloaded != 0 && ref_data.n > 0 {
                inOpen = &ref_data.open[..ref_data.n];
                inHigh = &ref_data.high[..ref_data.n];
                inLow = &ref_data.low[..ref_data.n];
                inClose = &ref_data.close[..ref_data.n];
            } else {
                _json_inOpen = parse_f64_array(&params["inOpen"]);
                inOpen = &_json_inOpen;
                _json_inHigh = parse_f64_array(&params["inHigh"]);
                inHigh = &_json_inHigh;
                _json_inLow = parse_f64_array(&params["inLow"]);
                inLow = &_json_inLow;
                _json_inClose = parse_f64_array(&params["inClose"]);
                inClose = &_json_inClose;
            }
            let optInPenetration = params["optInPenetration"].as_f64().unwrap_or(0.5) as f64;
            let out_size = if endIdx >= startIdx { endIdx - startIdx + 1 } else { 0 };
            let mut outIntBuf0: Vec<i32> = vec![0i32; out_size];
            let mut outBegIdx: usize = 0;
            let mut outNBElement: usize = 0;
            let mut rc = RetCode::Success;
            let start_time = Instant::now();
            for _bi in 0..bench_iters {
            rc = core.cdldarkcloudcover(
                startIdx, endIdx,
                &inOpen,
                &inHigh,
                &inLow,
                &inClose,
                optInPenetration,
                &mut outBegIdx, &mut outNBElement, &mut outIntBuf0,
            );
            }
            let elapsed_ns = start_time.elapsed().as_nanos() as u64 / bench_iters as u64;
            let start_time_ung = Instant::now();
            for _biu in 0..bench_iters {
            rc = core.cdldarkcloudcover_unguarded(
                startIdx, endIdx,
                &inOpen,
                &inHigh,
                &inLow,
                &inClose,
                optInPenetration,
                &mut outBegIdx, &mut outNBElement, &mut outIntBuf0,
            );
            }
            let elapsed_ns_ung = start_time_ung.elapsed().as_nanos() as u64 / bench_iters as u64;
            let lookback = core.cdldarkcloudcover_lookback(optInPenetration);
            let mut resp = format!("{{\"retCode\":{},\"outBegIdx\":{},\"outNBElement\":{},\"lookback\":{},\"timing_ns\":{},\"timing_ns_unguarded\":{}", retcode_to_int(rc), outBegIdx, outNBElement, lookback, elapsed_ns, elapsed_ns_ung);
            resp.push_str(",\"outInteger\":"); resp.push_str(&json_i32_array(&outIntBuf0[..outNBElement]));
            resp.push('}');
            resp
        }
        "TA_CDLDOJI" => {
            let startIdx = params["startIdx"].as_u64().unwrap_or(0) as usize;
            let endIdx = params["endIdx"].as_u64().unwrap_or(0) as usize;
            let use_preloaded = params["use_preloaded"].as_i64().unwrap_or(0);
            let bench_iters = std::cmp::max(1, params["iters"].as_i64().unwrap_or(1)) as u64;
            let mut _json_inOpen: Vec<f64> = Vec::new();
            let mut _json_inHigh: Vec<f64> = Vec::new();
            let mut _json_inLow: Vec<f64> = Vec::new();
            let mut _json_inClose: Vec<f64> = Vec::new();
            let inOpen: &[f64];
            let inHigh: &[f64];
            let inLow: &[f64];
            let inClose: &[f64];
            if use_preloaded != 0 && ref_data.n > 0 {
                inOpen = &ref_data.open[..ref_data.n];
                inHigh = &ref_data.high[..ref_data.n];
                inLow = &ref_data.low[..ref_data.n];
                inClose = &ref_data.close[..ref_data.n];
            } else {
                _json_inOpen = parse_f64_array(&params["inOpen"]);
                inOpen = &_json_inOpen;
                _json_inHigh = parse_f64_array(&params["inHigh"]);
                inHigh = &_json_inHigh;
                _json_inLow = parse_f64_array(&params["inLow"]);
                inLow = &_json_inLow;
                _json_inClose = parse_f64_array(&params["inClose"]);
                inClose = &_json_inClose;
            }
            let out_size = if endIdx >= startIdx { endIdx - startIdx + 1 } else { 0 };
            let mut outIntBuf0: Vec<i32> = vec![0i32; out_size];
            let mut outBegIdx: usize = 0;
            let mut outNBElement: usize = 0;
            let mut rc = RetCode::Success;
            let start_time = Instant::now();
            for _bi in 0..bench_iters {
            rc = core.cdldoji(
                startIdx, endIdx,
                &inOpen,
                &inHigh,
                &inLow,
                &inClose,
                &mut outBegIdx, &mut outNBElement, &mut outIntBuf0,
            );
            }
            let elapsed_ns = start_time.elapsed().as_nanos() as u64 / bench_iters as u64;
            let start_time_ung = Instant::now();
            for _biu in 0..bench_iters {
            rc = core.cdldoji_unguarded(
                startIdx, endIdx,
                &inOpen,
                &inHigh,
                &inLow,
                &inClose,
                &mut outBegIdx, &mut outNBElement, &mut outIntBuf0,
            );
            }
            let elapsed_ns_ung = start_time_ung.elapsed().as_nanos() as u64 / bench_iters as u64;
            let lookback = core.cdldoji_lookback();
            let mut resp = format!("{{\"retCode\":{},\"outBegIdx\":{},\"outNBElement\":{},\"lookback\":{},\"timing_ns\":{},\"timing_ns_unguarded\":{}", retcode_to_int(rc), outBegIdx, outNBElement, lookback, elapsed_ns, elapsed_ns_ung);
            resp.push_str(",\"outInteger\":"); resp.push_str(&json_i32_array(&outIntBuf0[..outNBElement]));
            resp.push('}');
            resp
        }
        "TA_CDLDOJISTAR" => {
            let startIdx = params["startIdx"].as_u64().unwrap_or(0) as usize;
            let endIdx = params["endIdx"].as_u64().unwrap_or(0) as usize;
            let use_preloaded = params["use_preloaded"].as_i64().unwrap_or(0);
            let bench_iters = std::cmp::max(1, params["iters"].as_i64().unwrap_or(1)) as u64;
            let mut _json_inOpen: Vec<f64> = Vec::new();
            let mut _json_inHigh: Vec<f64> = Vec::new();
            let mut _json_inLow: Vec<f64> = Vec::new();
            let mut _json_inClose: Vec<f64> = Vec::new();
            let inOpen: &[f64];
            let inHigh: &[f64];
            let inLow: &[f64];
            let inClose: &[f64];
            if use_preloaded != 0 && ref_data.n > 0 {
                inOpen = &ref_data.open[..ref_data.n];
                inHigh = &ref_data.high[..ref_data.n];
                inLow = &ref_data.low[..ref_data.n];
                inClose = &ref_data.close[..ref_data.n];
            } else {
                _json_inOpen = parse_f64_array(&params["inOpen"]);
                inOpen = &_json_inOpen;
                _json_inHigh = parse_f64_array(&params["inHigh"]);
                inHigh = &_json_inHigh;
                _json_inLow = parse_f64_array(&params["inLow"]);
                inLow = &_json_inLow;
                _json_inClose = parse_f64_array(&params["inClose"]);
                inClose = &_json_inClose;
            }
            let out_size = if endIdx >= startIdx { endIdx - startIdx + 1 } else { 0 };
            let mut outIntBuf0: Vec<i32> = vec![0i32; out_size];
            let mut outBegIdx: usize = 0;
            let mut outNBElement: usize = 0;
            let mut rc = RetCode::Success;
            let start_time = Instant::now();
            for _bi in 0..bench_iters {
            rc = core.cdldojistar(
                startIdx, endIdx,
                &inOpen,
                &inHigh,
                &inLow,
                &inClose,
                &mut outBegIdx, &mut outNBElement, &mut outIntBuf0,
            );
            }
            let elapsed_ns = start_time.elapsed().as_nanos() as u64 / bench_iters as u64;
            let start_time_ung = Instant::now();
            for _biu in 0..bench_iters {
            rc = core.cdldojistar_unguarded(
                startIdx, endIdx,
                &inOpen,
                &inHigh,
                &inLow,
                &inClose,
                &mut outBegIdx, &mut outNBElement, &mut outIntBuf0,
            );
            }
            let elapsed_ns_ung = start_time_ung.elapsed().as_nanos() as u64 / bench_iters as u64;
            let lookback = core.cdldojistar_lookback();
            let mut resp = format!("{{\"retCode\":{},\"outBegIdx\":{},\"outNBElement\":{},\"lookback\":{},\"timing_ns\":{},\"timing_ns_unguarded\":{}", retcode_to_int(rc), outBegIdx, outNBElement, lookback, elapsed_ns, elapsed_ns_ung);
            resp.push_str(",\"outInteger\":"); resp.push_str(&json_i32_array(&outIntBuf0[..outNBElement]));
            resp.push('}');
            resp
        }
        "TA_CDLDRAGONFLYDOJI" => {
            let startIdx = params["startIdx"].as_u64().unwrap_or(0) as usize;
            let endIdx = params["endIdx"].as_u64().unwrap_or(0) as usize;
            let use_preloaded = params["use_preloaded"].as_i64().unwrap_or(0);
            let bench_iters = std::cmp::max(1, params["iters"].as_i64().unwrap_or(1)) as u64;
            let mut _json_inOpen: Vec<f64> = Vec::new();
            let mut _json_inHigh: Vec<f64> = Vec::new();
            let mut _json_inLow: Vec<f64> = Vec::new();
            let mut _json_inClose: Vec<f64> = Vec::new();
            let inOpen: &[f64];
            let inHigh: &[f64];
            let inLow: &[f64];
            let inClose: &[f64];
            if use_preloaded != 0 && ref_data.n > 0 {
                inOpen = &ref_data.open[..ref_data.n];
                inHigh = &ref_data.high[..ref_data.n];
                inLow = &ref_data.low[..ref_data.n];
                inClose = &ref_data.close[..ref_data.n];
            } else {
                _json_inOpen = parse_f64_array(&params["inOpen"]);
                inOpen = &_json_inOpen;
                _json_inHigh = parse_f64_array(&params["inHigh"]);
                inHigh = &_json_inHigh;
                _json_inLow = parse_f64_array(&params["inLow"]);
                inLow = &_json_inLow;
                _json_inClose = parse_f64_array(&params["inClose"]);
                inClose = &_json_inClose;
            }
            let out_size = if endIdx >= startIdx { endIdx - startIdx + 1 } else { 0 };
            let mut outIntBuf0: Vec<i32> = vec![0i32; out_size];
            let mut outBegIdx: usize = 0;
            let mut outNBElement: usize = 0;
            let mut rc = RetCode::Success;
            let start_time = Instant::now();
            for _bi in 0..bench_iters {
            rc = core.cdldragonflydoji(
                startIdx, endIdx,
                &inOpen,
                &inHigh,
                &inLow,
                &inClose,
                &mut outBegIdx, &mut outNBElement, &mut outIntBuf0,
            );
            }
            let elapsed_ns = start_time.elapsed().as_nanos() as u64 / bench_iters as u64;
            let start_time_ung = Instant::now();
            for _biu in 0..bench_iters {
            rc = core.cdldragonflydoji_unguarded(
                startIdx, endIdx,
                &inOpen,
                &inHigh,
                &inLow,
                &inClose,
                &mut outBegIdx, &mut outNBElement, &mut outIntBuf0,
            );
            }
            let elapsed_ns_ung = start_time_ung.elapsed().as_nanos() as u64 / bench_iters as u64;
            let lookback = core.cdldragonflydoji_lookback();
            let mut resp = format!("{{\"retCode\":{},\"outBegIdx\":{},\"outNBElement\":{},\"lookback\":{},\"timing_ns\":{},\"timing_ns_unguarded\":{}", retcode_to_int(rc), outBegIdx, outNBElement, lookback, elapsed_ns, elapsed_ns_ung);
            resp.push_str(",\"outInteger\":"); resp.push_str(&json_i32_array(&outIntBuf0[..outNBElement]));
            resp.push('}');
            resp
        }
        "TA_CDLENGULFING" => {
            let startIdx = params["startIdx"].as_u64().unwrap_or(0) as usize;
            let endIdx = params["endIdx"].as_u64().unwrap_or(0) as usize;
            let use_preloaded = params["use_preloaded"].as_i64().unwrap_or(0);
            let bench_iters = std::cmp::max(1, params["iters"].as_i64().unwrap_or(1)) as u64;
            let mut _json_inOpen: Vec<f64> = Vec::new();
            let mut _json_inHigh: Vec<f64> = Vec::new();
            let mut _json_inLow: Vec<f64> = Vec::new();
            let mut _json_inClose: Vec<f64> = Vec::new();
            let inOpen: &[f64];
            let inHigh: &[f64];
            let inLow: &[f64];
            let inClose: &[f64];
            if use_preloaded != 0 && ref_data.n > 0 {
                inOpen = &ref_data.open[..ref_data.n];
                inHigh = &ref_data.high[..ref_data.n];
                inLow = &ref_data.low[..ref_data.n];
                inClose = &ref_data.close[..ref_data.n];
            } else {
                _json_inOpen = parse_f64_array(&params["inOpen"]);
                inOpen = &_json_inOpen;
                _json_inHigh = parse_f64_array(&params["inHigh"]);
                inHigh = &_json_inHigh;
                _json_inLow = parse_f64_array(&params["inLow"]);
                inLow = &_json_inLow;
                _json_inClose = parse_f64_array(&params["inClose"]);
                inClose = &_json_inClose;
            }
            let out_size = if endIdx >= startIdx { endIdx - startIdx + 1 } else { 0 };
            let mut outIntBuf0: Vec<i32> = vec![0i32; out_size];
            let mut outBegIdx: usize = 0;
            let mut outNBElement: usize = 0;
            let mut rc = RetCode::Success;
            let start_time = Instant::now();
            for _bi in 0..bench_iters {
            rc = core.cdlengulfing(
                startIdx, endIdx,
                &inOpen,
                &inHigh,
                &inLow,
                &inClose,
                &mut outBegIdx, &mut outNBElement, &mut outIntBuf0,
            );
            }
            let elapsed_ns = start_time.elapsed().as_nanos() as u64 / bench_iters as u64;
            let start_time_ung = Instant::now();
            for _biu in 0..bench_iters {
            rc = core.cdlengulfing_unguarded(
                startIdx, endIdx,
                &inOpen,
                &inHigh,
                &inLow,
                &inClose,
                &mut outBegIdx, &mut outNBElement, &mut outIntBuf0,
            );
            }
            let elapsed_ns_ung = start_time_ung.elapsed().as_nanos() as u64 / bench_iters as u64;
            let lookback = core.cdlengulfing_lookback();
            let mut resp = format!("{{\"retCode\":{},\"outBegIdx\":{},\"outNBElement\":{},\"lookback\":{},\"timing_ns\":{},\"timing_ns_unguarded\":{}", retcode_to_int(rc), outBegIdx, outNBElement, lookback, elapsed_ns, elapsed_ns_ung);
            resp.push_str(",\"outInteger\":"); resp.push_str(&json_i32_array(&outIntBuf0[..outNBElement]));
            resp.push('}');
            resp
        }
        "TA_CDLEVENINGDOJISTAR" => {
            let startIdx = params["startIdx"].as_u64().unwrap_or(0) as usize;
            let endIdx = params["endIdx"].as_u64().unwrap_or(0) as usize;
            let use_preloaded = params["use_preloaded"].as_i64().unwrap_or(0);
            let bench_iters = std::cmp::max(1, params["iters"].as_i64().unwrap_or(1)) as u64;
            let mut _json_inOpen: Vec<f64> = Vec::new();
            let mut _json_inHigh: Vec<f64> = Vec::new();
            let mut _json_inLow: Vec<f64> = Vec::new();
            let mut _json_inClose: Vec<f64> = Vec::new();
            let inOpen: &[f64];
            let inHigh: &[f64];
            let inLow: &[f64];
            let inClose: &[f64];
            if use_preloaded != 0 && ref_data.n > 0 {
                inOpen = &ref_data.open[..ref_data.n];
                inHigh = &ref_data.high[..ref_data.n];
                inLow = &ref_data.low[..ref_data.n];
                inClose = &ref_data.close[..ref_data.n];
            } else {
                _json_inOpen = parse_f64_array(&params["inOpen"]);
                inOpen = &_json_inOpen;
                _json_inHigh = parse_f64_array(&params["inHigh"]);
                inHigh = &_json_inHigh;
                _json_inLow = parse_f64_array(&params["inLow"]);
                inLow = &_json_inLow;
                _json_inClose = parse_f64_array(&params["inClose"]);
                inClose = &_json_inClose;
            }
            let optInPenetration = params["optInPenetration"].as_f64().unwrap_or(0.3) as f64;
            let out_size = if endIdx >= startIdx { endIdx - startIdx + 1 } else { 0 };
            let mut outIntBuf0: Vec<i32> = vec![0i32; out_size];
            let mut outBegIdx: usize = 0;
            let mut outNBElement: usize = 0;
            let mut rc = RetCode::Success;
            let start_time = Instant::now();
            for _bi in 0..bench_iters {
            rc = core.cdleveningdojistar(
                startIdx, endIdx,
                &inOpen,
                &inHigh,
                &inLow,
                &inClose,
                optInPenetration,
                &mut outBegIdx, &mut outNBElement, &mut outIntBuf0,
            );
            }
            let elapsed_ns = start_time.elapsed().as_nanos() as u64 / bench_iters as u64;
            let start_time_ung = Instant::now();
            for _biu in 0..bench_iters {
            rc = core.cdleveningdojistar_unguarded(
                startIdx, endIdx,
                &inOpen,
                &inHigh,
                &inLow,
                &inClose,
                optInPenetration,
                &mut outBegIdx, &mut outNBElement, &mut outIntBuf0,
            );
            }
            let elapsed_ns_ung = start_time_ung.elapsed().as_nanos() as u64 / bench_iters as u64;
            let lookback = core.cdleveningdojistar_lookback(optInPenetration);
            let mut resp = format!("{{\"retCode\":{},\"outBegIdx\":{},\"outNBElement\":{},\"lookback\":{},\"timing_ns\":{},\"timing_ns_unguarded\":{}", retcode_to_int(rc), outBegIdx, outNBElement, lookback, elapsed_ns, elapsed_ns_ung);
            resp.push_str(",\"outInteger\":"); resp.push_str(&json_i32_array(&outIntBuf0[..outNBElement]));
            resp.push('}');
            resp
        }
        "TA_CDLEVENINGSTAR" => {
            let startIdx = params["startIdx"].as_u64().unwrap_or(0) as usize;
            let endIdx = params["endIdx"].as_u64().unwrap_or(0) as usize;
            let use_preloaded = params["use_preloaded"].as_i64().unwrap_or(0);
            let bench_iters = std::cmp::max(1, params["iters"].as_i64().unwrap_or(1)) as u64;
            let mut _json_inOpen: Vec<f64> = Vec::new();
            let mut _json_inHigh: Vec<f64> = Vec::new();
            let mut _json_inLow: Vec<f64> = Vec::new();
            let mut _json_inClose: Vec<f64> = Vec::new();
            let inOpen: &[f64];
            let inHigh: &[f64];
            let inLow: &[f64];
            let inClose: &[f64];
            if use_preloaded != 0 && ref_data.n > 0 {
                inOpen = &ref_data.open[..ref_data.n];
                inHigh = &ref_data.high[..ref_data.n];
                inLow = &ref_data.low[..ref_data.n];
                inClose = &ref_data.close[..ref_data.n];
            } else {
                _json_inOpen = parse_f64_array(&params["inOpen"]);
                inOpen = &_json_inOpen;
                _json_inHigh = parse_f64_array(&params["inHigh"]);
                inHigh = &_json_inHigh;
                _json_inLow = parse_f64_array(&params["inLow"]);
                inLow = &_json_inLow;
                _json_inClose = parse_f64_array(&params["inClose"]);
                inClose = &_json_inClose;
            }
            let optInPenetration = params["optInPenetration"].as_f64().unwrap_or(0.3) as f64;
            let out_size = if endIdx >= startIdx { endIdx - startIdx + 1 } else { 0 };
            let mut outIntBuf0: Vec<i32> = vec![0i32; out_size];
            let mut outBegIdx: usize = 0;
            let mut outNBElement: usize = 0;
            let mut rc = RetCode::Success;
            let start_time = Instant::now();
            for _bi in 0..bench_iters {
            rc = core.cdleveningstar(
                startIdx, endIdx,
                &inOpen,
                &inHigh,
                &inLow,
                &inClose,
                optInPenetration,
                &mut outBegIdx, &mut outNBElement, &mut outIntBuf0,
            );
            }
            let elapsed_ns = start_time.elapsed().as_nanos() as u64 / bench_iters as u64;
            let start_time_ung = Instant::now();
            for _biu in 0..bench_iters {
            rc = core.cdleveningstar_unguarded(
                startIdx, endIdx,
                &inOpen,
                &inHigh,
                &inLow,
                &inClose,
                optInPenetration,
                &mut outBegIdx, &mut outNBElement, &mut outIntBuf0,
            );
            }
            let elapsed_ns_ung = start_time_ung.elapsed().as_nanos() as u64 / bench_iters as u64;
            let lookback = core.cdleveningstar_lookback(optInPenetration);
            let mut resp = format!("{{\"retCode\":{},\"outBegIdx\":{},\"outNBElement\":{},\"lookback\":{},\"timing_ns\":{},\"timing_ns_unguarded\":{}", retcode_to_int(rc), outBegIdx, outNBElement, lookback, elapsed_ns, elapsed_ns_ung);
            resp.push_str(",\"outInteger\":"); resp.push_str(&json_i32_array(&outIntBuf0[..outNBElement]));
            resp.push('}');
            resp
        }
        "TA_CDLGAPSIDESIDEWHITE" => {
            let startIdx = params["startIdx"].as_u64().unwrap_or(0) as usize;
            let endIdx = params["endIdx"].as_u64().unwrap_or(0) as usize;
            let use_preloaded = params["use_preloaded"].as_i64().unwrap_or(0);
            let bench_iters = std::cmp::max(1, params["iters"].as_i64().unwrap_or(1)) as u64;
            let mut _json_inOpen: Vec<f64> = Vec::new();
            let mut _json_inHigh: Vec<f64> = Vec::new();
            let mut _json_inLow: Vec<f64> = Vec::new();
            let mut _json_inClose: Vec<f64> = Vec::new();
            let inOpen: &[f64];
            let inHigh: &[f64];
            let inLow: &[f64];
            let inClose: &[f64];
            if use_preloaded != 0 && ref_data.n > 0 {
                inOpen = &ref_data.open[..ref_data.n];
                inHigh = &ref_data.high[..ref_data.n];
                inLow = &ref_data.low[..ref_data.n];
                inClose = &ref_data.close[..ref_data.n];
            } else {
                _json_inOpen = parse_f64_array(&params["inOpen"]);
                inOpen = &_json_inOpen;
                _json_inHigh = parse_f64_array(&params["inHigh"]);
                inHigh = &_json_inHigh;
                _json_inLow = parse_f64_array(&params["inLow"]);
                inLow = &_json_inLow;
                _json_inClose = parse_f64_array(&params["inClose"]);
                inClose = &_json_inClose;
            }
            let out_size = if endIdx >= startIdx { endIdx - startIdx + 1 } else { 0 };
            let mut outIntBuf0: Vec<i32> = vec![0i32; out_size];
            let mut outBegIdx: usize = 0;
            let mut outNBElement: usize = 0;
            let mut rc = RetCode::Success;
            let start_time = Instant::now();
            for _bi in 0..bench_iters {
            rc = core.cdlgapsidesidewhite(
                startIdx, endIdx,
                &inOpen,
                &inHigh,
                &inLow,
                &inClose,
                &mut outBegIdx, &mut outNBElement, &mut outIntBuf0,
            );
            }
            let elapsed_ns = start_time.elapsed().as_nanos() as u64 / bench_iters as u64;
            let start_time_ung = Instant::now();
            for _biu in 0..bench_iters {
            rc = core.cdlgapsidesidewhite_unguarded(
                startIdx, endIdx,
                &inOpen,
                &inHigh,
                &inLow,
                &inClose,
                &mut outBegIdx, &mut outNBElement, &mut outIntBuf0,
            );
            }
            let elapsed_ns_ung = start_time_ung.elapsed().as_nanos() as u64 / bench_iters as u64;
            let lookback = core.cdlgapsidesidewhite_lookback();
            let mut resp = format!("{{\"retCode\":{},\"outBegIdx\":{},\"outNBElement\":{},\"lookback\":{},\"timing_ns\":{},\"timing_ns_unguarded\":{}", retcode_to_int(rc), outBegIdx, outNBElement, lookback, elapsed_ns, elapsed_ns_ung);
            resp.push_str(",\"outInteger\":"); resp.push_str(&json_i32_array(&outIntBuf0[..outNBElement]));
            resp.push('}');
            resp
        }
        "TA_CDLGRAVESTONEDOJI" => {
            let startIdx = params["startIdx"].as_u64().unwrap_or(0) as usize;
            let endIdx = params["endIdx"].as_u64().unwrap_or(0) as usize;
            let use_preloaded = params["use_preloaded"].as_i64().unwrap_or(0);
            let bench_iters = std::cmp::max(1, params["iters"].as_i64().unwrap_or(1)) as u64;
            let mut _json_inOpen: Vec<f64> = Vec::new();
            let mut _json_inHigh: Vec<f64> = Vec::new();
            let mut _json_inLow: Vec<f64> = Vec::new();
            let mut _json_inClose: Vec<f64> = Vec::new();
            let inOpen: &[f64];
            let inHigh: &[f64];
            let inLow: &[f64];
            let inClose: &[f64];
            if use_preloaded != 0 && ref_data.n > 0 {
                inOpen = &ref_data.open[..ref_data.n];
                inHigh = &ref_data.high[..ref_data.n];
                inLow = &ref_data.low[..ref_data.n];
                inClose = &ref_data.close[..ref_data.n];
            } else {
                _json_inOpen = parse_f64_array(&params["inOpen"]);
                inOpen = &_json_inOpen;
                _json_inHigh = parse_f64_array(&params["inHigh"]);
                inHigh = &_json_inHigh;
                _json_inLow = parse_f64_array(&params["inLow"]);
                inLow = &_json_inLow;
                _json_inClose = parse_f64_array(&params["inClose"]);
                inClose = &_json_inClose;
            }
            let out_size = if endIdx >= startIdx { endIdx - startIdx + 1 } else { 0 };
            let mut outIntBuf0: Vec<i32> = vec![0i32; out_size];
            let mut outBegIdx: usize = 0;
            let mut outNBElement: usize = 0;
            let mut rc = RetCode::Success;
            let start_time = Instant::now();
            for _bi in 0..bench_iters {
            rc = core.cdlgravestonedoji(
                startIdx, endIdx,
                &inOpen,
                &inHigh,
                &inLow,
                &inClose,
                &mut outBegIdx, &mut outNBElement, &mut outIntBuf0,
            );
            }
            let elapsed_ns = start_time.elapsed().as_nanos() as u64 / bench_iters as u64;
            let start_time_ung = Instant::now();
            for _biu in 0..bench_iters {
            rc = core.cdlgravestonedoji_unguarded(
                startIdx, endIdx,
                &inOpen,
                &inHigh,
                &inLow,
                &inClose,
                &mut outBegIdx, &mut outNBElement, &mut outIntBuf0,
            );
            }
            let elapsed_ns_ung = start_time_ung.elapsed().as_nanos() as u64 / bench_iters as u64;
            let lookback = core.cdlgravestonedoji_lookback();
            let mut resp = format!("{{\"retCode\":{},\"outBegIdx\":{},\"outNBElement\":{},\"lookback\":{},\"timing_ns\":{},\"timing_ns_unguarded\":{}", retcode_to_int(rc), outBegIdx, outNBElement, lookback, elapsed_ns, elapsed_ns_ung);
            resp.push_str(",\"outInteger\":"); resp.push_str(&json_i32_array(&outIntBuf0[..outNBElement]));
            resp.push('}');
            resp
        }
        "TA_CDLHAMMER" => {
            let startIdx = params["startIdx"].as_u64().unwrap_or(0) as usize;
            let endIdx = params["endIdx"].as_u64().unwrap_or(0) as usize;
            let use_preloaded = params["use_preloaded"].as_i64().unwrap_or(0);
            let bench_iters = std::cmp::max(1, params["iters"].as_i64().unwrap_or(1)) as u64;
            let mut _json_inOpen: Vec<f64> = Vec::new();
            let mut _json_inHigh: Vec<f64> = Vec::new();
            let mut _json_inLow: Vec<f64> = Vec::new();
            let mut _json_inClose: Vec<f64> = Vec::new();
            let inOpen: &[f64];
            let inHigh: &[f64];
            let inLow: &[f64];
            let inClose: &[f64];
            if use_preloaded != 0 && ref_data.n > 0 {
                inOpen = &ref_data.open[..ref_data.n];
                inHigh = &ref_data.high[..ref_data.n];
                inLow = &ref_data.low[..ref_data.n];
                inClose = &ref_data.close[..ref_data.n];
            } else {
                _json_inOpen = parse_f64_array(&params["inOpen"]);
                inOpen = &_json_inOpen;
                _json_inHigh = parse_f64_array(&params["inHigh"]);
                inHigh = &_json_inHigh;
                _json_inLow = parse_f64_array(&params["inLow"]);
                inLow = &_json_inLow;
                _json_inClose = parse_f64_array(&params["inClose"]);
                inClose = &_json_inClose;
            }
            let out_size = if endIdx >= startIdx { endIdx - startIdx + 1 } else { 0 };
            let mut outIntBuf0: Vec<i32> = vec![0i32; out_size];
            let mut outBegIdx: usize = 0;
            let mut outNBElement: usize = 0;
            let mut rc = RetCode::Success;
            let start_time = Instant::now();
            for _bi in 0..bench_iters {
            rc = core.cdlhammer(
                startIdx, endIdx,
                &inOpen,
                &inHigh,
                &inLow,
                &inClose,
                &mut outBegIdx, &mut outNBElement, &mut outIntBuf0,
            );
            }
            let elapsed_ns = start_time.elapsed().as_nanos() as u64 / bench_iters as u64;
            let start_time_ung = Instant::now();
            for _biu in 0..bench_iters {
            rc = core.cdlhammer_unguarded(
                startIdx, endIdx,
                &inOpen,
                &inHigh,
                &inLow,
                &inClose,
                &mut outBegIdx, &mut outNBElement, &mut outIntBuf0,
            );
            }
            let elapsed_ns_ung = start_time_ung.elapsed().as_nanos() as u64 / bench_iters as u64;
            let lookback = core.cdlhammer_lookback();
            let mut resp = format!("{{\"retCode\":{},\"outBegIdx\":{},\"outNBElement\":{},\"lookback\":{},\"timing_ns\":{},\"timing_ns_unguarded\":{}", retcode_to_int(rc), outBegIdx, outNBElement, lookback, elapsed_ns, elapsed_ns_ung);
            resp.push_str(",\"outInteger\":"); resp.push_str(&json_i32_array(&outIntBuf0[..outNBElement]));
            resp.push('}');
            resp
        }
        "TA_CDLHANGINGMAN" => {
            let startIdx = params["startIdx"].as_u64().unwrap_or(0) as usize;
            let endIdx = params["endIdx"].as_u64().unwrap_or(0) as usize;
            let use_preloaded = params["use_preloaded"].as_i64().unwrap_or(0);
            let bench_iters = std::cmp::max(1, params["iters"].as_i64().unwrap_or(1)) as u64;
            let mut _json_inOpen: Vec<f64> = Vec::new();
            let mut _json_inHigh: Vec<f64> = Vec::new();
            let mut _json_inLow: Vec<f64> = Vec::new();
            let mut _json_inClose: Vec<f64> = Vec::new();
            let inOpen: &[f64];
            let inHigh: &[f64];
            let inLow: &[f64];
            let inClose: &[f64];
            if use_preloaded != 0 && ref_data.n > 0 {
                inOpen = &ref_data.open[..ref_data.n];
                inHigh = &ref_data.high[..ref_data.n];
                inLow = &ref_data.low[..ref_data.n];
                inClose = &ref_data.close[..ref_data.n];
            } else {
                _json_inOpen = parse_f64_array(&params["inOpen"]);
                inOpen = &_json_inOpen;
                _json_inHigh = parse_f64_array(&params["inHigh"]);
                inHigh = &_json_inHigh;
                _json_inLow = parse_f64_array(&params["inLow"]);
                inLow = &_json_inLow;
                _json_inClose = parse_f64_array(&params["inClose"]);
                inClose = &_json_inClose;
            }
            let out_size = if endIdx >= startIdx { endIdx - startIdx + 1 } else { 0 };
            let mut outIntBuf0: Vec<i32> = vec![0i32; out_size];
            let mut outBegIdx: usize = 0;
            let mut outNBElement: usize = 0;
            let mut rc = RetCode::Success;
            let start_time = Instant::now();
            for _bi in 0..bench_iters {
            rc = core.cdlhangingman(
                startIdx, endIdx,
                &inOpen,
                &inHigh,
                &inLow,
                &inClose,
                &mut outBegIdx, &mut outNBElement, &mut outIntBuf0,
            );
            }
            let elapsed_ns = start_time.elapsed().as_nanos() as u64 / bench_iters as u64;
            let start_time_ung = Instant::now();
            for _biu in 0..bench_iters {
            rc = core.cdlhangingman_unguarded(
                startIdx, endIdx,
                &inOpen,
                &inHigh,
                &inLow,
                &inClose,
                &mut outBegIdx, &mut outNBElement, &mut outIntBuf0,
            );
            }
            let elapsed_ns_ung = start_time_ung.elapsed().as_nanos() as u64 / bench_iters as u64;
            let lookback = core.cdlhangingman_lookback();
            let mut resp = format!("{{\"retCode\":{},\"outBegIdx\":{},\"outNBElement\":{},\"lookback\":{},\"timing_ns\":{},\"timing_ns_unguarded\":{}", retcode_to_int(rc), outBegIdx, outNBElement, lookback, elapsed_ns, elapsed_ns_ung);
            resp.push_str(",\"outInteger\":"); resp.push_str(&json_i32_array(&outIntBuf0[..outNBElement]));
            resp.push('}');
            resp
        }
        "TA_CDLHARAMI" => {
            let startIdx = params["startIdx"].as_u64().unwrap_or(0) as usize;
            let endIdx = params["endIdx"].as_u64().unwrap_or(0) as usize;
            let use_preloaded = params["use_preloaded"].as_i64().unwrap_or(0);
            let bench_iters = std::cmp::max(1, params["iters"].as_i64().unwrap_or(1)) as u64;
            let mut _json_inOpen: Vec<f64> = Vec::new();
            let mut _json_inHigh: Vec<f64> = Vec::new();
            let mut _json_inLow: Vec<f64> = Vec::new();
            let mut _json_inClose: Vec<f64> = Vec::new();
            let inOpen: &[f64];
            let inHigh: &[f64];
            let inLow: &[f64];
            let inClose: &[f64];
            if use_preloaded != 0 && ref_data.n > 0 {
                inOpen = &ref_data.open[..ref_data.n];
                inHigh = &ref_data.high[..ref_data.n];
                inLow = &ref_data.low[..ref_data.n];
                inClose = &ref_data.close[..ref_data.n];
            } else {
                _json_inOpen = parse_f64_array(&params["inOpen"]);
                inOpen = &_json_inOpen;
                _json_inHigh = parse_f64_array(&params["inHigh"]);
                inHigh = &_json_inHigh;
                _json_inLow = parse_f64_array(&params["inLow"]);
                inLow = &_json_inLow;
                _json_inClose = parse_f64_array(&params["inClose"]);
                inClose = &_json_inClose;
            }
            let out_size = if endIdx >= startIdx { endIdx - startIdx + 1 } else { 0 };
            let mut outIntBuf0: Vec<i32> = vec![0i32; out_size];
            let mut outBegIdx: usize = 0;
            let mut outNBElement: usize = 0;
            let mut rc = RetCode::Success;
            let start_time = Instant::now();
            for _bi in 0..bench_iters {
            rc = core.cdlharami(
                startIdx, endIdx,
                &inOpen,
                &inHigh,
                &inLow,
                &inClose,
                &mut outBegIdx, &mut outNBElement, &mut outIntBuf0,
            );
            }
            let elapsed_ns = start_time.elapsed().as_nanos() as u64 / bench_iters as u64;
            let start_time_ung = Instant::now();
            for _biu in 0..bench_iters {
            rc = core.cdlharami_unguarded(
                startIdx, endIdx,
                &inOpen,
                &inHigh,
                &inLow,
                &inClose,
                &mut outBegIdx, &mut outNBElement, &mut outIntBuf0,
            );
            }
            let elapsed_ns_ung = start_time_ung.elapsed().as_nanos() as u64 / bench_iters as u64;
            let lookback = core.cdlharami_lookback();
            let mut resp = format!("{{\"retCode\":{},\"outBegIdx\":{},\"outNBElement\":{},\"lookback\":{},\"timing_ns\":{},\"timing_ns_unguarded\":{}", retcode_to_int(rc), outBegIdx, outNBElement, lookback, elapsed_ns, elapsed_ns_ung);
            resp.push_str(",\"outInteger\":"); resp.push_str(&json_i32_array(&outIntBuf0[..outNBElement]));
            resp.push('}');
            resp
        }
        "TA_CDLHARAMICROSS" => {
            let startIdx = params["startIdx"].as_u64().unwrap_or(0) as usize;
            let endIdx = params["endIdx"].as_u64().unwrap_or(0) as usize;
            let use_preloaded = params["use_preloaded"].as_i64().unwrap_or(0);
            let bench_iters = std::cmp::max(1, params["iters"].as_i64().unwrap_or(1)) as u64;
            let mut _json_inOpen: Vec<f64> = Vec::new();
            let mut _json_inHigh: Vec<f64> = Vec::new();
            let mut _json_inLow: Vec<f64> = Vec::new();
            let mut _json_inClose: Vec<f64> = Vec::new();
            let inOpen: &[f64];
            let inHigh: &[f64];
            let inLow: &[f64];
            let inClose: &[f64];
            if use_preloaded != 0 && ref_data.n > 0 {
                inOpen = &ref_data.open[..ref_data.n];
                inHigh = &ref_data.high[..ref_data.n];
                inLow = &ref_data.low[..ref_data.n];
                inClose = &ref_data.close[..ref_data.n];
            } else {
                _json_inOpen = parse_f64_array(&params["inOpen"]);
                inOpen = &_json_inOpen;
                _json_inHigh = parse_f64_array(&params["inHigh"]);
                inHigh = &_json_inHigh;
                _json_inLow = parse_f64_array(&params["inLow"]);
                inLow = &_json_inLow;
                _json_inClose = parse_f64_array(&params["inClose"]);
                inClose = &_json_inClose;
            }
            let out_size = if endIdx >= startIdx { endIdx - startIdx + 1 } else { 0 };
            let mut outIntBuf0: Vec<i32> = vec![0i32; out_size];
            let mut outBegIdx: usize = 0;
            let mut outNBElement: usize = 0;
            let mut rc = RetCode::Success;
            let start_time = Instant::now();
            for _bi in 0..bench_iters {
            rc = core.cdlharamicross(
                startIdx, endIdx,
                &inOpen,
                &inHigh,
                &inLow,
                &inClose,
                &mut outBegIdx, &mut outNBElement, &mut outIntBuf0,
            );
            }
            let elapsed_ns = start_time.elapsed().as_nanos() as u64 / bench_iters as u64;
            let start_time_ung = Instant::now();
            for _biu in 0..bench_iters {
            rc = core.cdlharamicross_unguarded(
                startIdx, endIdx,
                &inOpen,
                &inHigh,
                &inLow,
                &inClose,
                &mut outBegIdx, &mut outNBElement, &mut outIntBuf0,
            );
            }
            let elapsed_ns_ung = start_time_ung.elapsed().as_nanos() as u64 / bench_iters as u64;
            let lookback = core.cdlharamicross_lookback();
            let mut resp = format!("{{\"retCode\":{},\"outBegIdx\":{},\"outNBElement\":{},\"lookback\":{},\"timing_ns\":{},\"timing_ns_unguarded\":{}", retcode_to_int(rc), outBegIdx, outNBElement, lookback, elapsed_ns, elapsed_ns_ung);
            resp.push_str(",\"outInteger\":"); resp.push_str(&json_i32_array(&outIntBuf0[..outNBElement]));
            resp.push('}');
            resp
        }
        "TA_CDLHIGHWAVE" => {
            let startIdx = params["startIdx"].as_u64().unwrap_or(0) as usize;
            let endIdx = params["endIdx"].as_u64().unwrap_or(0) as usize;
            let use_preloaded = params["use_preloaded"].as_i64().unwrap_or(0);
            let bench_iters = std::cmp::max(1, params["iters"].as_i64().unwrap_or(1)) as u64;
            let mut _json_inOpen: Vec<f64> = Vec::new();
            let mut _json_inHigh: Vec<f64> = Vec::new();
            let mut _json_inLow: Vec<f64> = Vec::new();
            let mut _json_inClose: Vec<f64> = Vec::new();
            let inOpen: &[f64];
            let inHigh: &[f64];
            let inLow: &[f64];
            let inClose: &[f64];
            if use_preloaded != 0 && ref_data.n > 0 {
                inOpen = &ref_data.open[..ref_data.n];
                inHigh = &ref_data.high[..ref_data.n];
                inLow = &ref_data.low[..ref_data.n];
                inClose = &ref_data.close[..ref_data.n];
            } else {
                _json_inOpen = parse_f64_array(&params["inOpen"]);
                inOpen = &_json_inOpen;
                _json_inHigh = parse_f64_array(&params["inHigh"]);
                inHigh = &_json_inHigh;
                _json_inLow = parse_f64_array(&params["inLow"]);
                inLow = &_json_inLow;
                _json_inClose = parse_f64_array(&params["inClose"]);
                inClose = &_json_inClose;
            }
            let out_size = if endIdx >= startIdx { endIdx - startIdx + 1 } else { 0 };
            let mut outIntBuf0: Vec<i32> = vec![0i32; out_size];
            let mut outBegIdx: usize = 0;
            let mut outNBElement: usize = 0;
            let mut rc = RetCode::Success;
            let start_time = Instant::now();
            for _bi in 0..bench_iters {
            rc = core.cdlhighwave(
                startIdx, endIdx,
                &inOpen,
                &inHigh,
                &inLow,
                &inClose,
                &mut outBegIdx, &mut outNBElement, &mut outIntBuf0,
            );
            }
            let elapsed_ns = start_time.elapsed().as_nanos() as u64 / bench_iters as u64;
            let start_time_ung = Instant::now();
            for _biu in 0..bench_iters {
            rc = core.cdlhighwave_unguarded(
                startIdx, endIdx,
                &inOpen,
                &inHigh,
                &inLow,
                &inClose,
                &mut outBegIdx, &mut outNBElement, &mut outIntBuf0,
            );
            }
            let elapsed_ns_ung = start_time_ung.elapsed().as_nanos() as u64 / bench_iters as u64;
            let lookback = core.cdlhighwave_lookback();
            let mut resp = format!("{{\"retCode\":{},\"outBegIdx\":{},\"outNBElement\":{},\"lookback\":{},\"timing_ns\":{},\"timing_ns_unguarded\":{}", retcode_to_int(rc), outBegIdx, outNBElement, lookback, elapsed_ns, elapsed_ns_ung);
            resp.push_str(",\"outInteger\":"); resp.push_str(&json_i32_array(&outIntBuf0[..outNBElement]));
            resp.push('}');
            resp
        }
        "TA_CDLHIKKAKE" => {
            let startIdx = params["startIdx"].as_u64().unwrap_or(0) as usize;
            let endIdx = params["endIdx"].as_u64().unwrap_or(0) as usize;
            let use_preloaded = params["use_preloaded"].as_i64().unwrap_or(0);
            let bench_iters = std::cmp::max(1, params["iters"].as_i64().unwrap_or(1)) as u64;
            let mut _json_inOpen: Vec<f64> = Vec::new();
            let mut _json_inHigh: Vec<f64> = Vec::new();
            let mut _json_inLow: Vec<f64> = Vec::new();
            let mut _json_inClose: Vec<f64> = Vec::new();
            let inOpen: &[f64];
            let inHigh: &[f64];
            let inLow: &[f64];
            let inClose: &[f64];
            if use_preloaded != 0 && ref_data.n > 0 {
                inOpen = &ref_data.open[..ref_data.n];
                inHigh = &ref_data.high[..ref_data.n];
                inLow = &ref_data.low[..ref_data.n];
                inClose = &ref_data.close[..ref_data.n];
            } else {
                _json_inOpen = parse_f64_array(&params["inOpen"]);
                inOpen = &_json_inOpen;
                _json_inHigh = parse_f64_array(&params["inHigh"]);
                inHigh = &_json_inHigh;
                _json_inLow = parse_f64_array(&params["inLow"]);
                inLow = &_json_inLow;
                _json_inClose = parse_f64_array(&params["inClose"]);
                inClose = &_json_inClose;
            }
            let out_size = if endIdx >= startIdx { endIdx - startIdx + 1 } else { 0 };
            let mut outIntBuf0: Vec<i32> = vec![0i32; out_size];
            let mut outBegIdx: usize = 0;
            let mut outNBElement: usize = 0;
            let mut rc = RetCode::Success;
            let start_time = Instant::now();
            for _bi in 0..bench_iters {
            rc = core.cdlhikkake(
                startIdx, endIdx,
                &inOpen,
                &inHigh,
                &inLow,
                &inClose,
                &mut outBegIdx, &mut outNBElement, &mut outIntBuf0,
            );
            }
            let elapsed_ns = start_time.elapsed().as_nanos() as u64 / bench_iters as u64;
            let start_time_ung = Instant::now();
            for _biu in 0..bench_iters {
            rc = core.cdlhikkake_unguarded(
                startIdx, endIdx,
                &inOpen,
                &inHigh,
                &inLow,
                &inClose,
                &mut outBegIdx, &mut outNBElement, &mut outIntBuf0,
            );
            }
            let elapsed_ns_ung = start_time_ung.elapsed().as_nanos() as u64 / bench_iters as u64;
            let lookback = core.cdlhikkake_lookback();
            let mut resp = format!("{{\"retCode\":{},\"outBegIdx\":{},\"outNBElement\":{},\"lookback\":{},\"timing_ns\":{},\"timing_ns_unguarded\":{}", retcode_to_int(rc), outBegIdx, outNBElement, lookback, elapsed_ns, elapsed_ns_ung);
            resp.push_str(",\"outInteger\":"); resp.push_str(&json_i32_array(&outIntBuf0[..outNBElement]));
            resp.push('}');
            resp
        }
        "TA_CDLHIKKAKEMOD" => {
            let startIdx = params["startIdx"].as_u64().unwrap_or(0) as usize;
            let endIdx = params["endIdx"].as_u64().unwrap_or(0) as usize;
            let use_preloaded = params["use_preloaded"].as_i64().unwrap_or(0);
            let bench_iters = std::cmp::max(1, params["iters"].as_i64().unwrap_or(1)) as u64;
            let mut _json_inOpen: Vec<f64> = Vec::new();
            let mut _json_inHigh: Vec<f64> = Vec::new();
            let mut _json_inLow: Vec<f64> = Vec::new();
            let mut _json_inClose: Vec<f64> = Vec::new();
            let inOpen: &[f64];
            let inHigh: &[f64];
            let inLow: &[f64];
            let inClose: &[f64];
            if use_preloaded != 0 && ref_data.n > 0 {
                inOpen = &ref_data.open[..ref_data.n];
                inHigh = &ref_data.high[..ref_data.n];
                inLow = &ref_data.low[..ref_data.n];
                inClose = &ref_data.close[..ref_data.n];
            } else {
                _json_inOpen = parse_f64_array(&params["inOpen"]);
                inOpen = &_json_inOpen;
                _json_inHigh = parse_f64_array(&params["inHigh"]);
                inHigh = &_json_inHigh;
                _json_inLow = parse_f64_array(&params["inLow"]);
                inLow = &_json_inLow;
                _json_inClose = parse_f64_array(&params["inClose"]);
                inClose = &_json_inClose;
            }
            let out_size = if endIdx >= startIdx { endIdx - startIdx + 1 } else { 0 };
            let mut outIntBuf0: Vec<i32> = vec![0i32; out_size];
            let mut outBegIdx: usize = 0;
            let mut outNBElement: usize = 0;
            let mut rc = RetCode::Success;
            let start_time = Instant::now();
            for _bi in 0..bench_iters {
            rc = core.cdlhikkakemod(
                startIdx, endIdx,
                &inOpen,
                &inHigh,
                &inLow,
                &inClose,
                &mut outBegIdx, &mut outNBElement, &mut outIntBuf0,
            );
            }
            let elapsed_ns = start_time.elapsed().as_nanos() as u64 / bench_iters as u64;
            let start_time_ung = Instant::now();
            for _biu in 0..bench_iters {
            rc = core.cdlhikkakemod_unguarded(
                startIdx, endIdx,
                &inOpen,
                &inHigh,
                &inLow,
                &inClose,
                &mut outBegIdx, &mut outNBElement, &mut outIntBuf0,
            );
            }
            let elapsed_ns_ung = start_time_ung.elapsed().as_nanos() as u64 / bench_iters as u64;
            let lookback = core.cdlhikkakemod_lookback();
            let mut resp = format!("{{\"retCode\":{},\"outBegIdx\":{},\"outNBElement\":{},\"lookback\":{},\"timing_ns\":{},\"timing_ns_unguarded\":{}", retcode_to_int(rc), outBegIdx, outNBElement, lookback, elapsed_ns, elapsed_ns_ung);
            resp.push_str(",\"outInteger\":"); resp.push_str(&json_i32_array(&outIntBuf0[..outNBElement]));
            resp.push('}');
            resp
        }
        "TA_CDLHOMINGPIGEON" => {
            let startIdx = params["startIdx"].as_u64().unwrap_or(0) as usize;
            let endIdx = params["endIdx"].as_u64().unwrap_or(0) as usize;
            let use_preloaded = params["use_preloaded"].as_i64().unwrap_or(0);
            let bench_iters = std::cmp::max(1, params["iters"].as_i64().unwrap_or(1)) as u64;
            let mut _json_inOpen: Vec<f64> = Vec::new();
            let mut _json_inHigh: Vec<f64> = Vec::new();
            let mut _json_inLow: Vec<f64> = Vec::new();
            let mut _json_inClose: Vec<f64> = Vec::new();
            let inOpen: &[f64];
            let inHigh: &[f64];
            let inLow: &[f64];
            let inClose: &[f64];
            if use_preloaded != 0 && ref_data.n > 0 {
                inOpen = &ref_data.open[..ref_data.n];
                inHigh = &ref_data.high[..ref_data.n];
                inLow = &ref_data.low[..ref_data.n];
                inClose = &ref_data.close[..ref_data.n];
            } else {
                _json_inOpen = parse_f64_array(&params["inOpen"]);
                inOpen = &_json_inOpen;
                _json_inHigh = parse_f64_array(&params["inHigh"]);
                inHigh = &_json_inHigh;
                _json_inLow = parse_f64_array(&params["inLow"]);
                inLow = &_json_inLow;
                _json_inClose = parse_f64_array(&params["inClose"]);
                inClose = &_json_inClose;
            }
            let out_size = if endIdx >= startIdx { endIdx - startIdx + 1 } else { 0 };
            let mut outIntBuf0: Vec<i32> = vec![0i32; out_size];
            let mut outBegIdx: usize = 0;
            let mut outNBElement: usize = 0;
            let mut rc = RetCode::Success;
            let start_time = Instant::now();
            for _bi in 0..bench_iters {
            rc = core.cdlhomingpigeon(
                startIdx, endIdx,
                &inOpen,
                &inHigh,
                &inLow,
                &inClose,
                &mut outBegIdx, &mut outNBElement, &mut outIntBuf0,
            );
            }
            let elapsed_ns = start_time.elapsed().as_nanos() as u64 / bench_iters as u64;
            let start_time_ung = Instant::now();
            for _biu in 0..bench_iters {
            rc = core.cdlhomingpigeon_unguarded(
                startIdx, endIdx,
                &inOpen,
                &inHigh,
                &inLow,
                &inClose,
                &mut outBegIdx, &mut outNBElement, &mut outIntBuf0,
            );
            }
            let elapsed_ns_ung = start_time_ung.elapsed().as_nanos() as u64 / bench_iters as u64;
            let lookback = core.cdlhomingpigeon_lookback();
            let mut resp = format!("{{\"retCode\":{},\"outBegIdx\":{},\"outNBElement\":{},\"lookback\":{},\"timing_ns\":{},\"timing_ns_unguarded\":{}", retcode_to_int(rc), outBegIdx, outNBElement, lookback, elapsed_ns, elapsed_ns_ung);
            resp.push_str(",\"outInteger\":"); resp.push_str(&json_i32_array(&outIntBuf0[..outNBElement]));
            resp.push('}');
            resp
        }
        "TA_CDLIDENTICAL3CROWS" => {
            let startIdx = params["startIdx"].as_u64().unwrap_or(0) as usize;
            let endIdx = params["endIdx"].as_u64().unwrap_or(0) as usize;
            let use_preloaded = params["use_preloaded"].as_i64().unwrap_or(0);
            let bench_iters = std::cmp::max(1, params["iters"].as_i64().unwrap_or(1)) as u64;
            let mut _json_inOpen: Vec<f64> = Vec::new();
            let mut _json_inHigh: Vec<f64> = Vec::new();
            let mut _json_inLow: Vec<f64> = Vec::new();
            let mut _json_inClose: Vec<f64> = Vec::new();
            let inOpen: &[f64];
            let inHigh: &[f64];
            let inLow: &[f64];
            let inClose: &[f64];
            if use_preloaded != 0 && ref_data.n > 0 {
                inOpen = &ref_data.open[..ref_data.n];
                inHigh = &ref_data.high[..ref_data.n];
                inLow = &ref_data.low[..ref_data.n];
                inClose = &ref_data.close[..ref_data.n];
            } else {
                _json_inOpen = parse_f64_array(&params["inOpen"]);
                inOpen = &_json_inOpen;
                _json_inHigh = parse_f64_array(&params["inHigh"]);
                inHigh = &_json_inHigh;
                _json_inLow = parse_f64_array(&params["inLow"]);
                inLow = &_json_inLow;
                _json_inClose = parse_f64_array(&params["inClose"]);
                inClose = &_json_inClose;
            }
            let out_size = if endIdx >= startIdx { endIdx - startIdx + 1 } else { 0 };
            let mut outIntBuf0: Vec<i32> = vec![0i32; out_size];
            let mut outBegIdx: usize = 0;
            let mut outNBElement: usize = 0;
            let mut rc = RetCode::Success;
            let start_time = Instant::now();
            for _bi in 0..bench_iters {
            rc = core.cdlidentical3crows(
                startIdx, endIdx,
                &inOpen,
                &inHigh,
                &inLow,
                &inClose,
                &mut outBegIdx, &mut outNBElement, &mut outIntBuf0,
            );
            }
            let elapsed_ns = start_time.elapsed().as_nanos() as u64 / bench_iters as u64;
            let start_time_ung = Instant::now();
            for _biu in 0..bench_iters {
            rc = core.cdlidentical3crows_unguarded(
                startIdx, endIdx,
                &inOpen,
                &inHigh,
                &inLow,
                &inClose,
                &mut outBegIdx, &mut outNBElement, &mut outIntBuf0,
            );
            }
            let elapsed_ns_ung = start_time_ung.elapsed().as_nanos() as u64 / bench_iters as u64;
            let lookback = core.cdlidentical3crows_lookback();
            let mut resp = format!("{{\"retCode\":{},\"outBegIdx\":{},\"outNBElement\":{},\"lookback\":{},\"timing_ns\":{},\"timing_ns_unguarded\":{}", retcode_to_int(rc), outBegIdx, outNBElement, lookback, elapsed_ns, elapsed_ns_ung);
            resp.push_str(",\"outInteger\":"); resp.push_str(&json_i32_array(&outIntBuf0[..outNBElement]));
            resp.push('}');
            resp
        }
        "TA_CDLINNECK" => {
            let startIdx = params["startIdx"].as_u64().unwrap_or(0) as usize;
            let endIdx = params["endIdx"].as_u64().unwrap_or(0) as usize;
            let use_preloaded = params["use_preloaded"].as_i64().unwrap_or(0);
            let bench_iters = std::cmp::max(1, params["iters"].as_i64().unwrap_or(1)) as u64;
            let mut _json_inOpen: Vec<f64> = Vec::new();
            let mut _json_inHigh: Vec<f64> = Vec::new();
            let mut _json_inLow: Vec<f64> = Vec::new();
            let mut _json_inClose: Vec<f64> = Vec::new();
            let inOpen: &[f64];
            let inHigh: &[f64];
            let inLow: &[f64];
            let inClose: &[f64];
            if use_preloaded != 0 && ref_data.n > 0 {
                inOpen = &ref_data.open[..ref_data.n];
                inHigh = &ref_data.high[..ref_data.n];
                inLow = &ref_data.low[..ref_data.n];
                inClose = &ref_data.close[..ref_data.n];
            } else {
                _json_inOpen = parse_f64_array(&params["inOpen"]);
                inOpen = &_json_inOpen;
                _json_inHigh = parse_f64_array(&params["inHigh"]);
                inHigh = &_json_inHigh;
                _json_inLow = parse_f64_array(&params["inLow"]);
                inLow = &_json_inLow;
                _json_inClose = parse_f64_array(&params["inClose"]);
                inClose = &_json_inClose;
            }
            let out_size = if endIdx >= startIdx { endIdx - startIdx + 1 } else { 0 };
            let mut outIntBuf0: Vec<i32> = vec![0i32; out_size];
            let mut outBegIdx: usize = 0;
            let mut outNBElement: usize = 0;
            let mut rc = RetCode::Success;
            let start_time = Instant::now();
            for _bi in 0..bench_iters {
            rc = core.cdlinneck(
                startIdx, endIdx,
                &inOpen,
                &inHigh,
                &inLow,
                &inClose,
                &mut outBegIdx, &mut outNBElement, &mut outIntBuf0,
            );
            }
            let elapsed_ns = start_time.elapsed().as_nanos() as u64 / bench_iters as u64;
            let start_time_ung = Instant::now();
            for _biu in 0..bench_iters {
            rc = core.cdlinneck_unguarded(
                startIdx, endIdx,
                &inOpen,
                &inHigh,
                &inLow,
                &inClose,
                &mut outBegIdx, &mut outNBElement, &mut outIntBuf0,
            );
            }
            let elapsed_ns_ung = start_time_ung.elapsed().as_nanos() as u64 / bench_iters as u64;
            let lookback = core.cdlinneck_lookback();
            let mut resp = format!("{{\"retCode\":{},\"outBegIdx\":{},\"outNBElement\":{},\"lookback\":{},\"timing_ns\":{},\"timing_ns_unguarded\":{}", retcode_to_int(rc), outBegIdx, outNBElement, lookback, elapsed_ns, elapsed_ns_ung);
            resp.push_str(",\"outInteger\":"); resp.push_str(&json_i32_array(&outIntBuf0[..outNBElement]));
            resp.push('}');
            resp
        }
        "TA_CDLINVERTEDHAMMER" => {
            let startIdx = params["startIdx"].as_u64().unwrap_or(0) as usize;
            let endIdx = params["endIdx"].as_u64().unwrap_or(0) as usize;
            let use_preloaded = params["use_preloaded"].as_i64().unwrap_or(0);
            let bench_iters = std::cmp::max(1, params["iters"].as_i64().unwrap_or(1)) as u64;
            let mut _json_inOpen: Vec<f64> = Vec::new();
            let mut _json_inHigh: Vec<f64> = Vec::new();
            let mut _json_inLow: Vec<f64> = Vec::new();
            let mut _json_inClose: Vec<f64> = Vec::new();
            let inOpen: &[f64];
            let inHigh: &[f64];
            let inLow: &[f64];
            let inClose: &[f64];
            if use_preloaded != 0 && ref_data.n > 0 {
                inOpen = &ref_data.open[..ref_data.n];
                inHigh = &ref_data.high[..ref_data.n];
                inLow = &ref_data.low[..ref_data.n];
                inClose = &ref_data.close[..ref_data.n];
            } else {
                _json_inOpen = parse_f64_array(&params["inOpen"]);
                inOpen = &_json_inOpen;
                _json_inHigh = parse_f64_array(&params["inHigh"]);
                inHigh = &_json_inHigh;
                _json_inLow = parse_f64_array(&params["inLow"]);
                inLow = &_json_inLow;
                _json_inClose = parse_f64_array(&params["inClose"]);
                inClose = &_json_inClose;
            }
            let out_size = if endIdx >= startIdx { endIdx - startIdx + 1 } else { 0 };
            let mut outIntBuf0: Vec<i32> = vec![0i32; out_size];
            let mut outBegIdx: usize = 0;
            let mut outNBElement: usize = 0;
            let mut rc = RetCode::Success;
            let start_time = Instant::now();
            for _bi in 0..bench_iters {
            rc = core.cdlinvertedhammer(
                startIdx, endIdx,
                &inOpen,
                &inHigh,
                &inLow,
                &inClose,
                &mut outBegIdx, &mut outNBElement, &mut outIntBuf0,
            );
            }
            let elapsed_ns = start_time.elapsed().as_nanos() as u64 / bench_iters as u64;
            let start_time_ung = Instant::now();
            for _biu in 0..bench_iters {
            rc = core.cdlinvertedhammer_unguarded(
                startIdx, endIdx,
                &inOpen,
                &inHigh,
                &inLow,
                &inClose,
                &mut outBegIdx, &mut outNBElement, &mut outIntBuf0,
            );
            }
            let elapsed_ns_ung = start_time_ung.elapsed().as_nanos() as u64 / bench_iters as u64;
            let lookback = core.cdlinvertedhammer_lookback();
            let mut resp = format!("{{\"retCode\":{},\"outBegIdx\":{},\"outNBElement\":{},\"lookback\":{},\"timing_ns\":{},\"timing_ns_unguarded\":{}", retcode_to_int(rc), outBegIdx, outNBElement, lookback, elapsed_ns, elapsed_ns_ung);
            resp.push_str(",\"outInteger\":"); resp.push_str(&json_i32_array(&outIntBuf0[..outNBElement]));
            resp.push('}');
            resp
        }
        "TA_CDLKICKING" => {
            let startIdx = params["startIdx"].as_u64().unwrap_or(0) as usize;
            let endIdx = params["endIdx"].as_u64().unwrap_or(0) as usize;
            let use_preloaded = params["use_preloaded"].as_i64().unwrap_or(0);
            let bench_iters = std::cmp::max(1, params["iters"].as_i64().unwrap_or(1)) as u64;
            let mut _json_inOpen: Vec<f64> = Vec::new();
            let mut _json_inHigh: Vec<f64> = Vec::new();
            let mut _json_inLow: Vec<f64> = Vec::new();
            let mut _json_inClose: Vec<f64> = Vec::new();
            let inOpen: &[f64];
            let inHigh: &[f64];
            let inLow: &[f64];
            let inClose: &[f64];
            if use_preloaded != 0 && ref_data.n > 0 {
                inOpen = &ref_data.open[..ref_data.n];
                inHigh = &ref_data.high[..ref_data.n];
                inLow = &ref_data.low[..ref_data.n];
                inClose = &ref_data.close[..ref_data.n];
            } else {
                _json_inOpen = parse_f64_array(&params["inOpen"]);
                inOpen = &_json_inOpen;
                _json_inHigh = parse_f64_array(&params["inHigh"]);
                inHigh = &_json_inHigh;
                _json_inLow = parse_f64_array(&params["inLow"]);
                inLow = &_json_inLow;
                _json_inClose = parse_f64_array(&params["inClose"]);
                inClose = &_json_inClose;
            }
            let out_size = if endIdx >= startIdx { endIdx - startIdx + 1 } else { 0 };
            let mut outIntBuf0: Vec<i32> = vec![0i32; out_size];
            let mut outBegIdx: usize = 0;
            let mut outNBElement: usize = 0;
            let mut rc = RetCode::Success;
            let start_time = Instant::now();
            for _bi in 0..bench_iters {
            rc = core.cdlkicking(
                startIdx, endIdx,
                &inOpen,
                &inHigh,
                &inLow,
                &inClose,
                &mut outBegIdx, &mut outNBElement, &mut outIntBuf0,
            );
            }
            let elapsed_ns = start_time.elapsed().as_nanos() as u64 / bench_iters as u64;
            let start_time_ung = Instant::now();
            for _biu in 0..bench_iters {
            rc = core.cdlkicking_unguarded(
                startIdx, endIdx,
                &inOpen,
                &inHigh,
                &inLow,
                &inClose,
                &mut outBegIdx, &mut outNBElement, &mut outIntBuf0,
            );
            }
            let elapsed_ns_ung = start_time_ung.elapsed().as_nanos() as u64 / bench_iters as u64;
            let lookback = core.cdlkicking_lookback();
            let mut resp = format!("{{\"retCode\":{},\"outBegIdx\":{},\"outNBElement\":{},\"lookback\":{},\"timing_ns\":{},\"timing_ns_unguarded\":{}", retcode_to_int(rc), outBegIdx, outNBElement, lookback, elapsed_ns, elapsed_ns_ung);
            resp.push_str(",\"outInteger\":"); resp.push_str(&json_i32_array(&outIntBuf0[..outNBElement]));
            resp.push('}');
            resp
        }
        "TA_CDLKICKINGBYLENGTH" => {
            let startIdx = params["startIdx"].as_u64().unwrap_or(0) as usize;
            let endIdx = params["endIdx"].as_u64().unwrap_or(0) as usize;
            let use_preloaded = params["use_preloaded"].as_i64().unwrap_or(0);
            let bench_iters = std::cmp::max(1, params["iters"].as_i64().unwrap_or(1)) as u64;
            let mut _json_inOpen: Vec<f64> = Vec::new();
            let mut _json_inHigh: Vec<f64> = Vec::new();
            let mut _json_inLow: Vec<f64> = Vec::new();
            let mut _json_inClose: Vec<f64> = Vec::new();
            let inOpen: &[f64];
            let inHigh: &[f64];
            let inLow: &[f64];
            let inClose: &[f64];
            if use_preloaded != 0 && ref_data.n > 0 {
                inOpen = &ref_data.open[..ref_data.n];
                inHigh = &ref_data.high[..ref_data.n];
                inLow = &ref_data.low[..ref_data.n];
                inClose = &ref_data.close[..ref_data.n];
            } else {
                _json_inOpen = parse_f64_array(&params["inOpen"]);
                inOpen = &_json_inOpen;
                _json_inHigh = parse_f64_array(&params["inHigh"]);
                inHigh = &_json_inHigh;
                _json_inLow = parse_f64_array(&params["inLow"]);
                inLow = &_json_inLow;
                _json_inClose = parse_f64_array(&params["inClose"]);
                inClose = &_json_inClose;
            }
            let out_size = if endIdx >= startIdx { endIdx - startIdx + 1 } else { 0 };
            let mut outIntBuf0: Vec<i32> = vec![0i32; out_size];
            let mut outBegIdx: usize = 0;
            let mut outNBElement: usize = 0;
            let mut rc = RetCode::Success;
            let start_time = Instant::now();
            for _bi in 0..bench_iters {
            rc = core.cdlkickingbylength(
                startIdx, endIdx,
                &inOpen,
                &inHigh,
                &inLow,
                &inClose,
                &mut outBegIdx, &mut outNBElement, &mut outIntBuf0,
            );
            }
            let elapsed_ns = start_time.elapsed().as_nanos() as u64 / bench_iters as u64;
            let start_time_ung = Instant::now();
            for _biu in 0..bench_iters {
            rc = core.cdlkickingbylength_unguarded(
                startIdx, endIdx,
                &inOpen,
                &inHigh,
                &inLow,
                &inClose,
                &mut outBegIdx, &mut outNBElement, &mut outIntBuf0,
            );
            }
            let elapsed_ns_ung = start_time_ung.elapsed().as_nanos() as u64 / bench_iters as u64;
            let lookback = core.cdlkickingbylength_lookback();
            let mut resp = format!("{{\"retCode\":{},\"outBegIdx\":{},\"outNBElement\":{},\"lookback\":{},\"timing_ns\":{},\"timing_ns_unguarded\":{}", retcode_to_int(rc), outBegIdx, outNBElement, lookback, elapsed_ns, elapsed_ns_ung);
            resp.push_str(",\"outInteger\":"); resp.push_str(&json_i32_array(&outIntBuf0[..outNBElement]));
            resp.push('}');
            resp
        }
        "TA_CDLLADDERBOTTOM" => {
            let startIdx = params["startIdx"].as_u64().unwrap_or(0) as usize;
            let endIdx = params["endIdx"].as_u64().unwrap_or(0) as usize;
            let use_preloaded = params["use_preloaded"].as_i64().unwrap_or(0);
            let bench_iters = std::cmp::max(1, params["iters"].as_i64().unwrap_or(1)) as u64;
            let mut _json_inOpen: Vec<f64> = Vec::new();
            let mut _json_inHigh: Vec<f64> = Vec::new();
            let mut _json_inLow: Vec<f64> = Vec::new();
            let mut _json_inClose: Vec<f64> = Vec::new();
            let inOpen: &[f64];
            let inHigh: &[f64];
            let inLow: &[f64];
            let inClose: &[f64];
            if use_preloaded != 0 && ref_data.n > 0 {
                inOpen = &ref_data.open[..ref_data.n];
                inHigh = &ref_data.high[..ref_data.n];
                inLow = &ref_data.low[..ref_data.n];
                inClose = &ref_data.close[..ref_data.n];
            } else {
                _json_inOpen = parse_f64_array(&params["inOpen"]);
                inOpen = &_json_inOpen;
                _json_inHigh = parse_f64_array(&params["inHigh"]);
                inHigh = &_json_inHigh;
                _json_inLow = parse_f64_array(&params["inLow"]);
                inLow = &_json_inLow;
                _json_inClose = parse_f64_array(&params["inClose"]);
                inClose = &_json_inClose;
            }
            let out_size = if endIdx >= startIdx { endIdx - startIdx + 1 } else { 0 };
            let mut outIntBuf0: Vec<i32> = vec![0i32; out_size];
            let mut outBegIdx: usize = 0;
            let mut outNBElement: usize = 0;
            let mut rc = RetCode::Success;
            let start_time = Instant::now();
            for _bi in 0..bench_iters {
            rc = core.cdlladderbottom(
                startIdx, endIdx,
                &inOpen,
                &inHigh,
                &inLow,
                &inClose,
                &mut outBegIdx, &mut outNBElement, &mut outIntBuf0,
            );
            }
            let elapsed_ns = start_time.elapsed().as_nanos() as u64 / bench_iters as u64;
            let start_time_ung = Instant::now();
            for _biu in 0..bench_iters {
            rc = core.cdlladderbottom_unguarded(
                startIdx, endIdx,
                &inOpen,
                &inHigh,
                &inLow,
                &inClose,
                &mut outBegIdx, &mut outNBElement, &mut outIntBuf0,
            );
            }
            let elapsed_ns_ung = start_time_ung.elapsed().as_nanos() as u64 / bench_iters as u64;
            let lookback = core.cdlladderbottom_lookback();
            let mut resp = format!("{{\"retCode\":{},\"outBegIdx\":{},\"outNBElement\":{},\"lookback\":{},\"timing_ns\":{},\"timing_ns_unguarded\":{}", retcode_to_int(rc), outBegIdx, outNBElement, lookback, elapsed_ns, elapsed_ns_ung);
            resp.push_str(",\"outInteger\":"); resp.push_str(&json_i32_array(&outIntBuf0[..outNBElement]));
            resp.push('}');
            resp
        }
        "TA_CDLLONGLEGGEDDOJI" => {
            let startIdx = params["startIdx"].as_u64().unwrap_or(0) as usize;
            let endIdx = params["endIdx"].as_u64().unwrap_or(0) as usize;
            let use_preloaded = params["use_preloaded"].as_i64().unwrap_or(0);
            let bench_iters = std::cmp::max(1, params["iters"].as_i64().unwrap_or(1)) as u64;
            let mut _json_inOpen: Vec<f64> = Vec::new();
            let mut _json_inHigh: Vec<f64> = Vec::new();
            let mut _json_inLow: Vec<f64> = Vec::new();
            let mut _json_inClose: Vec<f64> = Vec::new();
            let inOpen: &[f64];
            let inHigh: &[f64];
            let inLow: &[f64];
            let inClose: &[f64];
            if use_preloaded != 0 && ref_data.n > 0 {
                inOpen = &ref_data.open[..ref_data.n];
                inHigh = &ref_data.high[..ref_data.n];
                inLow = &ref_data.low[..ref_data.n];
                inClose = &ref_data.close[..ref_data.n];
            } else {
                _json_inOpen = parse_f64_array(&params["inOpen"]);
                inOpen = &_json_inOpen;
                _json_inHigh = parse_f64_array(&params["inHigh"]);
                inHigh = &_json_inHigh;
                _json_inLow = parse_f64_array(&params["inLow"]);
                inLow = &_json_inLow;
                _json_inClose = parse_f64_array(&params["inClose"]);
                inClose = &_json_inClose;
            }
            let out_size = if endIdx >= startIdx { endIdx - startIdx + 1 } else { 0 };
            let mut outIntBuf0: Vec<i32> = vec![0i32; out_size];
            let mut outBegIdx: usize = 0;
            let mut outNBElement: usize = 0;
            let mut rc = RetCode::Success;
            let start_time = Instant::now();
            for _bi in 0..bench_iters {
            rc = core.cdllongleggeddoji(
                startIdx, endIdx,
                &inOpen,
                &inHigh,
                &inLow,
                &inClose,
                &mut outBegIdx, &mut outNBElement, &mut outIntBuf0,
            );
            }
            let elapsed_ns = start_time.elapsed().as_nanos() as u64 / bench_iters as u64;
            let start_time_ung = Instant::now();
            for _biu in 0..bench_iters {
            rc = core.cdllongleggeddoji_unguarded(
                startIdx, endIdx,
                &inOpen,
                &inHigh,
                &inLow,
                &inClose,
                &mut outBegIdx, &mut outNBElement, &mut outIntBuf0,
            );
            }
            let elapsed_ns_ung = start_time_ung.elapsed().as_nanos() as u64 / bench_iters as u64;
            let lookback = core.cdllongleggeddoji_lookback();
            let mut resp = format!("{{\"retCode\":{},\"outBegIdx\":{},\"outNBElement\":{},\"lookback\":{},\"timing_ns\":{},\"timing_ns_unguarded\":{}", retcode_to_int(rc), outBegIdx, outNBElement, lookback, elapsed_ns, elapsed_ns_ung);
            resp.push_str(",\"outInteger\":"); resp.push_str(&json_i32_array(&outIntBuf0[..outNBElement]));
            resp.push('}');
            resp
        }
        "TA_CDLLONGLINE" => {
            let startIdx = params["startIdx"].as_u64().unwrap_or(0) as usize;
            let endIdx = params["endIdx"].as_u64().unwrap_or(0) as usize;
            let use_preloaded = params["use_preloaded"].as_i64().unwrap_or(0);
            let bench_iters = std::cmp::max(1, params["iters"].as_i64().unwrap_or(1)) as u64;
            let mut _json_inOpen: Vec<f64> = Vec::new();
            let mut _json_inHigh: Vec<f64> = Vec::new();
            let mut _json_inLow: Vec<f64> = Vec::new();
            let mut _json_inClose: Vec<f64> = Vec::new();
            let inOpen: &[f64];
            let inHigh: &[f64];
            let inLow: &[f64];
            let inClose: &[f64];
            if use_preloaded != 0 && ref_data.n > 0 {
                inOpen = &ref_data.open[..ref_data.n];
                inHigh = &ref_data.high[..ref_data.n];
                inLow = &ref_data.low[..ref_data.n];
                inClose = &ref_data.close[..ref_data.n];
            } else {
                _json_inOpen = parse_f64_array(&params["inOpen"]);
                inOpen = &_json_inOpen;
                _json_inHigh = parse_f64_array(&params["inHigh"]);
                inHigh = &_json_inHigh;
                _json_inLow = parse_f64_array(&params["inLow"]);
                inLow = &_json_inLow;
                _json_inClose = parse_f64_array(&params["inClose"]);
                inClose = &_json_inClose;
            }
            let out_size = if endIdx >= startIdx { endIdx - startIdx + 1 } else { 0 };
            let mut outIntBuf0: Vec<i32> = vec![0i32; out_size];
            let mut outBegIdx: usize = 0;
            let mut outNBElement: usize = 0;
            let mut rc = RetCode::Success;
            let start_time = Instant::now();
            for _bi in 0..bench_iters {
            rc = core.cdllongline(
                startIdx, endIdx,
                &inOpen,
                &inHigh,
                &inLow,
                &inClose,
                &mut outBegIdx, &mut outNBElement, &mut outIntBuf0,
            );
            }
            let elapsed_ns = start_time.elapsed().as_nanos() as u64 / bench_iters as u64;
            let start_time_ung = Instant::now();
            for _biu in 0..bench_iters {
            rc = core.cdllongline_unguarded(
                startIdx, endIdx,
                &inOpen,
                &inHigh,
                &inLow,
                &inClose,
                &mut outBegIdx, &mut outNBElement, &mut outIntBuf0,
            );
            }
            let elapsed_ns_ung = start_time_ung.elapsed().as_nanos() as u64 / bench_iters as u64;
            let lookback = core.cdllongline_lookback();
            let mut resp = format!("{{\"retCode\":{},\"outBegIdx\":{},\"outNBElement\":{},\"lookback\":{},\"timing_ns\":{},\"timing_ns_unguarded\":{}", retcode_to_int(rc), outBegIdx, outNBElement, lookback, elapsed_ns, elapsed_ns_ung);
            resp.push_str(",\"outInteger\":"); resp.push_str(&json_i32_array(&outIntBuf0[..outNBElement]));
            resp.push('}');
            resp
        }
        "TA_CDLMARUBOZU" => {
            let startIdx = params["startIdx"].as_u64().unwrap_or(0) as usize;
            let endIdx = params["endIdx"].as_u64().unwrap_or(0) as usize;
            let use_preloaded = params["use_preloaded"].as_i64().unwrap_or(0);
            let bench_iters = std::cmp::max(1, params["iters"].as_i64().unwrap_or(1)) as u64;
            let mut _json_inOpen: Vec<f64> = Vec::new();
            let mut _json_inHigh: Vec<f64> = Vec::new();
            let mut _json_inLow: Vec<f64> = Vec::new();
            let mut _json_inClose: Vec<f64> = Vec::new();
            let inOpen: &[f64];
            let inHigh: &[f64];
            let inLow: &[f64];
            let inClose: &[f64];
            if use_preloaded != 0 && ref_data.n > 0 {
                inOpen = &ref_data.open[..ref_data.n];
                inHigh = &ref_data.high[..ref_data.n];
                inLow = &ref_data.low[..ref_data.n];
                inClose = &ref_data.close[..ref_data.n];
            } else {
                _json_inOpen = parse_f64_array(&params["inOpen"]);
                inOpen = &_json_inOpen;
                _json_inHigh = parse_f64_array(&params["inHigh"]);
                inHigh = &_json_inHigh;
                _json_inLow = parse_f64_array(&params["inLow"]);
                inLow = &_json_inLow;
                _json_inClose = parse_f64_array(&params["inClose"]);
                inClose = &_json_inClose;
            }
            let out_size = if endIdx >= startIdx { endIdx - startIdx + 1 } else { 0 };
            let mut outIntBuf0: Vec<i32> = vec![0i32; out_size];
            let mut outBegIdx: usize = 0;
            let mut outNBElement: usize = 0;
            let mut rc = RetCode::Success;
            let start_time = Instant::now();
            for _bi in 0..bench_iters {
            rc = core.cdlmarubozu(
                startIdx, endIdx,
                &inOpen,
                &inHigh,
                &inLow,
                &inClose,
                &mut outBegIdx, &mut outNBElement, &mut outIntBuf0,
            );
            }
            let elapsed_ns = start_time.elapsed().as_nanos() as u64 / bench_iters as u64;
            let start_time_ung = Instant::now();
            for _biu in 0..bench_iters {
            rc = core.cdlmarubozu_unguarded(
                startIdx, endIdx,
                &inOpen,
                &inHigh,
                &inLow,
                &inClose,
                &mut outBegIdx, &mut outNBElement, &mut outIntBuf0,
            );
            }
            let elapsed_ns_ung = start_time_ung.elapsed().as_nanos() as u64 / bench_iters as u64;
            let lookback = core.cdlmarubozu_lookback();
            let mut resp = format!("{{\"retCode\":{},\"outBegIdx\":{},\"outNBElement\":{},\"lookback\":{},\"timing_ns\":{},\"timing_ns_unguarded\":{}", retcode_to_int(rc), outBegIdx, outNBElement, lookback, elapsed_ns, elapsed_ns_ung);
            resp.push_str(",\"outInteger\":"); resp.push_str(&json_i32_array(&outIntBuf0[..outNBElement]));
            resp.push('}');
            resp
        }
        "TA_CDLMATCHINGLOW" => {
            let startIdx = params["startIdx"].as_u64().unwrap_or(0) as usize;
            let endIdx = params["endIdx"].as_u64().unwrap_or(0) as usize;
            let use_preloaded = params["use_preloaded"].as_i64().unwrap_or(0);
            let bench_iters = std::cmp::max(1, params["iters"].as_i64().unwrap_or(1)) as u64;
            let mut _json_inOpen: Vec<f64> = Vec::new();
            let mut _json_inHigh: Vec<f64> = Vec::new();
            let mut _json_inLow: Vec<f64> = Vec::new();
            let mut _json_inClose: Vec<f64> = Vec::new();
            let inOpen: &[f64];
            let inHigh: &[f64];
            let inLow: &[f64];
            let inClose: &[f64];
            if use_preloaded != 0 && ref_data.n > 0 {
                inOpen = &ref_data.open[..ref_data.n];
                inHigh = &ref_data.high[..ref_data.n];
                inLow = &ref_data.low[..ref_data.n];
                inClose = &ref_data.close[..ref_data.n];
            } else {
                _json_inOpen = parse_f64_array(&params["inOpen"]);
                inOpen = &_json_inOpen;
                _json_inHigh = parse_f64_array(&params["inHigh"]);
                inHigh = &_json_inHigh;
                _json_inLow = parse_f64_array(&params["inLow"]);
                inLow = &_json_inLow;
                _json_inClose = parse_f64_array(&params["inClose"]);
                inClose = &_json_inClose;
            }
            let out_size = if endIdx >= startIdx { endIdx - startIdx + 1 } else { 0 };
            let mut outIntBuf0: Vec<i32> = vec![0i32; out_size];
            let mut outBegIdx: usize = 0;
            let mut outNBElement: usize = 0;
            let mut rc = RetCode::Success;
            let start_time = Instant::now();
            for _bi in 0..bench_iters {
            rc = core.cdlmatchinglow(
                startIdx, endIdx,
                &inOpen,
                &inHigh,
                &inLow,
                &inClose,
                &mut outBegIdx, &mut outNBElement, &mut outIntBuf0,
            );
            }
            let elapsed_ns = start_time.elapsed().as_nanos() as u64 / bench_iters as u64;
            let start_time_ung = Instant::now();
            for _biu in 0..bench_iters {
            rc = core.cdlmatchinglow_unguarded(
                startIdx, endIdx,
                &inOpen,
                &inHigh,
                &inLow,
                &inClose,
                &mut outBegIdx, &mut outNBElement, &mut outIntBuf0,
            );
            }
            let elapsed_ns_ung = start_time_ung.elapsed().as_nanos() as u64 / bench_iters as u64;
            let lookback = core.cdlmatchinglow_lookback();
            let mut resp = format!("{{\"retCode\":{},\"outBegIdx\":{},\"outNBElement\":{},\"lookback\":{},\"timing_ns\":{},\"timing_ns_unguarded\":{}", retcode_to_int(rc), outBegIdx, outNBElement, lookback, elapsed_ns, elapsed_ns_ung);
            resp.push_str(",\"outInteger\":"); resp.push_str(&json_i32_array(&outIntBuf0[..outNBElement]));
            resp.push('}');
            resp
        }
        "TA_CDLMATHOLD" => {
            let startIdx = params["startIdx"].as_u64().unwrap_or(0) as usize;
            let endIdx = params["endIdx"].as_u64().unwrap_or(0) as usize;
            let use_preloaded = params["use_preloaded"].as_i64().unwrap_or(0);
            let bench_iters = std::cmp::max(1, params["iters"].as_i64().unwrap_or(1)) as u64;
            let mut _json_inOpen: Vec<f64> = Vec::new();
            let mut _json_inHigh: Vec<f64> = Vec::new();
            let mut _json_inLow: Vec<f64> = Vec::new();
            let mut _json_inClose: Vec<f64> = Vec::new();
            let inOpen: &[f64];
            let inHigh: &[f64];
            let inLow: &[f64];
            let inClose: &[f64];
            if use_preloaded != 0 && ref_data.n > 0 {
                inOpen = &ref_data.open[..ref_data.n];
                inHigh = &ref_data.high[..ref_data.n];
                inLow = &ref_data.low[..ref_data.n];
                inClose = &ref_data.close[..ref_data.n];
            } else {
                _json_inOpen = parse_f64_array(&params["inOpen"]);
                inOpen = &_json_inOpen;
                _json_inHigh = parse_f64_array(&params["inHigh"]);
                inHigh = &_json_inHigh;
                _json_inLow = parse_f64_array(&params["inLow"]);
                inLow = &_json_inLow;
                _json_inClose = parse_f64_array(&params["inClose"]);
                inClose = &_json_inClose;
            }
            let optInPenetration = params["optInPenetration"].as_f64().unwrap_or(0.5) as f64;
            let out_size = if endIdx >= startIdx { endIdx - startIdx + 1 } else { 0 };
            let mut outIntBuf0: Vec<i32> = vec![0i32; out_size];
            let mut outBegIdx: usize = 0;
            let mut outNBElement: usize = 0;
            let mut rc = RetCode::Success;
            let start_time = Instant::now();
            for _bi in 0..bench_iters {
            rc = core.cdlmathold(
                startIdx, endIdx,
                &inOpen,
                &inHigh,
                &inLow,
                &inClose,
                optInPenetration,
                &mut outBegIdx, &mut outNBElement, &mut outIntBuf0,
            );
            }
            let elapsed_ns = start_time.elapsed().as_nanos() as u64 / bench_iters as u64;
            let start_time_ung = Instant::now();
            for _biu in 0..bench_iters {
            rc = core.cdlmathold_unguarded(
                startIdx, endIdx,
                &inOpen,
                &inHigh,
                &inLow,
                &inClose,
                optInPenetration,
                &mut outBegIdx, &mut outNBElement, &mut outIntBuf0,
            );
            }
            let elapsed_ns_ung = start_time_ung.elapsed().as_nanos() as u64 / bench_iters as u64;
            let lookback = core.cdlmathold_lookback(optInPenetration);
            let mut resp = format!("{{\"retCode\":{},\"outBegIdx\":{},\"outNBElement\":{},\"lookback\":{},\"timing_ns\":{},\"timing_ns_unguarded\":{}", retcode_to_int(rc), outBegIdx, outNBElement, lookback, elapsed_ns, elapsed_ns_ung);
            resp.push_str(",\"outInteger\":"); resp.push_str(&json_i32_array(&outIntBuf0[..outNBElement]));
            resp.push('}');
            resp
        }
        "TA_CDLMORNINGDOJISTAR" => {
            let startIdx = params["startIdx"].as_u64().unwrap_or(0) as usize;
            let endIdx = params["endIdx"].as_u64().unwrap_or(0) as usize;
            let use_preloaded = params["use_preloaded"].as_i64().unwrap_or(0);
            let bench_iters = std::cmp::max(1, params["iters"].as_i64().unwrap_or(1)) as u64;
            let mut _json_inOpen: Vec<f64> = Vec::new();
            let mut _json_inHigh: Vec<f64> = Vec::new();
            let mut _json_inLow: Vec<f64> = Vec::new();
            let mut _json_inClose: Vec<f64> = Vec::new();
            let inOpen: &[f64];
            let inHigh: &[f64];
            let inLow: &[f64];
            let inClose: &[f64];
            if use_preloaded != 0 && ref_data.n > 0 {
                inOpen = &ref_data.open[..ref_data.n];
                inHigh = &ref_data.high[..ref_data.n];
                inLow = &ref_data.low[..ref_data.n];
                inClose = &ref_data.close[..ref_data.n];
            } else {
                _json_inOpen = parse_f64_array(&params["inOpen"]);
                inOpen = &_json_inOpen;
                _json_inHigh = parse_f64_array(&params["inHigh"]);
                inHigh = &_json_inHigh;
                _json_inLow = parse_f64_array(&params["inLow"]);
                inLow = &_json_inLow;
                _json_inClose = parse_f64_array(&params["inClose"]);
                inClose = &_json_inClose;
            }
            let optInPenetration = params["optInPenetration"].as_f64().unwrap_or(0.3) as f64;
            let out_size = if endIdx >= startIdx { endIdx - startIdx + 1 } else { 0 };
            let mut outIntBuf0: Vec<i32> = vec![0i32; out_size];
            let mut outBegIdx: usize = 0;
            let mut outNBElement: usize = 0;
            let mut rc = RetCode::Success;
            let start_time = Instant::now();
            for _bi in 0..bench_iters {
            rc = core.cdlmorningdojistar(
                startIdx, endIdx,
                &inOpen,
                &inHigh,
                &inLow,
                &inClose,
                optInPenetration,
                &mut outBegIdx, &mut outNBElement, &mut outIntBuf0,
            );
            }
            let elapsed_ns = start_time.elapsed().as_nanos() as u64 / bench_iters as u64;
            let start_time_ung = Instant::now();
            for _biu in 0..bench_iters {
            rc = core.cdlmorningdojistar_unguarded(
                startIdx, endIdx,
                &inOpen,
                &inHigh,
                &inLow,
                &inClose,
                optInPenetration,
                &mut outBegIdx, &mut outNBElement, &mut outIntBuf0,
            );
            }
            let elapsed_ns_ung = start_time_ung.elapsed().as_nanos() as u64 / bench_iters as u64;
            let lookback = core.cdlmorningdojistar_lookback(optInPenetration);
            let mut resp = format!("{{\"retCode\":{},\"outBegIdx\":{},\"outNBElement\":{},\"lookback\":{},\"timing_ns\":{},\"timing_ns_unguarded\":{}", retcode_to_int(rc), outBegIdx, outNBElement, lookback, elapsed_ns, elapsed_ns_ung);
            resp.push_str(",\"outInteger\":"); resp.push_str(&json_i32_array(&outIntBuf0[..outNBElement]));
            resp.push('}');
            resp
        }
        "TA_CDLMORNINGSTAR" => {
            let startIdx = params["startIdx"].as_u64().unwrap_or(0) as usize;
            let endIdx = params["endIdx"].as_u64().unwrap_or(0) as usize;
            let use_preloaded = params["use_preloaded"].as_i64().unwrap_or(0);
            let bench_iters = std::cmp::max(1, params["iters"].as_i64().unwrap_or(1)) as u64;
            let mut _json_inOpen: Vec<f64> = Vec::new();
            let mut _json_inHigh: Vec<f64> = Vec::new();
            let mut _json_inLow: Vec<f64> = Vec::new();
            let mut _json_inClose: Vec<f64> = Vec::new();
            let inOpen: &[f64];
            let inHigh: &[f64];
            let inLow: &[f64];
            let inClose: &[f64];
            if use_preloaded != 0 && ref_data.n > 0 {
                inOpen = &ref_data.open[..ref_data.n];
                inHigh = &ref_data.high[..ref_data.n];
                inLow = &ref_data.low[..ref_data.n];
                inClose = &ref_data.close[..ref_data.n];
            } else {
                _json_inOpen = parse_f64_array(&params["inOpen"]);
                inOpen = &_json_inOpen;
                _json_inHigh = parse_f64_array(&params["inHigh"]);
                inHigh = &_json_inHigh;
                _json_inLow = parse_f64_array(&params["inLow"]);
                inLow = &_json_inLow;
                _json_inClose = parse_f64_array(&params["inClose"]);
                inClose = &_json_inClose;
            }
            let optInPenetration = params["optInPenetration"].as_f64().unwrap_or(0.3) as f64;
            let out_size = if endIdx >= startIdx { endIdx - startIdx + 1 } else { 0 };
            let mut outIntBuf0: Vec<i32> = vec![0i32; out_size];
            let mut outBegIdx: usize = 0;
            let mut outNBElement: usize = 0;
            let mut rc = RetCode::Success;
            let start_time = Instant::now();
            for _bi in 0..bench_iters {
            rc = core.cdlmorningstar(
                startIdx, endIdx,
                &inOpen,
                &inHigh,
                &inLow,
                &inClose,
                optInPenetration,
                &mut outBegIdx, &mut outNBElement, &mut outIntBuf0,
            );
            }
            let elapsed_ns = start_time.elapsed().as_nanos() as u64 / bench_iters as u64;
            let start_time_ung = Instant::now();
            for _biu in 0..bench_iters {
            rc = core.cdlmorningstar_unguarded(
                startIdx, endIdx,
                &inOpen,
                &inHigh,
                &inLow,
                &inClose,
                optInPenetration,
                &mut outBegIdx, &mut outNBElement, &mut outIntBuf0,
            );
            }
            let elapsed_ns_ung = start_time_ung.elapsed().as_nanos() as u64 / bench_iters as u64;
            let lookback = core.cdlmorningstar_lookback(optInPenetration);
            let mut resp = format!("{{\"retCode\":{},\"outBegIdx\":{},\"outNBElement\":{},\"lookback\":{},\"timing_ns\":{},\"timing_ns_unguarded\":{}", retcode_to_int(rc), outBegIdx, outNBElement, lookback, elapsed_ns, elapsed_ns_ung);
            resp.push_str(",\"outInteger\":"); resp.push_str(&json_i32_array(&outIntBuf0[..outNBElement]));
            resp.push('}');
            resp
        }
        "TA_CDLONNECK" => {
            let startIdx = params["startIdx"].as_u64().unwrap_or(0) as usize;
            let endIdx = params["endIdx"].as_u64().unwrap_or(0) as usize;
            let use_preloaded = params["use_preloaded"].as_i64().unwrap_or(0);
            let bench_iters = std::cmp::max(1, params["iters"].as_i64().unwrap_or(1)) as u64;
            let mut _json_inOpen: Vec<f64> = Vec::new();
            let mut _json_inHigh: Vec<f64> = Vec::new();
            let mut _json_inLow: Vec<f64> = Vec::new();
            let mut _json_inClose: Vec<f64> = Vec::new();
            let inOpen: &[f64];
            let inHigh: &[f64];
            let inLow: &[f64];
            let inClose: &[f64];
            if use_preloaded != 0 && ref_data.n > 0 {
                inOpen = &ref_data.open[..ref_data.n];
                inHigh = &ref_data.high[..ref_data.n];
                inLow = &ref_data.low[..ref_data.n];
                inClose = &ref_data.close[..ref_data.n];
            } else {
                _json_inOpen = parse_f64_array(&params["inOpen"]);
                inOpen = &_json_inOpen;
                _json_inHigh = parse_f64_array(&params["inHigh"]);
                inHigh = &_json_inHigh;
                _json_inLow = parse_f64_array(&params["inLow"]);
                inLow = &_json_inLow;
                _json_inClose = parse_f64_array(&params["inClose"]);
                inClose = &_json_inClose;
            }
            let out_size = if endIdx >= startIdx { endIdx - startIdx + 1 } else { 0 };
            let mut outIntBuf0: Vec<i32> = vec![0i32; out_size];
            let mut outBegIdx: usize = 0;
            let mut outNBElement: usize = 0;
            let mut rc = RetCode::Success;
            let start_time = Instant::now();
            for _bi in 0..bench_iters {
            rc = core.cdlonneck(
                startIdx, endIdx,
                &inOpen,
                &inHigh,
                &inLow,
                &inClose,
                &mut outBegIdx, &mut outNBElement, &mut outIntBuf0,
            );
            }
            let elapsed_ns = start_time.elapsed().as_nanos() as u64 / bench_iters as u64;
            let start_time_ung = Instant::now();
            for _biu in 0..bench_iters {
            rc = core.cdlonneck_unguarded(
                startIdx, endIdx,
                &inOpen,
                &inHigh,
                &inLow,
                &inClose,
                &mut outBegIdx, &mut outNBElement, &mut outIntBuf0,
            );
            }
            let elapsed_ns_ung = start_time_ung.elapsed().as_nanos() as u64 / bench_iters as u64;
            let lookback = core.cdlonneck_lookback();
            let mut resp = format!("{{\"retCode\":{},\"outBegIdx\":{},\"outNBElement\":{},\"lookback\":{},\"timing_ns\":{},\"timing_ns_unguarded\":{}", retcode_to_int(rc), outBegIdx, outNBElement, lookback, elapsed_ns, elapsed_ns_ung);
            resp.push_str(",\"outInteger\":"); resp.push_str(&json_i32_array(&outIntBuf0[..outNBElement]));
            resp.push('}');
            resp
        }
        "TA_CDLPIERCING" => {
            let startIdx = params["startIdx"].as_u64().unwrap_or(0) as usize;
            let endIdx = params["endIdx"].as_u64().unwrap_or(0) as usize;
            let use_preloaded = params["use_preloaded"].as_i64().unwrap_or(0);
            let bench_iters = std::cmp::max(1, params["iters"].as_i64().unwrap_or(1)) as u64;
            let mut _json_inOpen: Vec<f64> = Vec::new();
            let mut _json_inHigh: Vec<f64> = Vec::new();
            let mut _json_inLow: Vec<f64> = Vec::new();
            let mut _json_inClose: Vec<f64> = Vec::new();
            let inOpen: &[f64];
            let inHigh: &[f64];
            let inLow: &[f64];
            let inClose: &[f64];
            if use_preloaded != 0 && ref_data.n > 0 {
                inOpen = &ref_data.open[..ref_data.n];
                inHigh = &ref_data.high[..ref_data.n];
                inLow = &ref_data.low[..ref_data.n];
                inClose = &ref_data.close[..ref_data.n];
            } else {
                _json_inOpen = parse_f64_array(&params["inOpen"]);
                inOpen = &_json_inOpen;
                _json_inHigh = parse_f64_array(&params["inHigh"]);
                inHigh = &_json_inHigh;
                _json_inLow = parse_f64_array(&params["inLow"]);
                inLow = &_json_inLow;
                _json_inClose = parse_f64_array(&params["inClose"]);
                inClose = &_json_inClose;
            }
            let out_size = if endIdx >= startIdx { endIdx - startIdx + 1 } else { 0 };
            let mut outIntBuf0: Vec<i32> = vec![0i32; out_size];
            let mut outBegIdx: usize = 0;
            let mut outNBElement: usize = 0;
            let mut rc = RetCode::Success;
            let start_time = Instant::now();
            for _bi in 0..bench_iters {
            rc = core.cdlpiercing(
                startIdx, endIdx,
                &inOpen,
                &inHigh,
                &inLow,
                &inClose,
                &mut outBegIdx, &mut outNBElement, &mut outIntBuf0,
            );
            }
            let elapsed_ns = start_time.elapsed().as_nanos() as u64 / bench_iters as u64;
            let start_time_ung = Instant::now();
            for _biu in 0..bench_iters {
            rc = core.cdlpiercing_unguarded(
                startIdx, endIdx,
                &inOpen,
                &inHigh,
                &inLow,
                &inClose,
                &mut outBegIdx, &mut outNBElement, &mut outIntBuf0,
            );
            }
            let elapsed_ns_ung = start_time_ung.elapsed().as_nanos() as u64 / bench_iters as u64;
            let lookback = core.cdlpiercing_lookback();
            let mut resp = format!("{{\"retCode\":{},\"outBegIdx\":{},\"outNBElement\":{},\"lookback\":{},\"timing_ns\":{},\"timing_ns_unguarded\":{}", retcode_to_int(rc), outBegIdx, outNBElement, lookback, elapsed_ns, elapsed_ns_ung);
            resp.push_str(",\"outInteger\":"); resp.push_str(&json_i32_array(&outIntBuf0[..outNBElement]));
            resp.push('}');
            resp
        }
        "TA_CDLRICKSHAWMAN" => {
            let startIdx = params["startIdx"].as_u64().unwrap_or(0) as usize;
            let endIdx = params["endIdx"].as_u64().unwrap_or(0) as usize;
            let use_preloaded = params["use_preloaded"].as_i64().unwrap_or(0);
            let bench_iters = std::cmp::max(1, params["iters"].as_i64().unwrap_or(1)) as u64;
            let mut _json_inOpen: Vec<f64> = Vec::new();
            let mut _json_inHigh: Vec<f64> = Vec::new();
            let mut _json_inLow: Vec<f64> = Vec::new();
            let mut _json_inClose: Vec<f64> = Vec::new();
            let inOpen: &[f64];
            let inHigh: &[f64];
            let inLow: &[f64];
            let inClose: &[f64];
            if use_preloaded != 0 && ref_data.n > 0 {
                inOpen = &ref_data.open[..ref_data.n];
                inHigh = &ref_data.high[..ref_data.n];
                inLow = &ref_data.low[..ref_data.n];
                inClose = &ref_data.close[..ref_data.n];
            } else {
                _json_inOpen = parse_f64_array(&params["inOpen"]);
                inOpen = &_json_inOpen;
                _json_inHigh = parse_f64_array(&params["inHigh"]);
                inHigh = &_json_inHigh;
                _json_inLow = parse_f64_array(&params["inLow"]);
                inLow = &_json_inLow;
                _json_inClose = parse_f64_array(&params["inClose"]);
                inClose = &_json_inClose;
            }
            let out_size = if endIdx >= startIdx { endIdx - startIdx + 1 } else { 0 };
            let mut outIntBuf0: Vec<i32> = vec![0i32; out_size];
            let mut outBegIdx: usize = 0;
            let mut outNBElement: usize = 0;
            let mut rc = RetCode::Success;
            let start_time = Instant::now();
            for _bi in 0..bench_iters {
            rc = core.cdlrickshawman(
                startIdx, endIdx,
                &inOpen,
                &inHigh,
                &inLow,
                &inClose,
                &mut outBegIdx, &mut outNBElement, &mut outIntBuf0,
            );
            }
            let elapsed_ns = start_time.elapsed().as_nanos() as u64 / bench_iters as u64;
            let start_time_ung = Instant::now();
            for _biu in 0..bench_iters {
            rc = core.cdlrickshawman_unguarded(
                startIdx, endIdx,
                &inOpen,
                &inHigh,
                &inLow,
                &inClose,
                &mut outBegIdx, &mut outNBElement, &mut outIntBuf0,
            );
            }
            let elapsed_ns_ung = start_time_ung.elapsed().as_nanos() as u64 / bench_iters as u64;
            let lookback = core.cdlrickshawman_lookback();
            let mut resp = format!("{{\"retCode\":{},\"outBegIdx\":{},\"outNBElement\":{},\"lookback\":{},\"timing_ns\":{},\"timing_ns_unguarded\":{}", retcode_to_int(rc), outBegIdx, outNBElement, lookback, elapsed_ns, elapsed_ns_ung);
            resp.push_str(",\"outInteger\":"); resp.push_str(&json_i32_array(&outIntBuf0[..outNBElement]));
            resp.push('}');
            resp
        }
        "TA_CDLRISEFALL3METHODS" => {
            let startIdx = params["startIdx"].as_u64().unwrap_or(0) as usize;
            let endIdx = params["endIdx"].as_u64().unwrap_or(0) as usize;
            let use_preloaded = params["use_preloaded"].as_i64().unwrap_or(0);
            let bench_iters = std::cmp::max(1, params["iters"].as_i64().unwrap_or(1)) as u64;
            let mut _json_inOpen: Vec<f64> = Vec::new();
            let mut _json_inHigh: Vec<f64> = Vec::new();
            let mut _json_inLow: Vec<f64> = Vec::new();
            let mut _json_inClose: Vec<f64> = Vec::new();
            let inOpen: &[f64];
            let inHigh: &[f64];
            let inLow: &[f64];
            let inClose: &[f64];
            if use_preloaded != 0 && ref_data.n > 0 {
                inOpen = &ref_data.open[..ref_data.n];
                inHigh = &ref_data.high[..ref_data.n];
                inLow = &ref_data.low[..ref_data.n];
                inClose = &ref_data.close[..ref_data.n];
            } else {
                _json_inOpen = parse_f64_array(&params["inOpen"]);
                inOpen = &_json_inOpen;
                _json_inHigh = parse_f64_array(&params["inHigh"]);
                inHigh = &_json_inHigh;
                _json_inLow = parse_f64_array(&params["inLow"]);
                inLow = &_json_inLow;
                _json_inClose = parse_f64_array(&params["inClose"]);
                inClose = &_json_inClose;
            }
            let out_size = if endIdx >= startIdx { endIdx - startIdx + 1 } else { 0 };
            let mut outIntBuf0: Vec<i32> = vec![0i32; out_size];
            let mut outBegIdx: usize = 0;
            let mut outNBElement: usize = 0;
            let mut rc = RetCode::Success;
            let start_time = Instant::now();
            for _bi in 0..bench_iters {
            rc = core.cdlrisefall3methods(
                startIdx, endIdx,
                &inOpen,
                &inHigh,
                &inLow,
                &inClose,
                &mut outBegIdx, &mut outNBElement, &mut outIntBuf0,
            );
            }
            let elapsed_ns = start_time.elapsed().as_nanos() as u64 / bench_iters as u64;
            let start_time_ung = Instant::now();
            for _biu in 0..bench_iters {
            rc = core.cdlrisefall3methods_unguarded(
                startIdx, endIdx,
                &inOpen,
                &inHigh,
                &inLow,
                &inClose,
                &mut outBegIdx, &mut outNBElement, &mut outIntBuf0,
            );
            }
            let elapsed_ns_ung = start_time_ung.elapsed().as_nanos() as u64 / bench_iters as u64;
            let lookback = core.cdlrisefall3methods_lookback();
            let mut resp = format!("{{\"retCode\":{},\"outBegIdx\":{},\"outNBElement\":{},\"lookback\":{},\"timing_ns\":{},\"timing_ns_unguarded\":{}", retcode_to_int(rc), outBegIdx, outNBElement, lookback, elapsed_ns, elapsed_ns_ung);
            resp.push_str(",\"outInteger\":"); resp.push_str(&json_i32_array(&outIntBuf0[..outNBElement]));
            resp.push('}');
            resp
        }
        "TA_CDLSEPARATINGLINES" => {
            let startIdx = params["startIdx"].as_u64().unwrap_or(0) as usize;
            let endIdx = params["endIdx"].as_u64().unwrap_or(0) as usize;
            let use_preloaded = params["use_preloaded"].as_i64().unwrap_or(0);
            let bench_iters = std::cmp::max(1, params["iters"].as_i64().unwrap_or(1)) as u64;
            let mut _json_inOpen: Vec<f64> = Vec::new();
            let mut _json_inHigh: Vec<f64> = Vec::new();
            let mut _json_inLow: Vec<f64> = Vec::new();
            let mut _json_inClose: Vec<f64> = Vec::new();
            let inOpen: &[f64];
            let inHigh: &[f64];
            let inLow: &[f64];
            let inClose: &[f64];
            if use_preloaded != 0 && ref_data.n > 0 {
                inOpen = &ref_data.open[..ref_data.n];
                inHigh = &ref_data.high[..ref_data.n];
                inLow = &ref_data.low[..ref_data.n];
                inClose = &ref_data.close[..ref_data.n];
            } else {
                _json_inOpen = parse_f64_array(&params["inOpen"]);
                inOpen = &_json_inOpen;
                _json_inHigh = parse_f64_array(&params["inHigh"]);
                inHigh = &_json_inHigh;
                _json_inLow = parse_f64_array(&params["inLow"]);
                inLow = &_json_inLow;
                _json_inClose = parse_f64_array(&params["inClose"]);
                inClose = &_json_inClose;
            }
            let out_size = if endIdx >= startIdx { endIdx - startIdx + 1 } else { 0 };
            let mut outIntBuf0: Vec<i32> = vec![0i32; out_size];
            let mut outBegIdx: usize = 0;
            let mut outNBElement: usize = 0;
            let mut rc = RetCode::Success;
            let start_time = Instant::now();
            for _bi in 0..bench_iters {
            rc = core.cdlseparatinglines(
                startIdx, endIdx,
                &inOpen,
                &inHigh,
                &inLow,
                &inClose,
                &mut outBegIdx, &mut outNBElement, &mut outIntBuf0,
            );
            }
            let elapsed_ns = start_time.elapsed().as_nanos() as u64 / bench_iters as u64;
            let start_time_ung = Instant::now();
            for _biu in 0..bench_iters {
            rc = core.cdlseparatinglines_unguarded(
                startIdx, endIdx,
                &inOpen,
                &inHigh,
                &inLow,
                &inClose,
                &mut outBegIdx, &mut outNBElement, &mut outIntBuf0,
            );
            }
            let elapsed_ns_ung = start_time_ung.elapsed().as_nanos() as u64 / bench_iters as u64;
            let lookback = core.cdlseparatinglines_lookback();
            let mut resp = format!("{{\"retCode\":{},\"outBegIdx\":{},\"outNBElement\":{},\"lookback\":{},\"timing_ns\":{},\"timing_ns_unguarded\":{}", retcode_to_int(rc), outBegIdx, outNBElement, lookback, elapsed_ns, elapsed_ns_ung);
            resp.push_str(",\"outInteger\":"); resp.push_str(&json_i32_array(&outIntBuf0[..outNBElement]));
            resp.push('}');
            resp
        }
        "TA_CDLSHOOTINGSTAR" => {
            let startIdx = params["startIdx"].as_u64().unwrap_or(0) as usize;
            let endIdx = params["endIdx"].as_u64().unwrap_or(0) as usize;
            let use_preloaded = params["use_preloaded"].as_i64().unwrap_or(0);
            let bench_iters = std::cmp::max(1, params["iters"].as_i64().unwrap_or(1)) as u64;
            let mut _json_inOpen: Vec<f64> = Vec::new();
            let mut _json_inHigh: Vec<f64> = Vec::new();
            let mut _json_inLow: Vec<f64> = Vec::new();
            let mut _json_inClose: Vec<f64> = Vec::new();
            let inOpen: &[f64];
            let inHigh: &[f64];
            let inLow: &[f64];
            let inClose: &[f64];
            if use_preloaded != 0 && ref_data.n > 0 {
                inOpen = &ref_data.open[..ref_data.n];
                inHigh = &ref_data.high[..ref_data.n];
                inLow = &ref_data.low[..ref_data.n];
                inClose = &ref_data.close[..ref_data.n];
            } else {
                _json_inOpen = parse_f64_array(&params["inOpen"]);
                inOpen = &_json_inOpen;
                _json_inHigh = parse_f64_array(&params["inHigh"]);
                inHigh = &_json_inHigh;
                _json_inLow = parse_f64_array(&params["inLow"]);
                inLow = &_json_inLow;
                _json_inClose = parse_f64_array(&params["inClose"]);
                inClose = &_json_inClose;
            }
            let out_size = if endIdx >= startIdx { endIdx - startIdx + 1 } else { 0 };
            let mut outIntBuf0: Vec<i32> = vec![0i32; out_size];
            let mut outBegIdx: usize = 0;
            let mut outNBElement: usize = 0;
            let mut rc = RetCode::Success;
            let start_time = Instant::now();
            for _bi in 0..bench_iters {
            rc = core.cdlshootingstar(
                startIdx, endIdx,
                &inOpen,
                &inHigh,
                &inLow,
                &inClose,
                &mut outBegIdx, &mut outNBElement, &mut outIntBuf0,
            );
            }
            let elapsed_ns = start_time.elapsed().as_nanos() as u64 / bench_iters as u64;
            let start_time_ung = Instant::now();
            for _biu in 0..bench_iters {
            rc = core.cdlshootingstar_unguarded(
                startIdx, endIdx,
                &inOpen,
                &inHigh,
                &inLow,
                &inClose,
                &mut outBegIdx, &mut outNBElement, &mut outIntBuf0,
            );
            }
            let elapsed_ns_ung = start_time_ung.elapsed().as_nanos() as u64 / bench_iters as u64;
            let lookback = core.cdlshootingstar_lookback();
            let mut resp = format!("{{\"retCode\":{},\"outBegIdx\":{},\"outNBElement\":{},\"lookback\":{},\"timing_ns\":{},\"timing_ns_unguarded\":{}", retcode_to_int(rc), outBegIdx, outNBElement, lookback, elapsed_ns, elapsed_ns_ung);
            resp.push_str(",\"outInteger\":"); resp.push_str(&json_i32_array(&outIntBuf0[..outNBElement]));
            resp.push('}');
            resp
        }
        "TA_CDLSHORTLINE" => {
            let startIdx = params["startIdx"].as_u64().unwrap_or(0) as usize;
            let endIdx = params["endIdx"].as_u64().unwrap_or(0) as usize;
            let use_preloaded = params["use_preloaded"].as_i64().unwrap_or(0);
            let bench_iters = std::cmp::max(1, params["iters"].as_i64().unwrap_or(1)) as u64;
            let mut _json_inOpen: Vec<f64> = Vec::new();
            let mut _json_inHigh: Vec<f64> = Vec::new();
            let mut _json_inLow: Vec<f64> = Vec::new();
            let mut _json_inClose: Vec<f64> = Vec::new();
            let inOpen: &[f64];
            let inHigh: &[f64];
            let inLow: &[f64];
            let inClose: &[f64];
            if use_preloaded != 0 && ref_data.n > 0 {
                inOpen = &ref_data.open[..ref_data.n];
                inHigh = &ref_data.high[..ref_data.n];
                inLow = &ref_data.low[..ref_data.n];
                inClose = &ref_data.close[..ref_data.n];
            } else {
                _json_inOpen = parse_f64_array(&params["inOpen"]);
                inOpen = &_json_inOpen;
                _json_inHigh = parse_f64_array(&params["inHigh"]);
                inHigh = &_json_inHigh;
                _json_inLow = parse_f64_array(&params["inLow"]);
                inLow = &_json_inLow;
                _json_inClose = parse_f64_array(&params["inClose"]);
                inClose = &_json_inClose;
            }
            let out_size = if endIdx >= startIdx { endIdx - startIdx + 1 } else { 0 };
            let mut outIntBuf0: Vec<i32> = vec![0i32; out_size];
            let mut outBegIdx: usize = 0;
            let mut outNBElement: usize = 0;
            let mut rc = RetCode::Success;
            let start_time = Instant::now();
            for _bi in 0..bench_iters {
            rc = core.cdlshortline(
                startIdx, endIdx,
                &inOpen,
                &inHigh,
                &inLow,
                &inClose,
                &mut outBegIdx, &mut outNBElement, &mut outIntBuf0,
            );
            }
            let elapsed_ns = start_time.elapsed().as_nanos() as u64 / bench_iters as u64;
            let start_time_ung = Instant::now();
            for _biu in 0..bench_iters {
            rc = core.cdlshortline_unguarded(
                startIdx, endIdx,
                &inOpen,
                &inHigh,
                &inLow,
                &inClose,
                &mut outBegIdx, &mut outNBElement, &mut outIntBuf0,
            );
            }
            let elapsed_ns_ung = start_time_ung.elapsed().as_nanos() as u64 / bench_iters as u64;
            let lookback = core.cdlshortline_lookback();
            let mut resp = format!("{{\"retCode\":{},\"outBegIdx\":{},\"outNBElement\":{},\"lookback\":{},\"timing_ns\":{},\"timing_ns_unguarded\":{}", retcode_to_int(rc), outBegIdx, outNBElement, lookback, elapsed_ns, elapsed_ns_ung);
            resp.push_str(",\"outInteger\":"); resp.push_str(&json_i32_array(&outIntBuf0[..outNBElement]));
            resp.push('}');
            resp
        }
        "TA_CDLSPINNINGTOP" => {
            let startIdx = params["startIdx"].as_u64().unwrap_or(0) as usize;
            let endIdx = params["endIdx"].as_u64().unwrap_or(0) as usize;
            let use_preloaded = params["use_preloaded"].as_i64().unwrap_or(0);
            let bench_iters = std::cmp::max(1, params["iters"].as_i64().unwrap_or(1)) as u64;
            let mut _json_inOpen: Vec<f64> = Vec::new();
            let mut _json_inHigh: Vec<f64> = Vec::new();
            let mut _json_inLow: Vec<f64> = Vec::new();
            let mut _json_inClose: Vec<f64> = Vec::new();
            let inOpen: &[f64];
            let inHigh: &[f64];
            let inLow: &[f64];
            let inClose: &[f64];
            if use_preloaded != 0 && ref_data.n > 0 {
                inOpen = &ref_data.open[..ref_data.n];
                inHigh = &ref_data.high[..ref_data.n];
                inLow = &ref_data.low[..ref_data.n];
                inClose = &ref_data.close[..ref_data.n];
            } else {
                _json_inOpen = parse_f64_array(&params["inOpen"]);
                inOpen = &_json_inOpen;
                _json_inHigh = parse_f64_array(&params["inHigh"]);
                inHigh = &_json_inHigh;
                _json_inLow = parse_f64_array(&params["inLow"]);
                inLow = &_json_inLow;
                _json_inClose = parse_f64_array(&params["inClose"]);
                inClose = &_json_inClose;
            }
            let out_size = if endIdx >= startIdx { endIdx - startIdx + 1 } else { 0 };
            let mut outIntBuf0: Vec<i32> = vec![0i32; out_size];
            let mut outBegIdx: usize = 0;
            let mut outNBElement: usize = 0;
            let mut rc = RetCode::Success;
            let start_time = Instant::now();
            for _bi in 0..bench_iters {
            rc = core.cdlspinningtop(
                startIdx, endIdx,
                &inOpen,
                &inHigh,
                &inLow,
                &inClose,
                &mut outBegIdx, &mut outNBElement, &mut outIntBuf0,
            );
            }
            let elapsed_ns = start_time.elapsed().as_nanos() as u64 / bench_iters as u64;
            let start_time_ung = Instant::now();
            for _biu in 0..bench_iters {
            rc = core.cdlspinningtop_unguarded(
                startIdx, endIdx,
                &inOpen,
                &inHigh,
                &inLow,
                &inClose,
                &mut outBegIdx, &mut outNBElement, &mut outIntBuf0,
            );
            }
            let elapsed_ns_ung = start_time_ung.elapsed().as_nanos() as u64 / bench_iters as u64;
            let lookback = core.cdlspinningtop_lookback();
            let mut resp = format!("{{\"retCode\":{},\"outBegIdx\":{},\"outNBElement\":{},\"lookback\":{},\"timing_ns\":{},\"timing_ns_unguarded\":{}", retcode_to_int(rc), outBegIdx, outNBElement, lookback, elapsed_ns, elapsed_ns_ung);
            resp.push_str(",\"outInteger\":"); resp.push_str(&json_i32_array(&outIntBuf0[..outNBElement]));
            resp.push('}');
            resp
        }
        "TA_CDLSTALLEDPATTERN" => {
            let startIdx = params["startIdx"].as_u64().unwrap_or(0) as usize;
            let endIdx = params["endIdx"].as_u64().unwrap_or(0) as usize;
            let use_preloaded = params["use_preloaded"].as_i64().unwrap_or(0);
            let bench_iters = std::cmp::max(1, params["iters"].as_i64().unwrap_or(1)) as u64;
            let mut _json_inOpen: Vec<f64> = Vec::new();
            let mut _json_inHigh: Vec<f64> = Vec::new();
            let mut _json_inLow: Vec<f64> = Vec::new();
            let mut _json_inClose: Vec<f64> = Vec::new();
            let inOpen: &[f64];
            let inHigh: &[f64];
            let inLow: &[f64];
            let inClose: &[f64];
            if use_preloaded != 0 && ref_data.n > 0 {
                inOpen = &ref_data.open[..ref_data.n];
                inHigh = &ref_data.high[..ref_data.n];
                inLow = &ref_data.low[..ref_data.n];
                inClose = &ref_data.close[..ref_data.n];
            } else {
                _json_inOpen = parse_f64_array(&params["inOpen"]);
                inOpen = &_json_inOpen;
                _json_inHigh = parse_f64_array(&params["inHigh"]);
                inHigh = &_json_inHigh;
                _json_inLow = parse_f64_array(&params["inLow"]);
                inLow = &_json_inLow;
                _json_inClose = parse_f64_array(&params["inClose"]);
                inClose = &_json_inClose;
            }
            let out_size = if endIdx >= startIdx { endIdx - startIdx + 1 } else { 0 };
            let mut outIntBuf0: Vec<i32> = vec![0i32; out_size];
            let mut outBegIdx: usize = 0;
            let mut outNBElement: usize = 0;
            let mut rc = RetCode::Success;
            let start_time = Instant::now();
            for _bi in 0..bench_iters {
            rc = core.cdlstalledpattern(
                startIdx, endIdx,
                &inOpen,
                &inHigh,
                &inLow,
                &inClose,
                &mut outBegIdx, &mut outNBElement, &mut outIntBuf0,
            );
            }
            let elapsed_ns = start_time.elapsed().as_nanos() as u64 / bench_iters as u64;
            let start_time_ung = Instant::now();
            for _biu in 0..bench_iters {
            rc = core.cdlstalledpattern_unguarded(
                startIdx, endIdx,
                &inOpen,
                &inHigh,
                &inLow,
                &inClose,
                &mut outBegIdx, &mut outNBElement, &mut outIntBuf0,
            );
            }
            let elapsed_ns_ung = start_time_ung.elapsed().as_nanos() as u64 / bench_iters as u64;
            let lookback = core.cdlstalledpattern_lookback();
            let mut resp = format!("{{\"retCode\":{},\"outBegIdx\":{},\"outNBElement\":{},\"lookback\":{},\"timing_ns\":{},\"timing_ns_unguarded\":{}", retcode_to_int(rc), outBegIdx, outNBElement, lookback, elapsed_ns, elapsed_ns_ung);
            resp.push_str(",\"outInteger\":"); resp.push_str(&json_i32_array(&outIntBuf0[..outNBElement]));
            resp.push('}');
            resp
        }
        "TA_CDLSTICKSANDWICH" => {
            let startIdx = params["startIdx"].as_u64().unwrap_or(0) as usize;
            let endIdx = params["endIdx"].as_u64().unwrap_or(0) as usize;
            let use_preloaded = params["use_preloaded"].as_i64().unwrap_or(0);
            let bench_iters = std::cmp::max(1, params["iters"].as_i64().unwrap_or(1)) as u64;
            let mut _json_inOpen: Vec<f64> = Vec::new();
            let mut _json_inHigh: Vec<f64> = Vec::new();
            let mut _json_inLow: Vec<f64> = Vec::new();
            let mut _json_inClose: Vec<f64> = Vec::new();
            let inOpen: &[f64];
            let inHigh: &[f64];
            let inLow: &[f64];
            let inClose: &[f64];
            if use_preloaded != 0 && ref_data.n > 0 {
                inOpen = &ref_data.open[..ref_data.n];
                inHigh = &ref_data.high[..ref_data.n];
                inLow = &ref_data.low[..ref_data.n];
                inClose = &ref_data.close[..ref_data.n];
            } else {
                _json_inOpen = parse_f64_array(&params["inOpen"]);
                inOpen = &_json_inOpen;
                _json_inHigh = parse_f64_array(&params["inHigh"]);
                inHigh = &_json_inHigh;
                _json_inLow = parse_f64_array(&params["inLow"]);
                inLow = &_json_inLow;
                _json_inClose = parse_f64_array(&params["inClose"]);
                inClose = &_json_inClose;
            }
            let out_size = if endIdx >= startIdx { endIdx - startIdx + 1 } else { 0 };
            let mut outIntBuf0: Vec<i32> = vec![0i32; out_size];
            let mut outBegIdx: usize = 0;
            let mut outNBElement: usize = 0;
            let mut rc = RetCode::Success;
            let start_time = Instant::now();
            for _bi in 0..bench_iters {
            rc = core.cdlsticksandwich(
                startIdx, endIdx,
                &inOpen,
                &inHigh,
                &inLow,
                &inClose,
                &mut outBegIdx, &mut outNBElement, &mut outIntBuf0,
            );
            }
            let elapsed_ns = start_time.elapsed().as_nanos() as u64 / bench_iters as u64;
            let start_time_ung = Instant::now();
            for _biu in 0..bench_iters {
            rc = core.cdlsticksandwich_unguarded(
                startIdx, endIdx,
                &inOpen,
                &inHigh,
                &inLow,
                &inClose,
                &mut outBegIdx, &mut outNBElement, &mut outIntBuf0,
            );
            }
            let elapsed_ns_ung = start_time_ung.elapsed().as_nanos() as u64 / bench_iters as u64;
            let lookback = core.cdlsticksandwich_lookback();
            let mut resp = format!("{{\"retCode\":{},\"outBegIdx\":{},\"outNBElement\":{},\"lookback\":{},\"timing_ns\":{},\"timing_ns_unguarded\":{}", retcode_to_int(rc), outBegIdx, outNBElement, lookback, elapsed_ns, elapsed_ns_ung);
            resp.push_str(",\"outInteger\":"); resp.push_str(&json_i32_array(&outIntBuf0[..outNBElement]));
            resp.push('}');
            resp
        }
        "TA_CDLTAKURI" => {
            let startIdx = params["startIdx"].as_u64().unwrap_or(0) as usize;
            let endIdx = params["endIdx"].as_u64().unwrap_or(0) as usize;
            let use_preloaded = params["use_preloaded"].as_i64().unwrap_or(0);
            let bench_iters = std::cmp::max(1, params["iters"].as_i64().unwrap_or(1)) as u64;
            let mut _json_inOpen: Vec<f64> = Vec::new();
            let mut _json_inHigh: Vec<f64> = Vec::new();
            let mut _json_inLow: Vec<f64> = Vec::new();
            let mut _json_inClose: Vec<f64> = Vec::new();
            let inOpen: &[f64];
            let inHigh: &[f64];
            let inLow: &[f64];
            let inClose: &[f64];
            if use_preloaded != 0 && ref_data.n > 0 {
                inOpen = &ref_data.open[..ref_data.n];
                inHigh = &ref_data.high[..ref_data.n];
                inLow = &ref_data.low[..ref_data.n];
                inClose = &ref_data.close[..ref_data.n];
            } else {
                _json_inOpen = parse_f64_array(&params["inOpen"]);
                inOpen = &_json_inOpen;
                _json_inHigh = parse_f64_array(&params["inHigh"]);
                inHigh = &_json_inHigh;
                _json_inLow = parse_f64_array(&params["inLow"]);
                inLow = &_json_inLow;
                _json_inClose = parse_f64_array(&params["inClose"]);
                inClose = &_json_inClose;
            }
            let out_size = if endIdx >= startIdx { endIdx - startIdx + 1 } else { 0 };
            let mut outIntBuf0: Vec<i32> = vec![0i32; out_size];
            let mut outBegIdx: usize = 0;
            let mut outNBElement: usize = 0;
            let mut rc = RetCode::Success;
            let start_time = Instant::now();
            for _bi in 0..bench_iters {
            rc = core.cdltakuri(
                startIdx, endIdx,
                &inOpen,
                &inHigh,
                &inLow,
                &inClose,
                &mut outBegIdx, &mut outNBElement, &mut outIntBuf0,
            );
            }
            let elapsed_ns = start_time.elapsed().as_nanos() as u64 / bench_iters as u64;
            let start_time_ung = Instant::now();
            for _biu in 0..bench_iters {
            rc = core.cdltakuri_unguarded(
                startIdx, endIdx,
                &inOpen,
                &inHigh,
                &inLow,
                &inClose,
                &mut outBegIdx, &mut outNBElement, &mut outIntBuf0,
            );
            }
            let elapsed_ns_ung = start_time_ung.elapsed().as_nanos() as u64 / bench_iters as u64;
            let lookback = core.cdltakuri_lookback();
            let mut resp = format!("{{\"retCode\":{},\"outBegIdx\":{},\"outNBElement\":{},\"lookback\":{},\"timing_ns\":{},\"timing_ns_unguarded\":{}", retcode_to_int(rc), outBegIdx, outNBElement, lookback, elapsed_ns, elapsed_ns_ung);
            resp.push_str(",\"outInteger\":"); resp.push_str(&json_i32_array(&outIntBuf0[..outNBElement]));
            resp.push('}');
            resp
        }
        "TA_CDLTASUKIGAP" => {
            let startIdx = params["startIdx"].as_u64().unwrap_or(0) as usize;
            let endIdx = params["endIdx"].as_u64().unwrap_or(0) as usize;
            let use_preloaded = params["use_preloaded"].as_i64().unwrap_or(0);
            let bench_iters = std::cmp::max(1, params["iters"].as_i64().unwrap_or(1)) as u64;
            let mut _json_inOpen: Vec<f64> = Vec::new();
            let mut _json_inHigh: Vec<f64> = Vec::new();
            let mut _json_inLow: Vec<f64> = Vec::new();
            let mut _json_inClose: Vec<f64> = Vec::new();
            let inOpen: &[f64];
            let inHigh: &[f64];
            let inLow: &[f64];
            let inClose: &[f64];
            if use_preloaded != 0 && ref_data.n > 0 {
                inOpen = &ref_data.open[..ref_data.n];
                inHigh = &ref_data.high[..ref_data.n];
                inLow = &ref_data.low[..ref_data.n];
                inClose = &ref_data.close[..ref_data.n];
            } else {
                _json_inOpen = parse_f64_array(&params["inOpen"]);
                inOpen = &_json_inOpen;
                _json_inHigh = parse_f64_array(&params["inHigh"]);
                inHigh = &_json_inHigh;
                _json_inLow = parse_f64_array(&params["inLow"]);
                inLow = &_json_inLow;
                _json_inClose = parse_f64_array(&params["inClose"]);
                inClose = &_json_inClose;
            }
            let out_size = if endIdx >= startIdx { endIdx - startIdx + 1 } else { 0 };
            let mut outIntBuf0: Vec<i32> = vec![0i32; out_size];
            let mut outBegIdx: usize = 0;
            let mut outNBElement: usize = 0;
            let mut rc = RetCode::Success;
            let start_time = Instant::now();
            for _bi in 0..bench_iters {
            rc = core.cdltasukigap(
                startIdx, endIdx,
                &inOpen,
                &inHigh,
                &inLow,
                &inClose,
                &mut outBegIdx, &mut outNBElement, &mut outIntBuf0,
            );
            }
            let elapsed_ns = start_time.elapsed().as_nanos() as u64 / bench_iters as u64;
            let start_time_ung = Instant::now();
            for _biu in 0..bench_iters {
            rc = core.cdltasukigap_unguarded(
                startIdx, endIdx,
                &inOpen,
                &inHigh,
                &inLow,
                &inClose,
                &mut outBegIdx, &mut outNBElement, &mut outIntBuf0,
            );
            }
            let elapsed_ns_ung = start_time_ung.elapsed().as_nanos() as u64 / bench_iters as u64;
            let lookback = core.cdltasukigap_lookback();
            let mut resp = format!("{{\"retCode\":{},\"outBegIdx\":{},\"outNBElement\":{},\"lookback\":{},\"timing_ns\":{},\"timing_ns_unguarded\":{}", retcode_to_int(rc), outBegIdx, outNBElement, lookback, elapsed_ns, elapsed_ns_ung);
            resp.push_str(",\"outInteger\":"); resp.push_str(&json_i32_array(&outIntBuf0[..outNBElement]));
            resp.push('}');
            resp
        }
        "TA_CDLTHRUSTING" => {
            let startIdx = params["startIdx"].as_u64().unwrap_or(0) as usize;
            let endIdx = params["endIdx"].as_u64().unwrap_or(0) as usize;
            let use_preloaded = params["use_preloaded"].as_i64().unwrap_or(0);
            let bench_iters = std::cmp::max(1, params["iters"].as_i64().unwrap_or(1)) as u64;
            let mut _json_inOpen: Vec<f64> = Vec::new();
            let mut _json_inHigh: Vec<f64> = Vec::new();
            let mut _json_inLow: Vec<f64> = Vec::new();
            let mut _json_inClose: Vec<f64> = Vec::new();
            let inOpen: &[f64];
            let inHigh: &[f64];
            let inLow: &[f64];
            let inClose: &[f64];
            if use_preloaded != 0 && ref_data.n > 0 {
                inOpen = &ref_data.open[..ref_data.n];
                inHigh = &ref_data.high[..ref_data.n];
                inLow = &ref_data.low[..ref_data.n];
                inClose = &ref_data.close[..ref_data.n];
            } else {
                _json_inOpen = parse_f64_array(&params["inOpen"]);
                inOpen = &_json_inOpen;
                _json_inHigh = parse_f64_array(&params["inHigh"]);
                inHigh = &_json_inHigh;
                _json_inLow = parse_f64_array(&params["inLow"]);
                inLow = &_json_inLow;
                _json_inClose = parse_f64_array(&params["inClose"]);
                inClose = &_json_inClose;
            }
            let out_size = if endIdx >= startIdx { endIdx - startIdx + 1 } else { 0 };
            let mut outIntBuf0: Vec<i32> = vec![0i32; out_size];
            let mut outBegIdx: usize = 0;
            let mut outNBElement: usize = 0;
            let mut rc = RetCode::Success;
            let start_time = Instant::now();
            for _bi in 0..bench_iters {
            rc = core.cdlthrusting(
                startIdx, endIdx,
                &inOpen,
                &inHigh,
                &inLow,
                &inClose,
                &mut outBegIdx, &mut outNBElement, &mut outIntBuf0,
            );
            }
            let elapsed_ns = start_time.elapsed().as_nanos() as u64 / bench_iters as u64;
            let start_time_ung = Instant::now();
            for _biu in 0..bench_iters {
            rc = core.cdlthrusting_unguarded(
                startIdx, endIdx,
                &inOpen,
                &inHigh,
                &inLow,
                &inClose,
                &mut outBegIdx, &mut outNBElement, &mut outIntBuf0,
            );
            }
            let elapsed_ns_ung = start_time_ung.elapsed().as_nanos() as u64 / bench_iters as u64;
            let lookback = core.cdlthrusting_lookback();
            let mut resp = format!("{{\"retCode\":{},\"outBegIdx\":{},\"outNBElement\":{},\"lookback\":{},\"timing_ns\":{},\"timing_ns_unguarded\":{}", retcode_to_int(rc), outBegIdx, outNBElement, lookback, elapsed_ns, elapsed_ns_ung);
            resp.push_str(",\"outInteger\":"); resp.push_str(&json_i32_array(&outIntBuf0[..outNBElement]));
            resp.push('}');
            resp
        }
        "TA_CDLTRISTAR" => {
            let startIdx = params["startIdx"].as_u64().unwrap_or(0) as usize;
            let endIdx = params["endIdx"].as_u64().unwrap_or(0) as usize;
            let use_preloaded = params["use_preloaded"].as_i64().unwrap_or(0);
            let bench_iters = std::cmp::max(1, params["iters"].as_i64().unwrap_or(1)) as u64;
            let mut _json_inOpen: Vec<f64> = Vec::new();
            let mut _json_inHigh: Vec<f64> = Vec::new();
            let mut _json_inLow: Vec<f64> = Vec::new();
            let mut _json_inClose: Vec<f64> = Vec::new();
            let inOpen: &[f64];
            let inHigh: &[f64];
            let inLow: &[f64];
            let inClose: &[f64];
            if use_preloaded != 0 && ref_data.n > 0 {
                inOpen = &ref_data.open[..ref_data.n];
                inHigh = &ref_data.high[..ref_data.n];
                inLow = &ref_data.low[..ref_data.n];
                inClose = &ref_data.close[..ref_data.n];
            } else {
                _json_inOpen = parse_f64_array(&params["inOpen"]);
                inOpen = &_json_inOpen;
                _json_inHigh = parse_f64_array(&params["inHigh"]);
                inHigh = &_json_inHigh;
                _json_inLow = parse_f64_array(&params["inLow"]);
                inLow = &_json_inLow;
                _json_inClose = parse_f64_array(&params["inClose"]);
                inClose = &_json_inClose;
            }
            let out_size = if endIdx >= startIdx { endIdx - startIdx + 1 } else { 0 };
            let mut outIntBuf0: Vec<i32> = vec![0i32; out_size];
            let mut outBegIdx: usize = 0;
            let mut outNBElement: usize = 0;
            let mut rc = RetCode::Success;
            let start_time = Instant::now();
            for _bi in 0..bench_iters {
            rc = core.cdltristar(
                startIdx, endIdx,
                &inOpen,
                &inHigh,
                &inLow,
                &inClose,
                &mut outBegIdx, &mut outNBElement, &mut outIntBuf0,
            );
            }
            let elapsed_ns = start_time.elapsed().as_nanos() as u64 / bench_iters as u64;
            let start_time_ung = Instant::now();
            for _biu in 0..bench_iters {
            rc = core.cdltristar_unguarded(
                startIdx, endIdx,
                &inOpen,
                &inHigh,
                &inLow,
                &inClose,
                &mut outBegIdx, &mut outNBElement, &mut outIntBuf0,
            );
            }
            let elapsed_ns_ung = start_time_ung.elapsed().as_nanos() as u64 / bench_iters as u64;
            let lookback = core.cdltristar_lookback();
            let mut resp = format!("{{\"retCode\":{},\"outBegIdx\":{},\"outNBElement\":{},\"lookback\":{},\"timing_ns\":{},\"timing_ns_unguarded\":{}", retcode_to_int(rc), outBegIdx, outNBElement, lookback, elapsed_ns, elapsed_ns_ung);
            resp.push_str(",\"outInteger\":"); resp.push_str(&json_i32_array(&outIntBuf0[..outNBElement]));
            resp.push('}');
            resp
        }
        "TA_CDLUNIQUE3RIVER" => {
            let startIdx = params["startIdx"].as_u64().unwrap_or(0) as usize;
            let endIdx = params["endIdx"].as_u64().unwrap_or(0) as usize;
            let use_preloaded = params["use_preloaded"].as_i64().unwrap_or(0);
            let bench_iters = std::cmp::max(1, params["iters"].as_i64().unwrap_or(1)) as u64;
            let mut _json_inOpen: Vec<f64> = Vec::new();
            let mut _json_inHigh: Vec<f64> = Vec::new();
            let mut _json_inLow: Vec<f64> = Vec::new();
            let mut _json_inClose: Vec<f64> = Vec::new();
            let inOpen: &[f64];
            let inHigh: &[f64];
            let inLow: &[f64];
            let inClose: &[f64];
            if use_preloaded != 0 && ref_data.n > 0 {
                inOpen = &ref_data.open[..ref_data.n];
                inHigh = &ref_data.high[..ref_data.n];
                inLow = &ref_data.low[..ref_data.n];
                inClose = &ref_data.close[..ref_data.n];
            } else {
                _json_inOpen = parse_f64_array(&params["inOpen"]);
                inOpen = &_json_inOpen;
                _json_inHigh = parse_f64_array(&params["inHigh"]);
                inHigh = &_json_inHigh;
                _json_inLow = parse_f64_array(&params["inLow"]);
                inLow = &_json_inLow;
                _json_inClose = parse_f64_array(&params["inClose"]);
                inClose = &_json_inClose;
            }
            let out_size = if endIdx >= startIdx { endIdx - startIdx + 1 } else { 0 };
            let mut outIntBuf0: Vec<i32> = vec![0i32; out_size];
            let mut outBegIdx: usize = 0;
            let mut outNBElement: usize = 0;
            let mut rc = RetCode::Success;
            let start_time = Instant::now();
            for _bi in 0..bench_iters {
            rc = core.cdlunique3river(
                startIdx, endIdx,
                &inOpen,
                &inHigh,
                &inLow,
                &inClose,
                &mut outBegIdx, &mut outNBElement, &mut outIntBuf0,
            );
            }
            let elapsed_ns = start_time.elapsed().as_nanos() as u64 / bench_iters as u64;
            let start_time_ung = Instant::now();
            for _biu in 0..bench_iters {
            rc = core.cdlunique3river_unguarded(
                startIdx, endIdx,
                &inOpen,
                &inHigh,
                &inLow,
                &inClose,
                &mut outBegIdx, &mut outNBElement, &mut outIntBuf0,
            );
            }
            let elapsed_ns_ung = start_time_ung.elapsed().as_nanos() as u64 / bench_iters as u64;
            let lookback = core.cdlunique3river_lookback();
            let mut resp = format!("{{\"retCode\":{},\"outBegIdx\":{},\"outNBElement\":{},\"lookback\":{},\"timing_ns\":{},\"timing_ns_unguarded\":{}", retcode_to_int(rc), outBegIdx, outNBElement, lookback, elapsed_ns, elapsed_ns_ung);
            resp.push_str(",\"outInteger\":"); resp.push_str(&json_i32_array(&outIntBuf0[..outNBElement]));
            resp.push('}');
            resp
        }
        "TA_CDLUPSIDEGAP2CROWS" => {
            let startIdx = params["startIdx"].as_u64().unwrap_or(0) as usize;
            let endIdx = params["endIdx"].as_u64().unwrap_or(0) as usize;
            let use_preloaded = params["use_preloaded"].as_i64().unwrap_or(0);
            let bench_iters = std::cmp::max(1, params["iters"].as_i64().unwrap_or(1)) as u64;
            let mut _json_inOpen: Vec<f64> = Vec::new();
            let mut _json_inHigh: Vec<f64> = Vec::new();
            let mut _json_inLow: Vec<f64> = Vec::new();
            let mut _json_inClose: Vec<f64> = Vec::new();
            let inOpen: &[f64];
            let inHigh: &[f64];
            let inLow: &[f64];
            let inClose: &[f64];
            if use_preloaded != 0 && ref_data.n > 0 {
                inOpen = &ref_data.open[..ref_data.n];
                inHigh = &ref_data.high[..ref_data.n];
                inLow = &ref_data.low[..ref_data.n];
                inClose = &ref_data.close[..ref_data.n];
            } else {
                _json_inOpen = parse_f64_array(&params["inOpen"]);
                inOpen = &_json_inOpen;
                _json_inHigh = parse_f64_array(&params["inHigh"]);
                inHigh = &_json_inHigh;
                _json_inLow = parse_f64_array(&params["inLow"]);
                inLow = &_json_inLow;
                _json_inClose = parse_f64_array(&params["inClose"]);
                inClose = &_json_inClose;
            }
            let out_size = if endIdx >= startIdx { endIdx - startIdx + 1 } else { 0 };
            let mut outIntBuf0: Vec<i32> = vec![0i32; out_size];
            let mut outBegIdx: usize = 0;
            let mut outNBElement: usize = 0;
            let mut rc = RetCode::Success;
            let start_time = Instant::now();
            for _bi in 0..bench_iters {
            rc = core.cdlupsidegap2crows(
                startIdx, endIdx,
                &inOpen,
                &inHigh,
                &inLow,
                &inClose,
                &mut outBegIdx, &mut outNBElement, &mut outIntBuf0,
            );
            }
            let elapsed_ns = start_time.elapsed().as_nanos() as u64 / bench_iters as u64;
            let start_time_ung = Instant::now();
            for _biu in 0..bench_iters {
            rc = core.cdlupsidegap2crows_unguarded(
                startIdx, endIdx,
                &inOpen,
                &inHigh,
                &inLow,
                &inClose,
                &mut outBegIdx, &mut outNBElement, &mut outIntBuf0,
            );
            }
            let elapsed_ns_ung = start_time_ung.elapsed().as_nanos() as u64 / bench_iters as u64;
            let lookback = core.cdlupsidegap2crows_lookback();
            let mut resp = format!("{{\"retCode\":{},\"outBegIdx\":{},\"outNBElement\":{},\"lookback\":{},\"timing_ns\":{},\"timing_ns_unguarded\":{}", retcode_to_int(rc), outBegIdx, outNBElement, lookback, elapsed_ns, elapsed_ns_ung);
            resp.push_str(",\"outInteger\":"); resp.push_str(&json_i32_array(&outIntBuf0[..outNBElement]));
            resp.push('}');
            resp
        }
        "TA_CDLXSIDEGAP3METHODS" => {
            let startIdx = params["startIdx"].as_u64().unwrap_or(0) as usize;
            let endIdx = params["endIdx"].as_u64().unwrap_or(0) as usize;
            let use_preloaded = params["use_preloaded"].as_i64().unwrap_or(0);
            let bench_iters = std::cmp::max(1, params["iters"].as_i64().unwrap_or(1)) as u64;
            let mut _json_inOpen: Vec<f64> = Vec::new();
            let mut _json_inHigh: Vec<f64> = Vec::new();
            let mut _json_inLow: Vec<f64> = Vec::new();
            let mut _json_inClose: Vec<f64> = Vec::new();
            let inOpen: &[f64];
            let inHigh: &[f64];
            let inLow: &[f64];
            let inClose: &[f64];
            if use_preloaded != 0 && ref_data.n > 0 {
                inOpen = &ref_data.open[..ref_data.n];
                inHigh = &ref_data.high[..ref_data.n];
                inLow = &ref_data.low[..ref_data.n];
                inClose = &ref_data.close[..ref_data.n];
            } else {
                _json_inOpen = parse_f64_array(&params["inOpen"]);
                inOpen = &_json_inOpen;
                _json_inHigh = parse_f64_array(&params["inHigh"]);
                inHigh = &_json_inHigh;
                _json_inLow = parse_f64_array(&params["inLow"]);
                inLow = &_json_inLow;
                _json_inClose = parse_f64_array(&params["inClose"]);
                inClose = &_json_inClose;
            }
            let out_size = if endIdx >= startIdx { endIdx - startIdx + 1 } else { 0 };
            let mut outIntBuf0: Vec<i32> = vec![0i32; out_size];
            let mut outBegIdx: usize = 0;
            let mut outNBElement: usize = 0;
            let mut rc = RetCode::Success;
            let start_time = Instant::now();
            for _bi in 0..bench_iters {
            rc = core.cdlxsidegap3methods(
                startIdx, endIdx,
                &inOpen,
                &inHigh,
                &inLow,
                &inClose,
                &mut outBegIdx, &mut outNBElement, &mut outIntBuf0,
            );
            }
            let elapsed_ns = start_time.elapsed().as_nanos() as u64 / bench_iters as u64;
            let start_time_ung = Instant::now();
            for _biu in 0..bench_iters {
            rc = core.cdlxsidegap3methods_unguarded(
                startIdx, endIdx,
                &inOpen,
                &inHigh,
                &inLow,
                &inClose,
                &mut outBegIdx, &mut outNBElement, &mut outIntBuf0,
            );
            }
            let elapsed_ns_ung = start_time_ung.elapsed().as_nanos() as u64 / bench_iters as u64;
            let lookback = core.cdlxsidegap3methods_lookback();
            let mut resp = format!("{{\"retCode\":{},\"outBegIdx\":{},\"outNBElement\":{},\"lookback\":{},\"timing_ns\":{},\"timing_ns_unguarded\":{}", retcode_to_int(rc), outBegIdx, outNBElement, lookback, elapsed_ns, elapsed_ns_ung);
            resp.push_str(",\"outInteger\":"); resp.push_str(&json_i32_array(&outIntBuf0[..outNBElement]));
            resp.push('}');
            resp
        }
        "TA_CEIL" => {
            let startIdx = params["startIdx"].as_u64().unwrap_or(0) as usize;
            let endIdx = params["endIdx"].as_u64().unwrap_or(0) as usize;
            let use_preloaded = params["use_preloaded"].as_i64().unwrap_or(0);
            let bench_iters = std::cmp::max(1, params["iters"].as_i64().unwrap_or(1)) as u64;
            let mut _json_inReal: Vec<f64> = Vec::new();
            let inReal: &[f64];
            if use_preloaded != 0 && ref_data.n > 0 {
                inReal = &ref_data.close[..ref_data.n];
            } else {
                _json_inReal = parse_f64_array(&params["inReal"]);
                inReal = &_json_inReal;
            }
            let out_size = if endIdx >= startIdx { endIdx - startIdx + 1 } else { 0 };
            let mut outBuf0: Vec<f64> = vec![0.0f64; out_size];
            let mut outBegIdx: usize = 0;
            let mut outNBElement: usize = 0;
            let mut rc = RetCode::Success;
            let start_time = Instant::now();
            for _bi in 0..bench_iters {
            rc = core.ceil(
                startIdx, endIdx,
                &inReal,
                &mut outBegIdx, &mut outNBElement, &mut outBuf0,
            );
            }
            let elapsed_ns = start_time.elapsed().as_nanos() as u64 / bench_iters as u64;
            let start_time_ung = Instant::now();
            for _biu in 0..bench_iters {
            rc = core.ceil_unguarded(
                startIdx, endIdx,
                &inReal,
                &mut outBegIdx, &mut outNBElement, &mut outBuf0,
            );
            }
            let elapsed_ns_ung = start_time_ung.elapsed().as_nanos() as u64 / bench_iters as u64;
            let lookback = core.ceil_lookback();
            let mut resp = format!("{{\"retCode\":{},\"outBegIdx\":{},\"outNBElement\":{},\"lookback\":{},\"timing_ns\":{},\"timing_ns_unguarded\":{}", retcode_to_int(rc), outBegIdx, outNBElement, lookback, elapsed_ns, elapsed_ns_ung);
            resp.push_str(",\"outReal\":"); resp.push_str(&json_f64_array(&outBuf0[..outNBElement]));
            resp.push('}');
            resp
        }
        "TA_CMO" => {
            let startIdx = params["startIdx"].as_u64().unwrap_or(0) as usize;
            let endIdx = params["endIdx"].as_u64().unwrap_or(0) as usize;
            let use_preloaded = params["use_preloaded"].as_i64().unwrap_or(0);
            let bench_iters = std::cmp::max(1, params["iters"].as_i64().unwrap_or(1)) as u64;
            let mut _json_inReal: Vec<f64> = Vec::new();
            let inReal: &[f64];
            if use_preloaded != 0 && ref_data.n > 0 {
                inReal = &ref_data.close[..ref_data.n];
            } else {
                _json_inReal = parse_f64_array(&params["inReal"]);
                inReal = &_json_inReal;
            }
            let optInTimePeriod = params["optInTimePeriod"].as_i64().unwrap_or(14) as i32;
            if let Some(period) = params["unstablePeriod"].as_i64() {
                core.unstable_period[3] = period as i32;
            }
            let out_size = if endIdx >= startIdx { endIdx - startIdx + 1 } else { 0 };
            let mut outBuf0: Vec<f64> = vec![0.0f64; out_size];
            let mut outBegIdx: usize = 0;
            let mut outNBElement: usize = 0;
            let mut rc = RetCode::Success;
            let start_time = Instant::now();
            for _bi in 0..bench_iters {
            rc = core.cmo(
                startIdx, endIdx,
                &inReal,
                optInTimePeriod,
                &mut outBegIdx, &mut outNBElement, &mut outBuf0,
            );
            }
            let elapsed_ns = start_time.elapsed().as_nanos() as u64 / bench_iters as u64;
            let start_time_ung = Instant::now();
            for _biu in 0..bench_iters {
            rc = core.cmo_unguarded(
                startIdx, endIdx,
                &inReal,
                optInTimePeriod,
                &mut outBegIdx, &mut outNBElement, &mut outBuf0,
            );
            }
            let elapsed_ns_ung = start_time_ung.elapsed().as_nanos() as u64 / bench_iters as u64;
            let lookback = core.cmo_lookback(optInTimePeriod);
            let mut resp = format!("{{\"retCode\":{},\"outBegIdx\":{},\"outNBElement\":{},\"lookback\":{},\"timing_ns\":{},\"timing_ns_unguarded\":{}", retcode_to_int(rc), outBegIdx, outNBElement, lookback, elapsed_ns, elapsed_ns_ung);
            resp.push_str(",\"outReal\":"); resp.push_str(&json_f64_array(&outBuf0[..outNBElement]));
            resp.push('}');
            resp
        }
        "TA_CORREL" => {
            let startIdx = params["startIdx"].as_u64().unwrap_or(0) as usize;
            let endIdx = params["endIdx"].as_u64().unwrap_or(0) as usize;
            let use_preloaded = params["use_preloaded"].as_i64().unwrap_or(0);
            let bench_iters = std::cmp::max(1, params["iters"].as_i64().unwrap_or(1)) as u64;
            let mut _json_inReal0: Vec<f64> = Vec::new();
            let mut _json_inReal1: Vec<f64> = Vec::new();
            let inReal0: &[f64];
            let inReal1: &[f64];
            if use_preloaded != 0 && ref_data.n > 0 {
                inReal0 = &ref_data.close[..ref_data.n];
                inReal1 = &ref_data.high[..ref_data.n];
            } else {
                _json_inReal0 = parse_f64_array(&params["inReal0"]);
                inReal0 = &_json_inReal0;
                _json_inReal1 = parse_f64_array(&params["inReal1"]);
                inReal1 = &_json_inReal1;
            }
            let optInTimePeriod = params["optInTimePeriod"].as_i64().unwrap_or(30) as i32;
            let out_size = if endIdx >= startIdx { endIdx - startIdx + 1 } else { 0 };
            let mut outBuf0: Vec<f64> = vec![0.0f64; out_size];
            let mut outBegIdx: usize = 0;
            let mut outNBElement: usize = 0;
            let mut rc = RetCode::Success;
            let start_time = Instant::now();
            for _bi in 0..bench_iters {
            rc = core.correl(
                startIdx, endIdx,
                &inReal0,
                &inReal1,
                optInTimePeriod,
                &mut outBegIdx, &mut outNBElement, &mut outBuf0,
            );
            }
            let elapsed_ns = start_time.elapsed().as_nanos() as u64 / bench_iters as u64;
            let start_time_ung = Instant::now();
            for _biu in 0..bench_iters {
            rc = core.correl_unguarded(
                startIdx, endIdx,
                &inReal0,
                &inReal1,
                optInTimePeriod,
                &mut outBegIdx, &mut outNBElement, &mut outBuf0,
            );
            }
            let elapsed_ns_ung = start_time_ung.elapsed().as_nanos() as u64 / bench_iters as u64;
            let lookback = core.correl_lookback(optInTimePeriod);
            let mut resp = format!("{{\"retCode\":{},\"outBegIdx\":{},\"outNBElement\":{},\"lookback\":{},\"timing_ns\":{},\"timing_ns_unguarded\":{}", retcode_to_int(rc), outBegIdx, outNBElement, lookback, elapsed_ns, elapsed_ns_ung);
            resp.push_str(",\"outReal\":"); resp.push_str(&json_f64_array(&outBuf0[..outNBElement]));
            resp.push('}');
            resp
        }
        "TA_COS" => {
            let startIdx = params["startIdx"].as_u64().unwrap_or(0) as usize;
            let endIdx = params["endIdx"].as_u64().unwrap_or(0) as usize;
            let use_preloaded = params["use_preloaded"].as_i64().unwrap_or(0);
            let bench_iters = std::cmp::max(1, params["iters"].as_i64().unwrap_or(1)) as u64;
            let mut _json_inReal: Vec<f64> = Vec::new();
            let inReal: &[f64];
            if use_preloaded != 0 && ref_data.n > 0 {
                inReal = &ref_data.close[..ref_data.n];
            } else {
                _json_inReal = parse_f64_array(&params["inReal"]);
                inReal = &_json_inReal;
            }
            let out_size = if endIdx >= startIdx { endIdx - startIdx + 1 } else { 0 };
            let mut outBuf0: Vec<f64> = vec![0.0f64; out_size];
            let mut outBegIdx: usize = 0;
            let mut outNBElement: usize = 0;
            let mut rc = RetCode::Success;
            let start_time = Instant::now();
            for _bi in 0..bench_iters {
            rc = core.cos(
                startIdx, endIdx,
                &inReal,
                &mut outBegIdx, &mut outNBElement, &mut outBuf0,
            );
            }
            let elapsed_ns = start_time.elapsed().as_nanos() as u64 / bench_iters as u64;
            let start_time_ung = Instant::now();
            for _biu in 0..bench_iters {
            rc = core.cos_unguarded(
                startIdx, endIdx,
                &inReal,
                &mut outBegIdx, &mut outNBElement, &mut outBuf0,
            );
            }
            let elapsed_ns_ung = start_time_ung.elapsed().as_nanos() as u64 / bench_iters as u64;
            let lookback = core.cos_lookback();
            let mut resp = format!("{{\"retCode\":{},\"outBegIdx\":{},\"outNBElement\":{},\"lookback\":{},\"timing_ns\":{},\"timing_ns_unguarded\":{}", retcode_to_int(rc), outBegIdx, outNBElement, lookback, elapsed_ns, elapsed_ns_ung);
            resp.push_str(",\"outReal\":"); resp.push_str(&json_f64_array(&outBuf0[..outNBElement]));
            resp.push('}');
            resp
        }
        "TA_COSH" => {
            let startIdx = params["startIdx"].as_u64().unwrap_or(0) as usize;
            let endIdx = params["endIdx"].as_u64().unwrap_or(0) as usize;
            let use_preloaded = params["use_preloaded"].as_i64().unwrap_or(0);
            let bench_iters = std::cmp::max(1, params["iters"].as_i64().unwrap_or(1)) as u64;
            let mut _json_inReal: Vec<f64> = Vec::new();
            let inReal: &[f64];
            if use_preloaded != 0 && ref_data.n > 0 {
                inReal = &ref_data.close[..ref_data.n];
            } else {
                _json_inReal = parse_f64_array(&params["inReal"]);
                inReal = &_json_inReal;
            }
            let out_size = if endIdx >= startIdx { endIdx - startIdx + 1 } else { 0 };
            let mut outBuf0: Vec<f64> = vec![0.0f64; out_size];
            let mut outBegIdx: usize = 0;
            let mut outNBElement: usize = 0;
            let mut rc = RetCode::Success;
            let start_time = Instant::now();
            for _bi in 0..bench_iters {
            rc = core.cosh(
                startIdx, endIdx,
                &inReal,
                &mut outBegIdx, &mut outNBElement, &mut outBuf0,
            );
            }
            let elapsed_ns = start_time.elapsed().as_nanos() as u64 / bench_iters as u64;
            let start_time_ung = Instant::now();
            for _biu in 0..bench_iters {
            rc = core.cosh_unguarded(
                startIdx, endIdx,
                &inReal,
                &mut outBegIdx, &mut outNBElement, &mut outBuf0,
            );
            }
            let elapsed_ns_ung = start_time_ung.elapsed().as_nanos() as u64 / bench_iters as u64;
            let lookback = core.cosh_lookback();
            let mut resp = format!("{{\"retCode\":{},\"outBegIdx\":{},\"outNBElement\":{},\"lookback\":{},\"timing_ns\":{},\"timing_ns_unguarded\":{}", retcode_to_int(rc), outBegIdx, outNBElement, lookback, elapsed_ns, elapsed_ns_ung);
            resp.push_str(",\"outReal\":"); resp.push_str(&json_f64_array(&outBuf0[..outNBElement]));
            resp.push('}');
            resp
        }
        "TA_DEMA" => {
            let startIdx = params["startIdx"].as_u64().unwrap_or(0) as usize;
            let endIdx = params["endIdx"].as_u64().unwrap_or(0) as usize;
            let use_preloaded = params["use_preloaded"].as_i64().unwrap_or(0);
            let bench_iters = std::cmp::max(1, params["iters"].as_i64().unwrap_or(1)) as u64;
            let mut _json_inReal: Vec<f64> = Vec::new();
            let inReal: &[f64];
            if use_preloaded != 0 && ref_data.n > 0 {
                inReal = &ref_data.close[..ref_data.n];
            } else {
                _json_inReal = parse_f64_array(&params["inReal"]);
                inReal = &_json_inReal;
            }
            let optInTimePeriod = params["optInTimePeriod"].as_i64().unwrap_or(30) as i32;
            let out_size = if endIdx >= startIdx { endIdx - startIdx + 1 } else { 0 };
            let mut outBuf0: Vec<f64> = vec![0.0f64; out_size];
            let mut outBegIdx: usize = 0;
            let mut outNBElement: usize = 0;
            let mut rc = RetCode::Success;
            let start_time = Instant::now();
            for _bi in 0..bench_iters {
            rc = core.dema(
                startIdx, endIdx,
                &inReal,
                optInTimePeriod,
                &mut outBegIdx, &mut outNBElement, &mut outBuf0,
            );
            }
            let elapsed_ns = start_time.elapsed().as_nanos() as u64 / bench_iters as u64;
            let start_time_ung = Instant::now();
            for _biu in 0..bench_iters {
            rc = core.dema_unguarded(
                startIdx, endIdx,
                &inReal,
                optInTimePeriod,
                &mut outBegIdx, &mut outNBElement, &mut outBuf0,
            );
            }
            let elapsed_ns_ung = start_time_ung.elapsed().as_nanos() as u64 / bench_iters as u64;
            let lookback = core.dema_lookback(optInTimePeriod);
            let mut resp = format!("{{\"retCode\":{},\"outBegIdx\":{},\"outNBElement\":{},\"lookback\":{},\"timing_ns\":{},\"timing_ns_unguarded\":{}", retcode_to_int(rc), outBegIdx, outNBElement, lookback, elapsed_ns, elapsed_ns_ung);
            resp.push_str(",\"outReal\":"); resp.push_str(&json_f64_array(&outBuf0[..outNBElement]));
            resp.push('}');
            resp
        }
        "TA_DIV" => {
            let startIdx = params["startIdx"].as_u64().unwrap_or(0) as usize;
            let endIdx = params["endIdx"].as_u64().unwrap_or(0) as usize;
            let use_preloaded = params["use_preloaded"].as_i64().unwrap_or(0);
            let bench_iters = std::cmp::max(1, params["iters"].as_i64().unwrap_or(1)) as u64;
            let mut _json_inReal0: Vec<f64> = Vec::new();
            let mut _json_inReal1: Vec<f64> = Vec::new();
            let inReal0: &[f64];
            let inReal1: &[f64];
            if use_preloaded != 0 && ref_data.n > 0 {
                inReal0 = &ref_data.close[..ref_data.n];
                inReal1 = &ref_data.high[..ref_data.n];
            } else {
                _json_inReal0 = parse_f64_array(&params["inReal0"]);
                inReal0 = &_json_inReal0;
                _json_inReal1 = parse_f64_array(&params["inReal1"]);
                inReal1 = &_json_inReal1;
            }
            let out_size = if endIdx >= startIdx { endIdx - startIdx + 1 } else { 0 };
            let mut outBuf0: Vec<f64> = vec![0.0f64; out_size];
            let mut outBegIdx: usize = 0;
            let mut outNBElement: usize = 0;
            let mut rc = RetCode::Success;
            let start_time = Instant::now();
            for _bi in 0..bench_iters {
            rc = core.div(
                startIdx, endIdx,
                &inReal0,
                &inReal1,
                &mut outBegIdx, &mut outNBElement, &mut outBuf0,
            );
            }
            let elapsed_ns = start_time.elapsed().as_nanos() as u64 / bench_iters as u64;
            let start_time_ung = Instant::now();
            for _biu in 0..bench_iters {
            rc = core.div_unguarded(
                startIdx, endIdx,
                &inReal0,
                &inReal1,
                &mut outBegIdx, &mut outNBElement, &mut outBuf0,
            );
            }
            let elapsed_ns_ung = start_time_ung.elapsed().as_nanos() as u64 / bench_iters as u64;
            let lookback = core.div_lookback();
            let mut resp = format!("{{\"retCode\":{},\"outBegIdx\":{},\"outNBElement\":{},\"lookback\":{},\"timing_ns\":{},\"timing_ns_unguarded\":{}", retcode_to_int(rc), outBegIdx, outNBElement, lookback, elapsed_ns, elapsed_ns_ung);
            resp.push_str(",\"outReal\":"); resp.push_str(&json_f64_array(&outBuf0[..outNBElement]));
            resp.push('}');
            resp
        }
        "TA_DX" => {
            let startIdx = params["startIdx"].as_u64().unwrap_or(0) as usize;
            let endIdx = params["endIdx"].as_u64().unwrap_or(0) as usize;
            let use_preloaded = params["use_preloaded"].as_i64().unwrap_or(0);
            let bench_iters = std::cmp::max(1, params["iters"].as_i64().unwrap_or(1)) as u64;
            let mut _json_inHigh: Vec<f64> = Vec::new();
            let mut _json_inLow: Vec<f64> = Vec::new();
            let mut _json_inClose: Vec<f64> = Vec::new();
            let inHigh: &[f64];
            let inLow: &[f64];
            let inClose: &[f64];
            if use_preloaded != 0 && ref_data.n > 0 {
                inHigh = &ref_data.high[..ref_data.n];
                inLow = &ref_data.low[..ref_data.n];
                inClose = &ref_data.close[..ref_data.n];
            } else {
                _json_inHigh = parse_f64_array(&params["inHigh"]);
                inHigh = &_json_inHigh;
                _json_inLow = parse_f64_array(&params["inLow"]);
                inLow = &_json_inLow;
                _json_inClose = parse_f64_array(&params["inClose"]);
                inClose = &_json_inClose;
            }
            let optInTimePeriod = params["optInTimePeriod"].as_i64().unwrap_or(14) as i32;
            if let Some(period) = params["unstablePeriod"].as_i64() {
                core.unstable_period[4] = period as i32;
            }
            let out_size = if endIdx >= startIdx { endIdx - startIdx + 1 } else { 0 };
            let mut outBuf0: Vec<f64> = vec![0.0f64; out_size];
            let mut outBegIdx: usize = 0;
            let mut outNBElement: usize = 0;
            let mut rc = RetCode::Success;
            let start_time = Instant::now();
            for _bi in 0..bench_iters {
            rc = core.dx(
                startIdx, endIdx,
                &inHigh,
                &inLow,
                &inClose,
                optInTimePeriod,
                &mut outBegIdx, &mut outNBElement, &mut outBuf0,
            );
            }
            let elapsed_ns = start_time.elapsed().as_nanos() as u64 / bench_iters as u64;
            let start_time_ung = Instant::now();
            for _biu in 0..bench_iters {
            rc = core.dx_unguarded(
                startIdx, endIdx,
                &inHigh,
                &inLow,
                &inClose,
                optInTimePeriod,
                &mut outBegIdx, &mut outNBElement, &mut outBuf0,
            );
            }
            let elapsed_ns_ung = start_time_ung.elapsed().as_nanos() as u64 / bench_iters as u64;
            let lookback = core.dx_lookback(optInTimePeriod);
            let mut resp = format!("{{\"retCode\":{},\"outBegIdx\":{},\"outNBElement\":{},\"lookback\":{},\"timing_ns\":{},\"timing_ns_unguarded\":{}", retcode_to_int(rc), outBegIdx, outNBElement, lookback, elapsed_ns, elapsed_ns_ung);
            resp.push_str(",\"outReal\":"); resp.push_str(&json_f64_array(&outBuf0[..outNBElement]));
            resp.push('}');
            resp
        }
        "TA_EMA" => {
            let startIdx = params["startIdx"].as_u64().unwrap_or(0) as usize;
            let endIdx = params["endIdx"].as_u64().unwrap_or(0) as usize;
            let use_preloaded = params["use_preloaded"].as_i64().unwrap_or(0);
            let bench_iters = std::cmp::max(1, params["iters"].as_i64().unwrap_or(1)) as u64;
            let mut _json_inReal: Vec<f64> = Vec::new();
            let inReal: &[f64];
            if use_preloaded != 0 && ref_data.n > 0 {
                inReal = &ref_data.close[..ref_data.n];
            } else {
                _json_inReal = parse_f64_array(&params["inReal"]);
                inReal = &_json_inReal;
            }
            let optInTimePeriod = params["optInTimePeriod"].as_i64().unwrap_or(30) as i32;
            if let Some(period) = params["unstablePeriod"].as_i64() {
                core.unstable_period[5] = period as i32;
            }
            let out_size = if endIdx >= startIdx { endIdx - startIdx + 1 } else { 0 };
            let mut outBuf0: Vec<f64> = vec![0.0f64; out_size];
            let mut outBegIdx: usize = 0;
            let mut outNBElement: usize = 0;
            let mut rc = RetCode::Success;
            let start_time = Instant::now();
            for _bi in 0..bench_iters {
            rc = core.ema(
                startIdx, endIdx,
                &inReal,
                optInTimePeriod,
                &mut outBegIdx, &mut outNBElement, &mut outBuf0,
            );
            }
            let elapsed_ns = start_time.elapsed().as_nanos() as u64 / bench_iters as u64;
            let start_time_ung = Instant::now();
            for _biu in 0..bench_iters {
            rc = core.ema_unguarded(
                startIdx, endIdx,
                &inReal,
                optInTimePeriod,
                &mut outBegIdx, &mut outNBElement, &mut outBuf0,
            );
            }
            let elapsed_ns_ung = start_time_ung.elapsed().as_nanos() as u64 / bench_iters as u64;
            let lookback = core.ema_lookback(optInTimePeriod);
            let mut resp = format!("{{\"retCode\":{},\"outBegIdx\":{},\"outNBElement\":{},\"lookback\":{},\"timing_ns\":{},\"timing_ns_unguarded\":{}", retcode_to_int(rc), outBegIdx, outNBElement, lookback, elapsed_ns, elapsed_ns_ung);
            resp.push_str(",\"outReal\":"); resp.push_str(&json_f64_array(&outBuf0[..outNBElement]));
            resp.push('}');
            resp
        }
        "TA_EXP" => {
            let startIdx = params["startIdx"].as_u64().unwrap_or(0) as usize;
            let endIdx = params["endIdx"].as_u64().unwrap_or(0) as usize;
            let use_preloaded = params["use_preloaded"].as_i64().unwrap_or(0);
            let bench_iters = std::cmp::max(1, params["iters"].as_i64().unwrap_or(1)) as u64;
            let mut _json_inReal: Vec<f64> = Vec::new();
            let inReal: &[f64];
            if use_preloaded != 0 && ref_data.n > 0 {
                inReal = &ref_data.close[..ref_data.n];
            } else {
                _json_inReal = parse_f64_array(&params["inReal"]);
                inReal = &_json_inReal;
            }
            let out_size = if endIdx >= startIdx { endIdx - startIdx + 1 } else { 0 };
            let mut outBuf0: Vec<f64> = vec![0.0f64; out_size];
            let mut outBegIdx: usize = 0;
            let mut outNBElement: usize = 0;
            let mut rc = RetCode::Success;
            let start_time = Instant::now();
            for _bi in 0..bench_iters {
            rc = core.exp(
                startIdx, endIdx,
                &inReal,
                &mut outBegIdx, &mut outNBElement, &mut outBuf0,
            );
            }
            let elapsed_ns = start_time.elapsed().as_nanos() as u64 / bench_iters as u64;
            let start_time_ung = Instant::now();
            for _biu in 0..bench_iters {
            rc = core.exp_unguarded(
                startIdx, endIdx,
                &inReal,
                &mut outBegIdx, &mut outNBElement, &mut outBuf0,
            );
            }
            let elapsed_ns_ung = start_time_ung.elapsed().as_nanos() as u64 / bench_iters as u64;
            let lookback = core.exp_lookback();
            let mut resp = format!("{{\"retCode\":{},\"outBegIdx\":{},\"outNBElement\":{},\"lookback\":{},\"timing_ns\":{},\"timing_ns_unguarded\":{}", retcode_to_int(rc), outBegIdx, outNBElement, lookback, elapsed_ns, elapsed_ns_ung);
            resp.push_str(",\"outReal\":"); resp.push_str(&json_f64_array(&outBuf0[..outNBElement]));
            resp.push('}');
            resp
        }
        "TA_FLOOR" => {
            let startIdx = params["startIdx"].as_u64().unwrap_or(0) as usize;
            let endIdx = params["endIdx"].as_u64().unwrap_or(0) as usize;
            let use_preloaded = params["use_preloaded"].as_i64().unwrap_or(0);
            let bench_iters = std::cmp::max(1, params["iters"].as_i64().unwrap_or(1)) as u64;
            let mut _json_inReal: Vec<f64> = Vec::new();
            let inReal: &[f64];
            if use_preloaded != 0 && ref_data.n > 0 {
                inReal = &ref_data.close[..ref_data.n];
            } else {
                _json_inReal = parse_f64_array(&params["inReal"]);
                inReal = &_json_inReal;
            }
            let out_size = if endIdx >= startIdx { endIdx - startIdx + 1 } else { 0 };
            let mut outBuf0: Vec<f64> = vec![0.0f64; out_size];
            let mut outBegIdx: usize = 0;
            let mut outNBElement: usize = 0;
            let mut rc = RetCode::Success;
            let start_time = Instant::now();
            for _bi in 0..bench_iters {
            rc = core.floor(
                startIdx, endIdx,
                &inReal,
                &mut outBegIdx, &mut outNBElement, &mut outBuf0,
            );
            }
            let elapsed_ns = start_time.elapsed().as_nanos() as u64 / bench_iters as u64;
            let start_time_ung = Instant::now();
            for _biu in 0..bench_iters {
            rc = core.floor_unguarded(
                startIdx, endIdx,
                &inReal,
                &mut outBegIdx, &mut outNBElement, &mut outBuf0,
            );
            }
            let elapsed_ns_ung = start_time_ung.elapsed().as_nanos() as u64 / bench_iters as u64;
            let lookback = core.floor_lookback();
            let mut resp = format!("{{\"retCode\":{},\"outBegIdx\":{},\"outNBElement\":{},\"lookback\":{},\"timing_ns\":{},\"timing_ns_unguarded\":{}", retcode_to_int(rc), outBegIdx, outNBElement, lookback, elapsed_ns, elapsed_ns_ung);
            resp.push_str(",\"outReal\":"); resp.push_str(&json_f64_array(&outBuf0[..outNBElement]));
            resp.push('}');
            resp
        }
        "TA_HT_DCPERIOD" => {
            let startIdx = params["startIdx"].as_u64().unwrap_or(0) as usize;
            let endIdx = params["endIdx"].as_u64().unwrap_or(0) as usize;
            let use_preloaded = params["use_preloaded"].as_i64().unwrap_or(0);
            let bench_iters = std::cmp::max(1, params["iters"].as_i64().unwrap_or(1)) as u64;
            let mut _json_inReal: Vec<f64> = Vec::new();
            let inReal: &[f64];
            if use_preloaded != 0 && ref_data.n > 0 {
                inReal = &ref_data.close[..ref_data.n];
            } else {
                _json_inReal = parse_f64_array(&params["inReal"]);
                inReal = &_json_inReal;
            }
            if let Some(period) = params["unstablePeriod"].as_i64() {
                core.unstable_period[6] = period as i32;
            }
            let out_size = if endIdx >= startIdx { endIdx - startIdx + 1 } else { 0 };
            let mut outBuf0: Vec<f64> = vec![0.0f64; out_size];
            let mut outBegIdx: usize = 0;
            let mut outNBElement: usize = 0;
            let mut rc = RetCode::Success;
            let start_time = Instant::now();
            for _bi in 0..bench_iters {
            rc = core.ht_dcperiod(
                startIdx, endIdx,
                &inReal,
                &mut outBegIdx, &mut outNBElement, &mut outBuf0,
            );
            }
            let elapsed_ns = start_time.elapsed().as_nanos() as u64 / bench_iters as u64;
            let start_time_ung = Instant::now();
            for _biu in 0..bench_iters {
            rc = core.ht_dcperiod_unguarded(
                startIdx, endIdx,
                &inReal,
                &mut outBegIdx, &mut outNBElement, &mut outBuf0,
            );
            }
            let elapsed_ns_ung = start_time_ung.elapsed().as_nanos() as u64 / bench_iters as u64;
            let lookback = core.ht_dcperiod_lookback();
            let mut resp = format!("{{\"retCode\":{},\"outBegIdx\":{},\"outNBElement\":{},\"lookback\":{},\"timing_ns\":{},\"timing_ns_unguarded\":{}", retcode_to_int(rc), outBegIdx, outNBElement, lookback, elapsed_ns, elapsed_ns_ung);
            resp.push_str(",\"outReal\":"); resp.push_str(&json_f64_array(&outBuf0[..outNBElement]));
            resp.push('}');
            resp
        }
        "TA_HT_DCPHASE" => {
            let startIdx = params["startIdx"].as_u64().unwrap_or(0) as usize;
            let endIdx = params["endIdx"].as_u64().unwrap_or(0) as usize;
            let use_preloaded = params["use_preloaded"].as_i64().unwrap_or(0);
            let bench_iters = std::cmp::max(1, params["iters"].as_i64().unwrap_or(1)) as u64;
            let mut _json_inReal: Vec<f64> = Vec::new();
            let inReal: &[f64];
            if use_preloaded != 0 && ref_data.n > 0 {
                inReal = &ref_data.close[..ref_data.n];
            } else {
                _json_inReal = parse_f64_array(&params["inReal"]);
                inReal = &_json_inReal;
            }
            if let Some(period) = params["unstablePeriod"].as_i64() {
                core.unstable_period[7] = period as i32;
            }
            let out_size = if endIdx >= startIdx { endIdx - startIdx + 1 } else { 0 };
            let mut outBuf0: Vec<f64> = vec![0.0f64; out_size];
            let mut outBegIdx: usize = 0;
            let mut outNBElement: usize = 0;
            let mut rc = RetCode::Success;
            let start_time = Instant::now();
            for _bi in 0..bench_iters {
            rc = core.ht_dcphase(
                startIdx, endIdx,
                &inReal,
                &mut outBegIdx, &mut outNBElement, &mut outBuf0,
            );
            }
            let elapsed_ns = start_time.elapsed().as_nanos() as u64 / bench_iters as u64;
            let start_time_ung = Instant::now();
            for _biu in 0..bench_iters {
            rc = core.ht_dcphase_unguarded(
                startIdx, endIdx,
                &inReal,
                &mut outBegIdx, &mut outNBElement, &mut outBuf0,
            );
            }
            let elapsed_ns_ung = start_time_ung.elapsed().as_nanos() as u64 / bench_iters as u64;
            let lookback = core.ht_dcphase_lookback();
            let mut resp = format!("{{\"retCode\":{},\"outBegIdx\":{},\"outNBElement\":{},\"lookback\":{},\"timing_ns\":{},\"timing_ns_unguarded\":{}", retcode_to_int(rc), outBegIdx, outNBElement, lookback, elapsed_ns, elapsed_ns_ung);
            resp.push_str(",\"outReal\":"); resp.push_str(&json_f64_array(&outBuf0[..outNBElement]));
            resp.push('}');
            resp
        }
        "TA_HT_PHASOR" => {
            let startIdx = params["startIdx"].as_u64().unwrap_or(0) as usize;
            let endIdx = params["endIdx"].as_u64().unwrap_or(0) as usize;
            let use_preloaded = params["use_preloaded"].as_i64().unwrap_or(0);
            let bench_iters = std::cmp::max(1, params["iters"].as_i64().unwrap_or(1)) as u64;
            let mut _json_inReal: Vec<f64> = Vec::new();
            let inReal: &[f64];
            if use_preloaded != 0 && ref_data.n > 0 {
                inReal = &ref_data.close[..ref_data.n];
            } else {
                _json_inReal = parse_f64_array(&params["inReal"]);
                inReal = &_json_inReal;
            }
            if let Some(period) = params["unstablePeriod"].as_i64() {
                core.unstable_period[8] = period as i32;
            }
            let out_size = if endIdx >= startIdx { endIdx - startIdx + 1 } else { 0 };
            let mut outBuf0: Vec<f64> = vec![0.0f64; out_size];
            let mut outBuf1: Vec<f64> = vec![0.0f64; out_size];
            let mut outBegIdx: usize = 0;
            let mut outNBElement: usize = 0;
            let mut rc = RetCode::Success;
            let start_time = Instant::now();
            for _bi in 0..bench_iters {
            rc = core.ht_phasor(
                startIdx, endIdx,
                &inReal,
                &mut outBegIdx, &mut outNBElement, &mut outBuf0, &mut outBuf1,
            );
            }
            let elapsed_ns = start_time.elapsed().as_nanos() as u64 / bench_iters as u64;
            let start_time_ung = Instant::now();
            for _biu in 0..bench_iters {
            rc = core.ht_phasor_unguarded(
                startIdx, endIdx,
                &inReal,
                &mut outBegIdx, &mut outNBElement, &mut outBuf0, &mut outBuf1,
            );
            }
            let elapsed_ns_ung = start_time_ung.elapsed().as_nanos() as u64 / bench_iters as u64;
            let lookback = core.ht_phasor_lookback();
            let mut resp = format!("{{\"retCode\":{},\"outBegIdx\":{},\"outNBElement\":{},\"lookback\":{},\"timing_ns\":{},\"timing_ns_unguarded\":{}", retcode_to_int(rc), outBegIdx, outNBElement, lookback, elapsed_ns, elapsed_ns_ung);
            resp.push_str(",\"outReal\":"); resp.push_str(&json_f64_array(&outBuf0[..outNBElement]));
            resp.push_str(",\"outReal1\":"); resp.push_str(&json_f64_array(&outBuf1[..outNBElement]));
            resp.push('}');
            resp
        }
        "TA_HT_SINE" => {
            let startIdx = params["startIdx"].as_u64().unwrap_or(0) as usize;
            let endIdx = params["endIdx"].as_u64().unwrap_or(0) as usize;
            let use_preloaded = params["use_preloaded"].as_i64().unwrap_or(0);
            let bench_iters = std::cmp::max(1, params["iters"].as_i64().unwrap_or(1)) as u64;
            let mut _json_inReal: Vec<f64> = Vec::new();
            let inReal: &[f64];
            if use_preloaded != 0 && ref_data.n > 0 {
                inReal = &ref_data.close[..ref_data.n];
            } else {
                _json_inReal = parse_f64_array(&params["inReal"]);
                inReal = &_json_inReal;
            }
            if let Some(period) = params["unstablePeriod"].as_i64() {
                core.unstable_period[9] = period as i32;
            }
            let out_size = if endIdx >= startIdx { endIdx - startIdx + 1 } else { 0 };
            let mut outBuf0: Vec<f64> = vec![0.0f64; out_size];
            let mut outBuf1: Vec<f64> = vec![0.0f64; out_size];
            let mut outBegIdx: usize = 0;
            let mut outNBElement: usize = 0;
            let mut rc = RetCode::Success;
            let start_time = Instant::now();
            for _bi in 0..bench_iters {
            rc = core.ht_sine(
                startIdx, endIdx,
                &inReal,
                &mut outBegIdx, &mut outNBElement, &mut outBuf0, &mut outBuf1,
            );
            }
            let elapsed_ns = start_time.elapsed().as_nanos() as u64 / bench_iters as u64;
            let start_time_ung = Instant::now();
            for _biu in 0..bench_iters {
            rc = core.ht_sine_unguarded(
                startIdx, endIdx,
                &inReal,
                &mut outBegIdx, &mut outNBElement, &mut outBuf0, &mut outBuf1,
            );
            }
            let elapsed_ns_ung = start_time_ung.elapsed().as_nanos() as u64 / bench_iters as u64;
            let lookback = core.ht_sine_lookback();
            let mut resp = format!("{{\"retCode\":{},\"outBegIdx\":{},\"outNBElement\":{},\"lookback\":{},\"timing_ns\":{},\"timing_ns_unguarded\":{}", retcode_to_int(rc), outBegIdx, outNBElement, lookback, elapsed_ns, elapsed_ns_ung);
            resp.push_str(",\"outReal\":"); resp.push_str(&json_f64_array(&outBuf0[..outNBElement]));
            resp.push_str(",\"outReal1\":"); resp.push_str(&json_f64_array(&outBuf1[..outNBElement]));
            resp.push('}');
            resp
        }
        "TA_HT_TRENDLINE" => {
            let startIdx = params["startIdx"].as_u64().unwrap_or(0) as usize;
            let endIdx = params["endIdx"].as_u64().unwrap_or(0) as usize;
            let use_preloaded = params["use_preloaded"].as_i64().unwrap_or(0);
            let bench_iters = std::cmp::max(1, params["iters"].as_i64().unwrap_or(1)) as u64;
            let mut _json_inReal: Vec<f64> = Vec::new();
            let inReal: &[f64];
            if use_preloaded != 0 && ref_data.n > 0 {
                inReal = &ref_data.close[..ref_data.n];
            } else {
                _json_inReal = parse_f64_array(&params["inReal"]);
                inReal = &_json_inReal;
            }
            if let Some(period) = params["unstablePeriod"].as_i64() {
                core.unstable_period[10] = period as i32;
            }
            let out_size = if endIdx >= startIdx { endIdx - startIdx + 1 } else { 0 };
            let mut outBuf0: Vec<f64> = vec![0.0f64; out_size];
            let mut outBegIdx: usize = 0;
            let mut outNBElement: usize = 0;
            let mut rc = RetCode::Success;
            let start_time = Instant::now();
            for _bi in 0..bench_iters {
            rc = core.ht_trendline(
                startIdx, endIdx,
                &inReal,
                &mut outBegIdx, &mut outNBElement, &mut outBuf0,
            );
            }
            let elapsed_ns = start_time.elapsed().as_nanos() as u64 / bench_iters as u64;
            let start_time_ung = Instant::now();
            for _biu in 0..bench_iters {
            rc = core.ht_trendline_unguarded(
                startIdx, endIdx,
                &inReal,
                &mut outBegIdx, &mut outNBElement, &mut outBuf0,
            );
            }
            let elapsed_ns_ung = start_time_ung.elapsed().as_nanos() as u64 / bench_iters as u64;
            let lookback = core.ht_trendline_lookback();
            let mut resp = format!("{{\"retCode\":{},\"outBegIdx\":{},\"outNBElement\":{},\"lookback\":{},\"timing_ns\":{},\"timing_ns_unguarded\":{}", retcode_to_int(rc), outBegIdx, outNBElement, lookback, elapsed_ns, elapsed_ns_ung);
            resp.push_str(",\"outReal\":"); resp.push_str(&json_f64_array(&outBuf0[..outNBElement]));
            resp.push('}');
            resp
        }
        "TA_HT_TRENDMODE" => {
            let startIdx = params["startIdx"].as_u64().unwrap_or(0) as usize;
            let endIdx = params["endIdx"].as_u64().unwrap_or(0) as usize;
            let use_preloaded = params["use_preloaded"].as_i64().unwrap_or(0);
            let bench_iters = std::cmp::max(1, params["iters"].as_i64().unwrap_or(1)) as u64;
            let mut _json_inReal: Vec<f64> = Vec::new();
            let inReal: &[f64];
            if use_preloaded != 0 && ref_data.n > 0 {
                inReal = &ref_data.close[..ref_data.n];
            } else {
                _json_inReal = parse_f64_array(&params["inReal"]);
                inReal = &_json_inReal;
            }
            if let Some(period) = params["unstablePeriod"].as_i64() {
                core.unstable_period[11] = period as i32;
            }
            let out_size = if endIdx >= startIdx { endIdx - startIdx + 1 } else { 0 };
            let mut outIntBuf0: Vec<i32> = vec![0i32; out_size];
            let mut outBegIdx: usize = 0;
            let mut outNBElement: usize = 0;
            let mut rc = RetCode::Success;
            let start_time = Instant::now();
            for _bi in 0..bench_iters {
            rc = core.ht_trendmode(
                startIdx, endIdx,
                &inReal,
                &mut outBegIdx, &mut outNBElement, &mut outIntBuf0,
            );
            }
            let elapsed_ns = start_time.elapsed().as_nanos() as u64 / bench_iters as u64;
            let start_time_ung = Instant::now();
            for _biu in 0..bench_iters {
            rc = core.ht_trendmode_unguarded(
                startIdx, endIdx,
                &inReal,
                &mut outBegIdx, &mut outNBElement, &mut outIntBuf0,
            );
            }
            let elapsed_ns_ung = start_time_ung.elapsed().as_nanos() as u64 / bench_iters as u64;
            let lookback = core.ht_trendmode_lookback();
            let mut resp = format!("{{\"retCode\":{},\"outBegIdx\":{},\"outNBElement\":{},\"lookback\":{},\"timing_ns\":{},\"timing_ns_unguarded\":{}", retcode_to_int(rc), outBegIdx, outNBElement, lookback, elapsed_ns, elapsed_ns_ung);
            resp.push_str(",\"outInteger\":"); resp.push_str(&json_i32_array(&outIntBuf0[..outNBElement]));
            resp.push('}');
            resp
        }
        "TA_IMI" => {
            let startIdx = params["startIdx"].as_u64().unwrap_or(0) as usize;
            let endIdx = params["endIdx"].as_u64().unwrap_or(0) as usize;
            let use_preloaded = params["use_preloaded"].as_i64().unwrap_or(0);
            let bench_iters = std::cmp::max(1, params["iters"].as_i64().unwrap_or(1)) as u64;
            let mut _json_inOpen: Vec<f64> = Vec::new();
            let mut _json_inClose: Vec<f64> = Vec::new();
            let inOpen: &[f64];
            let inClose: &[f64];
            if use_preloaded != 0 && ref_data.n > 0 {
                inOpen = &ref_data.open[..ref_data.n];
                inClose = &ref_data.close[..ref_data.n];
            } else {
                _json_inOpen = parse_f64_array(&params["inOpen"]);
                inOpen = &_json_inOpen;
                _json_inClose = parse_f64_array(&params["inClose"]);
                inClose = &_json_inClose;
            }
            let optInTimePeriod = params["optInTimePeriod"].as_i64().unwrap_or(14) as i32;
            if let Some(period) = params["unstablePeriod"].as_i64() {
                core.unstable_period[12] = period as i32;
            }
            let out_size = if endIdx >= startIdx { endIdx - startIdx + 1 } else { 0 };
            let mut outBuf0: Vec<f64> = vec![0.0f64; out_size];
            let mut outBegIdx: usize = 0;
            let mut outNBElement: usize = 0;
            let mut rc = RetCode::Success;
            let start_time = Instant::now();
            for _bi in 0..bench_iters {
            rc = core.imi(
                startIdx, endIdx,
                &inOpen,
                &inClose,
                optInTimePeriod,
                &mut outBegIdx, &mut outNBElement, &mut outBuf0,
            );
            }
            let elapsed_ns = start_time.elapsed().as_nanos() as u64 / bench_iters as u64;
            let start_time_ung = Instant::now();
            for _biu in 0..bench_iters {
            rc = core.imi_unguarded(
                startIdx, endIdx,
                &inOpen,
                &inClose,
                optInTimePeriod,
                &mut outBegIdx, &mut outNBElement, &mut outBuf0,
            );
            }
            let elapsed_ns_ung = start_time_ung.elapsed().as_nanos() as u64 / bench_iters as u64;
            let lookback = core.imi_lookback(optInTimePeriod);
            let mut resp = format!("{{\"retCode\":{},\"outBegIdx\":{},\"outNBElement\":{},\"lookback\":{},\"timing_ns\":{},\"timing_ns_unguarded\":{}", retcode_to_int(rc), outBegIdx, outNBElement, lookback, elapsed_ns, elapsed_ns_ung);
            resp.push_str(",\"outReal\":"); resp.push_str(&json_f64_array(&outBuf0[..outNBElement]));
            resp.push('}');
            resp
        }
        "TA_KAMA" => {
            let startIdx = params["startIdx"].as_u64().unwrap_or(0) as usize;
            let endIdx = params["endIdx"].as_u64().unwrap_or(0) as usize;
            let use_preloaded = params["use_preloaded"].as_i64().unwrap_or(0);
            let bench_iters = std::cmp::max(1, params["iters"].as_i64().unwrap_or(1)) as u64;
            let mut _json_inReal: Vec<f64> = Vec::new();
            let inReal: &[f64];
            if use_preloaded != 0 && ref_data.n > 0 {
                inReal = &ref_data.close[..ref_data.n];
            } else {
                _json_inReal = parse_f64_array(&params["inReal"]);
                inReal = &_json_inReal;
            }
            let optInTimePeriod = params["optInTimePeriod"].as_i64().unwrap_or(30) as i32;
            if let Some(period) = params["unstablePeriod"].as_i64() {
                core.unstable_period[13] = period as i32;
            }
            let out_size = if endIdx >= startIdx { endIdx - startIdx + 1 } else { 0 };
            let mut outBuf0: Vec<f64> = vec![0.0f64; out_size];
            let mut outBegIdx: usize = 0;
            let mut outNBElement: usize = 0;
            let mut rc = RetCode::Success;
            let start_time = Instant::now();
            for _bi in 0..bench_iters {
            rc = core.kama(
                startIdx, endIdx,
                &inReal,
                optInTimePeriod,
                &mut outBegIdx, &mut outNBElement, &mut outBuf0,
            );
            }
            let elapsed_ns = start_time.elapsed().as_nanos() as u64 / bench_iters as u64;
            let start_time_ung = Instant::now();
            for _biu in 0..bench_iters {
            rc = core.kama_unguarded(
                startIdx, endIdx,
                &inReal,
                optInTimePeriod,
                &mut outBegIdx, &mut outNBElement, &mut outBuf0,
            );
            }
            let elapsed_ns_ung = start_time_ung.elapsed().as_nanos() as u64 / bench_iters as u64;
            let lookback = core.kama_lookback(optInTimePeriod);
            let mut resp = format!("{{\"retCode\":{},\"outBegIdx\":{},\"outNBElement\":{},\"lookback\":{},\"timing_ns\":{},\"timing_ns_unguarded\":{}", retcode_to_int(rc), outBegIdx, outNBElement, lookback, elapsed_ns, elapsed_ns_ung);
            resp.push_str(",\"outReal\":"); resp.push_str(&json_f64_array(&outBuf0[..outNBElement]));
            resp.push('}');
            resp
        }
        "TA_LINEARREG" => {
            let startIdx = params["startIdx"].as_u64().unwrap_or(0) as usize;
            let endIdx = params["endIdx"].as_u64().unwrap_or(0) as usize;
            let use_preloaded = params["use_preloaded"].as_i64().unwrap_or(0);
            let bench_iters = std::cmp::max(1, params["iters"].as_i64().unwrap_or(1)) as u64;
            let mut _json_inReal: Vec<f64> = Vec::new();
            let inReal: &[f64];
            if use_preloaded != 0 && ref_data.n > 0 {
                inReal = &ref_data.close[..ref_data.n];
            } else {
                _json_inReal = parse_f64_array(&params["inReal"]);
                inReal = &_json_inReal;
            }
            let optInTimePeriod = params["optInTimePeriod"].as_i64().unwrap_or(14) as i32;
            let out_size = if endIdx >= startIdx { endIdx - startIdx + 1 } else { 0 };
            let mut outBuf0: Vec<f64> = vec![0.0f64; out_size];
            let mut outBegIdx: usize = 0;
            let mut outNBElement: usize = 0;
            let mut rc = RetCode::Success;
            let start_time = Instant::now();
            for _bi in 0..bench_iters {
            rc = core.linearreg(
                startIdx, endIdx,
                &inReal,
                optInTimePeriod,
                &mut outBegIdx, &mut outNBElement, &mut outBuf0,
            );
            }
            let elapsed_ns = start_time.elapsed().as_nanos() as u64 / bench_iters as u64;
            let start_time_ung = Instant::now();
            for _biu in 0..bench_iters {
            rc = core.linearreg_unguarded(
                startIdx, endIdx,
                &inReal,
                optInTimePeriod,
                &mut outBegIdx, &mut outNBElement, &mut outBuf0,
            );
            }
            let elapsed_ns_ung = start_time_ung.elapsed().as_nanos() as u64 / bench_iters as u64;
            let lookback = core.linearreg_lookback(optInTimePeriod);
            let mut resp = format!("{{\"retCode\":{},\"outBegIdx\":{},\"outNBElement\":{},\"lookback\":{},\"timing_ns\":{},\"timing_ns_unguarded\":{}", retcode_to_int(rc), outBegIdx, outNBElement, lookback, elapsed_ns, elapsed_ns_ung);
            resp.push_str(",\"outReal\":"); resp.push_str(&json_f64_array(&outBuf0[..outNBElement]));
            resp.push('}');
            resp
        }
        "TA_LINEARREG_ANGLE" => {
            let startIdx = params["startIdx"].as_u64().unwrap_or(0) as usize;
            let endIdx = params["endIdx"].as_u64().unwrap_or(0) as usize;
            let use_preloaded = params["use_preloaded"].as_i64().unwrap_or(0);
            let bench_iters = std::cmp::max(1, params["iters"].as_i64().unwrap_or(1)) as u64;
            let mut _json_inReal: Vec<f64> = Vec::new();
            let inReal: &[f64];
            if use_preloaded != 0 && ref_data.n > 0 {
                inReal = &ref_data.close[..ref_data.n];
            } else {
                _json_inReal = parse_f64_array(&params["inReal"]);
                inReal = &_json_inReal;
            }
            let optInTimePeriod = params["optInTimePeriod"].as_i64().unwrap_or(14) as i32;
            let out_size = if endIdx >= startIdx { endIdx - startIdx + 1 } else { 0 };
            let mut outBuf0: Vec<f64> = vec![0.0f64; out_size];
            let mut outBegIdx: usize = 0;
            let mut outNBElement: usize = 0;
            let mut rc = RetCode::Success;
            let start_time = Instant::now();
            for _bi in 0..bench_iters {
            rc = core.linearreg_angle(
                startIdx, endIdx,
                &inReal,
                optInTimePeriod,
                &mut outBegIdx, &mut outNBElement, &mut outBuf0,
            );
            }
            let elapsed_ns = start_time.elapsed().as_nanos() as u64 / bench_iters as u64;
            let start_time_ung = Instant::now();
            for _biu in 0..bench_iters {
            rc = core.linearreg_angle_unguarded(
                startIdx, endIdx,
                &inReal,
                optInTimePeriod,
                &mut outBegIdx, &mut outNBElement, &mut outBuf0,
            );
            }
            let elapsed_ns_ung = start_time_ung.elapsed().as_nanos() as u64 / bench_iters as u64;
            let lookback = core.linearreg_angle_lookback(optInTimePeriod);
            let mut resp = format!("{{\"retCode\":{},\"outBegIdx\":{},\"outNBElement\":{},\"lookback\":{},\"timing_ns\":{},\"timing_ns_unguarded\":{}", retcode_to_int(rc), outBegIdx, outNBElement, lookback, elapsed_ns, elapsed_ns_ung);
            resp.push_str(",\"outReal\":"); resp.push_str(&json_f64_array(&outBuf0[..outNBElement]));
            resp.push('}');
            resp
        }
        "TA_LINEARREG_INTERCEPT" => {
            let startIdx = params["startIdx"].as_u64().unwrap_or(0) as usize;
            let endIdx = params["endIdx"].as_u64().unwrap_or(0) as usize;
            let use_preloaded = params["use_preloaded"].as_i64().unwrap_or(0);
            let bench_iters = std::cmp::max(1, params["iters"].as_i64().unwrap_or(1)) as u64;
            let mut _json_inReal: Vec<f64> = Vec::new();
            let inReal: &[f64];
            if use_preloaded != 0 && ref_data.n > 0 {
                inReal = &ref_data.close[..ref_data.n];
            } else {
                _json_inReal = parse_f64_array(&params["inReal"]);
                inReal = &_json_inReal;
            }
            let optInTimePeriod = params["optInTimePeriod"].as_i64().unwrap_or(14) as i32;
            let out_size = if endIdx >= startIdx { endIdx - startIdx + 1 } else { 0 };
            let mut outBuf0: Vec<f64> = vec![0.0f64; out_size];
            let mut outBegIdx: usize = 0;
            let mut outNBElement: usize = 0;
            let mut rc = RetCode::Success;
            let start_time = Instant::now();
            for _bi in 0..bench_iters {
            rc = core.linearreg_intercept(
                startIdx, endIdx,
                &inReal,
                optInTimePeriod,
                &mut outBegIdx, &mut outNBElement, &mut outBuf0,
            );
            }
            let elapsed_ns = start_time.elapsed().as_nanos() as u64 / bench_iters as u64;
            let start_time_ung = Instant::now();
            for _biu in 0..bench_iters {
            rc = core.linearreg_intercept_unguarded(
                startIdx, endIdx,
                &inReal,
                optInTimePeriod,
                &mut outBegIdx, &mut outNBElement, &mut outBuf0,
            );
            }
            let elapsed_ns_ung = start_time_ung.elapsed().as_nanos() as u64 / bench_iters as u64;
            let lookback = core.linearreg_intercept_lookback(optInTimePeriod);
            let mut resp = format!("{{\"retCode\":{},\"outBegIdx\":{},\"outNBElement\":{},\"lookback\":{},\"timing_ns\":{},\"timing_ns_unguarded\":{}", retcode_to_int(rc), outBegIdx, outNBElement, lookback, elapsed_ns, elapsed_ns_ung);
            resp.push_str(",\"outReal\":"); resp.push_str(&json_f64_array(&outBuf0[..outNBElement]));
            resp.push('}');
            resp
        }
        "TA_LINEARREG_SLOPE" => {
            let startIdx = params["startIdx"].as_u64().unwrap_or(0) as usize;
            let endIdx = params["endIdx"].as_u64().unwrap_or(0) as usize;
            let use_preloaded = params["use_preloaded"].as_i64().unwrap_or(0);
            let bench_iters = std::cmp::max(1, params["iters"].as_i64().unwrap_or(1)) as u64;
            let mut _json_inReal: Vec<f64> = Vec::new();
            let inReal: &[f64];
            if use_preloaded != 0 && ref_data.n > 0 {
                inReal = &ref_data.close[..ref_data.n];
            } else {
                _json_inReal = parse_f64_array(&params["inReal"]);
                inReal = &_json_inReal;
            }
            let optInTimePeriod = params["optInTimePeriod"].as_i64().unwrap_or(14) as i32;
            let out_size = if endIdx >= startIdx { endIdx - startIdx + 1 } else { 0 };
            let mut outBuf0: Vec<f64> = vec![0.0f64; out_size];
            let mut outBegIdx: usize = 0;
            let mut outNBElement: usize = 0;
            let mut rc = RetCode::Success;
            let start_time = Instant::now();
            for _bi in 0..bench_iters {
            rc = core.linearreg_slope(
                startIdx, endIdx,
                &inReal,
                optInTimePeriod,
                &mut outBegIdx, &mut outNBElement, &mut outBuf0,
            );
            }
            let elapsed_ns = start_time.elapsed().as_nanos() as u64 / bench_iters as u64;
            let start_time_ung = Instant::now();
            for _biu in 0..bench_iters {
            rc = core.linearreg_slope_unguarded(
                startIdx, endIdx,
                &inReal,
                optInTimePeriod,
                &mut outBegIdx, &mut outNBElement, &mut outBuf0,
            );
            }
            let elapsed_ns_ung = start_time_ung.elapsed().as_nanos() as u64 / bench_iters as u64;
            let lookback = core.linearreg_slope_lookback(optInTimePeriod);
            let mut resp = format!("{{\"retCode\":{},\"outBegIdx\":{},\"outNBElement\":{},\"lookback\":{},\"timing_ns\":{},\"timing_ns_unguarded\":{}", retcode_to_int(rc), outBegIdx, outNBElement, lookback, elapsed_ns, elapsed_ns_ung);
            resp.push_str(",\"outReal\":"); resp.push_str(&json_f64_array(&outBuf0[..outNBElement]));
            resp.push('}');
            resp
        }
        "TA_LN" => {
            let startIdx = params["startIdx"].as_u64().unwrap_or(0) as usize;
            let endIdx = params["endIdx"].as_u64().unwrap_or(0) as usize;
            let use_preloaded = params["use_preloaded"].as_i64().unwrap_or(0);
            let bench_iters = std::cmp::max(1, params["iters"].as_i64().unwrap_or(1)) as u64;
            let mut _json_inReal: Vec<f64> = Vec::new();
            let inReal: &[f64];
            if use_preloaded != 0 && ref_data.n > 0 {
                inReal = &ref_data.close[..ref_data.n];
            } else {
                _json_inReal = parse_f64_array(&params["inReal"]);
                inReal = &_json_inReal;
            }
            let out_size = if endIdx >= startIdx { endIdx - startIdx + 1 } else { 0 };
            let mut outBuf0: Vec<f64> = vec![0.0f64; out_size];
            let mut outBegIdx: usize = 0;
            let mut outNBElement: usize = 0;
            let mut rc = RetCode::Success;
            let start_time = Instant::now();
            for _bi in 0..bench_iters {
            rc = core.ln(
                startIdx, endIdx,
                &inReal,
                &mut outBegIdx, &mut outNBElement, &mut outBuf0,
            );
            }
            let elapsed_ns = start_time.elapsed().as_nanos() as u64 / bench_iters as u64;
            let start_time_ung = Instant::now();
            for _biu in 0..bench_iters {
            rc = core.ln_unguarded(
                startIdx, endIdx,
                &inReal,
                &mut outBegIdx, &mut outNBElement, &mut outBuf0,
            );
            }
            let elapsed_ns_ung = start_time_ung.elapsed().as_nanos() as u64 / bench_iters as u64;
            let lookback = core.ln_lookback();
            let mut resp = format!("{{\"retCode\":{},\"outBegIdx\":{},\"outNBElement\":{},\"lookback\":{},\"timing_ns\":{},\"timing_ns_unguarded\":{}", retcode_to_int(rc), outBegIdx, outNBElement, lookback, elapsed_ns, elapsed_ns_ung);
            resp.push_str(",\"outReal\":"); resp.push_str(&json_f64_array(&outBuf0[..outNBElement]));
            resp.push('}');
            resp
        }
        "TA_LOG10" => {
            let startIdx = params["startIdx"].as_u64().unwrap_or(0) as usize;
            let endIdx = params["endIdx"].as_u64().unwrap_or(0) as usize;
            let use_preloaded = params["use_preloaded"].as_i64().unwrap_or(0);
            let bench_iters = std::cmp::max(1, params["iters"].as_i64().unwrap_or(1)) as u64;
            let mut _json_inReal: Vec<f64> = Vec::new();
            let inReal: &[f64];
            if use_preloaded != 0 && ref_data.n > 0 {
                inReal = &ref_data.close[..ref_data.n];
            } else {
                _json_inReal = parse_f64_array(&params["inReal"]);
                inReal = &_json_inReal;
            }
            let out_size = if endIdx >= startIdx { endIdx - startIdx + 1 } else { 0 };
            let mut outBuf0: Vec<f64> = vec![0.0f64; out_size];
            let mut outBegIdx: usize = 0;
            let mut outNBElement: usize = 0;
            let mut rc = RetCode::Success;
            let start_time = Instant::now();
            for _bi in 0..bench_iters {
            rc = core.log10(
                startIdx, endIdx,
                &inReal,
                &mut outBegIdx, &mut outNBElement, &mut outBuf0,
            );
            }
            let elapsed_ns = start_time.elapsed().as_nanos() as u64 / bench_iters as u64;
            let start_time_ung = Instant::now();
            for _biu in 0..bench_iters {
            rc = core.log10_unguarded(
                startIdx, endIdx,
                &inReal,
                &mut outBegIdx, &mut outNBElement, &mut outBuf0,
            );
            }
            let elapsed_ns_ung = start_time_ung.elapsed().as_nanos() as u64 / bench_iters as u64;
            let lookback = core.log10_lookback();
            let mut resp = format!("{{\"retCode\":{},\"outBegIdx\":{},\"outNBElement\":{},\"lookback\":{},\"timing_ns\":{},\"timing_ns_unguarded\":{}", retcode_to_int(rc), outBegIdx, outNBElement, lookback, elapsed_ns, elapsed_ns_ung);
            resp.push_str(",\"outReal\":"); resp.push_str(&json_f64_array(&outBuf0[..outNBElement]));
            resp.push('}');
            resp
        }
        "TA_MA" => {
            let startIdx = params["startIdx"].as_u64().unwrap_or(0) as usize;
            let endIdx = params["endIdx"].as_u64().unwrap_or(0) as usize;
            let use_preloaded = params["use_preloaded"].as_i64().unwrap_or(0);
            let bench_iters = std::cmp::max(1, params["iters"].as_i64().unwrap_or(1)) as u64;
            let mut _json_inReal: Vec<f64> = Vec::new();
            let inReal: &[f64];
            if use_preloaded != 0 && ref_data.n > 0 {
                inReal = &ref_data.close[..ref_data.n];
            } else {
                _json_inReal = parse_f64_array(&params["inReal"]);
                inReal = &_json_inReal;
            }
            let optInTimePeriod = params["optInTimePeriod"].as_i64().unwrap_or(30) as i32;
            let optInMAType = params["optInMAType"].as_i64().unwrap_or(0) as i32;
            let out_size = if endIdx >= startIdx { endIdx - startIdx + 1 } else { 0 };
            let mut outBuf0: Vec<f64> = vec![0.0f64; out_size];
            let mut outBegIdx: usize = 0;
            let mut outNBElement: usize = 0;
            let mut rc = RetCode::Success;
            let start_time = Instant::now();
            for _bi in 0..bench_iters {
            rc = core.ma(
                startIdx, endIdx,
                &inReal,
                optInTimePeriod,
                optInMAType,
                &mut outBegIdx, &mut outNBElement, &mut outBuf0,
            );
            }
            let elapsed_ns = start_time.elapsed().as_nanos() as u64 / bench_iters as u64;
            let start_time_ung = Instant::now();
            for _biu in 0..bench_iters {
            rc = core.ma_unguarded(
                startIdx, endIdx,
                &inReal,
                optInTimePeriod,
                optInMAType,
                &mut outBegIdx, &mut outNBElement, &mut outBuf0,
            );
            }
            let elapsed_ns_ung = start_time_ung.elapsed().as_nanos() as u64 / bench_iters as u64;
            let lookback = core.ma_lookback(optInTimePeriod, optInMAType);
            let mut resp = format!("{{\"retCode\":{},\"outBegIdx\":{},\"outNBElement\":{},\"lookback\":{},\"timing_ns\":{},\"timing_ns_unguarded\":{}", retcode_to_int(rc), outBegIdx, outNBElement, lookback, elapsed_ns, elapsed_ns_ung);
            resp.push_str(",\"outReal\":"); resp.push_str(&json_f64_array(&outBuf0[..outNBElement]));
            resp.push('}');
            resp
        }
        "TA_MACD" => {
            let startIdx = params["startIdx"].as_u64().unwrap_or(0) as usize;
            let endIdx = params["endIdx"].as_u64().unwrap_or(0) as usize;
            let use_preloaded = params["use_preloaded"].as_i64().unwrap_or(0);
            let bench_iters = std::cmp::max(1, params["iters"].as_i64().unwrap_or(1)) as u64;
            let mut _json_inReal: Vec<f64> = Vec::new();
            let inReal: &[f64];
            if use_preloaded != 0 && ref_data.n > 0 {
                inReal = &ref_data.close[..ref_data.n];
            } else {
                _json_inReal = parse_f64_array(&params["inReal"]);
                inReal = &_json_inReal;
            }
            let optInFastPeriod = params["optInFastPeriod"].as_i64().unwrap_or(12) as i32;
            let optInSlowPeriod = params["optInSlowPeriod"].as_i64().unwrap_or(26) as i32;
            let optInSignalPeriod = params["optInSignalPeriod"].as_i64().unwrap_or(9) as i32;
            let out_size = if endIdx >= startIdx { endIdx - startIdx + 1 } else { 0 };
            let mut outBuf0: Vec<f64> = vec![0.0f64; out_size];
            let mut outBuf1: Vec<f64> = vec![0.0f64; out_size];
            let mut outBuf2: Vec<f64> = vec![0.0f64; out_size];
            let mut outBegIdx: usize = 0;
            let mut outNBElement: usize = 0;
            let mut rc = RetCode::Success;
            let start_time = Instant::now();
            for _bi in 0..bench_iters {
            rc = core.macd(
                startIdx, endIdx,
                &inReal,
                optInFastPeriod,
                optInSlowPeriod,
                optInSignalPeriod,
                &mut outBegIdx, &mut outNBElement, &mut outBuf0, &mut outBuf1, &mut outBuf2,
            );
            }
            let elapsed_ns = start_time.elapsed().as_nanos() as u64 / bench_iters as u64;
            let start_time_ung = Instant::now();
            for _biu in 0..bench_iters {
            rc = core.macd_unguarded(
                startIdx, endIdx,
                &inReal,
                optInFastPeriod,
                optInSlowPeriod,
                optInSignalPeriod,
                &mut outBegIdx, &mut outNBElement, &mut outBuf0, &mut outBuf1, &mut outBuf2,
            );
            }
            let elapsed_ns_ung = start_time_ung.elapsed().as_nanos() as u64 / bench_iters as u64;
            let lookback = core.macd_lookback(optInFastPeriod, optInSlowPeriod, optInSignalPeriod);
            let mut resp = format!("{{\"retCode\":{},\"outBegIdx\":{},\"outNBElement\":{},\"lookback\":{},\"timing_ns\":{},\"timing_ns_unguarded\":{}", retcode_to_int(rc), outBegIdx, outNBElement, lookback, elapsed_ns, elapsed_ns_ung);
            resp.push_str(",\"outReal\":"); resp.push_str(&json_f64_array(&outBuf0[..outNBElement]));
            resp.push_str(",\"outReal1\":"); resp.push_str(&json_f64_array(&outBuf1[..outNBElement]));
            resp.push_str(",\"outReal2\":"); resp.push_str(&json_f64_array(&outBuf2[..outNBElement]));
            resp.push('}');
            resp
        }
        "TA_MACDEXT" => {
            let startIdx = params["startIdx"].as_u64().unwrap_or(0) as usize;
            let endIdx = params["endIdx"].as_u64().unwrap_or(0) as usize;
            let use_preloaded = params["use_preloaded"].as_i64().unwrap_or(0);
            let bench_iters = std::cmp::max(1, params["iters"].as_i64().unwrap_or(1)) as u64;
            let mut _json_inReal: Vec<f64> = Vec::new();
            let inReal: &[f64];
            if use_preloaded != 0 && ref_data.n > 0 {
                inReal = &ref_data.close[..ref_data.n];
            } else {
                _json_inReal = parse_f64_array(&params["inReal"]);
                inReal = &_json_inReal;
            }
            let optInFastPeriod = params["optInFastPeriod"].as_i64().unwrap_or(12) as i32;
            let optInFastMAType = params["optInFastMAType"].as_i64().unwrap_or(0) as i32;
            let optInSlowPeriod = params["optInSlowPeriod"].as_i64().unwrap_or(26) as i32;
            let optInSlowMAType = params["optInSlowMAType"].as_i64().unwrap_or(0) as i32;
            let optInSignalPeriod = params["optInSignalPeriod"].as_i64().unwrap_or(9) as i32;
            let optInSignalMAType = params["optInSignalMAType"].as_i64().unwrap_or(0) as i32;
            let out_size = if endIdx >= startIdx { endIdx - startIdx + 1 } else { 0 };
            let mut outBuf0: Vec<f64> = vec![0.0f64; out_size];
            let mut outBuf1: Vec<f64> = vec![0.0f64; out_size];
            let mut outBuf2: Vec<f64> = vec![0.0f64; out_size];
            let mut outBegIdx: usize = 0;
            let mut outNBElement: usize = 0;
            let mut rc = RetCode::Success;
            let start_time = Instant::now();
            for _bi in 0..bench_iters {
            rc = core.macdext(
                startIdx, endIdx,
                &inReal,
                optInFastPeriod,
                optInFastMAType,
                optInSlowPeriod,
                optInSlowMAType,
                optInSignalPeriod,
                optInSignalMAType,
                &mut outBegIdx, &mut outNBElement, &mut outBuf0, &mut outBuf1, &mut outBuf2,
            );
            }
            let elapsed_ns = start_time.elapsed().as_nanos() as u64 / bench_iters as u64;
            let start_time_ung = Instant::now();
            for _biu in 0..bench_iters {
            rc = core.macdext_unguarded(
                startIdx, endIdx,
                &inReal,
                optInFastPeriod,
                optInFastMAType,
                optInSlowPeriod,
                optInSlowMAType,
                optInSignalPeriod,
                optInSignalMAType,
                &mut outBegIdx, &mut outNBElement, &mut outBuf0, &mut outBuf1, &mut outBuf2,
            );
            }
            let elapsed_ns_ung = start_time_ung.elapsed().as_nanos() as u64 / bench_iters as u64;
            let lookback = core.macdext_lookback(optInFastPeriod, optInFastMAType, optInSlowPeriod, optInSlowMAType, optInSignalPeriod, optInSignalMAType);
            let mut resp = format!("{{\"retCode\":{},\"outBegIdx\":{},\"outNBElement\":{},\"lookback\":{},\"timing_ns\":{},\"timing_ns_unguarded\":{}", retcode_to_int(rc), outBegIdx, outNBElement, lookback, elapsed_ns, elapsed_ns_ung);
            resp.push_str(",\"outReal\":"); resp.push_str(&json_f64_array(&outBuf0[..outNBElement]));
            resp.push_str(",\"outReal1\":"); resp.push_str(&json_f64_array(&outBuf1[..outNBElement]));
            resp.push_str(",\"outReal2\":"); resp.push_str(&json_f64_array(&outBuf2[..outNBElement]));
            resp.push('}');
            resp
        }
        "TA_MACDFIX" => {
            let startIdx = params["startIdx"].as_u64().unwrap_or(0) as usize;
            let endIdx = params["endIdx"].as_u64().unwrap_or(0) as usize;
            let use_preloaded = params["use_preloaded"].as_i64().unwrap_or(0);
            let bench_iters = std::cmp::max(1, params["iters"].as_i64().unwrap_or(1)) as u64;
            let mut _json_inReal: Vec<f64> = Vec::new();
            let inReal: &[f64];
            if use_preloaded != 0 && ref_data.n > 0 {
                inReal = &ref_data.close[..ref_data.n];
            } else {
                _json_inReal = parse_f64_array(&params["inReal"]);
                inReal = &_json_inReal;
            }
            let optInSignalPeriod = params["optInSignalPeriod"].as_i64().unwrap_or(9) as i32;
            let out_size = if endIdx >= startIdx { endIdx - startIdx + 1 } else { 0 };
            let mut outBuf0: Vec<f64> = vec![0.0f64; out_size];
            let mut outBuf1: Vec<f64> = vec![0.0f64; out_size];
            let mut outBuf2: Vec<f64> = vec![0.0f64; out_size];
            let mut outBegIdx: usize = 0;
            let mut outNBElement: usize = 0;
            let mut rc = RetCode::Success;
            let start_time = Instant::now();
            for _bi in 0..bench_iters {
            rc = core.macdfix(
                startIdx, endIdx,
                &inReal,
                optInSignalPeriod,
                &mut outBegIdx, &mut outNBElement, &mut outBuf0, &mut outBuf1, &mut outBuf2,
            );
            }
            let elapsed_ns = start_time.elapsed().as_nanos() as u64 / bench_iters as u64;
            let start_time_ung = Instant::now();
            for _biu in 0..bench_iters {
            rc = core.macdfix_unguarded(
                startIdx, endIdx,
                &inReal,
                optInSignalPeriod,
                &mut outBegIdx, &mut outNBElement, &mut outBuf0, &mut outBuf1, &mut outBuf2,
            );
            }
            let elapsed_ns_ung = start_time_ung.elapsed().as_nanos() as u64 / bench_iters as u64;
            let lookback = core.macdfix_lookback(optInSignalPeriod);
            let mut resp = format!("{{\"retCode\":{},\"outBegIdx\":{},\"outNBElement\":{},\"lookback\":{},\"timing_ns\":{},\"timing_ns_unguarded\":{}", retcode_to_int(rc), outBegIdx, outNBElement, lookback, elapsed_ns, elapsed_ns_ung);
            resp.push_str(",\"outReal\":"); resp.push_str(&json_f64_array(&outBuf0[..outNBElement]));
            resp.push_str(",\"outReal1\":"); resp.push_str(&json_f64_array(&outBuf1[..outNBElement]));
            resp.push_str(",\"outReal2\":"); resp.push_str(&json_f64_array(&outBuf2[..outNBElement]));
            resp.push('}');
            resp
        }
        "TA_MAMA" => {
            let startIdx = params["startIdx"].as_u64().unwrap_or(0) as usize;
            let endIdx = params["endIdx"].as_u64().unwrap_or(0) as usize;
            let use_preloaded = params["use_preloaded"].as_i64().unwrap_or(0);
            let bench_iters = std::cmp::max(1, params["iters"].as_i64().unwrap_or(1)) as u64;
            let mut _json_inReal: Vec<f64> = Vec::new();
            let inReal: &[f64];
            if use_preloaded != 0 && ref_data.n > 0 {
                inReal = &ref_data.close[..ref_data.n];
            } else {
                _json_inReal = parse_f64_array(&params["inReal"]);
                inReal = &_json_inReal;
            }
            let optInFastLimit = params["optInFastLimit"].as_f64().unwrap_or(0.5) as f64;
            let optInSlowLimit = params["optInSlowLimit"].as_f64().unwrap_or(0.05) as f64;
            if let Some(period) = params["unstablePeriod"].as_i64() {
                core.unstable_period[14] = period as i32;
            }
            let out_size = if endIdx >= startIdx { endIdx - startIdx + 1 } else { 0 };
            let mut outBuf0: Vec<f64> = vec![0.0f64; out_size];
            let mut outBuf1: Vec<f64> = vec![0.0f64; out_size];
            let mut outBegIdx: usize = 0;
            let mut outNBElement: usize = 0;
            let mut rc = RetCode::Success;
            let start_time = Instant::now();
            for _bi in 0..bench_iters {
            rc = core.mama(
                startIdx, endIdx,
                &inReal,
                optInFastLimit,
                optInSlowLimit,
                &mut outBegIdx, &mut outNBElement, &mut outBuf0, &mut outBuf1,
            );
            }
            let elapsed_ns = start_time.elapsed().as_nanos() as u64 / bench_iters as u64;
            let start_time_ung = Instant::now();
            for _biu in 0..bench_iters {
            rc = core.mama_unguarded(
                startIdx, endIdx,
                &inReal,
                optInFastLimit,
                optInSlowLimit,
                &mut outBegIdx, &mut outNBElement, &mut outBuf0, &mut outBuf1,
            );
            }
            let elapsed_ns_ung = start_time_ung.elapsed().as_nanos() as u64 / bench_iters as u64;
            let lookback = core.mama_lookback(optInFastLimit, optInSlowLimit);
            let mut resp = format!("{{\"retCode\":{},\"outBegIdx\":{},\"outNBElement\":{},\"lookback\":{},\"timing_ns\":{},\"timing_ns_unguarded\":{}", retcode_to_int(rc), outBegIdx, outNBElement, lookback, elapsed_ns, elapsed_ns_ung);
            resp.push_str(",\"outReal\":"); resp.push_str(&json_f64_array(&outBuf0[..outNBElement]));
            resp.push_str(",\"outReal1\":"); resp.push_str(&json_f64_array(&outBuf1[..outNBElement]));
            resp.push('}');
            resp
        }
        "TA_MAVP" => {
            let startIdx = params["startIdx"].as_u64().unwrap_or(0) as usize;
            let endIdx = params["endIdx"].as_u64().unwrap_or(0) as usize;
            let use_preloaded = params["use_preloaded"].as_i64().unwrap_or(0);
            let bench_iters = std::cmp::max(1, params["iters"].as_i64().unwrap_or(1)) as u64;
            let mut _json_inReal0: Vec<f64> = Vec::new();
            let mut _json_inReal1: Vec<f64> = Vec::new();
            let inReal0: &[f64];
            let inReal1: &[f64];
            if use_preloaded != 0 && ref_data.n > 0 {
                inReal0 = &ref_data.close[..ref_data.n];
                inReal1 = &ref_data.high[..ref_data.n];
            } else {
                _json_inReal0 = parse_f64_array(&params["inReal0"]);
                inReal0 = &_json_inReal0;
                _json_inReal1 = parse_f64_array(&params["inReal1"]);
                inReal1 = &_json_inReal1;
            }
            let optInMinPeriod = params["optInMinPeriod"].as_i64().unwrap_or(2) as i32;
            let optInMaxPeriod = params["optInMaxPeriod"].as_i64().unwrap_or(30) as i32;
            let optInMAType = params["optInMAType"].as_i64().unwrap_or(0) as i32;
            let out_size = if endIdx >= startIdx { endIdx - startIdx + 1 } else { 0 };
            let mut outBuf0: Vec<f64> = vec![0.0f64; out_size];
            let mut outBegIdx: usize = 0;
            let mut outNBElement: usize = 0;
            let mut rc = RetCode::Success;
            let start_time = Instant::now();
            for _bi in 0..bench_iters {
            rc = core.mavp(
                startIdx, endIdx,
                &inReal0,
                &inReal1,
                optInMinPeriod,
                optInMaxPeriod,
                optInMAType,
                &mut outBegIdx, &mut outNBElement, &mut outBuf0,
            );
            }
            let elapsed_ns = start_time.elapsed().as_nanos() as u64 / bench_iters as u64;
            let start_time_ung = Instant::now();
            for _biu in 0..bench_iters {
            rc = core.mavp_unguarded(
                startIdx, endIdx,
                &inReal0,
                &inReal1,
                optInMinPeriod,
                optInMaxPeriod,
                optInMAType,
                &mut outBegIdx, &mut outNBElement, &mut outBuf0,
            );
            }
            let elapsed_ns_ung = start_time_ung.elapsed().as_nanos() as u64 / bench_iters as u64;
            let lookback = core.mavp_lookback(optInMinPeriod, optInMaxPeriod, optInMAType);
            let mut resp = format!("{{\"retCode\":{},\"outBegIdx\":{},\"outNBElement\":{},\"lookback\":{},\"timing_ns\":{},\"timing_ns_unguarded\":{}", retcode_to_int(rc), outBegIdx, outNBElement, lookback, elapsed_ns, elapsed_ns_ung);
            resp.push_str(",\"outReal\":"); resp.push_str(&json_f64_array(&outBuf0[..outNBElement]));
            resp.push('}');
            resp
        }
        "TA_MAX" => {
            let startIdx = params["startIdx"].as_u64().unwrap_or(0) as usize;
            let endIdx = params["endIdx"].as_u64().unwrap_or(0) as usize;
            let use_preloaded = params["use_preloaded"].as_i64().unwrap_or(0);
            let bench_iters = std::cmp::max(1, params["iters"].as_i64().unwrap_or(1)) as u64;
            let mut _json_inReal: Vec<f64> = Vec::new();
            let inReal: &[f64];
            if use_preloaded != 0 && ref_data.n > 0 {
                inReal = &ref_data.close[..ref_data.n];
            } else {
                _json_inReal = parse_f64_array(&params["inReal"]);
                inReal = &_json_inReal;
            }
            let optInTimePeriod = params["optInTimePeriod"].as_i64().unwrap_or(30) as i32;
            let out_size = if endIdx >= startIdx { endIdx - startIdx + 1 } else { 0 };
            let mut outBuf0: Vec<f64> = vec![0.0f64; out_size];
            let mut outBegIdx: usize = 0;
            let mut outNBElement: usize = 0;
            let mut rc = RetCode::Success;
            let start_time = Instant::now();
            for _bi in 0..bench_iters {
            rc = core.max(
                startIdx, endIdx,
                &inReal,
                optInTimePeriod,
                &mut outBegIdx, &mut outNBElement, &mut outBuf0,
            );
            }
            let elapsed_ns = start_time.elapsed().as_nanos() as u64 / bench_iters as u64;
            let start_time_ung = Instant::now();
            for _biu in 0..bench_iters {
            rc = core.max_unguarded(
                startIdx, endIdx,
                &inReal,
                optInTimePeriod,
                &mut outBegIdx, &mut outNBElement, &mut outBuf0,
            );
            }
            let elapsed_ns_ung = start_time_ung.elapsed().as_nanos() as u64 / bench_iters as u64;
            let lookback = core.max_lookback(optInTimePeriod);
            let mut resp = format!("{{\"retCode\":{},\"outBegIdx\":{},\"outNBElement\":{},\"lookback\":{},\"timing_ns\":{},\"timing_ns_unguarded\":{}", retcode_to_int(rc), outBegIdx, outNBElement, lookback, elapsed_ns, elapsed_ns_ung);
            resp.push_str(",\"outReal\":"); resp.push_str(&json_f64_array(&outBuf0[..outNBElement]));
            resp.push('}');
            resp
        }
        "TA_MAXINDEX" => {
            let startIdx = params["startIdx"].as_u64().unwrap_or(0) as usize;
            let endIdx = params["endIdx"].as_u64().unwrap_or(0) as usize;
            let use_preloaded = params["use_preloaded"].as_i64().unwrap_or(0);
            let bench_iters = std::cmp::max(1, params["iters"].as_i64().unwrap_or(1)) as u64;
            let mut _json_inReal: Vec<f64> = Vec::new();
            let inReal: &[f64];
            if use_preloaded != 0 && ref_data.n > 0 {
                inReal = &ref_data.close[..ref_data.n];
            } else {
                _json_inReal = parse_f64_array(&params["inReal"]);
                inReal = &_json_inReal;
            }
            let optInTimePeriod = params["optInTimePeriod"].as_i64().unwrap_or(30) as i32;
            let out_size = if endIdx >= startIdx { endIdx - startIdx + 1 } else { 0 };
            let mut outIntBuf0: Vec<i32> = vec![0i32; out_size];
            let mut outBegIdx: usize = 0;
            let mut outNBElement: usize = 0;
            let mut rc = RetCode::Success;
            let start_time = Instant::now();
            for _bi in 0..bench_iters {
            rc = core.maxindex(
                startIdx, endIdx,
                &inReal,
                optInTimePeriod,
                &mut outBegIdx, &mut outNBElement, &mut outIntBuf0,
            );
            }
            let elapsed_ns = start_time.elapsed().as_nanos() as u64 / bench_iters as u64;
            let start_time_ung = Instant::now();
            for _biu in 0..bench_iters {
            rc = core.maxindex_unguarded(
                startIdx, endIdx,
                &inReal,
                optInTimePeriod,
                &mut outBegIdx, &mut outNBElement, &mut outIntBuf0,
            );
            }
            let elapsed_ns_ung = start_time_ung.elapsed().as_nanos() as u64 / bench_iters as u64;
            let lookback = core.maxindex_lookback(optInTimePeriod);
            let mut resp = format!("{{\"retCode\":{},\"outBegIdx\":{},\"outNBElement\":{},\"lookback\":{},\"timing_ns\":{},\"timing_ns_unguarded\":{}", retcode_to_int(rc), outBegIdx, outNBElement, lookback, elapsed_ns, elapsed_ns_ung);
            resp.push_str(",\"outInteger\":"); resp.push_str(&json_i32_array(&outIntBuf0[..outNBElement]));
            resp.push('}');
            resp
        }
        "TA_MEDPRICE" => {
            let startIdx = params["startIdx"].as_u64().unwrap_or(0) as usize;
            let endIdx = params["endIdx"].as_u64().unwrap_or(0) as usize;
            let use_preloaded = params["use_preloaded"].as_i64().unwrap_or(0);
            let bench_iters = std::cmp::max(1, params["iters"].as_i64().unwrap_or(1)) as u64;
            let mut _json_inHigh: Vec<f64> = Vec::new();
            let mut _json_inLow: Vec<f64> = Vec::new();
            let inHigh: &[f64];
            let inLow: &[f64];
            if use_preloaded != 0 && ref_data.n > 0 {
                inHigh = &ref_data.high[..ref_data.n];
                inLow = &ref_data.low[..ref_data.n];
            } else {
                _json_inHigh = parse_f64_array(&params["inHigh"]);
                inHigh = &_json_inHigh;
                _json_inLow = parse_f64_array(&params["inLow"]);
                inLow = &_json_inLow;
            }
            let out_size = if endIdx >= startIdx { endIdx - startIdx + 1 } else { 0 };
            let mut outBuf0: Vec<f64> = vec![0.0f64; out_size];
            let mut outBegIdx: usize = 0;
            let mut outNBElement: usize = 0;
            let mut rc = RetCode::Success;
            let start_time = Instant::now();
            for _bi in 0..bench_iters {
            rc = core.medprice(
                startIdx, endIdx,
                &inHigh,
                &inLow,
                &mut outBegIdx, &mut outNBElement, &mut outBuf0,
            );
            }
            let elapsed_ns = start_time.elapsed().as_nanos() as u64 / bench_iters as u64;
            let start_time_ung = Instant::now();
            for _biu in 0..bench_iters {
            rc = core.medprice_unguarded(
                startIdx, endIdx,
                &inHigh,
                &inLow,
                &mut outBegIdx, &mut outNBElement, &mut outBuf0,
            );
            }
            let elapsed_ns_ung = start_time_ung.elapsed().as_nanos() as u64 / bench_iters as u64;
            let lookback = core.medprice_lookback();
            let mut resp = format!("{{\"retCode\":{},\"outBegIdx\":{},\"outNBElement\":{},\"lookback\":{},\"timing_ns\":{},\"timing_ns_unguarded\":{}", retcode_to_int(rc), outBegIdx, outNBElement, lookback, elapsed_ns, elapsed_ns_ung);
            resp.push_str(",\"outReal\":"); resp.push_str(&json_f64_array(&outBuf0[..outNBElement]));
            resp.push('}');
            resp
        }
        "TA_MFI" => {
            let startIdx = params["startIdx"].as_u64().unwrap_or(0) as usize;
            let endIdx = params["endIdx"].as_u64().unwrap_or(0) as usize;
            let use_preloaded = params["use_preloaded"].as_i64().unwrap_or(0);
            let bench_iters = std::cmp::max(1, params["iters"].as_i64().unwrap_or(1)) as u64;
            let mut _json_inHigh: Vec<f64> = Vec::new();
            let mut _json_inLow: Vec<f64> = Vec::new();
            let mut _json_inClose: Vec<f64> = Vec::new();
            let mut _json_inVolume: Vec<f64> = Vec::new();
            let inHigh: &[f64];
            let inLow: &[f64];
            let inClose: &[f64];
            let inVolume: &[f64];
            if use_preloaded != 0 && ref_data.n > 0 {
                inHigh = &ref_data.high[..ref_data.n];
                inLow = &ref_data.low[..ref_data.n];
                inClose = &ref_data.close[..ref_data.n];
                inVolume = &ref_data.volume[..ref_data.n];
            } else {
                _json_inHigh = parse_f64_array(&params["inHigh"]);
                inHigh = &_json_inHigh;
                _json_inLow = parse_f64_array(&params["inLow"]);
                inLow = &_json_inLow;
                _json_inClose = parse_f64_array(&params["inClose"]);
                inClose = &_json_inClose;
                _json_inVolume = parse_f64_array(&params["inVolume"]);
                inVolume = &_json_inVolume;
            }
            let optInTimePeriod = params["optInTimePeriod"].as_i64().unwrap_or(14) as i32;
            if let Some(period) = params["unstablePeriod"].as_i64() {
                core.unstable_period[15] = period as i32;
            }
            let out_size = if endIdx >= startIdx { endIdx - startIdx + 1 } else { 0 };
            let mut outBuf0: Vec<f64> = vec![0.0f64; out_size];
            let mut outBegIdx: usize = 0;
            let mut outNBElement: usize = 0;
            let mut rc = RetCode::Success;
            let start_time = Instant::now();
            for _bi in 0..bench_iters {
            rc = core.mfi(
                startIdx, endIdx,
                &inHigh,
                &inLow,
                &inClose,
                &inVolume,
                optInTimePeriod,
                &mut outBegIdx, &mut outNBElement, &mut outBuf0,
            );
            }
            let elapsed_ns = start_time.elapsed().as_nanos() as u64 / bench_iters as u64;
            let start_time_ung = Instant::now();
            for _biu in 0..bench_iters {
            rc = core.mfi_unguarded(
                startIdx, endIdx,
                &inHigh,
                &inLow,
                &inClose,
                &inVolume,
                optInTimePeriod,
                &mut outBegIdx, &mut outNBElement, &mut outBuf0,
            );
            }
            let elapsed_ns_ung = start_time_ung.elapsed().as_nanos() as u64 / bench_iters as u64;
            let lookback = core.mfi_lookback(optInTimePeriod);
            let mut resp = format!("{{\"retCode\":{},\"outBegIdx\":{},\"outNBElement\":{},\"lookback\":{},\"timing_ns\":{},\"timing_ns_unguarded\":{}", retcode_to_int(rc), outBegIdx, outNBElement, lookback, elapsed_ns, elapsed_ns_ung);
            resp.push_str(",\"outReal\":"); resp.push_str(&json_f64_array(&outBuf0[..outNBElement]));
            resp.push('}');
            resp
        }
        "TA_MIDPOINT" => {
            let startIdx = params["startIdx"].as_u64().unwrap_or(0) as usize;
            let endIdx = params["endIdx"].as_u64().unwrap_or(0) as usize;
            let use_preloaded = params["use_preloaded"].as_i64().unwrap_or(0);
            let bench_iters = std::cmp::max(1, params["iters"].as_i64().unwrap_or(1)) as u64;
            let mut _json_inReal: Vec<f64> = Vec::new();
            let inReal: &[f64];
            if use_preloaded != 0 && ref_data.n > 0 {
                inReal = &ref_data.close[..ref_data.n];
            } else {
                _json_inReal = parse_f64_array(&params["inReal"]);
                inReal = &_json_inReal;
            }
            let optInTimePeriod = params["optInTimePeriod"].as_i64().unwrap_or(14) as i32;
            let out_size = if endIdx >= startIdx { endIdx - startIdx + 1 } else { 0 };
            let mut outBuf0: Vec<f64> = vec![0.0f64; out_size];
            let mut outBegIdx: usize = 0;
            let mut outNBElement: usize = 0;
            let mut rc = RetCode::Success;
            let start_time = Instant::now();
            for _bi in 0..bench_iters {
            rc = core.midpoint(
                startIdx, endIdx,
                &inReal,
                optInTimePeriod,
                &mut outBegIdx, &mut outNBElement, &mut outBuf0,
            );
            }
            let elapsed_ns = start_time.elapsed().as_nanos() as u64 / bench_iters as u64;
            let start_time_ung = Instant::now();
            for _biu in 0..bench_iters {
            rc = core.midpoint_unguarded(
                startIdx, endIdx,
                &inReal,
                optInTimePeriod,
                &mut outBegIdx, &mut outNBElement, &mut outBuf0,
            );
            }
            let elapsed_ns_ung = start_time_ung.elapsed().as_nanos() as u64 / bench_iters as u64;
            let lookback = core.midpoint_lookback(optInTimePeriod);
            let mut resp = format!("{{\"retCode\":{},\"outBegIdx\":{},\"outNBElement\":{},\"lookback\":{},\"timing_ns\":{},\"timing_ns_unguarded\":{}", retcode_to_int(rc), outBegIdx, outNBElement, lookback, elapsed_ns, elapsed_ns_ung);
            resp.push_str(",\"outReal\":"); resp.push_str(&json_f64_array(&outBuf0[..outNBElement]));
            resp.push('}');
            resp
        }
        "TA_MIDPRICE" => {
            let startIdx = params["startIdx"].as_u64().unwrap_or(0) as usize;
            let endIdx = params["endIdx"].as_u64().unwrap_or(0) as usize;
            let use_preloaded = params["use_preloaded"].as_i64().unwrap_or(0);
            let bench_iters = std::cmp::max(1, params["iters"].as_i64().unwrap_or(1)) as u64;
            let mut _json_inHigh: Vec<f64> = Vec::new();
            let mut _json_inLow: Vec<f64> = Vec::new();
            let inHigh: &[f64];
            let inLow: &[f64];
            if use_preloaded != 0 && ref_data.n > 0 {
                inHigh = &ref_data.high[..ref_data.n];
                inLow = &ref_data.low[..ref_data.n];
            } else {
                _json_inHigh = parse_f64_array(&params["inHigh"]);
                inHigh = &_json_inHigh;
                _json_inLow = parse_f64_array(&params["inLow"]);
                inLow = &_json_inLow;
            }
            let optInTimePeriod = params["optInTimePeriod"].as_i64().unwrap_or(14) as i32;
            let out_size = if endIdx >= startIdx { endIdx - startIdx + 1 } else { 0 };
            let mut outBuf0: Vec<f64> = vec![0.0f64; out_size];
            let mut outBegIdx: usize = 0;
            let mut outNBElement: usize = 0;
            let mut rc = RetCode::Success;
            let start_time = Instant::now();
            for _bi in 0..bench_iters {
            rc = core.midprice(
                startIdx, endIdx,
                &inHigh,
                &inLow,
                optInTimePeriod,
                &mut outBegIdx, &mut outNBElement, &mut outBuf0,
            );
            }
            let elapsed_ns = start_time.elapsed().as_nanos() as u64 / bench_iters as u64;
            let start_time_ung = Instant::now();
            for _biu in 0..bench_iters {
            rc = core.midprice_unguarded(
                startIdx, endIdx,
                &inHigh,
                &inLow,
                optInTimePeriod,
                &mut outBegIdx, &mut outNBElement, &mut outBuf0,
            );
            }
            let elapsed_ns_ung = start_time_ung.elapsed().as_nanos() as u64 / bench_iters as u64;
            let lookback = core.midprice_lookback(optInTimePeriod);
            let mut resp = format!("{{\"retCode\":{},\"outBegIdx\":{},\"outNBElement\":{},\"lookback\":{},\"timing_ns\":{},\"timing_ns_unguarded\":{}", retcode_to_int(rc), outBegIdx, outNBElement, lookback, elapsed_ns, elapsed_ns_ung);
            resp.push_str(",\"outReal\":"); resp.push_str(&json_f64_array(&outBuf0[..outNBElement]));
            resp.push('}');
            resp
        }
        "TA_MIN" => {
            let startIdx = params["startIdx"].as_u64().unwrap_or(0) as usize;
            let endIdx = params["endIdx"].as_u64().unwrap_or(0) as usize;
            let use_preloaded = params["use_preloaded"].as_i64().unwrap_or(0);
            let bench_iters = std::cmp::max(1, params["iters"].as_i64().unwrap_or(1)) as u64;
            let mut _json_inReal: Vec<f64> = Vec::new();
            let inReal: &[f64];
            if use_preloaded != 0 && ref_data.n > 0 {
                inReal = &ref_data.close[..ref_data.n];
            } else {
                _json_inReal = parse_f64_array(&params["inReal"]);
                inReal = &_json_inReal;
            }
            let optInTimePeriod = params["optInTimePeriod"].as_i64().unwrap_or(30) as i32;
            let out_size = if endIdx >= startIdx { endIdx - startIdx + 1 } else { 0 };
            let mut outBuf0: Vec<f64> = vec![0.0f64; out_size];
            let mut outBegIdx: usize = 0;
            let mut outNBElement: usize = 0;
            let mut rc = RetCode::Success;
            let start_time = Instant::now();
            for _bi in 0..bench_iters {
            rc = core.min(
                startIdx, endIdx,
                &inReal,
                optInTimePeriod,
                &mut outBegIdx, &mut outNBElement, &mut outBuf0,
            );
            }
            let elapsed_ns = start_time.elapsed().as_nanos() as u64 / bench_iters as u64;
            let start_time_ung = Instant::now();
            for _biu in 0..bench_iters {
            rc = core.min_unguarded(
                startIdx, endIdx,
                &inReal,
                optInTimePeriod,
                &mut outBegIdx, &mut outNBElement, &mut outBuf0,
            );
            }
            let elapsed_ns_ung = start_time_ung.elapsed().as_nanos() as u64 / bench_iters as u64;
            let lookback = core.min_lookback(optInTimePeriod);
            let mut resp = format!("{{\"retCode\":{},\"outBegIdx\":{},\"outNBElement\":{},\"lookback\":{},\"timing_ns\":{},\"timing_ns_unguarded\":{}", retcode_to_int(rc), outBegIdx, outNBElement, lookback, elapsed_ns, elapsed_ns_ung);
            resp.push_str(",\"outReal\":"); resp.push_str(&json_f64_array(&outBuf0[..outNBElement]));
            resp.push('}');
            resp
        }
        "TA_MININDEX" => {
            let startIdx = params["startIdx"].as_u64().unwrap_or(0) as usize;
            let endIdx = params["endIdx"].as_u64().unwrap_or(0) as usize;
            let use_preloaded = params["use_preloaded"].as_i64().unwrap_or(0);
            let bench_iters = std::cmp::max(1, params["iters"].as_i64().unwrap_or(1)) as u64;
            let mut _json_inReal: Vec<f64> = Vec::new();
            let inReal: &[f64];
            if use_preloaded != 0 && ref_data.n > 0 {
                inReal = &ref_data.close[..ref_data.n];
            } else {
                _json_inReal = parse_f64_array(&params["inReal"]);
                inReal = &_json_inReal;
            }
            let optInTimePeriod = params["optInTimePeriod"].as_i64().unwrap_or(30) as i32;
            let out_size = if endIdx >= startIdx { endIdx - startIdx + 1 } else { 0 };
            let mut outIntBuf0: Vec<i32> = vec![0i32; out_size];
            let mut outBegIdx: usize = 0;
            let mut outNBElement: usize = 0;
            let mut rc = RetCode::Success;
            let start_time = Instant::now();
            for _bi in 0..bench_iters {
            rc = core.minindex(
                startIdx, endIdx,
                &inReal,
                optInTimePeriod,
                &mut outBegIdx, &mut outNBElement, &mut outIntBuf0,
            );
            }
            let elapsed_ns = start_time.elapsed().as_nanos() as u64 / bench_iters as u64;
            let start_time_ung = Instant::now();
            for _biu in 0..bench_iters {
            rc = core.minindex_unguarded(
                startIdx, endIdx,
                &inReal,
                optInTimePeriod,
                &mut outBegIdx, &mut outNBElement, &mut outIntBuf0,
            );
            }
            let elapsed_ns_ung = start_time_ung.elapsed().as_nanos() as u64 / bench_iters as u64;
            let lookback = core.minindex_lookback(optInTimePeriod);
            let mut resp = format!("{{\"retCode\":{},\"outBegIdx\":{},\"outNBElement\":{},\"lookback\":{},\"timing_ns\":{},\"timing_ns_unguarded\":{}", retcode_to_int(rc), outBegIdx, outNBElement, lookback, elapsed_ns, elapsed_ns_ung);
            resp.push_str(",\"outInteger\":"); resp.push_str(&json_i32_array(&outIntBuf0[..outNBElement]));
            resp.push('}');
            resp
        }
        "TA_MINMAX" => {
            let startIdx = params["startIdx"].as_u64().unwrap_or(0) as usize;
            let endIdx = params["endIdx"].as_u64().unwrap_or(0) as usize;
            let use_preloaded = params["use_preloaded"].as_i64().unwrap_or(0);
            let bench_iters = std::cmp::max(1, params["iters"].as_i64().unwrap_or(1)) as u64;
            let mut _json_inReal: Vec<f64> = Vec::new();
            let inReal: &[f64];
            if use_preloaded != 0 && ref_data.n > 0 {
                inReal = &ref_data.close[..ref_data.n];
            } else {
                _json_inReal = parse_f64_array(&params["inReal"]);
                inReal = &_json_inReal;
            }
            let optInTimePeriod = params["optInTimePeriod"].as_i64().unwrap_or(30) as i32;
            let out_size = if endIdx >= startIdx { endIdx - startIdx + 1 } else { 0 };
            let mut outBuf0: Vec<f64> = vec![0.0f64; out_size];
            let mut outBuf1: Vec<f64> = vec![0.0f64; out_size];
            let mut outBegIdx: usize = 0;
            let mut outNBElement: usize = 0;
            let mut rc = RetCode::Success;
            let start_time = Instant::now();
            for _bi in 0..bench_iters {
            rc = core.minmax(
                startIdx, endIdx,
                &inReal,
                optInTimePeriod,
                &mut outBegIdx, &mut outNBElement, &mut outBuf0, &mut outBuf1,
            );
            }
            let elapsed_ns = start_time.elapsed().as_nanos() as u64 / bench_iters as u64;
            let start_time_ung = Instant::now();
            for _biu in 0..bench_iters {
            rc = core.minmax_unguarded(
                startIdx, endIdx,
                &inReal,
                optInTimePeriod,
                &mut outBegIdx, &mut outNBElement, &mut outBuf0, &mut outBuf1,
            );
            }
            let elapsed_ns_ung = start_time_ung.elapsed().as_nanos() as u64 / bench_iters as u64;
            let lookback = core.minmax_lookback(optInTimePeriod);
            let mut resp = format!("{{\"retCode\":{},\"outBegIdx\":{},\"outNBElement\":{},\"lookback\":{},\"timing_ns\":{},\"timing_ns_unguarded\":{}", retcode_to_int(rc), outBegIdx, outNBElement, lookback, elapsed_ns, elapsed_ns_ung);
            resp.push_str(",\"outReal\":"); resp.push_str(&json_f64_array(&outBuf0[..outNBElement]));
            resp.push_str(",\"outReal1\":"); resp.push_str(&json_f64_array(&outBuf1[..outNBElement]));
            resp.push('}');
            resp
        }
        "TA_MINMAXINDEX" => {
            let startIdx = params["startIdx"].as_u64().unwrap_or(0) as usize;
            let endIdx = params["endIdx"].as_u64().unwrap_or(0) as usize;
            let use_preloaded = params["use_preloaded"].as_i64().unwrap_or(0);
            let bench_iters = std::cmp::max(1, params["iters"].as_i64().unwrap_or(1)) as u64;
            let mut _json_inReal: Vec<f64> = Vec::new();
            let inReal: &[f64];
            if use_preloaded != 0 && ref_data.n > 0 {
                inReal = &ref_data.close[..ref_data.n];
            } else {
                _json_inReal = parse_f64_array(&params["inReal"]);
                inReal = &_json_inReal;
            }
            let optInTimePeriod = params["optInTimePeriod"].as_i64().unwrap_or(30) as i32;
            let out_size = if endIdx >= startIdx { endIdx - startIdx + 1 } else { 0 };
            let mut outIntBuf0: Vec<i32> = vec![0i32; out_size];
            let mut outIntBuf1: Vec<i32> = vec![0i32; out_size];
            let mut outBegIdx: usize = 0;
            let mut outNBElement: usize = 0;
            let mut rc = RetCode::Success;
            let start_time = Instant::now();
            for _bi in 0..bench_iters {
            rc = core.minmaxindex(
                startIdx, endIdx,
                &inReal,
                optInTimePeriod,
                &mut outBegIdx, &mut outNBElement, &mut outIntBuf0, &mut outIntBuf1,
            );
            }
            let elapsed_ns = start_time.elapsed().as_nanos() as u64 / bench_iters as u64;
            let start_time_ung = Instant::now();
            for _biu in 0..bench_iters {
            rc = core.minmaxindex_unguarded(
                startIdx, endIdx,
                &inReal,
                optInTimePeriod,
                &mut outBegIdx, &mut outNBElement, &mut outIntBuf0, &mut outIntBuf1,
            );
            }
            let elapsed_ns_ung = start_time_ung.elapsed().as_nanos() as u64 / bench_iters as u64;
            let lookback = core.minmaxindex_lookback(optInTimePeriod);
            let mut resp = format!("{{\"retCode\":{},\"outBegIdx\":{},\"outNBElement\":{},\"lookback\":{},\"timing_ns\":{},\"timing_ns_unguarded\":{}", retcode_to_int(rc), outBegIdx, outNBElement, lookback, elapsed_ns, elapsed_ns_ung);
            resp.push_str(",\"outInteger\":"); resp.push_str(&json_i32_array(&outIntBuf0[..outNBElement]));
            resp.push_str(",\"outInteger1\":"); resp.push_str(&json_i32_array(&outIntBuf1[..outNBElement]));
            resp.push('}');
            resp
        }
        "TA_MINUS_DI" => {
            let startIdx = params["startIdx"].as_u64().unwrap_or(0) as usize;
            let endIdx = params["endIdx"].as_u64().unwrap_or(0) as usize;
            let use_preloaded = params["use_preloaded"].as_i64().unwrap_or(0);
            let bench_iters = std::cmp::max(1, params["iters"].as_i64().unwrap_or(1)) as u64;
            let mut _json_inHigh: Vec<f64> = Vec::new();
            let mut _json_inLow: Vec<f64> = Vec::new();
            let mut _json_inClose: Vec<f64> = Vec::new();
            let inHigh: &[f64];
            let inLow: &[f64];
            let inClose: &[f64];
            if use_preloaded != 0 && ref_data.n > 0 {
                inHigh = &ref_data.high[..ref_data.n];
                inLow = &ref_data.low[..ref_data.n];
                inClose = &ref_data.close[..ref_data.n];
            } else {
                _json_inHigh = parse_f64_array(&params["inHigh"]);
                inHigh = &_json_inHigh;
                _json_inLow = parse_f64_array(&params["inLow"]);
                inLow = &_json_inLow;
                _json_inClose = parse_f64_array(&params["inClose"]);
                inClose = &_json_inClose;
            }
            let optInTimePeriod = params["optInTimePeriod"].as_i64().unwrap_or(14) as i32;
            if let Some(period) = params["unstablePeriod"].as_i64() {
                core.unstable_period[16] = period as i32;
            }
            let out_size = if endIdx >= startIdx { endIdx - startIdx + 1 } else { 0 };
            let mut outBuf0: Vec<f64> = vec![0.0f64; out_size];
            let mut outBegIdx: usize = 0;
            let mut outNBElement: usize = 0;
            let mut rc = RetCode::Success;
            let start_time = Instant::now();
            for _bi in 0..bench_iters {
            rc = core.minus_di(
                startIdx, endIdx,
                &inHigh,
                &inLow,
                &inClose,
                optInTimePeriod,
                &mut outBegIdx, &mut outNBElement, &mut outBuf0,
            );
            }
            let elapsed_ns = start_time.elapsed().as_nanos() as u64 / bench_iters as u64;
            let start_time_ung = Instant::now();
            for _biu in 0..bench_iters {
            rc = core.minus_di_unguarded(
                startIdx, endIdx,
                &inHigh,
                &inLow,
                &inClose,
                optInTimePeriod,
                &mut outBegIdx, &mut outNBElement, &mut outBuf0,
            );
            }
            let elapsed_ns_ung = start_time_ung.elapsed().as_nanos() as u64 / bench_iters as u64;
            let lookback = core.minus_di_lookback(optInTimePeriod);
            let mut resp = format!("{{\"retCode\":{},\"outBegIdx\":{},\"outNBElement\":{},\"lookback\":{},\"timing_ns\":{},\"timing_ns_unguarded\":{}", retcode_to_int(rc), outBegIdx, outNBElement, lookback, elapsed_ns, elapsed_ns_ung);
            resp.push_str(",\"outReal\":"); resp.push_str(&json_f64_array(&outBuf0[..outNBElement]));
            resp.push('}');
            resp
        }
        "TA_MINUS_DM" => {
            let startIdx = params["startIdx"].as_u64().unwrap_or(0) as usize;
            let endIdx = params["endIdx"].as_u64().unwrap_or(0) as usize;
            let use_preloaded = params["use_preloaded"].as_i64().unwrap_or(0);
            let bench_iters = std::cmp::max(1, params["iters"].as_i64().unwrap_or(1)) as u64;
            let mut _json_inHigh: Vec<f64> = Vec::new();
            let mut _json_inLow: Vec<f64> = Vec::new();
            let inHigh: &[f64];
            let inLow: &[f64];
            if use_preloaded != 0 && ref_data.n > 0 {
                inHigh = &ref_data.high[..ref_data.n];
                inLow = &ref_data.low[..ref_data.n];
            } else {
                _json_inHigh = parse_f64_array(&params["inHigh"]);
                inHigh = &_json_inHigh;
                _json_inLow = parse_f64_array(&params["inLow"]);
                inLow = &_json_inLow;
            }
            let optInTimePeriod = params["optInTimePeriod"].as_i64().unwrap_or(14) as i32;
            if let Some(period) = params["unstablePeriod"].as_i64() {
                core.unstable_period[17] = period as i32;
            }
            let out_size = if endIdx >= startIdx { endIdx - startIdx + 1 } else { 0 };
            let mut outBuf0: Vec<f64> = vec![0.0f64; out_size];
            let mut outBegIdx: usize = 0;
            let mut outNBElement: usize = 0;
            let mut rc = RetCode::Success;
            let start_time = Instant::now();
            for _bi in 0..bench_iters {
            rc = core.minus_dm(
                startIdx, endIdx,
                &inHigh,
                &inLow,
                optInTimePeriod,
                &mut outBegIdx, &mut outNBElement, &mut outBuf0,
            );
            }
            let elapsed_ns = start_time.elapsed().as_nanos() as u64 / bench_iters as u64;
            let start_time_ung = Instant::now();
            for _biu in 0..bench_iters {
            rc = core.minus_dm_unguarded(
                startIdx, endIdx,
                &inHigh,
                &inLow,
                optInTimePeriod,
                &mut outBegIdx, &mut outNBElement, &mut outBuf0,
            );
            }
            let elapsed_ns_ung = start_time_ung.elapsed().as_nanos() as u64 / bench_iters as u64;
            let lookback = core.minus_dm_lookback(optInTimePeriod);
            let mut resp = format!("{{\"retCode\":{},\"outBegIdx\":{},\"outNBElement\":{},\"lookback\":{},\"timing_ns\":{},\"timing_ns_unguarded\":{}", retcode_to_int(rc), outBegIdx, outNBElement, lookback, elapsed_ns, elapsed_ns_ung);
            resp.push_str(",\"outReal\":"); resp.push_str(&json_f64_array(&outBuf0[..outNBElement]));
            resp.push('}');
            resp
        }
        "TA_MOM" => {
            let startIdx = params["startIdx"].as_u64().unwrap_or(0) as usize;
            let endIdx = params["endIdx"].as_u64().unwrap_or(0) as usize;
            let use_preloaded = params["use_preloaded"].as_i64().unwrap_or(0);
            let bench_iters = std::cmp::max(1, params["iters"].as_i64().unwrap_or(1)) as u64;
            let mut _json_inReal: Vec<f64> = Vec::new();
            let inReal: &[f64];
            if use_preloaded != 0 && ref_data.n > 0 {
                inReal = &ref_data.close[..ref_data.n];
            } else {
                _json_inReal = parse_f64_array(&params["inReal"]);
                inReal = &_json_inReal;
            }
            let optInTimePeriod = params["optInTimePeriod"].as_i64().unwrap_or(10) as i32;
            let out_size = if endIdx >= startIdx { endIdx - startIdx + 1 } else { 0 };
            let mut outBuf0: Vec<f64> = vec![0.0f64; out_size];
            let mut outBegIdx: usize = 0;
            let mut outNBElement: usize = 0;
            let mut rc = RetCode::Success;
            let start_time = Instant::now();
            for _bi in 0..bench_iters {
            rc = core.mom(
                startIdx, endIdx,
                &inReal,
                optInTimePeriod,
                &mut outBegIdx, &mut outNBElement, &mut outBuf0,
            );
            }
            let elapsed_ns = start_time.elapsed().as_nanos() as u64 / bench_iters as u64;
            let start_time_ung = Instant::now();
            for _biu in 0..bench_iters {
            rc = core.mom_unguarded(
                startIdx, endIdx,
                &inReal,
                optInTimePeriod,
                &mut outBegIdx, &mut outNBElement, &mut outBuf0,
            );
            }
            let elapsed_ns_ung = start_time_ung.elapsed().as_nanos() as u64 / bench_iters as u64;
            let lookback = core.mom_lookback(optInTimePeriod);
            let mut resp = format!("{{\"retCode\":{},\"outBegIdx\":{},\"outNBElement\":{},\"lookback\":{},\"timing_ns\":{},\"timing_ns_unguarded\":{}", retcode_to_int(rc), outBegIdx, outNBElement, lookback, elapsed_ns, elapsed_ns_ung);
            resp.push_str(",\"outReal\":"); resp.push_str(&json_f64_array(&outBuf0[..outNBElement]));
            resp.push('}');
            resp
        }
        "TA_MULT" => {
            let startIdx = params["startIdx"].as_u64().unwrap_or(0) as usize;
            let endIdx = params["endIdx"].as_u64().unwrap_or(0) as usize;
            let use_preloaded = params["use_preloaded"].as_i64().unwrap_or(0);
            let bench_iters = std::cmp::max(1, params["iters"].as_i64().unwrap_or(1)) as u64;
            let mut _json_inReal0: Vec<f64> = Vec::new();
            let mut _json_inReal1: Vec<f64> = Vec::new();
            let inReal0: &[f64];
            let inReal1: &[f64];
            if use_preloaded != 0 && ref_data.n > 0 {
                inReal0 = &ref_data.close[..ref_data.n];
                inReal1 = &ref_data.high[..ref_data.n];
            } else {
                _json_inReal0 = parse_f64_array(&params["inReal0"]);
                inReal0 = &_json_inReal0;
                _json_inReal1 = parse_f64_array(&params["inReal1"]);
                inReal1 = &_json_inReal1;
            }
            let out_size = if endIdx >= startIdx { endIdx - startIdx + 1 } else { 0 };
            let mut outBuf0: Vec<f64> = vec![0.0f64; out_size];
            let mut outBegIdx: usize = 0;
            let mut outNBElement: usize = 0;
            let mut rc = RetCode::Success;
            let start_time = Instant::now();
            for _bi in 0..bench_iters {
            rc = core.mult(
                startIdx, endIdx,
                &inReal0,
                &inReal1,
                &mut outBegIdx, &mut outNBElement, &mut outBuf0,
            );
            }
            let elapsed_ns = start_time.elapsed().as_nanos() as u64 / bench_iters as u64;
            let start_time_ung = Instant::now();
            for _biu in 0..bench_iters {
            rc = core.mult_unguarded(
                startIdx, endIdx,
                &inReal0,
                &inReal1,
                &mut outBegIdx, &mut outNBElement, &mut outBuf0,
            );
            }
            let elapsed_ns_ung = start_time_ung.elapsed().as_nanos() as u64 / bench_iters as u64;
            let lookback = core.mult_lookback();
            let mut resp = format!("{{\"retCode\":{},\"outBegIdx\":{},\"outNBElement\":{},\"lookback\":{},\"timing_ns\":{},\"timing_ns_unguarded\":{}", retcode_to_int(rc), outBegIdx, outNBElement, lookback, elapsed_ns, elapsed_ns_ung);
            resp.push_str(",\"outReal\":"); resp.push_str(&json_f64_array(&outBuf0[..outNBElement]));
            resp.push('}');
            resp
        }
        "TA_NATR" => {
            let startIdx = params["startIdx"].as_u64().unwrap_or(0) as usize;
            let endIdx = params["endIdx"].as_u64().unwrap_or(0) as usize;
            let use_preloaded = params["use_preloaded"].as_i64().unwrap_or(0);
            let bench_iters = std::cmp::max(1, params["iters"].as_i64().unwrap_or(1)) as u64;
            let mut _json_inHigh: Vec<f64> = Vec::new();
            let mut _json_inLow: Vec<f64> = Vec::new();
            let mut _json_inClose: Vec<f64> = Vec::new();
            let inHigh: &[f64];
            let inLow: &[f64];
            let inClose: &[f64];
            if use_preloaded != 0 && ref_data.n > 0 {
                inHigh = &ref_data.high[..ref_data.n];
                inLow = &ref_data.low[..ref_data.n];
                inClose = &ref_data.close[..ref_data.n];
            } else {
                _json_inHigh = parse_f64_array(&params["inHigh"]);
                inHigh = &_json_inHigh;
                _json_inLow = parse_f64_array(&params["inLow"]);
                inLow = &_json_inLow;
                _json_inClose = parse_f64_array(&params["inClose"]);
                inClose = &_json_inClose;
            }
            let optInTimePeriod = params["optInTimePeriod"].as_i64().unwrap_or(14) as i32;
            if let Some(period) = params["unstablePeriod"].as_i64() {
                core.unstable_period[18] = period as i32;
            }
            let out_size = if endIdx >= startIdx { endIdx - startIdx + 1 } else { 0 };
            let mut outBuf0: Vec<f64> = vec![0.0f64; out_size];
            let mut outBegIdx: usize = 0;
            let mut outNBElement: usize = 0;
            let mut rc = RetCode::Success;
            let start_time = Instant::now();
            for _bi in 0..bench_iters {
            rc = core.natr(
                startIdx, endIdx,
                &inHigh,
                &inLow,
                &inClose,
                optInTimePeriod,
                &mut outBegIdx, &mut outNBElement, &mut outBuf0,
            );
            }
            let elapsed_ns = start_time.elapsed().as_nanos() as u64 / bench_iters as u64;
            let start_time_ung = Instant::now();
            for _biu in 0..bench_iters {
            rc = core.natr_unguarded(
                startIdx, endIdx,
                &inHigh,
                &inLow,
                &inClose,
                optInTimePeriod,
                &mut outBegIdx, &mut outNBElement, &mut outBuf0,
            );
            }
            let elapsed_ns_ung = start_time_ung.elapsed().as_nanos() as u64 / bench_iters as u64;
            let lookback = core.natr_lookback(optInTimePeriod);
            let mut resp = format!("{{\"retCode\":{},\"outBegIdx\":{},\"outNBElement\":{},\"lookback\":{},\"timing_ns\":{},\"timing_ns_unguarded\":{}", retcode_to_int(rc), outBegIdx, outNBElement, lookback, elapsed_ns, elapsed_ns_ung);
            resp.push_str(",\"outReal\":"); resp.push_str(&json_f64_array(&outBuf0[..outNBElement]));
            resp.push('}');
            resp
        }
        "TA_OBV" => {
            let startIdx = params["startIdx"].as_u64().unwrap_or(0) as usize;
            let endIdx = params["endIdx"].as_u64().unwrap_or(0) as usize;
            let use_preloaded = params["use_preloaded"].as_i64().unwrap_or(0);
            let bench_iters = std::cmp::max(1, params["iters"].as_i64().unwrap_or(1)) as u64;
            let mut _json_inReal: Vec<f64> = Vec::new();
            let mut _json_inVolume: Vec<f64> = Vec::new();
            let inReal: &[f64];
            let inVolume: &[f64];
            if use_preloaded != 0 && ref_data.n > 0 {
                inReal = &ref_data.close[..ref_data.n];
                inVolume = &ref_data.volume[..ref_data.n];
            } else {
                _json_inReal = parse_f64_array(&params["inReal"]);
                inReal = &_json_inReal;
                _json_inVolume = parse_f64_array(&params["inVolume"]);
                inVolume = &_json_inVolume;
            }
            let out_size = if endIdx >= startIdx { endIdx - startIdx + 1 } else { 0 };
            let mut outBuf0: Vec<f64> = vec![0.0f64; out_size];
            let mut outBegIdx: usize = 0;
            let mut outNBElement: usize = 0;
            let mut rc = RetCode::Success;
            let start_time = Instant::now();
            for _bi in 0..bench_iters {
            rc = core.obv(
                startIdx, endIdx,
                &inReal,
                &inVolume,
                &mut outBegIdx, &mut outNBElement, &mut outBuf0,
            );
            }
            let elapsed_ns = start_time.elapsed().as_nanos() as u64 / bench_iters as u64;
            let start_time_ung = Instant::now();
            for _biu in 0..bench_iters {
            rc = core.obv_unguarded(
                startIdx, endIdx,
                &inReal,
                &inVolume,
                &mut outBegIdx, &mut outNBElement, &mut outBuf0,
            );
            }
            let elapsed_ns_ung = start_time_ung.elapsed().as_nanos() as u64 / bench_iters as u64;
            let lookback = core.obv_lookback();
            let mut resp = format!("{{\"retCode\":{},\"outBegIdx\":{},\"outNBElement\":{},\"lookback\":{},\"timing_ns\":{},\"timing_ns_unguarded\":{}", retcode_to_int(rc), outBegIdx, outNBElement, lookback, elapsed_ns, elapsed_ns_ung);
            resp.push_str(",\"outReal\":"); resp.push_str(&json_f64_array(&outBuf0[..outNBElement]));
            resp.push('}');
            resp
        }
        "TA_PLUS_DI" => {
            let startIdx = params["startIdx"].as_u64().unwrap_or(0) as usize;
            let endIdx = params["endIdx"].as_u64().unwrap_or(0) as usize;
            let use_preloaded = params["use_preloaded"].as_i64().unwrap_or(0);
            let bench_iters = std::cmp::max(1, params["iters"].as_i64().unwrap_or(1)) as u64;
            let mut _json_inHigh: Vec<f64> = Vec::new();
            let mut _json_inLow: Vec<f64> = Vec::new();
            let mut _json_inClose: Vec<f64> = Vec::new();
            let inHigh: &[f64];
            let inLow: &[f64];
            let inClose: &[f64];
            if use_preloaded != 0 && ref_data.n > 0 {
                inHigh = &ref_data.high[..ref_data.n];
                inLow = &ref_data.low[..ref_data.n];
                inClose = &ref_data.close[..ref_data.n];
            } else {
                _json_inHigh = parse_f64_array(&params["inHigh"]);
                inHigh = &_json_inHigh;
                _json_inLow = parse_f64_array(&params["inLow"]);
                inLow = &_json_inLow;
                _json_inClose = parse_f64_array(&params["inClose"]);
                inClose = &_json_inClose;
            }
            let optInTimePeriod = params["optInTimePeriod"].as_i64().unwrap_or(14) as i32;
            if let Some(period) = params["unstablePeriod"].as_i64() {
                core.unstable_period[19] = period as i32;
            }
            let out_size = if endIdx >= startIdx { endIdx - startIdx + 1 } else { 0 };
            let mut outBuf0: Vec<f64> = vec![0.0f64; out_size];
            let mut outBegIdx: usize = 0;
            let mut outNBElement: usize = 0;
            let mut rc = RetCode::Success;
            let start_time = Instant::now();
            for _bi in 0..bench_iters {
            rc = core.plus_di(
                startIdx, endIdx,
                &inHigh,
                &inLow,
                &inClose,
                optInTimePeriod,
                &mut outBegIdx, &mut outNBElement, &mut outBuf0,
            );
            }
            let elapsed_ns = start_time.elapsed().as_nanos() as u64 / bench_iters as u64;
            let start_time_ung = Instant::now();
            for _biu in 0..bench_iters {
            rc = core.plus_di_unguarded(
                startIdx, endIdx,
                &inHigh,
                &inLow,
                &inClose,
                optInTimePeriod,
                &mut outBegIdx, &mut outNBElement, &mut outBuf0,
            );
            }
            let elapsed_ns_ung = start_time_ung.elapsed().as_nanos() as u64 / bench_iters as u64;
            let lookback = core.plus_di_lookback(optInTimePeriod);
            let mut resp = format!("{{\"retCode\":{},\"outBegIdx\":{},\"outNBElement\":{},\"lookback\":{},\"timing_ns\":{},\"timing_ns_unguarded\":{}", retcode_to_int(rc), outBegIdx, outNBElement, lookback, elapsed_ns, elapsed_ns_ung);
            resp.push_str(",\"outReal\":"); resp.push_str(&json_f64_array(&outBuf0[..outNBElement]));
            resp.push('}');
            resp
        }
        "TA_PLUS_DM" => {
            let startIdx = params["startIdx"].as_u64().unwrap_or(0) as usize;
            let endIdx = params["endIdx"].as_u64().unwrap_or(0) as usize;
            let use_preloaded = params["use_preloaded"].as_i64().unwrap_or(0);
            let bench_iters = std::cmp::max(1, params["iters"].as_i64().unwrap_or(1)) as u64;
            let mut _json_inHigh: Vec<f64> = Vec::new();
            let mut _json_inLow: Vec<f64> = Vec::new();
            let inHigh: &[f64];
            let inLow: &[f64];
            if use_preloaded != 0 && ref_data.n > 0 {
                inHigh = &ref_data.high[..ref_data.n];
                inLow = &ref_data.low[..ref_data.n];
            } else {
                _json_inHigh = parse_f64_array(&params["inHigh"]);
                inHigh = &_json_inHigh;
                _json_inLow = parse_f64_array(&params["inLow"]);
                inLow = &_json_inLow;
            }
            let optInTimePeriod = params["optInTimePeriod"].as_i64().unwrap_or(14) as i32;
            if let Some(period) = params["unstablePeriod"].as_i64() {
                core.unstable_period[20] = period as i32;
            }
            let out_size = if endIdx >= startIdx { endIdx - startIdx + 1 } else { 0 };
            let mut outBuf0: Vec<f64> = vec![0.0f64; out_size];
            let mut outBegIdx: usize = 0;
            let mut outNBElement: usize = 0;
            let mut rc = RetCode::Success;
            let start_time = Instant::now();
            for _bi in 0..bench_iters {
            rc = core.plus_dm(
                startIdx, endIdx,
                &inHigh,
                &inLow,
                optInTimePeriod,
                &mut outBegIdx, &mut outNBElement, &mut outBuf0,
            );
            }
            let elapsed_ns = start_time.elapsed().as_nanos() as u64 / bench_iters as u64;
            let start_time_ung = Instant::now();
            for _biu in 0..bench_iters {
            rc = core.plus_dm_unguarded(
                startIdx, endIdx,
                &inHigh,
                &inLow,
                optInTimePeriod,
                &mut outBegIdx, &mut outNBElement, &mut outBuf0,
            );
            }
            let elapsed_ns_ung = start_time_ung.elapsed().as_nanos() as u64 / bench_iters as u64;
            let lookback = core.plus_dm_lookback(optInTimePeriod);
            let mut resp = format!("{{\"retCode\":{},\"outBegIdx\":{},\"outNBElement\":{},\"lookback\":{},\"timing_ns\":{},\"timing_ns_unguarded\":{}", retcode_to_int(rc), outBegIdx, outNBElement, lookback, elapsed_ns, elapsed_ns_ung);
            resp.push_str(",\"outReal\":"); resp.push_str(&json_f64_array(&outBuf0[..outNBElement]));
            resp.push('}');
            resp
        }
        "TA_PPO" => {
            let startIdx = params["startIdx"].as_u64().unwrap_or(0) as usize;
            let endIdx = params["endIdx"].as_u64().unwrap_or(0) as usize;
            let use_preloaded = params["use_preloaded"].as_i64().unwrap_or(0);
            let bench_iters = std::cmp::max(1, params["iters"].as_i64().unwrap_or(1)) as u64;
            let mut _json_inReal: Vec<f64> = Vec::new();
            let inReal: &[f64];
            if use_preloaded != 0 && ref_data.n > 0 {
                inReal = &ref_data.close[..ref_data.n];
            } else {
                _json_inReal = parse_f64_array(&params["inReal"]);
                inReal = &_json_inReal;
            }
            let optInFastPeriod = params["optInFastPeriod"].as_i64().unwrap_or(12) as i32;
            let optInSlowPeriod = params["optInSlowPeriod"].as_i64().unwrap_or(26) as i32;
            let optInMAType = params["optInMAType"].as_i64().unwrap_or(0) as i32;
            let out_size = if endIdx >= startIdx { endIdx - startIdx + 1 } else { 0 };
            let mut outBuf0: Vec<f64> = vec![0.0f64; out_size];
            let mut outBegIdx: usize = 0;
            let mut outNBElement: usize = 0;
            let mut rc = RetCode::Success;
            let start_time = Instant::now();
            for _bi in 0..bench_iters {
            rc = core.ppo(
                startIdx, endIdx,
                &inReal,
                optInFastPeriod,
                optInSlowPeriod,
                optInMAType,
                &mut outBegIdx, &mut outNBElement, &mut outBuf0,
            );
            }
            let elapsed_ns = start_time.elapsed().as_nanos() as u64 / bench_iters as u64;
            let start_time_ung = Instant::now();
            for _biu in 0..bench_iters {
            rc = core.ppo_unguarded(
                startIdx, endIdx,
                &inReal,
                optInFastPeriod,
                optInSlowPeriod,
                optInMAType,
                &mut outBegIdx, &mut outNBElement, &mut outBuf0,
            );
            }
            let elapsed_ns_ung = start_time_ung.elapsed().as_nanos() as u64 / bench_iters as u64;
            let lookback = core.ppo_lookback(optInFastPeriod, optInSlowPeriod, optInMAType);
            let mut resp = format!("{{\"retCode\":{},\"outBegIdx\":{},\"outNBElement\":{},\"lookback\":{},\"timing_ns\":{},\"timing_ns_unguarded\":{}", retcode_to_int(rc), outBegIdx, outNBElement, lookback, elapsed_ns, elapsed_ns_ung);
            resp.push_str(",\"outReal\":"); resp.push_str(&json_f64_array(&outBuf0[..outNBElement]));
            resp.push('}');
            resp
        }
        "TA_ROC" => {
            let startIdx = params["startIdx"].as_u64().unwrap_or(0) as usize;
            let endIdx = params["endIdx"].as_u64().unwrap_or(0) as usize;
            let use_preloaded = params["use_preloaded"].as_i64().unwrap_or(0);
            let bench_iters = std::cmp::max(1, params["iters"].as_i64().unwrap_or(1)) as u64;
            let mut _json_inReal: Vec<f64> = Vec::new();
            let inReal: &[f64];
            if use_preloaded != 0 && ref_data.n > 0 {
                inReal = &ref_data.close[..ref_data.n];
            } else {
                _json_inReal = parse_f64_array(&params["inReal"]);
                inReal = &_json_inReal;
            }
            let optInTimePeriod = params["optInTimePeriod"].as_i64().unwrap_or(10) as i32;
            let out_size = if endIdx >= startIdx { endIdx - startIdx + 1 } else { 0 };
            let mut outBuf0: Vec<f64> = vec![0.0f64; out_size];
            let mut outBegIdx: usize = 0;
            let mut outNBElement: usize = 0;
            let mut rc = RetCode::Success;
            let start_time = Instant::now();
            for _bi in 0..bench_iters {
            rc = core.roc(
                startIdx, endIdx,
                &inReal,
                optInTimePeriod,
                &mut outBegIdx, &mut outNBElement, &mut outBuf0,
            );
            }
            let elapsed_ns = start_time.elapsed().as_nanos() as u64 / bench_iters as u64;
            let start_time_ung = Instant::now();
            for _biu in 0..bench_iters {
            rc = core.roc_unguarded(
                startIdx, endIdx,
                &inReal,
                optInTimePeriod,
                &mut outBegIdx, &mut outNBElement, &mut outBuf0,
            );
            }
            let elapsed_ns_ung = start_time_ung.elapsed().as_nanos() as u64 / bench_iters as u64;
            let lookback = core.roc_lookback(optInTimePeriod);
            let mut resp = format!("{{\"retCode\":{},\"outBegIdx\":{},\"outNBElement\":{},\"lookback\":{},\"timing_ns\":{},\"timing_ns_unguarded\":{}", retcode_to_int(rc), outBegIdx, outNBElement, lookback, elapsed_ns, elapsed_ns_ung);
            resp.push_str(",\"outReal\":"); resp.push_str(&json_f64_array(&outBuf0[..outNBElement]));
            resp.push('}');
            resp
        }
        "TA_ROCP" => {
            let startIdx = params["startIdx"].as_u64().unwrap_or(0) as usize;
            let endIdx = params["endIdx"].as_u64().unwrap_or(0) as usize;
            let use_preloaded = params["use_preloaded"].as_i64().unwrap_or(0);
            let bench_iters = std::cmp::max(1, params["iters"].as_i64().unwrap_or(1)) as u64;
            let mut _json_inReal: Vec<f64> = Vec::new();
            let inReal: &[f64];
            if use_preloaded != 0 && ref_data.n > 0 {
                inReal = &ref_data.close[..ref_data.n];
            } else {
                _json_inReal = parse_f64_array(&params["inReal"]);
                inReal = &_json_inReal;
            }
            let optInTimePeriod = params["optInTimePeriod"].as_i64().unwrap_or(10) as i32;
            let out_size = if endIdx >= startIdx { endIdx - startIdx + 1 } else { 0 };
            let mut outBuf0: Vec<f64> = vec![0.0f64; out_size];
            let mut outBegIdx: usize = 0;
            let mut outNBElement: usize = 0;
            let mut rc = RetCode::Success;
            let start_time = Instant::now();
            for _bi in 0..bench_iters {
            rc = core.rocp(
                startIdx, endIdx,
                &inReal,
                optInTimePeriod,
                &mut outBegIdx, &mut outNBElement, &mut outBuf0,
            );
            }
            let elapsed_ns = start_time.elapsed().as_nanos() as u64 / bench_iters as u64;
            let start_time_ung = Instant::now();
            for _biu in 0..bench_iters {
            rc = core.rocp_unguarded(
                startIdx, endIdx,
                &inReal,
                optInTimePeriod,
                &mut outBegIdx, &mut outNBElement, &mut outBuf0,
            );
            }
            let elapsed_ns_ung = start_time_ung.elapsed().as_nanos() as u64 / bench_iters as u64;
            let lookback = core.rocp_lookback(optInTimePeriod);
            let mut resp = format!("{{\"retCode\":{},\"outBegIdx\":{},\"outNBElement\":{},\"lookback\":{},\"timing_ns\":{},\"timing_ns_unguarded\":{}", retcode_to_int(rc), outBegIdx, outNBElement, lookback, elapsed_ns, elapsed_ns_ung);
            resp.push_str(",\"outReal\":"); resp.push_str(&json_f64_array(&outBuf0[..outNBElement]));
            resp.push('}');
            resp
        }
        "TA_ROCR" => {
            let startIdx = params["startIdx"].as_u64().unwrap_or(0) as usize;
            let endIdx = params["endIdx"].as_u64().unwrap_or(0) as usize;
            let use_preloaded = params["use_preloaded"].as_i64().unwrap_or(0);
            let bench_iters = std::cmp::max(1, params["iters"].as_i64().unwrap_or(1)) as u64;
            let mut _json_inReal: Vec<f64> = Vec::new();
            let inReal: &[f64];
            if use_preloaded != 0 && ref_data.n > 0 {
                inReal = &ref_data.close[..ref_data.n];
            } else {
                _json_inReal = parse_f64_array(&params["inReal"]);
                inReal = &_json_inReal;
            }
            let optInTimePeriod = params["optInTimePeriod"].as_i64().unwrap_or(10) as i32;
            let out_size = if endIdx >= startIdx { endIdx - startIdx + 1 } else { 0 };
            let mut outBuf0: Vec<f64> = vec![0.0f64; out_size];
            let mut outBegIdx: usize = 0;
            let mut outNBElement: usize = 0;
            let mut rc = RetCode::Success;
            let start_time = Instant::now();
            for _bi in 0..bench_iters {
            rc = core.rocr(
                startIdx, endIdx,
                &inReal,
                optInTimePeriod,
                &mut outBegIdx, &mut outNBElement, &mut outBuf0,
            );
            }
            let elapsed_ns = start_time.elapsed().as_nanos() as u64 / bench_iters as u64;
            let start_time_ung = Instant::now();
            for _biu in 0..bench_iters {
            rc = core.rocr_unguarded(
                startIdx, endIdx,
                &inReal,
                optInTimePeriod,
                &mut outBegIdx, &mut outNBElement, &mut outBuf0,
            );
            }
            let elapsed_ns_ung = start_time_ung.elapsed().as_nanos() as u64 / bench_iters as u64;
            let lookback = core.rocr_lookback(optInTimePeriod);
            let mut resp = format!("{{\"retCode\":{},\"outBegIdx\":{},\"outNBElement\":{},\"lookback\":{},\"timing_ns\":{},\"timing_ns_unguarded\":{}", retcode_to_int(rc), outBegIdx, outNBElement, lookback, elapsed_ns, elapsed_ns_ung);
            resp.push_str(",\"outReal\":"); resp.push_str(&json_f64_array(&outBuf0[..outNBElement]));
            resp.push('}');
            resp
        }
        "TA_ROCR100" => {
            let startIdx = params["startIdx"].as_u64().unwrap_or(0) as usize;
            let endIdx = params["endIdx"].as_u64().unwrap_or(0) as usize;
            let use_preloaded = params["use_preloaded"].as_i64().unwrap_or(0);
            let bench_iters = std::cmp::max(1, params["iters"].as_i64().unwrap_or(1)) as u64;
            let mut _json_inReal: Vec<f64> = Vec::new();
            let inReal: &[f64];
            if use_preloaded != 0 && ref_data.n > 0 {
                inReal = &ref_data.close[..ref_data.n];
            } else {
                _json_inReal = parse_f64_array(&params["inReal"]);
                inReal = &_json_inReal;
            }
            let optInTimePeriod = params["optInTimePeriod"].as_i64().unwrap_or(10) as i32;
            let out_size = if endIdx >= startIdx { endIdx - startIdx + 1 } else { 0 };
            let mut outBuf0: Vec<f64> = vec![0.0f64; out_size];
            let mut outBegIdx: usize = 0;
            let mut outNBElement: usize = 0;
            let mut rc = RetCode::Success;
            let start_time = Instant::now();
            for _bi in 0..bench_iters {
            rc = core.rocr100(
                startIdx, endIdx,
                &inReal,
                optInTimePeriod,
                &mut outBegIdx, &mut outNBElement, &mut outBuf0,
            );
            }
            let elapsed_ns = start_time.elapsed().as_nanos() as u64 / bench_iters as u64;
            let start_time_ung = Instant::now();
            for _biu in 0..bench_iters {
            rc = core.rocr100_unguarded(
                startIdx, endIdx,
                &inReal,
                optInTimePeriod,
                &mut outBegIdx, &mut outNBElement, &mut outBuf0,
            );
            }
            let elapsed_ns_ung = start_time_ung.elapsed().as_nanos() as u64 / bench_iters as u64;
            let lookback = core.rocr100_lookback(optInTimePeriod);
            let mut resp = format!("{{\"retCode\":{},\"outBegIdx\":{},\"outNBElement\":{},\"lookback\":{},\"timing_ns\":{},\"timing_ns_unguarded\":{}", retcode_to_int(rc), outBegIdx, outNBElement, lookback, elapsed_ns, elapsed_ns_ung);
            resp.push_str(",\"outReal\":"); resp.push_str(&json_f64_array(&outBuf0[..outNBElement]));
            resp.push('}');
            resp
        }
        "TA_RSI" => {
            let startIdx = params["startIdx"].as_u64().unwrap_or(0) as usize;
            let endIdx = params["endIdx"].as_u64().unwrap_or(0) as usize;
            let use_preloaded = params["use_preloaded"].as_i64().unwrap_or(0);
            let bench_iters = std::cmp::max(1, params["iters"].as_i64().unwrap_or(1)) as u64;
            let mut _json_inReal: Vec<f64> = Vec::new();
            let inReal: &[f64];
            if use_preloaded != 0 && ref_data.n > 0 {
                inReal = &ref_data.close[..ref_data.n];
            } else {
                _json_inReal = parse_f64_array(&params["inReal"]);
                inReal = &_json_inReal;
            }
            let optInTimePeriod = params["optInTimePeriod"].as_i64().unwrap_or(14) as i32;
            if let Some(period) = params["unstablePeriod"].as_i64() {
                core.unstable_period[21] = period as i32;
            }
            let out_size = if endIdx >= startIdx { endIdx - startIdx + 1 } else { 0 };
            let mut outBuf0: Vec<f64> = vec![0.0f64; out_size];
            let mut outBegIdx: usize = 0;
            let mut outNBElement: usize = 0;
            let mut rc = RetCode::Success;
            let start_time = Instant::now();
            for _bi in 0..bench_iters {
            rc = core.rsi(
                startIdx, endIdx,
                &inReal,
                optInTimePeriod,
                &mut outBegIdx, &mut outNBElement, &mut outBuf0,
            );
            }
            let elapsed_ns = start_time.elapsed().as_nanos() as u64 / bench_iters as u64;
            let start_time_ung = Instant::now();
            for _biu in 0..bench_iters {
            rc = core.rsi_unguarded(
                startIdx, endIdx,
                &inReal,
                optInTimePeriod,
                &mut outBegIdx, &mut outNBElement, &mut outBuf0,
            );
            }
            let elapsed_ns_ung = start_time_ung.elapsed().as_nanos() as u64 / bench_iters as u64;
            let lookback = core.rsi_lookback(optInTimePeriod);
            let mut resp = format!("{{\"retCode\":{},\"outBegIdx\":{},\"outNBElement\":{},\"lookback\":{},\"timing_ns\":{},\"timing_ns_unguarded\":{}", retcode_to_int(rc), outBegIdx, outNBElement, lookback, elapsed_ns, elapsed_ns_ung);
            resp.push_str(",\"outReal\":"); resp.push_str(&json_f64_array(&outBuf0[..outNBElement]));
            resp.push('}');
            resp
        }
        "TA_SAR" => {
            let startIdx = params["startIdx"].as_u64().unwrap_or(0) as usize;
            let endIdx = params["endIdx"].as_u64().unwrap_or(0) as usize;
            let use_preloaded = params["use_preloaded"].as_i64().unwrap_or(0);
            let bench_iters = std::cmp::max(1, params["iters"].as_i64().unwrap_or(1)) as u64;
            let mut _json_inHigh: Vec<f64> = Vec::new();
            let mut _json_inLow: Vec<f64> = Vec::new();
            let inHigh: &[f64];
            let inLow: &[f64];
            if use_preloaded != 0 && ref_data.n > 0 {
                inHigh = &ref_data.high[..ref_data.n];
                inLow = &ref_data.low[..ref_data.n];
            } else {
                _json_inHigh = parse_f64_array(&params["inHigh"]);
                inHigh = &_json_inHigh;
                _json_inLow = parse_f64_array(&params["inLow"]);
                inLow = &_json_inLow;
            }
            let optInAcceleration = params["optInAcceleration"].as_f64().unwrap_or(0.02) as f64;
            let optInMaximum = params["optInMaximum"].as_f64().unwrap_or(0.2) as f64;
            let out_size = if endIdx >= startIdx { endIdx - startIdx + 1 } else { 0 };
            let mut outBuf0: Vec<f64> = vec![0.0f64; out_size];
            let mut outBegIdx: usize = 0;
            let mut outNBElement: usize = 0;
            let mut rc = RetCode::Success;
            let start_time = Instant::now();
            for _bi in 0..bench_iters {
            rc = core.sar(
                startIdx, endIdx,
                &inHigh,
                &inLow,
                optInAcceleration,
                optInMaximum,
                &mut outBegIdx, &mut outNBElement, &mut outBuf0,
            );
            }
            let elapsed_ns = start_time.elapsed().as_nanos() as u64 / bench_iters as u64;
            let start_time_ung = Instant::now();
            for _biu in 0..bench_iters {
            rc = core.sar_unguarded(
                startIdx, endIdx,
                &inHigh,
                &inLow,
                optInAcceleration,
                optInMaximum,
                &mut outBegIdx, &mut outNBElement, &mut outBuf0,
            );
            }
            let elapsed_ns_ung = start_time_ung.elapsed().as_nanos() as u64 / bench_iters as u64;
            let lookback = core.sar_lookback(optInAcceleration, optInMaximum);
            let mut resp = format!("{{\"retCode\":{},\"outBegIdx\":{},\"outNBElement\":{},\"lookback\":{},\"timing_ns\":{},\"timing_ns_unguarded\":{}", retcode_to_int(rc), outBegIdx, outNBElement, lookback, elapsed_ns, elapsed_ns_ung);
            resp.push_str(",\"outReal\":"); resp.push_str(&json_f64_array(&outBuf0[..outNBElement]));
            resp.push('}');
            resp
        }
        "TA_SAREXT" => {
            let startIdx = params["startIdx"].as_u64().unwrap_or(0) as usize;
            let endIdx = params["endIdx"].as_u64().unwrap_or(0) as usize;
            let use_preloaded = params["use_preloaded"].as_i64().unwrap_or(0);
            let bench_iters = std::cmp::max(1, params["iters"].as_i64().unwrap_or(1)) as u64;
            let mut _json_inHigh: Vec<f64> = Vec::new();
            let mut _json_inLow: Vec<f64> = Vec::new();
            let inHigh: &[f64];
            let inLow: &[f64];
            if use_preloaded != 0 && ref_data.n > 0 {
                inHigh = &ref_data.high[..ref_data.n];
                inLow = &ref_data.low[..ref_data.n];
            } else {
                _json_inHigh = parse_f64_array(&params["inHigh"]);
                inHigh = &_json_inHigh;
                _json_inLow = parse_f64_array(&params["inLow"]);
                inLow = &_json_inLow;
            }
            let optInStartValue = params["optInStartValue"].as_f64().unwrap_or(0.0) as f64;
            let optInOffsetOnReverse = params["optInOffsetOnReverse"].as_f64().unwrap_or(0.0) as f64;
            let optInAccelerationInitLong = params["optInAccelerationInitLong"].as_f64().unwrap_or(0.02) as f64;
            let optInAccelerationLong = params["optInAccelerationLong"].as_f64().unwrap_or(0.02) as f64;
            let optInAccelerationMaxLong = params["optInAccelerationMaxLong"].as_f64().unwrap_or(0.2) as f64;
            let optInAccelerationInitShort = params["optInAccelerationInitShort"].as_f64().unwrap_or(0.02) as f64;
            let optInAccelerationShort = params["optInAccelerationShort"].as_f64().unwrap_or(0.02) as f64;
            let optInAccelerationMaxShort = params["optInAccelerationMaxShort"].as_f64().unwrap_or(0.2) as f64;
            let out_size = if endIdx >= startIdx { endIdx - startIdx + 1 } else { 0 };
            let mut outBuf0: Vec<f64> = vec![0.0f64; out_size];
            let mut outBegIdx: usize = 0;
            let mut outNBElement: usize = 0;
            let mut rc = RetCode::Success;
            let start_time = Instant::now();
            for _bi in 0..bench_iters {
            rc = core.sarext(
                startIdx, endIdx,
                &inHigh,
                &inLow,
                optInStartValue,
                optInOffsetOnReverse,
                optInAccelerationInitLong,
                optInAccelerationLong,
                optInAccelerationMaxLong,
                optInAccelerationInitShort,
                optInAccelerationShort,
                optInAccelerationMaxShort,
                &mut outBegIdx, &mut outNBElement, &mut outBuf0,
            );
            }
            let elapsed_ns = start_time.elapsed().as_nanos() as u64 / bench_iters as u64;
            let start_time_ung = Instant::now();
            for _biu in 0..bench_iters {
            rc = core.sarext_unguarded(
                startIdx, endIdx,
                &inHigh,
                &inLow,
                optInStartValue,
                optInOffsetOnReverse,
                optInAccelerationInitLong,
                optInAccelerationLong,
                optInAccelerationMaxLong,
                optInAccelerationInitShort,
                optInAccelerationShort,
                optInAccelerationMaxShort,
                &mut outBegIdx, &mut outNBElement, &mut outBuf0,
            );
            }
            let elapsed_ns_ung = start_time_ung.elapsed().as_nanos() as u64 / bench_iters as u64;
            let lookback = core.sarext_lookback(optInStartValue, optInOffsetOnReverse, optInAccelerationInitLong, optInAccelerationLong, optInAccelerationMaxLong, optInAccelerationInitShort, optInAccelerationShort, optInAccelerationMaxShort);
            let mut resp = format!("{{\"retCode\":{},\"outBegIdx\":{},\"outNBElement\":{},\"lookback\":{},\"timing_ns\":{},\"timing_ns_unguarded\":{}", retcode_to_int(rc), outBegIdx, outNBElement, lookback, elapsed_ns, elapsed_ns_ung);
            resp.push_str(",\"outReal\":"); resp.push_str(&json_f64_array(&outBuf0[..outNBElement]));
            resp.push('}');
            resp
        }
        "TA_SIN" => {
            let startIdx = params["startIdx"].as_u64().unwrap_or(0) as usize;
            let endIdx = params["endIdx"].as_u64().unwrap_or(0) as usize;
            let use_preloaded = params["use_preloaded"].as_i64().unwrap_or(0);
            let bench_iters = std::cmp::max(1, params["iters"].as_i64().unwrap_or(1)) as u64;
            let mut _json_inReal: Vec<f64> = Vec::new();
            let inReal: &[f64];
            if use_preloaded != 0 && ref_data.n > 0 {
                inReal = &ref_data.close[..ref_data.n];
            } else {
                _json_inReal = parse_f64_array(&params["inReal"]);
                inReal = &_json_inReal;
            }
            let out_size = if endIdx >= startIdx { endIdx - startIdx + 1 } else { 0 };
            let mut outBuf0: Vec<f64> = vec![0.0f64; out_size];
            let mut outBegIdx: usize = 0;
            let mut outNBElement: usize = 0;
            let mut rc = RetCode::Success;
            let start_time = Instant::now();
            for _bi in 0..bench_iters {
            rc = core.sin(
                startIdx, endIdx,
                &inReal,
                &mut outBegIdx, &mut outNBElement, &mut outBuf0,
            );
            }
            let elapsed_ns = start_time.elapsed().as_nanos() as u64 / bench_iters as u64;
            let start_time_ung = Instant::now();
            for _biu in 0..bench_iters {
            rc = core.sin_unguarded(
                startIdx, endIdx,
                &inReal,
                &mut outBegIdx, &mut outNBElement, &mut outBuf0,
            );
            }
            let elapsed_ns_ung = start_time_ung.elapsed().as_nanos() as u64 / bench_iters as u64;
            let lookback = core.sin_lookback();
            let mut resp = format!("{{\"retCode\":{},\"outBegIdx\":{},\"outNBElement\":{},\"lookback\":{},\"timing_ns\":{},\"timing_ns_unguarded\":{}", retcode_to_int(rc), outBegIdx, outNBElement, lookback, elapsed_ns, elapsed_ns_ung);
            resp.push_str(",\"outReal\":"); resp.push_str(&json_f64_array(&outBuf0[..outNBElement]));
            resp.push('}');
            resp
        }
        "TA_SINH" => {
            let startIdx = params["startIdx"].as_u64().unwrap_or(0) as usize;
            let endIdx = params["endIdx"].as_u64().unwrap_or(0) as usize;
            let use_preloaded = params["use_preloaded"].as_i64().unwrap_or(0);
            let bench_iters = std::cmp::max(1, params["iters"].as_i64().unwrap_or(1)) as u64;
            let mut _json_inReal: Vec<f64> = Vec::new();
            let inReal: &[f64];
            if use_preloaded != 0 && ref_data.n > 0 {
                inReal = &ref_data.close[..ref_data.n];
            } else {
                _json_inReal = parse_f64_array(&params["inReal"]);
                inReal = &_json_inReal;
            }
            let out_size = if endIdx >= startIdx { endIdx - startIdx + 1 } else { 0 };
            let mut outBuf0: Vec<f64> = vec![0.0f64; out_size];
            let mut outBegIdx: usize = 0;
            let mut outNBElement: usize = 0;
            let mut rc = RetCode::Success;
            let start_time = Instant::now();
            for _bi in 0..bench_iters {
            rc = core.sinh(
                startIdx, endIdx,
                &inReal,
                &mut outBegIdx, &mut outNBElement, &mut outBuf0,
            );
            }
            let elapsed_ns = start_time.elapsed().as_nanos() as u64 / bench_iters as u64;
            let start_time_ung = Instant::now();
            for _biu in 0..bench_iters {
            rc = core.sinh_unguarded(
                startIdx, endIdx,
                &inReal,
                &mut outBegIdx, &mut outNBElement, &mut outBuf0,
            );
            }
            let elapsed_ns_ung = start_time_ung.elapsed().as_nanos() as u64 / bench_iters as u64;
            let lookback = core.sinh_lookback();
            let mut resp = format!("{{\"retCode\":{},\"outBegIdx\":{},\"outNBElement\":{},\"lookback\":{},\"timing_ns\":{},\"timing_ns_unguarded\":{}", retcode_to_int(rc), outBegIdx, outNBElement, lookback, elapsed_ns, elapsed_ns_ung);
            resp.push_str(",\"outReal\":"); resp.push_str(&json_f64_array(&outBuf0[..outNBElement]));
            resp.push('}');
            resp
        }
        "TA_SMA" => {
            let startIdx = params["startIdx"].as_u64().unwrap_or(0) as usize;
            let endIdx = params["endIdx"].as_u64().unwrap_or(0) as usize;
            let use_preloaded = params["use_preloaded"].as_i64().unwrap_or(0);
            let bench_iters = std::cmp::max(1, params["iters"].as_i64().unwrap_or(1)) as u64;
            let mut _json_inReal: Vec<f64> = Vec::new();
            let inReal: &[f64];
            if use_preloaded != 0 && ref_data.n > 0 {
                inReal = &ref_data.close[..ref_data.n];
            } else {
                _json_inReal = parse_f64_array(&params["inReal"]);
                inReal = &_json_inReal;
            }
            let optInTimePeriod = params["optInTimePeriod"].as_i64().unwrap_or(30) as i32;
            let out_size = if endIdx >= startIdx { endIdx - startIdx + 1 } else { 0 };
            let mut outBuf0: Vec<f64> = vec![0.0f64; out_size];
            let mut outBegIdx: usize = 0;
            let mut outNBElement: usize = 0;
            let mut rc = RetCode::Success;
            let start_time = Instant::now();
            for _bi in 0..bench_iters {
            rc = core.sma(
                startIdx, endIdx,
                &inReal,
                optInTimePeriod,
                &mut outBegIdx, &mut outNBElement, &mut outBuf0,
            );
            }
            let elapsed_ns = start_time.elapsed().as_nanos() as u64 / bench_iters as u64;
            let start_time_ung = Instant::now();
            for _biu in 0..bench_iters {
            rc = core.sma_unguarded(
                startIdx, endIdx,
                &inReal,
                optInTimePeriod,
                &mut outBegIdx, &mut outNBElement, &mut outBuf0,
            );
            }
            let elapsed_ns_ung = start_time_ung.elapsed().as_nanos() as u64 / bench_iters as u64;
            let lookback = core.sma_lookback(optInTimePeriod);
            let mut resp = format!("{{\"retCode\":{},\"outBegIdx\":{},\"outNBElement\":{},\"lookback\":{},\"timing_ns\":{},\"timing_ns_unguarded\":{}", retcode_to_int(rc), outBegIdx, outNBElement, lookback, elapsed_ns, elapsed_ns_ung);
            resp.push_str(",\"outReal\":"); resp.push_str(&json_f64_array(&outBuf0[..outNBElement]));
            resp.push('}');
            resp
        }
        "TA_SQRT" => {
            let startIdx = params["startIdx"].as_u64().unwrap_or(0) as usize;
            let endIdx = params["endIdx"].as_u64().unwrap_or(0) as usize;
            let use_preloaded = params["use_preloaded"].as_i64().unwrap_or(0);
            let bench_iters = std::cmp::max(1, params["iters"].as_i64().unwrap_or(1)) as u64;
            let mut _json_inReal: Vec<f64> = Vec::new();
            let inReal: &[f64];
            if use_preloaded != 0 && ref_data.n > 0 {
                inReal = &ref_data.close[..ref_data.n];
            } else {
                _json_inReal = parse_f64_array(&params["inReal"]);
                inReal = &_json_inReal;
            }
            let out_size = if endIdx >= startIdx { endIdx - startIdx + 1 } else { 0 };
            let mut outBuf0: Vec<f64> = vec![0.0f64; out_size];
            let mut outBegIdx: usize = 0;
            let mut outNBElement: usize = 0;
            let mut rc = RetCode::Success;
            let start_time = Instant::now();
            for _bi in 0..bench_iters {
            rc = core.sqrt(
                startIdx, endIdx,
                &inReal,
                &mut outBegIdx, &mut outNBElement, &mut outBuf0,
            );
            }
            let elapsed_ns = start_time.elapsed().as_nanos() as u64 / bench_iters as u64;
            let start_time_ung = Instant::now();
            for _biu in 0..bench_iters {
            rc = core.sqrt_unguarded(
                startIdx, endIdx,
                &inReal,
                &mut outBegIdx, &mut outNBElement, &mut outBuf0,
            );
            }
            let elapsed_ns_ung = start_time_ung.elapsed().as_nanos() as u64 / bench_iters as u64;
            let lookback = core.sqrt_lookback();
            let mut resp = format!("{{\"retCode\":{},\"outBegIdx\":{},\"outNBElement\":{},\"lookback\":{},\"timing_ns\":{},\"timing_ns_unguarded\":{}", retcode_to_int(rc), outBegIdx, outNBElement, lookback, elapsed_ns, elapsed_ns_ung);
            resp.push_str(",\"outReal\":"); resp.push_str(&json_f64_array(&outBuf0[..outNBElement]));
            resp.push('}');
            resp
        }
        "TA_STDDEV" => {
            let startIdx = params["startIdx"].as_u64().unwrap_or(0) as usize;
            let endIdx = params["endIdx"].as_u64().unwrap_or(0) as usize;
            let use_preloaded = params["use_preloaded"].as_i64().unwrap_or(0);
            let bench_iters = std::cmp::max(1, params["iters"].as_i64().unwrap_or(1)) as u64;
            let mut _json_inReal: Vec<f64> = Vec::new();
            let inReal: &[f64];
            if use_preloaded != 0 && ref_data.n > 0 {
                inReal = &ref_data.close[..ref_data.n];
            } else {
                _json_inReal = parse_f64_array(&params["inReal"]);
                inReal = &_json_inReal;
            }
            let optInTimePeriod = params["optInTimePeriod"].as_i64().unwrap_or(5) as i32;
            let optInNbDev = params["optInNbDev"].as_f64().unwrap_or(1.0) as f64;
            let out_size = if endIdx >= startIdx { endIdx - startIdx + 1 } else { 0 };
            let mut outBuf0: Vec<f64> = vec![0.0f64; out_size];
            let mut outBegIdx: usize = 0;
            let mut outNBElement: usize = 0;
            let mut rc = RetCode::Success;
            let start_time = Instant::now();
            for _bi in 0..bench_iters {
            rc = core.stddev(
                startIdx, endIdx,
                &inReal,
                optInTimePeriod,
                optInNbDev,
                &mut outBegIdx, &mut outNBElement, &mut outBuf0,
            );
            }
            let elapsed_ns = start_time.elapsed().as_nanos() as u64 / bench_iters as u64;
            let start_time_ung = Instant::now();
            for _biu in 0..bench_iters {
            rc = core.stddev_unguarded(
                startIdx, endIdx,
                &inReal,
                optInTimePeriod,
                optInNbDev,
                &mut outBegIdx, &mut outNBElement, &mut outBuf0,
            );
            }
            let elapsed_ns_ung = start_time_ung.elapsed().as_nanos() as u64 / bench_iters as u64;
            let lookback = core.stddev_lookback(optInTimePeriod, optInNbDev);
            let mut resp = format!("{{\"retCode\":{},\"outBegIdx\":{},\"outNBElement\":{},\"lookback\":{},\"timing_ns\":{},\"timing_ns_unguarded\":{}", retcode_to_int(rc), outBegIdx, outNBElement, lookback, elapsed_ns, elapsed_ns_ung);
            resp.push_str(",\"outReal\":"); resp.push_str(&json_f64_array(&outBuf0[..outNBElement]));
            resp.push('}');
            resp
        }
        "TA_STOCH" => {
            let startIdx = params["startIdx"].as_u64().unwrap_or(0) as usize;
            let endIdx = params["endIdx"].as_u64().unwrap_or(0) as usize;
            let use_preloaded = params["use_preloaded"].as_i64().unwrap_or(0);
            let bench_iters = std::cmp::max(1, params["iters"].as_i64().unwrap_or(1)) as u64;
            let mut _json_inHigh: Vec<f64> = Vec::new();
            let mut _json_inLow: Vec<f64> = Vec::new();
            let mut _json_inClose: Vec<f64> = Vec::new();
            let inHigh: &[f64];
            let inLow: &[f64];
            let inClose: &[f64];
            if use_preloaded != 0 && ref_data.n > 0 {
                inHigh = &ref_data.high[..ref_data.n];
                inLow = &ref_data.low[..ref_data.n];
                inClose = &ref_data.close[..ref_data.n];
            } else {
                _json_inHigh = parse_f64_array(&params["inHigh"]);
                inHigh = &_json_inHigh;
                _json_inLow = parse_f64_array(&params["inLow"]);
                inLow = &_json_inLow;
                _json_inClose = parse_f64_array(&params["inClose"]);
                inClose = &_json_inClose;
            }
            let optInFastK_Period = params["optInFastK_Period"].as_i64().unwrap_or(5) as i32;
            let optInSlowK_Period = params["optInSlowK_Period"].as_i64().unwrap_or(3) as i32;
            let optInSlowK_MAType = params["optInSlowK_MAType"].as_i64().unwrap_or(0) as i32;
            let optInSlowD_Period = params["optInSlowD_Period"].as_i64().unwrap_or(3) as i32;
            let optInSlowD_MAType = params["optInSlowD_MAType"].as_i64().unwrap_or(0) as i32;
            let out_size = if endIdx >= startIdx { endIdx - startIdx + 1 } else { 0 };
            let mut outBuf0: Vec<f64> = vec![0.0f64; out_size];
            let mut outBuf1: Vec<f64> = vec![0.0f64; out_size];
            let mut outBegIdx: usize = 0;
            let mut outNBElement: usize = 0;
            let mut rc = RetCode::Success;
            let start_time = Instant::now();
            for _bi in 0..bench_iters {
            rc = core.stoch(
                startIdx, endIdx,
                &inHigh,
                &inLow,
                &inClose,
                optInFastK_Period,
                optInSlowK_Period,
                optInSlowK_MAType,
                optInSlowD_Period,
                optInSlowD_MAType,
                &mut outBegIdx, &mut outNBElement, &mut outBuf0, &mut outBuf1,
            );
            }
            let elapsed_ns = start_time.elapsed().as_nanos() as u64 / bench_iters as u64;
            let start_time_ung = Instant::now();
            for _biu in 0..bench_iters {
            rc = core.stoch_unguarded(
                startIdx, endIdx,
                &inHigh,
                &inLow,
                &inClose,
                optInFastK_Period,
                optInSlowK_Period,
                optInSlowK_MAType,
                optInSlowD_Period,
                optInSlowD_MAType,
                &mut outBegIdx, &mut outNBElement, &mut outBuf0, &mut outBuf1,
            );
            }
            let elapsed_ns_ung = start_time_ung.elapsed().as_nanos() as u64 / bench_iters as u64;
            let lookback = core.stoch_lookback(optInFastK_Period, optInSlowK_Period, optInSlowK_MAType, optInSlowD_Period, optInSlowD_MAType);
            let mut resp = format!("{{\"retCode\":{},\"outBegIdx\":{},\"outNBElement\":{},\"lookback\":{},\"timing_ns\":{},\"timing_ns_unguarded\":{}", retcode_to_int(rc), outBegIdx, outNBElement, lookback, elapsed_ns, elapsed_ns_ung);
            resp.push_str(",\"outReal\":"); resp.push_str(&json_f64_array(&outBuf0[..outNBElement]));
            resp.push_str(",\"outReal1\":"); resp.push_str(&json_f64_array(&outBuf1[..outNBElement]));
            resp.push('}');
            resp
        }
        "TA_STOCHF" => {
            let startIdx = params["startIdx"].as_u64().unwrap_or(0) as usize;
            let endIdx = params["endIdx"].as_u64().unwrap_or(0) as usize;
            let use_preloaded = params["use_preloaded"].as_i64().unwrap_or(0);
            let bench_iters = std::cmp::max(1, params["iters"].as_i64().unwrap_or(1)) as u64;
            let mut _json_inHigh: Vec<f64> = Vec::new();
            let mut _json_inLow: Vec<f64> = Vec::new();
            let mut _json_inClose: Vec<f64> = Vec::new();
            let inHigh: &[f64];
            let inLow: &[f64];
            let inClose: &[f64];
            if use_preloaded != 0 && ref_data.n > 0 {
                inHigh = &ref_data.high[..ref_data.n];
                inLow = &ref_data.low[..ref_data.n];
                inClose = &ref_data.close[..ref_data.n];
            } else {
                _json_inHigh = parse_f64_array(&params["inHigh"]);
                inHigh = &_json_inHigh;
                _json_inLow = parse_f64_array(&params["inLow"]);
                inLow = &_json_inLow;
                _json_inClose = parse_f64_array(&params["inClose"]);
                inClose = &_json_inClose;
            }
            let optInFastK_Period = params["optInFastK_Period"].as_i64().unwrap_or(5) as i32;
            let optInFastD_Period = params["optInFastD_Period"].as_i64().unwrap_or(3) as i32;
            let optInFastD_MAType = params["optInFastD_MAType"].as_i64().unwrap_or(0) as i32;
            let out_size = if endIdx >= startIdx { endIdx - startIdx + 1 } else { 0 };
            let mut outBuf0: Vec<f64> = vec![0.0f64; out_size];
            let mut outBuf1: Vec<f64> = vec![0.0f64; out_size];
            let mut outBegIdx: usize = 0;
            let mut outNBElement: usize = 0;
            let mut rc = RetCode::Success;
            let start_time = Instant::now();
            for _bi in 0..bench_iters {
            rc = core.stochf(
                startIdx, endIdx,
                &inHigh,
                &inLow,
                &inClose,
                optInFastK_Period,
                optInFastD_Period,
                optInFastD_MAType,
                &mut outBegIdx, &mut outNBElement, &mut outBuf0, &mut outBuf1,
            );
            }
            let elapsed_ns = start_time.elapsed().as_nanos() as u64 / bench_iters as u64;
            let start_time_ung = Instant::now();
            for _biu in 0..bench_iters {
            rc = core.stochf_unguarded(
                startIdx, endIdx,
                &inHigh,
                &inLow,
                &inClose,
                optInFastK_Period,
                optInFastD_Period,
                optInFastD_MAType,
                &mut outBegIdx, &mut outNBElement, &mut outBuf0, &mut outBuf1,
            );
            }
            let elapsed_ns_ung = start_time_ung.elapsed().as_nanos() as u64 / bench_iters as u64;
            let lookback = core.stochf_lookback(optInFastK_Period, optInFastD_Period, optInFastD_MAType);
            let mut resp = format!("{{\"retCode\":{},\"outBegIdx\":{},\"outNBElement\":{},\"lookback\":{},\"timing_ns\":{},\"timing_ns_unguarded\":{}", retcode_to_int(rc), outBegIdx, outNBElement, lookback, elapsed_ns, elapsed_ns_ung);
            resp.push_str(",\"outReal\":"); resp.push_str(&json_f64_array(&outBuf0[..outNBElement]));
            resp.push_str(",\"outReal1\":"); resp.push_str(&json_f64_array(&outBuf1[..outNBElement]));
            resp.push('}');
            resp
        }
        "TA_STOCHRSI" => {
            let startIdx = params["startIdx"].as_u64().unwrap_or(0) as usize;
            let endIdx = params["endIdx"].as_u64().unwrap_or(0) as usize;
            let use_preloaded = params["use_preloaded"].as_i64().unwrap_or(0);
            let bench_iters = std::cmp::max(1, params["iters"].as_i64().unwrap_or(1)) as u64;
            let mut _json_inReal: Vec<f64> = Vec::new();
            let inReal: &[f64];
            if use_preloaded != 0 && ref_data.n > 0 {
                inReal = &ref_data.close[..ref_data.n];
            } else {
                _json_inReal = parse_f64_array(&params["inReal"]);
                inReal = &_json_inReal;
            }
            let optInTimePeriod = params["optInTimePeriod"].as_i64().unwrap_or(14) as i32;
            let optInFastK_Period = params["optInFastK_Period"].as_i64().unwrap_or(5) as i32;
            let optInFastD_Period = params["optInFastD_Period"].as_i64().unwrap_or(3) as i32;
            let optInFastD_MAType = params["optInFastD_MAType"].as_i64().unwrap_or(0) as i32;
            if let Some(period) = params["unstablePeriod"].as_i64() {
                core.unstable_period[22] = period as i32;
            }
            let out_size = if endIdx >= startIdx { endIdx - startIdx + 1 } else { 0 };
            let mut outBuf0: Vec<f64> = vec![0.0f64; out_size];
            let mut outBuf1: Vec<f64> = vec![0.0f64; out_size];
            let mut outBegIdx: usize = 0;
            let mut outNBElement: usize = 0;
            let mut rc = RetCode::Success;
            let start_time = Instant::now();
            for _bi in 0..bench_iters {
            rc = core.stochrsi(
                startIdx, endIdx,
                &inReal,
                optInTimePeriod,
                optInFastK_Period,
                optInFastD_Period,
                optInFastD_MAType,
                &mut outBegIdx, &mut outNBElement, &mut outBuf0, &mut outBuf1,
            );
            }
            let elapsed_ns = start_time.elapsed().as_nanos() as u64 / bench_iters as u64;
            let start_time_ung = Instant::now();
            for _biu in 0..bench_iters {
            rc = core.stochrsi_unguarded(
                startIdx, endIdx,
                &inReal,
                optInTimePeriod,
                optInFastK_Period,
                optInFastD_Period,
                optInFastD_MAType,
                &mut outBegIdx, &mut outNBElement, &mut outBuf0, &mut outBuf1,
            );
            }
            let elapsed_ns_ung = start_time_ung.elapsed().as_nanos() as u64 / bench_iters as u64;
            let lookback = core.stochrsi_lookback(optInTimePeriod, optInFastK_Period, optInFastD_Period, optInFastD_MAType);
            let mut resp = format!("{{\"retCode\":{},\"outBegIdx\":{},\"outNBElement\":{},\"lookback\":{},\"timing_ns\":{},\"timing_ns_unguarded\":{}", retcode_to_int(rc), outBegIdx, outNBElement, lookback, elapsed_ns, elapsed_ns_ung);
            resp.push_str(",\"outReal\":"); resp.push_str(&json_f64_array(&outBuf0[..outNBElement]));
            resp.push_str(",\"outReal1\":"); resp.push_str(&json_f64_array(&outBuf1[..outNBElement]));
            resp.push('}');
            resp
        }
        "TA_SUB" => {
            let startIdx = params["startIdx"].as_u64().unwrap_or(0) as usize;
            let endIdx = params["endIdx"].as_u64().unwrap_or(0) as usize;
            let use_preloaded = params["use_preloaded"].as_i64().unwrap_or(0);
            let bench_iters = std::cmp::max(1, params["iters"].as_i64().unwrap_or(1)) as u64;
            let mut _json_inReal0: Vec<f64> = Vec::new();
            let mut _json_inReal1: Vec<f64> = Vec::new();
            let inReal0: &[f64];
            let inReal1: &[f64];
            if use_preloaded != 0 && ref_data.n > 0 {
                inReal0 = &ref_data.close[..ref_data.n];
                inReal1 = &ref_data.high[..ref_data.n];
            } else {
                _json_inReal0 = parse_f64_array(&params["inReal0"]);
                inReal0 = &_json_inReal0;
                _json_inReal1 = parse_f64_array(&params["inReal1"]);
                inReal1 = &_json_inReal1;
            }
            let out_size = if endIdx >= startIdx { endIdx - startIdx + 1 } else { 0 };
            let mut outBuf0: Vec<f64> = vec![0.0f64; out_size];
            let mut outBegIdx: usize = 0;
            let mut outNBElement: usize = 0;
            let mut rc = RetCode::Success;
            let start_time = Instant::now();
            for _bi in 0..bench_iters {
            rc = core.sub(
                startIdx, endIdx,
                &inReal0,
                &inReal1,
                &mut outBegIdx, &mut outNBElement, &mut outBuf0,
            );
            }
            let elapsed_ns = start_time.elapsed().as_nanos() as u64 / bench_iters as u64;
            let start_time_ung = Instant::now();
            for _biu in 0..bench_iters {
            rc = core.sub_unguarded(
                startIdx, endIdx,
                &inReal0,
                &inReal1,
                &mut outBegIdx, &mut outNBElement, &mut outBuf0,
            );
            }
            let elapsed_ns_ung = start_time_ung.elapsed().as_nanos() as u64 / bench_iters as u64;
            let lookback = core.sub_lookback();
            let mut resp = format!("{{\"retCode\":{},\"outBegIdx\":{},\"outNBElement\":{},\"lookback\":{},\"timing_ns\":{},\"timing_ns_unguarded\":{}", retcode_to_int(rc), outBegIdx, outNBElement, lookback, elapsed_ns, elapsed_ns_ung);
            resp.push_str(",\"outReal\":"); resp.push_str(&json_f64_array(&outBuf0[..outNBElement]));
            resp.push('}');
            resp
        }
        "TA_SUM" => {
            let startIdx = params["startIdx"].as_u64().unwrap_or(0) as usize;
            let endIdx = params["endIdx"].as_u64().unwrap_or(0) as usize;
            let use_preloaded = params["use_preloaded"].as_i64().unwrap_or(0);
            let bench_iters = std::cmp::max(1, params["iters"].as_i64().unwrap_or(1)) as u64;
            let mut _json_inReal: Vec<f64> = Vec::new();
            let inReal: &[f64];
            if use_preloaded != 0 && ref_data.n > 0 {
                inReal = &ref_data.close[..ref_data.n];
            } else {
                _json_inReal = parse_f64_array(&params["inReal"]);
                inReal = &_json_inReal;
            }
            let optInTimePeriod = params["optInTimePeriod"].as_i64().unwrap_or(30) as i32;
            let out_size = if endIdx >= startIdx { endIdx - startIdx + 1 } else { 0 };
            let mut outBuf0: Vec<f64> = vec![0.0f64; out_size];
            let mut outBegIdx: usize = 0;
            let mut outNBElement: usize = 0;
            let mut rc = RetCode::Success;
            let start_time = Instant::now();
            for _bi in 0..bench_iters {
            rc = core.sum(
                startIdx, endIdx,
                &inReal,
                optInTimePeriod,
                &mut outBegIdx, &mut outNBElement, &mut outBuf0,
            );
            }
            let elapsed_ns = start_time.elapsed().as_nanos() as u64 / bench_iters as u64;
            let start_time_ung = Instant::now();
            for _biu in 0..bench_iters {
            rc = core.sum_unguarded(
                startIdx, endIdx,
                &inReal,
                optInTimePeriod,
                &mut outBegIdx, &mut outNBElement, &mut outBuf0,
            );
            }
            let elapsed_ns_ung = start_time_ung.elapsed().as_nanos() as u64 / bench_iters as u64;
            let lookback = core.sum_lookback(optInTimePeriod);
            let mut resp = format!("{{\"retCode\":{},\"outBegIdx\":{},\"outNBElement\":{},\"lookback\":{},\"timing_ns\":{},\"timing_ns_unguarded\":{}", retcode_to_int(rc), outBegIdx, outNBElement, lookback, elapsed_ns, elapsed_ns_ung);
            resp.push_str(",\"outReal\":"); resp.push_str(&json_f64_array(&outBuf0[..outNBElement]));
            resp.push('}');
            resp
        }
        "TA_T3" => {
            let startIdx = params["startIdx"].as_u64().unwrap_or(0) as usize;
            let endIdx = params["endIdx"].as_u64().unwrap_or(0) as usize;
            let use_preloaded = params["use_preloaded"].as_i64().unwrap_or(0);
            let bench_iters = std::cmp::max(1, params["iters"].as_i64().unwrap_or(1)) as u64;
            let mut _json_inReal: Vec<f64> = Vec::new();
            let inReal: &[f64];
            if use_preloaded != 0 && ref_data.n > 0 {
                inReal = &ref_data.close[..ref_data.n];
            } else {
                _json_inReal = parse_f64_array(&params["inReal"]);
                inReal = &_json_inReal;
            }
            let optInTimePeriod = params["optInTimePeriod"].as_i64().unwrap_or(5) as i32;
            let optInVFactor = params["optInVFactor"].as_f64().unwrap_or(0.7) as f64;
            if let Some(period) = params["unstablePeriod"].as_i64() {
                core.unstable_period[23] = period as i32;
            }
            let out_size = if endIdx >= startIdx { endIdx - startIdx + 1 } else { 0 };
            let mut outBuf0: Vec<f64> = vec![0.0f64; out_size];
            let mut outBegIdx: usize = 0;
            let mut outNBElement: usize = 0;
            let mut rc = RetCode::Success;
            let start_time = Instant::now();
            for _bi in 0..bench_iters {
            rc = core.t3(
                startIdx, endIdx,
                &inReal,
                optInTimePeriod,
                optInVFactor,
                &mut outBegIdx, &mut outNBElement, &mut outBuf0,
            );
            }
            let elapsed_ns = start_time.elapsed().as_nanos() as u64 / bench_iters as u64;
            let start_time_ung = Instant::now();
            for _biu in 0..bench_iters {
            rc = core.t3_unguarded(
                startIdx, endIdx,
                &inReal,
                optInTimePeriod,
                optInVFactor,
                &mut outBegIdx, &mut outNBElement, &mut outBuf0,
            );
            }
            let elapsed_ns_ung = start_time_ung.elapsed().as_nanos() as u64 / bench_iters as u64;
            let lookback = core.t3_lookback(optInTimePeriod, optInVFactor);
            let mut resp = format!("{{\"retCode\":{},\"outBegIdx\":{},\"outNBElement\":{},\"lookback\":{},\"timing_ns\":{},\"timing_ns_unguarded\":{}", retcode_to_int(rc), outBegIdx, outNBElement, lookback, elapsed_ns, elapsed_ns_ung);
            resp.push_str(",\"outReal\":"); resp.push_str(&json_f64_array(&outBuf0[..outNBElement]));
            resp.push('}');
            resp
        }
        "TA_TAN" => {
            let startIdx = params["startIdx"].as_u64().unwrap_or(0) as usize;
            let endIdx = params["endIdx"].as_u64().unwrap_or(0) as usize;
            let use_preloaded = params["use_preloaded"].as_i64().unwrap_or(0);
            let bench_iters = std::cmp::max(1, params["iters"].as_i64().unwrap_or(1)) as u64;
            let mut _json_inReal: Vec<f64> = Vec::new();
            let inReal: &[f64];
            if use_preloaded != 0 && ref_data.n > 0 {
                inReal = &ref_data.close[..ref_data.n];
            } else {
                _json_inReal = parse_f64_array(&params["inReal"]);
                inReal = &_json_inReal;
            }
            let out_size = if endIdx >= startIdx { endIdx - startIdx + 1 } else { 0 };
            let mut outBuf0: Vec<f64> = vec![0.0f64; out_size];
            let mut outBegIdx: usize = 0;
            let mut outNBElement: usize = 0;
            let mut rc = RetCode::Success;
            let start_time = Instant::now();
            for _bi in 0..bench_iters {
            rc = core.tan(
                startIdx, endIdx,
                &inReal,
                &mut outBegIdx, &mut outNBElement, &mut outBuf0,
            );
            }
            let elapsed_ns = start_time.elapsed().as_nanos() as u64 / bench_iters as u64;
            let start_time_ung = Instant::now();
            for _biu in 0..bench_iters {
            rc = core.tan_unguarded(
                startIdx, endIdx,
                &inReal,
                &mut outBegIdx, &mut outNBElement, &mut outBuf0,
            );
            }
            let elapsed_ns_ung = start_time_ung.elapsed().as_nanos() as u64 / bench_iters as u64;
            let lookback = core.tan_lookback();
            let mut resp = format!("{{\"retCode\":{},\"outBegIdx\":{},\"outNBElement\":{},\"lookback\":{},\"timing_ns\":{},\"timing_ns_unguarded\":{}", retcode_to_int(rc), outBegIdx, outNBElement, lookback, elapsed_ns, elapsed_ns_ung);
            resp.push_str(",\"outReal\":"); resp.push_str(&json_f64_array(&outBuf0[..outNBElement]));
            resp.push('}');
            resp
        }
        "TA_TANH" => {
            let startIdx = params["startIdx"].as_u64().unwrap_or(0) as usize;
            let endIdx = params["endIdx"].as_u64().unwrap_or(0) as usize;
            let use_preloaded = params["use_preloaded"].as_i64().unwrap_or(0);
            let bench_iters = std::cmp::max(1, params["iters"].as_i64().unwrap_or(1)) as u64;
            let mut _json_inReal: Vec<f64> = Vec::new();
            let inReal: &[f64];
            if use_preloaded != 0 && ref_data.n > 0 {
                inReal = &ref_data.close[..ref_data.n];
            } else {
                _json_inReal = parse_f64_array(&params["inReal"]);
                inReal = &_json_inReal;
            }
            let out_size = if endIdx >= startIdx { endIdx - startIdx + 1 } else { 0 };
            let mut outBuf0: Vec<f64> = vec![0.0f64; out_size];
            let mut outBegIdx: usize = 0;
            let mut outNBElement: usize = 0;
            let mut rc = RetCode::Success;
            let start_time = Instant::now();
            for _bi in 0..bench_iters {
            rc = core.tanh(
                startIdx, endIdx,
                &inReal,
                &mut outBegIdx, &mut outNBElement, &mut outBuf0,
            );
            }
            let elapsed_ns = start_time.elapsed().as_nanos() as u64 / bench_iters as u64;
            let start_time_ung = Instant::now();
            for _biu in 0..bench_iters {
            rc = core.tanh_unguarded(
                startIdx, endIdx,
                &inReal,
                &mut outBegIdx, &mut outNBElement, &mut outBuf0,
            );
            }
            let elapsed_ns_ung = start_time_ung.elapsed().as_nanos() as u64 / bench_iters as u64;
            let lookback = core.tanh_lookback();
            let mut resp = format!("{{\"retCode\":{},\"outBegIdx\":{},\"outNBElement\":{},\"lookback\":{},\"timing_ns\":{},\"timing_ns_unguarded\":{}", retcode_to_int(rc), outBegIdx, outNBElement, lookback, elapsed_ns, elapsed_ns_ung);
            resp.push_str(",\"outReal\":"); resp.push_str(&json_f64_array(&outBuf0[..outNBElement]));
            resp.push('}');
            resp
        }
        "TA_TEMA" => {
            let startIdx = params["startIdx"].as_u64().unwrap_or(0) as usize;
            let endIdx = params["endIdx"].as_u64().unwrap_or(0) as usize;
            let use_preloaded = params["use_preloaded"].as_i64().unwrap_or(0);
            let bench_iters = std::cmp::max(1, params["iters"].as_i64().unwrap_or(1)) as u64;
            let mut _json_inReal: Vec<f64> = Vec::new();
            let inReal: &[f64];
            if use_preloaded != 0 && ref_data.n > 0 {
                inReal = &ref_data.close[..ref_data.n];
            } else {
                _json_inReal = parse_f64_array(&params["inReal"]);
                inReal = &_json_inReal;
            }
            let optInTimePeriod = params["optInTimePeriod"].as_i64().unwrap_or(30) as i32;
            let out_size = if endIdx >= startIdx { endIdx - startIdx + 1 } else { 0 };
            let mut outBuf0: Vec<f64> = vec![0.0f64; out_size];
            let mut outBegIdx: usize = 0;
            let mut outNBElement: usize = 0;
            let mut rc = RetCode::Success;
            let start_time = Instant::now();
            for _bi in 0..bench_iters {
            rc = core.tema(
                startIdx, endIdx,
                &inReal,
                optInTimePeriod,
                &mut outBegIdx, &mut outNBElement, &mut outBuf0,
            );
            }
            let elapsed_ns = start_time.elapsed().as_nanos() as u64 / bench_iters as u64;
            let start_time_ung = Instant::now();
            for _biu in 0..bench_iters {
            rc = core.tema_unguarded(
                startIdx, endIdx,
                &inReal,
                optInTimePeriod,
                &mut outBegIdx, &mut outNBElement, &mut outBuf0,
            );
            }
            let elapsed_ns_ung = start_time_ung.elapsed().as_nanos() as u64 / bench_iters as u64;
            let lookback = core.tema_lookback(optInTimePeriod);
            let mut resp = format!("{{\"retCode\":{},\"outBegIdx\":{},\"outNBElement\":{},\"lookback\":{},\"timing_ns\":{},\"timing_ns_unguarded\":{}", retcode_to_int(rc), outBegIdx, outNBElement, lookback, elapsed_ns, elapsed_ns_ung);
            resp.push_str(",\"outReal\":"); resp.push_str(&json_f64_array(&outBuf0[..outNBElement]));
            resp.push('}');
            resp
        }
        "TA_TRANGE" => {
            let startIdx = params["startIdx"].as_u64().unwrap_or(0) as usize;
            let endIdx = params["endIdx"].as_u64().unwrap_or(0) as usize;
            let use_preloaded = params["use_preloaded"].as_i64().unwrap_or(0);
            let bench_iters = std::cmp::max(1, params["iters"].as_i64().unwrap_or(1)) as u64;
            let mut _json_inHigh: Vec<f64> = Vec::new();
            let mut _json_inLow: Vec<f64> = Vec::new();
            let mut _json_inClose: Vec<f64> = Vec::new();
            let inHigh: &[f64];
            let inLow: &[f64];
            let inClose: &[f64];
            if use_preloaded != 0 && ref_data.n > 0 {
                inHigh = &ref_data.high[..ref_data.n];
                inLow = &ref_data.low[..ref_data.n];
                inClose = &ref_data.close[..ref_data.n];
            } else {
                _json_inHigh = parse_f64_array(&params["inHigh"]);
                inHigh = &_json_inHigh;
                _json_inLow = parse_f64_array(&params["inLow"]);
                inLow = &_json_inLow;
                _json_inClose = parse_f64_array(&params["inClose"]);
                inClose = &_json_inClose;
            }
            let out_size = if endIdx >= startIdx { endIdx - startIdx + 1 } else { 0 };
            let mut outBuf0: Vec<f64> = vec![0.0f64; out_size];
            let mut outBegIdx: usize = 0;
            let mut outNBElement: usize = 0;
            let mut rc = RetCode::Success;
            let start_time = Instant::now();
            for _bi in 0..bench_iters {
            rc = core.trange(
                startIdx, endIdx,
                &inHigh,
                &inLow,
                &inClose,
                &mut outBegIdx, &mut outNBElement, &mut outBuf0,
            );
            }
            let elapsed_ns = start_time.elapsed().as_nanos() as u64 / bench_iters as u64;
            let start_time_ung = Instant::now();
            for _biu in 0..bench_iters {
            rc = core.trange_unguarded(
                startIdx, endIdx,
                &inHigh,
                &inLow,
                &inClose,
                &mut outBegIdx, &mut outNBElement, &mut outBuf0,
            );
            }
            let elapsed_ns_ung = start_time_ung.elapsed().as_nanos() as u64 / bench_iters as u64;
            let lookback = core.trange_lookback();
            let mut resp = format!("{{\"retCode\":{},\"outBegIdx\":{},\"outNBElement\":{},\"lookback\":{},\"timing_ns\":{},\"timing_ns_unguarded\":{}", retcode_to_int(rc), outBegIdx, outNBElement, lookback, elapsed_ns, elapsed_ns_ung);
            resp.push_str(",\"outReal\":"); resp.push_str(&json_f64_array(&outBuf0[..outNBElement]));
            resp.push('}');
            resp
        }
        "TA_TRIMA" => {
            let startIdx = params["startIdx"].as_u64().unwrap_or(0) as usize;
            let endIdx = params["endIdx"].as_u64().unwrap_or(0) as usize;
            let use_preloaded = params["use_preloaded"].as_i64().unwrap_or(0);
            let bench_iters = std::cmp::max(1, params["iters"].as_i64().unwrap_or(1)) as u64;
            let mut _json_inReal: Vec<f64> = Vec::new();
            let inReal: &[f64];
            if use_preloaded != 0 && ref_data.n > 0 {
                inReal = &ref_data.close[..ref_data.n];
            } else {
                _json_inReal = parse_f64_array(&params["inReal"]);
                inReal = &_json_inReal;
            }
            let optInTimePeriod = params["optInTimePeriod"].as_i64().unwrap_or(30) as i32;
            let out_size = if endIdx >= startIdx { endIdx - startIdx + 1 } else { 0 };
            let mut outBuf0: Vec<f64> = vec![0.0f64; out_size];
            let mut outBegIdx: usize = 0;
            let mut outNBElement: usize = 0;
            let mut rc = RetCode::Success;
            let start_time = Instant::now();
            for _bi in 0..bench_iters {
            rc = core.trima(
                startIdx, endIdx,
                &inReal,
                optInTimePeriod,
                &mut outBegIdx, &mut outNBElement, &mut outBuf0,
            );
            }
            let elapsed_ns = start_time.elapsed().as_nanos() as u64 / bench_iters as u64;
            let start_time_ung = Instant::now();
            for _biu in 0..bench_iters {
            rc = core.trima_unguarded(
                startIdx, endIdx,
                &inReal,
                optInTimePeriod,
                &mut outBegIdx, &mut outNBElement, &mut outBuf0,
            );
            }
            let elapsed_ns_ung = start_time_ung.elapsed().as_nanos() as u64 / bench_iters as u64;
            let lookback = core.trima_lookback(optInTimePeriod);
            let mut resp = format!("{{\"retCode\":{},\"outBegIdx\":{},\"outNBElement\":{},\"lookback\":{},\"timing_ns\":{},\"timing_ns_unguarded\":{}", retcode_to_int(rc), outBegIdx, outNBElement, lookback, elapsed_ns, elapsed_ns_ung);
            resp.push_str(",\"outReal\":"); resp.push_str(&json_f64_array(&outBuf0[..outNBElement]));
            resp.push('}');
            resp
        }
        "TA_TRIX" => {
            let startIdx = params["startIdx"].as_u64().unwrap_or(0) as usize;
            let endIdx = params["endIdx"].as_u64().unwrap_or(0) as usize;
            let use_preloaded = params["use_preloaded"].as_i64().unwrap_or(0);
            let bench_iters = std::cmp::max(1, params["iters"].as_i64().unwrap_or(1)) as u64;
            let mut _json_inReal: Vec<f64> = Vec::new();
            let inReal: &[f64];
            if use_preloaded != 0 && ref_data.n > 0 {
                inReal = &ref_data.close[..ref_data.n];
            } else {
                _json_inReal = parse_f64_array(&params["inReal"]);
                inReal = &_json_inReal;
            }
            let optInTimePeriod = params["optInTimePeriod"].as_i64().unwrap_or(30) as i32;
            let out_size = if endIdx >= startIdx { endIdx - startIdx + 1 } else { 0 };
            let mut outBuf0: Vec<f64> = vec![0.0f64; out_size];
            let mut outBegIdx: usize = 0;
            let mut outNBElement: usize = 0;
            let mut rc = RetCode::Success;
            let start_time = Instant::now();
            for _bi in 0..bench_iters {
            rc = core.trix(
                startIdx, endIdx,
                &inReal,
                optInTimePeriod,
                &mut outBegIdx, &mut outNBElement, &mut outBuf0,
            );
            }
            let elapsed_ns = start_time.elapsed().as_nanos() as u64 / bench_iters as u64;
            let start_time_ung = Instant::now();
            for _biu in 0..bench_iters {
            rc = core.trix_unguarded(
                startIdx, endIdx,
                &inReal,
                optInTimePeriod,
                &mut outBegIdx, &mut outNBElement, &mut outBuf0,
            );
            }
            let elapsed_ns_ung = start_time_ung.elapsed().as_nanos() as u64 / bench_iters as u64;
            let lookback = core.trix_lookback(optInTimePeriod);
            let mut resp = format!("{{\"retCode\":{},\"outBegIdx\":{},\"outNBElement\":{},\"lookback\":{},\"timing_ns\":{},\"timing_ns_unguarded\":{}", retcode_to_int(rc), outBegIdx, outNBElement, lookback, elapsed_ns, elapsed_ns_ung);
            resp.push_str(",\"outReal\":"); resp.push_str(&json_f64_array(&outBuf0[..outNBElement]));
            resp.push('}');
            resp
        }
        "TA_TSF" => {
            let startIdx = params["startIdx"].as_u64().unwrap_or(0) as usize;
            let endIdx = params["endIdx"].as_u64().unwrap_or(0) as usize;
            let use_preloaded = params["use_preloaded"].as_i64().unwrap_or(0);
            let bench_iters = std::cmp::max(1, params["iters"].as_i64().unwrap_or(1)) as u64;
            let mut _json_inReal: Vec<f64> = Vec::new();
            let inReal: &[f64];
            if use_preloaded != 0 && ref_data.n > 0 {
                inReal = &ref_data.close[..ref_data.n];
            } else {
                _json_inReal = parse_f64_array(&params["inReal"]);
                inReal = &_json_inReal;
            }
            let optInTimePeriod = params["optInTimePeriod"].as_i64().unwrap_or(14) as i32;
            let out_size = if endIdx >= startIdx { endIdx - startIdx + 1 } else { 0 };
            let mut outBuf0: Vec<f64> = vec![0.0f64; out_size];
            let mut outBegIdx: usize = 0;
            let mut outNBElement: usize = 0;
            let mut rc = RetCode::Success;
            let start_time = Instant::now();
            for _bi in 0..bench_iters {
            rc = core.tsf(
                startIdx, endIdx,
                &inReal,
                optInTimePeriod,
                &mut outBegIdx, &mut outNBElement, &mut outBuf0,
            );
            }
            let elapsed_ns = start_time.elapsed().as_nanos() as u64 / bench_iters as u64;
            let start_time_ung = Instant::now();
            for _biu in 0..bench_iters {
            rc = core.tsf_unguarded(
                startIdx, endIdx,
                &inReal,
                optInTimePeriod,
                &mut outBegIdx, &mut outNBElement, &mut outBuf0,
            );
            }
            let elapsed_ns_ung = start_time_ung.elapsed().as_nanos() as u64 / bench_iters as u64;
            let lookback = core.tsf_lookback(optInTimePeriod);
            let mut resp = format!("{{\"retCode\":{},\"outBegIdx\":{},\"outNBElement\":{},\"lookback\":{},\"timing_ns\":{},\"timing_ns_unguarded\":{}", retcode_to_int(rc), outBegIdx, outNBElement, lookback, elapsed_ns, elapsed_ns_ung);
            resp.push_str(",\"outReal\":"); resp.push_str(&json_f64_array(&outBuf0[..outNBElement]));
            resp.push('}');
            resp
        }
        "TA_TYPPRICE" => {
            let startIdx = params["startIdx"].as_u64().unwrap_or(0) as usize;
            let endIdx = params["endIdx"].as_u64().unwrap_or(0) as usize;
            let use_preloaded = params["use_preloaded"].as_i64().unwrap_or(0);
            let bench_iters = std::cmp::max(1, params["iters"].as_i64().unwrap_or(1)) as u64;
            let mut _json_inHigh: Vec<f64> = Vec::new();
            let mut _json_inLow: Vec<f64> = Vec::new();
            let mut _json_inClose: Vec<f64> = Vec::new();
            let inHigh: &[f64];
            let inLow: &[f64];
            let inClose: &[f64];
            if use_preloaded != 0 && ref_data.n > 0 {
                inHigh = &ref_data.high[..ref_data.n];
                inLow = &ref_data.low[..ref_data.n];
                inClose = &ref_data.close[..ref_data.n];
            } else {
                _json_inHigh = parse_f64_array(&params["inHigh"]);
                inHigh = &_json_inHigh;
                _json_inLow = parse_f64_array(&params["inLow"]);
                inLow = &_json_inLow;
                _json_inClose = parse_f64_array(&params["inClose"]);
                inClose = &_json_inClose;
            }
            let out_size = if endIdx >= startIdx { endIdx - startIdx + 1 } else { 0 };
            let mut outBuf0: Vec<f64> = vec![0.0f64; out_size];
            let mut outBegIdx: usize = 0;
            let mut outNBElement: usize = 0;
            let mut rc = RetCode::Success;
            let start_time = Instant::now();
            for _bi in 0..bench_iters {
            rc = core.typprice(
                startIdx, endIdx,
                &inHigh,
                &inLow,
                &inClose,
                &mut outBegIdx, &mut outNBElement, &mut outBuf0,
            );
            }
            let elapsed_ns = start_time.elapsed().as_nanos() as u64 / bench_iters as u64;
            let start_time_ung = Instant::now();
            for _biu in 0..bench_iters {
            rc = core.typprice_unguarded(
                startIdx, endIdx,
                &inHigh,
                &inLow,
                &inClose,
                &mut outBegIdx, &mut outNBElement, &mut outBuf0,
            );
            }
            let elapsed_ns_ung = start_time_ung.elapsed().as_nanos() as u64 / bench_iters as u64;
            let lookback = core.typprice_lookback();
            let mut resp = format!("{{\"retCode\":{},\"outBegIdx\":{},\"outNBElement\":{},\"lookback\":{},\"timing_ns\":{},\"timing_ns_unguarded\":{}", retcode_to_int(rc), outBegIdx, outNBElement, lookback, elapsed_ns, elapsed_ns_ung);
            resp.push_str(",\"outReal\":"); resp.push_str(&json_f64_array(&outBuf0[..outNBElement]));
            resp.push('}');
            resp
        }
        "TA_ULTOSC" => {
            let startIdx = params["startIdx"].as_u64().unwrap_or(0) as usize;
            let endIdx = params["endIdx"].as_u64().unwrap_or(0) as usize;
            let use_preloaded = params["use_preloaded"].as_i64().unwrap_or(0);
            let bench_iters = std::cmp::max(1, params["iters"].as_i64().unwrap_or(1)) as u64;
            let mut _json_inHigh: Vec<f64> = Vec::new();
            let mut _json_inLow: Vec<f64> = Vec::new();
            let mut _json_inClose: Vec<f64> = Vec::new();
            let inHigh: &[f64];
            let inLow: &[f64];
            let inClose: &[f64];
            if use_preloaded != 0 && ref_data.n > 0 {
                inHigh = &ref_data.high[..ref_data.n];
                inLow = &ref_data.low[..ref_data.n];
                inClose = &ref_data.close[..ref_data.n];
            } else {
                _json_inHigh = parse_f64_array(&params["inHigh"]);
                inHigh = &_json_inHigh;
                _json_inLow = parse_f64_array(&params["inLow"]);
                inLow = &_json_inLow;
                _json_inClose = parse_f64_array(&params["inClose"]);
                inClose = &_json_inClose;
            }
            let optInTimePeriod1 = params["optInTimePeriod1"].as_i64().unwrap_or(7) as i32;
            let optInTimePeriod2 = params["optInTimePeriod2"].as_i64().unwrap_or(14) as i32;
            let optInTimePeriod3 = params["optInTimePeriod3"].as_i64().unwrap_or(28) as i32;
            let out_size = if endIdx >= startIdx { endIdx - startIdx + 1 } else { 0 };
            let mut outBuf0: Vec<f64> = vec![0.0f64; out_size];
            let mut outBegIdx: usize = 0;
            let mut outNBElement: usize = 0;
            let mut rc = RetCode::Success;
            let start_time = Instant::now();
            for _bi in 0..bench_iters {
            rc = core.ultosc(
                startIdx, endIdx,
                &inHigh,
                &inLow,
                &inClose,
                optInTimePeriod1,
                optInTimePeriod2,
                optInTimePeriod3,
                &mut outBegIdx, &mut outNBElement, &mut outBuf0,
            );
            }
            let elapsed_ns = start_time.elapsed().as_nanos() as u64 / bench_iters as u64;
            let start_time_ung = Instant::now();
            for _biu in 0..bench_iters {
            rc = core.ultosc_unguarded(
                startIdx, endIdx,
                &inHigh,
                &inLow,
                &inClose,
                optInTimePeriod1,
                optInTimePeriod2,
                optInTimePeriod3,
                &mut outBegIdx, &mut outNBElement, &mut outBuf0,
            );
            }
            let elapsed_ns_ung = start_time_ung.elapsed().as_nanos() as u64 / bench_iters as u64;
            let lookback = core.ultosc_lookback(optInTimePeriod1, optInTimePeriod2, optInTimePeriod3);
            let mut resp = format!("{{\"retCode\":{},\"outBegIdx\":{},\"outNBElement\":{},\"lookback\":{},\"timing_ns\":{},\"timing_ns_unguarded\":{}", retcode_to_int(rc), outBegIdx, outNBElement, lookback, elapsed_ns, elapsed_ns_ung);
            resp.push_str(",\"outReal\":"); resp.push_str(&json_f64_array(&outBuf0[..outNBElement]));
            resp.push('}');
            resp
        }
        "TA_VAR" => {
            let startIdx = params["startIdx"].as_u64().unwrap_or(0) as usize;
            let endIdx = params["endIdx"].as_u64().unwrap_or(0) as usize;
            let use_preloaded = params["use_preloaded"].as_i64().unwrap_or(0);
            let bench_iters = std::cmp::max(1, params["iters"].as_i64().unwrap_or(1)) as u64;
            let mut _json_inReal: Vec<f64> = Vec::new();
            let inReal: &[f64];
            if use_preloaded != 0 && ref_data.n > 0 {
                inReal = &ref_data.close[..ref_data.n];
            } else {
                _json_inReal = parse_f64_array(&params["inReal"]);
                inReal = &_json_inReal;
            }
            let optInTimePeriod = params["optInTimePeriod"].as_i64().unwrap_or(5) as i32;
            let optInNbDev = params["optInNbDev"].as_f64().unwrap_or(1.0) as f64;
            let out_size = if endIdx >= startIdx { endIdx - startIdx + 1 } else { 0 };
            let mut outBuf0: Vec<f64> = vec![0.0f64; out_size];
            let mut outBegIdx: usize = 0;
            let mut outNBElement: usize = 0;
            let mut rc = RetCode::Success;
            let start_time = Instant::now();
            for _bi in 0..bench_iters {
            rc = core.var(
                startIdx, endIdx,
                &inReal,
                optInTimePeriod,
                optInNbDev,
                &mut outBegIdx, &mut outNBElement, &mut outBuf0,
            );
            }
            let elapsed_ns = start_time.elapsed().as_nanos() as u64 / bench_iters as u64;
            let start_time_ung = Instant::now();
            for _biu in 0..bench_iters {
            rc = core.var_unguarded(
                startIdx, endIdx,
                &inReal,
                optInTimePeriod,
                optInNbDev,
                &mut outBegIdx, &mut outNBElement, &mut outBuf0,
            );
            }
            let elapsed_ns_ung = start_time_ung.elapsed().as_nanos() as u64 / bench_iters as u64;
            let lookback = core.var_lookback(optInTimePeriod, optInNbDev);
            let mut resp = format!("{{\"retCode\":{},\"outBegIdx\":{},\"outNBElement\":{},\"lookback\":{},\"timing_ns\":{},\"timing_ns_unguarded\":{}", retcode_to_int(rc), outBegIdx, outNBElement, lookback, elapsed_ns, elapsed_ns_ung);
            resp.push_str(",\"outReal\":"); resp.push_str(&json_f64_array(&outBuf0[..outNBElement]));
            resp.push('}');
            resp
        }
        "TA_WCLPRICE" => {
            let startIdx = params["startIdx"].as_u64().unwrap_or(0) as usize;
            let endIdx = params["endIdx"].as_u64().unwrap_or(0) as usize;
            let use_preloaded = params["use_preloaded"].as_i64().unwrap_or(0);
            let bench_iters = std::cmp::max(1, params["iters"].as_i64().unwrap_or(1)) as u64;
            let mut _json_inHigh: Vec<f64> = Vec::new();
            let mut _json_inLow: Vec<f64> = Vec::new();
            let mut _json_inClose: Vec<f64> = Vec::new();
            let inHigh: &[f64];
            let inLow: &[f64];
            let inClose: &[f64];
            if use_preloaded != 0 && ref_data.n > 0 {
                inHigh = &ref_data.high[..ref_data.n];
                inLow = &ref_data.low[..ref_data.n];
                inClose = &ref_data.close[..ref_data.n];
            } else {
                _json_inHigh = parse_f64_array(&params["inHigh"]);
                inHigh = &_json_inHigh;
                _json_inLow = parse_f64_array(&params["inLow"]);
                inLow = &_json_inLow;
                _json_inClose = parse_f64_array(&params["inClose"]);
                inClose = &_json_inClose;
            }
            let out_size = if endIdx >= startIdx { endIdx - startIdx + 1 } else { 0 };
            let mut outBuf0: Vec<f64> = vec![0.0f64; out_size];
            let mut outBegIdx: usize = 0;
            let mut outNBElement: usize = 0;
            let mut rc = RetCode::Success;
            let start_time = Instant::now();
            for _bi in 0..bench_iters {
            rc = core.wclprice(
                startIdx, endIdx,
                &inHigh,
                &inLow,
                &inClose,
                &mut outBegIdx, &mut outNBElement, &mut outBuf0,
            );
            }
            let elapsed_ns = start_time.elapsed().as_nanos() as u64 / bench_iters as u64;
            let start_time_ung = Instant::now();
            for _biu in 0..bench_iters {
            rc = core.wclprice_unguarded(
                startIdx, endIdx,
                &inHigh,
                &inLow,
                &inClose,
                &mut outBegIdx, &mut outNBElement, &mut outBuf0,
            );
            }
            let elapsed_ns_ung = start_time_ung.elapsed().as_nanos() as u64 / bench_iters as u64;
            let lookback = core.wclprice_lookback();
            let mut resp = format!("{{\"retCode\":{},\"outBegIdx\":{},\"outNBElement\":{},\"lookback\":{},\"timing_ns\":{},\"timing_ns_unguarded\":{}", retcode_to_int(rc), outBegIdx, outNBElement, lookback, elapsed_ns, elapsed_ns_ung);
            resp.push_str(",\"outReal\":"); resp.push_str(&json_f64_array(&outBuf0[..outNBElement]));
            resp.push('}');
            resp
        }
        "TA_WILLR" => {
            let startIdx = params["startIdx"].as_u64().unwrap_or(0) as usize;
            let endIdx = params["endIdx"].as_u64().unwrap_or(0) as usize;
            let use_preloaded = params["use_preloaded"].as_i64().unwrap_or(0);
            let bench_iters = std::cmp::max(1, params["iters"].as_i64().unwrap_or(1)) as u64;
            let mut _json_inHigh: Vec<f64> = Vec::new();
            let mut _json_inLow: Vec<f64> = Vec::new();
            let mut _json_inClose: Vec<f64> = Vec::new();
            let inHigh: &[f64];
            let inLow: &[f64];
            let inClose: &[f64];
            if use_preloaded != 0 && ref_data.n > 0 {
                inHigh = &ref_data.high[..ref_data.n];
                inLow = &ref_data.low[..ref_data.n];
                inClose = &ref_data.close[..ref_data.n];
            } else {
                _json_inHigh = parse_f64_array(&params["inHigh"]);
                inHigh = &_json_inHigh;
                _json_inLow = parse_f64_array(&params["inLow"]);
                inLow = &_json_inLow;
                _json_inClose = parse_f64_array(&params["inClose"]);
                inClose = &_json_inClose;
            }
            let optInTimePeriod = params["optInTimePeriod"].as_i64().unwrap_or(14) as i32;
            let out_size = if endIdx >= startIdx { endIdx - startIdx + 1 } else { 0 };
            let mut outBuf0: Vec<f64> = vec![0.0f64; out_size];
            let mut outBegIdx: usize = 0;
            let mut outNBElement: usize = 0;
            let mut rc = RetCode::Success;
            let start_time = Instant::now();
            for _bi in 0..bench_iters {
            rc = core.willr(
                startIdx, endIdx,
                &inHigh,
                &inLow,
                &inClose,
                optInTimePeriod,
                &mut outBegIdx, &mut outNBElement, &mut outBuf0,
            );
            }
            let elapsed_ns = start_time.elapsed().as_nanos() as u64 / bench_iters as u64;
            let start_time_ung = Instant::now();
            for _biu in 0..bench_iters {
            rc = core.willr_unguarded(
                startIdx, endIdx,
                &inHigh,
                &inLow,
                &inClose,
                optInTimePeriod,
                &mut outBegIdx, &mut outNBElement, &mut outBuf0,
            );
            }
            let elapsed_ns_ung = start_time_ung.elapsed().as_nanos() as u64 / bench_iters as u64;
            let lookback = core.willr_lookback(optInTimePeriod);
            let mut resp = format!("{{\"retCode\":{},\"outBegIdx\":{},\"outNBElement\":{},\"lookback\":{},\"timing_ns\":{},\"timing_ns_unguarded\":{}", retcode_to_int(rc), outBegIdx, outNBElement, lookback, elapsed_ns, elapsed_ns_ung);
            resp.push_str(",\"outReal\":"); resp.push_str(&json_f64_array(&outBuf0[..outNBElement]));
            resp.push('}');
            resp
        }
        "TA_WMA" => {
            let startIdx = params["startIdx"].as_u64().unwrap_or(0) as usize;
            let endIdx = params["endIdx"].as_u64().unwrap_or(0) as usize;
            let use_preloaded = params["use_preloaded"].as_i64().unwrap_or(0);
            let bench_iters = std::cmp::max(1, params["iters"].as_i64().unwrap_or(1)) as u64;
            let mut _json_inReal: Vec<f64> = Vec::new();
            let inReal: &[f64];
            if use_preloaded != 0 && ref_data.n > 0 {
                inReal = &ref_data.close[..ref_data.n];
            } else {
                _json_inReal = parse_f64_array(&params["inReal"]);
                inReal = &_json_inReal;
            }
            let optInTimePeriod = params["optInTimePeriod"].as_i64().unwrap_or(30) as i32;
            let out_size = if endIdx >= startIdx { endIdx - startIdx + 1 } else { 0 };
            let mut outBuf0: Vec<f64> = vec![0.0f64; out_size];
            let mut outBegIdx: usize = 0;
            let mut outNBElement: usize = 0;
            let mut rc = RetCode::Success;
            let start_time = Instant::now();
            for _bi in 0..bench_iters {
            rc = core.wma(
                startIdx, endIdx,
                &inReal,
                optInTimePeriod,
                &mut outBegIdx, &mut outNBElement, &mut outBuf0,
            );
            }
            let elapsed_ns = start_time.elapsed().as_nanos() as u64 / bench_iters as u64;
            let start_time_ung = Instant::now();
            for _biu in 0..bench_iters {
            rc = core.wma_unguarded(
                startIdx, endIdx,
                &inReal,
                optInTimePeriod,
                &mut outBegIdx, &mut outNBElement, &mut outBuf0,
            );
            }
            let elapsed_ns_ung = start_time_ung.elapsed().as_nanos() as u64 / bench_iters as u64;
            let lookback = core.wma_lookback(optInTimePeriod);
            let mut resp = format!("{{\"retCode\":{},\"outBegIdx\":{},\"outNBElement\":{},\"lookback\":{},\"timing_ns\":{},\"timing_ns_unguarded\":{}", retcode_to_int(rc), outBegIdx, outNBElement, lookback, elapsed_ns, elapsed_ns_ung);
            resp.push_str(",\"outReal\":"); resp.push_str(&json_f64_array(&outBuf0[..outNBElement]));
            resp.push('}');
            resp
        }
        "list_functions" => {
            let funcs: Vec<&str> = vec![
                "TA_ACCBANDS",
                "TA_ACOS",
                "TA_AD",
                "TA_ADD",
                "TA_ADOSC",
                "TA_ADX",
                "TA_ADXR",
                "TA_APO",
                "TA_AROON",
                "TA_AROONOSC",
                "TA_ASIN",
                "TA_ATAN",
                "TA_ATR",
                "TA_AVGDEV",
                "TA_AVGPRICE",
                "TA_BBANDS",
                "TA_BETA",
                "TA_BOP",
                "TA_CCI",
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
                "TA_CEIL",
                "TA_CMO",
                "TA_CORREL",
                "TA_COS",
                "TA_COSH",
                "TA_DEMA",
                "TA_DIV",
                "TA_DX",
                "TA_EMA",
                "TA_EXP",
                "TA_FLOOR",
                "TA_HT_DCPERIOD",
                "TA_HT_DCPHASE",
                "TA_HT_PHASOR",
                "TA_HT_SINE",
                "TA_HT_TRENDLINE",
                "TA_HT_TRENDMODE",
                "TA_IMI",
                "TA_KAMA",
                "TA_LINEARREG",
                "TA_LINEARREG_ANGLE",
                "TA_LINEARREG_INTERCEPT",
                "TA_LINEARREG_SLOPE",
                "TA_LN",
                "TA_LOG10",
                "TA_MA",
                "TA_MACD",
                "TA_MACDEXT",
                "TA_MACDFIX",
                "TA_MAMA",
                "TA_MAVP",
                "TA_MAX",
                "TA_MAXINDEX",
                "TA_MEDPRICE",
                "TA_MFI",
                "TA_MIDPOINT",
                "TA_MIDPRICE",
                "TA_MIN",
                "TA_MININDEX",
                "TA_MINMAX",
                "TA_MINMAXINDEX",
                "TA_MINUS_DI",
                "TA_MINUS_DM",
                "TA_MOM",
                "TA_MULT",
                "TA_NATR",
                "TA_OBV",
                "TA_PLUS_DI",
                "TA_PLUS_DM",
                "TA_PPO",
                "TA_ROC",
                "TA_ROCP",
                "TA_ROCR",
                "TA_ROCR100",
                "TA_RSI",
                "TA_SAR",
                "TA_SAREXT",
                "TA_SIN",
                "TA_SINH",
                "TA_SMA",
                "TA_SQRT",
                "TA_STDDEV",
                "TA_STOCH",
                "TA_STOCHF",
                "TA_STOCHRSI",
                "TA_SUB",
                "TA_SUM",
                "TA_T3",
                "TA_TAN",
                "TA_TANH",
                "TA_TEMA",
                "TA_TRANGE",
                "TA_TRIMA",
                "TA_TRIX",
                "TA_TSF",
                "TA_TYPPRICE",
                "TA_ULTOSC",
                "TA_VAR",
                "TA_WCLPRICE",
                "TA_WILLR",
                "TA_WMA",
            ];
            serde_json::json!({ "functions": funcs }).to_string()
        }
        "set_unstable_period" => {
            let id = params["id"].as_u64().unwrap_or(99) as usize;
            let period = params["period"].as_i64().unwrap_or(0) as i32;
            if id == FuncUnstId::FuncUnstAll as usize {
                for i in 0..(FuncUnstId::FuncUnstAll as usize) { core.unstable_period[i] = period; }
                "{\"status\":\"ok\"}".to_string()
            } else if id < FuncUnstId::FuncUnstAll as usize {
                core.unstable_period[id] = period;
                "{\"status\":\"ok\"}".to_string()
            } else {
                "{\"error\":\"Invalid unstable period id\"}".to_string()
            }
        }
        "set_compatibility" => {
            let mode = params["mode"].as_u64().unwrap_or(0);
            core.compatibility = match mode {
                1 => Compatibility::Metastock,
                _ => Compatibility::Default,
            };
            "{\"status\":\"ok\"}".to_string()
        }
        "TA_GetFuncInfo" => {
            let name = params["funcName"].as_str().unwrap_or("");
            match abstract_api::get_func_handle(name) {
                Some(id) => {
                    let fi = id.info();
                    serde_json::json!({
                        "name": fi.name,
                        "group": fi.group.as_str(),
                        "hint": fi.hint,
                        "camelCaseName": fi.camel_case_name,
                        "flags": fi.flags.bits(),
                        "nbInput": fi.nb_input(),
                        "nbOptInput": fi.nb_opt_input(),
                        "nbOutput": fi.nb_output(),
                    }).to_string()
                }
                None => "{\"retCode\":2}".to_string(),
            }
        }
        "TA_GetInputParameterInfo" => {
            let name = params["funcName"].as_str().unwrap_or("");
            let idx = params["paramIndex"].as_u64().unwrap_or(0) as usize;
            match abstract_api::get_func_handle(name)
                .and_then(|id| abstract_api::get_input_parameter_info(id, idx)) {
                Some(ii) => {
                    let ty = match ii.kind {
                        InputType::Price => 0,
                        InputType::Real => 1,
                        InputType::Integer => 2,
                    };
                    serde_json::json!({
                        "type": ty,
                        "paramName": ii.param_name,
                        "flags": ii.flags.bits(),
                    }).to_string()
                }
                None => "{\"retCode\":2}".to_string(),
            }
        }
        "TA_GetOptInputParameterInfo" => {
            let name = params["funcName"].as_str().unwrap_or("");
            let idx = params["paramIndex"].as_u64().unwrap_or(0) as usize;
            match abstract_api::get_func_handle(name)
                .and_then(|id| abstract_api::get_opt_input_parameter_info(id, idx)) {
                Some(oi) => {
                    let (ty, default): (i32, f64) = match oi.domain {
                        OptDomain::RealRange { default, .. } => (0, default),
                        OptDomain::RealList { default, .. } => (1, default),
                        OptDomain::IntegerRange { default, .. } => (2, default as f64),
                        OptDomain::IntegerList { default, .. } => (3, default as f64),
                    };
                    let mut resp = serde_json::json!({
                        "type": ty,
                        "paramName": oi.param_name,
                        "flags": oi.flags.bits(),
                        "displayName": oi.display_name,
                        "defaultValue": default,
                    });
                    match oi.domain {
                        OptDomain::RealRange { min, max, precision, suggested, .. } => {
                            resp["min"] = serde_json::json!(min);
                            resp["max"] = serde_json::json!(max);
                            resp["precision"] = serde_json::json!(precision);
                            resp["suggestedStart"] = serde_json::json!(suggested.0);
                            resp["suggestedEnd"] = serde_json::json!(suggested.1);
                            resp["suggestedIncrement"] = serde_json::json!(suggested.2);
                        }
                        OptDomain::IntegerRange { min, max, suggested, .. } => {
                            resp["min"] = serde_json::json!(min);
                            resp["max"] = serde_json::json!(max);
                            resp["suggestedStart"] = serde_json::json!(suggested.0);
                            resp["suggestedEnd"] = serde_json::json!(suggested.1);
                            resp["suggestedIncrement"] = serde_json::json!(suggested.2);
                        }
                        OptDomain::IntegerList { values, .. } => {
                            let mut vl = String::new();
                            for (i, (v, label)) in values.iter().enumerate() {
                                if i > 0 { vl.push(';'); }
                                vl.push_str(&format!("{}={}", v, label));
                            }
                            resp["valueList"] = serde_json::json!(vl);
                        }
                        OptDomain::RealList { .. } => {}
                    }
                    resp.to_string()
                }
                None => "{\"retCode\":2}".to_string(),
            }
        }
        "TA_GetOutputParameterInfo" => {
            let name = params["funcName"].as_str().unwrap_or("");
            let idx = params["paramIndex"].as_u64().unwrap_or(0) as usize;
            match abstract_api::get_func_handle(name)
                .and_then(|id| abstract_api::get_output_parameter_info(id, idx)) {
                Some(oo) => {
                    let ty = match oo.kind {
                        OutputType::Real => 0,
                        OutputType::Integer => 1,
                    };
                    serde_json::json!({
                        "type": ty,
                        "paramName": oo.param_name,
                        "flags": oo.flags.bits(),
                    }).to_string()
                }
                None => "{\"retCode\":2}".to_string(),
            }
        }
        "abstract_call" => {
            let fname = params["funcName"].as_str().unwrap_or("");
            if fname.is_empty() {
                return "{\"error\":\"Missing funcName\"}".to_string();
            }
            let rerouted = format!("TA_{}", fname);
            dispatch(core, ref_data, &rerouted, params)
        }
        "abstract_get_lookback" => {
            let fname = params["funcName"].as_str().unwrap_or("");
            match abstract_lookback(core, fname, params) {
                Some(lb) => format!("{{\"lookback\":{}}}", lb),
                None => format!("{{\"error\":\"Unknown function: {}\"}}", fname),
            }
        }
        "abstract_for_each_func" => {
            let mut arr: Vec<Value> = Vec::new();
            abstract_api::for_each_func(|fi| {
                arr.push(serde_json::json!({
                    "name": fi.name,
                    "group": fi.group.as_str(),
                    "nbInput": fi.nb_input(),
                    "nbOptInput": fi.nb_opt_input(),
                    "nbOutput": fi.nb_output(),
                }));
            });
            serde_json::json!({ "functions": arr }).to_string()
        }
        "TA_FunctionDescriptionXML" => {
            let xml = abstract_api::function_description_xml();
            let length = xml.len();
            let checksum: u64 = xml.bytes().map(|b| u64::from(b)).sum();
            format!("{{\"length\":{},\"checksum\":{}}}", length, checksum)
        }
        _ => {
            format!("{{\"error\":\"Unknown method: {}\"}}", method)
        }
    }
}

fn abstract_lookback(core: &Core, func_name: &str, params: &Value) -> Option<usize> {
    match func_name {
        "ACCBANDS" => {
            let optInTimePeriod = params["optInTimePeriod"].as_i64().unwrap_or(20) as i32;
            Some(core.accbands_lookback(optInTimePeriod))
        }
        "ACOS" => {
            Some(core.acos_lookback())
        }
        "AD" => {
            Some(core.ad_lookback())
        }
        "ADD" => {
            Some(core.add_lookback())
        }
        "ADOSC" => {
            let optInFastPeriod = params["optInFastPeriod"].as_i64().unwrap_or(3) as i32;
            let optInSlowPeriod = params["optInSlowPeriod"].as_i64().unwrap_or(10) as i32;
            Some(core.adosc_lookback(optInFastPeriod, optInSlowPeriod))
        }
        "ADX" => {
            let optInTimePeriod = params["optInTimePeriod"].as_i64().unwrap_or(14) as i32;
            Some(core.adx_lookback(optInTimePeriod))
        }
        "ADXR" => {
            let optInTimePeriod = params["optInTimePeriod"].as_i64().unwrap_or(14) as i32;
            Some(core.adxr_lookback(optInTimePeriod))
        }
        "APO" => {
            let optInFastPeriod = params["optInFastPeriod"].as_i64().unwrap_or(12) as i32;
            let optInSlowPeriod = params["optInSlowPeriod"].as_i64().unwrap_or(26) as i32;
            let optInMAType = params["optInMAType"].as_i64().unwrap_or(0) as i32;
            Some(core.apo_lookback(optInFastPeriod, optInSlowPeriod, optInMAType))
        }
        "AROON" => {
            let optInTimePeriod = params["optInTimePeriod"].as_i64().unwrap_or(14) as i32;
            Some(core.aroon_lookback(optInTimePeriod))
        }
        "AROONOSC" => {
            let optInTimePeriod = params["optInTimePeriod"].as_i64().unwrap_or(14) as i32;
            Some(core.aroonosc_lookback(optInTimePeriod))
        }
        "ASIN" => {
            Some(core.asin_lookback())
        }
        "ATAN" => {
            Some(core.atan_lookback())
        }
        "ATR" => {
            let optInTimePeriod = params["optInTimePeriod"].as_i64().unwrap_or(14) as i32;
            Some(core.atr_lookback(optInTimePeriod))
        }
        "AVGDEV" => {
            let optInTimePeriod = params["optInTimePeriod"].as_i64().unwrap_or(14) as i32;
            Some(core.avgdev_lookback(optInTimePeriod))
        }
        "AVGPRICE" => {
            Some(core.avgprice_lookback())
        }
        "BBANDS" => {
            let optInTimePeriod = params["optInTimePeriod"].as_i64().unwrap_or(5) as i32;
            let optInNbDevUp = params["optInNbDevUp"].as_f64().unwrap_or(2.0) as f64;
            let optInNbDevDn = params["optInNbDevDn"].as_f64().unwrap_or(2.0) as f64;
            let optInMAType = params["optInMAType"].as_i64().unwrap_or(0) as i32;
            Some(core.bbands_lookback(optInTimePeriod, optInNbDevUp, optInNbDevDn, optInMAType))
        }
        "BETA" => {
            let optInTimePeriod = params["optInTimePeriod"].as_i64().unwrap_or(5) as i32;
            Some(core.beta_lookback(optInTimePeriod))
        }
        "BOP" => {
            Some(core.bop_lookback())
        }
        "CCI" => {
            let optInTimePeriod = params["optInTimePeriod"].as_i64().unwrap_or(14) as i32;
            Some(core.cci_lookback(optInTimePeriod))
        }
        "CDL2CROWS" => {
            Some(core.cdl2crows_lookback())
        }
        "CDL3BLACKCROWS" => {
            Some(core.cdl3blackcrows_lookback())
        }
        "CDL3INSIDE" => {
            Some(core.cdl3inside_lookback())
        }
        "CDL3LINESTRIKE" => {
            Some(core.cdl3linestrike_lookback())
        }
        "CDL3OUTSIDE" => {
            Some(core.cdl3outside_lookback())
        }
        "CDL3STARSINSOUTH" => {
            Some(core.cdl3starsinsouth_lookback())
        }
        "CDL3WHITESOLDIERS" => {
            Some(core.cdl3whitesoldiers_lookback())
        }
        "CDLABANDONEDBABY" => {
            let optInPenetration = params["optInPenetration"].as_f64().unwrap_or(0.3) as f64;
            Some(core.cdlabandonedbaby_lookback(optInPenetration))
        }
        "CDLADVANCEBLOCK" => {
            Some(core.cdladvanceblock_lookback())
        }
        "CDLBELTHOLD" => {
            Some(core.cdlbelthold_lookback())
        }
        "CDLBREAKAWAY" => {
            Some(core.cdlbreakaway_lookback())
        }
        "CDLCLOSINGMARUBOZU" => {
            Some(core.cdlclosingmarubozu_lookback())
        }
        "CDLCONCEALBABYSWALL" => {
            Some(core.cdlconcealbabyswall_lookback())
        }
        "CDLCOUNTERATTACK" => {
            Some(core.cdlcounterattack_lookback())
        }
        "CDLDARKCLOUDCOVER" => {
            let optInPenetration = params["optInPenetration"].as_f64().unwrap_or(0.5) as f64;
            Some(core.cdldarkcloudcover_lookback(optInPenetration))
        }
        "CDLDOJI" => {
            Some(core.cdldoji_lookback())
        }
        "CDLDOJISTAR" => {
            Some(core.cdldojistar_lookback())
        }
        "CDLDRAGONFLYDOJI" => {
            Some(core.cdldragonflydoji_lookback())
        }
        "CDLENGULFING" => {
            Some(core.cdlengulfing_lookback())
        }
        "CDLEVENINGDOJISTAR" => {
            let optInPenetration = params["optInPenetration"].as_f64().unwrap_or(0.3) as f64;
            Some(core.cdleveningdojistar_lookback(optInPenetration))
        }
        "CDLEVENINGSTAR" => {
            let optInPenetration = params["optInPenetration"].as_f64().unwrap_or(0.3) as f64;
            Some(core.cdleveningstar_lookback(optInPenetration))
        }
        "CDLGAPSIDESIDEWHITE" => {
            Some(core.cdlgapsidesidewhite_lookback())
        }
        "CDLGRAVESTONEDOJI" => {
            Some(core.cdlgravestonedoji_lookback())
        }
        "CDLHAMMER" => {
            Some(core.cdlhammer_lookback())
        }
        "CDLHANGINGMAN" => {
            Some(core.cdlhangingman_lookback())
        }
        "CDLHARAMI" => {
            Some(core.cdlharami_lookback())
        }
        "CDLHARAMICROSS" => {
            Some(core.cdlharamicross_lookback())
        }
        "CDLHIGHWAVE" => {
            Some(core.cdlhighwave_lookback())
        }
        "CDLHIKKAKE" => {
            Some(core.cdlhikkake_lookback())
        }
        "CDLHIKKAKEMOD" => {
            Some(core.cdlhikkakemod_lookback())
        }
        "CDLHOMINGPIGEON" => {
            Some(core.cdlhomingpigeon_lookback())
        }
        "CDLIDENTICAL3CROWS" => {
            Some(core.cdlidentical3crows_lookback())
        }
        "CDLINNECK" => {
            Some(core.cdlinneck_lookback())
        }
        "CDLINVERTEDHAMMER" => {
            Some(core.cdlinvertedhammer_lookback())
        }
        "CDLKICKING" => {
            Some(core.cdlkicking_lookback())
        }
        "CDLKICKINGBYLENGTH" => {
            Some(core.cdlkickingbylength_lookback())
        }
        "CDLLADDERBOTTOM" => {
            Some(core.cdlladderbottom_lookback())
        }
        "CDLLONGLEGGEDDOJI" => {
            Some(core.cdllongleggeddoji_lookback())
        }
        "CDLLONGLINE" => {
            Some(core.cdllongline_lookback())
        }
        "CDLMARUBOZU" => {
            Some(core.cdlmarubozu_lookback())
        }
        "CDLMATCHINGLOW" => {
            Some(core.cdlmatchinglow_lookback())
        }
        "CDLMATHOLD" => {
            let optInPenetration = params["optInPenetration"].as_f64().unwrap_or(0.5) as f64;
            Some(core.cdlmathold_lookback(optInPenetration))
        }
        "CDLMORNINGDOJISTAR" => {
            let optInPenetration = params["optInPenetration"].as_f64().unwrap_or(0.3) as f64;
            Some(core.cdlmorningdojistar_lookback(optInPenetration))
        }
        "CDLMORNINGSTAR" => {
            let optInPenetration = params["optInPenetration"].as_f64().unwrap_or(0.3) as f64;
            Some(core.cdlmorningstar_lookback(optInPenetration))
        }
        "CDLONNECK" => {
            Some(core.cdlonneck_lookback())
        }
        "CDLPIERCING" => {
            Some(core.cdlpiercing_lookback())
        }
        "CDLRICKSHAWMAN" => {
            Some(core.cdlrickshawman_lookback())
        }
        "CDLRISEFALL3METHODS" => {
            Some(core.cdlrisefall3methods_lookback())
        }
        "CDLSEPARATINGLINES" => {
            Some(core.cdlseparatinglines_lookback())
        }
        "CDLSHOOTINGSTAR" => {
            Some(core.cdlshootingstar_lookback())
        }
        "CDLSHORTLINE" => {
            Some(core.cdlshortline_lookback())
        }
        "CDLSPINNINGTOP" => {
            Some(core.cdlspinningtop_lookback())
        }
        "CDLSTALLEDPATTERN" => {
            Some(core.cdlstalledpattern_lookback())
        }
        "CDLSTICKSANDWICH" => {
            Some(core.cdlsticksandwich_lookback())
        }
        "CDLTAKURI" => {
            Some(core.cdltakuri_lookback())
        }
        "CDLTASUKIGAP" => {
            Some(core.cdltasukigap_lookback())
        }
        "CDLTHRUSTING" => {
            Some(core.cdlthrusting_lookback())
        }
        "CDLTRISTAR" => {
            Some(core.cdltristar_lookback())
        }
        "CDLUNIQUE3RIVER" => {
            Some(core.cdlunique3river_lookback())
        }
        "CDLUPSIDEGAP2CROWS" => {
            Some(core.cdlupsidegap2crows_lookback())
        }
        "CDLXSIDEGAP3METHODS" => {
            Some(core.cdlxsidegap3methods_lookback())
        }
        "CEIL" => {
            Some(core.ceil_lookback())
        }
        "CMO" => {
            let optInTimePeriod = params["optInTimePeriod"].as_i64().unwrap_or(14) as i32;
            Some(core.cmo_lookback(optInTimePeriod))
        }
        "CORREL" => {
            let optInTimePeriod = params["optInTimePeriod"].as_i64().unwrap_or(30) as i32;
            Some(core.correl_lookback(optInTimePeriod))
        }
        "COS" => {
            Some(core.cos_lookback())
        }
        "COSH" => {
            Some(core.cosh_lookback())
        }
        "DEMA" => {
            let optInTimePeriod = params["optInTimePeriod"].as_i64().unwrap_or(30) as i32;
            Some(core.dema_lookback(optInTimePeriod))
        }
        "DIV" => {
            Some(core.div_lookback())
        }
        "DX" => {
            let optInTimePeriod = params["optInTimePeriod"].as_i64().unwrap_or(14) as i32;
            Some(core.dx_lookback(optInTimePeriod))
        }
        "EMA" => {
            let optInTimePeriod = params["optInTimePeriod"].as_i64().unwrap_or(30) as i32;
            Some(core.ema_lookback(optInTimePeriod))
        }
        "EXP" => {
            Some(core.exp_lookback())
        }
        "FLOOR" => {
            Some(core.floor_lookback())
        }
        "HT_DCPERIOD" => {
            Some(core.ht_dcperiod_lookback())
        }
        "HT_DCPHASE" => {
            Some(core.ht_dcphase_lookback())
        }
        "HT_PHASOR" => {
            Some(core.ht_phasor_lookback())
        }
        "HT_SINE" => {
            Some(core.ht_sine_lookback())
        }
        "HT_TRENDLINE" => {
            Some(core.ht_trendline_lookback())
        }
        "HT_TRENDMODE" => {
            Some(core.ht_trendmode_lookback())
        }
        "IMI" => {
            let optInTimePeriod = params["optInTimePeriod"].as_i64().unwrap_or(14) as i32;
            Some(core.imi_lookback(optInTimePeriod))
        }
        "KAMA" => {
            let optInTimePeriod = params["optInTimePeriod"].as_i64().unwrap_or(30) as i32;
            Some(core.kama_lookback(optInTimePeriod))
        }
        "LINEARREG" => {
            let optInTimePeriod = params["optInTimePeriod"].as_i64().unwrap_or(14) as i32;
            Some(core.linearreg_lookback(optInTimePeriod))
        }
        "LINEARREG_ANGLE" => {
            let optInTimePeriod = params["optInTimePeriod"].as_i64().unwrap_or(14) as i32;
            Some(core.linearreg_angle_lookback(optInTimePeriod))
        }
        "LINEARREG_INTERCEPT" => {
            let optInTimePeriod = params["optInTimePeriod"].as_i64().unwrap_or(14) as i32;
            Some(core.linearreg_intercept_lookback(optInTimePeriod))
        }
        "LINEARREG_SLOPE" => {
            let optInTimePeriod = params["optInTimePeriod"].as_i64().unwrap_or(14) as i32;
            Some(core.linearreg_slope_lookback(optInTimePeriod))
        }
        "LN" => {
            Some(core.ln_lookback())
        }
        "LOG10" => {
            Some(core.log10_lookback())
        }
        "MA" => {
            let optInTimePeriod = params["optInTimePeriod"].as_i64().unwrap_or(30) as i32;
            let optInMAType = params["optInMAType"].as_i64().unwrap_or(0) as i32;
            Some(core.ma_lookback(optInTimePeriod, optInMAType))
        }
        "MACD" => {
            let optInFastPeriod = params["optInFastPeriod"].as_i64().unwrap_or(12) as i32;
            let optInSlowPeriod = params["optInSlowPeriod"].as_i64().unwrap_or(26) as i32;
            let optInSignalPeriod = params["optInSignalPeriod"].as_i64().unwrap_or(9) as i32;
            Some(core.macd_lookback(optInFastPeriod, optInSlowPeriod, optInSignalPeriod))
        }
        "MACDEXT" => {
            let optInFastPeriod = params["optInFastPeriod"].as_i64().unwrap_or(12) as i32;
            let optInFastMAType = params["optInFastMAType"].as_i64().unwrap_or(0) as i32;
            let optInSlowPeriod = params["optInSlowPeriod"].as_i64().unwrap_or(26) as i32;
            let optInSlowMAType = params["optInSlowMAType"].as_i64().unwrap_or(0) as i32;
            let optInSignalPeriod = params["optInSignalPeriod"].as_i64().unwrap_or(9) as i32;
            let optInSignalMAType = params["optInSignalMAType"].as_i64().unwrap_or(0) as i32;
            Some(core.macdext_lookback(optInFastPeriod, optInFastMAType, optInSlowPeriod, optInSlowMAType, optInSignalPeriod, optInSignalMAType))
        }
        "MACDFIX" => {
            let optInSignalPeriod = params["optInSignalPeriod"].as_i64().unwrap_or(9) as i32;
            Some(core.macdfix_lookback(optInSignalPeriod))
        }
        "MAMA" => {
            let optInFastLimit = params["optInFastLimit"].as_f64().unwrap_or(0.5) as f64;
            let optInSlowLimit = params["optInSlowLimit"].as_f64().unwrap_or(0.05) as f64;
            Some(core.mama_lookback(optInFastLimit, optInSlowLimit))
        }
        "MAVP" => {
            let optInMinPeriod = params["optInMinPeriod"].as_i64().unwrap_or(2) as i32;
            let optInMaxPeriod = params["optInMaxPeriod"].as_i64().unwrap_or(30) as i32;
            let optInMAType = params["optInMAType"].as_i64().unwrap_or(0) as i32;
            Some(core.mavp_lookback(optInMinPeriod, optInMaxPeriod, optInMAType))
        }
        "MAX" => {
            let optInTimePeriod = params["optInTimePeriod"].as_i64().unwrap_or(30) as i32;
            Some(core.max_lookback(optInTimePeriod))
        }
        "MAXINDEX" => {
            let optInTimePeriod = params["optInTimePeriod"].as_i64().unwrap_or(30) as i32;
            Some(core.maxindex_lookback(optInTimePeriod))
        }
        "MEDPRICE" => {
            Some(core.medprice_lookback())
        }
        "MFI" => {
            let optInTimePeriod = params["optInTimePeriod"].as_i64().unwrap_or(14) as i32;
            Some(core.mfi_lookback(optInTimePeriod))
        }
        "MIDPOINT" => {
            let optInTimePeriod = params["optInTimePeriod"].as_i64().unwrap_or(14) as i32;
            Some(core.midpoint_lookback(optInTimePeriod))
        }
        "MIDPRICE" => {
            let optInTimePeriod = params["optInTimePeriod"].as_i64().unwrap_or(14) as i32;
            Some(core.midprice_lookback(optInTimePeriod))
        }
        "MIN" => {
            let optInTimePeriod = params["optInTimePeriod"].as_i64().unwrap_or(30) as i32;
            Some(core.min_lookback(optInTimePeriod))
        }
        "MININDEX" => {
            let optInTimePeriod = params["optInTimePeriod"].as_i64().unwrap_or(30) as i32;
            Some(core.minindex_lookback(optInTimePeriod))
        }
        "MINMAX" => {
            let optInTimePeriod = params["optInTimePeriod"].as_i64().unwrap_or(30) as i32;
            Some(core.minmax_lookback(optInTimePeriod))
        }
        "MINMAXINDEX" => {
            let optInTimePeriod = params["optInTimePeriod"].as_i64().unwrap_or(30) as i32;
            Some(core.minmaxindex_lookback(optInTimePeriod))
        }
        "MINUS_DI" => {
            let optInTimePeriod = params["optInTimePeriod"].as_i64().unwrap_or(14) as i32;
            Some(core.minus_di_lookback(optInTimePeriod))
        }
        "MINUS_DM" => {
            let optInTimePeriod = params["optInTimePeriod"].as_i64().unwrap_or(14) as i32;
            Some(core.minus_dm_lookback(optInTimePeriod))
        }
        "MOM" => {
            let optInTimePeriod = params["optInTimePeriod"].as_i64().unwrap_or(10) as i32;
            Some(core.mom_lookback(optInTimePeriod))
        }
        "MULT" => {
            Some(core.mult_lookback())
        }
        "NATR" => {
            let optInTimePeriod = params["optInTimePeriod"].as_i64().unwrap_or(14) as i32;
            Some(core.natr_lookback(optInTimePeriod))
        }
        "OBV" => {
            Some(core.obv_lookback())
        }
        "PLUS_DI" => {
            let optInTimePeriod = params["optInTimePeriod"].as_i64().unwrap_or(14) as i32;
            Some(core.plus_di_lookback(optInTimePeriod))
        }
        "PLUS_DM" => {
            let optInTimePeriod = params["optInTimePeriod"].as_i64().unwrap_or(14) as i32;
            Some(core.plus_dm_lookback(optInTimePeriod))
        }
        "PPO" => {
            let optInFastPeriod = params["optInFastPeriod"].as_i64().unwrap_or(12) as i32;
            let optInSlowPeriod = params["optInSlowPeriod"].as_i64().unwrap_or(26) as i32;
            let optInMAType = params["optInMAType"].as_i64().unwrap_or(0) as i32;
            Some(core.ppo_lookback(optInFastPeriod, optInSlowPeriod, optInMAType))
        }
        "ROC" => {
            let optInTimePeriod = params["optInTimePeriod"].as_i64().unwrap_or(10) as i32;
            Some(core.roc_lookback(optInTimePeriod))
        }
        "ROCP" => {
            let optInTimePeriod = params["optInTimePeriod"].as_i64().unwrap_or(10) as i32;
            Some(core.rocp_lookback(optInTimePeriod))
        }
        "ROCR" => {
            let optInTimePeriod = params["optInTimePeriod"].as_i64().unwrap_or(10) as i32;
            Some(core.rocr_lookback(optInTimePeriod))
        }
        "ROCR100" => {
            let optInTimePeriod = params["optInTimePeriod"].as_i64().unwrap_or(10) as i32;
            Some(core.rocr100_lookback(optInTimePeriod))
        }
        "RSI" => {
            let optInTimePeriod = params["optInTimePeriod"].as_i64().unwrap_or(14) as i32;
            Some(core.rsi_lookback(optInTimePeriod))
        }
        "SAR" => {
            let optInAcceleration = params["optInAcceleration"].as_f64().unwrap_or(0.02) as f64;
            let optInMaximum = params["optInMaximum"].as_f64().unwrap_or(0.2) as f64;
            Some(core.sar_lookback(optInAcceleration, optInMaximum))
        }
        "SAREXT" => {
            let optInStartValue = params["optInStartValue"].as_f64().unwrap_or(0.0) as f64;
            let optInOffsetOnReverse = params["optInOffsetOnReverse"].as_f64().unwrap_or(0.0) as f64;
            let optInAccelerationInitLong = params["optInAccelerationInitLong"].as_f64().unwrap_or(0.02) as f64;
            let optInAccelerationLong = params["optInAccelerationLong"].as_f64().unwrap_or(0.02) as f64;
            let optInAccelerationMaxLong = params["optInAccelerationMaxLong"].as_f64().unwrap_or(0.2) as f64;
            let optInAccelerationInitShort = params["optInAccelerationInitShort"].as_f64().unwrap_or(0.02) as f64;
            let optInAccelerationShort = params["optInAccelerationShort"].as_f64().unwrap_or(0.02) as f64;
            let optInAccelerationMaxShort = params["optInAccelerationMaxShort"].as_f64().unwrap_or(0.2) as f64;
            Some(core.sarext_lookback(optInStartValue, optInOffsetOnReverse, optInAccelerationInitLong, optInAccelerationLong, optInAccelerationMaxLong, optInAccelerationInitShort, optInAccelerationShort, optInAccelerationMaxShort))
        }
        "SIN" => {
            Some(core.sin_lookback())
        }
        "SINH" => {
            Some(core.sinh_lookback())
        }
        "SMA" => {
            let optInTimePeriod = params["optInTimePeriod"].as_i64().unwrap_or(30) as i32;
            Some(core.sma_lookback(optInTimePeriod))
        }
        "SQRT" => {
            Some(core.sqrt_lookback())
        }
        "STDDEV" => {
            let optInTimePeriod = params["optInTimePeriod"].as_i64().unwrap_or(5) as i32;
            let optInNbDev = params["optInNbDev"].as_f64().unwrap_or(1.0) as f64;
            Some(core.stddev_lookback(optInTimePeriod, optInNbDev))
        }
        "STOCH" => {
            let optInFastK_Period = params["optInFastK_Period"].as_i64().unwrap_or(5) as i32;
            let optInSlowK_Period = params["optInSlowK_Period"].as_i64().unwrap_or(3) as i32;
            let optInSlowK_MAType = params["optInSlowK_MAType"].as_i64().unwrap_or(0) as i32;
            let optInSlowD_Period = params["optInSlowD_Period"].as_i64().unwrap_or(3) as i32;
            let optInSlowD_MAType = params["optInSlowD_MAType"].as_i64().unwrap_or(0) as i32;
            Some(core.stoch_lookback(optInFastK_Period, optInSlowK_Period, optInSlowK_MAType, optInSlowD_Period, optInSlowD_MAType))
        }
        "STOCHF" => {
            let optInFastK_Period = params["optInFastK_Period"].as_i64().unwrap_or(5) as i32;
            let optInFastD_Period = params["optInFastD_Period"].as_i64().unwrap_or(3) as i32;
            let optInFastD_MAType = params["optInFastD_MAType"].as_i64().unwrap_or(0) as i32;
            Some(core.stochf_lookback(optInFastK_Period, optInFastD_Period, optInFastD_MAType))
        }
        "STOCHRSI" => {
            let optInTimePeriod = params["optInTimePeriod"].as_i64().unwrap_or(14) as i32;
            let optInFastK_Period = params["optInFastK_Period"].as_i64().unwrap_or(5) as i32;
            let optInFastD_Period = params["optInFastD_Period"].as_i64().unwrap_or(3) as i32;
            let optInFastD_MAType = params["optInFastD_MAType"].as_i64().unwrap_or(0) as i32;
            Some(core.stochrsi_lookback(optInTimePeriod, optInFastK_Period, optInFastD_Period, optInFastD_MAType))
        }
        "SUB" => {
            Some(core.sub_lookback())
        }
        "SUM" => {
            let optInTimePeriod = params["optInTimePeriod"].as_i64().unwrap_or(30) as i32;
            Some(core.sum_lookback(optInTimePeriod))
        }
        "T3" => {
            let optInTimePeriod = params["optInTimePeriod"].as_i64().unwrap_or(5) as i32;
            let optInVFactor = params["optInVFactor"].as_f64().unwrap_or(0.7) as f64;
            Some(core.t3_lookback(optInTimePeriod, optInVFactor))
        }
        "TAN" => {
            Some(core.tan_lookback())
        }
        "TANH" => {
            Some(core.tanh_lookback())
        }
        "TEMA" => {
            let optInTimePeriod = params["optInTimePeriod"].as_i64().unwrap_or(30) as i32;
            Some(core.tema_lookback(optInTimePeriod))
        }
        "TRANGE" => {
            Some(core.trange_lookback())
        }
        "TRIMA" => {
            let optInTimePeriod = params["optInTimePeriod"].as_i64().unwrap_or(30) as i32;
            Some(core.trima_lookback(optInTimePeriod))
        }
        "TRIX" => {
            let optInTimePeriod = params["optInTimePeriod"].as_i64().unwrap_or(30) as i32;
            Some(core.trix_lookback(optInTimePeriod))
        }
        "TSF" => {
            let optInTimePeriod = params["optInTimePeriod"].as_i64().unwrap_or(14) as i32;
            Some(core.tsf_lookback(optInTimePeriod))
        }
        "TYPPRICE" => {
            Some(core.typprice_lookback())
        }
        "ULTOSC" => {
            let optInTimePeriod1 = params["optInTimePeriod1"].as_i64().unwrap_or(7) as i32;
            let optInTimePeriod2 = params["optInTimePeriod2"].as_i64().unwrap_or(14) as i32;
            let optInTimePeriod3 = params["optInTimePeriod3"].as_i64().unwrap_or(28) as i32;
            Some(core.ultosc_lookback(optInTimePeriod1, optInTimePeriod2, optInTimePeriod3))
        }
        "VAR" => {
            let optInTimePeriod = params["optInTimePeriod"].as_i64().unwrap_or(5) as i32;
            let optInNbDev = params["optInNbDev"].as_f64().unwrap_or(1.0) as f64;
            Some(core.var_lookback(optInTimePeriod, optInNbDev))
        }
        "WCLPRICE" => {
            Some(core.wclprice_lookback())
        }
        "WILLR" => {
            let optInTimePeriod = params["optInTimePeriod"].as_i64().unwrap_or(14) as i32;
            Some(core.willr_lookback(optInTimePeriod))
        }
        "WMA" => {
            let optInTimePeriod = params["optInTimePeriod"].as_i64().unwrap_or(30) as i32;
            Some(core.wma_lookback(optInTimePeriod))
        }
        _ => None,
    }
}

fn main() {
    let mut core = Core::new();
    let mut ref_data = RefData::new();
    let stdin = io::stdin();
    let stdout = io::stdout();
    let mut stdout = stdout.lock();
    for line in stdin.lock().lines() {
        let line = match line {
            Ok(l) => l,
            Err(_) => break,
        };
        let line = line.trim();
        if line.is_empty() {
            continue;
        }
        let resp = handle_request(&mut core, &mut ref_data, line);
        writeln!(stdout, "{}", resp).ok();
        stdout.flush().ok();
    }
}
