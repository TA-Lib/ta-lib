use std::collections::HashMap;

use std::cell::Cell;

use crate::candle_settings::{detect_candle_settings, emit_c_unpacking};
use crate::helper_registry::{hoist_block_helpers, try_inline_expr, HelperRegistry};
use crate::ir::{
    BinOp, CircBuf, CircBufLayout, EnumDef, Expr, FuncDef, LookbackExpr, ParamType, Statement,
    VarType,
};
use crate::parser::enums::lookup_variant;
use crate::registry::{Lang, Registry};
use super::common::{expr_directly_contains_candle_call, pascal_word};
use super::expr_walk::{binop_prec, expr_prec, wrap_child, wrap_inlined, ExprEmitter};
use super::builtins::{MathFn, SpecialBuiltin, StdlibFn};
use super::stmt_walk::StatementEmitter;

/// Candle helper functions emitted as C preprocessor macros instead of expanded code.
/// This enables compiler loop-unswitching, matching the reference library's macro pattern.
const C_CANDLE_MACRO_FNS: &[&str] = &["ta_candlerange", "ta_candleaverage"];

/// Per-render state for the C backend, mirroring `RustRenderCtx` in `rust_lang.rs`.
/// Bundles the loose state (precision flag + inline-helper counter) threaded through
/// the recursive renderer. Services (enums/registry/helpers) stay as separate params.
struct CRenderCtx<'a> {
    single_precision: bool,
    inline_counter: &'a std::cell::Cell<usize>,
    /// Stream-transition rendering: candle helper calls take scalar bar
    /// values, so they render as TA_STREAM_CANDLE* macros (mirroring the
    /// batch macros' arithmetic exactly) instead of the array-indexed
    /// TA_CANDLE* macros.
    stream_scalar_candles: bool,
}

#[allow(clippy::implicit_hasher)]
pub fn generate(
    func: &FuncDef,
    enums: &HashMap<String, EnumDef>,
    registry: &Registry,
    helpers: &HelperRegistry,
) -> String {
    let mut out = String::new();
    out.push_str(&gen_header());
    out.push_str(&gen_header_comments(func));
    out.push_str(&gen_lookback(func, enums, registry, helpers));

    // For functions with explicit _private, emit Private and Logic BEFORE guarded
    // so the C compiler knows the signatures when the guarded body calls them.
    if func.has_explicit_private {
        out.push_str(&gen_private(func, false, enums, registry, helpers)); // double private
        out.push_str(&gen_private(func, true, enums, registry, helpers)); // single-precision private
        out.push_str(&gen_func(func, false, true, enums, registry, helpers)); // double-precision logic
        out.push_str(&gen_func(func, false, false, enums, registry, helpers)); // double-precision guarded
        out.push_str(&gen_func(func, true, true, enums, registry, helpers)); // single-precision logic
        out.push_str(&gen_func(func, true, false, enums, registry, helpers)); // single-precision guarded
    } else {
        out.push_str(&gen_func(func, false, false, enums, registry, helpers)); // double-precision guarded
        out.push_str(&gen_func(func, false, true, enums, registry, helpers)); // double-precision logic
        out.push_str(&gen_func(func, true, false, enums, registry, helpers)); // single-precision guarded
        out.push_str(&gen_func(func, true, true, enums, registry, helpers)); // single-precision logic
    }

    // Streaming API section (only for YAML-declared streamable functions).
    if func.streaming {
        out.push_str(&super::c_stream::generate(func, enums, registry, helpers));
    }
    out
}

/// Render a C variable declaration (`type name`, including pointer and array
/// forms) for the given [`VarType`]. Single source for the per-statement,
/// hoisted-block, and lookback-local declaration emitters.
pub(crate) fn c_decl(var_type: &VarType, name: &str) -> String {
    match var_type {
        VarType::Real => format!("double {name}"),
        VarType::Integer | VarType::Index => format!("int {name}"),
        VarType::RetCodeType => format!("TA_RetCode {name}"),
        VarType::RealPointer => format!("double *{name}"),
        VarType::IntPointer => format!("int *{name}"),
        VarType::RealArray(size) => format!("double {name}[{size}]"),
        VarType::IntArray(size) => format!("int {name}[{size}]"),
    }
}

/// The scalar C type name for a [`VarType`], used for cast expressions.
fn c_type_name(var_type: &VarType) -> &'static str {
    match var_type {
        VarType::Real => "double",
        VarType::Integer | VarType::Index => "int",
        VarType::RetCodeType => "TA_RetCode",
        VarType::RealPointer => "double *",
        VarType::IntPointer => "int *",
        VarType::RealArray(_) | VarType::IntArray(_) => {
            unreachable!("array-typed cast is not representable in C")
        }
    }
}

/// The field-split storage buffers backing a CIRCBUF. `Plain` is a single buffer named
/// `<id>`; `Class` is one buffer per struct field named `<id>_<field>` (matching the
/// `CIRCBUF_REF` access flatten). Returns `(storage_name, element_type)` pairs.
fn circbuf_fields(id: &str, layout: &CircBufLayout) -> Vec<(String, VarType)> {
    match layout {
        CircBufLayout::Plain(t) => vec![(id.to_string(), t.clone())],
        CircBufLayout::Class(fields) => fields
            .iter()
            .map(|(f, t)| (format!("{id}_{f}"), t.clone()))
            .collect(),
    }
}

/// The C type of an optional input parameter (`double` / `int` / `TA_<Enum>`),
/// shared by the lookback and function signature builders.
fn c_opt_param_type(param_type: &ParamType) -> String {
    match param_type {
        ParamType::Real => "double".to_string(),
        ParamType::Integer => "int".to_string(),
        ParamType::Enum(name) => format!("TA_{name}"),
        ParamType::Price(_) => unreachable!("Price expanded during parsing"),
    }
}

/// Emit the file-level comment blocks carried from the input `.c` (e.g. the
/// contributors / change-history block), each as a block comment, so authorship
/// is preserved in the generated source.
fn gen_header_comments(func: &FuncDef) -> String {
    let mut out = String::new();
    for block in &func.header_comments {
        out.push_str(&super::stmt_walk::block_comment(block, 0));
        out.push('\n');
    }
    out
}

fn gen_header() -> String {
    let mut out = String::new();
    out.push_str(
        "/* TA-LIB Copyright (c) 1999-2025, Mario Fortier\n\
         * All rights reserved.\n\
         *\n\
         * Redistribution and use in source and binary forms, with or\n\
         * without modification, are permitted provided that the following\n\
         * conditions are met:\n\
         *\n\
         * - Redistributions of source code must retain the above copyright\n\
         *   notice, this list of conditions and the following disclaimer.\n\
         *\n\
         * - Redistributions in binary form must reproduce the above copyright\n\
         *   notice, this list of conditions and the following disclaimer in\n\
         *   the documentation and/or other materials provided with the\n\
         *   distribution.\n\
         *\n\
         * - Neither name of author nor the names of its contributors\n\
         *   may be used to endorse or promote products derived from this\n\
         *   software without specific prior written permission.\n\
         *\n\
         * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS\n\
         * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT\n\
         * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS\n\
         * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE\n\
         * REGENTS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,\n\
         * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES\n\
         * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS\n\
         * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS\n\
         * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,\n\
         * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE\n\
         * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,\n\
         * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.\n\
         */\n\n",
    );

    out.push_str(
        "/* AUTO-GENERATED by ta_codegen — DO NOT EDIT.\n\
         * Source of truth: ta_codegen/input/<name>/  (regenerate: cd ta_codegen/generator && cargo run -- generate)\n\
         */\n\n",
    );

    // Match the c-ref (gen_code output) includes, plus ta_func_unguarded.h so
    // cross-indicator `TA_*_Unguarded` / `TA_*_Private` calls have a visible
    // prototype in separate-compilation (library) builds — otherwise each
    // src/ta_func/*.c hits -Wimplicit-function-declaration. That header is
    // guarded, shipped (in the dist), and pure declarations, so it is a no-op in
    // the single-TU server/bench builds.
    out.push_str(
        "#include <string.h>\n\
         #include <math.h>\n\
         #include \"ta_func.h\"\n\
         #include \"ta_utility.h\"\n\
         #include \"ta_memory.h\"\n\
         #include \"ta_func_unguarded.h\"\n\n",
    );

    out
}

/// Optional-parameter validation prologue: map the TA_INTEGER_DEFAULT /
/// TA_REAL_DEFAULT sentinels to the documented default value, then reject
/// out-of-range values. One source of truth for both function variants:
/// guarded functions fail with `TA_BAD_PARAM`, lookback functions fail with
/// `-1` (the classic lookback bad-param contract that wrappers rely on).
// Integer optional-param defaults and ranges are stored as `f64` in the IR; casting
// the integer-valued ones to `i32` for literal emission is exact, not truncating.
#[allow(clippy::cast_possible_truncation)]
pub(crate) fn emit_opt_param_validation(func: &FuncDef, fail: &str) -> String {
    let mut out = String::new();
    for opt in &func.optional_inputs {
        match &opt.param_type {
            ParamType::Integer | ParamType::Enum(_) => {
                if let Some(default_val) = opt.default {
                    out.push_str(&format!(
                        "   if( (int){name} == (int)0x80000000 )\n      {name} = {val};\n",
                        name = opt.name,
                        val = default_val as i32
                    ));
                    if let Some((min, max)) = opt.range {
                        let min_i = min as i32;
                        let max_i = max as i32;
                        out.push_str(&format!(
                            "   else if( (int){name} < {min_i} || (int){name} > {max_i} )\n      return {fail};\n",
                            name = opt.name
                        ));
                    }
                }
            }
            ParamType::Real => {
                if let Some(default_val) = opt.default {
                    out.push_str(&format!(
                        "   if( {name} == -4e37 )\n      {name} = {val};\n",
                        name = opt.name,
                        val = default_val
                    ));
                    if let Some((min, max)) = opt.range {
                        // Skip unbounded ranges (f64::MIN/MAX = no real constraint)
                        if min > f64::MIN / 2.0 || max < f64::MAX / 2.0 {
                            out.push_str(&format!(
                                "   else if( {name} < {min:e} || {name} > {max:e} )\n      return {fail};\n",
                                name = opt.name
                            ));
                        }
                    }
                }
            }
            // Price params expand to arrays handled separately; no scalar validation.
            ParamType::Price(_) => {}
        }
    }
    out
}

