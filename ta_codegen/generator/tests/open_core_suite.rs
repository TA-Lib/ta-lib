//! The Open family's single-core shape, over the REAL ta_codegen/input corpus.
//!
//! `Open` and `OpenAndFill` are the same whole-history transcription; they differ
//! only in where the per-bar output writes land. They are emitted ONCE, as
//! `TA_<N>_OpenImpl( ..., int outStride )`: `Open` passes stride 0 and a
//! one-element scalar sink (every write collapses to slot 0, which is also what
//! makes the previous-output feedback reads resolve), `OpenAndFill` passes stride
//! 1 and the caller's array.
//!
//! Two tiers are exempt and keep their own bodies, for reasons that are NOT stylistic:
//! `Dispatch` (MA) delegates the fill to a sub's public `OpenAndFill`, and
//! `PeriodBank` (MAVP) genuinely runs a DIFFERENT warm-up algorithm in each mode
//! (scalar warms the bank with one whole-history sub-open; fill opens at
//! `lookbackTotal + 1` and drives a per-bar `TA_MA_Update` loop). Both already
//! hand-write their two bodies and never used the shared open machinery.

use std::collections::HashMap;
use std::path::Path;

use ta_codegen_lib::backends::c_stream;
use ta_codegen_lib::helper_registry::HelperRegistry;
use ta_codegen_lib::ir;
use ta_codegen_lib::parser;
use ta_codegen_lib::registry::Registry;

fn input_dir() -> std::path::PathBuf {
    Path::new(env!("CARGO_MANIFEST_DIR")).join("../input")
}

fn load(name: &str) -> (ir::FuncDef, HashMap<String, ir::EnumDef>) {
    let base = input_dir();
    let enums = parser::enums::load_enums(&base.join("enums.yaml"));
    let mut func = parser::yaml::parse_yaml(&base.join(format!("{name}/{name}.yaml")));
    let parsed = parser::c_source::parse_c_source(&base.join(format!("{name}/{name}.c")));
    parser::c_source::wire_parsed_source(&mut func, &parsed);
    (func, enums)
}

/// Generate the C streaming section for one function, exactly as the C backend does.
fn stream_c(name: &str) -> String {
    let (func, enums) = load(name);
    let registry = Registry::from_dir(&input_dir());
    let helpers = HelperRegistry::from_dir(&input_dir().join("helpers"));
    c_stream::generate(&func, &enums, &registry, &helpers)
}

/// The body of the first definition whose signature line matches `needle`,
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

/// Strip `/* ... */` comments — the emitted prose mentions output names freely.
fn strip_comments(s: &str) -> String {
    let mut out = String::with_capacity(s.len());
    let b = s.as_bytes();
    let mut i = 0;
    while i < b.len() {
        if b[i] == b'/' && i + 1 < b.len() && b[i + 1] == b'*' {
            match s[i + 2..].find("*/") {
                Some(e) => i = i + 2 + e + 2,
                None => break,
            }
        } else {
            out.push(b[i] as char);
            i += 1;
        }
    }
    out
}

/// Every function in the corpus that streams (the emitters' own gate decides).
fn streaming_funcs() -> Vec<String> {
    let mut names = Vec::new();
    for entry in std::fs::read_dir(input_dir()).expect("input dir") {
        let path = entry.expect("dir entry").path();
        if !path.is_dir() {
            continue;
        }
        let name = path.file_name().unwrap().to_str().unwrap().to_string();
        if !path.join(format!("{name}.yaml")).exists() || !path.join(format!("{name}.c")).exists() {
            continue;
        }
        let (func, _) = load(&name);
        if func.streaming {
            names.push(name);
        }
    }
    names.sort();
    names
}

// ---------------------------------------------------------------------------
// The merged shape
// ---------------------------------------------------------------------------

#[test]
fn loop_tier_emits_one_core_and_three_thin_entries() {
    let src = stream_c("cdlhammer");
    assert_eq!(
        src.matches("static TA_RetCode TA_CDLHAMMER_OpenImpl(").count(),
        1,
        "the core is emitted exactly once"
    );
    assert!(
        src.contains("int outStride )"),
        "the core takes an outStride parameter"
    );

    // Every entry delegates; none carries the transcribed algorithm. The public
    // fill goes through the anchored seam rather than straight down, which is
    // what gives that seam a caller for all 175 rather than only the 16 that
    // something composes over.
    for (wrapper, callee) in [
        ("TA_RetCode TA_CDLHAMMER_OpenInternal(", "TA_CDLHAMMER_OpenImpl("),
        ("TA_RetCode TA_CDLHAMMER_OpenAndFillInternal(", "TA_CDLHAMMER_OpenImpl("),
        ("TA_RetCode TA_CDLHAMMER_OpenAndFill(", "TA_CDLHAMMER_OpenAndFillInternal("),
    ] {
        let body = body_of(&src, wrapper);
        assert!(
            body.contains(callee),
            "{wrapper} must delegate to {callee}, got:\n{body}"
        );
        assert!(
            !body.contains("BodyPeriodTotal"),
            "{wrapper} must not re-transcribe the algorithm"
        );
    }
}

