//! Render pins for the Java stream emitter (`backends/java_stream.rs`) —
//! the Java twin of `rust_stream_suite.rs`.
//!
//! Pins are substring/count/ordering assertions over the generated fragment
//! (never full snapshots), one per tier/mechanism plus the design-review
//! obligations (copy-constructor deep-copy traps, exception typing, aliasing
//! guards, candle snapshot, cached Value). Every pin doubles as a neuter
//! check: the transition build panics on a cursor/startIdx leak, so a clean
//! render proves the analyzer normalizations fired.

use std::collections::{BTreeMap, BTreeSet, HashMap};
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

/// Every indicator directory whose YAML declares a stream.
fn streaming_indicators() -> Vec<String> {
    let mut v: Vec<String> = std::fs::read_dir(input_dir())
        .expect("input dir")
        .filter_map(Result::ok)
        .filter(|e| e.path().is_dir())
        .filter_map(|e| {
            let name = e.file_name().to_string_lossy().to_string();
            let yaml = e.path().join(format!("{name}.yaml"));
            yaml.exists()
                .then(|| parser::yaml::parse_yaml(&yaml))
                .filter(|f| f.streaming)
                .map(|_| name)
        })
        .collect();
    v.sort();
    v
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
    assert!(s.contains("Core core;"));
    assert!(s.contains("double[] ring_trailingIdx_inReal;"));
    assert!(s.contains("int ringPos_trailingIdx;"));
    assert!(!s.contains("public SmaStream("), "handle ctors stay non-public");
    // Deep-copy constructor clones the ring array.
    assert!(s.contains("this.ring_trailingIdx_inReal = other.ring_trailingIdx_inReal.clone();"));
    // ...and nothing else copies a handle: the in-place twin `copyFrom` existed
    // only to refresh peek's scratch, and there are no scratches.
    assert!(!s.contains("copyFrom"));
    // Peek copies nothing: it runs a frame against this handle, storing what
    // the step would commit in locals instead.
    assert!(!s.contains("PEEK_SCRATCH"));
    assert!(!s.contains("SmaStream scratch = new SmaStream(this);"));
    assert!(s.contains("SmaStream sp = this;"));
    // No backend carries a mirror or a routing flag any more, so these hold
    // everywhere rather than marking a difference.
    assert!(!s.contains("Mirror"), "no peek mirrors in the Java tier");
    assert!(!s.contains("peekMode"), "no peekMode in the Java tier");
    // Lifecycle surface.
    assert!(s.contains("public double update( double inReal ) {"));
    assert!(s.contains("public double peek( double inReal ) {"));
    assert!(s.contains("public double value() {"));
    assert!(s.contains("public SmaStream clone() {"));
    assert!(!s.contains("public SmaStream fork()"), "clone(), never fork()");
    // The override is what makes the name legal without the Cloneable protocol;
    // dropping it would compile but stop being an override the day the return
    // type or visibility drifts.
    assert!(s.contains("@Override\n      public SmaStream clone()"), "clone() is an @Override");
    assert!(!s.contains("implements Cloneable"), "no Cloneable: the body is a copy constructor");
    // Step is a package-private Core method writing the cur_ field.
    assert!(s.contains("void smaStepImpl( SmaStream sp, double inReal )"));
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
    assert!(s.contains("public SmaStream smaOpenAndFill( double inReal[], int optInTimePeriod, double outReal[] )"));
    // The range rides on the handle instead of a pair of out-params, and it is
    // the whole produced range, not one call's fill (#241): seeded by EVERY
    // opener — the plain one included, which wrote nothing before — and extended
    // by each committed bar. The accessor builds the record, so `update` keeps
    // its "never allocates handle state" promise.
    assert!(s.contains("int outRangeBegIdx;") && s.contains("int outRangeCount;"));
    assert!(s.contains(
        "public OutRange outRange() { return new OutRange(outRangeBegIdx, outRangeCount); }"
    ));
    assert!(!s.contains("fillRange"), "fillRange is gone, not aliased");
    let scalar = s
        .split("SmaStream smaOpenInternal( double inReal[], int startIdx, int optInTimePeriod )")
        .nth(1)
        .expect("smaOpenInternal")
        .split("\n   }")
        .next()
        .unwrap()
        .to_string();
    assert!(
        scalar.contains("sp.outRangeBegIdx = outBegIdx.value;")
            && scalar.contains("sp.outRangeCount = outNBElement.value;"),
        "the plain open reads the range back off the numerics it just ran:\n{scalar}"
    );
    assert!(
        s.contains("if( this.outRangeCount < MAX_INDEX ) this.outRangeCount++;"),
        "update advances the count, saturating"
    );
    // The copy constructor carries it — the one path left that copies a handle,
    // now that peek runs a frame instead of restoring a scratch.
    assert_eq!(s.matches("this.outRangeBegIdx = other.outRangeBegIdx;").count(), 1);
    assert_eq!(s.matches("this.outRangeCount = other.outRangeCount;").count(), 1);
    // Composition seam is package-private with a startIdx anchor.
    assert!(s.contains("SmaStream smaOpenInternal( double inReal[], int startIdx, int optInTimePeriod )"));
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
    // Multi-output => a caller-owned OUT class at Core level, fields named after
    // the outputs and in batch output order, mutable and with no value equality
    // (#310). Deliberately not a record: a reused instance as a map key would
    // break the hash invariant the moment its fields moved.
    assert!(s.contains("public static final class MamaOut {"));
    assert!(s.contains("public double mama;"));
    assert!(s.contains("public double fama;"));
    assert!(!s.contains("public record Value("), "the record is gone, not renamed");
    let out_body = {
        let b = s.find("public static final class MamaOut {").expect("MamaOut");
        let rest = &s[b..];
        &rest[..rest.find("\n   }").expect("MamaOut close")]
    };
    assert!(
        !out_body.contains("equals(") && !out_body.contains("hashCode("),
        "an Out carries no value equality"
    );
    // The object protocol is the record's, so what used to be three generated
    // methods is now the absence of them — assert they are gone rather than
    // leaving the check vacuous.
    assert!(!s.contains("@Override public String toString() {"));
    assert!(!s.contains("Double.doubleToLongBits(this.mama)"));
    assert!(!s.contains("@Override public int hashCode() {"));
    // Fields carry the batch method's own prose (java_doc::output_desc), so one
    // output reads the same in both tiers. A record documented its components
    // with @param; a class documents each field where the field is.
    for field in ["mama", "fama"] {
        let decl = s
            .find(&format!("public double {field};"))
            .unwrap_or_else(|| panic!("{field} is not a field of the Out class"));
        // The line immediately above carries the batch method's own prose, so a
        // silently undocumented field fails rather than passing on a doc comment
        // that happens to exist elsewhere in the section.
        let above = s[..decl].rfind('\n').and_then(|e| s[..e].rfind('\n').map(|b| &s[b + 1..e]));
        let above = above.unwrap_or("").trim();
        assert!(
            above.starts_with("/** ") && above.len() > 12,
            "{field} has no prose on the line above it, found: {above:?}"
        );
    }
    // No cache: update/peek/value write the caller's sink and store nothing.
    assert!(!s.contains("cachedValue"), "the cached instance is gone (#310)");
    assert!(s.contains("public void value( MamaOut out ) {"));
    assert!(s.contains("public void update( double inReal, MamaOut out ) {"));
    assert!(s.contains("public void peek( double inReal, MamaOut out ) {"));
}

