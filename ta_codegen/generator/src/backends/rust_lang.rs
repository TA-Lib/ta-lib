use std::cell::Cell;
use std::collections::HashMap;

use crate::candle_settings::{detect_candle_settings, emit_rust_unpacking};
use crate::helper_registry::{hoist_block_helpers, try_inline_expr, HelperRegistry};
use crate::ir::{
    BinOp, CircBuf, CircBufLayout, EnumDef, Expr, FuncDef, LookbackExpr, OptInput, ParamType,
    Statement, VarType,
};
use crate::parser::enums::lookup_variant;
use crate::registry::Registry;
use super::common::{contains_alloc_err_return, expr_directly_contains_candle_call, find_sizeof_type};
use super::builtins::{MathFn, SpecialBuiltin, StdlibFn};
use super::expr_walk::ExprEmitter;
use super::stmt_walk::StatementEmitter;

/// Controls how the Rust renderer emits code.
pub struct RustRenderCtx {
    /// If true, emit a pre-loop bounds-assert preamble at the top of the body (the
    /// `_unguarded`/`_private` variants). The asserts give LLVM the proof it needs to
    /// elide the per-access bounds checks on the safe `[]` indexing that follows — the
    /// generated code never uses `unsafe`. See `gen_unguarded_or_private_func`.
    pub bounds_asserts: bool,
    /// Variable names declared as `VarType::Integer` or `VarType::Index` (usize in Rust).
    /// Used by type inference in expression rendering.
    pub index_vars: std::collections::HashSet<String>,
    /// Variable names declared as `VarType::Real` (T in Rust generic mode).
    pub real_vars: std::collections::HashSet<String>,
    /// Variable names declared as `VarType::RealPointer` or `VarType::IntPointer` (Vec<T> / Vec<i32>).
    /// These need `&name` / `&mut name[..]` conversion when passed to cross-indicator calls.
    pub vec_vars: std::collections::HashSet<String>,
    /// Variable names declared as `VarType::RealArray` (e.g., `[T; N]`).
    /// These need `&mut name` when passed in output position to cross-indicator calls.
    pub real_array_vars: std::collections::HashSet<String>,
    /// Output parameter names that are integer (i32) arrays (e.g., outInteger, outMaxIdx).
    /// Values assigned to these arrays need `as i32` cast when they are usize-typed.
    pub int_output_names: std::collections::HashSet<String>,
    /// Variable names declared as `VarType::IntPointer` (Vec<i32>).
    /// Array accesses on these produce i32 values. Assignments to these need i32 values.
    pub int_vec_vars: std::collections::HashSet<String>,
    /// If true, we're inside a lookback function (returns usize, not RetCode).
    /// Return values that are i32-typed will be cast to usize.
    pub is_lookback: bool,
    /// Variable names that are assigned negative values (e.g., `highestIdx = -1`).
    /// These are declared as `i32` instead of `usize` to preserve sentinel semantics.
    /// When used as array indices, they get `as usize` casts.
    pub sentinel_vars: std::collections::HashSet<String>,
}

impl RustRenderCtx {
    pub fn for_lookback() -> Self {
        RustRenderCtx {
            bounds_asserts: false,
            index_vars: std::collections::HashSet::new(),
            real_vars: std::collections::HashSet::new(),
            vec_vars: std::collections::HashSet::new(),
            real_array_vars: std::collections::HashSet::new(),
            int_output_names: std::collections::HashSet::new(),
            int_vec_vars: std::collections::HashSet::new(),
            is_lookback: true,
            sentinel_vars: std::collections::HashSet::new(),
        }
    }
}

/// Check if an array variable is a Vec<i32> or [i32; N] (integer array).
/// Elements produce i32 values. Assignments need i32 cast.
fn is_int_array_or_vec(name: &str, ctx: &RustRenderCtx) -> bool {
    ctx.int_vec_vars.contains(name)
        || is_int_array_var(name)
        || ctx.int_output_names.contains(name)
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
    // File-level comments carried from the input .c (e.g. contributors/history).
    for block in &func.header_comments {
        out.push_str(&super::stmt_walk::block_comment(block, 0));
        out.push('\n');
    }
    out.push_str(&gen_imports());
    out.push_str(&gen_impl_block(func, enums, registry, helpers));
    out.push_str(&gen_footer());
    out
}

fn gen_header() -> String {
    let mut out = String::new();
    out.push_str("/* TA-LIB Copyright (c) 1999-2025, Mario Fortier\n");
    out.push_str(" * All rights reserved.\n");
    out.push_str(" *\n");
    out.push_str(" * Redistribution and use in source and binary forms, with or\n");
    out.push_str(" * without modification, are permitted provided that the following\n");
    out.push_str(" * conditions are met:\n");
    out.push_str(" *\n");
    out.push_str(" * - Redistributions of source code must retain the above copyright\n");
    out.push_str(" *   notice, this list of conditions and the following disclaimer.\n");
    out.push_str(" *\n");
    out.push_str(" * - Redistributions in binary form must reproduce the above copyright\n");
    out.push_str(" *   notice, this list of conditions and the following disclaimer in\n");
    out.push_str(" *   the documentation and/or other materials provided with the\n");
    out.push_str(" *   distribution.\n");
    out.push_str(" *\n");
    out.push_str(" * - Neither name of author nor the names of its contributors\n");
    out.push_str(" *   may be used to endorse or promote products derived from this\n");
    out.push_str(" *   software without specific prior written permission.\n");
    out.push_str(" *\n");
    out.push_str(" * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS\n");
    out.push_str(" * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT\n");
    out.push_str(" * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS\n");
    out.push_str(" * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE\n");
    out.push_str(" * REGENTS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,\n");
    out.push_str(" * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES\n");
    out.push_str(" * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS\n");
    out.push_str(" * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS\n");
    out.push_str(" * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,\n");
    out.push_str(" * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE\n");
    out.push_str(" * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,\n");
    out.push_str(" * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.\n");
    out.push_str(" */\n\n");
    out.push_str("/* Important:\n");
    out.push_str(" *  This file is automatically generated by the utility ta_codegen.\n");
    out.push_str(" *  Any modifications will be lost on next execution of ta_codegen.\n");
    out.push_str(" *\n");
    out.push_str(" *  Modifications should instead be done with the \"C\" source file\n");
    out.push_str(" *  in ta-lib\\src\\ta_func\n");
    out.push_str(" */\n\n");
    out
}

fn gen_imports() -> String {
    "// Import types from parent module\n\
     use super::*;\n\n"
        .to_string()
}

fn gen_impl_block(func: &FuncDef, enums: &HashMap<String, EnumDef>, registry: &Registry, helpers: &HelperRegistry) -> String {
    let mut out = String::new();
    let snake = func.name.to_lowercase();

    out.push_str(
        "// Allow non-snake-case names to maintain TA-Lib API compatibility\n\
         #[allow(non_snake_case)]\n\
         #[allow(unused_variables)]\n\
         #[allow(dead_code)]\n\
         #[allow(unused_mut)]\n\
         #[allow(unused_assignments)]\n\
         impl Core {\n",
    );

    out.push_str(&gen_lookback(func, &snake, enums, registry, helpers));

    // Guarded: validates params, delegates to unguarded
    out.push_str(&gen_guarded_func(func, &snake, enums, registry, helpers));

    // Build a temporary FuncDef with private_body for the unguarded/private variants
    let mut body_func = func.clone();
    body_func.body.clone_from(&func.private_body);

    // Collect variable type info from private body for type inference
    let mut index_vars = std::collections::HashSet::new();
    let mut real_vars = std::collections::HashSet::new();
    let mut vec_vars = std::collections::HashSet::new();
    let mut real_array_vars = std::collections::HashSet::new();
    let mut int_vec_vars = std::collections::HashSet::new();
    collect_var_types(&body_func.body, &mut index_vars, &mut real_vars, &mut vec_vars, &mut real_array_vars, &mut int_vec_vars);
    // Also add parameter names
    index_vars.insert("startIdx".to_string());
    index_vars.insert("endIdx".to_string());
    index_vars.insert("outBegIdx".to_string());
    index_vars.insert("outNBElement".to_string());

    // Pre-scan for sentinel variables (assigned -1) — these must be i32, not usize
    let mut sentinel_vars = std::collections::HashSet::new();
    collect_sentinel_vars(&body_func.body, &mut sentinel_vars);
    // Also detect integer variables that participate in signed arithmetic (< 0, 0 - N)
    collect_signed_int_vars(&body_func.body, &index_vars, &mut sentinel_vars);
    // Remove sentinel/signed vars from index_vars — they're i32, not usize
    for sv in &sentinel_vars {
        index_vars.remove(sv);
    }

    // Collect integer output names for i32 cast detection
    let int_output_set: std::collections::HashSet<String> = func.outputs.iter()
        .filter(|o| o.param_type == ParamType::Integer)
        .map(|o| o.name.clone())
        .collect();

    let ctx = RustRenderCtx {
        bounds_asserts: true,
        index_vars,
        real_vars,
        vec_vars,
        real_array_vars,
        int_output_names: int_output_set,
        int_vec_vars,
        is_lookback: false,
        sentinel_vars,
    };

    // For functions with explicit _private:
    // 1. Generate _private (extra params, private_body) — generic, handles f32/f64
    // 2. Generate _unguarded (same signature as guarded, delegates to _private via body)
    // For functions without _private:
    // 1. Generate _unguarded (private_body = copy of body, same as before)
    if func.has_explicit_private {
        // `_private` holds the algorithm (Rust has no single-precision variant) —
        // keep its comments. The thin `_unguarded` delegate duplicates the guarded
        // body, so strip its comments.
        out.push_str(&gen_private_func(
            &body_func, &snake, &ctx, enums, registry, helpers,
        ));
        let mut unguarded = func.clone();
        unguarded.body = super::stmt_walk::strip_comments(&func.body);
        out.push_str(&gen_unguarded_func(
            &unguarded, &snake, &ctx, enums, registry, helpers,
        ));
    } else {
        // `_unguarded` duplicates the guarded body — strip its comments so they
        // appear only in the guarded variant.
        let mut unguarded = body_func.clone();
        unguarded.body = super::stmt_walk::strip_comments(&body_func.body);
        out.push_str(&gen_unguarded_func(
            &unguarded, &snake, &ctx, enums, registry, helpers,
        ));
    }

    out.push_str("}\n");
    out
}

fn gen_lookback(
    func: &FuncDef,
    snake: &str,
    enums: &HashMap<String, EnumDef>,
    registry: &Registry,
    helpers: &HelperRegistry,
) -> String {
    let mut out = String::new();
    out.push_str(&super::rust_doc::lookback_docs(func, snake, enums));

    let has_opt_inputs = !func.optional_inputs.is_empty();

    // Emit the lookback `return` expression — identical for the with-params and
    // no-params signatures.
    let emit_lookback_return = |out: &mut String| match &func.lookback {
        Some(LookbackExpr::Literal(n)) => {
            out.push_str(&format!("        return {n};\n"));
        }
        Some(LookbackExpr::ParamMinus(param, offset)) => {
            out.push_str(&format!("        return ({param} - {offset}) as usize;\n"));
        }
        Some(LookbackExpr::Code(stmts)) => {
            out.push_str(&render_lookback_code(stmts, enums, registry, helpers));
        }
        None => {
            out.push_str("        return 0;\n");
        }
    };

    if has_opt_inputs {
        // Build parameter list
        let mut params = Vec::new();
        for opt in &func.optional_inputs {
            let rust_type = match opt.param_type {
                ParamType::Real => "f64",
                ParamType::Integer | ParamType::Enum(_) | ParamType::Price(_) => "i32",
            };
            params.push(format!("mut {}: {}", opt.name, rust_type));
        }

        out.push_str("    #[inline]\n");
        out.push_str(&format!(
            "    pub fn {}_lookback(&self, {}) -> usize {{\n",
            snake,
            params.join(", ")
        ));

        // Param validation
        for opt in &func.optional_inputs {
            out.push_str(&gen_opt_param_validation(opt, "        ", true));
        }

        // Return lookback expression
        emit_lookback_return(&mut out);
    } else {
        out.push_str(&format!("    pub fn {snake}_lookback(&self) -> usize {{\n"));
        emit_lookback_return(&mut out);
    }

    out.push_str("    }\n");
    out
}

/// Generate the guarded public function.
/// Validates params, delegates to `{snake}_unguarded`.
#[allow(clippy::too_many_lines, clippy::cognitive_complexity)]
fn gen_guarded_func(
    func: &FuncDef,
    snake: &str,
    enums: &HashMap<String, EnumDef>,
    registry: &Registry,
    helpers: &HelperRegistry,
) -> String {
    let mut out = String::new();

    // Doc comments + #[doc(alias)] attributes from the canonical <name>.md
    out.push_str(&super::rust_doc::guarded_docs(func, snake, enums, registry));

    // Function signature
    out.push_str(&format!("    pub fn {snake}(\n"));
    out.push_str("        &self,\n");
    out.push_str("        startIdx: usize,\n");
    out.push_str("        endIdx: usize,\n");
    out.push_str(&gen_generic_params(func));
    out.push_str("        outBegIdx: &mut usize,\n");
    out.push_str("        outNBElement: &mut usize,\n");
    out.push_str(&gen_generic_output_params(func));
    out.push_str("    ) -> RetCode {\n");

    // Range check
    out.push_str("        if endIdx < startIdx {\n");
    out.push_str("            return RetCode::OutOfRangeStartIndex;\n");
    out.push_str("        }\n");

    // Param validation
    for opt in &func.optional_inputs {
        out.push_str(&gen_opt_param_validation(opt, "        ", false));
    }

    // Output-distinctness (issue #108): aliasing two different output buffers has
    // no correct result. The borrow checker already forbids a safe caller from
    // passing the same `&mut` slice twice, so this only guards the unsafe/FFI
    // boundary; it is kept for parity with the C/Java/.NET backends. Input ==
    // output aliasing stays allowed.
    if func.outputs.len() >= 2 {
        let mut pairs: Vec<String> = Vec::new();
        for i in 0..func.outputs.len() {
            for j in (i + 1)..func.outputs.len() {
                pairs.push(format!(
                    "{}.as_ptr() == {}.as_ptr()",
                    func.outputs[i].name, func.outputs[j].name
                ));
            }
        }
        out.push_str(&format!("        if {} {{\n", pairs.join(" || ")));
        out.push_str("            return RetCode::BadParam;\n");
        out.push_str("        }\n");
    }

    if func.has_explicit_private {
        // The guarded body already contains delegation to _unguarded with extra params.
        // Render it directly (it includes pre-computation + delegation call).
        let mut g_index_vars = std::collections::HashSet::new();
        let mut g_real_vars = std::collections::HashSet::new();
        let mut g_vec_vars = std::collections::HashSet::new();
        let mut g_real_array_vars = std::collections::HashSet::new();
        let mut g_int_vec_vars = std::collections::HashSet::new();
        collect_var_types(&func.body, &mut g_index_vars, &mut g_real_vars, &mut g_vec_vars, &mut g_real_array_vars, &mut g_int_vec_vars);
        g_index_vars.insert("startIdx".to_string());
        g_index_vars.insert("endIdx".to_string());
        let mut g_sentinel_vars = std::collections::HashSet::new();
        collect_sentinel_vars(&func.body, &mut g_sentinel_vars);
        collect_signed_int_vars(&func.body, &g_index_vars, &mut g_sentinel_vars);
        for sv in &g_sentinel_vars {
            g_index_vars.remove(sv);
        }
        let g_int_output_set: std::collections::HashSet<String> = func.outputs.iter()
            .filter(|o| o.param_type == ParamType::Integer)
            .map(|o| o.name.clone())
            .collect();
        let g_ctx = RustRenderCtx {
            bounds_asserts: false,
            index_vars: g_index_vars,
            real_vars: g_real_vars,
            vec_vars: g_vec_vars,
            real_array_vars: g_real_array_vars,
            int_output_names: g_int_output_set,
            int_vec_vars: g_int_vec_vars,
            is_lookback: false,
            sentinel_vars: g_sentinel_vars,
        };
        let g_for_loop_vars = collect_for_loop_vars(&func.body);
        let g_var_inits: std::collections::HashMap<String, &Expr> = func
            .body
            .iter()
            .filter_map(|s| {
                if let Statement::VarDecl { name, init: Some(init), .. } = s {
                    Some((name.clone(), init))
                } else {
                    None
                }
            })
            .collect();
        let g_output_names: Vec<String> = func.outputs.iter().map(|o| o.name.clone()).collect();
        let g_opt_real_params: Vec<String> = func.optional_inputs.iter()
            .filter(|o| o.param_type == ParamType::Real)
            .map(|o| o.name.clone())
            .collect();
        let g_inline_counter = std::cell::Cell::new(0);
        // For explicit guarded bodies, emit VarDecls as let bindings first
        for stmt in &func.body {
            if let Statement::CircBuf(CircBuf::Prolog {
                id,
                layout,
                static_size,
            }) = stmt
            {
                out.push_str(&emit_circbuf_prolog_rust(id, layout, *static_size));
                continue;
            }
            if let Statement::VarDecl { var_type, name, init } = stmt {
                let rust_type = match var_type {
                    VarType::Integer => "i32",
                    VarType::Index => if g_ctx.sentinel_vars.contains(name) { "i32" } else { "usize" },
                    VarType::Real | VarType::RetCodeType | VarType::RealPointer
                    | VarType::IntPointer | VarType::RealArray(_) | VarType::IntArray(_) => "f64",
                };
                if let Some(init_expr) = init {
                    let rendered = render_expr(init_expr, &g_ctx, &g_opt_real_params, registry, helpers);
                    out.push_str(&format!("        let mut {name}: {rust_type} = {rendered};\n"));
                } else {
                    out.push_str(&format!("        let mut {name}: {rust_type} = 0 as {rust_type};\n"));
                }
            }
        }
        // Render non-VarDecl statements
        for stmt in &func.body {
            if matches!(stmt, Statement::VarDecl { .. }) {
                continue;
            }
            out.push_str(&render_statement(
                stmt, 8, &g_ctx, &g_for_loop_vars, &g_var_inits,
                &g_output_names, &g_opt_real_params, enums, registry, helpers, &g_inline_counter,
            ));
        }
    } else {
        // Render algorithm body inline with safe [] indexing (no get_unchecked).
        // The signature takes immutable startIdx, but the body mutates it,
        // so rebind as mutable.
        out.push_str("        let mut startIdx = startIdx;\n");

        let mut g_index_vars = std::collections::HashSet::new();
        let mut g_real_vars = std::collections::HashSet::new();
        let mut g_vec_vars = std::collections::HashSet::new();
        let mut g_real_array_vars = std::collections::HashSet::new();
        let mut g_int_vec_vars = std::collections::HashSet::new();
        collect_var_types(&func.body, &mut g_index_vars, &mut g_real_vars, &mut g_vec_vars, &mut g_real_array_vars, &mut g_int_vec_vars);
        g_index_vars.insert("startIdx".to_string());
        g_index_vars.insert("endIdx".to_string());
        g_index_vars.insert("outBegIdx".to_string());
        g_index_vars.insert("outNBElement".to_string());
        let mut g_sentinel_vars = std::collections::HashSet::new();
        collect_sentinel_vars(&func.body, &mut g_sentinel_vars);
        collect_signed_int_vars(&func.body, &g_index_vars, &mut g_sentinel_vars);
        for sv in &g_sentinel_vars {
            g_index_vars.remove(sv);
        }
        let g_int_output_set: std::collections::HashSet<String> = func.outputs.iter()
            .filter(|o| o.param_type == ParamType::Integer)
            .map(|o| o.name.clone())
            .collect();
        let g_ctx = RustRenderCtx {
            bounds_asserts: false,
            index_vars: g_index_vars,
            real_vars: g_real_vars,
            vec_vars: g_vec_vars,
            real_array_vars: g_real_array_vars,
            int_output_names: g_int_output_set,
            int_vec_vars: g_int_vec_vars,
            is_lookback: false,
            sentinel_vars: g_sentinel_vars,
        };

        // Use the same full rendering as gen_unguarded_func
        let g_for_loop_vars = collect_for_loop_vars(&func.body);
        let g_var_inits: std::collections::HashMap<String, &Expr> = func
            .body
            .iter()
            .filter_map(|s| {
                if let Statement::VarDecl { name, init: Some(init), .. } = s {
                    Some((name.clone(), init))
                } else {
                    None
                }
            })
            .collect();
        let g_output_names: Vec<String> = func.outputs.iter().map(|o| o.name.clone()).collect();
        let g_opt_real_params: Vec<String> = func.optional_inputs.iter()
            .filter(|o| o.param_type == ParamType::Real)
            .map(|o| o.name.clone())
            .collect();
        let g_inline_counter = std::cell::Cell::new(0);

        // Variable declarations (same pattern as gen_unguarded_func)
        for stmt in &func.body {
            if let Statement::CircBuf(CircBuf::Prolog {
                id,
                layout,
                static_size,
            }) = stmt
            {
                out.push_str(&emit_circbuf_prolog_rust(id, layout, *static_size));
                continue;
            }
            if let Statement::VarDecl { var_type, name, .. } = stmt {
                if g_for_loop_vars.contains(name) {
                    continue;
                }
                let total_assigns = count_assignments(name, &func.body);
                let needs_mut = total_assigns > 0;
                let is_sentinel = g_ctx.sentinel_vars.contains(name);
                let rust_type = match var_type {
                    VarType::Real => "f64",
                    VarType::Integer | VarType::Index => {
                        if is_sentinel { "i32" } else { "usize" }
                    }
                    VarType::RetCodeType => "RetCode",
                    VarType::RealPointer => "Vec<f64>",
                    VarType::IntPointer => "Vec<i32>",
                    VarType::RealArray(size) => {
                        out.push_str(&format!(
                            "        let mut {name}: [f64; {size} as usize] = [0.0_f64; {size} as usize];\n"
                        ));
                        continue;
                    }
                    VarType::IntArray(size) => {
                        out.push_str(&format!(
                            "        let mut {name}: [i32; {size} as usize] = [0i32; {size} as usize];\n"
                        ));
                        continue;
                    }
                };
                let default_val = match var_type {
                    VarType::Real => "0.0_f64",
                    VarType::Integer | VarType::Index => {
                        if is_sentinel { "0_i32" } else { "0_usize" }
                    }
                    VarType::RetCodeType => "RetCode::Success",
                    VarType::RealPointer | VarType::IntPointer => "Vec::new()",
                    _ => unreachable!(),
                };
                if needs_mut {
                    out.push_str(&format!("        let mut {name}: {rust_type} = {default_val};\n"));
                } else {
                    out.push_str(&format!("        let {name}: {rust_type} = {default_val};\n"));
                }
            }
        }

        // Candle settings unpacking
        let candle_used = detect_candle_settings(&func.body);
        if !candle_used.is_empty() {
            out.push_str(&emit_rust_unpacking(&candle_used, 8));
        }

        // Body-assigned vars (for skipping VarDecl inits that get overwritten)
        let g_body_assigned: std::collections::HashSet<String> = func
            .body
            .iter()
            .filter_map(|s| {
                if let Statement::Assign { target: Expr::Var(name), .. } = s {
                    Some(name.clone())
                } else {
                    None
                }
            })
            .collect();

        // VarDecl initializations (only when not body-assigned)
        for stmt in &func.body {
            if let Statement::VarDecl { name, var_type: vt, init: Some(init) } = stmt {
                if g_for_loop_vars.contains(name) || g_body_assigned.contains(name) {
                    continue;
                }
                let mut hoisted = Vec::new();
                let mut cnt = g_inline_counter.get();
                let new_init = hoist_block_helpers(init, helpers, &mut hoisted, &mut cnt, &[]);
                g_inline_counter.set(cnt);
                out.push_str(&render_hoisted_blocks(
                    &hoisted, 8, &g_ctx, &g_for_loop_vars, &g_var_inits,
                    &g_output_names, &g_opt_real_params, enums, registry,
                    helpers, &g_inline_counter,
                ));
                let rendered_init = render_expr(&new_init, &g_ctx, &g_opt_real_params, registry, helpers);
                let wrapped_init = if (g_ctx.real_vars.contains(name) || *vt == VarType::Real) && expr_is_untyped_integer(&new_init) {
                    format!("(({rendered_init}) as f64)")
                } else {
                    rendered_init
                };
                out.push_str(&format!("        {name} = {wrapped_init};\n"));
            }
        }

        // Render body statements
        for stmt in &func.body {
            if matches!(stmt, Statement::VarDecl { .. }) {
                continue;
            }
            out.push_str(&render_statement(
                stmt, 8, &g_ctx, &g_for_loop_vars, &g_var_inits,
                &g_output_names, &g_opt_real_params, enums, registry, helpers, &g_inline_counter,
            ));
        }
    }
    out.push_str("    }\n");

    out
}

