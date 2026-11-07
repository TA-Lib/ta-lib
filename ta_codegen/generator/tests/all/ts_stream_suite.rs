//! Render pins for the TypeScript stream emitter (`backends/ts_stream.rs`):
//! the Open family's stride mechanics, the handle's field visibility and deep
//! copy, the fused sites, and the frames' allocation shape.

use crate::common;
use std::collections::{BTreeSet, HashMap};
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

fn streaming_funcs() -> Vec<String> {
    let mut out: Vec<String> = common::discover_indicators()
        .into_iter()
        .filter(|n| load(n).0.streaming)
        .collect();
    out.sort();
    assert!(out.len() >= 200, "the corpus sweep found only {} functions", out.len());
    out
}

fn section_of(full: &str) -> Option<String> {
    full.find("/**** Streaming API *****/").map(|at| full[at..].to_string())
}

fn section(name: &str) -> String {
    let (func, enums) = load(name);
    let full = backends::ts::generate(&func, &enums, common::make_registry(), common::make_helpers());
    section_of(&full).unwrap_or_else(|| panic!("{name}: stream section missing"))
}

/// The brace-balanced body of the first definition whose text contains
/// `needle`, from the first brace that ends its line: an inline object type in
/// the signature is passed over.
fn body_of(src: &str, needle: &str) -> String {
    let i = src.find(needle).unwrap_or_else(|| panic!("no definition matching {needle:?}"));
    let j = src[i..].find("{\n").expect("definition has a body") + i;
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

/// The parameter list of the first definition whose text contains `needle`.
fn signature_of<'a>(src: &'a str, needle: &str) -> &'a str {
    let at = src.find(needle).unwrap_or_else(|| panic!("no definition matching {needle:?}"));
    &src[at..at + src[at..].find(')').expect("signature closes")]
}

#[test]
fn every_stream_flagged_function_emits_a_ts_stream() {
    let registry = common::make_registry();
    let helpers = common::make_helpers();
    let mut n = 0usize;
    for name in streaming_funcs() {
        let (func, enums) = load(&name);
        let full = backends::ts::generate(&func, &enums, registry, helpers);
        assert!(section_of(&full).is_some(), "{name}: declared streamable but no TS stream section");
        n += 1;
    }
    assert!(n >= 200, "only {n} streams swept");
}

#[test]
fn ts_open_family_is_one_core_with_four_entries() {
    let s = section("cdlhammer");
    assert_eq!(s.matches("\nfunction cdlhammerOpenImpl(").count(), 1, "the numerics are emitted exactly once");
    assert!(signature_of(&s, "\nfunction cdlhammerOpenImpl(").contains("outStride: number"), "the numerics take a stride");
    for (entry, callee) in [
        ("export function cdlhammerOpenInternal(", "cdlhammerOpenImpl("),
        ("export function cdlhammerOpenAndFillInternal(", "cdlhammerOpenImpl("),
        ("export function cdlhammerOpenAndFillPositional(", "cdlhammerOpenAndFillInternal("),
        ("export function cdlhammerOpenPositional(", "cdlhammerOpenInternal("),
        ("export function cdlhammerOpenAndFill(", "cdlhammerOpenAndFillPositional("),
        ("export function cdlhammerOpen(", "cdlhammerOpenPositional("),
    ] {
        let body = body_of(&s, entry);
        assert!(body.contains(callee), "{entry} delegates to {callee}:\n{body}");
        assert!(!body.contains("BodyPeriodTotal"), "{entry} must not re-transcribe the algorithm");
    }
    assert!(!s.contains("cdlhammerOpenAndFillImpl("), "a merged function has no second body");
    let open = body_of(&s, "export function cdlhammerOpenPositional(");
    assert!(open.contains("float64(inOpen)"), "the positional open copies a non-Float64Array input once:\n{open}");
}

