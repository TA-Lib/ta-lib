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
    assert!(s.contains("public static final class SmaStream {"));
    assert!(s.contains("final Core core;"));
    assert!(s.contains("double[] ring_trailingIdx_inReal;"));
    assert!(s.contains("int ringPos_trailingIdx;"));
    assert!(!s.contains("public SmaStream("), "handle ctors stay non-public");
    // Deep-copy constructor clones the ring array.
    assert!(s.contains("this.ring_trailingIdx_inReal = other.ring_trailingIdx_inReal.clone();"));
    // The C mirror/peekMode machinery is deleted by design (copy-peek).
    assert!(!s.contains("Mirror"), "no peek mirrors in the Java tier");
    assert!(!s.contains("peekMode"), "no peekMode in the Java tier");
    // Lifecycle surface.
    assert!(s.contains("public double update( double inReal ) {"));
    assert!(s.contains("public double peek( double inReal ) {"));
    assert!(s.contains("public double value() {"));
    assert!(s.contains("public SmaStream copy() {"));
    assert!(!s.contains("public SmaStream fork()"), "copy(), never fork()");
    // Step is a package-private Core method writing the cur_ field.
    assert!(s.contains("void smaStreamStep( SmaStream sp, double inReal )"));
    assert!(s.contains("sp.cur_outReal ="));
    // Open body: early-success no-data guard maps to the in-band
    // insufficient-history signal; the wrapper types it.
    assert!(s.contains("return RetCode.OutOfRangeEndIndex ;"));
    assert!(s.contains("throw new InsufficientHistoryException(\"TA_SMA open:"));
    assert!(s.contains("throw new IllegalStateException(\"TA_SMA open: internal error\");"));
    // OpenAndFill: aliasing guard (Java is the one managed backend where
    // out == in compiles) and the batch output tail.
    assert!(s.contains("(Object)outReal == (Object)inReal"));
    assert!(s.contains("public SmaStream smaOpenAndFill( double inReal[], int optInTimePeriod, MInteger outBegIdx, MInteger outNBElement, double outReal[] )"));
    // Composition seam is package-private with a startIdx anchor.
    assert!(s.contains("SmaStream smaOpenInternal( double inReal[], int startIdx, int optInTimePeriod )"));
}

#[test]
fn test_java_ema_private_extra_param_and_compat() {
    let s = java_stream_section("ema");
    // The k factor is captured state, computed after default substitution.
    assert!(s.contains("double optInK_1;"));
    assert!(s.contains("sp.optInK_1 = optInK_1;"));
    // Compatibility branches transcribe into the open (settings at open).
    assert!(s.contains("this.compatibility == Compatibility.Default"));
}

#[test]
fn test_java_mama_value_class_protocol() {
    let s = java_stream_section("mama");
    // Multi-output => nested immutable Value with named final fields.
    assert!(s.contains("public static final class Value {"));
    assert!(s.contains("public final double mama;"));
    assert!(s.contains("public final double fama;"));
    // Generated object protocol (design-review FLAW fix): toString + bit-based
    // equals/hashCode.
    assert!(s.contains("@Override public String toString() {"));
    assert!(s.contains("Double.doubleToLongBits(this.mama) == Double.doubleToLongBits(v.mama)"));
    assert!(s.contains("@Override public int hashCode() {"));
    // update caches the instance so value() is a pure field read.
    assert!(s.contains("this.cachedValue ="));
    assert!(s.contains("return this.cachedValue;"));
}

#[test]
fn test_java_cdl_candle_snapshot() {
    let s = java_stream_section("cdl3blackcrows");
    // Candle settings snapshot: primitive fields captured at open...
    assert!(s.contains("int cs_ShadowVeryShort_rangeType;"));
    assert!(s.contains("sp.cs_ShadowVeryShort_avgPeriod = ShadowVeryShort_avgPeriod;"));
    // ...and the step reads ONLY the snapshot, never the live objects.
    assert!(s.contains("int ShadowVeryShort_rangeType = sp.cs_ShadowVeryShort_rangeType;"));
    let step_start = s.find("void cdl3BlackCrowsStreamStep").expect("step");
    let step_end = s[step_start..].find("private RetCode").expect("open follows") + step_start;
    assert!(
        !s[step_start..step_end].contains("this.candleSettings"),
        "the step must not read live candle settings (torn-read hazard)"
    );
}

