//! Rust stream emitter — the Rust twin of `backends/c_stream.rs`.
//!
//! For every YAML-declared streamable function this appends a
//! `/**** Streaming API *****/` section to the generated per-function Rust
//! file: an opaque `#[derive(Clone)]` handle (`<Pascal>Stream { core, state }`),
//! a private state struct mirroring the C stream struct field-for-field, a
//! `<snake>_step_internal` transition method on `Core` (so batch rendering
//! conventions — `self.candle_settings`, `self.compatibility`, lookback calls —
//! work verbatim), a `pub(crate) <snake>_open_internal(.., startIdx, ..)`
//! composition seam, the public `<snake>_open` / `<snake>_open_and_fill`
//! constructors, and `update`/`peek` on the handle.
//!
//! Bit-exactness argument (same as C): `open` transcribes the ENTIRE batch
//! body through the same statement renderer as the batch backend, then
//! captures the still-live locals into the handle; the per-bar step is
//! `streaming::build_transition` rendered through the same walkers. No
//! expression text is hand-built outside the shared renderers.
//!
//! Deliberate simplifications vs the C emitter (see design spec):
//! - `peek(&self)` = `self.clone()` + `update` on the throwaway clone (the
//!   design doc's stated cost model). No mirror buffers, no `peekMode`.
//! - Drop replaces Close; RAII replaces every OOM-unwind ladder.
//! - `historyLen` is the input slice length; multi-input opens require
//!   non-empty, equal-length slices (`Err(BadParam)` otherwise).

use std::cell::Cell;
use std::collections::{HashMap, HashSet};
use std::fmt::Write;

use crate::helper_registry::HelperRegistry;
use crate::ir::{CircBuf, EnumDef, Expr, FuncDef, ParamType, Statement, VarType};
use crate::registry::Registry;
use crate::streaming::{self, StreamModel, StreamPlan};

use super::rust_lang::{
    collect_for_loop_vars, collect_sentinel_vars, collect_signed_int_vars, collect_var_types,
    emit_circbuf_prolog_rust, expr_is_untyped_integer, gen_opt_param_validation_with,
    render_expr, render_hoisted_blocks, render_statement, to_pascal_case, RustRenderCtx,
};
use crate::helper_registry::hoist_block_helpers;

/// Marker heading the generated stream section (tests slice on it; mirrors C).
pub const SECTION_MARKER: &str = "/**** Streaming API *****/";

/// Whether a Rust stream section is emitted for this function (all six
/// StreamPlan tiers are implemented, so this is simply "declared streamable").
pub fn emits_stream(func: &FuncDef, lookup: &dyn streaming::CalleeLookup) -> bool {
    if !func.streaming {
        return false;
    }
    // Every StreamPlan tier now emits a Rust stream; a plan failure would have
    // failed `generate`'s analyzability gate long before this predicate runs.
    streaming::validate_streamable(func, lookup).is_ok()
}

/// Public handle type name: shared `to_pascal_case` + "Stream" (one naming
/// authority with `FuncUnstId` — `MinusDIStream`, `HtDcPeriodStream`).
pub fn stream_type_name(func: &FuncDef) -> String {
    format!("{}Stream", to_pascal_case(&func.name))
}

fn state_type_name(func: &FuncDef) -> String {
    format!("{}StreamState", to_pascal_case(&func.name))
}

fn snake(func: &FuncDef) -> String {
    func.name.to_lowercase()
}

fn out_is_int(func: &FuncDef, name: &str) -> bool {
    func.outputs
        .iter()
        .any(|o| o.name == name && o.param_type == ParamType::Integer)
}

/// `f64` / `i32` element type of an output.
fn out_rust_type(func: &FuncDef, name: &str) -> &'static str {
    if out_is_int(func, name) {
        "i32"
    } else {
        "f64"
    }
}

/// Update/peek/open return value type: single output flattened, multi-output a
/// tuple in batch output order.
fn value_type(func: &FuncDef) -> String {
    let types: Vec<&str> = func
        .outputs
        .iter()
        .map(|o| out_rust_type(func, &o.name))
        .collect();
    if types.len() == 1 {
        types[0].to_string()
    } else {
        format!("({})", types.join(", "))
    }
}

/// Rust type of an optional parameter (batch convention: enums are `i32`).
fn opt_param_rust_type(p: &ParamType) -> &'static str {
    match p {
        ParamType::Real => "f64",
        ParamType::Integer | ParamType::Enum(_) => "i32",
        ParamType::Price(_) => unreachable!("price optional params do not exist"),
    }
}

/// Rust type of a private extra param (EMA's k factor): C type string → Rust.
fn extra_param_rust_type(c_type: &str) -> &'static str {
    match c_type {
        "double" => "f64",
        "int" => "i32",
        other => panic!("unsupported private extra param type: {other}"),
    }
}

// ---------------------------------------------------------------------------
// NameMap: state through `sp.`, bars as same-named scalars, outputs as
// `(*out)` writes against `&mut` step params.
// ---------------------------------------------------------------------------

struct RustStreamNames;

impl streaming::NameMap for RustStreamNames {
    fn state(&self, name: &str) -> String {
        format!("sp.{name}")
    }
    fn bar(&self, array: &str) -> String {
        array.to_string()
    }
    fn output(&self, name: &str) -> Expr {
        Expr::PointerDeref(name.to_string())
    }
    fn ring_buf(&self, var: &str, array: &str) -> String {
        format!("sp.ring_{var}_{array}")
    }
    fn ring_pos(&self, var: &str) -> String {
        format!("sp.ringPos_{var}")
    }
    fn ring_lag(&self, var: &str) -> String {
        format!("sp.ringLag_{var}")
    }
    fn ring_cap(&self, var: &str) -> String {
        format!("sp.ringCap_{var}")
    }
    fn win_buf(&self, var: &str, array: &str) -> String {
        format!("sp.win_{var}_{array}")
    }
    fn win_pos(&self, var: &str) -> String {
        format!("sp.winPos_{var}")
    }
    fn win_cap(&self, var: &str) -> String {
        format!("sp.winCap_{var}")
    }
    fn circ_buf(&self, storage: &str) -> String {
        format!("sp.cb_{storage}")
    }
    fn extrema_buf(&self, array: &str) -> String {
        format!("sp.x_{array}")
    }
    fn extrema_cap(&self) -> String {
        "sp.xCap".to_string()
    }
}

// ---------------------------------------------------------------------------
// Typing oracle: the batch type-inference verdicts for every local, reused for
// state-struct field types AND the render contexts (so cast insertion matches
// batch decisions exactly). Extrema/AIA override: cursor/trailing/index fields
// (and xCap) are forced `i32` — C's `int` — because the 2^30 rebase arithmetic
// does not exist in batch bodies for the inference to type (usize subtraction
// there could underflow in debug builds; index-only, zero FP impact).
// ---------------------------------------------------------------------------

struct Typing {
    ctx: RustRenderCtx,
    /// Names forced to `i32` in the state struct (extrema machinery).
    extrema_i32: HashSet<String>,
}

fn build_typing(func: &FuncDef, model: &StreamModel) -> Typing {
    build_typing_from(func, model.body, &[model])
}

/// [`build_typing`] over an explicit body region (dual-mode/fast-path-skip:
/// the concatenated `prologue ++ arm(s) ++ epilogue`, so the inference sees
/// the same statement population batch typing saw) and every arm model (for
/// the extrema override union).
fn build_typing_from(func: &FuncDef, body: &[Statement], models: &[&StreamModel]) -> Typing {
    let mut index_vars = HashSet::new();
    let mut real_vars = HashSet::new();
    let mut vec_vars = HashSet::new();
    let mut real_array_vars = HashSet::new();
    let mut int_vec_vars = HashSet::new();
    collect_var_types(
        body,
        &mut index_vars,
        &mut real_vars,
        &mut vec_vars,
        &mut real_array_vars,
        &mut int_vec_vars,
    );
    index_vars.insert("startIdx".to_string());
    index_vars.insert("endIdx".to_string());
    index_vars.insert("historyLen".to_string());
    let mut sentinel_vars = HashSet::new();
    collect_sentinel_vars(body, &mut sentinel_vars);
    collect_signed_int_vars(body, &index_vars, &mut sentinel_vars);
    for sv in &sentinel_vars {
        index_vars.remove(sv);
    }
    let int_output_names: HashSet<String> = func
        .outputs
        .iter()
        .filter(|o| o.param_type == ParamType::Integer)
        .map(|o| o.name.clone())
        .collect();

    let mut extrema_i32 = HashSet::new();
    for model in models {
        if let Some(ex) = model.extrema() {
            extrema_i32.insert(model.cursor.clone());
            extrema_i32.insert(ex.trailing.clone());
            for v in &ex.index_vars {
                extrema_i32.insert(v.clone());
            }
        }
    }

    Typing {
        ctx: RustRenderCtx {
            bounds_asserts: false,
            index_vars,
            real_vars,
            vec_vars,
            real_array_vars,
            int_output_names,
            int_vec_vars,
            is_lookback: false,
            sentinel_vars,
            result_error_returns: true,
        },
        extrema_i32,
    }
}

/// Rust type + default of a state-struct field / hoisted local, honoring the
/// sentinel verdicts — and, for STATE fields only, the extrema-i32 override
/// (the transcribed open body keeps pure batch typing; the capture epilogue
/// casts at the struct literal).
fn field_type_and_default(
    typing: &Typing,
    name: &str,
    ty: &VarType,
    state: bool,
) -> (String, String) {
    let i32ish = typing.ctx.sentinel_vars.contains(name)
        || (state && typing.extrema_i32.contains(name));
    match ty {
        VarType::Real => ("f64".into(), "0.0_f64".into()),
        VarType::Integer | VarType::Index => {
            if i32ish {
                ("i32".into(), "0_i32".into())
            } else {
                ("usize".into(), "0_usize".into())
            }
        }
        VarType::RetCodeType => ("RetCode".into(), "RetCode::Success".into()),
        VarType::RealPointer => ("Vec<f64>".into(), "Vec::new()".into()),
        VarType::IntPointer => ("Vec<i32>".into(), "Vec::new()".into()),
        VarType::RealArray(size) => (
            format!("[f64; {size} as usize]"),
            format!("[0.0_f64; {size} as usize]"),
        ),
        VarType::IntArray(size) => (
            format!("[i32; {size} as usize]"),
            format!("[0_i32; {size} as usize]"),
        ),
    }
}

/// The full ordered field list of the state struct: (name, rust_type,
/// identity-path default). Order mirrors the C stream struct.
fn state_fields(func: &FuncDef, model: &StreamModel, typing: &Typing) -> Vec<(String, String, String)> {
    state_fields_from(func, model, typing, &model.state)
}

/// [`state_fields`] with the carried-scalar set supplied by the caller
/// (dual-mode: the type-checked union of both modes' scalars; passing `&[]`
/// yields the params + non-scalar tail, used for the dual-mode shape assert).
fn state_fields_from(
    func: &FuncDef,
    model: &StreamModel,
    typing: &Typing,
    scalars: &[(String, VarType)],
) -> Vec<(String, String, String)> {
    let mut fields: Vec<(String, String, String)> = Vec::new();
    for p in &func.optional_inputs {
        // Params are always captured (identity path included).
        fields.push((
            p.name.clone(),
            opt_param_rust_type(&p.param_type).to_string(),
            p.name.clone(),
        ));
    }
    for (name, c_type) in &func.private_extra_params {
        fields.push((
            name.clone(),
            extra_param_rust_type(c_type).to_string(),
            name.clone(),
        ));
    }
    for (name, ty) in scalars {
        let (rty, default) = field_type_and_default(typing, name, ty, true);
        fields.push((name.clone(), rty, default));
    }
    for name in &model.out_feedback {
        let t = out_rust_type(func, name);
        let d = if t == "i32" { "0_i32" } else { "0.0_f64" };
        fields.push((format!("lastOut_{name}"), t.to_string(), d.to_string()));
    }
    for lag in &model.lags {
        for k in 1..=lag.depth {
            fields.push((
                StreamModel::lag_field(&lag.array, k),
                "f64".to_string(),
                "0.0_f64".to_string(),
            ));
        }
    }
    for ring in model.rings() {
        let v = &ring.var;
        fields.push((format!("ringPos_{v}"), "usize".into(), "0_usize".into()));
        // Identity path: cap 0 (back==0) / back+1 (back>0) with 1-slot buffers,
        // keeping the transition's cap-0 guard and any read well-defined.
        let id_cap = if ring.back > 0 {
            format!("{}_usize", ring.back + 1)
        } else {
            "0_usize".into()
        };
        fields.push((format!("ringCap_{v}"), "usize".into(), id_cap));
        if ring.back > 0 {
            fields.push((format!("ringLag_{v}"), "usize".into(), "0_usize".into()));
        }
        for arr in &ring.arrays {
            let id_len = if ring.back > 0 {
                format!("{}", ring.back + 1)
            } else {
                "1".into()
            };
            fields.push((
                format!("ring_{v}_{arr}"),
                "Vec<f64>".into(),
                format!("vec![0.0_f64; {id_len}]"),
            ));
        }
    }
    for win in model.windows() {
        let v = &win.var;
        fields.push((format!("winPos_{v}"), "usize".into(), "0_usize".into()));
        fields.push((format!("winCap_{v}"), "usize".into(), "1_usize".into()));
        for arr in &win.arrays {
            fields.push((
                format!("win_{v}_{arr}"),
                "Vec<f64>".into(),
                "vec![0.0_f64; 1]".into(),
            ));
        }
    }
    for circ in model.circs() {
        fields.push((format!("cbSize_{}", circ.id), "usize".into(), "0_usize".into()));
        for (storage, ty) in streaming::circ_storages(circ) {
            let (t, d) = if matches!(ty, VarType::Integer) {
                ("Vec<i32>", "Vec::new()")
            } else {
                ("Vec<f64>", "Vec::new()")
            };
            fields.push((format!("cb_{storage}"), t.into(), d.into()));
        }
    }
    if let Some(ex) = model.extrema() {
        fields.push(("xCap".into(), "i32".into(), "1_i32".into()));
        for arr in &ex.arrays {
            fields.push((
                format!("x_{arr}"),
                "Vec<f64>".into(),
                "vec![0.0_f64; 1]".into(),
            ));
        }
    }
    fields
}

// ---------------------------------------------------------------------------
// Entry point
// ---------------------------------------------------------------------------

/// Generate the whole stream section for one function's `.rs` file.
///
/// Panics on analysis failure: the declared-tier gate in `generate` validates
/// first, so a failure here means the gate was bypassed — fail loudly.
#[allow(clippy::implicit_hasher)]
pub fn generate(
    func: &FuncDef,
    enums: &HashMap<String, EnumDef>,
    registry: &Registry,
    helpers: &HelperRegistry,
) -> String {
    assert!(
        func.streaming,
        "rust_stream::generate called without a streaming declaration"
    );
    let plan = streaming::validate_streamable(func, registry)
        .unwrap_or_else(|e| panic!("streaming gate: {e}"));

    let counter = Cell::new(0usize);
    let mut o = String::new();

    let _ = writeln!(o, "{SECTION_MARKER}\n");
    match &plan {
        StreamPlan::Loop(model) => {
            emit_loop(&mut o, func, model, enums, registry, helpers, &counter);
        }
        StreamPlan::DualMode(dmp) => {
            emit_dual_mode(&mut o, func, dmp, enums, registry, helpers, &counter);
        }
        StreamPlan::FastPathSkip(fp) => {
            emit_fastpath_skip(&mut o, func, fp, enums, registry, helpers, &counter);
        }
        StreamPlan::Dispatch(dp) => {
            emit_dispatch(&mut o, func, dp, enums, registry, helpers, &counter);
        }
        StreamPlan::PeriodBank(pb) => {
            emit_period_bank(&mut o, func, pb, registry, helpers);
        }
        StreamPlan::Composed(cp) => {
            emit_composed(&mut o, func, cp, enums, registry, helpers, &counter);
        }
    }

    o
}

/// The lint preamble shared by every tier's generated `impl Core` block.
const IMPL_ALLOW: &str = "#[allow(non_snake_case)]\n#[allow(unused_variables)]\n#[allow(dead_code)]\n#[allow(unused_mut)]\n#[allow(unused_assignments)]\n#[allow(unused_parens)]\n";

