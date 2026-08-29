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
use std::collections::{BTreeMap, BTreeSet, HashMap};
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
fn func_unst_id(name: &str, enums: &HashMap<String, EnumDef>) -> Option<i32> {
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

    // The range head every generated stream struct leads with (issue #241).
    // TA_StreamOutRange reads a handle through this type, which is what lets ONE
    // public accessor serve every stream instead of one typed accessor per
    // function. Rendered from the same field list the structs are, so the two
    // cannot drift.
    s.push_str("/* The leading members of every struct TA_<N>_Stream, in order: the range of\n");
    s.push_str(" * bars the handle has produced a value for. TA_StreamOutRange (ta_utility.c)\n");
    s.push_str(" * copies a handle's head out through this type. */\n");
    s.push_str("typedef struct\n{\n");
    for decl in crate::backends::c_stream::RANGE_HEAD_FIELDS {
        s.push_str(&format!("   {decl}\n"));
    }
    s.push_str("} TA_StreamRangeHead;\n\n");

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
    // (docs/streaming-api-proposal.md, Verification). fuzz_data.h is included
    // HERE — after the indicator code — because its file-scope
    // `#pragma STDC FP_CONTRACT OFF` must not alter indicator contraction.
    // Absent entirely under TA_REF_SERVE (frozen libs have no stream symbols).
    s.push_str("#ifndef TA_REF_SERVE\n#include \"fuzz_data.h\"\n#endif\n\n");
    s.push_str(&generate_c_stream_verify(funcs, enums));

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

    s
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

/// Emit the per-output bitwise (double) / exact (int) comparison lines of a
/// stream_verify leg.
fn emit_sv_compare(
    s: &mut String,
    out_is_int: &[bool],
    bbuf: &[String],
    pad: &str,
    idx: &str,
    bar: &str,
    pre: &str,
) {
    for (i, is_int) in out_is_int.iter().enumerate() {
        let b = &bbuf[i];
        if *is_int {
            let _ = std::fmt::Write::write_fmt(s, format_args!(
                "{pad}if( {pre} v{i} != {b}[{idx}] ) {{ ok = 0; badBar = {bar}; badOut = {i}; bv = (double){b}[{idx}]; sv = (double)v{i}; }}\n"
            ));
        } else {
            let _ = std::fmt::Write::write_fmt(s, format_args!(
                "{pad}if( {pre} sv_xtier_ne(v{i}, {b}[{idx}], &svZsign) ) {{ ok = 0; badBar = {bar}; badOut = {i}; bv = {b}[{idx}]; sv = v{i}; }}\n"
            ));
        }
    }
}

/// The fuzz-convention input array for one expanded input name: price
/// components map to their OHLCV series; generic reals map real0→close,
/// real1→volume (matches abstract_call/fuzz-064 and the driver).
fn sv_input_array(name: &str, generic_idx: &mut usize) -> &'static str {
    match sv_input_suffix(name, generic_idx) {
        "o" => "sv_o",
        "h" => "sv_h",
        "l" => "sv_l",
        "c" => "sv_c",
        "v" => "sv_v",
        _ => "sv_oi",
    }
}

/// The bare OHLCV/OI suffix an input maps to (shared by the per-server
/// `sv_*`/`fz_*` array-name helpers so the mapping can never drift).
fn sv_input_suffix(name: &str, generic_idx: &mut usize) -> &'static str {
    match name {
        "inOpen" => "o",
        "inHigh" => "h",
        "inLow" => "l",
        "inClose" => "c",
        "inVolume" => "v",
        "inOpenInterest" => "oi",
        _ => {
            let arr = if *generic_idx == 0 { "c" } else { "v" };
            *generic_idx += 1;
            arr
        }
    }
}

/// Emit `handle_stream_verify`: for each streamable function, run batch
/// (startIdx=0) and the stream trajectory in-process on identical seeded
/// inputs, compare BITWISE per bar (memcmp on doubles), spot-assert
/// peek == update, and answer flat JSON (`ok`, per-leg match flags, first
/// divergence as %a on mismatch). See docs/streaming-api-proposal.md,
/// Verification. The whole handler is compiled out under TA_REF_SERVE
/// (frozen reference libraries have no stream symbols).
/// Tail of the batch-failure branch: candle functions record the outcome and
/// continue to the next settings round (a failed round must not truncate the
/// sweep); non-candle functions respond and return as before.
fn emit_sv_batch_fail_tail(s: &mut String, candle: bool) {
    // Reject parity: whenever the batch leg produced nothing — an error
    // (bad params, e.g. an out-of-list enum hitting a dispatch default arm)
    // or an empty range — the stream's Open must reject too. Open mirrors
    // the batch validation and min-history by construction, so a stream
    // that opens where batch fails is always a contract break. Never force
    // ok=1 on a batch error -- that shields exactly this case.
    if candle {
        s.push_str("            if( !openRejects ) allOk = 0;\n");
        s.push_str("            if( rd + 1 < rounds ) continue;\n");
        s.push_str("            TA_SetCompatibility((TA_Compatibility)savedCompat);\n");
        s.push_str("            TA_RestoreCandleDefaultSettings( TA_AllCandleSettings );\n");
        // Reachable after earlier candle rounds already compared, so the benign
        // count travels with it — otherwise those cases vanish from the summary.
        s.push_str("            pos = json_appendf(resp, resp_size, pos, \",\\\"rrc\\\":%d,\\\"legs\\\":%d,\\\"nb\\\":%d,\\\"openRejects\\\":%d,\\\"ok\\\":%d,\\\"peek_ok\\\":%d,\\\"benign\\\":%d}\", (int)rc, lgi, svNb, openRejects, allOk ? 1 : 0, peekAll, svZsign);\n");
    } else {
        s.push_str("            TA_SetCompatibility((TA_Compatibility)savedCompat);\n");
        s.push_str("            snprintf(resp, resp_size, \"{\\\"retCode\\\":%d,\\\"legs\\\":0,\\\"nb\\\":%d,\\\"openRejects\\\":%d,\\\"ok\\\":%d,\\\"peek_ok\\\":1}\", (int)rc, svNb, openRejects, openRejects);\n");
    }
    s.push_str("            return;\n");
    s.push_str("        }\n");
}

/// Period-bank functions (MAVP): the fuzz period-selector input (mapped to a
/// generic real series ~volume, always >= 1000) would clamp to `maxPeriod` at
/// every bar, so the stream_verify would only ever exercise ONE bank slot and
/// pass vacuously for all others. Overwrite the selector with a ramp spanning
/// `[minPeriod-1, maxPeriod+1]` (fed identically to the batch and the stream),
/// so every bank slot AND both clamp directions are exercised. Regenerated per
/// request (fuzz_gen runs first), so this override does not leak to other funcs.
fn emit_sv_period_bank_input(
    s: &mut String,
    func: &FuncDef,
    funcs: &[FuncDef],
    input_arrays: &[&str],
) {
    let lookup = crate::streaming::FuncsLookup(funcs);
    let Ok(crate::streaming::StreamPlan::PeriodBank(pb)) =
        crate::streaming::validate_streamable(func, &lookup)
    else {
        return;
    };
    let inputs = crate::streaming::input_array_names(func);
    let Some(idx) = inputs.iter().position(|i| *i == pb.period_input) else {
        return;
    };
    let arr = input_arrays[idx];
    s.push_str(&format!(
        "        {{ int _pi; for( _pi = 0; _pi < svN; _pi++ ) {arr}[_pi] = (double)({min} + (_pi % ({max} - {min} + 3)) - 1); }}\n",
        min = pb.min_param,
        max = pb.max_param
    ));
}

/// Dispatch functions (MA): enum values whose arm has no sub-stream reject
/// at Open — a DOCUMENTED capability limitation, verified loudly here
/// (never a silent vacuous pass). The identity path (period==1) is exempt:
/// it streams for every arm value, exactly as the batch checks it before
/// dispatching. The unsupported set is derived from the callees' stream
/// flags at generation time, so a callee gaining the flag (TRIMA) flips its
/// legs from expect-reject to verified automatically on the next generate.
fn emit_sv_dispatch_precheck(
    s: &mut String,
    func: &FuncDef,
    funcs: &[FuncDef],
    input_arrays: &[&str],
    n_outs: usize,
    name: &str,
) {
    let Some(guard) = sv_reject_condition(func, funcs, None) else {
        return;
    };
    let mut pre_opt_args = String::new();
    for o in &func.optional_inputs {
        let _ = std::fmt::Write::write_fmt(&mut pre_opt_args, format_args!("{}, ", o.name));
    }
    let mut pre_in_args = String::new();
    for a in input_arrays {
        let _ = std::fmt::Write::write_fmt(&mut pre_in_args, format_args!("{a}, "));
    }
    let decls: String = func
        .outputs
        .iter()
        .enumerate()
        .map(|(i, ou)| {
            if ou.param_type == ParamType::Integer {
                format!("int v{i} = 0;")
            } else {
                format!("double v{i} = 0.0;")
            }
        })
        .collect::<Vec<_>>()
        .join(" ");
    let addrs = (0..n_outs)
        .map(|i| format!("&v{i}"))
        .collect::<Vec<_>>()
        .join(", ");
    // An unsupported arm (MAMA) must reject at OpenAndFill too, not just Open —
    // otherwise a regression could return SUCCESS with an unwritten output array
    // (silent garbage) on the exact param the header documents as rejected.
    // Every streamable function has an OpenAndFill, so this is unconditional.
    let fill_block = {
        let (mut ri, mut ii) = (0usize, 0usize);
        let fill_bufs: String = func
            .outputs
            .iter()
            .map(|ou| {
                if ou.param_type == ParamType::Integer {
                    let e = format!("sv_if{ii}");
                    ii += 1;
                    e
                } else {
                    let e = format!("sv_f{ri}");
                    ri += 1;
                    e
                }
            })
            .collect::<Vec<_>>()
            .join(", ");
        format!(
            "            {{ TA_{name}_Stream *stf = NULL; int fBeg = 0, fNb = 0;\n              TA_RetCode frc = TA_{name}_OpenAndFill( &stf, {pre_in_args}svN, {pre_opt_args}&fBeg, &fNb, {fill_bufs} );\n              if( !( frc != TA_SUCCESS && !stf ) ) rejected = 0;\n              if( stf ) TA_{name}_Close( stf ); }}\n"
        )
    };
    s.push_str(&format!(
        "        if( {guard} )\n        {{\n            TA_{name}_Stream *st = NULL; {decls} TA_RetCode orc;\n            int rejected;\n            orc = TA_{name}_Open( &st, {pre_in_args}svN, {pre_opt_args}{addrs} );\n            rejected = ( orc != TA_SUCCESS && !st ) ? 1 : 0;\n            if( st ) TA_{name}_Close( st );\n{fill_block}            TA_SetCompatibility((TA_Compatibility)savedCompat);\n            snprintf(resp, resp_size, \"{{\\\"retCode\\\":0,\\\"legs\\\":0,\\\"unsupportedArm\\\":1,\\\"ok\\\":%d,\\\"peek_ok\\\":1}}\", rejected);\n            return;\n        }}\n"
    ));
}

/// Unstable-period ids a function's stream values depend on: its own id
/// plus every unstable id reachable through the TRANSITIVE closure of
/// `<base>_lookback` calls starting from its lookback body (STOCH ->
/// ma_lookback -> ema_lookback -> EMA). Composed/dispatch functions honor
/// ambient K only through the callees' lookbacks, so the lookback closure
/// covers exactly the sub-stream selection space.
fn collect_pin_ids(func: &FuncDef, funcs: &[FuncDef], enums: &HashMap<String, EnumDef>) -> Vec<i32> {
    let mut pin_ids: Vec<i32> = Vec::new();
    let mut visited: std::collections::BTreeSet<String> = std::collections::BTreeSet::new();
    let mut queue: Vec<String> = vec![func.name.to_uppercase()];
    while let Some(cur) = queue.pop() {
        if !visited.insert(cur.clone()) {
            continue;
        }
        if let Some(id) = func_unst_id(&cur, enums) {
            if !pin_ids.contains(&id) {
                pin_ids.push(id);
            }
        }
        let Some(fd) = funcs.iter().find(|f| f.name.eq_ignore_ascii_case(&cur)) else {
            continue;
        };
        if let Some(crate::ir::LookbackExpr::Code(stmts)) = &fd.lookback {
            for st in stmts {
                crate::streaming::walk_stmt_exprs(st, &mut |e| {
                    crate::streaming::walk_expr(e, &mut |x| {
                        if let crate::ir::Expr::FuncCall(fname, _) = x {
                            if let Some(base) = fname.strip_suffix("_lookback") {
                                queue.push(base.to_uppercase());
                            }
                        }
                    });
                });
            }
        }
    }
    pin_ids
}

/// True when `func`'s stream honestly rejects Open at exactly `lookback+1`
/// under Metastock — a seed boundary — either directly (RSI/CMO emit a seed
/// output then rewind, so no bit-exact continuation exists from the seed exit)
/// or through composition: a composed/dispatch function that consumes a
/// seed-boundary callee inherits the boundary (STOCHRSI's `rsi` sub-stream
/// cannot open at its own seed boundary, so STOCHRSI's Open rejects one bar
/// longer). The closure is the same `<base>_lookback` transitive walk
/// [`collect_pin_ids`] uses — every stream-composed callee appears there — so
/// the verifier shifts the boundary leg for exactly the functions whose stream
/// rejects it.
fn func_has_seed_boundary(func: &FuncDef, funcs: &[FuncDef]) -> bool {
    let mut visited: std::collections::BTreeSet<String> = std::collections::BTreeSet::new();
    let mut queue: Vec<String> = vec![func.name.to_uppercase()];
    while let Some(cur) = queue.pop() {
        if !visited.insert(cur.clone()) {
            continue;
        }
        let Some(fd) = funcs.iter().find(|f| f.name.eq_ignore_ascii_case(&cur)) else {
            continue;
        };
        // Direct (loop-tier) seed boundary. `analyze` is Err for composed /
        // dispatch bodies — those inherit the boundary through their callees.
        if let Ok(m) = crate::streaming::analyze(fd) {
            if m.seed_boundary {
                return true;
            }
        }
        if let Some(crate::ir::LookbackExpr::Code(stmts)) = &fd.lookback {
            for st in stmts {
                crate::streaming::walk_stmt_exprs(st, &mut |e| {
                    crate::streaming::walk_expr(e, &mut |x| {
                        if let crate::ir::Expr::FuncCall(fname, _) = x {
                            if let Some(base) = fname.strip_suffix("_lookback") {
                                queue.push(base.to_uppercase());
                            }
                        }
                    });
                });
            }
        }
    }
    false
}

/// The C condition under which a function's stream Open HONESTLY rejects a
/// param set the batch accepts (a documented capability limitation), or
/// None when no such set exists. Composes recursively:
/// - Dispatch (MA): `!identity && (param in unsupported labels)`.
/// - Composed (STOCH): OR over its sub-calls, with the sub's optional
///   argument EXPRESSIONS substituted for the callee's params — so MA's
///   `optInTimePeriod == 1` identity exemption becomes
///   `optInSlowK_Period == 1` at the STOCH level, and TRIMA landing later
///   narrows every dependent precheck automatically on regenerate.
/// - Loop tier: never (None).
///
/// `subst` maps the callee's param names to caller-level argument exprs
/// (None at the top level: the function's own params are in scope).
fn sv_reject_condition(
    func: &FuncDef,
    funcs: &[FuncDef],
    subst: Option<&std::collections::BTreeMap<String, crate::ir::Expr>>,
) -> Option<String> {
    use crate::ir::Expr;
    let lookup = crate::streaming::FuncsLookup(funcs);
    let render_arg = |e: &Expr| -> String {
        let mapped = match (e, subst) {
            (Expr::Var(v), Some(m)) => m.get(v).cloned().unwrap_or_else(|| e.clone()),
            _ => e.clone(),
        };
        sv_render_scalar(&mapped)
    };
    match crate::streaming::validate_streamable(func, &lookup) {
        Ok(crate::streaming::StreamPlan::Dispatch(dp)) => {
            let unsupported = dp.unsupported_labels();
            if unsupported.is_empty() {
                return None;
            }
            let param_c = render_arg(&Expr::Var(dp.param.clone()));
            let arm_match = unsupported
                .iter()
                .map(|l| {
                    let c_const = if l.starts_with("TA_") {
                        (*l).to_string()
                    } else {
                        format!("TA_{l}")
                    };
                    format!("{param_c} == {c_const}")
                })
                .collect::<Vec<_>>()
                .join(" || ");
            match dp.identity.as_ref().and_then(|i| {
                sv_identity_guard_subst(&i.condition, &render_arg)
            }) {
                Some(g) => Some(format!("( !({g}) && ( {arm_match} ) )")),
                None => Some(format!("( {arm_match} )")),
            }
        }
        Ok(crate::streaming::StreamPlan::Composed(cp)) => {
            let mut parts: Vec<String> = Vec::new();
            for sub in &cp.subs {
                let callee = funcs
                    .iter()
                    .find(|f| f.name.eq_ignore_ascii_case(&sub.callee))?;
                // Map the callee's params to the sub-call's argument exprs,
                // resolved through the CURRENT substitution.
                let mut m = std::collections::BTreeMap::new();
                for (p, a) in callee.optional_inputs.iter().zip(sub.opt_args.iter()) {
                    let resolved = match (a, subst) {
                        (Expr::Var(v), Some(outer)) => {
                            outer.get(v).cloned().unwrap_or_else(|| a.clone())
                        }
                        _ => a.clone(),
                    };
                    m.insert(p.name.clone(), resolved);
                }
                if let Some(cond) = sv_reject_condition(callee, funcs, Some(&m)) {
                    parts.push(cond);
                }
            }
            if parts.is_empty() {
                None
            } else {
                Some(format!("( {} )", parts.join(" || ")))
            }
        }
        Ok(crate::streaming::StreamPlan::PeriodBank(pb)) => {
            // The bank opens the callee (`ma`) at every period in [min,max], so
            // MAVP rejects when the callee rejects for the forwarded MAType at
            // ANY of those periods. The callee's period guard (its `period == 1`
            // identity path exempts MAType=MAMA) is resolved against the LARGEST
            // period in the bank: `ma(maxPeriod, MAMA)` rejects whenever
            // maxPeriod > 1, which is exactly when a non-identity slot exists.
            let callee = funcs
                .iter()
                .find(|f| f.name.eq_ignore_ascii_case(&pb.callee))?;
            let resolve = |name: &str| -> Expr {
                match subst {
                    Some(outer) => outer
                        .get(name)
                        .cloned()
                        .unwrap_or_else(|| Expr::Var(name.to_string())),
                    None => Expr::Var(name.to_string()),
                }
            };
            let mut m = std::collections::BTreeMap::new();
            for p in &callee.optional_inputs {
                match &p.param_type {
                    crate::ir::ParamType::Enum(e) if e == "MAType" => {
                        m.insert(p.name.clone(), resolve(&pb.matype_param));
                    }
                    crate::ir::ParamType::Integer => {
                        m.insert(p.name.clone(), resolve(&pb.max_param));
                    }
                    _ => {}
                }
            }
            sv_reject_condition(callee, funcs, Some(&m))
        }
        _ => None,
    }
}

/// Render a param-pure scalar expression for the verify precheck (the
/// analyzer guarantees purity; anything else is a generate-time panic so a
/// silently-omitted precheck can never ship).
fn sv_render_scalar(e: &crate::ir::Expr) -> String {
    use crate::ir::Expr;
    match e {
        Expr::Var(v) => v.clone(),
        Expr::IntLiteral(k) => k.to_string(),
        Expr::Literal(x) => format!("{x:?}"),
        _ => panic!("stream_verify precheck: unrenderable sub-call argument {e:?}"),
    }
}

/// The identity guard with the callee's params substituted through
/// `render_arg` (`optInTimePeriod == 1` -> `optInSlowK_Period == 1`).
fn sv_identity_guard_subst(
    cond: &crate::ir::Expr,
    render_arg: &dyn Fn(&crate::ir::Expr) -> String,
) -> Option<String> {
    use crate::ir::{BinOp, Expr};
    if let Expr::BinOp(l, op, r) = cond {
        if let (Expr::Var(_), Expr::IntLiteral(k)) = (l.as_ref(), r.as_ref()) {
            let op_s = match op {
                BinOp::Eq => "==",
                BinOp::LessEq => "<=",
                _ => return None,
            };
            let lhs = render_arg(l);
            return Some(format!("{lhs} {op_s} {k}"));
        }
    }
    None
}

/// Stamp the canary across the FULL width of the C fill buffers.
///
/// Not `svN`: a lookback-0 function fills the whole series, so `fNb == svN` and
/// a `[fNb, svN)` window is empty — the assert would be a no-op for exactly the
/// functions whose overrun has the furthest to travel. A one-past-the-range
/// write also lands at index `svN` itself. Stamp and assert must use the same
/// bound; widening only the assert would read whatever an earlier function in
/// the same request left in these `static` buffers and fail spuriously.
fn c_canary_stamp(fbuf: &[String], out_is_int: &[bool]) -> String {
    let mut s = String::from("            for( ft = 0; ft < SV_MAXN; ft++ ) {\n");
    for (i, is_int) in out_is_int.iter().enumerate() {
        let canary = if *is_int { "SV_FILL_CANARY_I" } else { "SV_FILL_CANARY" };
        let _ = writeln!(s, "               {}[ft] = {canary};", fbuf[i]);
    }
    s.push_str("            }\n");
    s
}

/// Assert the slack above the produced range still holds the canary. Nothing
/// else in the tree checks it: every gate sizes the fill buffer at full history
/// and compares only `[0, nb)`, so a write past `nb` lands in unread space.
fn c_canary_check(fbuf: &[String], out_is_int: &[bool]) -> String {
    let mut s = String::from("            if( frc == TA_SUCCESS )\n");
    s.push_str("               for( ft = fNb; fillOk && ft < SV_MAXN; ft++ ) {\n");
    for (i, is_int) in out_is_int.iter().enumerate() {
        let canary = if *is_int { "SV_FILL_CANARY_I" } else { "SV_FILL_CANARY" };
        let _ = writeln!(s, "                  if( {}[ft] != {canary} ) fillOk = 0;", fbuf[i]);
    }
    s.push_str("               }\n");
    s
}

// ---------------------------------------------------------------------------
// State-equivalence comparators (issue #240)
// ---------------------------------------------------------------------------
//
// A candlestick's only observable is a 3-valued integer, so an arithmetic error
// inside a `<Setting>PeriodTotal` stays invisible until it crosses a decision
// threshold. Measured on the #229 window fold: a permanent one-bar rotation of a
// folded ring read moved the OUTPUT of 3 of 14 functions and left 11 green in
// every language. It moves the STATE of all 14, on the first bar it is read.
//
// So compare state, not output: the handle after `Open(P)` plus `n - P` updates
// must equal the handle after `Open(n)`, bit for bit. That holds by
// construction — `Update` is the transcribed batch loop body, so both paths run
// the identical operation sequence over the identical bars, and the ring cursor
// is `historyLen % cap` on one side against the same number of `+1`s on the
// other. The property is independent of firing density and of decision margin,
// which is exactly what the output comparison is not.
//
// The comparator is derived from `c_stream::state_struct_text` — the same text
// the shipped struct is emitted from — so it cannot fall behind a new field, and
// a pointer field it cannot associate with a length is a hard generation
// failure, never a silent skip.

/// How the state comparator treats one pointer field of a stream struct.
enum SvPtr {
    /// A Peek scratch mirror: written only inside `Peek`, never part of the
    /// state two opens must agree on. (Also never fully initialised — Peek
    /// copies into it — so reading it would compare malloc leftovers.)
    Mirror,
    /// `count` elements. With `phase`, the buffer is a ring and the compare is
    /// LOGICAL: slot `k` of one handle is `(phase + k) % count`, so two handles
    /// holding the same bars at different rotations compare equal.
    ///
    /// They legitimately do. The trailing-ring tiers capture in two different
    /// layouts: the absolute-mod one (`back > 0`) seeds `ringPos = historyLen %
    /// cap`, which the updates reproduce exactly, but the plain oldest-slot one
    /// re-bases every open to phase 0 (`memcpy` of the last `cap` bars,
    /// `ringPos = 0`) while an update just advances the cursor. Nothing can
    /// observe the difference — every read is relative to `ringPos` — so
    /// comparing raw slots would fail 90 of 175 functions on the rotation
    /// alone. Rotating by each handle's own phase still catches a cursor that
    /// advances wrongly: that misaligns the CONTENT, which is what is compared.
    Slots { count: String, is_int: bool, phase: Option<String> },
    /// The extrema automaton's absolute-index buffer. Only the `cap` bars
    /// `[trailing-1, trailing-1+cap)` are ever written, at `bar & mask`; the
    /// allocation is the next power of two, so the slack above the window holds
    /// whatever malloc returned and must not be read. (`trailing - 1`, not
    /// `trailing`: `xCap` is `today - trailing + 1` captured with `today`
    /// already advanced past the last bar, so the filled range is
    /// `[historyLen - xCap, historyLen)`. `+ phys` keeps the index
    /// non-negative when `trailing` is 0.)
    Extrema { cap: String, trailing: String, mask: String, phys: String },
    /// A typed sub-stream handle: recurse into the callee's comparator.
    Sub { callee: String },
    /// The dispatch tier's untyped handle, tagged by an enum param.
    DispatchSub { tag: String, arms: Vec<(String, String)> },
    /// MAVP's bank of `count` sub-handles.
    Bank { count: String, callee: String },
}

/// One parsed field declaration from a `struct TA_<N>_Stream` body.
struct SvDecl {
    name: String,
    /// `*` count on the declarator (0 for a scalar).
    ptr: usize,
    /// Array extent; 1 for a plain scalar.
    len: usize,
    is_int: bool,
}

/// Parse the emitted struct body into field declarations.
///
/// Every line is the opening/closing brace, a comment, or
/// `   <type> [*...]<name>[[N]];`. Anything else is a tier change this code has
/// not been taught: panic rather than silently drop a field from the compare.
fn sv_parse_state_struct(name: &str, text: &str) -> Vec<SvDecl> {
    let mut out = Vec::new();
    let mut in_comment = false;
    for raw in text.lines() {
        let line = raw.trim();
        // The opening line, not every line starting with `struct` — MAVP's bank
        // is declared `struct TA_MA_Stream **bank;` and skipping it here left
        // the bank uncompared (caught by the claimed-set assert below).
        if line.is_empty() || line.ends_with('{') || line == "};" {
            continue;
        }
        if in_comment {
            in_comment = !line.contains("*/");
            continue;
        }
        if line.starts_with("/*") {
            in_comment = !line.contains("*/");
            continue;
        }
        // A trailing comment on the declaration line (`int unused; /* ... */`).
        let line = match line.find("/*") {
            Some(at) => line[..at].trim_end(),
            None => line,
        };
        let decl = line
            .strip_suffix(';')
            .unwrap_or_else(|| panic!("{name}: unparsable stream state line `{line}`"));
        let mut tokens: Vec<&str> = decl.split_whitespace().collect();
        let mut declarator = tokens.pop().unwrap_or_default().to_string();
        let ptr = declarator.chars().take_while(|c| *c == '*').count();
        declarator = declarator.trim_start_matches('*').to_string();
        let mut len = 1usize;
        if let Some(open) = declarator.find('[') {
            let close = declarator
                .find(']')
                .unwrap_or_else(|| panic!("{name}: unparsable declarator `{declarator}`"));
            len = declarator[open + 1..close]
                .parse()
                .unwrap_or_else(|_| panic!("{name}: non-literal extent in `{declarator}`"));
            declarator.truncate(open);
        }
        let ty = tokens.join(" ");
        assert!(
            !declarator.is_empty() && !ty.is_empty(),
            "{name}: unparsable stream state line `{line}`"
        );
        // `double` and `int` are the only scalar storages the tiers emit; an
        // enum param (TA_MAType) compares like an int.
        out.push(SvDecl { name: declarator, ptr, len, is_int: ty != "double" });
    }
    out
}

/// Pointer roles contributed by one loop model's buffers.
fn sv_model_ptrs(
    model: &crate::streaming::StreamModel,
    roles: &mut BTreeMap<String, SvPtr>,
    phases: &mut BTreeSet<String>,
) {
    for r in model.rings() {
        let cap = format!("ringCap_{}", r.var);
        let pos = format!("ringPos_{}", r.var);
        phases.insert(pos.clone());
        for arr in &r.arrays {
            roles.insert(
                format!("ring_{}_{arr}", r.var),
                SvPtr::Slots { count: cap.clone(), is_int: false, phase: Some(pos.clone()) },
            );
            roles.insert(format!("ringMirror_{}_{arr}", r.var), SvPtr::Mirror);
        }
    }
    for w in model.windows() {
        let cap = format!("winCap_{}", w.var);
        let pos = format!("winPos_{}", w.var);
        phases.insert(pos.clone());
        for arr in &w.arrays {
            roles.insert(
                format!("win_{}_{arr}", w.var),
                SvPtr::Slots { count: cap.clone(), is_int: false, phase: Some(pos.clone()) },
            );
            roles.insert(format!("winMirror_{}_{arr}", w.var), SvPtr::Mirror);
        }
    }
    for c in model.circs() {
        // No phase: a CIRCBUF is captured LIVE from the batch — contents AND
        // rotation — so both opens agree on the raw slots, and the rotation
        // index is an ordinary state scalar that is compared as one.
        let size = format!("cbSize_{}", c.id);
        for (storage, ty) in crate::streaming::circ_storages(c) {
            let is_int = matches!(ty, crate::ir::VarType::Integer);
            roles.insert(
                format!("cb_{storage}"),
                SvPtr::Slots { count: size.clone(), is_int, phase: None },
            );
            roles.insert(format!("cbMirror_{storage}"), SvPtr::Mirror);
        }
    }
    if let Some(ex) = model.extrema() {
        for arr in &ex.arrays {
            roles.insert(
                format!("x_{arr}"),
                SvPtr::Extrema {
                    cap: "xCap".to_string(),
                    trailing: ex.trailing.clone(),
                    mask: "xMask".to_string(),
                    phys: "xPhys".to_string(),
                },
            );
            roles.insert(format!("xMirror_{arr}"), SvPtr::Mirror);
        }
    }
}