fn gen_lookback(
    func: &FuncDef,
    enums: &HashMap<String, EnumDef>,
    registry: &Registry,
    helpers: &HelperRegistry,
) -> String {
    let name = &func.name;

    let has_opt_params = !func.optional_inputs.is_empty();

    // Build parameter list for signature
    let param_str = if has_opt_params {
        let params: Vec<String> = func
            .optional_inputs
            .iter()
            .map(|opt| {
                let c_type = c_opt_param_type(&opt.param_type);
                format!("{} {}", c_type, opt.name)
            })
            .collect();
        format!(" {} ", params.join(", "))
    } else {
        " void ".to_string()
    };

    // Same param validation as the guarded function, with the lookback
    // bad-param contract: out-of-range returns -1. For Code bodies it is
    // injected after the local declarations (C89 ordering).
    let validation = emit_opt_param_validation(func, "-1");

    let body = match &func.lookback {
        Some(LookbackExpr::Literal(n)) => format!("{validation}   return {n};\n"),
        Some(LookbackExpr::ParamMinus(param, offset)) => {
            format!("{validation}   return {param} - {offset};\n")
        }
        Some(LookbackExpr::Code(stmts)) => {
            render_lookback_code(stmts, &validation, enums, registry, helpers)
        }
        None => format!("{validation}   return 0;\n"),
    };

    format!(
        "TA_LIB_API int TA_{name}_Lookback({param_str})\n\
         {{\n\
         {body}\
         }}\n\n"
    )
}

/// Generate a `TA_{NAME}_Private` (double) or `TA_S_{NAME}_Private` (single-precision)
/// function variant. Same body/params as `gen_func(logic=true)` but with `_Private`
/// naming and the extra private params (e.g. EMA's k factor).
///
/// The single-precision variant takes `const float` inputs (double intermediates/
/// outputs, per the library convention). It exists so that single-precision callers
/// which apply an indicator to the ORIGINAL float input with a precomputed param
/// (e.g. `TA_S_MACD` -> `TA_S_EMA_Private(... inReal ...)`) resolve; calls that act on
/// double intermediate buffers continue to use the double `TA_{NAME}_Private`.
fn gen_private(
    func: &FuncDef,
    single_precision: bool,
    enums: &HashMap<String, EnumDef>,
    registry: &Registry,
    helpers: &HelperRegistry,
) -> String {
    let name_override = if single_precision {
        format!("TA_S_{}_Private", func.name)
    } else {
        format!("TA_{}_Private", func.name)
    };
    gen_func_inner(func, single_precision, true, Some(&name_override), enums, registry, helpers)
}

#[allow(clippy::too_many_lines)]
fn gen_func(
    func: &FuncDef,
    single_precision: bool,
    logic: bool,
    enums: &HashMap<String, EnumDef>,
    registry: &Registry,
    helpers: &HelperRegistry,
) -> String {
    gen_func_inner(func, single_precision, logic, None, enums, registry, helpers)
}

// Integer/enum optIn defaults and ranges come from f64 metadata but are whole
// numbers; the `as i32` casts in the validation emitter are intentional.
#[allow(clippy::too_many_lines, clippy::cognitive_complexity, clippy::cast_possible_truncation)]
fn gen_func_inner(
    func: &FuncDef,
    single_precision: bool,
    logic: bool,
    name_override: Option<&str>,
    enums: &HashMap<String, EnumDef>,
    registry: &Registry,
    helpers: &HelperRegistry,
) -> String {
    let mut out = String::new();

    let prefix = if let Some(name) = name_override {
        name.to_string()
    } else {
        match (single_precision, logic) {
            (false, false) => format!("TA_{}", func.name),
            (false, true) => format!("TA_{}_Unguarded", func.name),
            (true, false) => format!("TA_S_{}", func.name),
            (true, true) => format!("TA_S_{}_Unguarded", func.name),
        }
    };

    let ret_type = if single_precision {
        "TA_RetCode"
    } else {
        "TA_LIB_API TA_RetCode"
    };

    // Build parameter list
    let mut params: Vec<String> = Vec::new();
    params.push("int    startIdx".to_string());
    params.push("int    endIdx".to_string());

    for input in &func.inputs {
        let c_type = if single_precision {
            match input.param_type {
                ParamType::Real => "const float",
                ParamType::Integer | ParamType::Enum(_) | ParamType::Price(_) => "const int",
            }
        } else {
            match input.param_type {
                ParamType::Real => "const double",
                ParamType::Integer | ParamType::Enum(_) | ParamType::Price(_) => "const int",
            }
        };
        params.push(format!("{} {}[]", c_type, input.name));
    }

    for opt in &func.optional_inputs {
        let c_type = c_opt_param_type(&opt.param_type);
        params.push(format!("{} {}", c_type, opt.name));
    }

    // Extra params only on _Private variant (via name_override)
    if name_override.is_some() {
        for (param_name, c_type) in &func.private_extra_params {
            params.push(format!("{c_type} {param_name}"));
        }
    }

    // Output scalars (always present)
    params.push("int          *outBegIdx".to_string());
    params.push("int          *outNBElement".to_string());

    for output in &func.outputs {
        let c_type = match output.param_type {
            ParamType::Real => "double",
            ParamType::Integer | ParamType::Enum(_) | ParamType::Price(_) => "int",
        };
        params.push(format!("{}        {}[]", c_type, output.name));
    }

    // Format the function signature
    let indent = " ".repeat(ret_type.len() + 1 + prefix.len() + 2);
    out.push_str(&format!("{ret_type} {prefix}( "));
    for (i, param) in params.iter().enumerate() {
        if i > 0 {
            out.push_str(&format!(",\n{indent}"));
        }
        out.push_str(param);
    }
    out.push_str(" )\n");

    // Function body
    out.push_str("{\n");

    // Body selection:
    // - Private variant (name_override set): always private_body
    // - S_ variants with explicit _private: inline private_body (can't call double-only Private)
    // - Double variants with explicit _private (guarded OR logic): body delegates to TA_*_Private
    // - Logic without _private: private_body (auto-copied from body, same content)
    // - Guarded without _private: body
    let body = if name_override.is_some() || (single_precision && func.has_explicit_private) {
        // Private variant (actual computation body), or S_ variant inlining it
        &func.private_body
    } else if func.has_explicit_private {
        &func.body          // double guarded/logic: body delegates to _Private
    } else if logic {
        &func.private_body  // logic variant without _private
    } else {
        &func.body          // guarded without _private
    };

    // Carry source comments only in the double-precision implementation: the
    // guarded `TA_*` and (for explicit-private functions) the `TA_*_Private`
    // that holds the algorithm. Strip them from every single-precision (`TA_S_*`)
    // copy and from the double `*_Unguarded`/logic duplicate, so they appear once.
    let keep_comments = !single_precision && (name_override.is_some() || !logic);
    let body_stripped;
    let body: &[Statement] = if keep_comments {
        body
    } else {
        body_stripped = super::stmt_walk::strip_comments(body);
        &body_stripped
    };

    // Declare local variables from body (deduplicated)
    let mut declared_vars: Vec<String> = Vec::new();
    for stmt in body {
        emit_c_var_decls(stmt, &mut out, &mut declared_vars);
    }

    let inline_counter = Cell::new(0);
    let ctx = &CRenderCtx { single_precision, inline_counter: &inline_counter, stream_scalar_candles: false };

    // For S_ variants with explicit _private: emit private_param_init as local VarDecls.
    // These provide the extra params (e.g., k factor) that the inlined private body needs.
    // Both guarded and logic S_ variants need this (both use private_body).
    if single_precision && func.has_explicit_private && name_override.is_none() {
        for (param_name, init_expr) in &func.private_param_init {
            let c_type = func
                .private_extra_params
                .iter()
                .find(|(n, _)| n == param_name)
                .map_or("double", |(_, t)| t.as_str());
            let init_c = render_expr(init_expr, ctx, registry, helpers);
            out.push_str(&format!("   {c_type} {param_name} = {init_c};\n"));
            declared_vars.push(param_name.clone());
        }
    }

    // Emit candle settings unpacking (only for referenced settings)
    let candle_used = detect_candle_settings(body);
    if !candle_used.is_empty() {
        out.push_str(&emit_c_unpacking(&candle_used, 3));
    }

    out.push('\n');

    // Validation (omitted for Logic/unguarded variant)
    if !logic {
        out.push_str("   if( startIdx < 0 )\n");
        out.push_str("      return TA_OUT_OF_RANGE_START_INDEX;\n");
        out.push_str("   if( (endIdx < 0) || (endIdx < startIdx) )\n");
        out.push_str("      return TA_OUT_OF_RANGE_END_INDEX;\n");
        out.push('\n');

        // Input array NULL checks
        for input in &func.inputs {
            out.push_str(&format!("   if( !{} )\n", input.name));
            out.push_str("      return TA_BAD_PARAM;\n");
        }

        // Optional parameter validation (default + range)
        out.push_str(&emit_opt_param_validation(func, "TA_BAD_PARAM"));

        // Output array NULL checks
        for output in &func.outputs {
            out.push_str(&format!("   if( !{} )\n", output.name));
            out.push_str("      return TA_BAD_PARAM;\n");
        }
        out.push('\n');
    }

    // Determine which VarDecl names are first-occurrence (eligible for hoisted init).
    // A VarDecl with init is only hoisted if it's the first occurrence of that name.
    // Duplicate VarDecls (same name, second or later occurrence) are rendered inline.
    let mut first_seen_names: Vec<String> = Vec::new();
    collect_first_seen_var_names(body, &mut first_seen_names);

    // Emit hoisted VarDecl initializations (first-occurrence only)
    for stmt in body {
        if let Statement::VarDecl {
            name,
            init: Some(init),
            ..
        } = stmt
        {
            if first_seen_names.contains(name) {
                // This is the first occurrence with init — hoist it
                first_seen_names.retain(|n| n != name); // remove so duplicates aren't hoisted
                let mut hoisted = Vec::new();
                let mut cnt = ctx.inline_counter.get();
                let new_init = hoist_block_helpers(init, helpers, &mut hoisted, &mut cnt, C_CANDLE_MACRO_FNS);
                ctx.inline_counter.set(cnt);
                out.push_str(&render_hoisted_blocks(
                    &hoisted,
                    3,
                    ctx,
                    enums,
                    registry,
                    helpers,
                ));
                out.push_str(&format!(
                    "   {} = {};\n",
                    name,
                    render_expr(&new_init, ctx, registry, helpers)
                ));
            }
        }
    }

    // Render remaining body statements.
    // First-occurrence VarDecls (with or without init) are skipped.
    // Duplicate VarDecls with init are rendered as plain assignments in their natural position.
    let mut body_seen: Vec<String> = Vec::new();
    for stmt in body {
        match stmt {
            Statement::VarDecl { name, init, .. } => {
                if body_seen.contains(name) {
                    // Duplicate VarDecl: render as assignment if it has an init
                    if let Some(init_expr) = init {
                        let mut hoisted = Vec::new();
                        let mut cnt = ctx.inline_counter.get();
                        let new_init =
                            hoist_block_helpers(init_expr, helpers, &mut hoisted, &mut cnt, C_CANDLE_MACRO_FNS);
                        ctx.inline_counter.set(cnt);
                        out.push_str(&render_hoisted_blocks(
                            &hoisted,
                            3,
                            ctx,
                            enums,
                            registry,
                            helpers,
                        ));
                        out.push_str(&format!(
                            "   {} = {};\n",
                            name,
                            render_expr(&new_init, ctx, registry, helpers)
                        ));
                    }
                } else {
                    body_seen.push(name.clone());
                }
            }
            Statement::Block { body } => {
                // Track VarDecl names inside Blocks (multi-var declarations)
                // and render non-VarDecl statements (e.g., chained assignments
                // like `*outBegIdx = today = startIdx;` which parse into a Block
                // containing [Assign(today=startIdx), Assign(*outBegIdx=today)]).
                for s in body {
                    if let Statement::VarDecl { name, .. } = s {
                        if !body_seen.contains(name) {
                            body_seen.push(name.clone());
                        }
                    } else {
                        out.push_str(&render_stmt(
                            s,
                            3,
                            ctx,
                            enums,
                            registry,
                            helpers,
                        ));
                    }
                }
            }
            _ => {
                out.push_str(&render_stmt(
                    stmt,
                    3,
                    ctx,
                    enums,
                    registry,
                    helpers,
                ));
            }
        }
    }

    out.push_str("}\n\n");

    out
}

