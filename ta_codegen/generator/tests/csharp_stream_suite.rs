//! Render pins for the C# stream emitter (`backends/csharp_stream.rs`): the Open
//! family's stride mechanics, the C# twin of the same pins in
//! `java_stream_suite.rs`. Every runtime gate that could see these runs nightly.

use std::collections::HashMap;
use std::path::PathBuf;
use ta_codegen_lib::helper_registry::HelperRegistry;
use ta_codegen_lib::registry::Registry;
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
    let registry = Registry::from_dir(&input_dir());
    let helpers = HelperRegistry::from_dir(&input_dir());
    let full = backends::csharp::generate(&func, &enums, &registry, &helpers);
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
        ("internal CdlhammerStream CdlhammerOpenInternal(", "CdlhammerOpenImpl("),
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
    let s = section("cdlhammer");
    let body = body_of(&s, "internal CdlhammerStream CdlhammerOpenInternal(");
    assert!(body.contains("new int[1]"), "an int output sinks into a 1-element array:\n{body}");
    assert!(body.contains(", 0);"), "the plain open passes stride 0:\n{body}");
    let fill = body_of(&s, "internal CdlhammerStream CdlhammerOpenAndFillInternal(");
    assert!(fill.contains(", 1);"), "the anchored fill passes stride 1:\n{fill}");
}

#[test]
fn csharp_output_writes_are_stride_scaled() {
    let s = section("cdlhammer");
    assert!(
        s.contains("outInteger[outIdx++ * outStride] = 100;"),
        "per-bar output writes scale by the stride"
    );
}

#[test]
fn csharp_public_fill_keeps_the_aliasing_guards() {
    // The public frame is the only one handed a caller-owned span it did not
    // vet; the seams below it get a fresh sink or a proved-disjoint destination.
    let s = section("accbands");
    let body = body_of(&s, "public AccbandsStream AccbandsOpenAndFill(");
    for guard in ["outRealUpperBand.Overlaps(inHigh)", "outRealUpperBand.Overlaps(outRealMiddleBand)"] {
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
        assert!(!sbody.contains("Overlaps("), "{seam} must not carry the guard:\n{sbody}");
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
