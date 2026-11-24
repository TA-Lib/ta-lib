//! The npm package's default entry (`library/src/engine/`): for each function
//! with a WASM export, a module that re-exports its `./js` module and replaces
//! the public batch call and `<n>Positional`, which `ta-lib/abstract` calls,
//! with ones that run in the engine when they can; the
//! barrel; the Node entry, which hands the engine its bytes; and
//! `test/engineParity.test.ts`, every routed function against `./js`.

use super::rust_phantom_io::{leg_kind, vectors, VectorSpelling as _};
use super::ts::{self, api_input_sig, api_inputs, buffer_key, module_stem, param_key, result_key, ts_license, ts_names};
use super::ts_phantom_io::TsSpelling;
use super::wasm;
use crate::ir::{EnumDef, FuncDef, OptInput, ParamType};
use crate::registry::Registry;
use std::collections::{BTreeMap, BTreeSet, HashMap};
use std::fmt::Write as _;
use std::path::Path;

/// The routing table's file under `ta_codegen/input/`.
pub const ROUTING_FILE: &str = "wasm_routing.yaml";

/// The bar counts at which one function's batch call goes to the engine:
/// `from` on, through `to` inclusive when it is set.
#[derive(Clone, Copy, Debug, PartialEq, Eq, serde::Deserialize)]
#[serde(from = "RouteEntry")]
pub struct Route {
    pub from: u32,
    pub to: Option<u32>,
}

#[derive(serde::Deserialize)]
#[serde(untagged)]
enum RouteEntry {
    From(u32),
    Within([u32; 2]),
}

impl From<RouteEntry> for Route {
    fn from(e: RouteEntry) -> Self {
        match e {
            RouteEntry::From(from) => Route { from, to: None },
            RouteEntry::Within([from, to]) => Route { from, to: Some(to) },
        }
    }
}

#[derive(serde::Deserialize)]
#[serde(deny_unknown_fields)]
struct RoutingFile {
    batch: BTreeMap<String, Route>,
}

/// Per function, the bar counts at which its batch call goes to the engine.
///
/// # Panics
/// When the file cannot be read or parsed, names a function without a WASM
/// export, gives a count below 1, or closes a range below its start.
#[must_use]
pub fn load_routing(path: &Path, funcs: &[FuncDef]) -> BTreeMap<String, Route> {
    let text = std::fs::read_to_string(path).unwrap_or_else(|e| panic!("cannot read {}: {e}", path.display()));
    let file: RoutingFile = serde_yaml::from_str(&text).unwrap_or_else(|e| panic!("{}: {e}", path.display()));
    let exported: BTreeSet<&str> = wasm::exported(funcs).iter().map(|f| f.name.as_str()).collect();
    for (name, route) in &file.batch {
        let bars = route.from;
        assert!(exported.contains(name.as_str()), "{}: {name} has no WASM export", path.display());
        assert!(bars >= 1, "{}: {name} routes from {bars} bars; the least is 1", path.display());
        if let Some(to) = route.to {
            assert!(to >= bars, "{}: {name} routes from {bars} bars to {to}; the range ends below its start", path.display());
        }
    }
    file.batch
}

/// The functions the default entry routes: every one with a WASM export.
///
/// # Panics
/// On an exported function without inputs, which has no series to copy.
#[must_use]
pub fn routed(funcs: &[FuncDef]) -> Vec<&FuncDef> {
    let out = wasm::exported(funcs);
    for f in &out {
        assert!(!f.inputs.is_empty(), "{}: a WASM export without inputs", f.name);
    }
    out
}

const RUNTIME: &str = "../../runtime/wasm.ts";

fn is_real(t: &ParamType) -> bool {
    matches!(t, ParamType::Real)
}

fn sentinel(opt: &OptInput) -> &'static str {
    if is_real(&opt.param_type) {
        "Core.REAL_DEFAULT"
    } else {
        "Core.INTEGER_DEFAULT"
    }
}

/// The bytes one call places in linear memory: `(per input bar, per output
/// bar)`. The glue asks `wasmFrame` for exactly this.
#[must_use]
#[allow(clippy::cast_possible_truncation)]
pub fn frame_bytes(func: &FuncDef) -> (u32, u32) {
    let n_real = func.outputs.iter().filter(|o| is_real(&o.param_type)).count() as u32;
    let n_int = func.outputs.len() as u32 - n_real;
    (8 * func.inputs.len() as u32, 8 * n_real + 4 * n_int)
}

fn plus(base: &str, terms: &[(u32, &str)]) -> String {
    let mut s = base.to_string();
    for (k, var) in terms {
        if *k != 0 {
            let _ = write!(s, " + {var} * {k}");
        }
    }
    s
}

/// The lookback `./js` computes for the call's parameters, over the locals
/// that hold them with every omitted one as its sentinel.
fn lookback_call(func: &FuncDef) -> String {
    let args: Vec<&str> = func.optional_inputs.iter().map(|o| o.name.as_str()).collect();
    format!("{}({})", ts_names(&func.name).lookback_positional, args.join(", "))
}

/// The `outputRoom` call over `inputs` (the caller's own series) and every
/// output, at the `lookback` local.
fn room_call(func: &FuncDef, first: &str, last: &str, outs: &[String], inputs: &[String]) -> String {
    let types: Vec<&str> = func.outputs.iter().map(|o| ts::array_ts_type(&o.param_type)).collect();
    format!("outputRoom({first}, {last}, lookback, [{}], [{}], [{}])", inputs.join(", "), outs.join(", "), types.join(", "))
}

/// `lookback` and `lo`, the first bar the range reads (rL1), then the table's
/// test on the bars `lo` through `last`, the ones the call copies: an entry's
/// counts are of whole-series calls, which copy every bar.
fn copy_start(func: &FuncDef, first: &str, last: &str, route: Option<Route>) -> String {
    let mut s = format!(
        "  const lookback = {};\n  if (lookback < 0) {{\n    return fellThrough('parameter');\n  }}\n  \
         const lo = ({first} > lookback ? {first} : lookback) - lookback;\n",
        lookback_call(func)
    );
    if let Some(route) = route {
        let bars = format!("{last} - lo + 1");
        let test = match route.to {
            None => format!("{bars} < WASM_FROM_BARS"),
            Some(_) => format!("({bars} < WASM_FROM_BARS || {bars} > WASM_TO_BARS)"),
        };
        let _ = writeln!(s, "  if (engineRoute.mode === 0 && {test}) {{\n    return null;\n  }}");
    }
    s
}

/// An output whose values are bar positions, which the glue rebases.
fn holds_bars(o: &crate::ir::Output) -> bool {
    o.flags.iter().any(|f| f == "bar_index")
}

/// Every input after the first is as long as the first; `./js` accepts other
/// lengths, which the module does not.
fn same_lengths(rest: &[String]) -> String {
    if rest.is_empty() {
        return String::new();
    }
    let checks: Vec<String> = rest.iter().map(|r| format!("seriesLength({r}) !== n")).collect();
    format!("  if ({}) {{\n    return fellThrough('input');\n  }}\n", checks.join(" || "))
}

/// A check over each parameter's local, `pred` naming the test for its type.
fn param_check(func: &FuncDef, pred: impl Fn(&OptInput) -> String) -> String {
    if func.optional_inputs.is_empty() {
        return String::new();
    }
    let checks: Vec<String> = func.optional_inputs.iter().map(|o| format!("!{}", pred(o))).collect();
    format!("  if ({}) {{\n    return fellThrough('parameter');\n  }}\n", checks.join(" || "))
}

/// The parameter values the module declines, over the locals that hold them
/// with an omitted one as its sentinel; empty for a function that has none.
fn declined_check(func: &FuncDef, funcs: &[FuncDef], enums: &HashMap<String, EnumDef>) -> String {
    let tests: Vec<String> = func
        .optional_inputs
        .iter()
        .flat_map(|o| wasm::declined_values(o, funcs, enums).into_iter().map(move |v| format!("{} === {v}", o.name)))
        .collect();
    if tests.is_empty() {
        return String::new();
    }
    format!("  if ({}) {{\n    return fellThrough('declined');\n  }}\n", tests.join(" || "))
}

/// The frame's bytes over the input bars `in_len` and the output length `out_len`.
fn frame_size(func: &FuncDef, in_len: &str, out_len: &str) -> String {
    let (per_in, per_out) = frame_bytes(func);
    plus(&format!("{in_len} * {per_in}"), &[(per_out, out_len)])
}

/// `engineFor` over the frame the call will take: the last check before the
/// first conversion.
fn engine_for(func: &FuncDef, in_len: &str, out_len: &str) -> String {
    format!("  if (!engineFor({})) {{\n    return null;\n  }}\n", frame_size(func, in_len, out_len))
}

