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

/// The body of the first item whose signature line matches `needle`,
/// brace-balanced. Panics if absent — every caller asserts presence first.
fn body_of(src: &str, needle: &str) -> String {
    let i = src.find(needle).unwrap_or_else(|| panic!("no definition matching {needle:?}"));
    let j = src[i..].find('{').expect("definition has a body") + i;
    let bytes = src.as_bytes();
    let (mut depth, mut k) = (0usize, j);
    loop {
        match bytes[k] {
            b'{' => depth += 1,
            b'}' => {
                depth -= 1;
                if depth == 0 {
                    break;
                }
            }
            _ => {}
        }
        k += 1;
    }
    src[j..=k].to_string()
}

// ---------------------------------------------------------------------------
// Loop tier
// ---------------------------------------------------------------------------

#[test]
fn test_rust_sma_ring_stream_section() {
    let s = rust_stream_section("sma");
    // Handle + state struct shapes.
    assert!(s.contains("pub struct SMA_Stream {"));
    assert!(s.contains("core: Core,"));
    assert!(s.contains("state: SMA_StreamState,"));
    assert!(s.contains("struct SMA_StreamState {"));
    assert!(s.contains("ring_trailingIdx_inReal: Vec<f64>,"));
    assert!(s.contains("ringPos_trailingIdx: usize,"));
    // The C mirror/peekMode machinery is deleted by design (clone-peek).
    assert!(!s.contains("Mirror"), "no peek mirrors in the Rust tier");
    assert!(!s.contains("peekMode"), "no peekMode in the Rust tier");
    assert!(!s.contains("unsafe"), "stream sections are safe Rust");
    // Step: ring read-old-then-push order, `(*outReal)` write.
    assert!(s.contains("fn SMA_step_internal(&self, sp: &mut SMA_StreamState, inReal: f64, outReal: &mut f64)"));
    assert!(s.contains("(*outReal) = sp.tempReal / (sp.optInTimePeriod as f64);"));
    assert!(s.contains("sp.ring_trailingIdx_inReal[sp.ringPos_trailingIdx] = inReal;"));
    // Open family: internal seam + thin wrapper + fill in batch param order.
    assert!(s.contains("pub(crate) fn SMA_OpenInternal("));
    assert!(s.contains("self.SMA_OpenInternal(inReal, 0, optInTimePeriod)"));
    assert!(s.contains("pub fn SMA_OpenAndFill(\n        &self, inReal: &[f64], mut optInTimePeriod: i32, outBegIdx: &mut usize, outNBElement: &mut usize, outReal: &mut [f64],"));
    // Capture: numeric ring cap from live locals + tail copy.
    assert!(s.contains("let cap_trailingIdx: i64 = (i as i64) - (trailingIdx as i64);"));
    assert!(s.contains(".copy_from_slice(&inReal[historyLen - cap_trailingIdx as usize..]);"));
    // Handle impl: infallible update, scratch-peek, auto-trait pin.
    assert!(s.contains("pub fn update(&mut self, inReal: f64) -> f64 {"));
    // Every state gets the buffer-reusing restore (#201) — a state can be some
    // other handle's sub — but SMA's own peek does not use it: one ring, no
    // sub-handle and a loop-free transition is the shape whose stack copy the
    // optimizer deletes outright, which no scratch can beat.
    assert!(s.contains("self.ring_trailingIdx_inReal.clone_from(&src.ring_trailingIdx_inReal);"));
    assert!(s.contains("pub fn peek(&self, inReal: f64) -> f64 {"));
    assert!(s.contains("let mut scratch = self.clone();"));
    assert!(s.contains("scratch.update(inReal)"));
    assert!(!s.contains("PEEK_SCRATCH"));
    assert!(s.contains("_assert_auto::<SMA_Stream>();"));
    // Short history is an error, not batch's empty success.
    assert!(s.contains("return Err(RetCode::BadParam);"));
}

#[test]
fn test_rust_ema_scalar_recurrence_stream_section() {
    let s = rust_stream_section("ema");
    // T2 scalar state incl. the private K factor; no heap buffers at all.
    assert!(s.contains("struct EMA_StreamState {"));
    assert!(s.contains("prevMA: f64,"));
    assert!(s.contains("optInK_1: f64,"));
    assert!(!s.contains("Vec<f64>,"), "EMA carries only scalars");
    // Compatibility is consumed during the transcribed open (read from self).
    assert!(s.contains("self.compatibility"));
    // Update returns the bare value.
    assert!(s.contains("pub fn update(&mut self, inReal: f64) -> f64 {"));
    // No heap in the state, so peek keeps the throwaway copy and no
    // thread-local scratch is emitted at all (#201).
    assert!(s.contains("let mut scratch = self.clone();"));
    assert!(!s.contains("PEEK_SCRATCH"), "a scalar state needs no scratch buffer");
    // The restore is still emitted: EMA is a sub-stream of several composed
    // handles, whose own scratch restores through it.
    assert!(s.contains("fn restore_from(&mut self, src: &Self) {"));
}

