//! Render pins for the Rust stream emitter (`backends/rust_stream.rs`) —
//! the Rust twin of backend_suite's `test_c_*_stream_section` family.
//!
//! Pins are substring/count/ordering assertions over the generated file (never
//! full snapshots), one per tier/mechanism. Every pin doubles as a neuter
//! check: the transition build panics on a cursor/startIdx leak, so a clean
//! render proves the analyzer normalizations fired.

use std::collections::HashMap;
use std::path::PathBuf;
use ta_codegen_lib::helper_registry::HelperRegistry;
use ta_codegen_lib::registry::Registry;
use ta_codegen_lib::{backends, ir, parser};

fn input_dir() -> PathBuf {
    PathBuf::from(env!("CARGO_MANIFEST_DIR")).join("../input")
}

fn load_indicator(name: &str) -> (ir::FuncDef, HashMap<String, ir::EnumDef>) {
    let dir = input_dir().join(name);
    let yaml = dir.join(format!("{name}.yaml"));
    let csrc = dir.join(format!("{name}.c"));
    let mut func = parser::yaml::parse_yaml(&yaml);
    let parsed = parser::c_source::parse_c_source(&csrc);
    parser::c_source::wire_parsed_source(&mut func, &parsed);
    let enums = parser::enums::load_enums(&input_dir().join("enums.yaml"));
    (func, enums)
}

fn rust_stream_section(name: &str) -> String {
    let (func, enums) = load_indicator(name);
    assert!(func.streaming, "{name}: yaml must carry the stream flag");
    let registry = Registry::from_dir(&input_dir());
    let helpers = HelperRegistry::empty();
    let full = backends::rust_lang::generate(&func, &enums, &registry, &helpers);
    let start = full
        .find("/**** Streaming API *****/")
        .unwrap_or_else(|| panic!("{name}: stream section missing"));
    full[start..].to_string()
}

// ---------------------------------------------------------------------------
// Loop tier
// ---------------------------------------------------------------------------

#[test]
fn test_rust_sma_ring_stream_section() {
    let s = rust_stream_section("sma");
    // Handle + state struct shapes.
    assert!(s.contains("pub struct SmaStream {"));
    assert!(s.contains("core: Core,"));
    assert!(s.contains("state: SmaStreamState,"));
    assert!(s.contains("struct SmaStreamState {"));
    assert!(s.contains("ring_trailingIdx_inReal: Vec<f64>,"));
    assert!(s.contains("ringPos_trailingIdx: usize,"));
    // The C mirror/peekMode machinery is deleted by design (clone-peek).
    assert!(!s.contains("Mirror"), "no peek mirrors in the Rust tier");
    assert!(!s.contains("peekMode"), "no peekMode in the Rust tier");
    assert!(!s.contains("unsafe"), "stream sections are safe Rust");
    // Step: ring read-old-then-push order, `(*outReal)` write.
    assert!(s.contains("fn sma_step_internal(&self, sp: &mut SmaStreamState, inReal: f64, outReal: &mut f64)"));
    assert!(s.contains("(*outReal) = sp.tempReal / (sp.optInTimePeriod as f64);"));
    assert!(s.contains("sp.ring_trailingIdx_inReal[sp.ringPos_trailingIdx] = inReal;"));
    // Open family: internal seam + thin wrapper + fill in batch param order.
    assert!(s.contains("pub(crate) fn sma_open_internal("));
    assert!(s.contains("self.sma_open_internal(inReal, 0, optInTimePeriod)"));
    assert!(s.contains("pub fn sma_open_and_fill(\n        &self, inReal: &[f64], mut optInTimePeriod: i32, outBegIdx: &mut usize, outNBElement: &mut usize, outReal: &mut [f64],"));
    // Capture: numeric ring cap from live locals + tail copy.
    assert!(s.contains("let cap_trailingIdx: i64 = (i as i64) - (trailingIdx as i64);"));
    assert!(s.contains(".copy_from_slice(&inReal[historyLen - cap_trailingIdx as usize..]);"));
    // Handle impl: infallible update, clone-peek, auto-trait pin.
    assert!(s.contains("pub fn update(&mut self, inReal: f64) -> f64 {"));
    assert!(s.contains("let mut scratch = self.clone();"));
    assert!(s.contains("_assert_auto::<SmaStream>();"));
    // Short history is an error, not batch's empty success.
    assert!(s.contains("return Err(RetCode::BadParam);"));
}

#[test]
fn test_rust_ema_scalar_recurrence_stream_section() {
    let s = rust_stream_section("ema");
    // T2 scalar state incl. the private K factor; no heap buffers at all.
    assert!(s.contains("struct EmaStreamState {"));
    assert!(s.contains("prevMA: f64,"));
    assert!(s.contains("optInK_1: f64,"));
    assert!(!s.contains("Vec<f64>,"), "EMA carries only scalars");
    // Compatibility is consumed during the transcribed open (read from self).
    assert!(s.contains("self.compatibility"));
    // Update returns the bare value.
    assert!(s.contains("pub fn update(&mut self, inReal: f64) -> f64 {"));
}