/// Each `(local, series)` converted as `./js` converts it, then refused together.
fn conversions(pairs: &[(String, String)]) -> String {
    let mut s = String::new();
    for (local, series) in pairs {
        let _ = writeln!(s, "  const {local} = routedSeries({series});");
    }
    // The length test follows the last conversion because a conversion may run caller code.
    let refused: Vec<String> = pairs.iter().map(|(local, _)| format!("{local} === null || {local}.length !== n")).collect();
    let _ = writeln!(s, "  if ({}) {{\n    return fellThrough('input');\n  }}", refused.join(" || "));
    s
}

/// The call into the module over the bars `lo` through `last` of converted
/// inputs, up to a count it accepted, with every bar position made absolute.
///
/// # Panics
/// On a `bar_index` output that is not an integer.
#[allow(clippy::cast_possible_truncation)]
fn frame_call(func: &FuncDef) -> String {
    // Nothing from `wasmFrame` on may run caller code: a `valueOf` that calls a
    // routed function back takes the same scratch and may grow linear memory,
    // detaching the views `wasmFrame` derived.
    let mut c = String::from("  const outLen = last - first + 1;\n  const len = last - lo + 1;\n");
    let n_in = func.inputs.len() as u32;
    let (reals, ints): (Vec<_>, Vec<_>) = func.outputs.iter().partition(|o| is_real(&o.param_type));
    let n_real = reals.len() as u32;
    for i in &func.inputs {
        let _ = writeln!(c, "  const {0}Bars = seriesBars({0}, lo, len);", i.name);
    }
    let _ = writeln!(c, "  const w = wasmFrame({});\n  if (w === null) {{\n    return null;\n  }}\n  const at = w.base;", frame_size(func, "len", "outLen"));
    for (k, i) in func.inputs.iter().enumerate() {
        let at = plus("at", &[(8 * k as u32, "len")]);
        let _ = writeln!(c, "  const {}At = {at};\n  w.f64.set({0}Bars, {0}At / 8);", i.name);
    }
    for (j, o) in reals.iter().enumerate() {
        let _ = writeln!(c, "  const {}At = {};", o.name, plus("at", &[(8 * n_in, "len"), (8 * j as u32, "outLen")]));
    }
    for (j, o) in ints.iter().enumerate() {
        let _ = writeln!(c, "  const {}At = {};", o.name, plus("at", &[(8 * n_in, "len"), (8 * n_real + 4 * j as u32, "outLen")]));
    }
    let mut call: Vec<String> = vec!["first - lo".into(), "last - lo".into()];
    call.extend(func.inputs.iter().map(|i| format!("{}At", i.name)));
    call.push("len".into());
    call.extend(func.optional_inputs.iter().map(|o| o.name.clone()));
    call.extend(func.outputs.iter().map(|o| format!("{}At", o.name)));
    call.push("outLen".into());
    let export = format!("{}_batch", super::common::snake_words(&func.name));
    let _ = writeln!(
        c,
        "  let count: number;\n  try {{\n    count = w.x.{export}({});\n  }} catch (e) {{\n    return trapped(w, e);\n  }}\n  \
         if (count < 0) {{\n    return refused(count);\n  }}",
        call.join(", ")
    );
    for o in func.outputs.iter().filter(|o| holds_bars(o)) {
        assert!(!is_real(&o.param_type), "{}: {} holds bar positions in a real output", func.name, o.name);
        let _ = writeln!(c, "  w.shiftInts({}At, count, lo);", o.name);
    }
    c
}

/// The table-mode test every front makes on the input's length before reading
/// anything else, so the call forms cannot route one length differently; empty
/// for an untabled function. No call copies more bars than the input holds, so
/// a call this leaves in `./js` neither walks its options nor reads the
/// engine's state here; [`copy_start`] tests the count copied.
fn route_check(route: Option<Route>) -> &'static str {
    match route {
        None => "",
        Some(_) => "  if (engineRoute.mode === 0 && n < WASM_FROM_BARS) {\n    return null;\n  }\n",
    }
}

/// What one entry point's wrapper declares and returns around the WASM attempt.
struct Front<'a> {
    docs: &'a str,
    public: &'a str,
    sig: &'a str,
    result: &'a str,
    wasm_call: &'a str,
    js_call: &'a str,
}

/// The public wrapper both entry points share: the WASM attempt, then `./js`
/// with the caller's arguments.
fn wrapper(f: &Front, name: &str, tabled: bool) -> String {
    // Anything thrown on the WASM path is a fall-through: `./js` then answers
    // or throws its own error.
    let Front { docs, public, sig, result, wasm_call, js_call } = f;
    let (open, pad, close) = if tabled { ("", "  ", "") } else { ("  if (engineRoute.mode !== 0) {\n", "    ", "  }\n") };
    format!(
        "\n{docs}export function {public}({sig}): {result} {{\n{open}\
         {pad}let routed: {result} | null;\n\
         {pad}try {{\n{pad}  routed = {wasm_call};\n{pad}}} catch {{\n{pad}  routed = fellThrough('threw');\n{pad}}}\n\
         {pad}if (routed !== null) {{\n{pad}  return routed;\n{pad}}}\n\
         {pad}if (engineRoute.mode === 2) {{\n{pad}  return strictFallThrough('{name}', {js_call});\n{pad}}}\n\
         {close}  return {js_call};\n}}\n"
    )
}

