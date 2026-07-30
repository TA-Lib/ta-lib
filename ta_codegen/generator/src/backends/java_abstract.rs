//! Java analog of [`rust_abstract`](super::rust_abstract) / C's `ta_abstract_serve.c`:
//! emits the `ta_abstract` introspection metadata table plus the JSON-RPC handlers
//! (`TA_GetFuncInfo`, `TA_Get{Input,OptInput,Output}ParameterInfo`,
//! `abstract_for_each_func`, `TA_FunctionDescriptionXML`) into the self-contained
//! Java server (`TaCodegenServe`), so `test_abstract.c` drives Java-vs-C metadata
//! parity (issue #114).
//!
//! The metadata VALUES — flag bit-masks, price-input collapsing, real sentinels —
//! are computed by the SAME helpers that build the C and Rust tables
//! ([`func_flag_bits`], [`price_bundle`], ...), so the three
//! backends agree by construction rather than by three hand-maintained copies.

#![allow(
    clippy::cast_possible_truncation,
    clippy::cast_sign_loss,
    clippy::cast_precision_loss
)]

use std::collections::HashMap;
use std::fmt::Write;

use super::price_bundle;
use super::rust_abstract::{func_flag_bits, opt_flag_bits, output_flag_bits};
use crate::ir::{EnumDef, FuncDef, Input, OptInput, Output, ParamType};

/// Format an `f64` as a valid Java `double` literal. Rust's `Debug` yields the
/// shortest round-tripping form (`30.0`, `0.1`, `3e37`) — all valid Java doubles.
fn jd(v: f64) -> String {
    format!("{v:?}")
}

/// Emit `s` as a Java string literal (escaping `"` and `\`).
fn js(s: &str) -> String {
    let mut o = String::from("\"");
    for c in s.chars() {
        if c == '"' || c == '\\' {
            o.push('\\');
        }
        o.push(c);
    }
    o.push('"');
    o
}

// ---------------------------------------------------------------------------
// Structured rows — the single source both Java metadata surfaces render from
// ---------------------------------------------------------------------------
//
// The values (flag bit-masks, price-input collapsing, real sentinels) are computed
// ONCE here, from the same helpers that build the C and Rust tables. The JSON-RPC
// server's inline table and the shipped `io.github.talib.metadata` registry are
// two renderers over these rows, so they cannot disagree — which is what lets
// `test_abstract.c`'s Java-vs-C parity check speak for the shipped registry too.

/// One input, with price components already folded into a single bundle.
#[derive(Clone)]
pub(crate) struct InRow {
    /// C `TA_InputParameterType`: Price=0, Real=1, Integer=2.
    pub ty: i32,
    pub param_name: String,
    pub flags: u32,
}

/// One output.
#[derive(Clone)]
pub(crate) struct OutRow {
    /// C `TA_OutputParameterType`: Real=0, Integer=1.
    pub ty: i32,
    pub param_name: String,
    pub flags: u32,
}

/// One optional input. Carries every domain's fields; `ty` selects which apply.
#[derive(Clone)]
pub(crate) struct OptRow {
    /// C `TA_OptInputParameterType`: RealRange=0, RealList=1, IntegerRange=2, IntegerList=3.
    pub ty: i32,
    pub param_name: String,
    pub flags: u32,
    pub display_name: String,
    /// Per-parameter help text. Present in the IR and the YAML but never rendered
    /// by the old hand-written island — the shipped registry populates it.
    pub hint: String,
    pub default_value: f64,
    // RealRange
    pub rmin: f64,
    pub rmax: f64,
    pub precision: i32,
    pub rsug: (f64, f64, f64),
    // IntegerRange
    pub imin: i32,
    pub imax: i32,
    pub isug: (i32, i32, i32),
    // IntegerList
    pub value_list: Option<String>,
}

/// One function's complete metadata row.
#[derive(Clone)]
pub(crate) struct FuncRow {
    pub name: String,
    pub group: String,
    pub hint: String,
    pub camel_case_name: String,
    /// The actual Java method name on `Core`. NOT the same as `camel_case_name`,
    /// which is C's `TA_FuncInfo.camelCaseName` and is Pascal-cased for several
    /// functions (`Accbands`, `MovingAverage`). Computed with the very function
    /// `java.rs` emits the method with, so it cannot drift from reality.
    pub java_method_name: String,
    pub flags: u32,
    pub inputs: Vec<InRow>,
    pub opt_inputs: Vec<OptRow>,
    pub outputs: Vec<OutRow>,
}