#[allow(clippy::too_many_arguments)]
fn emit_loop(
    o: &mut String,
    func: &FuncDef,
    model: &StreamModel,
    enums: &HashMap<String, EnumDef>,
    registry: &Registry,
    helpers: &HelperRegistry,
    counter: &Cell<usize>,
) {
    let typing = build_typing(func, model);
    emit_handle_and_state_structs(o, func, model, &typing);

    let _ = writeln!(o, "{IMPL_ALLOW}impl Core {{");
    emit_step(o, func, model, &typing, enums, registry, helpers, counter);
    emit_open_internal(o, func, model, &typing, model.body, enums, registry, helpers, counter, OutMode::Scalar);
    emit_open_wrapper(o, func);
    emit_open_internal(o, func, model, &typing, model.body, enums, registry, helpers, counter, OutMode::Fill);
    let _ = writeln!(o, "}}\n");

    emit_update_and_peek(o, func);
    emit_trait_pin(o, func);
}

/// Output mode for the open family (mirrors `c_stream::OutMode`).
#[derive(Clone, Copy, PartialEq, Eq)]
enum OutMode {
    Scalar,
    Fill,
}

// ---------------------------------------------------------------------------
// Structs
// ---------------------------------------------------------------------------

fn emit_handle_and_state_structs(
    o: &mut String,
    func: &FuncDef,
    model: &StreamModel,
    typing: &Typing,
) {
    emit_handle_struct(o, func);
    emit_state_struct_from(o, func, &state_fields(func, model, typing));
}

/// The public opaque handle struct (identical for every tier).
fn emit_handle_struct(o: &mut String, func: &FuncDef) {
    let handle = stream_type_name(func);
    let state = state_type_name(func);
    let sn = snake(func);
    let n = func.name.to_uppercase();
    let _ = writeln!(
        o,
        "/// Live {n} stream: one value per closed bar, bit-identical to [`Core::{sn}`]\n\
         /// over the same series. Open with [`Core::{sn}_open`]; dropping the handle\n\
         /// closes the stream. Cloning it forks an independent stream.\n\
         #[must_use = \"a stream does nothing unless updated; dropping it closes the stream\"]\n\
         #[derive(Debug, Clone)]\n\
         #[doc(alias = \"TA_{n}_Stream\")]\n\
         pub struct {handle} {{\n    core: Core,\n    state: {state},\n}}\n"
    );
}

/// The private state struct from a prebuilt (name, rust_type, default) list.
fn emit_state_struct_from(o: &mut String, func: &FuncDef, fields: &[(String, String, String)]) {
    let state = state_type_name(func);
    let _ = writeln!(o, "#[derive(Debug, Clone)]\n#[allow(non_snake_case, dead_code)]\nstruct {state} {{");
    for (name, rty, _) in fields {
        let _ = writeln!(o, "    {name}: {rty},");
    }
    let _ = writeln!(o, "}}\n");
}

// ---------------------------------------------------------------------------
// StepInternal
// ---------------------------------------------------------------------------

/// Step context: the open-body typing plus every state name aliased under its
/// `sp.` field path, plus the per-bar input scalars as reals.
fn build_step_ctx(func: &FuncDef, model: &StreamModel, typing: &Typing) -> RustRenderCtx {
    let mut ctx = typing.ctx.clone();
    for bar in streaming::input_array_names(func) {
        ctx.real_vars.insert(bar);
    }
    // Alias every state field name under `sp.` in the same set its bare name
    // occupies, so the cast-inference matches batch decisions on field reads.
    let alias = |set: &mut HashSet<String>| {
        let names: Vec<String> = set.iter().cloned().collect();
        for n in names {
            set.insert(format!("sp.{n}"));
        }
    };
    // Extrema override: cursor machinery is i32 in the handle.
    for n in &typing.extrema_i32 {
        ctx.index_vars.remove(n);
        ctx.sentinel_vars.insert(n.clone());
    }
    // Emitter-owned fields. The f64 buffers also register as real arrays so
    // element reads type as float in the shared inference (batch's `in[i]`
    // heuristics don't recognize the ring/window names).
    for ring in model.rings() {
        let v = &ring.var;
        ctx.index_vars.insert(format!("ringPos_{v}"));
        ctx.index_vars.insert(format!("ringCap_{v}"));
        ctx.index_vars.insert(format!("ringLag_{v}"));
        for arr in &ring.arrays {
            ctx.vec_vars.insert(format!("ring_{v}_{arr}"));
            ctx.real_array_vars.insert(format!("ring_{v}_{arr}"));
        }
    }
    for win in model.windows() {
        let v = &win.var;
        ctx.index_vars.insert(format!("winPos_{v}"));
        ctx.index_vars.insert(format!("winCap_{v}"));
        for arr in &win.arrays {
            ctx.vec_vars.insert(format!("win_{v}_{arr}"));
            ctx.real_array_vars.insert(format!("win_{v}_{arr}"));
        }
    }
    for circ in model.circs() {
        ctx.index_vars.insert(format!("cbSize_{}", circ.id));
        for (storage, ty) in streaming::circ_storages(circ) {
            ctx.vec_vars.insert(format!("cb_{storage}"));
            if matches!(ty, VarType::Integer) {
                ctx.int_vec_vars.insert(format!("cb_{storage}"));
            } else {
                ctx.real_array_vars.insert(format!("cb_{storage}"));
            }
        }
    }
    if let Some(ex) = model.extrema() {
        ctx.sentinel_vars.insert("xCap".to_string());
        for arr in &ex.arrays {
            ctx.vec_vars.insert(format!("x_{arr}"));
            ctx.real_array_vars.insert(format!("x_{arr}"));
        }
    }
    for name in &model.out_feedback {
        let f = format!("lastOut_{name}");
        if out_is_int(func, name) {
            ctx.sentinel_vars.insert(f);
        } else {
            ctx.real_vars.insert(f);
        }
    }
    for lag in &model.lags {
        for k in 1..=lag.depth {
            ctx.real_vars.insert(StreamModel::lag_field(&lag.array, k));
        }
    }
    alias(&mut ctx.index_vars);
    alias(&mut ctx.real_vars);
    alias(&mut ctx.vec_vars);
    alias(&mut ctx.real_array_vars);
    alias(&mut ctx.int_vec_vars);
    alias(&mut ctx.sentinel_vars);
    ctx
}

/// `fn <snake>_step_internal(&self, sp: &mut State, <bars>, <&mut outs>)`.
#[allow(clippy::too_many_arguments)]
fn emit_step(
    o: &mut String,
    func: &FuncDef,
    model: &StreamModel,
    typing: &Typing,
    enums: &HashMap<String, EnumDef>,
    registry: &Registry,
    helpers: &HelperRegistry,
    counter: &Cell<usize>,
) {
    emit_step_sig(o, func);
    let ctx = build_step_ctx(func, model, typing);
    emit_step_body(o, func, model, typing, &ctx, enums, registry, helpers, counter, 8);
    let _ = writeln!(o, "    }}\n");
}

/// The step signature line, shared by every tier (dispatch/period-bank steps
/// hand-roll their bodies but keep the identical surface).
fn emit_step_sig(o: &mut String, func: &FuncDef) {
    let sn = snake(func);
    let state = state_type_name(func);
    let mut params = String::new();
    for bar in streaming::input_array_names(func) {
        let _ = write!(params, ", {bar}: f64");
    }
    for out in &func.outputs {
        let _ = write!(params, ", {}: &mut {}", out.name, out_rust_type(func, &out.name));
    }
    let _ = writeln!(o, "    fn {sn}_step_internal(&self, sp: &mut {state}{params}) {{");
}

/// One model's per-bar step body at a given indent: temp decls, the extrema
/// rebase, candle unpacking, and the rendered transition. Called once by the
/// loop tier (indent 8) and once per arm by the dual-mode step (indent 12).
#[allow(clippy::too_many_arguments)]
fn emit_step_body(
    o: &mut String,
    func: &FuncDef,
    model: &StreamModel,
    typing: &Typing,
    ctx: &RustRenderCtx,
    enums: &HashMap<String, EnumDef>,
    registry: &Registry,
    helpers: &HelperRegistry,
    counter: &Cell<usize>,
    indent: usize,
) {
    let pad = " ".repeat(indent);
    for (name, ty) in &model.temps {
        let (rty, default) = field_type_and_default(typing, name, ty, false);
        let _ = writeln!(o, "{pad}let mut {name}: {rty} = {default};");
    }
    emit_extrema_rebase(o, model, indent);

    let transition = streaming::build_transition(model, &RustStreamNames)
        .unwrap_or_else(|e| panic!("streaming transition: {e}"));
    // C's `*outReal = 0;` must type as f64: float integer literals written to
    // a REAL output become float literals (the renderer's ArrayAccess-target
    // wrap does this for batch writes; the step's `(*out)` deref needs it here).
    let real_outs: HashSet<String> = func
        .outputs
        .iter()
        .filter(|out| out.param_type != ParamType::Integer)
        .map(|out| out.name.clone())
        .collect();
    let transition = streaming::rewrite_stmts(&transition, &|e| e, &|s| match s {
        Statement::Assign {
            target: Expr::PointerDeref(nm),
            value: Expr::IntLiteral(n),
            compound: false,
        } if real_outs.contains(&nm) =>
        {
            #[allow(clippy::cast_precision_loss)]
            Some(Statement::Assign {
                target: Expr::PointerDeref(nm),
                value: Expr::Literal(n as f64),
                compound: false,
            })
        }
        other => Some(other),
    });
    let output_names: Vec<String> = func.outputs.iter().map(|out| out.name.clone()).collect();
    let opt_real_params: Vec<String> = func
        .optional_inputs
        .iter()
        .filter(|p| p.param_type == ParamType::Real)
        .map(|p| p.name.clone())
        .collect();
    let var_inits: HashMap<String, &Expr> = HashMap::new();
    let mut body = String::new();
    for s in &transition {
        body.push_str(&render_statement(
            s, indent, ctx, &[], &var_inits, &output_names, &opt_real_params, enums, registry,
            helpers, counter,
        ));
    }
    // Candle settings are read where batch reads them (from the immutable Core
    // snapshot in the handle — `self` here is that Core).
    let step_settings = crate::candle_settings::detect_candle_settings(&model.steady_stmts);
    if !step_settings.is_empty() {
        o.push_str(&crate::candle_settings::emit_rust_unpacking(&step_settings, indent));
    }
    o.push_str(&body);
}

/// Extrema automatons carry batch-absolute i32 indices; rebase them by a
/// multiple of the ring capacity long before i32::MAX (mirrors C verbatim —
/// index differences and `% cap` residues are invariant).
fn emit_extrema_rebase(o: &mut String, model: &StreamModel, indent: usize) {
    if let Some(ex) = model.extrema() {
        let pad = " ".repeat(indent);
        let inner = " ".repeat(indent + 4);
        let mut vars: Vec<String> = vec![model.cursor.clone(), ex.trailing.clone()];
        vars.extend(ex.index_vars.iter().cloned());
        let _ = writeln!(o, "{pad}if sp.{} >= 1073741824 {{", model.cursor);
        let _ = writeln!(
            o,
            "{inner}let rebaseShift: i32 = (sp.{} / sp.xCap) * sp.xCap;",
            ex.trailing
        );
        for v in &vars {
            let _ = writeln!(o, "{inner}sp.{v} -= rebaseShift;");
        }
        let _ = writeln!(o, "{pad}}}");
    }
}

// ---------------------------------------------------------------------------
// Open transcription
// ---------------------------------------------------------------------------

/// Map a batch return-code expression to the stream tier's `Result` shape.
/// Any early SUCCESS return maps to `Err(BadParam)` (strict min-history — the
/// no-data guard AND the Metastock seed-boundary return); error codes map to
/// their `Err(...)` equivalents.
fn map_return_code(v: &str) -> String {
    match v {
        "SUCCESS" | "TA_SUCCESS" | "BAD_PARAM" | "TA_BAD_PARAM" => {
            "Err(RetCode::BadParam)".to_string()
        }
        "ALLOC_ERR" | "TA_ALLOC_ERR" => "Err(RetCode::AllocErr)".to_string(),
        "INTERNAL_ERROR" | "TA_INTERNAL_ERROR" => "Err(RetCode::InternalError)".to_string(),
        "OUT_OF_RANGE_START_INDEX" | "TA_OUT_OF_RANGE_START_INDEX" => {
            "Err(RetCode::OutOfRangeStartIndex)".to_string()
        }
        "OUT_OF_RANGE_END_INDEX" | "TA_OUT_OF_RANGE_END_INDEX" => {
            "Err(RetCode::OutOfRangeEndIndex)".to_string()
        }
        // A RetCode-typed local (`if retCode != SUCCESS { return retCode; }`
        // error propagation after an internal cross-call, e.g. SAR's MINUS_DM
        // seed) — wrap it. Success can never reach here: the batch guards the
        // return on != SUCCESS, and the final top-level return was dropped.
        local if local.starts_with("retCode") => format!("Err({local})"),
        other => panic!("stream open: unmapped return code `{other}`"),
    }
}

/// Transcribe a batch body region for the Rust open: out-meta writes to dummy
/// locals (Scalar), output-array writes to `lastValue_*` scalars (Scalar) or
/// kept (Fill), previous-output feedback reads to `lastValue_*` (Scalar),
/// every return mapped to the `Result` shape, the final top-level return and
/// top-level CIRCBUF destroys dropped, and (when the open head already
/// short-circuits it) the body's own dead identity branch deleted — in C it is
/// merely dead code, but in Rust it may reference output arrays that do not
/// exist in Scalar mode.
fn build_open_body_rust(model: &StreamModel, body: &[Statement], mode: OutMode) -> Vec<Statement> {
    let outputs = model.outputs.clone();
    let fb_outputs = model.out_feedback.clone();
    let real_outs: HashSet<String> = model
        .func
        .outputs
        .iter()
        .filter(|out| out.param_type != ParamType::Integer)
        .map(|out| out.name.clone())
        .collect();
    let scalar = mode == OutMode::Scalar;
    let fe = move |e: Expr| -> Expr {
        match e {
            Expr::PointerDeref(nm) if scalar && nm == "outBegIdx" => {
                Expr::Var("dummyBegIdx".into())
            }
            Expr::PointerDeref(nm) if scalar && nm == "outNBElement" => {
                Expr::Var("dummyNBElement".into())
            }
            Expr::ArrayAccess(nm, idx)
                if scalar
                    && fb_outputs.contains(&nm)
                    && streaming::is_prev_output_read(&idx) =>
            {
                Expr::Var(format!("lastValue_{nm}"))
            }
            other => other,
        }
    };
    let fs = move |s: Statement| -> Option<Statement> {
        match s {
            Statement::Assign {
                target: Expr::ArrayAccess(nm, idx),
                value,
                compound,
            } if outputs.contains(&nm) => {
                // A float integer literal written to a REAL output types as
                // f64 (`lastValue_*` is an f64 local; the batch renderer's
                // ArrayAccess-target wrap is bypassed by the Var rewrite).
                let value = match value {
                    Expr::IntLiteral(n) if real_outs.contains(&nm) && !compound =>
                    {
                        #[allow(clippy::cast_precision_loss)]
                        Expr::Literal(n as f64)
                    }
                    other => other,
                };
                if scalar {
                    Some(Statement::Assign {
                        target: Expr::Var(format!("lastValue_{nm}")),
                        value,
                        compound,
                    })
                } else {
                    Some(Statement::Assign {
                        target: Expr::ArrayAccess(nm, idx),
                        value,
                        compound,
                    })
                }
            }
            Statement::Return { value } => {
                let mapped = match value {
                    Some(Expr::Var(v)) => Some(Expr::Var(map_return_code(&v))),
                    other => panic!("stream open: unexpected return shape {other:?}"),
                };
                Some(Statement::Return { value: mapped })
            }
            other => Some(other),
        }
    };

    let mut body: Vec<Statement> = body.to_vec();
    if matches!(body.last(), Some(Statement::Return { .. })) {
        body.pop();
    }
    body.retain(|st| !matches!(st, Statement::CircBuf(CircBuf::Destroy { .. })));
    // Delete the body's own identity branch (dead: the open head short-circuits
    // the same condition before the body runs).
    if let Some(idp) = &model.identity {
        let cond_dbg = format!("{:?}", idp.condition);
        body.retain(|st| {
            !matches!(st, Statement::If { condition, .. } if format!("{condition:?}") == cond_dbg)
        });
    }
    streaming::rewrite_stmts(&body, &fe, &fs)
}

