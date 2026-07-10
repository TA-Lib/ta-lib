//! Generate `ta_func_api.xml` — an XML description of all TA-Lib functions.
//!
//! Produces output matching the legacy `gen_code` XML format.
//! `<Precision>` is always emitted (defaults to 0 when not specified in YAML).

use crate::ir::{FuncDef, ParamType};
use std::fmt::Write as _;
use std::path::Path;
use super::common::ta_real_sentinel;

/// Generate `ta_func_api.xml` from the given function definitions.
///
/// Sorts alphabetically by name, formats as XML matching the legacy
/// `gen_code` layout, and writes only if content has changed.
pub fn generate(funcs: &[FuncDef], out_path: &Path) {
    let out = generate_string(funcs);
    super::write_if_changed(out_path, &out, "ta_func_api.xml", funcs.len());
}

/// Build the `ta_func_api.xml` content as a string (sorted alphabetically by name).
/// Exposed so other backends can embed the identical XML (e.g. the Rust
/// `function_description_xml()` analog of C's `TA_FunctionDescriptionXML`).
pub fn generate_string(funcs: &[FuncDef]) -> String {
    let mut sorted: Vec<&FuncDef> = funcs.iter().collect();
    sorted.sort_by(|a, b| a.name.cmp(&b.name));

    let mut out = String::new();
    out.push_str("<?xml version=\"1.0\" encoding=\"utf-8\" ?>\n");
    out.push_str("<FinancialFunctions>\n");

    for func in &sorted {
        write_function(&mut out, func);
    }

    out.push_str("</FinancialFunctions>\n");
    out
}

fn write_function(out: &mut String, func: &FuncDef) {
    let _ = writeln!(out, "\t<!-- {} -->", func.name);
    out.push_str("\t<FinancialFunction>\n");

    let _ = writeln!(out, "\t\t<Abbreviation>{}</Abbreviation>", func.name);
    let camel = func.camel_case.as_deref().unwrap_or(&func.name);
    let _ = writeln!(out, "\t\t<CamelCaseName>{camel}</CamelCaseName>");
    let hint = func.hint.as_deref().unwrap_or("");
    let _ = writeln!(
        out,
        "\t\t<ShortDescription>{}</ShortDescription>",
        xml_escape(hint)
    );
    let _ = writeln!(out, "\t\t<GroupId>{}</GroupId>", func.group);

    write_func_flags(out, &func.flags, func.streaming);
    write_inputs(out, func);
    if !func.optional_inputs.is_empty() {
        write_optional_inputs(out, func);
    }
    write_outputs(out, func);

    out.push_str("\t</FinancialFunction>\n");
    out.push('\n');
    out.push('\n');
}

// --- Function flags ---

/// Canonical function flag order (matches gen_code).
const FUNC_FLAGS: &[(&str, &str)] = &[
    ("overlap", "Overlap"),
    ("volume", "Volume"),
    ("candlestick", "Candlestick"),
    ("unstable_period", "Unstable Period"),
];

fn write_func_flags(out: &mut String, flags: &[String], streaming: bool) {
    let mut matched: Vec<&str> = FUNC_FLAGS
        .iter()
        .filter(|(key, _)| flags.iter().any(|f| f == key))
        .map(|(_, xml)| *xml)
        .collect();
    if streaming {
        matched.push("Streaming");
    }

    if matched.is_empty() {
        return;
    }

    out.push_str("\t\t<Flags>\n");
    for name in &matched {
        let _ = writeln!(out, "\t\t\t<Flag>{name}</Flag>");
    }
    out.push_str("\t\t</Flags>\n");
}

// --- Required inputs ---

/// Known price component prefixes (YAML parser expands `Price` inputs
/// into individual `Real` inputs named `in{Component}`).  We detect and
/// reverse-map them to the XML price-component format.
const PRICE_COMPONENTS: &[(&str, &str)] = &[
    ("inOpen", "Open"),
    ("inHigh", "High"),
    ("inLow", "Low"),
    ("inClose", "Close"),
    ("inVolume", "Volume"),
    ("inOpenInterest", "Open Interest"),
    ("inTimestamp", "Timestamp"),
];

/// Check if an input name is an expanded price component.
fn as_price_component(name: &str) -> Option<&'static str> {
    PRICE_COMPONENTS
        .iter()
        .find(|(prefix, _)| *prefix == name)
        .map(|(_, xml_name)| *xml_name)
}

