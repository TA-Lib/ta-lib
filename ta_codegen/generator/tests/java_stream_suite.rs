//! Render pins for the Java stream emitter (`backends/java_stream.rs`) —
//! the Java twin of `rust_stream_suite.rs`.
//!
//! Pins are substring/count/ordering assertions over the generated fragment
//! (never full snapshots), one per tier/mechanism plus the design-review
//! obligations (copy-constructor deep-copy traps, exception typing, aliasing
//! guards, candle snapshot, cached Value). Every pin doubles as a neuter
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

fn java_stream_section(name: &str) -> String {
    let (func, enums) = load_indicator(name);
    assert!(func.streaming, "{name}: yaml must carry the stream flag");
    let registry = Registry::from_dir(&input_dir());
    let helpers = HelperRegistry::from_dir(&input_dir().join("helpers"));
    let full = backends::java::generate(&func, &enums, &registry, &helpers);
    let start = full
        .find("/**** Streaming API *****/")
        .unwrap_or_else(|| panic!("{name}: stream section missing"));
    full[start..].to_string()
}

// ---------------------------------------------------------------------------
// Loop tier
// ---------------------------------------------------------------------------

#[test]
fn test_java_sma_ring_stream_section() {
    let s = java_stream_section("sma");
    // Nested handle class shape: package-private fields, no public ctor.
    assert!(s.contains("public static final class SMA_Stream {"));
    assert!(s.contains("Core core;"));
    assert!(s.contains("double[] ring_trailingIdx_inReal;"));
    assert!(s.contains("int ringPos_trailingIdx;"));
    assert!(!s.contains("public SMA_Stream("), "handle ctors stay non-public");
    // Deep-copy constructor clones the ring array.
    assert!(s.contains("this.ring_trailingIdx_inReal = other.ring_trailingIdx_inReal.clone();"));
    // ...and every class gets its in-place twin (#201), because any of them can
    // be some other handle's sub-stream.
    assert!(s.contains("void copyFrom( SMA_Stream other ) {"));
    assert!(s.contains("System.arraycopy( other.ring_trailingIdx_inReal, 0, this.ring_trailingIdx_inReal, 0, other.ring_trailingIdx_inReal.length );"));
    // One array and no sub-stream: peek keeps the plain copy, because the
    // scratch lookup would cost more than the allocation it saves.
    assert!(!s.contains("PEEK_SCRATCH"));
    assert!(s.contains("SMA_Stream scratch = new SMA_Stream(this);"));
    // The C mirror/peekMode machinery is deleted by design (copy-peek).
    assert!(!s.contains("Mirror"), "no peek mirrors in the Java tier");
    assert!(!s.contains("peekMode"), "no peekMode in the Java tier");
    // Lifecycle surface.
    assert!(s.contains("public double update( double inReal ) {"));
    assert!(s.contains("public double peek( double inReal ) {"));
    assert!(s.contains("public double value() {"));
    assert!(s.contains("public SMA_Stream copy() {"));
    assert!(!s.contains("public SMA_Stream fork()"), "copy(), never fork()");
    // Step is a package-private Core method writing the cur_ field.
    assert!(s.contains("void SMA_StreamStep( SMA_Stream sp, double inReal )"));
    assert!(s.contains("sp.cur_outReal ="));
    // Open body: the early-success no-data guard maps to InsufficientHistory,
    // which the wrapper types. It used to BORROW OutOfRangeEndIndex in band,
    // which also meant a history LONGER than MAX_INDEX + 1 -- the only other
    // producer of that code -- surfaced as "history shorter than lookback + 1".
    assert!(s.contains("return RetCode.InsufficientHistory ;"));
    assert!(!s.contains("return RetCode.OutOfRangeEndIndex ;"),
            "the borrowed in-band code is gone from the open body");
    // The message names the function as the metadata registry spells it, with
    // no C `TA_` prefix (that is C's namespacing, meaningless on a classpath).
    assert!(s.contains("throw new InsufficientHistoryException(\"SMA open:"));
    // Carrying, not a plain JDK type: the code has to be recoverable from every
    // failure the library raises, on this ladder as much as the batch one.
    assert!(s.contains("throw new TaLibStateException(\"SMA open: internal error\", retCode);"));
    assert!(!s.contains("\"TA_SMA open:"), "no C-namespaced prefix survives");
    // OpenAndFill: aliasing guard (Java is the one managed backend where
    // out == in compiles) and the batch output tail.
    assert!(s.contains("(Object)outReal == (Object)inReal"));
    assert!(s.contains("public SMA_Stream SMA_OpenAndFill( double inReal[], int optInTimePeriod, double outReal[] )"));
    // The filled range rides on the handle instead of a pair of out-params.
    assert!(s.contains("public OutRange fillRange() { return fillRange; }"));
    assert!(s.contains("sp.fillRange = new OutRange(outBegIdx.value, outNBElement.value);"));
    // Composition seam is package-private with a startIdx anchor.
    assert!(s.contains("SMA_Stream SMA_OpenInternal( double inReal[], int startIdx, int optInTimePeriod )"));
}

