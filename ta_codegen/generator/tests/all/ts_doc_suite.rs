//! The TypeScript docs (`backends/ts_doc.rs`): every public declaration the TS
//! emitters write carries a TSDoc block (the analogue of C#'s CS1591, which
//! `tsc` has no flag for), and every function's examples reach the generated
//! tests.

use crate::common;
use std::collections::{BTreeMap, BTreeSet, HashMap};
use std::path::PathBuf;
use ta_codegen_lib::{backends, ir, parser};

fn input_dir() -> PathBuf {
    PathBuf::from(env!("CARGO_MANIFEST_DIR")).join("../input")
}

/// Shipped functions only: `synth_gate.py` copies its fixtures into `input/`,
/// and a fixture ships no docs.
fn shipped() -> (Vec<ir::FuncDef>, HashMap<String, ir::EnumDef>) {
    let enums = parser::enums::load_enums(&input_dir().join("enums.yaml"));
    let funcs = common::discover_indicators()
        .into_iter()
        .filter(|n| !n.starts_with("synth"))
        .map(|name| {
            let dir = input_dir().join(&name);
            let mut func = parser::yaml::parse_yaml(&dir.join(format!("{name}.yaml")));
            let parsed = parser::c_source::parse_c_source(&dir.join(format!("{name}.c")));
            parser::c_source::wire_parsed_source(&mut func, &parsed);
            func.doc = parser::doc_md::parse_doc_md(&dir.join(format!("{name}.md")));
            func
        })
        .collect();
    (funcs, enums)
}

#[derive(Debug, PartialEq)]
enum Ctx {
    Top,
    Function,
    /// `public` is whether the barrel exports the container: an `@internal`
    /// one is stripped whole, members included.
    Class { name: String, public: bool },
    Object { name: String, public: bool },
}

#[derive(Debug, Default)]
struct Sweep {
    /// `module: name` per exported declaration with a prose block.
    public_exports: BTreeMap<String, usize>,
    members: usize,
    params: usize,
    internal: usize,
    failures: Vec<String>,
}

/// The parameter names of the signature starting at `lines[at]`.
fn params(lines: &[&str], at: usize) -> Vec<String> {
    let text: String = lines[at..].iter().take(64).copied().collect::<Vec<_>>().join("\n");
    let open = text.find('(').expect("a signature has a parameter list");
    let mut depth = 0usize;
    let mut cur = String::new();
    let mut out = Vec::new();
    for c in text[open..].chars() {
        match c {
            '(' | '<' | '[' | '{' => {
                depth += 1;
                if depth == 1 {
                    continue;
                }
            }
            ')' | '>' | ']' | '}' => {
                depth -= 1;
                if depth == 0 {
                    out.push(std::mem::take(&mut cur));
                    break;
                }
            }
            ',' if depth == 1 => {
                out.push(std::mem::take(&mut cur));
                continue;
            }
            _ => {}
        }
        cur.push(c);
    }
    out.iter()
        .map(|p| p.trim())
        .filter(|p| !p.is_empty())
        .map(|p| p.split([':', '?', '=']).next().unwrap().trim().to_string())
        .collect()
}

/// A block's prose: the text before its first tag.
fn has_prose(doc: &str) -> bool {
    let body: String = doc
        .lines()
        .map(|l| l.trim().trim_start_matches("/**").trim_end_matches("*/").trim_start_matches('*').trim())
        .take_while(|l| !l.starts_with('@'))
        .collect();
    !body.trim().is_empty() && !doc.contains("@internal")
}

fn ident_after<'a>(line: &'a str, prefix: &str) -> Option<&'a str> {
    let rest = line.strip_prefix(prefix)?;
    let end = rest.find(|c: char| !(c.is_ascii_alphanumeric() || c == '_' || c == '$')).unwrap_or(rest.len());
    (end > 0).then(|| &rest[..end])
}