/// One routed module: the options call and `<n>Positional`, each a front that
/// checks its own arguments. The options front answers a call without buffers
/// in new arrays itself; `<n>Into` copies into the caller's buffers.
#[allow(clippy::implicit_hasher, clippy::too_many_lines)]
#[must_use]
pub fn render_routed(func: &FuncDef, funcs: &[FuncDef], enums: &HashMap<String, EnumDef>, registry: &Registry, route: Option<Route>) -> String {
    super::ts_doc::seed_function_names(registry.names());
    let names = ts_names(&func.name);
    let stem = module_stem(func);
    let js_fn = format!("{stem}Js");
    let js_positional = format!("{}Js", names.positional);
    let wasm_fn = format!("{stem}Wasm");
    let buffered_fn = format!("{stem}Buffered");
    let positional_wasm_fn = format!("{}Wasm", names.positional);
    let into_fn = format!("{stem}Into");
    let js_module = format!("../../func/{stem}.ts");
    let api = api_inputs(func);
    let arg_names: Vec<String> = api.iter().map(|a| a.name.clone()).collect();
    let leg_exprs = ts::api_leg_args(func, &func.inputs.iter().map(|i| i.name.clone()).collect::<Vec<_>>());
    let raw: Vec<String> = func
        .inputs
        .iter()
        .zip(&leg_exprs)
        .map(|(i, e)| if e.contains('.') { format!("{}Arg", i.name) } else { e.clone() })
        .collect();
    let inputs: Vec<String> = func.inputs.iter().map(|i| i.name.clone()).collect();
    let has_real = func.optional_inputs.iter().any(|o| is_real(&o.param_type));
    let multi = func.outputs.len() > 1;
    // State only, ahead of the options: a call with no engine reads them once.
    let engine_down = "  if (engineDown()) {\n    return null;\n  }\n";
    let room_fail = "  if (room < 0) {\n    return fellThrough(room === -1 ? 'parameter' : 'buffer');\n  }\n";
    let declined = declined_check(func, funcs, enums);
    let params: Vec<String> = func.optional_inputs.iter().map(|o| o.name.clone()).collect();
    let out_names: Vec<String> = func.outputs.iter().map(|o| o.name.clone()).collect();
    let converted = |suffix: &str, from: &[String]| -> Vec<(String, String)> {
        inputs.iter().zip(from).map(|(i, r)| (format!("{i}{suffix}"), r.clone())).collect()
    };
    let args = |head: &[&str], ins: &[String], outs: &[String], tail: &[&str]| -> String {
        let mut v: Vec<String> = head.iter().map(|s| (*s).to_string()).collect();
        v.extend(ins.iter().cloned());
        v.extend(params.iter().cloned());
        v.extend(outs.iter().cloned());
        v.extend(tail.iter().map(|s| (*s).to_string()));
        v.join(", ")
    };

    // The options front. Conversions can run caller code and allocate, so they
    // come after every check the front makes, in `./js`'s order.
    let mut b = String::new();
    if api.iter().any(|a| a.is_price) {
        let _ = writeln!(b, "  if (typeof {0} !== 'object' || {0} === null) {{\n    return fellThrough('input');\n  }}", ts::PRICES_PARAM);
    }
    for (e, r) in leg_exprs.iter().zip(&raw) {
        if r != e {
            let _ = writeln!(b, "  const {r} = {e};");
        }
    }
    let _ = writeln!(b, "  const n = seriesLength({});\n  if (n < 1) {{\n    return fellThrough('input');\n  }}", raw[0]);
    b.push_str(route_check(route));
    b.push_str(engine_down);
    b.push_str(&same_lengths(&raw[1..]));
    // After the table's length check, so a call too short for the table
    // walks the keys only in `./js`.
    let _ = writeln!(
        b,
        "  let o: {};\n  try {{\n    o = {}('{}', options);\n  }} catch {{\n    return fellThrough('options');\n  }}",
        names.options_type, names.options_of, func.name
    );
    b.push_str("  const startIdx = o.startIdx;\n  const endIdx = o.endIdx;\n");
    b.push_str(
        "  if (!usesDefaultCore(o.core) || !optionalInt32(startIdx) || !optionalInt32(endIdx)) {\n    return fellThrough('options');\n  }\n",
    );
    for opt in &func.optional_inputs {
        let _ = writeln!(b, "  const {}Arg = o.{};", opt.name, param_key(&opt.name));
    }
    b.push_str(&param_check(func, |o| format!("{}({}Arg)", if is_real(&o.param_type) { "optionalReal" } else { "optionalInt32" }, o.name)));
    for opt in &func.optional_inputs {
        let _ = writeln!(b, "  const {0} = {0}Arg === undefined ? {1} : {0}Arg;", opt.name, sentinel(opt));
    }
    b.push_str(&declined);
    for o in &func.outputs {
        let _ = writeln!(b, "  const {} = o.{};", o.name, buffer_key(func, o));
    }
    b.push_str(
        "  const first = startIdx === undefined ? 0 : startIdx;\n  const last = endIdx === undefined ? n - 1 : endIdx;\n  \
         if (first < 0 || last < first || last >= n) {\n    return fellThrough('range');\n  }\n",
    );
    b.push_str(&copy_start(func, "first", "last", route));
    let given: Vec<String> = out_names.iter().map(|o| format!("{o} !== undefined")).collect();
    let _ = writeln!(
        b,
        "  if ({}) {{\n    return {buffered_fn}({});\n  }}",
        given.join(" || "),
        args(&["first", "last", "lo", "lookback", "n"], &raw, &out_names, &[])
    );
    b.push_str(&engine_for(func, "(last - lo + 1)", "(last - first + 1)"));
    b.push_str(&conversions(&converted("", &raw)));
    b.push_str(&frame_call(func));
    let made: Vec<String> = func
        .outputs
        .iter()
        .map(|o| format!("{}: w.{}({}At, count)", result_key(func, o), if is_real(&o.param_type) { "reals" } else { "ints" }, o.name))
        .collect();
    let _ = writeln!(b, "  const result: {} = {{ begIdx: w.begIdx() + lo, {} }};\n  w.done();\n  return result;", names.result_type, made.join(", "));

    // The options call with buffers: any buffer it was not given is allocated
    // as `./js` allocates it, once the engine is known to run the call.
    let mut bf = String::new();
    let _ = write!(bf, "  const room = {};\n{room_fail}", room_call(func, "first", "last", &out_names, &raw));
    bf.push_str(&engine_for(func, "(last - lo + 1)", "(last - first + 1)"));
    bf.push_str(&conversions(&converted("", &raw)));
    let filled: Vec<String> = if multi {
        for o in &func.outputs {
            let ctor = ts::array_ts_type(&o.param_type);
            let _ = writeln!(bf, "  const {0}Buf = {0} ?? new {ctor}(room === 0 ? 1 : room);", o.name);
        }
        out_names.iter().map(|o| format!("{o}Buf")).collect()
    } else {
        out_names.clone()
    };
    let _ = writeln!(bf, "  const range = {into_fn}({});", args(&["first", "last", "lo"], &inputs, &filled, &["room"]));
    let views: Vec<String> =
        func.outputs.iter().zip(&filled).map(|(o, f)| format!("{}: {f}.subarray(0, range.count)", result_key(func, o))).collect();
    let _ = writeln!(bf, "  return range === null ? null : {{ begIdx: range.begIdx, {} }};", views.join(", "));

    // The positional front: its buffers are the caller's, all of them.
    let mut p = String::new();
    let _ = writeln!(p, "  const n = seriesLength({});\n  if (n < 1) {{\n    return fellThrough('input');\n  }}", inputs[0]);
    p.push_str(route_check(route));
    p.push_str(engine_down);
    p.push_str(&same_lengths(&inputs[1..]));
    p.push_str("  if (!usesDefaultCore(core) || !isInt32(startIdx) || !isInt32(endIdx)) {\n    return fellThrough('options');\n  }\n");
    p.push_str(&param_check(func, |o| format!("{}({})", if is_real(&o.param_type) { "isReal" } else { "isInt32" }, o.name)));
    p.push_str(&declined);
    let absent: Vec<String> = out_names.iter().map(|o| format!("{o} === undefined")).collect();
    let _ = writeln!(p, "  if ({}) {{\n    return fellThrough('buffer');\n  }}", absent.join(" || "));
    p.push_str("  if (startIdx < 0 || endIdx < startIdx || endIdx >= n) {\n    return fellThrough('range');\n  }\n");
    p.push_str(&copy_start(func, "startIdx", "endIdx", route));
    let _ = write!(p, "  const room = {};\n{room_fail}", room_call(func, "startIdx", "endIdx", &out_names, &inputs));
    p.push_str(&engine_for(func, "(endIdx - lo + 1)", "(endIdx - startIdx + 1)"));
    p.push_str(&conversions(&converted("F64", &inputs)));
    let f64s: Vec<String> = inputs.iter().map(|i| format!("{i}F64")).collect();
    let _ = writeln!(p, "  return {into_fn}({});", args(&["startIdx", "endIdx", "lo"], &f64s, &out_names, &["room"]));

    // Into the caller's buffers: every output is copied before any view of one is built.
    let mut it = frame_call(func);
    it.push_str("  if (count > room) {\n    return fellThrough('overrun');\n  }\n  const begIdx = w.begIdx() + lo;\n");
    for o in &func.outputs {
        let _ = writeln!(it, "  w.{}({1}At, count, {1});", if is_real(&o.param_type) { "realsInto" } else { "intsInto" }, o.name);
    }
    it.push_str("  w.done();\n  return new OutRange(begIdx, count);\n");

    let mut sig = api_input_sig(func);
    sig.push(format!("options?: {}", names.options_type));
    let mut wasm_sig = api_input_sig(func);
    wasm_sig.push(format!("options: {} | undefined", names.options_type));
    let mut call_args = arg_names.clone();
    call_args.push("options".into());
    let call_args = call_args.join(", ");
    let positional_sig = ts::positional_params(func);
    let positional_args = args(&["startIdx", "endIdx"], &inputs, &out_names, &["core"]);
    let mut positional_wasm_sig = positional_sig.clone();
    *positional_wasm_sig.last_mut().unwrap() = "core: Core".into();
    let head_sig = ["first: number".to_string(), "last: number".into(), "lo: number".into()];
    let param_sig: Vec<String> = params.iter().map(|o| format!("{o}: number")).collect();
    let public_type = |o: &crate::ir::Output| ts::public_array_ts_type(&o.param_type);
    let mut buffered_sig: Vec<String> = head_sig.to_vec();
    buffered_sig.extend(["lookback: number".to_string(), "n: number".into()]);
    buffered_sig.extend(raw.iter().map(|r| format!("{r}: ArrayLike<number>")));
    buffered_sig.extend(positional_sig[2 + inputs.len()..2 + inputs.len() + params.len()].iter().cloned());
    buffered_sig.extend(func.outputs.iter().map(|o| format!("{}: {}{}", o.name, public_type(o), if multi { " | undefined" } else { "" })));
    let mut into_sig: Vec<String> = head_sig.to_vec();
    into_sig.extend(inputs.iter().map(|i| format!("{i}: Float64Array")));
    into_sig.extend(param_sig.iter().cloned());
    into_sig.extend(func.outputs.iter().map(|o| format!("{}: {}", o.name, ts::array_ts_type(&o.param_type))));
    into_sig.push("room: number".into());

    let mut imports = vec![
        "engineDown", "engineFor", "engineRoute", "fellThrough", "isInt32", "optionalInt32", "outputRoom", "refused", "routedSeries",
        "seriesBars", "seriesLength", "strictFallThrough", "trapped", "usesDefaultCore", "wasmFrame",
    ];
    if has_real {
        imports.extend(["isReal", "optionalReal"]);
    }
    imports.sort_unstable();
    let mut enum_types: Vec<&str> = func
        .optional_inputs
        .iter()
        .filter_map(|o| match &o.param_type {
            ParamType::Enum(e) => Some(e.as_str()),
            _ => None,
        })
        .collect();
    enum_types.sort_unstable();
    enum_types.dedup();

    let mut s = ts_license();
    s.push_str("\n/* Generated by ta_codegen \u{2014} do not edit. */\n\n");
    let _ = writeln!(
        s,
        "import {{ {} as {js_fn}, {}, {}, {} as {js_positional} }} from '{js_module}';",
        names.public, names.lookback_positional, names.options_of, names.positional
    );
    let _ = writeln!(s, "import type {{ {}, {} }} from '{js_module}';", names.options_type, names.result_type);
    if !enum_types.is_empty() {
        let _ = writeln!(s, "import type {{ {} }} from '../../enums.ts';", enum_types.join(", "));
    }
    s.push_str("import { Core } from '../../runtime/core.ts';\n");
    s.push_str("import { OutRange } from '../../runtime/outRange.ts';\n");
    let _ = writeln!(s, "import {{ {} }} from '{RUNTIME}';\n", imports.join(", "));
    let _ = writeln!(s, "export * from '{js_module}';");
    if let Some(r) = route {
        let _ = writeln!(s, "\nconst WASM_FROM_BARS = {};", r.from);
        if let Some(to) = r.to {
            let _ = writeln!(s, "const WASM_TO_BARS = {to};");
        }
    }
    s.push_str(&wrapper(
        &Front {
            docs: &super::ts_doc::batch_docs(func, enums, registry),
            public: &names.public,
            sig: &sig.join(", "),
            result: &names.result_type,
            wasm_call: &format!("{wasm_fn}({call_args})"),
            js_call: &format!("{js_fn}({call_args})"),
        },
        &func.name,
        route.is_some(),
    ));
    s.push_str(&wrapper(
        &Front {
            docs: "/** @internal */\n",
            public: &names.positional,
            sig: &format!("\n  {},\n", positional_sig.join(",\n  ")),
            result: "OutRange",
            wasm_call: &format!("{positional_wasm_fn}({positional_args})"),
            js_call: &format!("{js_positional}({positional_args})"),
        },
        &func.name,
        route.is_some(),
    ));
    let _ = write!(s, "\nfunction {wasm_fn}({}): {} | null {{\n{b}}}\n", wasm_sig.join(", "), names.result_type);
    let _ = write!(s, "\nfunction {buffered_fn}({}): {} | null {{\n{bf}}}\n", buffered_sig.join(", "), names.result_type);
    let _ = write!(s, "\nfunction {positional_wasm_fn}({}): OutRange | null {{\n{p}}}\n", positional_wasm_sig.join(", "));
    let _ = write!(s, "\nfunction {into_fn}({}): OutRange | null {{\n{it}}}\n", into_sig.join(", "));
    s
}