#[test]
fn test_java_ema_derived_state_and_compat() {
    let s = java_stream_section("ema");
    // The k factor is captured state, computed after default substitution.
    assert!(s.contains("double optInK_1;"));
    assert!(s.contains("sp.optInK_1 = optInK_1;"));
    // Compatibility is pinned to Default in Java and folded away at render
    // time, so the open carries the Default arm inline with no branch left.
    assert!(!s.contains("compatibility"), "no compatibility reference survives the fold");
    assert!(!s.contains("Compatibility."), "no Compatibility enum reference survives the fold");
    // Non-vacuity: the Default arm's body is what remains.
    assert!(s.contains("sp.optInK_1 = optInK_1;"));
}

#[test]
fn test_java_mama_value_class_protocol() {
    let s = java_stream_section("mama");
    // Multi-output => nested immutable Value record, components named after the
    // outputs and in batch output order.
    assert!(s.contains("public record Value(double mama, double fama) { }"));
    // The object protocol is the record's, so what used to be three generated
    // methods is now the absence of them — assert they are gone rather than
    // leaving the check vacuous.
    assert!(!s.contains("@Override public String toString() {"));
    assert!(!s.contains("Double.doubleToLongBits(this.mama)"));
    assert!(!s.contains("@Override public int hashCode() {"));
    // Components carry the batch method's own prose (java_doc::output_desc), so
    // one output reads the same in both tiers.
    assert!(s.contains("@param mama "));
    assert!(s.contains("@param fama "));
    // update caches the instance so value() is a pure field read.
    assert!(s.contains("this.cachedValue ="));
    assert!(s.contains("return this.cachedValue;"));
}

#[test]
fn test_java_cdl_candle_snapshot() {
    let s = java_stream_section("cdl3blackcrows");
    // A candle handle owns a ring per price per averaged setting, so peek runs
    // on the reused per-thread scratch rather than allocating a peer (#201).
    assert!(s.contains("private static final ThreadLocal<CDL3BLACKCROWS_Stream> PEEK_SCRATCH = new ThreadLocal<>();"));
    assert!(s.contains("CDL3BLACKCROWS_Stream scratch = PEEK_SCRATCH.get();"));
    assert!(s.contains("scratch.copyFrom(this);"));
    // Candle settings snapshot: primitive fields captured at open...
    assert!(s.contains("int cs_ShadowVeryShort_rangeType;"));
    assert!(s.contains("sp.cs_ShadowVeryShort_avgPeriod = ShadowVeryShort_avgPeriod;"));
    // ...and the step reads ONLY the snapshot, never the live objects.
    assert!(s.contains("int ShadowVeryShort_rangeType = sp.cs_ShadowVeryShort_rangeType;"));
    let step_start = s.find("void CDL3BLACKCROWS_StreamStep").expect("step");
    let step_end = s[step_start..].find("private RetCode").expect("open follows") + step_start;
    assert!(
        !s[step_start..step_end].contains("this.candleSettings"),
        "the step must not read live candle settings (torn-read hazard)"
    );
}

// ---------------------------------------------------------------------------
// Dual-mode tier
// ---------------------------------------------------------------------------

#[test]
fn test_java_trima_dual_mode() {
    let s = java_stream_section("trima");
    // One step, the arm re-derived from the stored param (no mode tag).
    assert!(s.contains("void TRIMA_StreamStep( TRIMA_Stream sp, double inReal )"));
    assert!(s.contains("sp.optInTimePeriod % 2"));
    // Both open arms transcribe under one shared validation head.
    let opens = s.matches("private RetCode TRIMA_OpenImpl").count();
    assert_eq!(opens, 1, "one Scalar open body");
}