/// The open-family emitter: `pub(crate) <snake>_open_internal` (Scalar) or
/// `pub <snake>_open_and_fill` (Fill). `body` is the transcribed batch region
/// (loop tier: `model.body`; fast-path-skip: `prologue ++ general arm ++
/// epilogue`).
#[allow(clippy::too_many_arguments)]
fn emit_open_internal(
    o: &mut String,
    func: &FuncDef,
    model: &StreamModel,
    typing: &Typing,
    body: &[Statement],
    enums: &HashMap<String, EnumDef>,
    registry: &Registry,
    helpers: &HelperRegistry,
    counter: &Cell<usize>,
    mode: OutMode,
) {
    emit_open_sig(o, func, mode);
    emit_open_validation_head(o, func, mode);
    emit_open_inits(o, func, &model.outputs, typing, registry, helpers, mode);

    emit_identity_fast_path(o, func, model, typing, registry, helpers, counter, mode);

    // --- transcribed batch body --------------------------------------------
    let open_body = build_open_body_rust(model, body, mode);
    emit_open_region(
        o, func, model, typing, &open_body, enums, registry, helpers, counter, &[],
    );

    emit_capture_and_publish(
        o, func, model, &model.state, typing, registry, helpers, counter, mode, "",
    );
    let _ = writeln!(o, "    }}\n");
}

/// Doc comment + signature line of `open_internal` (Scalar) / `open_and_fill`
/// (Fill). Shared by every tier, including the hand-rolled dispatch and
/// period-bank opens.
fn emit_open_sig(o: &mut String, func: &FuncDef, mode: OutMode) {
    let sn = snake(func);
    let n = func.name.to_uppercase();
    let handle = stream_type_name(func);
    let inputs = streaming::input_array_names(func);
    let vt = value_type(func);
    let mut sig_inputs = String::new();
    for a in &inputs {
        let _ = write!(sig_inputs, "{a}: &[f64], ");
    }
    let mut sig_opts = String::new();
    for p in &func.optional_inputs {
        let _ = write!(sig_opts, ", mut {}: {}", p.name, opt_param_rust_type(&p.param_type));
    }
    match mode {
        OutMode::Scalar => {
            let _ = writeln!(
                o,
                "    /// Internal startIdx-anchored open behind [`Core::{sn}_open`] (composition seam)."
            );
            let _ = writeln!(
                o,
                "    pub(crate) fn {sn}_open_internal(\n        &self, {sig_inputs}startIdx: usize{sig_opts},\n    ) -> Result<({handle}, {vt}), RetCode> {{"
            );
        }
        // Batch parameter order: inputs, optional params, then the output tail
        // (`outBegIdx`, `outNBElement`, one slice per output) — "open's input
        // head followed by batch's output tail".
        OutMode::Fill => {
            let mut outs = String::new();
            for out in &func.outputs {
                let _ = write!(outs, ", {}: &mut [{}]", out.name, out_rust_type(func, &out.name));
            }
            let _ = writeln!(
                o,
                "    /// [`Core::{sn}_open`] that also fills the output array(s) bit-identically to\n    /// [`Core::{sn}`] over `0..len` in the same single pass. Output slices must hold\n    /// `len - lookback` values; undersized slices panic (the batch sizing contract)."
            );
            let _ = writeln!(o, "    #[doc(alias = \"TA_{n}_OpenAndFill\")]");
            let opts_head = sig_opts.trim_start_matches(", ");
            let opts_head = if opts_head.is_empty() {
                String::new()
            } else {
                format!("{opts_head}, ")
            };
            let _ = writeln!(
                o,
                "    pub fn {sn}_open_and_fill(\n        &self, {sig_inputs}{opts_head}outBegIdx: &mut usize, outNBElement: &mut usize{outs},\n    ) -> Result<{handle}, RetCode> {{"
            );
        }
    }
}

/// The open validation head: non-empty (+ equal-length) inputs, the Fill-mode
/// output-distinctness guard (#108), then optional-param validation. Shared by
/// every tier.
fn emit_open_validation_head(o: &mut String, func: &FuncDef, mode: OutMode) {
    let inputs = streaming::input_array_names(func);
    let first = &inputs[0];
    let mut empties: Vec<String> = inputs.iter().map(|i| format!("{i}.is_empty()")).collect();
    for extra in &inputs[1..] {
        empties.push(format!("{extra}.len() != {first}.len()"));
    }
    let _ = writeln!(o, "        if {} {{\n            return Err(RetCode::BadParam);\n        }}", empties.join(" || "));
    // C-parity input-size ceiling: the C API's `historyLen` is an `int`, so a
    // history beyond i32::MAX bars is unrepresentable there — and the AIA tier
    // carries batch-absolute i32 cursors that a longer warm-up would wrap at
    // the capture cast (review finding: update() would panic where batch
    // succeeds). Rejecting up front keeps "no panics post-open" true.
    let _ = writeln!(
        o,
        "        if {first}.len() > i32::MAX as usize {{\n            return Err(RetCode::BadParam);\n        }}"
    );
    if mode == OutMode::Fill {
        // Output mutual-distinctness (#108) — same guard the batch emits.
        let outs: Vec<&str> = func.outputs.iter().map(|out| out.name.as_str()).collect();
        for i in 0..outs.len() {
            for b in &outs[i + 1..] {
                let _ = writeln!(
                    o,
                    "        if {a}.as_ptr() == {b}.as_ptr() {{\n            return Err(RetCode::BadParam);\n        }}",
                    a = outs[i]
                );
            }
        }
    }
    for p in &func.optional_inputs {
        o.push_str(&gen_opt_param_validation_with(
            p,
            "        ",
            "return Err(RetCode::BadParam);",
        ));
    }
}

/// The open initialization block: `historyLen`/`endIdx`/`startIdx`, out-meta
/// dummies, the Scalar-mode `lastValue_*` sinks, and private-extra-param
/// locals. Shared by the transcribing tiers (loop/fast-path-skip/dual-mode).
fn emit_open_inits(
    o: &mut String,
    func: &FuncDef,
    outputs: &[String],
    typing: &Typing,
    registry: &Registry,
    helpers: &HelperRegistry,
    mode: OutMode,
) {
    let inputs = streaming::input_array_names(func);
    let first = &inputs[0];
    let _ = writeln!(o, "        let historyLen: usize = {first}.len();");
    let _ = writeln!(o, "        let endIdx: usize = historyLen - 1;");
    match mode {
        OutMode::Scalar => {
            let _ = writeln!(o, "        let mut startIdx = startIdx;");
        }
        OutMode::Fill => {
            let _ = writeln!(o, "        let mut startIdx: usize = 0;");
        }
    }
    let _ = writeln!(o, "        let mut dummyBegIdx: usize = 0;");
    let _ = writeln!(o, "        let mut dummyNBElement: usize = 0;");
    if mode == OutMode::Scalar {
        for out in outputs {
            let (t, d) = if out_is_int(func, out) {
                ("i32", "0_i32")
            } else {
                ("f64", "0.0_f64")
            };
            let _ = writeln!(o, "        let mut lastValue_{out}: {t} = {d};");
        }
    }
    let opt_real_params: Vec<String> = func
        .optional_inputs
        .iter()
        .filter(|p| p.param_type == ParamType::Real)
        .map(|p| p.name.clone())
        .collect();
    for (name, c_type) in &func.private_extra_params {
        let init = func
            .private_param_init
            .iter()
            .find(|(pn, _)| pn == name)
            .map_or_else(
                || panic!("{}: no init for private param {name}", func.name),
                |(_, e)| render_expr(e, &typing.ctx, &opt_real_params, registry, helpers),
            );
        let _ = writeln!(
            o,
            "        let mut {name}: {} = {init};",
            extra_param_rust_type(c_type)
        );
    }
}

/// The capture comment + capture epilogue + `Ok(...)` publish tail. `scalars`
/// is the carried-scalar field list of the state struct (loop: `model.state`;
/// dual-mode: the union of both modes').
#[allow(clippy::too_many_arguments)]
fn emit_capture_and_publish(
    o: &mut String,
    func: &FuncDef,
    model: &StreamModel,
    scalars: &[(String, VarType)],
    typing: &Typing,
    registry: &Registry,
    helpers: &HelperRegistry,
    counter: &Cell<usize>,
    mode: OutMode,
    extra_fields: &str,
) {
    let handle = stream_type_name(func);
    let _ = writeln!(o, "\n        // Capture the live batch state into the handle.");
    emit_capture(o, func, model, scalars, typing, registry, helpers, counter, mode, extra_fields);
    match mode {
        OutMode::Scalar => {
            let value = open_value_tuple(model, "lastValue_");
            let _ = writeln!(
                o,
                "        Ok(({handle} {{ core: self.clone(), state }}, {value}))"
            );
        }
        OutMode::Fill => {
            let _ = writeln!(o, "        Ok({handle} {{ core: self.clone(), state }})");
        }
    }
}

/// `lastValue_out` / `(lastValue_a, lastValue_b, ...)` in batch output order.
fn open_value_tuple(model: &StreamModel, prefix: &str) -> String {
    let vals: Vec<String> = model.outputs.iter().map(|out| format!("{prefix}{out}")).collect();
    if vals.len() == 1 {
        vals[0].clone()
    } else {
        format!("({})", vals.join(", "))
    }
}

/// Render the transcribed open region with batch-identical hoisting: circbuf
/// prologs, `let mut` declarations for every top-level VarDecl, candle
/// unpacking, VarDecl-init re-emission, then the statements.
#[allow(clippy::too_many_arguments)]
fn emit_open_region(
    o: &mut String,
    func: &FuncDef,
    _model: &StreamModel,
    typing: &Typing,
    body: &[Statement],
    enums: &HashMap<String, EnumDef>,
    registry: &Registry,
    helpers: &HelperRegistry,
    counter: &Cell<usize>,
    inserts: &[(usize, String)],
) {
    let ctx = &typing.ctx;
    let for_loop_vars = collect_for_loop_vars(body);
    let output_names: Vec<String> = func.outputs.iter().map(|out| out.name.clone()).collect();
    let opt_real_params: Vec<String> = func
        .optional_inputs
        .iter()
        .filter(|p| p.param_type == ParamType::Real)
        .map(|p| p.name.clone())
        .collect();
    let var_inits: HashMap<String, &Expr> = body
        .iter()
        .filter_map(|s| {
            if let Statement::VarDecl { name, init: Some(init), .. } = s {
                Some((name.clone(), init))
            } else {
                None
            }
        })
        .collect();

    // Declarations (hoisted; always `mut` — the crate allows unused_mut).
    for stmt in body {
        if let Statement::CircBuf(CircBuf::Prolog { id, layout, static_size }) = stmt {
            o.push_str(&emit_circbuf_prolog_rust(id, layout, *static_size));
            continue;
        }
        if let Statement::VarDecl { var_type, name, .. } = stmt {
            if for_loop_vars.contains(name) {
                continue;
            }
            let (rty, default) = field_type_and_default(typing, name, var_type, false);
            let _ = writeln!(o, "        let mut {name}: {rty} = {default};");
        }
    }

    let candle_used = crate::candle_settings::detect_candle_settings(body);
    if !candle_used.is_empty() {
        o.push_str(&crate::candle_settings::emit_rust_unpacking(&candle_used, 8));
    }

    // VarDecl initializations (skipped when the body reassigns the same var).
    let body_assigned: HashSet<String> = body
        .iter()
        .filter_map(|s| {
            if let Statement::Assign { target: Expr::Var(name), .. } = s {
                Some(name.clone())
            } else {
                None
            }
        })
        .collect();
    for stmt in body {
        if let Statement::VarDecl { name, var_type, init: Some(init) } = stmt {
            if for_loop_vars.contains(name) || body_assigned.contains(name) {
                continue;
            }
            let mut hoisted = Vec::new();
            let mut cnt = counter.get();
            let new_init = hoist_block_helpers(init, helpers, &mut hoisted, &mut cnt, &[]);
            counter.set(cnt);
            o.push_str(&render_hoisted_blocks(
                &hoisted, 8, ctx, &for_loop_vars, &var_inits, &output_names,
                &opt_real_params, enums, registry, helpers, counter,
            ));
            let rendered = render_expr(&new_init, ctx, &opt_real_params, registry, helpers);
            let wrapped = if (ctx.real_vars.contains(name) || *var_type == VarType::Real)
                && expr_is_untyped_integer(&new_init)
            {
                format!("(({rendered}) as f64)")
            } else {
                rendered
            };
            let _ = writeln!(o, "        {name} = {wrapped};");
        }
    }

    for (i, stmt) in body.iter().enumerate() {
        // Composed tier: sub-stream opens splice in IMMEDIATELY before the
        // batch call that consumes their series (order is the contract —
        // in-place smoothing overwrites the raw series right after).
        for (at, text) in inserts {
            if *at == i {
                o.push_str(text);
            }
        }
        if matches!(stmt, Statement::VarDecl { .. }) {
            continue;
        }
        o.push_str(&render_statement(
            stmt, 8, ctx, &for_loop_vars, &var_inits, &output_names, &opt_real_params,
            enums, registry, helpers, counter,
        ));
    }
}

/// The param==1 identity fast path in the open head (mirrors C).
#[allow(clippy::too_many_arguments)]
fn emit_identity_fast_path(
    o: &mut String,
    func: &FuncDef,
    model: &StreamModel,
    typing: &Typing,
    registry: &Registry,
    helpers: &HelperRegistry,
    _counter: &Cell<usize>,
    mode: OutMode,
) {
    let Some(idp) = &model.identity else { return };
    let handle = stream_type_name(func);
    let state = state_type_name(func);
    let sn = snake(func);
    let opt_real_params: Vec<String> = func
        .optional_inputs
        .iter()
        .filter(|p| p.param_type == ParamType::Real)
        .map(|p| p.name.clone())
        .collect();
    let cond = render_expr(&idp.condition, &typing.ctx, &opt_real_params, registry, helpers);
    let lb_args: Vec<String> = func.optional_inputs.iter().map(|p| p.name.clone()).collect();
    let lb_call = format!("self.{sn}_lookback({})", lb_args.join(", "));
    let _ = writeln!(o, "        if {cond} {{");
    let _ = writeln!(
        o,
        "            if historyLen < {lb_call} + 1 {{\n                return Err(RetCode::BadParam);\n            }}"
    );
    // Identity state: params captured, everything else deterministic defaults
    // (1-slot buffers keep the transition's cap-0 guard well-defined).
    let _ = writeln!(o, "            let state = {state} {{");
    for (name, _, default) in state_fields(func, model, typing) {
        let _ = writeln!(o, "                {name}: {default},");
    }
    let _ = writeln!(o, "            }};");
    match mode {
        OutMode::Scalar => {
            let vals: Vec<String> = idp
                .pairs
                .iter()
                .map(|(_, inp)| format!("{inp}[historyLen - 1]"))
                .collect();
            let value = if vals.len() == 1 {
                vals[0].clone()
            } else {
                format!("({})", vals.join(", "))
            };
            let _ = writeln!(
                o,
                "            return Ok(({handle} {{ core: self.clone(), state }}, {value}));"
            );
        }
        OutMode::Fill => {
            let _ = writeln!(o, "            let fillLb: usize = {lb_call};");
            let _ = writeln!(o, "            (*outBegIdx) = fillLb;");
            let _ = writeln!(o, "            (*outNBElement) = historyLen - fillLb;");
            let _ = writeln!(o, "            let mut fillIdx: usize = 0;");
            let _ = writeln!(o, "            while fillIdx < historyLen - fillLb {{");
            for (out, inp) in &idp.pairs {
                let _ = writeln!(o, "                {out}[fillIdx] = {inp}[fillLb + fillIdx];");
            }
            let _ = writeln!(o, "                fillIdx += 1;");
            let _ = writeln!(o, "            }}");
            let _ = writeln!(
                o,
                "            return Ok({handle} {{ core: self.clone(), state }});"
            );
        }
    }
    let _ = writeln!(o, "        }}");
}

// ---------------------------------------------------------------------------
// State capture
// ---------------------------------------------------------------------------

