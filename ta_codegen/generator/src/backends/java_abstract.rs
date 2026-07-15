//! Java analog of [`rust_abstract`](super::rust_abstract) / C's `ta_abstract_serve.c`:
//! emits the `ta_abstract` introspection metadata table plus the JSON-RPC handlers
//! (`TA_GetFuncInfo`, `TA_Get{Input,OptInput,Output}ParameterInfo`,
//! `abstract_for_each_func`, `TA_FunctionDescriptionXML`) into the self-contained
//! Java server (`TaCodegenServe`), so `test_abstract.c` drives Java-vs-C metadata
//! parity (issue #114).
//!
//! The metadata VALUES — flag bit-masks, price-input collapsing, real sentinels —
//! are computed by the SAME helpers that build the C and Rust tables
//! ([`func_flag_bits`], [`canonical_price`], `ta_real_sentinel`, ...), so the three
//! backends agree by construction rather than by three hand-maintained copies.

#![allow(
    clippy::cast_possible_truncation,
    clippy::cast_sign_loss,
    clippy::cast_precision_loss
)]

use std::collections::HashMap;
use std::fmt::Write;

use super::common::ta_real_sentinel;
use super::rust_abstract::{
    canonical_price, func_flag_bits, opt_flag_bits, output_flag_bits, price_component_of,
};
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

/// Generate the complete `ta_abstract` metadata block (nested data classes, the
/// populated `ABSTRACT` table, and the metadata RPC handler methods) as Java
/// source to be spliced into the `TaCodegenServe` class body.
///
/// `funcs` are enumerated in name-sorted order (matching `rust_abstract`'s `FUNCS`).
#[allow(clippy::implicit_hasher)]
pub fn generate(funcs: &[FuncDef], enums: &HashMap<String, EnumDef>) -> String {
    let mut sorted: Vec<&FuncDef> = funcs.iter().collect();
    sorted.sort_by(|a, b| a.name.cmp(&b.name));

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
    for f in &sorted {
        emit_func_registration(&mut s, f, enums);
    }
    s.push_str("    }\n\n");

    s.push_str(&emit_handlers(xml_len, xml_checksum));
    s
}

/// One `ABSTRACT.put("NAME", new AbsFunc(...))` registration.
fn emit_func_registration(s: &mut String, f: &FuncDef, enums: &HashMap<String, EnumDef>) {
    let camel = f.camel_case.as_deref().unwrap_or(&f.name);
    let hint = f.hint.as_deref().unwrap_or("");
    let _ = writeln!(
        s,
        "        ABSTRACT.put({}, new AbsFunc({}, {}, {}, {}, {},",
        js(&f.name),
        js(&f.name),
        js(&f.group),
        js(hint),
        js(camel),
        func_flag_bits(&f.flags)
    );
    let _ = writeln!(s, "            new AbsIn[]{{ {} }},", emit_inputs(&f.inputs));
    let _ = writeln!(s, "            new AbsOpt[]{{ {} }},", emit_opts(&f.optional_inputs, enums));
    let _ = writeln!(s, "            new AbsOut[]{{ {} }}));", emit_outputs(&f.outputs));
}

/// Re-collapse the parser's expanded price components back into a single
/// `Price` input with an OHLCV flag bitmask + canonical `inPriceXXX` name
/// (identical logic to `rust_abstract::emit_inputs`). Input type codes match C:
/// Price=0, Real=1, Integer=2.
fn emit_inputs(inputs: &[Input]) -> String {
    let mut items: Vec<String> = Vec::new();
    let mut price: Vec<&str> = Vec::new();
    for inp in inputs {
        match &inp.param_type {
            ParamType::Price(components) => {
                flush_price(&mut items, &mut price);
                let comps: Vec<&str> = components.iter().map(String::as_str).collect();
                let (name, bits) = canonical_price(&comps);
                items.push(format!("new AbsIn(0,{},{})", js(&name), bits));
            }
            ParamType::Real => {
                if let Some(comp) = price_component_of(&inp.name) {
                    price.push(comp);
                } else {
                    flush_price(&mut items, &mut price);
                    items.push(format!("new AbsIn(1,{},0)", js(&inp.name)));
                }
            }
            ParamType::Integer => {
                flush_price(&mut items, &mut price);
                items.push(format!("new AbsIn(2,{},0)", js(&inp.name)));
            }
            ParamType::Enum(_) => {}
        }
    }
    flush_price(&mut items, &mut price);
    items.join(", ")
}

