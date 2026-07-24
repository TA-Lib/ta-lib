//! Assembles the shipped `java/src/com/tictactec/ta/lib/Core.java` GENCODE section
//! from the per-indicator Java methods — ta_codegen's takeover of gen_code's role
//! for the shipped Java library.
//!
//! The per-indicator method text is the SAME output the JSON-RPC Java server
//! inlines (so numerical correctness is already proven by `ta_regtest --codegen`);
//! here it is spliced between the `GENCODE SECTION 1` markers, preserving the
//! hand-written scaffolding (license, constructor, candle/unstable-period
//! accessors) outside them. Each indicator contributes a lookback, the guarded
//! `xxx`, the public unguarded `xxxUnguarded`, and (where applicable) the
//! `xxxPrivate` variant — guarded + unguarded both public, matching C's API.

use std::collections::HashMap;
use std::fmt::Write as _;
use std::path::Path;

use super::java::to_java_method_name;
use super::price_bundle;
use crate::helper_registry::HelperRegistry;
use crate::ir::{EnumDef, FuncDef, Input, OptInput, Output, ParamType, PriceComponent};
use crate::registry::Registry;

const CORE_START: &str = "/**** START GENCODE SECTION 1 - DO NOT DELETE THIS LINE ****/";
const CORE_END: &str = "/**** END GENCODE SECTION 1 - DO NOT DELETE THIS LINE ****/";

/// Splice the generated indicator methods into `Core.java`'s GENCODE section.
#[allow(clippy::implicit_hasher)]
pub fn generate_core(
    funcs: &[FuncDef],
    enums: &HashMap<String, EnumDef>,
    registry: &Registry,
    helpers: &HelperRegistry,
    core_path: &Path,
) {
    let mut section = String::new();
    for func in funcs {
        section.push_str(&super::java::generate(func, enums, registry, helpers));
    }

    let existing = std::fs::read_to_string(core_path)
        .unwrap_or_else(|e| panic!("reading {}: {e}", core_path.display()));
    let start = existing
        .find(CORE_START)
        .unwrap_or_else(|| panic!("START GENCODE marker missing in {}", core_path.display()));
    // Keep everything up to and including the START marker line (the marker text
    // is preceded by its hand-written 3-space indent, which stays untouched).
    let after_start = existing[start..]
        .find('\n')
        .map_or(existing.len(), |i| start + i + 1);
    let end = existing
        .find(CORE_END)
        .unwrap_or_else(|| panic!("END GENCODE marker missing in {}", core_path.display()));
    // Resume at the start of the END marker line, preserving its indent + the rest.
    let end_line = existing[..end].rfind('\n').map_or(0, |i| i + 1);

    let new = format!("{}{}{}", &existing[..after_start], section, &existing[end_line..]);
    super::write_if_changed(core_path, &new, "Core.java (GENCODE)", funcs.len());
}

// ---------------------------------------------------------------------------
// CoreAnnotated.java — the @FuncInfo/@*ParameterInfo reflection wrappers.
// ---------------------------------------------------------------------------
//
// This file is 100% generated (gen_code's `printJavaFunctionAnnotation`). The
// hand-written license header + the class-closing footer are preserved verbatim;
// everything between `class CoreAnnotated extends Core {` and the final `}` is
// regenerated. The format is a faithful port of `gen_code.c`'s annotation
// emitter so the output stays a drop-in replacement for the shipped file.

/// Marker after which the generated annotation methods begin.
const ANNOTATED_CLASS_OPEN: &str = "public class CoreAnnotated extends Core {\n";