#[test]
fn scalar_sink_replaces_last_value_locals() {
    let src = stream_c("cdlhammer");
    // The `lastValue_*` scalar was the old scalar-mode output sink. The merged
    // core writes through the caller's pointer at stride 0 instead, so the
    // concept is gone from the emitted C entirely.
    assert!(
        !src.contains("lastValue_"),
        "lastValue_* sinks are replaced by the stride-0 slot"
    );
}

#[test]
fn output_writes_are_stride_scaled() {
    let src = stream_c("cdlhammer");
    let core = strip_comments(&body_of(&src, "TA_CDLHAMMER_OpenImpl("));
    assert!(
        core.contains("outInteger[outIdx++ * outStride] = 100;"),
        "per-bar output writes scale by outStride, got:\n{core}"
    );
}

#[test]
fn compound_stride_indices_are_parenthesized() {
    // `outIdx++ * outStride` is correct unparenthesized (`++` binds tighter),
    // but a compound index MUST be wrapped: `*outNBElement - 1 * outStride`
    // would silently mean `*outNBElement - outStride`. Every emitted subscript
    // containing an operator has to carry its own parentheses.
    let mut bad = Vec::new();
    for name in streaming_funcs() {
        let src = stream_c(&name);
        for (pos, _) in src.match_indices("* outStride]") {
            let open = src[..pos].rfind('[').expect("subscript opens");
            let idx = &src[open + 1..pos];
            let bare = idx.trim().trim_end_matches('*').trim();
            // A compound index is safe only if it is wrapped in its own parens.
            let compound = bare.contains(" - ") || bare.contains(" + ");
            if compound && !(bare.starts_with('(') && bare.ends_with(')')) {
                bad.push(format!("{name}: [{idx}* outStride]"));
            }
        }
    }
    bad.sort();
    bad.dedup();
    assert!(bad.is_empty(), "unparenthesized compound stride index:\n  {}", bad.join("\n  "));
}

#[test]
fn open_internal_passes_stride_zero_and_fill_passes_one() {
    let src = stream_c("cdlhammer");
    let scalar = body_of(&src, "TA_RetCode TA_CDLHAMMER_OpenInternal(");
    // The stride is now chosen one frame down, on the anchored seam the public
    // fill delegates to; the public frame supplies the anchor instead.
    let fill = body_of(&src, "TA_RetCode TA_CDLHAMMER_OpenAndFillInternal(");
    let public_fill = body_of(&src, "TA_RetCode TA_CDLHAMMER_OpenAndFill(");
    assert!(scalar.contains(", 0 );"), "Open passes stride 0:\n{scalar}");
    assert!(fill.contains(", 1 );"), "the anchored fill passes stride 1:\n{fill}");
    assert!(
        public_fill.contains("return TA_CDLHAMMER_OpenAndFillInternal( "),
        "the public fill delegates rather than choosing a stride:\n{public_fill}"
    );
}

// ---------------------------------------------------------------------------
// Fill-only validation must MOVE to the wrapper, not vanish
// ---------------------------------------------------------------------------

#[test]
fn fill_wrapper_keeps_out_meta_null_checks() {
    let src = stream_c("cdlhammer");
    let fill = body_of(&src, "TA_RetCode TA_CDLHAMMER_OpenAndFill(");
    assert!(
        fill.contains("!outBegIdx") && fill.contains("!outNBElement"),
        "OpenAndFill still rejects NULL out-meta pointers:\n{fill}"
    );
}