/// Build every function's row, name-sorted (matching `rust_abstract`'s `FUNCS`).
#[allow(clippy::implicit_hasher)]
pub(crate) fn rows(funcs: &[FuncDef], enums: &HashMap<String, EnumDef>) -> Vec<FuncRow> {
    let mut sorted: Vec<&FuncDef> = funcs.iter().collect();
    sorted.sort_by(|a, b| a.name.cmp(&b.name));
    sorted
        .into_iter()
        .map(|f| FuncRow {
            name: f.name.clone(),
            group: f.group.clone(),
            hint: f.hint.clone().unwrap_or_default(),
            camel_case_name: f.camel_case.clone().unwrap_or_else(|| f.name.clone()),
            java_method_name: super::java::to_java_method_name(&f.name, f.camel_case.as_deref()),
            flags: func_flag_bits(&f.flags),
            inputs: input_rows(&f.inputs),
            opt_inputs: f.optional_inputs.iter().map(|o| opt_row(o, enums)).collect(),
            outputs: output_rows(&f.outputs),
        })
        .collect()
}

/// Fold the parser's expanded price components back into a single `Price` input with
/// an OHLCV flag bitmask + canonical `inPriceXXX` name, via the shared
/// [`price_bundle::group`] the C and Rust abstract backends also use.
fn input_rows(inputs: &[Input]) -> Vec<InRow> {
    let mut out = Vec::new();
    for grouped in price_bundle::group(inputs) {
        match grouped {
            price_bundle::Grouped::Price(bundle) => {
                let components = price_bundle::components(&bundle);
                out.push(InRow {
                    ty: 0,
                    param_name: price_bundle::canonical_name(&components),
                    flags: price_bundle::flags(&components),
                });
            }
            price_bundle::Grouped::Single(inp) => match &inp.param_type {
                ParamType::Real => {
                    out.push(InRow { ty: 1, param_name: inp.name.clone(), flags: 0 });
                }
                ParamType::Integer => {
                    out.push(InRow { ty: 2, param_name: inp.name.clone(), flags: 0 });
                }
                ParamType::Price(_) | ParamType::Enum(_) => {}
            },
        }
    }
    out
}

fn output_rows(outputs: &[Output]) -> Vec<OutRow> {
    outputs
        .iter()
        .map(|out| OutRow {
            ty: i32::from(out.param_type == ParamType::Integer),
            param_name: out.name.clone(),
            flags: output_flag_bits(&out.flags),
        })
        .collect()
}

#[allow(clippy::implicit_hasher)]
fn opt_row(opt: &OptInput, enums: &HashMap<String, EnumDef>) -> OptRow {
    let base = OptRow {
        ty: 0,
        param_name: opt.name.clone(),
        flags: opt_flag_bits(&opt.flags),
        display_name: opt.display_name.clone().unwrap_or_else(|| opt.name.clone()),
        hint: opt.hint.clone().unwrap_or_default(),
        default_value: 0.0,
        rmin: 0.0,
        rmax: 0.0,
        precision: 0,
        rsug: (0.0, 0.0, 0.0),
        imin: 0,
        imax: 0,
        isug: (0, 0, 0),
        value_list: None,
    };
    match &opt.param_type {
        ParamType::Real => {
            let (min, max) = opt.range.unwrap_or((0.0, 0.0));
            let (sg, en, ic) = opt.suggested.unwrap_or((0.0, 0.0, 0.0));
            OptRow {
                ty: 0,
                default_value: opt.default.unwrap_or(0.0),
                rmin: min,
                rmax: max,
                precision: opt.precision.unwrap_or(0),
                rsug: (sg, en, ic),
                ..base
            }
        }
        ParamType::Integer => {
            let (min, max) = opt.range.unwrap_or((0.0, 0.0));
            let (min, max) = (min as i32, max as i32);
            let (sg, en, ic) = match opt.suggested {
                Some((a, b, c)) => (a as i32, b as i32, c as i32),
                None => (max, max, max),
            };
            OptRow {
                ty: 2,
                default_value: f64::from(opt.default.unwrap_or(0.0) as i32),
                imin: min,
                imax: max,
                isug: (sg, en, ic),
                ..base
            }
        }
        ParamType::Enum(name) => {
            let mut vl = String::new();
            if let Some(ed) = enums.get(name) {
                for (idx, v) in ed.variants.iter().enumerate() {
                    if idx > 0 {
                        vl.push(';');
                    }
                    let _ = write!(vl, "{}={}", i64::from(v.value), v.short_name);
                }
            }
            OptRow {
                ty: 3,
                default_value: opt.default.unwrap_or(0.0) as i64 as f64,
                value_list: Some(vl),
                ..base
            }
        }
        ParamType::Price(_) => OptRow { ty: 0, ..base },
    }
}

