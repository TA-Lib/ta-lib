use super::common::pascal_word;
use crate::helper_registry::HelperRegistry;
use crate::ir::{EnumDef, FuncDef, LookbackExpr, ParamType};
use crate::registry::Registry;
use std::collections::HashMap;

#[allow(clippy::implicit_hasher)]
pub fn generate(
    func: &FuncDef,
    _enums: &HashMap<String, EnumDef>,
    _registry: &Registry,
    _helpers: &HelperRegistry,
) -> String {
    let mut out = String::new();
    let pascal = pascal_word(&func.name);
    let extra_params: &[(String, String)] = &[];

    out.push_str(&gen_lookback(func, &pascal));
    out.push('\n');

    // Private variant declarations (double-only, with extra params)
    if func.has_explicit_private {
        out.push_str(&gen_private(func, &pascal));
        out.push('\n');
    }

    out.push_str("#if defined( _MANAGED ) && defined( USE_SUBARRAY )\n");
    out.push_str(&gen_subarray_decl(func, &pascal, false, extra_params));
    out.push('\n');
    out.push_str(&gen_subarray_decl(func, &pascal, true, extra_params));
    out.push('\n');
    out.push_str(&gen_cli_array_dispatch(func, &pascal, false, extra_params));
    out.push_str(&gen_cli_array_dispatch(func, &pascal, true, extra_params));
    out.push_str("#elif defined( _MANAGED )\n");
    out.push_str(&gen_cli_array_decl(func, &pascal, false, extra_params));
    out.push_str(&gen_cli_array_decl(func, &pascal, true, extra_params));
    out.push_str("#endif\n");
    out.push('\n');

    // Logic variant declarations (same signatures as guarded, different name)
    out.push_str("#if defined( _MANAGED ) && defined( USE_SUBARRAY )\n");
    let logic_pascal = format!("{pascal}Logic");
    out.push_str(&gen_subarray_decl(func, &logic_pascal, false, extra_params));
    out.push('\n');
    out.push_str(&gen_subarray_decl(func, &logic_pascal, true, extra_params));
    out.push('\n');
    out.push_str(&gen_cli_array_dispatch(func, &logic_pascal, false, extra_params));
    out.push_str(&gen_cli_array_dispatch(func, &logic_pascal, true, extra_params));
    out.push_str("#elif defined( _MANAGED )\n");
    out.push_str(&gen_cli_array_decl(func, &logic_pascal, false, extra_params));
    out.push_str(&gen_cli_array_decl(func, &logic_pascal, true, extra_params));
    out.push_str("#endif\n");
    out.push('\n');

    out.push_str(&gen_macros(func, &pascal));

    out
}

/// Map a `ParamType` to its double-precision C++/CLI type string:
/// `Real` becomes `double`, everything else becomes `int`. Shared by the
/// optional-input, output, and (double branch of) input type mappings.
fn real_double_else_int(pt: &ParamType) -> &'static str {
    match pt {
        ParamType::Real => "double",
        ParamType::Integer | ParamType::Enum(_) | ParamType::Price(_) => "int",
    }
}

/// Map a `ParamType` to the C++/CLI type string for optional inputs.
fn opt_input_type(pt: &ParamType) -> &'static str {
    real_double_else_int(pt)
}

/// Map a `ParamType` to a C++/CLI type string for inputs. With
/// `single_precision`, `Real` becomes `float`; otherwise `double`.
fn input_type(pt: &ParamType, single_precision: bool) -> &'static str {
    if single_precision {
        match pt {
            ParamType::Real => "float",
            ParamType::Integer | ParamType::Enum(_) | ParamType::Price(_) => "int",
        }
    } else {
        real_double_else_int(pt)
    }
}

/// Map a `ParamType` to a C++/CLI type string for outputs.
fn output_type(pt: &ParamType) -> &'static str {
    real_double_else_int(pt)
}