#[test]
fn test_rust_macd_three_output_tuple() {
    let s = rust_stream_section("macd");
    assert!(s.contains("-> Result<(MACD_Stream, (f64, f64, f64)), RetCode>"));
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
    // Four buffers is several allocations per clone, none of which the
    // optimizer folds away, so peek reuses a per-thread scratch (#201).
    assert!(s.contains("static CDLDOJI_PEEK_SCRATCH: std::cell::Cell<Option<Box<CDLDOJI_Stream>>>"));
    assert!(s.contains("CDLDOJI_PEEK_SCRATCH.with(|cell| {"));
    assert!(s.contains("scratch.restore_from(self);"));
    // The scratch is a HANDLE and peek calls `update` on it, so the transition
    // keeps the single call site it had before #201 — see the emitter's note.
    assert!(s.contains("let value = scratch.update(inOpen, inHigh, inLow, inClose);"));
    assert!(s.contains("cell.set(Some(scratch));"));
}

#[test]
fn test_rust_minmaxindex_extrema_i32_and_rebase() {
    let s = rust_stream_section("minmaxindex");
    // AIA cursor machinery forced i32 (C's int) in the STATE...
    assert!(s.contains("xMask: i32,"));
    // ...with the batch-absolute rebase guard mirrored verbatim.
    assert!(s.contains("if sp.today >= 1073741824 {"));
    assert!(s.contains("let rebaseShift: i32 ="));
    // Capture casts the still-live batch locals at the struct literal.
    assert!(s.contains("today: (today) as i32,"));
    // Index outputs stay batch-exact i32 pairs.
    assert!(s.contains("pub fn update(&mut self, inReal: f64) -> (i32, i32) {"));
    // One buffer: peek keeps the throwaway copy, the shape whose clone the
    // optimizer folds away (#201).
    assert!(!s.contains("PEEK_SCRATCH"));
}

#[test]
fn test_rust_ht_dcperiod_parity_stream_section() {
    let s = rust_stream_section("ht_dcperiod");
    // Carried parity: seeded to the NEXT bar's parity, flipped per update.
    assert!(s.contains("streamParity: historyLen % 2,"));
    assert!(s.contains("sp.streamParity"));
    // The gate strip + parity carry leave no cursor/startIdx leak in the step.
    let step = s
        .split("fn HT_DCPERIOD_step_internal")
        .nth(1)
        .and_then(|t| t.split("/// The single whole-history transcription").next())
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
    assert!(s.contains("lastOut_outReal: outReal[(*outNBElement - 1) * outStride],"));
}