#[test]
fn ts_plain_open_uses_a_one_element_sink_at_stride_zero() {
    let s = section("cdlhammer");
    let body = body_of(&s, "export function cdlhammerOpenInternal(");
    assert!(body.contains("new Int32Array(1)"), "an int output sinks into a 1-element array:\n{body}");
    assert!(body.contains(", 0);"), "the plain open passes stride 0:\n{body}");
    let fill = body_of(&s, "export function cdlhammerOpenAndFillInternal(");
    assert!(fill.contains(", 1);"), "the anchored fill passes stride 1:\n{fill}");
    assert!(s.contains("outInteger[((outIdx++ * outStride) | 0)] = 100;"), "per-bar output writes scale by the stride");
}

#[test]
fn ts_public_fill_keeps_the_overlap_guards() {
    let s = section("accbands");
    let body = body_of(&s, "export function accbandsOpenAndFillPositional(");
    for (guard, names) in [
        ("overlaps(outRealUpperBand, inHighF64)", "'outUpperBand', 'prices.high'"),
        ("overlaps(outRealUpperBand, outRealMiddleBand)", "'outUpperBand', 'outMiddleBand'"),
    ] {
        let needle = format!("if (guardOutLen > 0 && {guard}) {{\n    throw sharedMemory('ACCBANDS openAndFill', {names});");
        assert!(body.contains(&needle), "{guard} survives on the public fill, naming its pair:\n{body}");
    }
    for seam in ["export function accbandsOpenInternal(", "export function accbandsOpenAndFillInternal("] {
        assert!(!body_of(&s, seam).contains("overlaps("), "{seam} must not carry the guard");
    }
}

#[test]
fn ts_exempt_tiers_keep_a_body_per_entry() {
    for name in ["ma", "mavp"] {
        let s = section(name);
        let sig = signature_of(&s, &format!("\nfunction {name}OpenImpl("));
        assert!(!sig.contains("outStride"), "{name}OpenImpl is exempt: its open body is not the strided numerics:\n{sig}");
        assert!(s.contains(&format!("\nfunction {name}OpenAndFillImpl(")), "{name} keeps a separate fill body");
    }
}

#[test]
fn ts_identity_fast_path_short_circuits_at_stride_zero() {
    let s = section("t3");
    assert!(s.contains("if (outStride === 0) {"), "identity arm short-circuits at stride 0");
    assert!(s.contains("outReal[0] = inReal[historyLen - 1];"), "stride-0 arm takes the last bar directly");
    assert!(s.contains("outReal[fillIdx] = inReal[fillLb + fillIdx];"), "fill arm indexes plainly");
}

/// A step that commits the bar before the window value exists breaks "a throw
/// changes nothing", and nothing provokes a failure inside the evaluation.
#[test]
fn ts_mavp_window_step_evaluates_before_it_commits() {
    let s = section("mavp");
    let step = body_of(&s, "\nfunction mavpStepImpl(");
    let eval = step.find("mavpEvalWindow(").expect("the step has a window arm");
    let commit = step.find("sp.tape[sp.tapePos] =").expect("the step commits the bar");
    assert!(eval < commit, "the window value is computed before the tape takes the bar:\n{step}");
    let mode = body_of(&s, "\nfunction mavpWindowMode(");
    assert!(mode.contains("case MAType.ALMA:"), "the window labels reach the TS switch:\n{mode}");
}

