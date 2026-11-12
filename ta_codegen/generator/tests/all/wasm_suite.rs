//! The WebAssembly engine's exports (`backends::wasm`). A wrong export set or a
//! wrong argument order answers through the TypeScript fallback or with
//! swapped parameters, and no value gate there sees which.

use crate::common::{discover_indicators, load_enums, load_indicator, make_helpers, make_registry};
use std::collections::{BTreeMap, BTreeSet};
use ta_codegen_lib::backends::{self, wasm};
use ta_codegen_lib::ir::{FuncDef, Statement};

fn corpus() -> Vec<FuncDef> {
    discover_indicators().iter().map(|n| load_indicator(n).0).collect()
}

/// `(name, type)` per parameter of the `fn <name>(` block in `text`.
fn signature(text: &str, name: &str) -> Vec<(String, String)> {
    let head = format!(" fn {name}(\n");
    let start = text.find(&head).unwrap_or_else(|| panic!("no `fn {name}(`")) + head.len();
    let body = &text[start..start + text[start..].find(") ->").unwrap()];
    body.lines()
        .map(str::trim)
        .filter(|l| !l.is_empty() && *l != "&self,")
        .map(|l| {
            let (n, t) = l.trim_end_matches(',').split_once(": ").unwrap();
            (n.to_string(), t.to_string())
        })
        .collect()
}

/// The text of the export `<name>`, from its `fn` line to the line closing it.
fn export_body<'a>(lib: &'a str, name: &str) -> &'a str {
    let head = format!(" fn {name}(\n");
    let start = lib.find(&head).unwrap_or_else(|| panic!("no `fn {name}(`"));
    &lib[start..start + lib[start..].find("\n}\n").unwrap()]
}

/// The argument names of the `CORE.<snake>(...)` call in an export.
fn call_args(lib: &str, snake: &str) -> Vec<String> {
    let head = format!("answer(CORE.{snake}(");
    let start = lib.find(&head).unwrap_or_else(|| panic!("no call to {snake}")) + head.len();
    lib[start..start + lib[start..].find("))").unwrap()].split(", ").map(str::to_string).collect()
}

#[test]
fn wasm_exports_exactly_the_functions_no_call_of_which_is_transcendental() {
    let funcs = corpus();
    let enums = load_enums();
    let lib = wasm::render_lib(&funcs, &enums);

    let exported: BTreeSet<String> = lib
        .lines()
        .filter_map(|l| l.strip_prefix("pub unsafe extern \"C\" fn ")?.strip_suffix('(').map(str::to_string))
        .collect();
    let (plain, flagged): (Vec<&FuncDef>, Vec<&FuncDef>) =
        funcs.iter().partition(|f| !f.flags.iter().any(|x| x == wasm::TRANSCENDENTAL_FLAG));
    let want: BTreeSet<String> =
        plain.iter().map(|f| format!("{}_batch", backends::common::snake_words(&f.name))).collect();

    assert!(!plain.is_empty() && !flagged.is_empty(), "{} exported, {} withheld", plain.len(), flagged.len());
    assert_eq!(plain.len() + flagged.len(), funcs.len());
    assert_eq!(exported, want, "lib.rs exports");
}

#[test]
fn wasm_export_parameters_match_the_rust_public_signature() {
    let funcs = corpus();
    let enums = load_enums();
    let lib = wasm::render_lib(&funcs, &enums);
    let abi = |t: &str| -> String {
        match t {
            "&[f64]" => "*const f64".into(),
            "&[i32]" => "*const i32".into(),
            "&mut [f64]" => "*mut f64".into(),
            "&mut [i32]" => "*mut i32".into(),
            "f64" | "i32" | "usize" => t.into(),
            e if enums.contains_key(e) => "i32".into(),
            other => panic!("no ABI type for `{other}`"),
        }
    };
    let mut checked = 0;
    for func in wasm::exported(&funcs) {
        let snake = backends::common::snake_words(&func.name);
        let rust = backends::rust_lang::generate(func, &enums, make_registry(), make_helpers());
        let rust_sig = signature(&rust, &snake);
        let want: Vec<(String, String)> = rust_sig.iter().map(|(n, t)| (n.clone(), abi(t))).collect();
        let got: Vec<(String, String)> = signature(&lib, &format!("{snake}_batch"))
            .into_iter()
            .filter(|(n, _)| n != "len" && n != "out_len")
            .collect();
        assert_eq!(got, want, "{}: export parameters", func.name);
        let names: Vec<String> = rust_sig.iter().map(|(n, _)| n.clone()).collect();
        assert_eq!(call_args(&lib, &snake), names, "{}: call arguments", func.name);
        checked += 1;
    }
    assert!(checked > 0);
}

