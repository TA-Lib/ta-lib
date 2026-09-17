//! Generates JSON-RPC server source files for each target language.
//!
//! Each generated server reads JSON-RPC requests from stdin, dispatches to
//! the generated TA function implementations, and writes JSON responses to stdout.
//! All servers speak the same protocol as the existing Rust server in server.rs.

use std::fmt::Write as _;
use crate::backends::builtins::SpecialBuiltin;
use crate::backends::c::c_predicate_expr;
use crate::backends::java::java_predicate_expr;
use crate::backends::rust_lang::rust_predicate_expr;
use crate::ir::{EnumDef, FuncDef, Input, Output, ParamType};
use std::collections::HashMap;
use std::path::Path;

// The three boolean near-zero builtins exposed by the `eval_predicate` JSON-RPC
// method (integer `which` selector: 0=IS_ZERO, 1=IS_ZERO_SCALED, 2=IS_ZERO_OR_NEG).
// IS_ZERO_SCALED consumes a parallel `scale` array; the other two ignore it.
// The per-backend expression for each is produced by the same `*_predicate_expr`
// the indicator code path uses, so this test verifies the real emitted form.

/// The comma-separated `FuncUnstId` variant names from enums.yaml (the source of
/// truth), in ordinal order. Empty if the enum is somehow missing.
fn func_unst_variant_names(enums: &HashMap<String, EnumDef>) -> Vec<String> {
    enums
        .get("FuncUnstId")
        .map(|fu| fu.variants.iter().map(|v| v.name.clone()).collect())
        .unwrap_or_default()
}

/// Generate the JSON response key for an output at position `idx` among all outputs.
///
/// Naming convention (matches ta_regtest expectations): the type name, then the
/// output's rank among outputs of that same type, with the rank omitted at 0 —
/// `outReal`, `outReal1`, `outReal2`, …, `outInteger`, `outInteger1`, … The rank
/// is per-type, so two real outputs and one integer output yield `"outReal"`,
/// `"outReal1"`, `"outInteger"` (not `"outInteger2"`).
///
/// Derived, not enumerated, and the same rule is built by hand in two other
/// places that have to agree with it: the C server's abstract handler
/// (`templates/c/ta_abstract_serve.c`) and the driver that reads its reply
/// (`test_abstract.c`). Neither may go back to a hardcoded list of keys — such a
/// list stops at the corpus's widest function and silently blinds the gate past
/// it (#262).
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
/// Used by Java and C# servers (e.g., "inHigh" -> "refHigh").
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

/// Map a function name to its unstable-period id, derived from `enums.yaml`.
///
/// A function owns the id whose enumerator is `TA_FUNC_UNST_<NAME>` — the whole
/// naming convention of the enum. Never a hardcoded `match`: a second copy of
/// the numbering drifts silently, because the writer and the reader both use the
/// same wrong value. That is how ta-lib-python's ids came to mis-target after
/// the 0.6.0 renumbering.
///
/// Retired slots (`TA_FUNC_UNST_UNUSED_*`) match no function and yield `None`.
pub(crate) fn func_unst_id(name: &str, enums: &HashMap<String, EnumDef>) -> Option<i32> {
    let target = format!("TA_FUNC_UNST_{name}");
    // Expect, not `?`: an absent enum would hand every function `None`, emitting a
    // server with no unstable-period wiring at all and exiting 0. The C backend has
    // no other use for `enums`, so nothing else would catch it.
    enums
        .get("FuncUnstId")
        .expect("FuncUnstId enum missing from enums.yaml")
        .variants
        .iter()
        .find(|v| v.c_name == target)
        .map(|v| v.value)
}