/// Pointer roles for one streaming function, keyed by field name.
fn sv_ptr_roles(
    func: &FuncDef,
    funcs: &[FuncDef],
) -> (BTreeMap<String, SvPtr>, BTreeSet<String>) {
    use crate::streaming::{FuncsLookup, StreamPlan};
    let resolved = func.resolved_for(crate::ir::Lang::C);
    let plan = crate::streaming::validate_streamable(&resolved, &FuncsLookup(funcs))
        .unwrap_or_else(|e| panic!("streaming gate: {e}"));
    let mut roles: BTreeMap<String, SvPtr> = BTreeMap::new();
    let mut phases: BTreeSet<String> = BTreeSet::new();
    match &plan {
        StreamPlan::Loop(model) => sv_model_ptrs(model, &mut roles, &mut phases),
        StreamPlan::DualMode(dmp) => {
            sv_model_ptrs(&dmp.mode_a, &mut roles, &mut phases);
            sv_model_ptrs(&dmp.mode_b, &mut roles, &mut phases);
        }
        StreamPlan::Composed(cp) => {
            if let Some(model) = &cp.producer {
                sv_model_ptrs(model, &mut roles, &mut phases);
            }
            for (i, sub) in cp.subs.iter().enumerate() {
                roles.insert(format!("sub{i}"), SvPtr::Sub { callee: sub.callee.to_uppercase() });
            }
            for ring in &cp.sub_lag_rings {
                let s = &ring.series;
                phases.insert(format!("lagRingPos_{s}"));
                roles.insert(
                    format!("lagRing_{s}"),
                    SvPtr::Slots {
                        count: format!("lagRingCap_{s}"),
                        is_int: false,
                        phase: Some(format!("lagRingPos_{s}")),
                    },
                );
                roles.insert(format!("lagRingMirror_{s}"), SvPtr::Mirror);
            }
        }
        StreamPlan::Dispatch(dp) => {
            roles.insert(
                "sub".to_string(),
                SvPtr::DispatchSub {
                    tag: dp.param.clone(),
                    arms: dp
                        .arms
                        .iter()
                        .filter(|a| a.supported && !a.callee.is_empty())
                        .map(|a| (a.label.clone(), a.callee.to_uppercase()))
                        .collect(),
                },
            );
        }
        StreamPlan::PeriodBank(pbp) => {
            roles.insert(
                "bank".to_string(),
                SvPtr::Bank { count: "nBank".to_string(), callee: pbp.callee.to_uppercase() },
            );
            roles.insert(
                "scratch".to_string(),
                SvPtr::Slots { count: "nBank".to_string(), is_int: false, phase: None },
            );
        }
    }
    (roles, phases)
}

/// One function's comparator body plus the callees it recurses into.
struct SvComparator {
    body: String,
    deps: Vec<String>,
}

/// Bitwise compare of one scalar pair, as a C condition.
fn sv_ne(lhs: &str, rhs: &str, is_int: bool) -> String {
    if is_int {
        format!("{lhs} != {rhs}")
    } else {
        // Same rule the batch-vs-stream value legs use: differing bits that are
        // numerically equal can only be +0.0 vs -0.0, which max/min leave
        // unspecified. Counted as benign, never a mismatch (#147).
        format!("sv_xtier_ne({lhs}, {rhs}, z)")
    }
}

/// Build the comparator for one streaming function.
#[allow(clippy::too_many_lines)]
fn sv_comparator(func: &FuncDef, funcs: &[FuncDef], enums: &HashMap<String, EnumDef>) -> SvComparator {
    use crate::backends::c::render_c_switch_label;
    use crate::backends::c_stream;
    use crate::streaming::FuncsLookup;

    let name = &func.name;
    let text = c_stream::state_struct_text(func, &FuncsLookup(funcs));
    let decls = sv_parse_state_struct(name, &text);
    let (roles, phases) = sv_ptr_roles(func, funcs);
    let mut claimed: BTreeSet<&str> = BTreeSet::new();
    let mut body = String::new();
    let mut deps: Vec<String> = Vec::new();

    for d in &decls {
        let n = &d.name;
        if d.ptr == 0 {
            // A ring cursor is the buffer's phase, not state in its own right:
            // the two opens seed it differently by design and the rotated
            // buffer compare below is what actually pins the ring.
            if phases.contains(n.as_str()) {
                continue;
            }
            if d.len > 1 {
                let _ = writeln!(
                    body,
                    "   for( k = 0; k < {}; k++ ) if( {} ) {{ *w = \"{n}\"; return 1; }}",
                    d.len,
                    sv_ne(&format!("a->{n}[k]"), &format!("b->{n}[k]"), d.is_int)
                );
            } else {
                let _ = writeln!(
                    body,
                    "   if( {} ) {{ *w = \"{n}\"; return 1; }}",
                    sv_ne(&format!("a->{n}"), &format!("b->{n}"), d.is_int)
                );
            }
            continue;
        }
        let role = roles.get(n.as_str()).unwrap_or_else(|| {
            panic!(
                "TA_{name}: stream state pointer `{n}` has no state-equivalence rule. \
                 A new heap field must say how many elements it carries, or the #240 \
                 state leg would silently stop comparing it."
            )
        });
        claimed.insert(n.as_str());
        let guard = format!(
            "   if( (a->{n} == NULL) != (b->{n} == NULL) ) {{ *w = \"{n}\"; return 1; }}\n"
        );
        match role {
            SvPtr::Mirror => {}
            SvPtr::Slots { count, is_int, phase } => {
                body.push_str(&guard);
                match phase {
                    None => {
                        let _ = writeln!(
                            body,
                            "   if( a->{n} ) for( k = 0; k < a->{count}; k++ ) if( {} ) {{ *w = \"{n}\"; return 1; }}",
                            sv_ne(&format!("a->{n}[k]"), &format!("b->{n}[k]"), *is_int)
                        );
                    }
                    Some(pos) => {
                        let _ = writeln!(body, "   if( a->{n} ) for( k = 0; k < a->{count}; k++ )");
                        let _ = writeln!(body, "   {{");
                        let _ = writeln!(body, "      ia = (a->{pos} + k) % a->{count};");
                        let _ = writeln!(body, "      ib = (b->{pos} + k) % b->{count};");
                        let _ = writeln!(
                            body,
                            "      if( {} ) {{ *w = \"{n}\"; return 1; }}",
                            sv_ne(&format!("a->{n}[ia]"), &format!("b->{n}[ib]"), *is_int)
                        );
                        let _ = writeln!(body, "   }}");
                    }
                }
            }
            SvPtr::Extrema { cap, trailing, mask, phys } => {
                body.push_str(&guard);
                let _ = writeln!(body, "   if( a->{n} ) for( k = 0; k < a->{cap}; k++ )");
                let _ = writeln!(body, "   {{");
                let _ = writeln!(body, "      ix = (a->{trailing} - 1 + a->{phys} + k) & a->{mask};");
                let _ = writeln!(
                    body,
                    "      if( {} ) {{ *w = \"{n}\"; return 1; }}",
                    sv_ne(&format!("a->{n}[ix]"), &format!("b->{n}[ix]"), false)
                );
                let _ = writeln!(body, "   }}");
            }
            SvPtr::Sub { callee } => {
                deps.push(callee.clone());
                body.push_str(&guard);
                let _ = writeln!(
                    body,
                    "   if( a->{n} && sv_steq_TA_{callee}( a->{n}, b->{n}, w, z ) ) return 1;"
                );
            }
            SvPtr::DispatchSub { tag, arms } => {
                body.push_str(&guard);
                let _ = writeln!(body, "   if( a->{n} )");
                let _ = writeln!(body, "      switch( a->{tag} )");
                let _ = writeln!(body, "      {{");
                for (label, callee) in arms {
                    deps.push(callee.clone());
                    let case = render_c_switch_label(label, enums);
                    let cty = format!("const struct TA_{callee}_Stream *");
                    let _ = writeln!(body, "      case {case}:");
                    let _ = writeln!(
                        body,
                        "         if( sv_steq_TA_{callee}( ({cty})a->{n}, ({cty})b->{n}, w, z ) ) return 1;"
                    );
                    let _ = writeln!(body, "         break;");
                }
                let _ = writeln!(body, "      default:");
                let _ = writeln!(body, "         *w = \"{n}\"; return 1;");
                let _ = writeln!(body, "      }}");
            }
            SvPtr::Bank { count, callee } => {
                deps.push(callee.clone());
                body.push_str(&guard);
                let _ = writeln!(body, "   if( a->{n} ) for( k = 0; k < a->{count}; k++ )");
                let _ = writeln!(body, "   {{");
                let _ = writeln!(
                    body,
                    "      if( (a->{n}[k] == NULL) != (b->{n}[k] == NULL) ) {{ *w = \"{n}\"; return 1; }}"
                );
                let _ = writeln!(
                    body,
                    "      if( a->{n}[k] && sv_steq_TA_{callee}( a->{n}[k], b->{n}[k], w, z ) ) return 1;"
                );
                let _ = writeln!(body, "   }}");
            }
        }
    }

    // Every pointer role the model produced must have matched a declared field.
    // The reverse direction (a declared pointer with no role) panics above; this
    // catches a spec whose field name the struct emitter spells differently,
    // which would leave the buffer uncompared without either side noticing.
    for key in roles.keys() {
        assert!(
            claimed.contains(key.as_str()),
            "TA_{name}: state-equivalence rule for `{key}` matches no field in \
             `struct TA_{name}_Stream` — the rule and the struct emitter have drifted."
        );
    }

    SvComparator { body, deps }
}

/// Every state-equivalence comparator, plus the names of the functions that
/// have one.
///
/// A comparator that recurses into a sub-handle needs its callee's comparator,
/// so the set closes under a fixpoint: drop any function whose dependency was
/// dropped, until nothing moves. The C server's leg is emitted only for what
/// survives, and `ta_regtest` ratchets the surviving count against the number
/// of streaming functions, so a shrinking set fails rather than going quiet.
fn generate_c_state_eq(
    funcs: &[FuncDef],
    enums: &HashMap<String, EnumDef>,
) -> (String, BTreeSet<String>) {
    let mut comps: BTreeMap<String, SvComparator> = BTreeMap::new();
    for f in funcs.iter().filter(|f| f.streaming) {
        comps.insert(f.name.clone(), sv_comparator(f, funcs, enums));
    }
    let mut have: BTreeSet<String> = comps.keys().cloned().collect();
    loop {
        let dropped: Vec<String> = have
            .iter()
            .filter(|n| comps[*n].deps.iter().any(|d| !have.contains(d)))
            .cloned()
            .collect();
        if dropped.is_empty() {
            break;
        }
        for d in dropped {
            have.remove(&d);
        }
    }

    let mut s = String::new();
    s.push_str("/* ---- state-equivalence comparators (issue #240) ----\n");
    s.push_str(" * `Open(P)` + (n-P) updates must leave the handle bit-identical to\n");
    s.push_str(" * `Open(n)`. Compares every carried field; skips the Peek scratch mirrors\n");
    s.push_str(" * (written only inside Peek) and, for the extrema automaton, the slack\n");
    s.push_str(" * above the live window (never written, so it holds malloc leftovers).\n");
    s.push_str(" * Returns 1 and names the field on the first difference. */\n");
    for n in &have {
        let _ = writeln!(
            s,
            "static int sv_steq_TA_{n}( const struct TA_{n}_Stream *a, const struct TA_{n}_Stream *b, const char **w, int *z );"
        );
    }
    s.push('\n');
    for n in &have {
        let _ = writeln!(
            s,
            "static int sv_steq_TA_{n}( const struct TA_{n}_Stream *a, const struct TA_{n}_Stream *b, const char **w, int *z )\n{{"
        );
        s.push_str("   int k = 0, ix = 0, ia = 0, ib = 0;\n");
        s.push_str("   (void)a; (void)b; (void)w; (void)z; (void)k; (void)ix; (void)ia; (void)ib;\n");
        s.push_str(&comps[n].body);
        s.push_str("   return 0;\n}\n\n");
    }
    (s, have)
}

// --- the state-equivalence leg's five emission points (issue #240) ----------
//
// Each takes the `steq` flag and returns early rather than being called under
// an `if`: `generate_c_stream_verify` is already at clippy's cognitive-
// complexity ceiling, and five more branches in it push it over.

/// The leg's per-function locals: one reference handle over the whole history,
/// plus the verdict it produces. Reopened per candle round, because the
/// settings a round installs are part of what the state encodes.
/// The range leg (issue #241): a handle's `OutRange` must equal what the batch
/// call reports over the same bars, whichever opener produced it and however
/// many updates followed. Unlike the state leg this needs no private struct and
/// no second reference handle — the range is public API in all four backends and
/// the batch pair is already in scope — so it runs in every language server.
fn emit_sv_range_decls(s: &mut String) {
    s.push_str("        int rangeChecked = 0, rangeOk = 1, rangeLegs = 0, rangeSites = 0;\n");
    s.push_str("        int rB = 0, rN = 0;\n");
}

/// The range-compare SITES a server emits, in bit order. Each site sets its own
/// bit in `rangeSites`; the server declares the count as `range_sites_n`; the
/// driver ORs the mask across the run and requires every bit.
///
/// The leg's other floor is a total — it counts functions — so a whole site
/// class going dead in one language leaves it far above its floor and green.
/// This is the ratchet that sees it. Corpus-wide rather than per function,
/// because a site can legitimately not run for a given function or vector: the
/// anchored compare needs `lb < Sidx < svN - 1`, which a large lookback denies.
///
/// One definition per language, and the bit and the count are read from the SAME
/// place, because the drift that fails OPEN is a site added without bumping the
/// count: the mask then carries a bit the ratchet never demands. `sv_range_bit`
/// is the only way to spell a bit, and it asserts against the count.
#[derive(Clone, Copy, PartialEq, Eq)]
enum SvRangeSite {
    /// The `OpenAndFill` handle.
    Fill = 0,
    /// The `Open(P)` + updates handle.
    Prefix = 1,
    /// The `Open(P)` + ONE `UpdateAndFill` handle (issue #246). Numbered below
    /// `Anchored` on purpose: the driver's ratchet demands the mask be
    /// `(1 << n) - 1`, so the bits have to stay contiguous from 0 and Rust —
    /// which reaches every site but the anchored one — has to be the server
    /// whose declared count truncates the list.
    UpdateFill = 2,
    /// The `startIdx`-anchored `_OpenInternal` handle. Every server but Rust,
    /// whose server is a separate crate and cannot reach a `pub(crate)` seam.
    Anchored = 3,
}

/// The bit `site` sets, checked against the count the server will declare.
fn sv_range_bit(site: SvRangeSite, declared: u32) -> u32 {
    let bit = site as u32;
    assert!(
        bit < declared,
        "range site {bit} is outside the {declared} this server declares — the mask \
         would carry a bit the driver's ratchet never demands"
    );
    1u32 << bit
}

/// C, Java and C# reach the anchored seam; Rust cannot — its server is a
/// separate crate and `_OpenInternal` is `pub(crate)`.
const SV_RANGE_SITES_C: u32 = 4;
const SV_RANGE_SITES_JAVA: u32 = 4;
const SV_RANGE_SITES_CSHARP: u32 = 4;
const SV_RANGE_SITES_RUST: u32 = 3;

/// One comparison: `handle`'s range against the `(beg, nb)` the batch reported
/// for the same bars. `guard` is the leg's own success condition — a leg that
/// already failed has a handle short of the bars it was supposed to consume.
fn emit_sv_range_check(
    s: &mut String, indent: &str, handle: &str, guard: &str, beg: &str, nb: &str,
    site: SvRangeSite,
) {
    let _ = writeln!(s, "{indent}if( {guard} )");
    let _ = writeln!(s, "{indent}{{");
    let _ = writeln!(
        s,
        "{indent}    rangeChecked = 1; rangeLegs++; rangeSites |= {};",
        sv_range_bit(site, SV_RANGE_SITES_C)
    );
    let _ = writeln!(s, "{indent}    rB = -1; rN = -1;");
    let _ = writeln!(
        s,
        "{indent}    if( TA_StreamOutRange( {handle}, &rB, &rN ) != TA_SUCCESS || rB != {beg} || rN != {nb} ) rangeOk = 0;"
    );
    let _ = writeln!(s, "{indent}}}");
}

/// The `UpdateAndFill` leg (issue #246): `Open(P)`, then ONE `UpdateAndFill`
/// over the remaining bars instead of `svN - P` separate `Update` calls.
///
/// It is the n-bar entry point's only cross-tier gate — nothing else in the
/// tree calls it — and it compares the same two things the per-bar sweep above
/// does: every value against `batch(0, svN-1)` bitwise, and the handle's
/// `OutRange` against the batch range. Three cheap probes ride along on the
/// same handle because each is a rejection that leaves the handle untouched,
/// so none of them costs an extra open: the aliasing guard, the zero-count
/// no-op, and (C only, the one backend with the parameter) a negative count.
#[allow(clippy::too_many_arguments)]
fn emit_sv_update_fill_leg(
    s: &mut String,
    name: &str,
    input_arrays: &[&str],
    in_args: &str,
    opt_args: &str,
    out_is_int: &[bool],
    bbuf: &[String],
    fbuf: &[String],
) {
    let n_outs = out_is_int.len();
    let vout: String = (0..n_outs).map(|i| format!("&uv{i}")).collect::<Vec<_>>().join(", ");
    let shifted: String = input_arrays.iter().fold(String::new(), |mut acc, a| {
        let _ = write!(acc, "{a} + P, ");
        acc
    });
    let fill_args = fbuf.join(", ");
    s.push_str("        if( npref > 0 )\n        {\n");
    s.push_str("            int P = pref[0]; int ut, uB0 = -1, uN0 = -1, uB = -1, uN = -1;\n");
    for (i, is_int) in out_is_int.iter().enumerate() {
        let (ty, z) = if *is_int { ("int", "0") } else { ("double", "0.0") };
        let _ = writeln!(s, "            {ty} uv{i} = {z};");
    }
    let _ = writeln!(s, "            TA_{name}_Stream *stu = NULL;");
    s.push_str("            TA_RetCode urc;\n");
    let _ = writeln!(
        s,
        "            urc = TA_{name}_Open(&stu, {in_args}P, {opt_args}{vout});"
    );
    s.push_str("            ufillChecked = 1;\n");
    s.push_str("            if( urc != TA_SUCCESS || !stu ) ufillOk = 0;\n");
    s.push_str("            if( ufillOk )\n            {\n");
    // The range as the open left it. Both no-op probes below are checked
    // against THIS, not against a recomputed (lb, P - lb): the point is that
    // they change nothing, and re-deriving what they should not have changed
    // would let an opener bug and a filler bug cancel.
    s.push_str("                if( TA_StreamOutRange( stu, &uB0, &uN0 ) != TA_SUCCESS ) ufillOk = 0;\n");
    // Aliasing guard: output 0 handed the SAME pointer the first input is
    // handed, so the equality test the wrapper makes actually fires. Rejected,
    // handle untouched.
    if !out_is_int.first().copied().unwrap_or(true) && !input_arrays.is_empty() {
        let alias_args: Vec<String> = fbuf
            .iter()
            .enumerate()
            .map(|(i, b)| if i == 0 { format!("{} + P", input_arrays[0]) } else { b.clone() })
            .collect();
        let _ = writeln!(
            s,
            "                if( TA_{name}_UpdateAndFill( stu, {shifted}svN - P, {} ) != TA_BAD_PARAM ) ufillOk = 0;",
            alias_args.join(", ")
        );
    }
    // Zero bars is a success no-op; a negative count is a rejection. Neither
    // may move the handle.
    let _ = writeln!(
        s,
        "                if( TA_{name}_UpdateAndFill( stu, {shifted}0, {fill_args} ) != TA_SUCCESS ) ufillOk = 0;"
    );
    let _ = writeln!(
        s,
        "                if( TA_{name}_UpdateAndFill( stu, {shifted}-1, {fill_args} ) != TA_BAD_PARAM ) ufillOk = 0;"
    );
    s.push_str("                if( TA_StreamOutRange( stu, &uB, &uN ) != TA_SUCCESS || uB != uB0 || uN != uN0 ) ufillOk = 0;\n");
    s.push_str("            }\n");
    s.push_str("            if( ufillOk )\n            {\n");
    s.push_str("                for( ut = 0; ut < SV_MAXN; ut++ ) {\n");
    for (i, is_int) in out_is_int.iter().enumerate() {
        let canary = if *is_int { "SV_FILL_CANARY_I" } else { "SV_FILL_CANARY" };
        let _ = writeln!(s, "                    {}[ut] = {canary};", fbuf[i]);
    }
    s.push_str("                }\n");
    let _ = writeln!(
        s,
        "                urc = TA_{name}_UpdateAndFill( stu, {shifted}svN - P, {fill_args} );"
    );
    s.push_str("                if( urc != TA_SUCCESS ) ufillOk = 0;\n");
    s.push_str("                else for( ut = P; ufillOk && ut < svN; ut++ ) {\n");
    for (i, is_int) in out_is_int.iter().enumerate() {
        if *is_int {
            let _ = writeln!(
                s,
                "                    if( {}[ut - P] != {}[ut - svBeg] ) ufillOk = 0;",
                fbuf[i], bbuf[i]
            );
        } else {
            let _ = writeln!(
                s,
                "                    if( sv_xtier_ne({}[ut - P], {}[ut - svBeg], &svZsign) ) ufillOk = 0;",
                fbuf[i], bbuf[i]
            );
        }
    }
    s.push_str("                }\n");
    // Nothing above bar svN-1 may be written: the call was handed exactly
    // svN - P bars.
    s.push_str("                if( urc == TA_SUCCESS )\n");
    s.push_str("                    for( ut = svN - P; ufillOk && ut < SV_MAXN; ut++ ) {\n");
    for (i, is_int) in out_is_int.iter().enumerate() {
        let canary = if *is_int { "SV_FILL_CANARY_I" } else { "SV_FILL_CANARY" };
        let _ = writeln!(s, "                        if( {}[ut] != {canary} ) ufillOk = 0;", fbuf[i]);
    }
    s.push_str("                    }\n");
    s.push_str("            }\n");
    emit_sv_range_check(
        s, "            ", "stu", "ufillOk && stu", "svBeg", "svNb", SvRangeSite::UpdateFill,
    );
    let _ = writeln!(s, "            if( stu ) TA_{name}_Close(stu);");
    s.push_str("        }\n");
}

/// Folded into `ok` like the fill and state legs, so a driver check that ever
/// regresses still fails the run.
fn emit_sv_range_report(s: &mut String) {
    s.push_str("        if( rangeChecked && !rangeOk ) allOk = 0;\n");
    let _ = writeln!(
        s,
        "        pos = json_appendf(resp, resp_size, pos, \",\\\"range_checked\\\":%d,\\\"range_legs\\\":%d,\\\"range_sites\\\":%d,\\\"range_sites_n\\\":{SV_RANGE_SITES_C},\\\"range_ok\\\":%d\", rangeChecked, rangeLegs, rangeSites, rangeOk);"
    );
}

fn emit_sv_state_decls(s: &mut String, name: &str, steq: bool) {
    if !steq {
        return;
    }
    s.push_str("        int stateChecked = 0, stateOk = 1, stateLegs = 0;\n");
    s.push_str("        const char *stateWhat = \"-\";\n");
    let _ = writeln!(s, "        TA_{name}_Stream *stEq = NULL;");
}

/// `Open(svN)` -- the handle every prefix leg is compared against.
fn emit_sv_state_open(
    s: &mut String,
    name: &str,
    steq: bool,
    out_is_int: &[bool],
    in_args: &str,
    opt_args: &str,
) {
    if !steq {
        return;
    }
    let eouts: String = out_is_int
        .iter()
        .enumerate()
        .map(|(i, is_int)| if *is_int { format!("int e{i} = 0;") } else { format!("double e{i} = 0.0;") })
        .collect::<Vec<_>>()
        .join(" ");
    let eaddrs: String = (0..out_is_int.len())
        .map(|i| format!("&e{i}"))
        .collect::<Vec<_>>()
        .join(", ");
    s.push_str("        {\n");
    let _ = writeln!(s, "            {eouts}");
    let _ = writeln!(
        s,
        "            if( TA_{name}_Open( &stEq, {in_args}svN, {opt_args}{eaddrs} ) != TA_SUCCESS ) stEq = NULL;"
    );
    s.push_str("        }\n");
}

/// The compare itself: the two handles have now consumed the same `svN` bars by
/// different routes. Only when the value leg passed -- its loop breaks early on
/// a mismatch, so the handle would be short of the reference.
fn emit_sv_state_compare(s: &mut String, name: &str, steq: bool) {
    if !steq {
        return;
    }
    s.push_str("            if( ok && st && stEq )\n");
    s.push_str("            {\n");
    s.push_str("                stateChecked = 1; stateLegs++;\n");
    let _ = writeln!(
        s,
        "                if( sv_steq_TA_{name}( st, stEq, &stateWhat, &svZsign ) ) stateOk = 0;"
    );
    s.push_str("            }\n");
}

fn emit_sv_state_close(s: &mut String, name: &str, steq: bool) {
    if !steq {
        return;
    }
    let _ = writeln!(s, "        if( stEq ) {{ TA_{name}_Close(stEq); stEq = NULL; }}");
}

/// Folded into `ok` as a safety net, exactly like the fill leg: the driver
/// reads `state_ok` for the specific message, and a run whose driver check ever
/// regresses still fails on the generic flag.
fn emit_sv_state_report(s: &mut String, steq: bool) {
    if !steq {
        return;
    }
    s.push_str("        if( stateChecked && !stateOk ) allOk = 0;\n");
    s.push_str("        pos = json_appendf(resp, resp_size, pos, \",\\\"state_checked\\\":%d,\\\"state_legs\\\":%d,\\\"state_ok\\\":%d,\\\"state_bad\\\":\\\"%s\\\"\", stateChecked, stateLegs, stateOk, stateWhat);\n");
}

