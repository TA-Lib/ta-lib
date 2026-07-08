//! Generates JSON-RPC server source files for each target language.
//!
//! Each generated server reads JSON-RPC requests from stdin, dispatches to
//! the generated TA function implementations, and writes JSON responses to stdout.
//! All servers speak the same protocol as the existing Rust server in server.rs.

use crate::backends::java::to_java_method_name;
use crate::ir::{EnumDef, FuncDef, Input, OptInput, Output, ParamType};
use std::collections::HashMap;
use std::path::Path;

/// The comma-separated `FuncUnstId` variant names from enums.yaml (the source of
/// truth), in ordinal order. Empty if the enum is somehow missing.
fn func_unst_pascal_names(enums: &HashMap<String, EnumDef>) -> Vec<String> {
    enums
        .get("FuncUnstId")
        .map(|fu| fu.variants.iter().map(|v| v.pascal_name.clone()).collect())
        .unwrap_or_default()
}

/// Generate the JSON response key for an output at position `idx` among all outputs.
///
/// Naming convention (matches ta_regtest expectations):
/// - Real outputs: index 0 → `"outReal"`, index 1 → `"outReal1"`, index 2 → `"outReal2"`
/// - Integer outputs: index 0 → `"outInteger"`, index 1 → `"outInteger1"`
///
/// Counters are per-type: two real outputs and one integer output yield
/// `"outReal"`, `"outReal1"`, `"outInteger"` (not `"outInteger2"`).
fn output_json_key(outputs: &[Output], idx: usize) -> String {
    let out = &outputs[idx];
    // Count how many outputs of the same type appear before this one.
    let type_rank = outputs[..idx]
        .iter()
        .filter(|o| o.param_type == out.param_type)
        .count();
    match out.param_type {
        ParamType::Integer => {
            if type_rank == 0 {
                "outInteger".to_string()
            } else {
                format!("outInteger{type_rank}")
            }
        }
        _ => {
            if type_rank == 0 {
                "outReal".to_string()
            } else {
                format!("outReal{type_rank}")
            }
        }
    }
}

/// Expand a list of inputs into individual array parameter names.
///
/// Naming matches what `ta_regtest` sends in JSON-RPC requests:
/// - Single `ParamType::Real` input → `"inReal"` (original name preserved)
/// - Multiple `ParamType::Real` inputs → `"inReal0"`, `"inReal1"`, etc.
///   (e.g. MAVP has `inReal` + `inPeriods`, MULT has `inReal0` + `inReal1`;
///   both are sent as `inReal0`/`inReal1` by ta_regtest)
/// - `ParamType::Price(components)` → one name per component, capitalised:
///   `["high", "low", "close"]` → `["inHigh", "inLow", "inClose"]`
/// - All other types (Integer, Enum) are skipped.
pub(crate) fn expand_input_names(inputs: &[Input]) -> Vec<String> {
    // Count Real inputs that are NOT price-expanded (inHigh, inLow, inClose, etc.)
    // Price-expanded inputs keep their original names; only generic "inReal"/"inPeriods"
    // style inputs get renamed to inReal0/inReal1 for multi-input functions.
    let is_price_expanded = |name: &str| -> bool {
        matches!(
            name,
            "inHigh" | "inLow" | "inClose" | "inOpen" | "inVolume" | "inOpenInterest"
        )
    };
    let generic_real_count = inputs
        .iter()
        .filter(|i| i.param_type == ParamType::Real && !is_price_expanded(&i.name))
        .count();
    let mut names = Vec::new();
    let mut real_idx = 0usize;
    for inp in inputs {
        match &inp.param_type {
            ParamType::Real => {
                if is_price_expanded(&inp.name) {
                    // Price-expanded input — keep original name (matches ta_regtest)
                    names.push(inp.name.clone());
                } else if generic_real_count == 1 {
                    names.push(inp.name.clone());
                } else {
                    names.push(format!("inReal{real_idx}"));
                    real_idx += 1;
                }
            }
            ParamType::Price(components) => {
                for comp in components {
                    let name = format!(
                        "in{}{}",
                        comp[..1].to_uppercase(),
                        &comp[1..]
                    );
                    names.push(name);
                }
            }
            _ => {} // Integer / Enum inputs are not array parameters
        }
    }
    names
}

/// Map a price-component input name to its reference array name prefix.
/// Returns None for non-price input names.
/// Used by Java and .NET servers (e.g., "inHigh" -> "refHigh").
fn price_input_to_ref(name: &str) -> Option<&'static str> {
    match name {
        "inOpen" => Some("refOpen"),
        "inHigh" => Some("refHigh"),
        "inLow" => Some("refLow"),
        "inClose" => Some("refClose"),
        "inVolume" => Some("refVolume"),
        "inOpenInterest" => Some("refOI"),
        _ => None,
    }
}

/// Map a price-component input name to its RefData field name (Rust server).
/// Returns None for non-price input names.
fn price_input_to_rust_ref(name: &str) -> Option<&'static str> {
    match name {
        "inOpen" => Some("open"),
        "inHigh" => Some("high"),
        "inLow" => Some("low"),
        "inClose" => Some("close"),
        "inVolume" => Some("volume"),
        "inOpenInterest" => Some("oi"),
        _ => None,
    }
}

/// Map function name to its unstable period ID (integer value).
/// Returns None for functions without unstable period.
/// IDs must match the `TA_FuncUnstId` enum in `include/ta_defs.h`.
fn func_unst_id(name: &str) -> Option<i32> {
    match name {
        "ADX" => Some(0),        // TA_FUNC_UNST_ADX
        "ADXR" => Some(1),       // TA_FUNC_UNST_ADXR
        "ATR" => Some(2),        // TA_FUNC_UNST_ATR
        "CMO" => Some(3),        // TA_FUNC_UNST_CMO
        "DX" => Some(4),         // TA_FUNC_UNST_DX
        "EMA" => Some(5),        // TA_FUNC_UNST_EMA
        "HT_DCPERIOD" => Some(6), // TA_FUNC_UNST_HT_DCPERIOD
        "HT_DCPHASE" => Some(7), // TA_FUNC_UNST_HT_DCPHASE
        "HT_PHASOR" => Some(8),  // TA_FUNC_UNST_HT_PHASOR
        "HT_SINE" => Some(9),    // TA_FUNC_UNST_HT_SINE
        "HT_TRENDLINE" => Some(10), // TA_FUNC_UNST_HT_TRENDLINE
        "HT_TRENDMODE" => Some(11), // TA_FUNC_UNST_HT_TRENDMODE
        "KAMA" => Some(13),      // TA_FUNC_UNST_KAMA
        "MAMA" => Some(14),      // TA_FUNC_UNST_MAMA
        "MINUS_DI" => Some(16),  // TA_FUNC_UNST_MINUS_DI
        "MINUS_DM" => Some(17),  // TA_FUNC_UNST_MINUS_DM
        "NATR" => Some(18),      // TA_FUNC_UNST_NATR
        "PLUS_DI" => Some(19),   // TA_FUNC_UNST_PLUS_DI
        "PLUS_DM" => Some(20),   // TA_FUNC_UNST_PLUS_DM
        "RSI" => Some(21),       // TA_FUNC_UNST_RSI
        "STOCHRSI" => Some(22),  // TA_FUNC_UNST_STOCHRSI
        "T3" => Some(23),        // TA_FUNC_UNST_T3
        _ => None,
    }
}

/// Replace @@`CORE_XXX`@@ markers in the Java server template with actual
/// method bodies read from the generated Core_*.java files.
pub fn inline_java_core_methods(template: &str, java_dir: &Path, funcs: &[FuncDef]) -> String {
    let mut result = template.to_string();
    for func in funcs {
        let marker = format!("    // @@CORE_{}@@", func.name);
        let core_path = java_dir.join(format!("Core_{}.java", func.name));
        let replacement = if core_path.exists() {
            let content = std::fs::read_to_string(&core_path).unwrap();
            // Strip /* Generated */ prefix, remove duplicate consecutive return statements,
            // and indent properly
            let mut lines: Vec<String> = Vec::new();
            let mut prev_was_return = false;
            for line in content.lines() {
                let trimmed = line.strip_prefix("/* Generated */").unwrap_or(line);
                let is_return = trimmed.trim().starts_with("return ");
                // Skip duplicate consecutive return statements (Java treats these as errors)
                if is_return && prev_was_return {
                    continue;
                }
                prev_was_return = is_return;
                if trimmed.trim().is_empty() {
                    lines.push(String::new());
                } else {
                    lines.push(format!("    {trimmed}"));
                }
            }
            lines.join("\n")
        } else {
            format!("    // WARNING: {} not found", core_path.display())
        };
        result = result.replace(&marker, &replacement);
    }
    result
}

/// Build the C parameter string for the full indicator function (and _Unguarded variant).
///
/// Order: startIdx, endIdx, inputs, optional inputs, outBegIdx, outNBElement, outputs.
fn build_full_param_str(func: &FuncDef) -> String {
    let mut params: Vec<String> = vec!["int startIdx".to_string(), "int endIdx".to_string()];

    for input in &func.inputs {
        match &input.param_type {
            ParamType::Real => params.push(format!("const double {}[]", input.name)),
            ParamType::Price(components) => {
                for comp in components {
                    let name = format!("in{}{}", &comp[..1].to_uppercase(), &comp[1..]);
                    params.push(format!("const double {name}[]"));
                }
            }
            ParamType::Integer => params.push(format!("const int {}[]", input.name)),
            ParamType::Enum(_) => params.push(format!("int {}", input.name)),
        }
    }

    for opt in &func.optional_inputs {
        params.push(opt_input_to_c_param(opt));
    }

    params.push("int *outBegIdx".to_string());
    params.push("int *outNBElement".to_string());

    for out in &func.outputs {
        match out.param_type {
            ParamType::Integer => params.push(format!("int {}[]", out.name)),
            _ => params.push(format!("double {}[]", out.name)),
        }
    }

    params.join(", ")
}

/// Map an optional input to its C parameter declaration.
fn opt_input_to_c_param(opt: &OptInput) -> String {
    match &opt.param_type {
        ParamType::Real => format!("double {}", opt.name),
        ParamType::Enum(_) => format!("TA_MAType {}", opt.name),
        _ => format!("int {}", opt.name),
    }
}

/// Generate the `ta_func_unguarded.h` header for standalone compilation of the C server.
///
/// This file provides just the unguarded function forward declarations that the
/// generated ta_*.c files expect, without pulling in the full TA-Lib headers.
pub fn generate_c_header_stub(funcs: &[FuncDef]) -> String {
    let mut s = String::new();
    s.push_str("/* ta_func_unguarded.h — Unguarded (Logic) + private function declarations.\n");
    s.push_str(" * Auto-generated by ta_codegen.\n");
    s.push_str(" * Forward declarations are auto-generated per indicator.\n");
    s.push_str(" */\n");
    s.push_str("#ifndef TA_FUNC_UNGUARDED_H\n");
    s.push_str("#define TA_FUNC_UNGUARDED_H\n\n");

    // Include proper headers
    s.push_str("#ifndef TA_COMMON_H\n");
    s.push_str("   #include \"ta_common.h\"\n");
    s.push_str("#endif\n\n");
    s.push_str("#ifndef TA_DEFS_H\n");
    s.push_str("   #include \"ta_defs.h\"\n");
    s.push_str("#endif\n\n");

    // Only _Unguarded forward declarations. Guarded/Lookback/S_ guarded are in ta_func.h.
    s.push_str("/* Unguarded function forward declarations (same signature as guarded).\n");
    s.push_str(" *\n");
    s.push_str(" * The double-precision TA_*_Unguarded functions are exported public API\n");
    s.push_str(" * (TA_LIB_API). The single-precision TA_S_*_Unguarded declarations below\n");
    s.push_str(" * are INTERNAL: they are not exported from the shared library (plain\n");
    s.push_str(" * extern, for cross-TU calls within the library) and are not part of the\n");
    s.push_str(" * public contract. Use the exported TA_S_* guarded functions instead.\n");
    s.push_str(" */\n");
    for func in funcs {
        let upper = func.name.to_uppercase();
        let full_params = build_full_param_str(func);
        let sp_params = full_params.replace("const double", "const float");

        // The double-precision unguarded variants are exported public API:
        // their definitions carry TA_LIB_API, and MSVC requires declaration
        // and definition linkage to match (C2375 otherwise). The TA_S_
        // variants are internal (plain definitions), so plain extern here.
        s.push_str(&format!(
            "TA_LIB_API TA_RetCode TA_{upper}_Unguarded({full_params});\n"
        ));
        s.push_str(&format!(
            "extern TA_RetCode TA_S_{upper}_Unguarded({sp_params});\n"
        ));
    }
    s.push('\n');

    // Private-variant forward declarations (custom signatures, e.g. EMA's k) so
    // cross-indicator calls like MACD -> TA_EMA_Private compile in separate-
    // compilation (library) builds, where each src/ta_func/*.c includes this
    // (shipped, installed) header rather than the internal ta_func_private.h.
    s.push_str("/* Private-variant forward declarations (custom signatures) */\n");
    push_private_decls(&mut s, funcs);
    s.push('\n');

    s.push_str("/* Internal helper forward declarations */\n");
    s.push_str("extern void stddev_using_precalc_ma(const double inReal[], const double inMovAvg[], int inMovAvgBegIdx, int inMovAvgNbElement, int timePeriod, double output[]);\n");
    s.push('\n');

    s.push_str("#endif /* TA_FUNC_UNGUARDED_H */\n");
    s
}