/// Replace @@`CORE_XXX`@@ markers in the Java server template with actual
/// method bodies read from the generated Core_*.java files.
///
/// The text spliced here must stay **identical** (modulo the 4-space indent) to
/// what `java_shipped::generate_core` splices into the shipped `Core.java` — that
/// identity is the numerical-correctness proof: the server the cross-language
/// harness measures runs the same source the library ships.
///
/// Consecutive `return` lines are therefore a hard error rather than a silent
/// drop. Java rejects the second as unreachable, so a fragment containing them
/// would fail to compile in the shipped library anyway; dropping one here would
/// merely hide that behind a server whose bytes differ from the library's. All
/// 168 fragments contain zero such pairs, so this costs nothing today and stops
/// a future emitter from quietly breaking the identity.
pub fn inline_java_core_methods(template: &str, java_dir: &Path, funcs: &[FuncDef]) -> String {
    let mut result = template.to_string();
    for func in funcs {
        let marker = format!("    // @@CORE_{}@@", func.name);
        let core_path = java_dir.join(format!("Core_{}.java", func.name));
        let replacement = if core_path.exists() {
            let content = std::fs::read_to_string(&core_path).unwrap();
            // Strip the /* Generated */ prefix and indent into the server's Core.
            let mut lines: Vec<String> = Vec::new();
            let mut prev_was_return = false;
            for (n, line) in content.lines().enumerate() {
                let trimmed = line.strip_prefix("/* Generated */").unwrap_or(line);
                let is_return = trimmed.trim().starts_with("return ");
                assert!(
                    !(is_return && prev_was_return),
                    "{}:{}: two consecutive `return` lines. Java rejects the second as \
                     unreachable, so this fragment cannot compile in the shipped library — \
                     fix the emitter rather than letting the server and the library differ.",
                    core_path.display(),
                    n + 1
                );
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



/// Generate `src/ta_func/ta_func_stream_private.h` — the internal declarations the
/// generated `ta_*.c` files need from each other.
///
/// This is a PRIVATE header: it is not installed and is not part of the public
/// contract.
///
/// The public stream surface is already complete in `ta_func.h`: the
/// `typedef struct TA_<N>_Stream TA_<N>_Stream;` plus `_Open` / `_Update` / `_Peek`
/// / `_Close` / `_OpenAndFill`. What remains here is `TA_<N>_OpenInternal`, the
/// startIdx-aware worker behind the public `_Open`, which composed functions call
/// cross-TU when opening a sub-stream.
pub fn generate_c_stream_private_header(funcs: &[FuncDef]) -> String {
    // Resolve `PRAGMA TA_ALT` for this language before anything reads a body.
    let resolved = crate::ir::resolve_all(funcs, crate::ir::Lang::C);
    let funcs: &[FuncDef] = &resolved;
    let mut s = String::new();
    s.push_str("/* ta_func_stream_private.h — internal stream declarations.\n");
    s.push_str(" * Auto-generated by ta_codegen. NOT a public header: not installed,\n");
    s.push_str(" * not part of the API contract. The public stream surface lives in\n");
    s.push_str(" * ta_func.h.\n");
    s.push_str(" */\n");
    s.push_str("#ifndef TA_FUNC_STREAM_PRIVATE_H\n");
    s.push_str("#define TA_FUNC_STREAM_PRIVATE_H\n\n");

    s.push_str("#ifndef TA_COMMON_H\n");
    s.push_str("   #include \"ta_common.h\"\n");
    s.push_str("#endif\n\n");
    s.push_str("#ifndef TA_DEFS_H\n");
    s.push_str("   #include \"ta_defs.h\"\n");
    s.push_str("#endif\n\n");

    // TA_<N>_OpenInternal is the startIdx-aware worker behind the public
    // TA_<N>_Open (a thin wrapper passing startIdx=0). Only generated code — a
    // composed function opening a sub-stream — calls it, and it does so cross-TU,
    // so it needs a plain extern forward declaration here.
    s.push_str("/* Internal stream-open declarations (startIdx-aware; behind the public Open) */\n");
    // Forward-declare each stream handle tag at file scope so the prototypes
    // below refer to the same struct as the definitions (a bare `struct X` first
    // seen inside a prototype would get prototype scope and collide).
    for func in funcs.iter().filter(|f| f.streaming) {
        s.push_str(&format!("struct TA_{}_Stream;\n", func.name.to_uppercase()));
    }
    for func in funcs.iter().filter(|f| f.streaming) {
        s.push_str(&crate::backends::c_stream::open_internal_signature(func));
        s.push_str(";\n");
    }
    s.push('\n');

    // TA_<N>_OpenAndFillInternal is the same worker at stride 1: it warms the
    // handle AND fills the caller's arrays in the one pass, which is how a
    // composed Open avoids re-running the batch sub-call it just duplicated
    // (issue #192). Same cross-TU story as OpenInternal above.
    s.push_str("/* Internal stream open+fill declarations (startIdx-aware; one pass, fills arrays) */\n");
    let lookup = crate::streaming::FuncsLookup(funcs);
    for func in funcs
        .iter()
        .filter(|f| crate::streaming::emits_open_and_fill_internal(f, &lookup))
    {
        s.push_str(&crate::backends::c_stream::open_and_fill_internal_signature(func));
        s.push_str(";\n");
    }
    s.push('\n');

    s.push_str("#endif /* TA_FUNC_STREAM_PRIVATE_H */\n");
    s
}


/// Generate a standalone C JSON-RPC server source file.
///
/// The generated file #includes the generated ta_*.c files and provides
/// a `main()` loop that reads JSON-RPC from stdin.
#[allow(clippy::implicit_hasher)]
pub fn generate_c_server(funcs: &[FuncDef], enums: &HashMap<String, EnumDef>) -> String {
    // Resolve `PRAGMA TA_ALT` for this language before anything reads a body.
    let resolved = crate::ir::resolve_all(funcs, crate::ir::Lang::C);
    let funcs: &[FuncDef] = &resolved;
    let mut s = String::new();

    // Header
    s.push_str("/* Auto-generated JSON-RPC server for ta_codegen C output.\n");
    s.push_str(" * Reads JSON-RPC requests from stdin, writes responses to stdout.\n");
    s.push_str(" * Build: compile each ta_*.c separately, then link with this file.\n");
    s.push_str(" */\n");
    s.push_str("#include <stdio.h>\n");
    s.push_str("#include <stdlib.h>\n");
    s.push_str("#include <stdarg.h>\n");
    s.push_str("#include <string.h>\n");
    s.push_str("#include <limits.h>\n");
    s.push_str("#include <math.h>\n");
    s.push_str("#include <time.h>\n");
    s.push_str("#ifdef __APPLE__\n");
    s.push_str("#include <mach/mach_time.h>\n");
    s.push_str("#endif\n\n");

    // Internal stream declarations (TA_<N>_OpenInternal)
    s.push_str("#include \"ta_func/ta_func_stream_private.h\"\n\n");

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
    s.push_str(&generate_c_global_buffers(funcs));

    // Generic ta_abstract handlers (abstract_call, abstract_get_lookback, abstract_for_each_func)
    s.push_str(&generate_c_abstract_handlers());

    // stream_verify: in-process bitwise batch-vs-stream comparison
    // (docs/streaming-api-design.md, Verification). fuzz_data.h is included
    // HERE — after the indicator code — because its file-scope
    // `#pragma STDC FP_CONTRACT OFF` must not alter indicator contraction.
    // Absent entirely under TA_REF_SERVE (frozen libs have no stream symbols).
    s.push_str("#ifndef TA_REF_SERVE\n#include \"fuzz_data.h\"\n#endif\n\n");
    s.push_str(&crate::stream_verify_gen::c::generate_c_stream_verify(funcs, enums));

    // Ride-along: batch-vs-stream on whatever inputs the caller sent.
    s.push_str(&crate::ride_gen::c::generate_c_ridealong(funcs));

    // Dispatch function
    s.push_str(&generate_c_dispatch(funcs, enums));

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

    // Same phase the indicator files run: the per-function handlers declare a
    // block's names before they know which arms will read them.
    crate::backends::c_hygiene::scrub_void_casts(&s)
}

#[allow(clippy::too_many_lines)]
fn generate_c_json_helpers() -> String {
    r#"/* ---- Minimal JSON helpers ---- */

#define MAX_ARRAY_SIZE 200000

/* Bounded append helpers.
 *
 * `pos += snprintf(buf + pos, buf_size - pos, ...)` lets `pos` run past
 * `buf_size` as soon as one call truncates; the next call then passes a
 * negative size that converts to a huge size_t and writes past the buffer
 * (CodeQL cpp/overflowing-snprintf). These helpers saturate `pos` at
 * `buf_size - 1` instead, so the buffer stays NUL-terminated and in bounds.
 * All of them take and return an absolute write position. */
static int json_appendf(char *buf, int buf_size, int pos, const char *fmt, ...) {
    va_list ap;
    int avail, n;
    if( buf_size <= 0 ) return 0;
    if( pos < 0 ) pos = 0;
    if( pos >= buf_size - 1 ) return buf_size - 1;
    avail = buf_size - pos;
    va_start(ap, fmt);
    n = vsnprintf(buf + pos, (size_t)avail, fmt, ap);
    va_end(ap);
    /* C11 7.21.6.12: only a non-negative return guarantees what was written,
       so on an encoding error re-terminate rather than trust the buffer. */
    if( n < 0 ) { buf[pos] = '\0'; return pos; }
    if( n >= avail ) return buf_size - 1;
    return pos + n;
}

static int json_appendc(char *buf, int buf_size, int pos, char c) {
    if( buf_size <= 0 ) return 0;
    if( pos < 0 ) pos = 0;
    if( pos >= buf_size - 1 ) return buf_size - 1;
    buf[pos++] = c;
    buf[pos] = '\0';
    return pos;
}

/* Parses wide and saturates, rather than atoi's silent truncation to int.
 * A wire value of 2^32 truncates to 0, so `atoi` turned an out-of-domain
 * request into a legal one and the server answered "ok" to a setting nobody
 * asked for -- while the Rust and Java servers, which range-check a 64-bit
 * parse, rejected the same request. Saturating fails closed instead: no
 * parameter in the library has a legal domain reaching INT_MAX (the widest
 * integer range is 100000, and the index ceiling is TA_MAX_INDEX = 1e8), so a
 * saturated value is refused by whatever validation the field already has.
 *
 * INT_MIN is deliberately NOT the negative clamp: it is TA_INTEGER_DEFAULT,
 * and manufacturing it would turn an out-of-range request into "use the
 * documented default" -- silently wrong in the one direction that looks like
 * success. A wire value of exactly INT_MIN still parses to INT_MIN, since that
 * is how a caller legitimately asks for the default. */
static int json_find_int(const char *json, const char *field) {
    char pattern[256];
    long long v;
    snprintf(pattern, sizeof(pattern), "\"%s\":", field);
    const char *p = strstr(json, pattern);
    if( !p ) return 0;
    p += strlen(pattern);
    while( *p == ' ' ) p++;
    v = strtoll(p, NULL, 10);
    if( v > (long long)INT_MAX ) return INT_MAX;
    if( v < (long long)INT_MIN ) return INT_MIN + 1;
    return (int)v;
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

/* One f64, transported as the 16 hex chars of its IEEE-754 bit pattern.
 *
 * A scalar the caller wants delivered EXACTLY cannot go over the wire as a JSON
 * number. %.17g does round-trip every finite double, but NaN and the infinities
 * have no JSON number spelling at all -- and `factor` has to carry a NaN,
 * because refusing one is part of the contract being compared across languages.
 * Same encoding json_find_double_array already uses for arrays (#115), one
 * group instead of many. Returns `def` when the field is absent or malformed,
 * so a caller that omits it gets a documented value rather than a silent 0. */
static double json_find_f64_bits(const char *json, const char *field, double def) {
    char pattern[256];
    unsigned long long bits = 0;
    double out;
    int k;
    snprintf(pattern, sizeof(pattern), "\"%s\":\"", field);
    const char *p = strstr(json, pattern);
    if( !p ) return def;
    p += strlen(pattern);
    for( k = 0; k < 16; k++ ) {
        char c = p[k];
        unsigned int v;
        if     ( c >= '0' && c <= '9' ) v = (unsigned int)(c - '0');
        else if( c >= 'a' && c <= 'f' ) v = (unsigned int)(c - 'a' + 10);
        else if( c >= 'A' && c <= 'F' ) v = (unsigned int)(c - 'A' + 10);
        else return def;   /* short or non-hex group */
        bits = (bits << 4) | v;
    }
    memcpy(&out, &bits, sizeof(double));
    return out;
}

static int json_find_double_array(const char *json, const char *field,
                                   double *out, int max_count) {
    char pattern[256];
    snprintf(pattern, sizeof(pattern), "\"%s\":", field);
    const char *p = strstr(json, pattern);
    if( !p ) return 0;
    p += strlen(pattern);
    while( *p == ' ' ) p++;
    if( *p == '"' ) {
        /* Lossless hex-bits transport (issue #115): a string of concatenated
         * 16-hex-char groups, each one f64's IEEE-754 bit pattern. Decoded
         * exactly (no strtod rounding). Every other caller sends a [ ] array. */
        p++;
        int count = 0;
        while( count < max_count && *p && *p != '"' ) {
            unsigned long long bits = 0;
            int k;
            for( k = 0; k < 16 && p[k] && p[k] != '"'; k++ ) {
                char c = p[k];
                unsigned int v = (c >= '0' && c <= '9') ? (unsigned int)(c - '0')
                               : (c >= 'a' && c <= 'f') ? (unsigned int)(c - 'a' + 10)
                               : (c >= 'A' && c <= 'F') ? (unsigned int)(c - 'A' + 10) : 0u;
                bits = (bits << 4) | v;
            }
            if( k < 16 ) break;   /* truncated trailing group */
            memcpy(&out[count], &bits, sizeof(double));
            count++;
            p += 16;
        }
        return count;
    }
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

/* Real OUTPUT arrays ride the same lossless hex-bits transport the INPUT
 * arrays have used since #115: one string of concatenated 16-hex-char groups,
 * each one f64's IEEE-754 bit pattern (json_find_double_array's string arm is
 * the read side). Decimal text could not carry either half of what an output
 * has to carry -- %.15g rounds a finite double off by up to ~1-2 ULP (#257),
 * and no decimal spelling exists at all for an infinity or for WHICH NaN a
 * payload is (#258). Every backend now writes this same encoding, so the
 * transport is lossless by construction rather than by whichever native
 * formatter each language happens to ship. */
static int json_write_double_array(char *buf, int buf_size, int pos,
                                    const double *data, int count) {
    pos = json_appendc(buf, buf_size, pos, '"');
    for( int i = 0; i < count; i++ ) {
        unsigned long long bits;
        memcpy(&bits, &data[i], sizeof(double));
        pos = json_appendf(buf, buf_size, pos, "%016llx", bits);
    }
    return json_appendc(buf, buf_size, pos, '"');
}

static int json_write_int_array(char *buf, int buf_size, int pos,
                                 const int *data, int count) {
    pos = json_appendc(buf, buf_size, pos, '[');
    for( int i = 0; i < count; i++ ) {
        if( i > 0 ) pos = json_appendc(buf, buf_size, pos, ',');
        pos = json_appendf(buf, buf_size, pos, "%d", data[i]);
    }
    return json_appendc(buf, buf_size, pos, ']');
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
fn generate_c_global_buffers(funcs: &[FuncDef]) -> String {
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
    // Output buffers, one per slot the WIDEST function in the corpus uses —
    // counted, not written down. Three reals (MACD/BBANDS/STOCH) and two
    // integers (MINMAXINDEX) is what today's corpus needs; a literal there is
    // what made a third integer output fail to compile (#262).
    let (n_out_real, n_out_int) = crate::backends::common::max_output_arity(funcs);
    for k in 0..n_out_real {
        let _ = writeln!(s, "static double g_outBuf{k}[MAX_ARRAY_SIZE];");
    }
    for k in 0..n_out_int {
        let _ = writeln!(s, "static int g_outIntBuf{k}[MAX_ARRAY_SIZE];");
    }
    // Indexable views of the same buffers, plus the widths, for the hand-written
    // abstract handler (`templates/c/ta_abstract_serve.c`). It binds outputs by
    // ordinal and cannot name `g_outBuf0..n` itself without hardcoding an arity
    // — which is what it did: a third integer output bound `g_outIntBuf1` twice
    // and `TA_CallFunc` rejected the call as two outputs sharing a buffer (#262).
    let reals: Vec<String> = (0..n_out_real).map(|k| format!("g_outBuf{k}")).collect();
    let ints: Vec<String> = (0..n_out_int).map(|k| format!("g_outIntBuf{k}")).collect();
    let _ = writeln!(s, "static double *const g_outBufV[] = {{ {} }};", reals.join(", "));
    let _ = writeln!(s, "static int *const g_outIntBufV[] = {{ {} }};", ints.join(", "));
    let _ = writeln!(s, "#define TA_SERVE_MAX_OUT_REAL {n_out_real}");
    let _ = writeln!(s, "#define TA_SERVE_MAX_OUT_INT {n_out_int}");
    let _ = writeln!(
        s,
        "#define TA_SERVE_MAX_OUTPUT {}",
        funcs.iter().map(|f| f.outputs.len()).max().unwrap_or(1).max(1)
    );
    s.push('\n');

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

/// The abstract handler C code lives in ta_codegen/generator/templates/c/ta_abstract_serve.c
/// (native C, not generated). The server just #includes it.
fn generate_c_abstract_handlers() -> String {
    "#include \"ta_abstract_serve.c\"\n\n".to_string()
}

#[allow(clippy::too_many_lines)]
/// The `--mode=open` / `--mode=openfill` arms of the Rust bench loop. Handles
/// are dropped at end of scope, so the warm-up cost here is the Open alone.
/// `historyLen` is the whole preloaded slice: the streaming entry points pin
/// bar 0, which is what makes `OpenAndFill` bit-exact.
fn emit_rust_warmup_arms(
    s: &mut String,
    func: &FuncDef,
    input_names: &[String],
    outputs: &[Output],
) {
    let base = crate::backends::common::snake_words(&func.name);
    let mut ins = String::new();
    for name in input_names {
        // Slice to the benched range. Rust derives historyLen from the slice
        // length, and with use_preloaded the buffer is the whole preloaded
        // array -- passing it whole would replay a history unrelated to
        // --points (the C arm passes `endIdx + 1` for the same reason).
        let _ = write!(ins, "&{name}[..=endIdx], ");
    }
    let mut opts = String::new();
    for opt in &func.optional_inputs {
        let _ = write!(opts, "{}, ", opt.name);
    }
    let mut fill_outs = String::new();
    let (mut real_idx, mut int_idx) = (0usize, 0usize);
    for out in outputs {
        // A nullable output takes `Option<&mut [T]>` (rule B6a); this arm
        // compares values, so it always supplies one.
        let (op, cl) = if out.is_nullable() { ("Some(", ")") } else { ("", "") };
        if out.param_type == ParamType::Integer {
            let _ = write!(fill_outs, ", {op}&mut outIntBuf{int_idx}{cl}");
            int_idx += 1;
        } else {
            let _ = write!(fill_outs, ", {op}&mut outBuf{real_idx}{cl}");
            real_idx += 1;
        }
    }
    s.push_str("            if bench_mode == 1 {\n");
    s.push_str(&format!(
        "                rc = match core.{base}_open({ins}{opts}) {{ Ok(_h) => RetCode::Success, Err(e) => e }};\n"
    ));
    s.push_str("            } else {\n");
    // The fill reports its range as an `OutRange` beside the handle (#179 C15);
    // unpack it into the same two locals the batch arm sets, which the output
    // hash below reads.
    s.push_str(&format!(
        "                rc = match core.{base}_open_and_fill({ins}{opts}{}) {{ Ok((_h, r)) => {{ outBegIdx = r.beg_idx; outNBElement = r.count; RetCode::Success }} Err(e) => e }};\n",
        fill_outs.trim_start_matches(", ")
    ));
    s.push_str("            }\n");
}

/// The `--mode=open` / `--mode=openfill` arms of the C bench loop: time the
/// streaming warm-up instead of the batch call. Every function streams, so both
/// arms exist unconditionally. `historyLen` is `endIdx + 1` — the streaming
/// entry points pin bar 0 (that is what makes `OpenAndFill` bit-exact), so they
/// replay `0..endIdx` regardless of `startIdx`. Each arm closes the handle it
/// opened: a 168-function sweep would otherwise leak one per iteration, and the
/// free is nanoseconds against a whole-history replay.
fn emit_c_warmup_arms(s: &mut String, func: &FuncDef, input_names: &[String]) {
    let n = func.name.clone();
    let mut open_args = String::new();
    for (j, _name) in input_names.iter().enumerate() {
        let _ = write!(open_args, ", g_inBuf{j}");
    }
    let _ = write!(open_args, ", endIdx + 1");
    for opt in &func.optional_inputs {
        let _ = write!(open_args, ", {}", opt.name);
    }
    let mut scalar_outs = String::new();
    let mut fill_outs = String::new();
    let (mut real_idx, mut int_idx) = (0usize, 0usize);
    for (k, out) in func.outputs.iter().enumerate() {
        let _ = write!(scalar_outs, ", &_openOut{k}");
        if out.param_type == ParamType::Integer {
            let _ = write!(fill_outs, ", g_outIntBuf{int_idx}");
            int_idx += 1;
        } else {
            let _ = write!(fill_outs, ", g_outBuf{real_idx}");
            real_idx += 1;
        }
    }
    // Compiled out for the frozen reference server, whose library predates the
    // streaming API and exports no TA_<N>_Open / _Close / _OpenAndFill to link
    // against -- the same guard every other stream-touching handler here carries.
    // The `bench_mode != 0` early return above is what keeps that honest: without
    // it this chain would fall through with rc untouched and report the batch
    // timing as a warm-up number.
    s.push_str("#ifndef TA_REF_SERVE\n");
    s.push_str("        else if( bench_mode == 1 ) {\n");
    s.push_str(&format!("            TA_{n}_Stream *_h = NULL;\n"));
    for (k, out) in func.outputs.iter().enumerate() {
        let ty = if out.param_type == ParamType::Integer { "int" } else { "double" };
        s.push_str(&format!("            {ty} _openOut{k} = 0;\n"));
    }
    s.push_str(&format!("            rc = TA_{n}_Open( &_h{open_args}{scalar_outs} );\n"));
    s.push_str(&format!("            if( _h ) TA_{n}_Close( _h );\n"));
    s.push_str("        }\n");
    s.push_str("        else {\n");
    s.push_str(&format!("            TA_{n}_Stream *_h = NULL;\n"));
    s.push_str(&format!("            rc = TA_{n}_OpenAndFill( &_h{open_args}, &outBegIdx, &outNBElement{fill_outs} );\n"));
    s.push_str(&format!("            if( _h ) TA_{n}_Close( _h );\n"));
    s.push_str("        }\n");
    s.push_str("#endif /* TA_REF_SERVE */\n");
}

#[allow(clippy::too_many_lines)]
fn generate_c_dispatch(funcs: &[FuncDef], enums: &HashMap<String, EnumDef>) -> String {
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

    // stream_verify: batch-vs-stream bitwise comparison, computed in-process.
    s.push_str("    if ( methodLen == 13 && strncmp(method, \"stream_verify\", 13) == 0 ) {\n");
    s.push_str("        handle_stream_verify(json, resp, resp_size);\n");
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
        if let Some(id) = func_unst_id(&func.name, enums) {
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
        // bench_mode (ta_bench --mode): 0 = the batch call (default), 1 = the
        // streaming warm-up TA_<N>_Open, 2 = TA_<N>_OpenAndFill. The warm-up
        // arms time an Open+Close round trip: the handle has to be released
        // every iteration or a 168-function sweep leaks one per iteration, and
        // the free is nanoseconds against a whole-history replay.
        s.push_str("        int bench_mode = json_find_int(json, \"bench_mode\");\n");
        // The frozen reference server has no streaming API to warm up, exactly as
        // the C# backend has none -- so it gives the same answer C# does rather
        // than timing the batch call and reporting it as a warm-up. ta_bench drops
        // timing_ns 0 as a non-measurement, so the cref column reads blank.
        s.push_str("#ifdef TA_REF_SERVE\n");
        s.push_str("        if( bench_mode != 0 ) {\n");
        s.push_str("            snprintf(resp, resp_size, \"{\\\"retCode\\\":0,\\\"timing_ns\\\":0,\\\"unsupported_mode\\\":1}\");\n");
        s.push_str("            return;\n");
        s.push_str("        }\n");
        s.push_str("#endif /* TA_REF_SERVE */\n");

        s.push_str("        TA_RetCode rc = 0;\n");

        // Copy once before timing
        s.push_str("        if( use_preloaded ) {\n");
        s.push_str(&format!(
            "            preload_to_working({n_inputs}, {});\n",
            i32::from(is_price_input)
        ));
        s.push_str("        }\n");

        // Single timing block around ALL iterations — amortizes timer overhead.
        // Iteration 0 is ALWAYS a discarded warm-up, on every path including the
        // correctness ones. Two reasons, and the second is the important one:
        //
        //  1. Benchmarking: the cold call page-faults the output arrays, pulls
        //     the input into cache, and on the managed servers forces the JIT.
        //     It measures 1.5-10x the steady state, which at --iters=1 IS the
        //     whole number.
        //  2. Correctness: because the reported/hashed output now comes from a
        //     SECOND call while the golden is the in-process C library called
        //     ONCE, every gate becomes an idempotency check for free. A function
        //     that mutates its input, or uses its output buffer as scratch while
        //     assuming it starts clean, diverges from the golden and fails.
        //
        // The branch is per-iteration, not per-bar — free against a whole indicator.
        s.push_str("        long _t0 = 0;\n");
        s.push_str("        for( int _bi = 0; _bi <= bench_iters; _bi++ ) {\n");
        s.push_str("        if( _bi == 1 ) _t0 = get_nanotime();\n");

        // Call the function
        s.push_str("        if( bench_mode == 0 )\n");
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

        emit_c_warmup_arms(&mut s, func, &input_names);
        s.push_str("        }\n"); // end bench_iters loop
        s.push_str("        long elapsed_ns = (get_nanotime() - _t0) / bench_iters;\n");

        // want_hash mode (server_verify / issue #115): after the GUARDED call —
        // the same public API TA_CallFunc runs in-process for the golden — return
        // a full-precision FNV digest of the raw output bytes instead of the arrays
        // themselves, so a same-input C-vs-C build-flag drift is ONE value to
        // compare rather than outNBElement of them.
        // fuzz_hash_* live in fuzz_data.h, only present when not TA_REF_SERVE; the
        // frozen reference server never receives want_hash (server_verify drives
        // the four generated servers, not ta_ref_serve).
        s.push_str("#ifndef TA_REF_SERVE\n");
        s.push_str("        if( json_find_int(json, \"want_hash\") && !json_find_int(json, \"full_output\") ) {\n");
        s.push_str("            unsigned long long _oh = fuzz_hash_init();\n");
        s.push_str("            if( rc == TA_SUCCESS && outNBElement > 0 ) {\n");
        {
            let mut real_idx = 0usize;
            let mut int_idx = 0usize;
            for out in outputs {
                if out.param_type == ParamType::Integer {
                    s.push_str(&format!(
                        "                _oh = fuzz_hash_bytes(_oh, g_outIntBuf{int_idx}, (unsigned long)outNBElement * sizeof(int));\n"
                    ));
                    int_idx += 1;
                } else {
                    s.push_str(&format!(
                        "                _oh = fuzz_hash_bytes(_oh, g_outBuf{real_idx}, (unsigned long)outNBElement * sizeof(double));\n"
                    ));
                    real_idx += 1;
                }
            }
        }
        s.push_str("            }\n");
        s.push_str("            _oh = fuzz_hash_fin(_oh);\n");
        s.push_str("            int _hp = json_appendf(resp, resp_size, 0, \"{\\\"retCode\\\":%d,\\\"outBegIdx\\\":%d,\\\"outNBElement\\\":%d,\\\"out_hash\\\":\\\"%016llx\\\"\", (int)rc, outBegIdx, outNBElement, _oh);\n");
        if func.streaming {
            s.push_str("#ifndef TA_REF_SERVE\n");
            s.push_str(&format!(
                "            sr_{}( json, endIdx{}, resp, resp_size, &_hp );\n",
                func.name,
                func.optional_inputs.iter().fold(String::new(), |mut acc, o| {
                    let _ = write!(acc, ", {}", o.name);
                    acc
                })
            ));
            s.push_str("#endif /* TA_REF_SERVE */\n");
        }
        s.push_str("            json_appendf(resp, resp_size, _hp, \"}\");\n");
        s.push_str("            return;\n");
        s.push_str("        }\n");
        s.push_str("#endif /* TA_REF_SERVE */\n");


        // Float-variant leg: with "use_float":1 the call is re-run through the
        // single-precision TA_S_ API (inputs converted to float) and the
        // response carries the S-variant result instead. The frozen reference
        // library also exports the guarded TA_S_ functions, so ta_ref_serve
        // answers this too — giving S-vs-S comparison against the reference.
        // Mirrors the double flow: guarded first, then (outside ta_ref_serve)
        // the S variant over the same buffers.
        s.push_str("        int usedFloat = 0;\n");
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
        s.push_str("            usedFloat = 1;\n");
        s.push_str("        }\n");

        // Build response with correct key names and serialisers per output type.
        s.push_str("        int pos = json_appendf(resp, resp_size, 0,\n");
        // `out_len` is the length of the buffer the server handed the call, so the
        // harness can assert the bound is a MINIMUM against what the server did
        // rather than against what the harness asked for. C answers MAX_ARRAY_SIZE
        // because its outputs are file-scope statics -- it has no size to check
        // against and nothing to gain from an exact one -- so it is always slack,
        // and reporting that keeps the harness's floor total rather than
        // exempting a backend from it.
        s.push_str("            \"{\\\"retCode\\\":%d,\\\"outBegIdx\\\":%d,\\\"outNBElement\\\":%d,\\\"out_len\\\":%d,\\\"timing_ns\\\":%ld\",\n");
        s.push_str("            (int)rc, outBegIdx, outNBElement, (int)MAX_ARRAY_SIZE, elapsed_ns);\n");
        // no_output: ta_bench only reads timing_ns, but serialising a 100k-element
        // array costs more than the call being measured. Suppressing the
        // arrays keeps retCode/outBegIdx/outNBElement so the caller can still tell
        // a real result from an error.
        s.push_str("        if( !json_find_int(json, \"no_output\") ) {\n");
        {
            let mut real_idx = 0usize;
            let mut int_idx = 0usize;
            for (k, out) in outputs.iter().enumerate() {
                let key = output_json_key(outputs, k);
                s.push_str(&format!(
                    "        pos = json_appendf(resp, resp_size, pos, \",\\\"{key}\\\":\");\n"
                ));
                if out.param_type == ParamType::Integer {
                    s.push_str(&format!(
                        "        pos = json_write_int_array(resp, resp_size, pos, g_outIntBuf{int_idx}, outNBElement);\n"
                    ));
                    int_idx += 1;
                } else {
                    s.push_str(&format!(
                        "        pos = json_write_double_array(resp, resp_size, pos, g_outBuf{real_idx}, outNBElement);\n"
                    ));
                    real_idx += 1;
                }
            }
        }
        s.push_str("        }\n");
        s.push_str("        pos = json_appendf(resp, resp_size, pos, \",\\\"used_float\\\":%d\", usedFloat);\n");
        if func.streaming {
            s.push_str("#ifndef TA_REF_SERVE\n");
            s.push_str(&format!(
                "        sr_{}( json, endIdx{}, resp, resp_size, &pos );\n",
                func.name,
                func.optional_inputs.iter().fold(String::new(), |mut acc, o| {
                    let _ = write!(acc, ", {}", o.name);
                    acc
                })
            ));
            s.push_str("#endif /* TA_REF_SERVE */\n");
        }
        s.push_str("        pos = json_appendf(resp, resp_size, pos, \"}\");\n");

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
    s.push_str("        int pos = json_appendf(resp, resp_size, 0, \"{\\\"functions\\\":[\");\n");
    for (i, func) in funcs.iter().enumerate() {
        let comma = if i > 0 { "," } else { "" };
        s.push_str(&format!(
            "        pos = json_appendf(resp, resp_size, pos, \"{}\\\"TA_{}\\\"\");\n",
            comma, func.name
        ));
    }
    s.push_str("        json_appendf(resp, resp_size, pos, \"]}\");\n");
    s.push_str("    }\n");

    // set_unstable_period method — {"method":"set_unstable_period","params":{"id":21,"period":10}}
    s.push_str("    else if ( methodLen == 19 && strncmp(method, \"set_unstable_period\", 19) == 0 ) {\n");
    s.push_str("        int id = json_find_int(json, \"id\");\n");
    s.push_str("        int period = json_find_int(json, \"period\");\n");
    // The RetCode is reported, not discarded. Left unchecked, the one server that
    // IS the C library answered "ok" to calls the C library rejects -- so C, the
    // control arm of every cross-language comparison, was the only backend that
    // could not be held to its own contract (#186).
    s.push_str("        TA_RetCode unstRc;\n");
    s.push_str("        if( period < 0 ) {\n");
    s.push_str("           /* The C parameter is unsigned, so a negative would wrap to a huge\n");
    s.push_str("            * value rather than be rejected. Caught on the wire instead.\n");
    s.push_str("            */\n");
    s.push_str("           snprintf(resp, resp_size, \"{\\\"error\\\":\\\"Invalid unstable period value\\\"}\");\n");
    s.push_str("        } else {\n");
    s.push_str("           unstRc = TA_SetUnstablePeriod((TA_FuncUnstId)id, (unsigned int)period);\n");
    s.push_str("           if( unstRc == TA_SUCCESS )\n");
    s.push_str("              snprintf(resp, resp_size, \"{\\\"status\\\":\\\"ok\\\"}\");\n");
    s.push_str("           else\n");
    s.push_str("              snprintf(resp, resp_size, \"{\\\"error\\\":\\\"Invalid unstable period id or value\\\"}\");\n");
    s.push_str("        }\n");
    s.push_str("    }\n");

    // set_candle_settings method (#215) —
    // {"method":"set_candle_settings","params":{"settingType":6,"rangeType":2,"avgPeriod":10,"factorBits":"3ff0000000000000"}}
    //
    // The C server validates nothing of its own: it hands the four arguments
    // straight to the library and reports the RetCode. That is what makes C the
    // reference arm here rather than a fourth opinion -- every other server has
    // to reproduce the domain TA_SetCandleSettings enforces, and this one simply
    // asks it. (The same reasoning as #186, where a discarded RetCode had made C
    // the one backend that could not be held to its own contract.)
    s.push_str("    else if ( methodLen == 19 && strncmp(method, \"set_candle_settings\", 19) == 0 ) {\n");
    s.push_str("        int settingType = json_find_int(json, \"settingType\");\n");
    s.push_str("        int rangeType   = json_find_int(json, \"rangeType\");\n");
    s.push_str("        int avgPeriod   = json_find_int(json, \"avgPeriod\");\n");
    s.push_str("        double factor   = json_find_f64_bits(json, \"factorBits\", 1.0);\n");
    s.push_str("        TA_RetCode csRc = TA_SetCandleSettings((TA_CandleSettingType)settingType,\n");
    s.push_str("                                              (TA_RangeType)rangeType,\n");
    s.push_str("                                              avgPeriod, factor);\n");
    s.push_str("        if( csRc == TA_SUCCESS )\n");
    s.push_str("           snprintf(resp, resp_size, \"{\\\"status\\\":\\\"ok\\\"}\");\n");
    s.push_str("        else\n");
    s.push_str("           snprintf(resp, resp_size, \"{\\\"error\\\":\\\"Invalid candle setting\\\"}\");\n");
    s.push_str("    }\n");

    // restore_candle_default_settings method (#215) —
    // {"method":"restore_candle_default_settings","params":{"settingType":11}}
    //
    // settingType 11 (TA_AllCandleSettings) is the wildcard here, and is the one
    // value set_candle_settings must REJECT. Keeping both methods on the same
    // parameter name is what lets one cross-language table drive both and see
    // that asymmetry.
    s.push_str("    else if ( methodLen == 31 && strncmp(method, \"restore_candle_default_settings\", 31) == 0 ) {\n");
    s.push_str("        int settingType = json_find_int(json, \"settingType\");\n");
    s.push_str("        TA_RetCode csRc = TA_RestoreCandleDefaultSettings((TA_CandleSettingType)settingType);\n");
    s.push_str("        if( csRc == TA_SUCCESS )\n");
    s.push_str("           snprintf(resp, resp_size, \"{\\\"status\\\":\\\"ok\\\"}\");\n");
    s.push_str("        else\n");
    s.push_str("           snprintf(resp, resp_size, \"{\\\"error\\\":\\\"Invalid candle setting type\\\"}\");\n");
    s.push_str("    }\n");

    // eval_predicate — evaluate a boolean near-zero builtin on each input value,
    // returning a 0/1 int array. Uses the SAME rendered form the indicators use.
    s.push_str("    else if ( methodLen == 14 && strncmp(method, \"eval_predicate\", 14) == 0 ) {\n");
    s.push_str("        double _pv[512]; double _ps[512]; int _pr[512];\n");
    s.push_str("        int _pw  = json_find_int(json, \"which\");\n");
    s.push_str("        int _pn  = json_find_double_array(json, \"values\", _pv, 512);\n");
    s.push_str("        int _pns = json_find_double_array(json, \"scale\", _ps, 512);\n");
    s.push_str("        for( int i = 0; i < _pn; i++ ) {\n");
    s.push_str("            double v = _pv[i];\n");
    s.push_str("            double s = ( i < _pns ) ? _ps[i] : 0.0;\n");
    s.push_str(&format!(
        "            if( _pw == 1 )      _pr[i] = ( {} ) ? 1 : 0;\n",
        c_predicate_expr(SpecialBuiltin::IsZeroScaled, &["v".to_string(), "s".to_string()])
    ));
    s.push_str(&format!(
        "            else if( _pw == 2 ) _pr[i] = ( {} ) ? 1 : 0;\n",
        c_predicate_expr(SpecialBuiltin::IsZeroOrNeg, &["v".to_string()])
    ));
    s.push_str(&format!(
        "            else                _pr[i] = ( {} ) ? 1 : 0;\n",
        c_predicate_expr(SpecialBuiltin::IsZero, &["v".to_string()])
    ));
    s.push_str("        }\n");
    s.push_str("        int _pp = json_appendf(resp, resp_size, 0, \"{\\\"outInteger\\\":\");\n");
    s.push_str("        _pp = json_write_int_array(resp, resp_size, _pp, _pr, _pn);\n");
    s.push_str("        json_appendf(resp, resp_size, _pp, \"}\");\n");
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
#[allow(clippy::cognitive_complexity)]
pub fn generate_java_server(funcs: &[FuncDef], enums: &HashMap<String, EnumDef>) -> String {
    // Resolve `PRAGMA TA_ALT` for this language before anything reads a body.
    let resolved = crate::ir::resolve_all(funcs, crate::ir::Lang::Java);
    let funcs: &[FuncDef] = &resolved;
    let mut s = String::new();

    s.push_str("/* Auto-generated JSON-RPC server for ta_codegen Java output.\n");
    s.push_str(" * Build: javac TaCodegenServe.java && java TaCodegenServe\n");
    s.push_str(" */\n");
    s.push_str("import java.io.*;\n");
    s.push_str("import java.util.*;\n\n");

    // RetCode enum -- the default-package twin of the shipped
    // io.github.talib.RetCode. The C number is carried BY THE MEMBER, exactly as
    // it is there: a `switch` with a `default:` arm reported a member nobody had
    // added an arm for as TA_INTERNAL_ERROR, on the wire, silently.
    s.push_str("enum RetCode {\n");
    s.push_str("    Success(0), BadParam(2), AllocErr(3), OutOfRangeStartIndex(12),\n");
    s.push_str("    OutOfRangeEndIndex(13), InsufficientHistory(17), InternalError(5000);\n");
    s.push_str("    private final int cValue;\n");
    s.push_str("    RetCode(int cValue) { this.cValue = cValue; }\n");
    s.push_str("    public int toInt() { return cValue; }\n");
    s.push_str("}\n\n");

    // MInteger helper
    s.push_str("class MInteger { public int value; }\n\n");

    // OutRange + the RetCode->exception mapper: the shipped Core's public
    // wrappers are part of the spliced fragment text (that identity is the
    // correctness proof), so the server needs both to compile them. The server
    // itself calls the internal cores, never these wrappers.
    s.push_str("record OutRange(int begIdx, int count) {\n");
    s.push_str("    static final OutRange EMPTY = new OutRange(0, 0);\n");
    s.push_str("    boolean isEmpty() { return count == 0; }\n");
    s.push_str("}\n\n");

    // FuncUnstId enum (referenced by generated Core methods).
    // FuncUnstId is emitted from enums.yaml (source of truth), 6 names per line,
    // plus the `All` wildcard carrying C's pinned TA_FUNC_UNST_ALL value. The
    // ordinal cannot express that value, so the constants declare theirs and
    // COUNT sizes the table (#144).
    s.push_str("enum FuncUnstId {\n");
    {
        let names = func_unst_variant_names(enums);
        let nchunks = names.chunks(6).count().max(1);
        for (idx, chunk) in names.chunks(6).enumerate() {
            if idx + 1 == nchunks {
                // Last line carries the function ids; the wildcard follows.
                s.push_str(&format!("    {},\n", chunk.join(", ")));
            } else {
                s.push_str(&format!("    {},\n", chunk.join(", ")));
            }
        }
    }
    s.push_str("    ALL;\n");
    s.push_str(&format!("    static final int COUNT = {};\n", func_unst_variant_names(enums).len()));
    s.push_str("    int value() { return this == ALL ? 65535 : ordinal(); }\n");
    s.push_str("}\n\n");

    // MAType — ordinal == the C enum value (enums.yaml rows are ascending).
    s.push_str("enum MAType {\n");
    {
        let ma = enums.get("MAType").expect("MAType enum required");
        let names: Vec<&str> = ma.variants.iter().map(|v| v.name.as_str()).collect();
        s.push_str(&format!("    {};\n", names.join(", ")));
    }
    s.push_str("}\n\n");

    // RangeType — mirrors the shipped enum (RealBody=0, HighLow=1, Shadows=2) so the
    // canonical candle access (`rangeType.ordinal()`) compiles here as in Core.java.
    s.push_str("enum RangeType {\n");
    s.push_str("    RealBody, HighLow, Shadows;\n");
    s.push_str("}\n\n");

    // CandleSetting holds rangeType, avgPeriod, factor for one candle setting
    s.push_str("class CandleSetting {\n");
    // final, so the shared default instances below cannot be mutated through the
    // live array: restore_candle_default_settings hands the same objects back out,
    // and set_candle_settings always replaces a slot rather than writing into one.
    s.push_str("    final RangeType rangeType;\n");
    s.push_str("    final int avgPeriod;\n");
    s.push_str("    final double factor;\n");
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
    // The parameter sentinels the generated validation names. This server is a
    // standalone compilation unit, so it carries its own copy of what the shipped
    // io.github.talib.Core declares.
    s.push_str("    static final double REAL_DEFAULT = -4e37;\n");
    s.push_str("    static final double REAL_MIN = -3e37;\n");
    s.push_str("    static final double REAL_MAX = 3e37;\n");
    s.push_str("    static final int INTEGER_DEFAULT = Integer.MIN_VALUE;\n");
    s.push_str("    static final int INTEGER_MIN = Integer.MIN_VALUE + 1;\n");
    s.push_str("    static final int INTEGER_MAX = Integer.MAX_VALUE;\n");
    s.push_str("    static final int MAX_INDEX = 100000000;\n");
    // Sized by the id count, so the wildcard gets no slot -- matching the
    // shipped CoreBuilder (#144).
    s.push_str("    int[] unstablePeriod = new int[FuncUnstId.COUNT];\n");
    // candleSettings[] in CandleSettingType ordinal order. Defaults from
    // TA_RestoreCandleDefaultSettings in ta_global.c. RangeType: 0=RealBody, 1=HighLow, 2=Shadows.
    s.push_str("    static final CandleSetting[] DEFAULT_CANDLE_SETTINGS = {\n");
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
    // The live array is a clone, so restoring a default is a slot copy out of the
    // table above rather than a second literal that could drift from it (#215).
    s.push_str("    CandleSetting[] candleSettings = DEFAULT_CANDLE_SETTINGS.clone();\n\n");
    // Mirrors the shipped Core's mapper — the spliced public wrappers call it.
    s.push_str("    static RuntimeException failure(String funcName, RetCode retCode) {\n");
    s.push_str("        String where = funcName + \": \";\n");
    s.push_str("        switch (retCode) {\n");
    s.push_str("            case OutOfRangeStartIndex: return new TaLibIndexException(where + \"startIdx out of range\", retCode);\n");
    s.push_str("            case OutOfRangeEndIndex: return new TaLibIndexException(where + \"endIdx out of range\", retCode);\n");
    // Split exactly as the shipped `Core.java` splits it: the parity gate
    // compares these bodies token by token (issue #271 item 3).
    s.push_str("            case BadParam: return new TaLibArgumentException(\n");
    s.push_str("                where + \"bad parameter (out-of-range optional parameter, or two \"\n");
    s.push_str("                      + \"outputs sharing one array)\", retCode);\n");
    s.push_str("            case AllocErr: return new TaLibStateException(where + \"allocation failed\", retCode);\n");
    s.push_str("            case InternalError: return new TaLibStateException(where + \"internal error\", retCode);\n");
    s.push_str("            case InsufficientHistory: return new InsufficientHistoryException(where + \"history shorter than the lookback\");\n");
    s.push_str("            default: return new TaLibStateException(where + retCode, retCode);\n");
    s.push_str("        }\n");
    s.push_str("    }\n\n");
    // Same for the wrapper's argument checks (#172 C2). The server never calls a
    // public wrapper — it calls the cores — but the spliced text has to compile,
    // and it has to compile against the SAME helpers the library ships, or the
    // identity this splice exists to preserve would be an identity of text only.
    s.push_str("    static int clampedStart(String funcName, int startIdx, int lookback) {\n");
    s.push_str("        if (lookback < 0) {\n");
    s.push_str("            throw failure(funcName, RetCode.BadParam);\n");
    s.push_str("        }\n");
    s.push_str("        return startIdx > lookback ? startIdx : lookback;\n");
    s.push_str("    }\n\n");
    for ty in ["double", "float", "int"] {
        s.push_str(&format!(
            "    static void requireLength(String funcName, String argName, {ty}[] array, int required) {{\n"
        ));
        s.push_str("        checkLength(funcName, argName, array == null ? -1 : array.length, required);\n");
        s.push_str("    }\n\n");
    }
    s.push_str("    static void checkLength(String funcName, String argName, int actual, int required) {\n");
    s.push_str("        if (actual < 0) {\n");
    s.push_str("            throw new TaLibArgumentException(funcName + \": \" + argName + \" is null\", RetCode.BadParam);\n");
    s.push_str("        }\n");
    s.push_str("        if (actual < required) {\n");
    s.push_str("            throw new TaLibArgumentException(funcName + \": \" + argName\n");
    s.push_str("                + \" has length \" + actual + \", needs \" + required, RetCode.BadParam);\n");
    s.push_str("        }\n");
    s.push_str("    }\n\n");
    s.push_str("    static void requireIndexRange(String funcName, int startIdx, int endIdx) {\n");
    s.push_str("        if (startIdx < 0 || startIdx > MAX_INDEX) {\n");
    s.push_str("            throw failure(funcName, RetCode.OutOfRangeStartIndex);\n");
    s.push_str("        }\n");
    s.push_str("        if (endIdx < 0 || endIdx > MAX_INDEX || endIdx < startIdx) {\n");
    s.push_str("            throw failure(funcName, RetCode.OutOfRangeEndIndex);\n");
    s.push_str("        }\n");
    s.push_str("    }\n\n");
    s.push_str("    static int openFillCount(String funcName, int historyLen, int lookback) {\n");
    s.push_str("        if (lookback < 0) {\n");
    s.push_str("            throw failure(funcName, RetCode.BadParam);\n");
    s.push_str("        }\n");
    s.push_str("        return historyLen <= lookback ? 0 : historyLen - lookback;\n");
    s.push_str("    }\n\n");
    s.push_str("    static void requireHistoryLength(String funcName, String argName, int actual, int historyLen) {\n");
    s.push_str("        if (actual != historyLen) {\n");
    s.push_str("            throw new TaLibArgumentException(funcName + \": \" + argName + \" has length \" + actual\n");
    s.push_str("                  + \", needs \" + historyLen, RetCode.BadParam);\n");
    s.push_str("        }\n");
    s.push_str("    }\n\n");
    s.push_str("    static void requireHistory(String funcName, int historyLen) {\n");
    s.push_str("        if (historyLen < 1) {\n");
    s.push_str("            throw failure(funcName, RetCode.OutOfRangeStartIndex);\n");
    s.push_str("        }\n");
    s.push_str("        if (historyLen > MAX_INDEX + 1) {\n");
    s.push_str("            throw failure(funcName, RetCode.OutOfRangeEndIndex);\n");
    s.push_str("        }\n");
    s.push_str("    }\n\n");
    s.push_str("    static void requireArgument(String funcName, String argName, Object argument) {\n");
    s.push_str("        if (argument == null) {\n");
    s.push_str("            throw new TaLibArgumentException(funcName + \": \" + argName + \" is null\", RetCode.BadParam);\n");
    s.push_str("        }\n");
    s.push_str("    }\n\n");
    for func in funcs {
        s.push_str(&format!("    // @@CORE_{}@@\n", func.name));
    }
    s.push_str("}\n\n");

    // Main server class
    s.push_str("public class TaCodegenServe {\n");
    s.push_str("    static Core core = new Core();\n");
    // The spliced half of the identity pair. Substituted by JavaBackend::
    // generate_server from the fragments actually read off disk, so it names the
    // text of THIS file's Core, not the shipped library's (#322).
    s.push_str("    static final String SPLICED_GENCODE_DIGEST = \"@@GENCODE_DIGEST@@\";\n");
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

    // One f64 as the 16 hex chars of its IEEE-754 bits — the scalar counterpart
    // of jsonDoubleArray's transport (#115). Used for `factor`, which has to be
    // able to carry a NaN: NaN has no JSON number spelling, and refusing one is
    // part of the contract compared across languages (#215).
    s.push_str("    static double jsonF64Bits(String json, String field, double def) {\n");
    s.push_str("        int idx = json.indexOf('\"' + field + '\"');\n");
    s.push_str("        if (idx < 0) return def;\n");
    s.push_str("        idx = json.indexOf(':', idx) + 1;\n");
    s.push_str("        while (idx < json.length() && json.charAt(idx) == ' ') idx++;\n");
    s.push_str("        if (idx >= json.length() || json.charAt(idx) != '\"') return def;\n");
    s.push_str("        int end = json.indexOf('\"', idx + 1);\n");
    s.push_str("        if (end != idx + 17) return def;\n");
    s.push_str("        try {\n");
    // parseUnsignedLong, not parseLong: any bit pattern with the sign bit set
    // overflows a signed long and would throw.
    s.push_str("            return Double.longBitsToDouble(\n");
    s.push_str("                Long.parseUnsignedLong(json.substring(idx + 1, end), 16));\n");
    s.push_str("        } catch (NumberFormatException e) {\n");
    s.push_str("            return def;\n");
    s.push_str("        }\n");
    s.push_str("    }\n\n");

    s.push_str("    static double[] jsonDoubleArray(String json, String field) {\n");
    s.push_str("        int idx = json.indexOf('\"' + field + '\"');\n");
    s.push_str("        if (idx < 0) return new double[0];\n");
    s.push_str("        idx = json.indexOf(':', idx) + 1;\n");
    s.push_str("        while (idx < json.length() && json.charAt(idx) == ' ') idx++;\n");
    // Lossless hex-bits transport (issue #115): a string of concatenated 16-hex
    // groups, each one double's IEEE-754 bit pattern. Decoded exactly. Every
    // other caller sends a [ ] number array (fallback below).
    s.push_str("        if (idx < json.length() && json.charAt(idx) == '\"') {\n");
    s.push_str("            int hend = json.indexOf('\"', idx + 1);\n");
    s.push_str("            String hex = json.substring(idx + 1, hend);\n");
    s.push_str("            int cnt = hex.length() / 16;\n");
    s.push_str("            double[] r = new double[cnt];\n");
    s.push_str("            for (int i = 0; i < cnt; i++)\n");
    s.push_str("                r[i] = Double.longBitsToDouble(Long.parseUnsignedLong(hex.substring(i * 16, i * 16 + 16), 16));\n");
    s.push_str("            return r;\n");
    s.push_str("        }\n");
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

    // Real output arrays: the lossless hex-bits transport the inputs have used
    // since #115 -- one string of concatenated 16-hex-char groups, each value's
    // IEEE-754 bits. `sb.append(double)` (Double.toString) round-trips a FINITE
    // value exactly, but prints every NaN as the same "NaN" token, so the
    // payload `doubleToRawLongBits` (and therefore out_hash) sees was
    // unrecoverable from the text (#258). doubleToRawLongBits does not
    // canonicalize, so the bits written here are the bits hash mode hashes.
    s.push_str("    static String doubleArrayToJson(double[] arr, int count) {\n");
    s.push_str("        StringBuilder sb = new StringBuilder(count * 16 + 2);\n");
    s.push_str("        sb.append('\"');\n");
    s.push_str("        for (int i = 0; i < count; i++) {\n");
    s.push_str("            long bits = Double.doubleToRawLongBits(arr[i]);\n");
    s.push_str("            for (int n = 60; n >= 0; n -= 4)\n");
    s.push_str("                sb.append(\"0123456789abcdef\".charAt((int) ((bits >>> n) & 0xfL)));\n");
    s.push_str("        }\n");
    s.push_str("        sb.append('\"');\n");
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

    // FNV-1a output hasher for want_hash mode (server_verify / issue #115).
    // Byte-for-byte identical to fuzz_data.h / the Rust fuzz port: FNV-1a over
    // each value's LITTLE-ENDIAN raw bytes (doubleToRawLongBits preserves -0.0
    // and NaN payloads) + the fmix64 finalizer. Java's fdlibm makes transcendental
    // functions differ from the C libm by ~1 ULP, so the driver hashes only the
    // non-transcendental functions bitwise and tolerances the rest.
    s.push_str("    static long svHashInit() { return 1469598103934665603L; }\n");
    s.push_str("    static long svHashF64(long h, double[] a, int n) {\n");
    s.push_str("        for (int i = 0; i < n; i++) {\n");
    s.push_str("            long bits = Double.doubleToRawLongBits(a[i]);\n");
    s.push_str("            for (int b = 0; b < 8; b++) { h ^= (bits >>> (8 * b)) & 0xffL; h *= 1099511628211L; }\n");
    s.push_str("        }\n");
    s.push_str("        return h;\n");
    s.push_str("    }\n");
    s.push_str("    static long svHashI32(long h, int[] a, int n) {\n");
    s.push_str("        for (int i = 0; i < n; i++) {\n");
    s.push_str("            int bits = a[i];\n");
    s.push_str("            for (int b = 0; b < 4; b++) { h ^= (bits >>> (8 * b)) & 0xffL; h *= 1099511628211L; }\n");
    s.push_str("        }\n");
    s.push_str("        return h;\n");
    s.push_str("    }\n");
    s.push_str("    static long svHashFin(long h) {\n");
    s.push_str("        h ^= h >>> 33; h *= 0xFF51AFD7ED558CCDL;\n");
    s.push_str("        h ^= h >>> 33; h *= 0xC4CEB9FE1A85EC53L;\n");
    s.push_str("        h ^= h >>> 33; return h;\n");
    s.push_str("    }\n\n");

    // jsonString — extract a string field's value (funcName for the ta_abstract RPCs).
    s.push_str("    static String jsonString(String json, String field) {\n");
    s.push_str("        int idx = json.indexOf('\"' + field + '\"');\n");
    s.push_str("        if (idx < 0) return \"\";\n");
    s.push_str("        idx = json.indexOf(':', idx) + 1;\n");
    s.push_str("        while (idx < json.length() && (json.charAt(idx) == ' ' || json.charAt(idx) == '\"')) idx++;\n");
    s.push_str("        int end = idx;\n");
    s.push_str("        while (end < json.length() && json.charAt(end) != '\"' && json.charAt(end) != ',' && json.charAt(end) != '}') end++;\n");
    s.push_str("        return json.substring(idx, end);\n");
    s.push_str("    }\n\n");

    // ta_abstract metadata table + introspection RPC handlers (issue #114).
    s.push_str(&crate::backends::java_abstract::generate(funcs, enums));

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

    // stream_verify MUST dispatch before the per-function chain: its funcName
    // is TA_-prefixed, so the contains("\"TA_<NAME>\"") probes below would
    // misroute it to handle_<NAME> (the C server orders the same way).
    s.push_str("        else if (json.contains(\"\\\"stream_verify\\\"\")) return handle_stream_verify(json);\n");
    s.push_str("        else if (json.contains(\"\\\"fuzz_in_hash\\\"\")) return handle_fuzz_in_hash(json);\n");

    // Thin dispatch: each indicator delegates to its own static handle_XXX method.
    // This keeps handleRequest small enough for HotSpot C2 to JIT-compile it.
    for func in funcs {
        let method_name = format!("TA_{}", func.name);
        s.push_str(&format!(
            "        else if (json.contains(\"\\\"{method_name}\\\"\")) return handle_{}(json);\n",
            func.name
        ));
    }

    // gencode_digest — the two stamps ta_regtest compares. Java only: every other
    // backend's server compiles or links the shipped artifact, so it has no second
    // text that could drift (#322). Reads the SHIPPED constant off the loaded
    // class, never a source file, which is what makes a stale class directory
    // visible here.
    s.push_str("        else if (json.contains(\"\\\"gencode_digest\\\"\")) {\n");
    s.push_str("            return \"{\\\"spliced\\\":\\\"\" + SPLICED_GENCODE_DIGEST\n");
    s.push_str("                 + \"\\\",\\\"shipped\\\":\\\"\" + io.github.talib.BuildStamp.GENCODE_DIGEST\n");
    s.push_str("                 + \"\\\"}\";\n");
    s.push_str("        }\n");

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
    s.push_str("            rideGen++;\n");
    s.push_str("            int id = jsonInt(json, \"id\");\n");
    s.push_str("            int period = jsonInt(json, \"period\");\n");
    // The same 0..=MAX_INDEX domain the C library enforces. Checked before any
    // store, so a rejected call leaves every slot as it was (#186).
    s.push_str("            if (period < 0 || period > Core.MAX_INDEX) {\n");
    s.push_str("                return \"{\\\"error\\\":\\\"Invalid unstable period value\\\"}\"; \n");
    s.push_str("            }\n");
    // FuncUnstId.ALL is the "set all" sentinel (matches C TA_SetUnstablePeriod).
    s.push_str("            if (id == FuncUnstId.ALL.value()) {\n");
    s.push_str("                for (int i = 0; i < core.unstablePeriod.length; i++) core.unstablePeriod[i] = period;\n");
    s.push_str("                return \"{\\\"status\\\":\\\"ok\\\"}\"; \n");
    s.push_str("            }\n");
    s.push_str("            if (id >= 0 && id < core.unstablePeriod.length) {\n");
    s.push_str("                core.unstablePeriod[id] = period;\n");
    s.push_str("                return \"{\\\"status\\\":\\\"ok\\\"}\"; \n");
    s.push_str("            }\n");
    s.push_str("            return \"{\\\"error\\\":\\\"Invalid id\\\"}\"; \n");
    s.push_str("        }\n");

    // set_candle_settings (#215). The C server delegates to the library and just
    // reports its RetCode; this server has no such library to ask, so it spells
    // out the same domain TA_SetCandleSettings enforces — settingType names a
    // single setting (the AllCandleSettings wildcard is NOT a target), rangeType
    // is 0..2, avgPeriod is a lookback and bounded like one, and factor is any
    // non-NaN value. Every check precedes the write, so a rejected call leaves
    // all eleven settings as they were (#186).
    s.push_str("        else if (json.contains(\"\\\"set_candle_settings\\\"\")) {\n");
    s.push_str("            rideGen++;\n");
    s.push_str("            int settingType = jsonInt(json, \"settingType\");\n");
    s.push_str("            int rangeType = jsonInt(json, \"rangeType\");\n");
    s.push_str("            int avgPeriod = jsonInt(json, \"avgPeriod\");\n");
    s.push_str("            double factor = jsonF64Bits(json, \"factorBits\", 1.0);\n");
    s.push_str("            if (settingType < 0 || settingType >= CandleSettingType.AllCandleSettings.ordinal()) {\n");
    s.push_str("                return \"{\\\"error\\\":\\\"Invalid candle setting\\\"}\";\n");
    s.push_str("            }\n");
    s.push_str("            if (rangeType < 0 || rangeType > RangeType.Shadows.ordinal()) {\n");
    s.push_str("                return \"{\\\"error\\\":\\\"Invalid candle setting\\\"}\";\n");
    s.push_str("            }\n");
    s.push_str("            if (avgPeriod < 0 || avgPeriod > Core.MAX_INDEX) {\n");
    s.push_str("                return \"{\\\"error\\\":\\\"Invalid candle setting\\\"}\";\n");
    s.push_str("            }\n");
    s.push_str("            if (Double.isNaN(factor)) {\n");
    s.push_str("                return \"{\\\"error\\\":\\\"Invalid candle setting\\\"}\";\n");
    s.push_str("            }\n");
    s.push_str("            core.candleSettings[settingType] =\n");
    s.push_str("                new CandleSetting(RangeType.values()[rangeType], avgPeriod, factor);\n");
    s.push_str("            return \"{\\\"status\\\":\\\"ok\\\"}\";\n");
    s.push_str("        }\n");

    // restore_candle_default_settings (#215). AllCandleSettings IS a legal
    // argument here — it is the wildcard that set_candle_settings rejects.
    s.push_str("        else if (json.contains(\"\\\"restore_candle_default_settings\\\"\")) {\n");
    s.push_str("            rideGen++;\n");
    s.push_str("            int settingType = jsonInt(json, \"settingType\");\n");
    s.push_str("            if (settingType < 0 || settingType > CandleSettingType.AllCandleSettings.ordinal()) {\n");
    s.push_str("                return \"{\\\"error\\\":\\\"Invalid candle setting type\\\"}\";\n");
    s.push_str("            }\n");
    s.push_str("            if (settingType == CandleSettingType.AllCandleSettings.ordinal()) {\n");
    s.push_str("                System.arraycopy(Core.DEFAULT_CANDLE_SETTINGS, 0, core.candleSettings, 0,\n");
    s.push_str("                    core.candleSettings.length);\n");
    s.push_str("            } else {\n");
    s.push_str("                core.candleSettings[settingType] = Core.DEFAULT_CANDLE_SETTINGS[settingType];\n");
    s.push_str("            }\n");
    s.push_str("            return \"{\\\"status\\\":\\\"ok\\\"}\";\n");
    s.push_str("        }\n");

    // eval_predicate method — boolean near-zero builtin on each input value.
    s.push_str("        else if (json.contains(\"\\\"eval_predicate\\\"\")) {\n");
    s.push_str("            int which = jsonInt(json, \"which\");\n");
    s.push_str("            double[] values = jsonDoubleArray(json, \"values\");\n");
    s.push_str("            double[] scale = jsonDoubleArray(json, \"scale\");\n");
    s.push_str("            int n = values.length;\n");
    s.push_str("            int[] out = new int[n];\n");
    s.push_str("            for (int i = 0; i < n; i++) {\n");
    s.push_str("                double v = values[i];\n");
    s.push_str("                double s = (i < scale.length) ? scale[i] : 0.0;\n");
    s.push_str("                boolean r;\n");
    s.push_str(&format!(
        "                if (which == 1) r = {};\n",
        java_predicate_expr(SpecialBuiltin::IsZeroScaled, &["v".to_string(), "s".to_string()])
    ));
    s.push_str(&format!(
        "                else if (which == 2) r = {};\n",
        java_predicate_expr(SpecialBuiltin::IsZeroOrNeg, &["v".to_string()])
    ));
    s.push_str(&format!(
        "                else r = {};\n",
        java_predicate_expr(SpecialBuiltin::IsZero, &["v".to_string()])
    ));
    s.push_str("                out[i] = r ? 1 : 0;\n");
    s.push_str("            }\n");
    s.push_str("            return \"{\\\"outInteger\\\":\" + intArrayToJson(out, n) + \"}\";\n");
    s.push_str("        }\n");

    // ta_abstract introspection RPCs (issue #114) — metadata parity with C/Rust via test_abstract.c.
    s.push_str("        else if (json.contains(\"\\\"TA_GetFuncInfo\\\"\")) return handleGetFuncInfo(json);\n");
    s.push_str("        else if (json.contains(\"\\\"TA_GetInputParameterInfo\\\"\")) return handleGetInputParameterInfo(json);\n");
    s.push_str("        else if (json.contains(\"\\\"TA_GetOptInputParameterInfo\\\"\")) return handleGetOptInputParameterInfo(json);\n");
    s.push_str("        else if (json.contains(\"\\\"TA_GetOutputParameterInfo\\\"\")) return handleGetOutputParameterInfo(json);\n");
    s.push_str("        else if (json.contains(\"\\\"abstract_for_each_func\\\"\")) return handleForEachFunc();\n");
    s.push_str("        else if (json.contains(\"\\\"TA_FunctionDescriptionXML\\\"\")) return handleFunctionDescriptionXML();\n");
    s.push_str("        else if (json.contains(\"\\\"abstract_call\\\"\")) return handleAbstractCall(json);\n");
    s.push_str("        else if (json.contains(\"\\\"abstract_get_lookback\\\"\")) return \"{\\\"lookback\\\":\" + computeLookback(jsonString(json, \"funcName\"), json) + \"}\";\n");

    s.push_str("        else {\n");
    s.push_str("            return \"{\\\"error\\\":\\\"Unknown method\\\"}\";\n");
    s.push_str("        }\n");
    s.push_str("    }\n\n");

    // Per-function handler methods — each is small enough for C2 JIT compilation.
    for func in funcs {
        let func_base = func.name.clone();
        let func_base_camel = crate::backends::common::camel_words(&func.name);
        let func_stream_class = crate::backends::java_stream::stream_class_name(func);

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

        // Optional params. `_optRejected` (issue #256) catches an out-of-list
        // enum value BEFORE it reaches `.values()[...]` -- unlike the sv_<func>
        // and absBind()/computeLookback() paths, nothing here wrapped that index
        // in a try/catch, so an out-of-range optInMAType threw
        // ArrayIndexOutOfBoundsException out of the JSON parse itself, before the
        // library's own exception normalisation ever ran. Declared unconditionally
        // (read unconditionally below) rather than only for functions with an enum
        // param, so it is never an unused local either way.
        s.push_str("        boolean _optRejected = false;\n");
        for opt in &func.optional_inputs {
            if opt.param_type == ParamType::Real {
                s.push_str(&format!(
                    "        double {} = jsonDouble(json, \"{}\");\n",
                    opt.name, opt.name
                ));
            } else if let ParamType::Enum(ref enum_name) = opt.param_type {
                // Enum params: read as a raw int first, and reject out-of-list
                // before converting to the enum type. On rejection the enum local
                // is bound to a placeholder in-range value (index 0) purely so the
                // rest of this method still compiles and runs its normal shape;
                // `_optRejected` is what actually forces the BadParam response
                // below, and the placeholder is never observed in one.
                s.push_str(&format!(
                    "        int _raw_{0} = jsonInt(json, \"{0}\");\n\
                     \x20       if (_raw_{0} < 0 || _raw_{0} >= {1}.values().length) _optRejected = true;\n\
                     \x20       {1} {0} = {1}.values()[_optRejected ? 0 : _raw_{0}];\n",
                    opt.name, enum_name
                ));
            } else {
                s.push_str(&format!(
                    "        int {} = jsonInt(json, \"{}\");\n",
                    opt.name, opt.name
                ));
            }
        }

        // Apply unstable period if provided
        if let Some(id) = func_unst_id(&func.name, enums) {
            s.push_str(&format!(
                "        core.unstablePeriod[{id}] = jsonInt(json, \"unstablePeriod\");\n"
            ));
        }

        // Outputs — one array per output, typed correctly (double[] or int[])
        let outputs = &func.outputs;
        {
            let lb_args: Vec<String> =
                func.optional_inputs.iter().map(|o| o.name.clone()).collect();
            s.push_str(&doc_produced_extent("        ", "//"));
            s.push_str(&format!(
                "        int _lb = core.{func_base}_Lookback({});\n",
                lb_args.join(", ")
            ));
            s.push_str("        int _cs = startIdx > _lb ? startIdx : _lb;\n");
            s.push_str("        int _outLen = ((_lb < 0 || _cs > endIdx) ? 1 : endIdx - _cs + 1) + jsonInt(json, \"out_pad\");\n");
        }
        for (k, out) in outputs.iter().enumerate() {
            let arr_name = format!("outArr{k}");
            if out.param_type == ParamType::Integer {
                s.push_str(&format!(
                    "        int[] {arr_name} = new int[_outLen];\n"
                ));
            } else {
                s.push_str(&format!(
                    "        double[] {arr_name} = new double[_outLen];\n"
                ));
            }
        }
        s.push_str("        MInteger outBegIdx = new MInteger();\n");
        s.push_str("        MInteger outNBElement = new MInteger();\n");
        s.push_str("        RetCode rc = RetCode.Success;\n");

        // Benchmark iteration loop with timing. Iteration 0 is always a
        // discarded warm-up — see the C emitter: it removes the cold-call bias
        // AND makes every correctness gate an idempotency check.
        s.push_str("        int bench_mode = jsonInt(json, \"bench_mode\");\n");
        // Right-sized input views for the warm-up arms, bound ONCE outside the
        // timing loop. Java derives historyLen from array.length, and with
        // use_preloaded the buffer is `new double[MAX_ARRAY_SIZE]` with only
        // refN points copied in -- passing it whole replays a fixed, oversized
        // history regardless of --points (the C arm passes `endIdx + 1`).
        for name in &input_names {
            s.push_str(&format!(
                "        double[] _warm_{name} = bench_mode == 0 ? null : java.util.Arrays.copyOfRange({name}, 0, endIdx + 1);\n"
            ));
        }
        s.push_str("        long startNs = 0;\n");
        s.push_str("        for (int _bi = 0; _bi <= bench_iters; _bi++) {\n");
        s.push_str("        if (_bi == 1) startNs = System.nanoTime();\n");

        // Call
        // ---- Correctness goes through the PUBLIC API; the benchmark does not.
        //
        // A correctness request calls the tier a user can actually reach, and
        // the exception is normalised HERE, in the server, rather than by the
        // library pre-flattening it: a thrown failure carries its code, the
        // server reads it and reports the retCode / outBegIdx / outNBElement
        // wire shape (#236 steps 1 and 4).
        //
        // A request that declares itself TIMED (`"timed":1`, which only ta_bench
        // sends) calls the BODY -- the numerics and nothing else -- inside the
        // timed loop. These servers ARE the cross-language benchmark, and
        // nothing measured may quietly acquire the public tier's argument
        // checks.
        //
        // Declared, not inferred from `iters > 1`: `ta_bench --iters=1` is a
        // legitimate invocation, and inferring would have made it measure the
        // public tier in Java and C# while C and Rust stayed on their single
        // one -- a tier switch nothing in the output would mention.
        //
        // Only the library's OWN failure is converted. An out-of-bounds access
        // or an allocation failure is not something C can produce, so it is not
        // something to report as a code — it escapes to the top-level handler
        // and the driver treats the error response as the divergence it is.
        {
            let mut pub_args = String::from("startIdx, endIdx");
            let mut core_args = String::from("startIdx, endIdx");
            for name in &input_names {
                pub_args.push_str(&format!(", {name}"));
                core_args.push_str(&format!(", {name}"));
            }
            for opt in &func.optional_inputs {
                pub_args.push_str(&format!(", {}", opt.name));
                core_args.push_str(&format!(", {}", opt.name));
            }
            core_args.push_str(", outBegIdx, outNBElement");
            for k in 0..outputs.len() {
                pub_args.push_str(&format!(", outArr{k}"));
                core_args.push_str(&format!(", outArr{k}"));
            }
            s.push_str("        if (bench_mode == 0) {\n");
            s.push_str("        if (jsonInt(json, \"timed\") != 0) {\n");
            s.push_str("            if (_optRejected) {\n");
            s.push_str("                rc = RetCode.BadParam;\n");
            s.push_str("                outBegIdx.value = 0;\n");
            s.push_str("                outNBElement.value = 0;\n");
            s.push_str("            } else {\n");
            s.push_str("            try {\n");
            s.push_str(&format!("                rc = core.{func_base}_Impl({core_args});\n"));
            s.push_str("            } catch (RuntimeException _e) {\n");
            s.push_str("                if (!(_e instanceof TaLibFailure)) throw _e;\n");
            s.push_str("                rc = ((TaLibFailure) _e).retCode();\n");
            s.push_str("                outBegIdx.value = 0;\n");
            s.push_str("                outNBElement.value = 0;\n");
            s.push_str("            }\n");
            s.push_str("            }\n");
            s.push_str("        } else {\n");
            s.push_str("            if (_optRejected) {\n");
            s.push_str("                rc = RetCode.BadParam;\n");
            s.push_str("                outBegIdx.value = 0;\n");
            s.push_str("                outNBElement.value = 0;\n");
            s.push_str("            } else {\n");
            s.push_str("            try {\n");
            s.push_str(&format!("                OutRange _pr = core.{func_base}({pub_args});\n"));
            s.push_str("                outBegIdx.value = _pr.begIdx();\n");
            s.push_str("                outNBElement.value = _pr.count();\n");
            s.push_str("                rc = RetCode.Success;\n");
            s.push_str("            } catch (RuntimeException _e) {\n");
            s.push_str("                if (!(_e instanceof TaLibFailure)) throw _e;\n");
            s.push_str("                rc = ((TaLibFailure) _e).retCode();\n");
            s.push_str("                outBegIdx.value = 0;\n");
            s.push_str("                outNBElement.value = 0;\n");
            s.push_str("            }\n");
            s.push_str("            }\n");
            s.push_str("        }\n");
            s.push_str("        }\n");
        }
        // --- warm-up arms (ta_bench --mode=open / openfill). Java handles are
        // GC-managed (no Close) and the public Open throws instead of returning
        // a code, so the arms convert the throw into a RetCode.
        {
            // Java derives historyLen from array.length, and with
            // use_preloaded the buffer is `new double[MAX_ARRAY_SIZE]` with only
            // refN points copied in -- passing it whole replays a fixed,
            // oversized history regardless of --points. Bind a right-sized view
            // once, outside the timing loop, so the arm measures the same range
            // the batch call does (the C arm passes `endIdx + 1`).
            // Join once: a function with no optional params would otherwise
            // emit `Open(inReal, )`.
            let mut open_args: Vec<String> = input_names.iter().map(|n| format!("_warm_{n}")).collect();
            for opt in &func.optional_inputs {
                open_args.push(opt.name.clone());
            }
            let ins = open_args.join(", ");
            let mut fill_args = open_args.clone();
            for k in 0..outputs.len() {
                fill_args.push(format!("outArr{k}"));
            }
            let fill = fill_args.join(", ");
            s.push_str("        else if (_optRejected) { rc = RetCode.BadParam; }\n");
            s.push_str("        else { try {\n");
            s.push_str("            if (bench_mode == 1) {\n");
            s.push_str(&format!(
                "                core.{func_base_camel}Open({ins});\n"
            ));
            s.push_str("            } else {\n");
            // The fill reports its range via the returned handle's outRange()
            // (issue #256) -- unpack it into the same two locals the batch arm
            // sets, which the response builder below reads. Discarding the
            // handle (as before) left outBegIdx/outNBElement at whatever the
            // batch/float legs happened to leave them, invisible to ta_bench
            // (timing-only) but wrong for anything that reads the value, same
            // shape as the Rust arm's `Ok((_h, r)) => outBegIdx = r.beg_idx`.
            s.push_str(&format!(
                "                Core.{func_stream_class} _wh = core.{func_base_camel}OpenAndFill({fill});\n\
                 \x20               outBegIdx.value = _wh.outRange().begIdx();\n\
                 \x20               outNBElement.value = _wh.outRange().count();\n"
            ));
            s.push_str("            }\n");
            s.push_str("            rc = RetCode.Success;\n");
            // Report the code the open actually raised, not a stand-in. Every
            // failure the library throws carries it (#236 step 1); anything else
            // reaching here is not the library's and stays the catch-all.
            s.push_str("        } catch (RuntimeException _e) { rc = _e instanceof TaLibFailure ? ((TaLibFailure)_e).retCode() : RetCode.BadParam; } }\n");
        }
        s.push_str("        }\n"); // end bench_iters loop

        // Timing capture
        s.push_str("        long elapsedNs = (System.nanoTime() - startNs) / bench_iters;\n");

        // Float-variant leg ("use_float":1): re-run through the float[] overload
        // of the same core, over the same output buffers, so the response carries
        // the single-precision result. Mirrors the C server's TA_S_ leg. Without
        // it the 168 shipped float overloads have no value verification at all.
        s.push_str("        int usedFloat = 0;\n");
        s.push_str("        if (jsonInt(json, \"use_float\") != 0) {\n");
        for name in &input_names {
            s.push_str(&format!(
                "            float[] f_{name} = new float[{name}.length];\n\
                 \x20           for (int _fi = 0; _fi < {name}.length; _fi++) f_{name}[_fi] = (float){name}[_fi];\n"
            ));
        }
        // The float leg is a CORRECTNESS leg, so it takes the public overload
        // for the same reason the double one does. Normalised here, same shape.
        s.push_str("            if (_optRejected) {\n");
        s.push_str("                rc = RetCode.BadParam;\n");
        s.push_str("                outBegIdx.value = 0;\n");
        s.push_str("                outNBElement.value = 0;\n");
        s.push_str("            } else {\n");
        s.push_str("            try {\n");
        {
            let mut f_args = String::from("startIdx, endIdx");
            for name in &input_names {
                f_args.push_str(&format!(", f_{name}"));
            }
            for opt in &func.optional_inputs {
                f_args.push_str(&format!(", {}", opt.name));
            }
            for k in 0..outputs.len() {
                f_args.push_str(&format!(", outArr{k}"));
            }
            s.push_str(&format!("                OutRange _fr = core.{func_base}({f_args});\n"));
            s.push_str("                outBegIdx.value = _fr.begIdx();\n");
            s.push_str("                outNBElement.value = _fr.count();\n");
            s.push_str("                rc = RetCode.Success;\n");
            s.push_str("            } catch (RuntimeException _e) {\n");
            s.push_str("                if (!(_e instanceof TaLibFailure)) throw _e;\n");
            s.push_str("                rc = ((TaLibFailure) _e).retCode();\n");
            s.push_str("                outBegIdx.value = 0;\n");
            s.push_str("                outNBElement.value = 0;\n");
            s.push_str("            }\n");
        }
        s.push_str("            }\n");
        s.push_str("            usedFloat = 1;\n");
        s.push_str("        }\n");

        // want_hash mode (server_verify / issue #115): digest of the GUARDED
        // output (like-for-like with the in-process C golden's TA_CallFunc),
        // returned before the value response is built.
        s.push_str("        if (jsonInt(json, \"want_hash\") != 0 && jsonInt(json, \"full_output\") == 0) {\n");
        s.push_str("            long _h = svHashInit();\n");
        s.push_str("            if (rc == RetCode.Success && outNBElement.value > 0) {\n");
        for (k, out) in outputs.iter().enumerate() {
            if out.param_type == ParamType::Integer {
                s.push_str(&format!(
                    "                _h = svHashI32(_h, outArr{k}, outNBElement.value);\n"
                ));
            } else {
                s.push_str(&format!(
                    "                _h = svHashF64(_h, outArr{k}, outNBElement.value);\n"
                ));
            }
        }
        s.push_str("            }\n");
        s.push_str("            _h = svHashFin(_h);\n");
        s.push_str("            StringBuilder hb = new StringBuilder();\n");
        s.push_str("            hb.append(\"{\\\"retCode\\\":\").append(rc.toInt()).append(\",\\\"outBegIdx\\\":\").append(outBegIdx.value).append(\",\\\"outNBElement\\\":\").append(outNBElement.value).append(\",\\\"out_hash\\\":\\\"\").append(String.format(\"%016x\", _h)).append(\"\\\"\");\n");
        if func.streaming {
            let mut ride_args = String::new();
            for name in &input_names {
                let _ = write!(ride_args, "{name}, ");
            }
            for opt in &func.optional_inputs {
                let _ = write!(ride_args, "{}, ", opt.name);
            }
            s.push_str(&format!("            ride{}(core, json, endIdx, {}hb);\n", func.name, ride_args));
        }
        s.push_str("            hb.append(\"}\");\n");
        s.push_str("            return hb.toString();\n");
        s.push_str("        }\n");


        // Response — use correct key names and serialisers per output type
        s.push_str("        StringBuilder sb = new StringBuilder();\n");
        s.push_str("        sb.append(\"{\\\"retCode\\\":\").append(rc.toInt());\n");
        s.push_str(
            "        sb.append(\",\\\"outBegIdx\\\":\").append(outBegIdx.value);\n",
        );
        s.push_str(
            "        sb.append(\",\\\"outNBElement\\\":\").append(outNBElement.value);\n",
        );
        // The length the server ACTUALLY allocated. The harness asserts it
        // EXCEEDS the produced count on the padded leg -- otherwise the
        // "slack is legal" floor would be testing the harness's own intent, and
        // an out_pad the server silently ignored would read as coverage.
        s.push_str("        sb.append(\",\\\"out_len\\\":\").append(_outLen);\n");
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
        s.push_str("        sb.append(\",\\\"used_float\\\":\").append(usedFloat);\n");
        s.push_str("        sb.append(\",\\\"timing_ns\\\":\").append(elapsedNs);\n");
        if func.streaming {
            let mut ride_args = String::new();
            for name in &input_names {
                let _ = write!(ride_args, "{name}, ");
            }
            for opt in &func.optional_inputs {
                let _ = write!(ride_args, "{}, ", opt.name);
            }
            s.push_str(&format!(
                "        ride{}(core, json, endIdx, {}sb);\n",
                func.name, ride_args
            ));
        }
        s.push_str("        sb.append(\"}\");\n");
        s.push_str("        return sb.toString();\n");

        s.push_str("    }\n\n");
    }

    // ta_abstract dynamic dispatch. Both RPCs bind through the SHIPPED registry
    // (io.github.talib.metadata) rather than a server-private switch, so
    // test_abstract.c exercises the artifact that ships. Fully-qualified names
    // throughout: this file carries its own default-package Core/MAType/RetCode
    // twins, and importing the shipped ones would be ambiguous.
    //
    // Java's public API is exception-based -- Core returns OutRange and throws --
    // so the retCode C reports is reconstituted at this boundary. That is a
    // spelling difference the contract tolerates; what must match, and does, is
    // WHICH calls are rejected.
    s.push_str(r#"    static int computeLookback(String funcName, String json) {
        io.github.talib.metadata.FunctionInfo f = io.github.talib.metadata.Functions.byName(funcName);
        if (f == null) return -1;
        try {
            return absBind(f, json, null).lookback();
        } catch (RuntimeException e) {
            return -1;
        }
    }

    /* Binds every declared parameter of `f` from the request. `outs` receives the
       output arrays when the caller needs them back; pass null for the lookback
       tier, which binds none. */
    static io.github.talib.metadata.ParamHolder absBind(
            io.github.talib.metadata.FunctionInfo f, String json, Object[] outs) {
        io.github.talib.metadata.ParamHolder h = f.newCall();
        int startIdx = jsonInt(json, "startIdx");
        int endIdx = jsonInt(json, "endIdx");
        int n = endIdx - startIdx + 1;
        if (n < 1) n = 1;

        for (int i = 0; i < f.inputs().size(); i++) {
            io.github.talib.metadata.InputInfo in = f.inputs().get(i);
            switch (in.type()) {
                case PRICE -> h.setPriceInput(i,
                    jsonDoubleArray(json, "inOpen"), jsonDoubleArray(json, "inHigh"),
                    jsonDoubleArray(json, "inLow"), jsonDoubleArray(json, "inClose"),
                    jsonDoubleArray(json, "inVolume"), jsonDoubleArray(json, "inOpenInterest"));
                case REAL -> h.setInput(i, absRealInput(json, f, i));
                case INTEGER -> {
                    double[] raw = absRealInput(json, f, i);
                    int[] ints = new int[raw.length];
                    for (int k = 0; k < raw.length; k++) ints[k] = (int) raw[k];
                    h.setInput(i, ints);
                }
            }
        }

        for (int i = 0; i < f.optInputs().size(); i++) {
            io.github.talib.metadata.OptInputInfo o = f.optInputs().get(i);
            switch (o.type()) {
                case REAL_RANGE, REAL_LIST -> h.setOptInput(i, jsonDouble(json, o.paramName()));
                default -> h.setOptInput(i, jsonInt(json, o.paramName()));
            }
        }

        if (outs != null) {
            for (int k = 0; k < f.outputs().size(); k++) {
                if (f.outputs().get(k).type() == io.github.talib.metadata.OutputType.REAL) {
                    double[] a = new double[n];
                    outs[k] = a;
                    h.setOutput(k, a);
                } else {
                    int[] a = new int[n];
                    outs[k] = a;
                    h.setOutput(k, a);
                }
            }
        }
        return h;
    }

    /* inReal / inReal0 / inReal1, matching the driver's key scheme. */
    static double[] absRealInput(String json, io.github.talib.metadata.FunctionInfo f, int slot) {
        int generic = 0;
        for (int i = 0; i < slot; i++) {
            if (f.inputs().get(i).type() != io.github.talib.metadata.InputType.PRICE) generic++;
        }
        int total = 0;
        for (int i = 0; i < f.inputs().size(); i++) {
            if (f.inputs().get(i).type() != io.github.talib.metadata.InputType.PRICE) total++;
        }
        return jsonDoubleArray(json, total == 1 ? "inReal" : ("inReal" + generic));
    }

    static String handleAbstractCall(String json) {
        String fn = jsonString(json, "funcName");
        io.github.talib.metadata.FunctionInfo f = io.github.talib.metadata.Functions.byName(fn);
        if (f == null) return "{\"error\":\"Unknown function\"}";

        Object[] outs = new Object[f.outputs().size()];
        int lb;
        int rc = 0;
        int beg = 0;
        int nb = 0;
        try {
            io.github.talib.metadata.ParamHolder h = absBind(f, json, outs);
            lb = h.lookback();
            io.github.talib.OutRange r = h.call(jsonInt(json, "startIdx"), jsonInt(json, "endIdx"));
            beg = r.begIdx();
            nb = r.count();
        } catch (RuntimeException e) {
            /* The shipped binder signals a rejected call by throwing; C's
               TA_CallFunc returns TA_BAD_PARAM for the same conditions. */
            lb = -1;
            rc = 2;
        }

        StringBuilder b = new StringBuilder();
        b.append("{\"lookback\":").append(lb)
         .append(",\"retCode\":").append(rc)
         .append(",\"outBegIdx\":").append(beg)
         .append(",\"outNBElement\":").append(nb);
        /* Real and integer outputs are numbered INDEPENDENTLY, each from its own
           counter -- MINMAXINDEX has two integer outputs, so one shared "outInteger"
           key made the second overwrite the first. Matches the driver's scheme in
           test_abstract.c. */
        int realIdx = 0;
        int intIdx = 0;
        for (int k = 0; k < f.outputs().size(); k++) {
            boolean isReal = f.outputs().get(k).type() == io.github.talib.metadata.OutputType.REAL;
            String key;
            if (isReal) {
                key = realIdx == 0 ? "outReal" : ("outReal" + realIdx);
                realIdx++;
            } else {
                key = intIdx == 0 ? "outInteger" : ("outInteger" + intIdx);
                intIdx++;
            }
            b.append(",\"").append(key).append("\":");
            if (isReal) b.append(doubleArrayToJson((double[]) outs[k], nb));
            else b.append(intArrayToJson((int[]) outs[k], nb));
        }
        b.append('}');
        return b.toString();
    }

"#);

    // stream_verify: Java stream vs Java batch, bitwise (drives the ta_regtest
    // stream pass the moment the capability probe sees "not_streamable").
    s.push_str(&crate::stream_verify_gen::java::generate_java_stream_verify(funcs, enums));
    s.push_str(&crate::ride_gen::java::generate_java_ridealong(funcs));

    // Main method
    s.push_str("    public static void main(String[] args) throws Exception {\n");
    s.push_str(
        "        BufferedReader reader = new BufferedReader(new InputStreamReader(System.in));\n",
    );
    s.push_str("        String line;\n");
    s.push_str("        while ((line = reader.readLine()) != null) {\n");
    s.push_str("            if (line.trim().isEmpty()) continue;\n");
    // An escaping exception kills the JVM, and the driver then reports a
    // pipe-read failure for every REMAINING function instead of naming the one
    // request that broke. That is exactly how MAType.values()[Integer.MIN_VALUE]
    // presented (issue #164): a dead pipe, not a diagnosable answer.
    s.push_str("            String reply;\n");
    s.push_str("            try { reply = handleRequest(line); }\n");
    s.push_str("            catch (Throwable t) {\n");
    s.push_str("                reply = \"{\\\"error\\\":\" + absStr(t.getClass().getName() + \": \" + t.getMessage()) + \"}\";\n");
    s.push_str("            }\n");
    s.push_str("            System.out.println(reply);\n");
    s.push_str("            System.out.flush();\n");
    s.push_str("        }\n");
    s.push_str("    }\n");
    s.push_str("}\n\n");

    // Stream scaffolding: the verified FuzzData port + the typed open-reject
    // exception (top-level classes after the server class).
    s.push_str(&java_server_stream_scaffolding());

    s
}

/// Generate the managed C# JSON-RPC server source file.
///
/// Emits a complete C# program whose csproj (see [`csharp_server_csproj`])
/// compiles the **shipped library sources directly** — the same `.cs` files
/// `output/csharp/library/` ships, not a spliced copy — so what the
/// cross-language harness measures is byte-for-byte the shipped code. That is
/// the same-text identity proof `inline_java_core_methods` gives Java, without
/// the splice: C#'s `partial class Core` makes the server's `Core` and the
/// library's the same type.
///
/// Coverage rule (no P/Invoke fallback, ever): every function dispatches to
/// the managed core or errors. A hybrid server would let most functions
/// vacuously "pass" by really being the C library — the exact failure mode
/// this project bans.
#[allow(clippy::too_many_lines, clippy::cognitive_complexity)]
#[allow(clippy::implicit_hasher)]
pub fn generate_csharp_server(funcs: &[FuncDef], enums: &HashMap<String, EnumDef>) -> String {
    // Resolve `PRAGMA TA_ALT` for this language before anything reads a body.
    let resolved = crate::ir::resolve_all(funcs, crate::ir::Lang::CSharp);
    let funcs: &[FuncDef] = &resolved;
    let mut s = String::new();

    s.push_str("// Auto-generated JSON-RPC server for ta_codegen C# output (managed).\n");
    s.push_str("// The csproj compiles the shipped library sources from ../library — the\n");
    s.push_str("// server's Core IS the shipped partial class, not a copy.\n");
    s.push_str("using System;\n");
    s.push_str("using System.Text.Json;\n");
    s.push_str("using System.Diagnostics;\n");
    s.push_str("using TALib;\n");
    s.push_str("using TALib.Metadata;\n\n");

    s.push_str("public class TaCodegenServe {\n");
    s.push_str("    static Core core = new Core();\n");
    s.push_str("    const int MAX_ARRAY_SIZE = 200000;\n");
    s.push_str("    static double[] refOpen = new double[MAX_ARRAY_SIZE];\n");
    s.push_str("    static double[] refHigh = new double[MAX_ARRAY_SIZE];\n");
    s.push_str("    static double[] refLow = new double[MAX_ARRAY_SIZE];\n");
    s.push_str("    static double[] refClose = new double[MAX_ARRAY_SIZE];\n");
    s.push_str("    static double[] refVolume = new double[MAX_ARRAY_SIZE];\n");
    s.push_str("    static double[] refOI = new double[MAX_ARRAY_SIZE];\n");
    s.push_str("    static int refN = 0;\n\n");

    // An empty `params` object for the RPCs that send none. Parsed once; the
    // JsonDocument is deliberately kept alive for the process lifetime, because
    // a JsonElement is only valid while its owning document is.
    s.push_str("    static readonly JsonDocument EmptyParamsDoc = JsonDocument.Parse(\"{}\");\n");
    s.push_str("    static JsonElement EmptyParams => EmptyParamsDoc.RootElement;\n\n");

    // Cross-platform high-resolution nanosecond timer via Stopwatch.
    // Split into whole-seconds + fractional to avoid long overflow.
    s.push_str("    static long GetNanoTime() {\n");
    s.push_str("        long ts = Stopwatch.GetTimestamp();\n");
    s.push_str("        long freq = Stopwatch.Frequency;\n");
    s.push_str("        return (ts / freq) * 1000000000L + (ts % freq) * 1000000000L / freq;\n");
    s.push_str("    }\n\n");

    // Tolerant JSON accessors (state methods lack most fields).
    s.push_str("    static int GetInt(JsonElement p, string name, int def) =>\n");
    s.push_str("        p.TryGetProperty(name, out var v) ? v.GetInt32() : def;\n\n");
    s.push_str("    static double GetDouble(JsonElement p, string name, double def) =>\n");
    s.push_str("        p.TryGetProperty(name, out var v) ? v.GetDouble() : def;\n\n");
    // One f64 as the 16 hex chars of its IEEE-754 bits — the scalar counterpart of
    // GetDoubleArray's transport (#115). Used for `factor`, which has to be able to
    // carry a NaN: NaN has no JSON number spelling, and refusing one is part of the
    // contract compared across languages (#215).
    s.push_str("    static double GetF64Bits(JsonElement p, string name, double def) {\n");
    s.push_str("        if (!p.TryGetProperty(name, out var v) || v.ValueKind != JsonValueKind.String)\n");
    s.push_str("            return def;\n");
    s.push_str("        string? h = v.GetString();\n");
    s.push_str("        if (h == null || h.Length != 16) return def;\n");
    s.push_str("        return ulong.TryParse(h, System.Globalization.NumberStyles.HexNumber,\n");
    s.push_str("                              System.Globalization.CultureInfo.InvariantCulture, out ulong bits)\n");
    s.push_str("            ? BitConverter.Int64BitsToDouble(unchecked((long)bits))\n");
    s.push_str("            : def;\n");
    s.push_str("    }\n\n");
    s.push_str("    static void LoadRef(JsonElement p, string name, double[] dst) {\n");
    s.push_str("        double[] tmp = GetDoubleArray(p, name);\n");
    s.push_str("        Array.Copy(tmp, dst, Math.Min(tmp.Length, MAX_ARRAY_SIZE));\n");
    s.push_str("    }\n\n");

    // Helper: extract double array from JSON. Lossless hex-bits transport
    // (issue #115): a string of concatenated 16-hex groups, each one double's
    // IEEE-754 bit pattern. Decoded exactly; every other caller sends an array.
    s.push_str("    static double[] GetDoubleArray(JsonElement p, string name) {\n");
    s.push_str("        if (!p.TryGetProperty(name, out var arr)) return Array.Empty<double>();\n");
    s.push_str("        if (arr.ValueKind == JsonValueKind.String) {\n");
    s.push_str("            string hex = arr.GetString()!;\n");
    s.push_str("            int cnt = hex.Length / 16;\n");
    s.push_str("            double[] r = new double[cnt];\n");
    s.push_str("            for (int i = 0; i < cnt; i++)\n");
    s.push_str("                r[i] = BitConverter.Int64BitsToDouble(unchecked((long)Convert.ToUInt64(hex.Substring(i * 16, 16), 16)));\n");
    s.push_str("            return r;\n");
    s.push_str("        }\n");
    s.push_str("        double[] result = new double[arr.GetArrayLength()];\n");
    s.push_str("        for (int i = 0; i < result.Length; i++)\n");
    s.push_str("            result[i] = arr[i].GetDouble();\n");
    s.push_str("        return result;\n");
    s.push_str("    }\n\n");

    // FNV-1a output hasher for want_hash mode (server_verify / issue #115),
    // byte-for-byte identical to fuzz_data.h: FNV-1a over each value's
    // LITTLE-ENDIAN raw bytes (DoubleToInt64Bits preserves -0.0 / NaN payloads)
    // + fmix64 finalizer. The managed library computes with the same IEEE ops
    // and correctly-rounded FusedMultiplyAdd, so this is compared bitwise
    // against the in-process C golden — zero tolerance.
    s.push_str("    static ulong SvHashInit() => 1469598103934665603UL;\n");
    s.push_str("    static ulong SvHashF64(ulong h, double[] a, int n) {\n");
    s.push_str("        for (int i = 0; i < n; i++) {\n");
    s.push_str("            long bits = BitConverter.DoubleToInt64Bits(a[i]);\n");
    s.push_str("            for (int b = 0; b < 8; b++) { h ^= (ulong)((bits >> (8 * b)) & 0xffL); h *= 1099511628211UL; }\n");
    s.push_str("        }\n");
    s.push_str("        return h;\n");
    s.push_str("    }\n");
    s.push_str("    static ulong SvHashI32(ulong h, int[] a, int n) {\n");
    s.push_str("        for (int i = 0; i < n; i++) {\n");
    s.push_str("            int bits = a[i];\n");
    s.push_str("            for (int b = 0; b < 4; b++) { h ^= (ulong)((bits >> (8 * b)) & 0xff); h *= 1099511628211UL; }\n");
    s.push_str("        }\n");
    s.push_str("        return h;\n");
    s.push_str("    }\n");
    s.push_str("    static ulong SvHashFin(ulong h) {\n");
    s.push_str("        h ^= h >> 33; h *= 0xFF51AFD7ED558CCDUL;\n");
    s.push_str("        h ^= h >> 33; h *= 0xC4CEB9FE1A85EC53UL;\n");
    s.push_str("        h ^= h >> 33; return h;\n");
    s.push_str("    }\n\n");

    // Array formatters. Real outputs ride the lossless hex-bits transport the
    // inputs have used since #115 -- one string of concatenated 16-hex-char
    // groups, each value's IEEE-754 bits. `double.ToString()` is
    // shortest-round-trip on modern .NET and InvariantGlobalization pins the
    // decimal separator, so it was correct today; hex bits make that a property
    // of the format instead of of the runtime's formatter (#257/#258), and give
    // NaN payloads and infinities a spelling decimal text has none for.
    s.push_str("    static string FormatArray(double[] arr, int count) {\n");
    s.push_str("        var parts = new string[count];\n");
    s.push_str("        for (int i = 0; i < count; i++)\n");
    s.push_str("            parts[i] = BitConverter.DoubleToInt64Bits(arr[i]).ToString(\"x16\");\n");
    s.push_str("        return \"\\\"\" + string.Concat(parts) + \"\\\"\";\n");
    s.push_str("    }\n\n");
    s.push_str("    static string FormatIntArray(int[] arr, int count) {\n");
    s.push_str("        var parts = new string[count];\n");
    s.push_str("        for (int i = 0; i < count; i++)\n");
    s.push_str("            parts[i] = arr[i].ToString();\n");
    s.push_str("        return \"[\" + string.Join(\",\", parts) + \"]\";\n");
    s.push_str("    }\n\n");

    // Dispatch method. NO blanket try/catch, deliberately: a .NET exception
    // out of an indicator core (index out of range, overflow, ...) must kill
    // the process so the driver's pipe read fails hard — the same crash
    // contract as Java (uncaught exception exits the JVM) and Rust (panic
    // aborts). A catch here would convert a crash into an {"error":...}
    // response, which every driver path treats as "unsupported — skip", and
    // a broken server would read as green (adversarial-review finding).
    s.push_str("    static string HandleRequest(string json) {\n");
    s.push_str("        using var doc = JsonDocument.Parse(json);\n");
    s.push_str("        var root = doc.RootElement;\n");
    s.push_str("        string method = root.GetProperty(\"method\").GetString()!;\n");
    // `params` is optional: TA_FunctionDescriptionXML is sent as a bare
    // {"method":...} with no params object (test_abstract.c). GetProperty would
    // throw KeyNotFoundException there, and the deliberate no-try/catch policy
    // below turns that into a process kill the driver reads as a pipe EOF.
    s.push_str("        var p = root.TryGetProperty(\"params\", out var pv) ? pv : EmptyParams;\n\n");

    // Handle load_data before extracting startIdx/endIdx (which load_data doesn't have)
    // Each component copies at its own capped length (Java-server parity): a
    // shorter secondary array must not turn the whole request into a crash.
    s.push_str("            if (method == \"load_data\") {\n");
    s.push_str("                double[] tmpOpen = GetDoubleArray(p, \"open\");\n");
    s.push_str("                refN = Math.Min(tmpOpen.Length, MAX_ARRAY_SIZE);\n");
    s.push_str("                Array.Copy(tmpOpen, refOpen, refN);\n");
    s.push_str("                LoadRef(p, \"high\", refHigh);\n");
    s.push_str("                LoadRef(p, \"low\", refLow);\n");
    s.push_str("                LoadRef(p, \"close\", refClose);\n");
    s.push_str("                LoadRef(p, \"volume\", refVolume);\n");
    s.push_str("                LoadRef(p, \"openInterest\", refOI);\n");
    s.push_str("                return $\"{{\\\"status\\\":\\\"ok\\\",\\\"n\\\":{refN}}}\";\n");
    s.push_str("            }\n\n");

    s.push_str("            int startIdx = GetInt(p, \"startIdx\", 0);\n");
    s.push_str("            int endIdx = GetInt(p, \"endIdx\", 0);\n\n");

    // Thin dispatch: each indicator delegates to its own static handler.
    for (i, func) in funcs.iter().enumerate() {
        let cond = if i == 0 { "if" } else { "else if" };
        s.push_str(&format!(
            "            {cond} (method == \"TA_{name}\") return Handle_{name}(p, startIdx, endIdx);\n",
            name = func.name
        ));
    }

    // list_functions method — returns {"functions":["TA_SMA","TA_RSI",...]}
    s.push_str("            else if (method == \"list_functions\") {\n");
    s.push_str("                var sb = new System.Text.StringBuilder(\"{\\\"functions\\\":[\");\n");
    for (i, func) in funcs.iter().enumerate() {
        if i > 0 {
            s.push_str("                sb.Append(\",\");\n");
        }
        s.push_str(&format!("                sb.Append(\"\\\"TA_{}\\\"\");\n", func.name));
    }
    s.push_str("                sb.Append(\"]}\");\n");
    s.push_str("                return sb.ToString();\n");
    s.push_str("            }\n");

    // set_unstable_period — FuncUnstId.ALL is the "set all" sentinel (matches
    // C's TA_SetUnstablePeriod and the Java server).
    s.push_str("            else if (method == \"set_unstable_period\") {\n");
    s.push_str("                rideGen++;\n");
    s.push_str("                int id = GetInt(p, \"id\", -1);\n");
    s.push_str("                int period = GetInt(p, \"period\", 0);\n");
    // The same 0..=MAX_INDEX domain the C library enforces. Checked before any
    // store, so a rejected call leaves every slot as it was (#186).
    s.push_str("                if (period < 0 || period > Core.MAX_INDEX) {\n");
    s.push_str("                    return \"{\\\"error\\\":\\\"Invalid unstable period value\\\"}\";\n");
    s.push_str("                }\n");
    s.push_str("                if (id == (int)FuncUnstId.ALL) {\n");
    s.push_str("                    for (int i = 0; i < core.unstablePeriod.Length; i++) core.unstablePeriod[i] = period;\n");
    s.push_str("                    return \"{\\\"status\\\":\\\"ok\\\"}\";\n");
    s.push_str("                }\n");
    s.push_str("                if (id >= 0 && id < core.unstablePeriod.Length) {\n");
    s.push_str("                    core.unstablePeriod[id] = period;\n");
    s.push_str("                    return \"{\\\"status\\\":\\\"ok\\\"}\";\n");
    s.push_str("                }\n");
    s.push_str("                return \"{\\\"error\\\":\\\"Invalid id\\\"}\";\n");
    s.push_str("            }\n");

    // set_candle_settings (#215). Unlike the unstable period above, this does NOT
    // reach into core's fields: it goes through the shipped CoreBuilder, so the
    // validation being compared across languages is the library's own and not a
    // second copy living in the server. Core is immutable, so a change is a
    // rebuild; the assignment happens only if the builder accepted every
    // argument, which is what keeps a rejected call from writing anything.
    s.push_str("            else if (method == \"set_candle_settings\") {\n");
    s.push_str("                rideGen++;\n");
    s.push_str("                int settingType = GetInt(p, \"settingType\", -1);\n");
    s.push_str("                int rangeType = GetInt(p, \"rangeType\", -1);\n");
    s.push_str("                int avgPeriod = GetInt(p, \"avgPeriod\", 0);\n");
    s.push_str("                double factor = GetF64Bits(p, \"factorBits\", 1.0);\n");
    s.push_str("                try {\n");
    s.push_str("                    core = core.ToBuilder()\n");
    s.push_str("                        .CandleSetting((CandleSettingType)settingType, (RangeType)rangeType,\n");
    s.push_str("                                       avgPeriod, factor)\n");
    s.push_str("                        .Build();\n");
    s.push_str("                } catch (ArgumentOutOfRangeException) {\n");
    s.push_str("                    return \"{\\\"error\\\":\\\"Invalid candle setting\\\"}\";\n");
    s.push_str("                }\n");
    s.push_str("                return \"{\\\"status\\\":\\\"ok\\\"}\";\n");
    s.push_str("            }\n");

    // restore_candle_default_settings (#215). AllCandleSettings IS a legal
    // argument here — it is the wildcard that set_candle_settings rejects.
    s.push_str("            else if (method == \"restore_candle_default_settings\") {\n");
    s.push_str("                rideGen++;\n");
    s.push_str("                int settingType = GetInt(p, \"settingType\", -1);\n");
    s.push_str("                try {\n");
    s.push_str("                    core = core.ToBuilder()\n");
    s.push_str("                        .RestoreCandleDefault((CandleSettingType)settingType)\n");
    s.push_str("                        .Build();\n");
    s.push_str("                } catch (ArgumentOutOfRangeException) {\n");
    s.push_str("                    return \"{\\\"error\\\":\\\"Invalid candle setting type\\\"}\";\n");
    s.push_str("                }\n");
    s.push_str("                return \"{\\\"status\\\":\\\"ok\\\"}\";\n");
    s.push_str("            }\n");

    // eval_predicate — boolean near-zero builtin on each input value; the SAME
    // C# form the generated indicators use (csharp_predicate_expr is the single
    // source of both).
    s.push_str("            else if (method == \"eval_predicate\") {\n");
    s.push_str("                int which = GetInt(p, \"which\", 0);\n");
    s.push_str("                double[] values = GetDoubleArray(p, \"values\");\n");
    s.push_str("                double[] scale = GetDoubleArray(p, \"scale\");\n");
    s.push_str("                var parts = new string[values.Length];\n");
    s.push_str("                for (int i = 0; i < values.Length; i++) {\n");
    s.push_str("                    double v = values[i];\n");
    s.push_str("                    double sc = (i < scale.Length) ? scale[i] : 0.0;\n");
    s.push_str("                    bool r;\n");
    s.push_str(&format!(
        "                    if (which == 1) r = {};\n",
        crate::backends::csharp::csharp_predicate_expr(
            SpecialBuiltin::IsZeroScaled,
            &["v".to_string(), "sc".to_string()]
        )
    ));
    s.push_str(&format!(
        "                    else if (which == 2) r = {};\n",
        crate::backends::csharp::csharp_predicate_expr(
            SpecialBuiltin::IsZeroOrNeg,
            &["v".to_string()]
        )
    ));
    s.push_str(&format!(
        "                    else r = {};\n",
        crate::backends::csharp::csharp_predicate_expr(
            SpecialBuiltin::IsZero,
            &["v".to_string()]
        )
    ));
    s.push_str("                    parts[i] = r ? \"1\" : \"0\";\n");
    s.push_str("                }\n");
    s.push_str("                return \"{\\\"outInteger\\\":[\" + string.Join(\",\", parts) + \"]}\";\n");
    s.push_str("            }\n");

    // abstract_get_lookback — the lookback-tier RPC the --xlang-hash gate
    // sweeps every parameter vector through (out-of-range vectors must come
    // back as -1, exactly what the guarded *Lookback methods return).
    s.push_str("            else if (method == \"abstract_get_lookback\") {\n");
    s.push_str("                string fn = p.GetProperty(\"funcName\").GetString()!;\n");
    s.push_str("                return $\"{{\\\"lookback\\\":{ComputeLookback(fn, p)}}}\";\n");
    s.push_str("            }\n");

    // ta_abstract introspection + dynamic dispatch, answered from the SHIPPED
    // TALib.Metadata catalogue (the csproj compiles the library sources, so the
    // server has no table of its own). test_abstract.c therefore proves the
    // shipped artifact against the C library rather than a test-only copy.
    s.push_str("            else if (method == \"TA_GetFuncInfo\") return AbsFuncInfo(p);\n");
    s.push_str("            else if (method == \"TA_GetInputParameterInfo\") return AbsInputInfo(p);\n");
    s.push_str("            else if (method == \"TA_GetOptInputParameterInfo\") return AbsOptInputInfo(p);\n");
    s.push_str("            else if (method == \"TA_GetOutputParameterInfo\") return AbsOutputInfo(p);\n");
    s.push_str("            else if (method == \"abstract_for_each_func\") return AbsForEachFunc();\n");
    s.push_str("            else if (method == \"TA_FunctionDescriptionXML\") return AbsDescriptionXml();\n");
    s.push_str("            else if (method == \"abstract_call\") return AbsCall(p);\n");
    // stream_verify: C# stream vs C# batch, bitwise, in-process. Drives the
    // ta_regtest stream pass the moment the capability probe answers
    // "not_streamable" — see the TODO(S9) in generate_csharp_stream_verify.
    s.push_str("            else if (method == \"stream_verify\") return HandleStreamVerify(p);\n");
    s.push_str("            else if (method == \"fuzz_in_hash\") return HandleFuzzInHash(p);\n");
    // Unknown method: an error RESPONSE (not a crash) — this is the driver's
    // capability-probe path (stream_verify, fuzz_in_hash, abstract RPCs).
    s.push_str("            else {\n");
    s.push_str("                return $\"{{\\\"error\\\":\\\"Unknown method: {method}\\\"}}\";\n");
    s.push_str("            }\n");
    s.push_str("    }\n\n");

    // The ta_abstract handlers. Fixed source: they read the shipped catalogue,
    // so there is no per-function generated code here at all.
    s.push_str(CSHARP_ABSTRACT_HANDLERS);

    // The stream-verification section: the bit-compare helpers, the fuzz input
    // generator, one sv_<NAME> per streaming function, and the dispatcher.
    // `funcs` is already Lang::CSharp-resolved by the caller, which matters —
    // six functions carry a PRAGMA TA_ALT body claiming the STREAM tier.
    s.push_str(&crate::stream_verify_gen::csharp::generate_csharp_stream_verify(funcs, enums));
    s.push_str(&crate::ride_gen::csharp::generate_csharp_ridealong(funcs));

    // ComputeLookback: parse a function's opt params (same JSON keys and 0/0.0
    // absent-field fallbacks as the per-function handlers) and call its guarded
    // <Name>Lookback. Mirrors the Java server's computeLookback.
    //
    // Deliberately NOT routed through FunctionCall: the --xlang-hash sweep
    // drives out-of-range parameter vectors through abstract_get_lookback and
    // requires -1 back, which is exactly what the guarded *Lookback prologue
    // returns. A validating binder would throw before reaching it, and a
    // try/catch "fix" would silently turn that gate into a test of the binder's
    // own range table instead of the codegen's validation.
    s.push_str("    static long ComputeLookback(string funcName, JsonElement p) {\n");
    s.push_str("        switch (funcName) {\n");
    for func in funcs {
        let base = func.name.clone();
        s.push_str(&format!("        case \"{}\": {{\n", func.name));
        for opt in &func.optional_inputs {
            match &opt.param_type {
                ParamType::Real => s.push_str(&format!(
                    "            double {name} = GetDouble(p, \"{name}\", 0.0);\n",
                    name = opt.name
                )),
                ParamType::Enum(enum_name) => s.push_str(&format!(
                    "            {ty} {name} = ({ty})GetInt(p, \"{name}\", 0);\n",
                    ty = enum_name,
                    name = opt.name
                )),
                _ => s.push_str(&format!(
                    "            int {name} = GetInt(p, \"{name}\", 0);\n",
                    name = opt.name
                )),
            }
        }
        let args: Vec<&str> = func.optional_inputs.iter().map(|o| o.name.as_str()).collect();
        s.push_str(&format!(
            "            return core.{base}_Lookback({});\n",
            args.join(", ")
        ));
        s.push_str("        }\n");
    }
    s.push_str("        default: return -1;\n");
    s.push_str("        }\n");
    s.push_str("    }\n\n");

    // Per-function handler methods.
    for func in funcs {
        let base = func.name.clone();
        let base_pascal = crate::backends::common::pascal_words(&func.name);
        let stream_class = crate::backends::csharp_stream::stream_class_name(func);
        let input_names = expand_input_names(&func.inputs);
        let outputs = &func.outputs;

        s.push_str(&format!(
            "    static string Handle_{}(JsonElement p, int startIdx, int endIdx) {{\n",
            func.name
        ));
        s.push_str("        int use_preloaded = GetInt(p, \"use_preloaded\", 0);\n");
        s.push_str("        int bench_iters = GetInt(p, \"iters\", 1);\n");
        s.push_str("        if (bench_iters < 1) bench_iters = 1;\n");
        // bench_mode (ta_bench --mode): 0 = batch (default), 1 = the streaming
        // warm-up <N>_Open, 2 = <N>_OpenAndFill (issue #256). Answering
        // "unsupported_mode" here silently times nothing for --mode=open and
        // --mode=openfill. Handles are GC-managed (no Close), and the public Open/
        // OpenAndFill throw instead of returning a code, same as the batch
        // call below -- the arms convert the throw into a RetCode the same way.
        s.push_str("        int bench_mode = GetInt(p, \"bench_mode\", 0);\n");

        // Inputs: preloaded reference data or from the request.
        for name in &input_names {
            s.push_str(&format!("        double[] {name};\n"));
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
                "            {name} = new double[refN]; Array.Copy({ref_src}, {name}, refN);\n"
            ));
        }
        s.push_str("        } else {\n");
        for name in &input_names {
            s.push_str(&format!("            {name} = GetDoubleArray(p, \"{name}\");\n"));
        }
        s.push_str("        }\n");

        // Right-sized warm-up views for the Open/OpenAndFill arms, bound ONCE
        // outside the timing loop. Guarded on bench_mode, same as Java's
        // null-when-unused: endIdx+1 can exceed the array's real length on
        // purpose (the index-range boundary sweep sends endIdx near
        // TA_MAX_INDEX on a small array to prove the batch call's OWN
        // validation rejects it) -- AsSpan's own bounds check would throw
        // ArgumentOutOfRangeException before that validation ever runs if
        // this were unconditional, on every plain batch call, not just the
        // warm-up ones. `default` is a valid empty ReadOnlySpan<double>.
        // Java derives historyLen from array.length, and with use_preloaded the
        // buffer is refN-sized already; slicing to endIdx+1 matches what the
        // C/Rust/Java arms do for the same reason (measure the same range the
        // batch call does, not whatever --points happened to preload).
        for name in &input_names {
            s.push_str(&format!(
                "        ReadOnlySpan<double> _warm_{name} = bench_mode == 0 ? default : {name}.AsSpan(0, endIdx + 1);\n"
            ));
        }

        // Optional params (enum params read as int, cast to the enum type).
        // An absent field defaults to 0/0.0, matching the C and Java servers
        // exactly — the driver always sends every param, and a divergent
        // fallback here could mask a driver bug behind a YAML default.
        for opt in &func.optional_inputs {
            match &opt.param_type {
                ParamType::Real => {
                    s.push_str(&format!(
                        "        double {name} = GetDouble(p, \"{name}\", 0.0);\n",
                        name = opt.name
                    ));
                }
                ParamType::Enum(enum_name) => {
                    s.push_str(&format!(
                        "        {ty} {name} = ({ty})GetInt(p, \"{name}\", 0);\n",
                        ty = enum_name,
                        name = opt.name
                    ));
                }
                _ => {
                    s.push_str(&format!(
                        "        int {name} = GetInt(p, \"{name}\", 0);\n",
                        name = opt.name
                    ));
                }
            }
        }

        // Apply unstable period if this function has one. The SLOT comes from
        // the shipped catalogue, not from the generated id: that makes the
        // registry's `UnstableId` load-bearing, so a wrong one writes the wrong
        // slot and the existing cross-language unstable-period value sweep
        // diverges from C. (Nothing else checks these ids in any language —
        // the ta-lib-python #752 failure mode.)
        if func_unst_id(&func.name, enums).is_some() {
            s.push_str(&format!(
                "        core.unstablePeriod[(int)FunctionCatalog.Default[\"{name}\"].UnstableId!.Value] = GetInt(p, \"unstablePeriod\", 0);\n",
                name = func.name
            ));
        }

        // Output arrays, typed per output, sized to the produced extent.
        {
            let lb_args: Vec<String> =
                func.optional_inputs.iter().map(|o| o.name.clone()).collect();
            s.push_str(&doc_produced_extent("        ", "//"));
            s.push_str(&format!(
                "        int _lb = core.{base}_Lookback({});\n",
                lb_args.join(", ")
            ));
            s.push_str("        int _cs = startIdx > _lb ? startIdx : _lb;\n");
            s.push_str("        int _outLen = ((_lb < 0 || _cs > endIdx) ? 1 : endIdx - _cs + 1) + GetInt(p, \"out_pad\", 0);\n");
        }
        for (k, out) in outputs.iter().enumerate() {
            if out.param_type == ParamType::Integer {
                s.push_str(&format!("        int[] outArr{k} = new int[_outLen];\n"));
            } else {
                s.push_str(&format!("        double[] outArr{k} = new double[_outLen];\n"));
            }
        }
        s.push_str("        int outBegIdx = 0, outNBElement = 0;\n");
        s.push_str("        RetCode rc = RetCode.Success;\n");

        // Guarded timing loop.
        let mut call_args = String::from("startIdx, endIdx");
        for name in &input_names {
            call_args.push_str(&format!(", {name}"));
        }
        for opt in &func.optional_inputs {
            call_args.push_str(&format!(", {}", opt.name));
        }
        call_args.push_str(", out outBegIdx, out outNBElement");
        for k in 0..outputs.len() {
            call_args.push_str(&format!(", outArr{k}"));
        }
        // Iteration 0 is always a discarded warm-up — see the C emitter. It
        // matters most here (the cold call is 1.5-10x steady state) and it makes
        // every correctness gate an idempotency check.
        s.push_str("        long _t0 = 0;\n");
        s.push_str("        for (int _bi = 0; _bi <= bench_iters; _bi++) {\n");
        s.push_str("            if (_bi == 1) _t0 = GetNanoTime();\n");
        // Correctness through the PUBLIC overload, the benchmark through the
        // C-shaped one. See the Java emitter for why, and for why only the
        // library's own failure is converted to a code here.
        {
            let mut pub_args = String::from("startIdx, endIdx");
            for name in &input_names {
                pub_args.push_str(&format!(", {name}"));
            }
            for opt in &func.optional_inputs {
                pub_args.push_str(&format!(", {}", opt.name));
            }
            for k in 0..outputs.len() {
                pub_args.push_str(&format!(", outArr{k}"));
            }
            s.push_str("            if (bench_mode == 0) {\n");
            s.push_str("            if (GetInt(p, \"timed\", 0) != 0) {\n");
            s.push_str("                try {\n");
            s.push_str(&format!("                    rc = core.{base}_Impl({call_args});\n"));
            s.push_str("                } catch (Exception _e2) when (_e2 is ITaLibFailure) {\n");
            s.push_str("                    rc = ((ITaLibFailure)_e2).RetCode;\n");
            s.push_str("                    outBegIdx = 0;\n");
            s.push_str("                    outNBElement = 0;\n");
            s.push_str("                }\n");
            s.push_str("            } else {\n");
            s.push_str("                try {\n");
            s.push_str(&format!("                    OutRange _pr = core.{base}({pub_args});\n"));
            s.push_str("                    outBegIdx = _pr.BegIdx;\n");
            s.push_str("                    outNBElement = _pr.Count;\n");
            s.push_str("                    rc = RetCode.Success;\n");
            s.push_str("                } catch (Exception _e) when (_e is ITaLibFailure) {\n");
            s.push_str("                    rc = ((ITaLibFailure)_e).RetCode;\n");
            s.push_str("                    outBegIdx = 0;\n");
            s.push_str("                    outNBElement = 0;\n");
            s.push_str("                }\n");
            s.push_str("            }\n");
            // --- warm-up arms (ta_bench --mode=open / openfill), issue #256.
            // Handles are GC-managed (no Close) and the public Open/OpenAndFill
            // throw instead of returning a code, so these arms convert the
            // throw into a RetCode the same way the batch call above does.
            let mut open_args: Vec<String> =
                input_names.iter().map(|n| format!("_warm_{n}")).collect();
            for opt in &func.optional_inputs {
                open_args.push(opt.name.clone());
            }
            let ins = open_args.join(", ");
            let mut fill_args = open_args.clone();
            for k in 0..outputs.len() {
                fill_args.push(format!("outArr{k}"));
            }
            let fill = fill_args.join(", ");
            s.push_str("            } else if (bench_mode == 1) {\n");
            s.push_str("                try {\n");
            s.push_str(&format!("                    core.{base_pascal}Open({ins});\n"));
            s.push_str("                    rc = RetCode.Success;\n");
            s.push_str("                } catch (Exception _e3) when (_e3 is ITaLibFailure) {\n");
            s.push_str("                    rc = ((ITaLibFailure)_e3).RetCode;\n");
            s.push_str("                }\n");
            s.push_str("            } else {\n");
            s.push_str("                try {\n");
            // The fill reports its range via the returned handle's OutRange
            // property -- unpack it into the same two locals the batch arm
            // sets, which the response builder below reads.
            s.push_str(&format!(
                "                    Core.{stream_class} _wh = core.{base_pascal}OpenAndFill({fill});\n"
            ));
            s.push_str("                    outBegIdx = _wh.OutRange.BegIdx;\n");
            s.push_str("                    outNBElement = _wh.OutRange.Count;\n");
            s.push_str("                    rc = RetCode.Success;\n");
            s.push_str("                } catch (Exception _e3) when (_e3 is ITaLibFailure) {\n");
            s.push_str("                    rc = ((ITaLibFailure)_e3).RetCode;\n");
            s.push_str("                    outBegIdx = 0;\n");
            s.push_str("                    outNBElement = 0;\n");
            s.push_str("                }\n");
            s.push_str("            }\n");
        }
        s.push_str("        }\n");
        s.push_str("        long elapsedNs = (GetNanoTime() - _t0) / bench_iters;\n");

        // Float-variant leg ("use_float":1): re-run through the float[] overload
        // of the same core, over the same output buffers, so the response carries
        // the single-precision result. Mirrors the C server's TA_S_ leg. Without
        // it the 168 shipped float overloads have no value verification at all.
        {
            let mut f_args = String::from("startIdx, endIdx");
            for name in &input_names {
                f_args.push_str(&format!(", f_{name}"));
            }
            for opt in &func.optional_inputs {
                f_args.push_str(&format!(", {}", opt.name));
            }
            f_args.push_str(", out outBegIdx, out outNBElement");
            for k in 0..outputs.len() {
                f_args.push_str(&format!(", outArr{k}"));
            }
            s.push_str("        int usedFloat = 0;\n");
            s.push_str("        if (GetInt(p, \"use_float\", 0) != 0) {\n");
            for name in &input_names {
                s.push_str(&format!(
                    "            var f_{name} = new float[{name}.Length];\n\
                     \x20           for (int _fi = 0; _fi < {name}.Length; _fi++) f_{name}[_fi] = (float){name}[_fi];\n"
                ));
            }
            {
                let mut fpub = String::from("startIdx, endIdx");
                for name in &input_names {
                    fpub.push_str(&format!(", f_{name}"));
                }
                for opt in &func.optional_inputs {
                    fpub.push_str(&format!(", {}", opt.name));
                }
                for k in 0..outputs.len() {
                    fpub.push_str(&format!(", outArr{k}"));
                }
                s.push_str("            try {\n");
                s.push_str(&format!("                OutRange _fr = core.{base}({fpub});\n"));
                s.push_str("                outBegIdx = _fr.BegIdx;\n");
                s.push_str("                outNBElement = _fr.Count;\n");
                s.push_str("                rc = RetCode.Success;\n");
                s.push_str("            } catch (Exception _e) when (_e is ITaLibFailure) {\n");
                s.push_str("                rc = ((ITaLibFailure)_e).RetCode;\n");
                s.push_str("                outBegIdx = 0;\n");
                s.push_str("                outNBElement = 0;\n");
                s.push_str("            }\n");
            }
            s.push_str("            usedFloat = 1;\n");
            s.push_str("        }\n");
        }

        // want_hash mode (server_verify / issue #115): digest of the GUARDED
        // output, returned before the value response is built.
        s.push_str("        if (GetInt(p, \"want_hash\", 0) != 0 && GetInt(p, \"full_output\", 0) == 0) {\n");
        s.push_str("            ulong _h = SvHashInit();\n");
        s.push_str("            if (rc == RetCode.Success && outNBElement > 0) {\n");
        for (k, out) in outputs.iter().enumerate() {
            if out.param_type == ParamType::Integer {
                s.push_str(&format!(
                    "                _h = SvHashI32(_h, outArr{k}, outNBElement);\n"
                ));
            } else {
                s.push_str(&format!(
                    "                _h = SvHashF64(_h, outArr{k}, outNBElement);\n"
                ));
            }
        }
        s.push_str("            }\n");
        s.push_str("            _h = SvHashFin(_h);\n");
        s.push_str("            var hb = new System.Text.StringBuilder();\n");
        s.push_str("            hb.Append($\"{{\\\"retCode\\\":{(int)rc},\\\"outBegIdx\\\":{outBegIdx},\\\"outNBElement\\\":{outNBElement},\\\"out_hash\\\":\\\"{_h:x16}\\\"\");\n");
        if func.streaming {
            let mut ride_args = String::new();
            for name in &input_names {
                let _ = write!(ride_args, "{name}, ");
            }
            for opt in &func.optional_inputs {
                let _ = write!(ride_args, "{}, ", opt.name);
            }
            s.push_str(&format!("            Ride{}(core, p, endIdx, {}hb);\n", func.name, ride_args));
        }
        s.push_str("            hb.Append(\"}\");\n");
        s.push_str("            return hb.ToString();\n");
        s.push_str("        }\n");


        // Response. no_output (ta_bench): timings only — serialising a
        // 100k-element array nobody reads is ~97% of a bench run's wall clock.
        s.push_str("        var sb = new System.Text.StringBuilder();\n");
        s.push_str("        sb.Append($\"{{\\\"retCode\\\":{(int)rc},\\\"outBegIdx\\\":{outBegIdx},\\\"outNBElement\\\":{outNBElement}\");\n");
        s.push_str("        sb.Append($\",\\\"out_len\\\":{_outLen}\");\n");
        s.push_str("        if (GetInt(p, \"no_output\", 0) == 0) {\n");
        for (k, out) in outputs.iter().enumerate() {
            let key = output_json_key(outputs, k);
            if out.param_type == ParamType::Integer {
                s.push_str(&format!(
                    "            sb.Append(\",\\\"{key}\\\":\"); sb.Append(FormatIntArray(outArr{k}, outNBElement));\n"
                ));
            } else {
                s.push_str(&format!(
                    "            sb.Append(\",\\\"{key}\\\":\"); sb.Append(FormatArray(outArr{k}, outNBElement));\n"
                ));
            }
        }
        s.push_str("        }\n");
        s.push_str("        sb.Append($\",\\\"used_float\\\":{usedFloat}\");\n");
        s.push_str("        sb.Append($\",\\\"timing_ns\\\":{elapsedNs}\");\n");
        if func.streaming {
            let mut ride_args = String::new();
            for name in &input_names {
                let _ = write!(ride_args, "{name}, ");
            }
            for opt in &func.optional_inputs {
                let _ = write!(ride_args, "{}, ", opt.name);
            }
            s.push_str(&format!(
                "        Ride{}(core, p, endIdx, {}sb);\n",
                func.name, ride_args
            ));
        }
        s.push_str("        sb.Append(\"}\");\n");
        s.push_str("        return sb.ToString();\n");
        s.push_str("    }\n\n");
    }

    // Main. The handler is wrapped because an escaping exception kills the
    // process, and the driver then reports a pipe-read failure for every
    // REMAINING function rather than naming the one request that broke -- the
    // failure mode that made the Java server's MAType decode so hard to place
    // (issue #164). A thrown request now answers with an error object and the
    // server stays up for the next one.
    s.push_str("    static void Main(string[] args) {\n");
    s.push_str("        string? line;\n");
    s.push_str("        while ((line = Console.ReadLine()) != null) {\n");
    s.push_str("            if (string.IsNullOrWhiteSpace(line)) continue;\n");
    s.push_str("            string reply;\n");
    s.push_str("            try { reply = HandleRequest(line); }\n");
    s.push_str("            catch (Exception e) {\n");
    s.push_str("                reply = \"{\\\"error\\\":\" + AbsStr(e.GetType().Name + \": \" + e.Message) + \"}\";\n");
    s.push_str("            }\n");
    s.push_str("            Console.WriteLine(reply);\n");
    s.push_str("            Console.Out.Flush();\n");
    s.push_str("        }\n");
    s.push_str("    }\n");
    s.push_str("}\n");

    // The fuzz input generator, appended after the server class (global
    // namespace, like the Java port's default package). Verified byte-identical
    // to fuzz_data.h at port time; the fuzz_in_hash RPC re-proves it per run.
    s.push('\n');
    s.push_str(CSHARP_FUZZ);

    s
}

/// The C# port of `fuzz_data.h` — byte-identical input generation, verified
/// bit-for-bit against the C original by a differential harness at port time
/// (2.4M doubles, 13 seeds, 12 lengths, every shape).
const CSHARP_FUZZ: &str = include_str!("../templates/csharp/FuzzData.cs");

/// The generated csproj for the managed C# server. Compiling the shipped
/// library sources into the server's own assembly (rather than referencing a
/// prebuilt TALib.dll) is deliberate on two counts: the harness provably runs
/// the shipped source text, and the server can reach the `internal` cores and
/// `unstablePeriod` state because `internal` is assembly-scoped.
pub fn csharp_server_csproj() -> String {
    r#"<Project Sdk="Microsoft.NET.Sdk">

  <!-- Auto-generated by ta_codegen (generate-servers) - do not edit. -->

  <PropertyGroup>
    <OutputType>Exe</OutputType>
    <TargetFramework>net10.0</TargetFramework>
    <Nullable>enable</Nullable>
    <LangVersion>latest</LangVersion>
    <!-- Pin invariant culture so double.ToString() cannot vary by locale. -->
    <InvariantGlobalization>true</InvariantGlobalization>
  </PropertyGroup>

  <ItemGroup>
    <!-- The shipped library sources, compiled directly: same files, same
         bytes, so the cross-language harness measures the shipped code. -->
    <Compile Include="../library/*.cs" />
    <Compile Include="../library/src/**/*.cs" />
  </ItemGroup>

</Project>
"#
    .to_string()
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

/// Bit-exact Rust port of `src/tools/ta_regtest/fuzz_data.h` (seed-based OHLCV
/// generator + FNV output hasher), embedded verbatim into the Rust server to
/// power the cross-language bitwise-parity gate (`--xlang-hash`, issue #113).
/// Verified byte-for-byte against the C generator. Kept as a standalone template
/// file so it reads/reviews as normal Rust rather than an escaped string blob.
const RUST_FUZZ: &str = include_str!("../templates/rust/fuzz.rs");

/// Generate a Rust JSON-RPC server source file.
///
/// The generated file is a standalone binary that imports from the `ta_lib` crate.
/// It reads JSON-RPC requests from stdin, dispatches to the generated TA function
/// implementations, and writes JSON responses to stdout.
#[allow(clippy::too_many_lines)]
#[allow(clippy::implicit_hasher)]
#[allow(clippy::cognitive_complexity)]
pub fn generate_rust_server(funcs: &[FuncDef], enums: &HashMap<String, EnumDef>) -> String {
    // Resolve `PRAGMA TA_ALT` for this language before anything reads a body.
    let resolved = crate::ir::resolve_all(funcs, crate::ir::Lang::Rust);
    let funcs: &[FuncDef] = &resolved;
    let mut s = String::new();

    // File-level attributes
    s.push_str("#![forbid(unsafe_code)]\n");
    s.push_str(
        "#![allow(non_snake_case, unused_variables, dead_code, unused_parens, clippy::all)]\n\n",
    );

    // Imports
    s.push_str("use serde_json::{self, Value};\n");
    s.push_str("use std::io::{self, BufRead, Write};\n");
    s.push_str("use std::time::Instant;\n");
    s.push_str("use ta_lib::{Core, CoreBuilder, RetCode, FuncUnstId};\n");
    s.push_str("use ta_lib::{CandleSetting, CandleSettings, CandleSettingType, RangeType};\n");
    s.push_str("use ta_lib::abstract_api::{self, InputType, OutputType, OptInputType};\n");
    // The enum types the handlers convert wire ints into, from what the
    // definitions actually declare rather than a name spelled here.
    let mut enum_tys: Vec<&str> = funcs
        .iter()
        .flat_map(|f| &f.optional_inputs)
        .filter_map(|o| match &o.param_type {
            ParamType::Enum(n) => Some(n.as_str()),
            _ => None,
        })
        .collect();
    enum_tys.sort_unstable();
    enum_tys.dedup();
    for ty in enum_tys {
        s.push_str(&format!("use ta_lib::{ty};\n"));
    }
    s.push('\n');

    // Seed-based fuzz input generator + FNV output hasher — a bit-exact port of
    // src/tools/ta_regtest/fuzz_data.h. Powers the cross-language bitwise-parity
    // gate (--xlang-hash, issue #113): the server regenerates the driver's seed
    // inputs in-process (no JSON float parse) and returns a full-precision hash
    // of its raw outputs, so ~1e-10 FMA drift is one value to compare, not many.
    s.push_str("// ---- fuzz_data.h port (issue #113 --xlang-hash) ----\n");
    s.push_str(RUST_FUZZ);
    s.push_str("\n// ---- end fuzz_data.h port ----\n\n");

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

    // Helper: parse f64 array from JSON value. Lossless hex-bits transport
    // (issue #115): an input array may arrive as a string of concatenated
    // 16-hex-char groups, each the IEEE-754 bit pattern of one f64 (from_bits =>
    // exact, no JSON float-parse rounding). Every other caller sends a number
    // array, which takes the fallback path unchanged.
    s.push_str("fn parse_f64_array(val: &Value) -> Vec<f64> {\n");
    s.push_str("    if let Some(hs) = val.as_str() {\n");
    s.push_str("        let b = hs.as_bytes();\n");
    s.push_str("        let mut out = Vec::with_capacity(b.len() / 16);\n");
    s.push_str("        let mut i = 0;\n");
    s.push_str("        while i + 16 <= b.len() {\n");
    s.push_str("            let mut bits: u64 = 0;\n");
    s.push_str("            for &c in &b[i..i + 16] {\n");
    s.push_str("                let d = match c {\n");
    s.push_str("                    b'0'..=b'9' => c - b'0',\n");
    s.push_str("                    b'a'..=b'f' => c - b'a' + 10,\n");
    s.push_str("                    b'A'..=b'F' => c - b'A' + 10,\n");
    s.push_str("                    _ => 0,\n");
    s.push_str("                };\n");
    s.push_str("                bits = (bits << 4) | d as u64;\n");
    s.push_str("            }\n");
    s.push_str("            out.push(f64::from_bits(bits));\n");
    s.push_str("            i += 16;\n");
    s.push_str("        }\n");
    s.push_str("        return out;\n");
    s.push_str("    }\n");
    s.push_str("    match val.as_array() {\n");
    s.push_str("        Some(arr) => arr.iter().filter_map(|v| v.as_f64()).collect(),\n");
    s.push_str("        None => Vec::new(),\n");
    s.push_str("    }\n");
    s.push_str("}\n\n");

    // Helper: RetCode to integer. Delegates to the library, whose match is total
    // -- re-spelling it here would need a `_` arm (`RetCode` is `#[non_exhaustive]`
    // and this is a downstream crate), and a new variant would then be reported to
    // the driver as whatever that arm said instead of failing to compile.
    s.push_str("fn retcode_to_int(rc: RetCode) -> i32 {\n");
    s.push_str("    rc.as_c_int()\n");
    s.push_str("}\n\n");

    // Helper: serialize an f64 slice as the lossless hex-bits transport the
    // inputs have used since #115 -- one string of concatenated 16-hex-char
    // groups, each value's IEEE-754 bits. serde_json's ryu formatting is
    // shortest-round-trip for a finite value, but `Number::from_f64` has no
    // number at all for a NaN or an infinity, so a decimal spelling needs a
    // bare `nan`/`inf` fallback token every driver's parser has to be taught.
    // `to_bits` needs no fallback and no token: every f64 has a spelling, and it
    // is the same one hash mode hashes (#257/#258).
    s.push_str("fn json_f64_array(data: &[f64]) -> String {\n");
    s.push_str("    let mut s = String::with_capacity(data.len() * 16 + 2);\n");
    s.push_str("    s.push('\"');\n");
    s.push_str("    for &v in data {\n");
    s.push_str("        s.push_str(&format!(\"{:016x}\", v.to_bits()));\n");
    s.push_str("    }\n");
    s.push_str("    s.push('\"');\n");
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
    for (i, name) in func_unst_variant_names(enums).iter().enumerate() {
        s.push_str(&format!("        {i} => Some(FuncUnstId::{name}),\n"));
    }
    s.push_str("        _ => None,\n");
    s.push_str("    }\n");
    s.push_str("}\n\n");

    // apply_unstable_period — rebuild the immutable `*core` with one function's
    // unstable period changed, going through the public builder API (`Core` has no
    // setters). Handles the `ALL` "set all" wildcard (id == ALL as
    // usize); returns false on an out-of-range id. Shared by the `set_unstable_period`
    // RPC and the inline per-function `unstablePeriod` override.
    // The period arrives as the raw wire i64 and is range-checked HERE, before any
    // cast: `as u32` on a negative would wrap it to a huge value, which is exactly
    // what C's `unsigned int` parameter exists to make unrepresentable. `*core` is
    // reassigned only on success, so a rejected RPC leaves the server's Core
    // untouched -- the same no-write rule the C library follows.
    s.push_str(
        "fn apply_unstable_period(core: &mut Core, id: usize, period: i64) -> Result<(), &'static str> {\n",
    );
    s.push_str("    let Ok(period) = u32::try_from(period) else {\n");
    s.push_str("        return Err(\"Invalid unstable period value\");\n");
    s.push_str("    };\n");
    s.push_str("    let cb = core.to_builder();\n");
    s.push_str("    let cb = if id == FuncUnstId::ALL as usize {\n");
    s.push_str("        cb.unstable_period(FuncUnstId::ALL, period)\n");
    s.push_str("    } else if let Some(uid) = func_unst_id_from_int(id) {\n");
    s.push_str("        cb.unstable_period(uid, period)\n");
    s.push_str("    } else {\n");
    s.push_str("        return Err(\"Invalid unstable period id\");\n");
    s.push_str("    };\n");
    s.push_str("    match cb.build() {\n");
    s.push_str("        Ok(built) => {\n");
    s.push_str("            *core = built;\n");
    s.push_str("            Ok(())\n");
    s.push_str("        }\n");
    s.push_str("        Err(_) => Err(\"Invalid unstable period value\"),\n");
    s.push_str("    }\n");
    s.push_str("}\n\n");

    // Helper: one f64 from the 16 hex chars of its IEEE-754 bits — the scalar
    // counterpart of parse_f64_array's transport (#115). Used for `factor`, which
    // has to be able to carry a NaN: NaN has no JSON number spelling, and refusing
    // one is part of the contract compared across languages (#215).
    s.push_str("fn parse_f64_bits(val: &Value, def: f64) -> f64 {\n");
    s.push_str("    let Some(h) = val.as_str() else { return def };\n");
    s.push_str("    if h.len() != 16 { return def; }\n");
    s.push_str("    match u64::from_str_radix(h, 16) {\n");
    s.push_str("        Ok(bits) => f64::from_bits(bits),\n");
    s.push_str("        Err(_) => def,\n");
    s.push_str("    }\n");
    s.push_str("}\n\n");

    // Helper: CandleSettingType from integer, in C TA_CandleSettingType order.
    // 11 (AllCandleSettings) is included: it is a legal RESTORE selector, and
    // candle_setting's own rejection of it is what the cross-language gate reads.
    s.push_str("fn candle_setting_type_from_int(id: i64) -> Option<CandleSettingType> {\n");
    s.push_str("    Some(match id {\n");
    s.push_str("        0 => CandleSettingType::BodyLong,\n");
    s.push_str("        1 => CandleSettingType::BodyVeryLong,\n");
    s.push_str("        2 => CandleSettingType::BodyShort,\n");
    s.push_str("        3 => CandleSettingType::BodyDoji,\n");
    s.push_str("        4 => CandleSettingType::ShadowLong,\n");
    s.push_str("        5 => CandleSettingType::ShadowVeryLong,\n");
    s.push_str("        6 => CandleSettingType::ShadowShort,\n");
    s.push_str("        7 => CandleSettingType::ShadowVeryShort,\n");
    s.push_str("        8 => CandleSettingType::Near,\n");
    s.push_str("        9 => CandleSettingType::Far,\n");
    s.push_str("        10 => CandleSettingType::Equal,\n");
    s.push_str("        11 => CandleSettingType::AllCandleSettings,\n");
    s.push_str("        _ => return None,\n");
    s.push_str("    })\n");
    s.push_str("}\n\n");

    // apply_candle_setting — the candle counterpart of apply_unstable_period, and
    // for the same reason: `Core` is immutable, so a settings change is a rebuild
    // through the public builder. Every value check lives in the library, not
    // here; the server's only job is to keep an unrepresentable wire value from
    // becoming a wrapped one, so rangeType and avgPeriod are `try_from`'d rather
    // than cast. `*core` is reassigned only on success, so a rejected RPC leaves
    // all eleven settings exactly as they were.
    s.push_str(
        "fn apply_candle_setting(core: &mut Core, st: i64, rt: i64, ap: i64, factor: f64) -> Result<(), &'static str> {\n",
    );
    s.push_str("    let Some(setting_type) = candle_setting_type_from_int(st) else {\n");
    s.push_str("        return Err(\"Invalid candle setting\");\n");
    s.push_str("    };\n");
    // The range type is an enum in the crate, so the wire integer is converted
    // here rather than at the builder: `RangeType::try_from` is what rejects an
    // out-of-domain one, and it must answer the same "Invalid candle setting"
    // the C server answers TA_BAD_PARAM to.
    s.push_str("    let (Ok(rt32), Ok(avg_period)) = (i32::try_from(rt), i32::try_from(ap)) else {\n");
    s.push_str("        return Err(\"Invalid candle setting\");\n");
    s.push_str("    };\n");
    s.push_str("    let Ok(range_type) = RangeType::try_from(rt32) else {\n");
    s.push_str("        return Err(\"Invalid candle setting\");\n");
    s.push_str("    };\n");
    s.push_str("    let setting = CandleSetting { range_type, avg_period, factor };\n");
    s.push_str("    match core.to_builder().candle_setting(setting_type, setting).build() {\n");
    s.push_str("        Ok(built) => {\n");
    s.push_str("            *core = built;\n");
    s.push_str("            Ok(())\n");
    s.push_str("        }\n");
    s.push_str("        Err(_) => Err(\"Invalid candle setting\"),\n");
    s.push_str("    }\n");
    s.push_str("}\n\n");

    // apply_restore_candle_default — same rebuild, and the one place where
    // AllCandleSettings is an argument rather than an error.
    s.push_str(
        "fn apply_restore_candle_default(core: &mut Core, st: i64) -> Result<(), &'static str> {\n",
    );
    s.push_str("    let Some(setting_type) = candle_setting_type_from_int(st) else {\n");
    s.push_str("        return Err(\"Invalid candle setting type\");\n");
    s.push_str("    };\n");
    s.push_str("    match core.to_builder().restore_candle_default(setting_type).build() {\n");
    s.push_str("        Ok(built) => {\n");
    s.push_str("            *core = built;\n");
    s.push_str("            Ok(())\n");
    s.push_str("        }\n");
    s.push_str("        Err(_) => Err(\"Invalid candle setting type\"),\n");
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
        let fn_name = func.name.clone();

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
        // bench_mode (ta_bench --mode): 0 = batch (default), 1 = the streaming
        // warm-up Open, 2 = OpenAndFill. Rust handles are dropped at end of
        // scope, so unlike C there is nothing to close explicitly.
        s.push_str("            let bench_mode = params[\"bench_mode\"].as_i64().unwrap_or(0);\n");
        // --xlang-hash (issue #113): seed-based input generation + out_hash. Absent
        // (0) for the normal per-function / preloaded paths.
        s.push_str("            let gen_present = params[\"gen_present\"].as_i64().unwrap_or(0);\n");
        s.push_str("            let gen_shape = params[\"gen_shape\"].as_i64().unwrap_or(0) as i32;\n");
        s.push_str("            let gen_seed = params[\"gen_seed\"].as_i64().unwrap_or(0) as i32;\n");
        s.push_str("            let gen_n = params[\"gen_n\"].as_i64().unwrap_or(0) as usize;\n");
        s.push_str("            let full_output = params[\"full_output\"].as_i64().unwrap_or(0);\n");
        // --xlang-hash uses gen_present; server_verify (issue #115) uses want_hash
        // with explicit lossless inputs. Either drives the same out_hash return.
        s.push_str("            let want_hash = params[\"want_hash\"].as_i64().unwrap_or(0);\n");

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

        // Populate from seed-generated fuzz inputs (--xlang-hash), preloaded, or JSON.
        // The fuzz convention mirrors the C driver: price components read their OHLCV
        // series; generic real inputs read real0=close, real1=volume.
        s.push_str("            if gen_present != 0 {\n");
        s.push_str("                let mut _fz_o = vec![0.0f64; gen_n];\n");
        s.push_str("                let mut _fz_h = vec![0.0f64; gen_n];\n");
        s.push_str("                let mut _fz_l = vec![0.0f64; gen_n];\n");
        s.push_str("                let mut _fz_c = vec![0.0f64; gen_n];\n");
        s.push_str("                let mut _fz_v = vec![0.0f64; gen_n];\n");
        s.push_str("                let mut _fz_oi = vec![0.0f64; gen_n];\n");
        s.push_str("                fuzz_gen(gen_shape, gen_seed, gen_n as i32, &mut _fz_o, &mut _fz_h, &mut _fz_l, &mut _fz_c, &mut _fz_v, &mut _fz_oi);\n");
        {
            let mut fz_real_idx = 0usize;
            for name in &input_names {
                let src = match name.as_str() {
                    "inOpen" => "_fz_o",
                    "inHigh" => "_fz_h",
                    "inLow" => "_fz_l",
                    "inClose" => "_fz_c",
                    "inVolume" => "_fz_v",
                    "inOpenInterest" => "_fz_oi",
                    _ => {
                        // generic real: real0=close, real1=volume (matches the C driver)
                        let a = if fz_real_idx == 1 { "_fz_v" } else { "_fz_c" };
                        fz_real_idx += 1;
                        a
                    }
                };
                s.push_str(&format!("                _json_{name} = {src}.clone();\n"));
                s.push_str(&format!("                {name} = &_json_{name};\n"));
            }
        }
        s.push_str("            } else if use_preloaded != 0 && ref_data.n > 0 {\n");
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
                if let ParamType::Enum(enum_name) = &opt.param_type {
                    // The wire carries a bare int, so an out-of-domain value
                    // reaches here; a typed enum cannot hold it, so the call is
                    // skipped and BAD_PARAM reported -- the same rc C gives,
                    // reached by the type system instead of the prologue. The
                    // library's `TryFrom` is what decides validity.
                    let first = enums
                        .get(enum_name)
                        .and_then(|e| e.variants.first())
                        .map_or_else(|| "0".to_string(), |v| format!("{enum_name}::{}", v.name));
                    s.push_str(&format!(
                        "            let {n}_raw = params[\"{n}\"].as_i64().unwrap_or({default_i}) as i32;\n",
                        n = opt.name
                    ));
                    s.push_str(&format!(
                        "            let {n}_res = {enum_name}::try_from({n}_raw);\n",
                        n = opt.name
                    ));
                    // A concrete binding so every downstream use types; it is
                    // only ever reached when the conversion succeeded.
                    s.push_str(&format!(
                        "            let {n} = {n}_res.unwrap_or({first});\n",
                        n = opt.name
                    ));
                } else {
                    s.push_str(&format!(
                        "            let {} = params[\"{}\"].as_i64().unwrap_or({}) as i32;\n",
                        opt.name, opt.name, default_i
                    ));
                }
            }
        }

        // Apply unstable period if provided
        if let Some(id) = func_unst_id(&func.name, enums) {
            s.push_str("            if let Some(period) = params[\"unstablePeriod\"].as_i64() {\n");
            s.push_str(&format!(
                "                let _ = apply_unstable_period(core, {id}, period);\n"
            ));
            s.push_str("            }\n");
        }

        // Allocate output buffers, sized to the produced extent.
        {
            let lb_args: Vec<String> =
                func.optional_inputs.iter().map(|o| o.name.clone()).collect();
            s.push_str(&doc_produced_extent("            ", "//"));
            s.push_str(&format!(
                "            let _lb = core.{}_Lookback({}).unwrap_or(usize::MAX);\n",
                func.name,
                lb_args.join(", ")
            ));
            s.push_str("            let _cs = if startIdx > _lb { startIdx } else { _lb };\n");
            s.push_str("            let out_size = (if _cs > endIdx { 1 } else { endIdx - _cs + 1 }) + params[\"out_pad\"].as_u64().unwrap_or(0) as usize;\n");
        }
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

        // Guarded timing loop. Iteration 0 is always a discarded warm-up — see
        // the C emitter: cold-call bias, plus a free idempotency check.
        s.push_str("            let mut start_time = Instant::now();\n");
        // An enum parameter that failed to convert reports its own RetCode and
        // the call is skipped, so the response matches C's -- which returned
        // BAD_PARAM from the prologue without running the body either.
        let enum_opts: Vec<&str> = func
            .optional_inputs
            .iter()
            .filter(|o| matches!(o.param_type, ParamType::Enum(_)))
            .map(|o| o.name.as_str())
            .collect();
        if !enum_opts.is_empty() {
            let bad = enum_opts
                .iter()
                .map(|n| format!("{n}_res.is_err()"))
                .collect::<Vec<_>>()
                .join(" || ");
            s.push_str(&format!("            let _enum_bad = {bad};\n"));
            s.push_str("            if _enum_bad { rc = RetCode::BadParam; } else {\n");
        }
        s.push_str("            for _bi in 0..=bench_iters {\n");
        s.push_str("                if _bi == 1 { start_time = Instant::now(); }\n");
        s.push_str("            if bench_mode == 0 {\n");
        // `tools` is a separate crate, so the only entry point reachable here is
        // the public one -- which means the value gates drive the API users call,
        // not the crate-private C-shaped body behind it.
        s.push_str(&format!(
            "            let _out = core.{fn_name}(\n"
        ));
        s.push_str("                startIdx, endIdx,\n");
        for name in &input_names {
            s.push_str(&format!("                &{name},\n"));
        }
        for opt in &func.optional_inputs {
            s.push_str(&format!("                {},\n", opt.name));
        }
        real_idx = 0;
        int_idx = 0;
        let mut out_args: Vec<String> = Vec::new();
        for out in outputs {
            let buf = if out.param_type == ParamType::Integer {
                int_idx += 1;
                format!("&mut outIntBuf{}", int_idx - 1)
            } else {
                real_idx += 1;
                format!("&mut outBuf{}", real_idx - 1)
            };
            // A nullable output takes `Option<&mut [T]>` (rule B6a). The server
            // always supplies it: a correctness request goes through the public
            // API with every declared output bound, which is what the C
            // reference is compared against.
            out_args.push(if out.is_nullable() { format!("Some({buf})") } else { buf });
        }
        s.push_str(&format!("                {},\n", out_args.join(", ")));
        s.push_str("            );\n");
        s.push_str("            rc = match _out {\n");
        s.push_str("                Ok(r) => { outBegIdx = r.beg_idx; outNBElement = r.count; RetCode::Success }\n");
        s.push_str("                Err(e) => { outBegIdx = 0; outNBElement = 0; e }\n");
        s.push_str("            };\n");
        s.push_str("            } else {\n");
        emit_rust_warmup_arms(&mut s, func, &input_names, outputs);
        s.push_str("            }\n");
        s.push_str("            }\n"); // end guarded bench loop
        if !enum_opts.is_empty() {
            s.push_str("            }\n"); // end enum-conversion guard
        }
        s.push_str("            let elapsed_ns = start_time.elapsed().as_nanos() as u64 / bench_iters as u64;\n");

        // [fuzz] out_hash mode (--xlang-hash, issue #113): after the GUARDED call —
        // the public API the C golden's TA_CallFunc also runs — return a
        // full-precision FNV digest of the raw outputs instead of the arrays, so a
        // ~1e-10 cross-language divergence is one value to compare. full_output
        // suppresses it (arrays to pinpoint WHICH element diverged). Hashes
        // outputs in logical order; nothing unless the call succeeded.
        s.push_str("            if (gen_present != 0 || want_hash != 0) && full_output == 0 {\n");
        s.push_str("                let mut _oh = fuzz_hash_init();\n");
        s.push_str("                if matches!(rc, RetCode::Success) && outNBElement > 0 {\n");
        {
            let mut r2 = 0usize;
            let mut i2 = 0usize;
            for out in outputs {
                if out.param_type == ParamType::Integer {
                    s.push_str(&format!(
                        "                    _oh = fuzz_hash_bytes_i32(_oh, &outIntBuf{i2}[..outNBElement]);\n"
                    ));
                    i2 += 1;
                } else {
                    s.push_str(&format!(
                        "                    _oh = fuzz_hash_bytes_f64(_oh, &outBuf{r2}[..outNBElement]);\n"
                    ));
                    r2 += 1;
                }
            }
        }
        s.push_str("                }\n");
        s.push_str("                _oh = fuzz_hash_fin(_oh);\n");
        s.push_str("                let mut hresp = format!(\"{{\\\"retCode\\\":{},\\\"outBegIdx\\\":{},\\\"outNBElement\\\":{},\\\"out_hash\\\":\\\"{:016x}\\\"\", retcode_to_int(rc), outBegIdx, outNBElement, _oh);\n");
        if func.streaming {
            let mut ride_args = String::new();
            for name in &input_names {
                let _ = write!(ride_args, "&{name}, ");
            }
            for opt in &func.optional_inputs {
                let _ = write!(ride_args, "{}, ", opt.name);
            }
            s.push_str(&format!("                ride_{}(&core, params, endIdx, {}&mut hresp);\n", crate::backends::common::snake_words(&func.name), ride_args));
        }
        s.push_str("                hresp.push('}');\n");
        s.push_str("                return hresp;\n");
        s.push_str("            }\n");


        // Lookback (mirrors C's TA_<NAME>_Lookback). Emitted on every response so the
        // abstract_call reroute returns the `lookback` field the C ta_abstract path
        // exposes; harmless extra field for the regular per-function path. Computed
        // after the unstable-period assignment above so it reflects that state.
        // A typed enum cannot carry an out-of-domain value, so there is no
        // lookback to report for one -- C computes a real number there (19 for
        // BBANDS, 4 for STOCHF, 18 for STOCHRSI: NOT a uniform 0), and this tier
        // cannot reproduce it. Report the driver's "rejected" marker rather than
        // a fabricated number, which is also what the abstract tier returns.
        if enum_opts.is_empty() {
            s.push_str(&format!("            let lookback: i64 = core.{fn_name}_Lookback("));
        } else {
            s.push_str(&format!(
                "            let lookback: i64 = if _enum_bad {{ -1 }} else {{ core.{fn_name}_Lookback("
            ));
        }
        let lb_args: Vec<String> = func
            .optional_inputs
            .iter()
            .map(|o| o.name.clone())
            .collect();
        s.push_str(&lb_args.join(", "));
        // Non-enum functions and the enum branch's callee both normalize the
        // same way: Ok -> the real value, Err -> -1, matching C/Java/C#'s wire
        // shape. `_enum_bad` stays a separate pre-condition -- an out-of-domain
        // enum member can't be constructed to pass to `_Lookback` at all.
        if enum_opts.is_empty() {
            s.push_str(").map_or(-1, |v| v as i64);\n");
        } else {
            s.push_str(").map_or(-1, |v| v as i64) };\n");
        }

        // Built manually rather than via serde_json: an output array is not a
        // JSON number array at all any more but the hex-bits string every
        // backend now writes (json_f64_array), which serde_json has no shape
        // for — and a non-finite f64 would have serialized as `null`, which is
        // neither the value nor something the driver's parser can count.
        s.push_str("            let mut resp = format!(\"{{\\\"retCode\\\":{},\\\"outBegIdx\\\":{},\\\"outNBElement\\\":{},\\\"out_len\\\":{},\\\"lookback\\\":{},\\\"timing_ns\\\":{}\", retcode_to_int(rc), outBegIdx, outNBElement, out_size, lookback, elapsed_ns);\n");

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

        if func.streaming {
            let mut ride_args = String::new();
            for name in &input_names {
                let _ = write!(ride_args, "&{name}, ");
            }
            for opt in &func.optional_inputs {
                let _ = write!(ride_args, "{}, ", opt.name);
            }
            s.push_str(&format!(
                "            ride_{}(&core, params, endIdx, {}&mut resp);\n",
                crate::backends::common::snake_words(&func.name), ride_args
            ));
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
    s.push_str("            RIDE_GEN.with(|g| g.set(g.get().wrapping_add(1)));\n");
    s.push_str(
        "            let id = params[\"id\"].as_u64().unwrap_or(99) as usize;\n",
    );
    s.push_str(
        "            let period = params[\"period\"].as_i64().unwrap_or(0);\n",
    );
    // apply_unstable_period rebuilds the immutable Core via the builder and handles
    // the `ALL` "set all" sentinel (matches C TA_SetUnstablePeriod). It reports an
    // out-of-range id and an out-of-range value distinctly, so a cross-language
    // failure names which half of the contract was broken.
    s.push_str("            match apply_unstable_period(core, id, period) {\n");
    s.push_str(
        "                Ok(()) => \"{\\\"status\\\":\\\"ok\\\"}\".to_string(),\n",
    );
    s.push_str(
        "                Err(msg) => format!(\"{{\\\"error\\\":\\\"{msg}\\\"}}\"),\n",
    );
    s.push_str("            }\n");
    s.push_str("        }\n");

    // set_candle_settings method (#215).
    s.push_str("        \"set_candle_settings\" => {\n");
    s.push_str("            RIDE_GEN.with(|g| g.set(g.get().wrapping_add(1)));\n");
    s.push_str("            let st = params[\"settingType\"].as_i64().unwrap_or(-1);\n");
    s.push_str("            let rt = params[\"rangeType\"].as_i64().unwrap_or(-1);\n");
    s.push_str("            let ap = params[\"avgPeriod\"].as_i64().unwrap_or(0);\n");
    s.push_str("            let factor = parse_f64_bits(&params[\"factorBits\"], 1.0);\n");
    s.push_str("            match apply_candle_setting(core, st, rt, ap, factor) {\n");
    s.push_str(
        "                Ok(()) => \"{\\\"status\\\":\\\"ok\\\"}\".to_string(),\n",
    );
    s.push_str(
        "                Err(msg) => format!(\"{{\\\"error\\\":\\\"{msg}\\\"}}\"),\n",
    );
    s.push_str("            }\n");
    s.push_str("        }\n");

    // restore_candle_default_settings method (#215).
    s.push_str("        \"restore_candle_default_settings\" => {\n");
    s.push_str("            RIDE_GEN.with(|g| g.set(g.get().wrapping_add(1)));\n");
    s.push_str("            let st = params[\"settingType\"].as_i64().unwrap_or(-1);\n");
    s.push_str("            match apply_restore_candle_default(core, st) {\n");
    s.push_str(
        "                Ok(()) => \"{\\\"status\\\":\\\"ok\\\"}\".to_string(),\n",
    );
    s.push_str(
        "                Err(msg) => format!(\"{{\\\"error\\\":\\\"{msg}\\\"}}\"),\n",
    );
    s.push_str("            }\n");
    s.push_str("        }\n");

    // eval_predicate method — boolean near-zero builtin on each input value.
    s.push_str("        \"eval_predicate\" => {\n");
    s.push_str("            let which = params[\"which\"].as_i64().unwrap_or(0);\n");
    s.push_str("            let values = parse_f64_array(&params[\"values\"]);\n");
    s.push_str("            let scale = parse_f64_array(&params[\"scale\"]);\n");
    s.push_str("            let out: Vec<i32> = values.iter().enumerate().map(|(i, &v)| {\n");
    s.push_str("                let s = *scale.get(i).unwrap_or(&0.0);\n");
    s.push_str("                let r = match which {\n");
    s.push_str(&format!(
        "                    1 => {},\n",
        rust_predicate_expr(SpecialBuiltin::IsZeroScaled, &["v".to_string(), "s".to_string()])
    ));
    s.push_str(&format!(
        "                    2 => {},\n",
        rust_predicate_expr(SpecialBuiltin::IsZeroOrNeg, &["v".to_string()])
    ));
    s.push_str(&format!(
        "                    _ => {},\n",
        rust_predicate_expr(SpecialBuiltin::IsZero, &["v".to_string()])
    ));
    s.push_str("                };\n");
    s.push_str("                i32::from(r)\n");
    s.push_str("            }).collect();\n");
    s.push_str("            format!(\"{{\\\"outInteger\\\":{}}}\", json_i32_array(&out))\n");
    s.push_str("        }\n");

    // fuzz_in_hash — cross-language input-port self-check (--xlang-hash, issue #113).
    // Generates the OHLCV+OI inputs from (gen_shape,gen_seed,gen_n) and returns a
    // 64-bit FNV digest of the six raw arrays in O,H,L,C,V,OI order, byte-identical
    // to the C driver's in-process generation — so a ported-fuzz_gen divergence is
    // caught as an INPUT mismatch, isolated from any indicator-output divergence.
    s.push_str("        \"fuzz_in_hash\" => {\n");
    s.push_str("            let shape = params[\"gen_shape\"].as_i64().unwrap_or(0) as i32;\n");
    s.push_str("            let seed = params[\"gen_seed\"].as_i64().unwrap_or(0) as i32;\n");
    s.push_str("            let n = params[\"gen_n\"].as_i64().unwrap_or(0) as usize;\n");
    s.push_str("            let mut fo = vec![0.0f64; n]; let mut fh = vec![0.0f64; n]; let mut fl = vec![0.0f64; n];\n");
    s.push_str("            let mut fc = vec![0.0f64; n]; let mut fv = vec![0.0f64; n]; let mut foi = vec![0.0f64; n];\n");
    s.push_str("            fuzz_gen(shape, seed, n as i32, &mut fo, &mut fh, &mut fl, &mut fc, &mut fv, &mut foi);\n");
    s.push_str("            let mut h = fuzz_hash_init();\n");
    s.push_str("            for arr in [&fo, &fh, &fl, &fc, &fv, &foi] { h = fuzz_hash_bytes_f64(h, arr); }\n");
    s.push_str("            h = fuzz_hash_fin(h);\n");
    s.push_str("            format!(\"{{\\\"in_hash\\\":\\\"{:016x}\\\"}}\", h)\n");
    s.push_str("        }\n");

    // Stream verify: Rust stream vs Rust batch, in-process bitwise (the same
    // driver pass the C server answers; see generate_rust_stream_verify).
    s.push_str("        \"stream_verify\" => handle_stream_verify(core, params),\n");

    // Abstract/introspection metadata handlers (mirror ta_abstract_serve.c),
    // backed by the generated abstract_api registry. Used by ta_regtest to lock
    // Rust introspection metadata parity against the C reference.
    s.push_str(RUST_ABSTRACT_METADATA_HANDLERS);

    // Abstract dynamic-dispatch handlers (abstract_call, abstract_get_lookback,
    // abstract_for_each_func) + TA_FunctionDescriptionXML. Completes the Rust mirror
    // of C's ta_abstract serve path so the full test_abstract() drives the Rust
    // server (numeric output comparison, not just metadata). Both dynamic arms
    // bind through the SHIPPED abstract_api::ParamHolder -- see RUST_ABSTRACT_BINDER.
    s.push_str(RUST_ABSTRACT_DYNAMIC_HANDLERS);

    // Unknown method
    s.push_str("        _ => {\n");
    s.push_str(
        "            format!(\"{{\\\"error\\\":\\\"Unknown method: {}\\\"}}\", method)\n",
    );
    s.push_str("        }\n");
    s.push_str("    }\n");
    s.push_str("}\n\n");

    // The dynamic tier binds through the SHIPPED abstract_api::ParamHolder, so
    // test_abstract.c drives the surface that ships rather than a copy that only
    // exists in this file (issue #164 — the same correction D1 made for Java).
    s.push_str(RUST_ABSTRACT_BINDER);

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

    // Stream verify section (sv_<name> per streamable function + dispatcher).
    s.push_str(&crate::stream_verify_gen::rust_lang::generate_rust_stream_verify(funcs, enums));
    s.push_str(&crate::ride_gen::rust_lang::generate_rust_ridealong(funcs));

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
                    let (ty, default): (i32, f64) = match oi.kind {
                        OptInputType::RealRange { default, .. } => (0, default),
                        OptInputType::RealList { default, .. } => (1, default),
                        OptInputType::IntegerRange { default, .. } => (2, default as f64),
                        OptInputType::IntegerList { default, .. } => (3, default as f64),
                    };
                    let mut resp = serde_json::json!({
                        "type": ty,
                        "paramName": oi.param_name,
                        "flags": oi.flags.bits(),
                        "displayName": oi.display_name,
                        "hint": oi.hint,
                        "defaultValue": default,
                    });
                    match oi.kind {
                        OptInputType::RealRange { min, max, precision, suggested, .. } => {
                            resp["min"] = serde_json::json!(min);
                            resp["max"] = serde_json::json!(max);
                            resp["precision"] = serde_json::json!(precision);
                            resp["suggestedStart"] = serde_json::json!(suggested.0);
                            resp["suggestedEnd"] = serde_json::json!(suggested.1);
                            resp["suggestedIncrement"] = serde_json::json!(suggested.2);
                        }
                        OptInputType::IntegerRange { min, max, suggested, .. } => {
                            resp["min"] = serde_json::json!(min);
                            resp["max"] = serde_json::json!(max);
                            resp["suggestedStart"] = serde_json::json!(suggested.0);
                            resp["suggestedEnd"] = serde_json::json!(suggested.1);
                            resp["suggestedIncrement"] = serde_json::json!(suggested.2);
                        }
                        OptInputType::IntegerList { values, .. } => {
                            let mut vl = String::new();
                            for (i, (v, label)) in values.iter().enumerate() {
                                if i > 0 { vl.push(';'); }
                                vl.push_str(&format!("{}={}", v, label));
                            }
                            resp["valueList"] = serde_json::json!(vl);
                        }
                        OptInputType::RealList { .. } => {}
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

/// The Rust server's ta_abstract dynamic tier, bound through the SHIPPED
/// `abstract_api::ParamHolder` rather than a server-local dispatch.
///
/// Before this, `abstract_call` rerouted to the per-function handler and
/// `abstract_get_lookback` used a generated 168-arm match reading JSON — so
/// `test_abstract.c` proved the Rust *metadata* and nothing about a shipped
/// binder, because there wasn't one. Now the gate drives what ships, which is the
/// same correction D1 made for Java (issue #164).
const RUST_ABSTRACT_BINDER: &str = r#"
/// One input array. Delegates to `parse_f64_array` rather than reimplementing the
/// number-array half: that function also decodes the lossless hex-of-IEEE-bits
/// encoding (issue #115), which the per-function handlers accept and which the
/// reroute this replaced therefore accepted too. Handling only decimals here would
/// answer an empty slice for a hex payload, and an empty input slice reaches the
/// guarded body's bounds assert and panics the process.
fn abs_f64s(params: &Value, key: &str) -> Vec<f64> {
    parse_f64_array(&params[key])
}

/// `None` for an absent component, so `set_price_input` sees exactly what the
/// request carried — a component the function does not consume is accepted and
/// ignored, as in C and the other binders.
fn abs_opt(v: &[f64]) -> Option<&[f64]> {
    if v.is_empty() { None } else { Some(v) }
}

/// Bind every declared parameter from the request and run the call, through the
/// shipped binder. Answers the same shape the C server's ta_abstract path does.
fn abs_call(core: &Core, params: &Value) -> String {
    let fname = params["funcName"].as_str().unwrap_or("");
    let Some(id) = abstract_api::get_func_handle(fname) else {
        return format!("{{\"error\":\"Unknown function: {fname}\"}}");
    };
    let info = id.info();
    // C answers TA_OUT_OF_RANGE_START_INDEX / _END_INDEX for these rather than
    // clamping, and the driver compares retCodes.
    let raw_start = params["startIdx"].as_i64().unwrap_or(0);
    let raw_end = params["endIdx"].as_i64().unwrap_or(0);
    if raw_start < 0 || raw_start > Core::MAX_INDEX as i64 {
        return format!("{{\"binder\":1,\"lookback\":-1,\"retCode\":{},\"outBegIdx\":0,\"outNBElement\":0}}",
                       retcode_to_int(RetCode::OutOfRangeStartIndex));
    }
    if raw_end < 0 || raw_end > Core::MAX_INDEX as i64 || raw_end < raw_start {
        return format!("{{\"binder\":1,\"lookback\":-1,\"retCode\":{},\"outBegIdx\":0,\"outNBElement\":0}}",
                       retcode_to_int(RetCode::OutOfRangeEndIndex));
    }
    let start = raw_start as usize;
    let end = raw_end as usize;
    let n = end - start + 1;

    // Declared before the holder so they outlive the borrows it takes.
    let po = abs_f64s(params, "inOpen");
    let ph = abs_f64s(params, "inHigh");
    let pl = abs_f64s(params, "inLow");
    let pc = abs_f64s(params, "inClose");
    let pv = abs_f64s(params, "inVolume");
    let pi = abs_f64s(params, "inOpenInterest");

    let generic_total = info.inputs.iter().filter(|i| i.kind != InputType::Price).count();
    let mut reals: Vec<Vec<f64>> = Vec::new();
    for k in 0..generic_total {
        let key = if generic_total == 1 { "inReal".to_string() } else { format!("inReal{k}") };
        reals.push(abs_f64s(params, &key));
    }
    // No shipped function declares an integer input; carried so the arm is total.
    let ints: Vec<Vec<i32>> = (0..generic_total).map(|_| Vec::new()).collect();

    let mut rbuf: Vec<Vec<f64>> = (0..info.outputs.len()).map(|_| vec![0.0; n]).collect();
    let mut ibuf: Vec<Vec<i32>> = (0..info.outputs.len()).map(|_| vec![0; n]).collect();

    let (rc, lb, beg, nb) = {
        let mut h = id.new_call(core);
        // The first bind failure is ANSWERED, not swallowed. A discarded Err
        // leaves the parameter at its constructor sentinel, which every function
        // maps to that parameter's documented default -- and the only vectors that
        // drive this path bind the defaults, so a binder that REJECTED the bind
        // would have produced byte-identical output and a green gate.
        let mut bind_err: Option<RetCode> = None;
        let mut note = |r: Result<&mut abstract_api::ParamHolder<'_>, RetCode>| {
            if let Err(e) = r { if bind_err.is_none() { bind_err = Some(e); } }
        };
        let mut gi = 0usize;
        for (slot, inp) in info.inputs.iter().enumerate() {
            match inp.kind {
                InputType::Price => {
                    note(h.set_price_input(slot, abs_opt(&po), abs_opt(&ph), abs_opt(&pl),
                                           abs_opt(&pc), abs_opt(&pv), abs_opt(&pi)));
                }
                InputType::Real => { note(h.set_input(slot, &reals[gi])); gi += 1; }
                InputType::Integer => { note(h.set_int_input(slot, &ints[gi])); gi += 1; }
            }
        }
        for (k, opt) in info.opt_inputs.iter().enumerate() {
            match opt.kind {
                OptInputType::RealRange { .. } | OptInputType::RealList { .. } => {
                    if let Some(v) = params[opt.param_name].as_f64() { note(h.set_opt(k, v)); }
                }
                _ => {
                    if let Some(v) = params[opt.param_name].as_i64() {
                        note(h.set_opt(k, v as i32));
                    }
                }
            }
        }
        for (k, buf) in rbuf.iter_mut().enumerate() {
            if info.outputs[k].kind == OutputType::Real { note(h.set_output(k, buf)); }
        }
        for (k, buf) in ibuf.iter_mut().enumerate() {
            if info.outputs[k].kind == OutputType::Integer { note(h.set_int_output(k, buf)); }
        }

        let lb = h.lookback().map_or(-1i64, |v| v as i64);
        if let Some(e) = bind_err {
            (retcode_to_int(e), lb, 0usize, 0usize)
        } else {
            match h.call(start, end) {
                Ok(r) => (0i32, lb, r.beg_idx, r.count),
                Err(e) => (retcode_to_int(e), lb, 0usize, 0usize),
            }
        }
    };

    // `binder:1` says this reply came from the SHIPPED ParamHolder. The transport
    // split below is chosen by sniffing request flags, so without a positive
    // marker, adding want_hash to the driver would silently move the whole sweep
    // back onto the per-function handler with every assertion still passing --
    // the same vacuity shape the choice-list floor exists to catch, pointing the
    // other way. test_abstract.c requires it of the Rust server.
    let mut out = format!(
        "{{\"binder\":1,\"lookback\":{lb},\"retCode\":{rc},\"outBegIdx\":{beg},\"outNBElement\":{nb}"
    );
    // Real and integer outputs are numbered from INDEPENDENT counters, matching
    // the driver: MINMAXINDEX has two integer outputs, and one shared key would
    // make the second overwrite the first.
    let mut ri = 0usize;
    let mut ii = 0usize;
    for (k, o) in info.outputs.iter().enumerate() {
        let is_real = o.kind == OutputType::Real;
        let key = if is_real {
            let s = if ri == 0 { "outReal".to_string() } else { format!("outReal{ri}") };
            ri += 1;
            s
        } else {
            let s = if ii == 0 { "outInteger".to_string() } else { format!("outInteger{ii}") };
            ii += 1;
            s
        };
        out.push_str(&format!(",\"{key}\":"));
        if is_real {
            out.push_str(&json_f64_array(&rbuf[k][..nb]));
        } else {
            let items: Vec<String> = ibuf[k][..nb].iter().map(ToString::to_string).collect();
            out.push_str(&format!("[{}]", items.join(",")));
        }
    }
    out.push('}');
    out
}

/// The lookback tier, through the same binder.
fn abs_lookback(core: &Core, params: &Value) -> Option<i64> {
    let fname = params["funcName"].as_str().unwrap_or("");
    let id = abstract_api::get_func_handle(fname)?;
    let mut h = id.new_call(core);
    for (k, opt) in id.info().opt_inputs.iter().enumerate() {
        match opt.kind {
            OptInputType::RealRange { .. } | OptInputType::RealList { .. } => {
                if let Some(v) = params[opt.param_name].as_f64() { let _ = h.set_opt(k, v); }
            }
            _ => {
                if let Some(v) = params[opt.param_name].as_i64() { let _ = h.set_opt(k, v as i32); }
            }
        }
    }
    Some(h.lookback().map_or(-1i64, |v| v as i64))
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
            // Two callers, two contracts. test_abstract.c drives the BINDER and
            // wants values back; --xlang-hash drives the same RPC as its seed
            // transport and wants the per-function handler's fuzz-generated
            // inputs and out_hash, which is a statement about the FUNCTION, not
            // about a binder. Route by what the request carries.
            if params["gen_present"].as_i64().unwrap_or(0) != 0
                || params["want_hash"].as_i64().unwrap_or(0) != 0
            {
                let rerouted = format!("TA_{}", fname);
                dispatch(core, ref_data, &rerouted, params)
            } else {
                let _ = ref_data;
                abs_call(core, params)
            }
        }
        "abstract_get_lookback" => {
            let fname = params["funcName"].as_str().unwrap_or("");
            match abs_lookback(core, params) {
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

/// Why the servers size their output buffers to the produced count rather than
/// the width of the requested range (#236 step 2). One text, three indentations.
const DOC_PRODUCED_EXTENT: &str = "\
The output buffers are sized to the count the call actually PRODUCES --\n\
endIdx - max(startIdx, lookback) + 1 -- plus `out_pad` from the request, and\n\
never below one. Not to the width of the requested range: that is the bound the\n\
managed backends check and the Rust asserts state, and at the range width it was\n\
slack by exactly the lookback, so no call could ever approach it.\n\
The pad is there because a bound is a MINIMUM, never an equality. A caller\n\
re-using a pre-allocated buffer passes a larger one, and that is not an error --\n\
the reported OutRange is what says which part was written. So the harness sends\n\
both: the startIdx axis sends no pad (the bound is reachable) while the\n\
full-range value comparison sends one (slack is legal). Sizing every call one way\n\
would silently drop the other property.\n\
FLOORED AT ONE, deliberately. Zero is what the formula gives for a rejected call\n\
(the lookback is -1, or usize::MAX in Rust, for an out-of-range parameter) and\n\
for a range shorter than the lookback, where the output bound switches off and\n\
the spec says any length will do, including none. It does not: two EMPTY output\n\
buffers are rejected as aliased by C# (an explicit IsEmpty clause) and by Rust\n\
(the empty Vec the server hands each output shares one dangling as_ptr()), and\n\
accepted by C and Java -- a four-way divergence on a call the specification says\n\
all four accept. Sizing to zero here would reach it on every multi-output\n\
function, which is a semantic question, not a harness one. Recorded as\n\
error-handling-spec, open item 11.\n\
The C server keeps its MAX_ARRAY_SIZE statics: C is handed bare pointers, has no\n\
sizes and cannot make the check, so an exact buffer would test nothing there.";

/// [`DOC_PRODUCED_EXTENT`] as an 8-space `//` comment block (Java, C#).
fn doc_produced_extent(indent: &str, marker: &str) -> String {
    let mut out = String::new();
    for line in DOC_PRODUCED_EXTENT.lines() {
        let _ = writeln!(out, "{indent}{marker} {line}");
    }
    out
}

/// The Java port of `fuzz_data.h` (byte-identical input generation, verified
/// bit-for-bit against the C original by a differential harness at port time).
const JAVA_FUZZ: &str = include_str!("../templates/java/FuzzData.java");

/// The scaffolding classes appended after the server's main class: the fuzz
/// generator and the streaming open-reject exception (mirrors the shipped
/// library's hand-written `InsufficientHistoryException`).
pub(crate) fn java_server_stream_scaffolding() -> String {
    let mut s = String::new();
    s.push_str(JAVA_FAILURES);
    s.push('\n');
    s.push_str(JAVA_IHE);
    s.push('\n');
    s.push_str(JAVA_FUZZ);
    s
}

/// Default-package twin of the shipped hand-written
/// `InsufficientHistoryException` (template file, per the no-inline-scaffolding
/// rule in CLAUDE.md).
const JAVA_IHE: &str = include_str!("../templates/java/InsufficientHistoryException.java");

/// Default-package twins of the shipped `TaLibFailure` interface and the four
/// exception classes that carry a `RetCode` (#236 step 1). Same rule, same
/// reason: the spliced wrappers throw these by name.
const JAVA_FAILURES: &str = include_str!("../templates/java/Failures.java");
/// The C# server's `ta_abstract` handlers. Fixed source — every answer is read
/// from the shipped `TALib.Metadata` catalogue, which the server csproj compiles
/// directly, so there is no second metadata table to drift.
const CSHARP_ABSTRACT_HANDLERS: &str = r#"    static string AbsStr(string? v) {
        if (v is null) return "\"\"";
        var b = new System.Text.StringBuilder("\"");
        foreach (char c in v) {
            /* The full JSON string grammar, not just quote and backslash. The
               transport is NEWLINE-FRAMED (codegen_pipe reads to the next '\n'),
               so an unescaped control character in an error message would split
               one reply into two lines and hand the second to the NEXT request --
               desynchronising the stream permanently, which is worse than the
               crash the surrounding try/catch replaces. */
            switch (c) {
                case '"':  b.Append("\\\""); break;
                case '\\': b.Append("\\\\"); break;
                case '\b': b.Append("\\b"); break;
                case '\f': b.Append("\\f"); break;
                case '\n': b.Append("\\n"); break;
                case '\r': b.Append("\\r"); break;
                case '\t': b.Append("\\t"); break;
                default:
                    if (c < 0x20) b.Append("\\u").Append(((int)c).ToString("x4"));
                    else b.Append(c);
                    break;
            }
        }
        b.Append('"');
        return b.ToString();
    }

    static string R(double v) => v.ToString("R", System.Globalization.CultureInfo.InvariantCulture);

    static int DomainCode(OptInputDomain d) => d switch {
        OptInputDomain.RealRange => 0,
        OptInputDomain.RealList => 1,
        OptInputDomain.IntegerRange => 2,
        OptInputDomain.IntegerList => 3,
        _ => throw new InvalidOperationException("unhandled OptInputDomain"),
    };

    static FunctionInfo? AbsLookup(JsonElement p) =>
        FunctionCatalog.Default.TryGet(p.GetProperty("funcName").GetString()!, out var f) ? f : null;

    static string AbsFuncInfo(JsonElement p) {
        var f = AbsLookup(p);
        if (f is null) return "{\"retCode\":2}";
        return $"{{\"name\":{AbsStr(f.Name)},\"group\":{AbsStr(f.Group.ToDisplayName())}"
             + $",\"hint\":{AbsStr(f.Hint)}"
             + $",\"flags\":{(uint)f.Flags},\"nbInput\":{f.Inputs.Length}"
             + $",\"nbOptInput\":{f.OptInputs.Length},\"nbOutput\":{f.Outputs.Length}}}";
    }

    static string AbsInputInfo(JsonElement p) {
        var f = AbsLookup(p);
        int i = GetInt(p, "paramIndex", -1);
        if (f is null || i < 0 || i >= f.Inputs.Length) return "{\"retCode\":2}";
        var ii = f.Inputs[i];
        return $"{{\"type\":{(int)ii.Kind},\"paramName\":{AbsStr(ii.ParamName)},\"flags\":{(uint)ii.Components}}}";
    }

    static string AbsOutputInfo(JsonElement p) {
        var f = AbsLookup(p);
        int i = GetInt(p, "paramIndex", -1);
        if (f is null || i < 0 || i >= f.Outputs.Length) return "{\"retCode\":2}";
        var oo = f.Outputs[i];
        return $"{{\"type\":{(int)oo.Kind},\"paramName\":{AbsStr(oo.ParamName)},\"flags\":{(uint)oo.Flags}}}";
    }

    static string AbsOptInputInfo(JsonElement p) {
        var f = AbsLookup(p);
        int i = GetInt(p, "paramIndex", -1);
        if (f is null || i < 0 || i >= f.OptInputs.Length) return "{\"retCode\":2}";
        var o = f.OptInputs[i];
        var b = new System.Text.StringBuilder($"{{\"type\":{DomainCode(o.Domain)}")
            .Append($",\"paramName\":{AbsStr(o.ParamName)}")
            .Append($",\"flags\":{(uint)o.Flags}")
            .Append($",\"displayName\":{AbsStr(o.DisplayName)}")
            .Append($",\"hint\":{AbsStr(o.Hint)}")
            .Append($",\"defaultValue\":{R(o.DefaultValue)}");
        switch (o.Domain) {
            case OptInputDomain.RealRange r:
                b.Append($",\"min\":{R(r.Min)},\"max\":{R(r.Max)},\"precision\":{r.Precision}")
                 .Append($",\"suggestedStart\":{R(r.SuggestedStart)}")
                 .Append($",\"suggestedEnd\":{R(r.SuggestedEnd)}")
                 .Append($",\"suggestedIncrement\":{R(r.SuggestedIncrement)}");
                break;
            case OptInputDomain.IntegerRange r:
                b.Append($",\"min\":{r.Min},\"max\":{r.Max}")
                 .Append($",\"suggestedStart\":{r.SuggestedStart}")
                 .Append($",\"suggestedEnd\":{r.SuggestedEnd}")
                 .Append($",\"suggestedIncrement\":{r.SuggestedIncrement}");
                break;
            case OptInputDomain.IntegerList l:
                b.Append($",\"valueList\":{AbsStr(l.ToValueListString())}");
                break;
            case OptInputDomain.RealList l:
                b.Append($",\"valueList\":{AbsStr(l.ToValueListString())}");
                break;
            default:
                throw new InvalidOperationException("unhandled OptInputDomain");
        }
        b.Append('}');
        return b.ToString();
    }

    static string AbsForEachFunc() {
        var b = new System.Text.StringBuilder("{\"functions\":[");
        bool first = true;
        foreach (var f in FunctionCatalog.Default) {
            if (!first) b.Append(',');
            first = false;
            b.Append($"{{\"name\":{AbsStr(f.Name)},\"group\":{AbsStr(f.Group.ToDisplayName())}")
             .Append($",\"nbInput\":{f.Inputs.Length},\"nbOptInput\":{f.OptInputs.Length}")
             .Append($",\"nbOutput\":{f.Outputs.Length}}}");
        }
        b.Append("]}");
        return b.ToString();
    }

    /* Measured at RUN TIME from the SHIPPED FunctionDescription. Baking the two
       numbers at generation time made this leg unfailable: it compared C's real
       bytes against constants derived from the same string C's own table is
       built from (#164). Now both sides are real bytes. */
    static string AbsDescriptionXml()
    {
        string xml = TALib.Metadata.FunctionDescription.Xml;
        ulong checksum = 0;
        foreach (char c in xml) checksum += (ulong)(c & 0xFF);
        return $"{{\"length\":{xml.Length},\"checksum\":{checksum}}}";
    }

    /* The JSON key the driver sends a required input under. Price bundles are
       sent one component per set bit; a lone real input keeps its own name,
       and several become inReal0/inReal1/... by rank (test_abstract.c's
       abstract_verify_server_call and expand_input_names agree on this). */
    static string AbsRealInputKey(FunctionInfo f, int slot) {
        int totalReal = 0, rank = 0;
        for (int i = 0; i < f.Inputs.Length; i++) {
            if (f.Inputs[i].Kind != InputKind.Real) continue;
            if (i < slot) rank++;
            totalReal++;
        }
        return totalReal == 1 ? f.Inputs[slot].ParamName : $"inReal{rank}";
    }

    static string AbsComponentKey(PriceComponents c) => c switch {
        PriceComponents.Open => "inOpen",
        PriceComponents.High => "inHigh",
        PriceComponents.Low => "inLow",
        PriceComponents.Close => "inClose",
        PriceComponents.Volume => "inVolume",
        PriceComponents.OpenInterest => "inOpenInterest",
        _ => throw new ArgumentException($"not a single component: {c}"),
    };

    /* abstract_call — the fully generic path, bound through FunctionCall. This
       is a genuinely independent second implementation rather than a reroute to
       the per-function handler (which is what the Rust and Java servers do), so
       a wrong slot index or a transposed price component shows up as diverging
       VALUES against the C reference. */
    static string AbsCall(JsonElement p) {
        var f = AbsLookup(p);
        if (f is null) return "{\"error\":\"Unknown function\"}";
        int startIdx = GetInt(p, "startIdx", 0);
        int endIdx = GetInt(p, "endIdx", 0);
        // Answer the range codes BEFORE sizing anything by the range (#180).
        // `n` below drives every output allocation, so validating after it
        // would turn an out-of-range request into an 800MB-per-output
        // allocation and take the server down instead of returning a code.
        if (startIdx < 0 || startIdx > Core.MAX_INDEX)
            return "{\"binder\":1,\"lookback\":-1,\"retCode\":12,\"outBegIdx\":0,\"outNBElement\":0}";
        if (endIdx < 0 || endIdx > Core.MAX_INDEX || endIdx < startIdx)
            return "{\"binder\":1,\"lookback\":-1,\"retCode\":13,\"outBegIdx\":0,\"outNBElement\":0}";
        int n = endIdx - startIdx + 1;
        if (n < 1) n = 1;

        var call = f.CreateCall(core);
        for (int i = 0; i < f.Inputs.Length; i++) {
            var info = f.Inputs[i];
            if (info.Kind == InputKind.Price) {
                foreach (var comp in info.SignatureOrder) {
                    call.SetPriceInput(i, comp, GetDoubleArray(p, AbsComponentKey(comp)));
                }
            } else if (info.Kind == InputKind.Real) {
                call.SetInput(i, GetDoubleArray(p, AbsRealInputKey(f, i)));
            } else {
                var raw = GetDoubleArray(p, info.ParamName);
                var ints = new int[raw.Length];
                for (int k = 0; k < raw.Length; k++) ints[k] = (int)raw[k];
                call.SetInput(i, ints);
            }
        }

        if (f.UnstableId is FuncUnstId unstId) {
            core.unstablePeriod[(int)unstId] = GetInt(p, "unstablePeriod", 0);
        }

        for (int i = 0; i < f.OptInputs.Length; i++) {
            var o = f.OptInputs[i];
            if (o.Domain is OptInputDomain.RealRange or OptInputDomain.RealList) {
                call.SetOption(i, GetDouble(p, o.ParamName, o.DefaultValue));
            } else {
                call.SetOption(i, GetInt(p, o.ParamName, (int)o.DefaultValue));
            }
        }

        var realOuts = new double[f.Outputs.Length][];
        var intOuts = new int[f.Outputs.Length][];
        for (int k = 0; k < f.Outputs.Length; k++) {
            if (f.Outputs[k].Kind == OutputKind.Real) {
                realOuts[k] = new double[n];
                call.SetOutput(k, realOuts[k]);
            } else {
                intOuts[k] = new int[n];
                call.SetOutput(k, intOuts[k]);
            }
        }

        int lookback = call.Lookback();
        RetCode rc = call.TryInvoke(startIdx, endIdx, out OutRange range);

        var b = new System.Text.StringBuilder();
        b.Append($"{{\"lookback\":{lookback},\"retCode\":{(int)rc}")
         .Append($",\"outBegIdx\":{range.BegIdx},\"outNBElement\":{range.Count}");
        int realRank = 0, intRank = 0;
        for (int k = 0; k < f.Outputs.Length; k++) {
            if (f.Outputs[k].Kind == OutputKind.Real) {
                string key = realRank == 0 ? "outReal" : $"outReal{realRank}";
                realRank++;
                b.Append($",\"{key}\":").Append(FormatArray(realOuts[k], range.Count));
            } else {
                string key = intRank == 0 ? "outInteger" : $"outInteger{intRank}";
                intRank++;
                b.Append($",\"{key}\":").Append(FormatIntArray(intOuts[k], range.Count));
            }
        }
        b.Append('}');
        return b.ToString();
    }

"#;

// ---------------------------------------------------------------------------

#[cfg(test)]
mod predicate_form_tests {
    use super::{c_predicate_expr, java_predicate_expr, rust_predicate_expr, SpecialBuiltin};

    /// Pin the exact per-backend form of the boolean near-zero builtins. These are
    /// the single source shared by the indicator code path AND the eval_predicate
    /// server handler, so any drift here is caught fast (and the runtime
    /// cross-language predicate-parity test in ta_regtest re-verifies equivalence).
    #[test]
    fn predicate_forms_are_stable() {
        let v = &["v".to_string()];
        let vs = &["v".to_string(), "s".to_string()];

        assert_eq!(c_predicate_expr(SpecialBuiltin::IsZero, v), "TA_IS_ZERO(v)");
        assert_eq!(c_predicate_expr(SpecialBuiltin::IsZeroScaled, vs), "TA_IS_ZERO_SCALED(v, s)");
        assert_eq!(c_predicate_expr(SpecialBuiltin::IsZeroOrNeg, v), "TA_IS_ZERO_OR_NEG(v)");

        assert_eq!(rust_predicate_expr(SpecialBuiltin::IsZero, v), "(v).abs() < 1e-14");
        assert_eq!(rust_predicate_expr(SpecialBuiltin::IsZeroScaled, vs), "((v).abs() <= 1e-14 * (s))");
        assert_eq!(rust_predicate_expr(SpecialBuiltin::IsZeroOrNeg, v), "(v) < 1e-14");

        assert_eq!(
            java_predicate_expr(SpecialBuiltin::IsZero, v),
            "((-0.00000000000001 < v) && (v < 0.00000000000001))"
        );
        assert_eq!(
            java_predicate_expr(SpecialBuiltin::IsZeroScaled, vs),
            "(Math.abs(v) <= 0.00000000000001 * (s))"
        );
        assert_eq!(java_predicate_expr(SpecialBuiltin::IsZeroOrNeg, v), "(v < 0.00000000000001)");
    }
}