/// The default entry's barrel: the `./js` barrel's names, each routed
/// function from its routed module, and `ready`.
#[must_use]
pub fn render_index(funcs: &[FuncDef]) -> String {
    let routed: BTreeSet<&str> = routed(funcs).iter().map(|f| f.name.as_str()).collect();
    let module_of = |f: &FuncDef| {
        if routed.contains(f.name.as_str()) {
            format!("./func/{}.ts", module_stem(f))
        } else {
            format!("../func/{}.ts", module_stem(f))
        }
    };
    ts::render_barrel(
        funcs,
        "../",
        &module_of,
        "export { ready } from '../runtime/wasm.ts';\nexport type { EngineReason, EngineStatus } from '../runtime/wasm.ts';\n",
    )
}

/// What an engine-less entry exports beyond its pure twin's names.
pub const PLAIN_ENGINE_EXPORTS: &str =
    "export { ready } from '../runtime/plain.ts';\nexport type { EngineReason, EngineStatus } from '../runtime/wasm.ts';\n";

/// The default entry for a runtime that cannot compile WebAssembly from
/// bytes: the `./js` barrel's names and a `ready` that loads nothing.
#[must_use]
pub fn render_plain_entry(funcs: &[FuncDef]) -> String {
    ts::render_barrel(funcs, "../", &|f| format!("../func/{}.ts", module_stem(f)), PLAIN_ENGINE_EXPORTS)
}

/// A `node`, `bun` and `deno` entry beside its `index.ts`, one directory
/// under `src/`: that entry, with the engine's bytes.
#[must_use]
pub fn node_entry_body() -> String {
    "import { provideWasmBytes } from '../runtime/wasm.ts';\n\
     import { WASM_BASE64 } from '../runtime/wasmBytes.js';\n\n\
     provideWasmBytes(WASM_BASE64);\n\n\
     export * from './index.ts';\n"
        .to_string()
}

/// The default entry's `node`, `bun` and `deno` entry.
#[must_use]
pub fn render_node_entry() -> String {
    let mut s = ts_license();
    s.push_str("\n/* Generated by ta_codegen \u{2014} do not edit. */\n\n");
    s.push_str(&node_entry_body());
    s
}

/// Write `src/engine/` under `lib`, deleting a routed module whose function
/// no longer has an export.
///
/// # Panics
/// When the routing table is invalid or an output path cannot be written.
#[allow(clippy::implicit_hasher)]
pub fn generate(funcs: &[FuncDef], enums: &HashMap<String, EnumDef>, registry: &Registry, root: &Path, lib: &Path) {
    let table = load_routing(&root.join("ta_codegen/input").join(ROUTING_FILE), funcs);
    let dir = lib.join("src/engine");
    let func_dir = dir.join("func");
    std::fs::create_dir_all(&func_dir).unwrap();
    let mut written = BTreeSet::new();
    for f in routed(funcs) {
        let name = format!("{}.ts", module_stem(f));
        crate::emit::write_if_changed(func_dir.join(&name), render_routed(f, funcs, enums, registry, table.get(&f.name).copied())).unwrap();
        written.insert(name);
    }
    for entry in std::fs::read_dir(&func_dir).unwrap().flatten() {
        let name = entry.file_name().to_string_lossy().to_string();
        if !written.contains(&name) {
            std::fs::remove_file(entry.path()).unwrap();
        }
    }
    crate::emit::write_if_changed(dir.join("index.ts"), render_index(funcs)).unwrap();
    crate::emit::write_if_changed(dir.join("node.ts"), render_node_entry()).unwrap();
    crate::emit::write_if_changed(dir.join("plain.ts"), render_plain_entry(funcs)).unwrap();
    println!("  TypeScript default entry -> {} ({} routed, {} in the table)", dir.display(), written.len(), table.len());
}

fn literal(v: f64) -> String {
    format!("{v:?}")
}

fn declined_spellings(opt: &OptInput, funcs: &[FuncDef], enums: &HashMap<String, EnumDef>) -> Vec<String> {
    let ParamType::Enum(enum_name) = &opt.param_type else {
        return Vec::new();
    };
    let declined = wasm::declined_values(opt, funcs, enums);
    enums[enum_name]
        .variants
        .iter()
        .filter(|v| declined.contains(&v.value))
        .map(|v| TsSpelling.member(enum_name, &v.name))
        .collect()
}

fn param_row(opt: &OptInput, funcs: &[FuncDef], enums: &HashMap<String, EnumDef>) -> String {
    let key = param_key(&opt.name);
    let (lo, hi) = opt.range.unwrap_or((1.0, 50.0));
    let dflt = literal(opt.default.unwrap_or(lo));
    match &opt.param_type {
        ParamType::Enum(enum_name) => {
            let declined = wasm::declined_values(opt, funcs, enums);
            let members: Vec<String> = enums[enum_name]
                .variants
                .iter()
                .filter(|v| !v.reserved && !declined.contains(&v.value))
                .map(|v| v.value.to_string())
                .collect();
            format!("{{ key: '{key}', kind: 'enum', dflt: {dflt}, min: 0, max: 0, members: [{}] }}", members.join(", "))
        }
        t => {
            let kind = if is_real(t) { "real" } else { "int" };
            format!("{{ key: '{key}', kind: '{kind}', dflt: {dflt}, min: {}, max: {}, members: [] }}", literal(lo), literal(hi))
        }
    }
}

/// Each input's data series, by input name: a price component its own, a
/// period series `periods`, and plain series in turn from close, open, high, low.
fn input_data(func: &FuncDef) -> BTreeMap<String, String> {
    const PLAIN: [&str; 4] = ["close", "open", "high", "low"];
    let mut plain = 0;
    let mut out = BTreeMap::new();
    for a in api_inputs(func) {
        for (i, _) in &a.legs {
            let series = if a.is_price {
                i.price.expect("a price component").component.token().to_string()
            } else if leg_kind(&i.name) == "inPeriods" {
                "periods".to_string()
            } else {
                plain += 1;
                PLAIN[(plain - 1) % PLAIN.len()].to_string()
            };
            out.insert(i.name.clone(), format!("d.{series}"));
        }
    }
    out
}

/// The options call's inputs.
fn args_row(func: &FuncDef) -> String {
    let data = input_data(func);
    let parts: Vec<String> = api_inputs(func)
        .iter()
        .map(|a| {
            if a.is_price {
                let keys: Vec<String> = a
                    .legs
                    .iter()
                    .map(|(i, _)| format!("{}: {}", i.price.expect("a price component").component.token(), data[&i.name]))
                    .collect();
                format!("{{ {} }}", keys.join(", "))
            } else {
                data[&a.legs[0].0.name].clone()
            }
        })
        .collect();
    format!("(d) => [{}]", parts.join(", "))
}