#[test]
fn test_rust_identity_fast_path_t3() {
    let s = rust_stream_section("t3");
    // param==1 identity short-circuit before the transcribed body: min-history
    // check via lookback, passthrough value, default state.
    assert!(s.contains("if historyLen < self.T3_Lookback(optInTimePeriod, optInVFactor) + 1 {"));
    // Stride 0 short-circuits to the last bar; only the fill arm loops. Letting
    // the loop run at stride 0 is correct but makes the scalar Open O(history).
    assert!(s.contains("if outStride == 0 {"), "identity arm short-circuits at stride 0");
    assert!(s.contains("outReal[0] = inReal[historyLen - 1];"), "stride-0 arm takes the last bar");
    assert!(s.contains("outReal[fillIdx] = inReal[fillLb + fillIdx];"), "fill arm indexes plainly");
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

// ---------------------------------------------------------------------------
// Merged Open family (`OpenCore` + stride)
// ---------------------------------------------------------------------------
//
// `OpenInternal` and `OpenAndFill` are one emission: `<sn>_OpenCore(..., outStride:
// usize)`. Fill passes stride 1 and the caller's slice; the scalar path passes
// stride 0 and a one-element sink, so every write collapses onto slot 0 and that
// slot ends holding the last history value. `Dispatch` (MA) and `PeriodBank`
// (MAVP) are exempt — they hand the fill to a sub / run a different warm-up.

#[test]
fn rust_open_family_is_one_core_with_two_wrappers() {
    let s = rust_stream_section("cdlhammer");
    assert_eq!(
        s.matches("fn CDLHAMMER_OpenCore(").count(),
        1,
        "the core is emitted exactly once"
    );
    assert!(s.contains("outStride: usize"), "the core takes a stride");
    // Both wrappers delegate; neither re-transcribes the algorithm.
    for w in ["fn CDLHAMMER_OpenInternal(", "pub fn CDLHAMMER_OpenAndFill("] {
        let at = s.find(w).unwrap_or_else(|| panic!("missing {w}"));
        let body = &s[at..at + 900.min(s.len() - at)];
        assert!(body.contains("CDLHAMMER_OpenCore("), "{w} delegates to the core");
        assert!(
            !body.contains("BodyPeriodTotal"),
            "{w} must not re-transcribe the algorithm"
        );
    }
}

#[test]
fn rust_scalar_wrapper_uses_a_one_element_sink_at_stride_zero() {
    let s = rust_stream_section("cdlhammer");
    let at = s.find("fn CDLHAMMER_OpenInternal(").expect("scalar wrapper");
    let body = &s[at..at + 900.min(s.len() - at)];
    assert!(body.contains("[0_i32; 1]"), "an int output sinks into a 1-element array:\n{body}");
    assert!(body.contains(", 0)?"), "scalar passes stride 0:\n{body}");
    assert!(body.contains("sink_outInteger[0]"), "the value comes back from slot 0:\n{body}");
}

#[test]
fn rust_output_writes_are_stride_scaled() {
    let s = rust_stream_section("cdlhammer");
    assert!(
        s.contains("outInteger[({ let _v = outIdx; outIdx += 1; _v } * outStride) as usize] = 100;"),
        "per-bar output writes scale by the stride"
    );
}

#[test]
fn rust_fill_wrapper_keeps_the_output_distinctness_guard() {
    // #108: the capture epilogue reads the input tail after writing the outputs,
    // so two outputs may not share a slice. Rust's borrow checker rules out
    // output-vs-input, but not output-vs-output.
    let s = rust_stream_section("minmax");
    let at = s.find("pub fn MINMAX_OpenAndFill(").expect("fill wrapper");
    let body = &s[at..at + 700.min(s.len() - at)];
    assert!(
        body.contains("outMin.as_ptr() == outMax.as_ptr()"),
        "output distinctness survives on the fill wrapper:\n{body}"
    );
    // The scalar wrapper's sinks are its own locals — it must not pay for it.
    let sat = s.find("fn MINMAX_OpenInternal(").expect("scalar wrapper");
    let sbody = &s[sat..sat + 700.min(s.len() - sat)];
    assert!(
        !sbody.contains("as_ptr()"),
        "Open has no aliasing hazard and must not carry the guard:\n{sbody}"
    );
}

#[test]
fn rust_multi_output_scalar_wrapper_rebuilds_the_value_tuple() {
    let s = rust_stream_section("minmax");
    let at = s.find("fn MINMAX_OpenInternal(").expect("scalar wrapper");
    let body = &s[at..at + 900.min(s.len() - at)];
    assert!(
        body.contains("(sink_outMin[0], sink_outMax[0])"),
        "a 2-output scalar open returns the pair from the sinks:\n{body}"
    );
}

#[test]
fn rust_exempt_tiers_keep_their_own_bodies() {
    for (name, upper) in [("ma", "MA"), ("mavp", "MAVP")] {
        let s = rust_stream_section(name);
        assert!(
            !s.contains(&format!("fn {upper}_OpenCore(")),
            "{upper} is an exempt tier and must keep its own bodies"
        );
        assert!(s.contains(&format!("fn {upper}_OpenInternal(")));
        assert!(s.contains(&format!("fn {upper}_OpenAndFill(")));
    }
}

/// The Rust twin of `dispatch_open_modes_differ_only_where_intended`: since
/// issue #204 all three open entry points come out of one emitter over a mode
/// list, so this is what pins which mode owns which difference. Rust needs no
/// aliasing rejection — `&mut [f64]` parameters cannot overlap — so the
/// differences are the out-meta pair, the startIdx anchor, and the callee entry
/// point each arm delegates to.
#[test]
fn rust_dispatch_open_modes_differ_only_where_intended() {
    let s = rust_stream_section("ma");
    let scalar = body_of(&s, "fn MA_OpenInternal(");
    let fill = body_of(&s, "fn MA_OpenAndFill(");
    let internal = body_of(&s, "fn MA_OpenAndFillInternal(");

    assert!(!scalar.contains("outBegIdx"), "the scalar open has no out-meta:\n{scalar}");
    for (what, body) in [("OpenAndFill", &fill), ("OpenAndFillInternal", &internal)] {
        assert!(body.contains("outBegIdx"), "{what} carries the out-meta pair:\n{body}");
    }

    assert!(
        internal.contains("let fillLb = if startIdx > fillLb"),
        "OpenAndFillInternal must clamp the fill anchor up to startIdx:\n{internal}"
    );
    assert!(!fill.contains("startIdx"), "the public fill is anchored at bar 0:\n{fill}");

    assert!(
        scalar.contains("SMA_OpenInternal(") && !scalar.contains("_OpenAndFill"),
        "the scalar arms open the sub's OpenInternal:\n{scalar}"
    );
    assert!(
        fill.contains("SMA_OpenAndFill(") && !fill.contains("_OpenAndFillInternal("),
        "the public fill arms call the sub's public OpenAndFill:\n{fill}"
    );
    assert!(
        internal.contains("SMA_OpenAndFillInternal("),
        "the internal fill arms call the sub's OpenAndFillInternal:\n{internal}"
    );
}

#[test]
fn rust_composed_copy_out_is_stride_guarded() {
    // Both modes compute into `sc_*`; only the hand-back differs, and a bulk
    // copy takes a base pointer rather than a subscript — the one place the
    // stride cannot express the difference on its own.
    let s = rust_stream_section("adxr");
    assert!(
        s.contains("if outStride == 1 {") && s.contains("copy_from_slice"),
        "the composed copy-out is guarded by the stride"
    );
}