/// Emit `extern TA_*_Private` / `TA_S_*_Private` declarations for every
/// `has_explicit_private` function (custom signatures, e.g. EMA's k factor; the
/// SP variant takes `const float` inputs with double intermediates/outputs). Used
/// by both `ta_func_private.h` and `ta_func_unguarded.h` so a declaration can
/// never lag the definition emitted by `backends::c::gen_private`.
fn push_private_decls(s: &mut String, funcs: &[FuncDef]) {
    for func in funcs {
        // Declare for every function whose .c file defines a private variant
        // (backends::c::gen_private runs for all has_explicit_private), so the
        // declaration guard can never diverge from the definition guard.
        if !func.has_explicit_private {
            continue;
        }
        let upper = func.name.to_uppercase();

        // Build the private param list: same as guarded but with extra params
        // before outputs. The double and single-precision signatures are identical
        // except the real INPUT arrays are `const double` vs `const float`
        // (intermediates/outputs stay double, matching gen_func_inner).
        let build_params = |real_input_type: &str| -> String {
            let mut params = vec!["int startIdx".to_string(), "int endIdx".to_string()];
            for input in &func.inputs {
                let c_type = match input.param_type {
                    crate::ir::ParamType::Real => real_input_type,
                    _ => "const int",
                };
                params.push(format!("{c_type} {}[]", input.name));
            }
            for opt in &func.optional_inputs {
                let c_type = match &opt.param_type {
                    crate::ir::ParamType::Real => "double".to_string(),
                    crate::ir::ParamType::Integer => "int".to_string(),
                    crate::ir::ParamType::Enum(name) => format!("TA_{name}"),
                    crate::ir::ParamType::Price(_) => unreachable!(),
                };
                params.push(format!("{c_type} {}", opt.name));
            }
            // Extra private params (e.g., optInK_1)
            for (pname, ptype) in &func.private_extra_params {
                params.push(format!("{ptype} {pname}"));
            }
            params.push("int *outBegIdx".to_string());
            params.push("int *outNBElement".to_string());
            for output in &func.outputs {
                let c_type = match output.param_type {
                    crate::ir::ParamType::Real => "double",
                    _ => "int",
                };
                params.push(format!("{c_type} {}[]", output.name));
            }
            params.join(", ")
        };

        // Same linkage rule as the unguarded declarations: the double
        // private variants are defined with TA_LIB_API, the TA_S_ ones are
        // plain — MSVC rejects mismatched declaration/definition linkage.
        s.push_str(&format!(
            "TA_LIB_API TA_RetCode TA_{upper}_Private({});\n",
            build_params("const double")
        ));
        s.push_str(&format!(
            "extern TA_RetCode TA_S_{upper}_Private({});\n",
            build_params("const float")
        ));
    }
}

/// Generate `ta_func_private.h` — the private-variant forward declarations. See
/// [`push_private_decls`].
pub fn generate_c_private_header(funcs: &[FuncDef]) -> String {
    let mut s = String::new();
    s.push_str("/* Auto-generated by ta_codegen — do not edit.\n");
    s.push_str(" * Private function declarations (double + single-precision, custom signatures).\n");
    s.push_str(" */\n\n");
    s.push_str("#ifndef TA_FUNC_PRIVATE_H\n");
    s.push_str("#define TA_FUNC_PRIVATE_H\n\n");
    push_private_decls(&mut s, funcs);
    s.push_str("\n#endif /* TA_FUNC_PRIVATE_H */\n");
    s
}

/// Generate a standalone C JSON-RPC server source file.
///
/// The generated file #includes the generated ta_*.c files and provides
/// a `main()` loop that reads JSON-RPC from stdin.
pub fn generate_c_server(funcs: &[FuncDef]) -> String {
    let mut s = String::new();

    // Header
    s.push_str("/* Auto-generated JSON-RPC server for ta_codegen C output.\n");
    s.push_str(" * Reads JSON-RPC requests from stdin, writes responses to stdout.\n");
    s.push_str(" * Build: compile each ta_*.c separately, then link with this file.\n");
    s.push_str(" */\n");
    s.push_str("#include <stdio.h>\n");
    s.push_str("#include <stdlib.h>\n");
    s.push_str("#include <string.h>\n");
    s.push_str("#include <math.h>\n");
    s.push_str("#include <time.h>\n");
    s.push_str("#ifdef __APPLE__\n");
    s.push_str("#include <mach/mach_time.h>\n");
    s.push_str("#endif\n\n");

    // Include headers for unguarded and private function declarations
    s.push_str("#include \"ta_func_unguarded.h\"\n");
    s.push_str("#include \"ta_func/ta_func_private.h\"\n\n");

    // Include ta_common (globals, utility functions, version, retcode)
    s.push_str("#include \"ta_common/ta_global.c\"\n");
    s.push_str("#include \"ta_func/ta_utility.c\"\n");
    s.push_str("#include \"ta_common/ta_version.c\"\n");
    s.push_str("#include \"ta_common/ta_retcode.c\"\n\n");

    // Include generated function implementations (single TU for best optimization).
    // Order matters: functions that are called by others must come first.
    let mut sorted_names: Vec<&str> = funcs.iter().map(|f| f.name.as_str()).collect();
    sorted_names.sort_unstable();
    // Move MA to end if present (it calls other functions)
    if let Some(pos) = sorted_names.iter().position(|n| *n == "MA") {
        let ma = sorted_names.remove(pos);
        sorted_names.push(ma);
    }
    for name in &sorted_names {
        s.push_str(&format!("#include \"ta_func/ta_{name}.c\"\n"));
    }
    s.push('\n');
    // Include ta_abstract layer (tables, frames, abstract dispatch)
    s.push_str("#include \"ta_abstract_all.c\"\n");
    s.push_str("#include \"ta_abstract/ta_func_api.c\"\n\n");

    // JSON helpers
    s.push_str(&generate_c_json_helpers());

    // Shared static buffers (used by both abstract handlers and per-function dispatch)
    s.push_str(&generate_c_global_buffers());

    // Generic ta_abstract handlers (abstract_call, abstract_get_lookback, abstract_for_each_func)
    s.push_str(&generate_c_abstract_handlers());

    // Dispatch function
    s.push_str(&generate_c_dispatch(funcs));

    // Main loop
    s.push_str("int main(void) {\n");
    s.push_str("    TA_Initialize();\n");
    // Buffers sized for load_data: 100k points × 6 arrays × ~20 chars ≈ 12MB
    s.push_str("    static char line[16*1024*1024];\n");
    s.push_str("    static char response[16*1024*1024];\n");
    s.push_str("    while( fgets(line, sizeof(line), stdin) ) {\n");
    s.push_str("        handle_request(line, response, sizeof(response));\n");
    s.push_str("        printf(\"%s\\n\", response);\n");
    s.push_str("        fflush(stdout);\n");
    s.push_str("    }\n");
    s.push_str("    return 0;\n");
    s.push_str("}\n");

    s
}

fn generate_c_json_helpers() -> String {
    r#"/* ---- Minimal JSON helpers ---- */

#define MAX_ARRAY_SIZE 200000

static int json_find_int(const char *json, const char *field) {
    char pattern[256];
    snprintf(pattern, sizeof(pattern), "\"%s\":", field);
    const char *p = strstr(json, pattern);
    if( !p ) return 0;
    p += strlen(pattern);
    while( *p == ' ' ) p++;
    return atoi(p);
}

static double json_find_double(const char *json, const char *field) {
    char pattern[256];
    snprintf(pattern, sizeof(pattern), "\"%s\":", field);
    const char *p = strstr(json, pattern);
    if( !p ) return 0.0;
    p += strlen(pattern);
    while( *p == ' ' ) p++;
    return strtod(p, NULL);
}

static int json_find_double_array(const char *json, const char *field,
                                   double *out, int max_count) {
    char pattern[256];
    snprintf(pattern, sizeof(pattern), "\"%s\":", field);
    const char *p = strstr(json, pattern);
    if( !p ) return 0;
    p += strlen(pattern);
    while( *p == ' ' ) p++;
    if( *p != '[' ) return 0;
    p++;
    int count = 0;
    while( *p && *p != ']' && count < max_count ) {
        while( *p == ' ' || *p == ',' ) p++;
        if( *p == ']' ) break;
        out[count] = strtod(p, (char **)&p);
        count++;
    }
    return count;
}

static const char *json_find_string(const char *json, const char *field,
                                     int *len) {
    char pattern[256];
    snprintf(pattern, sizeof(pattern), "\"%s\":\"", field);
    const char *p = strstr(json, pattern);
    if( !p ) return NULL;
    p += strlen(pattern);
    const char *start = p;
    while( *p && *p != '"' ) p++;
    *len = (int)(p - start);
    return start;
}

static int json_write_double_array(char *buf, int buf_size,
                                    const double *data, int count) {
    int pos = 0;
    buf[pos++] = '[';
    for( int i = 0; i < count; i++ ) {
        if( i > 0 ) pos += snprintf(buf + pos, buf_size - pos, ",");
        pos += snprintf(buf + pos, buf_size - pos, "%.15g", data[i]);
    }
    buf[pos++] = ']';
    buf[pos] = '\0';
    return pos;
}

static int json_write_int_array(char *buf, int buf_size,
                                 const int *data, int count) {
    int pos = 0;
    buf[pos++] = '[';
    for( int i = 0; i < count; i++ ) {
        if( i > 0 ) pos += snprintf(buf + pos, buf_size - pos, ",");
        pos += snprintf(buf + pos, buf_size - pos, "%d", data[i]);
    }
    buf[pos++] = ']';
    buf[pos] = '\0';
    return pos;
}

static long get_nanotime(void) {
#ifdef __APPLE__
    /* mach_absolute_time has ~42ns resolution on Apple Silicon;
       clock_gettime(CLOCK_MONOTONIC) only has 1000ns resolution on macOS. */
    static mach_timebase_info_data_t info = {0, 0};
    if( info.denom == 0 ) mach_timebase_info(&info);
    uint64_t t = mach_absolute_time();
    return (long)(t * info.numer / info.denom);
#else
    struct timespec ts;
    if( clock_gettime(CLOCK_MONOTONIC, &ts) == 0 ) {
        return (long)ts.tv_sec * 1000000000LL + (long)ts.tv_nsec;
    }
    return 0;
#endif
}

"#
    .to_string()
}

/// Emit shared static buffer declarations used by both abstract handlers and
/// per-function dispatch.
fn generate_c_global_buffers() -> String {
    let mut s = String::new();
    // Static buffers for input arrays — up to 6 for full OHLCV + openInterest.
    s.push_str("static double g_inBuf0[MAX_ARRAY_SIZE];\n");
    s.push_str("static double g_inBuf1[MAX_ARRAY_SIZE];\n");
    s.push_str("static double g_inBuf2[MAX_ARRAY_SIZE];\n");
    s.push_str("static double g_inBuf3[MAX_ARRAY_SIZE];\n");
    s.push_str("static double g_inBuf4[MAX_ARRAY_SIZE];\n");
    s.push_str("static double g_inBuf5[MAX_ARRAY_SIZE];\n");
    // Single-precision mirrors of the input buffers for the "use_float" leg
    // (TA_S_ variants). Converted from g_inBuf* on demand.
    s.push_str("static float g_sinBuf0[MAX_ARRAY_SIZE];\n");
    s.push_str("static float g_sinBuf1[MAX_ARRAY_SIZE];\n");
    s.push_str("static float g_sinBuf2[MAX_ARRAY_SIZE];\n");
    s.push_str("static float g_sinBuf3[MAX_ARRAY_SIZE];\n");
    s.push_str("static float g_sinBuf4[MAX_ARRAY_SIZE];\n");
    s.push_str("static float g_sinBuf5[MAX_ARRAY_SIZE];\n");
    // Real output buffers — up to 3 for MACD/BBANDS/STOCH
    s.push_str("static double g_outBuf0[MAX_ARRAY_SIZE];\n");
    s.push_str("static double g_outBuf1[MAX_ARRAY_SIZE];\n");
    s.push_str("static double g_outBuf2[MAX_ARRAY_SIZE];\n");
    // Integer output buffers — for CDL* patterns and MINMAXINDEX
    s.push_str("static int g_outIntBuf0[MAX_ARRAY_SIZE];\n");
    s.push_str("static int g_outIntBuf1[MAX_ARRAY_SIZE];\n\n");

    // Pre-loaded reference data (immutable after load_data, copied to working buffers per call)
    s.push_str("/* Pre-loaded OHLCV reference data for perftest.\n");
    s.push_str(" * Stored separately from working buffers to protect against mutation. */\n");
    s.push_str("static double g_refOpen[MAX_ARRAY_SIZE];\n");
    s.push_str("static double g_refHigh[MAX_ARRAY_SIZE];\n");
    s.push_str("static double g_refLow[MAX_ARRAY_SIZE];\n");
    s.push_str("static double g_refClose[MAX_ARRAY_SIZE];\n");
    s.push_str("static double g_refVolume[MAX_ARRAY_SIZE];\n");
    s.push_str("static double g_refOI[MAX_ARRAY_SIZE];\n");
    s.push_str("static int g_refN = 0; /* number of pre-loaded points */\n\n");

    // Helper: copy pre-loaded data into working input buffers based on input type
    s.push_str("static void preload_to_working(int nInputs, int isPriceInput) {\n");
    s.push_str("    if( isPriceInput ) {\n");
    s.push_str("        /* OHLCV — map into g_inBuf0..4 in OHLCV order */\n");
    s.push_str("        memcpy(g_inBuf0, g_refOpen,   g_refN * sizeof(double));\n");
    s.push_str("        memcpy(g_inBuf1, g_refHigh,   g_refN * sizeof(double));\n");
    s.push_str("        memcpy(g_inBuf2, g_refLow,    g_refN * sizeof(double));\n");
    s.push_str("        memcpy(g_inBuf3, g_refClose,  g_refN * sizeof(double));\n");
    s.push_str("        memcpy(g_inBuf4, g_refVolume, g_refN * sizeof(double));\n");
    s.push_str("        memcpy(g_inBuf5, g_refOI,     g_refN * sizeof(double));\n");
    s.push_str("    } else {\n");
    s.push_str("        /* Single/dual real input — use close (and high for 2nd) */\n");
    s.push_str("        memcpy(g_inBuf0, g_refClose, g_refN * sizeof(double));\n");
    s.push_str("        if( nInputs > 1 ) memcpy(g_inBuf1, g_refHigh, g_refN * sizeof(double));\n");
    s.push_str("    }\n");
    s.push_str("}\n\n");
    s
}

/// The abstract handler C code lives in ta_codegen/input/lib/c/ta_abstract_serve.c
/// (native C, not generated). The server just #includes it.
fn generate_c_abstract_handlers() -> String {
    "#include \"ta_abstract_serve.c\"\n\n".to_string()
}