/// Render a ForC init or update clause. If it's a Block with multiple
/// statements, comma-separate them instead of using semicolons.
fn render_forc_part(
    stmt: &Statement,
    ctx: &CRenderCtx,
    enums: &HashMap<String, EnumDef>,
    registry: &Registry,
    helpers: &HelperRegistry,
) -> String {
    match stmt {
        Statement::Block { body } => body
            .iter()
            .map(|s| {
                render_stmt(s, 0, ctx, enums, registry, helpers)
                    .trim()
                    .trim_end_matches(';')
                    .to_string()
            })
            .collect::<Vec<_>>()
            .join(", "),
        _ => render_stmt(stmt, 0, ctx, enums, registry, helpers)
            .trim()
            .trim_end_matches(';')
            .to_string(),
    }
}

/// Render hoisted block-inline helpers as C code (temp var decl + body).
fn render_hoisted_blocks(
    hoisted: &[(String, VarType, Vec<Statement>)],
    indent: usize,
    ctx: &CRenderCtx,
    enums: &HashMap<String, EnumDef>,
    registry: &Registry,
    helpers: &HelperRegistry,
) -> String {
    let pad = " ".repeat(indent);
    let mut out = String::new();
    for (temp_name, var_type, body) in hoisted {
        let decl = c_decl(var_type, temp_name);
        out.push_str(&format!("{pad}{decl};\n"));
        for stmt in body {
            out.push_str(&render_stmt(
                stmt,
                indent,
                ctx,
                enums,
                registry,
                helpers,
            ));
        }
    }
    out
}

#[allow(clippy::implicit_hasher)]
pub fn render_statement(
    stmt: &Statement,
    indent: usize,
    single_precision: bool,
    enums: &HashMap<String, EnumDef>,
    registry: &Registry,
    helpers: &HelperRegistry,
    inline_counter: &Cell<usize>,
) -> String {
    let ctx = CRenderCtx { single_precision, inline_counter, stream_scalar_candles: false };
    render_stmt(stmt, indent, &ctx, enums, registry, helpers)
}

/// Like [`render_statement`], but for generated stream-transition bodies:
/// candle helper calls render as scalar TA_STREAM_CANDLE* macros.
#[allow(clippy::implicit_hasher)]
pub fn render_statement_stream(
    stmt: &Statement,
    indent: usize,
    enums: &HashMap<String, EnumDef>,
    registry: &Registry,
    helpers: &HelperRegistry,
    inline_counter: &Cell<usize>,
) -> String {
    let ctx = CRenderCtx {
        single_precision: false,
        inline_counter,
        stream_scalar_candles: true,
    };
    render_stmt(stmt, indent, &ctx, enums, registry, helpers)
}

/// C-backend leaf formatting for the shared [`StatementEmitter`] tree-walk.
/// Bundles the render context with the enum/registry/helper services the hooks
/// need; the recursion and variant dispatch live in [`StatementEmitter::walk_stmt`].
struct CStmt<'a> {
    ctx: &'a CRenderCtx<'a>,
    enums: &'a HashMap<String, EnumDef>,
    registry: &'a Registry,
    helpers: &'a HelperRegistry,
}

impl CStmt<'_> {
    /// Render the shared tail of an `if`: the then-body followed by the else
    /// branch (with the `} else if` collapse + leading-comment peel). Shared by
    /// the flat and multi-line-condition rendering paths.
    fn render_if_tail(&self, then_body: &[Statement], else_body: &[Statement], indent: usize) -> String {
        let pad = " ".repeat(indent);
        let mut out = String::new();
        for s in then_body {
            out.push_str(&self.walk_stmt(s, indent + 3));
        }
        if else_body.is_empty() {
            out.push_str(&format!("{pad}}}\n"));
        } else {
            let code_start = else_body
                .iter()
                .position(|s| !matches!(s, Statement::Comment(_)))
                .unwrap_or(else_body.len());
            let is_else_if = else_body.len() - code_start == 1
                && matches!(else_body.get(code_start), Some(Statement::If { .. }));
            if is_else_if {
                for c in &else_body[..code_start] {
                    out.push_str(&self.walk_stmt(c, indent));
                }
                out.push_str(&format!("{pad}}} else "));
                out.push_str(self.walk_stmt(&else_body[code_start], indent).trim_start());
                return out;
            }
            out.push_str(&format!("{pad}}} else "));
            out.push_str(&format!("\n{pad}{{\n"));
            for s in else_body {
                out.push_str(&self.walk_stmt(s, indent + 3));
            }
            out.push_str(&format!("{pad}}}\n"));
        }
        out
    }
}