#[test]
fn fill_wrapper_keeps_output_aliasing_guards() {
    // #108 / #130: the fill's capture epilogue reads the input tail AFTER writing
    // the outputs, so an output may alias neither an input nor another output.
    // The scalar path writes only to caller scalars and never had this hazard.
    let src = stream_c("accbands");
    let fill = body_of(&src, "TA_RetCode TA_ACCBANDS_OpenAndFill(");
    assert!(
        fill.contains("(const void *)outRealUpperBand == (const void *)inHigh"),
        "output-vs-input aliasing guard survives:\n{fill}"
    );
    assert!(
        fill.contains("(const void *)outRealUpperBand == (const void *)outRealMiddleBand"),
        "output-vs-output aliasing guard survives:\n{fill}"
    );
    // ...and the scalar wrapper must NOT inherit them: its sink is a local.
    let scalar = body_of(&src, "TA_RetCode TA_ACCBANDS_OpenInternal(");
    assert!(
        !scalar.contains("(const void *)"),
        "Open has no aliasing hazard and must not pay for the check:\n{scalar}"
    );
}

// ---------------------------------------------------------------------------
// Tier exemptions
// ---------------------------------------------------------------------------

#[test]
fn period_bank_keeps_two_bodies() {
    // MAVP's two modes are DIFFERENT algorithms, not one body with two sinks.
    let src = stream_c("mavp");
    assert!(
        !src.contains("TA_MAVP_OpenImpl("),
        "the period-bank tier is exempt from the merge"
    );
    assert!(src.contains("TA_MAVP_OpenInternal("));
    assert!(src.contains("TA_MAVP_OpenAndFill("));
    // The fill arm's per-bar Update loop is what makes it a different algorithm.
    let fill = body_of(&src, "TA_RetCode TA_MAVP_OpenAndFill(");
    assert!(
        fill.contains("_Update("),
        "MAVP's fill drives sub-Updates per bar:\n{fill}"
    );
}

#[test]
fn dispatch_keeps_its_own_bodies() {
    // MA's arms delegate the fill to each sub's PUBLIC OpenAndFill, which writes
    // the caller's array directly; there is no stride to thread through.
    let src = stream_c("ma");
    assert!(
        !src.contains("TA_MA_OpenImpl("),
        "the dispatch tier is exempt from the merge"
    );
    let fill = body_of(&src, "TA_RetCode TA_MA_OpenAndFill(");
    assert!(
        fill.contains("_OpenAndFill("),
        "MA's fill arms call the sub's OpenAndFill:\n{fill}"
    );
}

