//! The WebAssembly engine's exports (`backends::wasm`). A wrong export set or a
//! wrong argument order answers through the TypeScript fallback or with
//! swapped parameters, and no value gate there sees which.

use crate::common::{
    all_indicators, discover_indicators, load_enums, load_indicator, make_helpers, make_registry,
};
use std::collections::{BTreeMap, BTreeSet};
use ta_codegen_lib::backends::ts_engine::{self, Route};
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

    assert!(
        crate::common::sweep_filtered() || (!plain.is_empty() && !flagged.is_empty()),
        "{} exported, {} withheld",
        plain.len(),
        flagged.len()
    );
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
    let funcs: Vec<FuncDef> = all_indicators().iter().map(|n| load_indicator(n).0).collect();
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

/// A routing-table entry. Under the threshold the call must go to `./js`
/// without counting a fall-through (the strict gates read the counter) and
/// without taking a frame (which loads the engine), and so must a call that
/// copies fewer bars than the threshold; from the threshold on it routes in
/// table mode, whole input or ranged.
#[test]
fn ts_engine_routes_a_tabled_function_from_its_bar_count() {
    if crate::common::sweep_filtered() {
        return;
    }
    let funcs = corpus();
    let enums = load_enums();
    let sma = funcs.iter().find(|f| f.name == "SMA").unwrap();
    let text = backends::ts_engine::render_routed(sma, &funcs, &enums, make_registry(), Some(Route { from: 37, to: None }));
    assert!(text.contains("\nconst WASM_FROM_BARS = 37;\n"), "{text}");
    assert!(!text.contains("WASM_TO_BARS"), "an open entry has no upper bound:\n{text}");
    let at = |needle: &str| text.find(needle).unwrap_or_else(|| panic!("no `{needle}`:\n{text}"));
    let public = &text[at("export function sma(")..at("function smaWasm(")];
    assert!(!public.contains("engineRoute.mode !== 0"), "a tabled function routes in table mode:\n{public}");
    assert_eq!(text.matches("WASM_FROM_BARS").count(), 5, "{text}");
    let gate = "  if (engineRoute.mode === 0 && n < WASM_FROM_BARS) {\n    return null;\n  }\n";
    // Each front checks the input's length before anything but the length
    // itself, and the bars it copies once the range and parameters passed;
    // both before the frame. The copy into buffers checks neither.
    for (front, next, last) in [("function smaWasm(", "function smaBuffered(", "last"), ("function smaPositionalWasm(", "function smaInto(", "endIdx")] {
        let body = &text[at(front)..at(next)];
        let gate_at = body.find(gate).unwrap_or_else(|| panic!("no threshold check:\n{body}"));
        assert!(body.find("const n = seriesLength(").unwrap() < gate_at);
        assert_eq!(body[..gate_at].matches("fellThrough(").count(), 1, "only an empty input counts above the threshold check:\n{body}");
        assert!(body[..gate_at].contains("if (n < 1) {\n    return fellThrough('input');"), "{body}");
        assert!(body.find("if (engineDown()) {").unwrap() > gate_at, "{front} reads the engine state before the length:\n{body}");
        let copied = format!("  if (engineRoute.mode === 0 && {last} - lo + 1 < WASM_FROM_BARS) {{\n    return null;\n  }}\n");
        let copied_at = body.find(&copied).unwrap_or_else(|| panic!("no check of the bars copied:\n{body}"));
        assert!(body.find("const lo = (").unwrap() < copied_at, "{body}");
        assert!(body.find("fellThrough('range')").unwrap() < copied_at, "{front} checks the copy before the range:\n{body}");
        for later in ["engineFor(", "routedSeries(", "Buffered(", "Into(", "wasmFrame("] {
            if let Some(k) = body.find(later) {
                assert!(k > copied_at, "{front} reaches `{later}` before the check of the bars copied:\n{body}");
            }
        }
    }
    // A call the length check leaves in `./js` walks its option keys there alone.
    let options_front = &text[at("function smaWasm(")..at("function smaBuffered(")];
    let walk = options_front.find("o = smaOptionsOf('SMA', options);").unwrap_or_else(|| panic!("no options walk:\n{options_front}"));
    assert!(walk > options_front.find(gate).unwrap(), "the options walk precedes the table's check:\n{options_front}");
    let into = &text[at("function smaInto(")..];
    assert!(into.contains("wasmFrame(") && !into.contains("WASM_FROM_BARS"));

    let untabled = backends::ts_engine::render_routed(sma, &funcs, &enums, make_registry(), None);
    assert!(!untabled.contains("WASM_FROM_BARS"));
    assert!(untabled.contains("  if (engineRoute.mode !== 0) {\n"));
}