impl StatementEmitter for CStmt<'_> {
    fn comment(&self, lines: &[String], indent: usize) -> String {
        super::stmt_walk::block_comment(lines, indent)
    }

    #[allow(clippy::too_many_lines)]
    fn circ_buf(&self, op: &CircBuf, indent: usize) -> String {
        let pad = " ".repeat(indent);
        match op {
            // Storage is hoisted/declared by emit_c_var_decls; nothing to emit here.
            CircBuf::Prolog { .. } => String::new(),
            // Advance with conditional reset (not modulo) — matches the reference macro.
            CircBuf::Next { id } => {
                format!("{pad}{id}_Idx++;\n{pad}if( {id}_Idx > maxIdx_{id} ) {id}_Idx = 0;\n")
            }
            // Free each heap buffer iff it was allocated (pointer != the stack buffer).
            CircBuf::Destroy { id, layout } => {
                let mut s = String::new();
                for (storage, _t) in circbuf_fields(id, layout) {
                    s.push_str(&format!(
                        "{pad}if( {storage} != &local_{storage}[0] ) TA_Free( {storage} );\n"
                    ));
                }
                s
            }
            // Always use the stack buffer; bound from its static capacity.
            CircBuf::InitLocalOnly { id, layout } => {
                let fields = circbuf_fields(id, layout);
                let mut s = String::new();
                for (storage, _t) in &fields {
                    s.push_str(&format!("{pad}{storage} = &local_{storage}[0];\n"));
                }
                let (first, first_t) = &fields[0];
                let et = c_type_name(first_t);
                s.push_str(&format!(
                    "{pad}maxIdx_{id} = (int)(sizeof(local_{first})/sizeof({et}))-1;\n"
                ));
                s.push_str(&format!("{pad}{id}_Idx = 0;\n"));
                s
            }
            // Stack-first hybrid (ta_memory.h #else): heap-allocate only when the runtime
            // size exceeds the static stack capacity; otherwise point at the stack buffer.
            CircBuf::Init { id, layout, size } => {
                let fields = circbuf_fields(id, layout);
                let sz = render_expr(size, self.ctx, self.registry, self.helpers);
                let (first, first_t) = &fields[0];
                let et0 = c_type_name(first_t);
                let mut s = String::new();
                s.push_str(&format!(
                    "{pad}if( {sz} < 1 ) return TA_INTERNAL_ERROR(137);\n"
                ));
                s.push_str(&format!(
                    "{pad}if( (int){sz} > (int)(sizeof(local_{first})/sizeof({et0})) )\n{pad}{{\n"
                ));
                let mut allocated: Vec<String> = Vec::new();
                for (storage, t) in &fields {
                    let et = c_type_name(t);
                    s.push_str(&format!(
                        "{pad}   {storage} = TA_Malloc( sizeof({et})*{sz} );\n"
                    ));
                    s.push_str(&format!("{pad}   if( !{storage} )\n{pad}   {{\n"));
                    for prev in &allocated {
                        s.push_str(&format!("{pad}      TA_Free( {prev} );\n"));
                    }
                    s.push_str(&format!("{pad}      return TA_ALLOC_ERR;\n{pad}   }}\n"));
                    allocated.push(storage.clone());
                }
                s.push_str(&format!("{pad}}}\n{pad}else\n{pad}{{\n"));
                for (storage, _t) in &fields {
                    s.push_str(&format!("{pad}   {storage} = &local_{storage}[0];\n"));
                }
                s.push_str(&format!("{pad}}}\n"));
                s.push_str(&format!("{pad}maxIdx_{id} = ({sz}-1);\n"));
                s.push_str(&format!("{pad}{id}_Idx = 0;\n"));
                s
            }
        }
    }

    #[allow(clippy::too_many_lines, clippy::cognitive_complexity)]
    fn var_decl(&self, var_type: &VarType, name: &str, init: &Option<Expr>, indent: usize) -> String {
        let pad = " ".repeat(indent);
        let decl = c_decl(var_type, name);
        match init {
            Some(init_expr) => {
                // Hoist multi-statement helpers from the init expression
                let mut hoisted = Vec::new();
                let mut cnt = self.ctx.inline_counter.get();
                let new_init = hoist_block_helpers(
                    init_expr, self.helpers, &mut hoisted, &mut cnt, C_CANDLE_MACRO_FNS,
                );
                self.ctx.inline_counter.set(cnt);
                let mut out = render_hoisted_blocks(
                    &hoisted, indent, self.ctx, self.enums, self.registry,
                    self.helpers,
                );
                out.push_str(&format!(
                    "{pad}{decl} = {};\n",
                    render_expr(&new_init, self.ctx, self.registry, self.helpers)
                ));
                out
            }
            None => format!("{pad}{decl};\n"),
        }
    }

    #[allow(clippy::too_many_lines, clippy::cognitive_complexity)]
    fn assign(&self, target: &Expr, value: &Expr, compound: bool, indent: usize) -> String {
        let pad = " ".repeat(indent);
        // Hoist multi-statement helpers from the value expression
        let mut hoisted = Vec::new();
        let mut cnt = self.ctx.inline_counter.get();
        let new_value = hoist_block_helpers(
            value, self.helpers, &mut hoisted, &mut cnt, C_CANDLE_MACRO_FNS,
        );
        self.ctx.inline_counter.set(cnt);
        let mut out = render_hoisted_blocks(
            &hoisted, indent, self.ctx, self.enums, self.registry,
            self.helpers,
        );

        // Only fold compound assignments if the original source used +=/-=/etc.
        if compound {
            if let (Expr::Var(tname), Expr::BinOp(left, op, right)) = (target, &new_value) {
                if let Expr::Var(lname) = left.as_ref() {
                    if lname == tname {
                        let op_str = match op {
                            BinOp::Add => "+=",
                            BinOp::Sub => "-=",
                            BinOp::Mul => "*=",
                            BinOp::Div => "/=",
                            BinOp::Mod
                            | BinOp::LessEq
                            | BinOp::Less
                            | BinOp::Greater
                            | BinOp::GreaterEq
                            | BinOp::Eq
                            | BinOp::NotEq
                            | BinOp::And
                            | BinOp::Or
                            | BinOp::BitwiseOr
                            | BinOp::Shr
                            | BinOp::Shl => "",
                        };
                        if !op_str.is_empty() {
                            let target_str =
                                render_assign_target(target, self.ctx, self.registry, self.helpers);
                            out.push_str(&format!(
                                "{}{}{} {};\n",
                                pad,
                                target_str,
                                op_str,
                                render_expr(right, self.ctx, self.registry, self.helpers)
                            ));
                            return out;
                        }
                    }
                }
            }
        }
        let target_str = render_assign_target(target, self.ctx, self.registry, self.helpers);
        let value_str = render_expr(&new_value, self.ctx, self.registry, self.helpers);
        out.push_str(&format!("{pad}{target_str}= {value_str};\n"));
        out
    }

    fn expr_stmt(&self, e: &Expr, indent: usize) -> String {
        let pad = " ".repeat(indent);
        // Statement-level expression: render a bare call/macro for its side effects.
        // Skip bare variable statements (no side effects — e.g. inlined identity
        // helpers). Exception: a Var containing whitespace is a verbatim
        // statement escape (the stream emitter's `goto TA_stream_capture_`).
        if let Expr::Var(v) = e {
            if v.contains(' ') {
                return format!("{pad}{v};\n");
            }
            return String::new();
        }
        if let Expr::FuncCall(fname, args) = e {
            // Check if helper inlines to a bare variable (identity helper)
            if let Some(helper) = self.helpers.get(fname) {
                if let Some(inlined) = try_inline_expr(helper, args) {
                    if matches!(inlined, Expr::Var(_)) {
                        return String::new();
                    }
                }
            }
            return format!(
                "{}{};\n",
                pad,
                render_func_call(fname, args, self.ctx, self.registry, self.helpers)
            );
        }
        String::new()
    }

    fn while_loop(&self, condition: &Expr, body: &[Statement], indent: usize) -> String {
        let pad = " ".repeat(indent);
        let mut hoisted = Vec::new();
        let mut cnt = self.ctx.inline_counter.get();
        let new_cond = hoist_block_helpers(condition, self.helpers, &mut hoisted, &mut cnt, C_CANDLE_MACRO_FNS);
        self.ctx.inline_counter.set(cnt);
        let mut out = String::new();
        out.push_str(&render_hoisted_blocks(
            &hoisted, indent, self.ctx, self.enums, self.registry, self.helpers,
        ));
        out.push_str(&format!(
            "{}while( {} )\n{}{{\n",
            pad,
            render_expr(&new_cond, self.ctx, self.registry, self.helpers),
            pad
        ));
        for s in body {
            out.push_str(&self.walk_stmt(s, indent + 3));
        }
        out.push_str(&format!("{pad}}}\n"));
        out
    }

    fn do_while(&self, condition: &Expr, body: &[Statement], indent: usize) -> String {
        let pad = " ".repeat(indent);
        let mut out = format!("{pad}do\n{pad}{{\n");
        for s in body {
            out.push_str(&self.walk_stmt(s, indent + 3));
        }
        let mut hoisted = Vec::new();
        let mut cnt = self.ctx.inline_counter.get();
        let new_cond = hoist_block_helpers(condition, self.helpers, &mut hoisted, &mut cnt, C_CANDLE_MACRO_FNS);
        self.ctx.inline_counter.set(cnt);
        out.push_str(&render_hoisted_blocks(
            &hoisted, indent, self.ctx, self.enums, self.registry, self.helpers,
        ));
        out.push_str(&format!(
            "{}}} while( {} );\n",
            pad,
            render_expr(&new_cond, self.ctx, self.registry, self.helpers)
        ));
        out
    }

    #[allow(clippy::too_many_lines, clippy::cognitive_complexity)]
    fn if_stmt(&self, condition: &Expr, then_body: &[Statement], else_body: &[Statement], cond_comments: &[Option<Vec<String>>], indent: usize) -> String {
        let pad = " ".repeat(indent);
        // Split `if(A && B)` into nested `if(A) { if(B)` when either side
        // contains a candle macro call (ta_candlerange/ta_candleaverage).
        // This prevents the compiler from speculatively computing both sides
        // of the &&, which wastes expensive fdiv cycles on the common path
        // where the first condition fails. (Runs before the commented-condition
        // path so split decisions — and thus the emitted tokens — are unchanged.)
        if let Expr::BinOp(left, BinOp::And, right) = condition {
            if expr_directly_contains_candle_call(left)
                && expr_directly_contains_candle_call(right)
            {
                // Render as: if(left) { if(right) { then } else { els } } else { els }
                let inner_if = Statement::If {
                    condition: *right.clone(),
                    then_body: then_body.to_vec(),
                    else_body: else_body.to_vec(),
                    cond_comments: Vec::new(),
                };
                let outer_if = Statement::If {
                    condition: *left.clone(),
                    then_body: vec![inner_if],
                    else_body: else_body.to_vec(),
                    cond_comments: Vec::new(),
                };
                return self.walk_stmt(&outer_if, indent);
            }
        }

        // Inline per-operand comments: render the `&&`-chain multi-line, one
        // operand per line with its comment. Same tokens as the flat form (just
        // reformatted) plus the comments — so behaviour is unchanged.
        if !cond_comments.is_empty()
            && super::stmt_walk::flatten_and(condition).len() == cond_comments.len()
        {
            let mut hoisted = Vec::new();
            let mut cnt = self.ctx.inline_counter.get();
            let new_cond = hoist_block_helpers(condition, self.helpers, &mut hoisted, &mut cnt, C_CANDLE_MACRO_FNS);
            self.ctx.inline_counter.set(cnt);
            // These operands are re-joined with `&&`, so any operand that binds
            // looser than `&&` (an `||` chain or a ternary) must be wrapped to
            // preserve grouping — the binop hook can't see across this split.
            let and_prec = binop_prec(&BinOp::And);
            let op_strs: Vec<String> = super::stmt_walk::flatten_and(&new_cond)
                .iter()
                .map(|o| {
                    let s = render_expr(o, self.ctx, self.registry, self.helpers);
                    if expr_prec(o) < and_prec {
                        format!("({s})")
                    } else {
                        s
                    }
                })
                .collect();
            let mut out = render_hoisted_blocks(
                &hoisted, indent, self.ctx, self.enums, self.registry, self.helpers,
            );
            out.push_str(&format!("{pad}if( "));
            out.push_str(&super::stmt_walk::render_and_operands(
                &op_strs, cond_comments, &" ".repeat(indent + 4), " )", true,
            ));
            out.push_str(&format!("{pad}{{\n"));
            out.push_str(&self.render_if_tail(then_body, else_body, indent));
            return out;
        }

        let mut hoisted = Vec::new();
        let mut cnt = self.ctx.inline_counter.get();
        let new_cond = hoist_block_helpers(condition, self.helpers, &mut hoisted, &mut cnt, C_CANDLE_MACRO_FNS);
        self.ctx.inline_counter.set(cnt);
        let mut out = String::new();
        out.push_str(&render_hoisted_blocks(
            &hoisted, indent, self.ctx, self.enums, self.registry, self.helpers,
        ));
        out.push_str(&format!(
            "{}if( {} )\n{}{{\n",
            pad,
            render_expr(&new_cond, self.ctx, self.registry, self.helpers),
            pad
        ));
        out.push_str(&self.render_if_tail(then_body, else_body, indent));
        out
    }

    fn return_stmt(&self, value: &Option<Expr>, indent: usize) -> String {
        let pad = " ".repeat(indent);
        match value {
            Some(expr) => {
                let rendered = render_return_expr(expr, self.ctx, self.registry, self.helpers);
                format!("{pad}return {rendered};\n")
            }
            None => format!("{pad}return;\n"),
        }
    }

    fn for_loop(&self, var: &str, count: &Expr, body: &[Statement], indent: usize) -> String {
        let pad = " ".repeat(indent);
        let mut out = format!(
            "{}for( {} = {}; {} > 0; {}-- )\n{}{{\n",
            pad,
            var,
            render_expr(count, self.ctx, self.registry, self.helpers),
            var,
            var,
            pad
        );
        for s in body {
            out.push_str(&self.walk_stmt(s, indent + 3));
        }
        out.push_str(&format!("{pad}}}\n"));
        out
    }

    fn for_c(&self, init: &Statement, condition: &Expr, update: &Statement, body: &[Statement], indent: usize) -> String {
        let pad = " ".repeat(indent);
        let init_str = render_forc_part(
            init, self.ctx, self.enums, self.registry, self.helpers,
        );
        let update_str = render_forc_part(
            update, self.ctx, self.enums, self.registry, self.helpers,
        );
        let mut out = format!(
            "{}for( {}; {}; {} )\n{}{{\n",
            pad,
            init_str.trim(),
            render_expr(condition, self.ctx, self.registry, self.helpers),
            update_str.trim(),
            pad
        );
        for s in body {
            out.push_str(&self.walk_stmt(s, indent + 3));
        }
        out.push_str(&format!("{pad}}}\n"));
        out
    }

    #[allow(clippy::too_many_lines, clippy::cognitive_complexity)]
    fn switch(&self, expr: &Expr, cases: &[(String, Vec<Statement>)], default: &[Statement], indent: usize) -> String {
        let pad = " ".repeat(indent);
        // Detect candle rangeType switches: each case assigns to the same
        // variable. Emit as a ternary chain instead of a switch — the compiler
        // loop-unswitches ternary chains much more aggressively, producing
        // specialized tight loops like the reference library's macro expansion.
        if let Some(ternary) = try_render_switch_as_ternary(
            expr, cases, default, &pad, self.ctx, self.registry, self.helpers,
        ) {
            return ternary;
        }

        let mut out = format!(
            "{}switch( {} )\n{}{{\n",
            pad,
            render_expr(expr, self.ctx, self.registry, self.helpers),
            pad
        );
        for (label, case_body) in cases {
            let c_label = render_c_switch_label(label, self.enums);
            out.push_str(&format!("{pad}case {c_label}:\n"));
            for s in case_body {
                out.push_str(&self.walk_stmt(s, indent + 3));
            }
            out.push_str(&format!("{pad}   break;\n"));
        }
        if !default.is_empty() {
            out.push_str(&format!("{pad}default:\n"));
            for s in default {
                out.push_str(&self.walk_stmt(s, indent + 3));
            }
            out.push_str(&format!("{pad}   break;\n"));
        }
        out.push_str(&format!("{pad}}}\n"));
        out
    }
}