#[test]
fn test_java_midprice_stream_uses_the_declared_alternate() {
    let s = java_stream_section("midprice");
    // The stream runs `midprice_ALT1`'s automaton, one unconditional step — the
    // batch block scan never appears as a param-selected branch.
    assert!(s.contains("void MIDPRICE_StreamStep( MIDPRICE_Stream sp, double inHigh, double inLow )"));
    assert!(
        s.contains("/* Using midprice_ALT1 for TA_ALT={STREAM,ALL_LANGUAGES} */"),
        "the stream section must name the alternate it resolved to"
    );
    for marker in ["sufHighest", "preHighest", "blockNext"] {
        assert!(
            !s.contains(marker),
            "`{marker}` reached the Java stream: it resolved to the block scan, not the alternate"
        );
    }
}

// ---------------------------------------------------------------------------
// Dispatch / period-bank tiers
// ---------------------------------------------------------------------------

#[test]
fn test_java_ma_dispatch() {
    let s = java_stream_section("ma");
    // Tagged handle: Object sub, null on the identity path.
    assert!(s.contains("Object sub;"));
    // The copy constructor and the step switch derive from the SAME arm table
    // (design-review obligation): all 9 MATypes appear in both.
    for label in ["SMA", "EMA", "WMA", "DEMA", "TEMA", "TRIMA", "KAMA", "MAMA", "T3"] {
        assert!(
            s.matches(&format!("case {label}:")).count() >= 2,
            "arm {label} must appear in both the copy constructor and dispatch switches"
        );
    }
    // MAMA arm routes OutSlot Forward(0) through the Value field and discards
    // FAMA; the fill tail materializes a throwaway buffer for the Discard.
    assert!(s.contains("MAMA_Stream.Value subValue = ((MAMA_Stream) sp.sub).update(inReal);"));
    assert!(s.contains("sp.cur_outReal = subValue.mama();"));
    assert!(s.contains("new double[historyLen]"));
    // Identity path re-derived from the stored param on every step; the guard
    // also covers the period-independent TA_MAType_DISABLED identity (issue #93).
    assert!(s.contains("if( sp.optInTimePeriod == 1 || sp.optInMAType == MAType.DISABLED ) {"));
    // Case labels come from the shared enum authority, not hardcoded ints.
    assert!(s.contains("case MAMA:"));
}

#[test]
fn test_java_mavp_period_bank() {
    let s = java_stream_section("mavp");
    assert!(s.contains("MA_Stream[] bank;"));
    // T1 deep-copy trap (design review): the bank must copy ELEMENT-WISE —
    // Object-array clone() would alias sub-streams and corrupt peek.
    assert!(s.contains("this.bank[bankIdx] = new MA_Stream(other.bank[bankIdx]);"));
    assert!(!s.contains("other.bank.clone()"), "bank.clone() is the aliasing trap");
    // Lockstep advance + clamp-select.
    assert!(s.contains("for( int bankIdx = 0; bankIdx < sp.bank.length; bankIdx++ ) {"));
    // Shared max-period seeding anchor.
    assert!(s.contains("MA_Lookback(optInMaxPeriod, optInMAType)"));
    // Fill replays history (no per-bar array exists to un-discard).
    assert!(s.contains("java.util.Arrays.copyOfRange(inReal, 0, lookbackTotal + 1)"));
}

// ---------------------------------------------------------------------------
// Composed tier
// ---------------------------------------------------------------------------

#[test]
fn test_java_stoch_composed() {
    let s = java_stream_section("stoch");
    // Owned public sub-handles, deep-copied in the copy constructor.
    assert!(s.contains("MA_Stream sub0;"));
    assert!(s.contains("MA_Stream sub1;"));
    assert!(s.contains("this.sub0 = new MA_Stream(other.sub0);"));
    // Pipeline in batch tail order over per-bar scalars.
    assert!(s.contains("cur_tempBuffer = sp.sub0.update(cur_tempBuffer);"));
    // Open: scratch outputs + sub-opens spliced at the consumption points. At
    // stride 1 the scratch IS the caller's array, so the Fill tail has nothing
    // left to copy back (issue #205).
    assert!(s.contains("double[] sc_outSlowK = outStride == 1 ? outSlowK : new double[historyLen];"));
    assert!(s.contains("OpenInternal(java.util.Arrays.copyOfRange("));
    assert!(
        !s.contains("System.arraycopy(sc_outSlowK, 0, outSlowK, 0, outNBElement.value);"),
        "the stride-1 copy-back is elided: the scratch already IS outSlowK"
    );
    // Multi-output Value with the stripped component names.
    assert!(s.contains("public record Value(double slowK, double slowD) { }"));
}