fn gen_lookback(func: &FuncDef, pascal: &str) -> String {
    let params = match &func.lookback {
        Some(LookbackExpr::Literal(_)) | None => "void".to_string(),
        Some(LookbackExpr::ParamMinus(param, _)) => {
            // Find the opt input to get range info
            let opt = func.optional_inputs.iter().find(|o| o.name == *param);
            if let Some(opt) = opt {
                let c_type = opt_input_type(&opt.param_type);
                if let Some((lo, hi)) = opt.range {
                    format!(
                        "{c_type}           {param} );\
                     /* From {lo} to {hi} */"
                    )
                } else {
                    format!("{c_type}           {param}")
                }
            } else {
                "void".to_string()
            }
        }
        Some(LookbackExpr::Code(_)) => {
            if func.optional_inputs.is_empty() {
                "void".to_string()
            } else {
                let param_parts: Vec<String> = func
                    .optional_inputs
                    .iter()
                    .map(|opt| {
                        let c_type = opt_input_type(&opt.param_type);
                        let range = opt
                            .range
                            .map(|r| format!("  /* From {} to {} */", r.0, r.1))
                            .unwrap_or_default();
                        format!("{}           {}{}", c_type, opt.name, range)
                    })
                    .collect();
                param_parts.join(", ")
            }
        }
    };

    format!("         static int {pascal}Lookback( {params} );\n")
}

/// Generate Private variant declarations (double-only, with extra params).
fn gen_private(func: &FuncDef, pascal: &str) -> String {
    let private_pascal = format!("{pascal}Private");
    let extra = &func.private_extra_params;
    let mut out = String::new();
    out.push_str("#if defined( _MANAGED ) && defined( USE_SUBARRAY )\n");
    out.push_str(&gen_subarray_decl(func, &private_pascal, false, extra));
    out.push('\n');
    out.push_str(&gen_cli_array_dispatch(func, &private_pascal, false, extra));
    out.push_str("#elif defined( _MANAGED )\n");
    out.push_str(&gen_cli_array_decl(func, &private_pascal, false, extra));
    out.push_str("#endif\n");
    out
}

fn gen_subarray_decl(
    func: &FuncDef,
    pascal: &str,
    single_precision: bool,
    extra_params: &[(String, String)],
) -> String {
    let mut params: Vec<String> = Vec::new();
    params.push("int    startIdx".to_string());
    params.push("int    endIdx".to_string());

    for input in &func.inputs {
        let typ = input_type(&input.param_type, single_precision);
        params.push(format!("SubArray<{}>^ {}", typ, input.name));
    }

    for opt in &func.optional_inputs {
        let typ = opt_input_type(&opt.param_type);
        params.push(format!("{} {}", typ, opt.name));
    }

    for (param_name, c_type) in extra_params {
        params.push(format!("{c_type} {param_name}"));
    }

    params.push("[Out]int%    outBegIdx".to_string());
    params.push("[Out]int%    outNBElement".to_string());

    for output in &func.outputs {
        let typ = output_type(&output.param_type);
        params.push(format!("SubArray<{}>^  {}", typ, output.name));
    }

    format_decl(pascal, &params)
}

fn gen_cli_array_dispatch(
    func: &FuncDef,
    pascal: &str,
    single_precision: bool,
    extra_params: &[(String, String)],
) -> String {
    let mut params: Vec<String> = Vec::new();
    params.push("int    startIdx".to_string());
    params.push("int    endIdx".to_string());

    for input in &func.inputs {
        let typ = input_type(&input.param_type, single_precision);
        params.push(format!("cli::array<{}>^ {}", typ, input.name));
    }

    for opt in &func.optional_inputs {
        let typ = opt_input_type(&opt.param_type);
        params.push(format!("{} {}", typ, opt.name));
    }

    for (param_name, c_type) in extra_params {
        params.push(format!("{c_type} {param_name}"));
    }

    params.push("[Out]int%    outBegIdx".to_string());
    params.push("[Out]int%    outNBElement".to_string());

    for output in &func.outputs {
        let typ = output_type(&output.param_type);
        params.push(format!("cli::array<{}>^  {}", typ, output.name));
    }

    let mut out = format_decl_inline(pascal, &params);

    // Build dispatch call body. Output-distinctness (issue #108): aliasing two
    // different output arrays has no correct result, so reject it here at the
    // managed-array boundary (each array is wrapped into a distinct SubArray
    // below, so this is the level that still sees the shared handle). Input ==
    // output aliasing stays allowed. Only multi-output functions gain the guard;
    // single-output functions keep their original one-line body.
    if func.outputs.len() >= 2 {
        out.push_str("         {\n");
        let mut pairs: Vec<String> = Vec::new();
        for i in 0..func.outputs.len() {
            for j in (i + 1)..func.outputs.len() {
                pairs.push(format!(
                    "{} == {}",
                    func.outputs[i].name, func.outputs[j].name
                ));
            }
        }
        out.push_str(&format!(
            "            if( {} ) return RetCode::BadParam;\n",
            pairs.join(" || ")
        ));
        out.push_str(&format!("            return {pascal}( startIdx, endIdx,\n"));
    } else {
        out.push_str(&format!("         {{ return {pascal}( startIdx, endIdx,\n"));
    }

    // Wrap inputs
    for input in &func.inputs {
        let typ = input_type(&input.param_type, single_precision);
        out.push_str(&format!(
            "                         gcnew SubArrayFrom1D<{}>({},0),\n",
            typ, input.name
        ));
    }

    // Pass through optional inputs
    for opt in &func.optional_inputs {
        out.push_str(&format!("                         {},\n", opt.name));
    }

    // Pass through extra params
    for (param_name, _) in extra_params {
        out.push_str(&format!("                         {param_name},\n"));
    }

    // Pass through output scalars
    out.push_str("             outBegIdx,\n");
    out.push_str("             outNBElement,\n");

    // Wrap outputs
    for (i, output) in func.outputs.iter().enumerate() {
        let typ = output_type(&output.param_type);
        if i == func.outputs.len() - 1 {
            out.push_str(&format!(
                "               gcnew SubArrayFrom1D<{}>({},0) );\n",
                typ, output.name
            ));
        } else {
            out.push_str(&format!(
                "               gcnew SubArrayFrom1D<{}>({},0),\n",
                typ, output.name
            ));
        }
    }

    out.push_str("         }\n");

    out
}