/// The capture epilogue: compute ring/window/extrema capacities NUMERICALLY
/// from the still-live batch locals (through i64 so C's sanity guards keep
/// their `< 0` half without usize underflow), build the buffers, and finish
/// with the state-struct literal. CIRCBUF capture MOVES the batch-materialized
/// storage (contents AND rotation phase — the CCI-class summation-order
/// requirement) instead of copying.
#[allow(clippy::too_many_arguments, clippy::too_many_lines)]
fn emit_capture(
    o: &mut String,
    func: &FuncDef,
    model: &StreamModel,
    scalars: &[(String, VarType)],
    typing: &Typing,
    registry: &Registry,
    helpers: &HelperRegistry,
    counter: &Cell<usize>,
    mode: OutMode,
    extra_fields: &str,
) {
    let state = state_type_name(func);
    let _ = counter;

    for ring in model.rings() {
        let v = &ring.var;
        let back = ring.back;
        if back > 0 {
            let _ = writeln!(
                o,
                "        let capLag_{v}: i64 = ({c} as i64) - ({v} as i64);",
                c = model.cursor
            );
            let _ = writeln!(
                o,
                "        let cap_{v}: i64 = capLag_{v} + {};",
                back + 1
            );
            let _ = writeln!(
                o,
                "        if capLag_{v} < {fwd} || cap_{v} > historyLen as i64 {{\n            return Err(RetCode::InternalError);\n        }}",
                fwd = ring.fwd
            );
        } else {
            let _ = writeln!(
                o,
                "        let cap_{v}: i64 = ({c} as i64) - ({v} as i64);",
                c = model.cursor
            );
            let _ = writeln!(
                o,
                "        if cap_{v} < 0 || cap_{v} > historyLen as i64 {{\n            return Err(RetCode::InternalError);\n        }}"
            );
        }
        let _ = writeln!(
            o,
            "        let allocN_{v}: usize = if cap_{v} > 0 {{ cap_{v} as usize }} else {{ 1 }};"
        );
        for arr in &ring.arrays {
            let _ = writeln!(
                o,
                "        let mut ring_{v}_{arr}: Vec<f64> = vec![0.0_f64; allocN_{v}];"
            );
            if back > 0 {
                let _ = writeln!(o, "        {{");
                let _ = writeln!(
                    o,
                    "            let mut fillJ: usize = historyLen - cap_{v} as usize;"
                );
                let _ = writeln!(o, "            while fillJ < historyLen {{");
                let _ = writeln!(
                    o,
                    "                ring_{v}_{arr}[fillJ % cap_{v} as usize] = {arr}[fillJ];"
                );
                let _ = writeln!(o, "                fillJ += 1;");
                let _ = writeln!(o, "            }}");
                let _ = writeln!(o, "        }}");
            } else {
                let _ = writeln!(
                    o,
                    "        ring_{v}_{arr}[..cap_{v} as usize]\n            .copy_from_slice(&{arr}[historyLen - cap_{v} as usize..]);"
                );
            }
        }
    }
    for win in model.windows() {
        let v = &win.var;
        let opt_real_params: Vec<String> = func
            .optional_inputs
            .iter()
            .filter(|p| p.param_type == ParamType::Real)
            .map(|p| p.name.clone())
            .collect();
        let cap = render_expr(&win.cap, &typing.ctx, &opt_real_params, registry, helpers);
        let _ = writeln!(o, "        let cap_{v}: i64 = ({cap}) as i64;");
        let _ = writeln!(
            o,
            "        if cap_{v} < 1 || cap_{v} > historyLen as i64 {{\n            return Err(RetCode::InternalError);\n        }}"
        );
        for arr in &win.arrays {
            let _ = writeln!(
                o,
                "        let mut win_{v}_{arr}: Vec<f64> = vec![0.0_f64; cap_{v} as usize];"
            );
            let _ = writeln!(
                o,
                "        win_{v}_{arr}.copy_from_slice(&{arr}[historyLen - cap_{v} as usize..]);"
            );
        }
    }
    if let Some(ex) = model.extrema() {
        let _ = writeln!(
            o,
            "        let capX: i64 = ({c} as i64) - ({t} as i64) + 1;",
            c = model.cursor,
            t = ex.trailing
        );
        let _ = writeln!(
            o,
            "        if capX < 1 || capX > historyLen as i64 {{\n            return Err(RetCode::InternalError);\n        }}"
        );
        for arr in &ex.arrays {
            let _ = writeln!(
                o,
                "        let mut x_{arr}: Vec<f64> = vec![0.0_f64; capX as usize];"
            );
        }
        // Absolute slots: bar j lives at j % cap (a plain tail copy would
        // break the automaton's phase).
        let _ = writeln!(o, "        {{");
        let _ = writeln!(o, "            let mut fillJ: usize = historyLen - capX as usize;");
        let _ = writeln!(o, "            while fillJ < historyLen {{");
        for arr in &ex.arrays {
            let _ = writeln!(o, "                x_{arr}[fillJ % capX as usize] = {arr}[fillJ];");
        }
        let _ = writeln!(o, "                fillJ += 1;");
        let _ = writeln!(o, "            }}");
        let _ = writeln!(o, "        }}");
    }
    for circ in model.circs() {
        let id = &circ.id;
        let _ = writeln!(o, "        let cbSize_{id}: usize = maxIdx_{id} + 1;");
        let _ = writeln!(
            o,
            "        if cbSize_{id} > historyLen + 1 {{\n            return Err(RetCode::InternalError);\n        }}"
        );
    }

    // --- the state literal ---------------------------------------------------
    let _ = writeln!(o, "        let state = {state} {{");
    for p in &func.optional_inputs {
        let _ = writeln!(o, "            {},", p.name);
    }
    for (name, _) in &func.private_extra_params {
        let _ = writeln!(o, "            {name},");
    }
    for (name, _ty) in scalars {
        if model.parity.as_ref().is_some_and(|p| &p.field == name) {
            // Synthetic parity field: seeded to the NEXT bar's parity.
            let _ = writeln!(o, "            {name}: historyLen % 2,");
        } else if typing.extrema_i32.contains(name) {
            let _ = writeln!(o, "            {name}: ({name}) as i32,");
        } else {
            let _ = writeln!(o, "            {name},");
        }
    }
    for name in &model.out_feedback {
        match mode {
            OutMode::Scalar => {
                let _ = writeln!(o, "            lastOut_{name}: lastValue_{name},");
            }
            OutMode::Fill => {
                let _ = writeln!(o, "            lastOut_{name}: {name}[*outNBElement - 1],");
            }
        }
    }
    for lag in &model.lags {
        for k in 1..=lag.depth {
            let _ = writeln!(
                o,
                "            {}: {}[historyLen - {k}],",
                StreamModel::lag_field(&lag.array, k),
                lag.array
            );
        }
    }
    for ring in model.rings() {
        let v = &ring.var;
        if ring.back > 0 {
            let _ = writeln!(o, "            ringPos_{v}: historyLen % cap_{v} as usize,");
            let _ = writeln!(o, "            ringCap_{v}: cap_{v} as usize,");
            let _ = writeln!(o, "            ringLag_{v}: capLag_{v} as usize,");
        } else {
            let _ = writeln!(o, "            ringPos_{v}: 0_usize,");
            let _ = writeln!(o, "            ringCap_{v}: cap_{v} as usize,");
        }
        for arr in &ring.arrays {
            let _ = writeln!(o, "            ring_{v}_{arr},");
        }
    }
    for win in model.windows() {
        let v = &win.var;
        let _ = writeln!(o, "            winPos_{v}: 0_usize,");
        let _ = writeln!(o, "            winCap_{v}: cap_{v} as usize,");
        for arr in &win.arrays {
            let _ = writeln!(o, "            win_{v}_{arr},");
        }
    }
    for circ in model.circs() {
        let _ = writeln!(o, "            cbSize_{0}: cbSize_{0},", circ.id);
        for (storage, _) in streaming::circ_storages(circ) {
            // MOVE the live batch buffer (contents AND rotation phase).
            let _ = writeln!(o, "            cb_{storage}: {storage},");
        }
    }
    if let Some(ex) = model.extrema() {
        let _ = writeln!(o, "            xCap: capX as i32,");
        for arr in &ex.arrays {
            let _ = writeln!(o, "            x_{arr},");
        }
    }
    // Composed tier: sub handles + lag-ring fields join the same literal.
    o.push_str(extra_fields);
    let _ = writeln!(o, "        }};");
}

// ---------------------------------------------------------------------------
// Public wrappers + handle impl
// ---------------------------------------------------------------------------

fn emit_open_wrapper(o: &mut String, func: &FuncDef) {
    let sn = snake(func);
    let n = func.name.to_uppercase();
    let handle = stream_type_name(func);
    let vt = value_type(func);
    let inputs = streaming::input_array_names(func);
    let mut sig_inputs = String::new();
    let mut fwd_inputs = String::new();
    for a in &inputs {
        let _ = write!(sig_inputs, "{a}: &[f64], ");
        let _ = write!(fwd_inputs, "{a}, ");
    }
    let mut sig_opts = String::new();
    let mut fwd_opts = String::new();
    for p in &func.optional_inputs {
        let _ = write!(sig_opts, ", {}: {}", p.name, opt_param_rust_type(&p.param_type));
        let _ = write!(fwd_opts, ", {}", p.name);
    }
    o.push_str(&stream_open_docs(func));
    let _ = writeln!(o, "    #[doc(alias = \"TA_{n}_Open\")]");
    let _ = writeln!(
        o,
        "    pub fn {sn}_open(&self, {sig_inputs}{}) -> Result<({handle}, {vt}), RetCode> {{",
        sig_opts.trim_start_matches(", ")
    );
    let _ = writeln!(
        o,
        "        self.{sn}_open_internal({fwd_inputs}0{fwd_opts})"
    );
    let _ = writeln!(o, "    }}\n");
}

/// Rustdoc for `<snake>_open`, including the peek==update doctest witness.
fn stream_open_docs(func: &FuncDef) -> String {
    let sn = snake(func);
    let mut d = String::new();
    let _ = writeln!(
        d,
        "    /// Open a live {n} stream over the warm-up history; returns the handle and\n    /// the value at the last history bar — bit-identical to [`Core::{sn}`] at that bar.",
        n = func.name.to_uppercase()
    );
    let _ = writeln!(
        d,
        "    ///\n    /// # Errors\n    ///\n    /// [`RetCode::BadParam`] when a parameter is out of range, an input is empty or\n    /// input lengths differ, or the history is shorter than `lookback + 1` bars."
    );
    if let Some(doctest) = stream_doctest(func, &sn) {
        let _ = writeln!(d, "    ///");
        for line in doctest {
            if line.is_empty() {
                let _ = writeln!(d, "    ///");
            } else {
                let _ = writeln!(d, "    /// {line}");
            }
        }
    }
    d
}

/// A runnable peek==update doctest (the per-function bit-exactness witness).
fn stream_doctest(func: &FuncDef, sn: &str) -> Option<Vec<String>> {
    let mut lines: Vec<String> = Vec::new();
    lines.push("```".to_string());
    lines.push("use ta_lib::Core;".to_string());
    let mut args: Vec<String> = Vec::new();
    let mut bar_args: Vec<String> = Vec::new();
    for input in &func.inputs {
        let (var, def, bar) = match input.name.as_str() {
            "inOpen" => ("open", "100.0 + 10.0 * (0.1 * i as f64 - 0.05).sin()", "100.2"),
            "inHigh" => ("high", "101.0 + 10.0 * (0.1 * i as f64).sin()", "101.4"),
            "inLow" => ("low", "99.0 + 10.0 * (0.1 * i as f64).sin()", "99.1"),
            "inClose" => ("close", "100.0 + 10.0 * (0.1 * i as f64).sin()", "100.9"),
            "inVolume" => ("volume", "10_000.0 + 100.0 * i as f64", "12_345.0"),
            "inPeriods" => ("periods", "5.0 + (i % 10) as f64", "14.0"),
            "inReal" => ("data", "100.0 + 10.0 * (0.1 * i as f64).sin()", "100.9"),
            "inReal0" => ("data0", "100.0 + 10.0 * (0.1 * i as f64).sin()", "100.9"),
            "inReal1" => ("data1", "100.0 + 10.0 * (0.1 * i as f64 + 0.7).sin()", "101.3"),
            _ => return None,
        };
        lines.push(format!(
            "let {var}: Vec<f64> = (0..252).map(|i| {def}).collect();"
        ));
        args.push(format!("&{var}"));
        bar_args.push(bar.to_string());
    }
    for opt in &func.optional_inputs {
        let default = opt.default.unwrap_or(0.0);
        if opt.param_type == ParamType::Real {
            args.push(format!("{default:?}"));
        } else {
            #[allow(clippy::cast_possible_truncation)]
            let v = default as i64;
            args.push(format!("{v}"));
        }
    }
    lines.push(String::new());
    lines.push("let core = Core::new();".to_string());
    lines.push(format!(
        "let (mut s, _last) = core.{sn}_open({}).expect(\"enough history\");",
        args.join(", ")
    ));
    lines.push(format!("let peeked = s.peek({});", bar_args.join(", ")));
    lines.push(format!("let updated = s.update({});", bar_args.join(", ")));
    // peek == update, bit-for-bit (it is the same code on a throwaway clone).
    let n_outs = func.outputs.len();
    let int_out = func
        .outputs
        .first()
        .is_some_and(|out| out_is_int(func, &out.name));
    if n_outs == 1 {
        if int_out {
            lines.push("assert_eq!(peeked, updated);".to_string());
        } else {
            lines.push("assert_eq!(peeked.to_bits(), updated.to_bits());".to_string());
        }
    } else {
        for i in 0..n_outs {
            let is_int = out_is_int(func, &func.outputs[i].name);
            if is_int {
                lines.push(format!("assert_eq!(peeked.{i}, updated.{i});"));
            } else {
                lines.push(format!(
                    "assert_eq!(peeked.{i}.to_bits(), updated.{i}.to_bits());"
                ));
            }
        }
    }
    lines.push("```".to_string());
    Some(lines)
}

fn emit_update_and_peek(o: &mut String, func: &FuncDef) {
    let sn = snake(func);
    let n = func.name.to_uppercase();
    let handle = stream_type_name(func);
    let vt = value_type(func);
    let inputs = streaming::input_array_names(func);
    let mut sig_bars = String::new();
    let mut fwd_bars = String::new();
    for a in &inputs {
        let _ = write!(sig_bars, "{a}: f64, ");
        let _ = write!(fwd_bars, "{a}, ");
    }
    let sig_bars = sig_bars.trim_end_matches(", ");
    let fwd_bars = fwd_bars.trim_end_matches(", ");

    let mut out_decls = String::new();
    let mut out_refs = String::new();
    for out in &func.outputs {
        let (t, d) = if out_is_int(func, &out.name) {
            ("i32", "0_i32")
        } else {
            ("f64", "0.0_f64")
        };
        let _ = writeln!(out_decls, "        let mut {}: {t} = {d};", out.name);
        let _ = write!(out_refs, ", &mut {}", out.name);
    }
    let ret = open_value_tuple_names(func);

    let _ = writeln!(
        o,
        "#[allow(non_snake_case)]\n#[allow(unused_variables)]\nimpl {handle} {{"
    );
    let _ = writeln!(
        o,
        "    /// Commit one closed bar; always produces a value. Never allocates."
    );
    let _ = writeln!(o, "    #[doc(alias = \"TA_{n}_Update\")]");
    let _ = writeln!(o, "    pub fn update(&mut self, {sig_bars}) -> {vt} {{");
    o.push_str(&out_decls);
    let _ = writeln!(
        o,
        "        self.core.{sn}_step_internal(&mut self.state, {fwd_bars}{out_refs});"
    );
    let _ = writeln!(o, "        {ret}");
    let _ = writeln!(o, "    }}\n");
    let _ = writeln!(
        o,
        "    /// Evaluate a forming bar without committing — bit-identical to what the\n    /// next `update` with the same bar would return (it is the same code, run on\n    /// a throwaway clone). Clones the internal state (allocates for windowed\n    /// indicators)."
    );
    let _ = writeln!(o, "    #[doc(alias = \"TA_{n}_Peek\")]");
    let _ = writeln!(o, "    #[must_use]");
    let _ = writeln!(o, "    pub fn peek(&self, {sig_bars}) -> {vt} {{");
    let _ = writeln!(o, "        let mut scratch = self.clone();");
    let _ = writeln!(o, "        scratch.update({fwd_bars})");
    let _ = writeln!(o, "    }}\n}}\n");
}