/// The handle's state is public and `@internal` (D16): the frames are
/// module-level functions, which reach no `private` or `#` member, and the
/// tag is what keeps the fields out of the published types.
#[test]
fn every_handle_field_is_public_and_internal() {
    let mut fields = 0usize;
    for name in streaming_funcs() {
        let s = section(&name);
        let class_at = s.find("\nexport class ").expect("a stream class");
        let class = body_of(&s[class_at..], "export class ");
        // A constructor stripped as @internal leaves the published class an implicit public `new ()`.
        let ctor = class.find("\n\n  private constructor(").unwrap_or_else(|| panic!("{name}: no private constructor"));
        assert!(class.contains("\n  /** @internal */\n  static of(core: Core)"), "{name}: no @internal factory");
        let head = &class[..ctor];
        let lines: Vec<&str> = head.lines().collect();
        for (k, l) in lines.iter().enumerate() {
            if l.starts_with("  ") && !l.starts_with("   ") && l.trim_end().ends_with(';') {
                assert!(
                    k > 0 && lines[k - 1].trim() == "/** @internal */",
                    "{name}: field `{}` is not tagged @internal",
                    l.trim()
                );
                assert!(!l.contains("private ") && !l.trim_start().starts_with('#'), "{name}: `{}` is private", l.trim());
                fields += 1;
            }
        }
    }
    assert!(fields >= 2000, "only {fields} handle fields seen");
}

/// `clone()` copies every typed-array field and clones every sub-stream; a
/// shared buffer is latent for one bar and wrong from the second.
#[test]
fn clone_deep_copies_every_buffer_and_sub_stream() {
    let (mut arrays, mut subs) = (0usize, 0usize);
    for name in streaming_funcs() {
        let s = section(&name);
        let class_at = s.find("\nexport class ").expect("a stream class");
        let class = body_of(&s[class_at..], "export class ");
        let clone = body_of(&class, "\n  clone(): ");
        for l in class.lines() {
            let l = l.trim();
            let Some((field, rest)) = l.split_once(": ") else { continue };
            if !field.chars().all(|c| c.is_ascii_alphanumeric() || c == '_' || c == '!') {
                continue;
            }
            let field = field.trim_end_matches('!');
            if rest.starts_with("Float64Array = ") || rest.starts_with("Int32Array = ") {
                assert!(clone.contains(&format!("c.{field} = this.{field}.slice();")), "{name}: clone shares `{field}`");
                arrays += 1;
            } else if rest.contains("Stream") && rest.ends_with(';') {
                assert!(
                    clone.contains(&format!("this.{field}.clone()")) || clone.contains(&format!("this.{field}[bankIdx].clone()")),
                    "{name}: clone shares sub-stream `{field}`:\n{clone}"
                );
                subs += 1;
            }
        }
    }
    assert!(arrays >= 300, "only {arrays} array fields seen");
    assert!(subs >= 30, "only {subs} sub-stream fields seen");
}

/// The stream fuses `a*b+c` exactly where Java's does (H9, H10): the same
/// analysis, and the `sp.` / `cur_` prefixes `fma::stream_base` strips. Any
/// other spelling moves a site by an ulp with nothing else pointing at it. A
/// site with `fma`'s body copied in makes one `fmaSlow` call.
#[test]
fn the_stream_fuses_where_java_s_does() {
    let registry = common::make_registry();
    let helpers = common::make_helpers();
    let fused_call = |s: &str, needle: &str| {
        s.match_indices(needle)
            .filter(|(i, _)| !s[..*i].chars().next_back().is_some_and(|c| c.is_alphanumeric() || c == '_'))
            .count()
    };
    let mut fusing = 0usize;
    for name in streaming_funcs() {
        let (func, enums) = load(&name);
        let ts = section_of(&backends::ts::generate(&func, &enums, registry, helpers)).expect("ts stream");
        let java = section_of(&backends::java::generate(&func, &enums, registry, helpers)).expect("java stream");
        let (t, j) = (fused_call(&ts, "fma(") + fused_call(&ts, "fmaSlow("), fused_call(&java, "Math.fma("));
        assert_eq!(t, j, "{name}: the TS stream fuses {t} sites, Java's {j}");
        fusing += usize::from(t > 0);
    }
    assert!(fusing >= 30, "only {fusing} streams fuse at all");
}