#[allow(clippy::too_many_lines)]
fn generate_c_dispatch(funcs: &[FuncDef]) -> String {
    let mut s = String::new();

    // Global buffers and preload helper now emitted by generate_c_global_buffers()

    s.push_str("static void handle_request(const char *json, char *resp, int resp_size) {\n");

    // Extract method name
    s.push_str("    int methodLen = 0;\n");
    s.push_str("    const char *method = json_find_string(json, \"method\", &methodLen);\n");
    s.push_str("    if( !method ) {\n");
    s.push_str(
        "        snprintf(resp, resp_size, \"{\\\"error\\\":\\\"Missing method field\\\"}\");\n",
    );
    s.push_str("        return;\n");
    s.push_str("    }\n\n");

    // Handle load_data for perftest pre-loading
    s.push_str("    if ( methodLen == 9 && strncmp(method, \"load_data\", 9) == 0 ) {\n");
    s.push_str("        g_refN = json_find_double_array(json, \"open\",   g_refOpen,   MAX_ARRAY_SIZE);\n");
    s.push_str("        json_find_double_array(json, \"high\",          g_refHigh,   MAX_ARRAY_SIZE);\n");
    s.push_str("        json_find_double_array(json, \"low\",           g_refLow,    MAX_ARRAY_SIZE);\n");
    s.push_str("        json_find_double_array(json, \"close\",         g_refClose,  MAX_ARRAY_SIZE);\n");
    s.push_str("        json_find_double_array(json, \"volume\",        g_refVolume, MAX_ARRAY_SIZE);\n");
    s.push_str("        json_find_double_array(json, \"openInterest\",  g_refOI,     MAX_ARRAY_SIZE);\n");
    s.push_str("        snprintf(resp, resp_size, \"{\\\"status\\\":\\\"ok\\\",\\\"n\\\":%d}\", g_refN);\n");
    s.push_str("        return;\n");
    s.push_str("    }\n\n");

    // Dispatch each function
    for (i, func) in funcs.iter().enumerate() {
        let method_name = format!("TA_{}", func.name);
        let cond = if i == 0 { "if" } else { "else if" };

        s.push_str(&format!(
            "    {} ( methodLen == {} && strncmp(method, \"{}\", {}) == 0 ) {{\n",
            cond,
            method_name.len(),
            method_name,
            method_name.len()
        ));

        // Extract common params
        s.push_str("        int startIdx = json_find_int(json, \"startIdx\");\n");
        s.push_str("        int endIdx = json_find_int(json, \"endIdx\");\n");

        // Extract input arrays — either from pre-loaded reference data or inline JSON.
        let input_names = expand_input_names(&func.inputs);
        let is_price_input = input_names.iter().any(|n| {
            matches!(
                n.as_str(),
                "inOpen" | "inHigh" | "inLow" | "inClose" | "inVolume" | "inOpenInterest"
            )
        });
        let n_inputs = input_names.len();

        s.push_str("        int use_preloaded = json_find_int(json, \"use_preloaded\");\n");
        s.push_str("        if( use_preloaded && g_refN > 0 ) {\n");
        s.push_str(&format!(
            "            preload_to_working({n_inputs}, {});\n",
            i32::from(is_price_input)
        ));
        s.push_str("        } else {\n");
        for (j, name) in input_names.iter().enumerate() {
            let buf = format!("g_inBuf{j}");
            s.push_str(&format!(
                "            json_find_double_array(json, \"{name}\", {buf}, MAX_ARRAY_SIZE);\n",
            ));
        }
        s.push_str("        }\n");

        // Extract optional params
        for opt in &func.optional_inputs {
            if opt.param_type == ParamType::Real {
                s.push_str(&format!(
                    "        double {} = json_find_double(json, \"{}\");\n",
                    opt.name, opt.name
                ));
            } else if matches!(&opt.param_type, ParamType::Enum(_)) {
                s.push_str(&format!(
                    "        TA_MAType {} = (TA_MAType)json_find_int(json, \"{}\");\n",
                    opt.name, opt.name
                ));
            } else {
                s.push_str(&format!(
                    "        int {} = json_find_int(json, \"{}\");\n",
                    opt.name, opt.name
                ));
            }
        }

        // Apply unstable period if provided
        if let Some(id) = func_unst_id(&func.name) {
            s.push_str(&format!(
                "        TA_SetUnstablePeriod({id}, json_find_int(json, \"unstablePeriod\"));\n"
            ));
        }

        // Declare output variables
        s.push_str("        int outBegIdx = 0, outNBElement = 0;\n");

        // Benchmark iteration support: if request contains "iters", loop
        // the function call that many times. Copy input data before each
        // iteration (outside timing) to ensure identical input state.
        // Only the indicator call itself is timed.
        s.push_str("        int bench_iters = json_find_int(json, \"iters\");\n");
        s.push_str("        if( bench_iters < 1 ) bench_iters = 1;\n");

        s.push_str("        TA_RetCode rc = 0;\n");

        // Copy once before timing
        s.push_str("        if( use_preloaded ) {\n");
        s.push_str(&format!(
            "            preload_to_working({n_inputs}, {});\n",
            i32::from(is_price_input)
        ));
        s.push_str("        }\n");

        // Single timing block around ALL iterations — amortizes timer overhead
        s.push_str("        long _t0 = get_nanotime();\n");
        s.push_str("        for( int _bi = 0; _bi < bench_iters; _bi++ ) {\n");

        // Call the function
        s.push_str(&format!("        rc = TA_{}(\n", func.name));
        s.push_str("            startIdx, endIdx,\n");

        // Input arrays
        for (j, _name) in input_names.iter().enumerate() {
            s.push_str(&format!("            g_inBuf{j},\n"));
        }

        // Optional params
        for opt in &func.optional_inputs {
            s.push_str(&format!("            {},\n", opt.name));
        }

        // Output scalar params + output array params (one per output).
        // Real outputs → g_outBuf{real_idx}, integer outputs → g_outIntBuf{int_idx}.
        let outputs = &func.outputs;
        s.push_str("            &outBegIdx, &outNBElement");
        {
            let mut real_idx = 0usize;
            let mut int_idx = 0usize;
            for out in outputs {
                if out.param_type == ParamType::Integer {
                    s.push_str(&format!(", g_outIntBuf{int_idx}"));
                    int_idx += 1;
                } else {
                    s.push_str(&format!(", g_outBuf{real_idx}"));
                    real_idx += 1;
                }
            }
        }
        s.push_str(");\n");
        s.push_str("        }\n"); // end bench_iters loop
        s.push_str("        long elapsed_ns = (get_nanotime() - _t0) / bench_iters;\n");

        // Unguarded timing pass — skipped when built as ta_ref_serve (no _Unguarded in libta-lib.a)
        s.push_str("#ifndef TA_REF_SERVE\n");
        s.push_str("        long _t0_ung = get_nanotime();\n");
        s.push_str("        for( int _biu = 0; _biu < bench_iters; _biu++ ) {\n");
        s.push_str(&format!("        rc = TA_{}_Unguarded(\n", func.name));
        s.push_str("            startIdx, endIdx,\n");
        for (j, _name) in input_names.iter().enumerate() {
            s.push_str(&format!("            g_inBuf{j},\n"));
        }
        for opt in &func.optional_inputs {
            s.push_str(&format!("            {},\n", opt.name));
        }
        s.push_str("            &outBegIdx, &outNBElement");
        {
            let mut real_idx = 0usize;
            let mut int_idx = 0usize;
            for out in outputs {
                if out.param_type == ParamType::Integer {
                    s.push_str(&format!(", g_outIntBuf{int_idx}"));
                    int_idx += 1;
                } else {
                    s.push_str(&format!(", g_outBuf{real_idx}"));
                    real_idx += 1;
                }
            }
        }
        s.push_str(");\n");
        s.push_str("        }\n"); // end unguarded bench loop
        s.push_str("        long elapsed_ns_ung = (get_nanotime() - _t0_ung) / bench_iters;\n");
        s.push_str("#else\n");
        s.push_str("        long elapsed_ns_ung = 0;\n");
        s.push_str("#endif /* TA_REF_SERVE */\n");

        // Float-variant leg: with "use_float":1 the call is re-run through the
        // single-precision TA_S_ API (inputs converted to float) and the
        // response carries the S-variant result instead. The frozen reference
        // library also exports the guarded TA_S_ functions, so ta_ref_serve
        // answers this too — giving S-vs-S comparison against the reference.
        // Mirrors the double flow: guarded first, then (outside ta_ref_serve)
        // the unguarded S variant over the same buffers.
        s.push_str("        if( json_find_int(json, \"use_float\") ) {\n");
        for (j, _name) in input_names.iter().enumerate() {
            s.push_str(&format!(
                "            for( int _fi = 0; _fi <= endIdx; _fi++ ) g_sinBuf{j}[_fi] = (float)g_inBuf{j}[_fi];\n"
            ));
        }
        let emit_s_call = |s: &mut String, suffix: &str| {
            s.push_str(&format!("            rc = TA_S_{}{}(\n", func.name, suffix));
            s.push_str("                startIdx, endIdx,\n");
            for (j, _name) in input_names.iter().enumerate() {
                s.push_str(&format!("                g_sinBuf{j},\n"));
            }
            for opt in &func.optional_inputs {
                s.push_str(&format!("                {},\n", opt.name));
            }
            s.push_str("                &outBegIdx, &outNBElement");
            let mut real_idx = 0usize;
            let mut int_idx = 0usize;
            for out in &func.outputs {
                if out.param_type == ParamType::Integer {
                    s.push_str(&format!(", g_outIntBuf{int_idx}"));
                    int_idx += 1;
                } else {
                    s.push_str(&format!(", g_outBuf{real_idx}"));
                    real_idx += 1;
                }
            }
            s.push_str(");\n");
        };
        emit_s_call(&mut s, "");
        s.push_str("#ifndef TA_REF_SERVE\n");
        emit_s_call(&mut s, "_Unguarded");
        s.push_str("#endif /* TA_REF_SERVE */\n");
        s.push_str("        }\n");

        // Build response with correct key names and serialisers per output type.
        s.push_str("        int pos = snprintf(resp, resp_size,\n");
        s.push_str("            \"{\\\"retCode\\\":%d,\\\"outBegIdx\\\":%d,\\\"outNBElement\\\":%d,\\\"timing_ns\\\":%ld\",\n");
        s.push_str("            (int)rc, outBegIdx, outNBElement, elapsed_ns);\n");
        {
            let mut real_idx = 0usize;
            let mut int_idx = 0usize;
            for (k, out) in outputs.iter().enumerate() {
                let key = output_json_key(outputs, k);
                s.push_str(&format!(
                    "        pos += snprintf(resp + pos, resp_size - pos, \",\\\"{key}\\\":\");\n"
                ));
                if out.param_type == ParamType::Integer {
                    s.push_str(&format!(
                        "        pos += json_write_int_array(resp + pos, resp_size - pos, g_outIntBuf{int_idx}, outNBElement);\n"
                    ));
                    int_idx += 1;
                } else {
                    s.push_str(&format!(
                        "        pos += json_write_double_array(resp + pos, resp_size - pos, g_outBuf{real_idx}, outNBElement);\n"
                    ));
                    real_idx += 1;
                }
            }
        }
        s.push_str("        pos += snprintf(resp + pos, resp_size - pos, \",\\\"timing_ns_unguarded\\\":%ld}\", elapsed_ns_ung);\n");

        s.push_str("    }\n");
    }

    // Lookback dispatch for each function
    for func in funcs {
        let method_name = format!("TA_{}_Lookback", func.name);

        s.push_str(&format!(
            "    else if ( methodLen == {} && strncmp(method, \"{}\", {}) == 0 ) {{\n",
            method_name.len(),
            method_name,
            method_name.len()
        ));

        // Extract optional params
        for opt in &func.optional_inputs {
            if opt.param_type == ParamType::Real {
                s.push_str(&format!(
                    "        double {} = json_find_double(json, \"{}\");\n",
                    opt.name, opt.name
                ));
            } else if matches!(&opt.param_type, ParamType::Enum(_)) {
                s.push_str(&format!(
                    "        TA_MAType {} = (TA_MAType)json_find_int(json, \"{}\");\n",
                    opt.name, opt.name
                ));
            } else {
                s.push_str(&format!(
                    "        int {} = json_find_int(json, \"{}\");\n",
                    opt.name, opt.name
                ));
            }
        }

        // Call lookback function
        s.push_str(&format!(
            "        int lookback = TA_{}_Lookback(",
            func.name
        ));
        let opt_names: Vec<String> = func
            .optional_inputs
            .iter()
            .map(|o| o.name.clone())
            .collect();
        s.push_str(&opt_names.join(", "));
        s.push_str(");\n");

        // Build response
        s.push_str("        snprintf(resp, resp_size,\n");
        s.push_str("            \"{\\\"lookback\\\":%d}\", lookback);\n");
        s.push_str("    }\n");
    }

    // list_functions method — returns {"functions":["TA_SMA","TA_RSI",...]}
    s.push_str("    else if ( methodLen == 14 && strncmp(method, \"list_functions\", 14) == 0 ) {\n");
    s.push_str("        int pos = snprintf(resp, resp_size, \"{\\\"functions\\\":[\");\n");
    for (i, func) in funcs.iter().enumerate() {
        let comma = if i > 0 { "," } else { "" };
        s.push_str(&format!(
            "        pos += snprintf(resp + pos, resp_size - pos, \"{}\\\"TA_{}\\\"\");\n",
            comma, func.name
        ));
    }
    s.push_str("        snprintf(resp + pos, resp_size - pos, \"]}\");\n");
    s.push_str("    }\n");

    // set_unstable_period method — {"method":"set_unstable_period","params":{"id":21,"period":10}}
    s.push_str("    else if ( methodLen == 19 && strncmp(method, \"set_unstable_period\", 19) == 0 ) {\n");
    s.push_str("        int id = json_find_int(json, \"id\");\n");
    s.push_str("        int period = json_find_int(json, \"period\");\n");
    s.push_str("        TA_SetUnstablePeriod((TA_FuncUnstId)id, (unsigned int)period);\n");
    s.push_str("        snprintf(resp, resp_size, \"{\\\"status\\\":\\\"ok\\\"}\");\n");
    s.push_str("    }\n");

    // set_compatibility method — {"method":"set_compatibility","params":{"mode":1}}
    s.push_str("    else if ( methodLen == 17 && strncmp(method, \"set_compatibility\", 17) == 0 ) {\n");
    s.push_str("        int mode = json_find_int(json, \"mode\");\n");
    s.push_str("        TA_SetCompatibility((TA_Compatibility)mode);\n");
    s.push_str("        snprintf(resp, resp_size, \"{\\\"status\\\":\\\"ok\\\"}\");\n");
    s.push_str("    }\n");

    // abstract_call — generic function call via ta_abstract
    s.push_str("    else if ( methodLen == 13 && strncmp(method, \"abstract_call\", 13) == 0 ) {\n");
    s.push_str("        handle_abstract_call(json, resp, resp_size);\n");
    s.push_str("    }\n");

    // abstract_get_lookback — lookback query via ta_abstract
    s.push_str("    else if ( methodLen == 21 && strncmp(method, \"abstract_get_lookback\", 21) == 0 ) {\n");
    s.push_str("        handle_abstract_get_lookback(json, resp, resp_size);\n");
    s.push_str("    }\n");

    // abstract_for_each_func — enumerate functions via ta_abstract
    s.push_str("    else if ( methodLen == 22 && strncmp(method, \"abstract_for_each_func\", 22) == 0 ) {\n");
    s.push_str("        handle_abstract_for_each_func(json, resp, resp_size);\n");
    s.push_str("    }\n");

    // TA_GetFuncInfo — function metadata via ta_abstract
    s.push_str("    else if ( methodLen == 14 && strncmp(method, \"TA_GetFuncInfo\", 14) == 0 ) {\n");
    s.push_str("        handle_TA_GetFuncInfo(json, resp, resp_size);\n");
    s.push_str("    }\n");

    // TA_GetInputParameterInfo
    s.push_str("    else if ( methodLen == 24 && strncmp(method, \"TA_GetInputParameterInfo\", 24) == 0 ) {\n");
    s.push_str("        handle_TA_GetInputParameterInfo(json, resp, resp_size);\n");
    s.push_str("    }\n");

    // TA_GetOptInputParameterInfo
    s.push_str("    else if ( methodLen == 27 && strncmp(method, \"TA_GetOptInputParameterInfo\", 27) == 0 ) {\n");
    s.push_str("        handle_TA_GetOptInputParameterInfo(json, resp, resp_size);\n");
    s.push_str("    }\n");

    // TA_GetOutputParameterInfo
    s.push_str("    else if ( methodLen == 25 && strncmp(method, \"TA_GetOutputParameterInfo\", 25) == 0 ) {\n");
    s.push_str("        handle_TA_GetOutputParameterInfo(json, resp, resp_size);\n");
    s.push_str("    }\n");

    // TA_FunctionDescriptionXML
    s.push_str("    else if ( methodLen == 25 && strncmp(method, \"TA_FunctionDescriptionXML\", 25) == 0 ) {\n");
    s.push_str("        handle_TA_FunctionDescriptionXML(json, resp, resp_size);\n");
    s.push_str("    }\n");

    // Unknown method
    s.push_str("    else {\n");
    s.push_str("        snprintf(resp, resp_size,\n");
    s.push_str(
        "            \"{\\\"error\\\":\\\"Unknown method: %.*s\\\"}\", methodLen, method);\n",
    );
    s.push_str("    }\n");

    s.push_str("}\n\n");
    s
}