/// `outReal` / `(outA, outB, ...)` — the update return expression.
fn open_value_tuple_names(func: &FuncDef) -> String {
    let vals: Vec<String> = func.outputs.iter().map(|out| out.name.clone()).collect();
    if vals.len() == 1 {
        vals[0].clone()
    } else {
        format!("({})", vals.join(", "))
    }
}

fn emit_trait_pin(o: &mut String, func: &FuncDef) {
    let handle = stream_type_name(func);
    let _ = writeln!(
        o,
        "const _: () = {{\n    const fn _assert_auto<T: Send + Sync + Clone>() {{}}\n    _assert_auto::<{handle}>();\n}};\n"
    );
}

// ---------------------------------------------------------------------------
// Dual-mode tier (DI/DM scalar, TRIMA ring): two param-selected arms sharing
// one union handle. See streaming::DualModePlan and c_stream::emit_dual_mode.
// ---------------------------------------------------------------------------

/// Prefix every non-empty line of `s` with `extra` spaces — cosmetic
/// re-indent of a shared-emitter block nested inside an arm branch.
fn indent_block(s: &str, extra: usize) -> String {
    let pad = " ".repeat(extra);
    let mut out = String::new();
    for line in s.lines() {
        if !line.is_empty() {
            out.push_str(&pad);
            out.push_str(line);
        }
        out.push('\n');
    }
    out
}

/// The caller's optional params rewritten onto the handle (`optInTimePeriod`
/// -> `sp.optInTimePeriod`): the step re-derives its arm predicate from the
/// stored immutable param — no mode tag is ever stored.
fn params_on_state(func: &FuncDef, e: &Expr) -> Expr {
    let params: Vec<String> = func.optional_inputs.iter().map(|p| p.name.clone()).collect();
    streaming::rewrite_expr(e, &|x| match x {
        Expr::Var(v) if params.contains(&v) => Expr::Var(format!("sp.{v}")),
        other => other,
    })
}

/// The type-checked union of both modes' carried scalars: mode-A order first,
/// dedup by name, conflicting `VarType`s are a hard error (mirrors C's
/// `emit_dual_state_struct` assert). A mode-B-only field is captured from the
/// arm's untouched prologue-declared local under mode A (C memsets instead —
/// both are the type default).
fn dual_scalar_union(func: &FuncDef, ma: &StreamModel, mb: &StreamModel) -> Vec<(String, VarType)> {
    let mut order: Vec<(String, VarType)> = Vec::new();
    let mut seen: HashMap<String, VarType> = HashMap::new();
    for (name, ty) in ma.state.iter().chain(mb.state.iter()) {
        if let Some(prev) = seen.get(name) {
            assert!(
                prev == ty,
                "{}: dual-mode state `{name}` has conflicting types across modes",
                func.name
            );
        } else {
            seen.insert(name.clone(), ty.clone());
            order.push((name.clone(), ty.clone()));
        }
    }
    order
}

/// Emit the full dual-mode stream section: ONE union state struct, one
/// predicate-branching step, one predicate-branching open per `OutMode` (each
/// arm transcribing `prologue ++ its own body ++ epilogue`, then capturing
/// into the union struct), and the universal update/peek.
#[allow(clippy::too_many_arguments)]
fn emit_dual_mode(
    o: &mut String,
    func: &FuncDef,
    dmp: &streaming::DualModePlan,
    enums: &HashMap<String, EnumDef>,
    registry: &Registry,
    helpers: &HelperRegistry,
    counter: &Cell<usize>,
) {
    let ma = &dmp.mode_a;
    let mb = &dmp.mode_b;
    assert!(
        ma.identity.is_none() && mb.identity.is_none(),
        "{}: dual-mode arms must not carry an identity path",
        func.name
    );

    // Typing over the whole reconstructed body (prologue + both arms +
    // epilogue) so the inference sees the same statement population as batch.
    let mut tbody: Vec<Statement> = dmp.prologue.to_vec();
    tbody.extend_from_slice(ma.body);
    tbody.extend_from_slice(mb.body);
    tbody.extend_from_slice(dmp.epilogue);
    let typing = build_typing_from(func, &tbody, &[ma, mb]);

    let union_scalars = dual_scalar_union(func, ma, mb);
    // Both modes must carry IDENTICAL non-scalar state (TRIMA's odd/even arms
    // share the very same rings; DI/DM have none) — mirror C's assertion.
    assert!(
        state_fields_from(func, ma, &typing, &[]) == state_fields_from(func, mb, &typing, &[]),
        "{}: dual-mode modes carry differing non-scalar state (rings/windows/\
         circs/extrema/feedback/lags); a real per-arm union is not supported",
        func.name
    );

    emit_handle_struct(o, func);
    emit_state_struct_from(o, func, &state_fields_from(func, ma, &typing, &union_scalars));

    let _ = writeln!(o, "{IMPL_ALLOW}impl Core {{");

    // --- step: one function, the mode re-derived from the stored param ------
    let opt_real_params: Vec<String> = func
        .optional_inputs
        .iter()
        .filter(|p| p.param_type == ParamType::Real)
        .map(|p| p.name.clone())
        .collect();
    emit_step_sig(o, func);
    let ctx = build_step_ctx(func, ma, &typing);
    let pred_sp = params_on_state(func, &dmp.predicate);
    let pred_sp = render_expr(&pred_sp, &ctx, &opt_real_params, registry, helpers);
    let _ = writeln!(o, "        if {pred_sp} {{");
    emit_step_body(o, func, ma, &typing, &ctx, enums, registry, helpers, counter, 12);
    let _ = writeln!(o, "        }} else {{");
    emit_step_body(o, func, mb, &typing, &ctx, enums, registry, helpers, counter, 12);
    let _ = writeln!(o, "        }}");
    let _ = writeln!(o, "    }}\n");

    emit_dual_open(o, func, dmp, &typing, &union_scalars, enums, registry, helpers, counter, OutMode::Scalar);
    emit_open_wrapper(o, func);
    emit_dual_open(o, func, dmp, &typing, &union_scalars, enums, registry, helpers, counter, OutMode::Fill);
    let _ = writeln!(o, "}}\n");

    emit_update_and_peek(o, func);
    emit_trait_pin(o, func);
}

/// The dual-mode open (either `OutMode`): shared validation head, then one
/// predicate branch per mode, each transcribing `prologue ++ its arm ++
/// epilogue` and ending in the capture + `Ok(...)` publish (the if/else is
/// the function's tail expression — both arms return).
#[allow(clippy::too_many_arguments)]
fn emit_dual_open(
    o: &mut String,
    func: &FuncDef,
    dmp: &streaming::DualModePlan,
    typing: &Typing,
    union_scalars: &[(String, VarType)],
    enums: &HashMap<String, EnumDef>,
    registry: &Registry,
    helpers: &HelperRegistry,
    counter: &Cell<usize>,
    mode: OutMode,
) {
    let ma = &dmp.mode_a;
    let mb = &dmp.mode_b;
    emit_open_sig(o, func, mode);
    emit_open_validation_head(o, func, mode);
    emit_open_inits(o, func, &ma.outputs, typing, registry, helpers, mode);

    let opt_real_params: Vec<String> = func
        .optional_inputs
        .iter()
        .filter(|p| p.param_type == ParamType::Real)
        .map(|p| p.name.clone())
        .collect();
    let pred = render_expr(&dmp.predicate, &typing.ctx, &opt_real_params, registry, helpers);
    let _ = writeln!(o, "        if {pred} {{");
    for (k, arm) in [ma, mb].into_iter().enumerate() {
        if k == 1 {
            let _ = writeln!(o, "        }} else {{");
        }
        // prologue ++ this arm's body ++ epilogue — the prologue computes the
        // mode-appropriate lookback/clamp, so min-history is per-mode correct
        // by construction. No dead-decl drop (C prunes for -Wunused): unused
        // let-bindings are covered by the impl block's allow set, and the
        // other mode's prologue-declared scalars must stay in scope for the
        // union capture (their untouched defaults == C's memset zeros).
        let mut body: Vec<Statement> = dmp.prologue.to_vec();
        body.extend_from_slice(arm.body);
        body.extend_from_slice(dmp.epilogue);
        let open_body = build_open_body_rust(arm, &body, mode);
        let mut s = String::new();
        emit_open_region(&mut s, func, arm, typing, &open_body, enums, registry, helpers, counter, &[]);
        emit_capture_and_publish(&mut s, func, arm, union_scalars, typing, registry, helpers, counter, mode, "");
        o.push_str(&indent_block(&s, 4));
    }
    let _ = writeln!(o, "        }}");
    let _ = writeln!(o, "    }}\n");
}

// ---------------------------------------------------------------------------
// Fast-path-skip tier (MIDPRICE): the loop-tier lifecycle on the general arm.
// ---------------------------------------------------------------------------

/// Emit the fast-path-skip stream section: the standard loop-tier lifecycle
/// on the general (else) arm's model, except the opens transcribe `prologue
/// ++ general-arm body ++ epilogue` — the fast-path `then` arm is a batch-only
/// perf specialization skipped by the stream (its output is bit-identical by
/// construction; the differential gates enforce it across the threshold).
#[allow(clippy::too_many_arguments)]
fn emit_fastpath_skip(
    o: &mut String,
    func: &FuncDef,
    plan: &streaming::FastPathSkipPlan,
    enums: &HashMap<String, EnumDef>,
    registry: &Registry,
    helpers: &HelperRegistry,
    counter: &Cell<usize>,
) {
    let model = &plan.model;
    let mut tbody: Vec<Statement> = plan.prologue.to_vec();
    tbody.extend_from_slice(model.body);
    tbody.extend_from_slice(plan.epilogue);
    let typing = build_typing_from(func, &tbody, &[model]);

    emit_handle_and_state_structs(o, func, model, &typing);
    let _ = writeln!(o, "{IMPL_ALLOW}impl Core {{");
    emit_step(o, func, model, &typing, enums, registry, helpers, counter);
    emit_open_internal(o, func, model, &typing, &tbody, enums, registry, helpers, counter, OutMode::Scalar);
    emit_open_wrapper(o, func);
    emit_open_internal(o, func, model, &typing, &tbody, enums, registry, helpers, counter, OutMode::Fill);
    let _ = writeln!(o, "}}\n");
    emit_update_and_peek(o, func);
    emit_trait_pin(o, func);
}

// ---------------------------------------------------------------------------
// Dispatch tier (MA): a tagged enum over the callees' PUBLIC streams.
// ---------------------------------------------------------------------------

/// `SmaStream` for callee `sma` — the shared naming authority (uppercased so
/// the `to_pascal_case` FuncUnstId spellings apply: `minus_dm` -> `MinusDM`).
fn callee_stream_type(callee: &str) -> String {
    format!("{}Stream", to_pascal_case(&callee.to_uppercase()))
}

/// `MaSub` — the module-private sub-stream enum of a dispatch handle.
fn sub_enum_name(func: &FuncDef) -> String {
    format!("{}Sub", to_pascal_case(&func.name))
}

/// `Sma` — the enum variant name for a supported arm's callee.
fn callee_variant(callee: &str) -> String {
    to_pascal_case(&callee.to_uppercase())
}

/// A minimal render context for dispatch/period-bank expressions (identity
/// conditions, arm opt args): param-pure by plan construction, so only the
/// index scaffolding names are seeded.
fn plan_ctx(func: &FuncDef) -> RustRenderCtx {
    let mut index_vars = HashSet::new();
    index_vars.insert("startIdx".to_string());
    index_vars.insert("endIdx".to_string());
    index_vars.insert("historyLen".to_string());
    RustRenderCtx {
        bounds_asserts: false,
        index_vars,
        real_vars: HashSet::new(),
        vec_vars: HashSet::new(),
        real_array_vars: HashSet::new(),
        int_output_names: func
            .outputs
            .iter()
            .filter(|out| out.param_type == ParamType::Integer)
            .map(|out| out.name.clone())
            .collect(),
        int_vec_vars: HashSet::new(),
        is_lookback: false,
        sentinel_vars: HashSet::new(),
        result_error_returns: true,
    }
}

/// Render a dispatch case label (`MAType_SMA`) to its integer literal via the
/// shared enums map (the same `lookup_variant` authority batch switch labels
/// render through — SMA=0..T3=8) — never hardcoded per function. Panics on a
/// label that does not resolve.
fn dispatch_case_label(label: &str, enums: &HashMap<String, EnumDef>) -> String {
    let (_, variant) = crate::parser::enums::lookup_variant(label, enums)
        .unwrap_or_else(|| panic!("dispatch label `{label}` does not resolve to an enum variant"));
    variant.value.to_string()
}

