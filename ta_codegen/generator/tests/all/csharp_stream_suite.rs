//! Render pins for the C# stream emitter (`backends/csharp_stream.rs`): the Open
//! family's stride mechanics, the C# twin of the same pins in
//! `java_stream_suite.rs`. Every runtime gate that could see these runs nightly.

use crate::common;
use std::collections::HashMap;
use std::path::PathBuf;
use ta_codegen_lib::{backends, ir, parser};

fn input_dir() -> PathBuf {
    PathBuf::from(env!("CARGO_MANIFEST_DIR")).join("../input")
}

fn load(name: &str) -> (ir::FuncDef, HashMap<String, ir::EnumDef>) {
    let dir = input_dir().join(name);
    let mut func = parser::yaml::parse_yaml(&dir.join(format!("{name}.yaml")));
    let parsed = parser::c_source::parse_c_source(&dir.join(format!("{name}.c")));
    parser::c_source::wire_parsed_source(&mut func, &parsed);
    let enums = parser::enums::load_enums(&input_dir().join("enums.yaml"));
    (func, enums)
}

fn section(name: &str) -> String {
    let (func, enums) = load(name);
    assert!(func.streaming, "{name}: yaml must carry the stream flag");
    let registry = common::make_registry();
    let helpers = common::make_helpers();
    let full = backends::csharp::generate(&func, &enums, registry, helpers);
    let at = full
        .find("/**** Streaming API *****/")
        .unwrap_or_else(|| panic!("{name}: stream section missing"));
    full[at..].to_string()
}

/// The brace-balanced body of the first definition whose signature contains `needle`.
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

/// The parameter list of the first definition whose signature contains `needle`.
fn signature_of<'a>(src: &'a str, needle: &str) -> &'a str {
    let at = src.find(needle).unwrap_or_else(|| panic!("no definition matching {needle:?}"));
    &src[at..at + src[at..].find(')').expect("signature closes")]
}

#[test]
fn csharp_open_family_is_one_core_with_three_entries() {
    let s = section("cdlhammer");
    assert_eq!(
        s.matches("private RetCode CdlhammerOpenImpl(").count(),
        1,
        "the numerics are emitted exactly once"
    );
    assert!(
        signature_of(&s, "private RetCode CdlhammerOpenImpl(").contains("int outStride"),
        "the numerics take a stride"
    );
    // The public fill reaches the numerics through the anchored seam, which is
    // what keeps that seam reachable for every function.
    for (entry, callee) in [
        ("internal CdlhammerStream CdlhammerOpenInternal(", "CdlhammerOpenSinkFrame("),
        ("internal CdlhammerStream CdlhammerOpenAndFillInternal(", "CdlhammerOpenImpl("),
        ("public CdlhammerStream CdlhammerOpenAndFill(", "CdlhammerOpenAndFillInternal("),
        ("public CdlhammerStream CdlhammerOpen(", "CdlhammerOpenInternal("),
    ] {
        let body = body_of(&s, entry);
        assert!(body.contains(callee), "{entry} delegates to {callee}:\n{body}");
        assert!(!body.contains("BodyPeriodTotal"), "{entry} must not re-transcribe the algorithm");
    }
    // MA and MAVP still emit this name, so its absence here is a discriminator.
    assert!(!s.contains("CdlhammerOpenAndFillImpl("), "a merged function has no second body");
}

#[test]
fn csharp_plain_open_uses_a_one_element_sink_at_stride_zero() {
    let s = section("cdlhikkake");
    assert!(!s.contains("CdlhikkakeOpenSinkFrame("), "the pin needs an opener that keeps one body");
    let body = body_of(&s, "internal CdlhikkakeStream CdlhikkakeOpenInternal(");
    assert!(body.contains("new int[1]"), "an int output sinks into a 1-element array:\n{body}");
    assert!(body.contains(", 0);"), "the plain open passes stride 0:\n{body}");
    let fill = body_of(&s, "internal CdlhikkakeStream CdlhikkakeOpenAndFillInternal(");
    assert!(fill.contains(", 1);"), "the anchored fill passes stride 1:\n{fill}");
}

#[test]
fn csharp_output_writes_are_stride_scaled() {
    let s = section("cdlhikkake");
    assert!(!s.contains("CdlhikkakeOpenSinkFrame("), "the pin needs an opener that keeps one body");
    assert!(
        s.contains("outInteger[outIdx++ * outStride] = 0;"),
        "per-bar output writes scale by the stride"
    );
}

// ---------------------------------------------------------------------------
// Split candlestick openers
// ---------------------------------------------------------------------------