/// Generate a Java JSON-RPC server source file.
///
/// Generates a single TaCodegenServe.java with all necessary classes inline
/// (`RetCode` enum, `MInteger`, Core class with methods, main server loop).
#[allow(clippy::too_many_lines)]
#[allow(clippy::implicit_hasher)]
pub fn generate_java_server(funcs: &[FuncDef], enums: &HashMap<String, EnumDef>) -> String {
    let mut s = String::new();

    s.push_str("/* Auto-generated JSON-RPC server for ta_codegen Java output.\n");
    s.push_str(" * Build: javac TaCodegenServe.java && java TaCodegenServe\n");
    s.push_str(" */\n");
    s.push_str("import java.io.*;\n");
    s.push_str("import java.util.*;\n\n");

    // RetCode enum
    s.push_str("enum RetCode {\n");
    s.push_str("    Success, BadParam, OutOfRangeStartIndex, OutOfRangeEndIndex, AllocErr, InternalError;\n");
    s.push_str("    public int toInt() {\n");
    s.push_str("        switch(this) {\n");
    s.push_str("            case Success: return 0;\n");
    s.push_str("            case BadParam: return 2;\n");
    s.push_str("            case OutOfRangeStartIndex: return 12;\n");
    s.push_str("            case OutOfRangeEndIndex: return 13;\n");
    s.push_str("            case AllocErr: return 3;\n");
    s.push_str("            default: return 5000;\n");
    s.push_str("        }\n");
    s.push_str("    }\n");
    s.push_str("}\n\n");

    // MInteger helper
    s.push_str("class MInteger { public int value; }\n\n");

    // FuncUnstId and Compatibility enums (referenced by generated Core methods).
    // FuncUnstId is emitted from enums.yaml (source of truth), 6 names per line,
    // plus the server-side `None` sentinel (distinct from the shipped enum's `All`).
    s.push_str("enum FuncUnstId {\n");
    {
        let names = func_unst_pascal_names(enums);
        let nchunks = names.chunks(6).count().max(1);
        for (idx, chunk) in names.chunks(6).enumerate() {
            if idx + 1 == nchunks {
                // Last line carries the server-side `None` sentinel and the `;`.
                s.push_str(&format!("    {}, None;\n", chunk.join(", ")));
            } else {
                s.push_str(&format!("    {},\n", chunk.join(", ")));
            }
        }
    }
    s.push_str("}\n\n");

    s.push_str("enum Compatibility {\n");
    s.push_str("    Default, Metastock;\n");
    s.push_str("}\n\n");

    s.push_str("enum MAType {\n");
    s.push_str("    Sma, Ema, Wma, Dema, Tema, Trima, Kama, Mama, T3;\n");
    s.push_str("}\n\n");

    // RangeType — mirrors the shipped enum (RealBody=0, HighLow=1, Shadows=2) so the
    // canonical candle access (`rangeType.ordinal()`) compiles here as in Core.java.
    s.push_str("enum RangeType {\n");
    s.push_str("    RealBody, HighLow, Shadows;\n");
    s.push_str("}\n\n");

    // CandleSetting holds rangeType, avgPeriod, factor for one candle setting
    s.push_str("class CandleSetting {\n");
    s.push_str("    RangeType rangeType;\n");
    s.push_str("    int avgPeriod;\n");
    s.push_str("    double factor;\n");
    s.push_str("    CandleSetting(RangeType rt, int ap, double f) { rangeType = rt; avgPeriod = ap; factor = f; }\n");
    s.push_str("}\n\n");

    // CandleSettingType — ordinals index the `candleSettings` array below, matching
    // the canonical shipped Core.java access form emitted by emit_java_unpacking()
    // (`candleSettings[CandleSettingType.X.ordinal()]`).
    s.push_str("enum CandleSettingType {\n");
    s.push_str("    BodyLong, BodyVeryLong, BodyShort, BodyDoji,\n");
    s.push_str("    ShadowLong, ShadowVeryLong, ShadowShort, ShadowVeryShort,\n");
    s.push_str("    Near, Far, Equal, AllCandleSettings;\n");
    s.push_str("}\n\n");

    // Core class — method bodies are inlined by the caller via inline_java_core_methods()
    s.push_str("class Core {\n");
    s.push_str("    int[] unstablePeriod = new int[FuncUnstId.values().length];\n");
    s.push_str("    Compatibility compatibility = Compatibility.Default;\n");
    // candleSettings[] in CandleSettingType ordinal order. Defaults from
    // TA_RestoreCandleDefaultSettings in ta_global.c. RangeType: 0=RealBody, 1=HighLow, 2=Shadows.
    s.push_str("    CandleSetting[] candleSettings = {\n");
    s.push_str("        new CandleSetting(RangeType.RealBody, 10, 1.0),   // BodyLong\n");
    s.push_str("        new CandleSetting(RangeType.RealBody, 10, 3.0),   // BodyVeryLong\n");
    s.push_str("        new CandleSetting(RangeType.RealBody, 10, 1.0),   // BodyShort\n");
    s.push_str("        new CandleSetting(RangeType.HighLow,  10, 0.1),   // BodyDoji\n");
    s.push_str("        new CandleSetting(RangeType.RealBody, 0,  1.0),   // ShadowLong\n");
    s.push_str("        new CandleSetting(RangeType.RealBody, 0,  2.0),   // ShadowVeryLong\n");
    s.push_str("        new CandleSetting(RangeType.Shadows,  10, 1.0),   // ShadowShort\n");
    s.push_str("        new CandleSetting(RangeType.HighLow,  10, 0.1),   // ShadowVeryShort\n");
    s.push_str("        new CandleSetting(RangeType.HighLow,  5,  0.2),   // Near\n");
    s.push_str("        new CandleSetting(RangeType.HighLow,  5,  0.6),   // Far\n");
    s.push_str("        new CandleSetting(RangeType.HighLow,  5,  0.05),  // Equal\n");
    s.push_str("    };\n\n");
    for func in funcs {
        s.push_str(&format!("    // @@CORE_{}@@\n", func.name));
    }
    s.push_str("}\n\n");

    // Main server class
    s.push_str("public class TaCodegenServe {\n");
    s.push_str("    static Core core = new Core();\n");
    s.push_str("    static final int MAX_ARRAY_SIZE = 200000;\n");
    s.push_str("    static double[] refOpen = new double[MAX_ARRAY_SIZE];\n");
    s.push_str("    static double[] refHigh = new double[MAX_ARRAY_SIZE];\n");
    s.push_str("    static double[] refLow = new double[MAX_ARRAY_SIZE];\n");
    s.push_str("    static double[] refClose = new double[MAX_ARRAY_SIZE];\n");
    s.push_str("    static double[] refVolume = new double[MAX_ARRAY_SIZE];\n");
    s.push_str("    static double[] refOI = new double[MAX_ARRAY_SIZE];\n");
    s.push_str("    static int refN = 0;\n\n");

    // JSON helpers
    s.push_str("    static int jsonInt(String json, String field) {\n");
    s.push_str("        int idx = json.indexOf('\"' + field + '\"');\n");
    s.push_str("        if (idx < 0) return 0;\n");
    s.push_str("        idx = json.indexOf(':', idx) + 1;\n");
    s.push_str("        while (idx < json.length() && json.charAt(idx) == ' ') idx++;\n");
    s.push_str("        int end = idx;\n");
    s.push_str("        while (end < json.length() && \"0123456789-\".indexOf(json.charAt(end)) >= 0) end++;\n");
    s.push_str("        return Integer.parseInt(json.substring(idx, end));\n");
    s.push_str("    }\n\n");

    s.push_str("    static double jsonDouble(String json, String field) {\n");
    s.push_str("        int idx = json.indexOf('\"' + field + '\"');\n");
    s.push_str("        if (idx < 0) return 0.0;\n");
    s.push_str("        idx = json.indexOf(':', idx) + 1;\n");
    s.push_str("        while (idx < json.length() && json.charAt(idx) == ' ') idx++;\n");
    s.push_str("        int end = idx;\n");
    s.push_str("        while (end < json.length() && \"0123456789-.eE+\".indexOf(json.charAt(end)) >= 0) end++;\n");
    s.push_str("        return Double.parseDouble(json.substring(idx, end));\n");
    s.push_str("    }\n\n");

    s.push_str("    static double[] jsonDoubleArray(String json, String field) {\n");
    s.push_str("        int idx = json.indexOf('\"' + field + '\"');\n");
    s.push_str("        if (idx < 0) return new double[0];\n");
    s.push_str("        idx = json.indexOf('[', idx);\n");
    s.push_str("        int end = json.indexOf(']', idx);\n");
    s.push_str("        String inner = json.substring(idx + 1, end).trim();\n");
    s.push_str("        if (inner.isEmpty()) return new double[0];\n");
    s.push_str("        String[] parts = inner.split(\",\");\n");
    s.push_str("        double[] result = new double[parts.length];\n");
    s.push_str("        for (int i = 0; i < parts.length; i++)\n");
    s.push_str("            result[i] = Double.parseDouble(parts[i].trim());\n");
    s.push_str("        return result;\n");
    s.push_str("    }\n\n");

    s.push_str("    static String doubleArrayToJson(double[] arr, int count) {\n");
    s.push_str("        StringBuilder sb = new StringBuilder(\"[\");\n");
    s.push_str("        for (int i = 0; i < count; i++) {\n");
    s.push_str("            if (i > 0) sb.append(',');\n");
    s.push_str("            sb.append(arr[i]);\n");
    s.push_str("        }\n");
    s.push_str("        sb.append(']');\n");
    s.push_str("        return sb.toString();\n");
    s.push_str("    }\n\n");

    s.push_str("    static String intArrayToJson(int[] arr, int count) {\n");
    s.push_str("        StringBuilder sb = new StringBuilder(\"[\");\n");
    s.push_str("        for (int i = 0; i < count; i++) {\n");
    s.push_str("            if (i > 0) sb.append(',');\n");
    s.push_str("            sb.append(arr[i]);\n");
    s.push_str("        }\n");
    s.push_str("        sb.append(']');\n");
    s.push_str("        return sb.toString();\n");
    s.push_str("    }\n\n");

    // Dispatch method
    s.push_str("    static String handleRequest(String json) {\n");

    // Handle load_data for perftest pre-loading
    s.push_str("        if (json.contains(\"\\\"load_data\\\"\")) {\n");
    s.push_str("            double[] tmp = jsonDoubleArray(json, \"open\");\n");
    s.push_str("            refN = tmp.length;\n");
    s.push_str("            System.arraycopy(tmp, 0, refOpen, 0, refN);\n");
    s.push_str("            tmp = jsonDoubleArray(json, \"high\");\n");
    s.push_str("            System.arraycopy(tmp, 0, refHigh, 0, Math.min(tmp.length, MAX_ARRAY_SIZE));\n");
    s.push_str("            tmp = jsonDoubleArray(json, \"low\");\n");
    s.push_str("            System.arraycopy(tmp, 0, refLow, 0, Math.min(tmp.length, MAX_ARRAY_SIZE));\n");
    s.push_str("            tmp = jsonDoubleArray(json, \"close\");\n");
    s.push_str("            System.arraycopy(tmp, 0, refClose, 0, Math.min(tmp.length, MAX_ARRAY_SIZE));\n");
    s.push_str("            tmp = jsonDoubleArray(json, \"volume\");\n");
    s.push_str("            System.arraycopy(tmp, 0, refVolume, 0, Math.min(tmp.length, MAX_ARRAY_SIZE));\n");
    s.push_str("            tmp = jsonDoubleArray(json, \"openInterest\");\n");
    s.push_str("            System.arraycopy(tmp, 0, refOI, 0, Math.min(tmp.length, MAX_ARRAY_SIZE));\n");
    s.push_str("            return \"{\\\"status\\\":\\\"ok\\\",\\\"n\\\":\" + refN + \"}\";\n");
    s.push_str("        }\n");

    // Thin dispatch: each indicator delegates to its own static handle_XXX method.
    // This keeps handleRequest small enough for HotSpot C2 to JIT-compile it.
    for func in funcs {
        let method_name = format!("TA_{}", func.name);
        s.push_str(&format!(
            "        else if (json.contains(\"\\\"{method_name}\\\"\")) return handle_{}(json);\n",
            func.name
        ));
    }

    // list_functions method — returns {"functions":["TA_SMA","TA_RSI",...]}
    s.push_str("        else if (json.contains(\"\\\"list_functions\\\"\")) {\n");
    s.push_str("            StringBuilder sb = new StringBuilder(\"{\\\"functions\\\":[\");\n");
    for (i, func) in funcs.iter().enumerate() {
        if i > 0 {
            s.push_str("            sb.append(\",\");\n");
        }
        s.push_str(&format!("            sb.append(\"\\\"TA_{}\\\"\");\n", func.name));
    }
    s.push_str("            sb.append(\"]}\");\n");
    s.push_str("            return sb.toString();\n");
    s.push_str("        }\n");

    // set_unstable_period method — {"method":"set_unstable_period","params":{"id":21,"period":10}}
    s.push_str("        else if (json.contains(\"\\\"set_unstable_period\\\"\")) {\n");
    s.push_str("            int id = jsonInt(json, \"id\");\n");
    s.push_str("            int period = jsonInt(json, \"period\");\n");
    // id == length is the "set all" sentinel (matches C TA_SetUnstablePeriod).
    s.push_str("            if (id == core.unstablePeriod.length) {\n");
    s.push_str("                for (int i = 0; i < core.unstablePeriod.length; i++) core.unstablePeriod[i] = period;\n");
    s.push_str("                return \"{\\\"status\\\":\\\"ok\\\"}\"; \n");
    s.push_str("            }\n");
    s.push_str("            if (id >= 0 && id < core.unstablePeriod.length) {\n");
    s.push_str("                core.unstablePeriod[id] = period;\n");
    s.push_str("                return \"{\\\"status\\\":\\\"ok\\\"}\"; \n");
    s.push_str("            }\n");
    s.push_str("            return \"{\\\"error\\\":\\\"Invalid id\\\"}\"; \n");
    s.push_str("        }\n");

    // set_compatibility method
    s.push_str("        else if (json.contains(\"\\\"set_compatibility\\\"\")) {\n");
    s.push_str("            int mode = jsonInt(json, \"mode\");\n");
    s.push_str("            core.compatibility = (mode == 1) ? Compatibility.Metastock : Compatibility.Default;\n");
    s.push_str("            return \"{\\\"status\\\":\\\"ok\\\"}\";\n");
    s.push_str("        }\n");

    s.push_str("        else {\n");
    s.push_str("            return \"{\\\"error\\\":\\\"Unknown method\\\"}\";\n");
    s.push_str("        }\n");
    s.push_str("    }\n\n");

    // Per-function handler methods — each is small enough for C2 JIT compilation.
    for func in funcs {
        let func_lower = to_java_method_name(&func.name, func.camel_case.as_deref());

        s.push_str(&format!(
            "    static String handle_{}(String json) {{\n",
            func.name
        ));
        s.push_str("        int startIdx = jsonInt(json, \"startIdx\");\n");
        s.push_str("        int endIdx = jsonInt(json, \"endIdx\");\n");

        // Inputs — Real inputs use their own name; Price inputs expand to individual
        // component arrays (e.g. "inHigh", "inLow", "inClose").
        let input_names = expand_input_names(&func.inputs);

        // Check use_preloaded flag
        s.push_str("        int use_preloaded = jsonInt(json, \"use_preloaded\");\n");
        s.push_str("        int bench_iters = jsonInt(json, \"iters\");\n");
        s.push_str("        if (bench_iters < 1) bench_iters = 1;\n");

        // Parse input arrays or use pre-loaded data
        for name in &input_names {
            s.push_str(&format!(
                "        double[] {name} = new double[MAX_ARRAY_SIZE];\n"
            ));
        }
        s.push_str("        if (use_preloaded != 0 && refN > 0) {\n");
        for (j, name) in input_names.iter().enumerate() {
            let ref_src = if let Some(r) = price_input_to_ref(name) {
                r.to_string()
            } else if j == 0 {
                "refClose".to_string()
            } else {
                "refHigh".to_string()
            };
            s.push_str(&format!(
                "            System.arraycopy({ref_src}, 0, {name}, 0, refN);\n"
            ));
        }
        s.push_str("        } else {\n");
        for name in &input_names {
            s.push_str(&format!(
                "            double[] _tmp_{name} = jsonDoubleArray(json, \"{name}\");\n"
            ));
            s.push_str(&format!(
                "            {name} = _tmp_{name};\n"
            ));
        }
        s.push_str("        }\n");

        // Optional params
        for opt in &func.optional_inputs {
            if opt.param_type == ParamType::Real {
                s.push_str(&format!(
                    "        double {} = jsonDouble(json, \"{}\");\n",
                    opt.name, opt.name
                ));
            } else if let ParamType::Enum(ref enum_name) = opt.param_type {
                // Enum params: read as int, convert to enum type
                s.push_str(&format!(
                    "        {} {} = {}.values()[jsonInt(json, \"{}\")];\n",
                    enum_name, opt.name, enum_name, opt.name
                ));
            } else {
                s.push_str(&format!(
                    "        int {} = jsonInt(json, \"{}\");\n",
                    opt.name, opt.name
                ));
            }
        }

        // Apply unstable period if provided
        if let Some(id) = func_unst_id(&func.name) {
            s.push_str(&format!(
                "        core.unstablePeriod[{id}] = jsonInt(json, \"unstablePeriod\");\n"
            ));
        }

        // Outputs — one array per output, typed correctly (double[] or int[])
        let outputs = &func.outputs;
        for (k, out) in outputs.iter().enumerate() {
            let arr_name = format!("outArr{k}");
            if out.param_type == ParamType::Integer {
                s.push_str(&format!(
                    "        int[] {arr_name} = new int[endIdx - startIdx + 1];\n"
                ));
            } else {
                s.push_str(&format!(
                    "        double[] {arr_name} = new double[endIdx - startIdx + 1];\n"
                ));
            }
        }
        s.push_str("        MInteger outBegIdx = new MInteger();\n");
        s.push_str("        MInteger outNBElement = new MInteger();\n");
        s.push_str("        RetCode rc = RetCode.Success;\n");

        // Benchmark iteration loop with timing
        s.push_str("        long startNs = System.nanoTime();\n");
        s.push_str("        for (int _bi = 0; _bi < bench_iters; _bi++) {\n");

        // Call
        s.push_str(&format!("        rc = core.{func_lower}(\n"));
        s.push_str("            startIdx, endIdx,\n");
        for name in &input_names {
            s.push_str(&format!("            {name},\n"));
        }
        for opt in &func.optional_inputs {
            s.push_str(&format!("            {},\n", opt.name));
        }
        s.push_str("            outBegIdx, outNBElement");
        for k in 0..outputs.len() {
            s.push_str(&format!(", outArr{k}"));
        }
        s.push_str(");\n");
        s.push_str("        }\n"); // end bench_iters loop

        // Timing capture
        s.push_str("        long elapsedNs = (System.nanoTime() - startNs) / bench_iters;\n");

        // Unguarded timing loop
        let unguarded_name = format!("{func_lower}Unguarded");
        s.push_str("        long startNsUng = System.nanoTime();\n");
        s.push_str("        for (int _biu = 0; _biu < bench_iters; _biu++) {\n");
        s.push_str(&format!("        rc = core.{unguarded_name}(\n"));
        s.push_str("            startIdx, endIdx,\n");
        for name in &input_names {
            s.push_str(&format!("            {name},\n"));
        }
        for opt in &func.optional_inputs {
            s.push_str(&format!("            {},\n", opt.name));
        }
        s.push_str("            outBegIdx, outNBElement");
        for k in 0..outputs.len() {
            s.push_str(&format!(", outArr{k}"));
        }
        s.push_str(");\n");
        s.push_str("        }\n"); // end unguarded bench loop
        s.push_str("        long elapsedNsUng = (System.nanoTime() - startNsUng) / bench_iters;\n");

        // Response — use correct key names and serialisers per output type
        s.push_str("        StringBuilder sb = new StringBuilder();\n");
        s.push_str("        sb.append(\"{\\\"retCode\\\":\").append(rc.toInt());\n");
        s.push_str(
            "        sb.append(\",\\\"outBegIdx\\\":\").append(outBegIdx.value);\n",
        );
        s.push_str(
            "        sb.append(\",\\\"outNBElement\\\":\").append(outNBElement.value);\n",
        );
        for (k, out) in outputs.iter().enumerate() {
            let arr_name = format!("outArr{k}");
            let key = output_json_key(outputs, k);
            if out.param_type == ParamType::Integer {
                s.push_str(&format!(
                    "        sb.append(\",\\\"{key}\\\":\").append(intArrayToJson({arr_name}, outNBElement.value));\n"
                ));
            } else {
                s.push_str(&format!(
                    "        sb.append(\",\\\"{key}\\\":\").append(doubleArrayToJson({arr_name}, outNBElement.value));\n"
                ));
            }
        }
        s.push_str("        sb.append(\",\\\"timing_ns\\\":\").append(elapsedNs);\n");
        s.push_str("        sb.append(\",\\\"timing_ns_unguarded\\\":\").append(elapsedNsUng);\n");
        s.push_str("        sb.append(\"}\");\n");
        s.push_str("        return sb.toString();\n");

        s.push_str("    }\n\n");
    }

    // Main method
    s.push_str("    public static void main(String[] args) throws Exception {\n");
    s.push_str(
        "        BufferedReader reader = new BufferedReader(new InputStreamReader(System.in));\n",
    );
    s.push_str("        String line;\n");
    s.push_str("        while ((line = reader.readLine()) != null) {\n");
    s.push_str("            if (line.trim().isEmpty()) continue;\n");
    s.push_str("            System.out.println(handleRequest(line));\n");
    s.push_str("            System.out.flush();\n");
    s.push_str("        }\n");
    s.push_str("    }\n");
    s.push_str("}\n");

    s
}