/// No per-bar allocation in a committing or peeking frame: a multi-output
/// sub-stream writes a module-level sink instead of a fresh `<N>Out`.
#[test]
fn no_step_or_peek_frame_allocates_a_sink() {
    let mut frames = 0usize;
    let mut omitted_sinks = 0usize;
    let mut scratch: BTreeSet<String> = BTreeSet::new();
    for name in streaming_funcs() {
        let s = section(&name);
        let base = backends::common::camel_words(&name.to_uppercase());
        let mut bodies = vec![body_of(&s, &format!("\nfunction {base}StepImpl(")), body_of(&s, "\n  peek(")];
        for tape in ["StepTape(", "PeekTape("] {
            let def = format!("export function {base}{tape}");
            if s.contains(&def) {
                bodies.push(body_of(&s, &def));
            }
        }
        for b in &bodies {
            // `??= new` allocates once per module, on first use; the public
            // peek allocates only the sink its caller did not pass.
            let allocates = b.match_indices("new ").any(|(i, _)| {
                let omitted = b[..i].ends_with("out = ")
                    && b[..i].trim_end_matches("out = ").trim_end().ends_with("if (out === undefined) {");
                if omitted {
                    omitted_sinks += 1;
                }
                !b[..i].ends_with("??= ") && !omitted && b[i + 4..].split('(').next().is_some_and(|t| t.ends_with("Out"))
            });
            assert!(!allocates, "{name}: a frame allocates a sink per bar:\n{b}");
            frames += 1;
        }
        if s.contains("??= new ") {
            scratch.insert(name);
        }
    }
    assert!(frames >= 400, "only {frames} frames checked");
    assert!(!scratch.is_empty(), "no frame drives a multi-output sub-stream, so the sweep checks nothing");
    assert!(omitted_sinks >= 20, "only {omitted_sinks} multi-output peeks allocate an omitted sink");
}

/// Issue #205's fill-mode scratch aliases the caller's array, so no copy-back
/// survives. The negative alone would pass on any re-render of the copy, so it
/// is paired with the positive that replaces it.
#[test]
fn ts_composed_scratch_aliases_the_caller_at_stride_one() {
    for (name, out) in [("adxr", "outReal"), ("stoch", "outSlowK"), ("stoch", "outSlowD")] {
        let s = section(name);
        assert!(
            s.contains(&format!("const sc_{out} = outStride === 1 ? {out} : new Float64Array(historyLen);")),
            "{name}: fill mode aliases the scratch onto the caller's {out}"
        );
        assert!(
            !s.contains(&format!("{out}.set(sc_{out}")),
            "{name}: no copy-back into {out} survives; the scratch already IS {out} at stride 1"
        );
    }
}

/// The handle's fixed-size accumulator fields: a typed array the BATCH body
/// declares with a literal length. Off the emitted code, never a name list.
fn accumulator_fields(section: &str, batch: &str) -> BTreeSet<String> {
    let mut out = BTreeSet::new();
    for line in section.lines() {
        if !line.starts_with("  ") || line.starts_with("   ") {
            continue;
        }
        let Some((name, rest)) = line.trim().split_once(": ") else { continue };
        let Some(ty) = ["Float64Array", "Int32Array"].into_iter().find(|t| rest.starts_with(t)) else { continue };
        if !name.chars().all(|c| c.is_alphanumeric() || c == '_') {
            continue;
        }
        let literal = |decl: &str| {
            batch.match_indices(decl).any(|(i, _)| {
                batch[i + decl.len()..].split_once(')').is_some_and(|(n, _)| n.parse::<u32>().is_ok())
            })
        };
        if ["const", "let"].iter().any(|kw| literal(&format!("{kw} {name} = new {ty}("))) {
            out.insert(name.to_string());
        }
    }
    out
}