/// Emit the dispatch stream section (MA): a module-private enum over the
/// callees' public streams — one variant per SUPPORTED arm, derived from plan
/// data, plus `Identity` for the param==1 path — with exhaustive matches
/// everywhere. Supported arms delegate to the callee's `open_internal`
/// (forwarding `startIdx`) / `open_and_fill`; an arm with `supported == false`
/// returns `Err(RetCode::BadParam)` at open (a documented capability
/// limitation that regenerates as a live arm the moment the callee streams).
#[allow(clippy::too_many_lines, clippy::too_many_arguments)]
fn emit_dispatch(
    o: &mut String,
    func: &FuncDef,
    dp: &streaming::DispatchPlan,
    enums: &HashMap<String, EnumDef>,
    registry: &Registry,
    helpers: &HelperRegistry,
    counter: &Cell<usize>,
) {
    let _ = counter;
    let sn = snake(func);
    let handle = stream_type_name(func);
    let state = state_type_name(func);
    let sub_enum = sub_enum_name(func);
    let inputs = streaming::input_array_names(func);
    let outputs: Vec<String> = func.outputs.iter().map(|x| x.name.clone()).collect();
    let bar_args = inputs.join(", ");
    let ctx = plan_ctx(func);
    let params_join = func
        .optional_inputs
        .iter()
        .map(|p| p.name.clone())
        .collect::<Vec<_>>()
        .join(", ");
    let lb_args = params_join.clone();
    let lb_call = format!("self.{sn}_lookback({lb_args})");

    // --- structs + sub enum -------------------------------------------------
    emit_handle_struct(o, func);
    let _ = writeln!(o, "#[derive(Debug, Clone)]\n#[allow(non_snake_case, dead_code)]\nstruct {state} {{");
    for p in &func.optional_inputs {
        let _ = writeln!(o, "    {}: {},", p.name, opt_param_rust_type(&p.param_type));
    }
    let _ = writeln!(
        o,
        "    // Sub-stream, tagged by {}; `{sub_enum}::Identity` on the identity path.",
        dp.param
    );
    let _ = writeln!(o, "    sub: {sub_enum},");
    let _ = writeln!(o, "}}\n");
    let _ = writeln!(o, "#[derive(Debug, Clone)]\nenum {sub_enum} {{");
    if dp.identity.is_some() {
        let _ = writeln!(o, "    Identity,");
    }
    for arm in dp.arms.iter().filter(|a| a.supported) {
        let _ = writeln!(
            o,
            "    {}({}),",
            callee_variant(&arm.callee),
            callee_stream_type(&arm.callee)
        );
    }
    let _ = writeln!(o, "}}\n");

    let _ = writeln!(o, "{IMPL_ALLOW}impl Core {{");

    // --- step ---------------------------------------------------------------
    emit_step_sig(o, func);
    if let Some(idp) = &dp.identity {
        let cond = params_on_state(func, &idp.condition);
        let cond = render_expr(&cond, &ctx, &[], registry, helpers);
        let _ = writeln!(o, "        if {cond} {{");
        for (out, inp) in &idp.pairs {
            let _ = writeln!(o, "            (*{out}) = {inp};");
        }
        let _ = writeln!(o, "            return;");
        let _ = writeln!(o, "        }}");
    }
    let _ = writeln!(o, "        match &mut sp.sub {{");
    if let Some(idp) = &dp.identity {
        // Unreachable after the condition check above, but the exhaustive
        // match must cover the variant; it is the same passthrough.
        let _ = writeln!(o, "            {sub_enum}::Identity => {{");
        for (out, inp) in &idp.pairs {
            let _ = writeln!(o, "                (*{out}) = {inp};");
        }
        let _ = writeln!(o, "            }}");
    }
    for arm in dp.arms.iter().filter(|a| a.supported) {
        let _ = writeln!(o, "            {sub_enum}::{}(sub) => {{", callee_variant(&arm.callee));
        // Route callee output slots through the arm's OutSlot map: Forward(k)
        // lands in the dispatch func's output k, Discard drops the slot (the
        // nullable FAMA when MA routes only the MAMA line, #125).
        if arm.out_map.len() == 1 {
            let streaming::OutSlot::Forward(k) = arm.out_map[0] else {
                panic!("single-output arm cannot discard its only slot");
            };
            let _ = writeln!(o, "                (*{}) = sub.update({bar_args});", outputs[k]);
        } else {
            let _ = writeln!(o, "                let subValue = sub.update({bar_args});");
            for (i, slot) in arm.out_map.iter().enumerate() {
                if let streaming::OutSlot::Forward(k) = slot {
                    let _ = writeln!(o, "                (*{}) = subValue.{i};", outputs[*k]);
                }
            }
        }
        let _ = writeln!(o, "            }}");
    }
    let _ = writeln!(o, "        }}");
    let _ = writeln!(o, "    }}\n");

    // --- open_internal ------------------------------------------------------
    emit_open_sig(o, func, OutMode::Scalar);
    emit_open_validation_head(o, func, OutMode::Scalar);
    let _ = writeln!(o, "        let historyLen: usize = {}.len();", inputs[0]);
    if let Some(idp) = &dp.identity {
        // The identity path FIRST (batch order — it applies to every arm).
        let cond = render_expr(&idp.condition, &ctx, &[], registry, helpers);
        let _ = writeln!(o, "        if {cond} {{");
        let _ = writeln!(
            o,
            "            if historyLen < {lb_call} + 1 {{\n                return Err(RetCode::BadParam);\n            }}"
        );
        let _ = writeln!(
            o,
            "            let state = {state} {{ {params_join}, sub: {sub_enum}::Identity }};"
        );
        let vals: Vec<String> = idp
            .pairs
            .iter()
            .map(|(_, inp)| format!("{inp}[historyLen - 1]"))
            .collect();
        let value = if vals.len() == 1 {
            vals[0].clone()
        } else {
            format!("({})", vals.join(", "))
        };
        let _ = writeln!(
            o,
            "            return Ok(({handle} {{ core: self.clone(), state }}, {value}));"
        );
        let _ = writeln!(o, "        }}");
    }
    let _ = writeln!(o, "        let (sub, value) = match {} {{", dp.param);
    for arm in &dp.arms {
        let case = dispatch_case_label(&arm.label, enums);
        if arm.supported {
            let opts: Vec<String> = arm
                .opt_args
                .iter()
                .map(|e| render_expr(e, &ctx, &[], registry, helpers))
                .collect();
            let opts = if opts.is_empty() {
                String::new()
            } else {
                format!(", {}", opts.join(", "))
            };
            let _ = writeln!(o, "            {case} => {{");
            let _ = writeln!(
                o,
                "                let (sub, subValue) = self.{}_open_internal({bar_args}, startIdx{opts})?;",
                arm.callee
            );
            // Select the forwarded callee slot(s) in dispatch output order
            // (a multi-output callee's open value is a tuple; Discard slots
            // — MAMA's FAMA — are dropped, #125).
            let value_expr = if arm.out_map.len() == 1 {
                "subValue".to_string()
            } else {
                let mut parts: Vec<String> = Vec::new();
                for k in 0..outputs.len() {
                    let i = arm
                        .out_map
                        .iter()
                        .position(|slot| matches!(slot, streaming::OutSlot::Forward(f) if *f == k))
                        .expect("every dispatch output has a forwarded callee slot");
                    parts.push(format!("subValue.{i}"));
                }
                if parts.len() == 1 {
                    parts.remove(0)
                } else {
                    format!("({})", parts.join(", "))
                }
            };
            let _ = writeln!(
                o,
                "                ({sub_enum}::{}(sub), {value_expr})",
                callee_variant(&arm.callee)
            );
            let _ = writeln!(o, "            }}");
        } else {
            let what = if arm.callee.is_empty() { "delegation" } else { arm.callee.as_str() };
            let _ = writeln!(o, "            /* no {what} stream */");
            let _ = writeln!(o, "            {case} => return Err(RetCode::BadParam),");
        }
    }
    let _ = writeln!(o, "            _ => return Err(RetCode::BadParam),");
    let _ = writeln!(o, "        }};");
    let _ = writeln!(o, "        let state = {state} {{ {params_join}, sub }};");
    let _ = writeln!(o, "        Ok(({handle} {{ core: self.clone(), state }}, value))");
    let _ = writeln!(o, "    }}\n");

    emit_open_wrapper(o, func);

    // --- open_and_fill: delegate per arm; identity fills the shifted copy ---
    emit_open_sig(o, func, OutMode::Fill);
    emit_open_validation_head(o, func, OutMode::Fill);
    let _ = writeln!(o, "        let historyLen: usize = {}.len();", inputs[0]);
    if let Some(idp) = &dp.identity {
        let cond = render_expr(&idp.condition, &ctx, &[], registry, helpers);
        let _ = writeln!(o, "        if {cond} {{");
        let _ = writeln!(
            o,
            "            if historyLen < {lb_call} + 1 {{\n                return Err(RetCode::BadParam);\n            }}"
        );
        let _ = writeln!(o, "            let fillLb: usize = {lb_call};");
        let _ = writeln!(o, "            (*outBegIdx) = fillLb;");
        let _ = writeln!(o, "            (*outNBElement) = historyLen - fillLb;");
        let _ = writeln!(o, "            let mut fillIdx: usize = 0;");
        let _ = writeln!(o, "            while fillIdx < historyLen - fillLb {{");
        for (out, inp) in &idp.pairs {
            let _ = writeln!(o, "                {out}[fillIdx] = {inp}[fillLb + fillIdx];");
        }
        let _ = writeln!(o, "                fillIdx += 1;");
        let _ = writeln!(o, "            }}");
        let _ = writeln!(
            o,
            "            let state = {state} {{ {params_join}, sub: {sub_enum}::Identity }};"
        );
        let _ = writeln!(o, "            return Ok({handle} {{ core: self.clone(), state }});");
        let _ = writeln!(o, "        }}");
    }
    let _ = writeln!(o, "        let sub = match {} {{", dp.param);
    for arm in &dp.arms {
        let case = dispatch_case_label(&arm.label, enums);
        if arm.supported {
            let opts: Vec<String> = arm
                .opt_args
                .iter()
                .map(|e| render_expr(e, &ctx, &[], registry, helpers))
                .collect();
            let opts = if opts.is_empty() {
                String::new()
            } else {
                format!("{}, ", opts.join(", "))
            };
            let _ = writeln!(o, "            {case} => {sub_enum}::{}(", callee_variant(&arm.callee));
            // OutSlot-mapped fill tail: Forward(k) passes the dispatch func's
            // own array, Discard materializes a throwaway buffer (the Rust
            // rendering of C's NULL for a nullable output — same inline-Vec
            // idiom the batch dispatch uses, #125).
            let fill_outs: String = arm
                .out_map
                .iter()
                .map(|slot| match slot {
                    streaming::OutSlot::Forward(k) => outputs[*k].clone(),
                    streaming::OutSlot::Discard => format!(
                        "&mut vec![0.0_f64; {}.len()][..]",
                        inputs[0]
                    ),
                })
                .collect::<Vec<_>>()
                .join(", ");
            let _ = writeln!(
                o,
                "                self.{}_open_and_fill({bar_args}, {opts}outBegIdx, outNBElement, {fill_outs})?,",
                arm.callee
            );
            let _ = writeln!(o, "            ),");
        } else {
            let what = if arm.callee.is_empty() { "delegation" } else { arm.callee.as_str() };
            let _ = writeln!(o, "            /* no {what} stream */");
            let _ = writeln!(o, "            {case} => return Err(RetCode::BadParam),");
        }
    }
    let _ = writeln!(o, "            _ => return Err(RetCode::BadParam),");
    let _ = writeln!(o, "        }};");
    let _ = writeln!(o, "        let state = {state} {{ {params_join}, sub }};");
    let _ = writeln!(o, "        Ok({handle} {{ core: self.clone(), state }})");
    let _ = writeln!(o, "    }}\n");
    let _ = writeln!(o, "}}\n");

    emit_update_and_peek(o, func);
    emit_trait_pin(o, func);
}

// ---------------------------------------------------------------------------
// Period-bank tier (MAVP): a bank of sub-MA streams advanced in lockstep,
// selected per bar by the clamped variable period.
// ---------------------------------------------------------------------------

/// Emit the period-bank stream section: open builds `bank: Vec<MaStream>` (one
/// slot per period in `[min, max]`, all seeded at the SHARED max-period
/// lookback anchor); update advances every slot in lockstep and returns the
/// slot the clamped per-bar period selects; peek is the universal clone-peek.
/// The bank inherits the callee's per-MAType streamability (MAType_MAMA
/// rejects at the first sub-open, propagated by `?`).
#[allow(clippy::too_many_lines)]
fn emit_period_bank(
    o: &mut String,
    func: &FuncDef,
    plan: &streaming::PeriodBankPlan,
    registry: &Registry,
    helpers: &HelperRegistry,
) {
    let _ = (registry, helpers);
    let handle = stream_type_name(func);
    let state = state_type_name(func);
    let callee = plan.callee.as_str();
    let subty = callee_stream_type(callee);
    let min = plan.min_param.as_str();
    let max = plan.max_param.as_str();
    let price = plan.price_input.as_str();
    let period = plan.period_input.as_str();
    let out = plan.output.as_str();
    let params_join = func
        .optional_inputs
        .iter()
        .map(|p| p.name.clone())
        .collect::<Vec<_>>()
        .join(", ");

    // Callee opt args in the callee's signature order (from the plan; the
    // lookback binds the period slot to the MAX param — the shared anchor).
    let opts_of = |period_arg: &str| -> String {
        plan.callee_opts
            .iter()
            .map(|a| match a {
                streaming::PeriodBankArg::Period => period_arg.to_string(),
                streaming::PeriodBankArg::MAType => plan.matype_param.clone(),
            })
            .collect::<Vec<_>>()
            .join(", ")
    };
    let lb_args = opts_of(max);
    let open_opts = opts_of(&format!("{min} + (bankIdx as i32)"));

    // --- structs ------------------------------------------------------------
    emit_handle_struct(o, func);
    let _ = writeln!(o, "#[derive(Debug, Clone)]\n#[allow(non_snake_case, dead_code)]\nstruct {state} {{");
    for p in &func.optional_inputs {
        let _ = writeln!(o, "    {}: {},", p.name, opt_param_rust_type(&p.param_type));
    }
    let _ = writeln!(
        o,
        "    // One sub-{} stream per period in [{min}, {max}], advanced in lockstep.",
        callee.to_uppercase()
    );
    let _ = writeln!(o, "    bank: Vec<{subty}>,");
    let _ = writeln!(o, "}}\n");

    let _ = writeln!(o, "{IMPL_ALLOW}impl Core {{");

    // --- step: advance ALL slots, output the clamped-period slot ------------
    emit_step_sig(o, func);
    let _ = writeln!(o, "        let mut cp: i32 = {period} as i32;");
    let _ = writeln!(o, "        if cp < sp.{min} {{");
    let _ = writeln!(o, "            cp = sp.{min};");
    let _ = writeln!(o, "        }} else if cp > sp.{max} {{");
    let _ = writeln!(o, "            cp = sp.{max};");
    let _ = writeln!(o, "        }}");
    let _ = writeln!(o, "        let slot: usize = (cp - sp.{min}) as usize;");
    let _ = writeln!(o, "        for (bankIdx, sub) in sp.bank.iter_mut().enumerate() {{");
    let _ = writeln!(o, "            let subValue = sub.update({price});");
    let _ = writeln!(o, "            if bankIdx == slot {{");
    let _ = writeln!(o, "                (*{out}) = subValue;");
    let _ = writeln!(o, "            }}");
    let _ = writeln!(o, "        }}");
    let _ = writeln!(o, "    }}\n");

    // --- open_internal ------------------------------------------------------
    emit_open_sig(o, func, OutMode::Scalar);
    emit_open_validation_head(o, func, OutMode::Scalar);
    let _ = writeln!(o, "        // An inverted [min, max] period window is invalid (batch rejects).");
    let _ = writeln!(o, "        if {min} > {max} {{\n            return Err(RetCode::BadParam);\n        }}");
    let _ = writeln!(o, "        let historyLen: usize = {price}.len();");
    let _ = writeln!(
        o,
        "        // Seed EVERY sub-MA at the SHARED max-period lookback, exactly as the\n        // batch does: it clamps startIdx up to lookback(maxPeriod) and calls the\n        // callee with that same start for every period. Seeding each sub at its\n        // OWN (smaller) lookback would seed the recurrence from a different bar\n        // and diverge for every period < maxPeriod (order-1 for recursive MAs,\n        // running-sum residue for stable ones)."
    );
    let _ = writeln!(o, "        let lookbackTotal: usize = self.{callee}_lookback({lb_args});");
    let _ = writeln!(
        o,
        "        let subStart: usize = if startIdx < lookbackTotal {{ lookbackTotal }} else {{ startIdx }};"
    );
    let _ = writeln!(o, "        let nBank: usize = ({max} - {min} + 1) as usize;");
    let _ = writeln!(o, "        let mut bank: Vec<{subty}> = Vec::with_capacity(nBank);");
    let _ = writeln!(o, "        let mut scratch: Vec<f64> = Vec::with_capacity(nBank);");
    let _ = writeln!(o, "        for bankIdx in 0..nBank {{");
    let _ = writeln!(
        o,
        "            let (sub, subValue) = self.{callee}_open_internal({price}, subStart, {open_opts})?;"
    );
    let _ = writeln!(o, "            bank.push(sub);");
    let _ = writeln!(o, "            scratch.push(subValue);");
    let _ = writeln!(o, "        }}");
    let _ = writeln!(o, "        let mut cp: i32 = {period}[historyLen - 1] as i32;");
    let _ = writeln!(o, "        if cp < {min} {{\n            cp = {min};\n        }} else if cp > {max} {{\n            cp = {max};\n        }}");
    let _ = writeln!(o, "        let lastValue_{out}: f64 = scratch[(cp - {min}) as usize];");
    let _ = writeln!(o, "        let state = {state} {{ {params_join}, bank }};");
    let _ = writeln!(
        o,
        "        Ok(({handle} {{ core: self.clone(), state }}, lastValue_{out}))"
    );
    let _ = writeln!(o, "    }}\n");

    emit_open_wrapper(o, func);

    // --- open_and_fill ------------------------------------------------------
    // No per-bar output array exists to un-discard (the bank yields one
    // selected scalar per bar), so fill genuinely re-runs history: seed the
    // bank on the first-output-bar prefix, emit that bar, then REPLAY updates
    // over the remaining history selecting the clamped-period slot per bar.
    emit_open_sig(o, func, OutMode::Fill);
    emit_open_validation_head(o, func, OutMode::Fill);
    let _ = writeln!(o, "        // An inverted [min, max] period window is invalid (batch rejects).");
    let _ = writeln!(o, "        if {min} > {max} {{\n            return Err(RetCode::BadParam);\n        }}");
    let _ = writeln!(o, "        let historyLen: usize = {price}.len();");
    let _ = writeln!(o, "        let lookbackTotal: usize = self.{callee}_lookback({lb_args});");
    let _ = writeln!(
        o,
        "        if historyLen < lookbackTotal + 1 {{\n            return Err(RetCode::BadParam);\n        }}"
    );
    let _ = writeln!(o, "        let nBank: usize = ({max} - {min} + 1) as usize;");
    let _ = writeln!(o, "        // Seed each sub-MA at the first output bar (lookbackTotal), NOT the last.");
    let _ = writeln!(o, "        let mut bank: Vec<{subty}> = Vec::with_capacity(nBank);");
    let _ = writeln!(o, "        let mut scratch: Vec<f64> = Vec::with_capacity(nBank);");
    let _ = writeln!(o, "        for bankIdx in 0..nBank {{");
    let _ = writeln!(
        o,
        "            let (sub, subValue) = self.{callee}_open_internal(&{price}[..lookbackTotal + 1], lookbackTotal, {open_opts})?;"
    );
    let _ = writeln!(o, "            bank.push(sub);");
    let _ = writeln!(o, "            scratch.push(subValue);");
    let _ = writeln!(o, "        }}");
    let _ = writeln!(o, "        // First output bar (lookbackTotal), then replay the remaining history.");
    let _ = writeln!(o, "        let mut cp: i32 = {period}[lookbackTotal] as i32;");
    let _ = writeln!(o, "        if cp < {min} {{\n            cp = {min};\n        }} else if cp > {max} {{\n            cp = {max};\n        }}");
    let _ = writeln!(o, "        {out}[0] = scratch[(cp - {min}) as usize];");
    let _ = writeln!(o, "        let mut t: usize = lookbackTotal + 1;");
    let _ = writeln!(o, "        while t < historyLen {{");
    let _ = writeln!(o, "            for (bankIdx, sub) in bank.iter_mut().enumerate() {{");
    let _ = writeln!(o, "                scratch[bankIdx] = sub.update({price}[t]);");
    let _ = writeln!(o, "            }}");
    let _ = writeln!(o, "            cp = {period}[t] as i32;");
    let _ = writeln!(o, "            if cp < {min} {{\n                cp = {min};\n            }} else if cp > {max} {{\n                cp = {max};\n            }}");
    let _ = writeln!(o, "            {out}[t - lookbackTotal] = scratch[(cp - {min}) as usize];");
    let _ = writeln!(o, "            t += 1;");
    let _ = writeln!(o, "        }}");
    let _ = writeln!(o, "        (*outBegIdx) = lookbackTotal;");
    let _ = writeln!(o, "        (*outNBElement) = historyLen - lookbackTotal;");
    let _ = writeln!(o, "        let state = {state} {{ {params_join}, bank }};");
    let _ = writeln!(o, "        Ok({handle} {{ core: self.clone(), state }})");
    let _ = writeln!(o, "    }}\n");
    let _ = writeln!(o, "}}\n");

    emit_update_and_peek(o, func);
    emit_trait_pin(o, func);
}