/// Generate a .NET (C#) JSON-RPC server source file.
///
/// Emits a complete C# program that uses P/Invoke to call the generated C shared
/// library (`ta_codegen_funcs`), reads JSON-RPC requests from stdin, dispatches to
/// the imported TA functions, and writes JSON responses to stdout.
#[allow(clippy::too_many_lines, clippy::cognitive_complexity)]
pub fn generate_dotnet_server(funcs: &[FuncDef]) -> String {
    let mut s = String::new();

    s.push_str("// Auto-generated JSON-RPC server for ta_codegen .NET output.\n");
    s.push_str("// Uses P/Invoke to call the generated C shared library.\n");
    s.push_str("// Requires: dotnet 8.0+, libta_codegen_funcs.dylib/.so in bin/\n");
    s.push_str("using System;\n");
    s.push_str("using System.IO;\n");
    s.push_str("using System.Text.Json;\n");
    s.push_str("using System.Runtime.InteropServices;\n");
    s.push_str("using System.Diagnostics;\n\n");

    s.push_str("public class TaCodegenServe {\n");
    s.push_str("    const int MAX_ARRAY_SIZE = 200000;\n");
    s.push_str("    static double[] refOpen = new double[MAX_ARRAY_SIZE];\n");
    s.push_str("    static double[] refHigh = new double[MAX_ARRAY_SIZE];\n");
    s.push_str("    static double[] refLow = new double[MAX_ARRAY_SIZE];\n");
    s.push_str("    static double[] refClose = new double[MAX_ARRAY_SIZE];\n");
    s.push_str("    static double[] refVolume = new double[MAX_ARRAY_SIZE];\n");
    s.push_str("    static double[] refOI = new double[MAX_ARRAY_SIZE];\n");
    s.push_str("    static int refN = 0;\n\n");

    // Cross-platform high-resolution nanosecond timer via Stopwatch
    // Split into whole-seconds + fractional to avoid long overflow
    s.push_str("    static long GetNanoTime() {\n");
    s.push_str("        long ts = Stopwatch.GetTimestamp();\n");
    s.push_str("        long freq = Stopwatch.Frequency;\n");
    s.push_str("        return (ts / freq) * 1000000000L + (ts % freq) * 1000000000L / freq;\n");
    s.push_str("    }\n\n");

    // P/Invoke for TA_Initialize
    s.push_str("    [DllImport(\"ta_codegen_funcs\", EntryPoint = \"TA_Initialize\")]\n");
    s.push_str("    static extern int TA_Initialize();\n\n");

    // P/Invoke for unstable period setter
    s.push_str("    [DllImport(\"ta_codegen_funcs\", EntryPoint = \"TA_SetUnstablePeriod\")]\n");
    s.push_str("    static extern void TA_SetUnstablePeriod(int id, int period);\n\n");

    // P/Invoke for compatibility setter (exported by ta_utility.c in the shared lib)
    s.push_str("    [DllImport(\"ta_codegen_funcs\", EntryPoint = \"TA_SetCompatibility\")]\n");
    s.push_str("    static extern int TA_SetCompatibility(int value);\n\n");

    // P/Invoke declarations
    for func in funcs {
        let func_upper = &func.name;

        // Expand Price inputs into individual component arrays.
        let input_names = expand_input_names(&func.inputs);

        s.push_str(&format!(
            "    [DllImport(\"ta_codegen_funcs\", EntryPoint = \"TA_{func_upper}\")]\n"
        ));
        s.push_str(&format!("    static extern int TA_{func_upper}(\n"));
        s.push_str("        int startIdx, int endIdx,\n");

        for name in &input_names {
            s.push_str(&format!("        double[] {name},\n"));
        }

        for opt in &func.optional_inputs {
            let cs_type = if opt.param_type == ParamType::Real {
                "double"
            } else {
                "int"
            };
            s.push_str(&format!("        {cs_type} {},\n", opt.name));
        }

        s.push_str("        out int outBegIdx, out int outNBElement");
        for (k, out) in func.outputs.iter().enumerate() {
            let arr_name = format!("outArr{k}");
            let cs_arr_type = if out.param_type == ParamType::Integer { "int[]" } else { "double[]" };
            s.push_str(&format!(",\n        {cs_arr_type} {arr_name}"));
        }
        s.push_str(");\n\n");

        // Unguarded P/Invoke declaration
        s.push_str(&format!(
            "    [DllImport(\"ta_codegen_funcs\", EntryPoint = \"TA_{func_upper}_Unguarded\")]\n"
        ));
        s.push_str(&format!("    static extern int TA_{func_upper}_Unguarded(\n"));
        s.push_str("        int startIdx, int endIdx,\n");
        for name in &input_names {
            s.push_str(&format!("        double[] {name},\n"));
        }
        for opt in &func.optional_inputs {
            let cs_type = if opt.param_type == ParamType::Real {
                "double"
            } else {
                "int"
            };
            s.push_str(&format!("        {cs_type} {},\n", opt.name));
        }
        s.push_str("        out int outBegIdx, out int outNBElement");
        for (k, out) in func.outputs.iter().enumerate() {
            let arr_name = format!("outArr{k}");
            let cs_arr_type = if out.param_type == ParamType::Integer { "int[]" } else { "double[]" };
            s.push_str(&format!(",\n        {cs_arr_type} {arr_name}"));
        }
        s.push_str(");\n\n");
    }

    // Dispatch method
    s.push_str("    static string HandleRequest(string json) {\n");
    s.push_str("        try {\n");
    s.push_str("            using var doc = JsonDocument.Parse(json);\n");
    s.push_str("            var root = doc.RootElement;\n");
    s.push_str("            string method = root.GetProperty(\"method\").GetString()!;\n");
    s.push_str("            var p = root.GetProperty(\"params\");\n\n");

    // Handle load_data before extracting startIdx/endIdx (which load_data doesn't have)
    s.push_str("            if (method == \"load_data\") {\n");
    s.push_str("                double[] tmpOpen = GetDoubleArray(p, \"open\");\n");
    s.push_str("                refN = tmpOpen.Length;\n");
    s.push_str("                Array.Copy(tmpOpen, refOpen, refN);\n");
    s.push_str("                Array.Copy(GetDoubleArray(p, \"high\"), refHigh, refN);\n");
    s.push_str("                Array.Copy(GetDoubleArray(p, \"low\"), refLow, refN);\n");
    s.push_str("                Array.Copy(GetDoubleArray(p, \"close\"), refClose, refN);\n");
    s.push_str("                Array.Copy(GetDoubleArray(p, \"volume\"), refVolume, refN);\n");
    s.push_str("                Array.Copy(GetDoubleArray(p, \"openInterest\"), refOI, refN);\n");
    s.push_str("                return $\"{{\\\"status\\\":\\\"ok\\\",\\\"n\\\":{refN}}}\";\n");
    s.push_str("            }\n\n");

    // Tolerant extraction: state methods (set_unstable_period,
    // set_compatibility, list_functions) have params without startIdx/endIdx
    // and are dispatched further down.
    s.push_str("            int startIdx = p.TryGetProperty(\"startIdx\", out var _startIdxEl) ? _startIdxEl.GetInt32() : 0;\n");
    s.push_str("            int endIdx = p.TryGetProperty(\"endIdx\", out var _endIdxEl) ? _endIdxEl.GetInt32() : 0;\n");
    s.push_str("            int n = endIdx - startIdx + 1;\n\n");

    for (i, func) in funcs.iter().enumerate() {
        let method_name = format!("TA_{}", func.name);
        let cond = if i == 0 { "if" } else { "else if" };

        // Expand Price inputs into individual component arrays.
        let input_names = expand_input_names(&func.inputs);

        s.push_str(&format!(
            "            {cond} (method == \"{method_name}\") {{\n"
        ));

        // Check use_preloaded and iters
        s.push_str("                int use_preloaded = p.TryGetProperty(\"use_preloaded\", out var _upre) ? _upre.GetInt32() : 0;\n");
        s.push_str("                int bench_iters = p.TryGetProperty(\"iters\", out var _iters) ? _iters.GetInt32() : 1;\n");
        s.push_str("                if (bench_iters < 1) bench_iters = 1;\n");

        // Declare input arrays with default initialization (satisfies C# definite assignment)
        for name in &input_names {
            s.push_str(&format!(
                "                double[] {name} = Array.Empty<double>();\n"
            ));
        }

        // Populate from preloaded or JSON
        s.push_str("                if (use_preloaded != 0 && refN > 0) {\n");
        for (j, name) in input_names.iter().enumerate() {
            let ref_src = if let Some(r) = price_input_to_ref(name) {
                r.to_string()
            } else if j == 0 {
                "refClose".to_string()
            } else {
                "refHigh".to_string()
            };
            s.push_str(&format!(
                "                    {name} = new double[refN]; Array.Copy({ref_src}, {name}, refN);\n"
            ));
        }
        s.push_str("                } else {\n");
        for name in &input_names {
            s.push_str(&format!(
                "                    {name} = GetDoubleArray(p, \"{name}\");\n"
            ));
        }
        s.push_str("                }\n");

        // Extract optional params
        for opt in &func.optional_inputs {
            if opt.param_type == ParamType::Real {
                let default = opt.default.unwrap_or(0.0);
                s.push_str(&format!(
                    "                double {} = p.TryGetProperty(\"{}\", out var _{0}Val) ? _{0}Val.GetDouble() : {};\n",
                    opt.name, opt.name, default
                ));
            } else {
                #[allow(clippy::cast_possible_truncation)]
                let default = opt.default.unwrap_or(0.0) as i64;
                s.push_str(&format!(
                    "                int {} = p.TryGetProperty(\"{}\", out var _{0}Val) ? _{0}Val.GetInt32() : {};\n",
                    opt.name, opt.name, default
                ));
            }
        }

        // Apply unstable period if provided
        if let Some(id) = func_unst_id(&func.name) {
            s.push_str("                int unstablePeriod = p.TryGetProperty(\"unstablePeriod\", out var _upVal) ? _upVal.GetInt32() : 0;\n");
            s.push_str(&format!(
                "                TA_SetUnstablePeriod({id}, unstablePeriod);\n"
            ));
        }

        // Allocate output arrays — typed correctly (double[] or int[]) per output
        let outputs = &func.outputs;
        for (k, out) in outputs.iter().enumerate() {
            let arr_name = format!("outArr{k}");
            if out.param_type == ParamType::Integer {
                s.push_str(&format!(
                    "                int[] {arr_name} = new int[n];\n"
                ));
            } else {
                s.push_str(&format!(
                    "                double[] {arr_name} = new double[n];\n"
                ));
            }
        }

        // Benchmark iteration loop with timing
        s.push_str("                int rc = 0;\n");
        s.push_str("                int outBegIdx = 0, outNBElement = 0;\n");
        s.push_str("                long _t0 = GetNanoTime();\n");
        s.push_str("                for (int _bi = 0; _bi < bench_iters; _bi++) {\n");

        // Call
        s.push_str(&format!(
            "                rc = TA_{}(startIdx, endIdx, ",
            func.name
        ));
        for name in &input_names {
            s.push_str(&format!("{name}, "));
        }
        for opt in &func.optional_inputs {
            s.push_str(&format!("{}, ", opt.name));
        }
        s.push_str("out outBegIdx, out outNBElement");
        for k in 0..outputs.len() {
            s.push_str(&format!(", outArr{k}"));
        }
        s.push_str(");\n");
        s.push_str("                }\n"); // end bench_iters loop
        s.push_str("                long elapsedNs = (GetNanoTime() - _t0) / bench_iters;\n");

        // Unguarded timing loop
        s.push_str("                long _t0u = GetNanoTime();\n");
        s.push_str("                for (int _biu = 0; _biu < bench_iters; _biu++) {\n");
        s.push_str(&format!(
            "                rc = TA_{}_Unguarded(startIdx, endIdx, ",
            func.name
        ));
        for name in &input_names {
            s.push_str(&format!("{name}, "));
        }
        for opt in &func.optional_inputs {
            s.push_str(&format!("{}, ", opt.name));
        }
        s.push_str("out outBegIdx, out outNBElement");
        for k in 0..outputs.len() {
            s.push_str(&format!(", outArr{k}"));
        }
        s.push_str(");\n");
        s.push_str("                }\n"); // end unguarded bench loop
        s.push_str("                long elapsedNsUng = (GetNanoTime() - _t0u) / bench_iters;\n");

        // Build response — correct key names and serialisers per output type
        s.push_str("                var sb = new System.Text.StringBuilder();\n");
        s.push_str("                sb.Append($\"{{\\\"retCode\\\":{rc},\\\"outBegIdx\\\":{outBegIdx},\\\"outNBElement\\\":{outNBElement}\");\n");
        for (k, out) in outputs.iter().enumerate() {
            let arr_name = format!("outArr{k}");
            let key = output_json_key(outputs, k);
            if out.param_type == ParamType::Integer {
                s.push_str(&format!(
                    "                sb.Append($\",\\\"{key}\\\":\"); sb.Append(FormatIntArray({arr_name}, outNBElement));\n"
                ));
            } else {
                s.push_str(&format!(
                    "                sb.Append($\",\\\"{key}\\\":\"); sb.Append(FormatArray({arr_name}, outNBElement));\n"
                ));
            }
        }
        s.push_str("                sb.Append($\",\\\"timing_ns\\\":{elapsedNs}\");\n");
        s.push_str("                sb.Append($\",\\\"timing_ns_unguarded\\\":{elapsedNsUng}\");\n");
        s.push_str("                sb.Append(\"}\");\n");
        s.push_str("                return sb.ToString();\n");
        s.push_str("            }\n");
    }

    // list_functions method — returns {"functions":["TA_SMA","TA_RSI",...]}
    s.push_str("            else if (method == \"list_functions\") {\n");
    s.push_str("                var sb = new System.Text.StringBuilder(\"{\\\"functions\\\":[\");\n");
    for (i, func) in funcs.iter().enumerate() {
        if i > 0 {
            s.push_str("                sb.Append(\",\");\n");
        }
        s.push_str(&format!(
            "                sb.Append(\"\\\"TA_{}\\\"\");\n",
            func.name
        ));
    }
    s.push_str("                sb.Append(\"]}\");\n");
    s.push_str("                return sb.ToString();\n");
    s.push_str("            }\n");

    // set_unstable_period method — {"method":"set_unstable_period","params":{"id":21,"period":10}}
    s.push_str("            else if (method == \"set_unstable_period\") {\n");
    s.push_str("                int id = p.GetProperty(\"id\").GetInt32();\n");
    s.push_str("                int period = p.GetProperty(\"period\").GetInt32();\n");
    s.push_str("                TA_SetUnstablePeriod(id, period);\n");
    s.push_str("                return \"{\\\"status\\\":\\\"ok\\\"}\";\n");
    s.push_str("            }\n");

    // set_compatibility method
    s.push_str("            else if (method == \"set_compatibility\") {\n");
    s.push_str("                int mode = p.GetProperty(\"mode\").GetInt32();\n");
    s.push_str("                TA_SetCompatibility(mode);\n");
    s.push_str("                return \"{\\\"status\\\":\\\"ok\\\"}\";\n");
    s.push_str("            }\n");

    s.push_str("            else {\n");
    s.push_str("                return $\"{{\\\"error\\\":\\\"Unknown method: {method}\\\"}}\";\n");
    s.push_str("            }\n");
    s.push_str("        } catch (Exception ex) {\n");
    s.push_str("            return $\"{{\\\"error\\\":\\\"{ex.Message.Replace(\"\\\"\", \"'\")}\\\"}}\";\n");
    s.push_str("        }\n");
    s.push_str("    }\n\n");

    // Helper: extract double array from JSON
    s.push_str("    static double[] GetDoubleArray(JsonElement p, string name) {\n");
    s.push_str("        var arr = p.GetProperty(name);\n");
    s.push_str("        double[] result = new double[arr.GetArrayLength()];\n");
    s.push_str("        for (int i = 0; i < result.Length; i++)\n");
    s.push_str("            result[i] = arr[i].GetDouble();\n");
    s.push_str("        return result;\n");
    s.push_str("    }\n\n");

    // Helper: format a double array as a JSON array string
    s.push_str("    static string FormatArray(double[] arr, int count) {\n");
    s.push_str("        var parts = new string[count];\n");
    s.push_str("        for (int i = 0; i < count; i++)\n");
    s.push_str("            parts[i] = arr[i].ToString(\"G15\");\n");
    s.push_str("        return \"[\" + string.Join(\",\", parts) + \"]\";\n");
    s.push_str("    }\n\n");

    // Helper: format an int array as a JSON array string
    s.push_str("    static string FormatIntArray(int[] arr, int count) {\n");
    s.push_str("        var parts = new string[count];\n");
    s.push_str("        for (int i = 0; i < count; i++)\n");
    s.push_str("            parts[i] = arr[i].ToString();\n");
    s.push_str("        return \"[\" + string.Join(\",\", parts) + \"]\";\n");
    s.push_str("    }\n\n");

    // Main
    s.push_str("    static void Main(string[] args) {\n");
    s.push_str("        TA_Initialize();\n");
    s.push_str("        string? line;\n");
    s.push_str("        while ((line = Console.ReadLine()) != null) {\n");
    s.push_str("            if (string.IsNullOrWhiteSpace(line)) continue;\n");
    s.push_str("            Console.WriteLine(HandleRequest(line));\n");
    s.push_str("            Console.Out.Flush();\n");
    s.push_str("        }\n");
    s.push_str("    }\n");
    s.push_str("}\n");

    s
}

