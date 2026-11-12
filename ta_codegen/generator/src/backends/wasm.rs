//! The WebAssembly engine: `ta_codegen/output/wasm/`, a cdylib over the Rust
//! crate exporting each function's batch tier as `<snake>_batch`, plus
//! `exports.json`, the description the TypeScript layer binds to.

use super::common::{snake_words, ENUM_DEFAULT_VARIANT};
use crate::ir::{EnumDef, FuncDef, OptInput, ParamType};
use std::collections::{BTreeSet, HashMap};
use std::fmt::Write as _;
use std::path::Path;

pub const TRANSCENDENTAL_FLAG: &str = "uses_transcendental";

const OUT_DIR: &str = "wasm";

fn is_transcendental(func: &FuncDef) -> bool {
    func.flags.iter().any(|f| f == TRANSCENDENTAL_FLAG)
}

/// The functions that get an export: every one whose own call never reaches a
/// transcendental routine (W10).
#[must_use]
pub fn exported(funcs: &[FuncDef]) -> Vec<&FuncDef> {
    funcs.iter().filter(|f| !is_transcendental(f)).collect()
}

/// The values of `opt`, an enum parameter, that select a transcendental
/// function: a member named after a function carrying the flag, and the two
/// default sentinels when the parameter's default is such a member.
#[must_use]
#[allow(clippy::implicit_hasher, clippy::cast_possible_truncation)]
pub fn declined_values(opt: &OptInput, funcs: &[FuncDef], enums: &HashMap<String, EnumDef>) -> Vec<i32> {
    let ParamType::Enum(enum_name) = &opt.param_type else {
        return Vec::new();
    };
    let def = enums
        .get(enum_name)
        .unwrap_or_else(|| panic!("{}: enum {enum_name} is not in enums.yaml", opt.name));
    let flagged: BTreeSet<&str> = funcs.iter().filter(|f| is_transcendental(f)).map(|f| f.name.as_str()).collect();
    let mut out: Vec<i32> = def
        .variants
        .iter()
        .filter(|v| !v.reserved && flagged.contains(v.name.as_str()))
        .map(|v| v.value)
        .collect();
    if opt.default.is_some_and(|d| out.contains(&(d as i32))) {
        if let Some(v) = def.variants.iter().find(|v| v.name == ENUM_DEFAULT_VARIANT) {
            out.push(v.value);
        }
        out.push(i32::MIN);
    }
    out
}

fn elem(t: &ParamType) -> &'static str {
    match t {
        ParamType::Real => "f64",
        ParamType::Integer | ParamType::Enum(_) | ParamType::Price(_) => "i32",
    }
}

fn value_kind(t: &ParamType) -> &'static str {
    match t {
        ParamType::Real => "real",
        _ => "int",
    }
}

fn render_export(func: &FuncDef, funcs: &[FuncDef], enums: &HashMap<String, EnumDef>) -> String {
    let snake = snake_words(&func.name);
    let mut s = String::new();
    let _ = writeln!(s, "#[no_mangle]\npub unsafe extern \"C\" fn {snake}_batch(");
    s.push_str("    startIdx: usize,\n    endIdx: usize,\n");
    for i in &func.inputs {
        let _ = writeln!(s, "    {}: *const {},", i.name, elem(&i.param_type));
    }
    s.push_str("    len: usize,\n");
    for o in &func.optional_inputs {
        let _ = writeln!(s, "    {}: {},", o.name, elem(&o.param_type));
    }
    for o in &func.outputs {
        let _ = writeln!(s, "    {}: *mut {},", o.name, elem(&o.param_type));
    }
    s.push_str("    out_len: usize,\n) -> i32 {\n");
    for o in &func.optional_inputs {
        if let ParamType::Enum(_) = o.param_type {
            let declined: Vec<String> = declined_values(o, funcs, enums).iter().map(i32::to_string).collect();
            let _ = writeln!(
                s,
                "    let Some({0}) = enum_param({0}, &[{1}]) else {{\n        return DECLINED;\n    }};",
                o.name,
                declined.join(", ")
            );
        }
    }
    let outs: Vec<String> = func.outputs.iter().map(|o| format!("span({}, out_len)", o.name)).collect();
    let ins: Vec<String> = func.inputs.iter().map(|i| format!("span({}, len)", i.name)).collect();
    let _ = writeln!(
        s,
        "    if !disjoint(&[{}], &[{}]) {{\n        return DECLINED;\n    }}",
        outs.join(", "),
        ins.join(", ")
    );
    let names: Vec<&str> = func.inputs.iter().map(|i| i.name.as_str()).chain(func.outputs.iter().map(|o| o.name.as_str())).collect();
    let slices: Vec<String> = func
        .inputs
        .iter()
        .map(|i| format!("input({}, len)", i.name))
        .chain(func.outputs.iter().map(|o| format!("output({}, out_len)", o.name)))
        .collect();
    let binds: Vec<String> = names.iter().map(|n| format!("Some({n})")).collect();
    let _ = writeln!(
        s,
        "    let ({},) = ({},) else {{\n        return DECLINED;\n    }};",
        binds.join(", "),
        slices.join(", ")
    );
    let args: Vec<&str> = ["startIdx", "endIdx"]
        .into_iter()
        .chain(func.inputs.iter().map(|i| i.name.as_str()))
        .chain(func.optional_inputs.iter().map(|o| o.name.as_str()))
        .chain(func.outputs.iter().map(|o| o.name.as_str()))
        .collect();
    let _ = writeln!(s, "    answer(CORE.{snake}({}))\n}}", args.join(", "));
    s
}