// ---------------------------------------------------------------------------
// Composed tier (STOCH class): producer transition + pipeline of owned public
// sub-handles, mirroring c_stream.rs's emit_composed with the Rust
// simplifications the design blesses: RAII replaces every cleanup ladder and
// series-free replay, `free()` renders as a no-op so lag-ring seeding reads
// the still-live intermediate Vec, and clone-peek deletes peekMode entirely
// (sub handles derive Clone, so the universal peek deep-clones the tree).
// ---------------------------------------------------------------------------

/// Composed producer name map: identical to [`RustStreamNames`] except the
/// intermediate series' "output" write lands in a `cur_<series>` scalar.
struct RustComposedNames {
    series: String,
}

impl streaming::NameMap for RustComposedNames {
    fn state(&self, name: &str) -> String {
        format!("sp.{name}")
    }
    fn bar(&self, array: &str) -> String {
        array.to_string()
    }
    fn output(&self, name: &str) -> Expr {
        if name == self.series {
            Expr::Var(format!("cur_{name}"))
        } else {
            Expr::PointerDeref(name.to_string())
        }
    }
    fn ring_buf(&self, var: &str, array: &str) -> String {
        format!("sp.ring_{var}_{array}")
    }
    fn ring_pos(&self, var: &str) -> String {
        format!("sp.ringPos_{var}")
    }
    fn ring_lag(&self, var: &str) -> String {
        format!("sp.ringLag_{var}")
    }
    fn ring_cap(&self, var: &str) -> String {
        format!("sp.ringCap_{var}")
    }
    fn win_buf(&self, var: &str, array: &str) -> String {
        format!("sp.win_{var}_{array}")
    }
    fn win_pos(&self, var: &str) -> String {
        format!("sp.winPos_{var}")
    }
    fn win_cap(&self, var: &str) -> String {
        format!("sp.winCap_{var}")
    }
    fn circ_buf(&self, storage: &str) -> String {
        format!("sp.cb_{storage}")
    }
    fn extrema_buf(&self, array: &str) -> String {
        format!("sp.x_{array}")
    }
    fn extrema_cap(&self) -> String {
        "sp.xCap".to_string()
    }
}

/// The `cur_<name>` scalars the composed step declares (mirror of
/// `c_stream::composed_cur_scalars`): the producer series, each sub-call's
/// destinations in tail order (dedup), and map-defined outputs.
fn composed_cur_scalars(
    cp: &streaming::ComposedPlan,
    bar_inputs: &[String],
    outputs: &[String],
) -> Vec<String> {
    let mut out: Vec<String> = Vec::new();
    let mut seen: std::collections::BTreeSet<String> = std::collections::BTreeSet::new();
    if let Some(series) = &cp.series {
        seen.insert(series.clone());
        out.push(series.clone());
    }
    for sub in &cp.subs {
        for d in &sub.dsts {
            if !bar_inputs.contains(d) && seen.insert(d.clone()) {
                out.push(d.clone());
            }
        }
    }
    for step in &cp.steps {
        if let streaming::UpdateStep::Map { tail_idx } = step {
            for o in streaming::map_output_writes(&cp.tail[*tail_idx], outputs) {
                if !bar_inputs.contains(&o) && seen.insert(o.clone()) {
                    out.push(o);
                }
            }
        }
    }
    out
}

/// Drop the shells of the map's `for` loops, keeping inner `if` structure
/// (the per-bar step evaluates each element body exactly once).
fn drop_forc_shells(st: &Statement) -> Vec<Statement> {
    match st {
        Statement::ForC { body, .. } => body.iter().flat_map(drop_forc_shells).collect(),
        Statement::If {
            condition,
            then_body,
            else_body,
            cond_comments,
        } => vec![Statement::If {
            condition: condition.clone(),
            then_body: then_body.iter().flat_map(drop_forc_shells).collect(),
            else_body: else_body.iter().flat_map(drop_forc_shells).collect(),
            cond_comments: cond_comments.clone(),
        }],
        other => vec![other.clone()],
    }
}

/// The map loop's single cursor (`for` init variable) — distinguishes a lag
/// ring's current read (`series[cursor + lag]`) from its lagged read
/// (`series[cursor]`).
fn map_cursor(st: &Statement) -> Option<String> {
    let Statement::ForC { init, .. } = st else {
        return None;
    };
    let find = |s: &Statement| match s {
        Statement::Assign {
            target: Expr::Var(v),
            ..
        } => Some(v.clone()),
        _ => None,
    };
    match init.as_ref() {
        Statement::Block { body } => body.iter().find_map(find),
        one => find(one),
    }
}

/// Rust twin of `c_stream::transform_map_step`: series reads/writes become the
/// per-bar `cur_*` scalars (a lag-ring series' `[cursor]` read becomes the
/// ring's oldest slot), params read through `sp.`, `for` shells dropped.
fn transform_map_step(
    st: &Statement,
    cur: &std::collections::BTreeMap<String, String>,
    params: &std::collections::BTreeSet<String>,
    sub_lag_rings: &[streaming::SubLagRing],
) -> Vec<Statement> {
    let cursor = map_cursor(st);
    let lag_series: std::collections::BTreeSet<&str> =
        sub_lag_rings.iter().map(|r| r.series.as_str()).collect();
    let fe = |e: Expr| -> Expr {
        match e {
            Expr::ArrayAccess(name, idx) if lag_series.contains(name.as_str()) => {
                let is_lag = matches!(
                    (&cursor, idx.as_ref()),
                    (Some(c), Expr::Var(v)) if c == v
                );
                if is_lag {
                    // Oldest ring slot = the value `lag` bars behind.
                    Expr::ArrayAccess(
                        format!("sp.lagRing_{name}"),
                        Box::new(Expr::Var(format!("sp.lagRingPos_{name}"))),
                    )
                } else {
                    Expr::Var(cur.get(&name).cloned().unwrap_or_else(|| format!("cur_{name}")))
                }
            }
            Expr::ArrayAccess(name, _) if cur.contains_key(&name) => {
                Expr::Var(cur.get(&name).expect("checked").clone())
            }
            Expr::Var(v) if params.contains(&v) => Expr::Var(format!("sp.{v}")),
            other => other,
        }
    };
    let rewritten = streaming::rewrite_stmts(std::slice::from_ref(st), &fe, &|s| Some(s));
    rewritten.iter().flat_map(drop_forc_shells).collect()
}

/// Step render context for the composed step: the producer's sp-aliased ctx
/// (or the plan ctx when loopless) extended with the `cur_*` scalars, the
/// bar-input scalars, map temps, and the lag-ring fields.
fn composed_step_ctx(
    func: &FuncDef,
    cp: &streaming::ComposedPlan,
    typing: &Typing,
    cur_scalars: &[String],
) -> RustRenderCtx {
    let mut ctx = if let Some(model) = &cp.producer {
        build_step_ctx(func, model, typing)
    } else {
        let mut c = typing.ctx.clone();
        for p in &func.optional_inputs {
            if p.param_type == ParamType::Real {
                c.real_vars.insert(format!("sp.{}", p.name));
            }
        }
        for bar in streaming::input_array_names(func) {
            c.real_vars.insert(bar);
        }
        c
    };
    for name in cur_scalars {
        ctx.real_vars.insert(format!("cur_{name}"));
    }
    for (name, ty) in &cp.map_temps {
        match ty {
            VarType::Real => {
                ctx.real_vars.insert(name.clone());
            }
            VarType::Integer | VarType::Index => {
                ctx.index_vars.insert(name.clone());
            }
            _ => {}
        }
    }
    for ring in &cp.sub_lag_rings {
        let sn = &ring.series;
        ctx.vec_vars.insert(format!("sp.lagRing_{sn}"));
        ctx.real_array_vars.insert(format!("sp.lagRing_{sn}"));
        ctx.index_vars.insert(format!("sp.lagRingPos_{sn}"));
        ctx.index_vars.insert(format!("sp.lagRingCap_{sn}"));
    }
    ctx
}

/// The composed StepInternal: producer transition (writing `cur_<series>`),
/// then the batch-tail pipeline through the owned sub handles, combine maps
/// per bar, lag-ring pushes, and the output writes. No peek flag: peek is the
/// universal clone-of-the-whole-tree.
#[allow(clippy::too_many_arguments, clippy::too_many_lines)]
fn emit_composed_step(
    o: &mut String,
    func: &FuncDef,
    cp: &streaming::ComposedPlan,
    typing: &Typing,
    inputs: &[String],
    outputs: &[String],
    enums: &HashMap<String, EnumDef>,
    registry: &Registry,
    helpers: &HelperRegistry,
    counter: &Cell<usize>,
) {
    emit_step_sig(o, func);
    let cur_scalars = composed_cur_scalars(cp, inputs, outputs);
    let ctx = composed_step_ctx(func, cp, typing, &cur_scalars);

    if let Some(model) = &cp.producer {
        for (name, ty) in &model.temps {
            let (rty, default) = field_type_and_default(typing, name, ty, false);
            let _ = writeln!(o, "        let mut {name}: {rty} = {default};");
        }
    }
    for (name, ty) in &cp.map_temps {
        let (rty, default) = field_type_and_default(typing, name, ty, false);
        let _ = writeln!(o, "        let mut {name}: {rty} = {default};");
    }
    for name in &cur_scalars {
        let _ = writeln!(o, "        let mut cur_{name}: f64 = 0.0_f64;");
    }

    // The cur-map: bar inputs are the step's scalar parameters.
    let mut cur: std::collections::BTreeMap<String, String> = inputs
        .iter()
        .map(|b| (b.clone(), b.clone()))
        .collect();

    let output_names: Vec<String> = func.outputs.iter().map(|out| out.name.clone()).collect();
    let opt_real_params: Vec<String> = func
        .optional_inputs
        .iter()
        .filter(|p| p.param_type == ParamType::Real)
        .map(|p| p.name.clone())
        .collect();
    let var_inits: HashMap<String, &Expr> = HashMap::new();

    if let Some(model) = &cp.producer {
        emit_extrema_rebase(o, model, 8);
        let names = RustComposedNames {
            series: cp.series.clone().expect("producer plan carries a series"),
        };
        let transition = streaming::build_transition(model, &names)
            .unwrap_or_else(|e| panic!("streaming transition: {e}"));
        let mut body = String::new();
        for st in &transition {
            body.push_str(&render_statement(
                st, 8, &ctx, &[], &var_inits, &output_names, &opt_real_params, enums,
                registry, helpers, counter,
            ));
        }
        let step_settings = crate::candle_settings::detect_candle_settings(&model.steady_stmts);
        if !step_settings.is_empty() {
            o.push_str(&crate::candle_settings::emit_rust_unpacking(&step_settings, 8));
        }
        o.push_str(&body);
        let series = cp.series.clone().expect("producer plan carries a series");
        cur.insert(series.clone(), format!("cur_{series}"));
    }

    // Pipeline: the batch tail, one scalar per bar through the sub handles.
    let _ = writeln!(o, "\n        // Pipeline the new bar through the sub-streams (batch tail order).");
    let params: std::collections::BTreeSet<String> =
        func.optional_inputs.iter().map(|p| p.name.clone()).collect();
    for step in &cp.steps {
        match step {
            streaming::UpdateStep::Sub { sub_idx } => {
                let sub = &cp.subs[*sub_idx];
                let args: Vec<String> = sub
                    .srcs
                    .iter()
                    .map(|src| cur.get(src).expect("analyzer ordered sub srcs").clone())
                    .collect();
                let arg_str = args.join(", ");
                if sub.dsts.len() == 1 {
                    let d = &sub.dsts[0];
                    let _ = writeln!(o, "        cur_{d} = sp.sub{sub_idx}.update({arg_str});");
                } else {
                    let _ = writeln!(o, "        {{");
                    let _ = writeln!(o, "            let _sub_out = sp.sub{sub_idx}.update({arg_str});");
                    for (k, d) in sub.dsts.iter().enumerate() {
                        let _ = writeln!(o, "            cur_{d} = _sub_out.{k};");
                    }
                    let _ = writeln!(o, "        }}");
                }
                for d in &sub.dsts {
                    cur.insert(d.clone(), format!("cur_{d}"));
                }
            }
            streaming::UpdateStep::Align { dst, src } => {
                let alias = cur.get(src).expect("analyzer ordered align src").clone();
                cur.insert(dst.clone(), alias);
            }
            streaming::UpdateStep::Map { tail_idx } => {
                for out in streaming::map_output_writes(&cp.tail[*tail_idx], outputs) {
                    cur.entry(out.clone()).or_insert_with(|| format!("cur_{out}"));
                }
                let _ = writeln!(o, "        // Combine map (batch tail, per bar).");
                for st in &transform_map_step(&cp.tail[*tail_idx], &cur, &params, &cp.sub_lag_rings) {
                    o.push_str(&render_statement(
                        st, 8, &ctx, &[], &var_inits, &output_names, &opt_real_params,
                        enums, registry, helpers, counter,
                    ));
                }
            }
        }
    }
    // Push the new sub-output value into each lag ring AFTER every read of the
    // oldest slot in the combine above (mirrors C, incl. the modulo advance).
    for ring in &cp.sub_lag_rings {
        let sn = &ring.series;
        let _ = writeln!(o, "        sp.lagRing_{sn}[sp.lagRingPos_{sn}] = cur_{sn};");
        let _ = writeln!(
            o,
            "        sp.lagRingPos_{sn} = (sp.lagRingPos_{sn} + 1) % sp.lagRingCap_{sn};"
        );
    }
    for out in outputs {
        let _ = writeln!(
            o,
            "        (*{out}) = {};",
            cur.get(out).expect("analyzer gated output")
        );
    }
    let _ = writeln!(o, "    }}\n");
}