fn flush_price(items: &mut Vec<String>, price: &mut Vec<&str>) {
    if price.is_empty() {
        return;
    }
    let (name, bits) = canonical_price(price);
    items.push(format!("new AbsIn(0,{},{})", js(&name), bits));
    price.clear();
}

/// Output type codes match C: Real=0, Integer=1.
fn emit_outputs(outputs: &[Output]) -> String {
    outputs
        .iter()
        .map(|out| {
            let ty = i32::from(out.param_type == ParamType::Integer);
            format!("new AbsOut({},{},{})", ty, js(&out.name), output_flag_bits(&out.flags))
        })
        .collect::<Vec<_>>()
        .join(", ")
}

/// Opt-input domain type codes match C: RealRange=0, RealList=1, IntegerRange=2,
/// IntegerList=3. `AbsOpt` carries every domain's fields; the handler serializes
/// the ones the type selects.
fn emit_opts(opts: &[OptInput], enums: &HashMap<String, EnumDef>) -> String {
    opts.iter().map(|o| emit_opt(o, enums)).collect::<Vec<_>>().join(", ")
}

fn emit_opt(opt: &OptInput, enums: &HashMap<String, EnumDef>) -> String {
    let display = opt.display_name.as_deref().unwrap_or(&opt.name);
    let flags = opt_flag_bits(&opt.flags);
    // AbsOpt(type, paramName, flags, displayName, defaultValue,
    //        rmin, rmax, precision, rsugS, rsugE, rsugI,      // RealRange
    //        imin, imax, isugS, isugE, isugI, valueList)      // IntegerRange / IntegerList
    match &opt.param_type {
        ParamType::Real => {
            let (min, max) = opt.range.unwrap_or((0.0, 0.0));
            let prec = opt.precision.unwrap_or(0);
            let def = ta_real_sentinel(opt.default.unwrap_or(0.0));
            let (sg, en, ic) = opt.suggested.unwrap_or((0.0, 0.0, 0.0));
            format!(
                "new AbsOpt(0,{},{},{},{}, {},{},{},{},{},{}, 0,0,0,0,0, null)",
                js(&opt.name),
                flags,
                js(display),
                jd(def),
                jd(ta_real_sentinel(min)),
                jd(ta_real_sentinel(max)),
                prec,
                jd(ta_real_sentinel(sg)),
                jd(ta_real_sentinel(en)),
                jd(ta_real_sentinel(ic))
            )
        }
        ParamType::Integer => {
            let (min, max) = opt.range.unwrap_or((0.0, 0.0));
            let (min, max) = (min as i32, max as i32);
            let def = opt.default.unwrap_or(0.0) as i32;
            let (sg, en, ic) = match opt.suggested {
                Some((a, b, c)) => (a as i32, b as i32, c as i32),
                None => (max, max, max),
            };
            format!(
                "new AbsOpt(2,{},{},{},{}, 0,0,0,0,0,0, {},{},{},{},{}, null)",
                js(&opt.name),
                flags,
                js(display),
                jd(f64::from(def)),
                min,
                max,
                sg,
                en,
                ic
            )
        }
        ParamType::Enum(name) => {
            let def = opt.default.unwrap_or(0.0) as i64;
            let mut vl = String::new();
            if let Some(ed) = enums.get(name) {
                for (idx, v) in ed.variants.iter().enumerate() {
                    if idx > 0 {
                        vl.push(';');
                    }
                    let _ = write!(vl, "{}={}", i64::from(v.value), v.short_name);
                }
            }
            format!(
                "new AbsOpt(3,{},{},{},{}, 0,0,0,0,0,0, 0,0,0,0,0, {})",
                js(&opt.name),
                flags,
                js(display),
                jd(def as f64),
                js(&vl)
            )
        }
        ParamType::Price(_) => {
            // Not expected for optional inputs; emit a harmless empty real range.
            format!(
                "new AbsOpt(0,{},{},{},0.0, 0.0,0.0,0,0.0,0.0,0.0, 0,0,0,0,0, null)",
                js(&opt.name),
                flags,
                js(display)
            )
        }
    }
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