/// Every declaration in one emitted module against its doc block.
fn sweep_module(module: &str, src: &str, public: &BTreeSet<String>, sweep: &mut Sweep) {
    let lines: Vec<&str> = src.lines().collect();
    let mut ctx = Ctx::Top;
    let mut doc: Option<String> = None;
    let mut i = 0;
    while i < lines.len() {
        let line = lines[i];
        let trimmed = line.trim_start();
        let indent = line.len() - trimmed.len();
        let member_level = indent == 2 && matches!(ctx, Ctx::Class { .. } | Ctx::Object { .. });
        if trimmed.starts_with("/**") && (indent == 0 || member_level) {
            let mut text = String::new();
            while i < lines.len() {
                text.push_str(lines[i]);
                text.push('\n');
                if lines[i].contains("*/") {
                    break;
                }
                i += 1;
            }
            doc = Some(text);
            i += 1;
            continue;
        }
        let taken = doc.take();
        if indent == 0 {
            if line.starts_with('}') {
                ctx = Ctx::Top;
                i += 1;
                continue;
            }
            let decl = [
                ("export function ", Ctx::Function),
                ("export class ", Ctx::Class { name: String::new(), public: false }),
                ("export interface ", Ctx::Object { name: String::new(), public: false }),
                ("export const ", Ctx::Object { name: String::new(), public: false }),
                ("export type ", Ctx::Top),
            ]
            .into_iter()
            .find_map(|(p, c)| ident_after(line, p).map(|n| (n.to_string(), c)));
            if let Some((name, kind)) = decl {
                let label = format!("{module}: {name}");
                let is_public = public.contains(&name);
                match &taken {
                    None => sweep.failures.push(format!("{label}: no TSDoc block")),
                    Some(d) if is_public && !has_prose(d) => {
                        sweep.failures.push(format!("{label}: public, but its block has no prose or says @internal"));
                    }
                    Some(d) if !is_public && !d.contains("@internal") => {
                        sweep.failures.push(format!("{label}: exported outside the barrel without @internal"));
                    }
                    Some(d) => {
                        if is_public {
                            *sweep.public_exports.entry(module.to_string()).or_default() += 1;
                            if matches!(kind, Ctx::Function) {
                                check_params(&label, d, &params(&lines, i), sweep);
                            }
                        } else {
                            sweep.internal += 1;
                        }
                    }
                }
                ctx = match kind {
                    Ctx::Class { .. } if line.ends_with('{') => Ctx::Class { name, public: is_public },
                    Ctx::Object { .. } if line.ends_with('{') => Ctx::Object { name, public: is_public },
                    Ctx::Class { .. } | Ctx::Object { .. } | Ctx::Top => Ctx::Top,
                    Ctx::Function => Ctx::Function,
                };
            } else if line.starts_with("function ") {
                ctx = Ctx::Function;
            }
            i += 1;
            continue;
        }
        let public_owner = match &ctx {
            Ctx::Class { name, public: true } | Ctx::Object { name, public: true } => Some(name.clone()),
            _ => None,
        };
        let private = ["private ", "protected ", "#"].iter().any(|m| trimmed.starts_with(m));
        let member = member_level && !private && trimmed.starts_with(|c: char| c.is_ascii_alphabetic());
        if let (true, Some(owner)) = (member, public_owner) {
            let bare = ["static ", "readonly ", "get "]
                .iter()
                .fold(trimmed, |t, m| t.strip_prefix(m).unwrap_or(t));
            let name = ident_after(bare, "").expect("a member starts with an identifier");
            let label = format!("{module}: {owner}.{name}");
            match &taken {
                None => sweep.failures.push(format!("{label}: no TSDoc block")),
                Some(d) if d.contains("@internal") => sweep.internal += 1,
                Some(d) if !has_prose(d) => sweep.failures.push(format!("{label}: its block has no prose")),
                Some(d) => {
                    sweep.members += 1;
                    let is_method = bare[name.len()..].starts_with('(');
                    if is_method {
                        check_params(&label, d, &params(&lines, i), sweep);
                    }
                }
            }
        }
        i += 1;
    }
}

/// A public block tags every parameter, in order, and none the signature does
/// not have (C#'s CS1573, beside CS1591's missing block).
fn check_params(label: &str, doc: &str, names: &[String], sweep: &mut Sweep) {
    let tagged: Vec<String> = doc
        .lines()
        .filter_map(|l| l.trim().trim_start_matches('*').trim().strip_prefix("@param "))
        .map(|rest| rest.split_whitespace().next().unwrap_or("").to_string())
        .collect();
    if tagged != names {
        sweep.failures.push(format!("{label}: @param tags {tagged:?}, signature {names:?}"));
    } else {
        sweep.params += names.len();
    }
}