/// Every streaming candlestick: its C# base name and stream section.
fn candlestick_sections() -> Vec<(String, String)> {
    let mut names: Vec<String> = common::discover_indicators()
        .into_iter()
        .filter(|n| {
            let (func, _) = load(n);
            func.streaming && func.is_candlestick()
        })
        .collect();
    names.sort();
    names
        .into_iter()
        .map(|n| {
            let s = section(&n);
            let at = s.find("private RetCode ").expect("an opener") + "private RetCode ".len();
            let base = s[at..at + s[at..].find("OpenImpl(").expect("the opener is OpenImpl")].to_string();
            (base, s)
        })
        .collect()
}

#[test]
fn a_csharp_candlestick_open_internal_calls_its_sink_frame() {
    let (mut framed, mut single) = (0usize, 0usize);
    for (base, s) in candlestick_sections() {
        let open = common::strip_block_comments(&body_of(&s, &format!("private RetCode {base}OpenImpl(")));
        let sink = format!("private RetCode {base}OpenSinkFrame(");
        assert!(!s.contains(&format!("{base}OpenFillFrame(")), "{base}: stride 1 runs the strided body, not a frame");
        assert!(open.contains("* outStride] = "), "{base}: the opener keeps its strided body");
        if !s.contains(&sink) {
            single += 1;
            continue;
        }
        framed += 1;
        let internal = body_of(&s, &format!("Stream {base}OpenInternal("));
        assert!(
            internal.contains(&format!("{base}OpenSinkFrame(")) && !internal.contains(&format!("{base}OpenImpl(")),
            "{base}OpenInternal opens through the sink frame:\n{internal}"
        );
        assert!(!common::calls_an_opener(&open, &base), "{base}OpenImpl calls another opener:\n{open}");
        assert!(common::stride_only_scales(&open), "{base}OpenImpl branches on its stride:\n{open}");
        let frame = common::strip_block_comments(&body_of(&s, &sink));
        let guard = frame.find("if( startIdx >= historyLen ) {").unwrap_or_else(|| panic!("{base}: no anchor guard:\n{frame}"));
        let head = &frame[..guard];
        assert!(
            !head.contains("while(") && !head.contains("for(") && !head.contains("do {"),
            "{base}: the sink frame loops before its anchor guard:\n{frame}"
        );
        let from = open.find("int historyLen").expect("history length");
        let at = open.find("if( startIdx >= historyLen ) {").expect("anchor guard");
        let checks = &open[from..at + open[at..].find("}\n").expect("guard closes") + 2];
        assert!(frame.contains(checks), "{base}: the sink frame runs other checks than OpenImpl:\n{checks}");
        assert!(!frame.contains("outStride"), "{sink}: the frame has its stride built in");
    }
    // The misfits by construction: their decisions assign pattern state.
    assert_eq!(single, 2, "only CDLHIKKAKE and CDLHIKKAKEMOD keep one body");
    assert!(framed >= 50, "only {framed} candlestick openers split");
}

/// The sink frame's history loop is the strided opener's pass with only its
/// carried statements kept, in order; its last pass decides the last bar once,
/// into slot 0, and runs the same carried statements.
#[test]
fn a_csharp_sink_frame_decides_only_the_last_bar() {
    let (mut framed, mut loops, mut carried) = (0usize, 0usize, 0usize);
    for (base, s) in candlestick_sections() {
        let sink = format!("private RetCode {base}OpenSinkFrame(");
        if !s.contains(&sink) {
            continue;
        }
        framed += 1;
        let sink = body_of(&s, &sink);
        let fill = body_of(&s, &format!("private RetCode {base}OpenImpl("));
        let found = common::history_loops(&common::squeezed_lines(&sink));
        let passes = common::fill_carried(&common::squeezed_lines(&fill), "outInteger");
        assert_eq!(
            found.len(),
            sink.matches("outNBElement = outIdx;").count(),
            "{base}: one loop over the history per region:\n{sink}"
        );
        assert_eq!(found.len(), passes.len(), "{base}: a history loop per storing loop of the opener");
        for ((history, after), pass) in found.iter().zip(&passes) {
            loops += 1;
            carried += pass.len();
            assert!(history.iter().any(|l| l == "outIdx+=1;"), "{base}: the history loop drops the output count:\n{history:#?}");
            assert_eq!(&common::without_cursor_steps(history), pass, "{base}: the history loop is not the opener's carried pass");
            common::assert_last_pass(&base, after, pass, "outInteger");
        }
        assert_eq!(
            sink.matches("sp.cur_outInteger = outInteger[0];").count(),
            found.len(),
            "{base}: every region's capture reads the sink"
        );
        assert_eq!(
            fill.matches("sp.cur_outInteger = outInteger[(outNBElement - 1) * outStride];").count(),
            found.len(),
            "{base}: every region's capture reads the caller's last element"
        );
        assert!(fill.contains("outInteger[outIdx++ * outStride] = "), "{base}: the opener stores at the strided index");
    }
    assert!(framed >= 50 && loops >= framed && carried >= 2 * loops, "{framed} sink frames, {loops} history loops, {carried} carried lines");
}