/// Anchor rendering for a sub-open's startIdx: `max(0, a - b)`-form anchors
/// render as `saturating_sub` (a negative C anchor is clamped to the callee's
/// lookback either way — saturation to 0 lands below the lookback and clamps
/// identically), everything else casts through usize.
fn render_anchor(
    e: &Expr,
    ctx: &RustRenderCtx,
    opt_real_params: &[String],
    registry: &Registry,
    helpers: &HelperRegistry,
) -> String {
    if let Expr::BinOp(a, crate::ir::BinOp::Sub, b) = e {
        let ra = render_expr(a, ctx, opt_real_params, registry, helpers);
        let rb = render_expr(b, ctx, opt_real_params, registry, helpers);
        return format!("(({ra}) as usize).saturating_sub(({rb}) as usize)");
    }
    let r = render_expr(e, ctx, opt_real_params, registry, helpers);
    format!("(({r}) as usize)")
}

/// The transcribed (region, tail) for the composed open: output arrays renamed
/// to their `sc_` scratch Vecs, early returns mapped to the `Result` shape,
/// final tail return dropped. No out-meta rewrite (the open declares real
/// `&mut usize` bindings named `outBegIdx`/`outNBElement`, so batch text
/// renders verbatim) and no malloc null-check blocks (`vec!` aborts on OOM).
fn build_composed_open_bodies(
    cp: &streaming::ComposedPlan,
    outputs: &[String],
) -> (Vec<Statement>, Vec<Statement>) {
    let outs = outputs.to_vec();
    let fe = move |e: Expr| -> Expr {
        match e {
            Expr::Var(v) if outs.contains(&v) => Expr::Var(format!("sc_{v}")),
            Expr::ArrayAccess(name, idx) if outs.contains(&name) => {
                Expr::ArrayAccess(format!("sc_{name}"), idx)
            }
            other => other,
        }
    };
    let fs = move |s: Statement| -> Option<Statement> {
        match s {
            Statement::Return { value } => {
                let mapped = match value {
                    Some(Expr::Var(v)) => Some(Expr::Var(map_return_code(&v))),
                    other => panic!("composed open: unexpected return shape {other:?}"),
                };
                Some(Statement::Return { value: mapped })
            }
            other => Some(other),
        }
    };
    let region: Vec<Statement> = cp.region.clone();
    let mut tail: Vec<Statement> = cp.tail.to_vec();
    if matches!(tail.last(), Some(Statement::Return { .. })) {
        tail.pop();
    }
    (
        streaming::rewrite_stmts(&region, &fe, &fs),
        streaming::rewrite_stmts(&tail, &fe, &fs),
    )
}

/// Composed Open (Scalar = `open_internal`, Fill = `open_and_fill`):
/// scratch `sc_` output Vecs + verbatim transcription of the batch body with
/// sub-streams opened at the exact consumption points, then capture.
#[allow(clippy::too_many_arguments, clippy::too_many_lines)]
fn emit_composed_open(
    o: &mut String,
    func: &FuncDef,
    cp: &streaming::ComposedPlan,
    typing: &Typing,
    outputs: &[String],
    enums: &HashMap<String, EnumDef>,
    registry: &Registry,
    helpers: &HelperRegistry,
    counter: &Cell<usize>,
    mode: OutMode,
) {
    // The composed fill/scratch path hardcodes f64 Vecs (mirrors C's assert).
    assert!(
        func.outputs.iter().all(|out| !out_is_int(func, &out.name)),
        "composed open assumes real (f64) outputs; {} has an integer output",
        func.name
    );
    let handle = stream_type_name(func);
    let state = state_type_name(func);
    let sn = snake(func);
    let opt_real_params: Vec<String> = func
        .optional_inputs
        .iter()
        .filter(|p| p.param_type == ParamType::Real)
        .map(|p| p.name.clone())
        .collect();

    emit_open_sig(o, func, mode);
    emit_open_validation_head(o, func, mode);
    emit_open_inits(o, func, outputs, typing, registry, helpers, mode);
    if mode == OutMode::Scalar {
        // Real `&mut usize` bindings under the batch names, so the transcribed
        // tail (deref writes AND sub-call pass-through args) renders exactly
        // like the proven batch text.
        let _ = writeln!(o, "        let mut _begStore: usize = 0;");
        let _ = writeln!(o, "        let mut _nbStore: usize = 0;");
        let _ = writeln!(o, "        let outBegIdx: &mut usize = &mut _begStore;");
        let _ = writeln!(o, "        let outNBElement: &mut usize = &mut _nbStore;");
    }
    for out in outputs {
        let _ = writeln!(
            o,
            "        let mut sc_{out}: Vec<f64> = vec![0.0_f64; historyLen];"
        );
    }

    // Sub-open inserts, keyed to combined region++tail indices.
    let (region_stmts, tail_stmts) = build_composed_open_bodies(cp, outputs);
    let region_len = region_stmts.len();
    let mut inserts: Vec<(usize, String)> = Vec::new();
    for (si, sub) in cp.subs.iter().enumerate() {
        let mut t = String::new();
        let callee = sub.callee.to_lowercase();
        let anchor = render_anchor(&sub.s_arg, &typing.ctx, &opt_real_params, registry, helpers);
        let e_arg = render_expr(
            &streaming::rewrite_expr(&sub.e_arg, &|e| match e {
                Expr::Var(v) if outputs.contains(&v) => Expr::Var(format!("sc_{v}")),
                other => other,
            }),
            &typing.ctx,
            &opt_real_params,
            registry,
            helpers,
        );
        let srcs: Vec<String> = sub
            .srcs
            .iter()
            .map(|src| {
                let name = if outputs.contains(src) {
                    format!("sc_{src}")
                } else {
                    src.clone()
                };
                format!("&{name}[..(({e_arg}) as usize) + 1]")
            })
            .collect();
        let opts: Vec<String> = sub
            .opt_args
            .iter()
            .map(|a| render_expr(a, &typing.ctx, &opt_real_params, registry, helpers))
            .collect();
        let opt_tail = if opts.is_empty() {
            String::new()
        } else {
            format!(", {}", opts.join(", "))
        };
        let _ = writeln!(
            t,
            "        // Sub-stream {si}: {} over `{}`, warmed from bar 0 up to the",
            sub.callee,
            sub.srcs.join(", ")
        );
        let _ = writeln!(t, "        // sub-call's own startIdx (the seeding point).");
        let _ = writeln!(
            t,
            "        let (sub{si}, _) = self.{callee}_open_internal({}, {anchor}{opt_tail})?;",
            srcs.join(", ")
        );
        inserts.push((region_len + sub.tail_idx, t));
    }

    let combined: Vec<Statement> = region_stmts
        .into_iter()
        .chain(tail_stmts)
        .collect();
    emit_composed_region(
        o, func, typing, &combined, enums, registry, helpers, counter, &inserts,
    );

    // --- capture ------------------------------------------------------------
    let _ = writeln!(o, "\n        // Capture the live producer state + sub handles.");
    let _ = writeln!(
        o,
        "        if *outNBElement < 1 {{\n            return Err(RetCode::BadParam);\n        }}"
    );
    // Lag rings: seed from the tail of the still-live intermediate Vec (its
    // batch `free()` renders as a no-op in Rust, so no withheld-free dance).
    for ring in &cp.sub_lag_rings {
        let sr = &ring.series;
        let lag = render_expr(&ring.lag, &typing.ctx, &opt_real_params, registry, helpers);
        let _ = writeln!(o, "        let lagCap_{sr}: usize = ({lag}) as usize;");
        let _ = writeln!(
            o,
            "        let mut lagRing_{sr}: Vec<f64> = vec![0.0_f64; lagCap_{sr}];"
        );
        let _ = writeln!(o, "        {{");
        let _ = writeln!(o, "            let mut lagI: usize = 0;");
        let _ = writeln!(o, "            while lagI < lagCap_{sr} {{");
        let _ = writeln!(o, "                lagRing_{sr}[lagI] = {sr}[*outNBElement + lagI];");
        let _ = writeln!(o, "                lagI += 1;");
        let _ = writeln!(o, "            }}");
        let _ = writeln!(o, "        }}");
    }
    // Extra state-literal fields: sub handles + lag rings.
    let mut extra = String::new();
    for (si, _) in cp.subs.iter().enumerate() {
        let _ = writeln!(extra, "            sub{si},");
    }
    for ring in &cp.sub_lag_rings {
        let sr = &ring.series;
        let _ = writeln!(extra, "            lagRingPos_{sr}: 0_usize,");
        let _ = writeln!(extra, "            lagRingCap_{sr}: lagCap_{sr},");
        let _ = writeln!(extra, "            lagRing_{sr},");
    }
    if let Some(model) = &cp.producer {
        emit_capture(
            o, func, model, &model.state, typing, registry, helpers, counter,
            // The producer's own outputs never redirect to lastValue_ in the
            // composed transcription; Scalar semantics for out_feedback do
            // not arise (composed producers have none).
            OutMode::Fill, &extra,
        );
    } else {
        // Loopless pipeline: params + extras + subs/rings only.
        let _ = writeln!(o, "        let state = {state} {{");
        for p in &func.optional_inputs {
            let _ = writeln!(o, "            {},", p.name);
        }
        for (name, _) in &func.private_extra_params {
            let _ = writeln!(o, "            {name},");
        }
        o.push_str(&extra);
        let _ = writeln!(o, "        }};");
    }
    match mode {
        OutMode::Scalar => {
            let vals: Vec<String> = outputs
                .iter()
                .map(|out| format!("sc_{out}[*outNBElement - 1]"))
                .collect();
            let value = if vals.len() == 1 {
                vals[0].clone()
            } else {
                format!("({})", vals.join(", "))
            };
            let _ = writeln!(
                o,
                "        Ok(({handle} {{ core: self.clone(), state }}, {value}))"
            );
        }
        OutMode::Fill => {
            for out in outputs {
                let _ = writeln!(
                    o,
                    "        {out}[..*outNBElement].copy_from_slice(&sc_{out}[..*outNBElement]);"
                );
            }
            let _ = writeln!(o, "        Ok({handle} {{ core: self.clone(), state }})");
        }
    }
    let _ = writeln!(o, "    }}\n");
    if mode == OutMode::Scalar {
        let _ = sn;
        emit_open_wrapper(o, func);
    }
}

/// [`emit_open_region`] without a `StreamModel` (the composed tier may be
/// loopless): identical hoisting + rendering, insert-aware.
#[allow(clippy::too_many_arguments)]
fn emit_composed_region(
    o: &mut String,
    func: &FuncDef,
    typing: &Typing,
    body: &[Statement],
    enums: &HashMap<String, EnumDef>,
    registry: &Registry,
    helpers: &HelperRegistry,
    counter: &Cell<usize>,
    inserts: &[(usize, String)],
) {
    let ctx = &typing.ctx;
    let for_loop_vars = collect_for_loop_vars(body);
    let output_names: Vec<String> = func.outputs.iter().map(|out| out.name.clone()).collect();
    let opt_real_params: Vec<String> = func
        .optional_inputs
        .iter()
        .filter(|p| p.param_type == ParamType::Real)
        .map(|p| p.name.clone())
        .collect();
    let var_inits: HashMap<String, &Expr> = body
        .iter()
        .filter_map(|s| {
            if let Statement::VarDecl { name, init: Some(init), .. } = s {
                Some((name.clone(), init))
            } else {
                None
            }
        })
        .collect();

    for stmt in body {
        if let Statement::CircBuf(CircBuf::Prolog { id, layout, static_size }) = stmt {
            o.push_str(&emit_circbuf_prolog_rust(id, layout, *static_size));
            continue;
        }
        if let Statement::VarDecl { var_type, name, .. } = stmt {
            if for_loop_vars.contains(name) {
                continue;
            }
            let (rty, default) = field_type_and_default(typing, name, var_type, false);
            let _ = writeln!(o, "        let mut {name}: {rty} = {default};");
        }
    }

    let candle_used = crate::candle_settings::detect_candle_settings(body);
    if !candle_used.is_empty() {
        o.push_str(&crate::candle_settings::emit_rust_unpacking(&candle_used, 8));
    }

    let body_assigned: HashSet<String> = body
        .iter()
        .filter_map(|s| {
            if let Statement::Assign { target: Expr::Var(name), .. } = s {
                Some(name.clone())
            } else {
                None
            }
        })
        .collect();
    for stmt in body {
        if let Statement::VarDecl { name, var_type, init: Some(init) } = stmt {
            if for_loop_vars.contains(name) || body_assigned.contains(name) {
                continue;
            }
            let mut hoisted = Vec::new();
            let mut cnt = counter.get();
            let new_init = hoist_block_helpers(init, helpers, &mut hoisted, &mut cnt, &[]);
            counter.set(cnt);
            o.push_str(&render_hoisted_blocks(
                &hoisted, 8, ctx, &for_loop_vars, &var_inits, &output_names,
                &opt_real_params, enums, registry, helpers, counter,
            ));
            let rendered = render_expr(&new_init, ctx, &opt_real_params, registry, helpers);
            let wrapped = if (ctx.real_vars.contains(name) || *var_type == VarType::Real)
                && expr_is_untyped_integer(&new_init)
            {
                format!("(({rendered}) as f64)")
            } else {
                rendered
            };
            let _ = writeln!(o, "        {name} = {wrapped};");
        }
    }

    for (i, stmt) in body.iter().enumerate() {
        for (at, text) in inserts {
            if *at == i {
                o.push_str(text);
            }
        }
        if matches!(stmt, Statement::VarDecl { .. }) {
            continue;
        }
        o.push_str(&render_statement(
            stmt, 8, ctx, &for_loop_vars, &var_inits, &output_names, &opt_real_params,
            enums, registry, helpers, counter,
        ));
    }
}

/// The whole composed stream section.
#[allow(clippy::too_many_arguments)]
fn emit_composed(
    o: &mut String,
    func: &FuncDef,
    cp: &streaming::ComposedPlan,
    enums: &HashMap<String, EnumDef>,
    registry: &Registry,
    helpers: &HelperRegistry,
    counter: &Cell<usize>,
) {
    let inputs = streaming::input_array_names(func);
    let outputs: Vec<String> = func.outputs.iter().map(|x| x.name.clone()).collect();

    // Typing over the full transcription (region ++ tail), so inference sees
    // exactly the statement population batch rendering saw.
    let combined: Vec<Statement> = cp
        .region
        .iter()
        .cloned()
        .chain(cp.tail.iter().cloned())
        .collect();
    let models: Vec<&StreamModel> = cp.producer.iter().collect();
    let mut typing = build_typing_from(func, &combined, &models);
    for out in &outputs {
        typing.ctx.vec_vars.insert(format!("sc_{out}"));
        typing.ctx.real_array_vars.insert(format!("sc_{out}"));
    }

    // --- handle + state struct ---------------------------------------------
    emit_handle_struct(o, func);
    let mut fields: Vec<(String, String, String)> = if let Some(model) = &cp.producer {
        state_fields(func, model, &typing)
    } else {
        let mut f: Vec<(String, String, String)> = Vec::new();
        for p in &func.optional_inputs {
            f.push((
                p.name.clone(),
                opt_param_rust_type(&p.param_type).to_string(),
                p.name.clone(),
            ));
        }
        for (name, c_type) in &func.private_extra_params {
            f.push((
                name.clone(),
                extra_param_rust_type(c_type).to_string(),
                name.clone(),
            ));
        }
        f
    };
    for (si, sub) in cp.subs.iter().enumerate() {
        fields.push((
            format!("sub{si}"),
            callee_stream_type(&sub.callee),
            // Composed has no identity fast path; the default is never rendered.
            String::new(),
        ));
    }
    for ring in &cp.sub_lag_rings {
        let sr = &ring.series;
        fields.push((format!("lagRingPos_{sr}"), "usize".into(), String::new()));
        fields.push((format!("lagRingCap_{sr}"), "usize".into(), String::new()));
        fields.push((format!("lagRing_{sr}"), "Vec<f64>".into(), String::new()));
    }
    emit_state_struct_from(o, func, &fields);

    // --- impl Core ----------------------------------------------------------
    let _ = writeln!(o, "{IMPL_ALLOW}impl Core {{");
    emit_composed_step(
        o, func, cp, &typing, &inputs, &outputs, enums, registry, helpers, counter,
    );
    emit_composed_open(
        o, func, cp, &typing, &outputs, enums, registry, helpers, counter, OutMode::Scalar,
    );
    emit_composed_open(
        o, func, cp, &typing, &outputs, enums, registry, helpers, counter, OutMode::Fill,
    );
    let _ = writeln!(o, "}}\n");

    emit_update_and_peek(o, func);
    emit_trait_pin(o, func);
}