/// Generate the complete `ta_abstract` metadata block (nested data classes, the
/// populated `ABSTRACT` table, and the metadata RPC handler methods) as Java
/// source to be spliced into the `TaCodegenServe` class body.
///
/// `funcs` are enumerated in name-sorted order (matching `rust_abstract`'s `FUNCS`).
#[allow(clippy::implicit_hasher)]
pub fn generate(funcs: &[FuncDef], enums: &HashMap<String, EnumDef>) -> String {
    // The XML content is byte-identical to what C's TA_FunctionDescriptionXML and
    // the Rust server bake, so its length + unsigned-byte-sum are computed here at
    // generation time and emitted as constants (the Java server does not embed it).
    let xml = super::func_api_xml::generate_string(funcs);
    let xml_len = xml.len();
    let xml_checksum: u64 = xml.bytes().map(u64::from).sum();

    let mut s = String::new();
    s.push_str(META_CLASSES);

    // --- static metadata table ---
    s.push_str(
        "    static final java.util.LinkedHashMap<String,AbsFunc> ABSTRACT = new java.util.LinkedHashMap<>();\n",
    );
    s.push_str("    static {\n");
    for row in &rows(funcs, enums) {
        emit_func_registration(&mut s, row);
    }
    s.push_str("    }\n\n");

    s.push_str(&emit_handlers(xml_len, xml_checksum));
    s
}

/// One `ABSTRACT.put("NAME", new AbsFunc(...))` registration, rendered from a row.
fn emit_func_registration(s: &mut String, f: &FuncRow) {
    let _ = writeln!(
        s,
        "        ABSTRACT.put({}, new AbsFunc({}, {}, {}, {}, {},",
        js(&f.name),
        js(&f.name),
        js(&f.group),
        js(&f.hint),
        js(&f.camel_case_name),
        f.flags
    );
    let _ = writeln!(s, "            new AbsIn[]{{ {} }},", emit_inputs(&f.inputs));
    let _ = writeln!(s, "            new AbsOpt[]{{ {} }},", emit_opts(&f.opt_inputs));
    let _ = writeln!(s, "            new AbsOut[]{{ {} }}));", emit_outputs(&f.outputs));
}

fn emit_inputs(inputs: &[InRow]) -> String {
    inputs
        .iter()
        .map(|i| format!("new AbsIn({},{},{})", i.ty, js(&i.param_name), i.flags))
        .collect::<Vec<_>>()
        .join(", ")
}

fn emit_outputs(outputs: &[OutRow]) -> String {
    outputs
        .iter()
        .map(|o| format!("new AbsOut({},{},{})", o.ty, js(&o.param_name), o.flags))
        .collect::<Vec<_>>()
        .join(", ")
}