/// The dispatch tier's three open entry points come out of ONE emitter over a
/// mode list (issue #204), so nothing but this pins what each mode is supposed
/// to differ in. It has to be pinned here because a guard that migrates to the
/// wrong mode changes only which calls are REJECTED, and every value gate in
/// the tree compares outputs over the ranges a call accepts — an acceptance-set
/// change is invisible to all of them.
#[test]
fn dispatch_open_modes_differ_only_where_intended() {
    let src = stream_c("ma");
    let scalar = body_of(&src, "TA_RetCode TA_MA_OpenInternal(");
    let fill = body_of(&src, "TA_RetCode TA_MA_OpenAndFill(");
    let internal = body_of(&src, "TA_RetCode TA_MA_OpenAndFillInternal(");
    let modes = [
        ("OpenInternal", &scalar),
        ("OpenAndFill", &fill),
        ("OpenAndFillInternal", &internal),
    ];

    // The spine every mode shares. The #180 bound is the one that must agree
    // with the batch call bit for bit.
    for (what, body) in modes {
        assert!(
            body.contains("if( historyLen > TA_MAX_INDEX + 1 ) return TA_OUT_OF_RANGE_END_INDEX;"),
            "{what} lost the TA_MAX_INDEX bound:\n{body}"
        );
        assert!(
            body.contains("TA_Malloc( sizeof(*sp) )") && body.contains("if( !sp ) return TA_ALLOC_ERR;"),
            "{what} lost the handle allocation:\n{body}"
        );
        assert!(body.contains("TA_Free( sp );"), "{what} lost the free-on-error tail:\n{body}");
    }

    // The batch API's out-meta pair belongs to the two fills alone.
    assert!(!scalar.contains("outBegIdx"), "the scalar open has no out-meta:\n{scalar}");
    for (what, body) in [("OpenAndFill", &fill), ("OpenAndFillInternal", &internal)] {
        assert!(
            body.contains("!outBegIdx || !outNBElement"),
            "{what} must null-check the out-meta pair:\n{body}"
        );
    }

    // The aliasing rejection belongs to the PUBLIC fill alone: the internal one
    // is only ever called for a sub-call already proven non-aliasing (#192), and
    // the scalar sink cannot alias an input.
    assert!(fill.contains("(const void *)"), "the public fill must reject aliasing:\n{fill}");
    assert!(
        !internal.contains("(const void *)"),
        "OpenAndFillInternal must NOT carry the aliasing guard:\n{internal}"
    );
    assert!(!scalar.contains("(const void *)"), "the scalar open has no aliasing hazard:\n{scalar}");

    // Only the startIdx-anchored fill clamps its anchor; the public one has no
    // startIdx at all.
    assert!(
        internal.contains("if( startIdx > fillLb ) fillLb = startIdx;"),
        "OpenAndFillInternal must clamp the fill anchor up to startIdx:\n{internal}"
    );
    assert!(!fill.contains("startIdx"), "the public fill is anchored at bar 0:\n{fill}");

    // ORDER, per mode. The clamp and the history re-check are one edit: clamping
    // the anchor up to `startIdx` and then testing the history against the
    // PRE-clamp anchor lets an anchor the history does not reach through, and the
    // count it publishes is `historyLen - anchor` — negative in C, a `usize`
    // underflow in Rust. That is the defect #241 shipped and 96d1052f8 fixed.
    //
    // Asserted per mode, on a sliced body, because the three lines are emitted
    // BYTE-IDENTICALLY by the scalar and the anchored fill: a whole-file
    // `contains` is satisfied by whichever arm still happens to be right, so it
    // stays green when only one of them regresses. Measured, not assumed —
    // reordering the scalar arm alone left all 280 generator tests passing.
    let clamp = "if( startIdx > fillLb ) fillLb = startIdx;";
    let recheck = "if( historyLen < fillLb + 1 ) { TA_Free( sp ); return TA_INSUFFICIENT_HISTORY; }";
    for (what, body) in [("OpenInternal", &scalar), ("OpenAndFillInternal", &internal)] {
        let c = body
            .find(clamp)
            .unwrap_or_else(|| panic!("{what} lost the startIdx clamp:\n{body}"));
        let r = body
            .find(recheck)
            .unwrap_or_else(|| panic!("{what} lost the post-clamp history re-check:\n{body}"));
        assert!(
            c < r,
            "{what} re-checks the history BEFORE clamping the anchor to startIdx, so an \
             anchor past the history publishes a handle instead of rejecting:\n{body}"
        );
    }
    // The public fill has no startIdx to clamp with, so it checks once and that
    // is the whole of it.
    assert!(fill.contains(recheck), "the public fill must still check the history:\n{fill}");

    // Each mode delegates to the callee entry point that matches it.
    assert!(
        scalar.contains("TA_SMA_OpenInternal(") && !scalar.contains("_OpenAndFill"),
        "the scalar arms open the sub's OpenInternal:\n{scalar}"
    );
    assert!(
        fill.contains("TA_SMA_OpenAndFill(") && !fill.contains("_OpenAndFillInternal("),
        "the public fill arms call the sub's public OpenAndFill:\n{fill}"
    );
    assert!(
        internal.contains("TA_SMA_OpenAndFillInternal("),
        "the internal fill arms call the sub's OpenAndFillInternal:\n{internal}"
    );
}

// ---------------------------------------------------------------------------
// Composed tier: the scratch copy-out is the one place stride needs a branch
// ---------------------------------------------------------------------------

#[test]
fn composed_copy_out_is_stride_guarded() {
    // Fill mode aliases `sc_<out>` straight onto the caller's own output array
    // (issue #205: no allocation, no copy — the batch tail already wrote the
    // final values there). Scalar mode keeps the owned history-sized scratch
    // and takes its last element.
    let src = stream_c("adxr");
    let core = body_of(&src, "TA_ADXR_OpenImpl(");
    assert!(
        core.contains("if( outStride ) sc_outReal = outReal;"),
        "fill mode aliases the scratch onto the caller's array:\n{core}"
    );
    assert!(
        core.contains("if( !outStride ) outReal[0] = sc_outReal[dummyNBElement - 1];"),
        "scalar arm takes the last scratch element:\n{core}"
    );
    assert!(
        core.contains("if( !outStride ) TA_Free( sc_outReal );"),
        "scratch is freed only when this call actually owns it:\n{core}"
    );
}

// ---------------------------------------------------------------------------
// Nullable outputs
// ---------------------------------------------------------------------------