fn render_stmt(
    stmt: &Statement,
    indent: usize,
    ctx: &CRenderCtx,
    enums: &HashMap<String, EnumDef>,
    registry: &Registry,
    helpers: &HelperRegistry,
) -> String {
    CStmt { ctx, enums, registry, helpers }.walk_stmt(stmt, indent)
}

/// Try to render a switch as a ternary chain.
///
/// Detects the pattern where every case (and default) assigns to the SAME target variable.
/// This is the candle rangeType switch pattern. Ternary chains get loop-unswitched by
/// the compiler much more aggressively than switch statements, matching the reference
/// library's `TA_CandleRange()` macro behavior.
///
/// Returns `Some(rendered)` if the switch can be converted, `None` otherwise.
#[allow(clippy::too_many_arguments)]
fn try_render_switch_as_ternary(
    expr: &Expr,
    cases: &[(String, Vec<Statement>)],
    default: &[Statement],
    pad: &str,
    ctx: &CRenderCtx,
    registry: &Registry,
    helpers: &HelperRegistry,
) -> Option<String> {
    // Only convert switches with numeric case labels (candle rangeType pattern).
    // Enum switches (like MA's MAType dispatch) must stay as switch statements.
    for (label, _) in cases {
        if label.parse::<i32>().is_err() {
            return None;
        }
    }

    // Each case must be exactly one Assign statement to the same target.
    let mut target_name: Option<String> = None;
    let mut case_exprs: Vec<(&str, String)> = Vec::new();

    for (label, body) in cases {
        if body.len() != 1 {
            return None;
        }
        if let Statement::Assign {
            target,
            value,
            compound: false,
        } = &body[0]
        {
            let tgt = render_expr(target, ctx, registry, helpers);
            if let Some(ref prev) = target_name {
                if *prev != tgt {
                    return None; // Different targets — not a simple switch
                }
            } else {
                target_name = Some(tgt.clone());
            }
            let val = render_expr(value, ctx, registry, helpers);
            case_exprs.push((label.as_str(), val));
        } else {
            return None; // Not a simple assignment
        }
    }

    // Default must also be a single assignment to the same target (or empty)
    let default_expr = if default.len() == 1 {
        if let Statement::Assign {
            target,
            value,
            compound: false,
        } = &default[0]
        {
            let tgt = render_expr(target, ctx, registry, helpers);
            if target_name.as_ref().is_some_and(|t| *t != tgt) {
                return None;
            }
            render_expr(value, ctx, registry, helpers)
        } else {
            return None;
        }
    } else if default.is_empty() {
        "0.0".to_string()
    } else {
        return None;
    };

    let target = target_name?;
    let switch_expr = render_expr(expr, ctx, registry, helpers);

    // Build nested ternary: (expr==0 ? val0 : (expr==1 ? val1 : (expr==2 ? val2 : default)))
    let mut ternary = default_expr;
    for (label, val) in case_exprs.iter().rev() {
        ternary = format!("(({switch_expr}=={label}) ? ({val}) : ({ternary}))");
    }

    Some(format!("{pad}{target} = {ternary};\n"))
}

/// Render a switch case label for C output.
/// Looks up the label in the enum registry to generate `ENUM_CASE(Type, C_Name, Pascal)`.
/// Falls back to the raw label if not an enum variant.
fn render_c_switch_label(label: &str, enums: &HashMap<String, EnumDef>) -> String {
    if let Some((enum_name, variant)) = lookup_variant(label, enums) {
        format!(
            "ENUM_CASE({}, {}, {})",
            enum_name, variant.c_name, variant.pascal_name
        )
    } else {
        label.to_string()
    }
}

fn render_assign_target(
    expr: &Expr,
    ctx: &CRenderCtx,
    registry: &Registry,
    helpers: &HelperRegistry,
) -> String {
    match expr {
        Expr::Var(name) if name == "outBegIdx" || name == "outNBElement" => {
            format!("*{name} ")
        }
        Expr::Var(name) => format!("{name} "),
        Expr::ArrayAccess(name, idx) => {
            format!(
                "{}[{}] ",
                name,
                render_expr(idx, ctx, registry, helpers)
            )
        }
        Expr::Literal(_)
        | Expr::IntLiteral(_)
        | Expr::BinOp(_, _, _)
        | Expr::Cast(_, _)
        | Expr::Not(_)
        | Expr::FuncCall(_, _)
        | Expr::PointerDeref(_)
        | Expr::AddressOf(_)
        | Expr::PostIncrement(_)
        | Expr::PostDecrement(_)
        | Expr::PreIncrement(_)
        | Expr::PreDecrement(_)
        | Expr::Ternary(_, _, _) => render_expr(expr, ctx, registry, helpers),
    }
}

/// Render a return expression, mapping known enum values to C constants.
fn render_return_expr(
    expr: &Expr,
    ctx: &CRenderCtx,
    registry: &Registry,
    helpers: &HelperRegistry,
) -> String {
    // Handle known RetCode enum values when expressed as Var
    if let Expr::Var(name) = expr {
        return match name.as_str() {
            "SUCCESS" => "TA_SUCCESS".to_string(),
            "BadParam" => "TA_BAD_PARAM".to_string(),
            "OutOfRangeEndIndex" => "TA_OUT_OF_RANGE_END_INDEX".to_string(),
            "OutOfRangeStartIndex" => "TA_OUT_OF_RANGE_START_INDEX".to_string(),
            "ALLOC_ERR" => "TA_ALLOC_ERR".to_string(),
            "INTERNAL_ERROR" => "TA_INTERNAL_ERROR".to_string(),
            _ => render_expr(expr, ctx, registry, helpers),
        };
    }
    render_expr(expr, ctx, registry, helpers)
}

#[allow(clippy::too_many_lines)]
/// C-backend leaf formatting for the shared [`ExprEmitter`] tree-walk. Bundles the
/// render context with the registry/helper services the call-dispatch hooks need;
/// the recursion itself lives in [`ExprEmitter::walk`].
struct CExpr<'a> {
    ctx: &'a CRenderCtx<'a>,
    registry: &'a Registry,
    helpers: &'a HelperRegistry,
}