/// A ranged entry: the bars copied are checked on both ends, so a call copying
/// more stays in `./js` exactly as one copying fewer does; the input's length
/// is checked against the first count alone, since a range of a longer input
/// may copy fewer bars.
#[test]
fn ts_engine_routes_a_ranged_function_only_within_its_bar_counts() {
    let (sma, _) = load_indicator("sma");
    let enums = load_enums();
    let text = ts_engine::render_routed(&sma, std::slice::from_ref(&sma), &enums, make_registry(), Some(Route { from: 37, to: Some(90) }));
    assert!(text.contains("\nconst WASM_FROM_BARS = 37;\nconst WASM_TO_BARS = 90;\n"), "{text}");
    assert_eq!(text.matches("WASM_TO_BARS").count(), 3, "{text}");
    let at = |needle: &str| text.find(needle).unwrap_or_else(|| panic!("no `{needle}`:\n{text}"));
    for (front, next, last) in [("function smaWasm(", "function smaBuffered(", "last"), ("function smaPositionalWasm(", "function smaInto(", "endIdx")] {
        let body = &text[at(front)..at(next)];
        let gate = format!(
            "  if (engineRoute.mode === 0 && ({last} - lo + 1 < WASM_FROM_BARS || {last} - lo + 1 > WASM_TO_BARS)) {{\n    return null;\n  }}\n"
        );
        assert!(body.contains(&gate), "no bounded check of the bars copied:\n{body}");
        assert_eq!(body.matches("WASM_TO_BARS").count(), 1, "{body}");
        assert!(body.contains("  if (engineRoute.mode === 0 && n < WASM_FROM_BARS) {\n"), "{body}");
    }
}

/// The module sees only bars `lo` through `last`, rebased to start at 0: the
/// glue copies those, passes both bounds less `lo`, and adds `lo` back to
/// `begIdx` and to each bar position, in every output that holds one and in
/// no other. Each value gate compares through the public call, which hides a
/// copy of the whole input; only the frame's size shows it.
#[test]
fn ts_engine_copies_only_the_bars_a_range_reads() {
    if crate::common::sweep_filtered() {
        return;
    }
    let funcs = corpus();
    let enums = load_enums();
    let zigzag = funcs.iter().find(|f| f.name == "ZIGZAG").unwrap();
    let positions: Vec<&str> = zigzag.outputs.iter().filter(|o| o.flags.iter().any(|f| f == "bar_index")).map(|o| o.name.as_str()).collect();
    assert_eq!(positions, ["outPivotIdx"]);
    let text = ts_engine::render_routed(zigzag, &funcs, &enums, make_registry(), None);
    let at = |needle: &str| text.find(needle).unwrap_or_else(|| panic!("no `{needle}`:\n{text}"));
    for (front, next) in [("function zigzagWasm(", "function zigzagBuffered("), ("function zigzagPositionalWasm(", "function zigzagInto(")] {
        let body = &text[at(front)..at(next)];
        assert!(body.contains("const lookback = zigzagLookbackPositional(optInSensitivity, optInMinTrendLength);\n"), "{body}");
        assert!(body.contains(" - lo + 1) * 16 + ("), "{front} sizes its budget check over the whole input:\n{body}");
        assert!(!body.contains("engineFor(n *"), "{body}");
    }
    let options_front = &text[at("function zigzagWasm(")..at("function zigzagBuffered(")];
    let into = &text[at("function zigzagInto(")..];
    for body in [options_front, into] {
        assert!(body.contains("  const len = last - lo + 1;\n  const inHighBars = seriesBars(inHigh, lo, len);\n  const inLowBars = seriesBars(inLow, lo, len);\n  const w = wasmFrame(len * 16 + outLen * 16);\n"), "{body}");
        assert!(body.contains("w.f64.set(inHighBars, inHighAt / 8);") && body.contains("const inLowAt = at + len * 8;"), "{body}");
        assert!(!body.contains("w.f64.set(inHigh,") && !body.contains("w.f64.set(inLow,"), "{body}");
        assert!(body.contains("count = w.x.zigzag_batch(first - lo, last - lo, inHighAt, inLowAt, len, "), "{body}");
        let shift = body.find("  w.shiftInts(outPivotIdxAt, count, lo);\n").unwrap_or_else(|| panic!("no shift of the pivot index:\n{body}"));
        assert_eq!(body.matches("w.shiftInts(").count(), 1, "{body}");
        assert!(shift > body.find("if (count < 0) {").unwrap(), "{body}");
        let copied_out = body.find("w.ints(outPivotIdxAt, count)").or_else(|| body.find("w.intsInto(outPivotIdxAt, count, ")).unwrap();
        assert!(shift < copied_out, "the pivot index is copied out before its shift:\n{body}");
    }
    assert!(options_front.contains("{ begIdx: w.begIdx() + lo, "), "{options_front}");
    assert!(into.contains("  const begIdx = w.begIdx() + lo;\n"), "{into}");

    let ema = funcs.iter().find(|f| f.name == "EMA").unwrap();
    let plain = ts_engine::render_routed(ema, &funcs, &enums, make_registry(), None);
    assert!(!plain.contains("shiftInts"), "EMA holds no bar position:\n{plain}");
}

