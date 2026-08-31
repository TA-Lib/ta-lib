use std::cell::Cell;
use std::collections::HashMap;

use crate::candle_settings::{detect_candle_settings, emit_rust_unpacking};
use crate::helper_registry::{hoist_block_helpers, try_inline_expr, HelperRegistry};
use crate::ir::{
    BinOp, CircBuf, CircBufLayout, EnumDef, Expr, FuncDef, LookbackExpr, OptInput, Output,
    ParamType, Statement, VarType,
};
use crate::parser::enums::lookup_variant;
use crate::registry::Registry;
use super::common::{contains_alloc_err_return, expr_directly_contains_candle_call, find_sizeof_type};
use super::ir_cleanup;

/// Words this backend cannot render as an identifier (see [`crate::naming`]):
/// the strict keywords of every edition through 2024, plus the set reserved for
/// future use. Raw identifiers (`r#loop`) would sidestep most of these, but the
/// generator never emits one — a name that needs escaping to compile is a name
/// the input should not have used.
///
/// The weak keywords (`union`, `macro_rules`, `'static`, `dyn` pre-2018, and the
/// 2024 `safe`) are deliberately absent: they are legal identifiers.
pub(crate) const RESERVED_WORDS: &[&str] = &[
    // --- strict keywords ---
    "as",
    "async",
    "await",
    "break",
    "const",
    "continue",
    "crate",
    "dyn",
    "else",
    "enum",
    "extern",
    "false",
    "fn",
    "for",
    "if",
    "impl",
    "in",
    "let",
    "loop",
    "match",
    "mod",
    "move",
    "mut",
    "pub",
    "ref",
    "return",
    "self",
    "static",
    "struct",
    "super",
    "trait",
    "true",
    "type",
    "unsafe",
    "use",
    "where",
    "while",
    // `Self` is not listed: the match is case-insensitive, so `self` above
    // already covers it.
    // --- reserved for future use ---
    "abstract",
    "become",
    "box",
    "do",
    "final",
    "gen",
    "macro",
    "override",
    "priv",
    "try",
    "typeof",
    "unsized",
    "virtual",
    "yield",
];
use super::builtins::{MathFn, SpecialBuiltin, StdlibFn};
use super::expr_walk::{is_int_bitwise, ExprEmitter};
use super::fma::{self, is_i32_opt_in_param, is_integer_returning_helper, FmaCtx};
use super::stmt_walk::StatementEmitter;

/// Controls how the Rust renderer emits code.
#[derive(Clone)]
pub struct RustRenderCtx {
    /// Whether a `for(i=a; i<=b; i++)` may be lowered to `for i in a..b+1`.
    /// The lowering rebinds the counter as `usize`, which is right where the
    /// counter is a batch local and wrong in a peek frame, where it is a state
    /// field the handle declares `i32` — and it only becomes reachable there
    /// because the frame's locals drop the `sp.` qualifier the gate keys on.
    /// Off, the loop takes the same generic fallback the step takes.
    pub for_range_lowering: bool,
    /// If true, emit a pre-loop bounds-assert preamble at the top of the body. The
    /// asserts give LLVM the proof it needs to elide the per-access bounds checks on
    /// the safe `[]` indexing that follows — the generated code never uses `unsafe`.
    /// See `emit_bounds_asserts`.
    pub bounds_asserts: bool,
    /// Variable names declared as `VarType::Integer` or `VarType::Index` (usize in Rust).
    /// Used by type inference in expression rendering.
    pub index_vars: std::collections::HashSet<String>,
    /// Locals that hold an enum value rather than an integer, mapped to
    /// `(type, initialiser)` — MACDEXT's `int tempMAType`, which the C declares
    /// as an int but only ever assigns from an MAType parameter. Derived by
    /// [`enum_local_types`] from the same analysis Java and C# use, so no
    /// variable is named here. Such a local is deliberately kept OUT of
    /// `index_vars`: it must not take the integer casts.
    pub enum_vars: std::collections::HashMap<String, String>,
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
    /// If true, renderer-generated error returns (CIRCBUF init guard) emit
    /// `return Err(RetCode::X);` — the stream tier's `Result` shape — instead
    /// of the batch tier's bare `return RetCode::X;`.
    pub result_error_returns: bool,
    /// Fully-qualified MAType constant (`TA_MAType_SMA`) → its Rust rendering
    /// (`matype::SMA`, the generated crate-internal value), derived by
    /// [`build_matype_map`]. Populated for batch/lookback bodies — the only
    /// place `optInMAType == TA_MAType_*` comparisons render; stream bodies
    /// dispatch MA-type structurally (case labels / sub-opens) and leave this
    /// empty. Empty ⇒ the constant renders literally (unresolved), which a
    /// build catches immediately.
    pub matype_map: std::collections::HashMap<String, String>,
    /// CIRCBUF ids rendered with the C-style hybrid storage (stack array up to
    /// the PROLOG static size, heap `Vec` above it), mapped to that static
    /// size. Populated for batch bodies only; stream bodies leave it empty and
    /// keep pure-`Vec` storage, whose ownership the open path moves into the
    /// stream state struct.
    pub circbuf_hybrid_static: std::collections::HashMap<String, i64>,
    /// Output parameters typed `Option<&mut [T]>` because their .yaml marks them
    /// `nullable` (rule B6a). Every store into one is wrapped in an `if let
    /// Some(..) = ..as_deref_mut()`, so a caller that passed `None` is skipped.
    /// Populated for the batch bodies and the stream tier's transcribed open
    /// region; the step/peek frames keep their outputs required and leave
    /// this empty.
    pub nullable_outputs: std::collections::HashSet<String>,
    /// When true, a guarded nullable store also assigns `lastCur_<out>`
    /// unconditionally, beside the guard rather than inside it — the same
    /// scheme Java/C# already used (`emit_cur_capture`'s `CurSource`) and C
    /// mirrors too. The stream Open region is the only caller that needs the
    /// handle's `cur_<out>` to hold what the store *would* have written even
    /// when the caller declined the output: an Update always recomputes it,
    /// so a captured value that instead reads the (possibly absent) output
    /// array diverges from `Open(P)+updates` the instant the store isn't a
    /// bare identity copy. Off everywhere else, including the batch function,
    /// which has no `cur_*` state to feed.
    pub nullable_shadow: bool,
}

/// Locals that carry an enum value, mapped to their Rust type.
///
/// C declares MACDEXT's swap temporary as `int tempMAType;` and only ever
/// assigns an MAType parameter to it, so once the parameter is typed the local
/// must be too. Java and C# already derive this with `collect_matype_vars`;
/// this is the same call, so the three backends agree by construction rather
/// than by three copies of a name.
///
/// Needs no enums table — the type is the one the parameter declares — which is
/// what lets the streaming tier populate it as cheaply as the batch tier.
pub(crate) fn enum_local_types(func: &FuncDef) -> std::collections::HashMap<String, String> {
    let mut out = std::collections::HashMap::new();
    // Both bodies: the guarded and private variants render from different
    // statement lists, and a local is the same type in each.
    let body: Vec<Statement> = func
        .body
        .iter()
        .cloned()
        .chain(func.private_body.iter().cloned())
        .collect();
    for opt in &func.optional_inputs {
        let ParamType::Enum(enum_name) = &opt.param_type else {
            continue;
        };
        // The parameter itself belongs here too: it is no longer an `i32`, so
        // the cast inference must stop treating every `optIn*` name as one.
        out.insert(opt.name.clone(), enum_name.clone());
        let params: std::collections::HashSet<String> =
            std::iter::once(opt.name.clone()).collect();
        for v in super::java::collect_matype_vars(&body, &params) {
            out.insert(v, enum_name.clone());
        }
    }
    out
}

/// Drop enum-typed locals from an integer set.
///
/// `collect_var_types` files them under `index_vars` because C declares them
/// `int`; left there, every assignment to one takes an `as usize` and every read
/// an `as i32`. They are neither.
pub(crate) fn prune_enum_locals(
    index_vars: &mut std::collections::HashSet<String>,
    enum_vars: &std::collections::HashMap<String, String>,
) {
    for name in enum_vars.keys() {
        index_vars.remove(name);
    }
}

/// The Rust type of an optional parameter.
///
/// An `enum:` parameter is spelled as its enum, exactly as the YAML declares it
/// and as C, Java and C# emit it — never folded in with `Integer`, which hands
/// callers a bare `i32`.
pub(crate) fn opt_param_type(t: &ParamType) -> String {
    match t {
        ParamType::Real => "f64".to_string(),
        ParamType::Enum(name) => name.clone(),
        ParamType::Integer | ParamType::Price(_) => "i32".to_string(),
    }
}

/// Build the `TA_MAType_*` → qualified-member map the [`ExprEmitter::var`] hook
/// uses to render `optInMAType == TA_MAType_SMA` comparisons in the batch
/// functions. Derived from the `MAType` enum in `enums.yaml`, so a new
/// `TA_MAType_X` row needs no generator edit.
pub(crate) fn build_matype_map(
    enums: &HashMap<String, EnumDef>,
) -> std::collections::HashMap<String, String> {
    enums
        .get("MAType")
        .map(|e| {
            e.variants
                .iter()
                .map(|v| (v.c_name.clone(), format!("{}::{}", e.name, v.name)))
                .collect()
        })
        .unwrap_or_default()
}