/// Format a default f64 value for Rust source code.
/// Ensures integers get a `.0` suffix so they're valid f64 literals.
fn format_default_f64(v: f64) -> String {
    if (v - v.floor()).abs() < f64::EPSILON && v.abs() < 1e15 && !v.is_nan() && !v.is_infinite() {
        format!("{v:.1}")
    } else {
        format!("{v}")
    }
}

/// Generate a Rust JSON-RPC server source file.
///
/// The generated file is a standalone binary that imports from the `ta_lib` crate.
/// It reads JSON-RPC requests from stdin, dispatches to the generated TA function
/// implementations, and writes JSON responses to stdout.
#[allow(clippy::too_many_lines)]
#[allow(clippy::implicit_hasher)]
pub fn generate_rust_server(funcs: &[FuncDef], enums: &HashMap<String, EnumDef>) -> String {
    let mut s = String::new();

    // File-level attributes
    s.push_str("#![forbid(unsafe_code)]\n");
    s.push_str("#![allow(non_snake_case, unused_variables, dead_code, clippy::all)]\n\n");

    // Imports
    s.push_str("use serde_json::{self, Value};\n");
    s.push_str("use std::io::{self, BufRead, Write};\n");
    s.push_str("use std::time::Instant;\n");
    s.push_str("use ta_lib::{Core, RetCode, FuncUnstId, Compatibility};\n");
    s.push_str("use ta_lib::abstract_api::{self, InputType, OutputType, OptDomain};\n\n");

    // Pre-loaded reference data struct
    s.push_str("const MAX_ARRAY_SIZE: usize = 200000;\n\n");
    s.push_str("struct RefData {\n");
    s.push_str("    open: Vec<f64>,\n");
    s.push_str("    high: Vec<f64>,\n");
    s.push_str("    low: Vec<f64>,\n");
    s.push_str("    close: Vec<f64>,\n");
    s.push_str("    volume: Vec<f64>,\n");
    s.push_str("    oi: Vec<f64>,\n");
    s.push_str("    n: usize,\n");
    s.push_str("}\n\n");
    s.push_str("impl RefData {\n");
    s.push_str("    fn new() -> Self {\n");
    s.push_str("        RefData {\n");
    s.push_str("            open: vec![0.0; MAX_ARRAY_SIZE],\n");
    s.push_str("            high: vec![0.0; MAX_ARRAY_SIZE],\n");
    s.push_str("            low: vec![0.0; MAX_ARRAY_SIZE],\n");
    s.push_str("            close: vec![0.0; MAX_ARRAY_SIZE],\n");
    s.push_str("            volume: vec![0.0; MAX_ARRAY_SIZE],\n");
    s.push_str("            oi: vec![0.0; MAX_ARRAY_SIZE],\n");
    s.push_str("            n: 0,\n");
    s.push_str("        }\n");
    s.push_str("    }\n");
    s.push_str("}\n\n");

    // Helper: parse f64 array from JSON value
    s.push_str("fn parse_f64_array(val: &Value) -> Vec<f64> {\n");
    s.push_str("    match val.as_array() {\n");
    s.push_str("        Some(arr) => arr.iter().filter_map(|v| v.as_f64()).collect(),\n");
    s.push_str("        None => Vec::new(),\n");
    s.push_str("    }\n");
    s.push_str("}\n\n");

    // Helper: RetCode to integer
    s.push_str("fn retcode_to_int(rc: RetCode) -> i32 {\n");
    s.push_str("    match rc {\n");
    s.push_str("        RetCode::Success => 0,\n");
    s.push_str("        RetCode::BadParam => 2,\n");
    s.push_str("        RetCode::AllocErr => 3,\n");
    s.push_str("        RetCode::InternalError => 5000,\n");
    s.push_str("        RetCode::OutOfRangeStartIndex => 12,\n");
    s.push_str("        RetCode::OutOfRangeEndIndex => 13,\n");
    s.push_str("    }\n");
    s.push_str("}\n\n");

    // Helper: serialize an f64 slice as a JSON-ish array. Finite values use
    // serde_json's (ryu) formatting; non-finite values emit bare `nan`/`-nan`/
    // `inf`/`-inf` tokens to match the C server's `%.15g` output, which the test
    // harness's strtod-based parser understands (serde_json's `null` would not
    // advance the parser, ballooning the parsed element count).
    s.push_str("fn json_f64_array(data: &[f64]) -> String {\n");
    s.push_str("    let mut s = String::with_capacity(data.len() * 8 + 2);\n");
    s.push_str("    s.push('[');\n");
    s.push_str("    for (i, &v) in data.iter().enumerate() {\n");
    s.push_str("        if i > 0 { s.push(','); }\n");
    s.push_str("        match serde_json::Number::from_f64(v) {\n");
    s.push_str("            Some(n) => s.push_str(&n.to_string()),\n");
    s.push_str("            None => s.push_str(\n");
    s.push_str("                if v.is_nan() { if v.is_sign_negative() { \"-nan\" } else { \"nan\" } }\n");
    s.push_str("                else if v < 0.0 { \"-inf\" } else { \"inf\" }),\n");
    s.push_str("        }\n");
    s.push_str("    }\n");
    s.push_str("    s.push(']');\n");
    s.push_str("    s\n");
    s.push_str("}\n\n");

    // Helper: serialize an i32 slice as a JSON array.
    s.push_str("fn json_i32_array(data: &[i32]) -> String {\n");
    s.push_str("    let mut s = String::with_capacity(data.len() * 4 + 2);\n");
    s.push_str("    s.push('[');\n");
    s.push_str("    for (i, &v) in data.iter().enumerate() {\n");
    s.push_str("        if i > 0 { s.push(','); }\n");
    s.push_str("        s.push_str(&v.to_string());\n");
    s.push_str("    }\n");
    s.push_str("    s.push(']');\n");
    s.push_str("    s\n");
    s.push_str("}\n\n");

    // Helper: FuncUnstId from integer
    s.push_str("fn func_unst_id_from_int(id: usize) -> Option<FuncUnstId> {\n");
    s.push_str("    match id {\n");
    // Generated from enums.yaml (source of truth), in ordinal order.
    for (i, name) in func_unst_pascal_names(enums).iter().enumerate() {
        s.push_str(&format!("        {i} => Some(FuncUnstId::{name}),\n"));
    }
    s.push_str("        _ => None,\n");
    s.push_str("    }\n");
    s.push_str("}\n\n");

    // handle_request function
    s.push_str("fn handle_request(core: &mut Core, ref_data: &mut RefData, line: &str) -> String {\n");
    s.push_str("    let req: Value = match serde_json::from_str(line) {\n");
    s.push_str("        Ok(v) => v,\n");
    s.push_str(
        "        Err(e) => return format!(\"{{\\\"error\\\":\\\"Parse error: {}\\\"}}\", e),\n",
    );
    s.push_str("    };\n");
    s.push_str("    let method = match req[\"method\"].as_str() {\n");
    s.push_str("        Some(m) => m,\n");
    s.push_str(
        "        None => return \"{\\\"error\\\":\\\"Missing method field\\\"}\".to_string(),\n",
    );
    s.push_str("    };\n");
    s.push_str("    let params = &req[\"params\"];\n\n");
    s.push_str("    dispatch(core, ref_data, method, params)\n");
    s.push_str("}\n\n");

    // dispatch — the method router. Split out from handle_request so the
    // abstract_call RPC can re-enter it (reroute funcName -> \"TA_<funcName>\"),
    // mirroring C's handle_abstract_call which dispatches generically.
    s.push_str("fn dispatch(core: &mut Core, ref_data: &mut RefData, method: &str, params: &Value) -> String {\n");
    s.push_str("    match method {\n");

    // load_data handler
    s.push_str("        \"load_data\" => {\n");
    s.push_str("            let open = parse_f64_array(&params[\"open\"]);\n");
    s.push_str("            ref_data.n = open.len().min(MAX_ARRAY_SIZE);\n");
    s.push_str("            ref_data.open[..ref_data.n].copy_from_slice(&open[..ref_data.n]);\n");
    s.push_str("            let high = parse_f64_array(&params[\"high\"]);\n");
    s.push_str("            ref_data.high[..ref_data.n].copy_from_slice(&high[..ref_data.n]);\n");
    s.push_str("            let low = parse_f64_array(&params[\"low\"]);\n");
    s.push_str("            ref_data.low[..ref_data.n].copy_from_slice(&low[..ref_data.n]);\n");
    s.push_str("            let close = parse_f64_array(&params[\"close\"]);\n");
    s.push_str("            ref_data.close[..ref_data.n].copy_from_slice(&close[..ref_data.n]);\n");
    s.push_str("            let volume = parse_f64_array(&params[\"volume\"]);\n");
    s.push_str("            ref_data.volume[..ref_data.n].copy_from_slice(&volume[..ref_data.n]);\n");
    s.push_str("            let oi = parse_f64_array(&params[\"openInterest\"]);\n");
    s.push_str("            ref_data.oi[..ref_data.n].copy_from_slice(&oi[..ref_data.n]);\n");
    s.push_str("            format!(\"{{\\\"status\\\":\\\"ok\\\",\\\"n\\\":{}}}\", ref_data.n)\n");
    s.push_str("        }\n");

    // Per-function dispatch
    for func in funcs {
        let method_name = format!("TA_{}", func.name);
        let fn_name = func.name.to_lowercase();

        s.push_str(&format!("        \"{method_name}\" => {{\n"));

        // Parse startIdx, endIdx
        s.push_str(
            "            let startIdx = params[\"startIdx\"].as_u64().unwrap_or(0) as usize;\n",
        );
        s.push_str(
            "            let endIdx = params[\"endIdx\"].as_u64().unwrap_or(0) as usize;\n",
        );

        // Parse use_preloaded and iters
        let input_names = expand_input_names(&func.inputs);

        s.push_str("            let use_preloaded = params[\"use_preloaded\"].as_i64().unwrap_or(0);\n");
        s.push_str("            let bench_iters = std::cmp::max(1, params[\"iters\"].as_i64().unwrap_or(1)) as u64;\n");

        // Declare input arrays: Vec for JSON fallback, &[f64] for actual reference.
        // Preloaded path borrows from ref_data (zero-copy), JSON path owns a Vec.
        for name in &input_names {
            s.push_str(&format!(
                "            let mut _json_{name}: Vec<f64> = Vec::new();\n"
            ));
        }
        for name in &input_names {
            s.push_str(&format!(
                "            let {name}: &[f64];\n"
            ));
        }

        // Populate from preloaded or JSON
        s.push_str("            if use_preloaded != 0 && ref_data.n > 0 {\n");
        for (j, name) in input_names.iter().enumerate() {
            let ref_field = if let Some(f) = price_input_to_rust_ref(name) {
                f.to_string()
            } else if j == 0 {
                "close".to_string()
            } else {
                "high".to_string()
            };
            s.push_str(&format!(
                "                {name} = &ref_data.{ref_field}[..ref_data.n];\n"
            ));
        }
        s.push_str("            } else {\n");
        for name in &input_names {
            s.push_str(&format!(
                "                _json_{name} = parse_f64_array(&params[\"{name}\"]);\n"
            ));
            s.push_str(&format!(
                "                {name} = &_json_{name};\n"
            ));
        }
        s.push_str("            }\n");

        // Parse optional params
        for opt in &func.optional_inputs {
            let default_val = opt.default.unwrap_or(0.0);
            if opt.param_type == ParamType::Real {
                s.push_str(&format!(
                    "            let {} = params[\"{}\"].as_f64().unwrap_or({}) as f64;\n",
                    opt.name,
                    opt.name,
                    format_default_f64(default_val)
                ));
            } else {
                #[allow(clippy::cast_possible_truncation)]
                let default_i = default_val as i64;
                s.push_str(&format!(
                    "            let {} = params[\"{}\"].as_i64().unwrap_or({}) as i32;\n",
                    opt.name, opt.name, default_i
                ));
            }
        }

        // Apply unstable period if provided
        if let Some(id) = func_unst_id(&func.name) {
            s.push_str("            if let Some(period) = params[\"unstablePeriod\"].as_i64() {\n");
            s.push_str(&format!(
                "                core.unstable_period[{id}] = period as i32;\n"
            ));
            s.push_str("            }\n");
        }

        // Allocate output buffers
        // Size: endIdx - startIdx + 1 is a reasonable upper bound
        s.push_str("            let out_size = if endIdx >= startIdx { endIdx - startIdx + 1 } else { 0 };\n");
        let outputs = &func.outputs;
        let mut real_idx = 0usize;
        let mut int_idx = 0usize;
        for out in outputs {
            if out.param_type == ParamType::Integer {
                s.push_str(&format!(
                    "            let mut outIntBuf{int_idx}: Vec<i32> = vec![0i32; out_size];\n"
                ));
                int_idx += 1;
            } else {
                s.push_str(&format!(
                    "            let mut outBuf{real_idx}: Vec<f64> = vec![0.0f64; out_size];\n"
                ));
                real_idx += 1;
            }
        }

        // Declare output scalars
        s.push_str("            let mut outBegIdx: usize = 0;\n");
        s.push_str("            let mut outNBElement: usize = 0;\n");
        s.push_str("            let mut rc = RetCode::Success;\n");

        // Guarded timing loop
        s.push_str("            let start_time = Instant::now();\n");
        s.push_str("            for _bi in 0..bench_iters {\n");
        s.push_str(&format!(
            "            rc = core.{fn_name}(\n"
        ));
        s.push_str("                startIdx, endIdx,\n");
        for name in &input_names {
            s.push_str(&format!("                &{name},\n"));
        }
        for opt in &func.optional_inputs {
            s.push_str(&format!("                {},\n", opt.name));
        }
        s.push_str("                &mut outBegIdx, &mut outNBElement");
        real_idx = 0;
        int_idx = 0;
        for out in outputs {
            if out.param_type == ParamType::Integer {
                s.push_str(&format!(", &mut outIntBuf{int_idx}"));
                int_idx += 1;
            } else {
                s.push_str(&format!(", &mut outBuf{real_idx}"));
                real_idx += 1;
            }
        }
        s.push_str(",\n");
        s.push_str("            );\n");
        s.push_str("            }\n"); // end guarded bench loop
        s.push_str("            let elapsed_ns = start_time.elapsed().as_nanos() as u64 / bench_iters as u64;\n");

        // Unguarded timing loop (same signature as guarded, no extra params)
        s.push_str("            let start_time_ung = Instant::now();\n");
        s.push_str("            for _biu in 0..bench_iters {\n");
        s.push_str(&format!(
            "            rc = core.{fn_name}_unguarded(\n"
        ));
        s.push_str("                startIdx, endIdx,\n");
        for name in &input_names {
            s.push_str(&format!("                &{name},\n"));
        }
        for opt in &func.optional_inputs {
            s.push_str(&format!("                {},\n", opt.name));
        }
        s.push_str("                &mut outBegIdx, &mut outNBElement");
        real_idx = 0;
        int_idx = 0;
        for out in outputs {
            if out.param_type == ParamType::Integer {
                s.push_str(&format!(", &mut outIntBuf{int_idx}"));
                int_idx += 1;
            } else {
                s.push_str(&format!(", &mut outBuf{real_idx}"));
                real_idx += 1;
            }
        }
        s.push_str(",\n");
        s.push_str("            );\n");
        s.push_str("            }\n"); // end unguarded bench loop
        s.push_str("            let elapsed_ns_ung = start_time_ung.elapsed().as_nanos() as u64 / bench_iters as u64;\n");

        // Lookback (mirrors C's TA_<NAME>_Lookback). Emitted on every response so the
        // abstract_call reroute returns the `lookback` field the C ta_abstract path
        // exposes; harmless extra field for the regular per-function path. Computed
        // after the unstable-period assignment above so it reflects that state.
        s.push_str(&format!("            let lookback = core.{fn_name}_lookback("));
        let lb_args: Vec<String> = func
            .optional_inputs
            .iter()
            .map(|o| o.name.clone())
            .collect();
        s.push_str(&lb_args.join(", "));
        s.push_str(");\n");

        // Build the response string manually (not via serde_json) so non-finite
        // f64 outputs serialize as `nan`/`-nan`/`inf`/`-inf` — matching the C
        // server's `%.15g` — rather than serde_json's `null` (which the test
        // harness's strtod-based array parser cannot advance past, ballooning the
        // element count). Finite values use json_f64_array (serde_json formatting).
        s.push_str("            let mut resp = format!(\"{{\\\"retCode\\\":{},\\\"outBegIdx\\\":{},\\\"outNBElement\\\":{},\\\"lookback\\\":{},\\\"timing_ns\\\":{},\\\"timing_ns_unguarded\\\":{}\", retcode_to_int(rc), outBegIdx, outNBElement, lookback, elapsed_ns, elapsed_ns_ung);\n");

        // Add output arrays to response
        real_idx = 0;
        int_idx = 0;
        for (k, out) in outputs.iter().enumerate() {
            let key = output_json_key(outputs, k);
            if out.param_type == ParamType::Integer {
                s.push_str(&format!(
                    "            resp.push_str(\",\\\"{key}\\\":\"); resp.push_str(&json_i32_array(&outIntBuf{int_idx}[..outNBElement]));\n"
                ));
                int_idx += 1;
            } else {
                s.push_str(&format!(
                    "            resp.push_str(\",\\\"{key}\\\":\"); resp.push_str(&json_f64_array(&outBuf{real_idx}[..outNBElement]));\n"
                ));
                real_idx += 1;
            }
        }

        s.push_str("            resp.push('}');\n");
        s.push_str("            resp\n");
        s.push_str("        }\n");
    }

    // list_functions method
    s.push_str("        \"list_functions\" => {\n");
    s.push_str("            let funcs: Vec<&str> = vec![\n");
    for func in funcs {
        s.push_str(&format!("                \"TA_{}\",\n", func.name));
    }
    s.push_str("            ];\n");
    s.push_str(
        "            serde_json::json!({ \"functions\": funcs }).to_string()\n",
    );
    s.push_str("        }\n");

    // set_unstable_period method
    s.push_str("        \"set_unstable_period\" => {\n");
    s.push_str(
        "            let id = params[\"id\"].as_u64().unwrap_or(99) as usize;\n",
    );
    s.push_str(
        "            let period = params[\"period\"].as_i64().unwrap_or(0) as i32;\n",
    );
    // id == FuncUnstAll is the "set all" sentinel (matches C TA_SetUnstablePeriod).
    s.push_str(
        "            if id == FuncUnstId::FuncUnstAll as usize {\n",
    );
    s.push_str("                for i in 0..(FuncUnstId::FuncUnstAll as usize) { core.unstable_period[i] = period; }\n");
    s.push_str(
        "                \"{\\\"status\\\":\\\"ok\\\"}\".to_string()\n",
    );
    s.push_str("            } else if id < FuncUnstId::FuncUnstAll as usize {\n");
    s.push_str("                core.unstable_period[id] = period;\n");
    s.push_str(
        "                \"{\\\"status\\\":\\\"ok\\\"}\".to_string()\n",
    );
    s.push_str("            } else {\n");
    s.push_str(
        "                \"{\\\"error\\\":\\\"Invalid unstable period id\\\"}\".to_string()\n",
    );
    s.push_str("            }\n");
    s.push_str("        }\n");

    // set_compatibility method
    s.push_str("        \"set_compatibility\" => {\n");
    s.push_str(
        "            let mode = params[\"mode\"].as_u64().unwrap_or(0);\n",
    );
    s.push_str("            core.compatibility = match mode {\n");
    s.push_str("                1 => Compatibility::Metastock,\n");
    s.push_str("                _ => Compatibility::Default,\n");
    s.push_str("            };\n");
    s.push_str(
        "            \"{\\\"status\\\":\\\"ok\\\"}\".to_string()\n",
    );
    s.push_str("        }\n");

    // Abstract/introspection metadata handlers (mirror ta_abstract_serve.c),
    // backed by the generated abstract_api registry. Used by ta_regtest to lock
    // Rust introspection metadata parity against the C reference.
    s.push_str(RUST_ABSTRACT_METADATA_HANDLERS);

    // Abstract dynamic-dispatch handlers (abstract_call, abstract_get_lookback,
    // abstract_for_each_func) + TA_FunctionDescriptionXML. Completes the Rust mirror
    // of C's ta_abstract serve path so the full test_abstract() drives the Rust
    // server (numeric output comparison, not just metadata). abstract_call reroutes
    // through dispatch(); abstract_get_lookback uses the generated abstract_lookback().
    s.push_str(RUST_ABSTRACT_DYNAMIC_HANDLERS);

    // Unknown method
    s.push_str("        _ => {\n");
    s.push_str(
        "            format!(\"{{\\\"error\\\":\\\"Unknown method: {}\\\"}}\", method)\n",
    );
    s.push_str("        }\n");
    s.push_str("    }\n");
    s.push_str("}\n\n");

    // abstract_lookback — generic lookback dispatcher used by the abstract_get_lookback
    // RPC (funcName + opt params -> Core::<fn>_lookback). Opt-param parsing mirrors the
    // per-function arm exactly (same defaults/types), so the lookback matches c-ref's
    // TA_GetLookback for the same parameters. Returns None for an unknown funcName.
    s.push_str("fn abstract_lookback(core: &Core, func_name: &str, params: &Value) -> Option<usize> {\n");
    s.push_str("    match func_name {\n");
    for func in funcs {
        let fn_name = func.name.to_lowercase();
        s.push_str(&format!("        \"{}\" => {{\n", func.name));
        for opt in &func.optional_inputs {
            let default_val = opt.default.unwrap_or(0.0);
            if opt.param_type == ParamType::Real {
                s.push_str(&format!(
                    "            let {} = params[\"{}\"].as_f64().unwrap_or({}) as f64;\n",
                    opt.name,
                    opt.name,
                    format_default_f64(default_val)
                ));
            } else {
                #[allow(clippy::cast_possible_truncation)]
                let default_i = default_val as i64;
                s.push_str(&format!(
                    "            let {} = params[\"{}\"].as_i64().unwrap_or({}) as i32;\n",
                    opt.name, opt.name, default_i
                ));
            }
        }
        let lb_args: Vec<String> = func
            .optional_inputs
            .iter()
            .map(|o| o.name.clone())
            .collect();
        s.push_str(&format!(
            "            Some(core.{}_lookback({}))\n",
            fn_name,
            lb_args.join(", ")
        ));
        s.push_str("        }\n");
    }
    s.push_str("        _ => None,\n");
    s.push_str("    }\n");
    s.push_str("}\n\n");

    // Main function
    s.push_str("fn main() {\n");
    s.push_str("    let mut core = Core::new();\n");
    s.push_str("    let mut ref_data = RefData::new();\n");
    s.push_str("    let stdin = io::stdin();\n");
    s.push_str("    let stdout = io::stdout();\n");
    s.push_str("    let mut stdout = stdout.lock();\n");
    s.push_str("    for line in stdin.lock().lines() {\n");
    s.push_str("        let line = match line {\n");
    s.push_str("            Ok(l) => l,\n");
    s.push_str("            Err(_) => break,\n");
    s.push_str("        };\n");
    s.push_str("        let line = line.trim();\n");
    s.push_str("        if line.is_empty() {\n");
    s.push_str("            continue;\n");
    s.push_str("        }\n");
    s.push_str("        let resp = handle_request(&mut core, &mut ref_data, line);\n");
    s.push_str("        writeln!(stdout, \"{}\", resp).ok();\n");
    s.push_str("        stdout.flush().ok();\n");
    s.push_str("    }\n");
    s.push_str("}\n");

    s
}