/// `src/lib.rs`: one export per [`exported`] function.
#[must_use]
#[allow(clippy::implicit_hasher)]
pub fn render_lib(funcs: &[FuncDef], enums: &HashMap<String, EnumDef>) -> String {
    let mut s = String::from(
        "//! Generated by ta_codegen from ta_codegen/input/: do not edit. The calling\n\
         //! convention is described in exports.json beside this crate's manifest.\n\
         #![allow(non_snake_case, clippy::missing_safety_doc, clippy::too_many_arguments)]\n\n\
         mod runtime;\n\nuse runtime::{answer, disjoint, enum_param, input, output, span, CORE, DECLINED};\n",
    );
    for func in exported(funcs) {
        s.push('\n');
        s.push_str(&render_export(func, funcs, enums));
    }
    s
}

/// `exports.json`: what the TypeScript layer needs to call each export.
#[must_use]
#[allow(clippy::implicit_hasher)]
pub fn render_exports_json(funcs: &[FuncDef], enums: &HashMap<String, EnumDef>) -> String {
    use serde_json::{json, Value};
    let functions: Vec<Value> = exported(funcs)
        .into_iter()
        .map(|f| {
            let params: Vec<Value> = f
                .optional_inputs
                .iter()
                .map(|o| match &o.param_type {
                    ParamType::Enum(e) => json!({
                        "name": o.name, "kind": "enum", "enum": e,
                        "declined": declined_values(o, funcs, enums),
                    }),
                    t => json!({ "name": o.name, "kind": value_kind(t) }),
                })
                .collect();
            json!({
                "name": f.name,
                "export": format!("{}_batch", snake_words(&f.name)),
                "inputs": f.inputs.iter().map(|i| json!({ "name": i.name, "kind": value_kind(&i.param_type) })).collect::<Vec<_>>(),
                "params": params,
                "outputs": f.outputs.iter().map(|o| json!({ "name": o.name, "kind": value_kind(&o.param_type) })).collect::<Vec<_>>(),
            })
        })
        .collect();
    let withheld: Vec<&str> = funcs.iter().filter(|f| is_transcendental(f)).map(|f| f.name.as_str()).collect();
    let doc = json!({
        "abi": {
            "batch": "<export>(startIdx, endIdx, <one pointer per input>, len, <params>, <one pointer per output>, out_len) -> i32. \
                      Indices and lengths are u32; an int or enum param is i32, a real one f64; a real buffer holds f64, an int one i32. \
                      Each input is read as exactly len elements and each output as exactly out_len. \
                      Returns the output count, or a negative value: -retCode when the Rust public tier rejects the call, \
                      declinedCode when the export refuses it itself (an enum value naming no member or in that param's declined list, a buffer that is \
                      null or misaligned with a non-zero length, an output overlapping another buffer).",
            "declinedCode": i32::MIN,
            "outRange": "out_range() -> address of two u32: the begin index and the count of the last call that succeeded",
            "scratch": "scratch(bytes) -> address of at least bytes bytes aligned for f64, kept across calls; \
                        a call that grows it moves it. scratch_capacity() -> bytes",
            "fmaProbe": "fma_probe(a, b, c) -> f64: a*b+c through the instruction the fused sites use",
        },
        "functions": functions,
        "withheld": withheld,
    });
    let mut text = serde_json::to_string_pretty(&doc).expect("exports.json serialises");
    text.push('\n');
    text
}

fn render_manifest(version: &str) -> String {
    format!(
        "[package]\nname = \"ta-lib-wasm\"\nversion = \"{version}\"\nedition = \"2021\"\npublish = false\n\n\
         [lib]\ncrate-type = [\"cdylib\"]\npath = \"src/lib.rs\"\n\n\
         [dependencies]\nta-lib = {{ path = \"../rust/library\" }}\n\n\
         [profile.release]\nopt-level = 3\nlto = \"fat\"\ncodegen-units = 1\npanic = \"abort\"\n\n\
         [workspace]\n"
    )
}

/// Write the crate under `out_base/wasm/` from every definition.
///
/// # Panics
/// When `VERSION`, the runtime template or an output path cannot be read or written.
#[allow(clippy::implicit_hasher)]
pub fn generate(funcs: &[FuncDef], enums: &HashMap<String, EnumDef>, root: &Path) {
    let dir = root.join("ta_codegen/output").join(OUT_DIR);
    let src = dir.join("src");
    std::fs::create_dir_all(&src).unwrap();
    let version = std::fs::read_to_string(root.join("VERSION")).expect("cannot read VERSION");
    let n = exported(funcs).len();
    super::write_if_changed(&dir.join("Cargo.toml"), &render_manifest(version.trim()), "wasm Cargo.toml", n);
    super::write_if_changed(&src.join("lib.rs"), &render_lib(funcs, enums), "wasm lib.rs", n);
    super::write_if_changed(&dir.join("exports.json"), &render_exports_json(funcs, enums), "wasm exports.json", n);
    let template = root.join("ta_codegen/generator/templates/wasm/runtime.rs");
    crate::emit::copy_if_changed(&template, &src.join("runtime.rs")).unwrap();
}