/// `<n>Positional`'s inputs, the same series as [`args_row`]'s.
fn legs_row(func: &FuncDef) -> String {
    let data = input_data(func);
    let legs: Vec<&str> = func.inputs.iter().map(|i| data[&i.name].as_str()).collect();
    format!("(d) => [{}]", legs.join(", "))
}

/// The output kinds, `f` for a `Float64Array` and `i` for an `Int32Array`.
fn outs_row(func: &FuncDef) -> String {
    let kinds: Vec<&str> = func.outputs.iter().map(|o| if is_real(&o.param_type) { "'f'" } else { "'i'" }).collect();
    format!("[{}]", kinds.join(", "))
}

/// The options keys of the output buffers.
fn bufs_row(func: &FuncDef) -> String {
    let keys: Vec<String> = func.outputs.iter().map(|o| format!("'{}'", buffer_key(func, o))).collect();
    format!("[{}]", keys.join(", "))
}

/// The fields every row carries: the function, its module and its call shapes.
fn row_head(f: &FuncDef) -> String {
    format!(
        "name: '{}', fn: '{}', pos: '{}', lookback: '{}', stem: '{}', args: {}, legs: {}, outs: {}",
        f.name,
        ts_names(&f.name).public,
        ts_names(&f.name).positional,
        ts_names(&f.name).lookback,
        module_stem(f),
        args_row(f),
        legs_row(f),
        outs_row(f)
    )
}

/// `test/engineParity.test.ts`.
#[allow(clippy::implicit_hasher)]
#[must_use]
pub fn render_parity_test(funcs: &[FuncDef], enums: &HashMap<String, EnumDef>) -> String {
    let mut sorted = routed(funcs);
    sorted.sort_by_key(|f| module_stem(f));
    let mut o = String::from(super::ta_abstract_c::LICENSE);
    o.push_str("\n/* Generated by ta_codegen \u{2014} do not edit. */\n\n");
    o.push_str(PARITY_HEAD);
    o.push_str("\nconst ROWS: Row[] = [\n");
    for f in &sorted {
        let declined: Vec<Vec<String>> = f.optional_inputs.iter().map(|p| declined_spellings(p, funcs, enums)).collect();
        let (per_in, per_out) = frame_bytes(f);
        let _ = writeln!(o, "  {{\n    {},\n    bufs: {}, frame: [{per_in}, {per_out}],", row_head(f), bufs_row(f));
        let params: Vec<String> = f.optional_inputs.iter().map(|p| param_row(p, funcs, enums)).collect();
        if params.is_empty() {
            o.push_str("    params: [],\n");
        } else {
            let _ = writeln!(o, "    params: [\n      {},\n    ],", params.join(",\n      "));
        }
        o.push_str("    vectors: [\n");
        for v in vectors(f, enums, &TsSpelling) {
            if v.values.iter().zip(&declined).any(|(val, d)| d.contains(val)) {
                continue;
            }
            let mut row = vec![format!("'{}'", v.label)];
            row.extend(v.values);
            let _ = writeln!(o, "      [{}],", row.join(", "));
        }
        o.push_str("    ],\n  },\n");
    }
    o.push_str("];\n");
    o.push_str(WALK);
    o.push_str(PARITY_DRIVER);
    o
}

/// Write `test/engineParity.test.ts` under `test_dir`.
#[allow(clippy::implicit_hasher)]
pub fn generate_parity_test(funcs: &[FuncDef], enums: &HashMap<String, EnumDef>, test_dir: &Path) {
    std::fs::create_dir_all(test_dir).unwrap();
    super::write_if_changed(&test_dir.join("engineParity.test.ts"), &render_parity_test(funcs, enums), "engineParity.test.ts", routed(funcs).len());
}

fn routing_row(f: &FuncDef, route: Route) -> String {
    let sentinels: Vec<&str> = f.optional_inputs.iter().map(sentinel).collect();
    let to = route.to.map_or(String::new(), |to| format!(", to: {to}"));
    format!("{{ {}, bufs: {}, sentinels: [{}], bars: {}{to} }}", row_head(f), bufs_row(f), sentinels.join(", "), route.from)
}

/// `test/engineRouting.test.ts`: the table mode against the routing table,
/// read here and not from what [`render_routed`] was handed, so a routed
/// module whose bounds drifted from the table fails it.
///
/// # Panics
/// When no function has a WASM export.
#[must_use]
pub fn render_routing_test(funcs: &[FuncDef], table: &BTreeMap<String, Route>) -> String {
    let mut sorted = routed(funcs);
    sorted.sort_by_key(|f| module_stem(f));
    let untabled = sorted.iter().find(|f| !table.contains_key(&f.name)).expect("an exported function the table does not name");
    let max = table.values().map(|r| r.to.unwrap_or(r.from).saturating_add(RANGED_PAD)).max().unwrap_or(0).max(UNTABLED_BARS);
    let mut o = String::from(super::ta_abstract_c::LICENSE);
    o.push_str("\n/* Generated by ta_codegen \u{2014} do not edit. */\n\n");
    o.push_str(ROUTING_HEAD);
    o.push_str("\nconst TABLED: Row[] = [\n");
    for f in sorted.iter().filter(|f| table.contains_key(&f.name)) {
        let _ = writeln!(o, "  {},", routing_row(f, table[&f.name]));
    }
    o.push_str("];\n");
    let _ = write!(
        o,
        "const TABLED_FLOOR = {};\nconst UNTABLED: Row = {};\nconst RANGED_PAD = {RANGED_PAD};\nconst MAX_BARS = {max};\n",
        table.len(),
        routing_row(untabled, Route { from: UNTABLED_BARS, to: None })
    );
    o.push_str(WALK);
    o.push_str(ROUTING_DRIVER);
    o
}

/// Write `test/engineRouting.test.ts` under `test_dir`.
///
/// # Panics
/// When the routing table is invalid.
pub fn generate_routing_test(funcs: &[FuncDef], root: &Path, test_dir: &Path) {
    let table = load_routing(&root.join("ta_codegen/input").join(ROUTING_FILE), funcs);
    std::fs::create_dir_all(test_dir).unwrap();
    super::write_if_changed(&test_dir.join("engineRouting.test.ts"), &render_routing_test(funcs, &table), "engineRouting.test.ts", table.len());
}

const UNTABLED_BARS: u32 = 100_000;

/// Bars past an entry's last count in the input its ranged calls slice: room
/// for a copy of one bar over it that starts at bar 2.
const RANGED_PAD: u32 = 3;

const ROUTING_HEAD: &str = r"import assert from 'node:assert/strict';
import { test } from 'node:test';

import * as routed from 'ta-lib';
import * as js from 'ta-lib/js';
import { Core } from 'ta-lib/js';
import { hooks } from './engineHooks.ts';

type Fn = (...args: unknown[]) => Record<string, unknown>;
type Arr = Float64Array | Int32Array;
type Data = { open: Float64Array; high: Float64Array; low: Float64Array; close: Float64Array; volume: Float64Array; periods: Float64Array };
type Row = {
  name: string; fn: string; pos: string; lookback: string; stem: string; args: (d: Data) => unknown[]; legs: (d: Data) => unknown[]; outs: string[];
  bufs: string[]; sentinels: number[]; bars: number; to?: number;
};
type Range = { begIdx: number; count: number };
";

const ROUTING_DRIVER: &str = r"
const DATA = walk(20261112, MAX_BARS);
const MODULES = new Map<string, Modules>();
for (const row of [...TABLED, UNTABLED]) MODULES.set(row.stem, await modules(row.stem));

function head(n: number): Data {
  const d = {} as Record<string, Float64Array>;
  for (const [key, series] of Object.entries(DATA)) d[key] = series.slice(0, n);
  return d as unknown as Data;
}

type Ran = { ran: number; fell: number };
type Span = { startIdx?: number; endIdx?: number };

/** One call through each entry at `n` bars, over `span` when given: the module's batch calls and the fall-throughs the default entry's call made. */
function call(row: Row, n: number, span?: Span): Ran {
  const args = span === undefined ? row.args(head(n)) : [...row.args(head(n)), span];
  const where = `${row.name} at ${n} bars${span === undefined ? '' : ` over ${JSON.stringify(span)}`}`;
  const fell = hooks.fallThroughCount();
  const entered = hooks.moduleCalls();
  const a = (routed as unknown as Record<string, Fn>)[row.fn](...args);
  const ran = hooks.moduleCalls() - entered;
  const counted = hooks.fallThroughCount() - fell;
  const b = (js as unknown as Record<string, Fn>)[row.fn](...args);
  assert.deepEqual(Object.keys(a), Object.keys(b), where);
  assert.equal(a.begIdx, b.begIdx, `${where}: begIdx`);
  for (const key of Object.keys(b).filter((x) => x !== 'begIdx')) {
    assert.ok(bytes(a[key] as Arr).equals(bytes(b[key] as Arr)), `${where}: ${key} bits differ`);
  }
  return { ran, fell: counted };
}