#[test]
fn ts_engine_routing_test_checks_both_ends_of_a_ranged_entry() {
    let funcs: Vec<FuncDef> = ["sma", "ema", "wma"].iter().map(|n| load_indicator(n).0).collect();
    let table = BTreeMap::from([
        ("SMA".to_string(), Route { from: 5, to: Some(200_000) }),
        ("EMA".to_string(), Route { from: 7, to: None }),
    ]);
    let text = ts_engine::render_routing_test(&funcs, &table);
    let row = |name: &str| text.lines().find(|l| l.starts_with(&format!("  {{ name: '{name}', "))).unwrap_or_else(|| panic!("no {name} row:\n{text}"));
    assert!(row("SMA").ends_with(" bars: 5, to: 200000 },"), "{text}");
    assert!(row("EMA").ends_with(" bars: 7 },"), "{text}");
    assert!(row("EMA").contains(" lookback: 'emaLookback', "), "{text}");
    assert!(text.contains("\nconst RANGED_PAD = 3;\nconst MAX_BARS = 200003;\n"), "MAX_BARS must hold a copy of one bar over the range from bar 2:\n{text}");
    assert!(text.contains("const over = run(row, row.to + 1);"), "{text}");
    assert!(text.contains("const counts: [number, boolean][] = [[row.bars - 1, false], [row.bars, true]];"), "{text}");
    assert!(text.contains("if (row.to !== undefined) counts.push([row.to, true], [row.to + 1, false]);"), "{text}");
    assert!(text.contains("for (const lo of [0, 1, 2]) {"), "{text}");
    assert!(text.contains("const whole = run(row, row.bars, { startIdx: 0, endIdx: row.bars - 1 });"), "{text}");
    assert!(text.contains("[['options', call], ['buffered', callBuffered], ['positional', callPositional]]"), "{text}");
}

fn load_table(tag: &str, yaml: &str) -> BTreeMap<String, Route> {
    let path = std::env::temp_dir().join(format!("ta_wasm_routing_{}_{tag}.yaml", std::process::id()));
    std::fs::write(&path, yaml).unwrap();
    let funcs = vec![load_indicator("sma").0];
    let table = std::panic::catch_unwind(|| ts_engine::load_routing(&path, &funcs));
    let _ = std::fs::remove_file(&path);
    table.unwrap_or_else(|e| std::panic::resume_unwind(e))
}

#[test]
fn ts_engine_load_routing_reads_both_forms() {
    assert_eq!(load_table("open", "batch:\n  SMA: 4\n")["SMA"], Route { from: 4, to: None });
    assert_eq!(load_table("one", "batch:\n  SMA: [4, 4]\n")["SMA"], Route { from: 4, to: Some(4) });
}

#[test]
#[should_panic(expected = "the range ends below its start")]
fn ts_engine_load_routing_rejects_a_range_ending_below_its_start() {
    let _ = load_table("inverted", "batch:\n  SMA: [10, 9]\n");
}

#[test]
#[should_panic(expected = "the least is 1")]
fn ts_engine_load_routing_rejects_a_range_from_zero() {
    let _ = load_table("zero", "batch:\n  SMA: [0, 9]\n");
}
