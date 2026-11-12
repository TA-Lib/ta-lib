//! The npm package's default entry (`library/src/engine/`): for each function
//! with a WASM export, a module that re-exports its `./js` module and replaces
//! the public batch call with one that runs in the engine when it can; the
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

#[derive(serde::Deserialize)]
#[serde(deny_unknown_fields)]
struct RoutingFile {
    batch: BTreeMap<String, u32>,
}

/// Per function, the bar count from which its batch call goes to the engine.
///
/// # Panics
/// When the file cannot be read or parsed, names a function without a WASM
/// export, or gives a count below 1.
#[must_use]
pub fn load_routing(path: &Path, funcs: &[FuncDef]) -> BTreeMap<String, u32> {
    let text = std::fs::read_to_string(path).unwrap_or_else(|e| panic!("cannot read {}: {e}", path.display()));
    let file: RoutingFile = serde_yaml::from_str(&text).unwrap_or_else(|e| panic!("{}: {e}", path.display()));
    let exported: BTreeSet<&str> = wasm::exported(funcs).iter().map(|f| f.name.as_str()).collect();
    for (name, bars) in &file.batch {
        assert!(exported.contains(name.as_str()), "{}: {name} has no WASM export", path.display());
        assert!(*bars >= 1, "{}: {name} routes from {bars} bars; the least is 1", path.display());
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

/// One routed module.
#[allow(clippy::implicit_hasher, clippy::too_many_lines, clippy::cast_possible_truncation)]
#[must_use]
pub fn render_routed(func: &FuncDef, enums: &HashMap<String, EnumDef>, registry: &Registry, from_bars: Option<u32>) -> String {
    super::ts_doc::seed_function_names(registry.names());
    let names = ts_names(&func.name);
    let stem = module_stem(func);
    let js_fn = format!("{stem}Js");
    let wasm_fn = format!("{stem}Wasm");
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
    let has_real = func.optional_inputs.iter().any(|o| is_real(&o.param_type));
    let has_int = func.optional_inputs.iter().any(|o| !is_real(&o.param_type));

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
    if from_bars.is_some() {
        b.push_str("  if (engineRoute.mode === 0 && n < WASM_FROM_BARS) {\n    return null;\n  }\n");
    }
    b.push_str("  const o = routedOptions(options);\n");
    let mut buffers = String::new();
    for o in &func.outputs {
        let _ = write!(buffers, " || o.{} !== undefined", buffer_key(func, o));
    }
    let _ = writeln!(b, "  if (o === null{buffers}) {{\n    return fellThrough('options');\n  }}");
    b.push_str("  const startIdx = o.startIdx;\n  const endIdx = o.endIdx;\n");
    b.push_str(
        "  if (!usesDefaultCore(o.core) || !optionalInt32(startIdx) || !optionalInt32(endIdx)) {\n    return fellThrough('options');\n  }\n",
    );
    if !func.optional_inputs.is_empty() {
        let mut checks = Vec::new();
        for opt in &func.optional_inputs {
            let _ = writeln!(b, "  const {} = o.{};", opt.name, param_key(&opt.name));
            let check = if is_real(&opt.param_type) { "optionalReal" } else { "optionalInt32" };
            checks.push(format!("!{check}({})", opt.name));
        }
        let _ = writeln!(b, "  if ({}) {{\n    return fellThrough('parameter');\n  }}", checks.join(" || "));
    }
    for (i, r) in func.inputs.iter().zip(&raw) {
        let _ = writeln!(b, "  const {} = routedSeries({r}, n);", i.name);
    }
    let nulls: Vec<String> = func.inputs.iter().map(|i| format!("{} === null", i.name)).collect();
    let _ = writeln!(b, "  if ({}) {{\n    return fellThrough('input');\n  }}", nulls.join(" || "));
    b.push_str(
        "  const first = startIdx === undefined ? 0 : startIdx;\n  const last = endIdx === undefined ? n - 1 : endIdx;\n  \
         if (first < 0 || last < first || last >= n) {\n    return fellThrough('range');\n  }\n  const outLen = last - first + 1;\n",
    );
    let n_in = func.inputs.len() as u32;
    let (reals, ints): (Vec<_>, Vec<_>) = func.outputs.iter().partition(|o| is_real(&o.param_type));
    let n_real = reals.len() as u32;
    // Every read of a caller's argument stays above `wasmFrame`: a getter, a
    // Proxy trap or a `valueOf` can call a routed function back, which takes
    // the same scratch and may grow linear memory, detaching the views
    // `wasmFrame` derived.
    let (per_in, per_out) = frame_bytes(func);
    let bytes = plus(&format!("n * {per_in}"), &[(per_out, "outLen")]);
    let _ = writeln!(b, "  const w = wasmFrame({bytes});\n  if (w === null) {{\n    return null;\n  }}\n  const at = w.base;");
    for (k, i) in func.inputs.iter().enumerate() {
        let at = plus("at", &[(8 * k as u32, "n")]);
        let _ = writeln!(b, "  const {}At = {at};\n  w.f64.set({0}, {0}At / 8);", i.name);
    }
    for (j, o) in reals.iter().enumerate() {
        let _ = writeln!(b, "  const {}At = {};", o.name, plus("at", &[(8 * n_in, "n"), (8 * j as u32, "outLen")]));
    }
    for (j, o) in ints.iter().enumerate() {
        let _ = writeln!(b, "  const {}At = {};", o.name, plus("at", &[(8 * n_in, "n"), (8 * n_real + 4 * j as u32, "outLen")]));
    }
    let mut call: Vec<String> = vec!["first".into(), "last".into()];
    call.extend(func.inputs.iter().map(|i| format!("{}At", i.name)));
    call.push("n".into());
    call.extend(func.optional_inputs.iter().map(|o| format!("{0} === undefined ? {1} : {0}", o.name, sentinel(o))));
    call.extend(func.outputs.iter().map(|o| format!("{}At", o.name)));
    call.push("outLen".into());
    let export = format!("{}_batch", super::common::snake_words(&func.name));
    let _ = writeln!(
        b,
        "  let count: number;\n  try {{\n    count = w.x.{export}({});\n  }} catch (e) {{\n    return trapped(w, e);\n  }}\n  \
         if (count < 0) {{\n    return refused(count);\n  }}",
        call.join(", ")
    );
    let mut fields = vec!["begIdx: w.begIdx()".to_string()];
    for o in &func.outputs {
        let take = if is_real(&o.param_type) { "reals" } else { "ints" };
        fields.push(format!("{}: w.{take}({}At, count)", result_key(func, o), o.name));
    }
    let _ = writeln!(b, "  const result: {} = {{ {} }};\n  w.done();\n  return result;", names.result_type, fields.join(", "));

    let mut sig = api_input_sig(func);
    sig.push(format!("options?: {}", names.options_type));
    let mut wasm_sig = api_input_sig(func);
    wasm_sig.push(format!("options: {} | undefined", names.options_type));
    let mut args = arg_names.clone();
    args.push("options".into());
    let args = args.join(", ");

    let mut imports = vec!["engineRoute", "fellThrough", "optionalInt32", "refused", "routedOptions", "routedSeries", "seriesLength", "strictFallThrough", "trapped", "usesDefaultCore", "wasmFrame"];
    if has_real {
        imports.push("optionalReal");
    }
    imports.sort_unstable();

    let mut s = ts_license();
    s.push_str("\n/* Generated by ta_codegen \u{2014} do not edit. */\n\n");
    let _ = writeln!(s, "import {{ {} as {js_fn} }} from '{js_module}';", names.public);
    let _ = writeln!(s, "import type {{ {}, {} }} from '{js_module}';", names.options_type, names.result_type);
    if has_real || has_int {
        s.push_str("import { Core } from '../../runtime/core.ts';\n");
    }
    let _ = writeln!(s, "import {{ {} }} from '{RUNTIME}';\n", imports.join(", "));
    let _ = writeln!(s, "export * from '{js_module}';");
    if let Some(bars) = from_bars {
        let _ = writeln!(s, "\nconst WASM_FROM_BARS = {bars};");
    }
    // A caller's getter or trap that throws while the WASM path reads the
    // arguments is a fall-through: `./js` then throws its own error.
    let (open, pad, close) = if from_bars.is_some() { ("", "  ", "") } else { ("  if (engineRoute.mode !== 0) {\n", "    ", "  }\n") };
    let _ = write!(
        s,
        "\n{docs}export function {public}({sig}): {result} {{\n{open}\
         {pad}let routed: {result} | null;\n\
         {pad}try {{\n{pad}  routed = {wasm_fn}({args});\n{pad}}} catch {{\n{pad}  routed = fellThrough('threw');\n{pad}}}\n\
         {pad}if (routed !== null) {{\n{pad}  return routed;\n{pad}}}\n\
         {pad}if (engineRoute.mode === 2) {{\n{pad}  return strictFallThrough('{name}', {js_fn}({args}));\n{pad}}}\n\
         {close}  return {js_fn}({args});\n}}\n",
        docs = super::ts_doc::batch_docs(func, enums, registry),
        public = names.public,
        sig = sig.join(", "),
        result = names.result_type,
        name = func.name,
    );
    let _ = write!(s, "\nfunction {wasm_fn}({}): {} | null {{\n{b}}}\n", wasm_sig.join(", "), names.result_type);
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

/// The `node`, `bun` and `deno` entry: the barrel, with the engine's bytes.
#[must_use]
pub fn render_node_entry() -> String {
    let mut s = ts_license();
    s.push_str(
        "\n/* Generated by ta_codegen \u{2014} do not edit. */\n\n\
         import { provideWasmBytes } from '../runtime/wasm.ts';\n\
         import { WASM_BASE64 } from '../runtime/wasmBytes.js';\n\n\
         provideWasmBytes(WASM_BASE64);\n\n\
         export * from './index.ts';\n",
    );
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
        crate::emit::write_if_changed(func_dir.join(&name), render_routed(f, enums, registry, table.get(&f.name).copied())).unwrap();
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

fn args_row(func: &FuncDef) -> String {
    const PLAIN: [&str; 4] = ["close", "open", "high", "low"];
    let mut plain = 0;
    let parts: Vec<String> = api_inputs(func)
        .iter()
        .map(|a| {
            if a.is_price {
                let keys: Vec<String> = a
                    .legs
                    .iter()
                    .map(|(i, _)| {
                        let c = i.price.expect("a price component").component.token();
                        format!("{c}: d.{c}")
                    })
                    .collect();
                format!("{{ {} }}", keys.join(", "))
            } else if leg_kind(&a.legs[0].0.name) == "inPeriods" {
                "d.periods".to_string()
            } else {
                plain += 1;
                format!("d.{}", PLAIN[(plain - 1) % PLAIN.len()])
            }
        })
        .collect();
    format!("(d) => [{}]", parts.join(", "))
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
        let _ = writeln!(o, "  {{\n    name: '{}',\n    fn: '{}',\n    args: {},", f.name, ts_names(&f.name).public, args_row(f));
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
    o.push_str(PARITY_DRIVER);
    o
}

/// Write `test/engineParity.test.ts` under `test_dir`.
#[allow(clippy::implicit_hasher)]
pub fn generate_parity_test(funcs: &[FuncDef], enums: &HashMap<String, EnumDef>, test_dir: &Path) {
    std::fs::create_dir_all(test_dir).unwrap();
    super::write_if_changed(&test_dir.join("engineParity.test.ts"), &render_parity_test(funcs, enums), "engineParity.test.ts", routed(funcs).len());
}

const PARITY_HEAD: &str = r"import assert from 'node:assert/strict';
import { test } from 'node:test';

import * as routed from 'ta-lib';
import * as js from 'ta-lib/js';
import { Core, MAType, TALibError } from 'ta-lib/js';
import { hooks } from './engineHooks.ts';

type Fn = (...args: unknown[]) => Record<string, unknown>;
type Arr = Float64Array | Int32Array;
type Data = { open: Float64Array; high: Float64Array; low: Float64Array; close: Float64Array; volume: Float64Array; periods: Float64Array };
type Param = { key: string; kind: 'int' | 'real' | 'enum'; dflt: number; min: number; max: number; members: number[] };
type Row = { name: string; fn: string; args: (d: Data) => unknown[]; params: Param[]; vectors: (string | number)[][] };
";

const PARITY_DRIVER: &str = r"
const N = 400;

function rng(seed: number): () => number {
  let s = seed >>> 0;
  return () => {
    s = (Math.imul(s, 1664525) + 1013904223) >>> 0;
    return s / 4294967296;
  };
}

function walk(seed: number): Data {
  const rnd = rng(seed);
  const cents = (x: number): number => Math.round(x * 100) / 100;
  const d: Data = {
    open: new Float64Array(N), high: new Float64Array(N), low: new Float64Array(N),
    close: new Float64Array(N), volume: new Float64Array(N), periods: new Float64Array(N),
  };
  let c = 100;
  for (let i = 0; i < N; i++) {
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

const DATA = walk(20261112);

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

function bytes(a: Arr): Buffer {
  return Buffer.from(a.buffer, a.byteOffset, a.byteLength);
}

function outcome(f: Fn, args: unknown[]): { value?: Record<string, unknown>; error?: unknown } {
  try {
    return { value: f(...args) };
  } catch (error) {
    return { error };
  }
}

test('every routed function answers in WASM with the bits of ta-lib/js, in strict mode', () => {
  hooks.setEngineMode('strict');
  hooks.resetFallThroughs();
  const rnd = rng(7);
  let rejected = 0;
  let compared = 0;
  try {
    for (const row of ROWS) {
      const f = (routed as unknown as Record<string, Fn>)[row.fn];
      const g = (js as unknown as Record<string, Fn>)[row.fn];
      const sets: (string | number)[][] = [['omitted'], ...row.vectors];
      for (let k = 0; k < 2; k++) sets.push([`drawn ${k}`, ...row.params.map((p) => draw(p, rnd))]);
      let answered = 0;
      for (const [label, ...values] of sets) {
        const options: Record<string, unknown> = {};
        row.params.forEach((p, i) => {
          options[p.key] = values[i];
        });
        for (const range of [{}, { startIdx: 37, endIdx: N - 11 }]) {
          const args = [...row.args(DATA), { ...options, ...range }];
          const where = `${row.name} ${label} ${JSON.stringify(range)}`;
          const want = outcome(g, args);
          const entered = hooks.moduleCalls();
          const got = outcome(f, args);
          const ran = hooks.moduleCalls() - entered;
          if (want.error !== undefined) {
            assert.ok(want.error instanceof TALibError, `${where}: ta-lib/js threw ${String(want.error)}`);
            assert.ok(got.error instanceof TALibError, `${where}: ta-lib threw ${String(got.error)}, ta-lib/js ${want.error.code}`);
            assert.equal(got.error.code, want.error.code, where);
            assert.equal(got.error.message, want.error.message, where);
            rejected++;
            continue;
          }
          assert.equal(got.error, undefined, `${where}: ${String(got.error)}`);
          assert.equal(ran, 1, `${where}: the module counted ${ran} batch calls, not one`);
          const a = got.value as Record<string, unknown>;
          const b = want.value as Record<string, unknown>;
          assert.deepEqual(Object.keys(a), Object.keys(b), where);
          assert.equal(a.begIdx, b.begIdx, `${where}: begIdx`);
          for (const key of Object.keys(b).filter((x) => x !== 'begIdx')) {
            const x = a[key] as Arr;
            const y = b[key] as Arr;
            assert.equal(x.constructor, y.constructor, `${where}: ${key} type`);
            assert.equal(x.length, y.length, `${where}: ${key} length`);
            assert.ok(bytes(x).equals(bytes(y)), `${where}: ${key} bits differ`);
          }
          answered++;
          compared++;
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
";