#[test]
fn test_rust_macd_three_output_tuple() {
    let s = rust_stream_section("macd");
    assert!(s.contains("-> Result<(MacdStream, (f64, f64, f64)), RetCode>"));
    assert!(s.contains("pub fn update(&mut self, inReal: f64) -> (f64, f64, f64) {"));
    assert!(s.contains(", outMACD: &mut f64, outMACDSignal: &mut f64, outMACDHist: &mut f64)"));
    // Tuple assembled in batch output order.
    assert!(s.contains("(outMACD, outMACDSignal, outMACDHist)"));
}

#[test]
fn test_rust_cdldoji_candle_settings_and_int_output() {
    let s = rust_stream_section("cdldoji");
    // Candle settings read through the handle's immutable Core snapshot.
    assert!(s.contains("self.candle_settings"));
    // Integer output end to end.
    assert!(s.contains("pub fn update(&mut self, inOpen: f64, inHigh: f64, inLow: f64, inClose: f64) -> i32 {"));
    assert!(s.contains("outInteger: &mut i32"));
    // OHLC ring over all four price arrays.
    assert!(s.contains("ring_BodyDojiTrailingIdx_inOpen"));
    assert!(s.contains("ring_BodyDojiTrailingIdx_inClose"));
}

#[test]
fn test_rust_minmaxindex_extrema_i32_and_rebase() {
    let s = rust_stream_section("minmaxindex");
    // AIA cursor machinery forced i32 (C's int) in the STATE...
    assert!(s.contains("xCap: i32,"));
    // ...with the batch-absolute rebase guard mirrored verbatim.
    assert!(s.contains("if sp.today >= 1073741824 {"));
    assert!(s.contains("let rebaseShift: i32 ="));
    // Capture casts the still-live batch locals at the struct literal.
    assert!(s.contains("today: (today) as i32,"));
    // Index outputs stay batch-exact i32 pairs.
    assert!(s.contains("pub fn update(&mut self, inReal: f64) -> (i32, i32) {"));
}

#[test]
fn test_rust_ht_dcperiod_parity_stream_section() {
    let s = rust_stream_section("ht_dcperiod");
    // Carried parity: seeded to the NEXT bar's parity, flipped per update.
    assert!(s.contains("streamParity: historyLen % 2,"));
    assert!(s.contains("sp.streamParity"));
    // The gate strip + parity carry leave no cursor/startIdx leak in the step.
    let step = s
        .split("fn ht_dcperiod_step_internal")
        .nth(1)
        .and_then(|t| t.split("/// Internal startIdx-anchored open").next())
        .expect("step body");
    assert!(!step.contains("startIdx"), "gate strip removed startIdx from the step");
    // Fixed-size Hilbert arrays are carried whole.
    assert!(s.contains("detrender_Even: [f64; 3 as usize],"));
}

#[test]
fn test_rust_dx_out_feedback_carried() {
    let s = rust_stream_section("dx");
    // Previous-output feedback carried as lastOut state (zero-denominator repeat).
    assert!(s.contains("lastOut_outReal: f64,"));
    assert!(s.contains("lastOut_outReal: lastValue_outReal,"));
}

#[test]
fn test_rust_identity_fast_path_t3() {
    let s = rust_stream_section("t3");
    // param==1 identity short-circuit before the transcribed body: min-history
    // check via lookback, passthrough value, default state.
    assert!(s.contains("if historyLen < self.t3_lookback(optInTimePeriod, optInVFactor) + 1 {"));
    assert!(s.contains("inReal[historyLen - 1]"));
}

#[test]
fn test_rust_stream_doctest_witness_present() {
    let s = rust_stream_section("sma");
    // Every open carries a runnable peek==update bit-equality witness.
    assert!(s.contains("let peeked = s.peek("));
    assert!(s.contains("assert_eq!(peeked.to_bits(), updated.to_bits());"));
}

// ---------------------------------------------------------------------------
// Terminal ratchet: EVERY streamable function emits a Rust stream (all six
// StreamPlan tiers landed). A regression in any tier's emitter or analyzer
// fails here; the count floors keep discovery bugs from passing vacuously.
// ---------------------------------------------------------------------------

#[test]
fn every_streamable_func_emits_rust_stream() {
    let dir = input_dir();
    let registry = Registry::from_dir(&dir);
    let mut funcs: Vec<ir::FuncDef> = Vec::new();
    for entry in std::fs::read_dir(&dir).expect("input dir") {
        let entry = entry.expect("entry");
        if !entry.path().is_dir() {
            continue;
        }
        let name = entry.file_name().to_string_lossy().to_string();
        let yaml = entry.path().join(format!("{name}.yaml"));
        let csrc = entry.path().join(format!("{name}.c"));
        if !yaml.exists() || !csrc.exists() {
            continue;
        }
        let mut func = parser::yaml::parse_yaml(&yaml);
        let parsed = parser::c_source::parse_c_source(&csrc);
        parser::c_source::wire_parsed_source(&mut func, &parsed);
        funcs.push(func);
    }
    assert!(funcs.len() >= 160, "discovery floor: found {}", funcs.len());
    let mut checked = 0;
    for f in &funcs {
        if !f.streaming {
            continue;
        }
        checked += 1;
        assert!(
            backends::rust_stream::emits_stream(f, &registry),
            "{}: streamable but no Rust stream emitted (tier regression)",
            f.name
        );
    }
    assert!(checked >= 160, "streamable floor: checked {checked}");
}