impl ExprEmitter for CExpr<'_> {
    fn var(&self, name: &str) -> String {
        match name {
            "COMPATIBILITY" => "TA_GLOBALS_COMPATIBILITY".to_string(),
            "METASTOCK" => {
                "ENUM_VALUE(Compatibility,TA_COMPATIBILITY_METASTOCK,Metastock)".to_string()
            }
            "DEFAULT" => "ENUM_VALUE(Compatibility,TA_COMPATIBILITY_DEFAULT,Default)".to_string(),
            "BAD_PARAM" => "TA_BAD_PARAM".to_string(),
            "SUCCESS" => "TA_SUCCESS".to_string(),
            "ALLOC_ERR" => "TA_ALLOC_ERR".to_string(),
            "INTERNAL_ERROR" => "TA_INTERNAL_ERROR".to_string(),
            _ => name.to_string(),
        }
    }

    fn array_access(&self, name: &str, idx: &Expr) -> String {
        let access = format!("{}[{}]", name, self.walk(idx));
        // Single-precision (TA_S_) variants take `const float` inputs but compute
        // and store in `double`. Widen each float input element as it is read so
        // the arithmetic runs in double: otherwise two float inputs combined
        // directly (e.g. TA_S_MULT's inReal0[i]*inReal1[i], or the price-average
        // sums in TA_S_MEDPRICE/TYPPRICE) round — and can overflow to inf — in
        // float before the result is widened. Reported as PR #33 (@iglesias).
        // The cast is emitted only for float inputs in the single-precision
        // variant; the double variant is byte-for-byte unchanged.
        if self.ctx.single_precision && is_input_param_name(name) {
            format!("(double){access}")
        } else {
            access
        }
    }

    fn binop(&self, left: &Expr, op: &BinOp, right: &Expr) -> String {
        let op_str = match op {
            BinOp::Add => "+",
            BinOp::Sub => "-",
            BinOp::Mul => "*",
            BinOp::Div => "/",
            BinOp::Mod => "%",
            BinOp::LessEq => "<=",
            BinOp::Less => "<",
            BinOp::Greater => ">",
            BinOp::GreaterEq => ">=",
            BinOp::Eq => "==",
            BinOp::NotEq => "!=",
            BinOp::And => "&&",
            BinOp::Or => "||",
            BinOp::BitwiseOr => "|",
            BinOp::Shr => ">>",
            BinOp::Shl => "<<",
        };
        // In single-precision variants, pointer aliasing checks compare
        // float* inputs against double* outputs/buffers. Cast both to
        // void* to avoid -Wcompare-distinct-pointer-types warnings.
        let needs_void_cast = self.ctx.single_precision
            && matches!(op, BinOp::Eq | BinOp::NotEq)
            && is_pointer_var(left)
            && is_pointer_var(right);
        if needs_void_cast {
            // Operands are plain pointer identifiers (atomic) — no wrapping.
            return format!("(void *){} {op_str} (void *){}", self.walk(left), self.walk(right));
        }
        // Parenthesize an operand only when its own operator binds *looser* than
        // this one (or ties on the right, since every binary operator here is
        // left-associative) — the minimal parens that preserve the IR grouping.
        let pp = binop_prec(op);
        let l = wrap_child(self.walk(left), left, pp, false);
        let r = wrap_child(self.walk(right), right, pp, true);
        format!("{l} {op_str} {r}")
    }

    fn cast(&self, var_type: &VarType, inner: &Expr) -> String {
        let c_type = c_type_name(var_type);
        // In single-precision variants `array_access` already widens a float input
        // element read to double. Drop a redundant source-level `(double)` around
        // such a read so we emit `(double)inX[i]`, not `(double)(double)inX[i]`.
        if self.ctx.single_precision && matches!(var_type, VarType::Real) {
            if let Expr::ArrayAccess(name, _) = inner {
                if is_input_param_name(name) {
                    return self.walk(inner);
                }
            }
        }
        // A cast binds as a prefix-unary operator; only wrap a looser-binding
        // operand (a binary op or ternary).
        let s = self.walk(inner);
        let s = if expr_prec(inner) < 12 { format!("({s})") } else { s };
        format!("({c_type}){s}")
    }

    fn not(&self, inner: &Expr) -> String {
        let s = self.walk(inner);
        if expr_prec(inner) < 12 {
            format!("!({s})")
        } else {
            format!("!{s}")
        }
    }

    fn func_call(&self, name: &str, args: &[Expr]) -> String {
        render_func_call(name, args, self.ctx, self.registry, self.helpers)
    }

    fn pointer_deref(&self, name: &str) -> String {
        format!("*{name}")
    }

    fn address_of(&self, inner: &Expr) -> String {
        // `&arr[i]` needs an lvalue. In single-precision variants `array_access`
        // would widen an input read to `(double)inX[i]` (an rvalue), so taking its
        // address is ill-formed — emit the raw element access here instead.
        if let Expr::ArrayAccess(name, idx) = inner {
            return format!("&{}[{}]", name, self.walk(idx));
        }
        format!("&{}", self.walk(inner))
    }

    fn post_increment(&self, inner: &Expr) -> String {
        format!("{}++", self.walk(inner))
    }

    fn post_decrement(&self, inner: &Expr) -> String {
        format!("{}--", self.walk(inner))
    }

    fn pre_increment(&self, inner: &Expr) -> String {
        format!("++{}", self.walk(inner))
    }

    fn pre_decrement(&self, inner: &Expr) -> String {
        format!("--{}", self.walk(inner))
    }

    fn ternary(&self, cond: &Expr, then_expr: &Expr, else_expr: &Expr) -> String {
        let c = self.walk(cond);
        let t = self.walk(then_expr);
        let e = self.walk(else_expr);
        // Wrap a compound condition (conventional and readable); the arms only
        // need wrapping when they nest another ternary.
        let c = if matches!(cond, Expr::BinOp(..) | Expr::Ternary(..)) {
            format!("({c})")
        } else {
            c
        };
        let t = if matches!(then_expr, Expr::Ternary(..)) { format!("({t})") } else { t };
        let e = if matches!(else_expr, Expr::Ternary(..)) { format!("({e})") } else { e };
        format!("{c} ? {t} : {e}")
    }
}

fn render_expr(
    expr: &Expr,
    ctx: &CRenderCtx,
    registry: &Registry,
    helpers: &HelperRegistry,
) -> String {
    CExpr { ctx, registry, helpers }.walk(expr)
}

/// Crate-visible expression rendering (used by the stream emitter for
/// private-param init expressions and identity-guard conditions).
pub(crate) fn render_expression(
    expr: &Expr,
    registry: &Registry,
    helpers: &HelperRegistry,
    inline_counter: &std::cell::Cell<usize>,
) -> String {
    let ctx = CRenderCtx { single_precision: false, inline_counter, stream_scalar_candles: false };
    render_expr(expr, &ctx, registry, helpers)
}


/// Check if an expression is a variable that likely holds a pointer (array param
/// or buffer). Used to detect pointer aliasing comparisons that need void* casts.
/// Must NOT match integer variables like outBegIdx, outNBElement, tempInteger.
/// True for a TA-Lib input-array parameter name: `in` followed by an uppercase
/// letter (e.g. `inReal`, `inHigh`, `inClose`, `inReal0`).
fn is_input_param_name(name: &str) -> bool {
    name.starts_with("in") && name.len() > 2 && name.as_bytes()[2].is_ascii_uppercase()
}

fn is_pointer_var(expr: &Expr) -> bool {
    match expr {
        Expr::Var(name) => {
            // Input array params: inReal, inHigh, inLow, inClose, inVolume, inOpenInterest
            let is_input = is_input_param_name(name);
            // Output array params: outReal*, outSlowK, outFastK, outMACDSignal, etc.
            // Exclude integer outputs: outBegIdx, outNBElement, outIdx, outNbElement
            let is_output = name.starts_with("out")
                && !name.contains("BegIdx")
                && !name.contains("NbElement")
                && !name.contains("NBElement")
                && !name.contains("Idx")
                && name != "outIdx";
            // Temp buffers: tempBuffer*, firstEMA, secondEMA, fastEMABuffer, etc.
            // Exclude: tempInteger, tempReal (scalars)
            let is_buffer = name.starts_with("tempBuffer")
                || name.starts_with("firstEMA")
                || name.starts_with("secondEMA")
                || name.starts_with("thirdEMA")
                || name.starts_with("fastEMA")
                || name.starts_with("fastMA")
                || name.starts_with("slowEMA")
                || name.starts_with("slowMA")
                || name.starts_with("tempRSI");
            is_input || is_output || is_buffer
        }
        _ => false,
    }
}

/// Check if any argument in a cross-indicator call references a function input
/// parameter (inReal, inHigh, inClose, etc.). Input params are float* in single-
/// precision context, while intermediate buffers (tempBuffer, firstEMA, etc.) are
/// always double*. This determines whether to use TA_S_ or TA_ in SP variants.
fn args_have_input_param(args: &[Expr]) -> bool {
    args.iter().any(|arg| match arg {
        Expr::Var(name) => {
            // Input parameters follow TA-Lib naming: in + uppercase letter
            // e.g., inReal, inHigh, inLow, inClose, inVolume, inOpenInterest, inReal0
            is_input_param_name(name)
        }
        _ => false,
    })
}

/// Try to render a candle helper function call as a C preprocessor macro.
///
/// Converts `ta_candlerange(SET_rangeType, inOpen[idx], ...)` → `TA_CANDLERANGE(SET, idx)`
/// and `ta_candleaverage(SET_rangeType, ..., sum, inOpen[idx], ...)` → `TA_CANDLEAVERAGE(SET, sum, idx)`.
///
/// Returns `None` if the function isn't a candle helper or args don't match the expected pattern.
fn try_render_candle_macro(
    fname: &str,
    args: &[Expr],
    ctx: &CRenderCtx,
    registry: &Registry,
    helpers: &HelperRegistry,
) -> Option<String> {
    match fname {
        "ta_candlerange" if args.len() == 5 => {
            // ta_candlerange(SET_rangeType, inOpen[idx], inHigh[idx], inLow[idx], inClose[idx])
            let setting = extract_candle_setting_name(&args[0])?;
            let idx = extract_array_index(&args[4])?;
            let idx_str = render_expr(&idx, ctx, registry, helpers);
            Some(format!("TA_CANDLERANGE({setting},{idx_str})"))
        }
        "ta_candleaverage" if args.len() == 8 => {
            // ta_candleaverage(SET_rangeType, SET_avgPeriod, SET_factor, sum,
            //                  inOpen[idx], inHigh[idx], inLow[idx], inClose[idx])
            let setting = extract_candle_setting_name(&args[0])?;
            let sum_str = render_expr(&args[3], ctx, registry, helpers);
            let idx = extract_array_index(&args[7])?;
            let idx_str = render_expr(&idx, ctx, registry, helpers);
            Some(format!("TA_CANDLEAVERAGE({setting},{sum_str},{idx_str})"))
        }
        _ => None,
    }
}