impl RustRenderCtx {
    /// Borrowing view of the name-sets the shared FMA detector needs, so Rust
    /// fuses the identical sites C/Java derive from [`fma::build_fma_var_sets`].
    pub(crate) fn fma_view(&self) -> FmaCtx<'_> {
        FmaCtx {
            real_vars: &self.real_vars,
            index_vars: &self.index_vars,
            real_array_vars: &self.real_array_vars,
            int_output_names: &self.int_output_names,
            sentinel_vars: &self.sentinel_vars,
        }
    }

    /// A context that classifies nothing. Only useful as a starting point the
    /// caller fills in — every renderer that sees real input builds its sets
    /// from the body (issue #158). `is_lookback` starts `true`; callers using
    /// this as a blank body context clear it.
    pub fn empty() -> Self {
        RustRenderCtx {
            for_range_lowering: true,
            bounds_asserts: false,
            index_vars: std::collections::HashSet::new(),
            real_vars: std::collections::HashSet::new(),
            vec_vars: std::collections::HashSet::new(),
            real_array_vars: std::collections::HashSet::new(),
            int_output_names: std::collections::HashSet::new(),
            int_vec_vars: std::collections::HashSet::new(),
            is_lookback: true,
            sentinel_vars: std::collections::HashSet::new(),
            result_error_returns: false,
            matype_map: std::collections::HashMap::new(),
            enum_vars: std::collections::HashMap::new(),
            circbuf_hybrid_static: std::collections::HashMap::new(),
            nullable_outputs: std::collections::HashSet::new(),
            nullable_shadow: false,
        }
    }

    /// Context for a `LookbackExpr::Code` body.
    ///
    /// Must carry the body's declarations (#158). Built empty, every local in a
    /// lookback falls through to the naming heuristics — `expr_is_float_typed`
    /// hard-codes `k` as a Real name, EMA's k factor — so the same code compiles
    /// or does not depending on what its locals are called. The declared IR type
    /// decides, exactly as it does for batch bodies.
    pub fn for_lookback(body: &[Statement]) -> Self {
        let mut index_vars = std::collections::HashSet::new();
        let mut real_vars = std::collections::HashSet::new();
        let mut vec_vars = std::collections::HashSet::new();
        let mut real_array_vars = std::collections::HashSet::new();
        let mut int_vec_vars = std::collections::HashSet::new();
        collect_var_types(
            body,
            &mut index_vars,
            &mut real_vars,
            &mut vec_vars,
            &mut real_array_vars,
            &mut int_vec_vars,
        );
        // Same signed-local pipeline as the batch ctx, so a lookback local that
        // participates in signed arithmetic is declared i32 and used as i32
        // (`render_lookback_code` honours `sentinel_vars` in its decl emitter).
        let mut sentinel_vars = std::collections::HashSet::new();
        collect_sentinel_vars(body, &mut sentinel_vars);
        collect_signed_int_vars(body, &index_vars, &real_vars, &mut sentinel_vars);
        // The signed election is only sound alongside #160's rejection of the
        // cast shapes this backend cannot render sign-faithfully. Batch bodies
        // have run this since b8619ed6b; opting the lookback tier into the
        // election without it would let a nested `(int)` of a negative double
        // through here alone.
        reject_unsupported_negative_casts(body, &real_vars, "lookback");
        for sv in &sentinel_vars {
            index_vars.remove(sv);
        }
        RustRenderCtx {
            index_vars,
            real_vars,
            vec_vars,
            real_array_vars,
            int_vec_vars,
            sentinel_vars,
            // `matype_map` is populated by the caller (which has `enums`);
            // lookback bodies never reference MA-type constants, but keep it
            // consistent with batch.
            ..RustRenderCtx::empty()
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

/// Does this expression evaluate to `i32` because an `int` array element drives it?
///
/// True for a subscript of an `int` array, and — recursively — for arithmetic over
/// one whose other operand cannot widen the result (another such subscript, or an
/// integer literal).
///
/// This is deliberately **not** folded into [`expr_is_i32_typed_ctx`]. That
/// predicate feeds the sentinel test (`ctx-typed i32 && !plain-typed i32`), so
/// teaching it about arrays would make every `int` array element look like an
/// opt-param sentinel and route the comparison down the branch that narrows the
/// *other* side with `as i32` — turning `(periods[j] as usize) > longestPeriod`
/// into `periods[j] > (longestPeriod as i32)`, an index-domain value narrowed to
/// i32. Keeping it separate leaves a direct subscript rendering exactly as before
/// and extends only the nested case. Issue #163.
fn expr_is_int_array_typed(expr: &Expr, ctx: &RustRenderCtx) -> bool {
    match expr {
        Expr::ArrayAccess(name, _) => is_int_array_or_vec(name, ctx),
        Expr::BinOp(
            left,
            BinOp::Add
            | BinOp::Sub
            | BinOp::Mul
            | BinOp::Div
            | BinOp::Mod
            | BinOp::Shl
            | BinOp::Shr
            | BinOp::BitwiseAnd
            | BinOp::BitwiseOr
            | BinOp::BitwiseXor,
            right,
        ) => {
            // The result stays i32 only if the other operand cannot widen it:
            // another int-array element, an already-i32-typed expression (an
            // opt-in param, UNSTABLE_PERIOD, an explicit `(int)` cast), or an
            // untyped integer literal. `optInTimePeriod` is the one that matters
            // in practice — evicting a deque index that has left a period-sized
            // window is spelled `dqI[hd] + optInTimePeriod < today`.
            //
            // expr_is_i32_typed, not the _ctx form: sentinels must stay out, for
            // the reason in this function's doc comment.
            let stays_i32 = |e: &Expr| {
                expr_is_int_array_typed(e, ctx)
                    || expr_is_i32_typed(e)
                    || matches!(e, Expr::IntLiteral(_))
            };
            expr_is_int_array_typed(left, ctx) && stays_i32(right)
                || expr_is_int_array_typed(right, ctx) && stays_i32(left)
        }
        _ => false,
    }
}

#[allow(clippy::implicit_hasher)]
pub fn generate(
    func: &FuncDef,
    enums: &HashMap<String, EnumDef>,
    registry: &Registry,
    helpers: &HelperRegistry,
) -> String {
    // Resolve `PRAGMA TA_ALT` for this language (ir::FuncDef::resolved_for).
    let resolved = func.resolved_for(crate::ir::Lang::Rust);
    let func: &FuncDef = &resolved;
    let mut out = String::new();
    out.push_str(&gen_header());
    // File-level comments carried from the input .c (e.g. contributors/history).
    for block in &func.header_comments {
        out.push_str(&super::stmt_walk::block_comment(block, 0));
        out.push('\n');
    }
    out.push_str(&gen_imports());
    // Name the alternate that won the batch cell, if one did.
    if let Some(m) = func.alt_marker(crate::ir::Tier::Batch, crate::ir::Lang::Rust) {
        out.push_str(&format!("/* {m} */\n\n"));
    }
    out.push_str(&gen_impl_block(func, enums, registry, helpers));
    // Streaming API section (only for YAML-declared streamable functions).
    if func.streaming {
        out.push_str(&super::rust_stream::generate(func, enums, registry, helpers));
    }
    out.push_str(&gen_footer());
    out
}

fn gen_header() -> String {
    let mut out = String::new();
    out.push_str("/* TA-LIB Copyright (c) 1999-2026, Mario Fortier\n");
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
    out.push_str(" *  Modifications should instead be done in the function's canonical\n");
    out.push_str(" *  definition under ta_codegen/input/<name>/ — src/ta_func holds the\n");
    out.push_str(" *  generated C, not the source of truth.\n");
    out.push_str(" */\n\n");
    out
}

fn gen_imports() -> String {
    "// Import types from parent module\n\
     use super::*;\n\n"
        .to_string()
}

/// Runtime FMA dispatch (issue #156). When a rendered batch variant contains
/// fused multiply-adds, restructure it into the Rust analogue of the C
/// backend's `TA_FMA_MULTIVERSION` (`target_clones("default","fma")`): the
/// body moves verbatim to a private `{name}_impl` (`#[inline(always)]`, so it
/// compiles once per clone), `{name}_fma` is a `#[target_feature(enable =
/// "fma")]` clone whose codegen turns every `mul_add` into a hardware
/// `vfmadd`, and the public name becomes a dispatcher through
/// `ta_lib_dispatch::dispatch_fma!` (one cached CPU check per call; both
/// paths are correctly rounded, so which clone runs never changes bits).
/// Lookback and the stream tier stay undispatched, mirroring the C decision.
fn fma_dispatch_wrap(text: String, fn_name: &str, vis: &str) -> String {
    if !fma::EMIT_FMA || !text.contains(".mul_add(") {
        return text;
    }
    // From here the variant fuses, so a signature-pattern mismatch must fail
    // LOUD: a silent passthrough would ship the exact #156 regression while
    // every value gate stays green (both paths are bit-identical).
    let sig_open = format!("    {vis}fn {fn_name}(\n");
    let sig_close = "    ) -> RetCode {\n";
    let Some(sig_pos) = text.find(&sig_open) else {
        panic!("{fn_name}: fused body but the signature no longer matches `{sig_open:?}` — FMA dispatch would silently vanish");
    };
    let params_start = sig_pos + sig_open.len();
    let Some(close_rel) = text[params_start..].find(sig_close) else {
        panic!("{fn_name}: fused body but the signature terminator no longer matches `{sig_close:?}` — FMA dispatch would silently vanish");
    };
    let params_end = params_start + close_rel;
    let body_start = params_end + sig_close.len();
    // Only the *body* decides (site selection lives in fma::fuse_operands;
    // `.mul_add(` in the rendered body is its footprint) — a mention in the
    // doc comment alone must not dispatch.
    if !text[body_start..].contains(".mul_add(") {
        return text;
    }

    let header = &text[..sig_pos]; // docs + attributes (#[inline], #[doc(alias)], ...)
    let raw_params = &text[params_start..params_end]; // one `name: type,` per line, `&self,` first
    let body = &text[body_start..]; // through the fn's closing brace

    // Forwarding names, plus a `mut`-free signature for the dispatcher and
    // clone (only `_impl` keeps the original `mut` bindings).
    let mut clean_sig = String::new();
    let mut names: Vec<&str> = Vec::new();
    for line in raw_params.lines() {
        let t = line.trim();
        if t == "&self," {
            clean_sig.push_str("        &self,\n");
            continue;
        }
        let decl = t.strip_suffix(',').unwrap_or(t);
        let (name_part, ty) = decl
            .split_once(": ")
            .unwrap_or_else(|| panic!("unparsable param line in {fn_name}: {line}"));
        let name = name_part.strip_prefix("mut ").unwrap_or(name_part);
        clean_sig.push_str(&format!("        {name}: {ty},\n"));
        names.push(name);
    }
    let args = names.join(", ");

    let mut out = String::new();
    // 1. Public dispatcher: original name, original docs and attributes.
    out.push_str(header);
    out.push_str(&sig_open);
    out.push_str(&clean_sig);
    out.push_str(sig_close);
    out.push_str("        #[cfg(target_arch = \"x86_64\")]\n");
    out.push_str(&format!(
        "        return ta_lib_dispatch::dispatch_fma!(self, {fn_name}_fma, {fn_name}_impl, ({args}));\n"
    ));
    out.push_str("        #[cfg(not(target_arch = \"x86_64\"))]\n");
    out.push_str(&format!("        self.{fn_name}_impl({args})\n"));
    out.push_str("    }\n");
    // 2. The FMA clone: same body via forced inlining, codegen'd with fma on.
    out.push_str("    #[cfg(target_arch = \"x86_64\")]\n");
    out.push_str("    #[target_feature(enable = \"fma\")]\n");
    out.push_str(&format!("    fn {fn_name}_fma(\n"));
    out.push_str(&clean_sig);
    out.push_str(sig_close);
    out.push_str(&format!("        self.{fn_name}_impl({args})\n"));
    out.push_str("    }\n");
    // 3. The portable implementation: the original function, renamed.
    out.push_str("    #[inline(always)]\n");
    out.push_str(&format!("    fn {fn_name}_impl(\n"));
    out.push_str(raw_params);
    out.push_str(sig_close);
    out.push_str(body);
    out
}

fn gen_impl_block(func: &FuncDef, enums: &HashMap<String, EnumDef>, registry: &Registry, helpers: &HelperRegistry) -> String {
    let mut out = String::new();
    let snake = func.name.clone();

    // C's pointer-based scratch-buffer election becomes a rename here, so the
    // batch bodies below run the calculation directly in the caller's output
    // slices instead of allocating and copying (issue #146). Rust-only: the C,
    // Java and C# backends assign the pointer/reference and need no rewrite,
    // and the stream tier keeps the untransformed `func` (it composes its own
    // scratch buffers). See [`ScratchElection`].
    let mut elected = elect_output_scratch(func);
    // A cross-call's rejection is answered by the `match` arm the renderer emits
    // (#267), so the transcribed guard on the code it assigns is dead. Fold it
    // out here, ahead of everything derived from the body below, so nothing
    // downstream disagrees about which statements exist. `cross_call_split` is
    // the same admission test the renderer uses -- see its doc for why it is not
    // shared with Java and C#.
    let admits = |f: &str, args: &[Expr]| cross_call_split(f, args, registry).is_some();
    for body in [&mut elected.body, &mut elected.private_body] {
        *body = ir_cleanup::drop_answered_cross_call_guards(body, &admits, None);
        *body = ir_cleanup::drop_deallocation(body);
        *body = ir_cleanup::drop_inert_guards(body);
    }
    let func = &elected;

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

    // Guarded public entry: validates params, then either renders the algorithm
    // inline or delegates to `_private`.
    out.push_str(&fma_dispatch_wrap(
        gen_guarded_func(func, &snake, enums, registry, helpers),
        &format!("{snake}_Impl"),
        "pub(crate) ",
    ));
    out.push_str(&gen_public_entry(func, &snake, enums, registry));

    // Build a temporary FuncDef with private_body for the `_private` variant
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
    // Also detect integer variables that participate in signed arithmetic
    // (< 0, 0 - N, negative-capable casts — issue #160). Deliberately NOT
    // transitive through var-to-var copies: propagating the extremum family's
    // -1 sentinels into their loop indices churned 14 hot files for no
    // behavior change. A local needing signedness must be assigned a signed
    // EXPRESSION (cast, negative literal, 0-N) directly.
    collect_signed_int_vars(&body_func.body, &index_vars, &real_vars, &mut sentinel_vars);
    reject_unsupported_negative_casts(&body_func.body, &real_vars, &func.name);
    // Remove sentinel/signed vars from index_vars — they're i32, not usize
    for sv in &sentinel_vars {
        index_vars.remove(sv);
    }

    // Collect integer output names for i32 cast detection
    let int_output_set: std::collections::HashSet<String> = func.outputs.iter()
        .filter(|o| o.param_type == ParamType::Integer)
        .map(|o| o.name.clone())
        .collect();

    let enum_vars = enum_local_types(func);
    prune_enum_locals(&mut index_vars, &enum_vars);
    let ctx = RustRenderCtx {
            for_range_lowering: true,
        bounds_asserts: true,
        index_vars,
        real_vars,
        vec_vars,
        real_array_vars,
        int_output_names: int_output_set,
        int_vec_vars,
        is_lookback: false,
        sentinel_vars,
        result_error_returns: false,
        matype_map: build_matype_map(enums),
        enum_vars,
        circbuf_hybrid_static: collect_circbuf_static(&func.body),
        nullable_outputs: super::common::nullable_output_names(func),
        nullable_shadow: false,
    };

    // `_private` holds the algorithm for the functions that declare one (Rust has
    // no single-precision variant), and the guarded body above delegates to it.
    // Functions without an explicit `_private` render the algorithm inline in the
    // guarded body.
    if func.has_explicit_private {
        out.push_str(&fma_dispatch_wrap(
            gen_private_func(&body_func, &snake, &ctx, enums, registry, helpers),
            &format!("{snake}_Private"),
            "pub(crate) ",
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
            out.push_str(&format!("        return Ok({n});\n"));
        }
        Some(LookbackExpr::ParamMinus(param, offset)) => {
            out.push_str(&format!("        return Ok(({param} - {offset}) as usize);\n"));
        }
        Some(LookbackExpr::Code(stmts)) => {
            out.push_str(&render_lookback_code(stmts, func, enums, registry, helpers));
        }
        None => {
            out.push_str("        return Ok(0);\n");
        }
    };

    if has_opt_inputs {
        // Build parameter list
        let mut params = Vec::new();
        for opt in &func.optional_inputs {
            let rust_type = opt_param_type(&opt.param_type);
            params.push(format!("mut {}: {}", opt.name, rust_type));
        }

        out.push_str("    #[inline]\n");
        out.push_str(&format!(
            "    pub fn {}_Lookback(&self, {}) -> Result<usize, RetCode> {{\n",
            snake,
            params.join(", ")
        ));

        // Param validation
        for opt in &func.optional_inputs {
            out.push_str(&gen_opt_param_validation(opt, "        ", true, enums));
        }

        // Return lookback expression
        emit_lookback_return(&mut out);
    } else {
        out.push_str(&format!("    pub fn {snake}_Lookback(&self) -> Result<usize, RetCode> {{\n"));
        emit_lookback_return(&mut out);
    }

    out.push_str("    }\n");
    out
}

/// The name the LEGACY expression-position rendering calls a sibling indicator
/// by — the C-shaped `<N>_Impl`.
///
/// A transcribed body does not reach this: a cross-call is rewritten at
/// statement level by [`render_cross_indicator_call`], which names the public
/// tier (#267). What is left here is the fallback for a cross-call in
/// expression position, a shape no definition in the corpus has (the same status
/// Java's registry branch in `render_func_call` has had since #236 step 3), and
/// the `_Private` carve-out, which already names a distinct function.
fn internal_callee(name: &str) -> String {
    if name.ends_with("_Private") {
        name.to_string()
    } else {
        format!("{name}_Impl")
    }
}

/// Generate the batch entry point the crate actually exposes: the argument
/// contract, then `{snake}_Impl`, returning `Result<OutRange, RetCode>`.
///
/// This is the same two-tier shape Java and C# ship — their public `SMA` checks
/// its arguments, calls the code-returning one and turns a failure into an
/// exception, returning `OutRange` otherwise. Here the exception is an `Err`, so
/// the two values a caller wants (`begIdx`, `count`) come back by value and are
/// unreachable on failure, which is exactly the guarantee C's "ignore the
/// out-params unless TA_SUCCESS" rule states in prose and cannot enforce.
///
/// **The buffer bounds live here, not in the body** (#265). `emit_bounds_asserts`
/// stays where it is and states the same thing to LLVM, but an `assert!` is a
/// panic, and a caller who handed a short slice deserves the code every other
/// backend gives them. The two are consistent by construction: the output bound
/// below is the same inequality the assert makes, and the input bound is
/// strictly stronger — it drops the `_assertStart > endIdx ||` escape, so a
/// short input is rejected on a sub-lookback range too, as in Java and C#.
/// **Every cross-indicator call reaches this tier too** (#267), so the bound a
/// re-based chain meets is this one, not the assert's — the same bound Java has
/// applied at the identical call sites since #236 step 3. What the assert's
/// escape is still for is a call the lookback clamp leaves nothing to compute,
/// and the phantom-I/O sweep, which hands `<N>_Impl` zero-length slices on
/// purpose.
///
/// **Order is the contract, not an implementation detail.** The index rules
/// (B1, B2) first, then the parameters (B3, carried by the `<N>_Lookback` call's
/// `?`), then the buffers (B4, B5) — `docs/error-handling-spec.md` 2.2, and the
/// same order [`super::java::gen_argument_checks`] emits. Put the input bound at
/// the top and `SMA(10, 9, ..)` answers `BadParam` where `test_index_range_xlang`
/// requires `OutOfRangeEndIndex`.
fn gen_public_entry(
    func: &FuncDef,
    snake: &str,
    enums: &HashMap<String, EnumDef>,
    registry: &Registry,
) -> String {
    let mut out = String::new();

    // Doc comments + #[doc(alias)] attributes from the canonical <name>.md.
    out.push_str(&super::rust_doc::guarded_docs(func, snake, enums, registry));

    out.push_str(&format!("    pub fn {snake}(\n"));
    out.push_str("        &self,\n");
    out.push_str("        startIdx: usize,\n");
    out.push_str("        endIdx: usize,\n");
    // Same parameters as the internal entry point, minus its `mut` bindings (this
    // body only forwards) and minus the two out-params, which become the return.
    for input in &func.inputs {
        let ty = match input.param_type {
            ParamType::Real => "&[f64]",
            ParamType::Integer | ParamType::Enum(_) | ParamType::Price(_) => "&[i32]",
        };
        out.push_str(&format!("        {}: {},\n", input.name, ty));
    }
    for opt in &func.optional_inputs {
        out.push_str(&format!("        {}: {},\n", opt.name, opt_param_type(&opt.param_type)));
    }
    out.push_str(&gen_generic_output_params(func, false));
    out.push_str("    ) -> Result<OutRange, RetCode> {\n");

    let mut args: Vec<String> = vec!["startIdx".to_string(), "endIdx".to_string()];
    args.extend(func.inputs.iter().map(|i| i.name.clone()));
    args.extend(func.optional_inputs.iter().map(|o| o.name.clone()));
    args.push("&mut outBegIdx".to_string());
    args.push("&mut outNBElement".to_string());
    args.extend(func.outputs.iter().map(|o| o.name.clone()));

    out.push_str(&gen_argument_checks(func, snake));

    out.push_str("        let mut outBegIdx: usize = 0;\n");
    out.push_str("        let mut outNBElement: usize = 0;\n");
    // Always one argument per line: the shortest call in the corpus is already
    // past a sensible width, so a single-line form would be dead code.
    out.push_str(&format!("        let retCode = self.{snake}_Impl(\n"));
    for a in &args {
        out.push_str(&format!("            {a},\n"));
    }
    out.push_str("        );\n");
    out.push_str("        match retCode {\n");
    out.push_str("            RetCode::Success => Ok(OutRange { beg_idx: outBegIdx, count: outNBElement }),\n");
    out.push_str("            e => Err(e),\n");
    out.push_str("        }\n");
    out.push_str("    }\n\n");
    out
}

/// The public tier's argument contract: rules B1/B2, then B3, then B4/B5.
///
/// The Rust transcription of [`super::java::gen_argument_checks`], down to the
/// two guard widths. `guardInLen` is `endIdx + 1` **unconditionally** — `endIdx`
/// past the end of the series the caller supplied is a caller bug on every
/// range, and the only reason C answers it with `TA_SUCCESS` is that it has no
/// size to check against. `guardOutLen` is the count actually produced, which on
/// a range shorter than the lookback is `0`: no output space is owed, so any
/// length will do, including none (rule N1).
///
/// B3 rides on `<N>_Lookback`'s `?`. Rule L2 makes the lookback's parameter
/// decision the batch tier's own B3 decision on the same parameters, so one call
/// buys the check and the clamp together — which is what Java's `clampedStart`
/// does, and what puts B3 ahead of B4/B5.
///
/// No `requireArgument` counterpart: a Rust enum cannot be absent, so B4's
/// presence half is the type system's, as it is in C#.
fn gen_argument_checks(func: &FuncDef, snake: &str) -> String {
    let mut out = String::new();
    // B1/B2 first. `_Impl` states them again -- it is reachable on its own from
    // a cross-indicator call -- but they have to be HERE, ahead of the buffer
    // bounds, or a malformed range answers the wrong code. They also make
    // `endIdx + 1` below non-overflowing.
    out.push_str("        if startIdx > Self::MAX_INDEX {\n");
    out.push_str("            return Err(RetCode::OutOfRangeStartIndex);\n");
    out.push_str("        }\n");
    out.push_str("        if endIdx > Self::MAX_INDEX || endIdx < startIdx {\n");
    out.push_str("            return Err(RetCode::OutOfRangeEndIndex);\n");
    out.push_str("        }\n");
    if func.inputs.is_empty() && func.outputs.is_empty() {
        return out;
    }
    let lb_args: Vec<String> = func.optional_inputs.iter().map(|o| o.name.clone()).collect();
    out.push_str(&format!(
        "        let _guardLb = self.{snake}_Lookback({})?;\n",
        lb_args.join(", ")
    ));
    out.push_str(
        "        let _guardStart = if startIdx > _guardLb { startIdx } else { _guardLb };\n",
    );
    for input in &func.inputs {
        out.push_str(&format!(
            "        if {}.len() < endIdx + 1 {{\n            return Err(RetCode::BadParam);\n        }}\n",
            input.name
        ));
    }
    if !func.outputs.is_empty() {
        out.push_str(
            "        let _guardOutLen = if _guardStart > endIdx { 0 } else { endIdx - _guardStart + 1 };\n",
        );
    }
    for output in &func.outputs {
        // A nullable output may be declined with `None` (rule B6a): nothing is
        // written to it, so there is no capacity to owe. Supplied, it is bounded
        // like any other -- "declined" is `None` and nothing else.
        let cond = if output.is_nullable() {
            format!("{}.as_deref().is_some_and(|o| o.len() < _guardOutLen)", output.name)
        } else {
            format!("{}.len() < _guardOutLen", output.name)
        };
        out.push_str(&format!(
            "        if {cond} {{\n            return Err(RetCode::BadParam);\n        }}\n"
        ));
    }
    out
}

/// Generate the guarded entry point — `{snake}_Impl`, crate-private. Validates
/// params, then renders the algorithm inline (or delegates to `{snake}_Private`
/// when the function declares one).
///
/// This keeps C's shape — a `RetCode` plus `&mut outBegIdx` / `&mut outNBElement`
/// — because that is what the transcribed bodies are written against, and it is
/// where the FMA dispatch sits. It is not a cross-call target: a transcribed
/// body reaches its sibling through the public tier
/// ([`render_cross_indicator_call`]), as C, Java and C# do (#267). `gen_public_entry`
/// wraps this into the `Result<OutRange, RetCode>` the crate actually exposes.
#[allow(clippy::too_many_lines, clippy::cognitive_complexity)]
fn gen_guarded_func(
    func: &FuncDef,
    snake: &str,
    enums: &HashMap<String, EnumDef>,
    registry: &Registry,
    helpers: &HelperRegistry,
) -> String {
    let mut out = String::new();

    // The public wrapper carries the documentation; this one gets a pointer to it.
    out.push_str(&format!(
        "    /// C-shaped body behind [`Core::{snake}`]: a `RetCode` plus two out-params,\n    /// which is what the transcribed body is written against. Since #267 its only\n    /// callers are that wrapper and the phantom-I/O sweep.\n"
    ));
    out.push_str(&format!("    pub(crate) fn {snake}_Impl(\n"));
    out.push_str("        &self,\n");
    out.push_str("        startIdx: usize,\n");
    out.push_str("        endIdx: usize,\n");
    out.push_str(&gen_generic_params(func));
    out.push_str("        outBegIdx: &mut usize,\n");
    out.push_str("        outNBElement: &mut usize,\n");
    out.push_str(&gen_generic_output_params(func, true));
    out.push_str("    ) -> RetCode {\n");

    // Range check. `usize` makes C's two negative-index conditions
    // unrepresentable, so MAX_INDEX is what gives OutOfRangeStartIndex a
    // producer here at all. The end-index arm answers OutOfRangeEndIndex to
    // match C and the crate's own abstract tier (#180; C6 of #179). No gate can
    // see this arm: the JSON-RPC server re-implements C's guard, so the crate's
    // own answer never reaches the driver.
    out.push_str("        if startIdx > Self::MAX_INDEX {\n");
    out.push_str("            return RetCode::OutOfRangeStartIndex;\n");
    out.push_str("        }\n");
    out.push_str("        if endIdx > Self::MAX_INDEX || endIdx < startIdx {\n");
    out.push_str("            return RetCode::OutOfRangeEndIndex;\n");
    out.push_str("        }\n");

    // Param validation
    for opt in &func.optional_inputs {
        out.push_str(&gen_opt_param_validation(opt, "        ", false, enums));
    }

    // The bounds-assert preamble: the LLVM proof that elides per-access bounds
    // checks in a `#![forbid(unsafe_code)]` crate. `guard_empty_range` keeps a
    // call that computes nothing from panicking.
    //
    // BEFORE the aliasing guard below, because the spec orders B5 (a buffer too
    // short) ahead of B6 (two outputs are the same buffer) and a call that is
    // both must report the first (#261). The two answer DIFFERENT things here --
    // a panic against `BadParam` -- so, unlike two rules that share a code, the
    // order is observable and owed.
    out.push_str(&emit_bounds_asserts(func, snake, true));

    // Output-distinctness (issue #108): aliasing two different output buffers has
    // no correct result. The borrow checker already forbids a safe caller from
    // passing the same `&mut` slice twice, so this only guards the unsafe/FFI
    // boundary; it is kept for parity with the C/Java/C# backends. Input ==
    // output aliasing stays allowed.
    if func.outputs.len() >= 2 {
        let mut pairs: Vec<String> = Vec::new();
        for i in 0..func.outputs.len() {
            for j in (i + 1)..func.outputs.len() {
                let (a, b) = (&func.outputs[i], &func.outputs[j]);
                // Cross-typed pairs are skipped: `*const f64` and `*const i32`
                // are not comparable, and safe code cannot lay a `&mut [f64]`
                // over a `&mut [i32]` to begin with. All four backends now skip
                // them — Appendix E of `docs/error-handling-spec.md`, #262.
                if (a.param_type == ParamType::Integer) != (b.param_type == ParamType::Integer) {
                    continue;
                }
                pairs.push(alias_pair_expr(a, b));
            }
        }
        if !pairs.is_empty() {
            out.push_str(&format!("        if {} {{\n", pairs.join(" || ")));
            out.push_str("            return RetCode::BadParam;\n");
            out.push_str("        }\n");
        }
    }

    if func.has_explicit_private {
        // The guarded body contains the pre-computation plus the delegation call
        // to `_private`. Render it directly.
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
        collect_signed_int_vars(&func.body, &g_index_vars, &g_real_vars, &mut g_sentinel_vars);
        for sv in &g_sentinel_vars {
            g_index_vars.remove(sv);
        }
        let g_int_output_set: std::collections::HashSet<String> = func.outputs.iter()
            .filter(|o| o.param_type == ParamType::Integer)
            .map(|o| o.name.clone())
            .collect();
        let enum_vars = enum_local_types(func);
        prune_enum_locals(&mut g_index_vars, &enum_vars);
        let g_ctx = RustRenderCtx {
            for_range_lowering: true,
            // The guarded preamble is emitted once by gen_guarded_func above, not
            // from the statement renderer — keep this false so it cannot double.
            bounds_asserts: false,
            index_vars: g_index_vars,
            real_vars: g_real_vars,
            vec_vars: g_vec_vars,
            real_array_vars: g_real_array_vars,
            int_output_names: g_int_output_set,
            int_vec_vars: g_int_vec_vars,
            is_lookback: false,
            sentinel_vars: g_sentinel_vars,
            result_error_returns: false,
            matype_map: build_matype_map(enums),
            enum_vars,
            circbuf_hybrid_static: collect_circbuf_static(&func.body),
            nullable_outputs: super::common::nullable_output_names(func),
            nullable_shadow: false,
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
                out.push_str(&emit_circbuf_prolog_rust(id, layout, *static_size, batch_circbuf_tier(&func.body, id)));
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
        collect_signed_int_vars(&func.body, &g_index_vars, &g_real_vars, &mut g_sentinel_vars);
        for sv in &g_sentinel_vars {
            g_index_vars.remove(sv);
        }
        let g_int_output_set: std::collections::HashSet<String> = func.outputs.iter()
            .filter(|o| o.param_type == ParamType::Integer)
            .map(|o| o.name.clone())
            .collect();
        let enum_vars = enum_local_types(func);
        prune_enum_locals(&mut g_index_vars, &enum_vars);
        let g_ctx = RustRenderCtx {
            for_range_lowering: true,
            // The guarded preamble is emitted once by gen_guarded_func above, not
            // from the statement renderer — keep this false so it cannot double.
            bounds_asserts: false,
            index_vars: g_index_vars,
            real_vars: g_real_vars,
            vec_vars: g_vec_vars,
            real_array_vars: g_real_array_vars,
            int_output_names: g_int_output_set,
            int_vec_vars: g_int_vec_vars,
            is_lookback: false,
            sentinel_vars: g_sentinel_vars,
            result_error_returns: false,
            matype_map: build_matype_map(enums),
            enum_vars,
            circbuf_hybrid_static: collect_circbuf_static(&func.body),
            nullable_outputs: super::common::nullable_output_names(func),
            nullable_shadow: false,
        };

        // Use the same full rendering as the `_private` body
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

        // Variable declarations (same pattern as the `_private` body)
        for stmt in &func.body {
            if let Statement::CircBuf(CircBuf::Prolog {
                id,
                layout,
                static_size,
            }) = stmt
            {
                out.push_str(&emit_circbuf_prolog_rust(id, layout, *static_size, batch_circbuf_tier(&func.body, id)));
                continue;
            }
            if let Statement::VarDecl { var_type, name, .. } = stmt {
                if g_for_loop_vars.contains(name) {
                    continue;
                }
                let total_assigns = count_assignments(name, &func.body);
                let needs_mut = total_assigns > 0;
                let is_sentinel = g_ctx.sentinel_vars.contains(name);
                // An enum-typed local short-circuits: its C `int` declaration
                // describes the storage, not the value (see `enum_local_types`).
                if let Some(ty) = g_ctx.enum_vars.get(name) {
                    // Deferred initialisation: there is no neutral member to
                    // invent, and every such local is assigned before it is
                    // read -- if a body ever breaks that, rustc says so.
                    let m = if needs_mut { "mut " } else { "" };
                    out.push_str(&format!("        let {m}{name}: {ty};\n"));
                    continue;
                }
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
                let rendered_init = if g_ctx.sentinel_vars.contains(name) {
                    render_signed_dest_value(&new_init, &g_ctx, &g_opt_real_params, registry, helpers)
                } else {
                    render_expr(&new_init, &g_ctx, &g_opt_real_params, registry, helpers)
                };
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
    gen_private_func_inner(func, snake, ctx, enums, registry, helpers)
}

/// Pre-loop bounds-assert preamble: give LLVM proof that `endIdx` is within all
/// input/output array bounds. This enables loop unswitching and vectorization and
/// lets LLVM elide the per-access bounds checks on the safe `[]` indexing in the
/// body — no `unsafe` needed. O(1) per call.
///
/// Output arrays are sized by the caller for the elements actually written:
/// `endIdx - max(startIdx, lookback) + 1`. A composed caller passes exactly-sized
/// buffers with `startIdx` below the lookback (the re-based EMA chaining in
/// TEMA/DEMA/T3 reached through MA/MACDEXT), so the bound uses the adjusted start
/// — and since #267 that caller meets the same expression one tier up, in
/// [`gen_argument_checks`], before it ever reaches here. When the adjusted start
/// exceeds `endIdx` the function writes nothing and any length is fine.
///
/// `guard_empty_range` makes the INPUT assertion take that same escape, and is set
/// on `<N>_Impl` only. A call whose lookback clamp pushes the
/// start past `endIdx` returns `Success` with zero elements and touches neither
/// array, so asserting on it would panic where the contract says success. On every
/// call that *does* compute, the escape is false and the proof handed to LLVM is
/// identical.
///
/// **This is not what a caller of the crate sees.** Since #265 the public entry
/// point states the same bounds ahead of the call, as `RetCode::BadParam`
/// ([`gen_argument_checks`]) — strictly stronger on the input side, since it
/// takes no escape — so a `pub fn` call cannot reach these asserts with a slice
/// they would reject. Since #267 a cross-indicator call enters that same public
/// tier, so what still meets these asserts is `pub fn <N>` and the phantom-I/O
/// sweep — they remain the LLVM proof, and a panic is the right answer to what
/// would be a generator bug.
fn emit_bounds_asserts(func: &FuncDef, snake: &str, guard_empty_range: bool) -> String {
    let mut out = String::new();
    let needs_start = guard_empty_range || !func.outputs.is_empty();
    if needs_start {
        let lb_args: Vec<String> =
            func.optional_inputs.iter().map(|o| o.name.clone()).collect();
        out.push_str(&format!(
            "        let _assertLb = self.{snake}_Lookback({}).unwrap_or(usize::MAX);\n",
            lb_args.join(", ")
        ));
        out.push_str(
            "        let _assertStart = if startIdx > _assertLb { startIdx } else { _assertLb };\n",
        );
    }
    let escape = if guard_empty_range { "_assertStart > endIdx || " } else { "" };
    // EVERY declared input, including the seven candlestick legs whose body never
    // indexes them (#260). For the legs the body DOES read the assert is the LLVM
    // proof; for the seven it proves nothing and states B4/B5 instead — a `len()`
    // compare in the entry block, outside every loop, on a call that is about to
    // walk the series. Filtering them out bought that and cost the contract: a
    // declared input a caller may omit is an exception list, and C never had one,
    // so the same call was `TA_BAD_PARAM` there and a success here.
    for input in &func.inputs {
        out.push_str(&format!(
            "        assert!({escape}endIdx < {}.len());\n", input.name
        ));
    }
    for output in &func.outputs {
        // A declined output (`None`, rule B6a) has no capacity to bound: nothing
        // is written to it, so B5 has nothing to say about it.
        if output.is_nullable() {
            out.push_str(&format!(
                "        assert!(_assertStart > endIdx || {}.as_deref().is_none_or(|o| endIdx - _assertStart < o.len()));\n",
                output.name
            ));
        } else {
            out.push_str(&format!(
                "        assert!(_assertStart > endIdx || endIdx - _assertStart < {}.len());\n",
                output.name
            ));
        }
    }
    out
}

/// Render the `_private` body: the algorithm with the extra pre-computed params
/// and no validation prologue.
#[allow(clippy::too_many_lines)]
fn gen_private_func_inner(
    func: &FuncDef,
    snake: &str,
    ctx: &RustRenderCtx,
    enums: &HashMap<String, EnumDef>,
    registry: &Registry,
    helpers: &HelperRegistry,
) -> String {
    let mut out = String::new();
    let func_name = format!("{snake}_Private");

    out.push_str(&super::rust_doc::private_docs(func, snake));

    // `pub(crate)`, not `pub`: C makes `TA_XXX_Private` file-`static` and Java/C#
    // make theirs package-private/internal, so a `pub` here was the one backend
    // where a caller could reach an entry point with no validation prologue --
    // and therefore no TA_MAX_INDEX bound (#180). Cross-indicator calls are all
    // in-crate, so nothing legitimate loses access.
    // #[inline] enables cross-module inlining for cross-indicator calls
    out.push_str("    #[inline]\n");
    out.push_str(&format!("    pub(crate) fn {func_name}(\n"));
    out.push_str("        &self,\n");
    out.push_str("        mut startIdx: usize,\n");
    out.push_str("        endIdx: usize,\n");
    out.push_str(&gen_generic_params(func));
    // Extra params carried only by `_private` (e.g. EMA's k factor)
    for (param_name, c_type) in &func.private_extra_params {
        let rust_type = match c_type.as_str() {
            "double" => "f64",
            "int" => "i32",
            other => panic!("Unknown C type '{other}' for extra param '{param_name}'"),
        };
        out.push_str(&format!("        {param_name}: {rust_type},\n"));
    }
    out.push_str("        outBegIdx: &mut usize,\n");
    out.push_str("        outNBElement: &mut usize,\n");
    out.push_str(&gen_generic_output_params(func, true));
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
            out.push_str(&emit_circbuf_prolog_rust(id, layout, *static_size, batch_circbuf_tier(&func.body, id)));
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

    if ctx.bounds_asserts {
        out.push_str(&emit_bounds_asserts(func, snake, false));
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
            let rendered_init = if ctx.sentinel_vars.contains(name) {
                render_signed_dest_value(&new_init, ctx, &opt_real_params, registry, helpers)
            } else {
                render_expr(&new_init, ctx, &opt_real_params, registry, helpers)
            };
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
        let rust_type = opt_param_type(&opt.param_type);
        out.push_str(&format!("        mut {}: {},\n", opt.name, rust_type));
    }
    out
}

/// The Rust type of one output parameter.
///
/// A `nullable` output (rule B6a) is `Option<&mut [T]>`. Rust can spell
/// "declined" distinctly from "empty" and so it does, which leaves C# the only
/// backend where the two collapse — Appendix F of `docs/error-handling-spec.md`.
/// `None` means *compute it but do not write it out*: every store to that output
/// is guarded and its capacity assert is skipped.
fn output_param_type(output: &Output) -> String {
    let elem = match output.param_type {
        ParamType::Real => "f64",
        ParamType::Integer | ParamType::Enum(_) | ParamType::Price(_) => "i32",
    };
    if output.is_nullable() {
        format!("Option<&mut [{elem}]>")
    } else {
        format!("&mut [{elem}]")
    }
}

/// If `target` stores into one of the `nullable` outputs (an `Option<&mut [T]>`
/// the caller may pass `None` to decline — rule B6a), return its base name so
/// the store can be wrapped in `if let Some(..) = ..as_deref_mut()`. Matches the
/// array store `outX[i] = …` and the scalar store `outX = …`; the value side is
/// never involved.
fn nullable_target_base<'a>(
    target: &Expr,
    nullable: &'a std::collections::HashSet<String>,
) -> Option<&'a String> {
    let (Expr::ArrayAccess(name, _) | Expr::PointerDeref(name) | Expr::Var(name)) = target else {
        return None;
    };
    nullable.get(name)
}

/// One term of the output-distinctness guard (#108): do these two outputs name
/// the same buffer?
///
/// **Both operands must be non-empty.** Two zero-length slices cannot clobber
/// each other, and every unallocated `Vec` hands out the same dangling aligned
/// pointer — so a bare `as_ptr()` comparison rejected three separately allocated
/// empty `Vec`s while accepting three zero-length subslices of one buffer, which
/// is worse than either answer. A range shorter than the lookback produces
/// nothing and needs no output space (rule N1), so those calls are legal and C
/// and Java always accepted them (Appendix D item 11).
///
/// A nullable output contributes a term only when the caller supplied it: `None`
/// is a declaration that nothing is written there, not a buffer that could alias.
fn alias_pair_expr(a: &Output, b: &Output) -> String {
    match (a.is_nullable(), b.is_nullable()) {
        (false, false) => format!(
            "(!{0}.is_empty() && !{1}.is_empty() && {0}.as_ptr() == {1}.as_ptr())",
            a.name, b.name
        ),
        (true, false) => format!(
            "{0}.as_deref().is_some_and(|a| !a.is_empty() && !{1}.is_empty() && a.as_ptr() == {1}.as_ptr())",
            a.name, b.name
        ),
        (false, true) => format!(
            "{1}.as_deref().is_some_and(|b| !{0}.is_empty() && !b.is_empty() && {0}.as_ptr() == b.as_ptr())",
            a.name, b.name
        ),
        (true, true) => format!(
            "{0}.as_deref().zip({1}.as_deref()).is_some_and(|(a, b)| !a.is_empty() && !b.is_empty() && a.as_ptr() == b.as_ptr())",
            a.name, b.name
        ),
    }
}

/// Generate generic output parameter declarations for a function signature.
///
/// `mut_binding` is for the entry points that render a body: a nullable output
/// is re-borrowed with `as_deref_mut()` at every store, which needs a mutable
/// binding. The forwarding wrappers, which only pass it on, leave it off.
fn gen_generic_output_params(func: &FuncDef, mut_binding: bool) -> String {
    let mut out = String::new();
    for output in &func.outputs {
        let binding = if mut_binding && output.is_nullable() { "mut " } else { "" };
        out.push_str(&format!(
            "        {binding}{}: {},\n",
            output.name,
            output_param_type(output)
        ));
    }
    out
}

/// Generate optional parameter validation code.
fn gen_opt_param_validation(opt: &OptInput, pad: &str, is_lookback: bool, enums: &HashMap<String, EnumDef>) -> String {
    let err_return = if is_lookback {
        "return Err(RetCode::BadParam);"
    } else {
        "return RetCode::BadParam;"
    };
    gen_opt_param_validation_with(opt, pad, err_return, enums)
}

/// Core of the optional-parameter default-substitution + range check, with the
/// failure statement supplied by the caller (batch returns a bare `RetCode`,
/// the stream tier returns `Err(RetCode::BadParam)`).
///
/// An `enum:` param gets its own arm: the parameter is typed as its enum, so the
/// `i32::MIN` sentinel is unrepresentable there and only the `DEFAULT` member
/// (#182) is left to substitute — the shape Java has always had (#162).
#[allow(clippy::float_cmp)] // an enum default is an exact integer, not a measurement
pub(crate) fn gen_opt_param_validation_with(
    opt: &OptInput,
    pad: &str,
    err_return: &str,
    enums: &HashMap<String, EnumDef>,
) -> String {
    let mut out = String::new();
    let name = &opt.name;

    match &opt.param_type {
        // A typed enum parameter cannot hold the `i32::MIN` sentinel at all --
        // the same reason Java has never checked for it (issue #162) -- so the
        // `DEFAULT` member is the only spelling left to substitute.
        ParamType::Enum(enum_name) => {
            if let (Some(default), Some(def_variant)) = (
                opt.default,
                super::common::enum_default_variant(enums, enum_name),
            ) {
                #[allow(clippy::cast_possible_truncation)]
                let resolved = enums.get(enum_name).and_then(|e| {
                    e.variants
                        .iter()
                        .find(|v| f64::from(v.value) == default)
                        .map(|v| v.name.clone())
                });
                if let Some(resolved) = resolved {
                    out.push_str(&format!(
                        "{pad}if {name} == {enum_name}::{} {{\n",
                        def_variant.name
                    ));
                    out.push_str(&format!("{pad}    {name} = {enum_name}::{resolved};\n"));
                    out.push_str(&format!("{pad}}}\n"));
                }
            }
        }
        ParamType::Integer => {
            if let Some(default) = opt.default {
                out.push_str(&format!("{pad}if (({name}) as i32) == (i32::MIN) {{\n"));
                #[allow(clippy::cast_possible_truncation)]
                let default_i64 = default as i64;
                out.push_str(&format!("{pad}    {name} = {default_i64};\n"));

                if let Some((lo, hi)) = opt.range {
                    out.push_str(&format!(
                        "{pad}}} else if ((({name}) as i32) < {lo}) || ((({name}) as i32) > {hi}) {{\n"
                    ));
                    out.push_str(&format!("{pad}    {err_return}\n"));
                }

                out.push_str(&format!("{pad}}}\n"));
            }
        }
        ParamType::Real => {
            if let Some(default) = opt.default {
                // Bounds are emitted in exponent form so they are `f64` literals, not
                // integers, on the comparison's right-hand side.
                out.push_str(&format!("{pad}if {name} == Self::REAL_DEFAULT {{\n"));
                out.push_str(&format!("{pad}    {name} = {default:e};\n"));

                // Every declared bound is checked (see backends::c).
                if let Some((lo, hi)) = opt.range {
                    out.push_str(&format!(
                        "{pad}}} else if {cond} {{\n",
                        cond = super::common::real_range_reject(
                            name,
                            &super::common::real_bound_literal(lo, "Self::"),
                            &super::common::real_bound_literal(hi, "Self::"),
                            true
                        )
                    ));
                    out.push_str(&format!("{pad}    {err_return}\n"));
                }

                out.push_str(&format!("{pad}}}\n"));
            }
        }
        // Price params expand to arrays validated separately; no scalar
        // validation applies.
        ParamType::Price(_) => {}
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

/// Which tier a CIRCBUF prolog is being emitted for, and so which storage shape
/// it gets. The two are not interchangeable: [`Self::StreamVec`] is forced by
/// ownership, not chosen — the open path moves the storage into the stream state
/// struct, which outlives the frame, so a `&mut` slice into a stack array would
/// dangle. Only the batch tier can take the hybrid, and only it needs to.
#[derive(Clone, Copy)]
pub(crate) enum CircBufTier {
    /// Stream: an owning `Vec` per field-split storage.
    StreamVec,
    /// Batch: C's hybrid — a zeroed stack array at the static size, a heap `Vec`
    /// behind it, and a `&mut` slice the body indexes through. The heap `Vec` is
    /// declared only when a runtime `CIRCBUF_INIT` exists to reach it
    /// (`INIT_LOCAL_ONLY` never leaves the stack array).
    BatchHybrid { has_runtime_init: bool },
}

/// Emit the function-top declarations for a CIRCBUF prolog, plus the `usize`
/// rotation index and bound. The bound is seeded to `static_size - 1` (NOT 0)
/// so the `INIT_LOCAL_ONLY` path (HT functions) sizes its buffer correctly
/// before any `INIT` runs. Indent is the 8-space body level.
pub(crate) fn emit_circbuf_prolog_rust(
    id: &str,
    layout: &CircBufLayout,
    static_size: i64,
    tier: CircBufTier,
) -> String {
    let mut s = String::new();
    for (storage, t) in circbuf_storage(id, layout) {
        let (vt, zero) = if matches!(t, VarType::Integer) {
            ("i32", "0i32")
        } else {
            ("f64", "0.0_f64")
        };
        match tier {
            CircBufTier::StreamVec => {
                s.push_str(&format!(
                    "        let mut {storage}: Vec<{vt}> = Vec::new();\n"
                ));
            }
            CircBufTier::BatchHybrid { has_runtime_init } => {
                s.push_str(&format!(
                    "        let mut local_{storage}: [{vt}; {static_size}] = [{zero}; {static_size}];\n"
                ));
                if has_runtime_init {
                    s.push_str(&format!(
                        "        let mut heap_{storage}: Vec<{vt}> = Vec::new();\n"
                    ));
                }
                s.push_str(&format!(
                    "        let mut {storage}: &mut [{vt}] = &mut [];\n"
                ));
            }
        }
    }
    s.push_str(&format!("        let mut {id}_Idx: usize = 0;\n"));
    s.push_str(&format!(
        "        let mut maxIdx_{id}: usize = {};\n",
        static_size - 1
    ));
    s
}

/// CIRCBUF prologs in `body` as `id → static size` — the batch tier's
/// hybrid-storage map ([`RustRenderCtx::circbuf_hybrid_static`]). Prologs are
/// declarations, so only the top level of `body` is scanned.
pub(crate) fn collect_circbuf_static(body: &[Statement]) -> std::collections::HashMap<String, i64> {
    body.iter()
        .filter_map(|s| match s {
            Statement::CircBuf(CircBuf::Prolog { id, static_size, .. }) => {
                Some((id.clone(), *static_size))
            }
            _ => None,
        })
        .collect()
}

/// The batch tier's storage shape for `id` in `body`. The heap `Vec` is declared
/// only when a runtime `CIRCBUF_INIT` can reach it.
pub(crate) fn batch_circbuf_tier(body: &[Statement], id: &str) -> CircBufTier {
    CircBufTier::BatchHybrid { has_runtime_init: circbuf_has_runtime_init(body, id) }
}

/// Whether a runtime-sized `CIRCBUF_INIT` for `id` appears anywhere in `body`
/// (as opposed to `INIT_LOCAL_ONLY`, which never needs the heap fallback).
pub(crate) fn circbuf_has_runtime_init(body: &[Statement], id: &str) -> bool {
    body.iter().any(|stmt| match stmt {
        Statement::CircBuf(CircBuf::Init { id: init_id, .. }) => init_id == id,
        Statement::If { then_body, else_body, .. } => {
            circbuf_has_runtime_init(then_body, id) || circbuf_has_runtime_init(else_body, id)
        }
        Statement::While { body: b, .. }
        | Statement::DoWhile { body: b, .. }
        | Statement::For { body: b, .. }
        | Statement::ForC { body: b, .. }
        | Statement::Block { body: b } => circbuf_has_runtime_init(b, id),
        Statement::Switch { cases, default, .. } => {
            cases.iter().any(|(_, cb)| circbuf_has_runtime_init(cb, id))
                || circbuf_has_runtime_init(default, id)
        }
        _ => false,
    })
}

#[allow(clippy::too_many_lines)]
pub(crate) fn collect_var_types(
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
            // top; classify the names so the usize inference applies.
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

/// Issue #160: fail generation LOUDLY when a negative-capable `(int)(float)`
/// cast appears anywhere the Rust backend cannot yet render sign-faithfully.
/// Supported positions: the whole right-hand side of a plain assignment to a
/// signed local or an i32 array slot, or a VarDecl initializer of a signed
/// local. Everything else (nested in arithmetic, compound assigns, conditions,
/// call arguments) would silently saturate negatives to 0 — reject instead.
pub(crate) fn reject_unsupported_negative_casts(
    body: &[Statement],
    real_vars: &std::collections::HashSet<String>,
    func_name: &str,
) {
    fn check_expr(e: &Expr, rv: &std::collections::HashSet<String>, f: &str) {
        crate::streaming::walk_expr(e, &mut |x| {
            if let Expr::Cast(VarType::Integer | VarType::Index, inner) = x {
                assert!(
                    !cast_inner_negative_capable(inner, rv),
                    "{f}: a (int) cast of a possibly-negative double is only \
                     supported as the whole right-hand side of a plain \
                     assignment (issue #160); found it nested at {x:?}. \
                     Stage it through its own int local first."
                );
            }
        });
    }
    // Allowed root: the cast itself — but its INNER must still be clean.
    fn check_value(v: &Expr, rv: &std::collections::HashSet<String>, f: &str) {
        if let Expr::Cast(VarType::Integer | VarType::Index, inner) = v {
            check_expr(inner, rv, f);
        } else {
            check_expr(v, rv, f);
        }
    }
    for stmt in body {
        match stmt {
            Statement::VarDecl { init: Some(init), .. } => check_value(init, real_vars, func_name),
            Statement::Assign { value, compound, .. } => {
                if *compound {
                    // Desugared form embeds the target: check only the true RHS.
                    if let Expr::BinOp(_, _, rhs) = value {
                        check_expr(rhs, real_vars, func_name);
                    } else {
                        check_expr(value, real_vars, func_name);
                    }
                } else {
                    check_value(value, real_vars, func_name);
                }
            }
            Statement::If { condition, then_body, else_body, .. } => {
                check_expr(condition, real_vars, func_name);
                reject_unsupported_negative_casts(then_body, real_vars, func_name);
                reject_unsupported_negative_casts(else_body, real_vars, func_name);
            }
            Statement::While { condition, body: b } | Statement::DoWhile { condition, body: b } => {
                check_expr(condition, real_vars, func_name);
                reject_unsupported_negative_casts(b, real_vars, func_name);
            }
            Statement::For { count, body: b, .. } => {
                check_expr(count, real_vars, func_name);
                reject_unsupported_negative_casts(b, real_vars, func_name);
            }
            Statement::ForC { init, condition, update, body: b } => {
                reject_unsupported_negative_casts(std::slice::from_ref(init), real_vars, func_name);
                check_expr(condition, real_vars, func_name);
                reject_unsupported_negative_casts(std::slice::from_ref(update), real_vars, func_name);
                reject_unsupported_negative_casts(b, real_vars, func_name);
            }
            Statement::Block { body: b } => reject_unsupported_negative_casts(b, real_vars, func_name),
            Statement::Switch { expr, cases, default, .. } => {
                check_expr(expr, real_vars, func_name);
                for (_, cb) in cases {
                    reject_unsupported_negative_casts(cb, real_vars, func_name);
                }
                reject_unsupported_negative_casts(default, real_vars, func_name);
            }
            #[allow(clippy::match_same_arms)] // Return/Expr coincide today; distinct concepts
            Statement::Return { value: Some(e) } => check_expr(e, real_vars, func_name),
            Statement::Expr(e) => check_expr(e, real_vars, func_name),
            _ => {}
        }
    }
}

/// Issue #160: render a value destined for a SIGNED (i32) local or an i32
/// array slot. A whole-RHS `(int)(float)` cast renders `as i32` — the default
/// f64→usize cast saturates negatives to 0 before any later conversion could
/// recover them. Non-cast values render normally.
fn render_signed_dest_value(
    value: &Expr,
    ctx: &RustRenderCtx,
    opt_real_params: &[String],
    registry: &Registry,
    helpers: &HelperRegistry,
) -> String {
    if let Expr::Cast(VarType::Integer | VarType::Index, inner) = value {
        if expr_is_float_typed_ctx(inner, Some(ctx)) {
            let inner_str = render_expr(inner, ctx, opt_real_params, registry, helpers);
            return format!("({inner_str}) as i32");
        }
    }
    render_expr(value, ctx, opt_real_params, registry, helpers)
}

/// True when `inner` is a float-typed cast operand that could be negative —
/// the issue #160 class. `real_vars` recognizes plain double LOCALS (the
/// name-heuristic float classifier alone misses e.g. `double basis; (int)basis`).
/// sqrt/fabs/abs inners are provably non-negative (HMA's sqrtPeriod stays usize).
fn cast_inner_negative_capable(inner: &Expr, real_vars: &std::collections::HashSet<String>) -> bool {
    let is_float = fma::expr_is_float_typed(inner, None)
        || matches!(inner, Expr::Var(v) if real_vars.contains(v));
    is_float
        && !matches!(inner,
                     Expr::FuncCall(name, _) if name == "sqrt" || name == "fabs" || name == "abs")
}

/// Check if an expression can produce a negative integer value.
/// Catches: `0 - N`, `-N` literal, negative-capable `(int)` casts (#160),
/// arithmetic/ternary combinations of the above.
fn expr_can_be_negative(expr: &Expr, real_vars: &std::collections::HashSet<String>) -> bool {
    match expr {
        // 0 - N is negative; any subtraction of/by a negative-capable operand
        // can be negative too.
        Expr::BinOp(left, BinOp::Sub, right) => {
            matches!(left.as_ref(), Expr::IntLiteral(0))
                || expr_can_be_negative(left, real_vars)
                || expr_can_be_negative(right, real_vars)
        }
        // ~x is negative for every x >= 0 (two's complement)
        Expr::BitwiseNot(_) => true,
        // (int)(<float expr>) truncates: negative doubles yield negative ints (#160)
        Expr::Cast(VarType::Integer | VarType::Index, inner) => {
            cast_inner_negative_capable(inner, real_vars)
        }
        // Arithmetic where either side can be negative
        Expr::BinOp(left, BinOp::Mul | BinOp::Add | BinOp::Div | BinOp::Mod, right) => {
            expr_can_be_negative(left, real_vars) || expr_can_be_negative(right, real_vars)
        }
        // Ternary: if either branch can be negative
        Expr::Ternary(_, then_e, else_e) => {
            expr_can_be_negative(then_e, real_vars) || expr_can_be_negative(else_e, real_vars)
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
pub(crate) fn collect_signed_int_vars(
    body: &[Statement],
    int_vars: &std::collections::HashSet<String>,
    real_vars: &std::collections::HashSet<String>,
    signed_vars: &mut std::collections::HashSet<String>,
) {
    for stmt in body {
        match stmt {
            // Variable initialized with a negative expression
            Statement::VarDecl {
                var_type: VarType::Integer | VarType::Index,
                name,
                init: Some(init),
            } if expr_can_be_negative(init, real_vars) => {
                signed_vars.insert(name.clone());
            }
            // Integer variable assigned a negative expression
            Statement::Assign { target: Expr::Var(name), value, .. }
                if int_vars.contains(name) && expr_can_be_negative(value, real_vars) =>
            {
                signed_vars.insert(name.clone());
            }
            // Condition checking `var < 0` (only on integer vars)
            Statement::If { condition, then_body, else_body, .. } => {
                condition_implies_signed(condition, int_vars, signed_vars);
                collect_signed_int_vars(then_body, int_vars, real_vars, signed_vars);
                collect_signed_int_vars(else_body, int_vars, real_vars, signed_vars);
            }
            Statement::While { condition, body: inner, .. }
            | Statement::DoWhile { condition, body: inner, .. } => {
                condition_implies_signed(condition, int_vars, signed_vars);
                collect_signed_int_vars(inner, int_vars, real_vars, signed_vars);
            }
            Statement::For { body: inner, .. } => {
                collect_signed_int_vars(inner, int_vars, real_vars, signed_vars);
            }
            Statement::ForC { init, condition, body: inner, .. } => {
                condition_implies_signed(condition, int_vars, signed_vars);
                collect_signed_int_vars(&[init.as_ref().clone()], int_vars, real_vars, signed_vars);
                collect_signed_int_vars(inner, int_vars, real_vars, signed_vars);
            }
            Statement::Block { body: block_body } => {
                collect_signed_int_vars(block_body, int_vars, real_vars, signed_vars);
            }
            Statement::Switch { cases, default, .. } => {
                for (_, case_body) in cases {
                    collect_signed_int_vars(case_body, int_vars, real_vars, signed_vars);
                }
                collect_signed_int_vars(default, int_vars, real_vars, signed_vars);
            }
            _ => {}
        }
    }
}

/// Pre-scan function body for variables assigned negative values (sentinel pattern).
pub(crate) fn collect_sentinel_vars(
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
pub(crate) fn count_assignments(name: &str, body: &[Statement]) -> usize {
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
        Expr::Not(inner) | Expr::BitwiseNot(inner) | Expr::Cast(_, inner) => {
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
            | Statement::UnrollHint { .. }
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

pub(crate) fn collect_for_loop_vars(body: &[Statement]) -> Vec<String> {
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
            | Statement::UnrollHint { .. }
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
pub(crate) fn render_hoisted_blocks(
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
            // Runtime-sized. Batch tier (id present in circbuf_hybrid_static):
            // C-style hybrid — bind the slices to the prolog's stack arrays when
            // the runtime size fits the static capacity, heap-allocate otherwise.
            // Stream tier: (re)allocate each storage Vec to `size` (always heap;
            // the open path moves the Vecs into the stream state struct).
            CircBuf::Init { id, layout, size } => {
                let sz = render_expr(
                    size,
                    self.ctx,
                    self.opt_real_params,
                    self.registry,
                    self.helpers,
                );
                let mut s = String::new();
                // The size is derived, so < 1 is a logic defect rather than an
                // allocation failure: same code as C's TA_INTERNAL_ERROR(137)
                // (#178). Also prevents the `(sz as usize) - 1` underflow.
                let size_defect = if self.ctx.result_error_returns {
                    "return Err(RetCode::InternalError);"
                } else {
                    "return RetCode::InternalError;"
                };
                s.push_str(&format!(
                    "{pad}if {sz} < 1 {{ {size_defect} }}\n"
                ));
                if let Some(static_size) = self.ctx.circbuf_hybrid_static.get(id) {
                    s.push_str(&format!(
                        "{pad}if ({sz}) as usize <= {static_size}usize {{\n"
                    ));
                    for (storage, _) in circbuf_storage(id, layout) {
                        s.push_str(&format!(
                            "{pad}    {storage} = &mut local_{storage};\n"
                        ));
                    }
                    s.push_str(&format!("{pad}}} else {{\n"));
                    for (storage, t) in circbuf_storage(id, layout) {
                        let zero = if matches!(t, VarType::Integer) {
                            "0i32"
                        } else {
                            "0.0_f64"
                        };
                        s.push_str(&format!(
                            "{pad}    heap_{storage} = vec![{zero}; ({sz}) as usize];\n"
                        ));
                        s.push_str(&format!(
                            "{pad}    {storage} = &mut heap_{storage};\n"
                        ));
                    }
                    s.push_str(&format!("{pad}}}\n"));
                } else {
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
                }
                s.push_str(&format!("{pad}maxIdx_{id} = (({sz}) as usize) - 1;\n"));
                s.push_str(&format!("{pad}{id}_Idx = 0;\n"));
                s
            }
            // Always the static capacity; bound was seeded in the prolog (maxIdx + 1).
            // Batch tier: bind to the prolog's zeroed stack arrays — no allocation.
            CircBuf::InitLocalOnly { id, layout } => {
                let mut s = String::new();
                if self.ctx.circbuf_hybrid_static.contains_key(id) {
                    for (storage, _) in circbuf_storage(id, layout) {
                        s.push_str(&format!(
                            "{pad}{storage} = &mut local_{storage};\n"
                        ));
                    }
                } else {
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
        // `retCode = ma(..)` -- a cross-indicator call, which goes to the
        // callee's PUBLIC entry point and answers a range or an `Err` (#267).
        // The assigned code is `Success` by construction, and
        // `ir_cleanup::drop_answered_cross_call_guards` folds the guard that
        // follows out of the body. The assignment itself stays: 10 of those
        // guards carry a live `|| count == 0` half that still reads it.
        if !compound {
            if let Expr::FuncCall(fname, cargs) = value {
                if self.registry.contains(fname) {
                    if let Some(block) = render_cross_indicator_call(
                        fname, cargs, indent, self.ctx.result_error_returns, self.ctx,
                        self.opt_real_params, self.registry, self.helpers, self.inline_counter,
                    ) {
                        let t = render_assign_target(
                            target, self.ctx, self.opt_real_params, self.registry, self.helpers,
                        );
                        return format!("{block}{pad}{t} = RetCode::Success;\n");
                    }
                }
            }
        }
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
        // Canonicalize accumulator recurrences so all backends fuse the same
        // product regardless of operand order (cross-language / batch-vs-stream).
        let new_value = if fma::EMIT_FMA && !self.ctx.is_lookback {
            fma::canonicalize_accumulator_add(target, &new_value)
        } else {
            new_value
        };
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
                            BinOp::BitwiseAnd => "&=",
                            BinOp::BitwiseOr => "|=",
                            BinOp::BitwiseXor => "^=",
                            BinOp::Shl => "<<=",
                            BinOp::Shr => ">>=",
                            BinOp::Mod
                            | BinOp::LessEq
                            | BinOp::Less
                            | BinOp::Greater
                            | BinOp::GreaterEq
                            | BinOp::Eq
                            | BinOp::NotEq
                            | BinOp::And
                            | BinOp::Or => "",
                        };
                        if !op_str.is_empty() {
                            let target_str =
                                render_assign_target(target, self.ctx, self.opt_real_params, self.registry, self.helpers);
                            let rhs_str = render_expr(right, self.ctx, self.opt_real_params, self.registry, self.helpers);
                            // Issue #158: the target's Rust type is classified
                            // POSITIVELY (declared IR type first, naming
                            // heuristics only for names no declaration covers).
                            // Never a negative test — "not recognised as
                            // index-ish, therefore f64" puts an `as f64` RHS on
                            // any integer local whose name is off the hard-coded
                            // list.
                            let target_ty = scalar_target_ty(tname, self.ctx, self.opt_real_params, self.helpers);
                            let rhs_wrapped = match target_ty {
                                ScalarTy::F64 => {
                                    // `_ctx` and `renders_usize` rather than the
                                    // bare predicates: a sentinel local and a
                                    // usize⊕i32 BinOp both reach an f64 target
                                    // as integers, and neither may arrive uncast.
                                    if expr_is_untyped_integer(right)
                                        || expr_is_i32_typed_ctx(right, self.ctx)
                                        || (compound_rhs_renders_usize(right, self.ctx)
                                            && !expr_is_float_typed_ctx(right, Some(self.ctx)))
                                    {
                                        format!("(({rhs_str}) as f64)")
                                    } else {
                                        rhs_str
                                    }
                                }
                                // Enum target: the RHS is the same enum, so it
                                // renders bare -- a cast would not even compile.
                                // Coincides with the fallthrough today; distinct
                                // concepts, so it stays explicit.
                                #[allow(clippy::match_same_arms)]
                                ScalarTy::Enum => rhs_str,
                                ScalarTy::Usize if expr_is_i32_typed_ctx(right, self.ctx) => {
                                    // usize target, i32-typed RHS (incl. sentinel
                                    // locals, runtime-non-negative here): as usize
                                    format!("({rhs_str}) as usize")
                                }
                                ScalarTy::I32 => {
                                    // The third branch issue #158 was missing.
                                    // Bare only where bare compiles: an untyped
                                    // literal, or an RHS that is i32-typed AND
                                    // actually renders that way — `(int)d` is
                                    // i32-typed but renders `d as usize`.
                                    // A float RHS is deliberately left bare: the
                                    // crate build's E0277 is a better answer than
                                    // a silent `as i32` narrowing of a double.
                                    let bare = expr_is_untyped_integer(right)
                                        || expr_is_float_typed_ctx(right, Some(self.ctx))
                                        // The index domain never narrows to i32
                                        // (the runtime gates cap at 100k bars,
                                        // the API does not), so `k += endIdx -
                                        // startIdx` stays bare and fails to
                                        // compile rather than truncating above
                                        // 2^31. Same policy as the float arm.
                                        || expr_mentions_index_domain(right)
                                        || (expr_is_i32_typed_ctx(right, self.ctx)
                                            && !compound_rhs_renders_usize(right, self.ctx));
                                    if bare {
                                        rhs_str
                                    } else {
                                        format!("({rhs_str}) as i32")
                                    }
                                }
                                ScalarTy::Usize => rhs_str,
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
        // Issue #160: signed destinations render a whole-RHS (int)(float) cast
        // as `as i32` (see render_signed_dest_value). Applies to sentinel
        // locals and to i32 array slots (outInteger[i] = (int)(x)).
        let signed_dest = match target {
            Expr::Var(tname) => self.ctx.sentinel_vars.contains(tname),
            Expr::ArrayAccess(aname, _) => {
                self.ctx.int_output_names.contains(aname) || is_int_array_or_vec(aname, self.ctx)
            }
            _ => false,
        };
        let value_str = if signed_dest {
            render_signed_dest_value(&new_value, self.ctx, self.opt_real_params, self.registry, self.helpers)
        } else {
            render_expr(&new_value, self.ctx, self.opt_real_params, self.registry, self.helpers)
        };
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
                    || expr_binop_renders_as_usize(&new_value, self.ctx)
                    || expr_renders_as_usize_despite_i32(&new_value, self.ctx)
                    // A `*_lookback()` call returns usize. This gate was an
                    // allowlist of variable shapes, so `lb = sma_lookback(p);`
                    // into a signed local emitted bare and failed E0308.
                    || expr_returns_usize(&new_value)
                    || matches!(new_value, Expr::Var(ref v) if self.ctx.index_vars.contains(v) || is_likely_index_var(v)))
        } else {
            false
        };
        // An integer-output target (int array in batch; the stream tier's
        // `(*outInteger)` step write / `lastValue_outInteger` scalar sink).
        // Computed early: it excludes such targets from the usize casts below
        // (`lastValue_outMaxIdx` would otherwise match the `*Idx` heuristics).
        let int_target = match target {
            Expr::ArrayAccess(name, _) => {
                self.ctx.int_output_names.contains(name) || is_int_array_or_vec(name, self.ctx)
            }
            Expr::PointerDeref(name) => {
                self.ctx.int_output_names.contains(strip_state_prefix(name))
            }
            Expr::Var(name) => name
                .strip_prefix("lastValue_")
                .is_some_and(|base| self.ctx.int_output_names.contains(base)),
            _ => false,
        };
        // Sentinel var used as value: cast i32 sentinel to usize when assigning to usize target
        let needs_sentinel_usize_cast = if let Expr::Var(tname) = target {
            !int_target
                && !self.ctx.sentinel_vars.contains(tname)
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
            !self.ctx.enum_vars.contains_key(tname)
                && !int_target
                && !self.output_names.iter().any(|n| n == tname)
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
            !self.ctx.enum_vars.contains_key(tname)
                && is_i32_opt_in_param(tname)
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
        let needs_int_output_cast = int_target
            && !expr_is_i32_typed(&new_value)
            && !matches!(new_value, Expr::IntLiteral(_))
            && (expr_is_known_usize_ctx(&new_value, self.ctx)
                || matches!(&new_value, Expr::Var(v) if self.ctx.index_vars.contains(v) || is_likely_index_var(v))
                || matches!(&new_value, Expr::BinOp(_, _, _)));
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
                    self.output_names.contains(vname)
                        || vname.starts_with("in")
                        // Stream tier: the composed open renames output arrays
                        // to `sc_*` scratch Vecs; the batch's pointer-alias
                        // assignment becomes the same `.to_vec()` copy the
                        // batch text uses for the un-renamed output param.
                        // Batch-invariant: no batch name starts with `sc_`.
                        || (vname.starts_with("sc_") && self.ctx.vec_vars.contains(vname))
                } else {
                    false
                }
            } else {
                false
            }
        } else {
            false
        };
        // A store into a nullable output is wrapped: the parameter is
        // `Option<&mut [T]>`, and `None` means the caller declined it (rule
        // B6a). The `if let` shadows the parameter name with the slice, so
        // `target_str` and the whole cast chain below render unchanged. The
        // `outIdx` advance rides the non-nullable partner's write (see mama.c),
        // so guarding this store is complete.
        let (pad, close) = match nullable_target_base(target, &self.ctx.nullable_outputs) {
            Some(base) => {
                if self.ctx.nullable_shadow {
                    out.push_str(&format!("{pad}lastCur_{base} = {value_str};\n"));
                }
                out.push_str(&format!(
                    "{pad}if let Some({base}) = {base}.as_deref_mut() {{\n"
                ));
                (format!("{pad}    "), format!("{pad}}}\n"))
            }
            None => (pad, String::new()),
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
        out.push_str(&close);
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
                if self.ctx.for_range_lowering && self.for_loop_vars.contains(iter_name) {
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
            render_condition(condition, self.ctx, self.opt_real_params, self.registry, self.helpers)
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
            render_condition(condition, self.ctx, self.opt_real_params, self.registry, self.helpers)
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
        // `return macd(..)` -- the tail-call form of a cross-indicator call
        // (MACDEXT is the only one). Batch tier only: a stream Open maps its
        // returns through `rust_stream::map_return_code` before this sees them,
        // and a tail-call there would have no `Ok` value to answer with. None
        // exists; if one ever does, it falls through to the `<N>_Impl` rendering
        // and `rust_cross_calls_target_the_public_tier` fails on it.
        if !self.ctx.result_error_returns {
            if let Some(Expr::FuncCall(fname, cargs)) = value {
                if self.registry.contains(fname) {
                    if let Some(block) = render_cross_indicator_call(
                        fname, cargs, indent, false, self.ctx, self.opt_real_params,
                        self.registry, self.helpers, self.inline_counter,
                    ) {
                        return format!("{block}{pad}return RetCode::Success;\n");
                    }
                }
            }
        }
        match value {
            Some(expr) if self.ctx.is_lookback && is_negative_one(expr) => {
                // The lookback bad-param contract: -1 in C, Java and C#; Rust's
                // lookback returns `Result<usize, RetCode>`, so the same C-side
                // `return -1;` becomes an `Err` here rather than a sentinel value.
                format!("{pad}return Err(RetCode::BadParam);\n")
            }
            Some(expr) => {
                let rendered = render_return_expr(expr, self.ctx, self.opt_real_params, self.registry, self.helpers);
                // In lookback functions, return value must be usize. Cast any i32/mixed expression.
                let is_already_usize = matches!(expr, Expr::Var(ref n) if n == "retValue" || n == "lookbackTotal" || n == "emaLookback")
                    || expr_is_known_usize_ctx(expr, self.ctx)
                    || expr_returns_usize(expr);
                let needs_cast = self.ctx.is_lookback && !is_already_usize
                    && !matches!(expr, Expr::Var(ref n) if n == "SUCCESS" || n == "BadParam" || n.starts_with("RetCode"));
                let inner = if needs_cast { format!("({rendered}) as usize") } else { rendered };
                if self.ctx.is_lookback {
                    format!("{pad}return Ok({inner});\n")
                } else {
                    format!("{pad}return {inner};\n")
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
                // A dotted iter var (stream-state field, `sp.j`) is not a valid
                // `for` pattern binding — those loops take the generic fallback.
                if let Some(start_expr) = (self.ctx.for_range_lowering
                    && !iter_name.contains('.'))
                .then(|| extract_init_value(init, iter_name))
                .flatten()
                {
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
                                // A usize iterator seeded from an i32-typed
                                // start (stream transitions: `sp.optInX - 1`)
                                // needs the same cast the assign ladder inserts.
                                let start_str = if expr_is_i32_typed(start_expr)
                                    && !self.ctx.sentinel_vars.contains(&iter_name)
                                {
                                    format!("({start_str}) as usize")
                                } else {
                                    start_str
                                };
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

/// A `case TA_MAType_SMA:` label. Rendered as the qualified member now that the
/// switch subject is the enum itself; it used to be the bare value, which is all
/// an `i32` subject could match against.
fn render_switch_label(label: &str, enums: &HashMap<String, EnumDef>) -> String {
    if let Some((enum_name, variant)) = lookup_variant(label, enums) {
        format!("{enum_name}::{}", variant.name)
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
        Expr::Not(inner) | Expr::BitwiseNot(inner) => expr_has_uncast_array_access(inner),
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
        | Expr::BitwiseNot(_)
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

/// RUST operator precedence (not C's) — this table decides parenthesization of
/// the emitted Rust, so it must follow the Rust grammar. The visible difference
/// from C: bitwise `&`/`^`/`|` bind TIGHTER than comparisons in Rust, the
/// reverse of C. The IR tree already carries C's grouping (the input parser is
/// a C parser), so rendering with Rust's table inserts exactly the parens Rust
/// needs to preserve that grouping.
fn op_precedence(op: &BinOp) -> u8 {
    match op {
        BinOp::Or => 1,
        BinOp::And => 2,
        BinOp::Eq | BinOp::NotEq => 3,
        BinOp::Less | BinOp::LessEq | BinOp::Greater | BinOp::GreaterEq => 4,
        BinOp::BitwiseOr => 5,
        BinOp::BitwiseXor => 6,
        BinOp::BitwiseAnd => 7,
        BinOp::Shl | BinOp::Shr => 8,
        BinOp::Add | BinOp::Sub => 9,
        BinOp::Mul | BinOp::Div | BinOp::Mod => 10,
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
    let is_cmp = |op: &BinOp| {
        matches!(
            op,
            BinOp::Eq | BinOp::NotEq | BinOp::Less | BinOp::LessEq | BinOp::Greater | BinOp::GreaterEq
        )
    };
    match expr {
        Expr::Cast(_, _) => format!("({})", render_expr(expr, ctx, opt_real_params, registry, helpers)),
        // Integer-bitwise operand of a logical operator: C truthiness needs an
        // explicit comparison in Rust ( `(x & 1) && p` → `(x & 1) != 0 && p` ).
        e if is_int_bitwise(e) && matches!(parent_op, BinOp::And | BinOp::Or) => {
            let rendered = render_expr(expr, ctx, opt_real_params, registry, helpers);
            format!("({rendered}) != 0")
        }
        // `!(x & 1)` as a logical operand: invert to == 0 (Rust `!` on an
        // integer is bitwise complement, not logical not).
        Expr::Not(inner) if is_int_bitwise(inner) && matches!(parent_op, BinOp::And | BinOp::Or) => {
            let rendered = render_expr(inner, ctx, opt_real_params, registry, helpers);
            format!("({rendered}) == 0")
        }
        Expr::BinOp(_, child_op, _) => {
            let parent_prec = op_precedence(parent_op);
            let child_prec = op_precedence(child_op);
            // Rust comparisons are one non-associative tier: a comparison
            // nested in a comparison must be parenthesized to parse at all.
            if is_cmp(parent_op) && is_cmp(child_op)
                || child_prec < parent_prec
                || (!is_left && child_prec == parent_prec)
            {
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
        | Expr::BitwiseNot(_)
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
    // Integer-valued bitwise expression as condition (C truthiness): Rust
    // needs an explicit comparison. `!(x & k)` inverts to == 0.
    if is_int_bitwise(expr) {
        let rendered = render_expr(expr, ctx, opt_real_params, registry, helpers);
        return format!("({rendered}) != 0");
    }
    if let Expr::Not(inner) = expr {
        if is_int_bitwise(inner) {
            let rendered = render_expr(inner, ctx, opt_real_params, registry, helpers);
            return format!("({rendered}) == 0");
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
    // C's bitwise complement `~x` is spelled `!x` in Rust (on integers).
    fn bitwise_not(&self, inner: &Expr) -> String {
        format!("!({})", self.walk(inner))
    }

    // C's logical `!` over an integer-bitwise value yields int 0/1. Rust's `!`
    // on an integer is bitwise complement, so spell out the comparison. All
    // condition contexts intercept this shape earlier (render_condition /
    // render_binop_operand); this covers value position, where usize is the
    // backend's default integer — a signed target fails to compile rather than
    // silently wrapping.
    fn not(&self, inner: &Expr) -> String {
        if is_int_bitwise(inner) {
            return format!("usize::from(({}) == 0)", self.walk(inner));
        }
        format!("!({})", self.walk(inner))
    }

    fn var(&self, name: &str) -> String {
        match name {
            "COMPATIBILITY" => "(self.compatibility)".to_string(),
            "METASTOCK" => "Compatibility::Metastock".to_string(),
            "DEFAULT" => "Compatibility::Default".to_string(),
            "BAD_PARAM" => "RetCode::BadParam".to_string(),
            "SUCCESS" => "RetCode::Success".to_string(),
            "ALLOC_ERR" => "RetCode::AllocErr".to_string(),
            "INTERNAL_ERROR" => "RetCode::InternalError".to_string(),
            // MAType constants (`TA_MAType_SMA` → `"0"`) resolve from the
            // enums.yaml-derived map on the ctx; unknown names pass through.
            _ => self.ctx.matype_map.get(name).cloned().unwrap_or_else(|| name.to_string()),
        }
    }

    fn array_access(&self, name: &str, idx: &Expr) -> String {
        let idx_rendered =
            render_index_expr(idx, self.ctx, self.opt_real_params, self.registry, self.helpers);
        // Always safe `[]` indexing. The bounds-assert preamble lets LLVM elide the
        // per-access checks, so this is as fast as a raw-pointer path while keeping
        // the crate `unsafe`-free.
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
        // Negative-capable (int)(float) casts do NOT reach this hook in the
        // saturating form: issue #160 classifies their destinations signed
        // (render_signed_dest_value emits `as i32`), and any position the
        // backend cannot render sign-faithfully fails generation loudly in
        // reject_unsupported_negative_casts. This default `as usize` therefore
        // serves int-typed inners and provably non-negative float inners
        // (sqrt/fabs), where saturation is unreachable.
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

/// Parenthesize a whole `as` cast an operand is being wrapped in, e.g.
/// `dqI[hd]` → `((dqI[hd]) as usize)`.
///
/// The outer parens are not cosmetic. Rust refuses to *parse* a cast followed by
/// `<` or `<<` — it reads `usize <` as the start of generic arguments — so a bare
/// `(x) as usize` is unparseable both as the left operand of `a < b` and, via an
/// enclosing comparison, as the tail of a higher-precedence arithmetic operand
/// (`t + (x) as usize < u`; [`render_binop_operand`] leaves that child unwrapped
/// because its precedence is higher). Wrapping unconditionally at every site
/// keeps that out of the emitter's reasoning entirely.
///
/// Every cast [`render_binop`] emits from an *operand* goes through here —
/// including the `as f64` ones, which already wrapped and so are unchanged — so
/// the invariant is one function rather than a rule each site has to re-derive.
/// The one cast it does not cover is the FMA fusion's `({a} as f64).mul_add(..)`,
/// which returns immediately and whose cast is followed by `)` by construction.
/// Issue #159.
fn wrap_cast(operand: &str, ty: &str) -> String {
    format!("(({operand}) as {ty})")
}

/// Render an `Expr::BinOp` to Rust, including the FMA fusion (via the shared
/// [`fma::fuse_operands`] detector, gated by [`fma::EMIT_FMA`]), pointer-identity
/// buffer comparisons, and the operand int/usize/f64 cast inference. Delegated to
/// by [`RustExpr::binop`].
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
    // Fused multiply-add: (a * b) + c → (a as f64).mul_add(b, c). Emits ARM
    // fmadd (1 FP op) vs fmul+fadd (2 FP ops), IEEE-754 correctly-rounded so it
    // matches the C `fma()` / Java `Math.fma` the other backends emit at the same
    // sites (site selection lives in `fma::fuse_operands`, shared by all backends).
    // Never in a lookback body: C, Java and C# all pass `fma: None` there
    // ("pure integer index arithmetic", c.rs), so fusing only in Rust would
    // give one backend a different lookback — an outBegIdx/length divergence,
    // not a tolerance one. Before issue #158 the lookback ctx was empty, so
    // `real_vars` was too and the detector could never fire; populating it is
    // what made this reachable.
    if fma::EMIT_FMA && !ctx.is_lookback {
        if let Some((a, b, c)) = fma::fuse_operands(left, op, right, &ctx.fma_view()) {
            let a_str = render_expr(a, ctx, opt_real_params, registry, helpers);
            let b_str = render_expr(b, ctx, opt_real_params, registry, helpers);
            let c_str = render_expr(c, ctx, opt_real_params, registry, helpers);
            return format!("({a_str} as f64).mul_add({b_str}, {c_str})");
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
        BinOp::BitwiseXor => " ^ ",
        BinOp::BitwiseAnd => " & ",
        BinOp::Shr => " >> ",
        BinOp::Shl => " << ",
    };
    let is_arithmetic = matches!(
        op,
        BinOp::Add
            | BinOp::Sub
            | BinOp::Mul
            | BinOp::Div
            | BinOp::Mod
            | BinOp::Shr
            | BinOp::Shl
            | BinOp::BitwiseAnd
            | BinOp::BitwiseOr
            | BinOp::BitwiseXor
    );
    let mut left_str = render_binop_operand(left, op, true, ctx, opt_real_params, registry, helpers);
    let mut right_str = render_binop_operand(right, op, false, ctx, opt_real_params, registry, helpers);
    if is_arithmetic {
        // Sentinel vars are i32 — when mixed with usize, cast usize→i32
        let left_is_sentinel = expr_is_i32_typed_ctx(left, ctx) && !expr_is_i32_typed(left);
        let right_is_sentinel = expr_is_i32_typed_ctx(right, ctx) && !expr_is_i32_typed(right);
        if left_is_sentinel && !right_is_sentinel && !expr_is_i32_typed(right) && !expr_is_float_typed_ctx(right, Some(ctx)) && !matches!(right, Expr::IntLiteral(_)) {
            right_str = wrap_cast(&right_str, "i32");
        }
        if right_is_sentinel && !left_is_sentinel && !expr_is_i32_typed(left) && !expr_is_float_typed_ctx(left, Some(ctx)) && !matches!(left, Expr::IntLiteral(_)) {
            left_str = wrap_cast(&left_str, "i32");
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
                left_str = wrap_cast(&left_str, "f64");
            }
            if right_is_i32 && left_is_float && !right_is_int_lit && !right_is_float {
                right_str = wrap_cast(&right_str, "f64");
            }
            let left_is_untyped_int = expr_is_untyped_integer(left);
            let right_is_untyped_int = expr_is_untyped_integer(right);
            if left_is_untyped_int && !left_is_int_lit && right_is_float {
                left_str = wrap_cast(&left_str, "f64");
            }
            if right_is_untyped_int && !right_is_int_lit && left_is_float {
                right_str = wrap_cast(&right_str, "f64");
            }
            let left_is_known_usize = expr_is_known_usize_ctx(left, ctx);
            let right_is_known_usize = expr_is_known_usize_ctx(right, ctx);
            let left_eff_usize = left_is_known_usize
                || expr_binop_renders_as_usize(left, ctx);
            let right_eff_usize = right_is_known_usize
                || expr_binop_renders_as_usize(right, ctx);
            if left_eff_usize && right_is_float && !left_is_i32 {
                left_str = wrap_cast(&left_str, "f64");
            }
            if right_eff_usize && left_is_float && !right_is_i32 {
                right_str = wrap_cast(&right_str, "f64");
            }
        }
        // Cast i32 operands to usize when mixed with usize-typed operands (not float)
        // Also detect i32 array accesses (IntArray/IntPointer)
        let arith_left_is_i32_arr = expr_is_int_array_typed(left, ctx);
        let arith_right_is_i32_arr = expr_is_int_array_typed(right, ctx);
        let left_is_i32_eff = left_is_i32 || arith_left_is_i32_arr;
        let right_is_i32_eff = right_is_i32 || arith_right_is_i32_arr;
        let left_is_usize = !left_is_i32_eff
            && !left_is_float
            && !left_is_int_lit
            && !expr_is_enum_typed(left, ctx);
        let right_is_usize = !right_is_i32_eff
            && !right_is_float
            && !right_is_int_lit
            && !expr_is_enum_typed(right, ctx);
        if left_is_i32_eff && right_is_usize && !left_is_sentinel {
            left_str = wrap_cast(&left_str, "usize");
        }
        if right_is_i32_eff && left_is_usize && !right_is_sentinel {
            right_str = wrap_cast(&right_str, "usize");
        }
        // When both sides appear i32-typed but one actually renders as usize
        // (e.g., Cast(Integer, usize_expr) drops the cast), fix the mismatch.
        if left_is_i32_eff && right_is_i32_eff && !left_is_int_lit && !right_is_int_lit {
            let left_renders_usize = expr_renders_as_usize_despite_i32(left, ctx);
            let right_renders_usize = expr_renders_as_usize_despite_i32(right, ctx);
            if left_renders_usize && !right_renders_usize {
                right_str = wrap_cast(&right_str, "usize");
            }
            if right_renders_usize && !left_renders_usize {
                left_str = wrap_cast(&left_str, "usize");
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
            right_str = wrap_cast(&right_str, "i32");
        }
        if cmp_right_sentinel && !cmp_left_sentinel && !expr_is_i32_typed(left) && !expr_is_float_typed_ctx(left, Some(ctx)) && !matches!(left, Expr::IntLiteral(_)) {
            left_str = wrap_cast(&left_str, "i32");
        }
        let left_is_i32 =
            !expr_is_enum_typed(left, ctx) && (expr_is_i32_typed(left) || cmp_left_sentinel);
        let right_is_i32 =
            !expr_is_enum_typed(right, ctx) && (expr_is_i32_typed(right) || cmp_right_sentinel);
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
                left_str = wrap_cast(&left_str, "f64");
            }
            if right_is_i32 && left_is_float && !right_is_int_lit && !right_is_float {
                right_str = wrap_cast(&right_str, "f64");
            }
            if left_is_untyped_int && !left_is_int_lit && right_is_float {
                left_str = wrap_cast(&left_str, "f64");
            }
            if right_is_untyped_int && !right_is_int_lit && left_is_float {
                right_str = wrap_cast(&right_str, "f64");
            }
            let cmp_left_is_known_usize = expr_is_known_usize_ctx(left, ctx);
            let cmp_right_is_known_usize = expr_is_known_usize_ctx(right, ctx);
            if cmp_left_is_known_usize && right_is_float && !cmp_right_is_known_usize && !left_is_i32 && !left_is_int_lit {
                left_str = wrap_cast(&left_str, "f64");
            }
            if cmp_right_is_known_usize && left_is_float && !cmp_left_is_known_usize && !right_is_i32 && !right_is_int_lit {
                right_str = wrap_cast(&right_str, "f64");
            }
        }
        // Also detect i32 array accesses (IntArray/IntPointer) using context
        let left_is_i32_arr = expr_is_int_array_typed(left, ctx);
        let right_is_i32_arr = expr_is_int_array_typed(right, ctx);
        let left_is_i32_eff = left_is_i32 || left_is_i32_arr;
        let right_is_i32_eff = right_is_i32 || right_is_i32_arr;
        if left_is_i32_eff && !right_is_i32_eff && !right_is_float && !right_is_int_lit && !cmp_left_sentinel {
            left_str = wrap_cast(&left_str, "usize");
        }
        if right_is_i32_eff && !left_is_i32_eff && !left_is_float && !left_is_int_lit && !cmp_right_sentinel {
            right_str = wrap_cast(&right_str, "usize");
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
        e if is_int_bitwise(e) => true,
        Expr::Not(inner) if is_int_bitwise(inner) => true,
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

pub(crate) fn render_expr(
    expr: &Expr,
    ctx: &RustRenderCtx,
    opt_real_params: &[String],
    registry: &Registry,
    helpers: &HelperRegistry,
) -> String {
    RustExpr { ctx, opt_real_params, registry, helpers }.walk(expr)
}

/// Render one of the boolean value builtins (the near-zero trio IS_ZERO /
/// IS_ZERO_SCALED / IS_ZERO_OR_NEG, plus the exact IS_FINITE) in Rust from already-rendered argument strings. Single source
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
        SpecialBuiltin::IsFinite => args
            .first()
            .map_or_else(|| "false".to_string(), |x| format!("({x}).is_finite()")),
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

/// Thin adapter over the shared [`fma::expr_is_float_typed`] so existing Rust
/// call sites keep their `Option<&RustRenderCtx>` signature. Used to decide
/// whether an `IntLiteral` should be wrapped with `T::ta_from_i32()` and to gate
/// the FMA detector; conservative (strong evidence the expression produces T).
fn expr_is_float_typed_ctx(expr: &Expr, ctx: Option<&RustRenderCtx>) -> bool {
    // An enum-typed local is neither integer nor float. It leaves `index_vars`
    // (see `prune_enum_locals`), so without this the name heuristics claim it
    // for `f64` and every assignment renders `as f64`.
    if let (Expr::Var(name), Some(c)) = (expr, ctx) {
        if c.enum_vars.contains_key(name) {
            return false;
        }
    }
    let view = ctx.map(RustRenderCtx::fma_view);
    fma::expr_is_float_typed(expr, view.as_ref())
}

/// A value of an enum type: an `enum:` parameter, a local derived from one, or
/// one of the enum's own constants. Neither integer nor float, so it takes no
/// cast in either direction — and comparing two of them needs none.
fn expr_is_enum_typed(expr: &Expr, ctx: &RustRenderCtx) -> bool {
    matches!(expr, Expr::Var(name)
        if ctx.enum_vars.contains_key(strip_state_prefix(name))
            || ctx.matype_map.contains_key(name))
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
        Expr::BinOp(
            left,
            BinOp::Add
            | BinOp::Sub
            | BinOp::Mul
            | BinOp::Div
            | BinOp::Shr
            | BinOp::Shl
            | BinOp::Mod
            | BinOp::BitwiseAnd
            | BinOp::BitwiseOr
            | BinOp::BitwiseXor,
            right,
        ) => {
            expr_is_i32_typed(left) && (expr_is_i32_typed(right) || matches!(right.as_ref(), Expr::IntLiteral(_)))
                || expr_is_i32_typed(right) && matches!(left.as_ref(), Expr::IntLiteral(_))
        }
        Expr::BitwiseNot(inner) => expr_is_i32_typed(inner),
        Expr::Cast(VarType::Integer, _inner) => {
            true
        }
        _ => false,
    }
}

/// Context-aware version of `expr_is_i32_typed` that also recognizes sentinel variables.
fn expr_is_i32_typed_ctx(expr: &Expr, ctx: &RustRenderCtx) -> bool {
    if expr_is_enum_typed(expr, ctx) {
        return false;
    }
    if expr_is_i32_typed(expr) {
        return true;
    }
    match expr {
        Expr::Var(name) => ctx.sentinel_vars.contains(name),
        // The SAME operator set `expr_is_i32_typed` folds over — the shifts and
        // the bitwise ones included, not just the four arithmetic. Stop at those
        // four and `lag & 3` (a signed local masked by a literal) is typed by
        // nothing, so a usize target takes no cast from the assign ladder, though
        // the bare `head = lag` does. Issue #165.
        Expr::BinOp(
            left,
            BinOp::Add
                | BinOp::Sub
                | BinOp::Mul
                | BinOp::Div
                | BinOp::Mod
                | BinOp::Shl
                | BinOp::Shr
                | BinOp::BitwiseAnd
                | BinOp::BitwiseOr
                | BinOp::BitwiseXor,
            right,
        ) => {
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
pub(crate) fn expr_is_untyped_integer(expr: &Expr) -> bool {
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
    func: &FuncDef,
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

    // Issue #158: the declaration emitter and the use-site classifier must be
    // driven by the same context, or a local is declared one type and assigned
    // as another.
    let mut lookback_ctx = RustRenderCtx::for_lookback(stmts);
    lookback_ctx.matype_map = build_matype_map(enums);
    lookback_ctx.enum_vars = enum_local_types(func);
    prune_enum_locals(&mut lookback_ctx.index_vars, &lookback_ctx.enum_vars);

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
                    let is_sentinel = lookback_ctx.sentinel_vars.contains(name);
                    let rust_type = match var_type {
                        VarType::Real => "f64",
                        VarType::Integer | VarType::Index => {
                            if is_sentinel { "i32" } else { "usize" }
                        }
                        VarType::RetCodeType => "RetCode",
                        VarType::RealPointer => "Vec<f64>",
                        VarType::IntPointer => "Vec<i32>",
                        VarType::RealArray(_) | VarType::IntArray(_) => unreachable!(),
                    };
                    // Always initialize — lookback is always concrete (non-generic)
                    let default_val = match var_type {
                        VarType::Real => "0.0_f64",
                        VarType::Integer | VarType::Index => {
                            if is_sentinel { "0i32" } else { "0_usize" }
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

    // Emit candle settings unpacking for lookback body
    let candle_used = detect_candle_settings(stmts);
    if !candle_used.is_empty() {
        out.push_str(&emit_rust_unpacking(&candle_used, 8));
    }

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

/// The `match` arms of `ta_candlerange`, shared by the two sites that inline it
/// (`ta_candlerange` itself and the `avgPeriod == 0` fallback inside
/// `ta_candleaverage`).
///
/// The Shadows arm is upper + lower, NOT the algebraically equal
/// `(high - low) - |close - open|`. It must match `TA_CANDLERANGE` in
/// `ta_utility.h` term for term: the two forms differ by reassociation on any
/// bar whose low sits below half its high, and C is the reference (#217).
/// Like `java.rs` and `csharp.rs`, this spelling is hardcoded here rather than
/// read from `input/helpers/candlestick.c`, so a fix to the helper alone does
/// NOT reach Rust -- those two backends carry the same duplicate. The final arm
/// is `0.0` for the same reason: `TA_CANDLERANGE`'s innermost ternary falls
/// through to `0`, so folding it into the Shadows arm would answer an
/// out-of-range rangeType differently than C does.
fn candle_range_arms(open: &str, high: &str, low: &str, close: &str) -> String {
    format!(
        "0 => (({close}) - ({open})).abs(), \
         1 => ({high}) - ({low}), \
         2 => (({high}) - (if ({close}) >= ({open}) {{ ({close}) }} else {{ ({open}) }})) \
            + ((if ({close}) >= ({open}) {{ ({open}) }} else {{ ({close}) }}) - ({low})), \
         _ => 0.0"
    )
}

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
                    return format!("self.unstable_period[FuncUnstId::{base} as usize]");
                }
                "self.unstable_period[0]".to_string()
            }
            SpecialBuiltin::Compatibility => "self.compatibility".to_string(),
            pred @ (SpecialBuiltin::IsZero
                   | SpecialBuiltin::IsZeroScaled
                   | SpecialBuiltin::IsZeroOrNeg
                   | SpecialBuiltin::IsFinite) => {
                // The near-zero trio -> the Rust epsilon form; IS_FINITE -> is_finite().
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
    } else if fname.ends_with("_lookback") || fname.ends_with("_Lookback") {
        // Two authored spellings reach here and name the same function: the
        // prefix-free `sma_lookback`, and the legacy `TA_SMA_Lookback` whose
        // `TA_` the parser has already stripped. Lower-casing folds the legacy
        // form onto the registry's key, so both resolve through one rule — the C
        // backend keeps the same pair of spellings alive (`c.rs`, "Legacy:").
        let rust_name =
            registry.resolve_call(&fname.to_lowercase(), crate::registry::Lang::Rust);
        let rendered_args: Vec<String> = args
            .iter()
            .map(|a| {
                // Real optIn params stay as f64 in lookback calls (no T-wrapping, no i32 cast)
                let is_opt_real = matches!(a, Expr::Var(n) if !is_i32_opt_in_param(n) && strip_state_prefix(n).starts_with("optIn"));
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
        let call = format!("self.{}({})", rust_name, rendered_args.join(", "));
        // The callee returns `Result<usize, RetCode>`. Inside another
        // Result-returning body (a lookback body composing a callee's lookback,
        // or the stream tier) the failure genuinely propagates with `?`.
        // Inside the plain batch/`_Impl` tier (bare `RetCode`, no `?` available)
        // the call sits at a point where the same parameters were already
        // validated by this function's own prologue against an identical range,
        // so `Err` is unreachable in practice. `unwrap_or(usize::MAX)` rather
        // than an assert per call site: it yields the same sentinel a failing
        // lookback reports, so the behaviour holds even if that ever stops
        // being true.
        if ctx.is_lookback || ctx.result_error_returns {
            format!("{call}?")
        } else {
            format!("{call}.unwrap_or(usize::MAX)")
        }
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
                // Deallocation is removed from the IR before rendering, so this
                // arm is the assertion that it was -- not a second way to make a
                // `free` vanish, which could disagree with the pass.
                unreachable!(
                    "free() reached the Rust renderer: `ir_cleanup::drop_deallocation` \
                     runs on every body this backend renders, so a `free` here means a \
                     render path was added without the cleanup sequence"
                )
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
        format!("(match {rt} {{ {} }})", candle_range_arms(open, high, low, close))
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
             match {rt} {{ {} }} \
             }}) / (if ({rt}) == 2 {{ 2.0 }} else {{ 1.0 }}))",
            candle_range_arms(open, high, low, close)
        )
    } else if registry.contains(fname) || fname.ends_with("_private") {
        // Cross-indicator call: use registry to resolve the function name.
        // Bare names (ema) → ema. Private names (ema_private) → ema_private.
        let resolved = registry.resolve_call(fname, crate::registry::Lang::Rust);
        let nullable = registry.callee_out_nullable(fname.trim_end_matches("_private"));
        let (rendered_args, aliased) = render_cross_indicator_args(args, None, nullable, ctx, opt_real_params, registry, helpers);
        wrap_cross_indicator_call(format!("self.{}({})", internal_callee(&resolved), rendered_args.join(", ")), &aliased)
    } else if is_ta_function(fname) {
        let rust_name = fname.to_uppercase();
        let nullable = registry.callee_out_nullable(&fname.to_lowercase());
        let (rendered_args, aliased) = render_cross_indicator_args(args, None, nullable, ctx, opt_real_params, registry, helpers);
        wrap_cross_indicator_call(format!("self.{}({})", internal_callee(&rust_name), rendered_args.join(", ")), &aliased)
    } else {
        let rendered_args: Vec<String> = args
            .iter()
            .map(|a| render_expr(a, ctx, opt_real_params, registry, helpers))
            .collect();
        format!("{}({})", fname, rendered_args.join(", "))
    }
}

/// Emit a cross-indicator call to the callee's PUBLIC entry point (#267).
///
/// The Rust twin of [`super::java::render_cross_indicator_call`], and it exists
/// for the same reason: the C source is written in C's idiom -- `retCode = ma(
/// .., &beg, &nb, buf ); if( retCode != TA_SUCCESS ) return retCode;` -- so a
/// literal transcription needs a callee that answers a code through
/// out-parameters. C never did: `ta_APO.c` calls `TA_MA`, which IS C's public
/// API. All four backends now do the same, which is what puts the callee's
/// argument checks on the composed path.
///
/// One difference from Java, and it is the whole difference: Java's public tier
/// throws, so the caller's `retCode` is `Success` by construction and nothing
/// needs writing out. Rust's answers `Err(RetCode)`, and `?` is unavailable in
/// the 33 sites inside `<N>_Impl`, which returns a bare `RetCode` -- so the error
/// arm is spelled at every site, as `return _e` there and as `return Err(_e)` at
/// the three inside a `Result`-returning `<n>_open_impl` (`err_returns_result`).
/// `retCode` is still assigned `Success` afterwards. The guard that used to read
/// it is folded away by `ir_cleanup::drop_answered_cross_call_guards`, but 10
/// of the sites fold "success with zero output" into the same conditional
/// (`if retCode != Success || *outNBElement == 0`); the surviving half still
/// reads the variable, and dropping the store would need a liveness analysis
/// that `MA`'s ten dispatch arms defeat.
///
/// The two out-parameter arguments are dropped from the call and bound from the
/// returned [`OutRange`] instead. They are found by ARITHMETIC, as in Java --
/// the callee's signature is `(startIdx, endIdx, inputs.., opts.., outBegIdx,
/// outNBElement, outputs..)` and the registry knows how many outputs it declares
/// -- not by `find_output_boundary`'s scan, which cannot see the difference
/// between an out-meta pair and a pair of `&scalar` outputs. Returns `None` when
/// that arithmetic does not hold, so a shape this does not understand falls
/// through to the old rendering rather than being silently mis-sliced.
/// Where a cross-indicator call's out-meta pair sits in its argument list, and
/// the admission test for the whole rewrite: `None` means this shape is not
/// understood, the call falls through to `render_func_call`, and the caller's
/// `retCode` then really does carry the callee's code.
///
/// The renderer and [`ir_cleanup::drop_answered_cross_call_guards`] must agree
/// about that, which is the only reason this is a function rather than inline:
/// a pass that folded a guard the renderer had declined would swallow a live
/// rejection. The bound is `n_out + 4`, not Java's `n_out + 2` -- do not share
/// this across backends.
pub(super) fn cross_call_split(
    fname: &str,
    args: &[Expr],
    registry: &Registry,
) -> Option<usize> {
    let n_out = registry.callee_outputs(fname).len();
    if n_out == 0 || args.len() < n_out + 4 {
        return None;
    }
    let split = args.len() - n_out - 2;
    // Both out-meta arguments are either `&local` or a pointer parameter passed
    // straight through. Anything else means the arity arithmetic landed
    // somewhere it should not have.
    if !args[split..split + 2]
        .iter()
        .all(|a| matches!(a, Expr::AddressOf(_) | Expr::Var(_)))
    {
        return None;
    }
    Some(split)
}

fn render_cross_indicator_call(
    fname: &str,
    args: &[Expr],
    indent: usize,
    err_returns_result: bool,
    ctx: &RustRenderCtx,
    opt_real_params: &[String],
    registry: &Registry,
    helpers: &HelperRegistry,
    counter: &Cell<usize>,
) -> Option<String> {
    let split = cross_call_split(fname, args, registry)?;

    let pad = " ".repeat(indent);
    let public = registry.resolve_call(fname, crate::registry::Lang::Rust);
    let nullable = registry.callee_out_nullable(fname);
    let (rendered_args, aliased) = render_cross_indicator_args(
        args, Some(split), nullable, ctx, opt_real_params, registry, helpers,
    );
    let call = wrap_cross_indicator_call(
        format!("self.{public}({})", rendered_args.join(", ")),
        &aliased,
    );
    // The aliasing shim is a block expression; parenthesize it so it cannot be
    // read as the `match` arm list.
    let scrutinee = if aliased.is_empty() { call } else { format!("({call})") };

    let n = counter.get();
    counter.set(n + 1);
    let tmp = format!("_xr{n}");
    let err = if err_returns_result { "return Err(_e)" } else { "return _e" };
    let beg = out_meta_target(&args[split], ctx, opt_real_params, registry, helpers);
    let nb = out_meta_target(&args[split + 1], ctx, opt_real_params, registry, helpers);
    Some(format!(
        "{pad}let {tmp} = match {scrutinee} {{ Ok(_r) => _r, Err(_e) => {err} }};\n\
         {pad}{beg} = {tmp}.beg_idx;\n\
         {pad}{nb} = {tmp}.count;\n"
    ))
}

/// The assignment target an out-parameter argument names. `&fastBeg` is a local
/// `usize`; `outNBElement` passed straight through is the caller's own `&mut
/// usize` parameter and needs the deref. Only an rvalue READ of one goes through
/// [`render_expr`], which is why this cannot.
fn out_meta_target(
    arg: &Expr,
    ctx: &RustRenderCtx,
    opt_real_params: &[String],
    registry: &Registry,
    helpers: &HelperRegistry,
) -> String {
    match arg {
        Expr::AddressOf(inner) => match inner.as_ref() {
            Expr::Var(n) => n.clone(),
            other => render_expr(other, ctx, opt_real_params, registry, helpers),
        },
        Expr::Var(n) => format!("(*{n})"),
        other => render_expr(other, ctx, opt_real_params, registry, helpers),
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
    out_meta: Option<usize>,
    callee_out_nullable: &[bool],
    ctx: &RustRenderCtx,
    opt_real_params: &[String],
    registry: &Registry,
    helpers: &HelperRegistry,
) -> (Vec<String>, Vec<String>) {
    // `out_meta` is the index of the callee's `outBegIdx` argument when the call
    // goes to the PUBLIC entry point (#267): the two out-meta arguments are
    // DROPPED -- the public tier returns the range -- and the outputs start two
    // slots later, which keeps every `callee_out_nullable` ordinal correct.
    //
    // `None` is the legacy expression-position rendering, which still targets
    // `<N>_Impl` and still passes them. Nothing in the corpus reaches it (the
    // 36 cross-calls are all statements, and `rust_cross_calls_target_the_public_tier`
    // pins that); it is kept, like Java's, for a shape neither emitter has met.
    // There the boundary has to be found by scanning for two consecutive
    // AddressOf args -- not the LAST AddressOf, because outputs can be AddressOf
    // too (`&prevATR` for a scalar T).
    let output_start = out_meta.map_or_else(|| find_output_boundary(args), |i| i + 2);

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

    // Detect duplicate AddressOf vars (e.g., &tempInt used for both outBegIdx and
    // outNBElement). Only the legacy rendering needs this: on the public path
    // both of those arguments are dropped, and SAR/SAREXT -- the only calls that
    // pass the same scalar twice -- become two sequential writes to it instead.
    let mut seen_address_of: std::collections::HashSet<String> = std::collections::HashSet::new();
    let mut dup_vars: Vec<(usize, String)> = Vec::new();
    if out_meta.is_none() {
        for (i, arg) in args.iter().enumerate() {
            if let Expr::AddressOf(inner) = arg {
                if let Expr::Var(name) = inner.as_ref() {
                    if !seen_address_of.insert(name.clone()) {
                        dup_vars.push((i, name.clone()));
                    }
                }
            }
        }
    }

    let rendered = args.iter()
        .enumerate()
        .filter(|(i, _)| out_meta.is_none_or(|m| *i != m && *i != m + 1))
        .map(|(i, arg)| {
            let is_output = i >= output_start;
            let rendered = {
                // In-place aliasing: borrow the input immutably and redirect the output to the
                // scratch buffer that `render_func_call` swaps back after the call.
                if let Expr::Var(name) = arg {
                    if aliased_vars.contains(name) {
                        if is_output {
                            format!("&mut _{name}_alias[..]")
                        } else {
                            format!("&{name}")
                        }
                    } else if dup_vars.iter().any(|(idx, _)| *idx == i) {
                        // Duplicate &mut borrow: the pre-declared dummy variable.
                        "&mut _dup_out".to_string()
                    } else {
                        render_cross_indicator_arg(arg, i, is_output, ctx, opt_real_params, registry, helpers)
                    }
                } else if dup_vars.iter().any(|(idx, _)| *idx == i) {
                    "&mut _dup_out".to_string()
                } else {
                    render_cross_indicator_arg(arg, i, is_output, ctx, opt_real_params, registry, helpers)
                }
            };
            // A `nullable` callee output takes an `Option<&mut [T]>` (rule B6a):
            // `NULL` already rendered as `None`, and anything else is a buffer
            // the caller does supply. This wraps EVERY way an output argument can
            // be rendered, the scratch-buffer and dummy redirections included --
            // it sat on the fall-through path alone at first, so a nullable slot
            // reached by either of those would have emitted a bare `&mut [T]`
            // where an `Option` is wanted (unreachable today: MAMA is the only
            // nullable callee and MA hands it `NULL`).
            if is_output
                && callee_out_nullable.get(i - output_start).copied().unwrap_or(false)
                && rendered != "None"
            {
                return format!("Some({rendered})");
            }
            rendered
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
pub(crate) fn render_cross_indicator_arg(
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
        // NULL in an output position: a nullable output the caller declines (MA
        // passes NULL for MAMA's FAMA — issue #125). The callee's parameter is
        // `Option<&mut [T]>`, so this is `None` and nothing is allocated --
        // never a throwaway buffer spanning the output range, the "unchecked
        // malloc-dummy-discard" #125 removed from C and #262 from the rest.
        Expr::Var(name) if is_output_position && name == "NULL" => "None".to_string(),
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
            // where an i32 param is expected (e.g., curPeriod to ma), cast to i32
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

// ---------------------------------------------------------------------------
// Scratch-buffer election by name (issue #146)
// ---------------------------------------------------------------------------

/// Elections in force: local array variable → the output parameter it was
/// elected to. Valid for the remainder of the block the election was written in
/// (and the blocks nested inside it), which is exactly C's scope for it.
type ElectionMap = HashMap<String, String>;

/// Turns C's *pointer-based* scratch-buffer election into a rename, so the Rust
/// body runs the calculation directly in the caller's output slices.
///
/// `BBANDS` opens with `tempBuffer1 = outRealMiddleBand;` and calls it out in a
/// comment: *"Identify TWO temporary buffers among the outputs so the calculation
/// needs no memory allocation"*. In C that is a pointer assignment, so the
/// function allocates nothing and the copy-back below it is skipped. Rust has no
/// pointer to assign, so the statement renders as `.to_vec()` — an allocation
/// plus a copy of bytes that are overwritten before they are ever read, and one
/// sized by the *caller's slice* rather than by the data range, so a caller that
/// allocates its outputs once at capacity and then calls per window pays for the
/// capacity on every call (issue #146).
///
/// What licenses the rename is Rust's own aliasing rule, which is also what makes
/// C's aliasing branches dead code here:
///
/// * `inX: &[T]` and `outY: &mut [T]` are distinct parameters and can never
///   overlap, so every `inX == outY` pointer test is statically false and the
///   aliasing arms of C's election chain are unreachable.
/// * two `&mut [T]` parameters can never overlap either, so electing a *second*
///   output as scratch needs no further check — the entry point's distinctness
///   test on the outputs is C parity, not the licence for this.
///
/// The rule, stated over the IR and over nothing else — no function name, no
/// buffer name, no MA type appears anywhere in this pass:
///
/// 1. match an `if`/`else if`/…/`else` chain whose *every* condition is an
///    input↔output pointer equality and whose *every* arm is only
///    `scratch = someOutput;` elections ([`Self::election_chain`]);
/// 2. take the terminal `else`'s mapping — the binding safe Rust always reaches;
/// 3. delete the chain and rename those scratch names to their elected outputs
///    through the rest of the enclosing block;
/// 4. drop any guard the rename has turned into a self-comparison.
///
/// Being general is not the same as being greedy, and clause 1 is where the
/// restraint lives:
///
/// * `STOCH`, `STOCHF` and `MAVP` mix an allocation and an `…IsAllocated = 1;`
///   flag into a branch, so their arms are not elections and the chain is
///   rejected. Their output is byte-for-byte unchanged. Tolerating one allocating
///   arm would reach them, and is a widening of *this rule* for a later change —
///   never a per-function case.
/// * an election reaches only the end of its own block. `BBANDS` elects inside
///   `if( optInMAType == TA_MAType_SMA ) { ... }`, so the general MA path that
///   follows keeps its genuine `vec![0.0; ...]` allocations, and so do both
///   stream paths.
/// * clause 4 fires only where the rename actually rewrote a name, so a function
///   that elected nothing is never touched. It is also load-bearing rather than
///   cosmetic: `BBANDS`' copy-back guard collapses to
///   `if o.as_ptr() != o.as_ptr() { o[..n].copy_from_slice(&o[..n]) }`, which is
///   E0502 — the optimiser never gets the chance to fold it.
/// * if the local is assigned again anywhere still in scope, the election is left
///   alone, because the rename would then be wrong. The fallback is exactly
///   today's `.to_vec()`.
///
/// `BBANDS` is currently the only function in `input/` written in this shape, but
/// the pass never asks which function it is looking at; anything added in that
/// shape benefits automatically, and the other three backends stay byte-identical
/// because the pass does not run for them.
struct ScratchElection<'a> {
    /// Read-only array parameters (`&[T]`).
    inputs: &'a std::collections::HashSet<String>,
    /// Writable array parameters (`&mut [T]`).
    outputs: &'a std::collections::HashSet<String>,
    /// Array locals declared in the body (C `double *` / `int *`).
    locals: &'a std::collections::HashSet<String>,
}

/// Apply [`ScratchElection`] to both bodies the Rust batch tier renders. The
/// stream tier keeps the original `func` — it composes its own scratch buffers.
pub(crate) fn elect_output_scratch(func: &FuncDef) -> FuncDef {
    let inputs: std::collections::HashSet<String> =
        func.inputs.iter().map(|i| i.name.clone()).collect();
    let outputs: std::collections::HashSet<String> =
        func.outputs.iter().map(|o| o.name.clone()).collect();
    let mut out = func.clone();
    for body in [&mut out.body, &mut out.private_body] {
        let mut locals = std::collections::HashSet::new();
        collect_array_locals(body, &mut locals);
        let pass = ScratchElection { inputs: &inputs, outputs: &outputs, locals: &locals };
        *body = pass.block(body, &ElectionMap::new(), &[]);
    }
    out
}

/// Names declared in the body as C `double *` / `int *` — the only candidates for
/// an election (a fixed-size array cannot be re-pointed).
fn collect_array_locals(body: &[Statement], out: &mut std::collections::HashSet<String>) {
    visit_stmts(body, &mut |stmt| {
        if let Statement::VarDecl { var_type, name, .. } = stmt {
            if matches!(var_type, VarType::RealPointer | VarType::IntPointer) {
                out.insert(name.clone());
            }
        }
    });
}

/// The comment that stands in for C's pointer assignment. The C comments around
/// it keep naming the local — they are shared with every other backend — so the
/// note maps each local onto the output it became.
fn election_note(elected: &[(String, String)]) -> Vec<String> {
    let mut lines = vec![
        "Rust: C's pointer election here is a rename, so the calculation runs".to_string(),
        "directly in the caller's slices:".to_string(),
    ];
    for (local, out) in elected {
        lines.push(format!("  C's `{local}` is `{out}`"));
    }
    lines.push("This function therefore allocates nothing, exactly as the C does.".to_string());
    lines.push("The aliasing arms, the input-alias guard and the copy-back are all".to_string());
    lines.push("unreachable here: `&[T]` and `&mut [T]` parameters can never".to_string());
    lines.push("overlap, and neither can two `&mut [T]`. See issue #146.".to_string());
    lines
}

/// Pre-order walk over a statement tree, nested blocks included.
///
/// Exhaustive over `Statement` on purpose — no `_ => {}` arm. A catch-all here
/// would silently skip the body-bearing variants, so the walk would miss the
/// statements that matter most and would go on missing them the next time a
/// variant is added. Let the compiler raise it instead.
fn visit_stmts(stmts: &[Statement], f: &mut impl FnMut(&Statement)) {
    for stmt in stmts {
        f(stmt);
        match stmt {
            Statement::While { body, .. }
            | Statement::DoWhile { body, .. }
            | Statement::For { body, .. }
            | Statement::Block { body } => visit_stmts(body, f),
            Statement::If { then_body, else_body, .. } => {
                visit_stmts(then_body, f);
                visit_stmts(else_body, f);
            }
            Statement::Switch { cases, default, .. } => {
                for (_, body) in cases {
                    visit_stmts(body, f);
                }
                visit_stmts(default, f);
            }
            Statement::ForC { init, update, body, .. } => {
                f(init);
                f(update);
                visit_stmts(body, f);
            }
            // Leaves: no nested statements to walk into.
            Statement::VarDecl { .. }
            | Statement::Assign { .. }
            | Statement::UnrollHint { .. }
            | Statement::Return { .. }
            | Statement::Break
            | Statement::Continue
            | Statement::Expr(_)
            | Statement::CircBuf(_)
            | Statement::Comment(_) => {}
        }
    }
}

impl ScratchElection<'_> {
    /// Rewrite one block. `elections` are the ones inherited from enclosing blocks.
    fn block(&self, stmts: &[Statement], elections: &ElectionMap, outer: &[Statement]) -> Vec<Statement> {
        let mut elections = elections.clone();
        let mut queue: std::collections::VecDeque<Statement> = stmts.iter().cloned().collect();
        let mut out: Vec<Statement> = Vec::new();
        while let Some(stmt) = queue.pop_front() {
            // The elections in force rename this statement's own expressions. The
            // nested bodies are renamed when `descend` walks into them, so
            // `rewritten` stays a fact about *this* statement.
            let (stmt, rewritten) = rename_shallow(&stmt, &elections);
            // An election chain: install the terminal arm's mapping, delete the
            // chain, rename the uses that follow. This is the pass's *only* way
            // to create an election — see [`ScratchElection::election_chain`].
            if let Some(elected) = self.election_chain(&stmt) {
                if elected.iter().all(|(local, _)| {
                    !elections.contains_key(local)
                        && references_var(queue.iter(), local)
                        && !assigns_whole_var(queue.iter(), local)
                        // C's scratch local is *function*-scoped: a pointer elected
                        // inside a nested block still points at that output after the
                        // block ends. The rename only reaches the end of this block, so
                        // it is only equivalent when nothing after the block can observe
                        // the local — either control never falls out of here, or the
                        // enclosing scopes never mention it again.
                        && (tail_always_returns(queue.iter())
                            || !references_var(outer.iter(), local))
                }) {
                    for (local, src) in &elected {
                        elections.insert(local.clone(), src.clone());
                    }
                    out.push(Statement::Comment(election_note(&elected)));
                    continue;
                }
            }
            // A guard the rename turned into a self-comparison, which this
            // backend can now decide. Folding it is load-bearing, not tidiness:
            // left in place, `BBANDS`' copy-back would read and write the same
            // `&mut` slice in one statement, which is E0502.
            if rewritten {
                if let Statement::If { condition, then_body, else_body, .. } = &stmt {
                    if let Some(value) = self.static_ptr_cond(condition) {
                        let taken = if value { then_body } else { else_body };
                        if taken.iter().all(|s| matches!(s, Statement::Comment(_))) {
                            // The statement is gone; so is the comment that
                            // introduced it.
                            if matches!(out.last(), Some(Statement::Comment(_))) {
                                out.pop();
                            }
                        }
                        for s in taken.iter().rev() {
                            queue.push_front(s.clone());
                        }
                        continue;
                    }
                }
            }
            let outer_for_children: Vec<Statement> =
                queue.iter().cloned().chain(outer.iter().cloned()).collect();
            out.push(self.descend(stmt, &elections, &outer_for_children));
        }
        out
    }

    /// Match a whole `if`/`else if`/…/`else` chain that is *nothing but* a
    /// scratch-buffer election, and return the terminal `else`'s mapping — the
    /// binding safe Rust always reaches. `None` leaves the statement alone.
    ///
    /// All four conditions have to hold at once:
    ///
    /// 1. every link's condition is a bare pointer equality between an array
    ///    *parameter* pair Rust decides statically (an input against an output);
    /// 2. every `then` arm consists only of `local = someOutput;` elections
    ///    (comments aside) and elects at least one;
    /// 3. the chain ends in an `else` that does the same;
    /// 4. nothing else appears in any arm.
    ///
    /// (4) is what keeps the pass conservative rather than greedy, and it is the
    /// clause that declines `STOCH`, `STOCHF` and `MAVP`: their arms mix an
    /// allocation and a `…IsAllocated = 1;` flag into the branch, so they are not
    /// elections — they are a genuine in-place defence with a real buffer to
    /// allocate. `MAVP` is inverted as well (the allocation in the `then`, the
    /// election in the `else`), so (2) rejects it on the first link. Reaching those
    /// needs a matcher that tolerates one allocating arm; that is a widening of
    /// this rule, not a special case bolted onto it.
    ///
    /// Matching happens *before* [`Self::descend`] recurses (see
    /// [`ScratchElection`]): a pass that walked the child blocks first would
    /// collapse an inner `else if` link and silently truncate the chain.
    fn election_chain(&self, stmt: &Statement) -> Option<Vec<(String, String)>> {
        let Statement::If { condition, then_body, else_body, .. } = stmt else {
            return None;
        };
        // (1) the link's own condition.
        if !self.is_alias_test(condition) {
            return None;
        }
        // (2) the `then` arm elects, and does nothing else.
        self.arm_elections(then_body)?;
        // (3)/(4) either the chain continues, or this `else` is the terminal arm.
        let executable: Vec<&Statement> = else_body
            .iter()
            .filter(|s| !matches!(s, Statement::Comment(_)))
            .collect();
        if let [inner @ Statement::If { .. }] = executable[..] {
            return self.election_chain(inner);
        }
        self.arm_elections(else_body)
    }

    /// A bare pointer-identity test between two array parameters that Rust's
    /// aliasing rules settle at generation time — `inX == outY`. Deliberately
    /// narrower than [`Self::static_ptr_cond`]: an election chain's conditions are
    /// simple equalities, and admitting `&&`/`||`/`!=` here would let arbitrary
    /// guards license an election.
    fn is_alias_test(&self, cond: &Expr) -> bool {
        let Expr::BinOp(l, BinOp::Eq, r) = cond else {
            return false;
        };
        let (Expr::Var(a), Expr::Var(b)) = (&**l, &**r) else {
            return false;
        };
        let paired = (self.inputs.contains(a.as_str()) && self.outputs.contains(b.as_str()))
            || (self.outputs.contains(a.as_str()) && self.inputs.contains(b.as_str()));
        paired && self.same_buffer(a, b) == Some(false)
    }

    /// The elections this arm performs, or `None` if the arm is not *purely* an
    /// election — a single non-election statement disqualifies the whole chain.
    /// Comments are ignored; an arm that elects nothing returns `None`.
    fn arm_elections(&self, arm: &[Statement]) -> Option<Vec<(String, String)>> {
        let mut elected = Vec::new();
        for stmt in arm {
            match stmt {
                Statement::Comment(_) => {}
                Statement::Assign {
                    target: Expr::Var(local),
                    value: Expr::Var(src),
                    compound: false,
                } if self.locals.contains(local) && self.outputs.contains(src) => {
                    elected.push((local.clone(), src.clone()));
                }
                _ => return None,
            }
        }
        (!elected.is_empty()).then_some(elected)
    }

    /// Evaluate a pointer-identity condition that Rust's parameter aliasing rules
    /// settle at generation time. `None` = not decidable, leave the code alone.
    fn static_ptr_cond(&self, cond: &Expr) -> Option<bool> {
        match cond {
            Expr::BinOp(l, BinOp::Or, r) => {
                match (self.static_ptr_cond(l), self.static_ptr_cond(r)) {
                    (Some(true), _) | (_, Some(true)) => Some(true),
                    (Some(false), Some(false)) => Some(false),
                    _ => None,
                }
            }
            Expr::BinOp(l, BinOp::And, r) => {
                match (self.static_ptr_cond(l), self.static_ptr_cond(r)) {
                    (Some(false), _) | (_, Some(false)) => Some(false),
                    (Some(true), Some(true)) => Some(true),
                    _ => None,
                }
            }
            Expr::Not(inner) => self.static_ptr_cond(inner).map(|v| !v),
            Expr::BinOp(l, op @ (BinOp::Eq | BinOp::NotEq), r) => {
                let (Expr::Var(a), Expr::Var(b)) = (&**l, &**r) else {
                    return None;
                };
                let same = self.same_buffer(a, b)?;
                Some(if matches!(op, BinOp::Eq) { same } else { !same })
            }
            _ => None,
        }
    }

    /// Whether two array *parameters* can be the same buffer. Both names must
    /// already be resolved through the elections in force.
    fn same_buffer(&self, a: &str, b: &str) -> Option<bool> {
        let known = |n: &str| self.inputs.contains(n) || self.outputs.contains(n);
        if !known(a) || !known(b) {
            return None;
        }
        if a == b {
            return Some(true);
        }
        // `&[T]` vs `&mut [T]`, and two distinct `&mut [T]`, can never overlap.
        // Two distinct `&[T]` can — the caller may pass one slice twice — so that
        // pair stays undecided.
        if self.inputs.contains(a) && self.inputs.contains(b) {
            return None;
        }
        Some(false)
    }

    /// Rebuild `stmt` with its nested blocks rewritten under `elections`.
    ///
    /// Exhaustive over `Statement` on purpose — no `_ => stmt` arm. A catch-all
    /// here is the sharpest failure mode this pass has: a body-bearing variant it
    /// skipped would keep the *old* local name inside a loop body while the
    /// election had already replaced the local with an output. That output is
    /// well-typed, so the result compiles cleanly and only fails at run time, on
    /// the first index into a `Vec` that is still empty. The compiler must be the
    /// one to notice a new variant, not a fuzz run.
    fn descend(&self, stmt: Statement, elections: &ElectionMap, outer: &[Statement]) -> Statement {
        let go = |body: &[Statement]| self.block(body, elections, outer);
        match stmt {
            Statement::While { condition, body } => Statement::While { condition, body: go(&body) },
            Statement::DoWhile { condition, body } => Statement::DoWhile { condition, body: go(&body) },
            Statement::For { var, count, body } => Statement::For { var, count, body: go(&body) },
            Statement::If { condition, then_body, else_body, cond_comments } => Statement::If {
                condition,
                then_body: go(&then_body),
                else_body: go(&else_body),
                cond_comments,
            },
            Statement::Switch { expr, cases, default } => Statement::Switch {
                expr,
                cases: cases.into_iter().map(|(label, body)| (label, go(&body))).collect(),
                default: go(&default),
            },
            Statement::ForC { init, condition, update, body } => {
                let init = Box::new(descend_leaf(&init, elections));
                let update = Box::new(descend_leaf(&update, elections));
                Statement::ForC { init, condition, update, body: go(&body) }
            }
            Statement::Block { body } => Statement::Block { body: go(&body) },
            // Leaves: `rename_shallow` has already rewritten every expression
            // these own, and they hold no nested statements.
            stmt @ (Statement::VarDecl { .. }
            | Statement::Assign { .. }
            | Statement::UnrollHint { .. }
            | Statement::Return { .. }
            | Statement::Break
            | Statement::Continue
            | Statement::Expr(_)
            | Statement::CircBuf(_)
            | Statement::Comment(_)) => stmt,
        }
    }
}

/// A `ForC` init/update is a single statement, not a block: rename it in place
/// (it can never hold an election of its own).
fn descend_leaf(stmt: &Statement, elections: &ElectionMap) -> Statement {
    rename_shallow(stmt, elections).0
}

/// True if control can never fall out of the statements that remain in this block —
/// the last executable one is a `return`. When it holds, nothing after the enclosing
/// block can observe a local elected here, so the block-scoped rename is equivalent
/// to C's function-scoped pointer.
fn tail_always_returns<'a, I: Iterator<Item = &'a Statement>>(rest: I) -> bool {
    matches!(
        rest.filter(|s| !matches!(s, Statement::Comment(_))).last(),
        Some(Statement::Return { .. })
    )
}

/// True if `name` is still read or written somewhere in `rest`. An election with
/// no uses left in scope is dead code — `STOCH`/`STOCHF` write theirs on the
/// unreachable aliasing arm — and eliding it would change the generated text
/// without removing any work, so those are left exactly as they are.
fn references_var<'a, I: Iterator<Item = &'a Statement>>(rest: I, name: &str) -> bool {
    let stmts: Vec<Statement> = rest.cloned().collect();
    let mut found = false;
    visit_stmts(&stmts, &mut |stmt| {
        let mut probe = |e: &Expr| {
            if !found {
                found = expr_mentions_var(e, name);
            }
        };
        match stmt {
            Statement::Assign { target, value, .. } => {
                probe(target);
                probe(value);
            }
            Statement::While { condition, .. }
            | Statement::DoWhile { condition, .. }
            | Statement::If { condition, .. }
            | Statement::ForC { condition, .. } => probe(condition),
            Statement::VarDecl { init: Some(e), .. }
            | Statement::Return { value: Some(e) }
            | Statement::For { count: e, .. }
            | Statement::Switch { expr: e, .. }
            | Statement::Expr(e)
            | Statement::CircBuf(CircBuf::Init { size: e, .. }) => probe(e),
            _ => {}
        }
    });
    found
}

/// Whether `name` appears anywhere in `expr`, scalar or indexed.
fn expr_mentions_var(expr: &Expr, name: &str) -> bool {
    let mut hit = false;
    let renamed = ElectionMap::from([(name.to_string(), name.to_string())]);
    walk_rename(expr, &renamed, &mut hit);
    hit
}

/// True if any statement in `rest` assigns the whole of `name` (an indexed write
/// is fine — that is the calculation running in the elected buffer).
fn assigns_whole_var<'a, I: Iterator<Item = &'a Statement>>(rest: I, name: &str) -> bool {
    let mut found = false;
    let stmts: Vec<Statement> = rest.cloned().collect();
    visit_stmts(&stmts, &mut |stmt| {
        if let Statement::Assign { target: Expr::Var(t), .. } = stmt {
            if t == name {
                found = true;
            }
        }
        if let Statement::VarDecl { name: t, .. } = stmt {
            if t == name {
                found = true;
            }
        }
    });
    found
}

/// Rename the expressions a statement owns directly, leaving its nested blocks to
/// the caller. Returns whether anything was rewritten.
///
/// Exhaustive over `Statement` on purpose — no `other => other.clone()` arm, for
/// the same reason as [`ScratchElection::descend`]: a variant whose expressions
/// went un-renamed would keep referring to a local the election has retired, and
/// that miss is invisible until run time.
fn rename_shallow(stmt: &Statement, elections: &ElectionMap) -> (Statement, bool) {
    if elections.is_empty() {
        return (stmt.clone(), false);
    }
    let mut hit = false;
    let mut e = |x: &Expr| {
        let (out, changed) = rename_expr(x, elections);
        hit |= changed;
        out
    };
    let out = match stmt {
        Statement::VarDecl { var_type, name, init } => Statement::VarDecl {
            var_type: var_type.clone(),
            name: name.clone(),
            init: init.as_ref().map(&mut e),
        },
        Statement::Assign { target, value, compound } => Statement::Assign {
            target: e(target),
            value: e(value),
            compound: *compound,
        },
        Statement::While { condition, body } => Statement::While {
            condition: e(condition),
            body: body.clone(),
        },
        Statement::DoWhile { condition, body } => Statement::DoWhile {
            condition: e(condition),
            body: body.clone(),
        },
        Statement::For { var, count, body } => Statement::For {
            var: var.clone(),
            count: e(count),
            body: body.clone(),
        },
        Statement::If { condition, then_body, else_body, cond_comments } => Statement::If {
            condition: e(condition),
            then_body: then_body.clone(),
            else_body: else_body.clone(),
            cond_comments: cond_comments.clone(),
        },
        Statement::Return { value } => Statement::Return {
            value: value.as_ref().map(&mut e),
        },
        Statement::Switch { expr, cases, default } => Statement::Switch {
            expr: e(expr),
            cases: cases.clone(),
            default: default.clone(),
        },
        Statement::ForC { init, condition, update, body } => Statement::ForC {
            init: init.clone(),
            condition: e(condition),
            update: update.clone(),
            body: body.clone(),
        },
        Statement::Expr(expr) => Statement::Expr(e(expr)),
        Statement::CircBuf(CircBuf::Init { id, layout, size }) => {
            Statement::CircBuf(CircBuf::Init {
                id: id.clone(),
                layout: layout.clone(),
                size: e(size),
            })
        }
        // Own no expressions, so there is nothing here to rename. `Block`'s body
        // belongs to `descend`, and the remaining `CircBuf` operations carry only
        // identifiers and layouts.
        Statement::UnrollHint { .. }
        | Statement::Break
        | Statement::Continue
        | Statement::Block { .. }
        | Statement::Comment(_)
        | Statement::CircBuf(
            CircBuf::Prolog { .. }
            | CircBuf::InitLocalOnly { .. }
            | CircBuf::Next { .. }
            | CircBuf::Destroy { .. },
        ) => stmt.clone(),
    };
    (out, hit)
}

/// Substitute elected locals for the output parameters they were elected to.
fn rename_expr(expr: &Expr, elections: &ElectionMap) -> (Expr, bool) {
    let mut hit = false;
    let out = walk_rename(expr, elections, &mut hit);
    (out, hit)
}

fn walk_rename(expr: &Expr, elections: &ElectionMap, hit: &mut bool) -> Expr {
    match expr {
        Expr::Var(name) => match elections.get(name) {
            Some(elected) => {
                *hit = true;
                Expr::Var(elected.clone())
            }
            None => expr.clone(),
        },
        Expr::ArrayAccess(name, idx) => {
            let renamed = match elections.get(name) {
                Some(elected) => {
                    *hit = true;
                    elected.clone()
                }
                None => name.clone(),
            };
            Expr::ArrayAccess(renamed, Box::new(walk_rename(idx, elections, hit)))
        }
        Expr::BinOp(l, op, r) => Expr::BinOp(
            Box::new(walk_rename(l, elections, hit)),
            op.clone(),
            Box::new(walk_rename(r, elections, hit)),
        ),
        Expr::Cast(t, inner) => {
            Expr::Cast(t.clone(), Box::new(walk_rename(inner, elections, hit)))
        }
        Expr::Not(inner) => Expr::Not(Box::new(walk_rename(inner, elections, hit))),
        Expr::BitwiseNot(inner) => Expr::BitwiseNot(Box::new(walk_rename(inner, elections, hit))),
        Expr::FuncCall(name, args) => Expr::FuncCall(
            name.clone(),
            args.iter().map(|a| walk_rename(a, elections, hit)).collect(),
        ),
        Expr::AddressOf(inner) => Expr::AddressOf(Box::new(walk_rename(inner, elections, hit))),
        Expr::PostIncrement(inner) => {
            Expr::PostIncrement(Box::new(walk_rename(inner, elections, hit)))
        }
        Expr::PostDecrement(inner) => {
            Expr::PostDecrement(Box::new(walk_rename(inner, elections, hit)))
        }
        Expr::PreIncrement(inner) => {
            Expr::PreIncrement(Box::new(walk_rename(inner, elections, hit)))
        }
        Expr::PreDecrement(inner) => {
            Expr::PreDecrement(Box::new(walk_rename(inner, elections, hit)))
        }
        Expr::Ternary(c, t, f) => Expr::Ternary(
            Box::new(walk_rename(c, elections, hit)),
            Box::new(walk_rename(t, elections, hit)),
            Box::new(walk_rename(f, elections, hit)),
        ),
        Expr::Literal(_) | Expr::IntLiteral(_) | Expr::PointerDeref(_) => expr.clone(),
    }
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
/// Strip the stream-state field prefix so name-keyed inference helpers see the
/// batch-local name (`sp.optInTimePeriod` types exactly like `optInTimePeriod`).
/// A no-op for every batch name (none contain a dot).
pub(crate) fn strip_state_prefix(name: &str) -> &str {
    name.strip_prefix("sp.").unwrap_or(name)
}

fn is_int_array_var(name: &str) -> bool {
    matches!(strip_state_prefix(name),
        "periods" | "usedFlag" | "sortedPeriods"
    )
}

/// Does this expression *render* as `usize`?
///
/// Not "is it i32 in the C" — issue #158's compound arm has to match what the
/// expression emitter actually produced. `(int)d` is `expr_is_i32_typed`, but
/// [`RustExpr::cast`] renders it `d as usize` when `d` is already usize; and a
/// `usize ⊕ i32` BinOp renders usize because the BinOp handler casts the i32
/// side up. Both reach a compound assignment as `usize` text.
fn compound_rhs_renders_usize(expr: &Expr, ctx: &RustRenderCtx) -> bool {
    expr_is_known_usize_ctx(expr, ctx)
        || expr_binop_renders_as_usize(expr, ctx)
        || expr_renders_as_usize_despite_i32(expr, ctx)
}

/// Does this expression's VALUE carry `startIdx`/`endIdx` — the caller-supplied
/// bar range, which is `usize` and must never be truncated to `i32`?
///
/// Subscript positions do not count: `inPeriods[startIdx + i]` produces an
/// array element, not an index, and MAVP casts exactly that to i32.
fn expr_mentions_index_domain(expr: &Expr) -> bool {
    match expr {
        Expr::Var(n) | Expr::PointerDeref(n) => {
            matches!(strip_state_prefix(n), "startIdx" | "endIdx")
        }
        Expr::BinOp(l, _, r) => {
            expr_mentions_index_domain(l) || expr_mentions_index_domain(r)
        }
        Expr::Ternary(_, a, b) => {
            expr_mentions_index_domain(a) || expr_mentions_index_domain(b)
        }
        Expr::Cast(_, inner)
        | Expr::PostIncrement(inner)
        | Expr::PostDecrement(inner)
        | Expr::PreIncrement(inner)
        | Expr::PreDecrement(inner) => expr_mentions_index_domain(inner),
        // Everything else, including `ArrayAccess` — its subscript is an index
        // but its value is an element, so the walk deliberately stops here.
        _ => false,
    }
}

/// The Rust scalar type a named assignment target renders as.
#[derive(Clone, Copy, PartialEq, Eq, Debug)]
enum ScalarTy {
    F64,
    Usize,
    I32,
    /// An `enum:` parameter or a local derived from one. Assignments between
    /// two of these need no cast in either direction.
    Enum,
}

/// Classify a scalar assignment target POSITIVELY — issue #158.
///
/// Order is the point. The declared IR type wins over every naming heuristic:
/// `real_vars` / `index_vars` come straight from the `VarDecl`s via
/// [`collect_var_types`], and `sentinel_vars` from [`collect_signed_int_vars`],
/// so a local is classified as whatever the declaration emitter declared it.
/// The heuristics below them only cover names no `VarDecl` describes — the
/// function's own parameters, helper-inlined temporaries, and the streaming
/// tier's `sp.`-qualified state fields.
fn scalar_target_ty(
    name: &str,
    ctx: &RustRenderCtx,
    opt_real_params: &[String],
    helpers: &HelperRegistry,
) -> ScalarTy {
    // 1. Declared type. Enum first: such a local is filed under the `int` its
    //    C declaration spells, so a later set would otherwise claim it.
    if ctx.enum_vars.contains_key(strip_state_prefix(name)) {
        return ScalarTy::Enum;
    }
    if ctx.real_vars.contains(name) {
        return ScalarTy::F64;
    }
    if ctx.sentinel_vars.contains(name) {
        return ScalarTy::I32;
    }
    if ctx.index_vars.contains(name) {
        return ScalarTy::Usize;
    }
    // 2. Names carried by the signature rather than a declaration. The YAML
    //    Real params go FIRST: `is_i32_opt_in_param` is a negative allowlist
    //    ("starts with optIn and is not one of these known Real names"), so it
    //    calls an unlisted Real param i32.
    let base = strip_state_prefix(name);
    if opt_real_params.iter().any(|p| p == base) {
        return ScalarTy::F64;
    }
    if is_i32_opt_in_param(name) || base.ends_with("_avgPeriod") || base.ends_with("_rangeType") {
        return ScalarTy::I32;
    }
    // 3. A helper-inlined temporary: the inliner renames the helper's own
    //    local `range` to `range_0`, so it has no VarDecl in THIS body. The
    //    helper declared it — that is still a declaration, just one file over.
    if let Some(t) = helper_local_ty(base, helpers) {
        return t;
    }
    // 4. Last resort, and only ever to say "index", never to say "Real".
    if is_likely_index_var(name) {
        return ScalarTy::Usize;
    }
    // 5. Nothing knows this name. Keep the historical f64 answer rather than
    //    refusing to generate: every wrong verdict here is a *type* error the
    //    crate build catches, never a silently wrong number, so a hard failure
    //    would trade a loud compile error for a louder one while being able to
    //    block a build over a name the classifier simply has not met. What
    //    issue #158 required — that a DECLARED integer can never be called
    //    Real by its name — is settled by steps 1-3 above.
    ScalarTy::F64
}

/// The declared type of a helper's own local or parameter, matched through the
/// inliner's `<name>_<n>` renaming.
///
/// All matches must agree: `HelperRegistry::iter` is `HashMap` order, so a name
/// two helpers declare differently must not resolve by whichever came first.
fn helper_local_ty(base: &str, helpers: &HelperRegistry) -> Option<ScalarTy> {
    // Strip the inliner's numeric suffix, if any.
    let stem = base
        .rsplit_once('_')
        .filter(|(_, n)| !n.is_empty() && n.bytes().all(|b| b.is_ascii_digit()))
        .map_or(base, |(s, _)| s);
    let mut found: Option<ScalarTy> = None;
    for h in helpers.iter() {
        let declared = h
            .params
            .iter()
            .find(|p| p.name == stem)
            .map(|p| &p.var_type)
            .or_else(|| helper_decl_ty(&h.body, stem));
        let Some(t) = declared else { continue };
        let scalar = match t {
            VarType::Real => ScalarTy::F64,
            VarType::Integer | VarType::Index => ScalarTy::Usize,
            // Arrays/pointers/RetCode are never scalar compound targets.
            _ => return None,
        };
        match found {
            Some(prev) if prev != scalar => return None,
            _ => found = Some(scalar),
        }
    }
    found
}

fn helper_decl_ty<'a>(body: &'a [Statement], name: &str) -> Option<&'a VarType> {
    for stmt in body {
        match stmt {
            Statement::VarDecl { var_type, name: n, .. } if n == name => return Some(var_type),
            Statement::If { then_body, else_body, .. } => {
                if let Some(t) = helper_decl_ty(then_body, name) {
                    return Some(t);
                }
                if let Some(t) = helper_decl_ty(else_body, name) {
                    return Some(t);
                }
            }
            Statement::While { body: b, .. }
            | Statement::DoWhile { body: b, .. }
            | Statement::For { body: b, .. }
            | Statement::ForC { body: b, .. }
            | Statement::Block { body: b } => {
                if let Some(t) = helper_decl_ty(b, name) {
                    return Some(t);
                }
            }
            _ => {}
        }
    }
    None
}

/// Helper functions that return int (not double/T).
/// These must NOT be treated as float-typed by `expr_is_float_typed`.
/// Check if a variable name is likely an index/counter (usize) rather than a Real (T) variable.
fn is_likely_index_var(name: &str) -> bool {
    let name = strip_state_prefix(name);
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
                | "IS_FINITE"
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
