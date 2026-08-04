//! Java analog of [`rust_abstract`](super::rust_abstract) / C's `ta_abstract_serve.c`:
//! emits the `ta_abstract` introspection metadata table plus the JSON-RPC handlers
//! (`TA_GetFuncInfo`, `TA_Get{Input,OptInput,Output}ParameterInfo`,
//! `abstract_for_each_func`, `TA_FunctionDescriptionXML`) into the self-contained
//! Java server (`TaCodegenServe`), so `test_abstract.c` drives Java-vs-C metadata
//! parity (issue #114).
//!
//! The metadata VALUES — flag bit-masks, price-input collapsing, parameter
//! domains — are the backend-neutral [`abstract_rows`](super::abstract_rows) the
//! Rust and C# registries render too, so the backends agree by construction
//! rather than by parallel hand-maintained copies.

#![allow(
    clippy::cast_possible_truncation,
    clippy::cast_sign_loss,
    clippy::cast_precision_loss
)]

use std::collections::HashMap;
use std::fmt::Write;

use super::abstract_rows::{rows, FuncRow, InputRow, OptDomain, OptRow, OutputRow};
use crate::ir::{EnumDef, FuncDef};

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
        js(f.group.as_str()),
        js(&f.hint),
        js(f.camel_case_name()),
        f.flags
    );
    let _ = writeln!(s, "            new AbsIn[]{{ {} }},", emit_inputs(&f.inputs));
    let _ = writeln!(s, "            new AbsOpt[]{{ {} }},", emit_opts(&f.opt_inputs));
    let _ = writeln!(s, "            new AbsOut[]{{ {} }}));", emit_outputs(&f.outputs));
}

fn emit_inputs(inputs: &[InputRow]) -> String {
    inputs
        .iter()
        .map(|i| {
            format!("new AbsIn({},{},{})", i.kind.c_code(), js(&i.param_name), i.flags)
        })
        .collect::<Vec<_>>()
        .join(", ")
}

fn emit_outputs(outputs: &[OutputRow]) -> String {
    outputs
        .iter()
        .map(|o| {
            format!("new AbsOut({},{},{})", o.kind.c_code(), js(&o.param_name), o.flags)
        })
        .collect::<Vec<_>>()
        .join(", ")
}

/// `AbsOpt` carries every domain's fields; the handler serializes the ones `type`
/// selects, so the slots the domain does not use stay literal `0`.
///
/// `hint` is carried now that `test_abstract.c` compares it. It used to be
/// omitted precisely because nothing compared it — which is what made it worth
/// adding: for the ~80 opt slots whose C descriptor is a predefined
/// `TA_DEF_UI_*`, C's hint is a hand-written literal in `ta_abstract_c.rs` and
/// is not derived from the YAML, so the comparison is genuinely non-circular.
fn emit_opts(opts: &[OptRow]) -> String {
    opts.iter()
        .map(|o| {
            let name = js(&o.param_name);
            let display = js(&o.display_name);
            let hint = js(&o.hint);
            let flags = o.flags;
            let default = jd(o.domain.default_as_f64());
            match &o.domain {
                OptDomain::RealRange { min, max, precision, suggested, .. } => format!(
                    "new AbsOpt(0,{name},{flags},{display},{hint},{default}, {},{},{precision},{},{},{}, 0,0,0,0,0, null)",
                    jd(*min),
                    jd(*max),
                    jd(suggested.0),
                    jd(suggested.1),
                    jd(suggested.2)
                ),
                // No shipped function declares a real list; the arm exists so a
                // future one is serialized as its own domain rather than folded
                // into the integer-range shape by a catch-all.
                OptDomain::RealList { .. } => format!(
                    "new AbsOpt(1,{name},{flags},{display},{hint},{default}, 0,0,0,0,0,0, 0,0,0,0,0, {})",
                    js(&o.domain.value_list_string())
                ),
                OptDomain::IntegerRange { min, max, suggested, .. } => format!(
                    "new AbsOpt(2,{name},{flags},{display},{hint},{default}, 0,0,0,0,0,0, {min},{max},{},{},{}, null)",
                    suggested.0, suggested.1, suggested.2
                ),
                OptDomain::IntegerList { .. } => format!(
                    "new AbsOpt(3,{name},{flags},{display},{hint},{default}, 0,0,0,0,0,0, 0,0,0,0,0, {})",
                    js(&o.domain.value_list_string())
                ),
            }
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
        final int type; final String paramName; final int flags; final String displayName;
        final String hint; final double defaultValue;
        final double rmin, rmax; final int precision; final double rsugS, rsugE, rsugI; // RealRange
        final int imin, imax, isugS, isugE, isugI;                                      // IntegerRange
        final String valueList;                                                          // IntegerList
        AbsOpt(int type, String paramName, int flags, String displayName, String hint, double defaultValue,
               double rmin, double rmax, int precision, double rsugS, double rsugE, double rsugI,
               int imin, int imax, int isugS, int isugE, int isugI, String valueList) {
            this.type = type; this.paramName = paramName; this.flags = flags;
            this.displayName = displayName; this.hint = hint; this.defaultValue = defaultValue;
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
            .append(",\"hint\":").append(absStr(o.hint))
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