/// Generate the _private function (generic, with extra params).
/// Only generated for functions with `has_explicit_private`.
#[allow(clippy::too_many_lines)]
fn gen_private_func(
    func: &FuncDef,
    snake: &str,
    ctx: &RustRenderCtx,
    enums: &HashMap<String, EnumDef>,
    registry: &Registry,
    helpers: &HelperRegistry,
) -> String {
    gen_unguarded_or_private_func(func, snake, ctx, enums, registry, helpers, true)
}

/// Generate the unguarded function: same signature as guarded, no validation.
/// For functions without _private: real algorithm with `get_unchecked` array access.
/// For functions with _private: delegates to _private (uses guarded body).
#[allow(clippy::too_many_lines)]
fn gen_unguarded_func(
    func: &FuncDef,
    snake: &str,
    ctx: &RustRenderCtx,
    enums: &HashMap<String, EnumDef>,
    registry: &Registry,
    helpers: &HelperRegistry,
) -> String {
    gen_unguarded_or_private_func(func, snake, ctx, enums, registry, helpers, false)
}

#[allow(clippy::too_many_lines)]
fn gen_unguarded_or_private_func(
    func: &FuncDef,
    snake: &str,
    ctx: &RustRenderCtx,
    enums: &HashMap<String, EnumDef>,
    registry: &Registry,
    helpers: &HelperRegistry,
    is_private: bool,
) -> String {
    let mut out = String::new();
    let func_name = if is_private {
        format!("{snake}_private")
    } else {
        format!("{snake}_unguarded")
    };

    out.push_str(&super::rust_doc::unguarded_docs(func, snake, is_private));

    // Function signature — always pub fn (unsafe is contained internally)
    // #[inline] enables cross-module inlining for cross-indicator calls
    out.push_str("    #[inline]\n");
    out.push_str(&format!("    pub fn {func_name}(\n"));
    out.push_str("        &self,\n");
    out.push_str("        mut startIdx: usize,\n");
    out.push_str("        endIdx: usize,\n");
    out.push_str(&gen_generic_params(func));
    // Extra params only on _private variant (e.g., EMA's k factor)
    if is_private {
        for (param_name, c_type) in &func.private_extra_params {
            let rust_type = match c_type.as_str() {
                "double" => "f64",
                "int" => "i32",
                other => panic!("Unknown C type '{other}' for extra param '{param_name}'"),
            };
            out.push_str(&format!("        {param_name}: {rust_type},\n"));
        }
    }
    out.push_str("        outBegIdx: &mut usize,\n");
    out.push_str("        outNBElement: &mut usize,\n");
    out.push_str(&gen_generic_output_params(func));
    out.push_str("    ) -> RetCode {\n");

    // Declare local variables (excluding loop iterators consumed by for-loops)
    let for_loop_vars = collect_for_loop_vars(&func.body);
    let var_inits: std::collections::HashMap<String, &Expr> = func
        .body
        .iter()
        .filter_map(|s| {
            if let Statement::VarDecl {
                name,
                init: Some(init),
                ..
            } = s
            {
                Some((name.clone(), init))
            } else {
                None
            }
        })
        .collect();

    // Collect names of OptInput Real params for generic wrapping
    let opt_real_params: Vec<String> = func
        .optional_inputs
        .iter()
        .filter(|o| o.param_type == ParamType::Real)
        .map(|o| o.name.clone())
        .collect();

    for stmt in &func.body {
        if let Statement::CircBuf(CircBuf::Prolog {
            id,
            layout,
            static_size,
        }) = stmt
        {
            out.push_str(&emit_circbuf_prolog_rust(id, layout, *static_size));
            continue;
        }
        if let Statement::VarDecl { var_type, name, .. } = stmt {
            if for_loop_vars.contains(name) {
                continue;
            }
            let total_assigns = count_assignments(name, &func.body);
            // With default initialization, the let itself is an assignment,
            // so any body assignment means we need mut (threshold is > 0, not > 1)
            let needs_mut = total_assigns > 0;
            match var_type {
                VarType::RealArray(size) => {
                    let elem = "0.0_f64";
                    let ty = "f64";
                    out.push_str(&format!(
                        "        let mut {name}: [{ty}; {size} as usize] = [{elem}; {size} as usize];\n"
                    ));
                }
                VarType::IntArray(size) => {
                    out.push_str(&format!(
                        "        let mut {name}: [i32; {size} as usize] = [0i32; {size} as usize];\n"
                    ));
                }
                _ => {
                    // Sentinel vars (assigned -1) are i32, not usize
                    let is_sentinel = ctx.sentinel_vars.contains(name);
                    let rust_type = match var_type {
                        VarType::Real => {
                            "f64"
                        }
                        VarType::Integer | VarType::Index => {
                            if is_sentinel { "i32" } else { "usize" }
                        }
                        VarType::RetCodeType => "RetCode",
                        VarType::RealPointer => {
                            "Vec<f64>"
                        }
                        VarType::IntPointer => "Vec<i32>",
                        VarType::RealArray(_) | VarType::IntArray(_) => unreachable!(),
                    };
                    // Always initialize to avoid "used binding isn't initialized" errors
                    let default_val = match var_type {
                        VarType::Real => {
                            "0.0_f64"
                        }
                        VarType::Integer | VarType::Index => {
                            if is_sentinel { "0_i32" } else { "0_usize" }
                        }
                        VarType::RetCodeType => "RetCode::Success",
                        VarType::RealPointer | VarType::IntPointer => "Vec::new()",
                        VarType::RealArray(_) | VarType::IntArray(_) => unreachable!(),
                    };
                    if needs_mut {
                        out.push_str(&format!("        let mut {name}: {rust_type} = {default_val};\n"));
                    } else {
                        out.push_str(&format!("        let {name}: {rust_type} = {default_val};\n"));
                    }
                }
            }
        }
    }

    // Emit candle settings unpacking (only for referenced settings)
    let candle_used = detect_candle_settings(&func.body);
    if !candle_used.is_empty() {
        out.push_str(&emit_rust_unpacking(&candle_used, 8));
    }

    // Collect output array names for cast insertion
    let output_names: Vec<String> = func.outputs.iter().map(|o| o.name.clone()).collect();
    // (int_output_names tracked via ctx.int_output_names for i32 array cast detection)

    // Collect variables that have both VarDecl init AND a body assignment
    let body_assigned: std::collections::HashSet<String> = func
        .body
        .iter()
        .filter_map(|s| {
            if let Statement::Assign {
                target: Expr::Var(name),
                ..
            } = s
            {
                Some(name.clone())
            } else {
                None
            }
        })
        .collect();

    let inline_counter = Cell::new(0);

    // Emit the pre-loop bounds-assert preamble for the unguarded/private variants.
    if ctx.bounds_asserts {
        // Pre-loop bounds assertions: give LLVM proof that endIdx is within all
        // input/output array bounds. This enables loop unswitching and vectorization
        // and lets LLVM elide the per-access bounds checks on the safe `[]` indexing
        // in the body — no `unsafe` needed. O(1) per call.
        for input in &func.inputs {
            out.push_str(&format!(
                "        assert!(endIdx < {}.len());\n", input.name
            ));
        }
        // Output arrays are sized by the caller for the elements actually
        // written: endIdx - max(startIdx, lookback) + 1. Internal callers pass
        // exactly-sized buffers with startIdx below the lookback (e.g. the
        // re-based EMA chaining in TEMA/DEMA/T3 reached through MA/MACDEXT),
        // so the bound must use the adjusted start — still exactly the proof
        // LLVM needs to elide the write bounds checks. When the adjusted start
        // exceeds endIdx the function writes nothing and any length is fine.
        if !func.outputs.is_empty() {
            let lb_args: Vec<String> =
                func.optional_inputs.iter().map(|o| o.name.clone()).collect();
            out.push_str(&format!(
                "        let _assertLb = self.{snake}_lookback({});\n",
                lb_args.join(", ")
            ));
            out.push_str(
                "        let _assertStart = if startIdx > _assertLb { startIdx } else { _assertLb };\n",
            );
            for output in &func.outputs {
                out.push_str(&format!(
                    "        assert!(_assertStart > endIdx || endIdx - _assertStart < {}.len());\n",
                    output.name
                ));
            }
        }
    }

    // Emit VarDecl initializations only when there's no body assignment for the same var
    for stmt in &func.body {
        if let Statement::VarDecl {
            name,
            var_type: vt,
            init: Some(init),
        } = stmt
        {
            if for_loop_vars.contains(name) {
                continue;
            }
            if body_assigned.contains(name) {
                continue;
            }
            // Hoist multi-statement helpers from init expressions
            let mut hoisted = Vec::new();
            let mut cnt = inline_counter.get();
            let new_init = hoist_block_helpers(init, helpers, &mut hoisted, &mut cnt, &[]);
            inline_counter.set(cnt);
            out.push_str(&render_hoisted_blocks(
                &hoisted, 8, ctx, &for_loop_vars, &var_inits,
                &output_names, &opt_real_params, enums, registry,
                helpers, &inline_counter,
            ));
            let rendered_init = render_expr(&new_init, ctx, &opt_real_params, registry, helpers);
            let wrapped_init = if (ctx.real_vars.contains(name) || *vt == VarType::Real) && expr_is_untyped_integer(&new_init) {
                format!("(({rendered_init}) as f64)")
            } else {
                rendered_init
            };
            out.push_str(&format!("        {name} = {wrapped_init};\n"));
        }
    }

    // Render body statements
    for stmt in &func.body {
        if matches!(stmt, Statement::VarDecl { .. }) {
            continue;
        }
        out.push_str(&render_statement(
            stmt,
            8,
            ctx,
            &for_loop_vars,
            &var_inits,
            &output_names,
            &opt_real_params,
            enums,
            registry,
            helpers,
            &inline_counter,
        ));
    }

    out.push_str("    }\n");

    out
}

/// Generate generic input parameter declarations for a function signature.
fn gen_generic_params(func: &FuncDef) -> String {
    let mut out = String::new();
    for input in &func.inputs {
        let param_type = match input.param_type {
            ParamType::Real => "&[f64]",
            ParamType::Integer | ParamType::Enum(_) | ParamType::Price(_) => "&[i32]",
        };
        out.push_str(&format!("        {}: {},\n", input.name, param_type));
    }
    for opt in &func.optional_inputs {
        let rust_type = match opt.param_type {
            ParamType::Real => "f64",
            ParamType::Integer | ParamType::Enum(_) | ParamType::Price(_) => "i32",
        };
        out.push_str(&format!("        mut {}: {},\n", opt.name, rust_type));
    }
    out
}

/// Generate generic output parameter declarations for a function signature.
fn gen_generic_output_params(func: &FuncDef) -> String {
    let mut out = String::new();
    for output in &func.outputs {
        let param_type = match output.param_type {
            ParamType::Real => "&mut [f64]",
            ParamType::Integer | ParamType::Enum(_) | ParamType::Price(_) => "&mut [i32]",
        };
        out.push_str(&format!("        {}: {},\n", output.name, param_type));
    }
    out
}

/// Generate optional parameter validation code.
fn gen_opt_param_validation(opt: &OptInput, pad: &str, is_lookback: bool) -> String {
    let mut out = String::new();
    let name = &opt.name;

    if opt.param_type == ParamType::Integer {
        if let Some(default) = opt.default {
            out.push_str(&format!("{pad}if (({name}) as i32) == (i32::MIN) {{\n"));
            #[allow(clippy::cast_possible_truncation)]
            let default_i64 = default as i64;
            out.push_str(&format!("{pad}    {name} = {default_i64};\n"));

            if let Some((lo, hi)) = opt.range {
                let err_return = if is_lookback {
                    "return usize::MAX;"
                } else {
                    "return RetCode::BadParam;"
                };
                out.push_str(&format!(
                    "{pad}}} else if ((({name}) as i32) < {lo}) || ((({name}) as i32) > {hi}) {{\n"
                ));
                out.push_str(&format!("{pad}    {err_return}\n"));
            }

            out.push_str(&format!("{pad}}}\n"));
        }
    }

    out
}

/// Collect variable type information from VarDecl statements (recursively).
/// The heap-backed storage buffers for a CIRCBUF. `Plain` is a single `Vec` named
/// `<id>`; `Class` is one parallel `Vec` per struct field named `<id>_<field>`
/// (matching the `CIRCBUF_REF` access flatten). Returns `(storage_name, element_type)`
/// pairs. Mirrors `c::circbuf_fields`, but Rust always heaps (no stack-opt).
fn circbuf_storage(id: &str, layout: &CircBufLayout) -> Vec<(String, VarType)> {
    match layout {
        CircBufLayout::Plain(t) => vec![(id.to_string(), t.clone())],
        CircBufLayout::Class(fields) => fields
            .iter()
            .map(|(f, t)| (format!("{id}_{f}"), t.clone()))
            .collect(),
    }
}

/// Emit the function-top declarations for a CIRCBUF prolog: an empty `Vec` per
/// field-split storage, the `usize` rotation index, and the `usize` bound. The bound
/// is seeded to `static_size - 1` (NOT 0) so the `INIT_LOCAL_ONLY` path (HT functions)
/// sizes its buffer correctly before any `INIT` runs. Indent is the 8-space body level.
fn emit_circbuf_prolog_rust(id: &str, layout: &CircBufLayout, static_size: i64) -> String {
    let mut s = String::new();
    for (storage, t) in circbuf_storage(id, layout) {
        let vt = if matches!(t, VarType::Integer) {
            "i32"
        } else {
            "f64"
        };
        s.push_str(&format!(
            "        let mut {storage}: Vec<{vt}> = Vec::new();\n"
        ));
    }
    s.push_str(&format!("        let mut {id}_Idx: usize = 0;\n"));
    s.push_str(&format!(
        "        let mut maxIdx_{id}: usize = {};\n",
        static_size - 1
    ));
    s
}

#[allow(clippy::too_many_lines)]
fn collect_var_types(
    body: &[Statement],
    index_vars: &mut std::collections::HashSet<String>,
    real_vars: &mut std::collections::HashSet<String>,
    vec_vars: &mut std::collections::HashSet<String>,
    real_array_vars: &mut std::collections::HashSet<String>,
    int_vec_vars: &mut std::collections::HashSet<String>,
) {
    for stmt in body {
        match stmt {
            Statement::VarDecl { var_type, name, .. } => {
                match var_type {
                    VarType::Integer | VarType::Index => { index_vars.insert(name.clone()); }
                    VarType::Real => { real_vars.insert(name.clone()); }
                    VarType::RealPointer => { vec_vars.insert(name.clone()); }
                    VarType::IntPointer => { vec_vars.insert(name.clone()); int_vec_vars.insert(name.clone()); }
                    VarType::IntArray(_) => { index_vars.insert(name.clone()); int_vec_vars.insert(name.clone()); }
                    VarType::RealArray(_) => { real_array_vars.insert(name.clone()); }
                    VarType::RetCodeType => {}
                }
            }
            Statement::If { then_body, else_body, .. } => {
                collect_var_types(then_body, index_vars, real_vars, vec_vars, real_array_vars, int_vec_vars);
                collect_var_types(else_body, index_vars, real_vars, vec_vars, real_array_vars, int_vec_vars);
            }
            Statement::While { body: while_body, .. }
            | Statement::DoWhile { body: while_body, .. } => {
                collect_var_types(while_body, index_vars, real_vars, vec_vars, real_array_vars, int_vec_vars);
            }
            Statement::For { body: for_body, .. }
            | Statement::ForC { body: for_body, .. } => {
                collect_var_types(for_body, index_vars, real_vars, vec_vars, real_array_vars, int_vec_vars);
            }
            Statement::Block { body: block_body } => {
                collect_var_types(block_body, index_vars, real_vars, vec_vars, real_array_vars, int_vec_vars);
            }
            Statement::Switch { cases, default, .. } => {
                for (_, case_body) in cases {
                    collect_var_types(case_body, index_vars, real_vars, vec_vars, real_array_vars, int_vec_vars);
                }
                collect_var_types(default, index_vars, real_vars, vec_vars, real_array_vars, int_vec_vars);
            }
            // A CIRCBUF prolog declares Vec storage (+ usize index/bound) at the function
            // top; classify the names so get_unchecked / usize inference apply.
            Statement::CircBuf(CircBuf::Prolog { id, layout, .. }) => {
                for (storage, t) in circbuf_storage(id, layout) {
                    vec_vars.insert(storage.clone());
                    if matches!(t, VarType::Integer) {
                        int_vec_vars.insert(storage);
                    }
                }
                index_vars.insert(format!("{id}_Idx"));
                index_vars.insert(format!("maxIdx_{id}"));
            }
            _ => {}
        }
    }
}

/// Check if an expression is `0 - 1` (unary minus parsed as `BinOp(IntLiteral(0), Sub, IntLiteral(1))`).
fn is_negative_one(expr: &Expr) -> bool {
    matches!(
        expr,
        Expr::BinOp(left, BinOp::Sub, right)
            if matches!(left.as_ref(), Expr::IntLiteral(0))
            && matches!(right.as_ref(), Expr::IntLiteral(1))
    )
}

/// Check if an expression can produce a negative integer value.
/// Catches: `0 - N`, `-N` literal, `X * (... ? 1 : (0-N))`, ternary with negative branch.
fn expr_can_be_negative(expr: &Expr) -> bool {
    match expr {
        // 0 - N is negative
        Expr::BinOp(left, BinOp::Sub, _right) => {
            matches!(left.as_ref(), Expr::IntLiteral(0))
        }
        // Multiplication or addition where either side can be negative
        Expr::BinOp(left, BinOp::Mul | BinOp::Add, right) => {
            expr_can_be_negative(left) || expr_can_be_negative(right)
        }
        // Ternary: if either branch can be negative
        Expr::Ternary(_, then_e, else_e) => {
            expr_can_be_negative(then_e) || expr_can_be_negative(else_e)
        }
        // Negative integer literal
        Expr::IntLiteral(n) if *n < 0 => true,
        _ => false,
    }
}

