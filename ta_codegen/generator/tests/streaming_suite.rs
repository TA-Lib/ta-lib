//! Streaming-analysis integration tests over the REAL ta_codegen/input corpus.
//!
//! These pin the stage-1 tier boundary: which functions analyze as T1/T2,
//! and — just as load-bearing — which are rejected and why. A batch rewrite
//! that changes a function's stream shape fails here before it fails in CI.

use std::path::{Path, PathBuf};

use ta_codegen_lib::ir::{FuncDef, StreamTier};
use ta_codegen_lib::parser;
use ta_codegen_lib::streaming::{self, StreamError};

fn input_dir() -> PathBuf {
    Path::new(env!("CARGO_MANIFEST_DIR")).join("../input")
}

fn load(name: &str) -> FuncDef {
    let dir = input_dir().join(name);
    let mut func = parser::yaml::parse_yaml(&dir.join(format!("{name}.yaml")));
    let parsed = parser::c_source::parse_c_source(&dir.join(format!("{name}.c")));
    parser::c_source::wire_parsed_source(&mut func, &parsed);
    func
}

#[test]
fn mult_is_t1() {
    let f = load("mult");
    let m = streaming::analyze(&f).expect("MULT analyzes");
    assert_eq!(m.tier, StreamTier::T1);
    assert_eq!(m.bar_inputs, ["inReal0", "inReal1"]);
    assert!(m.state.is_empty() && m.lags.is_empty());
}

#[test]
fn ema_is_t2_scalar_recurrence() {
    let f = load("ema");
    let m = streaming::analyze(&f).expect("EMA analyzes");
    assert_eq!(m.tier, StreamTier::T2);
    assert!(m.lags.is_empty());
    assert!(m.state.iter().any(|(n, _)| n == "prevMA"));
}

#[test]
fn trange_is_t2_with_prev_close_lag() {
    let f = load("trange");
    let m = streaming::analyze(&f).expect("TRANGE analyzes");
    assert_eq!(m.tier, StreamTier::T2);
    assert_eq!(m.bar_inputs, ["inHigh", "inLow", "inClose"]);
    assert_eq!(m.lags.len(), 1);
    assert_eq!(m.lags[0].array, "inClose");
    assert_eq!(m.lags[0].depth, 1);
}

#[test]
fn macd_is_t2_multi_output() {
    let f = load("macd");
    let m = streaming::analyze(&f).expect("MACD analyzes");
    assert_eq!(m.tier, StreamTier::T2);
    assert_eq!(m.outputs.len(), 3);
}

#[test]
fn ad_countdown_loop_is_t2() {
    let f = load("ad");
    let m = streaming::analyze(&f).expect("AD analyzes");
    assert_eq!(m.tier, StreamTier::T2);
    assert!(m.counter.is_some());
}

#[test]
fn sma_is_t3_ring() {
    let f = load("sma");
    let m = streaming::analyze(&f).expect("SMA analyzes");
    assert_eq!(m.tier, StreamTier::T3);
    assert_eq!(m.rings.len(), 1);
    assert_eq!(m.rings[0].arrays, ["inReal"]);
}

#[test]
fn atr_period1_delegation_rejected() {
    assert!(matches!(
        streaming::analyze(&load("atr")),
        Err(StreamError::Unsupported(m)) if m.contains("return path")
    ));
}

#[test]
fn t3_identity_path_recognized() {
    let f = load("t3");
    let m = streaming::analyze(&f).expect("T3 analyzes with identity path");
    assert_eq!(m.tier, StreamTier::T2);
    let idp = m.identity.as_ref().expect("identity path");
    assert_eq!(idp.pairs, vec![("outReal".to_string(), "inReal".to_string())]);
}

#[test]
fn rsi_memmove_identity_rejected() {
    // RSI's period==1 path uses memmove (not a copy loop) and Metastock has a
    // mid-loop exit — both outside stage 1. Guard the boundary.
    let f = load("rsi");
    assert!(streaming::analyze(&f).is_err());
}

#[test]
fn bbands_composed_rejected() {
    assert!(streaming::analyze(&load("bbands")).is_err());
}

/// Whole-corpus gate: every `streaming: true` function must analyze clean.
/// (The same check `generate` enforces; here it runs in `cargo test`.)
#[test]
fn all_declared_functions_are_streamable() {
    let base = input_dir();
    let mut checked = 0;
    for entry in std::fs::read_dir(&base).expect("input dir") {
        let dir = entry.expect("entry").path();
        if !dir.is_dir() {
            continue;
        }
        let name = dir.file_name().unwrap().to_string_lossy().to_string();
        let yaml = dir.join(format!("{name}.yaml"));
        let c = dir.join(format!("{name}.c"));
        if !yaml.exists() || !c.exists() {
            continue;
        }
        let func = load(&name);
        if func.streaming {
            streaming::validate_streamable(&func).unwrap_or_else(|e| panic!("{e}"));
            checked += 1;
        }
    }
    assert!(checked >= 37, "expected 37+ declared functions, saw {checked}");
}