/** {@link call} with a caller buffer for every output. */
function callBuffered(row: Row, n: number, span?: Span): Ran {
  const d = head(n);
  const where = `${row.name} into buffers at ${n} bars${span === undefined ? '' : ` over ${JSON.stringify(span)}`}`;
  const mine = blank(row.outs, n);
  const theirs = blank(row.outs, n);
  const given = (bs: Arr[]): unknown[] => [...row.args(d), { ...span, ...Object.fromEntries(row.bufs.map((key, i) => [key, bs[i]])) }];
  const fell = hooks.fallThroughCount();
  const entered = hooks.moduleCalls();
  const a = (routed as unknown as Record<string, Fn>)[row.fn](...given(mine));
  const ran = hooks.moduleCalls() - entered;
  const counted = hooks.fallThroughCount() - fell;
  const b = (js as unknown as Record<string, Fn>)[row.fn](...given(theirs));
  assert.equal(a.begIdx, b.begIdx, `${where}: begIdx`);
  mine.forEach((x, k) => assert.ok(bytes(x).equals(bytes(theirs[k])), `${where}: buffer ${k} bits differ`));
  return { ran, fell: counted };
}

/** {@link call} for the positional tier, the one the Abstract API calls, with every parameter at its default. */
function callPositional(row: Row, n: number, span?: Span): Ran {
  const d = head(n);
  const s = span?.startIdx ?? 0;
  const e = span?.endIdx ?? n - 1;
  const where = `${row.pos} at ${n} bars over ${s}..${e}`;
  const m = MODULES.get(row.stem)!;
  const mine = blank(row.outs, n);
  const theirs = blank(row.outs, n);
  const fell = hooks.fallThroughCount();
  const entered = hooks.moduleCalls();
  const a = m.routed[row.pos](s, e, ...row.legs(d), ...row.sentinels, ...mine) as unknown as Range;
  const ran = hooks.moduleCalls() - entered;
  const counted = hooks.fallThroughCount() - fell;
  const b = m.js[row.pos](s, e, ...row.legs(d), ...row.sentinels, ...theirs) as unknown as Range;
  assert.deepEqual([a.begIdx, a.count], [b.begIdx, b.count], where);
  mine.forEach((x, k) => assert.ok(bytes(x).equals(bytes(theirs[k])), `${where}: buffer ${k} bits differ`));
  return { ran, fell: counted };
}

/** The range that copies `bars` bars from bar `lo` on, at the default lookback; `null` when every range copying from `lo > 0` copies more. */
function copying(row: Row, lo: number, bars: number): Span | null {
  if (lo === 0) return { startIdx: 0, endIdx: bars - 1 };
  const lookback = (js as unknown as Record<string, () => number>)[row.lookback]();
  return lookback < bars ? { startIdx: lo + lookback, endIdx: lo + bars - 1 } : null;
}

/** Every tabled function under and at its bar count, at and over the end of its range, over the explicit whole input, and over ranges of a longer input copying as many bars from bar 0, 1 and 2, and one untabled function at UNTABLED.bars, in every call form; returns the tabled functions, the ranged calls and those copying from a bar past 0 checked. */
function sweep(mode: string): { checked: number; ranged: number; shifted: number } {
  let checked = 0;
  let ranged = 0;
  let shifted = 0;
  for (const [form, run] of [['options', call], ['buffered', callBuffered], ['positional', callPositional]] as const) {
    for (const row of TABLED) {
      if (row.bars > 1) {
        const under = run(row, row.bars - 1);
        assert.equal(under.ran, 0, `${mode}, ${form}: ${row.name} at ${row.bars - 1} bars entered the module; the table routes it from ${row.bars}`);
        assert.equal(under.fell, 0, `${mode}, ${form}: ${row.name} at ${row.bars - 1} bars counted a fall-through`);
      }
      const at = run(row, row.bars);
      assert.equal(at.ran, 1, `${mode}, ${form}: ${row.name} at ${row.bars} bars: the module counted ${at.ran} batch calls, not one`);
      assert.equal(at.fell, 0, `${mode}, ${form}: ${row.name} at ${row.bars} bars fell through as '${hooks.lastFallThrough()}'`);
      if (row.to !== undefined) {
        const end = run(row, row.to);
        assert.equal(end.ran, 1, `${mode}, ${form}: ${row.name} at ${row.to} bars: the module counted ${end.ran} batch calls, not one`);
        assert.equal(end.fell, 0, `${mode}, ${form}: ${row.name} at ${row.to} bars fell through as '${hooks.lastFallThrough()}'`);
        const over = run(row, row.to + 1);
        assert.equal(over.ran, 0, `${mode}, ${form}: ${row.name} at ${row.to + 1} bars entered the module; the table routes it through ${row.to}`);
        assert.equal(over.fell, 0, `${mode}, ${form}: ${row.name} at ${row.to + 1} bars counted a fall-through`);
      }
      const whole = run(row, row.bars, { startIdx: 0, endIdx: row.bars - 1 });
      assert.equal(whole.ran, 1, `${mode}, ${form}: ${row.name} at ${row.bars} bars over the explicit whole input: the module counted ${whole.ran} batch calls, not one`);
      assert.equal(whole.fell, 0, `${mode}, ${form}: ${row.name} at ${row.bars} bars over the explicit whole input fell through as '${hooks.lastFallThrough()}'`);
      const n = (row.to ?? row.bars) + RANGED_PAD;
      const counts: [number, boolean][] = [[row.bars - 1, false], [row.bars, true]];
      if (row.to !== undefined) counts.push([row.to, true], [row.to + 1, false]);
      for (const lo of [0, 1, 2]) {
        for (const [bars, routes] of counts) {
          const span = bars < 1 ? null : copying(row, lo, bars);
          if (span === null) continue;
          const where = `${mode}, ${form}: ${row.name} at ${n} bars over ${JSON.stringify(span)}, copying ${bars} from bar ${lo}`;
          const r = run(row, n, span);
          assert.equal(r.ran, routes ? 1 : 0, routes ? `${where}: the module counted ${r.ran} batch calls, not one` : `${where} entered the module`);
          assert.equal(r.fell, 0, `${where} fell through as '${hooks.lastFallThrough()}'`);
          ranged++;
          if (lo > 0) shifted++;
        }
      }
      if (form === 'options') checked++;
    }
    const off = run(UNTABLED, UNTABLED.bars);
    assert.equal(off.ran, 0, `${mode}, ${form}: ${UNTABLED.name}, which the table does not name, entered the module`);
    assert.equal(off.fell, 0, `${mode}, ${form}: ${UNTABLED.name} counted a fall-through`);
  }
  return { checked, ranged, shifted };
}

test('a fresh engine routes by the table: each tabled function over the bars a call copies, whole input or ranged, and no other function', () => {
  const { checked, ranged, shifted } = sweep('fresh');
  assert.ok(checked >= TABLED_FLOOR, `${checked} tabled functions checked; the table names ${TABLED_FLOOR}`);
  assert.ok(ranged >= 3 * TABLED_FLOOR, `${ranged} ranged calls checked; at least one per call form for each of ${TABLED_FLOOR} tabled functions`);
  if (TABLED_FLOOR > 0) assert.ok(shifted > 0, 'no ranged call copied from a bar past 0');
  if (TABLED_FLOOR > 0) assert.deepEqual(hooks.engineStatus(), { engine: 'wasm', reason: null });
});

test('table mode routes by the table again after force mode', () => {
  hooks.setEngineMode('force');
  hooks.setEngineMode('table');
  const { checked, ranged } = sweep('table after force');
  assert.ok(checked >= TABLED_FLOOR && ranged >= 3 * TABLED_FLOOR);
});
";

const PARITY_HEAD: &str = r"import assert from 'node:assert/strict';
import { test } from 'node:test';

import * as routed from 'ta-lib';
import * as js from 'ta-lib/js';
import { Core, MAType, TALibError } from 'ta-lib/js';
import * as routedAbstract from 'ta-lib/abstract';
import * as jsAbstract from 'ta-lib/js/abstract';
import { hooks } from './engineHooks.ts';

type Fn = (...args: unknown[]) => Record<string, unknown>;
type Arr = Float64Array | Int32Array;
type Data = { open: Float64Array; high: Float64Array; low: Float64Array; close: Float64Array; volume: Float64Array; periods: Float64Array };
type Param = { key: string; kind: 'int' | 'real' | 'enum'; dflt: number; min: number; max: number; members: number[] };
type Row = {
  name: string; fn: string; pos: string; lookback: string; stem: string; args: (d: Data) => unknown[]; legs: (d: Data) => unknown[]; outs: string[];
  bufs: string[]; frame: [number, number]; params: Param[]; vectors: (string | number)[][];
};
type Range = { begIdx: number; count: number };
";

const WALK: &str = r"
function rng(seed: number): () => number {
  let s = seed >>> 0;
  return () => {
    s = (Math.imul(s, 1664525) + 1013904223) >>> 0;
    return s / 4294967296;
  };
}