/// Splice the generated annotation wrappers into `CoreAnnotated.java`, preserving
/// the hand-written license header and the class-closing footer.
#[allow(clippy::implicit_hasher)]
pub fn generate_annotated(funcs: &[FuncDef], enums: &HashMap<String, EnumDef>, path: &Path) {
    let existing = std::fs::read_to_string(path)
        .unwrap_or_else(|e| panic!("reading {}: {e}", path.display()));
    let header_end = existing.find(ANNOTATED_CLASS_OPEN).map_or_else(
        || panic!("CoreAnnotated class-open missing in {}", path.display()),
        |i| i + ANNOTATED_CLASS_OPEN.len(),
    );
    // The class-closing brace is the only line that is exactly `}` (function
    // bodies close with the indented `); }`). Its leading newline is the final
    // blank line that follows the last method.
    let footer_pos = existing
        .rfind("\n}\n")
        .unwrap_or_else(|| panic!("CoreAnnotated class-close missing in {}", path.display()));

    let mut out = String::from(&existing[..header_end]);
    out.push('\n');
    for func in funcs {
        out.push_str(&gen_annotation(func, enums));
    }
    out.push_str(&existing[footer_pos..]);

    super::write_if_changed(path, &out, "CoreAnnotated.java", funcs.len());
}

/// Render one function's lookback wrapper + annotated delegating method.
fn gen_annotation(func: &FuncDef, enums: &HashMap<String, EnumDef>) -> String {
    let name = to_java_method_name(&func.name, func.camel_case.as_deref());
    let descs = input_descriptors(&func.inputs);
    let mut o = String::new();

    // --- Lookback wrapper ---------------------------------------------------
    let _ = writeln!(o, "public int {name}Lookback(");
    for (i, opt) in func.optional_inputs.iter().enumerate() {
        let ty = opt_java_type(opt);
        let _ = write!(o, "        {ty} {}", opt.name);
        if i < func.optional_inputs.len() - 1 {
            o.push_str(",\n");
        }
    }
    let _ = writeln!(o, ") {{\n    return super.{name}Lookback(");
    for (i, opt) in func.optional_inputs.iter().enumerate() {
        let _ = write!(o, "        {}", opt.name);
        if i < func.optional_inputs.len() - 1 {
            o.push_str(",\n");
        }
    }
    o.push_str("); }\n\n");

    // --- @FuncInfo + method signature --------------------------------------
    o.push_str("@FuncInfo(\n");
    let _ = writeln!(o, "        name  = \"{}\",", func.name);
    let _ = writeln!(o, "        group = \"{}\",", func.group);
    let _ = writeln!(o, "        flags = {},", func_flags_value(&func.flags));
    let _ = writeln!(o, "        nbInput    = {},", descs.len());
    let _ = writeln!(o, "        nbOptInput = {},", func.optional_inputs.len());
    let _ = writeln!(o, "        nbOutput   = {}", func.outputs.len());
    o.push_str(")\n");
    let _ = writeln!(o, "public RetCode {name}(");
    o.push_str("            int startIdx,\n");
    o.push_str("            int endIdx,\n");

    for desc in &descs {
        emit_input_annotation(&mut o, desc);
    }
    for opt in &func.optional_inputs {
        emit_opt_input_annotation(&mut o, opt, enums);
    }
    o.push_str("            MInteger     outBegIdx,\n");
    o.push_str("            MInteger     outNBElement,\n");
    for (i, output) in func.outputs.iter().enumerate() {
        emit_output_annotation(&mut o, output, i + 1 == func.outputs.len());
    }

    // --- super delegation ---------------------------------------------------
    let _ = writeln!(o, ") {{\n    return super.{name} (");
    o.push_str("        startIdx,\n");
    o.push_str("        endIdx,\n");
    for desc in &descs {
        match desc {
            InputDesc::Price(components) => {
                for c in components {
                    let _ = writeln!(o, "        {} ,", c.input_name());
                }
            }
            InputDesc::Real(n) | InputDesc::Integer(n) => {
                let _ = writeln!(o, "        {n},");
            }
        }
    }
    for opt in &func.optional_inputs {
        let _ = writeln!(o, "        {},", opt.name);
    }
    o.push_str("        outBegIdx,\n");
    o.push_str("        outNBElement,\n");
    for (i, output) in func.outputs.iter().enumerate() {
        if i + 1 == func.outputs.len() {
            let _ = writeln!(o, "        {}", output.name);
        } else {
            let _ = writeln!(o, "        {},", output.name);
        }
    }
    o.push_str("); }\n\n\n");
    o
}