#[test]
fn test_java_cdl_candle_snapshot() {
    let s = java_stream_section("cdl3blackcrows");
    // A candle handle owns a ring per price per averaged setting — the shape
    // that used to need the per-thread scratch (#201). The frame reads those
    // rings in place, so there is no scratch to hold.
    assert!(!s.contains("PEEK_SCRATCH"));
    assert!(s.contains("Cdl3blackcrowsStream sp = this;"));
    // Candle settings snapshot: primitive fields captured at open...
    assert!(s.contains("int cs_ShadowVeryShort_rangeType;"));
    assert!(s.contains("sp.cs_ShadowVeryShort_avgPeriod = ShadowVeryShort_avgPeriod;"));
    // ...and the step reads ONLY the snapshot, never the live objects.
    assert!(s.contains("int ShadowVeryShort_rangeType = sp.cs_ShadowVeryShort_rangeType;"));
    let step_start = s.find("void cdl3blackcrowsStepImpl").expect("step");
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
    assert!(s.contains("void trimaStepImpl( TrimaStream sp, double inReal )"));
    assert!(s.contains("sp.optInTimePeriod % 2"));
    // Both open arms transcribe under one shared validation head.
    let opens = s.matches("private RetCode trimaOpenImpl").count();
    assert_eq!(opens, 1, "one Scalar open body");
}