function walk(seed: number, n: number): Data {
  const rnd = rng(seed);
  const cents = (x: number): number => Math.round(x * 100) / 100;
  const d: Data = {
    open: new Float64Array(n), high: new Float64Array(n), low: new Float64Array(n),
    close: new Float64Array(n), volume: new Float64Array(n), periods: new Float64Array(n),
  };
  let c = 100;
  for (let i = 0; i < n; i++) {
    const open = cents(c + rnd() - 0.5);
    c = cents(Math.max(5, c + (rnd() - 0.5) * 3));
    d.open[i] = open;
    d.close[i] = c;
    d.high[i] = cents(Math.max(open, c) + rnd() * 2);
    d.low[i] = cents(Math.min(open, c) - rnd() * 2);
    d.volume[i] = cents(1000 + rnd() * 9000);
    d.periods[i] = 2 + Math.floor(rnd() * 29);
  }
  return d;
}

function bytes(a: Arr): Buffer {
  return Buffer.from(a.buffer, a.byteOffset, a.byteLength);
}

type Modules = { routed: Record<string, Fn>; js: Record<string, Fn> };

/** A function's routed module and its ta-lib/js module, beside the entry 'ta-lib' resolves to: no subpath exports the positional tier. */
async function modules(stem: string): Promise<Modules> {
  const entry = import.meta.resolve('ta-lib');
  return {
    routed: (await import(new URL(`./func/${stem}.js`, entry).href)) as Record<string, Fn>,
    js: (await import(new URL(`../func/${stem}.js`, entry).href)) as Record<string, Fn>,
  };
}

/** Output buffers of `n` elements, each filled with a value no call computes, so an element written past the count shows. */
function blank(outs: string[], n: number): Arr[] {
  return outs.map((k) => (k === 'f' ? new Float64Array(n).fill(-777.25) : new Int32Array(n).fill(-777)));
}
";

const PARITY_DRIVER: &str = r"
const N = 400;
const CALL_SHAPED = ['options', 'parameter', 'input', 'range', 'rejected', 'declined'];
const DATA = walk(20261112, N);
const MODULES = new Map<string, Modules>();
for (const row of ROWS) MODULES.set(row.stem, await modules(row.stem));

function draw(p: Param, rnd: () => number): number {
  if (p.kind === 'enum') return p.members[Math.floor(rnd() * p.members.length)];
  if (p.kind === 'int') {
    const hi = Math.min(p.max, Math.max(p.min + 1, 3 * p.dflt));
    return p.min + Math.floor(rnd() * (hi - p.min + 1));
  }
  const span = Math.max(1, Math.abs(p.dflt));
  const lo = Math.max(p.min, p.dflt - span);
  const hi = Math.min(p.max, p.dflt + span);
  return Math.round((lo + rnd() * (hi - lo)) * 100) / 100;
}

function outcome(f: Fn, args: unknown[]): { value?: Record<string, unknown>; error?: unknown } {
  try {
    return { value: f(...args) };
  } catch (error) {
    return { error };
  }
}

function sameResult(a: Record<string, unknown>, b: Record<string, unknown>, where: string): void {
  assert.deepEqual(Object.keys(a), Object.keys(b), where);
  assert.equal(a.begIdx, b.begIdx, `${where}: begIdx`);
  for (const key of Object.keys(b).filter((x) => x !== 'begIdx')) {
    const x = a[key] as Arr;
    const y = b[key] as Arr;
    assert.equal(x.constructor, y.constructor, `${where}: ${key} type`);
    assert.equal(x.length, y.length, `${where}: ${key} length`);
    assert.ok(bytes(x).equals(bytes(y)), `${where}: ${key} bits differ`);
  }
}

/** Every element of each caller buffer, the ones past the count included. */
function sameBuffers(a: Arr[], b: Arr[], where: string): void {
  b.forEach((y, k) => assert.ok(bytes(a[k]).equals(bytes(y)), `${where}: buffer ${k} bits differ`));
}

test('every routed function answers in WASM with the bits of ta-lib/js, in strict mode, in every call form', () => {
  hooks.setEngineMode('strict');
  hooks.resetFallThroughs();
  const rnd = rng(7);
  let rejected = 0;
  let compared = 0;
  /** Both entries' answers to one call, or `null` once ta-lib/js rejected it and ta-lib rejected it alike. */
  const attempt = (where: string, f: Fn, g: Fn, mine: unknown[], theirs: unknown[]): [Record<string, unknown>, Record<string, unknown>] | null => {
    const want = outcome(g, theirs);
    const entered = hooks.moduleCalls();
    const got = outcome(f, mine);
    const ran = hooks.moduleCalls() - entered;
    if (want.error !== undefined) {
      assert.ok(want.error instanceof TALibError, `${where}: ta-lib/js threw ${String(want.error)}`);
      assert.ok(got.error instanceof TALibError, `${where}: ta-lib threw ${String(got.error)}, ta-lib/js ${want.error.code}`);
      assert.equal(got.error.code, want.error.code, where);
      assert.equal(got.error.message, want.error.message, where);
      const reason = hooks.lastFallThrough();
      assert.ok(CALL_SHAPED.includes(reason), `${where}: fell through as '${reason}', which no rejected call explains`);
      rejected++;
      return null;
    }
    assert.equal(got.error, undefined, `${where}: ${String(got.error)}`);
    assert.equal(ran, 1, `${where}: the module counted ${ran} batch calls, not one`);
    compared++;
    return [got.value as Record<string, unknown>, want.value as Record<string, unknown>];
  };
  try {
    for (const row of ROWS) {
      const f = (routed as unknown as Record<string, Fn>)[row.fn];
      const g = (js as unknown as Record<string, Fn>)[row.fn];
      const m = MODULES.get(row.stem)!;
      const sets: (string | number)[][] = [['omitted'], ...row.vectors];
      for (let k = 0; k < 2; k++) sets.push([`drawn ${k}`, ...row.params.map((p) => draw(p, rnd))]);
      let answered = 0;
      for (const [label, ...values] of sets) {
        const options: Record<string, unknown> = {};
        row.params.forEach((p, i) => {
          options[p.key] = values[i];
        });
        const params = row.params.map((p, i) => values[i] ?? (p.kind === 'real' ? Core.REAL_DEFAULT : Core.INTEGER_DEFAULT));
        const ranges: { startIdx?: number; endIdx?: number }[] = [
          {}, { startIdx: 37, endIdx: N - 11 }, { startIdx: 0, endIdx: 0 }, { startIdx: 3, endIdx: 120 }, { startIdx: N - 2, endIdx: N - 2 }, { startIdx: N - 1 },
        ];
        for (const range of ranges) {
          const where = `${row.name} ${label} ${JSON.stringify(range)}`;
          const args = [...row.args(DATA), { ...options, ...range }];
          const plain = attempt(where, f, g, args, args);
          if (plain !== null) {
            sameResult(plain[0], plain[1], where);
            answered++;
          }

          const mine = blank(row.outs, N);
          const theirs = blank(row.outs, N);
          const given = (bs: Arr[]): unknown[] => [...row.args(DATA), { ...options, ...range, ...Object.fromEntries(row.bufs.map((key, i) => [key, bs[i]])) }];
          const buffered = attempt(`${where} into buffers`, f, g, given(mine), given(theirs));
          if (buffered !== null) {
            sameResult(buffered[0], buffered[1], `${where} into buffers`);
            Object.keys(buffered[1]).filter((x) => x !== 'begIdx').forEach((key, k) => {
              const x = buffered[0][key] as Arr;
              const y = buffered[1][key] as Arr;
              assert.equal(x.buffer, mine[k].buffer, `${where} into buffers: ${key} is not a view of the caller's buffer`);
              assert.equal(x.byteOffset - mine[k].byteOffset, y.byteOffset - theirs[k].byteOffset, `${where} into buffers: ${key} offset`);
            });
          }
          sameBuffers(mine, theirs, `${where} into buffers`);

          const s = range.startIdx ?? 0;
          const e = range.endIdx ?? N - 1;
          const pm = blank(row.outs, N);
          const pt = blank(row.outs, N);
          const legs = row.legs(DATA);
          const positional = attempt(`${where} positional`, m.routed[row.pos], m.js[row.pos], [s, e, ...legs, ...params, ...pm], [s, e, ...legs, ...params, ...pt]);
          if (positional !== null) {
            const x = positional[0] as unknown as Range;
            const y = positional[1] as unknown as Range;
            assert.equal(x.constructor, y.constructor, `${where} positional: result type`);
            assert.deepEqual([x.begIdx, x.count], [y.begIdx, y.count], `${where} positional`);
          }
          sameBuffers(pm, pt, `${where} positional`);
        }
      }
      assert.ok(answered > 0, `${row.name}: no parameter set was accepted, so nothing was compared`);
    }
    assert.equal(hooks.fallThroughCount(), rejected, 'a call ta-lib/js accepts fell through');
    assert.deepEqual(hooks.engineStatus(), { engine: 'wasm', reason: null });
    console.log(`${ROWS.length} functions, ${compared} calls compared, ${rejected} rejected by both`);
  } finally {
    hooks.setEngineMode('table');
  }
});