fn gen_cli_array_decl(
    func: &FuncDef,
    pascal: &str,
    single_precision: bool,
    extra_params: &[(String, String)],
) -> String {
    let mut params: Vec<String> = Vec::new();
    params.push("int    startIdx".to_string());
    params.push("int    endIdx".to_string());

    for input in &func.inputs {
        let typ = input_type(&input.param_type, single_precision);
        params.push(format!("cli::array<{}>^ {}", typ, input.name));
    }

    for opt in &func.optional_inputs {
        let typ = opt_input_type(&opt.param_type);
        params.push(format!("{} {}", typ, opt.name));
    }

    for (param_name, c_type) in extra_params {
        params.push(format!("{c_type} {param_name}"));
    }

    params.push("[Out]int%    outBegIdx".to_string());
    params.push("[Out]int%    outNBElement".to_string());

    for output in &func.outputs {
        let typ = output_type(&output.param_type);
        params.push(format!("cli::array<{}>^  {}", typ, output.name));
    }

    format_decl(pascal, &params)
}

fn gen_macros(func: &FuncDef, pascal: &str) -> String {
    let upper = pascal.to_uppercase();
    let mut out = format!(
        "         #define TA_{upper} Core::{pascal}\n\
         \x20        #define TA_{upper}_Lookback Core::{pascal}Lookback\n\
         \x20        #define TA_{upper}_Logic Core::{pascal}Logic\n"
    );
    if func.has_explicit_private {
        out.push_str(&format!(
            "         #define TA_{upper}_Private Core::{pascal}Private\n"
        ));
    }
    out
}

/// Format a declaration signature, closing with the given `terminator`
/// after the parameter list (e.g. `" );\n"` or `" )\n"`).
fn format_decl_with(pascal: &str, params: &[String], terminator: &str) -> String {
    let prefix = format!("         static enum class RetCode {pascal}( ");
    let indent = " ".repeat(prefix.len());
    let mut out = prefix;
    for (i, param) in params.iter().enumerate() {
        if i > 0 {
            out.push_str(&format!(",\n{indent}"));
        }
        out.push_str(param);
    }
    out.push_str(terminator);
    out
}

/// Format a declaration-only signature (semicolon terminated).
fn format_decl(pascal: &str, params: &[String]) -> String {
    format_decl_with(pascal, params, " );\n")
}