/// The sub-open range materialization (issue #203). Java has no slice type, so
/// the range the callee may read is conveyed by copying it out — except where
/// the range is already the whole array, which is decidable here: the source is
/// one of the function's own inputs (all `historyLen` long, checked by the open
/// validation) and the range ends at `endIdx`.
///
/// **No value gate can see this.** The callee reads the same numbers either
/// way, so the servers, `stream_verify` and the cross-language hashes all agree
/// whichever shape is emitted; the shape is the only observable. Both
/// directions are therefore pinned, and both can fail: restoring the copy fails
/// the elision half, and eliding unconditionally fails the retention half —
/// the latter would truncate nothing and hand `MA` a `tempBuffer` far longer
/// than the sub-call's own `endIdx`.
#[test]
fn test_java_composed_sub_open_elides_only_whole_array_copies() {
    // Elided: three own price inputs, each at `endIdx`.
    let adxr = java_stream_section("adxr");
    assert!(
        adxr.contains(
            "ADX_OpenAndFillInternal(inHigh, inLow, inClose, startIdx - (optInTimePeriod - 1)"
        ),
        "own inputs at endIdx pass straight through"
    );
    // Retained: an intermediate buffer at a computed sub-range, in the same
    // function family — the copy is what carries the length there. STOCH's
    // slow-K is also the one sub-call the #192 fusion declines (in place), so
    // this pins the unfused shape and the ADXR row above pins the fused one.
    let stoch = java_stream_section("stoch");
    assert!(
        stoch.contains("MA_OpenInternal(java.util.Arrays.copyOfRange(tempBuffer, 0, (outIdx - 1) + 1)"),
        "an intermediate sub-range keeps its copy"
    );
    // Both shapes inside ONE function, so the rule cannot be satisfied by a
    // per-function switch: STOCHRSI's RSI sub reads the whole input, its STOCHF
    // sub a prefix of the RSI series.
    let stochrsi = java_stream_section("stochrsi");
    assert!(
        stochrsi.contains("RSI_OpenAndFillInternal(inReal, startIdx - lookbackSTOCHF"),
        "STOCHRSI's own input is elided"
    );
    assert!(
        stochrsi.contains(
            "STOCHF_OpenAndFillInternal(java.util.Arrays.copyOfRange(tempRSIBuffer, 0, (tempArraySize - 1) + 1)"
        ),
        "STOCHRSI's intermediate prefix keeps its copy"
    );

    // Sweep: no whole-array copy of an own input survives anywhere, and the
    // genuine sub-ranges are still there (a blanket elision would pass the
    // first half alone).
    let registry = Registry::from_dir(&input_dir());
    let helpers = HelperRegistry::from_dir(&input_dir().join("helpers"));
    let enums = parser::enums::load_enums(&input_dir().join("enums.yaml"));
    let mut retained = 0usize;
    for entry in std::fs::read_dir(input_dir()).expect("input dir") {
        let dir = entry.expect("entry").path();
        let Some(name) = dir.file_name().map(|n| n.to_string_lossy().to_string()) else {
            continue;
        };
        let yaml = dir.join(format!("{name}.yaml"));
        if !dir.is_dir() || !yaml.exists() {
            continue;
        }
        let mut func = parser::yaml::parse_yaml(&yaml);
        if !func.streaming {
            continue;
        }
        let parsed = parser::c_source::parse_c_source(&dir.join(format!("{name}.c")));
        parser::c_source::wire_parsed_source(&mut func, &parsed);
        let out = backends::java::generate(&func, &enums, &registry, &helpers);
        for input in ta_codegen_lib::streaming::input_array_names(&func) {
            assert!(
                !out.contains(&format!("copyOfRange({input}, 0, (endIdx) + 1)")),
                "{name}: whole-array copy of own input `{input}` — the array already says its length"
            );
        }
        // Both sub-open shapes: fused (#192) and the in-place one it declines.
        // Counted only when the line still carries the `, 0, (` the negative
        // needle above assumes, so a re-rendering of the copy cannot quietly
        // turn that assertion vacuous — it fails this floor instead.
        retained += out
            .lines()
            .filter(|l| l.contains("Internal(java.util.Arrays.copyOfRange(") && l.contains(", 0, ("))
            .count();
    }
    assert!(
        retained >= 5,
        "the genuine sub-range copies vanished ({retained} left), or the copy is no longer \
         rendered as `copyOfRange(x, 0, (e) + 1)` — in which case the whole-array assertion \
         above is matching nothing and must be respelled"
    );
}