fn write_inputs(out: &mut String, func: &FuncDef) {
    out.push_str("\t\t<RequiredInputArguments>\n");
    for input in &func.inputs {
        match &input.param_type {
            // `Price` inputs are expanded into individual `Real` inputs by the YAML
            // parser before they reach here (the live reverse-mapping is in the
            // `Real` arm via `as_price_component`), and enum inputs are not emitted
            // as required input arguments — so both arms are intentionally empty.
            ParamType::Price(_) | ParamType::Enum(_) => {}
            ParamType::Real => {
                out.push_str("\t\t\t<RequiredInputArgument>\n");
                // Detect expanded price components (inHigh → High)
                if let Some(pc) = as_price_component(&input.name) {
                    let _ = writeln!(out, "\t\t\t\t<Type>{pc}</Type>");
                    let _ = writeln!(out, "\t\t\t\t<Name>{pc}</Name>");
                } else {
                    out.push_str("\t\t\t\t<Type>Double Array</Type>\n");
                    let _ = writeln!(
                        out,
                        "\t\t\t\t<Name>{}</Name>",
                        input.name
                    );
                }
                out.push_str("\t\t\t</RequiredInputArgument>\n");
            }
            ParamType::Integer => {
                out.push_str("\t\t\t<RequiredInputArgument>\n");
                out.push_str("\t\t\t\t<Type>Integer Array</Type>\n");
                let _ =
                    writeln!(out, "\t\t\t\t<Name>{}</Name>", input.name);
                out.push_str("\t\t\t</RequiredInputArgument>\n");
            }
        }
    }
    out.push_str("\t\t</RequiredInputArguments>\n");
}

// --- Optional inputs ---

/// Canonical optional-input flag order (matches gen_code).
const OPT_INPUT_FLAGS: &[(&str, &str)] = &[
    ("percent", "Percent"),
    ("degree", "Degree"),
    ("currency", "Currency"),
    ("advanced", "Advanced"),
];

fn write_optional_inputs(out: &mut String, func: &FuncDef) {
    out.push_str("\t\t<OptionalInputArguments>\n");

    for opt in &func.optional_inputs {
        out.push_str("\t\t\t<OptionalInputArgument>\n");

        // Name (prefer display_name)
        let name = opt.display_name.as_deref().unwrap_or(&opt.name);
        let _ = writeln!(
            out,
            "\t\t\t\t<Name>{}</Name>",
            xml_escape(name)
        );

        // ShortDescription (hint)
        let hint = opt.hint.as_deref().unwrap_or("");
        let _ = writeln!(
            out,
            "\t\t\t\t<ShortDescription>{}</ShortDescription>",
            xml_escape(hint)
        );

        // Flags (if any)
        write_opt_input_flags(out, &opt.flags);

        // Type + Range + DefaultValue (varies by param type)
        match &opt.param_type {
            ParamType::Real => write_real_opt(out, opt),
            ParamType::Integer => write_integer_opt(out, opt),
            ParamType::Enum(_) => write_enum_opt(out, opt),
            ParamType::Price(_) => {}
        }

        out.push_str("\t\t\t</OptionalInputArgument>\n");
    }

    out.push_str("\t\t</OptionalInputArguments>\n");
}

fn write_opt_input_flags(out: &mut String, flags: &[String]) {
    let matched: Vec<&str> = OPT_INPUT_FLAGS
        .iter()
        .filter(|(key, _)| flags.iter().any(|f| f == key))
        .map(|(_, xml)| *xml)
        .collect();

    if matched.is_empty() {
        return;
    }

    out.push_str("\t\t\t\t<Flags>\n");
    for flag in &matched {
        let _ = writeln!(out, "\t\t\t\t\t<Flag>{flag}</Flag>");
    }
    out.push_str("\t\t\t\t</Flags>\n");
}

fn write_real_opt(
    out: &mut String,
    opt: &crate::ir::OptInput,
) {
    out.push_str("\t\t\t\t<Type>Double</Type>\n");
    if let Some((min, max)) = opt.range {
        let min_v = ta_real_sentinel(min);
        let max_v = ta_real_sentinel(max);
        out.push_str("\t\t\t\t<Range>\n");
        let _ = writeln!(
            out,
            "\t\t\t\t\t<Minimum>{}</Minimum>",
            double_to_str(min_v)
        );
        let _ = writeln!(
            out,
            "\t\t\t\t\t<Maximum>{}</Maximum>",
            double_to_str(max_v)
        );
        let p = opt.precision.unwrap_or(0);
        let _ = writeln!(out, "\t\t\t\t\t<Precision>{p}</Precision>");
        let (start, end, inc) = opt.suggested.unwrap_or((0.0, 0.0, 0.0));
        let _ = writeln!(
            out,
            "\t\t\t\t\t<SuggestedStart>{}</SuggestedStart>",
            double_to_str(ta_real_sentinel(start))
        );
        let _ = writeln!(
            out,
            "\t\t\t\t\t<SuggestedEnd>{}</SuggestedEnd>",
            double_to_str(ta_real_sentinel(end))
        );
        let _ = writeln!(
            out,
            "\t\t\t\t\t<SuggestedIncrement>{}</SuggestedIncrement>",
            double_to_str(ta_real_sentinel(inc))
        );
        out.push_str("\t\t\t\t</Range>\n");
    }
    let default = opt.default.unwrap_or(0.0);
    let _ = writeln!(
        out,
        "\t\t\t\t<DefaultValue>{}</DefaultValue>",
        double_to_str(ta_real_sentinel(default))
    );
}