/// Rust server match arms for the abstract/introspection metadata RPCs. Mirrors
/// the C server's `ta_abstract_serve.c` response shapes exactly (so the same
/// `test_abstract.c` comparator drives Rust-vs-C), but is backed by the generated
/// `abstract_api` registry instead of C's `ta_abstract`.
const RUST_ABSTRACT_METADATA_HANDLERS: &str = r#"        "TA_GetFuncInfo" => {
            let name = params["funcName"].as_str().unwrap_or("");
            match abstract_api::get_func_handle(name) {
                Some(id) => {
                    let fi = id.info();
                    serde_json::json!({
                        "name": fi.name,
                        "group": fi.group.as_str(),
                        "hint": fi.hint,
                        "camelCaseName": fi.camel_case_name,
                        "flags": fi.flags.bits(),
                        "nbInput": fi.nb_input(),
                        "nbOptInput": fi.nb_opt_input(),
                        "nbOutput": fi.nb_output(),
                    }).to_string()
                }
                None => "{\"retCode\":2}".to_string(),
            }
        }
        "TA_GetInputParameterInfo" => {
            let name = params["funcName"].as_str().unwrap_or("");
            let idx = params["paramIndex"].as_u64().unwrap_or(0) as usize;
            match abstract_api::get_func_handle(name)
                .and_then(|id| abstract_api::get_input_parameter_info(id, idx)) {
                Some(ii) => {
                    let ty = match ii.kind {
                        InputType::Price => 0,
                        InputType::Real => 1,
                        InputType::Integer => 2,
                    };
                    serde_json::json!({
                        "type": ty,
                        "paramName": ii.param_name,
                        "flags": ii.flags.bits(),
                    }).to_string()
                }
                None => "{\"retCode\":2}".to_string(),
            }
        }
        "TA_GetOptInputParameterInfo" => {
            let name = params["funcName"].as_str().unwrap_or("");
            let idx = params["paramIndex"].as_u64().unwrap_or(0) as usize;
            match abstract_api::get_func_handle(name)
                .and_then(|id| abstract_api::get_opt_input_parameter_info(id, idx)) {
                Some(oi) => {
                    let (ty, default): (i32, f64) = match oi.domain {
                        OptDomain::RealRange { default, .. } => (0, default),
                        OptDomain::RealList { default, .. } => (1, default),
                        OptDomain::IntegerRange { default, .. } => (2, default as f64),
                        OptDomain::IntegerList { default, .. } => (3, default as f64),
                    };
                    let mut resp = serde_json::json!({
                        "type": ty,
                        "paramName": oi.param_name,
                        "flags": oi.flags.bits(),
                        "displayName": oi.display_name,
                        "defaultValue": default,
                    });
                    match oi.domain {
                        OptDomain::RealRange { min, max, precision, suggested, .. } => {
                            resp["min"] = serde_json::json!(min);
                            resp["max"] = serde_json::json!(max);
                            resp["precision"] = serde_json::json!(precision);
                            resp["suggestedStart"] = serde_json::json!(suggested.0);
                            resp["suggestedEnd"] = serde_json::json!(suggested.1);
                            resp["suggestedIncrement"] = serde_json::json!(suggested.2);
                        }
                        OptDomain::IntegerRange { min, max, suggested, .. } => {
                            resp["min"] = serde_json::json!(min);
                            resp["max"] = serde_json::json!(max);
                            resp["suggestedStart"] = serde_json::json!(suggested.0);
                            resp["suggestedEnd"] = serde_json::json!(suggested.1);
                            resp["suggestedIncrement"] = serde_json::json!(suggested.2);
                        }
                        OptDomain::IntegerList { values, .. } => {
                            let mut vl = String::new();
                            for (i, (v, label)) in values.iter().enumerate() {
                                if i > 0 { vl.push(';'); }
                                vl.push_str(&format!("{}={}", v, label));
                            }
                            resp["valueList"] = serde_json::json!(vl);
                        }
                        OptDomain::RealList { .. } => {}
                    }
                    resp.to_string()
                }
                None => "{\"retCode\":2}".to_string(),
            }
        }
        "TA_GetOutputParameterInfo" => {
            let name = params["funcName"].as_str().unwrap_or("");
            let idx = params["paramIndex"].as_u64().unwrap_or(0) as usize;
            match abstract_api::get_func_handle(name)
                .and_then(|id| abstract_api::get_output_parameter_info(id, idx)) {
                Some(oo) => {
                    let ty = match oo.kind {
                        OutputType::Real => 0,
                        OutputType::Integer => 1,
                    };
                    serde_json::json!({
                        "type": ty,
                        "paramName": oo.param_name,
                        "flags": oo.flags.bits(),
                    }).to_string()
                }
                None => "{\"retCode\":2}".to_string(),
            }
        }