#[test]
fn test_java_adxr_sub_lag_ring() {
    let s = java_stream_section("adxr");
    assert!(s.contains("double[] lagRing_tempBuffer;") || s.contains("lagRing_"));
    // Read-oldest-then-push order with the modulo advance (mirrors C).
    let push = s.find("sp.lagRingPos_").expect("lag ring advance");
    let read = s.find("[sp.lagRingPos_").expect("lag ring read");
    assert!(read < push, "combine reads the oldest slot before the push");
}

// ---------------------------------------------------------------------------
// Emit ratchet
// ---------------------------------------------------------------------------

/// Every YAML stream-flagged function emits a Java stream section — the
/// terminal count is floored (the Rust suite's discovery-floor pattern) so a
/// silently-skipped tier, or a parser regression dropping `stream` flags, can
/// never read as green (the server set-parity gate is the runtime twin). A
/// floor rather than an exact pin: adding a stream function must not fail
/// this suite.
#[test]
fn test_java_stream_emit_ratchet() {
    let registry = Registry::from_dir(&input_dir());
    let helpers = HelperRegistry::from_dir(&input_dir().join("helpers"));
    let enums = parser::enums::load_enums(&input_dir().join("enums.yaml"));
    let mut emitted = 0usize;
    let mut total = 0usize;
    for entry in std::fs::read_dir(input_dir()).expect("input dir") {
        let dir = entry.expect("entry").path();
        if !dir.is_dir() {
            continue;
        }
        let name = dir.file_name().unwrap().to_string_lossy().to_string();
        let yaml = dir.join(format!("{name}.yaml"));
        if !yaml.exists() {
            continue;
        }
        let mut func = parser::yaml::parse_yaml(&yaml);
        if !func.streaming {
            continue;
        }
        total += 1;
        let parsed = parser::c_source::parse_c_source(&dir.join(format!("{name}.c")));
        parser::c_source::wire_parsed_source(&mut func, &parsed);
        let out = backends::java::generate(&func, &enums, &registry, &helpers);
        if out.contains("/**** Streaming API *****/") {
            emitted += 1;
        } else {
            panic!("{name}: declared streamable but no Java stream section");
        }
    }
    assert_eq!(emitted, total);
    assert!(
        emitted >= 168,
        "Java stream emit count fell below the 168 floor — a tier or `stream` flag was silently dropped (raise the floor deliberately as the family grows)"
    );
}

// ---------------------------------------------------------------------------
// Merged Open family (`OpenCore` + stride)
// ---------------------------------------------------------------------------
//
// `<base>_OpenImpl` and `<base>_OpenAndFillImpl` are one emission,
// `<base>_OpenPass(..., int outStride)`. Fill passes stride 1 and the caller's
// arrays; the scalar path passes stride 0 and a one-element sink, so every write
// collapses onto slot 0 and that slot ends holding the last history value —
// which is also what makes the `sp.cur_*` capture resolve with no special case.
// `Dispatch` (MA) and `PeriodBank` (MAVP) are exempt.