#[allow(clippy::cast_possible_truncation)]
fn write_integer_opt(
    out: &mut String,
    opt: &crate::ir::OptInput,
) {
    out.push_str("\t\t\t\t<Type>Integer</Type>\n");
    if let Some((min, max)) = opt.range {
        let min_i = min as i32;
        let max_i = max as i32;
        out.push_str("\t\t\t\t<Range>\n");
        let _ = writeln!(out, "\t\t\t\t\t<Minimum>{min_i}</Minimum>");
        let _ = writeln!(out, "\t\t\t\t\t<Maximum>{max_i}</Maximum>");
        // gen_code uses max for all integer suggested values.
        let _ = writeln!(
            out,
            "\t\t\t\t\t<SuggestedStart>{max_i}</SuggestedStart>"
        );
        let _ = writeln!(
            out,
            "\t\t\t\t\t<SuggestedEnd>{max_i}</SuggestedEnd>"
        );
        let _ = writeln!(
            out,
            "\t\t\t\t\t<SuggestedIncrement>{max_i}</SuggestedIncrement>"
        );
        out.push_str("\t\t\t\t</Range>\n");
    }
    let default = opt.default.unwrap_or(0.0) as i32;
    let _ = writeln!(
        out,
        "\t\t\t\t<DefaultValue>{default}</DefaultValue>"
    );
}

#[allow(clippy::cast_possible_truncation)]
fn write_enum_opt(
    out: &mut String,
    opt: &crate::ir::OptInput,
) {
    // gen_code hardcodes "MA Type" for all IntegerList params.
    out.push_str("\t\t\t\t<Type>MA Type</Type>\n");
    let default = opt.default.unwrap_or(0.0) as i32;
    let _ = writeln!(
        out,
        "\t\t\t\t<DefaultValue>{default}</DefaultValue>"
    );
}

// --- Outputs ---

/// Canonical output flag order (matches gen_code).
const OUTPUT_FLAGS: &[(&str, &str)] = &[
    ("line", "Line"),
    ("dot_line", "Dotted Line"),
    ("dash_line", "Dashed Line"),
    ("dot", "Dots"),
    ("histogram", "Histogram"),
    ("pattern_bool", "Pattern Bool"),
    ("pattern_bull_bear", "Pattern Bull Bear"),
    ("pattern_strength", "Pattern Strength"),
    ("positive", "Positive"),
    ("negative", "Negative"),
    ("zero", "Zero"),
    ("upper_limit", "Upper Limit"),
    ("lower_limit", "Lower Limit"),
];

fn write_outputs(out: &mut String, func: &FuncDef) {
    out.push_str("\t\t<OutputArguments>\n");

    for output in &func.outputs {
        out.push_str("\t\t\t<OutputArgument>\n");

        match &output.param_type {
            ParamType::Integer => {
                out.push_str("\t\t\t\t<Type>Integer Array</Type>\n");
            }
            _ => {
                out.push_str("\t\t\t\t<Type>Double Array</Type>\n");
            }
        }

        let _ = writeln!(out, "\t\t\t\t<Name>{}</Name>", output.name);

        let matched: Vec<&str> = OUTPUT_FLAGS
            .iter()
            .filter(|(key, _)| output.flags.iter().any(|f| f == key))
            .map(|(_, xml)| *xml)
            .collect();
        if !matched.is_empty() {
            out.push_str("\t\t\t\t<Flags>\n");
            for flag in &matched {
                let _ = writeln!(out, "\t\t\t\t\t<Flag>{flag}</Flag>");
            }
            out.push_str("\t\t\t\t</Flags>\n");
        }

        out.push_str("\t\t\t</OutputArgument>\n");
    }

    out.push_str("\t\t</OutputArguments>\n");
}

// --- Helpers ---

/// Format a float matching C `printf("%e")` after gen_code's `doubleToStr`
/// post-processing (strip leading exponent zeros, keep sign).
///
/// Rust's `{:.6e}` omits the `+` for positive exponents; we add it back.
fn double_to_str(value: f64) -> String {
    let s = format!("{value:.6e}");
    // Rust: "2.000000e0" or "-1.500000e-1"
    // Need: "2.000000e+0" or "-1.500000e-1"
    if let Some(pos) = s.rfind('e') {
        let after_e = &s[pos + 1..];
        if after_e.starts_with('-') {
            s
        } else {
            format!("{}e+{}", &s[..pos], after_e)
        }
    } else {
        s
    }
}

/// Escape XML reserved characters (same order as gen_code's
/// `ReplaceReservedXmlCharacters`: `&` first to avoid double-escaping).
fn xml_escape(s: &str) -> String {
    s.replace('&', "&amp;")
        .replace('<', "&lt;")
        .replace('>', "&gt;")
        .replace('\'', "&apos;")
        .replace('"', "&quot;")
}