/// A reconstructed abstract input descriptor (price components regrouped).
enum InputDesc {
    Real(String),
    Integer(String),
    /// One price bundle's components in **signature order**.
    ///
    /// Signature order, not canonical `OHLCV` order, because this vec drives two
    /// positional surfaces — the Java parameter declarations and the `super.<name>(...)`
    /// argument list — and `Core.java` (emitted by [`super::java`] straight from
    /// `func.inputs`) is in signature order. Canonicalising here would bind the arrays to
    /// the wrong parameters for any function whose YAML lists its components out of
    /// `OHLCV` order: it would still compile, and silently compute the wrong numbers.
    /// The descriptor's `paramName`/`flags` are canonicalised at their point of use.
    Price(Vec<PriceComponent>),
}

/// Fold the expanded price components (`inHigh`/`inLow`/`inClose`) into a single abstract
/// `Price` descriptor, via the shared [`price_bundle::group`] the other abstract backends
/// use — the grouping comes from the YAML declaration, not from the argument names.
fn input_descriptors(inputs: &[Input]) -> Vec<InputDesc> {
    price_bundle::group(inputs)
        .into_iter()
        .map(|g| match g {
            price_bundle::Grouped::Price(bundle) => InputDesc::Price(
                bundle
                    .iter()
                    .map(|i| i.price.expect("bundle member carries a PriceRef").component)
                    .collect(),
            ),
            price_bundle::Grouped::Single(inp) => match &inp.param_type {
                ParamType::Integer => InputDesc::Integer(inp.name.clone()),
                _ => InputDesc::Real(inp.name.clone()),
            },
        })
        .collect()
}

fn emit_input_annotation(o: &mut String, desc: &InputDesc) {
    match desc {
        InputDesc::Price(components) => {
            // The descriptor is canonical (public ABI); the parameter list below stays in
            // signature order so it lines up with `Core.java`.
            let canonical = price_bundle::canonical(components);
            let name = price_bundle::canonical_name(&canonical);
            let flags = price_bundle::flags(&canonical);
            o.push_str("            @InputParameterInfo(\n");
            let _ = writeln!(o, "                paramName = \"{name}\",");
            let _ = writeln!(o, "                flags     = {flags},");
            o.push_str("                type = InputParameterType.TA_Input_Price\n");
            o.push_str("            )\n");
            for c in components {
                // gen_code emits a trailing space before `[]` for price components.
                let _ = writeln!(o, "            double {} [],", c.input_name());
            }
        }
        InputDesc::Real(n) => {
            o.push_str("            @InputParameterInfo(\n");
            let _ = writeln!(o, "                paramName = \"{n}\",");
            o.push_str("                flags     = 0,\n");
            o.push_str("                type = InputParameterType.TA_Input_Real\n");
            o.push_str("            )\n");
            let _ = writeln!(o, "            double {n}[],");
        }
        InputDesc::Integer(n) => {
            o.push_str("            @InputParameterInfo(\n");
            let _ = writeln!(o, "                paramName = \"{n}\",");
            o.push_str("                flags     = 0,\n");
            o.push_str("                type = InputParameterType.TA_Input_Integer\n");
            o.push_str("            )\n");
            let _ = writeln!(o, "            int {n}[],");
        }
    }
}

/// Java parameter type for an optional input (lookback wrapper + signature).
fn opt_java_type(opt: &OptInput) -> &'static str {
    match &opt.param_type {
        ParamType::Real => "double",
        ParamType::Enum(_) => "MAType",
        _ => "int",
    }
}

/// Map the `f64::MIN`/`f64::MAX` range sentinels (parser's `TA_REAL_MIN`/`MAX`)
/// back to TA-Lib's `±3e37` for `%.5f` display. Bit comparison: these are exact
/// sentinel values, not approximate measurements.
fn real_display(v: f64) -> f64 {
    if v.to_bits() == f64::MIN.to_bits() {
        -3.0e37
    } else if v.to_bits() == f64::MAX.to_bits() {
        3.0e37
    } else {
        v
    }
}