/// Format a declaration with inline body (no semicolon, open for body).
fn format_decl_inline(pascal: &str, params: &[String]) -> String {
    format_decl_with(pascal, params, " )\n")
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::parser;
    use crate::registry::Registry;
    use std::path::Path;

    fn make_registry() -> Registry {
        let base = Path::new(env!("CARGO_MANIFEST_DIR")).join("../../ta_codegen/input");
        Registry::from_dir(&base)
    }

    fn load_sma() -> FuncDef {
        let base = Path::new(env!("CARGO_MANIFEST_DIR"));
        let yaml_path = base.join("../../ta_codegen/input/sma/sma.yaml");
        let c_path = base.join("../../ta_codegen/input/sma/sma.c");
        let mut func_def = parser::yaml::parse_yaml(&yaml_path);
        let parsed = parser::c_source::parse_c_source(&c_path);
        func_def.body = parsed.functions[0].body.clone();
        func_def.lookback = Some(LookbackExpr::Code(parsed.lookback_body));
        func_def
    }

    #[test]
    fn test_dotnet_generates_logic_declarations() {
        let func = load_sma();
        let enums = HashMap::new();
        let registry = make_registry();
        let output = generate(&func, &enums, &registry, &HelperRegistry::empty());

        // Should contain logic variant declaration
        assert!(output.contains("SmaLogic("), "Missing SmaLogic declaration");

        // TA_INT_* macros are no longer generated
        assert!(
            !output.contains("#define TA_INT_SMA"),
            "Should NOT have #define TA_INT_SMA"
        );

        // Should contain the TA_SMA_Logic macro
        assert!(
            output.contains("#define TA_SMA_Logic Core::SmaLogic"),
            "Missing #define TA_SMA_Logic"
        );

        // SMA has no explicit private, so no Private variant or macro
        assert!(
            !output.contains("SmaPrivate("),
            "SMA should NOT have Private variant"
        );
        assert!(
            !output.contains("#define TA_SMA_Private"),
            "SMA should NOT have TA_SMA_Private macro"
        );
    }

    fn load_ema() -> FuncDef {
        let base = Path::new(env!("CARGO_MANIFEST_DIR"));
        let yaml_path = base.join("../../ta_codegen/input/ema/ema.yaml");
        let c_path = base.join("../../ta_codegen/input/ema/ema.c");
        let mut func_def = parser::yaml::parse_yaml(&yaml_path);
        let parsed = parser::c_source::parse_c_source(&c_path);

        // Mirror main.rs wire_parsed_source: find guarded (non-_private) function
        let guarded = parsed
            .functions
            .iter()
            .find(|f| !f.name.ends_with("_private"))
            .expect("must have guarded function");
        func_def.body = guarded.body.clone();
        func_def.lookback = Some(crate::ir::LookbackExpr::Code(parsed.lookback_body));

        // Detect explicit _private variant (mirrors main.rs)
        if let Some(priv_fn) =
            parsed.functions.iter().find(|f| f.name.ends_with("_private"))
        {
            func_def.private_body = priv_fn.body.clone();
            func_def.has_explicit_private = true;
            let guarded_param_names: std::collections::HashSet<_> =
                guarded.params.iter().map(|(name, _)| name.clone()).collect();
            func_def.private_extra_params = priv_fn
                .params
                .iter()
                .filter(|(name, _)| !guarded_param_names.contains(name))
                .cloned()
                .collect();
        } else {
            func_def.private_body = func_def.body.clone();
        }

        func_def
    }

    #[test]
    fn test_dotnet_generates_private_declarations() {
        let func = load_ema();
        let enums = HashMap::new();
        let registry = make_registry();
        let output = generate(&func, &enums, &registry, &HelperRegistry::empty());

        // Should contain Private variant declaration with extra params
        assert!(
            output.contains("EmaPrivate("),
            "Missing EmaPrivate declaration"
        );
        // Extra param (optInK_1) should appear in Private declaration
        assert!(
            output.contains("optInK_1"),
            "EmaPrivate should include optInK_1 extra param"
        );

        // Should contain TA_EMA_Private macro
        assert!(
            output.contains("#define TA_EMA_Private Core::EmaPrivate"),
            "Missing #define TA_EMA_Private"
        );

        // Logic and guarded variants should NOT contain extra params
        // Find the EmaLogic and Ema declarations and verify they don't have optInK_1
        // Split output at EmaPrivate to isolate the non-private sections
        let after_private = output
            .split("EmaPrivate")
            .last()
            .unwrap_or("");
        // The Ema( and EmaLogic( declarations come after EmaPrivate
        // Check that optInK_1 does NOT appear in those sections' signatures
        // (it should only appear in the #define TA_EMA_Private line)
        let lines_with_k1: Vec<&str> = after_private
            .lines()
            .filter(|l| l.contains("optInK_1") && !l.contains("#define"))
            .collect();
        assert!(
            lines_with_k1.is_empty(),
            "optInK_1 should only appear in Private declarations and macros, \
             but found in: {lines_with_k1:?}"
        );
    }
}