/// Check if a condition compares an integer variable against 0 using `< 0`
/// (vacuously false for unsigned types, indicating the variable must be signed).
/// Only considers variables known to be integer-typed (in `int_vars`).
fn condition_implies_signed(
    condition: &Expr,
    int_vars: &std::collections::HashSet<String>,
    signed_vars: &mut std::collections::HashSet<String>,
) {
    match condition {
        // var < 0 (only meaningful for signed types — always false for usize)
        Expr::BinOp(left, BinOp::Less, right) => {
            if let Expr::Var(name) = left.as_ref() {
                if matches!(right.as_ref(), Expr::IntLiteral(0)) && int_vars.contains(name) {
                    signed_vars.insert(name.clone());
                }
            }
        }
        // Boolean AND / OR: recurse into both sides
        Expr::BinOp(left, BinOp::And | BinOp::Or, right) => {
            condition_implies_signed(left, int_vars, signed_vars);
            condition_implies_signed(right, int_vars, signed_vars);
        }
        Expr::Not(inner) => {
            condition_implies_signed(inner, int_vars, signed_vars);
        }
        _ => {}
    }
}

/// Scan function body for integer variables that require signed (i32) representation.
/// Only considers variables known to be integer-typed (in `int_vars`).
/// Extends `signed_vars` with variables assigned potentially-negative values or
/// compared with `< 0`.
fn collect_signed_int_vars(
    body: &[Statement],
    int_vars: &std::collections::HashSet<String>,
    signed_vars: &mut std::collections::HashSet<String>,
) {
    for stmt in body {
        match stmt {
            // Variable initialized with a negative expression
            Statement::VarDecl {
                var_type: VarType::Integer | VarType::Index,
                name,
                init: Some(init),
            } if expr_can_be_negative(init) => {
                signed_vars.insert(name.clone());
            }
            // Integer variable assigned a negative expression
            Statement::Assign { target: Expr::Var(name), value, .. }
                if int_vars.contains(name) && expr_can_be_negative(value) =>
            {
                signed_vars.insert(name.clone());
            }
            // Condition checking `var < 0` (only on integer vars)
            Statement::If { condition, then_body, else_body, .. } => {
                condition_implies_signed(condition, int_vars, signed_vars);
                collect_signed_int_vars(then_body, int_vars, signed_vars);
                collect_signed_int_vars(else_body, int_vars, signed_vars);
            }
            Statement::While { condition, body: inner, .. }
            | Statement::DoWhile { condition, body: inner, .. } => {
                condition_implies_signed(condition, int_vars, signed_vars);
                collect_signed_int_vars(inner, int_vars, signed_vars);
            }
            Statement::For { body: inner, .. } => {
                collect_signed_int_vars(inner, int_vars, signed_vars);
            }
            Statement::ForC { init, condition, body: inner, .. } => {
                condition_implies_signed(condition, int_vars, signed_vars);
                collect_signed_int_vars(&[init.as_ref().clone()], int_vars, signed_vars);
                collect_signed_int_vars(inner, int_vars, signed_vars);
            }
            Statement::Block { body: block_body } => {
                collect_signed_int_vars(block_body, int_vars, signed_vars);
            }
            Statement::Switch { cases, default, .. } => {
                for (_, case_body) in cases {
                    collect_signed_int_vars(case_body, int_vars, signed_vars);
                }
                collect_signed_int_vars(default, int_vars, signed_vars);
            }
            _ => {}
        }
    }
}

/// Pre-scan function body for variables assigned negative values (sentinel pattern).
fn collect_sentinel_vars(
    body: &[Statement],
    sentinel_vars: &mut std::collections::HashSet<String>,
) {
    for stmt in body {
        match stmt {
            Statement::VarDecl {
                var_type: VarType::Integer | VarType::Index,
                name,
                init: Some(init),
            } if is_negative_one(init) => {
                sentinel_vars.insert(name.clone());
            }
            Statement::Assign { target, value, .. } => {
                if is_negative_one(value) {
                    if let Expr::Var(name) = target {
                        sentinel_vars.insert(name.clone());
                    }
                }
            }
            Statement::Block { body: block_body } => {
                for s in block_body {
                    if let Statement::Assign { target: Expr::Var(name), value, .. } = s {
                        if is_negative_one(value) {
                            sentinel_vars.insert(name.clone());
                        }
                    }
                }
                for s in block_body {
                    if let Statement::Assign { target: Expr::Var(name), value: Expr::Var(vname), .. } = s {
                        if sentinel_vars.contains(vname) && !sentinel_vars.contains(name) {
                            sentinel_vars.insert(name.clone());
                        }
                    }
                }
                collect_sentinel_vars(block_body, sentinel_vars);
            }
            Statement::If { then_body, else_body, .. } => {
                collect_sentinel_vars(then_body, sentinel_vars);
                collect_sentinel_vars(else_body, sentinel_vars);
            }
            Statement::While { body: inner, .. }
            | Statement::DoWhile { body: inner, .. }
            | Statement::For { body: inner, .. }
            | Statement::ForC { body: inner, .. } => {
                collect_sentinel_vars(inner, sentinel_vars);
            }
            Statement::Switch { cases, default, .. } => {
                for (_, case_body) in cases {
                    collect_sentinel_vars(case_body, sentinel_vars);
                }
                collect_sentinel_vars(default, sentinel_vars);
            }
            _ => {}
        }
    }
}

/// Count how many times a variable is assigned in the body (including `VarDecl` inits).
fn count_assignments(name: &str, body: &[Statement]) -> usize {
    count_assignments_inner(name, body, false)
}

/// Count increment/decrement operations on a variable embedded in expressions.
fn count_increments_in_expr(name: &str, expr: &Expr) -> usize {
    match expr {
        // &var in a function call means the variable will be mutably borrowed
        Expr::PostIncrement(inner) | Expr::PostDecrement(inner)
        | Expr::PreIncrement(inner) | Expr::PreDecrement(inner)
        | Expr::AddressOf(inner) => {
            if let Expr::Var(vname) = inner.as_ref() {
                if vname == name { return 1; }
            }
            count_increments_in_expr(name, inner)
        }
        Expr::BinOp(left, _, right) => {
            count_increments_in_expr(name, left) + count_increments_in_expr(name, right)
        }
        Expr::ArrayAccess(_, idx) => count_increments_in_expr(name, idx),
        Expr::FuncCall(_, args) => args.iter().map(|a| count_increments_in_expr(name, a)).sum(),
        Expr::Not(inner) | Expr::Cast(_, inner) => {
            count_increments_in_expr(name, inner)
        }
        Expr::Ternary(cond, then_expr, else_expr) => {
            count_increments_in_expr(name, cond)
                + count_increments_in_expr(name, then_expr)
                + count_increments_in_expr(name, else_expr)
        }
        _ => 0,
    }
}

fn count_assignments_inner(name: &str, body: &[Statement], in_loop: bool) -> usize {
    let mut count = 0;
    for stmt in body {
        match stmt {
            Statement::VarDecl {
                name: vname, init, ..
            } => {
                if vname == name && init.is_some() {
                    count += 1;
                }
            }
            Statement::Assign { target, value, .. } => {
                if let Expr::Var(tname) = target {
                    if tname == name {
                        count += if in_loop { 2 } else { 1 };
                    }
                }
                // Also count increment/decrement embedded in expressions
                count += count_increments_in_expr(name, target);
                count += count_increments_in_expr(name, value);
            }
            Statement::Expr(e) => {
                // Count increment/decrement embedded in a statement expression
                count += count_increments_in_expr(name, e);
            }
            Statement::While {
                body: while_body, ..
            }
            | Statement::DoWhile {
                body: while_body, ..
            } => {
                count += count_assignments_inner(name, while_body, true);
            }
            Statement::For { body: for_body, .. } => {
                count += count_assignments_inner(name, for_body, true);
            }
            Statement::ForC { init, update, body: for_body, .. } => {
                count += count_assignments_inner(name, &[init.as_ref().clone()], in_loop);
                count += count_assignments_inner(name, &[update.as_ref().clone()], true);
                count += count_assignments_inner(name, for_body, true);
            }
            Statement::Block { body: block_body } => {
                count += count_assignments_inner(name, block_body, in_loop);
            }
            Statement::If {
                then_body,
                else_body,
                ..
            } => {
                count += count_assignments_inner(name, then_body, in_loop);
                count += count_assignments_inner(name, else_body, in_loop);
            }
            Statement::Return { .. }
            | Statement::Break
            | Statement::Continue
            | Statement::CircBuf(_)
            | Statement::Comment(_) => {}
            Statement::Switch { cases, default, .. } => {
                for (_, case_body) in cases {
                    count += count_assignments_inner(name, case_body, in_loop);
                }
                count += count_assignments_inner(name, default, in_loop);
            }
        }
    }
    count
}

fn collect_for_loop_vars(body: &[Statement]) -> Vec<String> {
    let mut vars = Vec::new();
    let decls: std::collections::HashMap<String, &Expr> = body
        .iter()
        .filter_map(|s| {
            if let Statement::VarDecl {
                name,
                init: Some(init),
                var_type,
            } = s
            {
                if *var_type == VarType::Index {
                    return Some((name.clone(), init));
                }
            }
            None
        })
        .collect();

    for stmt in body {
        match stmt {
            Statement::While {
                condition,
                body: while_body,
            } => {
                if let Some(iter_var) = detect_for_pattern(condition, while_body, &decls) {
                    vars.push(iter_var);
                }
            }
            Statement::For { .. }
            | Statement::ForC { .. }
            | Statement::Block { .. }
            | Statement::VarDecl { .. }
            | Statement::Assign { .. }
            | Statement::Expr(_)
            | Statement::If { .. }
            | Statement::Return { .. }
            | Statement::Break
            | Statement::Continue
            | Statement::Switch { .. }
            | Statement::DoWhile { .. }
            | Statement::CircBuf(_)
            | Statement::Comment(_) => {}
        }
    }
    vars
}