fn sweep_all() -> Sweep {
    let (funcs, enums) = shipped();
    let registry = common::make_registry();
    let helpers = common::make_helpers();
    let mut sweep = Sweep::default();
    let mut expected: BTreeMap<String, usize> = BTreeMap::new();
    for func in &funcs {
        let module = backends::ts::module_stem(func);
        let src = backends::ts::generate(func, &enums, registry, helpers);
        let public: BTreeSet<String> = backends::ts::public_exports(func).into_iter().collect();
        expected.insert(module.clone(), public.len());
        sweep_module(&module, &src, &public, &mut sweep);
    }
    let enums_src = backends::ts_enums::render(&funcs, &enums);
    let public: BTreeSet<String> = ["MAType", "FuncUnstId"].iter().map(|s| (*s).to_string()).collect();
    // `const` and `type` each, under one name.
    expected.insert("enums".to_string(), 2 * public.len());
    sweep_module("enums", &enums_src, &public, &mut sweep);
    sweep_metadata(&funcs, &enums, &mut sweep);
    let missing: Vec<String> = expected
        .iter()
        .filter(|(m, n)| sweep.public_exports.get(*m) != Some(n))
        .map(|(m, n)| format!("{m}: {n} barrel name(s), {:?} documented declaration(s)", sweep.public_exports.get(m)))
        .collect();
    sweep.failures.extend(missing);
    sweep
}

/// The `./metadata` modules, public by the subpath's barrel.
fn sweep_metadata(funcs: &[ir::FuncDef], enums: &HashMap<String, ir::EnumDef>, sweep: &mut Sweep) {
    let files = backends::ts_metadata::render_all(funcs, enums);
    let index = &files.iter().find(|(n, _)| *n == "index.ts").expect("metadata/index.ts").1;
    let mut public = BTreeSet::new();
    for list in index.split("export ").skip(1) {
        let list = list.trim_start_matches("type ");
        let Some(body) = list.strip_prefix('{').and_then(|l| l.split_once('}')) else { continue };
        public.extend(body.0.split(',').map(str::trim).filter(|n| !n.is_empty()).map(String::from));
    }
    assert!(public.len() > 10, "metadata/index.ts: only {public:?}");
    let before: usize = sweep.public_exports.values().sum();
    for (name, text) in files.iter().filter(|(n, _)| *n != "index.ts") {
        sweep_module(&format!("metadata/{name}"), text, &public, sweep);
    }
    let documented: usize = sweep.public_exports.values().sum::<usize>() - before;
    if documented < public.len() {
        sweep.failures.push(format!("metadata: {} barrel name(s), {documented} documented declaration(s)", public.len()));
    }
}

#[test]
fn every_public_ts_declaration_carries_a_tsdoc_block() {
    let sweep = sweep_all();
    assert!(
        sweep.failures.is_empty(),
        "{} TSDoc failure(s); fix the emitter in backends/ts_doc.rs:\n  {}",
        sweep.failures.len(),
        sweep.failures.join("\n  ")
    );
    let exports: usize = sweep.public_exports.values().sum();
    assert!(
        exports >= 3 * 200 && sweep.members > 200 && sweep.params > 6000 && sweep.internal > 200,
        "the sweep saw too little: {exports} public declarations, {} public members, {} params, \
         {} @internal declarations",
        sweep.members,
        sweep.params,
        sweep.internal
    );
}