// `cognitive_complexity`: the same allow the other whole-server emitters carry.
// This one crossed the threshold when #262's declined-output arm landed, and the
// shape it is complaining about is the sequence of `emit_sv_*` calls this
// function exists to order.
#[allow(clippy::too_many_lines, clippy::cognitive_complexity)]
fn generate_c_stream_verify(funcs: &[FuncDef], enums: &HashMap<String, EnumDef>) -> String {
    let mut s = String::new();
    s.push_str("/* ---- stream_verify: bitwise batch-vs-stream comparison ---- */\n");
    s.push_str("#ifndef TA_REF_SERVE\n");
    s.push_str("#define SV_MAXN 256\n");
    // Canary for the OpenAndFill slack. The fill buffers are SV_MAXN wide but
    // the call may only write `nb` elements; everything above that is stamped
    // before the call and asserted untouched after it, so a write past the
    // produced range fails instead of landing in unread space. Values no
    // indicator can produce from the generated series.
    s.push_str("#define SV_FILL_CANARY (-1.2345678901234e300)\n");
    s.push_str("#define SV_FILL_CANARY_I (-987654321)\n");
    s.push_str("#define SV_PEEK_EVERY 7\n");
    s.push_str("static double sv_o[SV_MAXN], sv_h[SV_MAXN], sv_l[SV_MAXN];\n");
    s.push_str("static double sv_c[SV_MAXN], sv_v[SV_MAXN], sv_oi[SV_MAXN];\n");
    // Batch output buffers, then the OpenAndFill scratch it is compared against
    // bitwise. One per slot the widest function uses, counted from the corpus
    // for the reason `max_output_arity` gives.
    let (sv_n_real, sv_n_int) = crate::backends::common::max_output_arity(funcs);
    for (prefix, ty, n) in [
        ("sv_b", "double", sv_n_real),
        ("sv_f", "double", sv_n_real),
        ("sv_ib", "int", sv_n_int),
        ("sv_if", "int", sv_n_int),
    ] {
        if n == 0 {
            continue;
        }
        let decl: Vec<String> = (0..n).map(|k| format!("{prefix}{k}[SV_MAXN]")).collect();
        let _ = writeln!(s, "static {ty} {};", decl.join(", "));
    }
    s.push_str("static int sv_bitne(double a, double b) { return memcmp(&a, &b, sizeof(double)) != 0; }\n");
    // Cross-tier compare (stream vs batch, and OpenAndFill's array vs batch).
    // Differing bits that are numerically equal can only be +0.0 vs -0.0, which
    // max/min leave unspecified: counted, never a mismatch — the same benign
    // class --fuzz-064 carries (issue #147). Same-tier compares (peek vs
    // update) keep sv_bitne: one code path has no licence to differ at all.
    s.push_str("static int sv_xtier_ne(double a, double b, int *zsign) {\n");
    s.push_str("    if( !sv_bitne(a, b) ) return 0;\n");
    s.push_str("    if( a == b ) { (*zsign)++; return 0; }\n");
    s.push_str("    return 1;\n");
    s.push_str("}\n");
    // Candle-settings variation for CDL streams: rounds 1/2 re-run the
    // batch-vs-stream comparison with every setting's avgPeriod bumped (+3)
    // or zeroed (the instant-candle degenerate, runtime trailing lag 0).
    // mode 0: avgPeriod += 3; mode 1: avgPeriod = 0 (instant candle, runtime
    // trailing lag 0); mode 2: rangeType = Shadows everywhere (gates the
    // TA_STREAM Shadows arithmetic, which no default setting exercises).
    s.push_str("static void sv_candle_avg(int mode) {\n");
    s.push_str("    int i;\n");
    s.push_str("    for( i = 0; i < (int)TA_AllCandleSettings; i++ )\n");
    s.push_str("        TA_SetCandleSettings( (TA_CandleSettingType)i,\n");
    s.push_str("                              mode == 2 ? TA_RangeType_Shadows : TA_Globals->candleSettings[i].rangeType,\n");
    s.push_str("                              mode == 1 ? 0 : (mode == 0 ? TA_Globals->candleSettings[i].avgPeriod + 3 : TA_Globals->candleSettings[i].avgPeriod),\n");
    s.push_str("                              TA_Globals->candleSettings[i].factor );\n");
    s.push_str("}\n\n");
    // State-equivalence comparators, emitted before the handler that calls them.
    let (steq_code, steq_have) = generate_c_state_eq(funcs, enums);
    s.push_str(&steq_code);
    s.push_str("static void handle_stream_verify(const char *json, char *resp, int resp_size) {\n");
    s.push_str("    int fnLen = 0;\n");
    s.push_str("    const char *fn = json_find_string(json, \"funcName\", &fnLen);\n");
    s.push_str("    int svShape  = json_find_int(json, \"gen_shape\");\n");
    s.push_str("    int svSeed   = json_find_int(json, \"gen_seed\");\n");
    s.push_str("    int svN      = json_find_int(json, \"gen_n\");\n");
    s.push_str("    int svK      = json_find_int(json, \"unstablePeriod\");\n");
    s.push_str("    int svCompat = json_find_int(json, \"compatibility\");\n");
    s.push_str("    int svCandle = json_find_int(json, \"candleLegs\");\n");
    s.push_str("    (void)svCandle;\n");
    s.push_str("    int savedCompat = (int)TA_GetCompatibility();\n");
    s.push_str("    (void)svK;\n");
    s.push_str("    if( !fn ) { snprintf(resp, resp_size, \"{\\\"error\\\":\\\"missing funcName\\\"}\"); return; }\n");
    s.push_str("    if( svN < 2 ) svN = 2;\n");
    s.push_str("    if( svN > SV_MAXN ) svN = SV_MAXN;\n");
    s.push_str("    fuzz_gen(svShape, svSeed, svN, sv_o, sv_h, sv_l, sv_c, sv_v, sv_oi);\n");
    s.push_str("    TA_SetCompatibility((TA_Compatibility)svCompat);\n\n");

    let mut first = true;
    for func in funcs.iter().filter(|f| f.streaming) {
        let name = &func.name;
        let method = format!("TA_{name}");
        let cond = if first { "if" } else { "else if" };
        first = false;

        // Input arrays in fuzz convention, in signature order.
        let input_names = expand_input_names(&func.inputs);
        let mut generic_idx = 0usize;
        let input_arrays: Vec<&str> = input_names
            .iter()
            .map(|n| sv_input_array(n, &mut generic_idx))
            .collect();
        let n_outs = func.outputs.len();
        // Unstable ids to pin: the function's own, plus any unstable
        // dependency reachable TRANSITIVELY through its lookback body
        // (DEMA/TEMA/TRIX/MACD call ema_lookback directly; STOCH/STOCHF
        // reach EMA/KAMA/T3 only through ma_lookback — a non-transitive
        // scan left their K-legs running vacuously at ambient K=0).
        let pin_ids: Vec<i32> = collect_pin_ids(func, funcs, enums);


        s.push_str(&format!(
            "    {cond}( fnLen == {} && strncmp(fn, \"{method}\", {}) == 0 ) {{\n",
            method.len(),
            method.len()
        ));

        // Optional params from the request.
        for opt in &func.optional_inputs {
            if opt.param_type == ParamType::Real {
                s.push_str(&format!(
                    "        double {0} = json_find_double(json, \"{0}\");\n",
                    opt.name
                ));
            } else if matches!(&opt.param_type, ParamType::Enum(_)) {
                s.push_str(&format!(
                    "        TA_MAType {0} = (TA_MAType)json_find_int(json, \"{0}\");\n",
                    opt.name
                ));
            } else {
                s.push_str(&format!(
                    "        int {0} = json_find_int(json, \"{0}\");\n",
                    opt.name
                ));
            }
        }

        emit_sv_period_bank_input(&mut s, func, funcs, &input_arrays);
        emit_sv_dispatch_precheck(&mut s, func, funcs, &input_arrays, n_outs, name);

        let candle = func.flags.iter().any(|f| f == "candlestick");
        let steq = steq_have.contains(name);
        s.push_str("        TA_RetCode rc;\n");
        s.push_str("        int svBeg = 0, svNb = 0, lb, li, npref, pos, allOk = 1, peekAll = 1;\n");
        s.push_str("        int fillOk = 1, fillChecked = 0;\n");
        emit_sv_state_decls(&mut s, name, steq);
        emit_sv_range_decls(&mut s);
        // The n-bar filler's own leg (issue #246), reported separately from the
        // open-time fill so a regression names the entry point it is in.
        s.push_str("        int ufillChecked = 0, ufillOk = 1;\n");
        // Benign +/-0 cases across every cross-tier compare in this request.
        s.push_str("        int svZsign = 0;\n");
        s.push_str("        int pref[4]; int pc[4];\n");
        if candle {
            // Candle functions honor "candleLegs": re-run the whole sweep
            // under bumped and zeroed avgPeriods (settings-stability rule:
            // settings are fixed per round; each round reopens its streams).
            s.push_str("        int rounds = svCandle ? 4 : 1; int rd, lgi = 0;\n");
        }
        for id in &pin_ids {
            s.push_str(&format!(
                "        TA_SetUnstablePeriod({id}, (unsigned int)svK);\n"
            ));
        }

        // Batch leg (startIdx=0, full range) + intrinsic-in-ambient-K lookback.
        let mut opt_args = String::new();
        for o in &func.optional_inputs {
            let _ = std::fmt::Write::write_fmt(&mut opt_args, format_args!("{}, ", o.name));
        }
        let mut in_args = String::new();
        for a in &input_arrays {
            let _ = std::fmt::Write::write_fmt(&mut in_args, format_args!("{a}, "));
        }
        let out_is_int: Vec<bool> = func
            .outputs
            .iter()
            .map(|ou| ou.param_type == ParamType::Integer)
            .collect();
        let mut out_args = String::new();
        {
            let (mut ri, mut ii) = (0usize, 0usize);
            for is_int in &out_is_int {
                if *is_int {
                    let _ = std::fmt::Write::write_fmt(&mut out_args, format_args!(", sv_ib{ii}"));
                    ii += 1;
                } else {
                    let _ = std::fmt::Write::write_fmt(&mut out_args, format_args!(", sv_b{ri}"));
                    ri += 1;
                }
            }
        }
        // Per-output batch buffer expression (indexed by output position).
        let bbuf: Vec<String> = {
            let (mut ri, mut ii) = (0usize, 0usize);
            out_is_int
                .iter()
                .map(|is_int| {
                    if *is_int {
                        let e = format!("sv_ib{ii}");
                        ii += 1;
                        e
                    } else {
                        let e = format!("sv_b{ri}");
                        ri += 1;
                        e
                    }
                })
                .collect()
        };
        if candle {
            s.push_str("        pos = json_appendf(resp, resp_size, 0, \"{\\\"retCode\\\":0\");\n");
            s.push_str("        for( rd = 0; rd < rounds; rd++ ) {\n");
            s.push_str("        if( rd > 0 ) TA_RestoreCandleDefaultSettings( TA_AllCandleSettings );\n");
            s.push_str("        if( rd > 0 ) sv_candle_avg(rd - 1);\n");
        }
        s.push_str(&format!(
            "        rc = {method}(0, svN - 1, {in_args}{opt_args}&svBeg, &svNb{out_args});\n"
        ));
        s.push_str(&format!("        lb = {method}_Lookback({});\n", {
            let a: Vec<String> = func.optional_inputs.iter().map(|o| o.name.clone()).collect();
            a.join(", ")
        }));
        // Batch failed or produced nothing: report and restore (a valid
        // stream cannot exist either — driver checks openRejects).
        s.push_str("        if( rc != TA_SUCCESS || svNb <= 0 ) {\n");
        s.push_str("            int openRejects = 0;\n");
        s.push_str(&format!(
            "            {{ TA_{name}_Stream *st = NULL; {} TA_RetCode orc = TA_{name}_Open(&st, {in_args}svN, {opt_args}{});\n",
            out_is_int
                .iter()
                .enumerate()
                .map(|(i, is_int)| if *is_int {
                    format!("int v{i} = 0;")
                } else {
                    format!("double v{i} = 0.0;")
                })
                .collect::<Vec<_>>()
                .join(" "),
            (0..n_outs).map(|i| format!("&v{i}")).collect::<Vec<_>>().join(", ")
        ));
        s.push_str(&format!(
            "              if( orc != TA_SUCCESS && !st ) openRejects = 1; else TA_{name}_Close(st); }}\n"
        ));
        for id in &pin_ids {
            s.push_str(&format!("            TA_SetUnstablePeriod({id}, 0);\n"));
        }
        emit_sv_batch_fail_tail(&mut s, candle);

        // OpenAndFill leg: the whole filled array must equal batch(0, svN-1)
        // bit-for-bit, and its begIdx/nb must match batch. Same seeded inputs +
        // same algorithm => same bits — an INVARIANT check (batch's numeric
        // correctness is owned by the reference-oracle tests, not re-checked
        // here). Runs where bbuf still holds batch(0,svN-1) at the current
        // K/compat/(candle round) settings. Every streamable function has an
        // OpenAndFill, so the leg is unconditional (the driver's fill-coverage
        // floor asserts every streaming function reaches here).
        let fbuf_names: Vec<String> = {
            let (mut ri, mut ii) = (0usize, 0usize);
            out_is_int
                .iter()
                .map(|is_int| {
                    if *is_int {
                        let e = format!("sv_if{ii}");
                        ii += 1;
                        e
                    } else {
                        let e = format!("sv_f{ri}");
                        ri += 1;
                        e
                    }
                })
                .collect()
        };
        {
            let fbuf: Vec<String> = fbuf_names.clone();
            let fill_arrays = fbuf.join(", ");
            s.push_str("        {\n");
            s.push_str("            int fBeg = 0, fNb = 0, ft;\n");
            s.push_str(&format!("            TA_{name}_Stream *stf = NULL;\n"));
            // Declared, not initialised: the canary stamp has to run between the
            // declarations and the call, and these buffers are `static` (reused
            // across every function in the request), so a stale value left by an
            // earlier call would otherwise read as a write by this one.
            s.push_str("            TA_RetCode frc;\n");
            // Stamped over the FULL buffer width, not `svN`. Two reasons, and
            // both bounds must move together: a lookback-0 function has
            // `fNb == svN`, so a window of `[fNb, svN)` is empty and the check
            // is a no-op for it; and a one-past-the-range write lands at index
            // `svN` itself, in the tail beyond the request's series length.
            // Widening only the assert would instead read bytes an earlier
            // function in the same request left behind (these are `static`).
            s.push_str(&c_canary_stamp(&fbuf, &out_is_int));
            s.push_str(&format!(
                "            frc = TA_{name}_OpenAndFill(&stf, {in_args}svN, {opt_args}&fBeg, &fNb, {fill_arrays});\n"
            ));
            s.push_str("            fillChecked = 1;\n");
            s.push_str("            if( frc != TA_SUCCESS || !stf || fBeg != svBeg || fNb != svNb ) fillOk = 0;\n");
            s.push_str("            else for( ft = 0; fillOk && ft < svNb; ft++ ) {\n");
            for (i, is_int) in out_is_int.iter().enumerate() {
                if *is_int {
                    s.push_str(&format!(
                        "                if( {}[ft] != {}[ft] ) fillOk = 0;\n",
                        fbuf[i], bbuf[i]
                    ));
                } else {
                    s.push_str(&format!(
                        "                if( sv_xtier_ne({}[ft], {}[ft], &svZsign) ) fillOk = 0;\n",
                        fbuf[i], bbuf[i]
                    ));
                }
            }
            s.push_str("            }\n");
            // The slack above the produced range must still hold the canary.
            // Nothing else in the tree checks it: every gate sizes the fill
            // buffer at full history and reads only [0, nb), so a write past
            // `nb` lands in `lookback` elements of unread space.
            s.push_str(&c_canary_check(&fbuf, &out_is_int));
            emit_sv_range_check(&mut s, "            ", "stf", "frc == TA_SUCCESS && stf", "svBeg", "svNb", SvRangeSite::Fill);
            s.push_str(&format!("            if( stf ) TA_{name}_Close(stf);\n"));
            s.push_str("        }\n");

            // Aliasing-guard probe: an OpenAndFill output that aliases an input
            // MUST reject (out==in is forbidden — stricter than batch — because
            // the capture epilogue re-reads the input tail; a dropped guard would
            // silently corrupt). The gate's normal leg passes distinct buffers,
            // so without this the guard is unverified. Only when output 0 is
            // real: an integer output cannot alias a double input, and there is
            // no integer input to alias against.
            if !out_is_int.first().copied().unwrap_or(true) && !input_arrays.is_empty() {
                let alias_out: Vec<String> = fbuf
                    .iter()
                    .enumerate()
                    .map(|(i, b)| if i == 0 { input_arrays[0].to_string() } else { b.clone() })
                    .collect::<Vec<_>>();
                let alias_args = alias_out.join(", ");
                s.push_str("        {\n");
                s.push_str("            int alB = 0, alN = 0;\n");
                s.push_str(&format!("            TA_{name}_Stream *sal = NULL;\n"));
                s.push_str(&format!(
                    "            TA_RetCode alrc = TA_{name}_OpenAndFill(&sal, {in_args}svN, {opt_args}&alB, &alN, {alias_args});\n"
                ));
                s.push_str("            if( !( alrc == TA_BAD_PARAM && !sal ) ) fillOk = 0;\n");
                s.push_str(&format!("            if( sal ) TA_{name}_Close(sal);\n"));
                s.push_str("        }\n");
            }
            // Output-output aliasing probe (multi-output funcs): two outputs
            // sharing a buffer must reject (#108 class). Covers the mutual-
            // distinctness guard the input-output probe above never touches, and
            // exercises the integer-output multi-out funcs (MINMAXINDEX) whose
            // guard the input-output probe skips.
            //
            // The pair has to be SAME-TYPED, and is searched for rather than
            // assumed to be (0, 1): substituting a `double*` buffer into an
            // `int*` slot is an incompatible-pointer-type, i.e. the generated
            // server would not compile. Searching keeps the probe alive for a
            // mixed-type function whose same-typed pair is not adjacent —
            // SYNTH12's reals are outputs 0 and 2. C# already pairs by type
            // (below); Java gates on `!out_is_int[1]`.
            let aa_pair = (0..n_outs)
                .flat_map(|i| ((i + 1)..n_outs).map(move |j| (i, j)))
                .find(|&(i, j)| out_is_int[i] == out_is_int[j]);
            if let Some((ai, aj)) = aa_pair {
                let aa_out: Vec<String> = fbuf
                    .iter()
                    .enumerate()
                    .map(|(i, b)| if i == aj { fbuf[ai].clone() } else { b.clone() })
                    .collect::<Vec<_>>();
                let aa_args = aa_out.join(", ");
                s.push_str("        {\n");
                s.push_str("            int aaB = 0, aaN = 0;\n");
                s.push_str(&format!("            TA_{name}_Stream *saa = NULL;\n"));
                s.push_str(&format!(
                    "            TA_RetCode aarc = TA_{name}_OpenAndFill(&saa, {in_args}svN, {opt_args}&aaB, &aaN, {aa_args});\n"
                ));
                s.push_str("            if( !( aarc == TA_BAD_PARAM && !saa ) ) fillOk = 0;\n");
                s.push_str(&format!("            if( saa ) TA_{name}_Close(saa);\n"));
                s.push_str("        }\n");
            }
        }

        // Prefix sweep candidates (dedup, clamped to [lb+1, svN-1]).
        // Seed-boundary functions (RSI/CMO under Metastock) honestly reject
        // Open at exactly lookback+1 — the batch would rewind past that
        // state — so the boundary leg starts one bar later there.
        let seed_shift = func_has_seed_boundary(func, funcs);
        s.push_str("        npref = 0;\n");
        if seed_shift {
            s.push_str("        pc[0] = lb + 1 + ((svCompat == 1) ? 1 : 0); pc[1] = lb + 13; pc[2] = svN / 2; pc[3] = svN - 1;\n");
        } else {
            s.push_str("        pc[0] = lb + 1; pc[1] = lb + 13; pc[2] = svN / 2; pc[3] = svN - 1;\n");
        }
        s.push_str("        for( li = 0; li < 4; li++ ) {\n");
        s.push_str("            int P = pc[li]; int seen = 0, k;\n");
        s.push_str("            if( P < lb + 1 ) P = lb + 1;\n");
        s.push_str("            if( P > svN - 1 ) P = svN - 1;\n");
        s.push_str("            if( P < 1 ) continue;\n");
        s.push_str("            for( k = 0; k < npref; k++ ) if( pref[k] == P ) seen = 1;\n");
        s.push_str("            if( !seen ) pref[npref++] = P;\n");
        s.push_str("        }\n");
        emit_sv_state_open(&mut s, name, steq, &out_is_int, &in_args, &opt_args);
        if !candle {
            s.push_str("        pos = json_appendf(resp, resp_size, 0, \"{\\\"retCode\\\":0,\\\"beg\\\":%d,\\\"nb\\\":%d,\\\"legs\\\":%d\", svBeg, svNb, npref);\n");
        }

        // Per-leg: open on prefix, update the rest, peek spot-asserts,
        // bitwise compare against the batch outputs at every bar.
        s.push_str("        for( li = 0; li < npref; li++ ) {\n");
        s.push_str("            int P = pref[li]; int t, ok = 1, pkOk = 1, badBar = -1, badOut = -1;\n");
        s.push_str("            double bv = 0.0, sv = 0.0;\n");
        s.push_str(&format!("            TA_{name}_Stream *st = NULL;\n"));
        for (i, is_int) in out_is_int.iter().enumerate() {
            if *is_int {
                s.push_str(&format!("            int v{i} = 0, pk{i} = 0;\n"));
            } else {
                s.push_str(&format!("            double v{i} = 0.0, pk{i} = 0.0;\n"));
            }
        }
        let vout_args: String = (0..n_outs)
            .map(|i| format!("&v{i}"))
            .collect::<Vec<_>>()
            .join(", ");
        let pkout_args: String = (0..n_outs)
            .map(|i| format!("&pk{i}"))
            .collect::<Vec<_>>()
            .join(", ");
        s.push_str(&format!(
            "            rc = TA_{name}_Open(&st, {in_args}P, {opt_args}{vout_args});\n"
        ));
        s.push_str("            if( rc != TA_SUCCESS || !st ) { ok = 0; badBar = P - 1; }\n");
        // Compare the open value (bar P-1).
        emit_sv_compare(&mut s, &out_is_int, &bbuf, "            ", "(P - 1) - svBeg", "P - 1", "ok &&");
        // Update the remaining bars.
        let mut bar_args = String::new();
        for a in &input_arrays {
            let _ = std::fmt::Write::write_fmt(&mut bar_args, format_args!("{a}[t], "));
        }
        s.push_str("            for( t = P; ok && t < svN; t++ ) {\n");
        s.push_str("                int doPeek = ((t % SV_PEEK_EVERY) == 0);\n");
        s.push_str(&format!(
            "                if( doPeek ) TA_{name}_Peek(st, {bar_args}{pkout_args});\n"
        ));
        s.push_str(&format!(
            "                TA_{name}_Update(st, {bar_args}{vout_args});\n"
        ));
        let peek_ne: Vec<String> = (0..n_outs)
            .map(|i| {
                if out_is_int[i] {
                    format!("(pk{i} != v{i})")
                } else {
                    format!("sv_bitne(pk{i}, v{i})")
                }
            })
            .collect();
        s.push_str(&format!(
            "                if( doPeek && ({}) ) pkOk = 0;\n",
            peek_ne.join(" || ")
        ));
        emit_sv_compare(&mut s, &out_is_int, &bbuf, "                ", "t - svBeg", "t", "");
        s.push_str("            }\n");
        emit_sv_state_compare(&mut s, name, steq);
        // Open(P) + (svN - P) updates: whatever P was, the handle has consumed
        // svN bars and must report exactly what batch(0, svN-1) did.
        emit_sv_range_check(&mut s, "            ", "st", "ok && st", "svBeg", "svNb", SvRangeSite::Prefix);
        s.push_str(&format!("            if( st ) TA_{name}_Close(st);\n"));
        if candle {
            s.push_str("            pos = json_appendf(resp, resp_size, pos, \",\\\"p%d\\\":%d,\\\"match%d\\\":%d,\\\"peek%d\\\":%d\", lgi, P, lgi, ok, lgi, pkOk);\n");
            s.push_str("            if( !ok ) { allOk = 0; pos = json_appendf(resp, resp_size, pos, \",\\\"bar%d\\\":%d,\\\"out%d\\\":%d,\\\"batchv%d\\\":\\\"%a\\\",\\\"streamv%d\\\":\\\"%a\\\"\", lgi, badBar, lgi, badOut, lgi, bv, lgi, sv); }\n");
            s.push_str("            if( !pkOk ) peekAll = 0;\n");
            s.push_str("            lgi++;\n");
            s.push_str("        }\n");
        } else {
            s.push_str("            pos = json_appendf(resp, resp_size, pos, \",\\\"p%d\\\":%d,\\\"match%d\\\":%d,\\\"peek%d\\\":%d\", li, P, li, ok, li, pkOk);\n");
            s.push_str("            if( !ok ) { allOk = 0; pos = json_appendf(resp, resp_size, pos, \",\\\"bar%d\\\":%d,\\\"out%d\\\":%d,\\\"batchv%d\\\":\\\"%a\\\",\\\"streamv%d\\\":\\\"%a\\\"\", li, badBar, li, badOut, li, bv, li, sv); }\n");
            s.push_str("            if( !pkOk ) peekAll = 0;\n");
            s.push_str("        }\n");
        }
        emit_sv_update_fill_leg(
            &mut s, name, &input_arrays, &in_args, &opt_args, &out_is_int, &bbuf, &fbuf_names,
        );
        emit_sv_state_close(&mut s, name, steq);
        if candle {
            s.push_str("        }\n");
            s.push_str("        if( rounds > 1 ) TA_RestoreCandleDefaultSettings( TA_AllCandleSettings );\n");
        }

        // startIdx>0 coverage: the anchored internal open (OpenInternal at a
        // non-zero startIdx over the FULL history from bar 0) must equal
        // batch(S). This exercises the extra anchor parameter for EVERY stream
        // function — not just composed sub-callees — under the same K/compat.
        // (Reuses the bbuf batch buffers, recomputed at startIdx=S; the prefix
        // sweep above is done with them.)
        {
            let aout: String = (0..n_outs).map(|i| format!("&v{i}")).collect::<Vec<_>>().join(", ");
            s.push_str("        {\n");
            s.push_str("            int Sidx = lb + (svN - lb) / 3;\n");
            s.push_str("            if( Sidx > lb && Sidx < svN - 1 ) {\n");
            s.push_str("                int svBegS = 0, svNbS = 0;\n");
            s.push_str(&format!(
                "                rc = {method}(Sidx, svN - 1, {in_args}{opt_args}&svBegS, &svNbS{out_args});\n"
            ));
            s.push_str("                if( rc == TA_SUCCESS && svNbS > 0 ) {\n");
            s.push_str("                    int ok = 1, badBar = -1, badOut = -1; double bv = 0.0, sv = 0.0;\n");
            for (i, is_int) in out_is_int.iter().enumerate() {
                let (ty, z) = if *is_int { ("int", "0") } else { ("double", "0.0") };
                s.push_str(&format!("                    {ty} v{i} = {z};\n"));
            }
            s.push_str(&format!("                    TA_{name}_Stream *stA = NULL;\n"));
            s.push_str(&format!(
                "                    TA_RetCode arc = TA_{name}_OpenInternal(&stA, {in_args}Sidx, svN, {opt_args}{aout});\n"
            ));
            s.push_str("                    if( arc != TA_SUCCESS || !stA ) ok = 0;\n");
            emit_sv_compare(&mut s, &out_is_int, &bbuf, "                    ", "(svN - 1) - svBegS", "svN - 1", "ok &&");
            emit_sv_range_check(&mut s, "                    ", "stA", "ok && stA", "svBegS", "svNbS", SvRangeSite::Anchored);

            s.push_str(&format!("                    if( stA ) TA_{name}_Close(stA);\n"));
            s.push_str("                    if( !ok ) allOk = 0;\n");
            s.push_str("                    (void)badBar; (void)badOut; (void)bv; (void)sv;\n");
            s.push_str("                }\n");
            s.push_str("            }\n");
            s.push_str("        }\n");
        }

        for id in &pin_ids {
            s.push_str(&format!("        TA_SetUnstablePeriod({id}, 0);\n"));
        }
        s.push_str("        TA_SetCompatibility((TA_Compatibility)savedCompat);\n");
        // Fold fill into ok as a safety net (the driver also checks fill_ok
        // explicitly for a clearer message), so a fill regression fails the run
        // even if the driver's fill check ever regresses.
        s.push_str("        if( fillChecked && !fillOk ) allOk = 0;\n");
        s.push_str("        if( ufillChecked && !ufillOk ) allOk = 0;\n");
        emit_sv_state_report(&mut s, steq);
        emit_sv_range_report(&mut s);
        if candle {
            s.push_str("        pos = json_appendf(resp, resp_size, pos, \",\\\"beg\\\":%d,\\\"nb\\\":%d,\\\"legs\\\":%d,\\\"fill_checked\\\":%d,\\\"fill_ok\\\":%d,\\\"ufill_checked\\\":%d,\\\"ufill_ok\\\":%d,\\\"ok\\\":%d,\\\"peek_ok\\\":%d,\\\"benign\\\":%d}\", svBeg, svNb, lgi, fillChecked, fillOk, ufillChecked, ufillOk, allOk, peekAll, svZsign);\n");
        } else {
            s.push_str("        pos = json_appendf(resp, resp_size, pos, \",\\\"fill_checked\\\":%d,\\\"fill_ok\\\":%d,\\\"ufill_checked\\\":%d,\\\"ufill_ok\\\":%d,\\\"ok\\\":%d,\\\"peek_ok\\\":%d,\\\"benign\\\":%d}\", fillChecked, fillOk, ufillChecked, ufillOk, allOk, peekAll, svZsign);\n");
        }
        s.push_str("        return;\n");
        s.push_str("    }\n");
    }

    // Unknown / non-streamable function.
    s.push_str("    TA_SetCompatibility((TA_Compatibility)savedCompat);\n");
    s.push_str("    snprintf(resp, resp_size, \"{\\\"error\\\":\\\"not_streamable\\\"}\");\n");
    s.push_str("}\n");
    s.push_str("#else /* TA_REF_SERVE: frozen libs have no stream symbols */\n");
    s.push_str("static void handle_stream_verify(const char *json, char *resp, int resp_size) {\n");
    s.push_str("    (void)json;\n");
    s.push_str("    snprintf(resp, resp_size, \"{\\\"error\\\":\\\"not supported\\\"}\");\n");
    s.push_str("}\n");
    s.push_str("#endif /* TA_REF_SERVE */\n\n");
    s
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
        s.push_str("            snprintf(resp, resp_size, \"{\\\"retCode\\\":%d,\\\"outBegIdx\\\":%d,\\\"outNBElement\\\":%d,\\\"out_hash\\\":\\\"%016llx\\\"}\", (int)rc, outBegIdx, outNBElement, _oh);\n");
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
        s.push_str("        pos = json_appendf(resp, resp_size, pos, \",\\\"used_float\\\":%d}\", usedFloat);\n");

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

    // set_compatibility method — {"method":"set_compatibility","params":{"mode":1}}
    s.push_str("    else if ( methodLen == 17 && strncmp(method, \"set_compatibility\", 17) == 0 ) {\n");
    s.push_str("        int mode = json_find_int(json, \"mode\");\n");
    s.push_str("        TA_SetCompatibility((TA_Compatibility)mode);\n");
    s.push_str("        snprintf(resp, resp_size, \"{\\\"status\\\":\\\"ok\\\"}\");\n");
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

    // FuncUnstId and Compatibility enums (referenced by generated Core methods).
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

    // No Compatibility enum: the Java backend constant-folds the Metastock arms
    // out of the generated indicator code (the shipped Core has no such setting),
    // so nothing spliced in here can reference one.

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

    // set_compatibility method. The Java library exposes no way to select a
    // compatibility variant (the Metastock arms are constant-folded out of the
    // generated code), so mode 0 is a no-op and any other mode is an explicit
    // error — the driver skips that leg rather than silently comparing a Default
    // run against a Metastock reference. Mirrors the Rust server.
    s.push_str("        else if (json.contains(\"\\\"set_compatibility\\\"\")) {\n");
    s.push_str("            int mode = jsonInt(json, \"mode\");\n");
    s.push_str("            if (mode == 0) {\n");
    s.push_str("                return \"{\\\"status\\\":\\\"ok\\\"}\";\n");
    s.push_str("            }\n");
    s.push_str("            return \"{\\\"error\\\":\\\"java has no compatibility API (pinned to Default)\\\"}\";\n");
    s.push_str("        }\n");

    // set_candle_settings (#215). The C server delegates to the library and just
    // reports its RetCode; this server has no such library to ask, so it spells
    // out the same domain TA_SetCandleSettings enforces — settingType names a
    // single setting (the AllCandleSettings wildcard is NOT a target), rangeType
    // is 0..2, avgPeriod is a lookback and bounded like one, and factor is any
    // non-NaN value. Every check precedes the write, so a rejected call leaves
    // all eleven settings as they were (#186).
    s.push_str("        else if (json.contains(\"\\\"set_candle_settings\\\"\")) {\n");
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
        s.push_str("            return \"{\\\"retCode\\\":\" + rc.toInt() + \",\\\"outBegIdx\\\":\" + outBegIdx.value + \",\\\"outNBElement\\\":\" + outNBElement.value + \",\\\"out_hash\\\":\\\"\" + String.format(\"%016x\", _h) + \"\\\"}\";\n");
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
    s.push_str(&generate_java_stream_verify(funcs, enums));

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
/// the managed core or errors. A hybrid server would let 161 functions
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

    // set_compatibility — the C# library exposes no compatibility selector
    // (the Metastock arms are constant-folded out of the generated code), so
    // mode 0 is a no-op and any other mode is an explicit error. Mirrors the
    // Rust and Java servers.
    s.push_str("            else if (method == \"set_compatibility\") {\n");
    s.push_str("                int mode = GetInt(p, \"mode\", 0);\n");
    s.push_str("                if (mode == 0) {\n");
    s.push_str("                    return \"{\\\"status\\\":\\\"ok\\\"}\";\n");
    s.push_str("                }\n");
    s.push_str("                return \"{\\\"error\\\":\\\"csharp has no compatibility API (pinned to Default)\\\"}\";\n");
    s.push_str("            }\n");

    // set_candle_settings (#215). Unlike the unstable period above, this does NOT
    // reach into core's fields: it goes through the shipped CoreBuilder, so the
    // validation being compared across languages is the library's own and not a
    // second copy living in the server. Core is immutable, so a change is a
    // rebuild; the assignment happens only if the builder accepted every
    // argument, which is what keeps a rejected call from writing anything.
    s.push_str("            else if (method == \"set_candle_settings\") {\n");
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
    s.push_str(&generate_csharp_stream_verify(funcs, enums));

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
        s.push_str("            return $\"{{\\\"retCode\\\":{(int)rc},\\\"outBegIdx\\\":{outBegIdx},\\\"outNBElement\\\":{outNBElement},\\\"out_hash\\\":\\\"{_h:x16}\\\"}}\";\n");
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
        s.push_str("                return format!(\"{{\\\"retCode\\\":{},\\\"outBegIdx\\\":{},\\\"outNBElement\\\":{},\\\"out_hash\\\":\\\"{:016x}\\\"}}\", retcode_to_int(rc), outBegIdx, outNBElement, _oh);\n");
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

    // set_compatibility method. The Rust crate exposes no way to select a
    // compatibility variant (it is pinned to Default), so mode 0 is a no-op and
    // any other mode is an explicit error — the driver skips that leg rather
    // than silently comparing a Default run against a Metastock reference.
    s.push_str("        \"set_compatibility\" => {\n");
    s.push_str(
        "            let mode = params[\"mode\"].as_u64().unwrap_or(0);\n",
    );
    s.push_str("            if mode == 0 {\n");
    s.push_str(
        "                \"{\\\"status\\\":\\\"ok\\\"}\".to_string()\n",
    );
    s.push_str("            } else {\n");
    s.push_str(
        "                \"{\\\"error\\\":\\\"rust has no compatibility API (pinned to Default)\\\"}\".to_string()\n",
    );
    s.push_str("            }\n");
    s.push_str("        }\n");

    // set_candle_settings method (#215).
    s.push_str("        \"set_candle_settings\" => {\n");
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
    s.push_str(&generate_rust_stream_verify(funcs, enums));

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

// ---------------------------------------------------------------------------
// Rust stream_verify (mirror of generate_c_stream_verify for the Rust server):
// per streamable function, run the Rust batch and the Rust stream trajectory
// in-process on identical seeded inputs, compare BITWISE per bar (to_bits),
// spot-assert peek == update, verify the OpenAndFill fill against the batch
// arrays, and answer the same flat JSON contract the ta_regtest driver reads
// (ok / peek_ok / legs / fill_checked / fill_ok / benign / unsupportedArm /
// "not_streamable"). Differences from the C gate, by design:
// - No startIdx-anchored OpenInternal leg: `<f>_open_internal` is pub(crate)
//   (the anchor seam is a bit-exactness footgun, not a public API); anchored
//   opens are exercised transitively through composed functions' own legs.
// - No aliasing probes: `&mut` exclusivity makes #108 unexpressible in the
//   safe public API (the driver never reads the probes directly).
// - Settings sweeps rebuild an immutable Core via the builder instead of
//   mutating globals.
// ---------------------------------------------------------------------------

/// Substitute C enum constants (`TA_MAType_HMA`) with their integer values so
/// a `sv_reject_condition` guard renders as valid Rust or Java (both compare
/// the raw enum int).
fn sv_guard_enum_ints(guard: &str, enums: &HashMap<String, EnumDef>) -> String {
    let mut out = guard.to_string();
    for e in enums.values() {
        for v in &e.variants {
            out = out.replace(&v.c_name, &v.value.to_string());
        }
    }
    out
}

/// The per-input expanded fuzz array variable in the generated Rust handler.
fn sv_rust_input_array(name: &str, generic_idx: &mut usize) -> &'static str {
    match name {
        "inOpen" => "fz_o",
        "inHigh" => "fz_h",
        "inLow" => "fz_l",
        "inClose" => "fz_c",
        "inVolume" => "fz_v",
        "inOpenInterest" => "fz_oi",
        _ => {
            let arr = if *generic_idx == 0 { "fz_c" } else { "fz_v" };
            *generic_idx += 1;
            arr
        }
    }
}