/// Map the integer-range sentinels (`TA_INTEGER_MIN`/`MAX` = `±2e9`); other
/// values are small integers stored as `f64`, so the cast never truncates.
#[allow(clippy::cast_possible_truncation)]
fn int_display(v: f64) -> i64 {
    if v.to_bits() == f64::MIN.to_bits() {
        -2_000_000_000
    } else if v.to_bits() == f64::MAX.to_bits() {
        2_000_000_000
    } else {
        v as i64
    }
}

fn emit_opt_input_annotation(o: &mut String, opt: &OptInput, enums: &HashMap<String, EnumDef>) {
    let display = opt.display_name.as_deref().unwrap_or("");
    o.push_str("            @OptInputParameterInfo(\n");
    let _ = writeln!(o, "                paramName    = \"{}\",", opt.name);
    let _ = writeln!(o, "                displayName  = \"{display}\",");
    let _ = writeln!(o, "                flags        = {},", opt_input_flags_value(&opt.flags));
    match &opt.param_type {
        ParamType::Real => {
            o.push_str("                type    = OptInputParameterType.TA_OptInput_RealRange,\n");
            o.push_str("                dataSet = com.tictactec.ta.lib.meta.annotation.RealRange.class\n");
            o.push_str("            )\n");
            let (min, max) = opt.range.unwrap_or((f64::MIN, f64::MAX));
            let (ss, se, si) = opt.suggested.unwrap_or((0.0, 0.0, 0.0));
            o.push_str("            @RealRange(\n");
            let _ = writeln!(o, "                    paramName    = \"{}\",", opt.name);
            let _ = writeln!(o, "                    defaultValue = {:.5},", opt.default.unwrap_or(0.0));
            let _ = writeln!(o, "                    min          = {:.5},", real_display(min));
            let _ = writeln!(o, "                    max          = {:.5},", real_display(max));
            let _ = writeln!(o, "                    precision    = {},", opt.precision.unwrap_or(0));
            let _ = writeln!(o, "                    suggested_start     = {ss:.5},");
            let _ = writeln!(o, "                    suggested_end       = {se:.5},");
            let _ = writeln!(o, "                    suggested_increment = {si:.5}");
            o.push_str("            )\n");
            let _ = writeln!(o, "            double {},", opt.name);
        }
        ParamType::Enum(_) => {
            o.push_str("                type    = OptInputParameterType.TA_OptInput_IntegerList,\n");
            o.push_str("                dataSet = com.tictactec.ta.lib.meta.annotation.IntegerList.class\n");
            o.push_str("            )\n");
            o.push_str("            @IntegerList(\n");
            let _ = writeln!(o, "                    paramName    = \"{}\",", opt.name);
            let _ = writeln!(o, "                    defaultValue = {},", int_display(opt.default.unwrap_or(0.0)));
            emit_matype_list(o, enums);
            o.push_str("            )\n");
            let _ = writeln!(o, "            MAType {},", opt.name);
        }
        _ => {
            o.push_str("                type    = OptInputParameterType.TA_OptInput_IntegerRange,\n");
            o.push_str("                dataSet = com.tictactec.ta.lib.meta.annotation.IntegerRange.class\n");
            o.push_str("            )\n");
            let (min, max) = opt.range.unwrap_or((f64::MIN, f64::MAX));
            let (ss, se, si) = opt.suggested.unwrap_or((0.0, 0.0, 0.0));
            o.push_str("            @IntegerRange(\n");
            let _ = writeln!(o, "                    paramName    = \"{}\",", opt.name);
            let _ = writeln!(o, "                    defaultValue = {},", int_display(opt.default.unwrap_or(0.0)));
            let _ = writeln!(o, "                    min          = {},", int_display(min));
            let _ = writeln!(o, "                    max          = {},", int_display(max));
            let _ = writeln!(o, "                    suggested_start     = {},", int_display(ss));
            let _ = writeln!(o, "                    suggested_end       = {},", int_display(se));
            let _ = writeln!(o, "                    suggested_increment = {}", int_display(si));
            o.push_str("            )\n");
            let _ = writeln!(o, "            int {},", opt.name);
        }
    }
}