/// Each probe is one defect the sweep exists for; a scanner that silently
/// stopped matching would let all of them through.
#[test]
fn the_tsdoc_sweep_sees_each_defect() {
    let public: BTreeSet<String> = ["f", "C"].iter().map(|s| (*s).to_string()).collect();
    let probes = [
        ("export function f(a: number): number {\n}\n", "no TSDoc block"),
        ("/** @internal */\nexport function f(a: number): number {\n}\n", "has no prose"),
        ("/**\n * Prose.\n */\nexport function g(a: number): number {\n}\n", "without @internal"),
        ("/**\n * Prose.\n * @param b - x\n */\nexport function f(a: number): number {\n}\n", "@param tags"),
        ("/**\n * Prose.\n */\nexport function f(a: number): number {\n}\n", "f: @param tags"),
        ("/** An object. */\nexport const C = {\n  /** Prose. */\n  at(a: number): number {\n    return a;\n  },\n};\n", "C.at: @param tags"),
        ("/** A class. */\nexport class C {\n  value(): number {\n    return 0;\n  }\n}\n", "C.value: no TSDoc"),
    ];
    for (src, needle) in probes {
        let mut sweep = Sweep::default();
        sweep_module("probe", src, &public, &mut sweep);
        assert!(
            sweep.failures.iter().any(|f| f.contains(needle)),
            "probe {src:?} should report {needle:?}, got {:?}",
            sweep.failures
        );
    }
    let mut sweep = Sweep::default();
    sweep_module(
        "probe",
        "/**\n * Prose.\n *\n * @param a - x\n * @param core - y\n */\nexport function f(\n  a: number,\n  core: Core = Core.DEFAULT,\n): number {\n  return a;\n}\n",
        &public,
        &mut sweep,
    );
    assert!(sweep.failures.is_empty(), "{:?}", sweep.failures);
    assert_eq!(sweep.params, 2);
}

/// Every function has a batch example, and a stream and a fill example when
/// it streams; each lands in exactly one generated test.
#[test]
fn every_function_has_its_examples_in_the_generated_tests() {
    let (funcs, enums) = shipped();
    let files = backends::ts_doc::render_example_tests(&funcs, &enums);
    let all: String = files.values().cloned().collect();
    let mut checked = 0;
    for func in &funcs {
        let names = backends::ts::ts_names(&func.name);
        let mut want = vec![names.public.clone()];
        if func.streaming {
            want.extend([names.open.clone(), names.open_and_fill.clone()]);
        }
        let titles: Vec<String> = backends::ts_doc::examples(func, &enums).into_iter().map(|e| e.title).collect();
        let expected: Vec<String> = want.iter().map(|w| format!("{}: {w} example", func.name)).collect();
        assert_eq!(titles, expected, "{}: examples", func.name);
        for t in &titles {
            assert_eq!(all.matches(&format!("test('{t}',")).count(), 1, "{t}: in the generated tests once");
        }
        checked += titles.len();
    }
    assert!(checked >= 3 * 200, "only {checked} examples");
}

/// The tests run against the package as a consumer sees it, so they import it
/// by its own name and never reach into `src/`.
#[test]
fn the_example_tests_import_the_package_by_name() {
    let (funcs, enums) = shipped();
    let files = backends::ts_doc::render_example_tests(&funcs, &enums);
    assert!(files.len() > 1, "{} test files", files.len());
    let from = format!("}} from '{}';", backends::ts::PACKAGE_NAME);
    for (name, text) in &files {
        assert!(name.ends_with(".test.ts"), "{name}: node --test globs *.test.ts");
        assert!(text.contains(&from), "{name}: no import from the package by name");
        for line in text.lines().filter(|l| l.contains(" from '")) {
            assert!(!line.contains(" from '.") && !line.contains(" from \"."), "{name}: relative import {line}");
        }
    }
}

/// The checks hold an integer output to its domain; one with no claim would
/// check nothing past `count > 0`.
#[test]
fn every_integer_output_carries_a_ts_example_claim() {
    let (funcs, enums) = shipped();
    let mut checked = 0;
    for func in &funcs {
        let Some(ex) = backends::ts_doc::batch_example(func, &enums) else { continue };
        let ints = func.outputs.iter().filter(|o| o.param_type == ir::ParamType::Integer).count();
        let claims = ex.checks.iter().filter(|c| c.starts_with("// ")).count();
        assert_eq!(
            claims, ints,
            "{}: {ints} integer output(s), {claims} domain claim(s) in the example checks -- add the \
             output's domain to rust_doc::integer_claim",
            func.name
        );
        checked += ints;
    }
    assert!(checked > 60, "only {checked} integer outputs");
}