#[test]
fn csharp_public_fill_keeps_the_aliasing_guards() {
    // The public frame is the only one handed a caller-owned span it did not
    // vet; the seams below it get a fresh sink or a proved-disjoint destination.
    let s = section("accbands");
    let body = body_of(&s, "public AccbandsStream AccbandsOpenAndFill(");
    for guard in ["outRealUpperBand.Overlaps(inHigh)", "OutputsAlias(outRealUpperBand, outRealMiddleBand)"] {
        assert!(body.contains(guard), "{guard} survives on the public fill:\n{body}");
    }
    assert!(
        body.contains("throw StreamFailure(\"ACCBANDS\", \"openAndFill\", RetCode.BadParam);"),
        "the guard throws through the opener's shared mapping:\n{body}"
    );
    for seam in [
        "internal AccbandsStream AccbandsOpenInternal(",
        "internal AccbandsStream AccbandsOpenAndFillInternal(",
    ] {
        let sbody = body_of(&s, seam);
        assert!(
            !sbody.contains("Overlaps(") && !sbody.contains("OutputsAlias("),
            "{seam} must not carry the guard:\n{sbody}"
        );
    }
}

#[test]
fn csharp_exempt_tiers_keep_a_body_per_entry() {
    // The discriminator is the signature: an exempt `OpenImpl` takes no stride.
    for (name, base) in [("ma", "Ma"), ("mavp", "Mavp")] {
        let s = section(name);
        let sig = signature_of(&s, &format!("private RetCode {base}OpenImpl("));
        assert!(
            !sig.contains("int outStride"),
            "{base}OpenImpl is exempt: its open body is not the strided numerics:\n{sig}"
        );
        assert!(
            s.contains(&format!("private RetCode {base}OpenAndFillImpl(")),
            "{base} keeps a separate fill body"
        );
    }
    let s = section("sma");
    assert!(
        signature_of(&s, "private RetCode SmaOpenImpl(").contains("int outStride"),
        "a merged tier's open body IS the strided numerics"
    );
    assert!(!s.contains("private RetCode SmaOpenAndFillImpl("), "and it needs no second body");
}

#[test]
fn csharp_ma_fill_hands_fama_a_history_scratch() {
    let s = section("ma");
    for call in [
        "MamaOpenAndFill(inReal, 0.5, 0.05, outReal, new double[historyLen]);",
        "MamaOpenAndFillInternal(inReal, startIdx, 0.5, 0.05, out outBegIdx, out outNBElement, outReal, new double[historyLen]);",
    ] {
        assert!(s.contains(call), "MA's MAMA fill arm must pass `{call}`");
    }
    assert!(!s.contains(", default)"), "no output is passed as an empty span");
}

#[test]
fn csharp_composed_scratch_aliases_the_caller_at_stride_one() {
    // Paired with the absent copy-back, so the pin fails whichever way the
    // shape drifts: a re-rendered copy passes the negative alone.
    for (name, out) in [("adxr", "outReal"), ("stoch", "outSlowK"), ("stoch", "outSlowD")] {
        let s = section(name);
        assert!(
            s.contains(&format!(
                "Span<double> sc_{out} = outStride == 1 ? {out} : new double[historyLen];"
            )),
            "{name}: fill mode aliases the scratch onto the caller's {out}"
        );
        assert!(
            !s.contains(&format!("CopyTo({out}")),
            "{name}: no copy-back into {out} survives; the scratch already IS {out} at stride 1"
        );
    }
}

#[test]
fn csharp_identity_fast_path_short_circuits_at_stride_zero() {
    let s = section("t3");
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

/// Nothing provokes a failure inside the window evaluation, so a step that
/// commits the bar first passes every runtime gate while breaking "a throw
/// changes nothing".
#[test]
fn csharp_mavp_window_step_evaluates_before_it_commits() {
    let s = section("mavp");
    let step = body_of(&s, "private void MavpStepImpl(");
    let eval = step.find("MavpEvalWindow(").expect("the step has a window arm");
    let commit = step.find("sp.tape[sp.tapePos] =").expect("the step commits the bar");
    assert!(eval < commit, "the window value is computed before the tape takes the bar:\n{step}");
    let narrow = format!("if( optInMaxPeriod - optInMinPeriod + 1 < {} ) window = false;", ta_codegen_lib::streaming::WINDOW_MIN_BAND);
    assert_eq!(s.matches(&narrow).count(), 2, "both opens keep the bank for a narrow band");
    let mode = body_of(&s, "private static bool MavpWindowMode(");
    assert!(
        mode.contains("case MAType.ALMA:") && mode.contains("case MAType.DISABLED:"),
        "the window labels reach the C# switch:\n{mode}"
    );
}