/// `AbsOpt` carries every domain's fields; the handler serializes the ones `type`
/// selects, so the slots the domain does not use stay literal `0` — matching what
/// this table emitted before the rows refactor, byte for byte, which is the proof
/// that routing through [`rows`] changed nothing the parity gate compares.
///
/// Deliberately no `hint`: the server table is diffed against C by
/// `test_abstract.c`, which carries no param-level hints, so emitting one here
/// would change the server bytes for nothing. The shipped registry does emit it.
fn emit_opts(opts: &[OptRow]) -> String {
    opts.iter()
        .map(|o| match o.ty {
            0 => format!(
                "new AbsOpt(0,{},{},{},{}, {},{},{},{},{},{}, 0,0,0,0,0, null)",
                js(&o.param_name),
                o.flags,
                js(&o.display_name),
                jd(o.default_value),
                jd(o.rmin),
                jd(o.rmax),
                o.precision,
                jd(o.rsug.0),
                jd(o.rsug.1),
                jd(o.rsug.2)
            ),
            3 => format!(
                "new AbsOpt(3,{},{},{},{}, 0,0,0,0,0,0, 0,0,0,0,0, {})",
                js(&o.param_name),
                o.flags,
                js(&o.display_name),
                jd(o.default_value),
                o.value_list.as_ref().map_or_else(|| "null".to_string(), |v| js(v))
            ),
            _ => format!(
                "new AbsOpt(2,{},{},{},{}, 0,0,0,0,0,0, {},{},{},{},{}, null)",
                js(&o.param_name),
                o.flags,
                js(&o.display_name),
                jd(o.default_value),
                o.imin,
                o.imax,
                o.isug.0,
                o.isug.1,
                o.isug.2
            ),
        })
        .collect::<Vec<_>>()
        .join(", ")
}

/// The nested data classes for the metadata table (emitted once).
const META_CLASSES: &str = r"    // ---- ta_abstract metadata (issue #114) ----
    // Type codes match the C TA_*ParameterType enums so test_abstract.c compares equal:
    //   input:  Price=0 Real=1 Integer=2      output: Real=0 Integer=1
    //   opt domain: RealRange=0 RealList=1 IntegerRange=2 IntegerList=3
    static final class AbsIn {
        final int type; final String paramName; final int flags;
        AbsIn(int t, String p, int f) { type = t; paramName = p; flags = f; }
    }
    static final class AbsOut {
        final int type; final String paramName; final int flags;
        AbsOut(int t, String p, int f) { type = t; paramName = p; flags = f; }
    }
    static final class AbsOpt {
        final int type; final String paramName; final int flags; final String displayName; final double defaultValue;
        final double rmin, rmax; final int precision; final double rsugS, rsugE, rsugI; // RealRange
        final int imin, imax, isugS, isugE, isugI;                                      // IntegerRange
        final String valueList;                                                          // IntegerList
        AbsOpt(int type, String paramName, int flags, String displayName, double defaultValue,
               double rmin, double rmax, int precision, double rsugS, double rsugE, double rsugI,
               int imin, int imax, int isugS, int isugE, int isugI, String valueList) {
            this.type = type; this.paramName = paramName; this.flags = flags;
            this.displayName = displayName; this.defaultValue = defaultValue;
            this.rmin = rmin; this.rmax = rmax; this.precision = precision;
            this.rsugS = rsugS; this.rsugE = rsugE; this.rsugI = rsugI;
            this.imin = imin; this.imax = imax; this.isugS = isugS; this.isugE = isugE; this.isugI = isugI;
            this.valueList = valueList;
        }
    }
    static final class AbsFunc {
        final String name, group, hint, camelCaseName; final int flags;
        final AbsIn[] inputs; final AbsOpt[] optInputs; final AbsOut[] outputs;
        AbsFunc(String name, String group, String hint, String camelCaseName, int flags,
                AbsIn[] inputs, AbsOpt[] optInputs, AbsOut[] outputs) {
            this.name = name; this.group = group; this.hint = hint; this.camelCaseName = camelCaseName;
            this.flags = flags; this.inputs = inputs; this.optInputs = optInputs; this.outputs = outputs;
        }
    }
";