test('a ranged call copies only the bars its range reads: under a budget of just those, the last bar runs in WASM in every call form, and a byte less falls through', () => {
  const previous = hooks.setBudget(hooks.BUDGET);
  try {
    for (const row of ROWS) {
      const lookback = (js as unknown as Record<string, () => number>)[row.lookback]();
      const lo = N - 1 - lookback;
      assert.ok(lo > 0, `${row.name}: a lookback of ${lookback} leaves no bar before the copy of the last bar of ${N}`);
      const budget = (N - lo) * row.frame[0] + row.frame[1];
      const f = (routed as unknown as Record<string, Fn>)[row.fn];
      const g = (js as unknown as Record<string, Fn>)[row.fn];
      const m = MODULES.get(row.stem)!;
      const sentinels = row.params.map((p) => (p.kind === 'real' ? Core.REAL_DEFAULT : Core.INTEGER_DEFAULT));
      const forms: [string, (outs: Arr[]) => unknown][] = [
        ['options', () => f(...row.args(DATA), { startIdx: N - 1 })],
        ['buffered', (outs) => f(...row.args(DATA), { startIdx: N - 1, ...Object.fromEntries(row.bufs.map((key, i) => [key, outs[i]])) })],
        ['positional', (outs) => m.routed[row.pos](N - 1, N - 1, ...row.legs(DATA), ...sentinels, ...outs)],
      ];
      const want = g(...row.args(DATA), { startIdx: N - 1 });
      const keys = Object.keys(want).filter((key) => key !== 'begIdx');
      for (const [form, run] of forms) {
        for (const [mode, room] of [['strict', budget], ['force', budget - 1]] as const) {
          const where = `${row.name}, ${form}, last bar, ${mode} mode under a budget of ${room} bytes`;
          hooks.setEngineMode(mode);
          hooks.setBudget(room);
          const outs = blank(row.outs, 1);
          const fell = hooks.fallThroughCount();
          const entered = hooks.moduleCalls();
          const got = run(outs) as Record<string, unknown>;
          const ran = hooks.moduleCalls() - entered;
          if (room === budget) {
            assert.equal(ran, 1, `${where}: the module counted ${ran} batch calls, not one`);
          } else {
            assert.equal(ran, 0, `${where} entered the module`);
            assert.equal(hooks.fallThroughCount() - fell, 1, `${where}: no fall-through counted`);
            assert.equal(hooks.lastFallThrough(), 'budget', where);
          }
          if (form === 'options') {
            sameResult(got, want, where);
          } else {
            const count = form === 'positional' ? (got as unknown as Range).count : (got[keys[0]] as Arr).length;
            assert.deepEqual([got.begIdx, count], [want.begIdx, (want[keys[0]] as Arr).length], where);
            keys.forEach((key, k) => assert.ok(bytes(outs[k]).equals(bytes(want[key] as Arr)), `${where}: buffer ${k} bits differ`));
          }
        }
      }
    }
  } finally {
    hooks.setBudget(previous);
    hooks.setEngineMode('table');
  }
});

test('ta-lib/abstract runs each routed function in WASM with the bits of ta-lib/js/abstract, which never enters the module', () => {
  assert.equal(routedAbstract.FuncInfo, jsAbstract.FuncInfo);
  assert.equal(routedAbstract.ParamHolder, jsAbstract.ParamHolder);
  assert.notEqual(routedAbstract.Functions, jsAbstract.Functions);
  const args = new Map(ROWS.map((row) => [row.name, row.args(DATA)]));
  const call = (lib: typeof jsAbstract, name: string, outs: Arr[]): { range: Range; ran: number } => {
    const info = lib.Functions.byName(name)!;
    const h = info.newCall();
    const given = args.get(name)!;
    assert.equal(info.inputs.length, given.length, `${name}: inputs`);
    info.inputs.forEach((input, i) => {
      if (input.type === lib.InputType.PRICE) h.setPriceInput(i, given[i] as jsAbstract.PriceInput);
      else h.setInput(i, given[i] as ArrayLike<number>);
    });
    outs.forEach((out, k) => h.setOutput(k, out));
    const entered = hooks.moduleCalls();
    const range = h.call(0, N - 1);
    return { range, ran: hooks.moduleCalls() - entered };
  };
  hooks.setEngineMode('strict');
  hooks.resetFallThroughs();
  try {
    for (const row of ROWS) {
      const mine = blank(row.outs, N);
      const theirs = blank(row.outs, N);
      const a = call(routedAbstract, row.name, mine);
      const b = call(jsAbstract, row.name, theirs);
      assert.equal(a.ran, 1, `${row.name}: ta-lib/abstract: the module counted ${a.ran} batch calls, not one`);
      assert.equal(b.ran, 0, `${row.name}: ta-lib/js/abstract entered the module`);
      assert.deepEqual([a.range.begIdx, a.range.count], [b.range.begIdx, b.range.count], row.name);
      sameBuffers(mine, theirs, `${row.name} through the abstract entries`);
    }
    assert.equal(hooks.fallThroughCount(), 0, `a call fell through as '${hooks.lastFallThrough()}'`);
  } finally {
    hooks.setEngineMode('table');
  }
});

test('the default entry replaces exactly the routed batch calls and re-exports every other value of ta-lib/js', () => {
  const r = routed as unknown as Record<string, unknown>;
  const j = js as unknown as Record<string, unknown>;
  const names = new Set(ROWS.map((row) => row.fn));
  assert.equal(names.size, ROWS.length);
  for (const key of Object.keys(j)) {
    if (names.has(key)) {
      assert.equal(typeof r[key], 'function', key);
      assert.notEqual(r[key], j[key], `${key} is not routed`);
    } else {
      assert.equal(r[key], j[key], `${key} is not the ta-lib/js value`);
    }
  }
  for (const name of names) assert.ok(name in j, `${name} is not a ta-lib/js export`);
});

test('every routed function rejects a key that is not one of its option keys before the module, as ta-lib/js does, and runs in WASM given every key it has', () => {
  hooks.setEngineMode('strict');
  hooks.resetFallThroughs();
  let rejected = 0;
  let ran = 0;
  try {
    for (const row of ROWS) {
      const f = (routed as unknown as Record<string, Fn>)[row.fn];
      const g = (js as unknown as Record<string, Fn>)[row.fn];
      const keys = [...row.params.map((p) => p.key), 'startIdx', 'endIdx', 'core', ...row.bufs];
      for (const bad of [{ [keys[0].toUpperCase()]: undefined }, new Map()]) {
        const where = `${row.name} ${bad instanceof Map ? 'a Map' : `key ${keys[0].toUpperCase()}`}`;
        const want = outcome(g, [...row.args(DATA), bad]);
        const entered = hooks.moduleCalls();
        const got = outcome(f, [...row.args(DATA), bad]);
        assert.ok(want.error instanceof TALibError && want.error.code === 'TA_BAD_PARAM', `${where}: ta-lib/js gave ${String(want.error)}`);
        assert.ok(got.error instanceof TALibError, `${where}: ta-lib threw ${String(got.error)}`);
        assert.equal(got.error.message, want.error.message, where);
        assert.equal(hooks.lastFallThrough(), 'options', where);
        assert.equal(hooks.moduleCalls(), entered, `${where}: entered the module`);
        rejected++;
      }
      const mine = blank(row.outs, N);
      const theirs = blank(row.outs, N);
      const every = (bs: Arr[]): Record<string, unknown> => ({
        ...Object.fromEntries(row.params.map((p) => [p.key, undefined])),
        startIdx: 0,
        endIdx: N - 1,
        core: Core.DEFAULT,
        ...Object.fromEntries(row.bufs.map((key, i) => [key, bs[i]])),
      });
      assert.deepEqual(Object.keys(every(mine)).sort(), [...keys].sort(), `${row.name}: the object misses a key`);
      const entered = hooks.moduleCalls();
      const got = outcome(f, [...row.args(DATA), every(mine)]);
      assert.equal(got.error, undefined, `${row.name} every key: ${String(got.error)}`);
      assert.equal(hooks.moduleCalls() - entered, 1, `${row.name} every key: the module counted no batch call`);
      sameResult(got.value!, outcome(g, [...row.args(DATA), every(theirs)]).value!, `${row.name} every key`);
      sameBuffers(mine, theirs, `${row.name} every key`);
      ran++;
    }
    assert.equal(hooks.fallThroughCount(), rejected, 'a call ta-lib/js accepts fell through');
  } finally {
    hooks.setEngineMode('table');
  }
  assert.equal(rejected, 2 * ROWS.length);
  assert.equal(ran, ROWS.length);
});
";