#[test]
fn test_java_midprice_stream_uses_the_declared_alternate() {
    let s = java_stream_section("midprice");
    // The stream runs `midprice_ALT1`'s automaton, one unconditional step — the
    // batch block scan never appears as a param-selected branch.
    assert!(s.contains("void midpriceStepImpl( MidpriceStream sp, double inHigh, double inLow )"));
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
    // (design-review obligation): every MAType naming a function appears in both.
    for label in [
        "SMA", "EMA", "WMA", "DEMA", "TEMA", "TRIMA", "KAMA", "MAMA", "T3", "HMA", "ZLEMA",
        "RMA",
    ] {
        assert!(
            s.matches(&format!("case {label}:")).count() >= 2,
            "arm {label} must appear in both the copy constructor and dispatch switches"
        );
    }
    // MAMA arm routes OutSlot Forward(0) and discards FAMA, through the same
    // caller-owned sink the composed peek uses: Java has no out-params, so a
    // multi-output sub-handle's N values leave the call in an object whichever
    // verb asks for them (#310, residue tracked by #325). Reading the
    // sub-handle's own committed `cur_*` would be free on this path, but that
    // needs a sink-less `update` the API does not have.
    assert!(s.contains("MamaOut subOut = new MamaOut();"));
    assert!(s.contains("((MamaStream) sp.sub).update(inReal, subOut);"));
    assert!(s.contains("sp.cur_outReal = subOut.mama;"));
    assert!(
        !s.contains("MamaStream.Value"),
        "the record is gone, not renamed"
    );
    // …and the Discard slot DECLINES the callee's nullable output rather than
    // materializing a throwaway buffer for it (rule B6a at the opener).
    assert!(s.contains("mamaOpenAndFill(inReal, 0.5, 0.05, outReal, null)"));
    assert!(!s.contains("new double[historyLen]"));
    // Identity path re-derived from the stored param on every step; the guard
    // also covers the period-independent TA_MAType_DISABLED identity (issue #93).
    assert!(s.contains("if( sp.optInTimePeriod == 1 || sp.optInMAType == MAType.DISABLED ) {"));
    // Case labels come from the shared enum authority, not hardcoded ints.
    assert!(s.contains("case MAMA:"));
}