/// The metadata RPC handler methods + a JSON string-escaper. `xml_len`/`xml_checksum`
/// are baked in as constants (the XML content itself is not shipped in the server).
fn emit_handlers(xml_len: usize, xml_checksum: u64) -> String {
    let mut s = String::new();
    s.push_str(
        r#"    // JSON string-escaper for metadata values (paramName/hint/... may contain quotes).
    static String absStr(String v) {
        if (v == null) return "\"\"";
        StringBuilder b = new StringBuilder("\"");
        for (int i = 0; i < v.length(); i++) {
            char c = v.charAt(i);
            if (c == '"' || c == '\\') b.append('\\');
            b.append(c);
        }
        b.append('"');
        return b.toString();
    }

    static String handleGetFuncInfo(String json) {
        AbsFunc f = ABSTRACT.get(jsonString(json, "funcName"));
        if (f == null) return "{\"retCode\":2}";
        return "{\"name\":" + absStr(f.name) + ",\"group\":" + absStr(f.group)
             + ",\"hint\":" + absStr(f.hint) + ",\"camelCaseName\":" + absStr(f.camelCaseName)
             + ",\"flags\":" + f.flags + ",\"nbInput\":" + f.inputs.length
             + ",\"nbOptInput\":" + f.optInputs.length + ",\"nbOutput\":" + f.outputs.length + "}";
    }

    static String handleGetInputParameterInfo(String json) {
        AbsFunc f = ABSTRACT.get(jsonString(json, "funcName"));
        int idx = jsonInt(json, "paramIndex");
        if (f == null || idx < 0 || idx >= f.inputs.length) return "{\"retCode\":2}";
        AbsIn ii = f.inputs[idx];
        return "{\"type\":" + ii.type + ",\"paramName\":" + absStr(ii.paramName) + ",\"flags\":" + ii.flags + "}";
    }

    static String handleGetOptInputParameterInfo(String json) {
        AbsFunc f = ABSTRACT.get(jsonString(json, "funcName"));
        int idx = jsonInt(json, "paramIndex");
        if (f == null || idx < 0 || idx >= f.optInputs.length) return "{\"retCode\":2}";
        AbsOpt o = f.optInputs[idx];
        StringBuilder b = new StringBuilder("{\"type\":").append(o.type)
            .append(",\"paramName\":").append(absStr(o.paramName))
            .append(",\"flags\":").append(o.flags)
            .append(",\"displayName\":").append(absStr(o.displayName))
            .append(",\"defaultValue\":").append(o.defaultValue);
        if (o.type == 0) { // RealRange
            b.append(",\"min\":").append(o.rmin).append(",\"max\":").append(o.rmax)
             .append(",\"precision\":").append(o.precision)
             .append(",\"suggestedStart\":").append(o.rsugS).append(",\"suggestedEnd\":").append(o.rsugE)
             .append(",\"suggestedIncrement\":").append(o.rsugI);
        } else if (o.type == 2) { // IntegerRange
            b.append(",\"min\":").append(o.imin).append(",\"max\":").append(o.imax)
             .append(",\"suggestedStart\":").append(o.isugS).append(",\"suggestedEnd\":").append(o.isugE)
             .append(",\"suggestedIncrement\":").append(o.isugI);
        } else if (o.type == 3) { // IntegerList
            b.append(",\"valueList\":").append(absStr(o.valueList));
        }
        b.append("}");
        return b.toString();
    }

    static String handleGetOutputParameterInfo(String json) {
        AbsFunc f = ABSTRACT.get(jsonString(json, "funcName"));
        int idx = jsonInt(json, "paramIndex");
        if (f == null || idx < 0 || idx >= f.outputs.length) return "{\"retCode\":2}";
        AbsOut oo = f.outputs[idx];
        return "{\"type\":" + oo.type + ",\"paramName\":" + absStr(oo.paramName) + ",\"flags\":" + oo.flags + "}";
    }

    static String handleForEachFunc() {
        StringBuilder b = new StringBuilder("{\"functions\":[");
        boolean first = true;
        for (AbsFunc f : ABSTRACT.values()) {
            if (!first) b.append(',');
            first = false;
            b.append("{\"name\":").append(absStr(f.name)).append(",\"group\":").append(absStr(f.group))
             .append(",\"nbInput\":").append(f.inputs.length).append(",\"nbOptInput\":").append(f.optInputs.length)
             .append(",\"nbOutput\":").append(f.outputs.length).append("}");
        }
        b.append("]}");
        return b.toString();
    }

"#,
    );
    // TA_FunctionDescriptionXML: length + unsigned-byte-sum checksum (order-independent
    // content check vs the C reference), baked at generation time.
    let _ = writeln!(s, "    static final int ABSTRACT_XML_LENGTH = {xml_len};");
    let _ = writeln!(s, "    static final long ABSTRACT_XML_CHECKSUM = {xml_checksum}L;");
    s.push_str(
        "    static String handleFunctionDescriptionXML() {\n\
        \x20       return \"{\\\"length\\\":\" + ABSTRACT_XML_LENGTH + \",\\\"checksum\\\":\" + ABSTRACT_XML_CHECKSUM + \"}\";\n\
        \x20   }\n\n",
    );
    s
}