// ---------------------------------------------------------------------------
// Dual-mode / fast-path-skip tiers
// ---------------------------------------------------------------------------

#[test]
fn test_java_trima_dual_mode() {
    let s = java_stream_section("trima");
    // One step, the arm re-derived from the stored param (no mode tag).
    assert!(s.contains("void trimaStreamStep( TrimaStream sp, double inReal )"));
    assert!(s.contains("sp.optInTimePeriod % 2"));
    // Both open arms transcribe under one shared validation head.
    let opens = s.matches("private RetCode trimaOpenBody").count();
    assert_eq!(opens, 1, "one Scalar open body");
}

#[test]
fn test_java_midprice_fastpath_skip() {
    let s = java_stream_section("midprice");
    // The stream always runs the general arm; the batch fast path never
    // appears as a param-selected branch in the step.
    assert!(s.contains("void midPriceStreamStep( MidPriceStream sp, double inHigh, double inLow )"));
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
    for label in ["Sma", "Ema", "Wma", "Dema", "Tema", "Trima", "Kama", "Mama", "T3"] {
        assert!(
            s.matches(&format!("case {label}:")).count() >= 2,
            "arm {label} must appear in both the copy constructor and dispatch switches"
        );
    }
    // MAMA arm routes OutSlot Forward(0) through the Value field and discards
    // FAMA; the fill tail materializes a throwaway buffer for the Discard.
    assert!(s.contains("MamaStream.Value subValue = ((MamaStream) sp.sub).update(inReal);"));
    assert!(s.contains("sp.cur_outReal = subValue.mama;"));
    assert!(s.contains("new double[historyLen]"));
    // Identity path re-derived from the stored param on every step.
    assert!(s.contains("if( sp.optInTimePeriod == 1 ) {"));
    // Case labels come from the shared enum authority, not hardcoded ints.
    assert!(s.contains("case Mama:"));
}

#[test]
fn test_java_mavp_period_bank() {
    let s = java_stream_section("mavp");
    assert!(s.contains("MovingAverageStream[] bank;"));
    // T1 deep-copy trap (design review): the bank must copy ELEMENT-WISE —
    // Object-array clone() would alias sub-streams and corrupt peek.
    assert!(s.contains("this.bank[bankIdx] = new MovingAverageStream(other.bank[bankIdx]);"));
    assert!(!s.contains("other.bank.clone()"), "bank.clone() is the aliasing trap");
    // Lockstep advance + clamp-select.
    assert!(s.contains("for( int bankIdx = 0; bankIdx < sp.bank.length; bankIdx++ ) {"));
    // Shared max-period seeding anchor.
    assert!(s.contains("movingAverageLookback(optInMaxPeriod, optInMAType)"));
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
    assert!(s.contains("MovingAverageStream sub0;"));
    assert!(s.contains("MovingAverageStream sub1;"));
    assert!(s.contains("this.sub0 = new MovingAverageStream(other.sub0);"));
    // Pipeline in batch tail order over per-bar scalars.
    assert!(s.contains("cur_tempBuffer = sp.sub0.update(cur_tempBuffer);"));
    // Open: scratch outputs + sub-opens spliced at the consumption points,
    // then the Fill tail memcpys scratch into the caller arrays.
    assert!(s.contains("double[] sc_outSlowK = new double[historyLen];"));
    assert!(s.contains("OpenInternal(java.util.Arrays.copyOfRange("));
    assert!(s.contains("System.arraycopy(sc_outSlowK, 0, outSlowK, 0, outNBElement.value);"));
    // Multi-output Value with the stripped field names.
    assert!(s.contains("public final double slowK;"));
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
/// terminal count is pinned so a silently-skipped tier can never read as
/// green (the server set-parity gate is the runtime twin of this pin).
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
    assert_eq!(
        emitted, 165,
        "Java stream emit count moved — update this ratchet deliberately"
    );
}