"#;

/// Rust server match arms for the abstract dynamic-dispatch RPCs. Mirrors C's
/// `ta_abstract_serve.c` (`handle_abstract_call`, `handle_abstract_get_lookback`,
/// `handle_abstract_for_each_func`) plus `TA_FunctionDescriptionXML`, so the same
/// `test_abstract.c` comparator drives Rust-vs-C numeric parity.
///
///  * `abstract_call` re-enters `dispatch()` as `TA_<funcName>` — the request keys
///    (startIdx/endIdx/inReal.../optIn... ) are identical to the per-function RPC, and
///    that arm now emits `lookback`, so the response matches the C abstract_call shape.
///  * `abstract_get_lookback` uses the generated `abstract_lookback()` dispatcher.
///  * `abstract_for_each_func` enumerates via the `abstract_api` registry.
///  * `TA_FunctionDescriptionXML` returns the byte length + byte-sum checksum of the
///    embedded `ta_func_api.xml` (order-independent content check vs the C reference).
const RUST_ABSTRACT_DYNAMIC_HANDLERS: &str = r#"        "abstract_call" => {
            let fname = params["funcName"].as_str().unwrap_or("");
            if fname.is_empty() {
                return "{\"error\":\"Missing funcName\"}".to_string();
            }
            let rerouted = format!("TA_{}", fname);
            dispatch(core, ref_data, &rerouted, params)
        }
        "abstract_get_lookback" => {
            let fname = params["funcName"].as_str().unwrap_or("");
            match abstract_lookback(core, fname, params) {
                Some(lb) => format!("{{\"lookback\":{}}}", lb),
                None => format!("{{\"error\":\"Unknown function: {}\"}}", fname),
            }
        }
        "abstract_for_each_func" => {
            let mut arr: Vec<Value> = Vec::new();
            abstract_api::for_each_func(|fi| {
                arr.push(serde_json::json!({
                    "name": fi.name,
                    "group": fi.group.as_str(),
                    "nbInput": fi.nb_input(),
                    "nbOptInput": fi.nb_opt_input(),
                    "nbOutput": fi.nb_output(),
                }));
            });
            serde_json::json!({ "functions": arr }).to_string()
        }
        "TA_FunctionDescriptionXML" => {
            let xml = abstract_api::function_description_xml();
            let length = xml.len();
            let checksum: u64 = xml.bytes().map(|b| u64::from(b)).sum();
            format!("{{\"length\":{},\"checksum\":{}}}", length, checksum)
        }
"#;