fn detect_for_pattern(
    condition: &Expr,
    while_body: &[Statement],
    decls: &std::collections::HashMap<String, &Expr>,
) -> Option<String> {
    if let Expr::BinOp(left, BinOp::LessEq, _right) = condition {
        if let Expr::Var(iter_name) = left.as_ref() {
            if decls.contains_key(iter_name) {
                if let Some(Statement::Assign {
                    target: Expr::Var(tname),
                    value,
                    ..
                }) = while_body.last()
                {
                    if tname == iter_name {
                        if let Expr::BinOp(l, BinOp::Add, r) = value {
                            if let (Expr::Var(ln), Expr::IntLiteral(1)) = (l.as_ref(), r.as_ref()) {
                                if ln == iter_name {
                                    return Some(iter_name.clone());
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    None
}

/// Extracts the init value if `stmt` is a single `Assign { target: Var(var_name), value, .. }`.
/// Returns `None` for multi-assignment blocks or mismatched targets.
fn extract_init_value<'a>(stmt: &'a Statement, var_name: &str) -> Option<&'a Expr> {
    if let Statement::Assign {
        target: Expr::Var(tname),
        value,
        ..
    } = stmt
    {
        if tname == var_name {
            return Some(value);
        }
    }
    None
}

/// Extract the variable name from an init statement (handles Block wrapping too).
fn extract_init_var(stmt: &Statement) -> Option<String> {
    match stmt {
        Statement::Assign {
            target: Expr::Var(name),
            ..
        } => Some(name.clone()),
        Statement::Block { body } if body.len() == 1 => extract_init_var(&body[0]),
        _ => None,
    }
}

/// Returns `true` if `stmt` is `var_name = var_name + 1` (simple increment by 1).
fn is_simple_increment(stmt: &Statement, var_name: &str) -> bool {
    if let Statement::Assign {
        target: Expr::Var(tname),
        value: Expr::BinOp(left, BinOp::Add, right),
        ..
    } = stmt
    {
        if tname != var_name {
            return false;
        }
        if let Expr::Var(lname) = left.as_ref() {
            if lname == var_name {
                return matches!(right.as_ref(), Expr::IntLiteral(1));
            }
        }
    }
    false
}

/// Returns `true` if `stmt` is `var_name = var_name - 1` or `--var_name` (simple decrement by 1).
fn is_simple_decrement(stmt: &Statement, var_name: &str) -> bool {
    match stmt {
        Statement::Assign {
            target: Expr::Var(tname),
            value: Expr::BinOp(left, BinOp::Sub, right),
            ..
        } => {
            if tname != var_name {
                return false;
            }
            if let Expr::Var(lname) = left.as_ref() {
                if lname == var_name {
                    return matches!(right.as_ref(), Expr::IntLiteral(1));
                }
            }
            false
        }
        // Pre-decrement: for(...; ...; --var)
        Statement::Assign {
            target: Expr::Var(tname),
            value: Expr::PreDecrement(inner),
            ..
        } => {
            if let Expr::Var(vname) = inner.as_ref() {
                return tname == var_name && vname == var_name;
            }
            false
        }
        _ => false,
    }
}


/// Render hoisted block-inline helpers as Rust code (temp var decl + body).
#[allow(clippy::too_many_arguments)]
fn render_hoisted_blocks(
    hoisted: &[(String, VarType, Vec<Statement>)],
    indent: usize,
    ctx: &RustRenderCtx,
    for_loop_vars: &[String],
    var_inits: &std::collections::HashMap<String, &Expr>,
    output_names: &[String],
    opt_real_params: &[String],
    enums: &HashMap<String, EnumDef>,
    registry: &Registry,
    helpers: &HelperRegistry,
    inline_counter: &Cell<usize>,
) -> String {
    let pad = " ".repeat(indent);
    let mut out = String::new();
    for (temp_name, var_type, body) in hoisted {
        let decl_line = match var_type {
            VarType::RealArray(size) => {
                let elem = "0.0_f64";
                let ty = "f64";
                format!("{pad}let mut {temp_name}: [{ty}; {size} as usize] = [{elem}; {size} as usize];\n")
            }
            VarType::IntArray(size) => {
                format!("{pad}let mut {temp_name}: [i32; {size} as usize] = [0i32; {size} as usize];\n")
            }
            _ => {
                let rust_type = match var_type {
                    VarType::Real => {
                        "f64"
                    }
                    VarType::Integer | VarType::Index => "usize",
                    VarType::RetCodeType => "RetCode",
                    VarType::RealPointer => {
                        "Vec<f64>"
                    }
                    VarType::IntPointer => "Vec<i32>",
                    VarType::RealArray(_) | VarType::IntArray(_) => unreachable!(),
                };
                format!("{pad}let mut {temp_name}: {rust_type};\n")
            }
        };
        out.push_str(&decl_line);
        // Emit VarDecl statements from the hoisted body with their init expressions.
        // These are local vars of the helper function that need inline declaration.
        // Using init expressions is critical: e.g., `double range = th - tl` in
        // ta_true_range must initialize range to the actual value, not zero.
        for stmt in body {
            if let Statement::VarDecl { var_type: vt, name, init } = stmt {
                let local_type = match vt {
                    VarType::Real => {
                        "f64"
                    }
                    VarType::Integer | VarType::Index => "usize",
                    VarType::RetCodeType => "RetCode",
                    VarType::RealPointer => {
                        "Vec<f64>"
                    }
                    VarType::IntPointer => "Vec<i32>",
                    VarType::RealArray(size) => {
                        out.push_str(&format!(
                            "{pad}let mut {name}: [{ty}; {size} as usize] = [{elem}; {size} as usize];\n",
                            ty = "f64",
                            elem = "0.0_f64",
                        ));
                        continue;
                    }
                    VarType::IntArray(size) => {
                        out.push_str(&format!(
                            "{pad}let mut {name}: [i32; {size} as usize] = [0i32; {size} as usize];\n"
                        ));
                        continue;
                    }
                };
                // Use the init expression if available, otherwise use a type default
                let init_str = if let Some(init_expr) = init {
                    render_expr(init_expr, ctx, opt_real_params, registry, helpers)
                } else {
                    match vt {
                        VarType::Real => {
                            "0.0_f64"
                        }
                        VarType::Integer | VarType::Index => "0_usize",
                        VarType::RetCodeType => "RetCode::Success",
                        VarType::RealPointer | VarType::IntPointer => "Vec::new()",
                        VarType::RealArray(_) | VarType::IntArray(_) => unreachable!(),
                    }.to_string()
                };
                out.push_str(&format!("{pad}let mut {name}: {local_type} = {init_str};\n"));
            }
        }
        // Render non-VarDecl statements from the body (skip VarDecls since they
        // were already emitted above with proper init expressions)
        for stmt in body {
            if matches!(stmt, Statement::VarDecl { .. }) {
                continue;
            }
            out.push_str(&render_statement(
                stmt,
                indent,
                ctx,
                for_loop_vars,
                var_inits,
                output_names,
                opt_real_params,
                enums,
                registry,
                helpers,
                inline_counter,
            ));
        }
    }
    out
}

/// Rust-backend leaf formatting for the shared [`StatementEmitter`] tree-walk.
/// Bundles the render context with the for-loop/var-init/output-name/opt-real
/// state and the enum/registry/helper services the hooks need; the recursion and
/// variant dispatch live in [`StatementEmitter::walk_stmt`].
struct RustStmt<'a, 'e> {
    ctx: &'a RustRenderCtx,
    for_loop_vars: &'a [String],
    var_inits: &'a HashMap<String, &'e Expr>,
    output_names: &'a [String],
    opt_real_params: &'a [String],
    enums: &'a HashMap<String, EnumDef>,
    registry: &'a Registry,
    helpers: &'a HelperRegistry,
    inline_counter: &'a Cell<usize>,
}

impl RustStmt<'_, '_> {
    /// Shared `if` tail (then-body + else branch with `} else if` collapse) used
    /// by both the flat and multi-line-condition rendering paths.
    fn render_if_tail(&self, then_body: &[Statement], else_body: &[Statement], indent: usize) -> String {
        let pad = " ".repeat(indent);
        let mut out = String::new();
        for s in then_body {
            out.push_str(&self.walk_stmt(s, indent + 4));
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
            out.push_str(&format!("{pad}}} else {{\n"));
            for s in else_body {
                out.push_str(&self.walk_stmt(s, indent + 4));
            }
            out.push_str(&format!("{pad}}}\n"));
        }
        out
    }
}

impl StatementEmitter for RustStmt<'_, '_> {
    fn comment(&self, lines: &[String], indent: usize) -> String {
        super::stmt_walk::line_comment(lines, indent)
    }

    fn circ_buf(&self, op: &CircBuf, indent: usize) -> String {
        let pad = " ".repeat(indent);
        match op {
            // Prolog: storage + index/bound declared at the function top by the decl pass.
            // Destroy: Vec storage drops automatically — no explicit free.
            CircBuf::Prolog { .. } | CircBuf::Destroy { .. } => String::new(),
            // Advance with conditional reset (not modulo) — matches the reference macro.
            CircBuf::Next { id } => {
                format!("{pad}{id}_Idx += 1;\n{pad}if {id}_Idx > maxIdx_{id} {{ {id}_Idx = 0; }}\n")
            }
            // Runtime-sized: (re)allocate each storage Vec to `size` (always heap).
            CircBuf::Init { id, layout, size } => {
                let sz = render_expr(
                    size,
                    self.ctx,
                    self.opt_real_params,
                    self.registry,
                    self.helpers,
                );
                let mut s = String::new();
                // Parity with the reference CIRCBUF_INIT guard (ta_memory.h _RUST branch);
                // also prevents the `(sz as usize) - 1` underflow on the unguarded path.
                s.push_str(&format!(
                    "{pad}if {sz} < 1 {{ return RetCode::AllocErr; }}\n"
                ));
                for (storage, t) in circbuf_storage(id, layout) {
                    let zero = if matches!(t, VarType::Integer) {
                        "0i32"
                    } else {
                        "0.0_f64"
                    };
                    s.push_str(&format!(
                        "{pad}{storage} = vec![{zero}; ({sz}) as usize];\n"
                    ));
                }
                s.push_str(&format!("{pad}maxIdx_{id} = (({sz}) as usize) - 1;\n"));
                s.push_str(&format!("{pad}{id}_Idx = 0;\n"));
                s
            }
            // Always the static capacity; bound was seeded in the prolog (maxIdx + 1).
            CircBuf::InitLocalOnly { id, layout } => {
                let mut s = String::new();
                for (storage, t) in circbuf_storage(id, layout) {
                    let zero = if matches!(t, VarType::Integer) {
                        "0i32"
                    } else {
                        "0.0_f64"
                    };
                    s.push_str(&format!(
                        "{pad}{storage} = vec![{zero}; maxIdx_{id} + 1];\n"
                    ));
                }
                s.push_str(&format!("{pad}{id}_Idx = 0;\n"));
                s
            }
        }
    }

    #[allow(clippy::too_many_lines, clippy::cognitive_complexity)]
    fn var_decl(&self, var_type: &VarType, name: &str, init: &Option<Expr>, indent: usize) -> String {
        let pad = " ".repeat(indent);
        // VarDecl at function top-level is handled by the separate declaration pass.
        // VarDecl inside blocks/loops/ifs needs inline declaration.
        // We always emit here; the top-level pass skips VarDecls to avoid duplicates.
        // The top-level code pre-emits declarations for all body-level VarDecls,
        // so if this VarDecl is at top level, this is a no-op duplicate that will
        // be filtered by the caller. For nested VarDecls (inside blocks), this is needed.
        if indent <= 8 {
            // Top-level VarDecl already handled
            return String::new();
        }
        // Sentinel vars (assigned -1) are i32, not usize
        let is_sentinel = self.ctx.sentinel_vars.contains(name);
        let rust_type = match var_type {
            VarType::Real => {
                "f64"
            }
            VarType::Integer | VarType::Index => {
                if is_sentinel { "i32" } else { "usize" }
            }
            VarType::RetCodeType => "RetCode",
            VarType::RealPointer => {
                "Vec<f64>"
            }
            VarType::IntPointer => "Vec<i32>",
            VarType::RealArray(size) => {
                let elem = "0.0_f64";
                let ty = "f64";
                return format!("{pad}let mut {name}: [{ty}; {size} as usize] = [{elem}; {size} as usize];\n");
            }
            VarType::IntArray(size) => {
                return format!("{pad}let mut {name}: [i32; {size} as usize] = [0i32; {size} as usize];\n");
            }
        };
        // Use init expression if available, otherwise fall back to type default
        let init_str = if let Some(init_expr) = init {
            render_expr(init_expr, self.ctx, self.opt_real_params, self.registry, self.helpers)
        } else {
            match var_type {
                VarType::Real => {
                    "0.0_f64"
                }
                VarType::Integer | VarType::Index => {
                    if is_sentinel { "0_i32" } else { "0_usize" }
                }
                VarType::RetCodeType => "RetCode::Success",
                VarType::RealPointer | VarType::IntPointer => "Vec::new()",
                VarType::RealArray(_) | VarType::IntArray(_) => unreachable!(),
            }
            .to_string()
        };
        format!("{pad}let mut {name}: {rust_type} = {init_str};\n")
    }

    #[allow(clippy::too_many_lines, clippy::cognitive_complexity)]
    fn assign(&self, target: &Expr, value: &Expr, compound: bool, indent: usize) -> String {
        let pad = " ".repeat(indent);
        // Split arr[idx++] = value into arr[idx] = value; idx += 1;
        // This enables LLVM auto-vectorization by exposing idx as a clean
        // linear induction variable (the block expression { let _v = idx; idx += 1; _v }
        // creates an opaque dependency that prevents vectorization).
        if let Expr::ArrayAccess(arr_name, idx_expr) = target {
            if let Expr::PostIncrement(inner) = idx_expr.as_ref() {
                let stripped_target = Expr::ArrayAccess(
                    arr_name.clone(),
                    Box::new(inner.as_ref().clone()),
                );
                let stripped_assign = Statement::Assign {
                    target: stripped_target,
                    value: value.clone(),
                    compound,
                };
                let mut out = self.walk_stmt(&stripped_assign, indent);
                let idx_str = render_expr(inner, self.ctx, self.opt_real_params, self.registry, self.helpers);
                out.push_str(&format!("{pad}{idx_str} += 1;\n"));
                return out;
            }
        }
        // Also split arr[idx++] patterns in value-side array reads:
        // prevMA = inReal[today++] becomes prevMA = inReal[today]; today += 1;
        // This is handled by splitting PostIncrement out of the value expression.
        // (The target-side split above is the most impactful for vectorization.)

        // Hoist multi-statement helpers from the value expression
        let mut hoisted = Vec::new();
        let mut cnt = self.inline_counter.get();
        let new_value = hoist_block_helpers(
            value, self.helpers, &mut hoisted, &mut cnt, &[],
        );
        self.inline_counter.set(cnt);
        let mut out = render_hoisted_blocks(
            &hoisted, indent, self.ctx, self.for_loop_vars, self.var_inits,
            self.output_names, self.opt_real_params, self.enums, self.registry,
            self.helpers, self.inline_counter,
        );

        // Emit dummy variable declaration for duplicate &mut borrows in cross-indicator calls
        if has_duplicate_address_of(&new_value) {
            out.push_str(&format!("{pad}let mut _dup_out: usize = 0_usize;\n"));
        }

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
                                render_assign_target(target, self.ctx, self.opt_real_params, self.registry, self.helpers);
                            let rhs_str = render_expr(right, self.ctx, self.opt_real_params, self.registry, self.helpers);
                            // Check if the target is T-typed (Real variable)
                            let target_is_real = self.ctx.real_vars.contains(tname)
                                || (!self.ctx.index_vars.contains(tname)
                                    && !is_likely_index_var(tname)
                                    && !is_i32_opt_in_param(tname)
                                    && !tname.ends_with("_avgPeriod")
                                    && !tname.ends_with("_rangeType"));
                            // Cast integer RHS in compound assignments to f64-typed variables
                            let rhs_wrapped = if target_is_real {
                                if expr_is_untyped_integer(right) || expr_is_i32_typed(right)
                                    || (expr_is_known_usize_ctx(right, self.ctx) && !expr_is_float_typed_ctx(right, Some(self.ctx)))
                                {
                                    format!("(({rhs_str}) as f64)")
                                } else {
                                    rhs_str
                                }
                            } else if !target_is_real
                                && is_likely_index_var(tname)
                                && expr_is_i32_typed(right)
                            {
                                // usize target, i32 RHS: cast to usize
                                format!("({rhs_str}) as usize")
                            } else {
                                rhs_str
                            };
                            out.push_str(&format!(
                                "{pad}{target_str} {op_str} {rhs_wrapped};\n"
                            ));
                            return out;
                        }
                    }
                }
            }
        }
        let target_str = render_assign_target(target, self.ctx, self.opt_real_params, self.registry, self.helpers);
        let value_str = render_expr(&new_value, self.ctx, self.opt_real_params, self.registry, self.helpers);
        let needs_f64_cast = if let Expr::ArrayAccess(name, _) = target {
            self.output_names.contains(name)
                && !self.ctx.int_output_names.contains(name)
                && expr_has_uncast_array_access(&new_value)
        } else {
            false
        };
        // Sentinel var assignment: cast usize value to i32 when assigning to sentinel target
        let needs_sentinel_i32_cast = if let Expr::Var(tname) = target {
            self.ctx.sentinel_vars.contains(tname)
                && !expr_is_i32_typed_ctx(&new_value, self.ctx)
                && !matches!(new_value, Expr::IntLiteral(_))
                && !is_negative_one(&new_value)
                && (expr_is_known_usize_ctx(&new_value, self.ctx)
                    || matches!(new_value, Expr::Var(ref v) if self.ctx.index_vars.contains(v) || is_likely_index_var(v)))
        } else {
            false
        };
        // Sentinel var used as value: cast i32 sentinel to usize when assigning to usize target
        let needs_sentinel_usize_cast = if let Expr::Var(tname) = target {
            !self.ctx.sentinel_vars.contains(tname)
                && (self.ctx.index_vars.contains(tname) || is_likely_index_var(tname))
                && expr_is_i32_typed_ctx(&new_value, self.ctx)
                && !expr_is_i32_typed(&new_value)
                && !matches!(new_value, Expr::IntLiteral(_))
        } else {
            false
        };
        // Check if we're assigning an i32-typed expression to a non-i32 target variable
        // (e.g., usize var = optInTimePeriod which is i32,
        //  or curPeriod = localPeriodArray[i] where array is Vec<i32>)
        let value_is_i32 = expr_is_i32_typed(&new_value)
            || matches!(new_value, Expr::Var(ref v) if is_i32_opt_in_param(v) || v.ends_with("_avgPeriod") || v.ends_with("_rangeType"))
            || matches!(&new_value, Expr::ArrayAccess(ref name, _) if is_int_array_or_vec(name, self.ctx));
        let needs_usize_cast = if let Expr::Var(tname) = target {
            !self.output_names.iter().any(|n| n == tname)
                && !is_i32_opt_in_param(tname)
                && !tname.ends_with("_avgPeriod")
                && !tname.ends_with("_rangeType")
                && !self.ctx.real_vars.contains(tname)
                && !self.ctx.sentinel_vars.contains(tname)
                && value_is_i32
        } else {
            false
        };
        // Check if target is an i32 optIn param and value is usize
        // (e.g., optInFastPeriod = tempInteger where tempInteger is usize)
        let needs_optin_i32_cast = if let Expr::Var(tname) = target {
            is_i32_opt_in_param(tname)
                && !expr_is_i32_typed(&new_value)
                && !matches!(new_value, Expr::IntLiteral(_))
                && (expr_is_known_usize_ctx(&new_value, self.ctx)
                    || matches!(new_value, Expr::Var(ref v) if self.ctx.index_vars.contains(v) || is_likely_index_var(v)))
        } else {
            false
        };
        // Check if target is an integer output/local array (e.g., outInteger[idx] = usize_val,
        // sortedPeriods[i] = longestPeriod, localPeriodArray[i] = tempInt)
        // Values assigned to i32 arrays need `as i32` cast when usize-typed
        let needs_int_output_cast = if let Expr::ArrayAccess(name, _) = target {
            (self.ctx.int_output_names.contains(name) || is_int_array_or_vec(name, self.ctx))
                && !expr_is_i32_typed(&new_value)
                && !matches!(new_value, Expr::IntLiteral(_))
                && (expr_is_known_usize_ctx(&new_value, self.ctx)
                    || matches!(&new_value, Expr::Var(v) if self.ctx.index_vars.contains(v) || is_likely_index_var(v))
                    || matches!(&new_value, Expr::BinOp(_, _, _)))
        } else {
            false
        };
        // Cast integer values to f64 when assigned to f64-typed variables
        let needs_f64_wrap = if let Expr::Var(tname) = target {
            // Require the target to be positively identified as Real (f64)
            let target_is_known_real = self.ctx.real_vars.contains(tname)
                || (expr_is_float_typed_ctx(&Expr::Var(tname.clone()), Some(self.ctx))
                    && !self.ctx.index_vars.contains(tname)
                    && !is_likely_index_var(tname)
                    && !is_i32_opt_in_param(tname)
                    && !self.ctx.sentinel_vars.contains(tname)
                    && !tname.ends_with("_avgPeriod")
                    && !tname.ends_with("_rangeType")
                    && !self.output_names.iter().any(|n| n == tname));
            target_is_known_real
                && (expr_is_untyped_integer(&new_value) || expr_is_i32_typed(&new_value)
                    || expr_is_known_usize_ctx(&new_value, self.ctx))
        } else if let Expr::ArrayAccess(name, _) = target {
            // Array target: Real arrays (output, temp, local) need f64 values
            // But NOT IntArray/IntPointer targets (periods, usedFlag, localPeriodArray, etc.)
            // and NOT integer output arrays (outInteger, outMaxIdx, etc.)
            !name.contains("Int") && !name.contains("integer")
                && !self.ctx.index_vars.contains(name)
                && !is_int_array_var(name)
                && !self.ctx.int_output_names.contains(name)
                && !self.ctx.int_vec_vars.contains(name)
                && (expr_is_untyped_integer(&new_value) || expr_is_i32_typed(&new_value)
                    || expr_is_known_usize_ctx(&new_value, self.ctx))
        } else {
            false
        };
        // Check if target is a Vec<T> variable and value is a slice/output param
        // (buffer aliasing pattern in BBANDS, STOCH, etc.)
        let needs_to_vec = if let Expr::Var(tname) = target {
            if self.ctx.vec_vars.contains(tname) || is_vec_local_var(tname) {
                if let Expr::Var(vname) = &new_value {
                    self.output_names.contains(vname) || vname.starts_with("in")
                } else {
                    false
                }
            } else {
                false
            }
        } else {
            false
        };
        if needs_to_vec {
            out.push_str(&format!("{pad}{target_str} = {value_str}.to_vec();\n"));
        } else if needs_sentinel_i32_cast {
            out.push_str(&format!("{pad}{target_str} = ({value_str}) as i32;\n"));
        } else if needs_sentinel_usize_cast {
            out.push_str(&format!("{pad}{target_str} = ({value_str}) as usize;\n"));
        } else if needs_int_output_cast {
            out.push_str(&format!("{pad}{target_str} = ({value_str}) as i32;\n"));
        } else if needs_f64_cast || needs_f64_wrap {
            // IntLiteral → emit as float literal instead of cast
            if let Expr::IntLiteral(n) = &new_value {
                out.push_str(&format!("{pad}{target_str} = {n}.0;\n"));
            } else {
                out.push_str(&format!("{pad}{target_str} = (({value_str}) as f64);\n"));
            }
        } else if needs_optin_i32_cast {
            out.push_str(&format!("{pad}{target_str} = ({value_str}) as i32;\n"));
        } else if needs_usize_cast {
            out.push_str(&format!("{pad}{target_str} = ({value_str}) as usize;\n"));
        } else {
            out.push_str(&format!("{pad}{target_str} = {value_str};\n"));
        }
        out
    }

    fn expr_stmt(&self, e: &Expr, indent: usize) -> String {
        let pad = " ".repeat(indent);
        // Statement-level expression: render a bare call/macro for its side effects.
        // Skip bare variable statements (no side effects — e.g. inlined identity helpers)
        if matches!(e, Expr::Var(_)) {
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
            let rendered = render_func_call(
                fname, args, self.ctx, self.opt_real_params, self.registry, self.helpers,
            );
            // Skip empty renders (e.g. free() returns "")
            if rendered.is_empty() {
                return String::new();
            }
            return format!("{pad}{rendered};\n");
        }
        String::new()
    }

    fn while_loop(&self, condition: &Expr, while_body: &[Statement], indent: usize) -> String {
        let pad = " ".repeat(indent);
        if let Expr::BinOp(left, BinOp::LessEq, right) = condition {
            if let Expr::Var(iter_name) = left.as_ref() {
                if self.for_loop_vars.contains(iter_name) {
                    let start_expr = if let Some(init) = self.var_inits.get(iter_name) {
                        render_expr(init, self.ctx, self.opt_real_params, self.registry, self.helpers)
                    } else {
                        iter_name.clone()
                    };
                    let end_expr = render_expr(right, self.ctx, self.opt_real_params, self.registry, self.helpers);
                    let mut out = format!(
                        "{pad}for {iter_name} in ({start_expr} as usize)..({end_expr} as usize) + 1 {{\n"
                    );
                    for s in &while_body[..while_body.len() - 1] {
                        out.push_str(&self.walk_stmt(s, indent + 4));
                    }
                    out.push_str(&format!("{pad}}}\n"));
                    return out;
                }
            }
        }
        let mut out = format!(
            "{}while {} {{\n",
            pad,
            render_expr(condition, self.ctx, self.opt_real_params, self.registry, self.helpers)
        );
        for s in while_body {
            out.push_str(&self.walk_stmt(s, indent + 4));
        }
        out.push_str(&format!("{pad}}}\n"));
        out
    }

    fn do_while(&self, condition: &Expr, while_body: &[Statement], indent: usize) -> String {
        let pad = " ".repeat(indent);
        let mut out = format!("{pad}loop {{\n");
        for s in while_body {
            out.push_str(&self.walk_stmt(s, indent + 4));
        }
        out.push_str(&format!(
            "{}    if !({}) {{ break; }}\n",
            pad,
            render_expr(condition, self.ctx, self.opt_real_params, self.registry, self.helpers)
        ));
        out.push_str(&format!("{pad}}}\n"));
        out
    }

    #[allow(clippy::too_many_lines, clippy::cognitive_complexity)]
    fn if_stmt(&self, condition: &Expr, then_body: &[Statement], else_body: &[Statement], cond_comments: &[Option<Vec<String>>], indent: usize) -> String {
        let pad = " ".repeat(indent);
        // Skip post-allocation null-check blocks (dead code in Rust — Vec::new() never fails)
        if contains_alloc_err_return(then_body) {
            return String::new();
        }
        // Split `if(A && B)` into nested `if(A) { if(B)` when both sides
        // contain a candle function call (ta_candlerange/ta_candleaverage).
        // This prevents the compiler from speculatively computing both sides
        // of the &&, which wastes expensive fdiv cycles on the common path
        // where the first condition fails.
        if let Expr::BinOp(left, BinOp::And, right) = condition {
            if expr_directly_contains_candle_call(left)
                && expr_directly_contains_candle_call(right)
            {
                // Render as: if left { if right { then } else { els } } else { els }
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
        // Inline per-operand comments: render the `&&`-chain multi-line, brace on
        // the following line (same condition, reformatted, plus the comments).
        if !cond_comments.is_empty()
            && super::stmt_walk::flatten_and(condition).len() == cond_comments.len()
        {
            let op_strs: Vec<String> = super::stmt_walk::flatten_and(condition)
                .iter()
                .map(|o| {
                    let s = render_condition(o, self.ctx, self.opt_real_params, self.registry, self.helpers);
                    // An operand that is itself a logical `||` or a ternary binds
                    // looser than `&&`; it must stay parenthesized to preserve
                    // precedence in the chain (render_condition adds no outer
                    // parens — the flat path relied on the enclosing render).
                    if matches!(o, Expr::BinOp(_, BinOp::Or, _) | Expr::Ternary(..)) {
                        format!("({s})")
                    } else {
                        s
                    }
                })
                .collect();
            let mut out = format!("{pad}if ");
            out.push_str(&super::stmt_walk::render_and_operands(
                &op_strs, cond_comments, &" ".repeat(indent + 3), "", false,
            ));
            out.push_str(&format!("{pad}{{\n"));
            out.push_str(&self.render_if_tail(then_body, else_body, indent));
            return out;
        }
        let mut out = format!(
            "{}if {} {{\n",
            pad,
            render_condition(condition, self.ctx, self.opt_real_params, self.registry, self.helpers)
        );
        out.push_str(&self.render_if_tail(then_body, else_body, indent));
        out
    }

    fn return_stmt(&self, value: &Option<Expr>, indent: usize) -> String {
        let pad = " ".repeat(indent);
        match value {
            Some(expr) => {
                let rendered = render_return_expr(expr, self.ctx, self.opt_real_params, self.registry, self.helpers);
                // In lookback functions, return value must be usize. Cast any i32/mixed expression.
                let is_already_usize = matches!(expr, Expr::Var(ref n) if n == "retValue" || n == "lookbackTotal" || n == "emaLookback")
                    || expr_is_known_usize_ctx(expr, self.ctx)
                    || expr_returns_usize(expr);
                if self.ctx.is_lookback && !is_already_usize
                    && !matches!(expr, Expr::Var(ref n) if n == "SUCCESS" || n == "BadParam" || n.starts_with("RetCode"))
                {
                    format!("{pad}return ({rendered}) as usize;\n")
                } else {
                    format!("{pad}return {rendered};\n")
                }
            }
            None => format!("{pad}return;\n"),
        }
    }

    #[allow(clippy::too_many_lines, clippy::cognitive_complexity)]
    fn for_loop(&self, var: &str, count: &Expr, for_body: &[Statement], indent: usize) -> String {
        let pad = " ".repeat(indent);
        let mut out = format!(
            "{}for {} in (1..={}).rev() {{\n",
            pad,
            var,
            render_expr(count, self.ctx, self.opt_real_params, self.registry, self.helpers)
        );
        for s in for_body {
            out.push_str(&self.walk_stmt(s, indent + 4));
        }
        out.push_str(&format!("{pad}}}\n"));
        out
    }

    #[allow(clippy::too_many_lines, clippy::cognitive_complexity)]
    fn for_c(&self, init: &Statement, condition: &Expr, update: &Statement, for_body: &[Statement], indent: usize) -> String {
        let pad = " ".repeat(indent);
        // Range-iteration fast path: for(i=start; i<=end; i++) → for i in start..(end+1)
        // Uses exclusive range (not ..=) because LLVM vectorizes exclusive ranges
        // but generates suboptimal cinc+double-compare for inclusive ranges.
        if let Expr::BinOp(cond_left, BinOp::LessEq, cond_right) = condition {
            if let Expr::Var(iter_name) = cond_left.as_ref() {
                if let Some(start_expr) = extract_init_value(init, iter_name) {
                    if is_simple_increment(update, iter_name) {
                        let start_str = render_expr(start_expr, self.ctx, self.opt_real_params, self.registry, self.helpers);
                        let end_str = render_expr(cond_right, self.ctx, self.opt_real_params, self.registry, self.helpers);
                        let mut out = format!(
                            "{pad}for {iter_name} in ({start_str} as usize)..({end_str} as usize) + 1 {{\n"
                        );
                        for s in for_body {
                            out.push_str(&self.walk_stmt(s, indent + 4));
                        }
                        out.push_str(&format!("{pad}}}\n"));
                        // In C, after for(i=start; i<=end; i++), i == end+1.
                        // Rust's for-in leaves the variable at the last iteration value.
                        // Fixup so downstream code sees the same post-loop value.
                        out.push_str(&format!(
                            "{pad}{iter_name} = ({end_str} as usize) + 1;\n"
                        ));
                        return out;
                    }
                }
            }
        }
        // Countdown loop: for(v = start; v >= bound; v--) with usize
        // The `v -= 1` from `bound` wraps to usize::MAX, causing OOB.
        // Emit a loop-with-break pattern:
        //   v = start; loop { body; if v == bound { break; } v -= 1; }
        if let Some(iter_name) = extract_init_var(init) {
            if is_simple_decrement(update, &iter_name) {
                if let Expr::BinOp(cond_left, BinOp::GreaterEq, cond_right) = condition {
                    if let Expr::Var(cname) = cond_left.as_ref() {
                        if cname == &iter_name {
                            if let Some(start_expr) = extract_init_value(init, &iter_name) {
                                let start_str = render_expr(
                                    start_expr, self.ctx, self.opt_real_params, self.registry, self.helpers,
                                );
                                let bound_str = render_expr(
                                    cond_right, self.ctx, self.opt_real_params, self.registry, self.helpers,
                                );
                                let mut out = format!(
                                    "{pad}// for( {iter_name} = {start_str}; \
                                     {iter_name} >= {bound_str}; {iter_name} -= 1 )\n"
                                );
                                out.push_str(&format!("{pad}{iter_name} = {start_str};\n"));
                                out.push_str(&format!("{pad}loop {{\n"));
                                for s in for_body {
                                    out.push_str(&self.walk_stmt(s, indent + 4));
                                }
                                let inner_pad = " ".repeat(indent + 4);
                                out.push_str(&format!(
                                    "{inner_pad}if {iter_name} == {bound_str} {{ break; }}\n"
                                ));
                                out.push_str(&format!(
                                    "{inner_pad}{iter_name} -= 1;\n"
                                ));
                                out.push_str(&format!("{pad}}}\n"));
                                return out;
                            }
                        }
                    }
                }
            }
        }
        // Generic fallback: init; while cond { body; update; }
        // Collect init statements (may be a Block with multiple assigns)
        let init_stmts = match init {
            Statement::Block { body: block_body } => block_body.clone(),
            other => vec![other.clone()],
        };
        // Collect update statements (may be a Block with multiple assigns)
        let update_stmts = match update {
            Statement::Block { body: block_body } => block_body.clone(),
            other => vec![other.clone()],
        };
        let cond_str =
            render_expr(condition, self.ctx, self.opt_real_params, self.registry, self.helpers);
        // Build single-line comment summarizing the original C for loop
        let init_parts: Vec<String> = init_stmts
            .iter()
            .map(|s| {
                self.walk_stmt(s, 0)
                .trim()
                .trim_end_matches(';')
                .to_string()
            })
            .collect();
        let update_parts: Vec<String> = update_stmts
            .iter()
            .map(|s| {
                self.walk_stmt(s, 0)
                .trim()
                .trim_end_matches(';')
                .to_string()
            })
            .collect();
        let mut out = format!(
            "{pad}// for( {}; {cond_str}; {} )\n",
            init_parts.join(", "),
            update_parts.join(", "),
        );
        // Emit init statements
        for s in &init_stmts {
            out.push_str(&self.walk_stmt(s, indent));
        }
        out.push_str(&format!(
            "{pad}while {cond_str} {{\n"
        ));
        for s in for_body {
            out.push_str(&self.walk_stmt(s, indent + 4));
        }
        // Emit update statements inside the while body
        for s in &update_stmts {
            out.push_str(&self.walk_stmt(s, indent + 4));
        }
        out.push_str(&format!("{pad}}}\n"));
        out
    }

    #[allow(clippy::too_many_lines, clippy::cognitive_complexity)]
    fn switch(&self, expr: &Expr, cases: &[(String, Vec<Statement>)], default: &[Statement], indent: usize) -> String {
        let pad = " ".repeat(indent);
        let mut out = format!(
            "{}match {} {{\n",
            pad,
            render_expr(expr, self.ctx, self.opt_real_params, self.registry, self.helpers)
        );
        for (label, case_body) in cases {
            let rust_label = render_switch_label(label, self.enums);
            out.push_str(&format!("{pad}    {rust_label} => {{\n"));
            for s in case_body {
                out.push_str(&self.walk_stmt(s, indent + 8));
            }
            out.push_str(&format!("{pad}    }}\n"));
        }
        if !default.is_empty() {
            out.push_str(&format!("{pad}    _ => {{\n"));
            for s in default {
                out.push_str(&self.walk_stmt(s, indent + 8));
            }
            out.push_str(&format!("{pad}    }}\n"));
        }
        out.push_str(&format!("{pad}}}\n"));
        out
    }
}

#[allow(clippy::too_many_arguments, clippy::implicit_hasher)]
pub fn render_statement(
    stmt: &Statement,
    indent: usize,
    ctx: &RustRenderCtx,
    for_loop_vars: &[String],
    var_inits: &std::collections::HashMap<String, &Expr>,
    output_names: &[String],
    opt_real_params: &[String],
    enums: &HashMap<String, EnumDef>,
    registry: &Registry,
    helpers: &HelperRegistry,
    inline_counter: &Cell<usize>,
) -> String {
    RustStmt {
        ctx,
        for_loop_vars,
        var_inits,
        output_names,
        opt_real_params,
        enums,
        registry,
        helpers,
        inline_counter,
    }
    .walk_stmt(stmt, indent)
}

fn render_switch_label(label: &str, enums: &HashMap<String, EnumDef>) -> String {
    if let Some((_enum_name, variant)) = lookup_variant(label, enums) {
        format!("{}", variant.value)
    } else {
        label.to_string()
    }
}

fn expr_has_uncast_array_access(expr: &Expr) -> bool {
    match expr {
        Expr::ArrayAccess(_, _) => true,
        Expr::Cast(_, _)
        | Expr::Literal(_)
        | Expr::IntLiteral(_)
        | Expr::Var(_)
        | Expr::PointerDeref(_)
        | Expr::AddressOf(_)
        | Expr::PostIncrement(_)
        | Expr::PostDecrement(_)
        | Expr::PreIncrement(_)
        | Expr::PreDecrement(_) => false,
        Expr::BinOp(left, _, right) => {
            expr_has_uncast_array_access(left) || expr_has_uncast_array_access(right)
        }
        Expr::Not(inner) => expr_has_uncast_array_access(inner),
        Expr::FuncCall(_, args) => args.iter().any(expr_has_uncast_array_access),
        Expr::Ternary(cond, then_expr, else_expr) => {
            expr_has_uncast_array_access(cond)
                || expr_has_uncast_array_access(then_expr)
                || expr_has_uncast_array_access(else_expr)
        }
    }
}

fn render_assign_target(
    expr: &Expr,
    ctx: &RustRenderCtx,
    opt_real_params: &[String],
    registry: &Registry,
    helpers: &HelperRegistry,
) -> String {
    match expr {
        Expr::Var(name) if name == "outBegIdx" || name == "outNBElement" => {
            format!("(*{name})")
        }
        Expr::Var(name) => name.clone(),
        Expr::ArrayAccess(name, idx) => {
            let idx_rendered = render_index_expr(idx, ctx, opt_real_params, registry, helpers);
            format!("{name}[{idx_rendered}]")
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
        | Expr::Ternary(_, _, _) => render_expr(expr, ctx, opt_real_params, registry, helpers),
    }
}

fn op_precedence(op: &BinOp) -> u8 {
    match op {
        BinOp::Or => 1,
        BinOp::And => 2,
        BinOp::BitwiseOr => 3,
        BinOp::Eq | BinOp::NotEq => 4,
        BinOp::Less | BinOp::LessEq | BinOp::Greater | BinOp::GreaterEq => 5,
        BinOp::Add | BinOp::Sub | BinOp::Shr | BinOp::Shl => 6,
        BinOp::Mul | BinOp::Div | BinOp::Mod => 7,
    }
}

fn render_binop_operand(
    expr: &Expr,
    parent_op: &BinOp,
    is_left: bool,
    ctx: &RustRenderCtx,
    opt_real_params: &[String],
    registry: &Registry,
    helpers: &HelperRegistry,
) -> String {
    match expr {
        Expr::Cast(_, _) => format!("({})", render_expr(expr, ctx, opt_real_params, registry, helpers)),
        Expr::BinOp(_, child_op, _) => {
            let parent_prec = op_precedence(parent_op);
            let child_prec = op_precedence(child_op);
            if child_prec < parent_prec || (!is_left && child_prec == parent_prec) {
                format!("({})", render_expr(expr, ctx, opt_real_params, registry, helpers))
            } else {
                render_expr(expr, ctx, opt_real_params, registry, helpers)
            }
        }
        Expr::Ternary(_, _, _) if matches!(parent_op, BinOp::And | BinOp::Or) => {
            // Ternary producing integer 1/0 used in boolean context needs != 0
            let rendered = render_expr(expr, ctx, opt_real_params, registry, helpers);
            format!("({rendered} != 0)")
        }
        Expr::FuncCall(fname, args) if matches!(parent_op, BinOp::And | BinOp::Or) => {
            // Check if this is a helper function that inlines to a ternary returning 1/0
            if let Some(helper) = helpers.get(fname) {
                if let Some(inlined) = try_inline_expr(helper, args) {
                    if matches!(inlined, Expr::Ternary(_, _, _)) {
                        let rendered = render_expr(expr, ctx, opt_real_params, registry, helpers);
                        return format!("({rendered} != 0)");
                    }
                }
            }
            render_expr(expr, ctx, opt_real_params, registry, helpers)
        }
        Expr::Literal(_)
        | Expr::IntLiteral(_)
        | Expr::Var(_)
        | Expr::ArrayAccess(_, _)
        | Expr::Not(_)
        | Expr::FuncCall(_, _)
        | Expr::PointerDeref(_)
        | Expr::AddressOf(_)
        | Expr::PostIncrement(_)
        | Expr::PostDecrement(_)
        | Expr::PreIncrement(_)
        | Expr::PreDecrement(_)
        | Expr::Ternary(_, _, _) => render_expr(expr, ctx, opt_real_params, registry, helpers),
    }
}

fn render_return_expr(
    expr: &Expr,
    ctx: &RustRenderCtx,
    opt_real_params: &[String],
    registry: &Registry,
    helpers: &HelperRegistry,
) -> String {
    if let Expr::Var(name) = expr {
        return match name.as_str() {
            "SUCCESS" => "RetCode::Success".to_string(),
            "BadParam" => "RetCode::BadParam".to_string(),
            "OutOfRangeEndIndex" => "RetCode::OutOfRangeEndIndex".to_string(),
            "OutOfRangeStartIndex" => "RetCode::OutOfRangeStartIndex".to_string(),
            "ALLOC_ERR" => "RetCode::AllocErr".to_string(),
            "INTERNAL_ERROR" => "RetCode::InternalError".to_string(),
            _ => render_expr(expr, ctx, opt_real_params, registry, helpers),
        };
    }
    render_expr(expr, ctx, opt_real_params, registry, helpers)
}

/// Render a condition expression, ensuring it's boolean-typed.
/// Bare integer/usize variables get `!= 0` wrapping.
fn render_condition(
    expr: &Expr,
    ctx: &RustRenderCtx,
    opt_real_params: &[String],
    registry: &Registry,
    helpers: &HelperRegistry,
) -> String {
    // Bare Var used as condition: needs != 0 if it's an integer/usize
    if let Expr::Var(name) = expr {
        if ctx.index_vars.contains(name) || is_likely_index_var(name)
            || is_i32_opt_in_param(name)
        {
            let rendered = render_expr(expr, ctx, opt_real_params, registry, helpers);
            return format!("{rendered} != 0");
        }
    }
    // Not(Var) → Var == 0 for integer vars
    if let Expr::Not(inner) = expr {
        if let Expr::Var(name) = inner.as_ref() {
            if ctx.index_vars.contains(name) || is_likely_index_var(name)
                || is_i32_opt_in_param(name)
            {
                let rendered = render_expr(inner, ctx, opt_real_params, registry, helpers);
                return format!("{rendered} == 0");
            }
        }
    }
    // Ternary producing integer used as condition: needs != 0
    if let Expr::Ternary(_, then_expr, _) = expr {
        if expr_is_untyped_integer(then_expr) || matches!(then_expr.as_ref(), Expr::IntLiteral(_)) {
            let rendered = render_expr(expr, ctx, opt_real_params, registry, helpers);
            return format!("({rendered} != 0)");
        }
    }
    // FuncCall that inlines to ternary producing integer: needs != 0
    if let Expr::FuncCall(fname, args) = expr {
        if let Some(helper) = helpers.get(fname) {
            if let Some(Expr::Ternary(_, ref then_expr, _)) = try_inline_expr(helper, args) {
                if expr_is_untyped_integer(then_expr) || matches!(then_expr.as_ref(), Expr::IntLiteral(_)) {
                    let rendered = render_expr(expr, ctx, opt_real_params, registry, helpers);
                    return format!("({rendered} != 0)");
                }
            }
        }
    }
    render_expr(expr, ctx, opt_real_params, registry, helpers)
}

#[allow(clippy::too_many_lines, clippy::cognitive_complexity)]
/// Check if an expression is definitely integer-typed (conservative — only triggers on
/// known integer variables, not heuristics). Used to gate mul_add emission.
fn is_definitely_integer(expr: &Expr, ctx: &RustRenderCtx) -> bool {
    match expr {
        Expr::Var(name) => {
            ctx.index_vars.contains(name)
                || ctx.sentinel_vars.contains(name)
                || ctx.int_output_names.contains(name)
        }
        Expr::BinOp(a, _, b) => is_definitely_integer(a, ctx) || is_definitely_integer(b, ctx),
        // IntLiteral and everything else: not definitely integer (literals coerce to f64).
        _ => false,
    }
}

/// Rust-backend leaf formatting for the shared [`ExprEmitter`] tree-walk. Bundles
/// the render context with `opt_real_params` and the registry/helper services the
/// type-inference hooks need; the recursion itself lives in [`ExprEmitter::walk`].
/// The heavy `BinOp`/`Ternary` arms keep dedicated free functions
/// ([`render_binop`]/[`render_ternary`]) that the hooks delegate to.
struct RustExpr<'a> {
    ctx: &'a RustRenderCtx,
    opt_real_params: &'a [String],
    registry: &'a Registry,
    helpers: &'a HelperRegistry,
}

impl ExprEmitter for RustExpr<'_> {
    fn var(&self, name: &str) -> String {
        match name {
            "COMPATIBILITY" => "(self.compatibility)".to_string(),
            "METASTOCK" => "Compatibility::Metastock".to_string(),
            "DEFAULT" => "Compatibility::Default".to_string(),
            "BAD_PARAM" => "RetCode::BadParam".to_string(),
            "SUCCESS" => "RetCode::Success".to_string(),
            "ALLOC_ERR" => "RetCode::AllocErr".to_string(),
            "INTERNAL_ERROR" => "RetCode::InternalError".to_string(),
            "TA_MAType_SMA" => "0".to_string(),
            "TA_MAType_EMA" => "1".to_string(),
            "TA_MAType_WMA" => "2".to_string(),
            "TA_MAType_DEMA" => "3".to_string(),
            "TA_MAType_TEMA" => "4".to_string(),
            "TA_MAType_TRIMA" => "5".to_string(),
            "TA_MAType_KAMA" => "6".to_string(),
            "TA_MAType_MAMA" => "7".to_string(),
            "TA_MAType_T3" => "8".to_string(),
            _ => name.to_string(),
        }
    }

    fn array_access(&self, name: &str, idx: &Expr) -> String {
        let idx_rendered =
            render_index_expr(idx, self.ctx, self.opt_real_params, self.registry, self.helpers);
        // Always safe `[]` indexing. In the `_unguarded`/`_private` variants the
        // bounds-assert preamble lets LLVM elide the per-access checks, so this is
        // as fast as the old raw-pointer path while keeping the crate `unsafe`-free.
        format!("{name}[{idx_rendered}]")
    }

    fn func_call(&self, name: &str, args: &[Expr]) -> String {
        render_func_call(name, args, self.ctx, self.opt_real_params, self.registry, self.helpers)
    }

    fn binop(&self, left: &Expr, op: &BinOp, right: &Expr) -> String {
        render_binop(left, op, right, self.ctx, self.opt_real_params, self.registry, self.helpers)
    }

    fn cast(&self, var_type: &VarType, inner: &Expr) -> String {
        // IntLiteral cast to f64 -> emit as float literal (e.g., 0.0 instead of (0) as f64)
        if matches!(var_type, VarType::Real) {
            if let Expr::IntLiteral(n) = inner {
                return format!("{n}.0");
            }
        }
        // IntLiteral cast to usize -> emit as bare literal (Rust infers usize from context)
        if matches!(var_type, VarType::Integer | VarType::Index) {
            if let Expr::IntLiteral(n) = inner {
                return format!("{n}");
            }
        }
        let rust_type = match var_type {
            VarType::Real => "f64",
            VarType::Integer | VarType::Index => "usize",
            VarType::RetCodeType => "RetCode",
            VarType::RealPointer | VarType::IntPointer => "/* ptr cast */",
            VarType::RealArray(_) | VarType::IntArray(_) => "/* array cast */",
        };
        // `as` binds tighter than every binary operator, so a binary-op inner
        // must be wrapped; atomic/unary inners do not, and a ternary already
        // self-parenthesizes as `(if ... else ...)`.
        let s = self.walk(inner);
        if matches!(inner, Expr::BinOp(..)) {
            format!("({s}) as {rust_type}")
        } else {
            format!("{s} as {rust_type}")
        }
    }

    fn pointer_deref(&self, name: &str) -> String {
        format!("(*{name})")
    }

    fn address_of(&self, inner: &Expr) -> String {
        // address-of not idiomatic in Rust; render inner expression directly
        self.walk(inner)
    }

    fn post_increment(&self, inner: &Expr) -> String {
        let rendered = self.walk(inner);
        format!("{{ let _v = {rendered}; {rendered} += 1; _v }}")
    }

    // C's decrement idioms (`while (i-- > 0)`) let an unsigned counter wrap past
    // zero on the final iteration. Release builds already wrap (that behavior is
    // regtest-verified); `wrapping_sub` makes debug builds match instead of
    // panicking with `attempt to subtract with overflow`.
    fn post_decrement(&self, inner: &Expr) -> String {
        let rendered = self.walk(inner);
        format!("{{ let _v = {rendered}; {rendered} = {rendered}.wrapping_sub(1); _v }}")
    }

    fn pre_increment(&self, inner: &Expr) -> String {
        let rendered = self.walk(inner);
        format!("{{ {rendered} += 1; {rendered} }}")
    }

    fn pre_decrement(&self, inner: &Expr) -> String {
        let rendered = self.walk(inner);
        format!("{{ {rendered} = {rendered}.wrapping_sub(1); {rendered} }}")
    }

    fn ternary(&self, cond: &Expr, then_expr: &Expr, else_expr: &Expr) -> String {
        render_ternary(
            cond, then_expr, else_expr, self.ctx, self.opt_real_params, self.registry, self.helpers,
        )
    }
}

/// FMA emission gate — intentionally OFF until the FMA-era transition
/// (docs/fma-proposal.md, PR #96). With default (baseline) x86-64 rustflags,
/// `f64::mul_add` cannot inline and degrades to a per-operation dispatch call
/// into software `fma` (measured ~2x slower on EMA-family recursions), and on
/// FMA-native targets (aarch64) it produces different bits than the C library,
/// breaking cross-platform and cross-language bitwise consistency. Plain
/// `a * b + c` is never contracted by rustc, so generated Rust matches C
/// bit-for-bit on every target. Re-enable as part of the one-time FMA
/// re-baselining event.
const EMIT_FMA: bool = false;

/// Render an `Expr::BinOp` to Rust, including the FMA fusion (gated by
/// [`EMIT_FMA`]), pointer-identity buffer comparisons, and the operand
/// int/usize/f64 cast inference. Delegated to by [`RustExpr::binop`].
#[allow(clippy::cognitive_complexity, clippy::too_many_lines)]
fn render_binop(
    left: &Expr,
    op: &BinOp,
    right: &Expr,
    ctx: &RustRenderCtx,
    opt_real_params: &[String],
    registry: &Registry,
    helpers: &HelperRegistry,
) -> String {
    // Fused multiply-add: (a * b) + c → a.mul_add(b, c)
    // Emits ARM fmadd (1 FP op, 4 cycles) vs fmul+fadd (2 FP ops, 8 cycles).
    // Only for f64 operands (not integer arithmetic).
    // Fused multiply-add: (a * b) + c → (a as f64).mul_add(b, c)
    // Emits ARM fmadd (1 FP op) vs fmul+fadd (2 FP ops).
    // Only when BOTH multiply operands are float (not i32 * f64 patterns).
    if EMIT_FMA && matches!(op, BinOp::Add) {
        if let Expr::BinOp(a, BinOp::Mul, b) = left {
            let a_ok = expr_is_float_typed_ctx(a, Some(ctx)) && !is_definitely_integer(a, ctx);
            let b_ok = expr_is_float_typed_ctx(b, Some(ctx)) && !is_definitely_integer(b, ctx);
            if a_ok && b_ok {
                let a_str = render_expr(a, ctx, opt_real_params, registry, helpers);
                let b_str = render_expr(b, ctx, opt_real_params, registry, helpers);
                let c_str = render_expr(right, ctx, opt_real_params, registry, helpers);
                return format!("({a_str} as f64).mul_add({b_str}, {c_str})");
            }
        }
        if let Expr::BinOp(a, BinOp::Mul, b) = right {
            let a_ok = expr_is_float_typed_ctx(a, Some(ctx)) && !is_definitely_integer(a, ctx);
            let b_ok = expr_is_float_typed_ctx(b, Some(ctx)) && !is_definitely_integer(b, ctx);
            if a_ok && b_ok {
                let a_str = render_expr(a, ctx, opt_real_params, registry, helpers);
                let b_str = render_expr(b, ctx, opt_real_params, registry, helpers);
                let c_str = render_expr(left, ctx, opt_real_params, registry, helpers);
                return format!("({a_str} as f64).mul_add({b_str}, {c_str})");
            }
        }
    }
    // C pointer-identity buffer comparisons (BBANDS' `inReal == outRealUpperBand`,
    // DEMA's `inReal == outReal`, and the alias-optimization guards) must become
    // Rust *pointer* comparisons, not value comparisons. In the borrow-checked
    // slice API an input and an output buffer can never alias, so identity is the
    // correct semantics; a value comparison wrongly trips on coincidentally-equal
    // contents (e.g. an all-zero input vs a zero-initialized output → false
    // TA_BAD_PARAM).
    if matches!(op, BinOp::Eq | BinOp::NotEq)
        && is_buffer_operand(left, ctx)
        && is_buffer_operand(right, ctx)
    {
        let l = render_expr(left, ctx, opt_real_params, registry, helpers);
        let r = render_expr(right, ctx, opt_real_params, registry, helpers);
        let cmp = if matches!(op, BinOp::Eq) { "==" } else { "!=" };
        return format!("{l}.as_ptr() {cmp} {r}.as_ptr()");
    }
    let op_str = match op {
        BinOp::Add => " + ",
        BinOp::Sub => " - ",
        BinOp::Mul => " * ",
        BinOp::Div => " / ",
        BinOp::Mod => " % ",
        BinOp::LessEq => " <= ",
        BinOp::Less => " < ",
        BinOp::Greater => " > ",
        BinOp::GreaterEq => " >= ",
        BinOp::Eq => " == ",
        BinOp::NotEq => " != ",
        BinOp::And => " && ",
        BinOp::Or => " || ",
        BinOp::BitwiseOr => " | ",
        BinOp::Shr => " >> ",
        BinOp::Shl => " << ",
    };
    let is_arithmetic = matches!(op, BinOp::Add | BinOp::Sub | BinOp::Mul | BinOp::Div | BinOp::Mod | BinOp::Shr | BinOp::Shl);
    let mut left_str = render_binop_operand(left, op, true, ctx, opt_real_params, registry, helpers);
    let mut right_str = render_binop_operand(right, op, false, ctx, opt_real_params, registry, helpers);
    if is_arithmetic {
        // Sentinel vars are i32 — when mixed with usize, cast usize→i32
        let left_is_sentinel = expr_is_i32_typed_ctx(left, ctx) && !expr_is_i32_typed(left);
        let right_is_sentinel = expr_is_i32_typed_ctx(right, ctx) && !expr_is_i32_typed(right);
        if left_is_sentinel && !right_is_sentinel && !expr_is_i32_typed(right) && !expr_is_float_typed_ctx(right, Some(ctx)) && !matches!(right, Expr::IntLiteral(_)) {
            right_str = format!("({right_str}) as i32");
        }
        if right_is_sentinel && !left_is_sentinel && !expr_is_i32_typed(left) && !expr_is_float_typed_ctx(left, Some(ctx)) && !matches!(left, Expr::IntLiteral(_)) {
            left_str = format!("({left_str}) as i32");
        }
        let left_is_i32 = expr_is_i32_typed(left) || left_is_sentinel;
        let right_is_i32 = expr_is_i32_typed(right) || right_is_sentinel;
        // i32-typed expressions are NOT float even if heuristics say otherwise
        let left_is_float = expr_is_float_typed_ctx(left, Some(ctx)) && !left_is_i32;
        let right_is_float = expr_is_float_typed_ctx(right, Some(ctx)) && !right_is_i32;
        let left_is_int_lit = matches!(left, Expr::IntLiteral(_));
        let right_is_int_lit = matches!(right, Expr::IntLiteral(_));

        {
            // Cast integer operands to f64 when doing arithmetic with f64-typed expressions
            if left_is_int_lit && right_is_float {
                if let Expr::IntLiteral(v) = left {
                    left_str = format!("{v}_f64");
                }
            }
            if right_is_int_lit && left_is_float {
                if let Expr::IntLiteral(v) = right {
                    right_str = format!("{v}_f64");
                }
            }
            if left_is_i32 && right_is_float && !left_is_int_lit && !left_is_float {
                left_str = format!("(({left_str}) as f64)");
            }
            if right_is_i32 && left_is_float && !right_is_int_lit && !right_is_float {
                right_str = format!("(({right_str}) as f64)");
            }
            let left_is_untyped_int = expr_is_untyped_integer(left);
            let right_is_untyped_int = expr_is_untyped_integer(right);
            if left_is_untyped_int && !left_is_int_lit && right_is_float {
                left_str = format!("(({left_str}) as f64)");
            }
            if right_is_untyped_int && !right_is_int_lit && left_is_float {
                right_str = format!("(({right_str}) as f64)");
            }
            let left_is_known_usize = expr_is_known_usize_ctx(left, ctx);
            let right_is_known_usize = expr_is_known_usize_ctx(right, ctx);
            let left_eff_usize = left_is_known_usize
                || expr_binop_renders_as_usize(left, ctx);
            let right_eff_usize = right_is_known_usize
                || expr_binop_renders_as_usize(right, ctx);
            if left_eff_usize && right_is_float && !left_is_i32 {
                left_str = format!("(({left_str}) as f64)");
            }
            if right_eff_usize && left_is_float && !right_is_i32 {
                right_str = format!("(({right_str}) as f64)");
            }
        }
        // Cast i32 operands to usize when mixed with usize-typed operands (not float)
        // Also detect i32 array accesses (IntArray/IntPointer)
        let arith_left_is_i32_arr = matches!(left, Expr::ArrayAccess(ref name, _) if is_int_array_or_vec(name, ctx));
        let arith_right_is_i32_arr = matches!(right, Expr::ArrayAccess(ref name, _) if is_int_array_or_vec(name, ctx));
        let left_is_i32_eff = left_is_i32 || arith_left_is_i32_arr;
        let right_is_i32_eff = right_is_i32 || arith_right_is_i32_arr;
        let left_is_usize = !left_is_i32_eff && !left_is_float && !left_is_int_lit;
        let right_is_usize = !right_is_i32_eff && !right_is_float && !right_is_int_lit;
        if left_is_i32_eff && right_is_usize && !left_is_sentinel {
            left_str = format!("({left_str}) as usize");
        }
        if right_is_i32_eff && left_is_usize && !right_is_sentinel {
            right_str = format!("({right_str}) as usize");
        }
        // When both sides appear i32-typed but one actually renders as usize
        // (e.g., Cast(Integer, usize_expr) drops the cast), fix the mismatch.
        if left_is_i32_eff && right_is_i32_eff && !left_is_int_lit && !right_is_int_lit {
            let left_renders_usize = expr_renders_as_usize_despite_i32(left, ctx);
            let right_renders_usize = expr_renders_as_usize_despite_i32(right, ctx);
            if left_renders_usize && !right_renders_usize {
                right_str = format!("({right_str}) as usize");
            }
            if right_renders_usize && !left_renders_usize {
                left_str = format!("({left_str}) as usize");
            }
        }
    }
    // For comparison operators, cast i32 to usize when mixed (not float)
    // and wrap IntLiterals with T::ta_zero() / T::ta_from_i32() when comparing with T-typed exprs
    if matches!(op, BinOp::Less | BinOp::LessEq | BinOp::Greater | BinOp::GreaterEq | BinOp::Eq | BinOp::NotEq) {
        // Sentinel vars are i32 — when compared with usize, cast usize→i32
        let cmp_left_sentinel = expr_is_i32_typed_ctx(left, ctx) && !expr_is_i32_typed(left);
        let cmp_right_sentinel = expr_is_i32_typed_ctx(right, ctx) && !expr_is_i32_typed(right);
        if cmp_left_sentinel && !cmp_right_sentinel && !expr_is_i32_typed(right) && !expr_is_float_typed_ctx(right, Some(ctx)) && !matches!(right, Expr::IntLiteral(_)) {
            right_str = format!("({right_str}) as i32");
        }
        if cmp_right_sentinel && !cmp_left_sentinel && !expr_is_i32_typed(left) && !expr_is_float_typed_ctx(left, Some(ctx)) && !matches!(left, Expr::IntLiteral(_)) {
            left_str = format!("({left_str}) as i32");
        }
        let left_is_i32 = expr_is_i32_typed(left) || cmp_left_sentinel;
        let right_is_i32 = expr_is_i32_typed(right) || cmp_right_sentinel;
        // i32-typed expressions are NOT float even if heuristics say otherwise
        let left_is_float = expr_is_float_typed_ctx(left, Some(ctx)) && !left_is_i32;
        let right_is_float = expr_is_float_typed_ctx(right, Some(ctx)) && !right_is_i32;
        let left_is_int_lit = matches!(left, Expr::IntLiteral(_));
        let right_is_int_lit = matches!(right, Expr::IntLiteral(_));
        {
            let left_is_untyped_int = expr_is_untyped_integer(left);
            let right_is_untyped_int = expr_is_untyped_integer(right);
            // Cast IntLiteral to f64 when compared against f64-typed expression
            if right_is_int_lit && left_is_float {
                if let Expr::IntLiteral(v) = right {
                    right_str = format!("{v}_f64");
                }
            }
            if left_is_int_lit && right_is_float {
                if let Expr::IntLiteral(v) = left {
                    left_str = format!("{v}_f64");
                }
            }
            if left_is_i32 && right_is_float && !left_is_int_lit && !left_is_float {
                left_str = format!("(({left_str}) as f64)");
            }
            if right_is_i32 && left_is_float && !right_is_int_lit && !right_is_float {
                right_str = format!("(({right_str}) as f64)");
            }
            if left_is_untyped_int && !left_is_int_lit && right_is_float {
                left_str = format!("(({left_str}) as f64)");
            }
            if right_is_untyped_int && !right_is_int_lit && left_is_float {
                right_str = format!("(({right_str}) as f64)");
            }
            let cmp_left_is_known_usize = expr_is_known_usize_ctx(left, ctx);
            let cmp_right_is_known_usize = expr_is_known_usize_ctx(right, ctx);
            if cmp_left_is_known_usize && right_is_float && !cmp_right_is_known_usize && !left_is_i32 && !left_is_int_lit {
                left_str = format!("(({left_str}) as f64)");
            }
            if cmp_right_is_known_usize && left_is_float && !cmp_left_is_known_usize && !right_is_i32 && !right_is_int_lit {
                right_str = format!("(({right_str}) as f64)");
            }
        }
        // Also detect i32 array accesses (IntArray/IntPointer) using context
        let left_is_i32_arr = matches!(left, Expr::ArrayAccess(ref name, _) if is_int_array_or_vec(name, ctx));
        let right_is_i32_arr = matches!(right, Expr::ArrayAccess(ref name, _) if is_int_array_or_vec(name, ctx));
        let left_is_i32_eff = left_is_i32 || left_is_i32_arr;
        let right_is_i32_eff = right_is_i32 || right_is_i32_arr;
        if left_is_i32_eff && !right_is_i32_eff && !right_is_float && !right_is_int_lit && !cmp_left_sentinel {
            left_str = format!("({left_str}) as usize");
        }
        if right_is_i32_eff && !left_is_i32_eff && !left_is_float && !left_is_int_lit && !cmp_right_sentinel {
            right_str = format!("({right_str}) as usize");
        }
    }
    format!("{left_str}{op_str}{right_str}")
}

/// Render an `Expr::Ternary` to a Rust `if`/`else` expression, choosing a boolean
/// vs. value condition rendering. Delegated to by [`RustExpr::ternary`].
fn render_ternary(
    cond: &Expr,
    then_expr: &Expr,
    else_expr: &Expr,
    ctx: &RustRenderCtx,
    opt_real_params: &[String],
    registry: &Registry,
    helpers: &HelperRegistry,
) -> String {
    // Use render_condition for the ternary condition when it's a non-boolean
    // expression (integer variable, ternary producing integer, etc.)
    let cond_needs_bool = match cond {
        Expr::Ternary(_, t, _) => expr_is_untyped_integer(t) || matches!(t.as_ref(), Expr::IntLiteral(_)),
        Expr::Var(name) => ctx.index_vars.contains(name) || is_likely_index_var(name) || is_i32_opt_in_param(name),
        Expr::Not(inner) => matches!(inner.as_ref(), Expr::Var(name) if ctx.index_vars.contains(name) || is_likely_index_var(name)),
        // FuncCall that inlines to integer-producing ternary (e.g., ta_realbodygapup)
        Expr::FuncCall(fname, args) => {
            if let Some(helper) = helpers.get(fname) {
                if let Some(inlined) = try_inline_expr(helper, args) {
                    if let Expr::Ternary(_, ref t, _) = inlined {
                        expr_is_untyped_integer(t) || matches!(t.as_ref(), Expr::IntLiteral(_))
                    } else { false }
                } else { is_integer_returning_helper(fname) }
            } else { is_integer_returning_helper(fname) }
        }
        _ => false,
    };
    let cond_str = if cond_needs_bool {
        render_condition(cond, ctx, opt_real_params, registry, helpers)
    } else {
        render_expr(cond, ctx, opt_real_params, registry, helpers)
    };
    let then_str = render_expr(then_expr, ctx, opt_real_params, registry, helpers);
    let else_str = render_expr(else_expr, ctx, opt_real_params, registry, helpers);
    format!(
        "(if {cond_str} {{ {then_str} }} else {{ {else_str} }})"
    )
}

fn render_expr(
    expr: &Expr,
    ctx: &RustRenderCtx,
    opt_real_params: &[String],
    registry: &Registry,
    helpers: &HelperRegistry,
) -> String {
    RustExpr { ctx, opt_real_params, registry, helpers }.walk(expr)
}

/// Render one of the boolean near-zero builtins (IS_ZERO / IS_ZERO_SCALED /
/// IS_ZERO_OR_NEG) in Rust from already-rendered argument strings. Single source
/// of the Rust form for these predicates — used by both the indicator render path
/// and the `eval_predicate` server handler (see the C backend for the rationale).
pub(crate) fn rust_predicate_expr(which: SpecialBuiltin, args: &[String]) -> String {
    match which {
        SpecialBuiltin::IsZero => args
            .first()
            .map_or_else(|| "false".to_string(), |x| format!("({x}).abs() < 1e-14")),
        SpecialBuiltin::IsZeroScaled => {
            if args.len() == 2 {
                format!("(({}).abs() <= 1e-14 * ({}))", args[0], args[1])
            } else {
                "false".to_string()
            }
        }
        SpecialBuiltin::IsZeroOrNeg => args
            .first()
            .map_or_else(|| "false".to_string(), |x| format!("({x}) < 1e-14")),
        _ => "false".to_string(),
    }
}

/// Check if an expression is clearly integer-typed (for Cast optimization in generic mode).
/// When true, `T::ta_from_i32(expr as i32)` will be used instead of `T::ta_from_f64(expr.ta_to_f64())`.
fn expr_is_integer(expr: &Expr) -> bool {
    match expr {
        Expr::Var(name) => {
            is_i32_opt_in_param(name) || name.ends_with("_avgPeriod")
                || name.ends_with("_rangeType")
        }
        Expr::IntLiteral(_) | Expr::Cast(VarType::Integer | VarType::Index, _) => true,
        Expr::BinOp(left, _, right) => expr_is_integer(left) && expr_is_integer(right),
        _ => false,
    }
}

/// Check if an optIn parameter name is i32 (Integer type, not Real type).
/// Real optIn params (optInAcceleration, optInFastLimit, optInSlowLimit, optInNbDevUp/Dn,
/// optInMaximum, optInPenetration, optInVFactor) are f64, NOT i32.
fn is_i32_opt_in_param(name: &str) -> bool {
    if !name.starts_with("optIn") {
        return false;
    }
    // Known Real optIn params that are f64
    !matches!(name,
        "optInAcceleration" | "optInMaximum" | "optInOffsetOnReverse"
        | "optInFastLimit" | "optInSlowLimit"
        | "optInNbDevUp" | "optInNbDevDn" | "optInNbDev"
        | "optInPenetration" | "optInVFactor"
        | "optInStartValue" | "optInPercentage"
        | "optInAccelerationInitLong" | "optInAccelerationLong" | "optInAccelerationMaxLong"
        | "optInAccelerationInitShort" | "optInAccelerationShort" | "optInAccelerationMaxShort"
    )
}

/// Check if an expression is likely T-typed (float/Real) in generic context.
/// Used to decide whether an IntLiteral should be wrapped with `T::ta_from_i32()`.
/// Conservative: only returns true when there's strong evidence the expression produces T.
fn expr_is_float_typed_ctx(expr: &Expr, ctx: Option<&RustRenderCtx>) -> bool {
    match expr {
        Expr::Literal(_) | Expr::Cast(VarType::Real, _) => true,
        Expr::Var(name) => {
            // Check context's real_vars set first
            if let Some(c) = ctx {
                if c.real_vars.contains(name) {
                    return true;
                }
                // If declared as index/integer in VarDecl, it's NOT float
                // Only use explicit declarations, not naming heuristics (which overlap)
                if c.index_vars.contains(name) {
                    return false;
                }
            }
            // optIn Real params are f64 in the function signature
            if name.starts_with("optIn") && !is_i32_opt_in_param(name) {
                return true;
            }
            // Only match known float-typed variable patterns.
            // Exclude anything that could be an integer/index variable.
            name.starts_with("temp") || name.starts_with("prev")
                || name.starts_with("sum") || name.starts_with("diff")
                || name.ends_with("PeriodTotal")
                || name == "k" || name == "k1"
                || name.starts_with("factor") || name.ends_with("_factor")
                || name.starts_with("_true_range")
                || name.starts_with("_candlerange")
                || name.starts_with("_periodTotal")
                || name.starts_with("_meanValue")
                || name.starts_with("_tempReal")
        }
        Expr::ArrayAccess(name, _) => {
            // Check context's real_array_vars set
            if let Some(c) = ctx {
                if c.real_array_vars.contains(name) {
                    return true;
                }
            }
            // Real arrays: input arrays, temp buffers, output Real arrays
            name.starts_with("in") || name.starts_with("temp")
                || (name.starts_with("out") && !name.contains("Int") && !name.contains("integer"))
                || name.contains("_Odd") || name.contains("_Even")
                || name.starts_with("detrender") || name.starts_with("Q1")
                || name.starts_with("jI") || name.starts_with("jQ")
        }
        Expr::FuncCall(name, _) => {
            // Math functions and ta_ methods return T, but NOT integer-returning helpers
            if is_integer_returning_helper(name) {
                return false;
            }
            name.starts_with("ta_") || name.contains("_from_")
                || matches!(name.as_str(),
                    "sqrt" | "sin" | "cos" | "tan" | "asin" | "acos" | "atan"
                    | "exp" | "log" | "log10" | "ceil" | "floor" | "abs" | "fabs"
                    | "cosh" | "sinh" | "tanh" | "max" | "fmax" | "min" | "fmin"
                    | "IS_ZERO" | "IS_ZERO_OR_NEG" | "PER_TO_K"
                )
        }
        Expr::BinOp(left, op, right) => {
            matches!(op, BinOp::Add | BinOp::Sub | BinOp::Mul | BinOp::Div)
                && (expr_is_float_typed_ctx(left, ctx) || expr_is_float_typed_ctx(right, ctx))
        }
        Expr::Ternary(_, then_expr, _) => expr_is_float_typed_ctx(then_expr, ctx),
        _ => false,
    }
}

/// Check if an expression is likely i32-typed (integer optIn params, unstable_period access, etc.)
/// Note: Real optIn params (optInAcceleration, optInFastLimit, optInSlowLimit, etc.) are f64, not i32.
fn expr_is_i32_typed(expr: &Expr) -> bool {
    match expr {
        Expr::Var(name) => {
            is_i32_opt_in_param(name) || name.ends_with("_avgPeriod")
                || name.ends_with("_rangeType")
        }
        Expr::FuncCall(name, args) => {
            if name == "UNSTABLE_PERIOD" {
                return true; // unstable_period array contains i32 values
            }
            // max(a,b) / min(a,b) preserve the type of their arguments.
            // If all args are i32-typed (or IntLiteral), the result is i32.
            if matches!(name.as_str(), "max" | "min" | "fmax" | "fmin") {
                return args.iter().all(|a| expr_is_i32_typed(a) || matches!(a, Expr::IntLiteral(_)));
            }
            false
        }
        Expr::BinOp(left, BinOp::Add | BinOp::Sub | BinOp::Mul | BinOp::Div | BinOp::Shr | BinOp::Shl | BinOp::Mod, right) => {
            expr_is_i32_typed(left) && (expr_is_i32_typed(right) || matches!(right.as_ref(), Expr::IntLiteral(_)))
                || expr_is_i32_typed(right) && matches!(left.as_ref(), Expr::IntLiteral(_))
        }
        Expr::Cast(VarType::Integer, _inner) => {
            true
        }
        _ => false,
    }
}

/// Context-aware version of `expr_is_i32_typed` that also recognizes sentinel variables.
fn expr_is_i32_typed_ctx(expr: &Expr, ctx: &RustRenderCtx) -> bool {
    if expr_is_i32_typed(expr) {
        return true;
    }
    match expr {
        Expr::Var(name) => ctx.sentinel_vars.contains(name),
        Expr::BinOp(left, BinOp::Add | BinOp::Sub | BinOp::Mul | BinOp::Div, right) => {
            let l_i32 = expr_is_i32_typed_ctx(left, ctx)
                || matches!(left.as_ref(), Expr::IntLiteral(_));
            let r_i32 = expr_is_i32_typed_ctx(right, ctx)
                || matches!(right.as_ref(), Expr::IntLiteral(_));
            if l_i32 && r_i32 { return true; }
            let l_sentinel = matches!(left.as_ref(), Expr::Var(n) if ctx.sentinel_vars.contains(n))
                || (l_i32 && !expr_is_i32_typed(left) && !matches!(left.as_ref(), Expr::IntLiteral(_)));
            let r_sentinel = matches!(right.as_ref(), Expr::Var(n) if ctx.sentinel_vars.contains(n))
                || (r_i32 && !expr_is_i32_typed(right) && !matches!(right.as_ref(), Expr::IntLiteral(_)));
            let l_usize = expr_is_known_usize_ctx(left, ctx) || matches!(left.as_ref(), Expr::IntLiteral(_));
            let r_usize = expr_is_known_usize_ctx(right, ctx) || matches!(right.as_ref(), Expr::IntLiteral(_));
            (l_sentinel && r_usize) || (r_sentinel && l_usize)
                || (l_i32 && r_usize && contains_sentinel_expr(left, ctx))
                || (r_i32 && l_usize && contains_sentinel_expr(right, ctx))
        }
        _ => false,
    }
}

fn contains_sentinel_expr(expr: &Expr, ctx: &RustRenderCtx) -> bool {
    match expr {
        Expr::Var(name) => ctx.sentinel_vars.contains(name),
        Expr::BinOp(left, _, right) => {
            contains_sentinel_expr(left, ctx) || contains_sentinel_expr(right, ctx)
        }
        _ => false,
    }
}

/// Check if an expression produces an untyped integer (IntLiteral, ternary with int branches, etc.)
/// These need wrapping with `T::ta_from_i32()` when used in a T-typed context.
fn expr_is_untyped_integer(expr: &Expr) -> bool {
    match expr {
        Expr::IntLiteral(_) => true,
        Expr::Ternary(_, then_expr, else_expr) => {
            expr_is_untyped_integer(then_expr) || expr_is_untyped_integer(else_expr)
        }
        Expr::FuncCall(name, _) => {
            // Integer-returning helpers inline to integer ternaries
            is_integer_returning_helper(name)
        }
        Expr::BinOp(left, BinOp::Add | BinOp::Sub | BinOp::Mul | BinOp::Div | BinOp::Mod, right) => {
            let left_is_int = expr_is_untyped_integer(left) || matches!(left.as_ref(), Expr::IntLiteral(_));
            let right_is_int = expr_is_untyped_integer(right) || matches!(right.as_ref(), Expr::IntLiteral(_));
            left_is_int && right_is_int && !expr_is_i32_typed(left) && !expr_is_i32_typed(right)
        }
        _ => false,
    }
}

/// Check if an expression is already usize-typed (to avoid redundant `as usize` casts).
/// Uses the context's `index_vars` set to recognize declared usize variables.
fn expr_is_usize(expr: &Expr, ctx: &RustRenderCtx) -> bool {
    match expr {
        // Variables declared as usize (loop counters, index vars) don't need casting
        Expr::Var(name) => {
            if ctx.sentinel_vars.contains(name) {
                return false;
            }
            ctx.index_vars.contains(name) || is_likely_index_var(name)
        }
        // Integer literals infer usize from array index context
        Expr::Cast(VarType::Index | VarType::Integer, _) | Expr::IntLiteral(_) => true,
        // PostIncrement/PostDecrement/PreIncrement/PreDecrement on usize vars produce usize
        Expr::PostIncrement(inner)
        | Expr::PostDecrement(inner)
        | Expr::PreIncrement(inner)
        | Expr::PreDecrement(inner) => expr_is_usize(inner, ctx),
        // BinOps where both sides are usize produce usize
        Expr::BinOp(left, BinOp::Add | BinOp::Sub | BinOp::Mul | BinOp::Div, right) => {
            expr_is_usize(left, ctx) && expr_is_usize(right, ctx)
        }
        _ => false,
    }
}

/// Check if an expression is provably usize-typed (for T-wrapping in arithmetic).
/// Uses the context's `index_vars` set when available, falls back to naming heuristics.
fn expr_is_known_usize_ctx(expr: &Expr, ctx: &RustRenderCtx) -> bool {
    match expr {
        Expr::Cast(VarType::Index | VarType::Integer, _) => true,
        Expr::Var(name) => {
            // Real vars are never usize, even if name matches heuristics
            if ctx.real_vars.contains(name) { return false; }
            // Sentinel vars (assigned -1) are i32, not usize
            if ctx.sentinel_vars.contains(name) { return false; }
            ctx.index_vars.contains(name) || is_likely_index_var(name)
        }
        Expr::PointerDeref(name) => {
            // *outBegIdx, *outNBElement are usize
            if ctx.sentinel_vars.contains(name) { return false; }
            ctx.index_vars.contains(name) || is_likely_index_var(name)
                || name == "outBegIdx" || name == "outNBElement"
        }
        Expr::BinOp(left, BinOp::Add | BinOp::Sub | BinOp::Mul | BinOp::Div, right) => {
            (expr_is_known_usize_ctx(left, ctx) || matches!(left.as_ref(), Expr::IntLiteral(_)))
                && (expr_is_known_usize_ctx(right, ctx) || matches!(right.as_ref(), Expr::IntLiteral(_)))
        }
        _ => false,
    }
}



/// Check if an expression considered "i32-typed" by `expr_is_i32_typed` actually
/// renders as usize due to containing a `Cast(Integer/Index, inner)` where `inner`
/// is already usize. Used to detect BinOps with mixed rendered types.
fn expr_renders_as_usize_despite_i32(expr: &Expr, ctx: &RustRenderCtx) -> bool {
    match expr {
        Expr::Cast(VarType::Integer | VarType::Index, inner) => {
            // The Cast handler renders as identity (no `as usize`) when inner is
            // already usize-typed, making the whole expr usize at runtime
            expr_is_known_usize_ctx(inner, ctx)
                || expr_is_integer(inner)
                || expr_returns_usize(inner)
        }
        Expr::BinOp(left, _, right) => {
            expr_renders_as_usize_despite_i32(left, ctx)
                || expr_renders_as_usize_despite_i32(right, ctx)
        }
        _ => false,
    }
}

/// Check if a BinOp expression evaluates to usize after rendering, considering
/// that the BinOp handler casts i32 operands to usize when mixed with usize.
/// This detects expressions like `optInTimePeriod - (today - highestIdx)` where
/// optInTimePeriod is i32 but gets cast to usize at render time.
fn expr_binop_renders_as_usize(expr: &Expr, ctx: &RustRenderCtx) -> bool {
    if let Expr::BinOp(left, op, right) = expr {
        if !matches!(op, BinOp::Add | BinOp::Sub | BinOp::Mul | BinOp::Div) {
            return false;
        }
        let l_usize = expr_is_known_usize_ctx(left, ctx)
            || matches!(left.as_ref(), Expr::IntLiteral(_));
        let r_usize = expr_is_known_usize_ctx(right, ctx)
            || matches!(right.as_ref(), Expr::IntLiteral(_));
        let l_i32 = expr_is_i32_typed(left);
        let r_i32 = expr_is_i32_typed(right);
        // If one side is usize and the other is i32, the handler casts i32 to usize
        // Also if one side is a sub-BinOp that itself renders as usize
        let l_eff_usize = l_usize || expr_binop_renders_as_usize(left, ctx)
            || expr_renders_as_usize_despite_i32(left, ctx);
        let r_eff_usize = r_usize || expr_binop_renders_as_usize(right, ctx)
            || expr_renders_as_usize_despite_i32(right, ctx);
        // Mixed: one side usize, other i32 => renders as usize
        (l_eff_usize && r_i32) || (r_eff_usize && l_i32)
            // Both usize
            || (l_eff_usize && r_eff_usize)
    } else {
        false
    }
}

/// Render an array index expression, adding `as usize` when needed.
fn render_index_expr(
    idx: &Expr,
    ctx: &RustRenderCtx,
    opt_real_params: &[String],
    registry: &Registry,
    helpers: &HelperRegistry,
) -> String {
    let rendered = render_expr(idx, ctx, opt_real_params, registry, helpers);
    if expr_is_usize(idx, ctx) {
        rendered
    } else {
        format!("({rendered}) as usize")
    }
}

/// Render a complex lookback body (`LookbackExpr::Code`) into Rust code.
fn render_lookback_code(
    stmts: &[Statement],
    enums: &HashMap<String, EnumDef>,
    registry: &Registry,
    helpers: &HelperRegistry,
) -> String {
    let mut out = String::new();
    let empty_for_loop_vars: Vec<String> = Vec::new();
    let empty_var_inits: std::collections::HashMap<String, &Expr> =
        std::collections::HashMap::new();
    let empty_output_names: Vec<String> = Vec::new();
    let empty_opt_real_params: Vec<String> = Vec::new();

    for stmt in stmts {
        if let Statement::VarDecl { var_type, name, .. } = stmt {
            let total_assigns = count_assignments(name, stmts);
            // With default initialization, the let itself is an assignment,
            // so any body assignment means we need mut (threshold is > 0, not > 1)
            let needs_mut = total_assigns > 0;
            match var_type {
                VarType::RealArray(size) => {
                    out.push_str(&format!(
                        "        let mut {name}: [f64; {size} as usize] = [0.0_f64; {size} as usize];\n"
                    ));
                }
                VarType::IntArray(size) => {
                    out.push_str(&format!(
                        "        let mut {name}: [i32; {size} as usize] = [0i32; {size} as usize];\n"
                    ));
                }
                _ => {
                    let rust_type = match var_type {
                        VarType::Real => "f64",
                        VarType::Integer | VarType::Index => "usize",
                        VarType::RetCodeType => "RetCode",
                        VarType::RealPointer => "Vec<f64>",
                        VarType::IntPointer => "Vec<i32>",
                        VarType::RealArray(_) | VarType::IntArray(_) => unreachable!(),
                    };
                    // Always initialize — lookback is always concrete (non-generic)
                    let default_val = match var_type {
                        VarType::Real => "0.0_f64",
                        VarType::Integer | VarType::Index => "0_usize",
                        VarType::RetCodeType => "RetCode::Success",
                        VarType::RealPointer | VarType::IntPointer => "Vec::new()",
                        VarType::RealArray(_) | VarType::IntArray(_) => unreachable!(),
                    };
                    if needs_mut {
                        out.push_str(&format!("        let mut {name}: {rust_type} = {default_val};\n"));
                    } else {
                        out.push_str(&format!("        let {name}: {rust_type} = {default_val};\n"));
                    }
                }
            }
        }
    }

    // Emit candle settings unpacking for lookback body
    let candle_used = detect_candle_settings(stmts);
    if !candle_used.is_empty() {
        out.push_str(&emit_rust_unpacking(&candle_used, 8));
    }

    let lookback_ctx = RustRenderCtx::for_lookback();
    let inline_counter = Cell::new(0);
    for stmt in stmts {
        if matches!(stmt, Statement::VarDecl { .. }) {
            continue;
        }
        out.push_str(&render_statement(
            stmt,
            8,
            &lookback_ctx,
            &empty_for_loop_vars,
            &empty_var_inits,
            &empty_output_names,
            &empty_opt_real_params,
            enums,
            registry,
            helpers,
            &inline_counter,
        ));
    }

    out
}

fn to_pascal_case(s: &str) -> String {
    // Direct mapping for known FuncUnstId names
    match s {
        "HT_DCPERIOD" => return "HtDcPeriod".to_string(),
        "HT_DCPHASE" => return "HtDcPhase".to_string(),
        "HT_PHASOR" => return "HtPhasor".to_string(),
        "HT_SINE" => return "HtSine".to_string(),
        "HT_TRENDLINE" => return "HtTrendline".to_string(),
        "HT_TRENDMODE" => return "HtTrendMode".to_string(),
        "MINUS_DI" => return "MinusDI".to_string(),
        "MINUS_DM" => return "MinusDM".to_string(),
        "PLUS_DI" => return "PlusDI".to_string(),
        "PLUS_DM" => return "PlusDM".to_string(),
        "STOCH_RSI" | "STOCHRSI" => return "StochRsi".to_string(),
        _ => {}
    }
    s.split('_')
        .filter(|part| !part.is_empty())
        .map(|part| {
            let lower = part.to_lowercase();
            let mut chars = lower.chars();
            match chars.next() {
                None => String::new(),
                Some(c) => c.to_uppercase().collect::<String>() + chars.as_str(),
            }
        })
        .collect()
}

/// Decompose an expression into (array_name, offset) for array copy operations.
/// `Var("arr")` → `("arr", "0")`; `AddressOf(ArrayAccess("arr", idx))` → `("arr", rendered_idx)`
fn decompose_rust_array_ref(
    expr: &Expr,
    ctx: &RustRenderCtx,
    opt_real_params: &[String],
    registry: &Registry,
    helpers: &HelperRegistry,
) -> (String, String) {
    match expr {
        Expr::AddressOf(inner) => if let Expr::ArrayAccess(name, offset) = inner.as_ref() {
            let off = render_index_expr(offset, ctx, opt_real_params, registry, helpers);
            (name.clone(), off)
        } else {
            let s = render_expr(expr, ctx, opt_real_params, registry, helpers);
            (s, "0".to_string())
        },
        Expr::Var(name) => (name.clone(), "0".to_string()),
        _ => {
            let s = render_expr(expr, ctx, opt_real_params, registry, helpers);
            (s, "0".to_string())
        }
    }
}

// NOTE: Candle range types (rangeType 0=RealBody, 1=HighLow, 2=Shadows)
// are configurable at runtime via candle settings, so we cannot
// constant-propagate them at code-generation time. The codegen emits
// runtime method calls (`self.ta_candlerange` / `self.ta_candleaverage`)
// which dispatch on the actual rangeType value.

#[allow(clippy::too_many_lines, clippy::cognitive_complexity)]
fn render_func_call(
    fname: &str,
    args: &[Expr],
    ctx: &RustRenderCtx,
    opt_real_params: &[String],
    registry: &Registry,
    helpers: &HelperRegistry,
) -> String {
    // Check if this is a call to a helper function that can be inlined
    if let Some(helper) = helpers.get(fname) {
        if let Some(inlined_expr) = try_inline_expr(helper, args) {
            let rendered = render_expr(&inlined_expr, ctx, opt_real_params, registry, helpers);
            // Wrap inlined BinOp in parens to preserve precedence when the
            // FuncCall sits inside a higher-precedence operator (e.g., `f(a,b) / 2`
            // where f inlines to `a - b` must become `(a - b) / 2`).
            if matches!(inlined_expr, Expr::BinOp(_, _, _)) {
                return format!("({rendered})");
            }
            return rendered;
        }
        // Multi-statement helpers are hoisted earlier by hoist_block_helpers.
    }
    if let Some(b) = SpecialBuiltin::from_name(fname) {
        match b {
            SpecialBuiltin::UnstablePeriod => {
                if let Some(Expr::Var(func_name)) = args.first() {
                    let base = func_name
                        .strip_prefix("FUNC_UNST_")
                        .unwrap_or(func_name);
                    let pascal = to_pascal_case(base);
                    return format!("self.unstable_period[FuncUnstId::{pascal} as usize]");
                }
                "self.unstable_period[0]".to_string()
            }
            SpecialBuiltin::Compatibility => "self.compatibility".to_string(),
            pred @ (SpecialBuiltin::IsZero
                   | SpecialBuiltin::IsZeroScaled
                   | SpecialBuiltin::IsZeroOrNeg) => {
                // IS_ZERO / IS_ZERO_SCALED / IS_ZERO_OR_NEG -> the Rust epsilon form.
                // rust_predicate_expr is the single source of that form (also used by
                // the eval_predicate server handler).
                let rendered: Vec<String> = args
                    .iter()
                    .map(|a| render_expr(a, ctx, opt_real_params, registry, helpers))
                    .collect();
                rust_predicate_expr(pred, &rendered)
            }
            SpecialBuiltin::ArrayCopy => {
                if args.len() == 5 {
                    let dst = render_expr(&args[0], ctx, opt_real_params, registry, helpers);
                    let dst_off = render_expr(&args[1], ctx, opt_real_params, registry, helpers);
                    let src = render_expr(&args[2], ctx, opt_real_params, registry, helpers);
                    let src_off = render_expr(&args[3], ctx, opt_real_params, registry, helpers);
                    let count = render_expr(&args[4], ctx, opt_real_params, registry, helpers);
                    return format!(
                        "{{\n            let _n = ({count}) as usize;\n            let _di = ({dst_off}) as usize;\n            let _si = ({src_off}) as usize;\n            {dst}[_di.._di + _n].copy_from_slice(&{src}[_si.._si + _n]);\n        }}"
                    );
                }
                "/* ARRAY_COPY: bad args */".to_string()
            }
            SpecialBuiltin::PerToK => {
                if let Some(arg) = args.first() {
                    let x = render_expr(arg, ctx, opt_real_params, registry, helpers);
                    return format!("2.0_f64 / (({x}) as f64 + 1.0_f64)");
                }
                "0.0_f64".to_string()
            }
        }
    } else if fname.ends_with("_Lookback") {
        let rust_name = fname.to_lowercase();
        let rendered_args: Vec<String> = args
            .iter()
            .map(|a| {
                // Real optIn params stay as f64 in lookback calls (no T-wrapping, no i32 cast)
                let is_opt_real = matches!(a, Expr::Var(n) if !is_i32_opt_in_param(n) && n.starts_with("optIn"));
                if is_opt_real {
                    // Render without T-wrapping
                    let empty_opt: Vec<String> = Vec::new();
                    return render_expr(a, ctx, &empty_opt, registry, helpers);
                }
                // Float literals are Real optIn values — don't cast to i32
                if matches!(a, Expr::Literal(_)) {
                    let empty_opt: Vec<String> = Vec::new();
                    return render_expr(a, ctx, &empty_opt, registry, helpers);
                }
                let rendered = render_expr(a, ctx, opt_real_params, registry, helpers);
                // Lookback functions take i32 params for Integer optIns; cast non-i32 args
                if !expr_is_i32_typed(a) && !matches!(a, Expr::IntLiteral(_)) {
                    format!("({rendered}) as i32")
                } else {
                    rendered
                }
            })
            .collect();
        format!("self.{}({})", rust_name, rendered_args.join(", "))
    } else if fname.ends_with("_lookback") {
        let rendered_args: Vec<String> = args
            .iter()
            .map(|a| {
                // Real optIn params stay as f64 in lookback calls (no T-wrapping, no i32 cast)
                let is_opt_real = matches!(a, Expr::Var(n) if !is_i32_opt_in_param(n) && n.starts_with("optIn"));
                if is_opt_real {
                    let empty_opt: Vec<String> = Vec::new();
                    return render_expr(a, ctx, &empty_opt, registry, helpers);
                }
                // Float literals are Real optIn values — don't cast to i32
                if matches!(a, Expr::Literal(_)) {
                    let empty_opt: Vec<String> = Vec::new();
                    return render_expr(a, ctx, &empty_opt, registry, helpers);
                }
                let rendered = render_expr(a, ctx, opt_real_params, registry, helpers);
                // Lookback functions take i32 params for Integer optIns; cast non-i32 args
                if !expr_is_i32_typed(a) && !matches!(a, Expr::IntLiteral(_)) {
                    format!("({rendered}) as i32")
                } else {
                    rendered
                }
            })
            .collect();
        format!("self.{}({})", fname, rendered_args.join(", "))
    } else if let Some(mf) = MathFn::from_name(fname) {
        // Math functions take priority over the indicator registry.
        // `atan(x)` in source means the C math function, not a cross-indicator call.
        //
        // 2-arg: max/fmax → a.max(b), min/fmin → a.min(b)
        // 1-arg: ABS/fabs → .ta_abs() (generic) or .abs() (concrete)
        // 1-arg: all others → .ta_{fname}() (generic) or .{fname}() (concrete)
        match mf {
            MathFn::Max if args.len() >= 2 => {
                let a = render_expr(&args[0], ctx, opt_real_params, registry, helpers);
                let b = render_expr(&args[1], ctx, opt_real_params, registry, helpers);
                return format!("({a}).max({b})");
            }
            MathFn::Min if args.len() >= 2 => {
                let a = render_expr(&args[0], ctx, opt_real_params, registry, helpers);
                let b = render_expr(&args[1], ctx, opt_real_params, registry, helpers);
                return format!("({a}).min({b})");
            }
            _ => {}
        }
        if let Some(arg) = args.first() {
            let x = render_expr(arg, ctx, opt_real_params, registry, helpers);
            // Use f64 method call syntax; fabs/ABS -> abs, log -> ln
            let method = match mf {
                MathFn::Log => "ln",
                other => other.canonical(),
            };
            // Cast integer args to f64 before calling f64 methods
            let x_wrapped = if matches!(arg, Expr::IntLiteral(_)) {
                if let Expr::IntLiteral(v) = arg {
                    format!("{v}_f64")
                } else {
                    x
                }
            } else if expr_is_untyped_integer(arg) {
                format!("({x} as f64)")
            } else {
                x
            };
            return format!("({x_wrapped}).{method}()");
        }
        format!("{fname}()")
    } else if let Some(s) = StdlibFn::from_name(fname) {
        match s {
            StdlibFn::Sizeof => {
                // sizeof(TYPE) → 1: normalizes byte counts to element counts for Rust array operations
                "1".to_string()
            }
            StdlibFn::Malloc => {
                // malloc(N * sizeof(TYPE)) → vec![default; N as usize]
                // sizeof renders as 1, so the arg is already the element count
                if let Some(arg) = args.first() {
                    let size = render_expr(arg, ctx, opt_real_params, registry, helpers);
                    match find_sizeof_type(arg).as_deref() {
                        Some("int") => format!("vec![0_i32; ({size}) as usize]"),
                        _ => format!("vec![0.0_f64; ({size}) as usize]"),
                    }
                } else {
                    "vec![]".to_string()
                }
            }
            StdlibFn::Free => {
                // No-op in Rust (Vec/Box drops automatically)
                String::new()
            }
            StdlibFn::Memcpy | StdlibFn::Memmove => {
                // memcpy/memmove(dst, src, count) → slice copy. When src and dst
                // resolve to the same backing array (an in-place, possibly
                // overlapping move) copy_from_slice cannot borrow the slice both
                // mutably and immutably, and is UB on overlap anyway, so use
                // copy_within — the overlap-safe primitive memmove exists for.
                // Distinct arrays keep the plain copy_from_slice.
                if args.len() >= 3 {
                    let (dst_arr, dst_off) =
                        decompose_rust_array_ref(&args[0], ctx, opt_real_params, registry, helpers);
                    let (src_arr, src_off) =
                        decompose_rust_array_ref(&args[1], ctx, opt_real_params, registry, helpers);
                    let count = render_expr(&args[2], ctx, opt_real_params, registry, helpers);
                    if dst_arr == src_arr {
                        format!(
                            "{{\n            let _n = ({count}) as usize;\
                             \n            let _di = ({dst_off}) as usize;\
                             \n            let _si = ({src_off}) as usize;\
                             \n            {dst_arr}.copy_within(_si.._si + _n, _di);\
                             \n        }}"
                        )
                    } else {
                        format!(
                            "{{\n            let _n = ({count}) as usize;\
                             \n            let _di = ({dst_off}) as usize;\
                             \n            let _si = ({src_off}) as usize;\
                             \n            {dst_arr}[_di.._di + _n].copy_from_slice(&{src_arr}[_si.._si + _n]);\
                             \n        }}"
                        )
                    }
                } else {
                    format!("/* {fname}: bad args */")
                }
            }
            StdlibFn::Memset => {
                // memset(buf, 0, count) → slice fill
                if args.len() >= 3 {
                    let (arr, off) =
                        decompose_rust_array_ref(&args[0], ctx, opt_real_params, registry, helpers);
                    let count = render_expr(&args[2], ctx, opt_real_params, registry, helpers);
                    let fill_val = match find_sizeof_type(&args[2]).as_deref() {
                        Some("int") => "0_i32".to_string(),
                        _ => "0.0_f64".to_string(),
                    };
                    format!(
                        "{{\n            let _n = ({count}) as usize;\
                         \n            let _si = ({off}) as usize;\
                         \n            {arr}[_si.._si + _n].fill({fill_val});\
                         \n        }}"
                    )
                } else {
                    "/* memset: bad args */".to_string()
                }
            }
        }
    } else if fname == "ta_candlerange" && args.len() == 5 {
        // Inline the full method body — all branches present, no constant propagation.
        // ta_candlerange(rangeType, open, high, low, close)
        let r: Vec<String> = args.iter()
            .map(|a| render_expr(a, ctx, opt_real_params, registry, helpers))
            .collect();
        let (rt, open, high, low, close) = (&r[0], &r[1], &r[2], &r[3], &r[4]);
        format!(
            "(match {rt} {{ 0 => ({close} - {open}).abs(), 1 => {high} - {low}, _ => {high} - {low} - ({close} - {open}).abs() }})"
        )
    } else if fname == "ta_candleaverage" && args.len() == 8 {
        // Inline as a single nested expression (no let bindings) matching C's ternary structure.
        // This enables LLVM loop unswitching on the invariant rangeType checks.
        // ta_candleaverage(rangeType, avgPeriod, factor, sum, open, high, low, close)
        let r: Vec<String> = args.iter()
            .map(|a| render_expr(a, ctx, opt_real_params, registry, helpers))
            .collect();
        let (rt, ap, factor, sum) = (&r[0], &r[1], &r[2], &r[3]);
        let (open, high, low, close) = (&r[4], &r[5], &r[6], &r[7]);
        // Single expression: factor * (if ap!=0 { sum/ap } else { candlerange }) / (if rt==2 { 2.0 } else { 1.0 })
        format!(
            "(({factor}) * (if ({ap}) != 0 {{ ({sum}) / ({ap} as f64) }} else {{ \
             match {rt} {{ 0 => ({close} - {open}).abs(), 1 => ({high}) - ({low}), _ => ({high}) - ({low}) - (({close}) - ({open})).abs() }} \
             }}) / (if ({rt}) == 2 {{ 2.0 }} else {{ 1.0 }}))"
        )
    } else if registry.contains(fname) || fname.ends_with("_private") {
        // Cross-indicator call: use registry to resolve the function name.
        // Bare names (ema) → ema_unguarded (skip validation).
        // Private names (ema_private) → ema_private (generic, handles both f32/f64).
        let resolved = registry.resolve_call(fname, crate::registry::Lang::Rust);
        let (rendered_args, aliased) = render_cross_indicator_args(args, ctx, opt_real_params, registry, helpers);
        wrap_cross_indicator_call(format!("self.{}({})", resolved, rendered_args.join(", ")), &aliased)
    } else if is_ta_function(fname) {
        let rust_name = fname.to_lowercase();
        let (rendered_args, aliased) = render_cross_indicator_args(args, ctx, opt_real_params, registry, helpers);
        wrap_cross_indicator_call(format!("self.{}({})", rust_name, rendered_args.join(", ")), &aliased)
    } else {
        let rendered_args: Vec<String> = args
            .iter()
            .map(|a| render_expr(a, ctx, opt_real_params, registry, helpers))
            .collect();
        format!("{}({})", fname, rendered_args.join(", "))
    }
}

/// Render all arguments for a cross-indicator call, detecting input vs output positions.
/// Cross-indicator signatures follow: startIdx, endIdx, inputs..., opts..., &outBegIdx, &outNBElement, outputs...
/// The two AddressOf args (outBegIdx, outNBElement) mark the boundary.
/// Input-position Vec locals use `&name` (coerces to `&[T]`).
/// Output-position Vec locals use `&mut name[..]` (coerces to `&mut [T]`).
///
/// Returns the rendered arguments plus the list of Vec locals that are passed as **both**
/// input and output (in-place aliasing, e.g. STOCH's in-place `ma` on `tempBuffer`). Rust
/// forbids a simultaneous `&`/`&mut` to the same buffer, so the aliased input is borrowed
/// immutably (`&name`) and the aliased output is redirected to a scratch buffer
/// (`&mut _name_alias[..]`); the caller (`render_func_call`) wraps the call in a block that
/// allocates the scratch buffer and `mem::swap`s the result back — a safe double-buffer that
/// avoids both `unsafe` and a full input clone.
fn render_cross_indicator_args(
    args: &[Expr],
    ctx: &RustRenderCtx,
    opt_real_params: &[String],
    registry: &Registry,
    helpers: &HelperRegistry,
) -> (Vec<String>, Vec<String>) {
    // Find the outBegIdx/outNBElement boundary: two consecutive AddressOf args.
    // Everything after the second one is output slice/array position.
    // Don't use last AddressOf because outputs can also be AddressOf (e.g., &prevATR for scalar T).
    let output_start = find_output_boundary(args);

    // Collect output variable names for aliasing detection
    let output_vars: Vec<&str> = args[output_start..].iter()
        .filter_map(|a| if let Expr::Var(n) = a { Some(n.as_str()) } else { None })
        .collect();

    // Vec locals passed as both input and output (in-place aliasing). Deduped, in first-seen
    // order, so the caller can declare one scratch buffer per aliased var.
    let mut aliased_vars: Vec<String> = Vec::new();
    for arg in &args[..output_start] {
        if let Expr::Var(name) = arg {
            if (ctx.vec_vars.contains(name) || is_vec_local_var(name))
                && output_vars.contains(&name.as_str())
                && !aliased_vars.contains(name)
            {
                aliased_vars.push(name.clone());
            }
        }
    }

    // Detect duplicate AddressOf vars (e.g., &tempInt used for both outBegIdx and outNBElement)
    let mut seen_address_of: std::collections::HashSet<String> = std::collections::HashSet::new();
    // Track which args need a pre-declared dummy (second mutable borrow of same var)
    let mut dup_vars: Vec<(usize, String)> = Vec::new();
    for (i, arg) in args.iter().enumerate() {
        if let Expr::AddressOf(inner) = arg {
            if let Expr::Var(name) = inner.as_ref() {
                if !seen_address_of.insert(name.clone()) {
                    dup_vars.push((i, name.clone()));
                }
            }
        }
    }

    let rendered = args.iter()
        .enumerate()
        .map(|(i, arg)| {
            let is_output = i >= output_start;
            // In-place aliasing: borrow the input immutably and redirect the output to the
            // scratch buffer that `render_func_call` swaps back after the call.
            if let Expr::Var(name) = arg {
                if aliased_vars.contains(name) {
                    if is_output {
                        return format!("&mut _{name}_alias[..]");
                    }
                    return format!("&{name}");
                }
            }
            // Detect duplicate &mut borrows: use the pre-declared dummy variable
            if let Some((_, _)) = dup_vars.iter().find(|(idx, _)| *idx == i) {
                return "&mut _dup_out".to_string();
            }
            render_cross_indicator_arg(arg, i, is_output, ctx, opt_real_params, registry, helpers)
        })
        .collect();
    (rendered, aliased_vars)
}

/// Wrap a cross-indicator `self.fn(args)` call in the safe double-buffer block when the call
/// aliases a Vec local as both input and output; otherwise return the call unchanged.
fn wrap_cross_indicator_call(call: String, aliased_vars: &[String]) -> String {
    if aliased_vars.is_empty() {
        return call;
    }
    let mut decls = String::new();
    let mut swaps = String::new();
    for v in aliased_vars {
        decls.push_str(&format!("let mut _{v}_alias: Vec<f64> = vec![0.0_f64; {v}.len()]; "));
        swaps.push_str(&format!("std::mem::swap(&mut {v}, &mut _{v}_alias); "));
    }
    format!("{{ {decls}let _rc = {call}; {swaps}_rc }}")
}

/// Render a single argument for a cross-indicator call.
/// - `AddressOf(Var(name))` → `&mut name` (C `&scalar` becomes Rust `&mut scalar`)
/// - `Var(name)` where name is a Vec local → `&name` for inputs, `&mut name[..]` for outputs
/// - First two args (idx 0,1 = startIdx, endIdx) cast i32 to usize
/// - Scalar T vars in output position get `std::slice::from_mut()` wrapping
/// - Literal values and optIn Real params are rendered as f64 (not T-wrapped)
fn render_cross_indicator_arg(
    arg: &Expr,
    position: usize,
    is_output_position: bool,
    ctx: &RustRenderCtx,
    opt_real_params: &[String],
    registry: &Registry,
    helpers: &HelperRegistry,
) -> String {
    match arg {
        // &outBegIdxDummy -> &mut outBegIdxDummy
        // Special handling for Vec and scalar T vars in output position
        Expr::AddressOf(inner) => {
            if let Expr::Var(name) = inner.as_ref() {
                if is_output_position && (ctx.vec_vars.contains(name) || is_vec_local_var(name)) {
                    return format!("&mut {name}[..]");
                }
                if is_output_position && ctx.real_vars.contains(name) {
                    return format!("std::slice::from_mut(&mut {name})");
                }
            }
            let rendered = render_expr(inner, ctx, opt_real_params, registry, helpers);
            format!("&mut {rendered}")
        }
        // Vec<T> local variables: &name for input position, &mut name[..] for output position
        Expr::Var(name) if ctx.vec_vars.contains(name) || is_vec_local_var(name) => {
            if is_output_position {
                format!("&mut {name}[..]")
            } else {
                format!("&{name}")
            }
        }
        // Literal values in cross-indicator calls: render as raw f64 (not T-wrapped)
        // because the callee's optIn Real params are f64
        Expr::Literal(f) => {
            #[allow(clippy::float_cmp)]
            let is_whole = *f == f.floor() && f.abs() < 1e15;
            if is_whole {
                #[allow(clippy::cast_possible_truncation)]
                let i = *f as i64;
                format!("{i}.0")
            } else {
                format!("{f}")
            }
        }
        // optIn Real params: render without T-wrapping for cross-indicator calls
        Expr::Var(name) if !is_i32_opt_in_param(name) && name.starts_with("optIn") => {
            name.clone()
        }
        // RealArray vars (e.g., [T; N]): &mut name in output position, &name in input
        Expr::Var(name) if ctx.real_array_vars.contains(name) => {
            if is_output_position {
                format!("&mut {name}")
            } else {
                format!("&{name}")
            }
        }
        _ => {
            let rendered = render_expr(arg, ctx, opt_real_params, registry, helpers);
            // First two positions are startIdx, endIdx (usize) — cast i32 args
            if position <= 1 && (expr_is_i32_typed(arg) || matches!(arg, Expr::BinOp(_, _, _) if has_any_i32_operand(arg))) {
                format!("({rendered}) as usize")
            // Output position: scalar T vars need slice wrapping
            } else if is_output_position {
                if let Expr::Var(name) = arg {
                    if ctx.real_vars.contains(name) {
                        return format!("std::slice::from_mut(&mut {name})");
                    }
                }
                rendered
            // Non-startIdx/endIdx, non-output position: if a usize variable is passed
            // where an i32 param is expected (e.g., curPeriod to ma_unguarded), cast to i32
            } else if position > 1 && !is_output_position {
                if let Expr::Var(name) = arg {
                    if (ctx.index_vars.contains(name) || is_likely_index_var(name))
                        && !name.starts_with("in") && !name.starts_with("out")
                    {
                        return format!("({rendered}) as i32");
                    }
                }
                rendered
            } else {
                rendered
            }
        }
    }
}

/// Find the output boundary in cross-indicator call args.
/// Cross-indicator signatures are: startIdx, endIdx, inputs..., opts..., &outBegIdx, &outNBElement, outputs...
/// The boundary is after the outBegIdx/outNBElement pair. These can be:
/// - AddressOf pairs: `&outBegIdx1, &outNbElement1`
/// - Var pairs when passing through from caller: `outBegIdx, outNBElement`
///
/// Returns the index of the first output arg.
fn find_output_boundary(args: &[Expr]) -> usize {
    // Look for consecutive AddressOf pairs first (starting from position 2+)
    for i in 2..args.len().saturating_sub(1) {
        if matches!(&args[i], Expr::AddressOf(_)) && matches!(&args[i + 1], Expr::AddressOf(_)) {
            return i + 2;
        }
    }
    // Also check for Var pairs that look like outBegIdx/outNBElement
    for i in 2..args.len().saturating_sub(1) {
        if is_beg_nb_var(&args[i]) && is_beg_nb_var(&args[i + 1]) {
            return i + 2;
        }
    }
    // Fallback: no boundary found, all args are non-output
    args.len()
}

/// Check if a Var expression looks like an outBegIdx or outNBElement parameter.
fn is_beg_nb_var(expr: &Expr) -> bool {
    match expr {
        Expr::Var(name) => {
            let lower = name.to_lowercase();
            lower.contains("begidx") || lower.contains("nbelement")
        }
        Expr::AddressOf(inner) => is_beg_nb_var(inner),
        _ => false,
    }
}

/// Check if a FuncCall expression has duplicate AddressOf args (same var borrowed mutably twice).
fn has_duplicate_address_of(expr: &Expr) -> bool {
    if let Expr::FuncCall(_, args) = expr {
        let mut seen = std::collections::HashSet::new();
        for arg in args {
            if let Expr::AddressOf(inner) = arg {
                if let Expr::Var(name) = inner.as_ref() {
                    if !seen.insert(name.clone()) {
                        return true;
                    }
                }
            }
        }
    }
    false
}

/// Check if a BinOp expression contains any i32-typed operands.
fn has_any_i32_operand(expr: &Expr) -> bool {
    match expr {
        Expr::BinOp(left, _, right) => {
            expr_is_i32_typed(left) || expr_is_i32_typed(right)
                || has_any_i32_operand(left) || has_any_i32_operand(right)
        }
        _ => expr_is_i32_typed(expr),
    }
}

/// True if `e` is a bare buffer/slice variable — an input slice (`inReal`,
/// `inHigh`, …), an output slice (`outReal`, `outRealUpperBand`, …), or a
/// Vec/array local (`tempBuffer1`, `firstEMA`, …).
///
/// Used to translate C *pointer-identity* comparisons of whole buffers into Rust
/// pointer comparisons instead of (wrong) element-wise value comparisons. The
/// scalar out-params `outBegIdx`/`outNBElement` are dereferenced as `PointerDeref`
/// in the IR (not bare `Var`) and are excluded by name for belt-and-suspenders.
fn is_buffer_operand(e: &Expr, ctx: &RustRenderCtx) -> bool {
    if let Expr::Var(name) = e {
        if ctx.vec_vars.contains(name)
            || ctx.int_vec_vars.contains(name)
            || ctx.real_array_vars.contains(name)
            || is_vec_local_var(name)
        {
            return true;
        }
        if (name.starts_with("in") || name.starts_with("out"))
            && name != "outBegIdx"
            && name != "outNBElement"
            && !ctx.index_vars.contains(name)
            && !ctx.real_vars.contains(name)
            && !ctx.sentinel_vars.contains(name)
        {
            return true;
        }
    }
    false
}

/// Check if a variable name is likely a Vec<T> local variable (allocated via malloc).
/// These need `&` to convert to `&[T]` when passed to cross-indicator calls.
fn is_vec_local_var(name: &str) -> bool {
    name.starts_with("tempBuffer")
        || name.starts_with("localBuffer")
        || name.starts_with("buffer")
        || name.starts_with("localOutput")
        || name.starts_with("tempOutput")
}

/// Check if an expression returns usize (e.g., lookback function calls, UNSTABLE_PERIOD).
fn expr_returns_usize(expr: &Expr) -> bool {
    match expr {
        Expr::FuncCall(name, _) => {
            // Note: UNSTABLE_PERIOD is NOT usize — it renders as an i32
            // array access (see expr_is_i32_typed), so a bare return of it
            // from a lookback needs the `as usize` cast.
            name.ends_with("_lookback") || name.ends_with("_Lookback")
        }
        _ => false,
    }
}

/// Check if a variable name is an IntArray (i32 element array).
/// These should NOT be wrapped with T::ta_from_i32() when assigned to.
fn is_int_array_var(name: &str) -> bool {
    matches!(name,
        "periods" | "usedFlag" | "sortedPeriods"
    )
}

/// Helper functions that return int (not double/T).
/// These must NOT be treated as float-typed by `expr_is_float_typed`.
fn is_integer_returning_helper(name: &str) -> bool {
    matches!(name,
        "ta_candlecolor" | "ta_realbodygapup" | "ta_realbodygapdown"
        | "ta_candlegapup" | "ta_candlegapdown"
    )
}

/// Check if a variable name is likely an index/counter (usize) rather than a Real (T) variable.
fn is_likely_index_var(name: &str) -> bool {
    // Never match optIn params — they are i32 in the function signature
    if name.starts_with("optIn") {
        return false;
    }
    name == "startIdx" || name == "endIdx" || name == "lookbackTotal"
        || name == "trailingIdx" || name == "today" || name == "i"
        || name == "outIdx" || name == "nbInitialElementNeeded"
        || name == "nbElement" || name == "nbElementToOutput"
        || name.ends_with("Idx")
        || name == "outBegIdx" || name == "outNBElement"
        || name.starts_with("nb")
        || name == "j" || name == "count"
        || name == "outputSize"
        || name.ends_with("Dummy") || name.ends_with("_idx")
        || name == "highestIdx" || name == "lowestIdx"
        || name == "isLong" || name == "isShort"
        || name == "currentBar"
        || name == "tempInteger" || name == "tempInt"
        || name == "tempMAType"
        || name == "retValue"
        || name == "trend" || name == "daysInTrend"
        || name == "patternResult" || name == "patternIdx"
        || name == "maxPeriod" || name == "longestPeriod"
        || name == "longestIndex" || name == "divider"
        || name == "curPeriod"
}

fn is_ta_function(name: &str) -> bool {
    !name.is_empty()
        && name.chars().all(|c| c.is_ascii_uppercase() || c == '_')
        && !matches!(
            name,
            "UNSTABLE_PERIOD"
                | "IS_ZERO"
                | "IS_ZERO_OR_NEG"
                | "ARRAY_COPY"
                | "PER_TO_K"
                | "COMPATIBILITY"
                | "METASTOCK"
                | "DEFAULT"
        )
        && !name.ends_with("_Lookback")
}

fn gen_footer() -> String {
    "/***************/\n\
     /* End of File */\n\
     /***************/\n"
        .to_string()
}