/// Each `case` of a switch on an enum parameter, with the functions its arm
/// calls by name.
fn enum_dispatch_arms(stmts: &[Statement], out: &mut Vec<(String, BTreeSet<String>)>) {
    for s in stmts {
        match s {
            Statement::Switch { cases, default, .. } => {
                for (label, body) in cases {
                    let mut callees = BTreeSet::new();
                    for st in body {
                        ta_codegen_lib::streaming::walk_stmt_exprs(st, &mut |e| {
                            ta_codegen_lib::streaming::walk_expr(e, &mut |e| {
                                if let ta_codegen_lib::ir::Expr::FuncCall(c, _) = e {
                                    callees.insert(c.trim_start_matches("TA_").to_uppercase());
                                }
                            });
                        });
                    }
                    out.push((label.clone(), callees));
                    enum_dispatch_arms(body, out);
                }
                enum_dispatch_arms(default, out);
            }
            Statement::If { then_body, else_body, .. } => {
                enum_dispatch_arms(then_body, out);
                enum_dispatch_arms(else_body, out);
            }
            Statement::While { body, .. }
            | Statement::DoWhile { body, .. }
            | Statement::For { body, .. }
            | Statement::ForC { body, .. }
            | Statement::Block { body } => enum_dispatch_arms(body, out),
            _ => {}
        }
    }
}

/// The emitter declines an enum member by its name; what makes it transcendental
/// is the function a dispatching switch calls for it. A member whose arm calls a
/// differently named function would otherwise be exported and differ from
/// TypeScript in the last bit, which the default entry compares nowhere.
#[test]
fn wasm_declines_exactly_the_enum_members_whose_dispatch_reaches_a_transcendental_function() {
    let funcs = corpus();
    let enums = load_enums();
    let flagged: BTreeSet<String> = funcs
        .iter()
        .filter(|f| f.flags.iter().any(|x| x == wasm::TRANSCENDENTAL_FLAG))
        .map(|f| f.name.to_uppercase())
        .collect();
    let mut arms = Vec::new();
    for f in &funcs {
        enum_dispatch_arms(&f.body, &mut arms);
    }
    let mut reaching: BTreeMap<String, BTreeSet<i32>> = BTreeMap::new();
    let mut arms_seen = 0;
    for (label, callees) in &arms {
        for (name, def) in &enums {
            if let Some(v) = def.variants.iter().find(|v| *label == format!("{name}_{}", v.name)) {
                arms_seen += 1;
                let entry = reaching.entry(name.clone()).or_default();
                if callees.iter().any(|c| flagged.contains(c)) {
                    entry.insert(v.value);
                }
            }
        }
    }
    assert!(arms_seen > 0, "no switch arm matched an enum member: the label spelling changed");

    let lib = wasm::render_lib(&funcs, &enums);
    let mut params = 0;
    for func in wasm::exported(&funcs) {
        for opt in &func.optional_inputs {
            let ta_codegen_lib::ir::ParamType::Enum(e) = &opt.param_type else { continue };
            params += 1;
            let declined: BTreeSet<i32> = wasm::declined_values(opt, &funcs, &enums)
                .into_iter()
                .filter(|v| enums[e].variants.iter().any(|m| m.value == *v && m.name != "DEFAULT"))
                .collect();
            let want = reaching.get(e).cloned().unwrap_or_default();
            assert!(!want.is_empty(), "{e}: no member reaches a transcendental function");
            assert_eq!(declined, want, "{}.{}: declined members", func.name, opt.name);
            let emitted = export_body(&lib, &format!("{}_batch", backends::common::snake_words(&func.name)));
            let all: Vec<String> = wasm::declined_values(opt, &funcs, &enums).iter().map(i32::to_string).collect();
            let guard = format!("enum_param({}, &[{}])", opt.name, all.join(", "));
            assert!(emitted.contains(&guard), "{}: lib.rs does not decline with `{guard}`", func.name);
        }
    }
    assert!(params > 0);
}

/// A routing-table entry, which the shipped table has none of. Under the
/// threshold the call must go to `./js` without counting a fall-through (the
/// strict gates read the counter) and without taking a frame (which loads the
/// engine); from the threshold on it must route in table mode.
#[test]
fn ts_engine_routes_a_tabled_function_from_its_bar_count() {
    let funcs = corpus();
    let enums = load_enums();
    let sma = funcs.iter().find(|f| f.name == "SMA").unwrap();
    let text = backends::ts_engine::render_routed(sma, &enums, make_registry(), Some(37));
    assert!(text.contains("\nconst WASM_FROM_BARS = 37;\n"), "{text}");
    let public = &text[text.find("export function sma(").unwrap()..text.find("function smaWasm(").unwrap()];
    assert!(!public.contains("engineRoute.mode !== 0"), "a tabled function routes in table mode:\n{public}");
    let body = &text[text.find("function smaWasm(").unwrap()..];
    let gate = "  if (engineRoute.mode === 0 && n < WASM_FROM_BARS) {\n    return null;\n  }\n";
    let at = body.find(gate).unwrap_or_else(|| panic!("no threshold check:\n{body}"));
    assert_eq!(body.matches("WASM_FROM_BARS").count(), 1, "{body}");
    assert!(body.find("const n = seriesLength(").unwrap() < at);
    assert!(at < body.find("wasmFrame(").unwrap(), "the threshold check is below wasmFrame:\n{body}");
    assert_eq!(body[..at].matches("fellThrough(").count(), 1, "only an empty input counts above the threshold check:\n{body}");
    assert!(body[..at].contains("if (n < 1) {\n    return fellThrough('input');"), "{body}");

    let untabled = backends::ts_engine::render_routed(sma, &enums, make_registry(), None);
    assert!(!untabled.contains("WASM_FROM_BARS"));
    assert!(untabled.contains("  if (engineRoute.mode !== 0) {\n"));
}