/// Every fill's `@throws` scopes its overlap rejection as the public frame
/// tests it: a shared start first, any other overlap only on a history long
/// enough to open, and only against a converted input.
#[test]
fn the_fill_throws_names_the_overlap_checks_the_public_frame_emits() {
    let (funcs, enums) = shipped();
    let registry = common::make_registry();
    let helpers = common::make_helpers();
    let mut claimed = 0;
    for func in &funcs {
        let src = backends::ts::generate(func, &enums, registry, helpers);
        let names = backends::ts::ts_names(&func.name);
        let Some(entry) = src.find(&format!("\nexport function {}(\n", names.open_and_fill)) else { continue };
        let doc = &src[src[..entry].rfind("/**").expect("a doc block")..entry];
        let doc = doc.split_whitespace().filter(|w| *w != "*").collect::<Vec<_>>().join(" ");
        let public = &src[entry..entry + src[entry..].find("\n}\n").expect("the fill ends")];
        let guarded = public.contains(".byteOffset)) {") || public.contains("OpenAndFillImpl(");
        assert!(guarded, "{}: a fill with no overlap guard", func.name);
        assert!(
            doc.contains("an output too short or starting where another output or a `Float64Array` input starts, or, on a history long enough to open, one sharing memory with another output or a `Float64Array` input"),
            "{}: the fill's @throws does not scope its overlap rejection:\n{doc}",
            func.name
        );
        claimed += 1;
    }
    assert!(claimed > 200, "only {claimed} fills");
}

/// The batch `@throws` names each overlap rejection exactly when the public tier
/// tests it: every pair of outputs, whatever their element types, and every
/// output against every converted input. The body keeps its own test of two
/// outputs at one address.
#[test]
fn the_batch_throws_names_the_overlap_checks_the_public_tier_emits() {
    let (funcs, enums) = shipped();
    let registry = common::make_registry();
    let helpers = common::make_helpers();
    let mut claimed = 0;
    let mut claimed_inputs = 0;
    for func in &funcs {
        let src = backends::ts::generate(func, &enums, registry, helpers);
        let names = backends::ts::ts_names(&func.name);
        let entry = src.find(&format!("\nexport function {}(\n", names.public)).expect("a public batch function");
        let doc = &src[src[..entry].rfind("/**").expect("a doc block")..entry];
        let doc = doc.split_whitespace().filter(|w| *w != "*").collect::<Vec<_>>().join(" ");
        let public = &src[entry..entry + src[entry..].find("\n}\n").expect("the public tier ends")];
        let body_at = src.find(&format!("\nexport function {}(\n", names.body)).expect("a body");
        let body = &src[body_at..body_at + src[body_at..].find("\n}\n").expect("the body ends")];
        let outs = &func.outputs;
        let pairs: Vec<(&str, &str)> = outs
            .iter()
            .enumerate()
            .flat_map(|(i, a)| outs[i + 1..].iter().map(move |b| (a.name.as_str(), b.name.as_str())))
            .collect();
        let emits = !pairs.is_empty()
            && pairs.iter().all(|(a, b)| {
                public.contains(&format!("overlaps({a}, {b})"))
                    && body.contains(&format!("{a}.buffer === {b}.buffer && {a}.byteOffset"))
            });
        assert_eq!(emits, !pairs.is_empty(), "{}: a pair of outputs left untested", func.name);
        assert_eq!(doc.contains("two outputs sharing memory"), emits, "{}: @throws against the output pairs", func.name);
        claimed += usize::from(emits);

        let emits_inputs = !func.inputs.is_empty()
            && outs.iter().all(|o| {
                let test = if o.param_type == ir::ParamType::Real { "overlapsPartly" } else { "overlaps" };
                func.inputs.iter().all(|i| public.contains(&format!("{test}({}, {}F64)", o.name, i.name)))
            });
        assert_eq!(emits_inputs, !func.inputs.is_empty(), "{}: an output and input pair left untested", func.name);
        assert_eq!(
            doc.contains("an output sharing memory with a `Float64Array` input it is not"),
            emits_inputs,
            "{}: @throws against the input pairs",
            func.name
        );
        claimed_inputs += usize::from(emits_inputs);
    }
    assert!(claimed > 20, "only {claimed} functions reject shared outputs");
    assert!(claimed_inputs > 200, "only {claimed_inputs} functions reject an output over an input");
}