/// Assert the slack above the produced range still holds the canary. The Rust
/// fill buffers are allocated at exactly `svN`, so the whole allocation beyond
/// `nb` is covered by this window (unlike C's fixed-width `static` buffers,
/// which must be walked to `SV_MAXN`).
fn rust_canary_check(out_is_int: &[bool]) -> String {
    let mut s = String::new();
    for (i, is_int) in out_is_int.iter().enumerate() {
        let canary = if *is_int { "-987654321i32" } else { "-1.2345678901234e300f64" };
        let _ = writeln!(s, "                    for i in nb..svN {{ if f{i}[i] != {canary} {{ fill_ok = false; }} }}");
    }
    s
}

/// One `sv_<name>` verify function for a function with an emitted Rust stream.
#[allow(clippy::too_many_lines)]
fn emit_rust_sv_func(func: &FuncDef, funcs: &[FuncDef], enums: &HashMap<String, EnumDef>) -> String {
    use std::fmt::Write as _;
    // The server-local verify fn stays snake_case (`sv_sma`); library calls use
    // the verbatim function name.
    let sn = func.name.to_lowercase();
    let fname = &func.name;
    let fname_snake = crate::backends::common::snake_words(fname);
    let candle = func.name.starts_with("CDL");
    let inputs = crate::streaming::input_array_names(func);
    let mut gi = 0usize;
    let arrays: Vec<&'static str> = inputs
        .iter()
        .map(|i| sv_rust_input_array(i, &mut gi))
        .collect();
    let out_is_int: Vec<bool> = func
        .outputs
        .iter()
        .map(|o| o.param_type == crate::ir::ParamType::Integer)
        .collect();

    let mut s = String::new();
    let _ = writeln!(s, "fn sv_{sn}(core: &Core, params: &Value) -> String {{");
    s.push_str("    let svShape = params[\"gen_shape\"].as_i64().unwrap_or(0) as i32;\n");
    s.push_str("    let svSeed = params[\"gen_seed\"].as_i64().unwrap_or(0) as i32;\n");
    s.push_str("    let mut svN = params[\"gen_n\"].as_i64().unwrap_or(0) as usize;\n");
    s.push_str("    if svN < 2 { svN = 2; }\n    if svN > 256 { svN = 256; }\n");
    // u32 to match the builder's setter. Refused rather than clamped: silently
    // reading a negative warm-up as 0 would make this arm pass while measuring
    // something the C side never asked for.
    s.push_str("    let svK = match u32::try_from(params[\"unstablePeriod\"].as_i64().unwrap_or(0)) {\n");
    s.push_str("        Ok(v) => v,\n");
    s.push_str("        Err(_) => return \"{\\\"error\\\":\\\"negative unstablePeriod\\\"}\".to_string(),\n");
    s.push_str("    };\n");
    s.push_str("    let svCompat = params[\"compatibility\"].as_i64().unwrap_or(0) as i32;\n");
    // Compatibility is pinned to Default in the Rust crate; a Metastock leg would
    // silently re-run the Default one, so refuse it instead of passing vacuously.
    s.push_str("    if svCompat != 0 {\n");
    s.push_str("        return \"{\\\"error\\\":\\\"rust has no compatibility API (pinned to Default)\\\"}\".to_string();\n");
    s.push_str("    }\n");
    if candle {
        s.push_str("    let candleLegs = params[\"candleLegs\"].as_i64().unwrap_or(0);\n");
    }
    // Optional params with YAML defaults (the driver always sends them, but the
    // defaults keep hand-driven requests working).
    for p in &func.optional_inputs {
        let name = &p.name;
        if p.param_type == crate::ir::ParamType::Real {
            let d = p.default.unwrap_or(0.0);
            let _ = writeln!(
                s,
                "    let {name} = params[\"{name}\"].as_f64().unwrap_or({d:?});"
            );
        } else {
            #[allow(clippy::cast_possible_truncation)]
            let d = p.default.unwrap_or(0.0) as i64;
            if let crate::ir::ParamType::Enum(enum_name) = &p.param_type {
                // The driver DOES send an out-of-list value here on purpose
                // (`maxList + 91`, test_codegen.c): batch rejects it at the
                // dispatch default arm, so the stream must reject too. A typed
                // enum cannot carry it, so reject at the type level and report
                // it — the same shape, and the same reason, as the Java server.
                let _ = writeln!(
                    s,
                    "    let {name}_raw = params[\"{name}\"].as_i64().unwrap_or({d}) as i32;"
                );
                let _ = writeln!(s, "    let {name} = match {enum_name}::try_from({name}_raw) {{");
                s.push_str("        Ok(v) => v,
");
                s.push_str("        Err(_) => return \"{\\\"retCode\\\":2,\\\"legs\\\":0,\\\"nb\\\":0,\\\"openRejects\\\":1,\\\"ok\\\":1,\\\"peek_ok\\\":1}\".to_string(),
");
                s.push_str("    };
");
            } else {
                let _ = writeln!(
                    s,
                    "    let {name} = params[\"{name}\"].as_i64().unwrap_or({d}) as i32;"
                );
            }
        }
    }
    // Seeded inputs.
    s.push_str("    let mut fz_o = vec![0.0f64; svN];\n    let mut fz_h = vec![0.0f64; svN];\n    let mut fz_l = vec![0.0f64; svN];\n    let mut fz_c = vec![0.0f64; svN];\n    let mut fz_v = vec![0.0f64; svN];\n    let mut fz_oi = vec![0.0f64; svN];\n");
    s.push_str("    fuzz_gen(svShape, svSeed, svN as i32, &mut fz_o, &mut fz_h, &mut fz_l, &mut fz_c, &mut fz_v, &mut fz_oi);\n");

    // Period-bank ramp: the fuzz period-selector series would clamp to
    // maxPeriod at every bar (vacuous slots) — overwrite with a ramp spanning
    // [min-1, max+1] fed identically to batch and stream.
    {
        let lookup = crate::streaming::FuncsLookup(funcs);
        if let Ok(crate::streaming::StreamPlan::PeriodBank(pb)) =
            crate::streaming::validate_streamable(func, &lookup)
        {
            if let Some(idx) = inputs.iter().position(|i| *i == pb.period_input) {
                let arr = arrays[idx];
                let _ = writeln!(
                    s,
                    "    for _pi in 0..svN {{ {arr}[_pi] = ({min} + ((_pi as i32) % ({max} - {min} + 3)) - 1) as f64; }}",
                    min = pb.min_param,
                    max = pb.max_param
                );
            }
        }
    }

    // Convenience strings for calls.
    let full_ins = arrays
        .iter()
        .map(|a| format!("&{a}"))
        .collect::<Vec<_>>()
        .join(", ");
    let pfx_ins = arrays
        .iter()
        .map(|a| format!("&{a}[..p]"))
        .collect::<Vec<_>>()
        .join(", ");
    let opts = func
        .optional_inputs
        .iter()
        .map(|p| p.name.clone())
        .collect::<Vec<_>>()
        .join(", ");
    let opts_lead = if opts.is_empty() { String::new() } else { format!("{opts}, ") };
    let opts_tail = if opts.is_empty() { String::new() } else { format!(", {opts}") };

    // Batch output buffers.
    let mut bdecls = String::new();
    let mut bargs = String::new();
    let mut fdecls = String::new();
    let mut fargs = String::new();
    for (i, is_int) in out_is_int.iter().enumerate() {
        let (ty, z) = if *is_int { ("i32", "0i32") } else { ("f64", "0.0f64") };
        // A nullable output takes `Option<&mut [T]>` (rule B6a); this harness
        // compares values, so it always supplies one.
        let some = func.outputs.get(i).is_some_and(crate::ir::Output::is_nullable);
        let (op, cl) = if some { ("Some(", ")") } else { ("", "") };
        let _ = writeln!(bdecls, "    let mut b{i}: Vec<{ty}> = vec![{z}; svN];");
        let _ = write!(bargs, ", {op}&mut b{i}{cl}");
        // Canary-filled, not zero-filled: the slack above the produced range is
        // asserted untouched after the call (#205's write bound), so a write
        // past `nb` fails instead of landing in unread space.
        let canary = if *is_int { "-987654321i32" } else { "-1.2345678901234e300f64" };
        let _ = writeln!(fdecls, "        let mut f{i}: Vec<{ty}> = vec![{canary}; svN];");
        let _ = write!(fargs, ", {op}&mut f{i}{cl}");
    }
    s.push_str(&bdecls);

    s.push_str("    let mut legs = 0i64;\n    let mut all_ok = true;\n    let mut peek_all = true;\n    let mut fill_checked = 0i32;\n    let mut fill_ok = true;\n    let mut beg = 0usize;\n    let mut nb = 0usize;\n    let mut diag = String::new();\n");
    // The range leg (#241): a handle's OutRange against what batch reported for
    // the same bars. Public API in every backend, so unlike the state leg this
    // one is not C-only.
    s.push_str("    let mut range_checked = 0i32;\n    let mut range_ok = true;\n    let mut range_legs = 0i64;\n    let mut range_sites = 0i32;\n");
    // The n-bar filler's own leg (issue #246), reported apart from the
    // open-time fill so a regression names the entry point it is in.
    s.push_str("    let mut ufill_checked = 0i32;\n    let mut ufill_ok = true;\n");
    // Benign +/-0 cases across every cross-tier compare in this request. `mut`
    // only when an output can reach sv_xtier_ne: an all-integer function (every
    // CDL*, MIN/MAX/MINMAXINDEX, HT_TRENDMODE) compares with `!=` and only ever
    // reads this, and rustc's unused_mut is not in the generated crate's allow
    // list — same reason cb_mut below is conditional.
    let z_mut = if out_is_int.iter().any(|b| !*b) { "mut " } else { "" };
    let _ = writeln!(s, "    let {z_mut}zsign = 0i64;");
    let rounds = if candle {
        "    let rounds = if candleLegs != 0 { 4 } else { 1 };\n"
    } else {
        "    let rounds = 1;\n"
    };
    s.push_str(rounds);
    s.push_str("    for rd in 0..rounds {\n        let _ = rd;\n");

    // Pinned + configured core for this round. `mut` only when something below
    // actually reassigns it (no compatibility leg any more — the Rust crate
    // pins the mode to Default), otherwise rustc warns on every such function.
    let pin_ids = collect_pin_ids(func, funcs, enums);
    let cb_mut = if pin_ids.is_empty() && !candle { "" } else { "mut " };
    let _ = writeln!(s, "        let {cb_mut}cb = core.to_builder();");
    for id in pin_ids {
        let _ = writeln!(
            s,
            "        if let Some(id) = func_unst_id_from_int({id}usize) {{ cb = cb.unstable_period(id, svK); }}"
        );
    }
    if candle {
        s.push_str("        cb = sv_apply_candles(cb, &sv_candle_settings(rd));\n");
    }
    // Reported, never unwrapped: this runs in a subprocess ta_regtest drives over
    // a pipe, so a panic here would surface as a dead pipe rather than a
    // diagnosable BadParam.
    s.push_str("        let c2 = match cb.build() {\n");
    s.push_str("            Ok(c) => c,\n");
    s.push_str(
        "            Err(_) => return \"{\\\"error\\\":\\\"unstablePeriod out of range\\\"}\".to_string(),\n",
    );
    s.push_str("        };\n");

    // Expected-reject precheck (dispatch / period-bank arms without a stream).
    if let Some(guard) = sv_reject_condition(func, funcs, None) {
        let guard = sv_guard_enum_ints(&guard, enums);
        let _ = writeln!(s, "        if {guard} {{");
        let _ = writeln!(
            s,
            "            let r1 = c2.{fname_snake}_open({full_ins}{opts_tail}).is_err();"
        );
        s.push_str(&fdecls.replace("        ", "            "));
        let _ = writeln!(
            s,
            "            let r2 = c2.{fname_snake}_open_and_fill({full_ins}{opts_tail}{fargs}).is_err();"
        );
        s.push_str("            let okr = r1 && r2;\n");
        s.push_str("            return format!(\"{{\\\"retCode\\\":0,\\\"legs\\\":0,\\\"unsupportedArm\\\":1,\\\"ok\\\":{},\\\"peek_ok\\\":1}}\", i32::from(okr));\n");
        s.push_str("        }\n");
    }

    // Batch leg.
    // The public tier returns the range; `beg`/`nb` stay as locals because the legs
    // below (OpenAndFill, Peek) compare against them.
    let bargs_head = bargs.trim_start_matches(", ");
    let _ = writeln!(
        s,
        "        let rc = match c2.{fname}(0, svN - 1, {full_ins}, {opts_lead}{bargs_head}) {{ Ok(r) => {{ beg = r.beg_idx; nb = r.count; RetCode::Success }} Err(e) => {{ beg = 0; nb = 0; e }} }};"
    );
    let _ = writeln!(s, "        let lb = c2.{fname}_Lookback({opts}).unwrap_or(usize::MAX);");
    s.push_str("        if rc != RetCode::Success || nb == 0 {\n");
    let _ = writeln!(
        s,
        "            let open_rejects = c2.{fname_snake}_open({full_ins}{opts_tail}).is_err();"
    );
    if candle {
        s.push_str("            if !open_rejects { all_ok = false; }\n");
        s.push_str("            if rd + 1 < rounds { continue; }\n");
        s.push_str("            return format!(\"{{\\\"retCode\\\":{},\\\"legs\\\":{},\\\"nb\\\":{},\\\"openRejects\\\":{},\\\"ok\\\":{},\\\"peek_ok\\\":{},\\\"benign\\\":{}}}\", retcode_to_int(rc), legs, nb, i32::from(open_rejects), i32::from(all_ok), i32::from(peek_all), zsign);\n");
    } else {
        s.push_str("            return format!(\"{{\\\"retCode\\\":{},\\\"legs\\\":0,\\\"nb\\\":{},\\\"openRejects\\\":{},\\\"ok\\\":{},\\\"peek_ok\\\":1}}\", retcode_to_int(rc), nb, i32::from(open_rejects), i32::from(open_rejects));\n");
    }
    s.push_str("        }\n");

    // OpenAndFill leg (fill == batch arrays, bitwise).
    s.push_str("        fill_checked = 1;\n        {\n");
    s.push_str(&fdecls);
    let _ = writeln!(
        s,
        "        match c2.{fname_snake}_open_and_fill({full_ins}{opts_tail}{fargs}) {{"
    );
    s.push_str("            Err(_) => { fill_ok = false; }\n");
    let fill_bit = sv_range_bit(SvRangeSite::Fill, SV_RANGE_SITES_RUST);
    s.push_str("            Ok((_h, fr)) => {\n                range_checked = 1; range_legs += 1; range_sites |= ");
    s.push_str(&fill_bit.to_string());
    s.push_str(";\n                if _h.out_range().beg_idx != beg || _h.out_range().count != nb { range_ok = false; }\n                if fr.beg_idx != beg || fr.count != nb { fill_ok = false; }\n                else {\n");
    for (i, is_int) in out_is_int.iter().enumerate() {
        if *is_int {
            let _ = writeln!(s, "                    for i in 0..nb {{ if f{i}[i] != b{i}[i] {{ fill_ok = false; }} }}");
        } else {
            let _ = writeln!(s, "                    for i in 0..nb {{ if sv_xtier_ne(f{i}[i], b{i}[i], &mut zsign) {{ fill_ok = false; }} }}");
        }
    }
    s.push_str(&rust_canary_check(&out_is_int));
    s.push_str("                }\n            }\n        }\n        }\n");

    let seed_boundary = func_has_seed_boundary(func, funcs);
    emit_rust_sv_prefix_sweep(&mut s, fname, &arrays, &pfx_ins, &opts_tail, &out_is_int, seed_boundary);
    let out_nullable: Vec<bool> = func.outputs.iter().map(crate::ir::Output::is_nullable).collect();
    emit_rust_sv_update_and_fill_leg(&mut s, fname, &arrays, &pfx_ins, &opts_tail, &out_is_int, &out_nullable);

    // Short-history reject leg: at `lb` bars no output is defined for ANY
    // configuration, so open must reject. (The seed-boundary bar `lb+1` is NOT
    // asserted either way — under an unstable period the skip can absorb the
    // Metastock seed, making it legitimately acceptable; the C gate only
    // shifts its first prefix.)
    s.push_str("        if lb >= 1 && lb < svN {\n");
    let short_ins = arrays
        .iter()
        .map(|a| format!("&{a}[..lb]"))
        .collect::<Vec<_>>()
        .join(", ");
    let _ = writeln!(
        s,
        "            if c2.{fname_snake}_open({short_ins}{opts_tail}).is_ok() {{ all_ok = false; if diag.is_empty() {{ diag = \",\\\"shortHistoryAccepted\\\":1\".to_string(); }} }}"
    );
    s.push_str("        }\n");

    s.push_str("    }\n");
    // fill_ok folds into ok as a safety net (mirrors the C gate), so a driver
    // reading only `ok` — e.g. the debug sweep — still fails on a fill regression.
    s.push_str("    format!(\"{{\\\"retCode\\\":0,\\\"beg\\\":{},\\\"nb\\\":{},\\\"legs\\\":{},\\\"fill_checked\\\":{},\\\"fill_ok\\\":{},\\\"ufill_checked\\\":{},\\\"ufill_ok\\\":{},\\\"range_checked\\\":{},\\\"range_legs\\\":{},\\\"range_sites\\\":{},\\\"range_sites_n\\\":"); s.push_str(&SV_RANGE_SITES_RUST.to_string()); s.push_str(",\\\"range_ok\\\":{},\\\"ok\\\":{},\\\"peek_ok\\\":{},\\\"benign\\\":{}{}}}\", beg, nb, legs, fill_checked, i32::from(fill_ok), ufill_checked, i32::from(ufill_ok), range_checked, range_legs, range_sites, i32::from(range_ok), i32::from(all_ok && fill_ok && ufill_ok && range_ok), i32::from(peek_all), zsign, diag)\n");
    s.push_str("}\n\n");
    s
}

/// The sweep over `pcs` (the seed-boundary / mid-corpus / tail prefixes): open at
/// each, compare the open value, then walk `update`/`peek` to the end of the
/// corpus and compare every bar against the batch arrays.
fn emit_rust_sv_prefix_sweep(
    s: &mut String,
    fname: &str,
    arrays: &[&'static str],
    pfx_ins: &str,
    opts_tail: &str,
    out_is_int: &[bool],
    seed_boundary: bool,
) {
    let n_out = out_is_int.len();
    let shift = if seed_boundary {
        "        let seed_shift: usize = if svCompat == 1 { 1 } else { 0 };\n"
    } else {
        "        let seed_shift: usize = 0;\n"
    };
    s.push_str(shift);
    s.push_str("        let mut pcs = vec![lb + 1 + seed_shift, lb + 13, svN / 2, svN - 1];\n");
    s.push_str("        pcs.retain(|p| *p >= lb + 1 + seed_shift && *p <= svN - 1);\n");
    s.push_str("        pcs.sort_unstable();\n        pcs.dedup();\n");
    s.push_str("        for &p in &pcs {\n");
    let fname_snake = crate::backends::common::snake_words(fname);
    let _ = writeln!(s, "            match c2.{fname_snake}_open({pfx_ins}{opts_tail}) {{");
    s.push_str("                Err(_) => { all_ok = false; if diag.is_empty() { diag = format!(\",\\\"openRejectP\\\":{}\", p); } }\n");
    s.push_str("                Ok((mut st, v0)) => {\n                    legs += 1;\n");
    // open-value compare
    let destructure = |var: &str| -> Vec<String> {
        if n_out == 1 {
            vec![var.to_string()]
        } else {
            (0..n_out).map(|i| format!("{var}.{i}")).collect()
        }
    };
    for (i, part) in destructure("v0").iter().enumerate() {
        if out_is_int[i] {
            let _ = writeln!(s, "                    if {part} != b{i}[p - 1 - beg] {{ all_ok = false; if diag.is_empty() {{ diag = format!(\",\\\"badBar\\\":{{}},\\\"badOut\\\":{i},\\\"where\\\":\\\"open\\\"\", p - 1); }} }}");
        } else {
            let _ = writeln!(s, "                    if sv_xtier_ne({part}, b{i}[p - 1 - beg], &mut zsign) {{ all_ok = false; if diag.is_empty() {{ diag = format!(\",\\\"badBar\\\":{{}},\\\"badOut\\\":{i},\\\"where\\\":\\\"open\\\"\", p - 1); }} }}");
        }
    }
    // update loop
    s.push_str("                    for t in p..svN {\n");
    let t_args = arrays.iter().map(|a| format!("{a}[t]")).collect::<Vec<_>>().join(", ");
    let _ = writeln!(s, "                        if t % 7 == 0 {{");
    // `update`/`peek` are fallible since the streaming tier rejects non-finite
    // bars. The fuzz corpus is finite everywhere, so a rejection here is a
    // defect, not an expected outcome — it fails the leg rather than panicking
    // the server, and names the bar in the diagnostic.
    let _ = writeln!(s, "                            let Ok(pk) = st.peek({t_args}) else {{ all_ok = false; if diag.is_empty() {{ diag = format!(\",\\\"peekRejected\\\":{{}}\", t); }} break; }};");
    let _ = writeln!(s, "                            let Ok(up) = st.update({t_args}) else {{ all_ok = false; if diag.is_empty() {{ diag = format!(\",\\\"updateRejected\\\":{{}}\", t); }} break; }};");
    let pk_parts = destructure("pk");
    let up_parts = destructure("up");
    for (i, (pk, up)) in pk_parts.iter().zip(up_parts.iter()).enumerate() {
        if out_is_int[i] {
            let _ = writeln!(s, "                            if {pk} != {up} {{ peek_all = false; }}");
        } else {
            let _ = writeln!(s, "                            if {pk}.to_bits() != {up}.to_bits() {{ peek_all = false; }}");
        }
    }
    for (i, up) in up_parts.iter().enumerate() {
        if out_is_int[i] {
            let _ = writeln!(s, "                            if {up} != b{i}[t - beg] {{ all_ok = false; if diag.is_empty() {{ diag = format!(\",\\\"badBar\\\":{{}},\\\"badOut\\\":{i},\\\"batchv\\\":\\\"{{}}\\\",\\\"streamv\\\":\\\"{{}}\\\"\", t, b{i}[t - beg], {up}); }} }}");
        } else {
            let _ = writeln!(s, "                            if sv_xtier_ne({up}, b{i}[t - beg], &mut zsign) {{ all_ok = false; if diag.is_empty() {{ diag = format!(\",\\\"badBar\\\":{{}},\\\"badOut\\\":{i},\\\"batchv\\\":\\\"{{:016x}}\\\",\\\"streamv\\\":\\\"{{:016x}}\\\"\", t, b{i}[t - beg].to_bits(), {up}.to_bits()); }} }}");
        }
    }
    s.push_str("                        } else {\n");
    let _ = writeln!(s, "                            let Ok(up) = st.update({t_args}) else {{ all_ok = false; if diag.is_empty() {{ diag = format!(\",\\\"updateRejected\\\":{{}}\", t); }} break; }};");
    for (i, up) in up_parts.iter().enumerate() {
        if out_is_int[i] {
            let _ = writeln!(s, "                            if {up} != b{i}[t - beg] {{ all_ok = false; if diag.is_empty() {{ diag = format!(\",\\\"badBar\\\":{{}},\\\"badOut\\\":{i},\\\"batchv\\\":\\\"{{}}\\\",\\\"streamv\\\":\\\"{{}}\\\"\", t, b{i}[t - beg], {up}); }} }}");
        } else {
            let _ = writeln!(s, "                            if sv_xtier_ne({up}, b{i}[t - beg], &mut zsign) {{ all_ok = false; if diag.is_empty() {{ diag = format!(\",\\\"badBar\\\":{{}},\\\"badOut\\\":{i},\\\"batchv\\\":\\\"{{:016x}}\\\",\\\"streamv\\\":\\\"{{:016x}}\\\"\", t, b{i}[t - beg].to_bits(), {up}.to_bits()); }} }}");
        }
    }
    s.push_str("                        }\n");
    s.push_str("                    }\n");
    // Open(p) + (svN - p) updates: whatever p was, the handle has consumed svN
    // bars and must report exactly what batch(0, svN-1) did.
    // Only when the value leg passed: its update loop breaks on a rejected bar,
    // which leaves the handle short of the bars it was supposed to consume.
    s.push_str("                    if all_ok {\n");
    s.push_str("                        range_checked = 1; range_legs += 1; range_sites |= ");
    s.push_str(&sv_range_bit(SvRangeSite::Prefix, SV_RANGE_SITES_RUST).to_string());
    s.push_str(";\n");
    s.push_str("                        if st.out_range().beg_idx != beg || st.out_range().count != nb { range_ok = false; }\n");
    s.push_str("                    }\n");
    s.push_str("                }\n            }\n        }\n");
}

/// Which output the "too short for the run" probe undersizes: the first one that
/// is NOT `nullable`.
///
/// Zero length is how C# spells "declined", so undersizing a declinable output
/// there asserts a declination is accepted, not that a short buffer is rejected
/// — the opposite of the rule the probe is standing in for. Java and Rust would
/// still reject it, so the choice only has to be right for C#; making it the
/// same everywhere keeps the harness from depending on that.
fn short_probe_index(out_nullable: &[bool]) -> usize {
    out_nullable
        .iter()
        .position(|n| !n)
        .expect("every function has a required output (backends::common's guardable-store assert)")
}

/// UpdateAndFill leg (#246): the same `Open(p)` the prefix sweep uses, then ONE
/// call over the tail instead of `svN - p` separate updates. Rust has no
/// aliasing probe (`&[f64]` and `&mut [f64]` cannot alias) and no negative
/// count (slices carry their own lengths), so the two rejections it CAN
/// reach ride here instead: a zero-length run, and an output shorter than
/// the bar count.
fn emit_rust_sv_update_and_fill_leg(
    s: &mut String,
    fname: &str,
    arrays: &[&'static str],
    pfx_ins: &str,
    opts_tail: &str,
    out_is_int: &[bool],
    out_nullable: &[bool],
) {
    s.push_str("        if let Some(&p) = pcs.first() {\n");
    let fname_snake = crate::backends::common::snake_words(fname);
    let _ = writeln!(s, "            match c2.{fname_snake}_open({pfx_ins}{opts_tail}) {{");
    s.push_str("                Err(_) => { ufill_ok = false; }\n");
    s.push_str("                Ok((mut stu, _uv0)) => {\n");
    s.push_str("                    ufill_checked = 1;\n");
    s.push_str("                    let r0 = stu.out_range();\n");
    for (i, is_int) in out_is_int.iter().enumerate() {
        let (ty, canary) = if *is_int {
            ("i32", "-987654321i32")
        } else {
            ("f64", "-1.2345678901234e300f64")
        };
        let _ = writeln!(s, "                    let mut u{i}: Vec<{ty}> = vec![{canary}; svN];");
    }
    // A nullable output takes `Option<&mut [T]>` at this tier too (rule U6a);
    // this harness compares values, so it always supplies one.
    let ubuf = |i: usize| {
        if out_nullable.get(i).copied().unwrap_or(false) {
            format!("Some(&mut u{i})")
        } else {
            format!("&mut u{i}")
        }
    };
    let uargs: String = (0..out_is_int.len()).fold(String::new(), |mut acc, i| {
        let _ = write!(acc, ", {}", ubuf(i));
        acc
    });
    let tail_ins = arrays
        .iter()
        .map(|a| format!("&{a}[p..]"))
        .collect::<Vec<_>>()
        .join(", ");
    let empty_ins = arrays
        .iter()
        .map(|a| format!("&{a}[p..p]"))
        .collect::<Vec<_>>()
        .join(", ");
    // Zero bars is a success no-op, and an output too short for the run is a
    // rejection. Neither may move the handle — checked against the range the
    // open left, not against a recomputed one.
    let _ = writeln!(
        s,
        "                    if stu.update_and_fill({empty_ins}{uargs}).is_err() {{ ufill_ok = false; }}"
    );
    {
        // The undersized buffer rides the first output that is NOT nullable.
        // Empty is how C# spells "declined", so a zero-length nullable output is
        // an accepted call there, not the U6 rejection this probe is asserting;
        // every function has at least one required output (the guardable-store
        // assert in `backends::common`), so there is always somewhere to put it.
        let short_idx = short_probe_index(out_nullable);
        let short: String = (0..out_is_int.len())
            .map(|i| if i == short_idx { format!(", &mut u{i}[..0]") } else { format!(", {}", ubuf(i)) })
            .collect();
        let _ = writeln!(
            s,
            "                    if stu.update_and_fill({tail_ins}{short}).is_ok() {{ ufill_ok = false; }}"
        );
    }
    s.push_str("                    if stu.out_range() != r0 { ufill_ok = false; }\n");
    let _ = writeln!(
        s,
        "                    match stu.update_and_fill({tail_ins}{uargs}) {{"
    );
    s.push_str("                        Err(_) => { ufill_ok = false; }\n");
    s.push_str("                        Ok(()) => {\n");
    for (i, is_int) in out_is_int.iter().enumerate() {
        if *is_int {
            let _ = writeln!(s, "                            for t in p..svN {{ if u{i}[t - p] != b{i}[t - beg] {{ ufill_ok = false; }} }}");
        } else {
            let _ = writeln!(s, "                            for t in p..svN {{ if sv_xtier_ne(u{i}[t - p], b{i}[t - beg], &mut zsign) {{ ufill_ok = false; }} }}");
        }
    }
    for (i, is_int) in out_is_int.iter().enumerate() {
        let canary = if *is_int { "-987654321i32" } else { "-1.2345678901234e300f64" };
        let _ = writeln!(s, "                            for t in (svN - p)..svN {{ if u{i}[t] != {canary} {{ ufill_ok = false; }} }}");
    }
    s.push_str("                            range_checked = 1; range_legs += 1; range_sites |= ");
    s.push_str(&sv_range_bit(SvRangeSite::UpdateFill, SV_RANGE_SITES_RUST).to_string());
    s.push_str(";\n");
    s.push_str("                            if stu.out_range().beg_idx != beg || stu.out_range().count != nb { ufill_ok = false; range_ok = false; }\n");
    s.push_str("                        }\n                    }\n");
    s.push_str("                }\n            }\n        }\n");
}

/// The whole Rust `stream_verify` section: candle-settings helpers, one
/// `sv_<name>` per function with an emitted Rust stream, and the dispatcher.
pub(crate) fn generate_rust_stream_verify(
    funcs: &[FuncDef],
    enums: &HashMap<String, EnumDef>,
) -> String {
    use std::fmt::Write as _;
    let mut s = String::new();
    s.push_str("// ---- stream_verify: Rust stream vs Rust batch, bitwise ----\n\n");
    // Cross-tier compare — see the C emitter for the rule. Differing bits that
    // compare equal are +0.0 vs -0.0 (issue #147): counted, never a mismatch.
    // The peek-vs-update spot-assert stays a strict `to_bits()` compare.
    s.push_str("fn sv_xtier_ne(a: f64, b: f64, zsign: &mut i64) -> bool {\n");
    s.push_str("    if a.to_bits() == b.to_bits() { return false; }\n");
    s.push_str("    if a == b { *zsign += 1; return false; }\n");
    s.push_str("    true\n");
    s.push_str("}\n\n");
    // Candle-settings rounds (mirror the C sweep): defaults / avgPeriod+3 /
    // avgPeriod=0 (instant candle) / rangeType=Shadows.
    s.push_str("fn sv_candle_settings(rd: i32) -> CandleSettings {\n    let mut s = CandleSettings::default_settings();\n    let all = |s: &mut CandleSettings, f: &dyn Fn(&mut CandleSetting)| {\n        for cs in [&mut s.body_long, &mut s.body_very_long, &mut s.body_short, &mut s.body_doji,\n                   &mut s.shadow_long, &mut s.shadow_very_long, &mut s.shadow_short,\n                   &mut s.shadow_very_short, &mut s.near, &mut s.far, &mut s.equal] {\n            f(cs);\n        }\n    };\n    match rd {\n        1 => all(&mut s, &|c| c.avg_period += 3),\n        2 => all(&mut s, &|c| c.avg_period = 0),\n        3 => all(&mut s, &|c| c.range_type = RangeType::Shadows),\n        _ => {}\n    }\n    s\n}\n\n");
    s.push_str("fn sv_apply_candles(b: CoreBuilder, s: &CandleSettings) -> CoreBuilder {\n    b.candle_setting(CandleSettingType::BodyLong, s.body_long)\n     .candle_setting(CandleSettingType::BodyVeryLong, s.body_very_long)\n     .candle_setting(CandleSettingType::BodyShort, s.body_short)\n     .candle_setting(CandleSettingType::BodyDoji, s.body_doji)\n     .candle_setting(CandleSettingType::ShadowLong, s.shadow_long)\n     .candle_setting(CandleSettingType::ShadowVeryLong, s.shadow_very_long)\n     .candle_setting(CandleSettingType::ShadowShort, s.shadow_short)\n     .candle_setting(CandleSettingType::ShadowVeryShort, s.shadow_very_short)\n     .candle_setting(CandleSettingType::Near, s.near)\n     .candle_setting(CandleSettingType::Far, s.far)\n     .candle_setting(CandleSettingType::Equal, s.equal)\n}\n\n");

    let lookup = crate::streaming::FuncsLookup(funcs);
    let emitted: Vec<&FuncDef> = funcs
        .iter()
        .filter(|f| crate::backends::rust_stream::emits_stream(f, &lookup))
        .collect();
    for f in &emitted {
        s.push_str(&emit_rust_sv_func(f, funcs, enums));
    }

    s.push_str("fn handle_stream_verify(core: &Core, params: &Value) -> String {\n");
    s.push_str("    let func_name = params[\"funcName\"].as_str().unwrap_or(\"\");\n");
    s.push_str("    match func_name {\n");
    for f in &emitted {
        let _ = writeln!(
            s,
            "        \"TA_{}\" => sv_{}(core, params),",
            f.name.to_uppercase(),
            f.name.to_lowercase()
        );
    }
    s.push_str("        _ => \"{\\\"error\\\":\\\"not_streamable\\\"}\".to_string(),\n    }\n}\n\n");
    s
}

// ---------------------------------------------------------------------------
// Java stream_verify (mirror of generate_rust_stream_verify for the Java
// server): per streamable function, run the Java batch and the Java stream
// trajectory in-process on identical seeded inputs (FuzzData port of
// fuzz_data.h), compare BITWISE per bar (doubleToRawLongBits), spot-assert
// peek == update AND value() == update, verify OpenAndFill against the batch
// arrays plus the aliasing rejection, drive a mid-stream copy() independence
// leg, check the Integer.MIN_VALUE default sentinel, and answer the same flat
// JSON contract the ta_regtest driver reads. Differences from C/Rust, by
// design:
// - Open rejects are exceptions: ONLY IllegalArgumentException (and its
//   InsufficientHistoryException subclass) counts as an expected reject — an
//   NPE/AIOOBE escapes and fails the run loudly instead of masquerading as
//   reject-parity.
// - An out-of-list enum request value is unrepresentable in the type-safe
//   Java surface (MAType.values()[x] would throw); the sv_ answers the
//   batch-fail reject-parity shape directly — type safety IS the rejection.
// - Settings sweeps configure a FRESH per-round Core instance (per-instance
//   settings; streams snapshot candle settings at open).
// ---------------------------------------------------------------------------

/// The per-input expanded fuzz array variable in the generated Java handler
/// (same mapping as the C/Rust servers: price components by name, generic
/// real0 -> close, real1 -> volume).
fn sv_java_input_array(name: &str, generic_idx: &mut usize) -> &'static str {
    match sv_input_suffix(name, generic_idx) {
        "o" => "fz_o",
        "h" => "fz_h",
        "l" => "fz_l",
        "c" => "fz_c",
        "v" => "fz_v",
        _ => "fz_oi",
    }
}

/// One `sv_<NAME>` verify method for a function with an emitted Java stream.
// Integer optional-param defaults are integer-valued `f64` in the IR; the
// `as i64` casts for literal emission are exact, not truncating.
#[allow(clippy::too_many_lines, clippy::cast_possible_truncation, clippy::cognitive_complexity)]
fn emit_java_sv_func(func: &FuncDef, funcs: &[FuncDef], enums: &HashMap<String, EnumDef>) -> String {
    use std::fmt::Write as _;
    let base = func.name.clone();
    let base_camel = crate::backends::common::camel_words(&func.name);
    let class = crate::backends::java_stream::stream_class_name(func);
    let candle = func.name.starts_with("CDL");
    let inputs = crate::streaming::input_array_names(func);
    let mut gi = 0usize;
    let arrays: Vec<&'static str> = inputs
        .iter()
        .map(|i| sv_java_input_array(i, &mut gi))
        .collect();
    let n_out = func.outputs.len();
    let multi = n_out > 1;
    let out_is_int: Vec<bool> = func
        .outputs
        .iter()
        .map(|o| o.param_type == crate::ir::ParamType::Integer)
        .collect();
    // The accessor CALL, not the name: `Value` is a record, so every read below
    // is `up.macd()`. Rendered once here because all ten read sites interpolate
    // this same list.
    let vfield: Vec<String> = func
        .outputs
        .iter()
        .map(|o| format!("{}()", crate::backends::java_stream::value_field_name(&o.name)))
        .collect();

    let mut s = String::new();
    let _ = writeln!(s, "    static String sv_{}(String json) {{", func.name);
    s.push_str("        int svShape = jsonInt(json, \"gen_shape\");\n");
    s.push_str("        int svSeed = jsonInt(json, \"gen_seed\");\n");
    s.push_str("        int svN = jsonInt(json, \"gen_n\");\n");
    s.push_str("        if (svN < 2) svN = 2;\n        if (svN > 256) svN = 256;\n");
    s.push_str("        int svK = jsonInt(json, \"unstablePeriod\");\n");
    s.push_str("        int svCompat = jsonInt(json, \"compatibility\");\n");
    // Compatibility is pinned to Default in the Java library (the Metastock arms
    // are constant-folded out of the generated code), so a Metastock leg would
    // silently re-run the Default one — refuse it instead of passing vacuously.
    s.push_str("        if (svCompat != 0) {\n");
    s.push_str("            return \"{\\\"error\\\":\\\"java has no compatibility API (pinned to Default)\\\"}\";\n");
    s.push_str("        }\n");
    if candle {
        s.push_str("        int candleLegs = jsonInt(json, \"candleLegs\");\n");
    }
    // Optional params with YAML defaults; enum params parse as raw ints first
    // (out-of-list detection), everything else straight to its Java type.
    let mut has_int_default = false;
    for p in &func.optional_inputs {
        let name = &p.name;
        match &p.param_type {
            crate::ir::ParamType::Real => {
                let d = p.default.unwrap_or(0.0);
                let _ = writeln!(
                    s,
                    "        double {name} = json.contains(\"\\\"{name}\\\"\") ? jsonDouble(json, \"{name}\") : {d:e};"
                );
            }
            crate::ir::ParamType::Enum(en) => {
                let d = p.default.unwrap_or(0.0) as i64;
                let _ = writeln!(
                    s,
                    "        int _raw_{name} = json.contains(\"\\\"{name}\\\"\") ? jsonInt(json, \"{name}\") : {d};"
                );
                let _ = writeln!(
                    s,
                    "        if (_raw_{name} < 0 || _raw_{name} >= {en}.values().length) {{"
                );
                s.push_str("            /* Out-of-list enum: unrepresentable in the type-safe Java surface —\n             * batch and stream both reject at the type level (reject parity). */\n");
                s.push_str("            return \"{\\\"retCode\\\":2,\\\"legs\\\":0,\\\"nb\\\":0,\\\"openRejects\\\":1,\\\"ok\\\":1,\\\"peek_ok\\\":1}\";\n");
                s.push_str("        }\n");
                let _ = writeln!(s, "        {en} {name} = {en}.values()[_raw_{name}];");
            }
            _ => {
                let d = p.default.unwrap_or(0.0) as i64;
                if p.default.is_some() {
                    has_int_default = true;
                }
                let _ = writeln!(
                    s,
                    "        int {name} = json.contains(\"\\\"{name}\\\"\") ? jsonInt(json, \"{name}\") : {d};"
                );
            }
        }
    }
    // Seeded inputs.
    s.push_str("        double[] fz_o = new double[svN];\n        double[] fz_h = new double[svN];\n        double[] fz_l = new double[svN];\n        double[] fz_c = new double[svN];\n        double[] fz_v = new double[svN];\n        double[] fz_oi = new double[svN];\n");
    s.push_str("        FuzzData.fuzzGen(svShape, svSeed, svN, fz_o, fz_h, fz_l, fz_c, fz_v, fz_oi);\n");

    // Period-bank ramp (see the C/Rust emitters): span [min-1, max+1] so every
    // bank slot and both clamp directions are exercised.
    {
        let lookup = crate::streaming::FuncsLookup(funcs);
        if let Ok(crate::streaming::StreamPlan::PeriodBank(pb)) =
            crate::streaming::validate_streamable(func, &lookup)
        {
            if let Some(idx) = inputs.iter().position(|i| *i == pb.period_input) {
                let arr = arrays[idx];
                let _ = writeln!(
                    s,
                    "        for (int _pi = 0; _pi < svN; _pi++) {{ {arr}[_pi] = {min} + (_pi % ({max} - {min} + 3)) - 1; }}",
                    min = pb.min_param,
                    max = pb.max_param
                );
            }
        }
    }

    let full_ins = arrays.join(", ");
    let pfx_ins = |p: &str| -> String {
        arrays
            .iter()
            .map(|a| format!("java.util.Arrays.copyOf({a}, {p})"))
            .collect::<Vec<_>>()
            .join(", ")
    };
    let opts = func
        .optional_inputs
        .iter()
        .map(|p| p.name.clone())
        .collect::<Vec<_>>()
        .join(", ");
    let opts_lead = if opts.is_empty() { String::new() } else { format!("{opts}, ") };
    let opts_tail = if opts.is_empty() { String::new() } else { format!(", {opts}") };
    let bar_args = |t: &str| -> String {
        arrays
            .iter()
            .map(|a| format!("{a}[{t}]"))
            .collect::<Vec<_>>()
            .join(", ")
    };
    let bars_t = bar_args("t");

    // Batch + fill output buffers.
    let mut bdecls = String::new();
    let mut bargs = String::new();
    let mut fdecls = String::new();
    let mut fargs = String::new();
    for (i, is_int) in out_is_int.iter().enumerate() {
        let ty = if *is_int { "int" } else { "double" };
        let _ = writeln!(bdecls, "        {ty}[] b{i} = new {ty}[svN];");
        let _ = write!(bargs, ", b{i}");
        // Canary-filled, not zero-filled: the slack above the produced range is
        // asserted untouched after the call (#205's write bound), so a write
        // past `nb` fails instead of landing in unread space.
        let canary = if *is_int { "-987654321" } else { "-1.2345678901234e300" };
        let _ = writeln!(fdecls, "            {ty}[] f{i} = new {ty}[svN];");
        let _ = writeln!(fdecls, "            java.util.Arrays.fill(f{i}, ({ty}){canary});");
        let _ = write!(fargs, ", f{i}");
    }
    s.push_str(&bdecls);

    s.push_str("        long legs = 0;\n        boolean allOk = true;\n        boolean peekAll = true;\n        int fillChecked = 0;\n        boolean fillOk = true;\n        MInteger beg = new MInteger();\n        MInteger nb = new MInteger();\n        String diag = \"\";\n");
    // The range leg (#241): a handle's outRange() against what batch reported
    // for the same bars. Public API in every backend, so unlike the state leg
    // this one is not C-only.
    s.push_str("        int rangeChecked = 0;\n        boolean rangeOk = true;\n        long rangeLegs = 0;\n        int rangeSites = 0;\n");
    // The n-bar filler's own leg (issue #246), reported apart from the
    // open-time fill so a regression names the entry point it is in.
    s.push_str("        int ufillChecked = 0;\n        boolean ufillOk = true;\n");
    // Benign +/-0 cases across every cross-tier compare in this request. A
    // one-element array, not a static: the server answers many requests per
    // process and a static would carry one function's count into the next.
    s.push_str("        long[] zsign = { 0 };\n");
    if candle {
        s.push_str("        int rounds = (candleLegs != 0) ? 4 : 1;\n");
    } else {
        s.push_str("        int rounds = 1;\n");
    }
    s.push_str("        for (int rd = 0; rd < rounds; rd++) {\n");

    // Fresh, pinned, configured Core for this round (per-instance settings).
    s.push_str("            Core c2 = new Core();\n");
    for id in collect_pin_ids(func, funcs, enums) {
        let _ = writeln!(s, "            c2.unstablePeriod[{id}] = svK;");
    }
    if candle {
        s.push_str("            svApplyCandleRound(c2, rd);\n");
    }

    // Expected-reject precheck (dispatch/period-bank arms without a stream) —
    // live again since #139: hma has no stream yet, so every MAType-dispatching
    // function (MA, BBANDS, APO/PPO/PVO, STOCH*, MACDEXT, MAVP) generates a
    // guard for the HMA arm. The guard compares raw enum ints, so substitute
    // C constants with values.
    if let Some(guard) = sv_reject_condition(func, funcs, None) {
        // Rewrite enum param names to their raw-int locals for the guard, and
        // C enum constants to their integer values (Java has no TA_MAType_*).
        let mut guard_java = sv_guard_enum_ints(&guard, enums);
        for p in &func.optional_inputs {
            if matches!(p.param_type, crate::ir::ParamType::Enum(_)) {
                guard_java = guard_java.replace(&p.name, &format!("_raw_{}", p.name));
            }
        }
        let _ = writeln!(s, "            if ({guard_java}) {{");
        s.push_str("                boolean r1;\n");
        let _ = writeln!(
            s,
            "                try {{ c2.{base_camel}Open({full_ins}{opts_tail}); r1 = false; }} catch (IllegalArgumentException _e) {{ r1 = true; }}"
        );
        s.push_str(&fdecls.replace("            ", "                "));
        s.push_str("                boolean r2;\n");
        let _ = writeln!(
            s,
            "                try {{ c2.{base_camel}OpenAndFill({full_ins}{opts_tail}{fargs}); r2 = false; }} catch (IllegalArgumentException _e) {{ r2 = true; }}"
        );
        s.push_str("                boolean okr = r1 && r2;\n");
        s.push_str("                return \"{\\\"retCode\\\":0,\\\"legs\\\":0,\\\"unsupportedArm\\\":1,\\\"ok\\\":\" + (okr ? 1 : 0) + \",\\\"peek_ok\\\":1}\";\n");
        s.push_str("            }\n");
    }

    // Batch leg.
    let _ = writeln!(
        s,
        "            RetCode rc;\n            try {{ rc = c2.{base}_Impl(0, svN - 1, {full_ins}, {opts_lead}beg, nb{bargs}); }}\n            catch (RuntimeException _sve) {{ if (!(_sve instanceof TaLibFailure)) throw _sve; rc = ((TaLibFailure) _sve).retCode(); beg.value = 0; nb.value = 0; }}"
    );
    let _ = writeln!(s, "            int lb = c2.{base}_Lookback({opts});");
    s.push_str("            if (rc != RetCode.Success || nb.value == 0) {\n");
    s.push_str("                boolean openRejects;\n");
    let _ = writeln!(
        s,
        "                try {{ c2.{base_camel}Open({full_ins}{opts_tail}); openRejects = false; }} catch (IllegalArgumentException _e) {{ openRejects = true; }}"
    );
    if candle {
        s.push_str("                if (!openRejects) allOk = false;\n");
        s.push_str("                if (rd + 1 < rounds) continue;\n");
        s.push_str("                return \"{\\\"retCode\\\":\" + rc.toInt() + \",\\\"legs\\\":\" + legs + \",\\\"nb\\\":\" + nb.value + \",\\\"openRejects\\\":\" + (openRejects ? 1 : 0) + \",\\\"ok\\\":\" + (allOk ? 1 : 0) + \",\\\"peek_ok\\\":\" + (peekAll ? 1 : 0) + \",\\\"benign\\\":\" + zsign[0] + \"}\";\n");
    } else {
        s.push_str("                return \"{\\\"retCode\\\":\" + rc.toInt() + \",\\\"legs\\\":0,\\\"nb\\\":\" + nb.value + \",\\\"openRejects\\\":\" + (openRejects ? 1 : 0) + \",\\\"ok\\\":\" + (openRejects ? 1 : 0) + \",\\\"peek_ok\\\":1}\";\n");
    }
    s.push_str("            }\n");

    // OpenAndFill leg (fill == batch arrays, bitwise) + aliasing probes.
    s.push_str("            fillChecked = 1;\n            try {\n");
    s.push_str(&fdecls.replace("            ", "                "));
    let _ = writeln!(
        s,
        "                Core.{class} _fh = c2.{base_camel}OpenAndFill({full_ins}{opts_tail}{fargs});"
    );
    s.push_str("                OutRange _fr = _fh.outRange();\n");
    let _ = writeln!(s, "                rangeChecked = 1; rangeLegs++; rangeSites |= {};", sv_range_bit(SvRangeSite::Fill, SV_RANGE_SITES_JAVA));
    s.push_str("                if (_fr.begIdx() != beg.value || _fr.count() != nb.value) rangeOk = false;\n");
    s.push_str("                if (_fr.begIdx() != beg.value || _fr.count() != nb.value) fillOk = false;\n                else {\n");
    for (i, is_int) in out_is_int.iter().enumerate() {
        if *is_int {
            let _ = writeln!(s, "                    for (int i = 0; i < nb.value; i++) if (f{i}[i] != b{i}[i]) fillOk = false;");
        } else {
            let _ = writeln!(s, "                    for (int i = 0; i < nb.value; i++) if (svXtierNe(f{i}[i], b{i}[i], zsign)) fillOk = false;");
        }
    }
    // Slack canary: everything above the produced range must be untouched.
    for (i, is_int) in out_is_int.iter().enumerate() {
        let ty = if *is_int { "int" } else { "double" };
        let canary = if *is_int { "-987654321" } else { "-1.2345678901234e300" };
        let _ = writeln!(s, "                    for (int i = nb.value; i < svN; i++) if (f{i}[i] != ({ty}){canary}) fillOk = false;");
    }
    s.push_str("                }\n");
    // Aliasing probes (Java arrays make out==in expressible; the guards must
    // reject with IAE and mint no handle).
    if !out_is_int[0] {
        let alias_args = {
            let mut fa = String::new();
            for (i, is_int) in out_is_int.iter().enumerate() {
                if i == 0 {
                    let _ = write!(fa, ", {}", arrays[0]);
                } else {
                    let _ = write!(fa, ", f{i}");
                }
                let _ = is_int;
            }
            fa
        };
        let _ = writeln!(
            s,
            "                try {{ c2.{base_camel}OpenAndFill({full_ins}{opts_tail}{alias_args}); fillOk = false; }} catch (IllegalArgumentException _e) {{ /* expected: output aliases input */ }}"
        );
        if multi && !out_is_int[1] {
            let alias_args2 = {
                let mut fa = String::new();
                for (i, _) in out_is_int.iter().enumerate() {
                    if i == 1 {
                        let _ = write!(fa, ", f0");
                    } else {
                        let _ = write!(fa, ", f{i}");
                    }
                }
                fa
            };
            let _ = writeln!(
                s,
                "                try {{ c2.{base_camel}OpenAndFill({full_ins}{opts_tail}{alias_args2}); fillOk = false; }} catch (IllegalArgumentException _e) {{ /* expected: output aliases output */ }}"
            );
        }
    }
    s.push_str("            } catch (IllegalArgumentException _e) { fillOk = false; }\n");

    // Prefix sweep.
    if func_has_seed_boundary(func, funcs) {
        s.push_str("            int seedShift = (svCompat == 1) ? 1 : 0;\n");
    } else {
        s.push_str("            int seedShift = 0;\n");
    }
    s.push_str("            int[] pcs = { lb + 1 + seedShift, lb + 13, svN / 2, svN - 1 };\n");
    s.push_str("            java.util.Arrays.sort(pcs);\n");
    s.push_str("            int prevP = -1;\n");
    s.push_str("            for (int pi = 0; pi < pcs.length; pi++) {\n");
    s.push_str("                int p = pcs[pi];\n");
    s.push_str("                if (p < lb + 1 + seedShift || p > svN - 1 || p == prevP) continue;\n");
    s.push_str("                prevP = p;\n");
    let _ = writeln!(s, "                Core.{class} st;");
    let _ = writeln!(
        s,
        "                try {{ st = c2.{base_camel}Open({}{opts_tail}); }}",
        pfx_ins("p")
    );
    s.push_str("                catch (IllegalArgumentException _e) { allOk = false; if (diag.isEmpty()) diag = \",\\\"openRejectP\\\":\" + p; continue; }\n");
    s.push_str("                legs++;\n");
    // Open-value compare through value() (load-bearing: Java open returns only
    // the handle, so the anchor compare IS the value() verification).
    if multi {
        let _ = writeln!(s, "                Core.{class}.Value v0 = st.value();");
        for (i, f) in vfield.iter().enumerate() {
            if out_is_int[i] {
                let _ = writeln!(s, "                if (v0.{f} != b{i}[p - 1 - beg.value]) {{ allOk = false; if (diag.isEmpty()) diag = \",\\\"badBar\\\":\" + (p - 1) + \",\\\"badOut\\\":{i},\\\"where\\\":\\\"open\\\"\"; }}");
            } else {
                let _ = writeln!(s, "                if (svXtierNe(v0.{f}, b{i}[p - 1 - beg.value], zsign)) {{ allOk = false; if (diag.isEmpty()) diag = \",\\\"badBar\\\":\" + (p - 1) + \",\\\"badOut\\\":{i},\\\"where\\\":\\\"open\\\"\"; }}");
            }
        }
    } else if out_is_int[0] {
        s.push_str("                if (st.value() != b0[p - 1 - beg.value]) { allOk = false; if (diag.isEmpty()) diag = \",\\\"badBar\\\":\" + (p - 1) + \",\\\"badOut\\\":0,\\\"where\\\":\\\"open\\\"\"; }\n");
    } else {
        s.push_str("                if (svXtierNe(st.value(), b0[p - 1 - beg.value], zsign)) { allOk = false; if (diag.isEmpty()) diag = \",\\\"badBar\\\":\" + (p - 1) + \",\\\"badOut\\\":0,\\\"where\\\":\\\"open\\\"\"; }\n");
    }
    // Update loop with peek-every-7 + value()==update.
    s.push_str("                for (int t = p; t < svN; t++) {\n");
    let (up_ty, up_decl) = if multi {
        (format!("Core.{class}.Value"), "up")
    } else if out_is_int[0] {
        ("int".to_string(), "up")
    } else {
        ("double".to_string(), "up")
    };
    s.push_str("                    if (t % 7 == 0) {\n");
    let _ = writeln!(s, "                        {up_ty} pk = st.peek({bars_t});");
    let _ = writeln!(s, "                        {up_ty} {up_decl} = st.update({bars_t});");
    if multi {
        for (i, f) in vfield.iter().enumerate() {
            if out_is_int[i] {
                let _ = writeln!(s, "                        if (pk.{f} != up.{f}) peekAll = false;");
            } else {
                let _ = writeln!(s, "                        if (svBne(pk.{f}, up.{f})) peekAll = false;");
            }
        }
        s.push_str("                        if (st.value() != up) allOk = false; /* cached Value identity */\n");
    } else if out_is_int[0] {
        s.push_str("                        if (pk != up) peekAll = false;\n");
        s.push_str("                        if (st.value() != up) allOk = false;\n");
    } else {
        s.push_str("                        if (svBne(pk, up)) peekAll = false;\n");
        s.push_str("                        if (svBne(st.value(), up)) allOk = false;\n");
    }
    let emit_up_compares = |s: &mut String, pad: &str| {
        if multi {
            for (i, f) in vfield.iter().enumerate() {
                if out_is_int[i] {
                    let _ = writeln!(s, "{pad}if (up.{f} != b{i}[t - beg.value]) {{ allOk = false; if (diag.isEmpty()) diag = \",\\\"badBar\\\":\" + t + \",\\\"badOut\\\":{i},\\\"batchv\\\":\\\"\" + b{i}[t - beg.value] + \"\\\",\\\"streamv\\\":\\\"\" + up.{f} + \"\\\"\"; }}");
                } else {
                    let _ = writeln!(s, "{pad}if (svXtierNe(up.{f}, b{i}[t - beg.value], zsign)) {{ allOk = false; if (diag.isEmpty()) diag = \",\\\"badBar\\\":\" + t + \",\\\"badOut\\\":{i},\\\"batchv\\\":\\\"\" + String.format(\"%016x\", Double.doubleToRawLongBits(b{i}[t - beg.value])) + \"\\\",\\\"streamv\\\":\\\"\" + String.format(\"%016x\", Double.doubleToRawLongBits(up.{f})) + \"\\\"\"; }}");
                }
            }
        } else if out_is_int[0] {
            let _ = writeln!(s, "{pad}if (up != b0[t - beg.value]) {{ allOk = false; if (diag.isEmpty()) diag = \",\\\"badBar\\\":\" + t + \",\\\"badOut\\\":0,\\\"batchv\\\":\\\"\" + b0[t - beg.value] + \"\\\",\\\"streamv\\\":\\\"\" + up + \"\\\"\"; }}");
        } else {
            let _ = writeln!(s, "{pad}if (svXtierNe(up, b0[t - beg.value], zsign)) {{ allOk = false; if (diag.isEmpty()) diag = \",\\\"badBar\\\":\" + t + \",\\\"badOut\\\":0,\\\"batchv\\\":\\\"\" + String.format(\"%016x\", Double.doubleToRawLongBits(b0[t - beg.value])) + \"\\\",\\\"streamv\\\":\\\"\" + String.format(\"%016x\", Double.doubleToRawLongBits(up)) + \"\\\"\"; }}");
        }
    };
    emit_up_compares(&mut s, "                        ");
    s.push_str("                    } else {\n");
    let _ = writeln!(s, "                        {up_ty} {up_decl} = st.update({bars_t});");
    emit_up_compares(&mut s, "                        ");
    s.push_str("                    }\n");
    s.push_str("                }\n");
    // Open(p) + (svN - p) updates: whatever p was, the handle has consumed svN
    // bars and must report exactly what batch(0, svN-1) did. Only when the value
    // leg passed — otherwise the handle is short of the bars it was to consume.
    s.push_str("                if (allOk) {\n");
    let _ = writeln!(s, "                    rangeChecked = 1; rangeLegs++; rangeSites |= {};", sv_range_bit(SvRangeSite::Prefix, SV_RANGE_SITES_JAVA));
    s.push_str("                    if (st.outRange().begIdx() != beg.value || st.outRange().count() != nb.value) rangeOk = false;\n");
    s.push_str("                }\n");
    s.push_str("            }\n");

    // UpdateAndFill leg (#246): the earliest prefix open, then ONE call over
    // the tail instead of `svN - p` separate updates.
    //
    // Three probes ride on the same handle because each leaves it untouched:
    // an output shorter than the run, an output that IS an input (two Java
    // arrays are identical or disjoint, so reference equality is the whole
    // guard), and a zero-bar call, which is a success that changes nothing.
    s.push_str("            {\n");
    s.push_str("                int p = lb + 1 + seedShift;\n");
    s.push_str("                if (p <= svN - 1) {\n");
    s.push_str("                    ufillChecked = 1;\n");
    s.push_str("                    try {\n");
    let _ = writeln!(
        s,
        "                        Core.{class} stu = c2.{base_camel}Open({}{opts_tail});",
        pfx_ins("p")
    );
    s.push_str("                        OutRange ur0 = stu.outRange();\n");
    for (i, is_int) in out_is_int.iter().enumerate() {
        let ty = if *is_int { "int" } else { "double" };
        let canary = if *is_int { "-987654321" } else { "-1.2345678901234e300" };
        let _ = writeln!(s, "                        {ty}[] u{i} = new {ty}[svN];");
        let _ = writeln!(s, "                        java.util.Arrays.fill(u{i}, ({ty}){canary});");
    }
    for a in &arrays {
        let _ = writeln!(
            s,
            "                        double[] tail_{a} = java.util.Arrays.copyOfRange({a}, p, svN);"
        );
    }
    let tail_ins: String = arrays
        .iter()
        .map(|a| format!("tail_{a}"))
        .collect::<Vec<_>>()
        .join(", ");
    let empty_ins: String = arrays
        .iter()
        .map(|_| "new double[0]".to_string())
        .collect::<Vec<_>>()
        .join(", ");
    let uargs: String = (0..out_is_int.len()).fold(String::new(), |mut acc, i| {
        let _ = write!(acc, ", u{i}");
        acc
    });
    let _ = writeln!(
        s,
        "                        stu.updateAndFill({empty_ins}{uargs});"
    );
    {
        let short_idx =
            short_probe_index(&func.outputs.iter().map(crate::ir::Output::is_nullable).collect::<Vec<_>>());
        let short: String = out_is_int
            .iter()
            .enumerate()
            .map(|(i, is_int)| {
                if i == short_idx {
                    format!(", new {}[0]", if *is_int { "int" } else { "double" })
                } else {
                    format!(", u{i}")
                }
            })
            .collect();
        let _ = writeln!(
            s,
            "                        try {{ stu.updateAndFill({tail_ins}{short}); ufillOk = false; }} catch (IllegalArgumentException _e) {{ /* expected: output shorter than the run */ }}"
        );
    }
    if !out_is_int[0] {
        let alias: String = (0..out_is_int.len())
            .map(|i| if i == 0 { format!(", tail_{}", arrays[0]) } else { format!(", u{i}") })
            .collect();
        let _ = writeln!(
            s,
            "                        try {{ stu.updateAndFill({tail_ins}{alias}); ufillOk = false; }} catch (IllegalArgumentException _e) {{ /* expected: output aliases input */ }}"
        );
    }
    s.push_str("                        if (stu.outRange().begIdx() != ur0.begIdx() || stu.outRange().count() != ur0.count()) ufillOk = false;\n");
    let _ = writeln!(s, "                        stu.updateAndFill({tail_ins}{uargs});");
    for (i, is_int) in out_is_int.iter().enumerate() {
        if *is_int {
            let _ = writeln!(s, "                        for (int t = p; t < svN; t++) if (u{i}[t - p] != b{i}[t - beg.value]) ufillOk = false;");
        } else {
            let _ = writeln!(s, "                        for (int t = p; t < svN; t++) if (svXtierNe(u{i}[t - p], b{i}[t - beg.value], zsign)) ufillOk = false;");
        }
    }
    for (i, is_int) in out_is_int.iter().enumerate() {
        let ty = if *is_int { "int" } else { "double" };
        let canary = if *is_int { "-987654321" } else { "-1.2345678901234e300" };
        let _ = writeln!(s, "                        for (int t = svN - p; t < svN; t++) if (u{i}[t] != ({ty}){canary}) ufillOk = false;");
    }
    let _ = writeln!(
        s,
        "                        rangeChecked = 1; rangeLegs++; rangeSites |= {};",
        sv_range_bit(SvRangeSite::UpdateFill, SV_RANGE_SITES_JAVA)
    );
    s.push_str("                        if (stu.outRange().begIdx() != beg.value || stu.outRange().count() != nb.value) { ufillOk = false; rangeOk = false; }\n");
    s.push_str("                    } catch (IllegalArgumentException _e) { ufillOk = false; }\n");
    s.push_str("                }\n");
    s.push_str("            }\n");

    // copy() independence leg: open at the earliest prefix, advance to mid,
    // copy, drive both to the end — both must match batch bitwise (a shallow
    // sub-handle/bank/ring copy diverges here).
    s.push_str("            {\n");
    s.push_str("                int p0 = lb + 1 + seedShift;\n");
    s.push_str("                if (p0 <= svN - 1) {\n");
    s.push_str("                    try {\n");
    let _ = writeln!(
        s,
        "                        Core.{class} sA = c2.{base_camel}Open({}{opts_tail});",
        pfx_ins("p0")
    );
    s.push_str("                        int mid = (p0 + svN) / 2;\n");
    let _ = writeln!(s, "                        for (int t = p0; t < mid; t++) sA.update({bars_t});");
    let _ = writeln!(s, "                        Core.{class} sB = sA.copy();");
    s.push_str("                        for (int t = mid; t < svN; t++) {\n");
    let _ = writeln!(s, "                            {up_ty} uA = sA.update({bars_t});");
    let _ = writeln!(s, "                            {up_ty} uB = sB.update({bars_t});");
    if multi {
        for (i, f) in vfield.iter().enumerate() {
            if out_is_int[i] {
                let _ = writeln!(s, "                            if (uA.{f} != uB.{f} || uA.{f} != b{i}[t - beg.value]) {{ allOk = false; if (diag.isEmpty()) diag = \",\\\"copyDiverged\\\":\" + t; }}");
            } else {
                let _ = writeln!(s, "                            if (svBne(uA.{f}, uB.{f}) || svXtierNe(uA.{f}, b{i}[t - beg.value], zsign)) {{ allOk = false; if (diag.isEmpty()) diag = \",\\\"copyDiverged\\\":\" + t; }}");
            }
        }
    } else if out_is_int[0] {
        s.push_str("                            if (uA != uB || uA != b0[t - beg.value]) { allOk = false; if (diag.isEmpty()) diag = \",\\\"copyDiverged\\\":\" + t; }\n");
    } else {
        s.push_str("                            if (svBne(uA, uB) || svXtierNe(uA, b0[t - beg.value], zsign)) { allOk = false; if (diag.isEmpty()) diag = \",\\\"copyDiverged\\\":\" + t; }\n");
    }
    s.push_str("                        }\n");
    s.push_str("                    } catch (IllegalArgumentException _e) { allOk = false; if (diag.isEmpty()) diag = \",\\\"copyOpenReject\\\":1\"; }\n");
    s.push_str("                }\n");
    s.push_str("            }\n");

    // Short-history reject leg: at `lb` bars no output is defined for ANY
    // configuration, so open must reject (with the typed exception).
    s.push_str("            if (lb >= 1 && lb < svN) {\n");
    let _ = writeln!(
        s,
        "                try {{ c2.{base_camel}Open({}{opts_tail}); allOk = false; if (diag.isEmpty()) diag = \",\\\"shortHistoryAccepted\\\":1\"; }}",
        pfx_ins("lb")
    );
    s.push_str("                catch (InsufficientHistoryException _e) { /* expected, typed */ }\n");
    s.push_str("                catch (IllegalArgumentException _e) { allOk = false; if (diag.isEmpty()) diag = \",\\\"shortHistoryWrongType\\\":1\"; }\n");
    s.push_str("            }\n");

    // Integer.MIN_VALUE default-sentinel leg: open(MIN_VALUE) must equal
    // open(explicit YAML default) bitwise (the batch guard transcribes into
    // the stream open, so defaulting can never silently diverge).
    if has_int_default {
        let mut sent_args: Vec<String> = Vec::new();
        let mut expl_args: Vec<String> = Vec::new();
        for p in &func.optional_inputs {
            match &p.param_type {
                crate::ir::ParamType::Integer if p.default.is_some() => {
                    let d = p.default.unwrap_or(0.0) as i64;
                    sent_args.push("Integer.MIN_VALUE".to_string());
                    expl_args.push(format!("{d}"));
                }
                _ => {
                    sent_args.push(p.name.clone());
                    expl_args.push(p.name.clone());
                }
            }
        }
        s.push_str("            try {\n");
        let _ = writeln!(
            s,
            "                Core.{class} sD = c2.{base_camel}Open({full_ins}, {});",
            sent_args.join(", ")
        );
        let _ = writeln!(
            s,
            "                Core.{class} sE = c2.{base_camel}Open({full_ins}, {});",
            expl_args.join(", ")
        );
        if multi {
            for (i, f) in vfield.iter().enumerate() {
                if out_is_int[i] {
                    let _ = writeln!(s, "                if (sD.value().{f} != sE.value().{f}) {{ allOk = false; if (diag.isEmpty()) diag = \",\\\"minValueDefault\\\":1\"; }}");
                } else {
                    let _ = writeln!(s, "                if (svBne(sD.value().{f}, sE.value().{f})) {{ allOk = false; if (diag.isEmpty()) diag = \",\\\"minValueDefault\\\":1\"; }}");
                }
            }
        } else if out_is_int[0] {
            s.push_str("                if (sD.value() != sE.value()) { allOk = false; if (diag.isEmpty()) diag = \",\\\"minValueDefault\\\":1\"; }\n");
        } else {
            s.push_str("                if (svBne(sD.value(), sE.value())) { allOk = false; if (diag.isEmpty()) diag = \",\\\"minValueDefault\\\":1\"; }\n");
        }
        s.push_str("            } catch (IllegalArgumentException _e) { /* defaults need more history than svN — skip */ }\n");
    }

    // startIdx-anchored range site (#241). `_OpenInternal` is the composition
    // seam, and its range is max(startIdx, lookback) — a DIFFERENT expression
    // from the two sites above, resolved by a different emitter branch. It was
    // gated in C alone, which is how a clamp landed in the three managed
    // backends without its history re-check and shipped: nothing here executed
    // it. Same shape as the C leg, against a reference recomputed for the
    // anchored range under this request's own settings.
    s.push_str("            {\n");
    let anchored_bit = sv_range_bit(SvRangeSite::Anchored, SV_RANGE_SITES_JAVA);
    s.push_str("                int Sidx = lb + (svN - lb) / 3;\n");
    s.push_str("                if (Sidx > lb && Sidx < svN - 1) {\n");
    s.push_str("                    MInteger begS = new MInteger();\n");
    s.push_str("                    MInteger nbS = new MInteger();\n");
    s.push_str("                    RetCode rcS;\n");
    let _ = writeln!(
        s,
        "                    try {{ rcS = c2.{base}_Impl(Sidx, svN - 1, {full_ins}, {opts_lead}begS, nbS{bargs}); }}\n\
         \x20                   catch (RuntimeException _sve) {{ if (!(_sve instanceof TaLibFailure)) throw _sve; rcS = ((TaLibFailure) _sve).retCode(); }}"
    );
    s.push_str("                    if (rcS == RetCode.Success && nbS.value > 0) {\n");
    let _ = writeln!(
        s,
        "                        try {{\n\
         \x20                           Core.{class} stA = c2.{base_camel}OpenInternal({}, Sidx{opts_tail});\n\
         \x20                           rangeChecked = 1; rangeLegs++; rangeSites |= {anchored_bit};\n\
         \x20                           if (stA.outRange().begIdx() != begS.value || stA.outRange().count() != nbS.value) rangeOk = false;\n\
         \x20                       }} catch (IllegalArgumentException _e) {{ rangeOk = false; if (diag.isEmpty()) diag = \",\\\"anchoredOpenRejected\\\":1\"; }}",
        pfx_ins("svN")
    );
    s.push_str("                    }\n");
    s.push_str("                }\n");

    s.push_str("            }\n");

    s.push_str("        }\n");
    // fill_ok folds into ok as a safety net (mirrors the C/Rust gates).

    s.push_str("        return \"{\\\"retCode\\\":0,\\\"beg\\\":\" + beg.value + \",\\\"nb\\\":\" + nb.value + \",\\\"legs\\\":\" + legs + \",\\\"fill_checked\\\":\" + fillChecked + \",\\\"fill_ok\\\":\" + (fillOk ? 1 : 0) + \",\\\"ufill_checked\\\":\" + ufillChecked + \",\\\"ufill_ok\\\":\" + (ufillOk ? 1 : 0) + \",\\\"range_checked\\\":\" + rangeChecked + \",\\\"range_legs\\\":\" + rangeLegs + \",\\\"range_sites\\\":\" + rangeSites + \",\\\"range_sites_n\\\":"); s.push_str(&SV_RANGE_SITES_JAVA.to_string()); s.push_str(",\\\"range_ok\\\":\" + (rangeOk ? 1 : 0) + \",\\\"ok\\\":\" + ((allOk && fillOk && ufillOk && rangeOk) ? 1 : 0) + \",\\\"peek_ok\\\":\" + (peekAll ? 1 : 0) + \",\\\"benign\\\":\" + zsign[0] + diag + \"}\";\n");
    s.push_str("    }\n\n");
    s
}

/// The whole Java `stream_verify` section: bit-compare + candle-round helpers,
/// one `sv_<NAME>` per function with an emitted Java stream, and the
/// dispatcher (unknown names — including TA_STREAM_PROBE — answer
/// "not_streamable", the driver's capability probe contract).
pub(crate) fn generate_java_stream_verify(
    funcs: &[FuncDef],
    enums: &HashMap<String, EnumDef>,
) -> String {
    use std::fmt::Write as _;
    let mut s = String::new();
    s.push_str("    // ---- stream_verify: Java stream vs Java batch, bitwise ----\n\n");
    s.push_str("    static boolean svBne(double a, double b) {\n        return Double.doubleToRawLongBits(a) != Double.doubleToRawLongBits(b);\n    }\n\n");
    // Cross-tier compare — see the C emitter for the rule. Differing bits that
    // compare equal are +0.0 vs -0.0 (issue #147): counted, never a mismatch.
    // peek/value()/copy-vs-copy stay on svBne — one code path, no licence to differ.
    s.push_str("    static boolean svXtierNe(double a, double b, long[] zsign) {\n");
    s.push_str("        if (!svBne(a, b)) return false;\n");
    s.push_str("        if (a == b) { zsign[0]++; return false; }\n");
    s.push_str("        return true;\n");
    s.push_str("    }\n\n");
    // Candle-settings rounds (mirror the C/Rust sweep): defaults / avgPeriod+3
    // / avgPeriod=0 (instant candle) / rangeType=Shadows.
    // REPLACES each slot rather than mutating the CandleSetting in it. `new Core()`
    // shallow-clones DEFAULT_CANDLE_SETTINGS, so every Core's slot i is the SAME
    // object as the default's slot i; writing `cs.avgPeriod += 3` there would edit
    // the defaults themselves, and round 1 of the first candlestick would leave
    // every later Core -- and every later round, and restore_candle_default_settings
    // -- reading +3. CandleSetting's fields are final so that spelling does not
    // compile (#215).
    s.push_str("    static void svApplyCandleRound(Core c, int rd) {\n");
    s.push_str("        if (rd == 0) return;\n");
    s.push_str("        for (int i = 0; i < c.candleSettings.length; i++) {\n");
    s.push_str("            CandleSetting cs = c.candleSettings[i];\n");
    s.push_str("            if (rd == 1) {\n");
    s.push_str("                c.candleSettings[i] =\n");
    s.push_str("                    new CandleSetting(cs.rangeType, cs.avgPeriod + 3, cs.factor);\n");
    s.push_str("            } else if (rd == 2) {\n");
    s.push_str("                c.candleSettings[i] = new CandleSetting(cs.rangeType, 0, cs.factor);\n");
    s.push_str("            } else if (rd == 3) {\n");
    s.push_str("                c.candleSettings[i] =\n");
    s.push_str("                    new CandleSetting(RangeType.Shadows, cs.avgPeriod, cs.factor);\n");
    s.push_str("            }\n");
    s.push_str("        }\n");
    s.push_str("    }\n\n");

    let lookup = crate::streaming::FuncsLookup(funcs);
    let emitted: Vec<&FuncDef> = funcs
        .iter()
        .filter(|f| crate::backends::java_stream::emits_stream(f, &lookup))
        .collect();
    for f in &emitted {
        s.push_str(&emit_java_sv_func(f, funcs, enums));
    }

    // fuzz_in_hash — the same input-port self-check the Rust server answers
    // (issue #113): proves the FuzzData port reproduces C's fuzz_gen bytes.
    // The stream pass probes it so the port can never silently rot.
    s.push_str("    static String handle_fuzz_in_hash(String json) {\n");
    s.push_str("        int shape = jsonInt(json, \"gen_shape\");\n");
    s.push_str("        int seed = jsonInt(json, \"gen_seed\");\n");
    s.push_str("        int n = jsonInt(json, \"gen_n\");\n");
    s.push_str("        if (n < 1) n = 1;\n");
    s.push_str("        if (n > MAX_ARRAY_SIZE) n = MAX_ARRAY_SIZE;\n");
    s.push_str("        double[] fo = new double[n]; double[] fh = new double[n]; double[] fl = new double[n];\n");
    s.push_str("        double[] fc = new double[n]; double[] fv = new double[n]; double[] foi = new double[n];\n");
    s.push_str("        FuzzData.fuzzGen(shape, seed, n, fo, fh, fl, fc, fv, foi);\n");
    s.push_str("        long hh = svHashInit();\n");
    s.push_str("        hh = svHashF64(hh, fo, n);\n");
    s.push_str("        hh = svHashF64(hh, fh, n);\n");
    s.push_str("        hh = svHashF64(hh, fl, n);\n");
    s.push_str("        hh = svHashF64(hh, fc, n);\n");
    s.push_str("        hh = svHashF64(hh, fv, n);\n");
    s.push_str("        hh = svHashF64(hh, foi, n);\n");
    s.push_str("        hh = svHashFin(hh);\n");
    s.push_str("        return \"{\\\"in_hash\\\":\\\"\" + String.format(\"%016x\", hh) + \"\\\"}\";\n");
    s.push_str("    }\n\n");

    s.push_str("    static String handle_stream_verify(String json) {\n");
    s.push_str("        String fn = jsonString(json, \"funcName\");\n");
    s.push_str("        switch (fn) {\n");
    for f in &emitted {
        let _ = writeln!(
            s,
            "        case \"TA_{}\": return sv_{}(json);",
            f.name.to_uppercase(),
            f.name
        );
    }
    s.push_str("        default: return \"{\\\"error\\\":\\\"not_streamable\\\"}\";\n");
    s.push_str("        }\n");
    s.push_str("    }\n\n");
    s
}

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

/// The per-input expanded fuzz array variable in the generated C# handler.
///
/// Same mapping as the C, Rust and Java twins -- price components to their
/// OHLCV series, generic reals to close then volume -- routed through the shared
/// [`sv_input_suffix`] so it cannot drift from what the driver seeds.
fn sv_csharp_input_array(name: &str, generic_idx: &mut usize) -> &'static str {
    match sv_input_suffix(name, generic_idx) {
        "o" => "fz_o",
        "h" => "fz_h",
        "l" => "fz_l",
        "c" => "fz_c",
        "v" => "fz_v",
        _ => "fz_oi",
    }
}

/// One `Sv_<NAME>` verify method for a function with an emitted C# stream.
// Integer optional-param defaults are integer-valued `f64` in the IR; the
// `as i64` casts for literal emission are exact, not truncating.
#[allow(clippy::too_many_lines, clippy::cast_possible_truncation, clippy::cognitive_complexity)]
fn emit_csharp_sv_func(
    func: &FuncDef,
    funcs: &[FuncDef],
    enums: &HashMap<String, EnumDef>,
) -> String {
    use std::fmt::Write as _;
    let base = func.name.clone();
    let base_pascal = crate::backends::common::pascal_words(&func.name);
    let class = crate::backends::csharp_stream::stream_class_name(func);
    let valty = crate::backends::csharp_stream::value_type_name(func);
    let candle = func.name.starts_with("CDL");
    let inputs = crate::streaming::input_array_names(func);
    let mut gi = 0usize;
    let arrays: Vec<&'static str> = inputs
        .iter()
        .map(|i| sv_csharp_input_array(i, &mut gi))
        .collect();
    let n_out = func.outputs.len();
    let multi = n_out > 1;
    let out_is_int: Vec<bool> = func
        .outputs
        .iter()
        .map(|o| o.param_type == crate::ir::ParamType::Integer)
        .collect();
    // `<NAME>_Value` member names: the output name with a leading `out` stripped,
    // PascalCase kept (`outSlowK` -> `SlowK`, `outMinIdx` -> `MinIdx`). Unlike
    // Java these are PROPERTIES on a readonly record struct, not accessor calls,
    // so no `()` and no null concerns.
    let vmem: Vec<String> = func
        .outputs
        .iter()
        .map(|o| crate::backends::csharp_stream::value_member_name(&o.name))
        .collect();

    // Read output `i` off an Update/Peek/Value expression. A single-output
    // function returns the scalar itself, so the read IS the expression.
    let rd_out = |v: &str, i: usize| -> String {
        if multi {
            format!("{v}.{}", vmem[i])
        } else {
            v.to_string()
        }
    };

    // RULE 1 -- THE COMPARATOR IS SELECTED PER OUTPUT TYPE, NOT PER LEG.
    //
    // INTEGER outputs compare with plain `!=` on BOTH tiers. Never cast one to
    // double: MINMAXINDEX's `<NAME>_Value` members are `int`,
    // `BitConverter.DoubleToInt64Bits` does not accept one, and a `(double)`
    // cast would compile and silently weaken a strict leg into a numeric one.
    //
    // REAL outputs pick by tier. SAME-TIER legs -- peek vs update, Value vs
    // update, cloneA vs cloneB, the int.MinValue sentinel pair -- run ONE code
    // path twice and have no licence to differ in a single bit, so they use the
    // strict `SvBne`. CROSS-TIER legs -- stream vs batch, fill vs batch -- reach
    // a zero by different but equally correct routes, so they use the
    // +/-0-tolerant `SvXtierNe`, which counts those cases as benign (#147).
    //
    // And: NEVER `==` / `Equals` on a `<NAME>_Value`. Record-struct equality
    // says `+0.0 == -0.0` and `NaN == NaN`, which makes every strict leg
    // vacuous. Java gets away with a reference-identity check on its cached
    // `Value` object; a returned record struct is COPIED into the caller's
    // frame, so there is no identity to check and C# must compare per
    // component. That is what `rd_out` above exists for.
    let same_tier_ne = |a: &str, b: &str, i: usize| -> String {
        if out_is_int[i] {
            format!("{a} != {b}")
        } else {
            format!("SvBne({a}, {b})")
        }
    };
    let xtier_ne = |a: &str, b: &str, i: usize, z: &str| -> String {
        if out_is_int[i] {
            format!("{a} != {b}")
        } else {
            format!("SvXtierNe({a}, {b}, ref {z})")
        }
    };
    // Diagnostic spelling: reals as their raw IEEE bits (a decimal rendering of
    // a 1-ULP miss is unreadable), ints as themselves.
    let diag_val = |e: &str, i: usize| -> String {
        if out_is_int[i] {
            e.to_string()
        } else {
            format!("BitConverter.DoubleToInt64Bits({e}).ToString(\"x16\")")
        }
    };

    let mut s = String::new();
    // The JsonElement parameter is deliberately NOT named `p`: the prefix sweep
    // below declares a local `int p`, and C# -- unlike Java -- rejects a local
    // that shadows an enclosing parameter (CS0136).
    let _ = writeln!(s, "    static string Sv_{}(JsonElement req) {{", func.name);
    s.push_str("        int svShape = GetInt(req, \"gen_shape\", 0);\n");
    s.push_str("        int svSeed = GetInt(req, \"gen_seed\", 0);\n");
    s.push_str("        int svN = GetInt(req, \"gen_n\", 0);\n");
    s.push_str("        if (svN < 2) svN = 2;\n        if (svN > 256) svN = 256;\n");
    s.push_str("        int svK = GetInt(req, \"unstablePeriod\", 0);\n");
    s.push_str("        int svCompat = GetInt(req, \"compatibility\", 0);\n");
    // RULE 6 -- compatibility != 0 is EXPLICITLY REFUSED, never a silent Default
    // re-run. The C# library has no compatibility selector (the Metastock arms
    // are constant-folded out of the generated code, and `COMPATIBILITY()`
    // panics the C# renderer), so a Metastock leg would re-run the Default one
    // and report a pass for a mode nothing executed.
    s.push_str("        if (svCompat != 0) {\n");
    s.push_str("            return \"{\\\"error\\\":\\\"csharp has no compatibility API (pinned to Default)\\\"}\";\n");
    s.push_str("        }\n");
    if candle {
        s.push_str("        int candleLegs = GetInt(req, \"candleLegs\", 0);\n");
    }

    // Optional params. `GetInt`/`GetDouble` already fall back to the YAML
    // default when the key is absent, so C# needs none of Java's
    // `json.contains("\"name\"")` dance. Enum params are decoded TWICE: as a raw
    // int (for R5's out-of-list probe and for `sv_reject_condition`'s guard,
    // which compares raw enum ints) and as the typed enum the API takes.
    let mut has_int_default = false;
    let mut enum_param_names: Vec<String> = Vec::new();
    for p in &func.optional_inputs {
        let name = &p.name;
        match &p.param_type {
            crate::ir::ParamType::Real => {
                let d = p.default.unwrap_or(0.0);
                // `{d:e}` renders `0.3` as `3e-1`, `2.0` as `2e0`, `-4e37`
                // verbatim -- all valid C# real literals, and the same spelling
                // the Java emitter uses.
                let _ = writeln!(
                    s,
                    "        double {name} = GetDouble(req, \"{name}\", {d:e});"
                );
            }
            crate::ir::ParamType::Enum(en) => {
                let d = p.default.unwrap_or(0.0) as i64;
                let _ = writeln!(s, "        int _raw_{name} = GetInt(req, \"{name}\", {d});");
                let _ = writeln!(s, "        {en} {name} = ({en})_raw_{name};");
                enum_param_names.push(name.clone());
            }
            _ => {
                let d = p.default.unwrap_or(0.0) as i64;
                if p.default.is_some() {
                    has_int_default = true;
                }
                let _ = writeln!(s, "        int {name} = GetInt(req, \"{name}\", {d});");
            }
        }
    }

    // Seeded inputs.
    s.push_str("        double[] fz_o = new double[svN];\n        double[] fz_h = new double[svN];\n        double[] fz_l = new double[svN];\n        double[] fz_c = new double[svN];\n        double[] fz_v = new double[svN];\n        double[] fz_oi = new double[svN];\n");
    // INTEGRATION NOTE 1: `FuzzData.FuzzGen` is the C# port of `fuzz_data.h`
    // that S2 adds as `templates/csharp/FuzzData.cs`. It does not exist yet, so
    // the class name, namespace (global, mirroring the Java port's default
    // package) and argument order are ASSUMED to mirror the Java template's
    // `FuzzData.fuzzGen(shape, seed, n, o, h, l, c, v, oi)`. Reconcile when the
    // template lands -- `HandleFuzzInHash` below makes the same call.
    s.push_str("        FuzzData.FuzzGen(svShape, svSeed, svN, fz_o, fz_h, fz_l, fz_c, fz_v, fz_oi);\n");

    // Period-bank ramp (see the C/Rust/Java emitters): the fuzz period-selector
    // series would clamp to `maxPeriod` at every bar, so every bank slot but one
    // would be vacuous. Span [min-1, max+1], fed identically to both arms.
    {
        let lookup = crate::streaming::FuncsLookup(funcs);
        if let Ok(crate::streaming::StreamPlan::PeriodBank(pb)) =
            crate::streaming::validate_streamable(func, &lookup)
        {
            if let Some(idx) = inputs.iter().position(|i| *i == pb.period_input) {
                let arr = arrays[idx];
                let _ = writeln!(
                    s,
                    "        for (int _pi = 0; _pi < svN; _pi++) {{ {arr}[_pi] = {min} + (_pi % ({max} - {min} + 3)) - 1; }}",
                    min = pb.min_param,
                    max = pb.max_param
                );
            }
        }
    }

    let full_ins = arrays.join(", ");
    // C# range slicing on an array (`a[..p]`) lowers to
    // `RuntimeHelpers.GetSubArray` and returns a FRESH `double[]` -- Java's
    // `Arrays.copyOf`, spelled shorter. Fresh is what the aliasing guards need:
    // two prefix opens never share a buffer by accident.
    let pfx_ins = |p: &str| -> String {
        arrays
            .iter()
            .map(|a| format!("{a}[..{p}]"))
            .collect::<Vec<_>>()
            .join(", ")
    };
    let opts = func
        .optional_inputs
        .iter()
        .map(|p| p.name.clone())
        .collect::<Vec<_>>()
        .join(", ");
    let opts_lead = if opts.is_empty() { String::new() } else { format!("{opts}, ") };
    let opts_tail = if opts.is_empty() { String::new() } else { format!(", {opts}") };
    let bar_args = |t: &str| -> String {
        arrays
            .iter()
            .map(|a| format!("{a}[{t}]"))
            .collect::<Vec<_>>()
            .join(", ")
    };
    let bars_t = bar_args("t");

    // Batch, fill and mutated-batch output buffers.
    let mut bdecls = String::new();
    let mut bargs = String::new();
    let mut fdecls = String::new();
    let mut fargs = String::new();
    let mut mdecls = String::new();
    let mut margs = String::new();
    for (i, is_int) in out_is_int.iter().enumerate() {
        let ty = if *is_int { "int" } else { "double" };
        let _ = writeln!(bdecls, "        {ty}[] b{i} = new {ty}[svN];");
        let _ = write!(bargs, ", b{i}");
        // Canary-filled, not zero-filled: the slack above the produced range is
        // asserted untouched after the call (#205's write bound), so a write
        // past `nb` fails instead of landing in unread space.
        let canary = if *is_int { "-987654321" } else { "-1.2345678901234e300" };
        let _ = writeln!(fdecls, "            {ty}[] f{i} = new {ty}[svN];");
        let _ = writeln!(fdecls, "            Array.Fill(f{i}, ({ty}){canary});");
        let _ = write!(fargs, ", f{i}");
        let _ = writeln!(mdecls, "                    {ty}[] m{i} = new {ty}[svN];");
        let _ = write!(margs, ", m{i}");
    }
    s.push_str(&bdecls);

    s.push_str("        long legs = 0;\n        bool allOk = true;\n        bool peekAll = true;\n        int fillChecked = 0;\n        bool fillOk = true;\n        int beg = 0, nb = 0;\n        string diag = \"\";\n");
    // The range leg (#241): a handle's OutRange against what batch reported for
    // the same bars. Public API in every backend, so unlike the state leg this
    // one is not C-only.
    s.push_str("        int rangeChecked = 0;\n        bool rangeOk = true;\n        long rangeLegs = 0;\n        int rangeSites = 0;\n");
    // The n-bar filler's own leg (issue #246), reported apart from the
    // open-time fill so a regression names the entry point it is in.
    s.push_str("        int ufillChecked = 0;\n        bool ufillOk = true;\n");
    // RULE 7 -- the benign +/-0 accumulator is a REQUEST-SCOPED LOCAL, passed by
    // `ref`. One process answers many requests and a `static` would carry one
    // function's count into the next -- and the plan's Java<->C# `benign`
    // equality check would then compare a running total against a per-request
    // one. (Java passes a one-element array only because Java has no `ref`.)
    s.push_str("        long zsign = 0;\n");
    // R4's result, reported as `updAlloc`; max over the rounds.
    s.push_str("        long updAlloc = 0;\n");
    if candle {
        // R3 bookkeeping. `zsignMut` is SEPARATE from `zsign` on purpose: the
        // mid-stream leg is a C#-only leg, and folding its benign cases into
        // `zsign` would make the reported count differ from Java's by
        // construction, breaking the cross-language equality check.
        s.push_str("        int candleMutRan = 0;\n        int candleMutMoved = 0;\n        long zsignMut = 0;\n");
    }

    // RULE 5 -- OUT-OF-LIST ENUM NEEDS A REAL REJECT CHECK.
    //
    // Java short-circuits here with "type safety IS the rejection": a value
    // outside the enum's list cannot be built, so batch and stream both reject
    // at the type level and the leg is a constant. Rust does the same through
    // `TryFrom`. Neither ports. A C# enum is int-backed and open, so
    // `(MAType)int.MinValue` is a perfectly representable value that the
    // driver's `maxList + 91` vector delivers intact to the library. The
    // rejection has to be OBSERVED, the way the C gate observes it.
    //
    // BOTH openers are probed, not just `Open`: `OpenAndFill` validates through
    // its own wrapper and has its own reject path.
    //
    // `c0` is a plain default Core on purpose -- a parameter-domain rejection
    // does not depend on unstable periods or candle settings, and the round
    // loop's `c2` does not exist yet at this point.
    if !enum_param_names.is_empty() {
        let cond = enum_param_names
            .iter()
            .map(|n| format!("!Enum.IsDefined({n})"))
            .collect::<Vec<_>>()
            .join(" || ");
        s.push_str("        Core c0 = new Core();\n");
        // `Enum.IsDefined<TEnum>(TEnum)` (the generic overload) is inferred from
        // the argument -- non-boxing and trim/AOT-safe, unlike the legacy
        // `Enum.IsDefined(typeof(T), (object)v)`.
        let _ = writeln!(s, "        if ({cond}) {{");
        s.push_str("            bool eOpen, eFill;\n");
        let _ = writeln!(
            s,
            "            try {{ _ = c0.{base_pascal}Open({full_ins}{opts_tail}); eOpen = false; }}"
        );
        s.push_str("            catch (ArgumentException) { eOpen = true; }\n");
        s.push_str(&fdecls);
        let _ = writeln!(
            s,
            "            try {{ _ = c0.{base_pascal}OpenAndFill({full_ins}{opts_tail}{fargs}); eFill = false; }}"
        );
        s.push_str("            catch (ArgumentException) { eFill = true; }\n");
        s.push_str("            bool eOk = eOpen && eFill;\n");
        // legs:0, exactly like Java's short-circuit answer, so the
        // cross-language `legs` equality holds for this vector too. `ok` is
        // COMPUTED, never the literal 1 Java can afford.
        s.push_str("            return \"{\\\"retCode\\\":2,\\\"legs\\\":0,\\\"nb\\\":0,\\\"openRejects\\\":\" + (eOk ? 1 : 0) + \",\\\"enumRejects\\\":\" + ((eOpen ? 1 : 0) + (eFill ? 1 : 0)) + \",\\\"ok\\\":\" + (eOk ? 1 : 0) + \",\\\"peek_ok\\\":1}\";\n");
        s.push_str("        }\n");
    }

    if candle {
        s.push_str("        int rounds = (candleLegs != 0) ? 4 : 1;\n");
    } else {
        s.push_str("        int rounds = 1;\n");
    }
    s.push_str("        for (int rd = 0; rd < rounds; rd++) {\n");

    // Fresh, pinned, configured Core for this round. Built through the SHIPPED
    // CoreBuilder rather than by reaching into the instance's fields, so the
    // validation exercised is the library's own; `Build()` snapshots, so the
    // Core never aliases the builder.
    s.push_str("            CoreBuilder cb = Core.Builder();\n");
    for id in collect_pin_ids(func, funcs, enums) {
        let _ = writeln!(s, "            cb = cb.UnstablePeriod((FuncUnstId){id}, svK);");
    }
    if candle {
        s.push_str("            cb = SvApplyCandleRound(cb, rd);\n");
    }
    // Reported, never thrown: this runs in a subprocess ta_regtest drives over a
    // pipe, so an escaping exception surfaces as a dead pipe instead of a
    // diagnosable answer.
    s.push_str("            Core c2;\n");
    s.push_str("            try { c2 = cb.Build(); }\n");
    s.push_str("            catch (ArgumentOutOfRangeException) {\n");
    s.push_str("                return \"{\\\"error\\\":\\\"unstablePeriod out of range\\\"}\";\n");
    s.push_str("            }\n");

    // Expected-reject precheck (dispatch/period-bank arms with no stream) --
    // live since #139: HMA has no stream yet, so every MAType-dispatching
    // function (MA, BBANDS, APO/PPO/PVO, STOCH*, MACDEXT, MAVP) generates a
    // guard for the HMA arm. The guard compares raw enum ints, so C enum
    // constants are substituted with their values and enum params rewritten to
    // their `_raw_` locals -- the same two rewrites the Java emitter does.
    if let Some(guard) = sv_reject_condition(func, funcs, None) {
        let mut guard_cs = sv_guard_enum_ints(&guard, enums);
        for p in &func.optional_inputs {
            if matches!(p.param_type, crate::ir::ParamType::Enum(_)) {
                guard_cs = guard_cs.replace(&p.name, &format!("_raw_{}", p.name));
            }
        }
        let _ = writeln!(s, "            if ({guard_cs}) {{");
        s.push_str("                bool r1, r2;\n");
        let _ = writeln!(
            s,
            "                try {{ _ = c2.{base_pascal}Open({full_ins}{opts_tail}); r1 = false; }}"
        );
        s.push_str("                catch (ArgumentException) { r1 = true; }\n");
        s.push_str(&fdecls.replace("            ", "                "));
        let _ = writeln!(
            s,
            "                try {{ _ = c2.{base_pascal}OpenAndFill({full_ins}{opts_tail}{fargs}); r2 = false; }}"
        );
        s.push_str("                catch (ArgumentException) { r2 = true; }\n");
        s.push_str("                bool okr = r1 && r2;\n");
        s.push_str("                return \"{\\\"retCode\\\":0,\\\"legs\\\":0,\\\"unsupportedArm\\\":1,\\\"ok\\\":\" + (okr ? 1 : 0) + \",\\\"peek_ok\\\":1}\";\n");
        s.push_str("            }\n");
    }

    // ---- batch leg ----
    // `c2.<NAME>(...)` with `out` args and output arrays binds the INTERNAL
    // RetCode overload, not the public `OutRange` one: the gate needs the return
    // code, including the ones the public surface converts into throws.
    let _ = writeln!(
        s,
        "            RetCode rc;\n            try {{ rc = c2.{base}_Impl(0, svN - 1, {full_ins}, {opts_lead}out beg, out nb{bargs}); }}\n            catch (Exception _sve) when (_sve is ITaLibFailure) {{ rc = ((ITaLibFailure)_sve).RetCode; beg = 0; nb = 0; }}"
    );
    let _ = writeln!(s, "            int lb = c2.{base}_Lookback({opts});");
    s.push_str("            if (rc != RetCode.Success || nb == 0) {\n");
    // Reject parity: whenever the batch produced nothing -- an error (bad
    // params, an out-of-list enum reaching a dispatch default arm) or an empty
    // range -- Open must reject too. Open mirrors the batch validation and the
    // min-history rule by construction, so a stream that opens where batch fails
    // is always a contract break.
    s.push_str("                bool openRejects;\n");
    let _ = writeln!(
        s,
        "                try {{ _ = c2.{base_pascal}Open({full_ins}{opts_tail}); openRejects = false; }}"
    );
    s.push_str("                catch (ArgumentException) { openRejects = true; }\n");
    if candle {
        s.push_str("                if (!openRejects) allOk = false;\n");
        // A failed round must not truncate the sweep.
        s.push_str("                if (rd + 1 < rounds) continue;\n");
        s.push_str("                return \"{\\\"retCode\\\":\" + (int)rc + \",\\\"legs\\\":\" + legs + \",\\\"nb\\\":\" + nb + \",\\\"openRejects\\\":\" + (openRejects ? 1 : 0) + \",\\\"ok\\\":\" + (allOk ? 1 : 0) + \",\\\"peek_ok\\\":\" + (peekAll ? 1 : 0) + \",\\\"benign\\\":\" + zsign + \"}\";\n");
    } else {
        s.push_str("                return \"{\\\"retCode\\\":\" + (int)rc + \",\\\"legs\\\":0,\\\"nb\\\":\" + nb + \",\\\"openRejects\\\":\" + (openRejects ? 1 : 0) + \",\\\"ok\\\":\" + (openRejects ? 1 : 0) + \",\\\"peek_ok\\\":1}\";\n");
    }
    s.push_str("            }\n");

    // ---- OpenAndFill leg: the filled array == batch(0, n-1), bitwise ----
    s.push_str("            fillChecked = 1;\n            try {\n");
    s.push_str(&fdecls.replace("            ", "                "));
    let _ = writeln!(
        s,
        "                Core.{class} _fh = c2.{base_pascal}OpenAndFill({full_ins}{opts_tail}{fargs});"
    );
    // `OutRange` is a PROPERTY on the C# handle (Java spells it `outRange()`),
    // returning the shipped `OutRange` with `BegIdx` / `Count`.
    s.push_str("                OutRange _fr = _fh.OutRange;\n");
    let _ = writeln!(s, "                rangeChecked = 1; rangeLegs++; rangeSites |= {};", sv_range_bit(SvRangeSite::Fill, SV_RANGE_SITES_CSHARP));
    s.push_str("                if (_fr.BegIdx != beg || _fr.Count != nb) rangeOk = false;\n");
    s.push_str("                if (_fr.BegIdx != beg || _fr.Count != nb) fillOk = false;\n                else {\n");
    for i in 0..n_out {
        let cmp = xtier_ne(&format!("f{i}[bi]"), &format!("b{i}[bi]"), i, "zsign");
        let _ = writeln!(
            s,
            "                    for (int bi = 0; bi < nb; bi++) if ({cmp}) fillOk = false;"
        );
    }
    // Slack canary: everything above the produced range must be untouched.
    for (i, is_int) in out_is_int.iter().enumerate() {
        let ty = if *is_int { "int" } else { "double" };
        let canary = if *is_int { "-987654321" } else { "-1.2345678901234e300" };
        let _ = writeln!(
            s,
            "                    for (int bi = nb; bi < svN; bi++) if (f{i}[bi] != ({ty}){canary}) fillOk = false;"
        );
    }
    s.push_str("                }\n");

    // RULE 2 -- FULL ALIASING CROSS PRODUCT.
    //
    // Java probes exactly two pairs: output0 == input0 and output1 == output0.
    // That leaves most of the surface untested, and it matters most in the
    // composed tier, where `fill_scratch_may_alias_output` deliberately makes
    // `sc_<out>` alias the CALLER'S array for eight functions -- and where the
    // C# failure mode is a wrong VALUE, not an exception, so nothing else would
    // notice.
    //
    // So: every real output i x every distinct input array, plus every
    // same-typed output pair i<j. Each must throw ArgumentException and mint no
    // handle (a throw is the only way not to return one). O(9) probes for the
    // widest function.
    //
    // The input arrays are passed BY REFERENCE, deliberately -- a defensive copy
    // would make `ReferenceEquals` false and the probe vacuous. A missing guard
    // therefore writes into the fuzz series and corrupts the legs after it; that
    // is acceptable because a missing guard already sets `fillOk = false` right
    // here, so the run reds either way, and it reds louder.
    s.push_str("                /* R2: aliasing cross product -- every real output x every input,\n");
    s.push_str("                   then every same-typed output pair. Each must throw. */\n");
    for (i, i_is_int) in out_is_int.iter().enumerate() {
        if *i_is_int {
            continue; // an int[] output slot cannot take a double[] input
        }
        // Several input positions can map to the same fuzz array (generic
        // real1/real2 both land on fz_v), and two identical probes prove nothing
        // twice.
        let mut seen: std::collections::BTreeSet<&str> = std::collections::BTreeSet::new();
        for (j, arr) in arrays.iter().enumerate() {
            if !seen.insert(*arr) {
                continue;
            }
            let mut aargs = String::new();
            for k in 0..n_out {
                if k == i {
                    let _ = write!(aargs, ", {arr}");
                } else {
                    let _ = write!(aargs, ", f{k}");
                }
            }
            let _ = writeln!(
                s,
                "                try {{ _ = c2.{base_pascal}OpenAndFill({full_ins}{opts_tail}{aargs}); fillOk = false; }}"
            );
            let _ = writeln!(
                s,
                "                catch (ArgumentException) {{ /* expected: output {i} aliases input {} */ }}",
                inputs[j]
            );
        }
    }
    for (i, i_is_int) in out_is_int.iter().enumerate() {
        for (j, j_is_int) in out_is_int.iter().enumerate().skip(i + 1) {
            if i_is_int != j_is_int {
                continue; // different element types cannot alias
            }
            let mut aargs = String::new();
            for k in 0..n_out {
                if k == j {
                    let _ = write!(aargs, ", f{i}");
                } else {
                    let _ = write!(aargs, ", f{k}");
                }
            }
            let _ = writeln!(
                s,
                "                try {{ _ = c2.{base_pascal}OpenAndFill({full_ins}{opts_tail}{aargs}); fillOk = false; }}"
            );
            let _ = writeln!(
                s,
                "                catch (ArgumentException) {{ /* expected: output {j} aliases output {i} */ }}"
            );
        }
    }
    // R2b: PARTIAL overlap, the case only spans can express and the one that
    // distinguishes `Overlaps` from reference identity.
    //
    // Every probe above passes a WHOLE, identical buffer, where `==`,
    // `ReferenceEquals` and `Overlaps` all fire alike — so those probes cannot
    // tell a correct guard from the pre-span one. Reverting `alias_reject` to
    // identity left the whole gate green while multi-output functions silently
    // returned Success with every value wrong. These probes are what make the
    // overlap guard a checked property rather than a claim.
    //
    // Two shapes per pair, because they fail differently:
    //   - offset:  a window and the same window shifted one element in
    //   - same start, different length: identical memory and start, which span
    //     `==` reads as NOT equal, so an `==`-based guard waves it through
    //
    // EVERY window is `svN` long or longer, cut from a buffer one element wider
    // than the series. Slicing `f{i}` itself cannot do that: the widest window
    // inside it that still leaves room to shift is `svN - 1`, which rule S5
    // rejects for capacity the moment the lookback is 0 — so for the 28
    // unconditional-zero-lookback functions, and every period-taking one at
    // `period = 1`, the probe caught a capacity fault and never reached the
    // overlap guard it is named for (issue #271 item 2).
    if n_out >= 1 {
        let pair = |ints: bool| {
            out_is_int.iter().enumerate().any(|(i, a)| {
                *a == ints
                    && out_is_int.iter().skip(i + 1).any(|b| *b == ints)
            })
        };
        if pair(false) {
            s.push_str("                double[] ovD = new double[svN + 1];\n");
        }
        if pair(true) {
            s.push_str("                int[] ovI = new int[svN + 1];\n");
        }
        if out_is_int.iter().any(|b| !*b) {
            if let Some(arr) = arrays.first() {
                // The input leg needs the OUTPUT to overlap an INPUT, so the
                // input has to come out of the wide buffer too — same values,
                // one element of headroom.
                let _ = writeln!(s, "                double[] ovIn = new double[svN + 1];");
                let _ = writeln!(s, "                Array.Copy({arr}, ovIn, svN);");
            }
        }
        s.push_str("                /* R2b: PARTIAL overlap -- only spans can express it, and it is
");
        s.push_str("                   the only shape that separates Overlaps from identity. */
");
        for (i, i_is_int) in out_is_int.iter().enumerate() {
            for (j, j_is_int) in out_is_int.iter().enumerate().skip(i + 1) {
                if i_is_int != j_is_int {
                    continue;
                }
                let ov = if *i_is_int { "ovI" } else { "ovD" };
                for (shape, expr) in [
                    ("offset", format!("{ov}.AsSpan(1, svN)")),
                    ("same start, longer", format!("{ov}.AsSpan(0, svN + 1)")),
                ] {
                    let mut aargs = String::new();
                    for k in 0..n_out {
                        if k == j {
                            let _ = write!(aargs, ", {expr}");
                        } else if k == i {
                            let _ = write!(aargs, ", {ov}.AsSpan(0, svN)");
                        } else {
                            let _ = write!(aargs, ", f{k}");
                        }
                    }
                    let _ = writeln!(
                        s,
                        "                try {{ _ = c2.{base_pascal}OpenAndFill({full_ins}{opts_tail}{aargs}); fillOk = false; }}"
                    );
                    let _ = writeln!(
                        s,
                        "                catch (ArgumentException) {{ /* expected: outputs {i}/{j} partially overlap ({shape}) */ }}"
                    );
                }
            }
        }
        // Output partially overlapping an INPUT. Whole-buffer in-place is
        // legitimate and stays accepted; only the partial case is a reject, so
        // this probe is the one that pins that distinction.
        for (i, i_is_int) in out_is_int.iter().enumerate() {
            if *i_is_int {
                continue;
            }
            if !arrays.is_empty() {
                let mut aargs = String::new();
                for k in 0..n_out {
                    if k == i {
                        let _ = write!(aargs, ", ovIn.AsSpan(1, svN)");
                    } else {
                        let _ = write!(aargs, ", f{k}");
                    }
                }
                // The history comes out of `ovIn` so the output window above
                // overlaps it; every other input stays the fuzz array, and the
                // two agree in length (rule S5's input half).
                let ov_ins = arrays
                    .iter()
                    .enumerate()
                    .map(|(k, a)| {
                        if k == 0 { "ovIn.AsSpan(0, svN)".to_string() } else { (*a).to_string() }
                    })
                    .collect::<Vec<_>>()
                    .join(", ");
                let _ = writeln!(
                    s,
                    "                try {{ _ = c2.{base_pascal}OpenAndFill({ov_ins}{opts_tail}{aargs}); fillOk = false; }}"
                );
                let _ = writeln!(
                    s,
                    "                catch (ArgumentException) {{ /* expected: output {i} partially overlaps an input */ }}"
                );
            }
        }
    }
    s.push_str("            } catch (ArgumentException) { fillOk = false; }\n");

    // ---- prefix sweep: the trajectory, bit-exact against batch ----
    if func_has_seed_boundary(func, funcs) {
        // The Metastock seed boundary shifts the earliest openable prefix by
        // one. svCompat != 0 is refused above (R6), so this is 0 in every
        // request the driver sends today -- kept COMPUTED, and derived from the
        // same helper the C, Rust and Java gates use, so the leg is already
        // right if C# ever grows a compatibility selector.
        s.push_str("            int seedShift = (svCompat == 1) ? 1 : 0;\n");
    } else {
        s.push_str("            int seedShift = 0;\n");
    }
    s.push_str("            int[] pcs = { lb + 1 + seedShift, lb + 13, svN / 2, svN - 1 };\n");
    s.push_str("            Array.Sort(pcs);\n");
    s.push_str("            int prevP = -1;\n");
    s.push_str("            for (int pi = 0; pi < pcs.Length; pi++) {\n");
    s.push_str("                int p = pcs[pi];\n");
    s.push_str("                if (p < lb + 1 + seedShift || p > svN - 1 || p == prevP) continue;\n");
    s.push_str("                prevP = p;\n");
    let _ = writeln!(s, "                Core.{class} st;");
    let _ = writeln!(
        s,
        "                try {{ st = c2.{base_pascal}Open({}{opts_tail}); }}",
        pfx_ins("p")
    );
    s.push_str("                catch (ArgumentException) { allOk = false; if (diag.Length == 0) diag = \",\\\"openRejectP\\\":\" + p; continue; }\n");
    // THE ONLY `legs++` IN THIS FILE. See the header note: the plan compares
    // this count against the Java row for equality, so every C#-only leg below
    // reports through its own counter instead.
    s.push_str("                legs++;\n");
    // Open-value compare, through `Value`. Load-bearing: Open returns only the
    // handle, so this anchor compare IS the `Value` verification at open.
    let up_ty = if multi {
        format!("Core.{valty}")
    } else if out_is_int[0] {
        "int".to_string()
    } else {
        "double".to_string()
    };
    let _ = writeln!(s, "                {up_ty} v0 = st.Value;");
    for i in 0..n_out {
        let cmp = xtier_ne(&rd_out("v0", i), &format!("b{i}[p - 1 - beg]"), i, "zsign");
        let _ = writeln!(
            s,
            "                if ({cmp}) {{ allOk = false; if (diag.Length == 0) diag = \",\\\"badBar\\\":\" + (p - 1) + \",\\\"badOut\\\":{i},\\\"where\\\":\\\"open\\\"\"; }}"
        );
    }

    // Update loop: peek-every-7 (same-tier) + Value == update (same-tier, per
    // component) + update vs batch (cross-tier).
    let emit_up_compares = |s: &mut String, pad: &str| {
        for i in 0..n_out {
            let a = rd_out("up", i);
            let b = format!("b{i}[t - beg]");
            let cmp = xtier_ne(&a, &b, i, "zsign");
            let bv = diag_val(&b, i);
            let sv = diag_val(&a, i);
            let _ = writeln!(
                s,
                "{pad}if ({cmp}) {{ allOk = false; if (diag.Length == 0) diag = \",\\\"badBar\\\":\" + t + \",\\\"badOut\\\":{i},\\\"batchv\\\":\\\"\" + {bv} + \"\\\",\\\"streamv\\\":\\\"\" + {sv} + \"\\\"\"; }}"
            );
        }
    };
    s.push_str("                for (int t = p; t < svN; t++) {\n");
    s.push_str("                    if (t % 7 == 0) {\n");
    let _ = writeln!(s, "                        {up_ty} pk = st.Peek({bars_t});");
    let _ = writeln!(s, "                        {up_ty} up = st.Update({bars_t});");
    for i in 0..n_out {
        let cmp = same_tier_ne(&rd_out("pk", i), &rd_out("up", i), i);
        let _ = writeln!(s, "                        if ({cmp}) peekAll = false;");
    }
    // `Value` == the value just returned, read AFTER an intervening `Peek`.
    //
    // The intervening peek is the whole leg. Without it this compares
    // `Update`'s return against a `Value` read with nothing in between, and
    // both render from the same generator expression over the same fields --
    // literally `new X_Value(cur_a, cur_b) != new X_Value(cur_a, cur_b)`. That
    // cannot fail, and it did not: it passed unchanged while the guard it was
    // meant to protect was reverted. Java's twin asserts REFERENCE IDENTITY of
    // its cached record, which pins an allocation property; deleting the cache
    // for a returned record struct removed the only thing being checked, and
    // keeping the comparison kept the shape without the substance.
    //
    // Peeking a DIFFERENT bar (t-1, always in range since t >= lb+1 >= 1) makes
    // it a real check of the documented contract: `Value` is a pure read that
    // `Peek` does not disturb. A `Peek` that commits advances the handle, and
    // `Value` then reports the peeked bar instead of the committed one. On
    // FUZZ_CONSTANT the two bars carry the same number, so there the leg leans
    // on state advancement rather than on a value difference -- which is why it
    // is a complement to `peek_ok`, not a replacement for it.
    //
    // Comparison is per component and strict: record-struct `==` would call
    // +0.0 equal to -0.0 and NaN equal to NaN, i.e. would pass on exactly the
    // corruption this leg exists to find.
    let _ = writeln!(s, "                        _ = st.Peek({});", bar_args("t - 1"));
    let _ = writeln!(s, "                        {up_ty} vc = st.Value;");
    for i in 0..n_out {
        let cmp = same_tier_ne(&rd_out("vc", i), &rd_out("up", i), i);
        let _ = writeln!(
            s,
            "                        if ({cmp}) {{ allOk = false; if (diag.Length == 0) diag = \",\\\"valueNeUpdate\\\":\" + t; }}"
        );
    }
    emit_up_compares(&mut s, "                        ");
    s.push_str("                    } else {\n");
    let _ = writeln!(s, "                        {up_ty} up = st.Update({bars_t});");
    emit_up_compares(&mut s, "                        ");
    s.push_str("                    }\n");
    s.push_str("                }\n");
    // Open(p) + (svN - p) updates: whatever p was, the handle has consumed svN
    // bars and must report exactly what batch(0, svN-1) did. Only when the value
    // leg passed — otherwise the handle is short of the bars it was to consume.
    s.push_str("                if (allOk) {\n");
    let _ = writeln!(s, "                    rangeChecked = 1; rangeLegs++; rangeSites |= {};", sv_range_bit(SvRangeSite::Prefix, SV_RANGE_SITES_CSHARP));
    s.push_str("                    if (st.OutRange.BegIdx != beg || st.OutRange.Count != nb) rangeOk = false;\n");
    s.push_str("                }\n");
    s.push_str("            }\n");


    // ---- UpdateAndFill leg (#246): the earliest prefix open, then ONE call
    // over the tail instead of `svN - p` separate updates.
    //
    // Three probes ride on the same handle because each leaves it untouched:
    // an output shorter than the run, an output that OVERLAPS an input (C#'s
    // `Span.Overlaps` sees the partial case Java's reference equality cannot),
    // and a zero-bar call, which is a success that changes nothing.
    s.push_str("            {\n");
    s.push_str("                int p = lb + 1 + seedShift;\n");
    s.push_str("                if (p <= svN - 1) {\n");
    s.push_str("                    ufillChecked = 1;\n");
    s.push_str("                    try {\n");
    let _ = writeln!(
        s,
        "                        Core.{class} stu = c2.{base_pascal}Open({}{opts_tail});",
        pfx_ins("p")
    );
    s.push_str("                        OutRange ur0 = stu.OutRange;\n");
    for (i, is_int) in out_is_int.iter().enumerate() {
        let ty = if *is_int { "int" } else { "double" };
        let canary = if *is_int { "-987654321" } else { "-1.2345678901234e300" };
        let _ = writeln!(s, "                        {ty}[] u{i} = new {ty}[svN];");
        let _ = writeln!(s, "                        Array.Fill(u{i}, ({ty}){canary});");
    }
    let tail_ins: String = arrays
        .iter()
        .map(|a| format!("{a}.AsSpan(p)"))
        .collect::<Vec<_>>()
        .join(", ");
    let empty_ins: String = arrays
        .iter()
        .map(|a| format!("{a}.AsSpan(p, 0)"))
        .collect::<Vec<_>>()
        .join(", ");
    let uargs: String = (0..out_is_int.len()).fold(String::new(), |mut acc, i| {
        let _ = write!(acc, ", u{i}");
        acc
    });
    let _ = writeln!(s, "                        stu.UpdateAndFill({empty_ins}{uargs});");
    {
        let short_idx =
            short_probe_index(&func.outputs.iter().map(crate::ir::Output::is_nullable).collect::<Vec<_>>());
        let short: String = out_is_int
            .iter()
            .enumerate()
            .map(|(i, is_int)| {
                if i == short_idx {
                    format!(", new {}[0]", if *is_int { "int" } else { "double" })
                } else {
                    format!(", u{i}")
                }
            })
            .collect();
        let _ = writeln!(
            s,
            "                        try {{ stu.UpdateAndFill({tail_ins}{short}); ufillOk = false; }} catch (ArgumentException) {{ /* expected: output shorter than the run */ }}"
        );
    }
    if !out_is_int[0] {
        let alias: String = (0..out_is_int.len())
            .map(|i| if i == 0 { format!(", {}.AsSpan(p)", arrays[0]) } else { format!(", u{i}") })
            .collect();
        let _ = writeln!(
            s,
            "                        try {{ stu.UpdateAndFill({tail_ins}{alias}); ufillOk = false; }} catch (ArgumentException) {{ /* expected: output overlaps input */ }}"
        );
    }
    s.push_str("                        if (stu.OutRange.BegIdx != ur0.BegIdx || stu.OutRange.Count != ur0.Count) ufillOk = false;\n");
    let _ = writeln!(s, "                        stu.UpdateAndFill({tail_ins}{uargs});");
    for i in 0..n_out {
        let cmp = xtier_ne(&format!("u{i}[t - p]"), &format!("b{i}[t - beg]"), i, "zsign");
        let _ = writeln!(
            s,
            "                        for (int t = p; t < svN; t++) if ({cmp}) ufillOk = false;"
        );
    }
    for (i, is_int) in out_is_int.iter().enumerate() {
        let ty = if *is_int { "int" } else { "double" };
        let canary = if *is_int { "-987654321" } else { "-1.2345678901234e300" };
        let _ = writeln!(s, "                        for (int t = svN - p; t < svN; t++) if (u{i}[t] != ({ty}){canary}) ufillOk = false;");
    }
    let _ = writeln!(
        s,
        "                        rangeChecked = 1; rangeLegs++; rangeSites |= {};",
        sv_range_bit(SvRangeSite::UpdateFill, SV_RANGE_SITES_CSHARP)
    );
    s.push_str("                        if (stu.OutRange.BegIdx != beg || stu.OutRange.Count != nb) { ufillOk = false; rangeOk = false; }\n");
    s.push_str("                    } catch (ArgumentException) { ufillOk = false; }\n");
    s.push_str("                }\n");
    s.push_str("            }\n");

    // ---- Clone() independence: open at the earliest prefix, advance to mid,
    // clone, drive both to the end. Both must match batch (cross-tier) and each
    // other (same-tier). A shallow ring / sub-handle / bank copy diverges here.
    // `Clone()` is C#'s spelling of Java's `copy()`.
    s.push_str("            {\n");
    s.push_str("                int p0 = lb + 1 + seedShift;\n");
    s.push_str("                if (p0 <= svN - 1) {\n");
    s.push_str("                    try {\n");
    let _ = writeln!(
        s,
        "                        Core.{class} sA = c2.{base_pascal}Open({}{opts_tail});",
        pfx_ins("p0")
    );
    s.push_str("                        int mid = (p0 + svN) / 2;\n");
    let _ = writeln!(
        s,
        "                        for (int t = p0; t < mid; t++) sA.Update({bars_t});"
    );
    let _ = writeln!(s, "                        Core.{class} sB = sA.Clone();");
    s.push_str("                        for (int t = mid; t < svN; t++) {\n");
    let _ = writeln!(s, "                            {up_ty} uA = sA.Update({bars_t});");
    let _ = writeln!(s, "                            {up_ty} uB = sB.Update({bars_t});");
    for i in 0..n_out {
        let same = same_tier_ne(&rd_out("uA", i), &rd_out("uB", i), i);
        let cross = xtier_ne(&rd_out("uA", i), &format!("b{i}[t - beg]"), i, "zsign");
        let _ = writeln!(
            s,
            "                            if ({same} || {cross}) {{ allOk = false; if (diag.Length == 0) diag = \",\\\"copyDiverged\\\":\" + t; }}"
        );
    }
    s.push_str("                        }\n");
    s.push_str("                    } catch (ArgumentException) { allOk = false; if (diag.Length == 0) diag = \",\\\"copyOpenReject\\\":1\"; }\n");
    s.push_str("                }\n");
    s.push_str("            }\n");

    // RULE 4 -- THE ALLOCATION PROBE, RESTRUCTURED.
    //
    // A separate loop, containing ONLY Update. Deliberately NOT folded into the
    // trajectory sweep: that loop Peeks every 7th bar, and Peek allocates a
    // scratch handle on the ~83 functions where the [ThreadStatic] scratch is
    // not elected, so a folded probe would red for a reason that is not a
    // defect.
    //
    // No warm-up pass is emitted: the sweep above has already JIT-compiled every
    // step and its callees on this thread. The measured handle is FRESH on
    // purpose -- a handle that allocates lazily on its first Update is a real
    // defect and this probe is meant to see it.
    //
    // `sink` accumulates ONE component and is consumed into a static field AFTER
    // the measured region, so the JIT cannot prove the Update results dead and
    // delete the calls. Nothing inside the region boxes the returned value (32
    // bytes per bar, measured).
    let sink_ty = if out_is_int[0] { "long" } else { "double" };
    let sink_zero = if out_is_int[0] { "0L" } else { "0.0" };
    s.push_str("            {\n");
    s.push_str("                int pa = lb + 1 + seedShift;\n");
    s.push_str("                if (pa <= svN - 1) {\n");
    s.push_str("                    try {\n");
    let _ = writeln!(
        s,
        "                        Core.{class} sQ = c2.{base_pascal}Open({}{opts_tail});",
        pfx_ins("pa")
    );
    let _ = writeln!(s, "                        {sink_ty} sink = {sink_zero};");
    s.push_str("                        long a0 = GC.GetAllocatedBytesForCurrentThread();\n");
    s.push_str("                        for (int t = pa; t < svN; t++) {\n");
    let _ = writeln!(s, "                            {up_ty} uq = sQ.Update({bars_t});");
    let _ = writeln!(s, "                            sink += {};", rd_out("uq", 0));
    s.push_str("                        }\n");
    s.push_str("                        long ad = GC.GetAllocatedBytesForCurrentThread() - a0;\n");
    s.push_str("                        svUpdSink += sink;\n");
    s.push_str("                        if (ad > updAlloc) updAlloc = ad;\n");
    s.push_str("                        if (ad != 0) { allOk = false; if (diag.Length == 0) diag = \",\\\"updAllocBytes\\\":\" + ad; }\n");
    s.push_str("                    } catch (ArgumentException) { /* open rejects here -- nothing to measure */ }\n");
    s.push_str("                }\n");
    s.push_str("            }\n");

    // RULE 3 -- A REAL MID-STREAM CANDLE-SETTINGS LEG.
    //
    // The four rounds above do NOT test the open-time snapshot at all: each
    // builds a fresh Core and opens on it, so the step's snapshot and the Core's
    // live settings hold the same value and a step reading the wrong one is
    // invisible. Keep the rounds as the open-time check; do not claim more.
    //
    // This leg opens under the round's settings, then OVERWRITES the settings
    // the step reads, then drives the remaining Update bars and requires them to
    // still match the PRE-mutation batch. It reds if and only if the step reads
    // live settings instead of its open-time snapshot.
    //
    // The settings are saved before the leg and restored in a `finally`, so
    // every later leg in the round still runs under the round's settings
    // whatever happens here -- which is what keeps this leg order-independent.
    if candle {
        s.push_str("            {\n");
        s.push_str("                int pc = lb + 1 + seedShift;\n");
        s.push_str("                if (pc <= svN - 1) {\n");
        s.push_str("                    CandleSetting[] svSaved = (CandleSetting[])c2.candleSettings.Clone();\n");
        s.push_str(&mdecls);
        s.push_str("                    try {\n");
        let _ = writeln!(
            s,
            "                        Core.{class} sC = c2.{base_pascal}Open({}{opts_tail});",
            pfx_ins("pc")
        );
        s.push_str("                        SvMutateLiveCandles(c2);\n");
        s.push_str("                        candleMutRan = 1;\n");
        // Non-vacuity witness. Re-run the BATCH under the mutated settings: if it
        // answers the same as the unmutated batch then a step reading live
        // settings would match too, and this leg proves nothing for this vector.
        // Reported, never failed -- a pattern that never fires on this shape
        // legitimately answers all-zero either way, so the S3 gate asserts
        // `candleMutMoved` rather than this leg guessing.
        s.push_str("                        int mBeg = 0, mNb = 0;\n");
        let _ = writeln!(
            s,
            "                        RetCode mrc;\n                        try {{ mrc = c2.{base}_Impl(0, svN - 1, {full_ins}, {opts_lead}out mBeg, out mNb{margs}); }}\n                        catch (Exception _mve) when (_mve is ITaLibFailure) {{ mrc = ((ITaLibFailure)_mve).RetCode; mBeg = 0; mNb = 0; }}"
        );
        s.push_str("                        if (mrc != RetCode.Success || mNb != nb || mBeg != beg) candleMutMoved = 1;\n");
        s.push_str("                        else {\n");
        for i in 0..n_out {
            let _ = writeln!(
                s,
                "                            for (int bi = 0; bi < nb; bi++) if (m{i}[bi] != b{i}[bi]) candleMutMoved = 1;"
            );
        }
        s.push_str("                        }\n");
        s.push_str("                        for (int t = pc; t < svN; t++) {\n");
        let _ = writeln!(s, "                            {up_ty} uC = sC.Update({bars_t});");
        for i in 0..n_out {
            let cmp = xtier_ne(&rd_out("uC", i), &format!("b{i}[t - beg]"), i, "zsignMut");
            let _ = writeln!(
                s,
                "                            if ({cmp}) {{ allOk = false; if (diag.Length == 0) diag = \",\\\"candleSnapshotLeaked\\\":\" + t + \",\\\"badOut\\\":{i}\"; }}"
            );
        }
        s.push_str("                        }\n");
        s.push_str("                    } catch (ArgumentException) { allOk = false; if (diag.Length == 0) diag = \",\\\"candleMutOpenReject\\\":1\"; }\n");
        // The ONE place a broader catch is right, and only because of what this
        // leg does: it deliberately drives the library into a state the contract
        // says the step must ignore. A step that does NOT ignore it can index a
        // ring sized from the open-time avgPeriod with a live-settings index and
        // throw IndexOutOfRange -- which, uncaught, kills the process and the
        // driver reports a pipe failure naming nothing. Converting it into a
        // named red is strictly more informative and cannot mask anything else:
        // no other leg runs inside this try.
        s.push_str("                    catch (IndexOutOfRangeException) { allOk = false; if (diag.Length == 0) diag = \",\\\"candleSnapshotLeakedThrow\\\":1\"; }\n");
        s.push_str("                    finally { SvRestoreLiveCandles(c2, svSaved); }\n");
        s.push_str("                }\n");
        s.push_str("            }\n");
    }

    // ---- short-history reject: at exactly `lb` bars no output is defined for
    // ANY configuration, so Open must reject -- and with the TYPED exception,
    // because `InsufficientHistoryException` is the one routine, data-dependent
    // failure a caller is meant to catch separately from a programming error.
    // The derived catch must come first (C# rejects the other order, CS0160),
    // which is also what makes the "wrong type" arm reachable.
    s.push_str("            if (lb >= 1 && lb < svN) {\n");
    let _ = writeln!(
        s,
        "                try {{ _ = c2.{base_pascal}Open({}{opts_tail}); allOk = false; if (diag.Length == 0) diag = \",\\\"shortHistoryAccepted\\\":1\"; }}",
        pfx_ins("lb")
    );
    s.push_str("                catch (InsufficientHistoryException) { /* expected, typed */ }\n");
    s.push_str("                catch (ArgumentException) { allOk = false; if (diag.Length == 0) diag = \",\\\"shortHistoryWrongType\\\":1\"; }\n");
    s.push_str("            }\n");

    // ---- int.MinValue default-sentinel pair: Open(int.MinValue) must equal
    // Open(the explicit YAML default) BITWISE. The batch guard transcribes into
    // the stream open, so defaulting can never silently diverge. SAME-TIER: one
    // code path twice, no licence to differ.
    if has_int_default {
        let mut sent_args: Vec<String> = Vec::new();
        let mut expl_args: Vec<String> = Vec::new();
        for p in &func.optional_inputs {
            match &p.param_type {
                crate::ir::ParamType::Integer if p.default.is_some() => {
                    let d = p.default.unwrap_or(0.0) as i64;
                    sent_args.push("int.MinValue".to_string());
                    expl_args.push(format!("{d}"));
                }
                _ => {
                    sent_args.push(p.name.clone());
                    expl_args.push(p.name.clone());
                }
            }
        }
        s.push_str("            try {\n");
        let _ = writeln!(
            s,
            "                Core.{class} sD = c2.{base_pascal}Open({full_ins}, {});",
            sent_args.join(", ")
        );
        let _ = writeln!(
            s,
            "                Core.{class} sE = c2.{base_pascal}Open({full_ins}, {});",
            expl_args.join(", ")
        );
        let _ = writeln!(s, "                {up_ty} vD = sD.Value;");
        let _ = writeln!(s, "                {up_ty} vE = sE.Value;");
        for i in 0..n_out {
            let cmp = same_tier_ne(&rd_out("vD", i), &rd_out("vE", i), i);
            let _ = writeln!(
                s,
                "                if ({cmp}) {{ allOk = false; if (diag.Length == 0) diag = \",\\\"minValueDefault\\\":1\"; }}"
            );
        }
        s.push_str("            } catch (ArgumentException) { /* defaults need more history than svN -- skip */ }\n");
    }

    // startIdx-anchored range site (#241) — the C leg's twin. `_OpenInternal`'s
    // range is max(startIdx, lookback), resolved by a different emitter branch
    // from the two sites above, and it was gated in C alone.
    s.push_str("            {\n");
    let anchored_bit = sv_range_bit(SvRangeSite::Anchored, SV_RANGE_SITES_CSHARP);
    s.push_str("                int Sidx = lb + (svN - lb) / 3;\n");
    s.push_str("                if (Sidx > lb && Sidx < svN - 1) {\n");
    s.push_str("                    int begS = 0, nbS = 0;\n");
    s.push_str("                    RetCode rcS;\n");
    let _ = writeln!(
        s,
        "                    try {{ rcS = c2.{base}_Impl(Sidx, svN - 1, {full_ins}, {opts_lead}out begS, out nbS{bargs}); }}\n\
         \x20                   catch (Exception _sve) when (_sve is ITaLibFailure) {{ rcS = ((ITaLibFailure)_sve).RetCode; }}"
    );
    s.push_str("                    if (rcS == RetCode.Success && nbS > 0) {\n");
    let _ = writeln!(
        s,
        "                        try {{\n\
         \x20                           Core.{class} stA = c2.{base_pascal}OpenInternal({}, Sidx{opts_tail});\n\
         \x20                           rangeChecked = 1; rangeLegs++; rangeSites |= {anchored_bit};\n\
         \x20                           if (stA.OutRange.BegIdx != begS || stA.OutRange.Count != nbS) rangeOk = false;\n\
         \x20                       }} catch (ArgumentException) {{ rangeOk = false; if (diag.Length == 0) diag = \",\\\"anchoredOpenRejected\\\":1\"; }}",
        pfx_ins("svN")
    );
    s.push_str("                    }\n");
    s.push_str("                }\n");

    s.push_str("            }\n");

    s.push_str("        }\n");
    // `fill_ok` folds into `ok` as a safety net (mirrors the C/Rust/Java gates),
    // and so does the allocation probe -- `updAlloc` is a diagnostic AND a
    // failure (R4). `benign` is the Java-comparable count; `benignMut` is the
    // C#-only mid-stream leg's, reported separately so the cross-language
    // equality check compares like with like.
    s.push_str("        string extra = \",\\\"updAlloc\\\":\" + updAlloc;\n");
    if candle {
        s.push_str("        extra += \",\\\"candleMut\\\":\" + candleMutRan + \",\\\"candleMutMoved\\\":\" + candleMutMoved + \",\\\"benignMut\\\":\" + zsignMut;\n");
    }

    s.push_str("        return \"{\\\"retCode\\\":0,\\\"beg\\\":\" + beg + \",\\\"nb\\\":\" + nb + \",\\\"legs\\\":\" + legs + \",\\\"fill_checked\\\":\" + fillChecked + \",\\\"fill_ok\\\":\" + (fillOk ? 1 : 0) + \",\\\"ufill_checked\\\":\" + ufillChecked + \",\\\"ufill_ok\\\":\" + (ufillOk ? 1 : 0) + \",\\\"range_checked\\\":\" + rangeChecked + \",\\\"range_legs\\\":\" + rangeLegs + \",\\\"range_sites\\\":\" + rangeSites + \",\\\"range_sites_n\\\":"); s.push_str(&SV_RANGE_SITES_CSHARP.to_string()); s.push_str(",\\\"range_ok\\\":\" + (rangeOk ? 1 : 0) + \",\\\"ok\\\":\" + ((allOk && fillOk && ufillOk && rangeOk) ? 1 : 0) + \",\\\"peek_ok\\\":\" + (peekAll ? 1 : 0) + \",\\\"benign\\\":\" + zsign + extra + diag + \"}\";\n");
    s.push_str("    }\n\n");
    s
}

/// The whole C# `stream_verify` section: the two comparators, the candle-round
/// and live-mutation helpers, the allocation sink, one `Sv_<NAME>` per function
/// with an emitted C# stream, the `fuzz_in_hash` self-check, and the dispatcher.
///
/// The dispatcher's unknown-method answer is DELIBERATELY NOT Java's
/// `not_streamable` -- see the block comment at the emit site.
#[allow(clippy::too_many_lines)]
pub(crate) fn generate_csharp_stream_verify(
    funcs: &[FuncDef],
    enums: &HashMap<String, EnumDef>,
) -> String {
    use std::fmt::Write as _;
    let mut s = String::new();
    s.push_str("    // ---- stream_verify: C# stream vs C# batch, bitwise ----\n\n");

    // Strict, same-tier comparator.
    s.push_str("    /* Strict comparator for SAME-TIER legs: peek vs update, Value vs update,\n");
    s.push_str("       clone vs clone, and the int.MinValue sentinel pair all run ONE code path\n");
    s.push_str("       twice, so they have no licence to differ in a single bit. -0.0 != +0.0\n");
    s.push_str("       here and NaN == NaN -- which is exactly why a <NAME>_Value's own `==`\n");
    s.push_str("       must never be used for these: record-struct equality says the opposite\n");
    s.push_str("       of both, so it would pass on precisely what these legs look for. */\n");
    s.push_str("    static bool SvBne(double a, double b) =>\n");
    s.push_str("        BitConverter.DoubleToInt64Bits(a) != BitConverter.DoubleToInt64Bits(b);\n\n");

    // Cross-tier comparator.
    s.push_str("    /* Cross-tier comparator: stream vs batch, fill vs batch. Bits that differ\n");
    s.push_str("       but compare equal are +0.0 vs -0.0 (issue #147) -- counted as benign,\n");
    s.push_str("       never a mismatch, because the two tiers reach a zero by different but\n");
    s.push_str("       equally correct routes.\n");
    s.push_str("       `zsign` is a `ref` to a REQUEST-SCOPED local, never a static: one process\n");
    s.push_str("       answers many requests and a static would carry one function's count into\n");
    s.push_str("       the next. (Java passes a one-element array here only because Java has no\n");
    s.push_str("       `ref`.) */\n");
    s.push_str("    static bool SvXtierNe(double a, double b, ref long zsign) {\n");
    s.push_str("        if (!SvBne(a, b)) return false;\n");
    s.push_str("        if (a == b) { zsign++; return false; }\n");
    s.push_str("        return true;\n");
    s.push_str("    }\n\n");

    // Elision barrier for the allocation probe.
    s.push_str("    /* The allocation probe accumulates one output component into a local and\n");
    s.push_str("       consumes it here, AFTER the measured region. Without a consumer the JIT\n");
    s.push_str("       can prove the Update results dead and delete the calls, and the probe\n");
    s.push_str("       then reads 0 bytes for a loop that never ran. Never read. */\n");
    s.push_str("    static double svUpdSink;\n\n");

    // Candle rounds, through the shipped builder.
    s.push_str("    /* Candle-settings rounds (mirror the C/Rust/Java sweep): defaults /\n");
    s.push_str("       avgPeriod+3 / avgPeriod=0 (instant candle) / rangeType=Shadows.\n");
    s.push_str("       Goes through the SHIPPED CoreBuilder, so the validation exercised is the\n");
    s.push_str("       library's own and not a second copy living in the server; Build() takes a\n");
    s.push_str("       snapshot, so a built Core never aliases the builder.\n");
    s.push_str("       Each round is derived from Core.DefaultCandleSettings, never from the\n");
    s.push_str("       previous round -- a fresh builder is seeded with exactly those defaults,\n");
    s.push_str("       so the rounds cannot compound. (C# needs none of Java's care about\n");
    s.push_str("       editing the shared defaults in place: Core clones the array and\n");
    s.push_str("       CandleSetting is immutable, so replacing a slot reaches nothing else.)\n");
    s.push_str("       WHAT THESE FOUR ROUNDS DO NOT TEST: the open-time snapshot. Each round\n");
    s.push_str("       builds its Core and opens on it, so the step's snapshot and the Core's\n");
    s.push_str("       live settings hold the same value and a step reading the wrong one is\n");
    s.push_str("       invisible here. SvMutateLiveCandles below is what tests that. */\n");
    s.push_str("    static CoreBuilder SvApplyCandleRound(CoreBuilder b, int rd) {\n");
    s.push_str("        if (rd == 0) return b;\n");
    s.push_str("        for (int ci = 0; ci < Core.DefaultCandleSettings.Length; ci++) {\n");
    s.push_str("            CandleSetting cs = Core.DefaultCandleSettings[ci];\n");
    s.push_str("            if (rd == 1)\n");
    s.push_str("                b = b.CandleSetting((CandleSettingType)ci, cs.RangeType, cs.AvgPeriod + 3, cs.Factor);\n");
    s.push_str("            else if (rd == 2)\n");
    s.push_str("                b = b.CandleSetting((CandleSettingType)ci, cs.RangeType, 0, cs.Factor);\n");
    s.push_str("            else if (rd == 3)\n");
    s.push_str("                b = b.CandleSetting((CandleSettingType)ci, TALib.RangeType.Shadows, cs.AvgPeriod, cs.Factor);\n");
    s.push_str("        }\n");
    s.push_str("        return b;\n");
    s.push_str("    }\n\n");

    // Live mutation of a BUILT Core -- the one thing the builder cannot express.
    s.push_str("    /* Overwrite a BUILT Core's live candle settings, in place.\n");
    s.push_str("       `Core.candleSettings` is `internal readonly CandleSetting[]` -- the\n");
    s.push_str("       REFERENCE is readonly, the ELEMENTS are not -- and CandleSetting's\n");
    s.push_str("       constructor is internal. Both are reachable because the server csproj\n");
    s.push_str("       compiles the library sources into its own assembly.\n");
    s.push_str("       This is the one thing CoreBuilder deliberately cannot express: a\n");
    s.push_str("       settings change AFTER the Core exists, which is precisely what the\n");
    s.push_str("       streaming open-time snapshot contract is about.\n");
    s.push_str("       All three fields move, and to values no round uses, so a step reading\n");
    s.push_str("       live settings must produce a DIFFERENT number rather than a\n");
    s.push_str("       differently-spelled same one.\n");
    s.push_str("       The caller saves and restores: every later leg in the round runs against\n");
    s.push_str("       the round's settings, not these. */\n");
    s.push_str("    static void SvMutateLiveCandles(Core c) {\n");
    s.push_str("        for (int ci = 0; ci < c.candleSettings.Length; ci++) {\n");
    s.push_str("            CandleSetting cs = c.candleSettings[ci];\n");
    s.push_str("            c.candleSettings[ci] = new CandleSetting(\n");
    s.push_str("                cs.RangeType == TALib.RangeType.Shadows ? TALib.RangeType.HighLow\n");
    s.push_str("                                                       : TALib.RangeType.Shadows,\n");
    s.push_str("                (cs.AvgPeriod + 7) % 13,\n");
    s.push_str("                cs.Factor * 2.0 + 0.5);\n");
    s.push_str("        }\n");
    s.push_str("    }\n\n");
    s.push_str("    static void SvRestoreLiveCandles(Core c, CandleSetting[] saved) {\n");
    s.push_str("        Array.Copy(saved, c.candleSettings, saved.Length);\n");
    s.push_str("    }\n\n");

    let lookup = crate::streaming::FuncsLookup(funcs);
    let emitted: Vec<&FuncDef> = funcs
        .iter()
        .filter(|f| crate::backends::csharp_stream::emits_stream(f, &lookup))
        .collect();
    for f in &emitted {
        s.push_str(&emit_csharp_sv_func(f, funcs, enums));
    }

    // fuzz_in_hash -- the same input-port self-check the Rust and Java servers
    // answer (issue #113): proves the FuzzData port reproduces C's fuzz_gen
    // bytes. The stream pass probes it, so the port cannot silently rot.
    //
    // INTEGRATION NOTE 2: the driver's existing self-check runs at exactly one
    // seed (gen_seed = 7, n = 240) while the vector loop uses others, and it
    // fails OPEN -- an absent `in_hash` just breaks the loop. Neither is fixable
    // from this side (both live in test_codegen.c), which is why S2's gate is on
    // the literal printed line "Fuzz-port self-check: 9/9 shapes bit-identical"
    // and not on the absence of a failure message.
    s.push_str("    static string HandleFuzzInHash(JsonElement req) {\n");
    s.push_str("        int shape = GetInt(req, \"gen_shape\", 0);\n");
    s.push_str("        int seed = GetInt(req, \"gen_seed\", 0);\n");
    s.push_str("        int n = GetInt(req, \"gen_n\", 0);\n");
    s.push_str("        if (n < 1) n = 1;\n");
    s.push_str("        if (n > MAX_ARRAY_SIZE) n = MAX_ARRAY_SIZE;\n");
    s.push_str("        double[] fo = new double[n]; double[] fh = new double[n]; double[] fl = new double[n];\n");
    s.push_str("        double[] fc = new double[n]; double[] fv = new double[n]; double[] foi = new double[n];\n");
    s.push_str("        FuzzData.FuzzGen(shape, seed, n, fo, fh, fl, fc, fv, foi);\n");
    s.push_str("        ulong hh = SvHashInit();\n");
    s.push_str("        hh = SvHashF64(hh, fo, n);\n");
    s.push_str("        hh = SvHashF64(hh, fh, n);\n");
    s.push_str("        hh = SvHashF64(hh, fl, n);\n");
    s.push_str("        hh = SvHashF64(hh, fc, n);\n");
    s.push_str("        hh = SvHashF64(hh, fv, n);\n");
    s.push_str("        hh = SvHashF64(hh, foi, n);\n");
    s.push_str("        hh = SvHashFin(hh);\n");
    s.push_str("        return \"{\\\"in_hash\\\":\\\"\" + hh.ToString(\"x16\") + \"\\\"}\";\n");
    s.push_str("    }\n\n");

    s.push_str("    static string HandleStreamVerify(JsonElement req) {\n");
    s.push_str("        string fn = req.GetProperty(\"funcName\").GetString()!;\n");
    s.push_str("        switch (fn) {\n");
    for f in &emitted {
        let _ = writeln!(
            s,
            "        case \"TA_{}\": return Sv_{}(req);",
            f.name.to_uppercase(),
            f.name
        );
    }
    // ============ THE DARK RESPONSE -- READ BEFORE CHANGING THIS LINE ========
    //
    // The driver's capability probe is a SUBSTRING test:
    //     test_codegen.c:3686   strstr(responseBuf, "not_streamable")
    // and it is all-or-nothing: the moment the C# server answers with that
    // token, ta_regtest starts requiring a stream handler for every function
    // carrying TA_FUNC_FLG_STREAM and prints STREAM SET MISMATCH for each one
    // that is missing. The C# metadata catalogue already publishes that flag on
    // all 172 functions, so copying Java's literal now would redden every
    // regtest.py run for the remaining stages, for a reason unrelated to the
    // work in flight.
    //
    // So the unknown-method answer -- including the TA_STREAM_PROBE the driver
    // sends -- is pinned to the string below, which must NOT contain the token
    // `not_streamable` anywhere in the emitted file. S2 gates on
    // `grep -c not_streamable TaCodegenServe.cs == 0` until S9.
    //
    // The flip happened once all 172 functions had a handler, which is the
    // precondition the all-or-nothing check needs. It is a capability
    // ANNOUNCEMENT, not a description of this method: the driver reads the
    // token off the unknown-name path (it probes with TA_STREAM_PROBE, which is
    // not a function), so answering it is how the server says "ask me about
    // streams". A real function name that fell through to here would be a
    // missing handler, and the driver reports that as STREAM SET MISMATCH.
    // ========================================================================
    s.push_str("        default: return \"{\\\"error\\\":\\\"not_streamable\\\"}\";\n");
    s.push_str("        }\n");
    s.push_str("    }\n\n");
    s
}

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