/// No peek copies its handle — swept over the whole corpus.
///
/// Structural, because no value gate can see it: a peek that copied and then
/// wrote the copy would still answer correctly. What it costs is the frame's
/// flat-in-period overhead, which is what the frame is for. Stores into the
/// handle are `no_managed_peek_writes_its_handle`'s.
#[test]
fn no_ts_peek_copies_the_handle() {
    let registry = common::make_registry();
    let helpers = common::make_helpers();
    let mut swept = 0usize;
    let mut frames = 0usize;
    let mut handle_free = 0usize;
    let mut tapes = 0usize;
    let mut sink_sites: BTreeSet<String> = BTreeSet::new();
    let mut bounded: BTreeSet<String> = BTreeSet::new();
    let mut fixtures = 0usize;
    let mut fully_shadowed: BTreeSet<String> = BTreeSet::new();
    let mut offenders: Vec<String> = Vec::new();
    let mut omitted_sinks = 0usize;
    for name in streaming_funcs() {
        let (func, enums) = load(&name);
        let full = backends::ts::generate(&func, &enums, registry, helpers);
        let at = full.find("/**** Streaming API *****/").unwrap_or_else(|| panic!("{name}: stream section missing"));
        let (batch, s) = full.split_at(at);
        let peek = body_of(s, "\n  peek(");
        swept += 1;
        if peek.contains("\n    const sp = this;\n") {
            frames += 1;
        } else if !peek
            .lines()
            .map(str::trim)
            .filter(|l| !(l.starts_with("//") || l.starts_with("/*") || l.starts_with('*')))
            .flat_map(|l| l.split(|c: char| !(c.is_alphanumeric() || c == '_')))
            .any(|w| w == "this" || w == "sp")
        {
            // A peek that never touches its handle has nothing to copy.
            handle_free += 1;
        }
        // A tape peek reads the handle it is handed, so it is a frame as well.
        let base = backends::common::camel_words(&name.to_uppercase());
        let mut bodies = vec![peek.clone()];
        let tape = format!("export function {base}PeekTape(");
        if s.contains(&tape) {
            bodies.push(body_of(s, &tape));
            tapes += 1;
        }
        let accs = accumulator_fields(s, batch);
        // A `synth<n>` fixture is a construct probe copied into input/ by
        // scripts/synth_gate.py, never a shipped function.
        let fixture = name.starts_with("synth");
        fixtures += usize::from(fixture);
        let mut prev = "";
        for l in bodies.iter().flat_map(|b| b.lines()).map(str::trim) {
            if l.starts_with("//") || l.starts_with("/*") || l.starts_with('*') {
                continue;
            }
            // The sink a composed frame allocates once per module for a
            // multi-output sub-stream; pinned to an exact set below.
            let composed_sink = l.contains("Out ??= new ") && l.ends_with("Out());");
            if composed_sink {
                sink_sites.insert(name.clone());
            }
            // The sink a caller did not pass, which the peek returns.
            let omitted_sink = prev == "if (out === undefined) {" && l.starts_with("out = new ") && l.ends_with("Out();");
            omitted_sinks += usize::from(omitted_sink);
            prev = l;
            if omitted_sink {
                continue;
            }
            // ONE copy is contract-legal: a fixed-size accumulator, whose copy
            // cannot grow with the period. The emitter reaches it only where it
            // cannot shadow the write in place, and no shipped function is in
            // that position, so a shipped one that copies has regressed from the
            // shadow into the fallback.
            let copied = l.split_once(" = sp.").and_then(|(_, rhs)| rhs.strip_suffix(".slice();"));
            if let Some(f) = copied {
                if fixture && accs.contains(f) {
                    bounded.insert(format!("{name}.{f}"));
                } else if accs.contains(f) {
                    offenders.push(format!(
                        "{name}: a SHIPPED peek copies the accumulator {f} rather than shadowing the write: {l}"
                    ));
                } else {
                    offenders.push(format!("{name}: copies the handle's {f}: {l}"));
                }
                continue;
            }
            let allocates = (l.contains("new ") && !l.starts_with("throw new ") && !composed_sink)
                || [".slice(", ".subarray(", "Array.from(", ".concat(", "[...", "structuredClone(", ".clone()", "Object.assign("]
                    .iter()
                    .any(|c| l.contains(c));
            if allocates {
                offenders.push(format!("{name}: {l}"));
            }
        }
        // The frame must READ an accumulator: a field it never names is no
        // evidence about the copy either way.
        if accs.iter().any(|f| peek.contains(&format!("sp.{f}["))) {
            fully_shadowed.insert(name.clone());
        }
    }
    assert!(swept >= 200, "only {swept} peek(s) swept");
    assert_eq!(frames + handle_free, swept, "{frames} of {swept} peek(s) run a frame against the handle, {handle_free} never touch it");
    assert!(frames >= 200, "only {frames} peek(s) run a frame");
    assert!(tapes >= 15, "only {tapes} tape peek(s) swept");
    assert!(
        fully_shadowed.len() >= 21,
        "only {} handle(s) have a peek frame that reads an accumulator — the sweep is looking \
         for something that is not there",
        fully_shadowed.len()
    );
    assert!(offenders.is_empty(), "a peek copies:\n{}", offenders.join("\n"));
    assert!(omitted_sinks >= 20, "only {omitted_sinks} multi-output peeks allocate an omitted sink");
    // On the shipped corpus the exemption is reached by nothing; when the
    // fixtures are in the tree it must be, or the branch is dead.
    if fixtures > 0 {
        assert!(
            !bounded.is_empty(),
            "{fixtures} fixture(s) swept and none reached the bounded-accumulator copy — the \
             exemption is dead code and proves nothing"
        );
    }
    // EXACTLY these three, as in Java: the composed peeks over a multi-output
    // callee. A fourth is a new one nobody costed.
    let expected: BTreeSet<String> = ["kdj", "ma", "stochrsi"].iter().map(|s| (*s).to_string()).collect();
    assert_eq!(sink_sites, expected, "the set of composed peeks allocating a sub-stream sink moved");
}