#[test]
fn java_open_family_is_one_core_with_two_wrappers() {
    let s = java_stream_section("cdlhammer");
    assert_eq!(
        s.matches("private RetCode CDLHAMMER_OpenPass(").count(),
        1,
        "the core is emitted exactly once"
    );
    assert!(s.contains("int outStride )"), "the core takes a stride");
    for w in [
        "private RetCode CDLHAMMER_OpenImpl(",
        "private RetCode CDLHAMMER_OpenAndFillImpl(",
    ] {
        let at = s.find(w).unwrap_or_else(|| panic!("missing {w}"));
        let body = &s[at..at + 800.min(s.len() - at)];
        assert!(body.contains("CDLHAMMER_OpenPass("), "{w} delegates to the core");
        assert!(
            !body.contains("BodyPeriodTotal"),
            "{w} must not re-transcribe the algorithm"
        );
    }
}

#[test]
fn java_scalar_wrapper_uses_a_one_element_sink_at_stride_zero() {
    let s = java_stream_section("cdlhammer");
    let at = s.find("private RetCode CDLHAMMER_OpenImpl(").expect("scalar wrapper");
    let body = &s[at..at + 800.min(s.len() - at)];
    assert!(body.contains("new int[1]"), "an int output sinks into a 1-element array:\n{body}");
    assert!(body.contains(", 0 );"), "scalar passes stride 0:\n{body}");
}

#[test]
fn java_output_writes_are_stride_scaled() {
    let s = java_stream_section("cdlhammer");
    assert!(
        s.contains("outInteger[outIdx++ * outStride] = 100;"),
        "per-bar output writes scale by the stride"
    );
}

#[test]
fn java_fill_wrapper_keeps_the_aliasing_guards() {
    // #108/#130: Java is the one managed backend where `out == in` compiles, so
    // the fill must reject it by reference. The scalar sink is a fresh array and
    // has no hazard.
    let s = java_stream_section("accbands");
    let at = s
        .find("private RetCode ACCBANDS_OpenAndFillImpl(")
        .expect("fill wrapper");
    let body = &s[at..at + 1600.min(s.len() - at)];
    assert!(
        body.contains("(Object)outRealUpperBand == (Object)inHigh"),
        "output-vs-input guard survives on the fill wrapper:\n{body}"
    );
    assert!(
        body.contains("(Object)outRealUpperBand == (Object)outRealMiddleBand"),
        "output-vs-output guard survives on the fill wrapper:\n{body}"
    );
    let sat = s.find("private RetCode ACCBANDS_OpenImpl(").expect("scalar wrapper");
    let sbody = &s[sat..sat + 800.min(s.len() - sat)];
    assert!(
        !sbody.contains("(Object)"),
        "Open has no aliasing hazard and must not carry the guard:\n{sbody}"
    );
}

#[test]
fn java_exempt_tiers_keep_two_bodies() {
    for (name, base) in [("ma", "MA"), ("mavp", "MAVP")] {
        let s = java_stream_section(name);
        assert!(
            !s.contains(&format!("{base}_OpenPass(")),
            "{base} is an exempt tier and must keep two bodies"
        );
        assert!(s.contains(&format!("{base}_OpenImpl(")));
        assert!(s.contains(&format!("{base}_OpenAndFillImpl(")));
    }
}

#[test]
fn java_composed_copy_out_is_stride_guarded() {
    // Issue #205: the fill-mode scratch aliases the caller's array, so the
    // stride-guarded copy-back is gone. The negative alone would pass on any
    // re-render of the copy, so it is paired with the positive that must
    // replace it — one of the two fails whichever way the shape drifts.
    let s = java_stream_section("adxr");
    assert!(
        s.contains("double[] sc_outReal = outStride == 1 ? outReal : new double[historyLen];"),
        "fill mode aliases the scratch onto the caller's array"
    );
    assert!(
        !s.contains("System.arraycopy(sc_outReal, 0, outReal,"),
        "no copy-back survives: the scratch already IS outReal at stride 1"
    );
}

#[test]
fn java_identity_fast_path_short_circuits_at_stride_zero() {
    // Java has no inliner guarantee here — a cold Open runs the loop in full —
    // so the stride-0 short-circuit matters more than in C/Rust, not less.
    let s = java_stream_section("t3");
    assert!(s.contains("if( outStride == 0 ) {"), "identity arm short-circuits at stride 0");
    assert!(
        s.contains("outReal[0] = inReal[historyLen - 1];"),
        "stride-0 arm takes the last bar directly"
    );
    assert!(
        s.contains("outReal[fillIdx] = inReal[fillLb + fillIdx];"),
        "fill arm indexes plainly"
    );
}