/// Emit the `value`/`string` rows of the MA-method `@IntegerList` from the
/// `MAType` enum (value = ordinal, string = the `TA_MAType_<X>` suffix).
fn emit_matype_list(o: &mut String, enums: &HashMap<String, EnumDef>) {
    let Some(ma) = enums.get("MAType") else { return };
    if ma.variants.is_empty() {
        return;
    }
    let strings: Vec<String> = ma
        .variants
        .iter()
        .map(|v| v.c_name.trim_start_matches("TA_MAType_").to_string())
        .collect();
    let values: Vec<String> = (0..ma.variants.len()).map(|i| i.to_string()).collect();
    let _ = writeln!(o, "                    value  = {{ {} }},", values.join(", "));
    let quoted: Vec<String> = strings.iter().map(|s| format!("\"{s}\"")).collect();
    let _ = writeln!(o, "                    string = {{ {} }}", quoted.join(", "));
}

fn emit_output_annotation(o: &mut String, output: &Output, last: bool) {
    let flags = output_flags_value(&output.flags);
    let is_int = matches!(output.param_type, ParamType::Integer);
    let (jtype, atype) = if is_int { ("int", "Integer") } else { ("double", "Real") };
    o.push_str("            @OutputParameterInfo(\n");
    let _ = writeln!(o, "                paramName = \"{}\",", output.name);
    let _ = writeln!(o, "                flags     = {flags},");
    let _ = writeln!(o, "                type = OutputParameterType.TA_Output_{atype}");
    o.push_str("            )\n");
    if is_int {
        // gen_code's integer-output format string carries a trailing newline, so a
        // non-last integer output puts its `,` on the next line.
        let _ = writeln!(o, "            {jtype} {}[]", output.name);
    } else {
        let _ = write!(o, "            {jtype} {}[]", output.name);
    }
    if !last {
        o.push(',');
    }
    o.push('\n');
}

/// `TA_FuncFlags` numeric value (OR of matched bits; `stream` =
/// TA_FUNC_FLG_STREAM).
pub(crate) fn func_flags_value(flags: &[String]) -> u32 {
    let mut v = 0;
    for f in flags {
        v |= match f.as_str() {
            "overlap" => 0x0100_0000,
            "stream" => 0x0200_0000,
            "volume" => 0x0400_0000,
            "unstable_period" => 0x0800_0000,
            "candlestick" => 0x1000_0000,
            "path_dependent" => 0x2000_0000, // TA_FUNC_FLG_PATH_DEP
            _ => 0,
        };
    }
    v
}

/// `TA_OptInputFlags` numeric value.
pub(crate) fn opt_input_flags_value(flags: &[String]) -> u32 {
    let mut v = 0;
    for f in flags {
        v |= match f.as_str() {
            "percent" => 0x0010_0000,
            "degree" => 0x0020_0000,
            "currency" => 0x0040_0000,
            "advanced" => 0x0100_0000,
            _ => 0,
        };
    }
    v
}

/// `TA_OutputFlags` numeric value.
pub(crate) fn output_flags_value(flags: &[String]) -> u32 {
    let mut v = 0;
    for f in flags {
        v |= match f.as_str() {
            "line" => 0x0000_0001,
            "dot_line" => 0x0000_0002,
            "dash_line" => 0x0000_0004,
            "dot" => 0x0000_0008,
            "histogram" => 0x0000_0010,
            "pattern_bool" => 0x0000_0020,
            "pattern_bull_bear" => 0x0000_0040,
            "pattern_strength" => 0x0000_0080,
            "positive" => 0x0000_0100,
            "negative" => 0x0000_0200,
            "zero" => 0x0000_0400,
            "upper_limit" => 0x0000_0800,
            "lower_limit" => 0x0000_1000,
            "nullable" => 0x0000_2000,
            _ => 0,
        };
    }
    v
}