/// Render candle range/average helper calls in stream-transition context:
/// scalar OHLC args (bar values or ring reads) map onto the
/// TA_STREAM_CANDLERANGE / TA_STREAM_CANDLEAVERAGE macros of ta_utility.h.
fn try_render_stream_candle_macro(
    fname: &str,
    args: &[Expr],
    ctx: &CRenderCtx,
    registry: &Registry,
    helpers: &HelperRegistry,
) -> Option<String> {
    match fname {
        "ta_candlerange" if args.len() == 5 => {
            // ta_candlerange(SET_rangeType, o, h, l, c)
            let setting = extract_candle_setting_name(&args[0])?;
            let ohlc: Vec<String> = args[1..5]
                .iter()
                .map(|a| render_expr(a, ctx, registry, helpers))
                .collect();
            Some(format!("TA_STREAM_CANDLERANGE({setting},{})", ohlc.join(",")))
        }
        "ta_candleaverage" if args.len() == 8 => {
            // ta_candleaverage(SET_rangeType, SET_avgPeriod, SET_factor, sum, o, h, l, c)
            let setting = extract_candle_setting_name(&args[0])?;
            let sum_str = render_expr(&args[3], ctx, registry, helpers);
            let ohlc: Vec<String> = args[4..8]
                .iter()
                .map(|a| render_expr(a, ctx, registry, helpers))
                .collect();
            Some(format!(
                "TA_STREAM_CANDLEAVERAGE({setting},{sum_str},{})",
                ohlc.join(",")
            ))
        }
        _ => None,
    }
}

/// Extract the candle setting name from a `_rangeType` variable reference.
/// `Expr::Var("ShadowVeryShort_rangeType")` → `Some("ShadowVeryShort")`
fn extract_candle_setting_name(expr: &Expr) -> Option<String> {
    if let Expr::Var(name) = expr {
        name.strip_suffix("_rangeType").map(String::from)
    } else {
        None
    }
}

/// Extract the index expression from an array access node.
/// `Expr::ArrayAccess("inClose", idx_expr)` → `Some(idx_expr)`
fn extract_array_index(expr: &Expr) -> Option<Expr> {
    if let Expr::ArrayAccess(_, idx) = expr {
        Some(*idx.clone())
    } else {
        None
    }
}

/// Render a `FuncCall` expression to C code.
#[allow(clippy::too_many_lines)]
fn render_func_call(
    fname: &str,
    args: &[Expr],
    ctx: &CRenderCtx,
    registry: &Registry,
    helpers: &HelperRegistry,
) -> String {
    // C candle macros: emit preprocessor macro calls instead of expanded code.
    // This enables compiler loop-unswitching, matching the reference library.
    // Stream transitions have scalar bars (no input arrays), so they use the
    // scalar TA_STREAM_CANDLE* mirrors instead — and must NOT hit the batch
    // matcher, whose array-access pattern would false-match ring reads.
    if ctx.stream_scalar_candles {
        if let Some(macro_call) = try_render_stream_candle_macro(fname, args, ctx, registry, helpers) {
            return macro_call;
        }
    } else if let Some(macro_call) = try_render_candle_macro(fname, args, ctx, registry, helpers) {
        return macro_call;
    }

    // Check if this is a call to a helper function that can be inlined
    if let Some(helper) = helpers.get(fname) {
        if let Some(inlined_expr) = try_inline_expr(helper, args) {
            let s = render_expr(&inlined_expr, ctx, registry, helpers);
            // The caller splices this in where a call result (atomic) is
            // expected, so a non-atomic inlined body (binary op / ternary) must
            // be parenthesized to keep the surrounding grouping intact.
            return wrap_inlined(s, &inlined_expr);
        }
        // Multi-statement helpers are hoisted earlier by hoist_block_helpers.
    }

    if let Some(b) = SpecialBuiltin::from_name(fname) {
        match b {
            SpecialBuiltin::UnstablePeriod => {
                // UNSTABLE_PERIOD(RSI) -> TA_GLOBALS_UNSTABLE_PERIOD(TA_FUNC_UNST_RSI,Rsi)
                // UNSTABLE_PERIOD(FUNC_UNST_ATR) -> strip FUNC_UNST_ prefix first
                if let Some(Expr::Var(func_name)) = args.first() {
                    let base = func_name
                        .strip_prefix("FUNC_UNST_")
                        .unwrap_or(func_name);
                    let upper = base.to_uppercase();
                    let pascal = pascal_word(base);
                    return format!("TA_GLOBALS_UNSTABLE_PERIOD(TA_FUNC_UNST_{upper},{pascal})");
                }
                "TA_GLOBALS_UNSTABLE_PERIOD(0,0)".to_string()
            }
            SpecialBuiltin::Compatibility => {
                // COMPATIBILITY() -> TA_GLOBALS_COMPATIBILITY
                "TA_GLOBALS_COMPATIBILITY".to_string()
            }
            SpecialBuiltin::IsZero => {
                // IS_ZERO(x) -> TA_IS_ZERO(x)
                if let Some(arg) = args.first() {
                    let x = render_expr(arg, ctx, registry, helpers);
                    return format!("TA_IS_ZERO({x})");
                }
                "TA_IS_ZERO(0)".to_string()
            }
            SpecialBuiltin::IsZeroOrNeg => {
                // IS_ZERO_OR_NEG(x) -> TA_IS_ZERO_OR_NEG(x)
                if let Some(arg) = args.first() {
                    let x = render_expr(arg, ctx, registry, helpers);
                    return format!("TA_IS_ZERO_OR_NEG({x})");
                }
                "TA_IS_ZERO_OR_NEG(0)".to_string()
            }
            SpecialBuiltin::ArrayCopy => {
                // ARRAY_COPY(dst, dstOff, src, srcOff, count)
                if args.len() == 5 {
                    let rendered: Vec<String> = args
                        .iter()
                        .map(|a| render_expr(a, ctx, registry, helpers))
                        .collect();
                    let macro_name = if ctx.single_precision {
                        "ARRAY_MEMMOVEMIX"
                    } else {
                        "ARRAY_MEMMOVE"
                    };
                    return format!(
                        "{}({},{},{},{},{})",
                        macro_name, rendered[0], rendered[1], rendered[2], rendered[3], rendered[4]
                    );
                }
                "/* ARRAY_COPY: bad args */".to_string()
            }
            SpecialBuiltin::PerToK => {
                // PER_TO_K(period) -> (2.0 / ((double)(period) + 1.0))
                if let Some(arg) = args.first() {
                    let x = render_expr(arg, ctx, registry, helpers);
                    return format!("(2.0 / ((double)({x}) + 1.0))");
                }
                "0.0".to_string()
            }
        }
    } else if let Some(mf) = MathFn::from_name(fname) {
        // Plain C math functions — remap names where needed, then emit as C function calls.
        // fabs/ABS → fabs (from <math.h>). max/min emit the branch macros from
        // ta_utility.h (`#define min(a,b) (((a)<(b))?(a):(b))`), NOT the C99
        // fmin/fmax: the macros match the pre-cutover reference bit-for-bit, lower
        // to a branchless min/max the compiler can vectorize, and keep integer args
        // in integer arithmetic. fmin/fmax carry IEEE-754 NaN/signed-zero semantics
        // that block that lowering (and forced int→double round-trips). See #102.
        let c_name = match mf {
            MathFn::Max => "max",
            MathFn::Min => "min",
            MathFn::Abs => "fabs",
            other => other.canonical(),
        };
        let rendered: Vec<String> = args
            .iter()
            .map(|a| render_expr(a, ctx, registry, helpers))
            .collect();
        format!("{}({})", c_name, rendered.join(","))
    } else if StdlibFn::from_name(fname).is_some() {
        // C stdlib functions — pass through as-is without TA_ prefix
        let rendered: Vec<String> = args
            .iter()
            .map(|a| render_expr(a, ctx, registry, helpers))
            .collect();
        format!("{}({})", fname, rendered.join(","))
    } else {
        // Try cross-call resolution through the registry
        let resolved = registry.resolve_call(fname, Lang::C);
        let rendered: Vec<String> = args
            .iter()
            .map(|a| render_expr(a, ctx, registry, helpers))
            .collect();
        if resolved != fname {
            // Registry resolved it (e.g. sma_lookback -> TA_SMA_Lookback, sma -> TA_SMA)
            // For single-precision variants, use TA_S_ ONLY when passing user input
            // arrays (float* in SP context). Intermediate buffers are always double*,
            // so calls with those must use the double-precision TA_ variant.
            // Matches the reference code's FUNCTION_CALL vs FUNCTION_CALL_DOUBLE distinction.
            let use_sp = ctx.single_precision
                && resolved.starts_with("TA_")
                && !resolved.starts_with("TA_S_")
                && !resolved.contains("_Lookback")
                && args_have_input_param(args);
            let final_name = if use_sp {
                format!("TA_S_{}", &resolved[3..])
            } else {
                resolved
            };
            format!("{}({})", final_name, rendered.join(","))
        } else if fname.ends_with("_Lookback") {
            // Legacy: RSI_Lookback(args...) -> TA_RSI_Lookback(args...)
            format!("TA_{}({})", fname, rendered.join(","))
        } else {
            // General TA function call: SMA(...) -> TA_SMA(...) or TA_S_SMA(...) for single precision
            // Same rule: only use TA_S_ when passing user input arrays.
            let use_sp = ctx.single_precision && args_have_input_param(args);
            let prefix = if use_sp { "TA_S_" } else { "TA_" };
            format!("{}{}({})", prefix, fname, rendered.join(","))
        }
    }
}