#[test]
fn nullable_output_stays_null_through_the_scalar_wrapper() {
    // MAMA's outFAMA may be NULL (discarded). The core's writes are NULL-guarded,
    // so the sink must stay NULL rather than becoming &lastValue.
    let src = stream_c("mama");
    let scalar = body_of(&src, "TA_RetCode TA_MAMA_OpenInternal(");
    assert!(
        scalar.contains("outFAMA ?"),
        "a nullable output's sink passes NULL through:\n{scalar}"
    );
}

// ---------------------------------------------------------------------------
// The invariant that catches the whole bug class
// ---------------------------------------------------------------------------

#[test]
fn no_output_array_escapes_the_core_unscaled() {
    // Every use of an output inside a core must be a subscript (which carries
    // `* outStride`) or an explicitly stride-guarded bulk copy. A bare use --
    // passing the array to a callee, memcpy'ing into it, taking its address --
    // would write N elements into the 1-element scalar sink. This is exactly how
    // the prototypes failed in all three languages, so it is pinned here.
    let mut offenders: Vec<String> = Vec::new();
    for name in streaming_funcs() {
        let src = stream_c(&name);
        let upper = name.to_uppercase();
        let needle = format!("TA_{upper}_OpenImpl(");
        if !src.contains(&needle) {
            continue; // exempt tier
        }
        let core = strip_comments(&body_of(&src, &needle));
        let (func, _) = load(&name);
        for out in &func.outputs {
            let o = &out.name;
            for (pos, _) in core.match_indices(o.as_str()) {
                // skip `sc_<out>` scratch and any longer identifier ending in this name
                let pre = core[..pos].chars().next_back().unwrap_or(' ');
                if pre.is_alphanumeric() || pre == '_' {
                    continue;
                }
                let rest = &core[pos + o.len()..];
                let next = rest.chars().next().unwrap_or(' ');
                if next.is_alphanumeric() || next == '_' {
                    continue; // a longer identifier, e.g. outRealUpperBand vs outReal
                }
                if rest.trim_start().starts_with('[') {
                    continue; // subscripted: carries * outStride
                }
                let line_start = core[..pos].rfind('\n').map_or(0, |x| x + 1);
                let line_end = core[pos..].find('\n').map_or(core.len(), |x| pos + x);
                let line = core[line_start..line_end].trim();
                if line.contains("outStride") {
                    continue; // explicitly stride-guarded (composed copy-out)
                }
                if line.contains("return TA_BAD_PARAM") {
                    continue; // a validation predicate, not a write
                }
                if line.contains("!= NULL") || line.contains("== NULL") {
                    continue; // a nullable output's discard guard, not a write
                }
                offenders.push(format!("{name}: {line}"));
            }
        }
    }
    offenders.sort();
    offenders.dedup();
    assert!(
        offenders.is_empty(),
        "output arrays used unscaled inside a core:\n  {}",
        offenders.join("\n  ")
    );
}

#[test]
fn every_mergeable_function_has_exactly_one_core() {
    // Guards against a tier silently regressing to two bodies.
    let exempt = ["ma", "mavp"];
    let mut missing = Vec::new();
    for name in streaming_funcs() {
        if exempt.contains(&name.as_str()) {
            continue;
        }
        let src = stream_c(&name);
        let needle = format!("TA_{}_OpenImpl(", name.to_uppercase());
        if !src.contains(&needle) {
            missing.push(name);
        }
    }
    assert!(
        missing.is_empty(),
        "these streaming functions did not get a merged core: {missing:?}"
    );
}

#[test]
fn identity_fast_path_short_circuits_at_stride_zero() {
    // The identity arm (period 1 => the output IS the input, shifted) must not
    // pay for the fill loop when there is nowhere to fill. At stride 0 every
    // iteration stores to slot 0 of the one-element sink, so a whole-history
    // loop would leave `Open` O(history) where it used to be O(1) — correct,
    // but linear in the warm-up for the 8 functions whose period-1 arm is
    // reachable (EMA, DEMA, TEMA, T3, KAMA, HMA, WMA, VWMA).
    let src = stream_c("ema");
    let core = strip_comments(&body_of(&src, "TA_EMA_OpenImpl("));
    assert!(
        core.contains("outReal[0] = inReal[historyLen - 1];"),
        "the identity arm takes the last bar directly at stride 0:\n{core}"
    );
    // ...and the fill arm keeps the plain (unscaled) subscript, since stride is
    // 1 there by construction.
    assert!(
        core.contains("outReal[fillIdx] = inReal[fillLb + fillIdx];"),
        "the identity fill arm indexes plainly:\n{core}"
    );
}