/// The public `update` and `peek` name the bar as the positional tier names the
/// series; the transcribed bodies and the `@internal` frames keep C's names.
#[test]
fn public_update_and_peek_name_the_bar_as_the_positional_tier_does() {
    let sma = section("sma");
    assert!(sma.contains("\n  update(real: number): number {\n"), "sma: {}", signature_of(&sma, "\n  update("));
    assert!(sma.contains("\n  peek(real: number): number {\n"), "sma: {}", signature_of(&sma, "\n  peek("));
    assert!(sma.contains("\n    const inReal = real;\n"), "the peek frame reads C's name through an alias");
    assert!(sma.contains("\nfunction smaStepImpl(sp: SmaStream, inReal: number): void {\n"), "the step body keeps C's name");
    assert!(sma.contains("\n    smaStepImpl(this, real);\n"));
    let atr = section("atr");
    assert!(atr.contains("\n  update(high: number, low: number, close: number): number {\n"), "atr: {}", signature_of(&atr, "\n  update("));
    // ad's body declares `high`, `low`, `close` and `volume` as locals: inside a
    // block they shadow the parameters, which the aliases ahead of it have read.
    let peek = body_of(&section("ad"), "  peek(high: number");
    let alias = peek.find("const inHigh = high;").expect("alias");
    let block = alias + peek[alias..].find("\n    {\n").expect("shadowing block");
    let local = peek.find("let high").expect("the body local");
    assert!(alias < block && block < local, "aliases, then the block, then the local:\n{peek}");

    let (registry, helpers) = (common::make_registry(), common::make_helpers());
    let mut methods = 0;
    for name in streaming_funcs() {
        let (func, enums) = load(&name);
        let src = section_of(&backends::ts::generate(&func, &enums, registry, helpers)).expect("stream section");
        for needle in ["\n  update(", "\n  peek("] {
            let sig = signature_of(&src, needle);
            let params = sig.split_once('(').map_or("", |(_, p)| p);
            let c_named = params.split(", ").any(|p| p.starts_with("in") && p[2..].starts_with(|c: char| c.is_ascii_uppercase()));
            assert!(!c_named, "{name}: C's input name in a public signature: {sig}");
            methods += 1;
        }
    }
    assert!(methods >= 400, "swept {methods} public methods");
}