/// Render a complex lookback body (`LookbackExpr::Code`) into C code.
fn render_lookback_code(
    stmts: &[Statement],
    validation: &str,
    enums: &HashMap<String, EnumDef>,
    registry: &Registry,
    helpers: &HelperRegistry,
) -> String {
    let mut out = String::new();
    let inline_counter = Cell::new(0);
    let ctx = &CRenderCtx { single_precision: false, inline_counter: &inline_counter, stream_scalar_candles: false };

    // Declare local variables (deduplicated)
    let mut declared_vars: Vec<String> = Vec::new();
    for stmt in stmts {
        emit_c_var_decls(stmt, &mut out, &mut declared_vars);
    }

    // Emit candle settings unpacking for lookback body. It declares locals
    // with initializers, so it must stay in the declaration block (C89
    // ordering); it reads only TA_Globals, never an optional param, so
    // running it before the validation is safe.
    let candle_used = detect_candle_settings(stmts);
    if !candle_used.is_empty() {
        out.push_str(&emit_c_unpacking(&candle_used, 3));
    }

    // Param validation comes after all declarations (C89 ordering) and
    // before any statement that could read an un-defaulted parameter.
    out.push_str(validation);

    // Emit VarDecl initializations (including those inside Block multi-var declarations)
    for stmt in stmts {
        let var_decls: Vec<&Statement> = match stmt {
            Statement::VarDecl { .. } => vec![stmt],
            Statement::Block { body } => body.iter().collect(),
            _ => vec![],
        };
        for s in var_decls {
            if let Statement::VarDecl {
                name,
                init: Some(init),
                ..
            } = s
            {
                out.push_str(&format!(
                    "   {} = {};\n",
                    name,
                    render_expr(init, ctx, registry, helpers)
                ));
            }
        }
    }

    // Render non-VarDecl statements (skip Blocks that only contain VarDecls)
    for stmt in stmts {
        if matches!(stmt, Statement::VarDecl { .. }) {
            continue;
        }
        if let Statement::Block { body } = stmt {
            if body.iter().all(|s| matches!(s, Statement::VarDecl { .. })) {
                continue;
            }
        }
        out.push_str(&render_stmt(
            stmt, 3, ctx, enums, registry, helpers,
        ));
    }

    out
}

/// Collect variable names whose first VarDecl occurrence has an initializer.
/// These are eligible for hoisted initialization. Names that first appear without init
/// are NOT included (their init comes from a later duplicate and should be emitted inline).
fn collect_first_seen_var_names(stmts: &[Statement], first_seen: &mut Vec<String>) {
    let mut all_seen: Vec<String> = Vec::new();
    for stmt in stmts {
        match stmt {
            Statement::VarDecl { name, init, .. } => {
                if !all_seen.contains(name) {
                    all_seen.push(name.clone());
                    if init.is_some() {
                        first_seen.push(name.clone());
                    }
                }
            }
            Statement::Block { body } => {
                for s in body {
                    if let Statement::VarDecl { name, init, .. } = s {
                        if !all_seen.contains(name) {
                            all_seen.push(name.clone());
                            if init.is_some() {
                                first_seen.push(name.clone());
                            }
                        }
                    }
                }
            }
            _ => {}
        }
    }
}

/// Recursively emit C variable declarations from a statement, deduplicating by name.
/// Walks into `Block` statements to handle multi-var declarations like `int i, j, k;`.
fn emit_c_var_decls(stmt: &Statement, out: &mut String, declared: &mut Vec<String>) {
    match stmt {
        Statement::VarDecl { var_type, name, .. } => {
            if declared.contains(name) {
                return;
            }
            declared.push(name.clone());
            let decl = c_decl(var_type, name);
            out.push_str(&format!("   {decl};\n"));
        }
        Statement::Block { body } => {
            for s in body {
                emit_c_var_decls(s, out, declared);
            }
        }
        // A CIRCBUF declares, hoisted to the function top: per field-split storage a
        // static stack buffer `<elem> local_<storage>[N]` and a pointer `<elem> *<storage>`,
        // plus the shared `int <id>_Idx` and `int maxIdx_<id>`. (matches ta_memory.h #else)
        Statement::CircBuf(CircBuf::Prolog {
            id,
            layout,
            static_size,
        }) => {
            for (storage, t) in circbuf_fields(id, layout) {
                let local = format!("local_{storage}");
                if !declared.contains(&local) {
                    declared.push(local.clone());
                    let arr_t = if matches!(t, VarType::Integer) {
                        VarType::IntArray(static_size.to_string())
                    } else {
                        VarType::RealArray(static_size.to_string())
                    };
                    out.push_str(&format!("   {};\n", c_decl(&arr_t, &local)));
                }
                if !declared.contains(&storage) {
                    declared.push(storage.clone());
                    let ptr_t = if matches!(t, VarType::Integer) {
                        VarType::IntPointer
                    } else {
                        VarType::RealPointer
                    };
                    out.push_str(&format!("   {};\n", c_decl(&ptr_t, &storage)));
                }
            }
            let idx = format!("{id}_Idx");
            if !declared.contains(&idx) {
                declared.push(idx.clone());
                out.push_str(&format!("   int {idx};\n"));
            }
            let maxidx = format!("maxIdx_{id}");
            if !declared.contains(&maxidx) {
                declared.push(maxidx.clone());
                out.push_str(&format!("   int {maxidx};\n"));
            }
        }
        _ => {}
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::ir;
    use crate::parser;
    use std::path::Path;

    /// Helper to load a FuncDef from the ta_codegen/input directory.
    fn load_func(name: &str) -> (FuncDef, HashMap<String, EnumDef>) {
        let base = Path::new(env!("CARGO_MANIFEST_DIR")).join("../../ta_codegen/input");
        let dir = base.join(name);
        let yaml_path = dir.join(format!("{name}.yaml"));
        let c_path = dir.join(format!("{name}.c"));

        let enums_path = base.join("enums.yaml");
        let enums = if enums_path.exists() {
            parser::enums::load_enums(&enums_path)
        } else {
            HashMap::new()
        };

        let mut func_def = parser::yaml::parse_yaml(&yaml_path);
        let parsed = parser::c_source::parse_c_source(&c_path);
        func_def.body = parsed.functions.first().unwrap().body.clone();
        func_def.lookback = Some(ir::LookbackExpr::Code(parsed.lookback_body));

        (func_def, enums)
    }

    fn make_registry() -> Registry {
        let base = Path::new(env!("CARGO_MANIFEST_DIR")).join("../../ta_codegen/input");
        Registry::from_dir(&base)
    }

    #[test]
    fn test_c_generates_all_variants() {
        let (func, enums) = load_func("sma");
        let registry = make_registry();
        let output = generate(&func, &enums, &registry, &HelperRegistry::empty());

        assert!(output.contains("TA_SMA_Lookback"), "Missing lookback");
        assert!(output.contains("TA_SMA("), "Missing guarded function");
        assert!(output.contains("TA_SMA_Unguarded("), "Missing logic function");
        // TA_INT_* macros are no longer generated
        assert!(!output.contains("TA_INT_SMA"), "Should not have TA_INT_ alias");
        assert!(output.contains("TA_S_SMA("), "Missing single-precision");
        assert!(
            output.contains("TA_S_SMA_Unguarded("),
            "Missing single-precision logic"
        );
    }

    #[test]
    fn test_c_logic_omits_range_checks() {
        let (func, enums) = load_func("sma");
        let registry = make_registry();
        let output = generate(&func, &enums, &registry, &HelperRegistry::empty());

        // Find the Logic function and verify it doesn't have range checks
        let logic_start = output
            .find("TA_SMA_Unguarded(")
            .expect("Missing logic function");
        let guarded_start = output
            .find("TA_LIB_API TA_RetCode TA_SMA(")
            .expect("Missing guarded function");

        // Extract each function body (up to next function or end)
        let logic_body = &output[logic_start..];
        let guarded_body = &output[guarded_start..logic_start];

        // Guarded function should have range checks
        assert!(
            guarded_body.contains("TA_OUT_OF_RANGE_START_INDEX"),
            "Guarded should have start index check"
        );
        assert!(
            guarded_body.contains("TA_OUT_OF_RANGE_END_INDEX"),
            "Guarded should have end index check"
        );

        // Logic function should NOT have range checks
        // Find end of the logic function body (before the next function)
        // Boundary: the single-precision variants follow the logic fn (they
        // carry no TA_LIB_API); the streaming section (TA_LIB_API-prefixed)
        // comes after those, so prefer the TA_S_ marker.
        let logic_end = logic_body
            .find("TA_RetCode TA_S_")
            .or_else(|| logic_body.find("TA_LIB_API"))
            .unwrap_or(logic_body.len());
        let logic_section = &logic_body[..logic_end];
        assert!(
            !logic_section.contains("TA_OUT_OF_RANGE_START_INDEX"),
            "Logic should not have start index check"
        );
        assert!(
            !logic_section.contains("TA_OUT_OF_RANGE_END_INDEX"),
            "Logic should not have end index check"
        );
    }

    #[test]
    fn test_c_no_int_alias() {
        // Verify TA_INT_* macros are no longer generated
        let (func, enums) = load_func("sma");
        let registry = make_registry();
        let output = generate(&func, &enums, &registry, &HelperRegistry::empty());

        assert!(
            !output.contains("#define TA_INT_"),
            "Should not generate TA_INT_ macros"
        );
        assert!(
            !output.contains("#define TA_S_INT_"),
            "Should not generate TA_S_INT_ macros"
        );
    }

    #[test]
    fn test_c_resolves_cross_calls() {
        let (func, enums) = load_func("ma");
        let registry = make_registry();
        let output = generate(&func, &enums, &registry, &HelperRegistry::empty());

        // The MA body has calls like sma_lookback() and ema_lookback()
        // In generated C, these should become TA_SMA_Lookback() and TA_EMA_Lookback()
        assert!(
            output.contains("TA_SMA_Lookback("),
            "sma_lookback should resolve to TA_SMA_Lookback"
        );
        assert!(
            output.contains("TA_EMA_Lookback("),
            "ema_lookback should resolve to TA_EMA_Lookback"
        );

        // bare sma and ema calls resolve to Unguarded (cross-indicator = skip validation)
        assert!(
            output.contains("TA_SMA_Unguarded("),
            "sma should resolve to TA_SMA_Unguarded"
        );
        assert!(
            output.contains("TA_EMA_Unguarded("),
            "ema should resolve to TA_EMA_Unguarded"
        );
    }
}