#[test]
fn test_java_mavp_period_bank() {
    let s = java_stream_section("mavp");
    assert!(s.contains("MaStream[] bank;"));
    // T1 deep-copy trap (design review): the bank must copy ELEMENT-WISE —
    // Object-array clone() would alias sub-streams and corrupt peek.
    assert!(s.contains("this.bank[bankIdx] = new MaStream(other.bank[bankIdx]);"));
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
    assert!(s.contains("MaStream sub0;"));
    assert!(s.contains("MaStream sub1;"));
    assert!(s.contains("this.sub0 = new MaStream(other.sub0);"));
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
    // Multi-output OUT class with the stripped field names (#310).
    assert!(s.contains("public static final class StochOut {"));
    assert!(s.contains("public double slowK;"));
    assert!(s.contains("public double slowD;"));
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
            "adxOpenAndFillInternal(inHigh, inLow, inClose, startIdx - (optInTimePeriod - 1)"
        ),
        "own inputs at endIdx pass straight through"
    );
    // Retained: an intermediate buffer at a computed sub-range, in the same
    // function family — the copy is what carries the length there. STOCH's
    // slow-K is also the one sub-call the #192 fusion declines (in place), so
    // this pins the unfused shape and the ADXR row above pins the fused one.
    let stoch = java_stream_section("stoch");
    assert!(
        stoch.contains("maOpenInternal(java.util.Arrays.copyOfRange(tempBuffer, 0, (outIdx - 1) + 1)"),
        "an intermediate sub-range keeps its copy"
    );
    // Both shapes inside ONE function, so the rule cannot be satisfied by a
    // per-function switch: STOCHRSI's RSI sub reads the whole input, its STOCHF
    // sub a prefix of the RSI series.
    let stochrsi = java_stream_section("stochrsi");
    assert!(
        stochrsi.contains("rsiOpenAndFillInternal(inReal, startIdx - lookbackSTOCHF"),
        "STOCHRSI's own input is elided"
    );
    assert!(
        stochrsi.contains(
            "stochfOpenAndFillInternal(java.util.Arrays.copyOfRange(tempRSIBuffer, 0, (tempArraySize - 1) + 1)"
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
// Merged Open family: three entry points over one `<base>_OpenImpl`
// ---------------------------------------------------------------------------
//
// The numerics are one emission, `<base>_OpenImpl(..., int outStride)`. The two
// filling entries pass stride 1 and the caller's arrays; `_OpenInternal` passes
// stride 0 and a one-element sink, so every write collapses onto slot 0 and that
// slot ends holding the last history value — which is also what makes the
// `sp.cur_*` capture resolve with no special case. The public `_OpenAndFill` is
// `_OpenAndFillInternal` at anchor 0 with the aliasing guard in front, mirroring
// `_Open` over `_OpenInternal`; that symmetry is what leaves no entry point
// emitted unreachable. `Dispatch` (MA) and `PeriodBank` (MAVP) are exempt and
// hand-roll a body per entry.

#[test]
fn java_open_family_is_one_core_with_three_entries() {
    let s = java_stream_section("cdlhammer");
    assert_eq!(
        s.matches("private RetCode cdlhammerOpenImpl(").count(),
        1,
        "the numerics are emitted exactly once"
    );
    assert!(s.contains("int outStride )"), "the numerics take a stride");
    // Each entry reaches the numerics, and none of them re-transcribes it.
    // The public fill goes through the anchored seam rather than straight down,
    // which is what keeps the seam reachable for every function.
    for (w, callee) in [
        ("CdlhammerStream cdlhammerOpenInternal(", "cdlhammerOpenImpl("),
        ("CdlhammerStream cdlhammerOpenAndFillInternal(", "cdlhammerOpenImpl("),
        ("public CdlhammerStream cdlhammerOpenAndFill(", "cdlhammerOpenAndFillInternal("),
        ("public CdlhammerStream cdlhammerOpen(", "cdlhammerOpenInternal("),
    ] {
        let at = s.find(w).unwrap_or_else(|| panic!("missing {w}"));
        // Wide enough for the four price legs' presence checks to precede the
        // delegating call on a candlestick's public fill.
        let body = &s[at..at + 1600.min(s.len() - at)];
        assert!(body.contains(callee), "{w} delegates to {callee}:\n{body}");
        assert!(
            !body.contains("BodyPeriodTotal"),
            "{w} must not re-transcribe the algorithm"
        );
    }
    // No adapter tier survives between an entry point and the numerics.
    // Both names still EXIST in the corpus -- MA and MAVP keep them -- so these
    // are discriminators, not assertions about a word nothing emits.
    for gone in ["CDLHAMMER_OpenAndFillImpl(", "CDLHAMMER_OpenAndFillInternalImpl("] {
        assert!(!s.contains(gone), "{gone} is a retired tier on a merged function");
    }
}

#[test]
fn java_plain_open_uses_a_one_element_sink_at_stride_zero() {
    let s = java_stream_section("cdlhammer");
    let at = s
        .find("CdlhammerStream cdlhammerOpenInternal(")
        .expect("the anchored plain open");
    let body = &s[at..at + 800.min(s.len() - at)];
    assert!(body.contains("new int[1]"), "an int output sinks into a 1-element array:\n{body}");
    assert!(body.contains(", 0);"), "the plain open passes stride 0:\n{body}");
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
fn java_public_fill_keeps_the_aliasing_guards() {
    // #108/#130: Java is the one managed backend where `out == in` compiles, so
    // a filling open must reject it by reference. The guard sits on the PUBLIC
    // frame, the only one that can be handed a caller-owned array it did not
    // vet: the plain open sinks into a fresh array, and every composed call into
    // the anchored seam passes a destination `SubCallStep::is_fusable` already
    // proved disjoint. It throws rather than answering a code, because the
    // public frame is where a code becomes an exception.
    let s = java_stream_section("accbands");
    let at = s
        .find("public AccbandsStream accbandsOpenAndFill(")
        .expect("public fill");
    // Sliced to the frame's END, not to a byte budget: the frame grows when a
    // rule is added to it, and a budget turns that into a false failure.
    let end = s[at..].find("\n   }\n").map_or(s.len() - at, |e| e + 4);
    let body = &s[at..at + end];
    assert!(
        body.contains("(Object)outRealUpperBand == (Object)inHigh"),
        "output-vs-input guard survives on the public fill:\n{body}"
    );
    assert!(
        body.contains("(Object)outRealUpperBand == (Object)outRealMiddleBand"),
        "output-vs-output guard survives on the public fill:\n{body}"
    );
    assert!(
        body.contains("throw new TaLibArgumentException(\"ACCBANDS openAndFill: \" + RetCode.BadParam, RetCode.BadParam);"),
        "the guard throws the same text the retired ladder produced:\n{body}"
    );
    // Paired negatives: both are false today only because the guard moved UP,
    // so a re-render that pushes it back down fails the positive above.
    for (w, why) in [
        ("AccbandsStream accbandsOpenInternal(", "the plain open sinks into fresh arrays"),
        ("AccbandsStream accbandsOpenAndFillInternal(", "the composed seam's destinations are proved disjoint"),
    ] {
        let sat = s.find(w).unwrap_or_else(|| panic!("missing {w}"));
        let sbody = &s[sat..sat + 900.min(s.len() - sat)];
        assert!(!sbody.contains("(Object)"), "{why}, so {w} must not carry the guard:\n{sbody}");
    }
}

#[test]
fn java_exempt_tiers_keep_a_body_per_entry() {
    // MA and MAVP differ between entry points by more than a stride -- which
    // callee tier they call, and an anchor clamp -- so they hand-roll one
    // RetCode-returning body each instead of sharing one strided `_OpenImpl`.
    // The discriminator is that signature: an exempt `_OpenImpl` takes no
    // stride. Asserting the ABSENCE of a name would now hold for all 176.
    for (name, base) in [("ma", "maOpenImpl"), ("mavp", "mavpOpenImpl")] {
        let s = java_stream_section(name);
        let at = s
            .find(&format!("private RetCode {base}("))
            .unwrap_or_else(|| panic!("{base}"));
        let sig = &s[at..at + s[at..].find(')').expect("signature closes")];
        assert!(
            !sig.contains("int outStride"),
            "{base} is exempt: its open body is not the strided numerics:\n{sig}"
        );
        let fill = base.replace("OpenImpl", "OpenAndFillImpl");
        assert!(
            s.contains(&format!("private RetCode {fill}(")),
            "{base} keeps a separate fill body"
        );
    }
    // ...and the merged tiers are the other side of that discriminator.
    let s = java_stream_section("sma");
    let at = s.find("private RetCode smaOpenImpl(").expect("smaOpenImpl");
    let sig = &s[at..at + s[at..].find(')').expect("signature closes")];
    assert!(sig.contains("int outStride"), "a merged tier's open body IS the strided numerics");
    assert!(!s.contains("private RetCode smaOpenAndFillImpl("), "and it needs no second body");
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

/// The handle's fixed-size accumulator fields: an array the BATCH body declares
/// with a literal size. Off the emitted code, never a name list.
fn accumulator_fields(section: &str, batch: &str) -> BTreeSet<String> {
    let mut out = BTreeSet::new();
    for line in section.lines() {
        if !line.starts_with("      ") || line.starts_with("       ") {
            continue;
        }
        let Some(d) = line.trim().strip_suffix(';') else { continue };
        let Some((ty, name)) = d.split_once(' ') else { continue };
        if !ty.ends_with("[]") || !name.chars().all(|c| c.is_alphanumeric() || c == '_') {
            continue;
        }
        let decl = format!("{ty} {name} = new {}[", ty.trim_end_matches("[]"));
        if batch.match_indices(&decl).any(|(i, _)| {
            batch[i + decl.len()..].split_once(']').is_some_and(|(n, _)| n.parse::<u32>().is_ok())
        }) {
            out.insert(name.to_string());
        }
    }
    out
}

/// No tier copies a handle to peek it — swept over the whole corpus.
///
/// Structural, because no value gate can see it: a peek that copied and then
/// wrote the copy would still answer correctly. What it costs is the
/// flat-in-period cost the frame is for.
#[test]
fn no_java_peek_copies_the_handle() {
    /// The handle's own fields: a two-token declaration at the class's own
    /// indent (`      double[] ring_x;`).
    fn handle_fields(s: &str) -> BTreeSet<String> {
        s.lines()
            .filter(|l| l.starts_with("      ") && !l.starts_with("       "))
            .filter_map(|l| l.trim().strip_suffix(';'))
            .filter_map(|d| d.split_once(' '))
            .filter(|(_, n)| {
                !n.is_empty() && n.chars().all(|c| c.is_alphanumeric() || c == '_')
            })
            .map(|(_, n)| n.to_string())
            .collect()
    }

    /// Every name `line` writes, with any subscript stripped, each paired with
    /// whether that write also DECLARES the name. Empty when it writes nothing.
    ///
    /// Every operator that stores, not just `=`. `x += e`, `x++` and `++x`
    /// reach the same slot `x = e` does, and none of them can declare a name,
    /// so each is a write to a name the frame must already have declared —
    /// which is the rule this sweep exists to enforce. The emitted frames carry
    /// all three shapes: `periodTotal += inReal;` is SMA's, `pkIdx0 = i++ &
    /// sp.xMask;` and `while( ++i <= today )` are the extrema and Hilbert
    /// tiers'. Reading `=` alone enforced the rule on one operator out of
    /// several and passed the rest through as if they wrote nothing.
    fn write_targets(line: &str) -> Vec<(&str, bool)> {
        /// The trailing name of `lhs`: `x` / `sp.x` / `x[i]` -> the name.
        fn name_of(lhs: &str) -> Option<&str> {
            // The declared name is the LAST token; strip its subscript there,
            // not over the whole left side — `double[] x = ...` carries a `[`
            // in the TYPE, and cutting at it would name the type instead.
            let last = lhs.trim().rsplit(' ').next()?;
            let last = last.split_once('[').map_or(last, |(h, _)| h);
            (!last.is_empty() && last.chars().all(|c| c.is_alphanumeric() || c == '_' || c == '.'))
                .then_some(last)
        }

        fn is_name_byte(c: u8) -> bool {
            c.is_ascii_alphanumeric() || c == b'_' || c == b'.'
        }

        /// The identifier ending at `end`, skipping back over a subscript so
        /// `buf[k]++` names `buf`. Empty when there is none.
        fn ident_before(s: &str, end: usize) -> &str {
            let b = s.as_bytes();
            let mut end = end;
            if end > 0 && b[end - 1] == b']' {
                let mut depth = 0usize;
                let mut k = end;
                while k > 0 {
                    k -= 1;
                    match b[k] {
                        b']' => depth += 1,
                        b'[' => {
                            depth -= 1;
                            if depth == 0 {
                                break;
                            }
                        }
                        _ => {}
                    }
                }
                if depth != 0 {
                    return "";
                }
                end = k;
            }
            let mut start = end;
            while start > 0 && is_name_byte(b[start - 1]) {
                start -= 1;
            }
            &s[start..end]
        }

        /// The identifier starting at `start`. Empty when there is none.
        fn ident_after(s: &str, start: usize) -> &str {
            let b = s.as_bytes();
            let mut end = start;
            while end < b.len() && is_name_byte(b[end]) {
                end += 1;
            }
            &s[start..end]
        }

        let l = line.trim().trim_end_matches(';').trim_end();
        let b = l.as_bytes();
        let mut out = Vec::new();

        // The assignment first, so a `for( int i = 0; ...; i++ )` header has
        // declared `i` before its own increment is read below.
        //
        // The first `=` that stores: `==`/`!=`/`<=`/`>=` compare, while
        // `<<=`/`>>=` store — which is why the `<`/`>` case looks one
        // character further back.
        let eq = (0..b.len()).find(|&i| {
            if b[i] != b'=' {
                return false;
            }
            let prev = i.checked_sub(1).map(|k| b[k]);
            let compare = b.get(i + 1) == Some(&b'=')
                || matches!(prev, Some(b'=' | b'!'))
                || (matches!(prev, Some(b'<' | b'>')) && i.checked_sub(2).map(|k| b[k]) != prev);
            !compare
        });
        if let Some(eq) = eq {
            const OPS: [char; 10] = ['+', '-', '*', '/', '%', '&', '|', '^', '<', '>'];
            let lhs = &l[..eq];
            let compound = lhs.ends_with(OPS);
            let lhs = lhs.trim_end_matches(OPS);
            // A declaration is `<type> <name> =`; a compound store never one.
            let declares = !compound && lhs.trim().split(' ').count() > 1;
            out.extend(name_of(lhs).map(|n| (n, declares)));
        }

        // `x++` / `++x`, anywhere on the line — an increment inside a `while`
        // or `if` header writes exactly as one on its own does.
        let mut i = 0usize;
        while i + 1 < b.len() {
            if !((b[i] == b'+' && b[i + 1] == b'+') || (b[i] == b'-' && b[i + 1] == b'-')) {
                i += 1;
                continue;
            }
            let before = ident_before(l, i);
            let name = if before.is_empty() { ident_after(l, i + 2) } else { before };
            if !name.is_empty() {
                out.push((name, false));
            }
            i += 2;
        }
        out
    }

    let mut swept = 0usize;
    let mut frames = 0usize;
    let mut writes = 0usize;
    let mut sink_sites: BTreeSet<String> = BTreeSet::new();
    let mut bounded: BTreeSet<String> = BTreeSet::new();
    let mut fixtures = 0usize;
    let mut fully_shadowed: BTreeSet<String> = BTreeSet::new();
    let mut offenders: Vec<String> = Vec::new();
    for name in streaming_indicators() {
        let (func, enums) = load_indicator(&name);
        let registry = Registry::from_dir(&input_dir());
        let helpers = HelperRegistry::from_dir(&input_dir().join("helpers"));
        let batch = backends::java::generate(&func, &enums, &registry, &helpers);
        let s = java_stream_section(&name);
        let Some(at) = s.find(" peek( ") else { continue };
        let start = s[..at].rfind("      public ").expect("a peek signature");
        let end = s[start..].find("\n      }").map_or(s.len(), |k| start + k);
        let peek = &s[start..end];
        swept += 1;
        if peek.contains(" sp = this;") {
            frames += 1;
        }
        let fields = handle_fields(&s);
        // Hoisted: the copy check below consults it per line, and the
        // non-vacuity counter after the loop still uses the same set.
        let accs = accumulator_fields(&s, &batch);
        // A `synth<n>` fixture is a construct probe copied into input/ by
        // scripts/synth_gate.py, never a shipped function — the same family
        // `--function=SYNTH` selects. It is the discriminator because the
        // fixtures live in input/ exactly as shipped functions do once
        // injected, and nothing else tells them apart.
        let fixture = name.starts_with("synth");
        if fixture {
            fixtures += 1;
        }
        let mut locals: BTreeSet<&str> = BTreeSet::new();
        for line in peek.lines() {
            let l = line.trim();
            // A frame writes locals. A bare `cur_x = ...` whose name the frame
            // never DECLARED resolves to the handle field of that name and
            // commits it — which is what a composed output reached only through
            // an alias used to do. A value gate sees it only if the corrupted
            // field is one an output reads back; this sweep names it directly.
            // A comment carries these operators too, so the write sweep reads
            // code lines only — as the allocation checks below already do.
            let code = l.starts_with("//") || l.starts_with("/*") || l.starts_with("*");
            let targets = if code { Vec::new() } else { write_targets(l) };
            for (t, declared) in targets {
                if declared {
                    locals.insert(t);
                } else if t.starts_with("sp.") || (fields.contains(t) && !locals.contains(t)) {
                    offenders.push(format!("{name}: writes the handle: {l}"));
                } else {
                    writes += 1;
                }
            }
            // Comments carry the word too, and `throw new
            // TaLibArgumentException` is the bar rejection — neither copies a
            // handle. `new <N>Out()` is the sink a COMPOSED frame allocates for
            // a multi-output sub-handle (#310); it is counted separately and
            // pinned to an exact set below rather than blanket-exempted, so a
            // third site fails and so does losing one of the two.
            let composed_sink = !code
                && l.contains(" = new ")
                && l.contains("Out();");
            if composed_sink {
                sink_sites.insert(name.clone());
            }
            let allocates_handle = !code
                && l.contains("new ")
                && !l.starts_with("throw new")
                && !composed_sink;
            // `.clone()` is the OTHER way to allocate here, and matching only
            // `new ` missed it: a frame clones a written array field because a
            // Java array is a reference.
            //
            // ONE copy is contract-legal, and only one: a FIXED-SIZE
            // accumulator, an array the batch body declares with a literal
            // dimension. The frame's job is that its cost not grow with the
            // period, and such a copy cannot -- which is what `peek`'s own
            // javadoc already promises the caller ("a small bounded amount per
            // call, a size fixed by the indicator, never by the period"), and
            // what the C# twin's doc comment has always claimed. Read off the
            // emitted declaration, never a name list, so a period-sized buffer
            // can never qualify.
            //
            // It stays an offender for a SHIPPED function even so. The emitter
            // reaches the copy only where it cannot shadow the write in place
            // (SMA's peek shadows a ring write with a pending `(slot, value)`
            // pair; that needs ONE known write slot, and an arbitrary computed
            // index has none). No shipped function is in that position today,
            // so a shipped one that started copying is a regression from the
            // shadow into the fallback, and this is the only thing that would
            // say so.
            let cloned = (!code)
                .then(|| l.split_once(" = sp."))
                .flatten()
                .and_then(|(_, rhs)| rhs.strip_suffix(".clone();"));
            if let Some(f) = cloned {
                if fixture && accs.contains(f) {
                    bounded.insert(format!("{name}.{f}"));
                } else if accs.contains(f) {
                    offenders.push(format!(
                        "{name}: a SHIPPED peek copies the accumulator {f} rather than shadowing the write: {l}"
                    ));
                } else {
                    offenders.push(format!("{name}: clones the handle's {f}: {l}"));
                }
            } else if allocates_handle || l.contains("copyFrom") || l.contains("PEEK_SCRATCH") {
                offenders.push(format!("{name}: {l}"));
            }
        }
        // The frame must READ an accumulator: a field it never names is
        // no evidence about the copy either way.
        if accs.iter().any(|f| peek.contains(&format!("{f}["))) {
            fully_shadowed.insert(name.clone());
        }
    }
    assert!(swept > 170, "only {swept} peek(s) swept");
    assert_eq!(frames, swept, "{frames} of {swept} peek(s) run a frame");
    assert!(writes >= 500, "only {writes} local writes seen — the store sweep found nothing");
    assert!(
        fully_shadowed.len() >= 21,
        "only {} handle(s) have a peek frame that reads an accumulator — the sweep \
         is looking for something that is not there",
        fully_shadowed.len()
    );
    assert!(offenders.is_empty(), "a peek copies:\n{}", offenders.join("\n"));

    // Non-vacuity for the exemption, asserted only where it can be: on the
    // shipped corpus the set is EMPTY and must be, so there is nothing to
    // prove. The fixtures are the only thing that reaches the fallback, so
    // when they are in the tree at least one copy must appear — otherwise the
    // branch above is dead and would pass whatever the emitter did with it.
    if fixtures > 0 {
        assert!(
            !bounded.is_empty(),
            "{fixtures} fixture(s) swept and none reached the bounded-accumulator \
             copy — the exemption is dead code and proves nothing"
        );
    }

    // EXACTLY these two, not "at most". Both are composed peeks whose callee is
    // far over C2's inline budget, so the sink is allocated whatever shape it
    // takes — it allocated as a returned `Value` before #310 too. #325 records
    // the analysis and the only fix (inlining the sub-frame). A third site
    // means a new composed multi-output peek nobody costed; losing one means
    // #325 landed and this bound should tighten with it.
    let expected: BTreeSet<String> =
        ["ma", "stochrsi"].iter().map(|s| (*s).to_string()).collect();
    assert_eq!(
        sink_sites, expected,
        "the set of composed peeks allocating a sub-handle sink moved"
    );
}

/// The SAME residue on the committing verbs, which the peek sweep above cannot
/// see: a composed `update` reaches its multi-output sub-handle through the same
/// caller-owned sink, because Java has no out-params and the API has no
/// sink-less `update`. Pinned as an exact per-function COUNT, not a set — the
/// two functions each carry one site per verb, and losing or gaining one on
/// either verb is the thing #325 changes.
///
/// Non-vacuity: the map is asserted non-empty and every counted line is required
/// to be a real `new <N>Out()` allocation, so an emitter that stopped writing
/// them fails here rather than passing with an empty sweep.
#[test]
fn the_composed_sub_handle_sinks_are_exactly_the_costed_four() {
    let mut sites: BTreeMap<String, usize> = BTreeMap::new();
    for name in streaming_indicators() {
        let s = java_stream_section(&name);
        let n = s
            .lines()
            .filter(|l| {
                let l = l.trim();
                !l.starts_with("//") && !l.starts_with("*") && l.contains(" = new ") && l.ends_with("Out();")
            })
            .count();
        if n > 0 {
            sites.insert(name, n);
        }
    }
    let expected: BTreeMap<String, usize> =
        [("ma".to_string(), 2usize), ("stochrsi".to_string(), 2usize)]
            .into_iter()
            .collect();
    assert!(!sites.is_empty(), "the sweep found no sink allocation at all");
    assert_eq!(
        sites, expected,
        "the composed sub-handle sink sites moved (one per verb on each of the two \
         composed multi-output callees, #325)"
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
