//! Java stream emitter — the Java twin of `backends/rust_stream.rs` /
//! `backends/c_stream.rs`.
//!
//! For every YAML-declared streamable function this appends a
//! `/**** Streaming API *****/` section to the generated per-function Java
//! fragment (which the shipped `Core.java` splice and the JSON-RPC server
//! inline both pick up unchanged): a `public static final class <Base>Stream`
//! nested in `Core` (per-handle state as package-private fields, `update`/
//! `peek`/`value`/`copy` methods, a deep-copy constructor), a package-private
//! `<base>_StepImpl(sp, bars...)` transition method on `Core` (so batch
//! rendering conventions — `this.compatibility`, cross-calls, `Math.fma`
//! sites — work verbatim), a `private RetCode <base>_OpenImpl(sp, ...)`
//! transcription of the whole batch body, the package-private
//! `<base>OpenInternal(in, startIdx, ...)` composition seam, and the public
//! `<base>Open` / `<base>OpenAndFill` constructors.
//!
//! Bit-exactness argument (same as C/Rust): the open body transcribes the
//! ENTIRE batch body through the same statement renderer as the batch backend,
//! then captures the still-live locals into the handle; the per-bar step is
//! `streaming::build_transition` rendered through the same walkers. No
//! expression text is hand-built outside the shared renderers.
//!
//! Deliberate Java shapings vs C/Rust (design-panel reviewed; see
//! docs/streaming-api-design.md Java sections):
//! - Open failures surface as unchecked exceptions. Inside the private
//!   `_OpenImpl` the batch body's reject returns stay plain `RetCode` (no throw
//!   statements ever cross the shared renderer — its `expr_stmt` hook skips
//!   bare identifiers); the early-SUCCESS no-data/seed-boundary returns are
//!   mapped to `InsufficientHistory` so the thin wrapper can type the
//!   one routine, data-dependent condition as `InsufficientHistoryException`
//!   (an `IllegalArgumentException` subclass). `InternalError` (capture
//!   invariant) becomes `IllegalStateException`; every other reject a plain
//!   `IllegalArgumentException`. Messages carry the stable prefix
//!   `"<NAME> open:"`, where `<NAME>` is the function as the metadata registry
//!   spells it (`SMA`, `HT_TRENDLINE`) — not C's `TA_`-prefixed symbol and not
//!   the Java method name. `update`/`peek` never throw after a successful open.
//! - There is no `close`: a handle is ordinary heap state — GC suffices (no
//!   AutoCloseable, no finalizer). Handles are deliberately NOT serializable;
//!   the sanctioned checkpoint story is re-opening from retained history.
//! - `peek` = deep-copy constructor + step on the throwaway copy (the design
//!   doc's stated cost model); `copy()` exposes the same constructor as an
//!   independent stream. No mirror buffers, no `peekMode`. The copy is deep:
//!   arrays clone, sub-handles copy recursively; only the `Core` reference is
//!   shared (settings identity is the contract).
//! - Multi-output functions return a per-function immutable `Value` class
//!   (public final fields, batch output order, generated toString/equals/
//!   hashCode); `update` caches the instance so `value()` is a pure field
//!   read. Single-output functions return the primitive directly.
//! - Candle settings are SNAPSHOTTED into the handle at open (primitive
//!   fields), matching Rust's frozen-by-copy observable semantics — the step
//!   never reads the live (mutable, torn-read-prone) `CandleSetting` objects.

use std::cell::Cell;
use std::collections::{BTreeSet, HashMap, HashSet};
use std::fmt::Write;

use crate::candle_settings::detect_candle_settings;
use crate::helper_registry::HelperRegistry;
use crate::ir::{CircBuf, EnumDef, Expr, FuncDef, ParamType, Statement, VarType};
use crate::registry::Registry;
use crate::streaming::{self, StreamModel, StreamPlan};

use super::fma::{self, FmaVarSets};
use super::java::{
    build_matype_map, collect_address_of_vars, collect_double_address_of_vars, collect_matype_vars,
    emit_opt_param_validation, java_type_str, render_expr, render_hoisted_blocks,
    render_statement_ctx, JavaRenderCtx, JAVA_CANDLE_FNS,
};
use crate::helper_registry::hoist_block_helpers;

/// Marker heading the generated stream section (tests slice on it; mirrors C/Rust).
pub const SECTION_MARKER: &str = "/**** Streaming API *****/";

/// Whether a Java stream section is emitted for this function (all six
/// StreamPlan tiers are implemented, so this is simply "declared streamable").
pub fn emits_stream(func: &FuncDef, lookup: &dyn streaming::CalleeLookup) -> bool {
    if !func.streaming {
        return false;
    }
    // Resolve `PRAGMA TA_ALT` here, not at the caller — see the Rust twin.
    streaming::validate_streamable(&func.resolved_for(crate::ir::Lang::Java), lookup).is_ok()
}

/// The base every Java identifier for this function is spelled from: the YAML
/// `name:` verbatim (`SMA`, `MA`, `CDL2CROWS`).
fn base_name(func: &FuncDef) -> String {
    func.name.clone()
}

/// Public handle class name, nested in `Core`: `SMA_Stream`, mirroring C's
/// `TA_SMA_Stream` minus the prefix.
pub fn stream_class_name(func: &FuncDef) -> String {
    format!("{}_Stream", base_name(func))
}

fn out_is_int(func: &FuncDef, name: &str) -> bool {
    func.outputs
        .iter()
        .any(|o| o.name == name && o.param_type == ParamType::Integer)
}

/// `double` / `int` element type of an output.
fn out_java_type(func: &FuncDef, name: &str) -> &'static str {
    if out_is_int(func, name) {
        "int"
    } else {
        "double"
    }
}

/// Whether update/peek/value return the multi-output `Value` class.
fn has_value_class(func: &FuncDef) -> bool {
    func.outputs.len() > 1
}

/// The `Value` field name for an output: `outMACDSignal` → `macdSignal`,
/// `outSlowK` → `slowK`, `outInteger` → `integer`, `outAroonDown` → `aroonDown`.
pub(crate) fn value_field_name(out_name: &str) -> String {
    let stripped = out_name.strip_prefix("out").unwrap_or(out_name);
    let chars: Vec<char> = stripped.chars().collect();
    if chars.is_empty() {
        return out_name.to_string();
    }
    // Lowercase the leading caps run, keeping the last capital when it starts
    // a new word (outMACDSignal → macdSignal; outMAMA → mama; outSlowK → slowK).
    let mut caps_run = 0;
    while caps_run < chars.len() && chars[caps_run].is_ascii_uppercase() {
        caps_run += 1;
    }
    let lower_to = if caps_run <= 1 {
        1
    } else if caps_run == chars.len() {
        caps_run
    } else {
        caps_run - 1
    };
    let head: String = chars[..lower_to].iter().collect::<String>().to_lowercase();
    let tail: String = chars[lower_to..].iter().collect();
    head + &tail
}

/// Java type of an optional parameter (batch convention: enums keep their type).
fn opt_param_java_type(p: &ParamType) -> String {
    match p {
        ParamType::Real => "double".to_string(),
        ParamType::Integer => "int".to_string(),
        ParamType::Enum(name) => name.clone(),
        ParamType::Price(_) => unreachable!("price optional params do not exist"),
    }
}

/// Java type of a private extra param (EMA's k factor): C type string → Java.
fn extra_param_java_type(c_type: &str) -> &'static str {
    match c_type {
        "double" => "double",
        "int" => "int",
        other => panic!("unsupported private extra param type: {other}"),
    }
}

/// The `", double inHigh, double inLow"`-style bar parameter list.
fn bar_params(func: &FuncDef) -> (String, String) {
    let inputs = streaming::input_array_names(func);
    let sig: Vec<String> = inputs.iter().map(|a| format!("double {a}")).collect();
    (sig.join(", "), inputs.join(", "))
}

// ---------------------------------------------------------------------------
// NameMap: state through `sp.`, bars as same-named scalars, outputs as
// `sp.cur_<name>` field writes.
// ---------------------------------------------------------------------------

struct JavaStreamNames;

impl streaming::NameMap for JavaStreamNames {
    fn state(&self, name: &str) -> String {
        format!("sp.{name}")
    }
    fn bar(&self, array: &str) -> String {
        array.to_string()
    }
    fn output(&self, name: &str) -> Expr {
        Expr::Var(format!("sp.cur_{name}"))
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
    fn extrema_mask(&self) -> String {
        "sp.xMask".to_string()
    }
}

// ---------------------------------------------------------------------------
// State fields
// ---------------------------------------------------------------------------

/// One handle field: (name, java_type, identity-path default expression).
/// Order mirrors the C stream struct / Rust state struct.
type Field = (String, String, String);

/// Java type + identity-path default for a carried scalar / temp.
fn field_type_and_default(ty: &VarType) -> (String, String) {
    match ty {
        VarType::Real => ("double".into(), "0.0".into()),
        VarType::Integer | VarType::Index => ("int".into(), "0".into()),
        VarType::RetCodeType => ("RetCode".into(), "RetCode.Success".into()),
        VarType::RealPointer => ("double[]".into(), "new double[1]".into()),
        VarType::IntPointer => ("int[]".into(), "new int[1]".into()),
        VarType::RealArray(size) => ("double[]".into(), format!("new double[{size}]")),
        VarType::IntArray(size) => ("int[]".into(), format!("new int[{size}]")),
    }
}

/// The params + `cur_<out>` (+ cachedValue) fields every tier's handle carries
/// (dispatch/period-bank/loopless-composed build on exactly this base).
fn base_fields(func: &FuncDef) -> Vec<Field> {
    let mut fields: Vec<Field> = Vec::new();
    for p in &func.optional_inputs {
        fields.push((p.name.clone(), opt_param_java_type(&p.param_type), p.name.clone()));
    }
    for (name, c_type) in &func.private_extra_params {
        fields.push((name.clone(), extra_param_java_type(c_type).to_string(), name.clone()));
    }
    for out in &func.outputs {
        fields.push((format!("cur_{}", out.name), out_java_type(func, &out.name).to_string(), "0".into()));
    }
    if has_value_class(func) {
        fields.push(("cachedValue".into(), "Value".into(), "null".into()));
    }
    fields
}

/// The full ordered field list of the handle's state (loop-tier shape).
/// `step_settings` = candle settings the transition reads (snapshotted).
fn state_fields(func: &FuncDef, model: &StreamModel, step_settings: &BTreeSet<String>) -> Vec<Field> {
    state_fields_from(func, model, &model.state, step_settings)
}

/// [`state_fields`] with the carried-scalar set supplied by the caller
/// (dual-mode: the union of both modes' scalars).
#[allow(clippy::too_many_lines)]
fn state_fields_from(
    func: &FuncDef,
    model: &StreamModel,
    scalars: &[(String, VarType)],
    step_settings: &BTreeSet<String>,
) -> Vec<Field> {
    let mut fields: Vec<Field> = Vec::new();
    for p in &func.optional_inputs {
        fields.push((
            p.name.clone(),
            opt_param_java_type(&p.param_type),
            p.name.clone(),
        ));
    }
    for (name, c_type) in &func.private_extra_params {
        fields.push((
            name.clone(),
            extra_param_java_type(c_type).to_string(),
            name.clone(),
        ));
    }
    for (name, ty) in scalars {
        let (jty, default) = field_type_and_default(ty);
        fields.push((name.clone(), jty, default));
    }
    for name in &model.out_feedback {
        let t = out_java_type(func, name);
        fields.push((format!("lastOut_{name}"), t.to_string(), "0".to_string()));
    }
    for lag in &model.lags {
        for k in 1..=lag.depth {
            fields.push((
                StreamModel::lag_field(&lag.array, k),
                "double".to_string(),
                "0.0".to_string(),
            ));
        }
    }
    for ring in model.rings() {
        let v = &ring.var;
        fields.push((format!("ringPos_{v}"), "int".into(), "0".into()));
        // Identity path: cap 0 (back==0) / back+1 (back>0) with 1-slot buffers,
        // keeping the transition's cap-0 guard and any read well-defined.
        let id_cap = if ring.back > 0 {
            format!("{}", ring.back + 1)
        } else {
            "0".into()
        };
        fields.push((format!("ringCap_{v}"), "int".into(), id_cap));
        if ring.back > 0 {
            fields.push((format!("ringLag_{v}"), "int".into(), "0".into()));
        }
        for arr in &ring.arrays {
            let id_len = if ring.back > 0 {
                format!("{}", ring.back + 1)
            } else {
                "1".into()
            };
            fields.push((
                format!("ring_{v}_{arr}"),
                "double[]".into(),
                format!("new double[{id_len}]"),
            ));
        }
    }
    for win in model.windows() {
        let v = &win.var;
        fields.push((format!("winPos_{v}"), "int".into(), "0".into()));
        fields.push((format!("winCap_{v}"), "int".into(), "1".into()));
        for arr in &win.arrays {
            fields.push((
                format!("win_{v}_{arr}"),
                "double[]".into(),
                "new double[1]".into(),
            ));
        }
    }
    for circ in model.circs() {
        fields.push((format!("cbSize_{}", circ.id), "int".into(), "0".into()));
        for (storage, ty) in streaming::circ_storages(circ) {
            let (t, d) = if matches!(ty, VarType::Integer) {
                ("int[]", "new int[1]")
            } else {
                ("double[]", "new double[1]")
            };
            fields.push((format!("cb_{storage}"), t.into(), d.into()));
        }
    }
    if let Some(ex) = model.extrema() {
        fields.push(("xMask".into(), "int".into(), "0".into()));
        for arr in &ex.arrays {
            fields.push((
                format!("x_{arr}"),
                "double[]".into(),
                "new double[1]".into(),
            ));
        }
    }
    // Candle-settings snapshot: the step reads these primitives, never the
    // live (mutable) CandleSetting objects — frozen-at-open like Rust.
    for s in step_settings {
        fields.push((format!("cs_{s}_rangeType"), "int".into(), "0".into()));
        fields.push((format!("cs_{s}_avgPeriod"), "int".into(), "0".into()));
        fields.push((format!("cs_{s}_factor"), "double".into(), "0.0".into()));
    }
    // The last committed value per output — `value()` reads these; update
    // returns them; open's capture seeds them (the "value at last history bar").
    for out in &func.outputs {
        let t = out_java_type(func, &out.name);
        fields.push((format!("cur_{}", out.name), t.to_string(), "0".to_string()));
    }
    if has_value_class(func) {
        fields.push(("cachedValue".into(), "Value".into(), "null".into()));
    }
    fields
}

// ---------------------------------------------------------------------------
// Entry point
// ---------------------------------------------------------------------------

/// Generate the whole stream section for one function's fragment.
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
    // Resolve `PRAGMA TA_ALT` here too — `generate` has direct callers.
    let resolved = func.resolved_for(crate::ir::Lang::Java);
    let func: &FuncDef = &resolved;
    assert!(
        func.streaming,
        "java_stream::generate called without a streaming declaration"
    );
    let plan = streaming::validate_streamable(func, registry)
        .unwrap_or_else(|e| panic!("streaming gate: {e}"));

    // FMA fusion sites: same detector recipe as the C stream emitter, so the
    // streamed per-bar code fuses `a*b+c` at the same sites as the batch body
    // (keeps the bitwise batch-vs-stream gate green under FMA). Bar inputs
    // become bare scalar params, so seed them into real_vars explicitly.
    //
    // `stream_source()`, not `private_body`: the fusion sets must be derived
    // from the very body these emitters render, and a
    // `PRAGMA TA_ALT={STREAM,...}` alternate is the case where the two stop
    // being the same slice. Deriving them from the batch body would fuse
    // `a*b+c` at different sites than Rust (which types from the stream model),
    // surfacing as a ~1 ULP cross-language mismatch with nothing pointing here.
    let mut stream_fma = fma::build_fma_var_sets(
        func.stream_source(),
        &func.outputs,
        &fma::INDEX_PARAM_SEEDS,
    );
    for input in streaming::input_array_names(func) {
        stream_fma.real_vars.insert(input);
    }

    let counter = Cell::new(0usize);
    let mut o = String::new();

    let _ = writeln!(o, "{SECTION_MARKER}\n");
    if let Some(m) = func.alt_marker(crate::ir::Tier::Stream, crate::ir::Lang::Java) {
        let _ = writeln!(o, "/* {m} */\n");
    }
    match &plan {
        StreamPlan::Loop(model) => {
            emit_loop(&mut o, func, model, &stream_fma, enums, registry, helpers, &counter);
        }
        StreamPlan::DualMode(dmp) => {
            emit_dual_mode(&mut o, func, dmp, &stream_fma, enums, registry, helpers, &counter);
        }
        StreamPlan::Dispatch(dp) => {
            emit_dispatch(&mut o, func, dp, &stream_fma, enums, registry, helpers, &counter);
        }
        StreamPlan::PeriodBank(pb) => {
            emit_period_bank(&mut o, func, pb, registry, helpers, enums);
        }
        StreamPlan::Composed(cp) => {
            emit_composed(&mut o, func, cp, &stream_fma, enums, registry, helpers, &counter);
        }
    }

    o
}


/// The output-aliasing pairs (#108/#130) as a Java boolean expression, or
/// `None` when a function has nothing to compare. Java is the one managed
/// backend where `out == in` compiles, and only a filling open writes
/// caller-owned arrays. The batch tier's in-place allowance is revoked here not
/// because the fill would compute the wrong answer — measured, it does not — but
/// because the margin between its writes and the capture's seed reads is an
/// accident nothing asserts (rule S6). Reference equality is complete for two
/// SUPPLIED arrays: they are identical or disjoint. A declined output is null,
/// which aliases nothing, so the pairs guard it.
fn alias_condition(func: &FuncDef) -> Option<String> {
    let inputs = streaming::input_array_names(func);
    let outs: Vec<&str> = func.outputs.iter().map(|out| out.name.as_str()).collect();
    let nullable = super::common::nullable_output_names(func);
    // A declined output aliases nothing — and two of them would otherwise
    // compare equal, `null == null`, rejecting a legal call. The batch emitter
    // guards the nullable operand for exactly this reason (rule B6a).
    let guarded = |a: &str, b: &str| {
        let term = format!("(Object){a} == (Object){b}");
        match (nullable.contains(a), nullable.contains(b)) {
            (false, false) => term,
            (true, false) => format!("({a} != null && {term})"),
            (false, true) => format!("({b} != null && {term})"),
            (true, true) => format!("({a} != null && {b} != null && {term})"),
        }
    };
    let mut pairs: Vec<String> = Vec::new();
    for out in &outs {
        for input in &inputs {
            pairs.push(guarded(out, input));
        }
    }
    for i in 0..outs.len() {
        for b in &outs[i + 1..] {
            pairs.push(guarded(outs[i], b));
        }
    }
    if pairs.is_empty() { None } else { Some(pairs.join(" || ")) }
}

/// Output mode for the open family (mirrors `c_stream`). `Core` is the ONE
/// transcription every entry point shares — `<base>_OpenImpl`: output writes are
/// subscripted `out[<idx> * outStride]`, so the filling entries pass stride 1
/// and the caller's arrays while the plain open passes stride 0 and a
/// one-element sink whose slot 0 ends holding the last history value.
/// `Scalar`/`Fill`/`FillInternal` survive as signature selectors for the two
/// exempt tiers (`Dispatch`, `PeriodBank`), which hand-roll a body per entry
/// because theirs differ by more than a stride.
#[derive(Clone, Copy, PartialEq, Eq)]
enum OutMode {
    Scalar,
    Fill,
    /// `Fill` anchored at a caller-supplied `startIdx` — the composed-open
    /// fusion seam (issue #192). Only the Dispatch tier renders a body for it;
    /// every tier that owns a `Core` reaches the same numerics at stride 1.
    FillInternal,
    Core,
}

/// `<idx>` -> `<idx> * outStride`, as IR.
fn scale_by_stride(idx: Expr) -> Expr {
    Expr::BinOp(
        Box::new(idx),
        crate::ir::BinOp::Mul,
        Box::new(Expr::Var("outStride".to_string())),
    )
}

#[allow(clippy::too_many_arguments)]
fn emit_loop(
    o: &mut String,
    func: &FuncDef,
    model: &StreamModel,
    stream_fma: &FmaVarSets,
    enums: &HashMap<String, EnumDef>,
    registry: &Registry,
    helpers: &HelperRegistry,
    counter: &Cell<usize>,
) {
    emit_loop_shape(o, func, model, model.body, stream_fma, enums, registry, helpers, counter);
}

/// The loop-tier lifecycle over an explicit body region — shared by the plain
/// loop tier (`model.body`) and dual-mode (`prologue ++ arm body ++
/// epilogue`).
#[allow(clippy::too_many_arguments)]
fn emit_loop_shape(
    o: &mut String,
    func: &FuncDef,
    model: &StreamModel,
    body: &[Statement],
    stream_fma: &FmaVarSets,
    enums: &HashMap<String, EnumDef>,
    registry: &Registry,
    helpers: &HelperRegistry,
    counter: &Cell<usize>,
) {
    let step_settings = detect_candle_settings(&model.steady_stmts);
    let fields = state_fields(func, model, &step_settings);
    emit_handle_class(o, func, &fields, &SubMembers::none());
    emit_step(o, func, model, &step_settings, stream_fma, enums, registry, helpers, counter);
    emit_open_body(
        o, func, model, body, &fields, &step_settings, stream_fma, enums, registry,
        helpers, counter,
    );
    emit_open_and_fill_internal_wrapper(o, func, true);
    emit_open_wrappers(o, func, true);
}

/// Prefix every non-empty line of `s` with `extra` spaces — cosmetic re-indent
/// of a shared-emitter block nested inside an arm branch (mirrors rust_stream).
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
/// -> `sp.optInTimePeriod`): steps re-derive their predicates from the stored
/// immutable param — no mode tag is ever stored.
fn params_on_state(func: &FuncDef, e: &Expr) -> Expr {
    let params: Vec<String> = func.optional_inputs.iter().map(|p| p.name.clone()).collect();
    streaming::rewrite_expr(e, &|x| match x {
        Expr::Var(v) if params.contains(&v) => Expr::Var(format!("sp.{v}")),
        other => other,
    })
}

/// Render a C-truthy predicate as a Java boolean condition (the same `!= 0`
/// wrap the shared statement walker applies to `if` conditions).
fn render_predicate(
    e: &Expr,
    ctx: &JavaRenderCtx,
    registry: &Registry,
    helpers: &HelperRegistry,
) -> String {
    let s = render_expr(e, ctx, registry, helpers);
    if super::java::is_boolean_expr(e, helpers) {
        s
    } else {
        format!("({s}) != 0")
    }
}

// ---------------------------------------------------------------------------
// Handle class
// ---------------------------------------------------------------------------

/// The tier-owned members of a handle beyond `fields`: the statements that
/// deep-copy them into a fresh handle, and the statements that overwrite them
/// in an existing one without allocating. Both are raw Java; the loop tier
/// owns none and passes [`SubMembers::none`].
struct SubMembers {
    /// Deep-copy statements for the copy constructor.
    copy: String,
    /// In-place overwrite statements for `copyFrom`.
    restore: String,
    /// How many sub-streams the handle owns. Each is a fresh object with its
    /// own arrays and recursively its own subs.
    subs: usize,
    /// Whether what the subs own is unknown at generation time: the dispatch
    /// arm is picked by an `MAType` at run time, and a period bank's width by
    /// an argument. Neither can be counted here, and both are deep.
    unbounded: bool,
}

impl SubMembers {
    fn none() -> Self {
        Self { copy: String::new(), restore: String::new(), subs: 0, unbounded: false }
    }
}

/// Emit the nested handle class. `subs` holds the tier-owned members
/// (sub-handle copies); loop tier passes [`SubMembers::none`].
fn emit_handle_class(o: &mut String, func: &FuncDef, fields: &[Field], subs: &SubMembers) -> bool {
    emit_handle_class_with_members(o, func, fields, subs, "")
}

/// [`emit_handle_class`] with additional raw member declarations (dispatch's
/// `Object sub;`, composed/period-bank sub-handle fields). Returns whether the
/// handle owns anything on the heap, which is what decides whether `peek`
/// reuses a scratch.
fn emit_handle_class_with_members(
    o: &mut String,
    func: &FuncDef,
    fields: &[Field],
    subs: &SubMembers,
    extra_members: &str,
) -> bool {
    let class = stream_class_name(func);
    let base = base_name(func);
    let n = func.name.to_uppercase();

    let _ = writeln!(
        o,
        "   /**\n\
         \x20   * A live {n} stream (unrelated to {{@code java.util.stream}}): one value per\n\
         \x20   * closed bar, bit-identical to {{@link Core#{base}}} over the same series.\n\
         \x20   * Open with {{@link Core#{base}_Open}}; there is no close — the handle is\n\
         \x20   * ordinary heap state, unreferenced handles are simply garbage-collected.\n\
         \x20   * <p>Concurrency: a handle is single-writer — {{@code update}}, {{@code peek}},\n\
         \x20   * {{@code value}} and {{@code copy}} must not race with an {{@code update}} on\n\
         \x20   * the same handle. With no concurrent {{@code update}}, {{@code peek}}/\n\
         \x20   * {{@code value}}/{{@code copy}} never write the handle and may be called\n\
         \x20   * concurrently after safe publication. Independent handles (including\n\
         \x20   * {{@code copy()}} results) are fully independent.\n\
         \x20   * <p>Not serializable by design: to checkpoint, retain the history and\n\
         \x20   * re-open — the result is bit-identical by contract.\n\
         \x20   */"
    );
    let _ = writeln!(o, "   public static final class {class} {{");
    // Not final: `copyFrom` retargets the peek scratch, which is one instance
    // per thread per class and so outlives any one handle's Core.
    let _ = writeln!(o, "      Core core;");
    for (name, jty, _) in fields {
        let _ = writeln!(o, "      {jty} {name};");
    }
    o.push_str(extra_members);
    // The bars this handle has produced a value for (issue #241). Two ints
    // rather than an `OutRange`: `update` runs on every bar and the emitted
    // javadoc promises it never allocates handle state, so the record is built
    // in the accessor instead of replaced per bar.
    let _ = writeln!(o, "      int outRangeBegIdx;");
    let _ = writeln!(o, "      int outRangeCount;");
    let _ = writeln!(o, "\n      {class}( Core core ) {{ this.core = core; }}");
    let _ = writeln!(
        o,
        "\n      /**\n\
         \x20      * The bars this stream has produced a value for, in the input series'\n\
         \x20      * coordinates: {{@code [begIdx, begIdx + count)}}.\n\
         \x20      * <p>It is what {{@link Core#{base}}} reports over the same bars: the\n\
         \x20      * opener sets it to {{@code (lookback, historyLen - lookback)}}, every\n\
         \x20      * accepted {{@code update}} adds one to the count, {{@code peek}} leaves\n\
         \x20      * it alone, and {{@code copy()}} carries it verbatim. A plain\n\
         \x20      * {{@code open}} hands back only the last value, a subset of this range,\n\
         \x20      * because the caller chose not to take the fill.\n\
         \x20      */\n\
         \x20     public OutRange outRange() {{ return new OutRange(outRangeBegIdx, outRangeCount); }}",
        base = base_name(func)
    );

    // Deep-copy constructor: scalars assign, arrays clone (element-wise for
    // sub-handle arrays via copy_extra), sub-handles copy recursively; the
    // Core reference is shared (settings identity is the contract).
    let _ = writeln!(o, "\n      {class}( {class} other ) {{");
    let _ = writeln!(o, "         this.core = other.core;");
    for (name, jty, _) in fields {
        if jty.ends_with("[]") {
            let _ = writeln!(o, "         this.{name} = other.{name}.clone();");
        } else {
            let _ = writeln!(o, "         this.{name} = other.{name};");
        }
    }
    o.push_str(&subs.copy);
    // The fork starts from the same produced range and diverges from there.
    let _ = writeln!(o, "         this.outRangeBegIdx = other.outRangeBegIdx;");
    let _ = writeln!(o, "         this.outRangeCount = other.outRangeCount;");
    let _ = writeln!(o, "      }}");

    // The copy constructor's in-place twin: same result, but it overwrites
    // whatever this instance already owns instead of allocating a peer for it.
    // Only `peek`'s scratch calls it, and only where there is an allocation to
    // save (#201) — a handle whose fields are all scalars is cheaper to
    // allocate outright than to look a scratch up.
    let mut arrays = 0;
    let _ = writeln!(o, "\n      void copyFrom( {class} other ) {{");
    let _ = writeln!(o, "         this.core = other.core;");
    for (name, jty, _) in fields {
        if jty.ends_with("[]") {
            arrays += 1;
            let _ = writeln!(
                o,
                "         if( this.{name} != null && this.{name}.length == other.{name}.length ) {{\n\
                 \x20           System.arraycopy( other.{name}, 0, this.{name}, 0, other.{name}.length );\n\
                 \x20        }} else {{\n\
                 \x20           this.{name} = other.{name}.clone();\n\
                 \x20        }}"
            );
        } else {
            let _ = writeln!(o, "         this.{name} = other.{name};");
        }
    }
    o.push_str(&subs.restore);
    let _ = writeln!(o, "         this.outRangeBegIdx = other.outRangeBegIdx;");
    let _ = writeln!(o, "         this.outRangeCount = other.outRangeCount;");
    let _ = writeln!(o, "      }}");

    // What `peek` trades away by reusing a scratch is a `ThreadLocal.get()`,
    // and what it buys back is the allocation of a peer handle. For one small
    // array that is a wash — measured, it is a slight loss — so the reuse is
    // for the shapes where the copy is several arrays or a sub-stream tree
    // (#201). Everything else keeps the plain copy constructor.
    //
    // Two shapes look like oversights and are not. A single sub-handle
    // (`STDDEV` over `VAR`), and one array plus a single sub-handle (`ADXR`
    // over `ADX`), are both declined although each copy is two or three
    // allocations deep. Measured on both, reusing the scratch is the slower
    // arm: de-selecting them ran −7.6% / −6.2% (`STDDEV`) and −4.1% / −13.8%
    // (`ADXR`) over two A/Bs against 167 unchanged controls, and `ADXR` cost
    // +7.6% / +9.3% selected on a second box and JDK. Taking any sub-handle
    // as sufficient — which is what this tested before — reinstates both.
    // The predicate is now Rust's [`StateShape::scratch_pays`], reached there
    // by the same measurement on the same shape.
    //
    // Java has no counterpart to Rust's elision signal (`peek` under its own
    // `update` names the copies the optimizer already deletes). The ratio is
    // not readable here: `peek` is timed as independent calls the CPU
    // overlaps, `update` as a serial dependency chain, so on this tier
    // `IMI`, `AROON` and the `HT_*` trio read under their own `update` while
    // owning arrays nobody claims are free. Settle a Java row with an A/B.
    let reuse = subs.unbounded || subs.subs >= 2 || arrays >= 2;
    if reuse {
        let _ = writeln!(
            o,
            "\n      /** {{@code peek}}'s reusable scratch — one per thread, see {{@code copyFrom}}. */"
        );
        let _ = writeln!(
            o,
            "      private static final ThreadLocal<{class}> PEEK_SCRATCH = new ThreadLocal<>();"
        );
    }

    emit_value_class(o, func);
    emit_update_peek_value_copy(o, func, reuse);

    let _ = writeln!(o, "   }}");
    reuse
}

/// The immutable multi-output value record (batch output order, components
/// named after the outputs: `outSlowK` → `slowK`).
///
/// A record, not a hand-rolled class: `equals`/`hashCode`/`toString` become
/// spec-guaranteed rather than 20 generated lines each that have to be argued
/// correct. The semantics are identical — a record compares `double` components
/// with `Double.compare`, which agrees with the `doubleToLongBits` comparison
/// this replaces on every input, `±0.0` and every NaN bit pattern included —
/// and the rendered `toString` is byte-for-byte the same `Value[slowK=…, …]`.
///
/// The one thing it costs is the canonical constructor: a public record cannot
/// hide one, so users can fabricate a `Value`. It carries no invariant to
/// protect (any tuple of outputs is a legitimate reading), and in exchange the
/// type destructures in record patterns and binds in JSON mappers with no
/// configuration.
fn emit_value_class(o: &mut String, func: &FuncDef) {
    if !has_value_class(func) {
        return;
    }
    let components: Vec<String> = func
        .outputs
        .iter()
        .map(|out| format!("{} {}", out_java_type(func, &out.name), value_field_name(&out.name)))
        .collect();
    let _ = writeln!(o, "\n      /**");
    let _ = writeln!(o, "       * One output set, in batch output order. Immutable.");
    let _ = writeln!(o, "       *");
    let _ = writeln!(
        o,
        "       * <p>{{@code equals}} compares every component bitwise, so {{@code NaN}}\n\
         \x20      * equals {{@code NaN}} and {{@code 0.0}} does not equal {{@code -0.0}}.\n\
         \x20      * {{@code hashCode}} is consistent with it but its exact value is\n\
         \x20      * unspecified — do not persist it or compare it across JVM versions.\n\
         \x20      *"
    );
    for out in &func.outputs {
        // Same prose the batch method's `@param out…` carries, so an output
        // reads identically in both tiers.
        let desc = func
            .doc
            .as_ref()
            .map_or_else(|| "Output values.".to_string(), |d| super::java_doc::output_desc(out, d));
        let _ = writeln!(o, "       * @param {} {desc}", value_field_name(&out.name));
    }
    let _ = writeln!(o, "       */");
    let _ = writeln!(o, "      public record Value({}) {{ }}", components.join(", "));
}

/// The value expression reading the current outputs off a handle variable.
fn fresh_value_expr(func: &FuncDef, handle_var: &str) -> String {
    if has_value_class(func) {
        let args: Vec<String> = func
            .outputs
            .iter()
            .map(|out| format!("{handle_var}.cur_{}", out.name))
            .collect();
        format!("new Value({})", args.join(", "))
    } else {
        format!("{handle_var}.cur_{}", func.outputs[0].name)
    }
}

/// The per-bar finite-input rejection for `update`/`peek`: one `Double.isFinite`
/// per scalar bar input, before the handle is touched.
///
/// The streaming tier's half of the boundary contract (see
/// `docs/streaming-api-design.md`). Batch does not filter — it computes on
/// whatever it is handed. A handle cannot do that, because its state is
/// retained: one non-finite bar poisons every recursive accumulator in it for
/// the rest of its life, long after the feed recovers.
///
/// `IllegalArgumentException` carrying the same `"<NAME> <what>: "` prefix the
/// open rejections use, so one catch clause covers the whole tier.
fn finite_bar_check(func: &FuncDef, indent: &str, what: &str) -> String {
    let bars = streaming::input_array_names(func);
    if bars.is_empty() {
        return String::new();
    }
    let n = base_name(func);
    let conds: Vec<String> = bars.iter().map(|b| format!("!Double.isFinite({b})")).collect();
    format!(
        "{indent}if( {} )\n{indent}   throw new TaLibArgumentException(\"{n} {what}: BadParam\", RetCode.BadParam);\n",
        conds.join(" || ")
    )
}


fn emit_update_peek_value_copy(o: &mut String, func: &FuncDef, reuse: bool) {
    emit_update_method(o, func);
    emit_update_and_fill_method(o, func);
    emit_peek_method(o, func, reuse);
    emit_value_method(o, func);
    emit_copy_method(o, func);
}

// --- update --------------------------------------------------------------------
fn emit_update_method(o: &mut String, func: &FuncDef) {
    let base = base_name(func);
    let vt = if has_value_class(func) {
        "Value".to_string()
    } else {
        out_java_type(func, &func.outputs[0].name).to_string()
    };
    let (sig_bars, fwd_bars) = bar_params(func);

    let _ = writeln!(
        o,
        "\n      /**\n\
         \x20      * Commit one closed bar, returning the new current value.\n\
         \x20      * Never allocates handle state.\n\
         \x20      * <p>Throws {{@link IllegalArgumentException}} if any bar value is not\n\
         \x20      * finite (NaN or an infinity). That check runs before anything is\n\
         \x20      * written, so the handle is left exactly as it was —\n\
         \x20      * the stream stays usable, so skip the bar or re-open on a clean\n\
         \x20      * history. This is the one place the streaming tier is stricter than\n\
         \x20      * the batch API, which computes on whatever it is given: a handle\n\
         \x20      * retains its state, so a single non-finite bar would poison every\n\
         \x20      * later value it produces.\n\
         \x20      */"
    );
    let _ = writeln!(o, "      public {vt} update( {sig_bars} ) {{");
    o.push_str(&finite_bar_check(func, "         ", "update"));
    let _ = writeln!(o, "         core.{base}_StepImpl(this, {fwd_bars});");
    // After the step and after the finite-bar reject, so a rejected bar leaves
    // the range where it was. `peek` runs the same step on a scratch copy and so
    // never reaches this. Saturating: nothing bounds how many bars a live stream
    // is fed, and past MAX_INDEX it has left the batch index domain anyway.
    let _ = writeln!(o, "         if( this.outRangeCount < MAX_INDEX ) this.outRangeCount++;");
    if has_value_class(func) {
        let _ = writeln!(o, "         this.cachedValue = {};", fresh_value_expr(func, "this"));
        let _ = writeln!(o, "         return this.cachedValue;");
    } else {
        let _ = writeln!(o, "         return {};", fresh_value_expr(func, "this"));
    }
    let _ = writeln!(o, "      }}");
}

// --- updateAndFill ---------------------------------------------------------------
// One emitter for every tier: each owns a `<base>_StepImpl` with the same
// surface, so the n-bar filler is that step in a loop (issue #246).
/// `updateAndFill`'s javadoc — hoisted so the emitter itself stays readable.
fn update_and_fill_doc(func: &FuncDef, count_src: &str) -> String {
    let mut o = String::new();
    // Rule U6a reads the same as S6a, and a caller of this tier needs telling in
    // the same place a caller of the opener is told.
    let declinable = {
        let names = super::common::nullable_output_list(func);
        if names.is_empty() {
            String::new()
        } else {
            let list = names
                .iter()
                .map(|n| format!("{{@code {n}}}"))
                .collect::<Vec<_>>()
                .join(", ");
            format!(
                "\x20      * <p>{list} may be declined with {{@code null}}, per call and\n\
                 \x20      * independently of what the opener was given: the value is still\n\
                 \x20      * computed — {{@link #value()}} reports it — and nothing is written out.\n"
            )
        }
    };
    let _ = writeln!(
        &mut o,
        "\n      /**\n\
         \x20      * Commit {{@code n}} closed bars and write their {{@code n}} values, in one\n\
         \x20      * call — exactly {{@code n}} back-to-back {{@code update}} calls, with one\n\
         \x20      * set of argument checks instead of {{@code n}}. {{@code n}} is\n\
         \x20      * {{@code {count_src}}}; the outputs must hold at least that many, and must\n\
         \x20      * not be the same array as an input or as each other.\n\
         {declinable}\
         \x20      * <p>{{@link #outRange()}} counts what was committed, which is what makes a\n\
         \x20      * rejection readable: a non-finite bar {{@code k}} throws\n\
         \x20      * {{@link IllegalArgumentException}} exactly as {{@code update}} would, with\n\
         \x20      * bars {{@code 0..k}} committed and written, bar {{@code k}} and everything\n\
         \x20      * after it not, and the count advanced by {{@code k}}.\n\
         \x20      */"
    );
    o
}

fn emit_update_and_fill_method(o: &mut String, func: &FuncDef) {
    let base = base_name(func);
    let inputs = streaming::input_array_names(func);
    let mut sig = String::new();
    for a in &inputs {
        let _ = write!(sig, "double {a}[], ");
    }
    for out in &func.outputs {
        let _ = write!(sig, "{} {}[], ", out_java_type(func, &out.name), out.name);
    }
    let sig = sig.trim_end_matches(", ");
    let count_src = inputs
        .first()
        .map_or_else(|| "0".to_string(), |a| format!("{a}.length"));
    let reject = format!(
        "throw new TaLibArgumentException(\"{base} updateAndFill: BadParam\", RetCode.BadParam);"
    );
    o.push_str(&update_and_fill_doc(func, &count_src));
    let _ = writeln!(o, "      public void updateAndFill( {sig} ) {{");
    // Rule U2, ahead of every length: a required array that is absent has no
    // length to read, so without this the tier answered a raw
    // `NullPointerException` naming neither the function nor the argument —
    // where the contract is `RetCode.BadParam`, which in Java is a
    // `TaLibArgumentException` that names both. It is `requireArgument`, the
    // same helper the openers use, so the two tiers reject alike.
    let nullable = super::common::nullable_output_names(func);
    for name in inputs
        .iter()
        .cloned()
        .chain(func.outputs.iter().map(|o| o.name.clone()).filter(|n| !nullable.contains(n)))
    {
        let _ = writeln!(
            o,
            "         requireArgument(\"{base} updateAndFill\", \"{name}\", {name});"
        );
    }
    let _ = writeln!(o, "         final int barCount = {count_src};");
    let mut checks: Vec<String> = inputs
        .iter()
        .skip(1)
        .map(|a| format!("{a}.length != barCount"))
        .collect();
    // A `nullable` output may be declined here exactly as at the opener (rule
    // U6a), per call: bounded only where it was supplied, and its store guarded.
    // Nothing recorded at `Open` constrains what this call presents.
    for out in &func.outputs {
        if nullable.contains(&out.name) {
            checks.push(format!("({0} != null && {0}.length < barCount)", out.name));
        } else {
            checks.push(format!("{}.length < barCount", out.name));
        }
    }
    if let Some(alias) = alias_condition(func) {
        checks.push(alias);
    }
    if !checks.is_empty() {
        let _ = writeln!(o, "         if( {} )", checks.join(" || "));
        let _ = writeln!(o, "            {reject}");
    }
    // `value()` must name the last COMMITTED bar on EVERY exit, the throwing
    // ones included, so the multi-output cache is refreshed in a `finally`.
    //
    // That is sound because of an invariant of the step, not by luck: a
    // composed `<base>_StepImpl` writes its `sp.cur_<out>` fields as its LAST
    // statements, after every sub-stream call — so the one thing that can throw
    // out of the middle of a bar (a sub rejecting a non-finite intermediate,
    // the documented composed hole) leaves `cur_*` still holding bar `i-1`,
    // which is exactly the bar `done` counts. `no_throwing_call_follows_the_cur_capture`
    // pins it, because without that ordering the `finally` would publish a
    // half-written bar.
    //
    // C# needs none of this: its `Value` is a record struct built fresh from
    // the same fields, so it is correct at every exit by construction. Leaving
    // Java's cache stale would make the two backends disagree on an observable
    // the streaming design says they agree on. Single-output handles read
    // `cur_<out>` directly and have no cache at all.
    let cached = has_value_class(func);
    if cached {
        let _ = writeln!(o, "         int done = 0;");
        let _ = writeln!(o, "         try {{");
    }
    let pad = if cached { "            " } else { "         " };
    let _ = writeln!(o, "{pad}for( int i = 0; i < barCount; i++ ) {{");
    let idx_bars: Vec<String> = inputs.iter().map(|a| format!("{a}[i]")).collect();
    if !inputs.is_empty() {
        let conds: Vec<String> = inputs
            .iter()
            .map(|b| format!("!Double.isFinite({b}[i])"))
            .collect();
        let _ = writeln!(o, "{pad}   if( {} )", conds.join(" || "));
        let _ = writeln!(o, "{pad}      {reject}");
    }
    let _ = writeln!(o, "{pad}   core.{base}_StepImpl(this, {});", idx_bars.join(", "));
    for out in &func.outputs {
        let name = &out.name;
        let guard = if nullable.contains(name) { format!("if( {name} != null ) ") } else { String::new() };
        let _ = writeln!(o, "{pad}   {guard}{name}[i] = this.cur_{name};");
    }
    let _ = writeln!(o, "{pad}   if( this.outRangeCount < MAX_INDEX ) this.outRangeCount++;");
    if cached {
        let _ = writeln!(o, "{pad}   done = i + 1;");
    }
    let _ = writeln!(o, "{pad}}}");
    if cached {
        let _ = writeln!(o, "         }} finally {{");
        let _ = writeln!(
            o,
            "            if( done > 0 ) this.cachedValue = {};",
            fresh_value_expr(func, "this")
        );
        let _ = writeln!(o, "         }}");
    }
    let _ = writeln!(o, "      }}");
}

// --- peek ------------------------------------------------------------------------
fn emit_peek_method(o: &mut String, func: &FuncDef, reuse: bool) {
    let class = stream_class_name(func);
    let base = base_name(func);
    let vt = if has_value_class(func) {
        "Value".to_string()
    } else {
        out_java_type(func, &func.outputs[0].name).to_string()
    };
    let (sig_bars, fwd_bars) = bar_params(func);

    let alloc_note = if reuse {
        "It runs on a scratch handle held per thread and\n\
         \x20      * reused, so the copy allocates nothing after the first peek of this\n\
         \x20      * indicator on this thread. That scratch is retained for the life of\n\
         \x20      * the thread."
    } else {
        "It runs on a throwaway copy, which for this\n\
         \x20      * handle's shape is cheaper than reusing one."
    };
    let _ = writeln!(
        o,
        "\n      /**\n\
         \x20      * Evaluate a forming bar without committing — bit-identical to what the\n\
         \x20      * next {{@code update}} with the same bar would return (it is the same\n\
         \x20      * generated code, run on a copy). Never writes this handle, so peeks may\n\
         \x20      * run concurrently with each other. {alloc_note}\n\
         \x20      */"
    );
    let _ = writeln!(o, "      public {vt} peek( {sig_bars} ) {{");
    // Ahead of the scratch copy, not left to the step: a rejected bar must not
    // pay for a handle copy.
    o.push_str(&finite_bar_check(func, "         ", "peek"));
    if reuse {
        // Per thread, not per handle: `peek` must not write the handle (two
        // threads may peek the same one), and a static ThreadLocal keeps the
        // reuse bounded — one scratch per thread per indicator, whatever the
        // number of live handles. `copyFrom` retargets it, Core included.
        //
        // What it retains: one handle copy per (thread, indicator peeked),
        // living as long as the thread and keeping that handle's Core and
        // arrays reachable — releasing every stream handle does not free it.
        // In a container this is the usual static-ThreadLocal shape, where a
        // pooled thread outliving a deployment pins the value and its
        // classloader; the streaming page says so.
        let _ = writeln!(o, "         {class} scratch = PEEK_SCRATCH.get();");
        let _ = writeln!(o, "         if( scratch == null ) {{");
        let _ = writeln!(o, "            scratch = new {class}(this);");
        let _ = writeln!(o, "            PEEK_SCRATCH.set(scratch);");
        let _ = writeln!(o, "         }} else {{");
        let _ = writeln!(o, "            scratch.copyFrom(this);");
        let _ = writeln!(o, "         }}");
    } else {
        let _ = writeln!(o, "         {class} scratch = new {class}(this);");
    }
    let _ = writeln!(o, "         core.{base}_StepImpl(scratch, {fwd_bars});");
    let _ = writeln!(o, "         return {};", fresh_value_expr(func, "scratch"));
    let _ = writeln!(o, "      }}");
}

// --- value -----------------------------------------------------------------------
fn emit_value_method(o: &mut String, func: &FuncDef) {
    let vt = if has_value_class(func) {
        "Value".to_string()
    } else {
        out_java_type(func, &func.outputs[0].name).to_string()
    };

    let _ = writeln!(
        o,
        "\n      /**\n\
         \x20      * The value at the most recently committed bar — the last history bar\n\
         \x20      * right after open, then whatever the latest {{@code update}} returned.\n\
         \x20      * A pure field read; {{@code peek}} does not change it.\n\
         \x20      */"
    );
    let _ = writeln!(o, "      public {vt} value() {{");
    if has_value_class(func) {
        let _ = writeln!(o, "         return this.cachedValue;");
    } else {
        let _ = writeln!(o, "         return {};", fresh_value_expr(func, "this"));
    }
    let _ = writeln!(o, "      }}");
}

// --- copy ------------------------------------------------------------------------
fn emit_copy_method(o: &mut String, func: &FuncDef) {
    let class = stream_class_name(func);

    let _ = writeln!(
        o,
        "\n      /**\n\
         \x20      * An independent deep copy of this stream: both evolve separately from\n\
         \x20      * here on (the Java rendering of the Rust handle's {{@code Clone}}).\n\
         \x20      */"
    );
    let _ = writeln!(o, "      public {class} copy() {{");
    let _ = writeln!(o, "         return new {class}(this);");
    let _ = writeln!(o, "      }}");
}

// ---------------------------------------------------------------------------
// StepImpl
// ---------------------------------------------------------------------------

/// Build the render context for stream-owned code (step bodies, captures).
/// Java needs no type-inference oracle — only the FMA sets matter here; the
/// address-of / matype sets are empty (transitions carry no out-params).
fn stream_ctx<'a>(
    empty: &'a HashSet<String>,
    counter: &'a Cell<usize>,
    fma_sets: &'a FmaVarSets,
) -> JavaRenderCtx<'a> {
    JavaRenderCtx {
        single_precision: false,
        address_of_vars: empty,
        double_address_of_vars: empty,
        float_input_params: empty,
        inline_counter: counter,
        fma: Some(fma_sets),
        // The streaming tier keeps every output required, so no store is guarded.
        nullable_outputs: empty,
        nullable_shadow: false,
        // Stream bodies dispatch MA-type structurally, never via `== TA_MAType_*`.
        matype_map: HashMap::new(),
    }
}

/// `void <base>_StepImpl( <Class> sp, double bar... )` — the one per-bar
/// transition; update runs it on live state, peek on a deep copy.
#[allow(clippy::too_many_arguments)]
fn emit_step(
    o: &mut String,
    func: &FuncDef,
    model: &StreamModel,
    step_settings: &BTreeSet<String>,
    stream_fma: &FmaVarSets,
    enums: &HashMap<String, EnumDef>,
    registry: &Registry,
    helpers: &HelperRegistry,
    counter: &Cell<usize>,
) {
    emit_step_sig(o, func);
    emit_step_body(
        o, func, model, step_settings, stream_fma, enums, registry, helpers, counter, 6,
    );
    let _ = writeln!(o, "   }}");
}

/// The step signature line, shared by every tier (dispatch/period-bank steps
/// hand-roll their bodies but keep the identical surface).
fn emit_step_sig(o: &mut String, func: &FuncDef) {
    let base = base_name(func);
    let class = stream_class_name(func);
    let (sig_bars, _) = bar_params(func);
    let _ = writeln!(o, "   void {base}_StepImpl( {class} sp, {sig_bars} )\n   {{");
}

/// One model's per-bar step body at a given indent: temp decls, the extrema
/// rebase, the candle-snapshot unpacking, and the rendered transition. Called
/// once by the loop tier (indent 6) and once per arm by the dual-mode step.
#[allow(clippy::too_many_arguments)]
fn emit_step_body(
    o: &mut String,
    _func: &FuncDef,
    model: &StreamModel,
    step_settings: &BTreeSet<String>,
    stream_fma: &FmaVarSets,
    enums: &HashMap<String, EnumDef>,
    registry: &Registry,
    helpers: &HelperRegistry,
    counter: &Cell<usize>,
    indent: usize,
) {
    let pad = " ".repeat(indent);
    for (name, ty) in &model.temps {
        let (jty, default) = field_type_and_default(ty);
        let _ = writeln!(o, "{pad}{jty} {name} = {default};");
    }
    emit_extrema_rebase(o, model, indent);
    // Candle settings from the open-time snapshot (never the live objects).
    for s in step_settings {
        let _ = writeln!(o, "{pad}int {s}_rangeType = sp.cs_{s}_rangeType;");
        let _ = writeln!(o, "{pad}int {s}_avgPeriod = sp.cs_{s}_avgPeriod;");
        let _ = writeln!(o, "{pad}double {s}_factor = sp.cs_{s}_factor;");
    }

    let transition = streaming::build_transition(model, &JavaStreamNames)
        .unwrap_or_else(|e| panic!("streaming transition: {e}"));
    let empty = HashSet::new();
    let ctx = stream_ctx(&empty, counter, stream_fma);
    for s in &transition {
        o.push_str(&render_statement_ctx(s, indent, &ctx, enums, registry, helpers));
    }
}

/// The identity short-circuit at the top of a dual-mode step, above the mode
/// predicate — the one place it belongs, since it holds for the whole function
/// (the arms are marked `identity_hoisted`, so they no longer carry a copy).
fn emit_identity_step_branch(
    o: &mut String,
    model: &StreamModel,
    ctx: &JavaRenderCtx,
    enums: &HashMap<String, EnumDef>,
    registry: &Registry,
    helpers: &HelperRegistry,
    indent: usize,
) {
    if let Some(s) = streaming::identity_step_branch(model, &JavaStreamNames) {
        o.push_str(&render_statement_ctx(&s, indent, ctx, enums, registry, helpers));
    }
}

/// Extrema automatons carry batch-absolute int indices; rebase them by a
/// multiple of the physical ring size long before Integer.MAX_VALUE (mirrors C
/// verbatim — index differences and `& xMask` slots are invariant).
fn emit_extrema_rebase(o: &mut String, model: &StreamModel, indent: usize) {
    if let Some(ex) = model.extrema() {
        let pad = " ".repeat(indent);
        let inner = " ".repeat(indent + 3);
        let mut vars: Vec<String> = vec![model.cursor.clone(), ex.trailing.clone()];
        vars.extend(ex.index_vars.iter().cloned());
        let _ = writeln!(o, "{pad}if( sp.{} >= 1073741824 ) {{", model.cursor);
        let _ = writeln!(
            o,
            "{inner}int rebaseShift = sp.{} & ~sp.xMask;",
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

/// Map a batch return-code variable for the open body. Early SUCCESS returns
/// (the no-data guard AND the Metastock seed-boundary return) become
/// `InsufficientHistory` — the wrapper types it as
/// `InsufficientHistoryException`. Everything else passes through (BAD_PARAM /
/// ALLOC_ERR / INTERNAL_ERROR render natively; `retCode` locals propagate a
/// failed cross-call).
fn map_open_return(v: &str) -> String {
    match v {
        "SUCCESS" | "TA_SUCCESS" => "InsufficientHistory".to_string(),
        other => other.to_string(),
    }
}

/// Transcribe a batch body region for the Java open: output-array writes to
/// `lastValue_*` scalars (Scalar) or kept (Fill), previous-output feedback
/// reads to `lastValue_*` (Scalar), early-success returns mapped, the final
/// top-level return dropped (capture + `return RetCode.Success` replace it),
/// and the body's own dead identity branch deleted (its whole-range copies
/// reference output arrays that do not exist in Scalar mode).
fn build_open_body_java(model: &StreamModel, body: &[Statement]) -> Vec<Statement> {
    let outputs = model.outputs.clone();
    let fb_outputs = model.out_feedback.clone();
    let fe = move |e: Expr| -> Expr {
        match e {
            // Previous-output feedback read, scaled like the writes: at stride 0
            // it reads slot 0, which still holds the previous bar's value.
            Expr::ArrayAccess(nm, idx)
                if fb_outputs.contains(&nm) && streaming::is_prev_output_read(&idx) =>
            {
                Expr::ArrayAccess(nm, Box::new(scale_by_stride(*idx)))
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
            } if outputs.contains(&nm) => Some(Statement::Assign {
                target: Expr::ArrayAccess(nm, Box::new(scale_by_stride(*idx))),
                value,
                compound,
            }),
            Statement::Return { value } => {
                let mapped = match value {
                    Some(Expr::Var(v)) => Some(Expr::Var(map_open_return(&v))),
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
    let body = streaming::strip_identity_branch(&body, model.identity.as_ref());
    streaming::rewrite_stmts(&body, &fe, &fs)
}

/// The open-body emitter: `private RetCode <base>_OpenImpl(sp, in..., startIdx,
/// opts..., outBegIdx, outNBElement, outs..., outStride)` for a merged tier
/// (`Core`), or the exempt tiers' `_OpenImpl` / `_OpenAndFillImpl` signatures. `body` is the
/// transcribed batch region (loop tier: `model.body`; dual-mode:
/// `prologue ++ general arm ++ epilogue`).
#[allow(clippy::too_many_arguments)]
fn emit_open_body(
    o: &mut String,
    func: &FuncDef,
    model: &StreamModel,
    body: &[Statement],
    fields: &[Field],
    step_settings: &BTreeSet<String>,
    stream_fma: &FmaVarSets,
    enums: &HashMap<String, EnumDef>,
    registry: &Registry,
    helpers: &HelperRegistry,
    counter: &Cell<usize>,
) {
    emit_open_body_sig(o, func, OutMode::Core);
    let open_body = cleanup_open_body(&build_open_body_java(model, body), registry);
    emit_open_prologue(o, func, &open_body, model, enums, registry, helpers, counter, stream_fma);
    emit_identity_fast_path(o, func, model, fields, registry, helpers, stream_fma, counter);
    emit_open_region(o, func, &open_body, enums, registry, helpers, counter, stream_fma, &[], &HashSet::new());
    let cur_source = CurSource::StridedArray;
    emit_capture(
        o, func, model, &model.state, step_settings, registry, helpers, stream_fma, counter, Some(cur_source), "",
    );
    let _ = writeln!(o, "      return RetCode.Success;");
    let _ = writeln!(o, "   }}");
}

/// The open-body signature. Scalar: sp + inputs + startIdx + opts. Fill: sp +
/// inputs + opts + batch output tail (no startIdx — pinning bar 0 is what
/// makes the fill bit-exact).
fn emit_open_body_sig(o: &mut String, func: &FuncDef, mode: OutMode) {
    let base = base_name(func);
    let class = stream_class_name(func);
    let mut params: Vec<String> = vec![format!("{class} sp")];
    for input in streaming::input_array_names(func) {
        params.push(format!("double {input}[]"));
    }
    if mode == OutMode::Scalar || mode == OutMode::Core {
        params.push("int startIdx".to_string());
    }
    for p in &func.optional_inputs {
        params.push(format!("{} {}", opt_param_java_type(&p.param_type), p.name));
    }
    let name = match mode {
        // Exempt tiers only: their plain-open body IS their numerics, so it
        // wears the same `_OpenImpl` name a merged tier's `Core` does. The two
        // are never emitted for the same function.
        OutMode::Scalar => format!("{base}_OpenImpl"),
        // The merged worker: every entry point's inputs, plus the stride that
        // selects where the per-bar writes land.
        OutMode::Core => {
            params.push("MInteger outBegIdx".to_string());
            params.push("MInteger outNBElement".to_string());
            for out in &func.outputs {
                params.push(format!("{} {}[]", out_java_type(func, &out.name), out.name));
            }
            params.push("int outStride".to_string());
            format!("{base}_OpenImpl")
        }
        OutMode::Fill => {
            params.push("MInteger outBegIdx".to_string());
            params.push("MInteger outNBElement".to_string());
            for out in &func.outputs {
                params.push(format!("{} {}[]", out_java_type(func, &out.name), out.name));
            }
            format!("{base}_OpenAndFillImpl")
        }
        OutMode::FillInternal => {
            params.insert(1 + streaming::input_array_names(func).len(), "int startIdx".to_string());
            params.push("MInteger outBegIdx".to_string());
            params.push("MInteger outNBElement".to_string());
            for out in &func.outputs {
                params.push(format!("{} {}[]", out_java_type(func, &out.name), out.name));
            }
            format!("{base}_OpenAndFillInternalImpl")
        }
    };
    let _ = writeln!(o, "   private RetCode {name}( {} )\n   {{", params.join(", "));
}

/// Declarations + validation head shared by both open bodies: the transcribed
/// body's VarDecls (address-of / MAType aware, mirroring the batch renderer),
/// Scalar-mode MInteger sinks and `lastValue_*` scalars, history metadata,
/// input-length validation, optional-param validation, Fill-mode aliasing
/// guards (#108 — Java is the one managed backend where `out == in` compiles),
/// private-extra-param locals, candle unpacking, and VarDecl initializations.
#[allow(clippy::too_many_arguments)]
fn emit_open_prologue(
    o: &mut String,
    func: &FuncDef,
    open_body: &[Statement],
    model: &StreamModel,
    enums: &HashMap<String, EnumDef>,
    registry: &Registry,
    helpers: &HelperRegistry,
    counter: &Cell<usize>,
    stream_fma: &FmaVarSets,
) {
    emit_body_decls(o, func, open_body);
    emit_open_head(o, func, &model.outputs);
    emit_open_validation(o, func, OutMode::Core, enums);
    emit_anchor_guard(o);
    emit_extras_and_candle(o, func, open_body, registry, helpers, counter, stream_fma);
    let _ = enums;
}

/// The anchor has to land inside the history — see `c_stream::emit_anchor_guard`
/// for why the transcribed bodies cannot be relied on for this (only 137 of 174
/// carry TA-Lib's "make sure there is still something to evaluate" preamble, and
/// the rest run `while nbBar != 0` on a count that went negative).
fn emit_anchor_guard(o: &mut String) {
    let _ = writeln!(o, "      if( startIdx > endIdx ) {{");
    let _ = writeln!(o, "         outBegIdx.value = 0;");
    let _ = writeln!(o, "         outNBElement.value = 0;");
    let _ = writeln!(o, "         return RetCode.InsufficientHistory;");
    let _ = writeln!(o, "      }}");
}

/// The transcribed body's VarDecls (address-of / MAType aware, mirroring the
/// batch renderer's decl pass), including CIRCBUF prologs.
fn emit_body_decls(o: &mut String, func: &FuncDef, open_body: &[Statement]) {
    // The handle caches every output's last value, and a declined output leaves
    // no array to read it back from — so the guarded store writes it here too.
    for name in super::common::nullable_output_list(func) {
        let _ = writeln!(o, "      {} lastCur_{name} = 0;", out_java_type(func, &name));
    }
    let mut address_of_vars = collect_address_of_vars(open_body);
    let matype_params: HashSet<String> = func
        .optional_inputs
        .iter()
        .filter(|p| matches!(&p.param_type, ParamType::Enum(n) if n == "MAType"))
        .map(|p| p.name.clone())
        .collect();
    let matype_vars = collect_matype_vars(open_body, &matype_params);
    let double_address_of_vars = collect_double_address_of_vars(open_body, &address_of_vars);
    for name in &double_address_of_vars {
        address_of_vars.remove(name);
    }

    for stmt in open_body {
        if let Statement::CircBuf(CircBuf::Prolog { id, layout, static_size }) = stmt {
            for (arr, t) in super::java::circbuf_arrays(id, layout) {
                let elem = if matches!(t, VarType::Integer) { "int" } else { "double" };
                let _ = writeln!(o, "      {elem}[] {arr};");
            }
            let _ = writeln!(o, "      int {id}_Idx = 0;");
            let _ = writeln!(o, "      int maxIdx_{id} = ({static_size})-1;");
            continue;
        }
        if let Statement::VarDecl { var_type, name, .. } = stmt {
            let java_decl = if matype_vars.contains(name) {
                format!("MAType {name}")
            } else if address_of_vars.contains(name)
                && matches!(var_type, VarType::Integer | VarType::Index)
            {
                format!("MInteger {name} = new MInteger()")
            } else if double_address_of_vars.contains(name) {
                format!("double[] {name} = new double[1]")
            } else {
                match var_type {
                    VarType::Real => format!("double {name} = 0"),
                    VarType::Integer | VarType::Index => format!("int {name} = 0"),
                    VarType::RealArray(size) => format!("double[] {name} = new double[{size}]"),
                    VarType::IntArray(size) => format!("int[] {name} = new int[{size}]"),
                    _ => format!("{} {name}", java_type_str(var_type)),
                }
            };
            let _ = writeln!(o, "      {java_decl};");
        }
    }
}

/// Scalar-mode MInteger sinks + `lastValue_*` scalars, history metadata.
fn emit_open_head(o: &mut String, func: &FuncDef, _outputs: &[String]) {
    let inputs = streaming::input_array_names(func);
    let first = &inputs[0];
    // startIdx is a parameter of the core, and the out-meta are real parameters
    // too, so the core declares neither.
    let _ = writeln!(o, "      int historyLen = {first}.length;");
    let _ = writeln!(o, "      int endIdx = historyLen - 1;");
}

/// Input-length + optional-param validation, and the Fill-mode aliasing guards.
fn emit_open_validation(o: &mut String, func: &FuncDef, mode: OutMode, enums: &HashMap<String, EnumDef>) {
    let inputs = streaming::input_array_names(func);
    let first = &inputs[0];
    // The implied index pair first: an opener is a batch call over
    // `[0, historyLen - 1]`, so S1 and S2 are B1 and B2 read on that range and
    // answer the same two codes (docs/error-handling-spec.md 2.3). `historyLen`
    // is the FIRST input's length, so a later input of a different length is an
    // argument disagreement, not an empty history.
    let _ = writeln!(o, "      if( historyLen < 1 ) {{");
    let _ = writeln!(o, "         return RetCode.OutOfRangeStartIndex;");
    let _ = writeln!(o, "      }}");
    // The fill covers bars 0..historyLen-1, so its last bar is an index like any
    // other and MAX_INDEX bounds it too (#180). Without this the streaming
    // entry points would compute over exactly the ranges the batch call refuses,
    // and the two are required to agree bit for bit.
    let _ = writeln!(o, "      if( historyLen > MAX_INDEX + 1 ) {{");
    let _ = writeln!(o, "         return RetCode.OutOfRangeEndIndex;");
    let _ = writeln!(o, "      }}");
    let mismatches: Vec<String> = inputs[1..]
        .iter()
        .map(|extra| format!("{extra}.length != {first}.length"))
        .collect();
    if !mismatches.is_empty() {
        let _ = writeln!(o, "      if( {} ) {{", mismatches.join(" || "));
        let _ = writeln!(o, "         return RetCode.BadParam;");
        let _ = writeln!(o, "      }}");
    }
    o.push_str(&emit_opt_param_validation(func, "RetCode.BadParam", enums));
    if mode == OutMode::Fill {
        // FILL ONLY, and exempt tiers only: a merged tier carries this guard in
        // its public `OpenAndFill`, which throws directly rather than answering
        // a code (see `emit_open_wrappers`). The two exempt tiers hand-roll a
        // RetCode-returning fill body, so theirs stays here.
        if let Some(cond) = alias_condition(func) {
            let _ = writeln!(o, "      if( {cond} ) {{");
            let _ = writeln!(o, "         return RetCode.BadParam;");
            let _ = writeln!(o, "      }}");
        }
    }
}

/// Private-extra-param locals (after default substitution) + candle unpacking
/// (batch-verbatim: open reads the live settings; only the step reads the
/// snapshot).
fn emit_extras_and_candle(
    o: &mut String,
    func: &FuncDef,
    open_body: &[Statement],
    registry: &Registry,
    helpers: &HelperRegistry,
    counter: &Cell<usize>,
    stream_fma: &FmaVarSets,
) {
    let empty = HashSet::new();
    let ctx = stream_ctx(&empty, counter, stream_fma);
    for (name, c_type) in &func.private_extra_params {
        let init = func
            .private_param_init
            .iter()
            .find(|(pn, _)| pn == name)
            .map_or_else(
                || panic!("{}: no init for private param {name}", func.name),
                |(_, e)| render_expr(e, &ctx, registry, helpers),
            );
        let _ = writeln!(o, "      {} {name} = {init};", extra_param_java_type(c_type));
    }
    let candle_used = detect_candle_settings(open_body);
    if !candle_used.is_empty() {
        o.push_str(&crate::candle_settings::emit_java_unpacking(&candle_used, 6));
    }
}

/// This backend's IR cleanup sequence, explicit so a pass can be made
/// conditional later. C states none: every one of these would be wrong there.
///
/// Run where the open body is BUILT, not where it is rendered: the declarations
/// and the statements have to be derived from the same body, or a local whose
/// only address-of use sits inside a folded guard is declared wrapped and used
/// plain (issue #271 item 5). Every pass is length-preserving, so the callers'
/// `inserts` / `replaced` statement indices survive it.
///
/// `InsufficientHistory` is what the surviving `count == 0` half of a folded
/// guard answers — an opener that produced nothing is rule S7, not a success.
/// Spelled as the RetCode member, the same name `map_open_return` hands the
/// renderer for the body's own early-success returns.
fn cleanup_open_body(body: &[Statement], registry: &Registry) -> Vec<Statement> {
    let admits = |f: &str, a: &[Expr]| super::java::cross_call_split(f, a, registry).is_some();
    let folded =
        super::ir_cleanup::drop_answered_cross_call_guards(body, &admits, Some("InsufficientHistory"));
    let folded = super::ir_cleanup::drop_deallocation(&folded);
    super::ir_cleanup::drop_inert_guards(&folded)
}

/// Render the transcribed open region: VarDecl initializations then the
/// statements, with tier inserts (composed sub-opens) spliced by index.
#[allow(clippy::too_many_arguments)]
fn emit_open_region(
    o: &mut String,
    func: &FuncDef,
    open_body: &[Statement],
    enums: &HashMap<String, EnumDef>,
    registry: &Registry,
    helpers: &HelperRegistry,
    counter: &Cell<usize>,
    stream_fma: &FmaVarSets,
    inserts: &[(usize, String)],
    replaced: &HashSet<usize>,
) {
    let mut address_of_vars = collect_address_of_vars(open_body);
    let matype_params: HashSet<String> = func
        .optional_inputs
        .iter()
        .filter(|p| matches!(&p.param_type, ParamType::Enum(n) if n == "MAType"))
        .map(|p| p.name.clone())
        .collect();
    let _ = collect_matype_vars(open_body, &matype_params);
    let double_address_of_vars = collect_double_address_of_vars(open_body, &address_of_vars);
    for name in &double_address_of_vars {
        address_of_vars.remove(name);
    }
    let empty = HashSet::new();
    let nullable = super::common::nullable_output_names(func);
    let ctx = JavaRenderCtx {
        single_precision: false,
        nullable_outputs: &nullable,
        nullable_shadow: true,
        address_of_vars: &address_of_vars,
        double_address_of_vars: &double_address_of_vars,
        float_input_params: &empty,
        inline_counter: counter,
        fma: Some(stream_fma),
        matype_map: HashMap::new(),
    };

    // VarDecl initializations (mirrors gen_func_inner).
    for stmt in open_body {
        if let Statement::VarDecl { name, init: Some(init), .. } = stmt {
            let mut hoisted_vec = Vec::new();
            let mut cnt = counter.get();
            let new_init = hoist_block_helpers(init, helpers, &mut hoisted_vec, &mut cnt, JAVA_CANDLE_FNS);
            counter.set(cnt);
            o.push_str(&render_hoisted_blocks(&hoisted_vec, 6, &ctx, enums, registry, helpers));
            let init_str = render_expr(&new_init, &ctx, registry, helpers);
            if address_of_vars.contains(name) {
                let _ = writeln!(o, "      {name}.value = {init_str};");
            } else if double_address_of_vars.contains(name) {
                let _ = writeln!(o, "      {name}[0] = {init_str};");
            } else {
                let _ = writeln!(o, "      {name} = {init_str};");
            }
        }
    }

    for (i, stmt) in open_body.iter().enumerate() {
        // Composed tier: sub-stream opens splice in IMMEDIATELY before the
        // batch call that consumes their series (order is the contract —
        // in-place smoothing overwrites the raw series right after).
        for (at, text) in inserts {
            if *at == i {
                o.push_str(text);
            }
        }
        // A fused sub-open produced this statement's outputs already.
        if matches!(stmt, Statement::VarDecl { .. }) || replaced.contains(&i) {
            continue;
        }
        o.push_str(&render_statement_ctx(stmt, 6, &ctx, enums, registry, helpers));
    }
}

/// The param==1 identity fast path in the open head (mirrors C/Rust).
#[allow(clippy::too_many_arguments)]
fn emit_identity_fast_path(
    o: &mut String,
    func: &FuncDef,
    model: &StreamModel,
    fields: &[Field],
    registry: &Registry,
    helpers: &HelperRegistry,
    stream_fma: &FmaVarSets,
    counter: &Cell<usize>,
) {
    let Some(idp) = &model.identity else { return };
    let base = base_name(func);
    let empty = HashSet::new();
    let ctx = stream_ctx(&empty, counter, stream_fma);
    let cond = render_expr(&idp.condition, &ctx, registry, helpers);
    let lb_args: Vec<String> = func.optional_inputs.iter().map(|p| p.name.clone()).collect();
    let lb_call = format!("{base}_Lookback({})", lb_args.join(", "));
    let _ = writeln!(o, "      if( {cond} ) {{");
    // batch( startIdx, .. ) begins at max(startIdx, lookback), and the anchored
    // `_Open*Internal` variants are the batch call over that same range. The
    // public entry points pass 0, so the clamp is a no-op for them — it is the
    // composition seams that were reporting (and filling) from the raw lookback.
    let _ = writeln!(o, "         int fillLb = {lb_call};");
    let _ = writeln!(o, "         if( startIdx > fillLb ) fillLb = startIdx;");
    let _ = writeln!(o, "         if( historyLen < fillLb + 1 ) {{");
    let _ = writeln!(o, "            return RetCode.InsufficientHistory;");
    let _ = writeln!(o, "         }}");
    // Identity state: params captured, everything else deterministic defaults
    // (1-slot buffers keep the transition's cap-0 guard well-defined).
    for (name, _, default) in fields {
        if name == "cachedValue" || name.starts_with("cur_") {
            continue;
        }
        if model.parity.as_ref().is_some_and(|p| &p.field == name) {
            let _ = writeln!(o, "         sp.{name} = historyLen % 2;");
        } else {
            let _ = writeln!(o, "         sp.{name} = {default};");
        }
    }
    // Fill the whole identity range: at stride 1 this is batch(0, len-1) for the
    // identity param. Stride 0 short-circuits to the last bar — letting the loop
    // run would be CORRECT but would make the scalar Open O(history) where it is
    // O(1), and here there is no inliner guarantee: a cold Open runs it in full.
    let _ = writeln!(o, "         outBegIdx.value = fillLb;");
    let _ = writeln!(o, "         outNBElement.value = historyLen - fillLb;");
    let _ = writeln!(o, "         if( outStride == 0 ) {{");
    for (out, inp) in &idp.pairs {
        let _ = writeln!(o, "            {out}[0] = {inp}[historyLen - 1];");
    }
    let _ = writeln!(o, "         }} else {{");
    let _ = writeln!(o, "            for( int fillIdx = 0; fillIdx < historyLen - fillLb; fillIdx++ ) {{");
    for (out, inp) in &idp.pairs {
        let _ = writeln!(o, "               {out}[fillIdx] = {inp}[fillLb + fillIdx];");
    }
    let _ = writeln!(o, "            }}");
    let _ = writeln!(o, "         }}");
    for (out, _inp) in &idp.pairs {
        let _ = writeln!(o, "         sp.cur_{out} = {out}[(outNBElement.value - 1) * outStride];");
    }
    if has_value_class(func) {
        let _ = writeln!(o, "         sp.cachedValue = {};", capture_value_expr(func));
    }
    let _ = writeln!(o, "         return RetCode.Success;");
    let _ = writeln!(o, "      }}");
}

/// `new Value(sp.cur_a, ...)` for the capture sites (Value resolves inside the
/// nested handle class; from Core scope it needs the class qualifier).
fn capture_value_expr(func: &FuncDef) -> String {
    let class = stream_class_name(func);
    let args: Vec<String> = func
        .outputs
        .iter()
        .map(|out| format!("sp.cur_{}", out.name))
        .collect();
    format!("new {class}.Value({})", args.join(", "))
}

// ---------------------------------------------------------------------------
// State capture
// ---------------------------------------------------------------------------

/// Derived ring (#229): `open` evaluates f(bar) over the history instead of
/// copying a raw column, which no longer exists under that name. Mirrors
/// `derived_fill_expr` (C) and `derived_fill_expr_rust`.
fn derived_fill_expr_java(
    dr: &streaming::DerivedRing,
    idx_var: &str,
    ctx: &JavaRenderCtx,
    registry: &Registry,
    helpers: &HelperRegistry,
) -> String {
    render_expr(&streaming::derived_fill_value(dr, idx_var), ctx, registry, helpers)
}

/// The capture epilogue: compute ring/window/extrema capacities NUMERICALLY
/// from the still-live batch locals (int arithmetic — C's int, no widening
/// needed in Java), build the buffers, then store every handle field.
/// CIRCBUF capture MOVES the batch-materialized storage reference (contents
/// AND rotation phase — the CCI-class summation-order requirement).
#[allow(clippy::too_many_arguments, clippy::too_many_lines)]
fn emit_capture(
    o: &mut String,
    func: &FuncDef,
    model: &StreamModel,
    scalars: &[(String, VarType)],
    step_settings: &BTreeSet<String>,
    registry: &Registry,
    helpers: &HelperRegistry,
    stream_fma: &FmaVarSets,
    counter: &Cell<usize>,
    cur_source: Option<CurSource>,
    extra_capture: &str,
) {
    let _ = writeln!(o, "      /* Capture the live batch state into the handle. */");

    for ring in model.rings() {
        let v = &ring.var;
        let back = ring.back;
        let c = &model.cursor;
        if back > 0 {
            let _ = writeln!(o, "      int capLag_{v} = {c} - {v};");
            let _ = writeln!(o, "      int cap_{v} = capLag_{v} + {};", back + 1);
            let _ = writeln!(
                o,
                "      if( capLag_{v} < {fwd} || cap_{v} > historyLen ) {{",
                fwd = ring.fwd
            );
            let _ = writeln!(o, "         return RetCode.InternalError;");
            let _ = writeln!(o, "      }}");
        } else {
            let _ = writeln!(o, "      int cap_{v} = {c} - {v};");
            let _ = writeln!(o, "      if( cap_{v} < 0 || cap_{v} > historyLen ) {{");
            let _ = writeln!(o, "         return RetCode.InternalError;");
            let _ = writeln!(o, "      }}");
        }
        let _ = writeln!(o, "      int allocN_{v} = (cap_{v} > 0)? cap_{v} : 1;");
        for arr in &ring.arrays {
            let _ = writeln!(o, "      double[] capRing_{v}_{arr} = new double[allocN_{v}];");
            // A derived ring stores f(bar); the fill VALUE changes but the slot
            // arithmetic must not -- see the note in `rust_stream.rs` (#229).
            let fill_rhs = ring.derived.as_ref().map(|dr| {
                let empty_fill = HashSet::new();
                let fill_ctx = stream_ctx(&empty_fill, counter, stream_fma);
                derived_fill_expr_java(dr, "fillJ", &fill_ctx, registry, helpers)
            });
            if back > 0 {
                // Absolute-mod layout: bar j lives at j % cap.
                let rhs = fill_rhs
                    .clone()
                    .unwrap_or_else(|| format!("{arr}[fillJ]"));
                let _ = writeln!(
                    o,
                    "      for( int fillJ = historyLen - cap_{v}; fillJ < historyLen; fillJ++ ) {{"
                );
                let _ = writeln!(o, "         capRing_{v}_{arr}[fillJ % cap_{v}] = {rhs};");
                let _ = writeln!(o, "      }}");
            } else if let Some(rhs) = fill_rhs {
                let _ = writeln!(
                    o,
                    "      for( int fillJ = historyLen - cap_{v}; fillJ < historyLen; fillJ++ ) {{"
                );
                let _ = writeln!(
                    o,
                    "         capRing_{v}_{arr}[fillJ - (historyLen - cap_{v})] = {rhs};"
                );
                let _ = writeln!(o, "      }}");
            } else {
                let _ = writeln!(
                    o,
                    "      System.arraycopy({arr}, historyLen - cap_{v}, capRing_{v}_{arr}, 0, cap_{v});"
                );
            }
        }
    }
    for win in model.windows() {
        let v = &win.var;
        let empty = HashSet::new();
        let ctx = stream_ctx(&empty, counter, stream_fma);
        let cap = render_expr(&win.cap, &ctx, registry, helpers);
        let _ = writeln!(o, "      int cap_{v} = (int)({cap});");
        let _ = writeln!(o, "      if( cap_{v} < 1 || cap_{v} > historyLen ) {{");
        let _ = writeln!(o, "         return RetCode.InternalError;");
        let _ = writeln!(o, "      }}");
        for arr in &win.arrays {
            let _ = writeln!(o, "      double[] capWin_{v}_{arr} = new double[cap_{v}];");
            let _ = writeln!(
                o,
                "      System.arraycopy({arr}, historyLen - cap_{v}, capWin_{v}_{arr}, 0, cap_{v});"
            );
        }
    }
    if let Some(ex) = model.extrema() {
        let c = &model.cursor;
        let t = &ex.trailing;
        let _ = writeln!(o, "      int capX = {c} - {t} + 1;");
        let _ = writeln!(o, "      if( capX < 1 || capX > historyLen ) {{");
        let _ = writeln!(o, "         return RetCode.InternalError;");
        let _ = writeln!(o, "      }}");
        // The slot map is a mask, so the ring is allocated at the next power of
        // two at or above the logical capacity: `idx & xMask` then equals
        // `idx % physX`, still injective over any capX consecutive bars.
        let _ = writeln!(o, "      int physX = 1;");
        let _ = writeln!(o, "      while( physX < capX ) {{");
        let _ = writeln!(o, "         physX <<= 1;");
        let _ = writeln!(o, "      }}");
        for arr in &ex.arrays {
            let _ = writeln!(o, "      double[] capX_{arr} = new double[physX];");
        }
        // Absolute slots: bar j lives at j & (physX-1) (a plain tail copy would
        // break the automaton's phase).
        let _ = writeln!(o, "      for( int fillJ = historyLen - capX; fillJ < historyLen; fillJ++ ) {{");
        for arr in &ex.arrays {
            let _ = writeln!(o, "         capX_{arr}[fillJ & (physX - 1)] = {arr}[fillJ];");
        }
        let _ = writeln!(o, "      }}");
    }
    for circ in model.circs() {
        let id = &circ.id;
        let _ = writeln!(o, "      int capCb_{id} = maxIdx_{id} + 1;");
        let _ = writeln!(o, "      if( capCb_{id} > historyLen + 1 ) {{");
        let _ = writeln!(o, "         return RetCode.InternalError;");
        let _ = writeln!(o, "      }}");
    }

    // --- field stores --------------------------------------------------------
    for p in &func.optional_inputs {
        let _ = writeln!(o, "      sp.{0} = {0};", p.name);
    }
    for (name, _) in &func.private_extra_params {
        let _ = writeln!(o, "      sp.{name} = {name};");
    }
    for (name, _ty) in scalars {
        if model.parity.as_ref().is_some_and(|p| &p.field == name) {
            // Synthetic parity field: seeded to the NEXT bar's parity.
            let _ = writeln!(o, "      sp.{name} = historyLen % 2;");
        } else {
            let _ = writeln!(o, "      sp.{name} = {name};");
        }
    }
    for name in &model.out_feedback {
        // At stride 0 this resolves to slot 0 — the scalar sink — so the one
        // expression serves both entry points.
        let _ = writeln!(
            o,
            "      sp.lastOut_{name} = {name}[(outNBElement.value - 1) * outStride];"
        );
    }
    for lag in &model.lags {
        for k in 1..=lag.depth {
            let _ = writeln!(
                o,
                "      sp.{} = {}[historyLen - {k}];",
                StreamModel::lag_field(&lag.array, k),
                lag.array
            );
        }
    }
    for ring in model.rings() {
        let v = &ring.var;
        if ring.back > 0 {
            let _ = writeln!(o, "      sp.ringPos_{v} = historyLen % cap_{v};");
            let _ = writeln!(o, "      sp.ringCap_{v} = cap_{v};");
            let _ = writeln!(o, "      sp.ringLag_{v} = capLag_{v};");
        } else {
            let _ = writeln!(o, "      sp.ringPos_{v} = 0;");
            let _ = writeln!(o, "      sp.ringCap_{v} = cap_{v};");
        }
        for arr in &ring.arrays {
            let _ = writeln!(o, "      sp.ring_{v}_{arr} = capRing_{v}_{arr};");
        }
    }
    for win in model.windows() {
        let v = &win.var;
        let _ = writeln!(o, "      sp.winPos_{v} = 0;");
        let _ = writeln!(o, "      sp.winCap_{v} = cap_{v};");
        for arr in &win.arrays {
            let _ = writeln!(o, "      sp.win_{v}_{arr} = capWin_{v}_{arr};");
        }
    }
    for circ in model.circs() {
        let id = &circ.id;
        let _ = writeln!(o, "      sp.cbSize_{id} = capCb_{id};");
        for (storage, _) in streaming::circ_storages(circ) {
            // MOVE the live batch buffer (contents AND rotation phase).
            let _ = writeln!(o, "      sp.cb_{storage} = {storage};");
        }
    }
    if let Some(ex) = model.extrema() {
        let _ = writeln!(o, "      sp.xMask = physX - 1;");
        for arr in &ex.arrays {
            let _ = writeln!(o, "      sp.x_{arr} = capX_{arr};");
        }
    }
    for s in step_settings {
        let _ = writeln!(o, "      sp.cs_{s}_rangeType = {s}_rangeType;");
        let _ = writeln!(o, "      sp.cs_{s}_avgPeriod = {s}_avgPeriod;");
        let _ = writeln!(o, "      sp.cs_{s}_factor = {s}_factor;");
    }
    o.push_str(extra_capture);
    // The composed tier seeds `cur_*` from the FUNCTION outputs itself (the
    // producer model's "output" is the intermediate series, not a real one).
    if let Some(cs) = cur_source {
        emit_cur_capture(o, func, &model.outputs, cs);
    }
}

/// Where the open seeds the handle's `cur_*` fields from.
#[derive(Clone, Copy)]
enum CurSource {
    /// The merged core's output slot, stride-scaled: the caller array's last
    /// valid element at stride 1, the one-element sink's slot 0 at stride 0.
    StridedArray,
    /// The composed tier's `sc_<out>` scratch array.
    Scratch,
}

/// Seed `sp.cur_*` (+ the cached Value) at the end of an open body.
fn emit_cur_capture(o: &mut String, func: &FuncDef, outputs: &[String], source: CurSource) {
    let nullable = super::common::nullable_output_names(func);
    assert!(
        !(matches!(source, CurSource::Scratch) && outputs.iter().any(|o| nullable.contains(o))),
        "{}: a composed open would cache 0 for a declined output — the shadow is written \
         beside a transcribed store, and this path has none",
        func.name
    );
    for out in outputs {
        let expr = if nullable.contains(out) {
            format!("lastCur_{out}")
        } else {
            match source {
                CurSource::StridedArray => format!("{out}[(outNBElement.value - 1) * outStride]"),
                CurSource::Scratch => format!("sc_{out}[outNBElement.value - 1]"),
            }
        };
        let _ = writeln!(o, "      sp.cur_{out} = {expr};");
    }
    if has_value_class(func) {
        let _ = writeln!(o, "      sp.cachedValue = {};", capture_value_expr(func));
    }
}

// ---------------------------------------------------------------------------
// Public wrappers
// ---------------------------------------------------------------------------

/// The reject-conversion tail shared by openInternal / openAndFill: stable
/// message prefix, typed insufficient-history, IllegalState for capture
/// invariants, IllegalArgument for everything else.
fn emit_reject_conversion(o: &mut String, func: &FuncDef, what: &str) {
    let n = func.name.to_uppercase();
    let _ = writeln!(o, "      if( retCode == RetCode.Success ) {{");
    let _ = writeln!(o, "         return sp;");
    let _ = writeln!(o, "      }}");
    let _ = writeln!(o, "      if( retCode == RetCode.InsufficientHistory ) {{");
    let _ = writeln!(
        o,
        "         throw new InsufficientHistoryException(\"{n} {what}: history shorter than lookback + 1\");"
    );
    let _ = writeln!(o, "      }}");
    let _ = writeln!(o, "      if( retCode == RetCode.InternalError ) {{");
    let _ = writeln!(o, "         throw new TaLibStateException(\"{n} {what}: internal error\", retCode);");
    let _ = writeln!(o, "      }}");
    // Carrying, like every other failure the library raises: the code has to be
    // recoverable from the thrown object on THIS ladder too, or "total" is a
    // claim about the batch tier wearing the name of the whole library (#236).
    let _ = writeln!(o, "      throw new TaLibArgumentException(\"{n} {what}: \" + retCode, retCode);");
}

/// `<base>_OpenInternal`: the `startIdx`-anchored plain open, package-private.
///
/// It is the composition seam for a scalar sub-open, and the entry the JSON-RPC
/// server drives to check an anchored range against the batch — 176 call sites,
/// one per function, which is why it is emitted for every tier and not only the
/// ones something composes over.
fn emit_open_internal_seam(
    o: &mut String,
    func: &FuncDef,
    merged: bool,
    in_sig: &[String],
    in_fwd: &[String],
    opt_sig_str: &str,
    opt_fwd_str: &str,
) {
    let base = base_name(func);
    let class = stream_class_name(func);
    let _ = writeln!(
        o,
        "   /* Internal startIdx-anchored open behind {base}_Open (composition seam). */"
    );
    let _ = writeln!(
        o,
        "   {class} {base}_OpenInternal( {}, int startIdx{opt_sig_str} )\n   {{",
        in_sig.join(", ")
    );
    let _ = writeln!(o, "      {class} sp = new {class}(this);");
    if merged {
        let _ = writeln!(o, "      MInteger outBegIdx = new MInteger();");
        let _ = writeln!(o, "      MInteger outNBElement = new MInteger();");
        let mut args: Vec<String> = vec!["sp".to_string()];
        args.extend(in_fwd.iter().cloned());
        args.push("startIdx".to_string());
        for p in &func.optional_inputs { args.push(p.name.clone()); }
        args.push("outBegIdx".to_string());
        args.push("outNBElement".to_string());
        for out in &func.outputs {
            let t = out_java_type(func, &out.name);
            let _ = writeln!(o, "      {t}[] sink_{} = new {t}[1];", out.name);
            args.push(format!("sink_{}", out.name));
        }
        // Stride 0 lands every per-bar write on slot 0, so after the replay it
        // holds the last history value — what `sp.cur_*` is seeded from.
        args.push("0".to_string());
        let _ = writeln!(o, "      RetCode retCode = {base}_OpenImpl({});", args.join(", "));
        // The numerics report their range through the pair whatever the stride,
        // so the plain open gets the same numbers a fill would — read back
        // rather than re-derived from the lookback (issue #241).
        let _ = writeln!(o, "      sp.outRangeBegIdx = outBegIdx.value;");
        let _ = writeln!(o, "      sp.outRangeCount = outNBElement.value;");
    } else {
        let _ = writeln!(
            o,
            "      RetCode retCode = {base}_OpenImpl(sp, {}, startIdx{opt_fwd_str});",
            in_fwd.join(", ")
        );
    }
    emit_reject_conversion(o, func, "open");
    let _ = writeln!(o, "   }}");

}

/// Rule S4 at the PUBLIC opener, with the implied index pair (S1/S2) in the
/// middle of it.
///
/// **Exactly one presence check precedes the pair**: the FIRST input's, because
/// that is the array `historyLen` is read from and a length cannot be taken from
/// an array that is not there. Every other argument — the remaining price legs
/// included — is checked after, which is the specified order
/// (`docs/error-handling-spec.md` §2.3, footnote [4]). Checking them all up
/// front reads as tidier and is wrong: a candlestick opened on an empty history
/// with one null leg would report the leg, where C reports the empty history.
///
/// `<N>_OpenImpl` answers the pair too; this runs first only to fix the ORDER,
/// exactly as `Core.requireIndexRange` does in the batch tier. Without it a
/// null output pre-empted the empty history, and a null input reached
/// `inReal.length` and surfaced as a bare `NullPointerException` naming nothing.
fn emit_public_open_guards(o: &mut String, func: &FuncDef, verb: &str, with_outputs: bool) {
    let n = func.name.to_uppercase();
    let inputs = streaming::input_array_names(func);
    let history = &inputs[0];
    let _ = writeln!(o, "      requireArgument(\"{n} {verb}\", \"{history}\", {history});");
    let _ = writeln!(o, "      requireHistory(\"{n} {verb}\", {history}.length);");
    // An enum parameter's domain excludes null, and Java is the only backend
    // where that is expressible (rule S3). It precedes the remaining presence
    // checks because S3 precedes S4 — the order the batch wrapper was given in
    // Appendix D items 2 and 3 — and it must in any case precede the
    // `_Lookback` call below, which is where a null one is first switched on.
    for p in &func.optional_inputs {
        if matches!(p.param_type, ParamType::Enum(_)) {
            let _ = writeln!(
                o,
                "      requireArgument(\"{n} {verb}\", \"{0}\", {0});",
                p.name
            );
        }
    }
    for input in &inputs[1..] {
        let _ = writeln!(o, "      requireArgument(\"{n} {verb}\", \"{input}\", {input});");
    }
    if with_outputs {
        // Rule S3 first, in the shape the buffer rules need it: `<N>_Lookback`
        // answers `-1` for an out-of-domain parameter and `openFillCount` raises
        // on it, so a bad parameter is reported as a bad parameter rather than
        // as whatever the buffer rules would have said about a call it made no
        // sense to size. Same thing `clampedStart` does one tier over.
        let lb_args: Vec<String> = func.optional_inputs.iter().map(|p| p.name.clone()).collect();
        let _ = writeln!(
            o,
            "      int guardOutLen = openFillCount(\"{n} {verb}\", {}.length, {n}_Lookback({}));",
            history,
            lb_args.join(", ")
        );
        // Rule S5, input half before output half — B5 states the two as one
        // rule, in that order, and this is B5 over `[0, historyLen - 1]`. The
        // core tests the inputs too, but only after the capacity bound below
        // would have answered, so a short input series was reported as an
        // output-capacity fault.
        o.push_str(&history_length_guards(func, &n, verb));
        // …then the outputs. `requireLength` carries S4 and S5 in one call,
        // exactly as the batch wrapper's does, and an output marked `nullable`
        // is bounded only where it was supplied (rule B6a).
        let nullable = super::common::nullable_output_names(func);
        for out in &func.outputs {
            let guard = if nullable.contains(&out.name) {
                format!("if( {0} != null ) ", out.name)
            } else {
                String::new()
            };
            let _ = writeln!(
                o,
                "      {guard}requireLength(\"{n} {verb}\", \"{0}\", {0}, guardOutLen);",
                out.name
            );
        }
    } else {
        // Rule S5's input half, which is the whole of S5 here — the plain open
        // writes nothing, so there is no output capacity to bound. It belongs
        // on this frame for the same reason the fill's does: the core makes the
        // test, but answers it as a bare `BadParam` naming nothing, where the
        // same fault at `OpenAndFill` named the leg (issue #271 item 1).
        o.push_str(&history_length_guards(func, &n, verb));
    }
}

/// Rule S5's input half, the same at both openers: the history's own length IS
/// the range, so every other declared input must AGREE with it rather than
/// merely reach it.
fn history_length_guards(func: &FuncDef, n: &str, verb: &str) -> String {
    let inputs = streaming::input_array_names(func);
    let history = &inputs[0];
    let mut s = String::new();
    for input in &inputs[1..] {
        let _ = writeln!(
            s,
            "      requireHistoryLength(\"{n} {verb}\", \"{input}\", {input}.length, {history}.length);"
        );
    }
    s
}

/// The javadoc sentence naming the outputs a caller may decline, or nothing when
/// the function has none. Rule B6a reads the same at both tiers, and a caller of
/// the opener needs telling in the same place a caller of the batch call is told.
fn declinable_note(func: &FuncDef, class: &str) -> String {
    let names = super::common::nullable_output_list(func);
    if names.is_empty() {
        return String::new();
    }
    let list = names.iter().map(|n| format!("{{@code {n}}}")).collect::<Vec<_>>().join(", ");
    format!(
        "\n\x20   * <p>{list} may be declined with {{@code null}}: the value is still\n\
         \x20   * computed — {{@link {class}#value()}} reports it — and nothing is written out."
    )
}

/// `openInternal` (the anchored plain open), the public `<base>_Open`, and the
/// public `<base>_OpenAndFill`.
///
/// `merged` says whether this function owns a `Core` — one stride-parameterized
/// `<base>_OpenImpl` that every entry point reaches. When it does, the two
/// package-private seams ARE the implementation: `_OpenInternal` synthesizes a
/// one-element sink per output and calls the numerics at stride 0, and the
/// public `_OpenAndFill` hoists the aliasing guard and then delegates to
/// `_OpenAndFillInternal`, exactly the way `_Open` delegates to `_OpenInternal`.
/// That symmetry is what makes the anchored fill seam reachable for every
/// function rather than only the sixteen something composes over.
///
/// The two exempt tiers (`Dispatch`, `PeriodBank`) hand-roll a RetCode-returning
/// body per entry point — theirs differ by which callee tier they call and by an
/// anchor clamp, not by a stride — so their wrappers stay thin over those.
fn emit_open_wrappers(o: &mut String, func: &FuncDef, merged: bool) {
    let base = base_name(func);
    let class = stream_class_name(func);
    let n = func.name.to_uppercase();

    let mut in_sig: Vec<String> = Vec::new();
    let mut in_fwd: Vec<String> = Vec::new();
    for input in streaming::input_array_names(func) {
        in_sig.push(format!("double {input}[]"));
        in_fwd.push(input.clone());
    }
    let mut opt_sig: Vec<String> = Vec::new();
    let mut opt_fwd: Vec<String> = Vec::new();
    for p in &func.optional_inputs {
        opt_sig.push(format!("{} {}", opt_param_java_type(&p.param_type), p.name));
        opt_fwd.push(p.name.clone());
    }
    let opt_sig_str = if opt_sig.is_empty() { String::new() } else { format!(", {}", opt_sig.join(", ")) };
    let opt_fwd_str = if opt_fwd.is_empty() { String::new() } else { format!(", {}", opt_fwd.join(", ")) };

    emit_open_internal_seam(o, func, merged, &in_sig, &in_fwd, &opt_sig_str, &opt_fwd_str);

    // Public open.
    let _ = writeln!(
        o,
        "   /**\n\
         \x20   * Open a live {n} stream over the warm-up history; the handle's\n\
         \x20   * {{@code value()}} starts at the last history bar's value — bit-identical\n\
         \x20   * to {{@link Core#{base}}} at that bar.\n\
         \x20   * <p>The history must hold at least {{@code {base}_Lookback(...) + 1}} bars\n\
         \x20   * (unstable-period aware), or {{@link InsufficientHistoryException}} is\n\
         \x20   * thrown. Out-of-range parameters throw {{@link IllegalArgumentException}}\n\
         \x20   * ({{@code Integer.MIN_VALUE}} selects an integer parameter's documented\n\
         \x20   * default, as in the batch API). An EMPTY history throws\n\
         \x20   * {{@link IndexOutOfBoundsException}} — its implied {{@code startIdx}} of 0\n\
         \x20   * names no bar — and a null argument {{@link IllegalArgumentException}},\n\
         \x20   * both ahead of everything above.\n\
         \x20   */"
    );
    let _ = writeln!(
        o,
        "   public {class} {base}_Open( {}{opt_sig_str} )\n   {{",
        in_sig.join(", ")
    );
    emit_public_open_guards(o, func, "open", false);
    let _ = writeln!(
        o,
        "      return {base}_OpenInternal({}, 0{opt_fwd_str});",
        in_fwd.join(", ")
    );
    let _ = writeln!(o, "   }}");

    // Public openAndFill. The filled range is reported on the handle
    // (`outRange()`), not through a pair of caller-supplied out-params.
    let mut fill_sig: Vec<String> = in_sig.clone();
    for p in &opt_sig {
        fill_sig.push(p.clone());
    }
    let mut fill_fwd: Vec<String> = in_fwd.clone();
    for p in &opt_fwd {
        fill_fwd.push(p.clone());
    }
    fill_fwd.push("outBegIdx".to_string());
    fill_fwd.push("outNBElement".to_string());
    for out in &func.outputs {
        fill_sig.push(format!("{} {}[]", out_java_type(func, &out.name), out.name));
        fill_fwd.push(out.name.clone());
    }
    let declinable = declinable_note(func, &class);
    let _ = writeln!(
        o,
        "   /**\n\
         \x20   * {{@link Core#{base}_Open}} that also fills the output array(s) bit-identically\n\
         \x20   * to {{@link Core#{base}}} over the whole history in the same single pass\n\
         \x20   * (no separate batch call needed for the warm-up plot). Output arrays must\n\
         \x20   * not alias the inputs or each other, and must hold\n\
         \x20   * {{@code historyLen - lookback}} values — both checked before anything is\n\
         \x20   * written, so an undersized array is an {{@link IllegalArgumentException}}\n\
         \x20   * naming it rather than a fault from inside the fill.{declinable}\n\
         \x20   * <p>The range written is on the returned handle:\n\
         \x20   * {{@link {class}#outRange()}}.\n\
         \x20   */"
    );
    let _ = writeln!(
        o,
        "   public {class} {base}_OpenAndFill( {} )\n   {{",
        fill_sig.join(", ")
    );
    emit_public_open_guards(o, func, "openAndFill", true);
    if merged {
        // The guard the anchored seam deliberately omits: every composed
        // sub-call passes a destination that aliases neither its sources nor
        // each other, so it belongs on the public frame, not the hot one. It
        // throws here rather than answering a code, producing the identical
        // text the shared ladder produced when the deleted fill body returned
        // BadParam into it.
        if let Some(cond) = alias_condition(func) {
            let _ = writeln!(o, "      if( {cond} ) {{");
            let _ = writeln!(
                o,
                "         throw new TaLibArgumentException(\"{n} openAndFill: \" + RetCode.BadParam, RetCode.BadParam);"
            );
            let _ = writeln!(o, "      }}");
        }
        let _ = writeln!(o, "      MInteger outBegIdx = new MInteger();");
        let _ = writeln!(o, "      MInteger outNBElement = new MInteger();");
        let mut args: Vec<String> = in_fwd.clone();
        args.push("0".to_string());
        args.extend(opt_fwd.iter().cloned());
        args.push("outBegIdx".to_string());
        args.push("outNBElement".to_string());
        for out in &func.outputs {
            args.push(out.name.clone());
        }
        let _ = writeln!(
            o,
            "      return {base}_OpenAndFillInternal({});",
            args.join(", ")
        );
    } else {
        let _ = writeln!(o, "      {class} sp = new {class}(this);");
        let _ = writeln!(o, "      MInteger outBegIdx = new MInteger();");
        let _ = writeln!(o, "      MInteger outNBElement = new MInteger();");
        let _ = writeln!(
            o,
            "      RetCode retCode = {base}_OpenAndFillImpl(sp, {});",
            fill_fwd.join(", ")
        );
        let _ = writeln!(o, "      sp.outRangeBegIdx = outBegIdx.value;");
        let _ = writeln!(o, "      sp.outRangeCount = outNBElement.value;");
        emit_reject_conversion(o, func, "openAndFill");
    }
    let _ = writeln!(o, "   }}");
}

// ---------------------------------------------------------------------------
// Dual-mode tier (DI/DM scalar, TRIMA ring): two param-selected arms sharing
// one union handle. Mirrors rust_stream::emit_dual_mode.
// ---------------------------------------------------------------------------

/// The type-checked union of both modes' carried scalars: mode-A order first,
/// dedup by name, conflicting `VarType`s are a hard error. A mode-B-only field
/// is captured from the arm's untouched prologue-declared local under mode A
/// (both are the type default).
fn dual_scalar_union(
    func: &FuncDef,
    ma: &StreamModel,
    mb: &StreamModel,
) -> Vec<(String, VarType)> {
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

/// The union field list of the handle class: mode-A fields first, then
/// mode-B-only fields (HMA: the general arm's half-period ring + d-CIRCBUF
/// array). A name both lists carry must agree on type (mirrors Rust).
fn dual_union_fields(func: &FuncDef, fields_a: &[Field], fields_b: &[Field]) -> Vec<Field> {
    let mut fields: Vec<Field> = fields_a.to_vec();
    let a_types: HashMap<&String, &String> = fields_a.iter().map(|(n, t, _)| (n, t)).collect();
    for f in fields_b {
        if let Some(prev) = a_types.get(&f.0) {
            assert!(
                **prev == f.1,
                "{}: dual-mode field `{}` has conflicting types across modes",
                func.name,
                f.0
            );
        } else {
            fields.push(f.clone());
        }
    }
    fields
}

/// `sp.<field> = <default>;` for the OTHER mode's exclusive ARRAY fields —
/// scalars stay at Java's zero default, but an array left null would NPE in
/// the deep-copy constructor (peek/copy clone every array field).
fn dual_complement_capture(own: &[Field], other: &[Field]) -> String {
    let own_names: HashSet<&String> = own.iter().map(|(n, _, _)| n).collect();
    let mut s = String::new();
    for (name, jty, default) in other {
        if !own_names.contains(name) && jty.ends_with("[]") {
            let _ = writeln!(s, "      sp.{name} = {default};");
        }
    }
    s
}

#[allow(clippy::too_many_arguments)]
fn emit_dual_mode(
    o: &mut String,
    func: &FuncDef,
    dmp: &streaming::DualModePlan,
    stream_fma: &FmaVarSets,
    enums: &HashMap<String, EnumDef>,
    registry: &Registry,
    helpers: &HelperRegistry,
    counter: &Cell<usize>,
) {
    let ma = &dmp.mode_a;
    let mb = &dmp.mode_b;
    let mut step_settings = detect_candle_settings(&ma.steady_stmts);
    step_settings.extend(detect_candle_settings(&mb.steady_stmts));

    let union_scalars = dual_scalar_union(func, ma, mb);
    // The handle carries the UNION of both modes' state: shared fields once
    // (TRIMA's odd/even arms share the very same rings; DI/DM overlap fully),
    // then mode-B-only fields (HMA's half-period ring + d-CIRCBUF array). The
    // mode is fixed at open, so each arm's step touches only its own fields;
    // the inactive mode's arrays are seeded to their 1-slot defaults in the
    // arm capture (the deep-copy constructor clones every array — mirrors
    // C/Rust's union struct).
    let fields_a = state_fields_from(func, ma, &union_scalars, &step_settings);
    let fields_b = state_fields_from(func, mb, &union_scalars, &step_settings);
    let fields = dual_union_fields(func, &fields_a, &fields_b);
    emit_handle_class(o, func, &fields, &SubMembers::none());

    // --- step: one function, the mode re-derived from the stored param ------
    emit_step_sig(o, func);
    let empty = HashSet::new();
    let ctx = stream_ctx(&empty, counter, stream_fma);
    // Identity (HMA period 1) short-circuits ahead of the predicate, as it does
    // in the batch and in the opens: it is a property of the function, not of a
    // mode.
    emit_identity_step_branch(o, ma, &ctx, enums, registry, helpers, 6);
    let pred_sp = params_on_state(func, &dmp.predicate);
    let pred_sp = render_predicate(&pred_sp, &ctx, registry, helpers);
    let _ = writeln!(o, "      if( {pred_sp} ) {{");
    emit_step_body(o, func, ma, &step_settings, stream_fma, enums, registry, helpers, counter, 9);
    let _ = writeln!(o, "      }} else {{");
    emit_step_body(o, func, mb, &step_settings, stream_fma, enums, registry, helpers, counter, 9);
    let _ = writeln!(o, "      }}");
    let _ = writeln!(o, "   }}");

    // --- opens: shared head, then one predicate branch per mode, each
    // transcribing `prologue ++ its arm ++ epilogue` and capturing into the
    // union (both branches return; nothing follows the if/else) --------------
    {
        emit_open_body_sig(o, func, OutMode::Core);
        emit_open_head(o, func, &ma.outputs);
        emit_open_validation(o, func, OutMode::Core, enums);
        // No `emit_anchor_guard` here, unlike the Loop and Composed prologues.
        // Every dual-mode arm already rejects the anchor on its own way in, and
        // for the identity arm the two conditions are the SAME one spelled
        // differently: it clamps `fillLb = max(lookback, startIdx)` and then
        // rejects `historyLen < fillLb + 1`, which at the identity period
        // (lookback 0) reduces to exactly `startIdx > endIdx`. Emitting the
        // prologue guard too would be dead code in 7 bodies per backend. C
        // hoists it instead, which is why the four backends differ in PLACEMENT
        // here and not in behaviour -- checked against every arm of all seven
        // (EFI, HMA, MINUS_DI, MINUS_DM, PLUS_DI, PLUS_DM, TRIMA).
        // Identity (HMA period 1) short-circuits ahead of the predicate: the
        // whole union sits at its defaults, including the arrays only the
        // general arm touches. What keeps that arm from running is the step's
        // own guard, hoisted ABOVE the mode predicate, so which arm the predicate
        // would pick is moot.
        emit_identity_fast_path(o, func, ma, &fields, registry, helpers, stream_fma, counter);
        let pred = render_predicate(&dmp.predicate, &ctx, registry, helpers);
        let _ = writeln!(o, "      if( {pred} ) {{");
        for (k, arm) in [ma, mb].into_iter().enumerate() {
            if k == 1 {
                let _ = writeln!(o, "      }} else {{");
            }
            // prologue ++ this arm's body ++ epilogue — the prologue computes
            // the mode-appropriate lookback/clamp, so min-history is per-mode
            // correct by construction. The other mode's prologue-declared
            // scalars stay in scope for the union capture (their untouched
            // defaults == C's memset zeros).
            let mut body: Vec<Statement> = dmp.prologue.to_vec();
            body.extend_from_slice(arm.body);
            body.extend_from_slice(dmp.epilogue);
            let open_body = cleanup_open_body(&build_open_body_java(arm, &body), registry);
            let mut s = String::new();
            emit_body_decls(&mut s, func, &open_body);
            emit_extras_and_candle(&mut s, func, &open_body, registry, helpers, counter, stream_fma);
            emit_open_region(&mut s, func, &open_body, enums, registry, helpers, counter, stream_fma, &[], &HashSet::new());
            let cur_source = CurSource::StridedArray;
            let (own, other) = if k == 0 { (&fields_a, &fields_b) } else { (&fields_b, &fields_a) };
            let complement = dual_complement_capture(own, other);
            emit_capture(
                &mut s, func, arm, &union_scalars, &step_settings, registry, helpers,
                stream_fma, counter, Some(cur_source), &complement,
            );
            let _ = writeln!(s, "      return RetCode.Success;");
            o.push_str(&indent_block(&s, 3));
        }
        let _ = writeln!(o, "      }}");
        let _ = writeln!(o, "   }}");
    }
    emit_open_and_fill_internal_wrapper(o, func, true);

    emit_open_wrappers(o, func, true);
}

// ---------------------------------------------------------------------------
// Dispatch tier (MA): params + an `Object sub` tagged by the stored enum
// param (the C `void *sub` model — Java has no payload enums at release 9).
// ---------------------------------------------------------------------------

/// `SMA_Stream` for callee `sma` — from the callee's own base name, the same
/// authority as the callee's generated handle.
fn callee_stream_class(registry: &Registry, callee: &str) -> String {
    format!("{}_Stream", registry.name_of(callee))
}

/// `sp.cur_<out>` / `Value` member routing for one forwarded callee slot.
fn callee_value_field(registry: &Registry, callee: &str, slot: usize) -> String {
    value_field_name(&registry.callee_outputs(callee)[slot])
}

/// The per-case body wiring a freshly opened `sub` into the handle: store the
/// sub and copy the forwarded open value(s) off the callee's `cur_*` fields.
fn dispatch_store_sub(
    o: &mut String,
    registry: &Registry,
    arm: &streaming::DispatchArm,
    outputs: &[String],
    pad: &str,
) {
    let _ = writeln!(o, "{pad}sp.sub = sub;");
    for (i, slot) in arm.out_map.iter().enumerate() {
        if let streaming::OutSlot::Forward(k) = slot {
            let _ = writeln!(
                o,
                "{pad}sp.cur_{} = sub.cur_{};",
                outputs[*k],
                registry.callee_outputs(&arm.callee)[i]
            );
        }
    }
}

#[allow(clippy::too_many_lines, clippy::too_many_arguments)]
fn emit_dispatch(
    o: &mut String,
    func: &FuncDef,
    dp: &streaming::DispatchPlan,
    stream_fma: &FmaVarSets,
    enums: &HashMap<String, EnumDef>,
    registry: &Registry,
    helpers: &HelperRegistry,
    counter: &Cell<usize>,
) {
    let outputs: Vec<String> = func.outputs.iter().map(|x| x.name.clone()).collect();
    let inputs = streaming::input_array_names(func);
    let bar_args = inputs.join(", ");
    let base = base_name(func);
    let empty = HashSet::new();
    let mut ctx = stream_ctx(&empty, counter, stream_fma);
    // The dispatch identity guard can compare `optInMAType == TA_MAType_*`
    // (TA_MAType_DISABLED, #93); resolve those to their constant like batch.
    ctx.matype_map = build_matype_map(enums);
    let lb_args: Vec<String> = func.optional_inputs.iter().map(|p| p.name.clone()).collect();
    let lb_call = format!("{base}_Lookback({})", lb_args.join(", "));

    // --- handle class -------------------------------------------------------
    let fields = base_fields(func);
    let extra_members = format!(
        "      // Sub-stream, tagged by {}; null on the identity path.\n      Object sub;\n",
        dp.param
    );
    // Deep copy of the tagged sub: switch on the stored enum param, invoke the
    // callee's copy constructor (generated from the same arm table as the
    // step/open switches, so a new MAType cannot be handled in one and missed
    // in the other).
    let mut copy_extra = String::new();
    let _ = writeln!(copy_extra, "         if( other.sub == null ) {{");
    let _ = writeln!(copy_extra, "            this.sub = null;");
    let _ = writeln!(copy_extra, "         }} else {{");
    let _ = writeln!(copy_extra, "            switch( this.{} )", dp.param);
    let _ = writeln!(copy_extra, "            {{");
    for arm in dp.arms.iter().filter(|a| a.supported) {
        let label = super::java::render_java_switch_label(&arm.label, enums);
        let cls = callee_stream_class(registry, &arm.callee);
        let _ = writeln!(copy_extra, "            case {label}:");
        let _ = writeln!(copy_extra, "               this.sub = new {cls}(({cls}) other.sub);");
        let _ = writeln!(copy_extra, "               break;");
    }
    let _ = writeln!(copy_extra, "            default:");
    let _ = writeln!(
        copy_extra,
        "               throw new IllegalStateException(\"unreachable: open rejects arms without a sub-stream\");"
    );
    let _ = writeln!(copy_extra, "            }}");
    let _ = writeln!(copy_extra, "         }}");
    // The same switch in place: the scratch keeps the sub it already holds when
    // the arm matches. It is only the same arm when the source handle's param
    // is the same, which is why the tag is read off `this` after the field
    // copy, exactly as the copy constructor reads it after its own.
    let mut restore_extra = String::new();
    let _ = writeln!(restore_extra, "         if( other.sub == null ) {{");
    let _ = writeln!(restore_extra, "            this.sub = null;");
    let _ = writeln!(restore_extra, "         }} else {{");
    let _ = writeln!(restore_extra, "            switch( this.{} )", dp.param);
    let _ = writeln!(restore_extra, "            {{");
    for arm in dp.arms.iter().filter(|a| a.supported) {
        let label = super::java::render_java_switch_label(&arm.label, enums);
        let cls = callee_stream_class(registry, &arm.callee);
        let _ = writeln!(restore_extra, "            case {label}:");
        let _ = writeln!(restore_extra, "               if( this.sub instanceof {cls} ) {{");
        let _ = writeln!(
            restore_extra,
            "                  (({cls}) this.sub).copyFrom(({cls}) other.sub);"
        );
        let _ = writeln!(restore_extra, "               }} else {{");
        let _ = writeln!(
            restore_extra,
            "                  this.sub = new {cls}(({cls}) other.sub);"
        );
        let _ = writeln!(restore_extra, "               }}");
        let _ = writeln!(restore_extra, "               break;");
    }
    let _ = writeln!(restore_extra, "            default:");
    let _ = writeln!(
        restore_extra,
        "               throw new IllegalStateException(\"unreachable: open rejects arms without a sub-stream\");"
    );
    let _ = writeln!(restore_extra, "            }}");
    let _ = writeln!(restore_extra, "         }}");
    // The dispatch arm is an `MAType` chosen at run time, so what the sub owns
    // is not knowable here.
    let subs = SubMembers { copy: copy_extra, restore: restore_extra, subs: 1, unbounded: true };
    emit_handle_class_with_members(o, func, &fields, &subs, &extra_members);

    // --- step ---------------------------------------------------------------
    emit_step_sig(o, func);
    if let Some(idp) = &dp.identity {
        let cond = params_on_state(func, &idp.condition);
        let cond = render_predicate(&cond, &ctx, registry, helpers);
        let _ = writeln!(o, "      if( {cond} ) {{");
        for (out, inp) in &idp.pairs {
            let _ = writeln!(o, "         sp.cur_{out} = {inp};");
        }
        let _ = writeln!(o, "         return;");
        let _ = writeln!(o, "      }}");
    }
    let _ = writeln!(o, "      switch( sp.{} )", dp.param);
    let _ = writeln!(o, "      {{");
    for arm in dp.arms.iter().filter(|a| a.supported) {
        let label = super::java::render_java_switch_label(&arm.label, enums);
        let cls = callee_stream_class(registry, &arm.callee);
        let _ = writeln!(o, "      case {label}: {{");
        // Route callee output slots through the arm's OutSlot map: Forward(k)
        // lands in the dispatch func's output k, Discard drops the slot (the
        // nullable FAMA when MA routes only the MAMA line, #125).
        if arm.out_map.len() == 1 {
            let streaming::OutSlot::Forward(k) = arm.out_map[0] else {
                panic!("single-output arm cannot discard its only slot");
            };
            let _ = writeln!(
                o,
                "         sp.cur_{} = (({cls}) sp.sub).update({bar_args});",
                outputs[k]
            );
        } else {
            let _ = writeln!(
                o,
                "         {cls}.Value subValue = (({cls}) sp.sub).update({bar_args});"
            );
            for (i, slot) in arm.out_map.iter().enumerate() {
                if let streaming::OutSlot::Forward(k) = slot {
                    let _ = writeln!(
                        o,
                        "         sp.cur_{} = subValue.{}();",
                        outputs[*k],
                        callee_value_field(registry, &arm.callee, i)
                    );
                }
            }
        }
        let _ = writeln!(o, "         break;");
        let _ = writeln!(o, "      }}");
    }
    let _ = writeln!(o, "      default:");
    let _ = writeln!(o, "         break; /* unreachable: open rejects arms without a sub-stream */");
    let _ = writeln!(o, "      }}");
    let _ = writeln!(o, "   }}");

    // --- open bodies (Scalar delegates to openInternal; Fill to openAndFill) -
    for mode in [OutMode::Scalar, OutMode::Fill, OutMode::FillInternal] {
        emit_open_body_sig(o, func, mode);
        let first = &inputs[0];
        let _ = writeln!(o, "      int historyLen = {first}.length;");
        emit_open_validation(o, func, mode, enums);
        // Own-lookback precheck BEFORE delegating: the callee's open would
        // reject too, but with ITS message prefix ("SMA open:" for a MA call)
        // — the documented stable "<NAME> open:" contract requires the reject
        // to carry this function's name.
        let _ = writeln!(o, "      if( historyLen < {lb_call} + 1 ) {{");
        let _ = writeln!(o, "         return RetCode.InsufficientHistory;");
        let _ = writeln!(o, "      }}");
        if let Some(idp) = &dp.identity {
            // The identity path FIRST (batch order — it applies to every arm).
            let cond = render_predicate(&idp.condition, &ctx, registry, helpers);
            let _ = writeln!(o, "      if( {cond} ) {{");
            let _ = writeln!(o, "         if( historyLen < {lb_call} + 1 ) {{");
            let _ = writeln!(o, "            return RetCode.InsufficientHistory;");
            let _ = writeln!(o, "         }}");
            for p in &func.optional_inputs {
                let _ = writeln!(o, "         sp.{0} = {0};", p.name);
            }
            let _ = writeln!(o, "         sp.sub = null;");
            match mode {
                // The dispatch tier is exempt from the Open merge (it hands the
                // fill to a sub's public OpenAndFill), so it only ever renders
                // the two original modes.
                OutMode::Core => unreachable!("dispatch tier is exempt from the merge"),
                OutMode::Scalar => {
                    for (out, inp) in &idp.pairs {
                        let _ = writeln!(o, "         sp.cur_{out} = {inp}[historyLen - 1];");
                    }
                    // No out-meta pair on this mode, so the range is resolved the
                    // way the batch resolves it (issue #241) — clamped to
                    // startIdx, and then re-checked against the history, or an
                    // anchor past it publishes a negative count.
                    let _ = writeln!(o, "         int fillLb = {lb_call};");
                    let _ = writeln!(o, "         if( startIdx > fillLb ) fillLb = startIdx;");
                    let _ = writeln!(o, "         if( historyLen < fillLb + 1 ) {{");
                    let _ = writeln!(o, "            return RetCode.InsufficientHistory;");
                    let _ = writeln!(o, "         }}");
                    let _ = writeln!(o, "         sp.outRangeBegIdx = fillLb;");
                    let _ = writeln!(o, "         sp.outRangeCount = historyLen - fillLb;");
                }
                OutMode::Fill | OutMode::FillInternal => {
                    let _ = writeln!(o, "         int fillLb = {lb_call};");
                    if mode == OutMode::FillInternal {
                        // batch( startIdx, .. ) begins at max(startIdx, lookback).
                        let _ = writeln!(o, "         if( startIdx > fillLb ) fillLb = startIdx;");
                        let _ = writeln!(o, "         if( historyLen < fillLb + 1 ) {{");
                        let _ = writeln!(o, "            return RetCode.InsufficientHistory;");
                        let _ = writeln!(o, "         }}");
                    }
                    let _ = writeln!(o, "         outBegIdx.value = fillLb;");
                    let _ = writeln!(o, "         outNBElement.value = historyLen - fillLb;");
                    let _ = writeln!(
                        o,
                        "         for( int fillIdx = 0; fillIdx < historyLen - fillLb; fillIdx++ ) {{"
                    );
                    for (out, inp) in &idp.pairs {
                        let _ = writeln!(o, "            {out}[fillIdx] = {inp}[fillLb + fillIdx];");
                    }
                    let _ = writeln!(o, "         }}");
                    for (out, _) in &idp.pairs {
                        let _ = writeln!(o, "         sp.cur_{out} = {out}[outNBElement.value - 1];");
                    }
                }
            }
            if has_value_class(func) {
                let _ = writeln!(o, "         sp.cachedValue = {};", capture_value_expr(func));
            }
            let _ = writeln!(o, "         return RetCode.Success;");
            let _ = writeln!(o, "      }}");
        }
        let _ = writeln!(o, "      switch( {} )", dp.param);
        let _ = writeln!(o, "      {{");
        for arm in &dp.arms {
            let label = super::java::render_java_switch_label(&arm.label, enums);
            if arm.supported {
                let cls = callee_stream_class(registry, &arm.callee);
                let callee_base = registry.name_of(&arm.callee);
                let opts: Vec<String> = arm
                    .opt_args
                    .iter()
                    .map(|e| render_expr(e, &ctx, registry, helpers))
                    .collect();
                let _ = writeln!(o, "      case {label}: {{");
                match mode {
                    OutMode::Core => unreachable!("dispatch tier is exempt from the merge"),
                    OutMode::Scalar => {
                        let opts = if opts.is_empty() {
                            String::new()
                        } else {
                            format!(", {}", opts.join(", "))
                        };
                        let _ = writeln!(
                            o,
                            "         {cls} sub = {callee_base}_OpenInternal({bar_args}, startIdx{opts});"
                        );
                        // The arm's handle already resolved the range; this mode has
                        // no out-meta pair to read it from instead.
                        let _ = writeln!(o, "         sp.outRangeBegIdx = sub.outRangeBegIdx;");
                        let _ = writeln!(o, "         sp.outRangeCount = sub.outRangeCount;");
                    }
                    OutMode::Fill | OutMode::FillInternal => {
                        // OutSlot-mapped fill tail: Forward(k) passes the
                        // dispatch func's own array, Discard declines the
                        // callee's nullable output outright — which is what C
                        // has always passed there, and what a `historyLen`-sized
                        // throwaway buffer per open was standing in for until
                        // the openers learned rule B6a.
                        let fill_outs: String = arm
                            .out_map
                            .iter()
                            .map(|slot| match slot {
                                streaming::OutSlot::Forward(k) => outputs[*k].clone(),
                                streaming::OutSlot::Discard => "null".to_string(),
                            })
                            .collect::<Vec<_>>()
                            .join(", ");
                        let opts = if opts.is_empty() {
                            String::new()
                        } else {
                            format!("{}, ", opts.join(", "))
                        };
                        // The callee's public openAndFill reports its filled
                        // range on the handle; this body still owes its caller
                        // the MInteger pair, so copy it back out.
                        if mode == OutMode::FillInternal {
                            // The internal variant takes the out-meta directly,
                            // so there is no range to copy back.
                            let _ = writeln!(
                                o,
                                "         {cls} sub = {callee_base}_OpenAndFillInternal({bar_args}, startIdx, {opts}outBegIdx, outNBElement, {fill_outs});"
                            );
                            dispatch_store_sub(o, registry, arm, &outputs, "         ");
                            let _ = writeln!(o, "         break;");
                            let _ = writeln!(o, "      }}");
                            continue;
                        }
                        let _ = writeln!(
                            o,
                            "         {cls} sub = {callee_base}_OpenAndFill({bar_args}, {opts}{fill_outs});"
                        );
                        let _ = writeln!(o, "         outBegIdx.value = sub.outRangeBegIdx;");
                        let _ = writeln!(
                            o,
                            "         outNBElement.value = sub.outRangeCount;"
                        );
                    }
                }
                dispatch_store_sub(o, registry, arm, &outputs, "         ");
                let _ = writeln!(o, "         break;");
                let _ = writeln!(o, "      }}");
            } else {
                let what = if arm.callee.is_empty() { "delegation" } else { arm.callee.as_str() };
                let _ = writeln!(o, "      case {label}:");
                let _ = writeln!(o, "         return RetCode.BadParam; /* no {what} stream */");
            }
        }
        let _ = writeln!(o, "      default:");
        let _ = writeln!(o, "         return RetCode.BadParam;");
        let _ = writeln!(o, "      }}");
        for p in &func.optional_inputs {
            let _ = writeln!(o, "      sp.{0} = {0};", p.name);
        }
        if has_value_class(func) {
            let _ = writeln!(o, "      sp.cachedValue = {};", capture_value_expr(func));
        }
        let _ = writeln!(o, "      return RetCode.Success;");
        let _ = writeln!(o, "   }}");
    }

    emit_open_wrappers(o, func, false);
    emit_open_and_fill_internal_wrapper(o, func, false);
}

// ---------------------------------------------------------------------------
// Period-bank tier (MAVP): a bank of sub-MA streams advanced in lockstep,
// selected per bar by the clamped variable period.
// ---------------------------------------------------------------------------

#[allow(clippy::too_many_lines)]
fn emit_period_bank(
    o: &mut String,
    func: &FuncDef,
    plan: &streaming::PeriodBankPlan,
    registry: &Registry,
    helpers: &HelperRegistry,
    enums: &HashMap<String, EnumDef>,
) {
    let _ = helpers;
    let callee = plan.callee.as_str();
    let callee_base = registry.name_of(callee);
    let subty = callee_stream_class(registry, callee);
    let callee_out0 = registry.callee_outputs(callee)[0].clone();
    let min = plan.min_param.as_str();
    let max = plan.max_param.as_str();
    let price = plan.price_input.as_str();
    let period = plan.period_input.as_str();
    let out = plan.output.as_str();

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
    let open_opts = opts_of(&format!("{min} + bankIdx"));

    // --- handle class -------------------------------------------------------
    let fields = base_fields(func);
    let extra_members = format!(
        "      // One sub-{} stream per period in [{min}, {max}], advanced in lockstep.\n      {subty}[] bank;\n",
        callee.to_uppercase()
    );
    // Object-array clone is SHALLOW: the bank must copy element-wise or a peek
    // would advance the live handle's sub-streams through the aliased slots.
    let mut copy_extra = String::new();
    let _ = writeln!(copy_extra, "         this.bank = new {subty}[other.bank.length];");
    let _ = writeln!(copy_extra, "         for( int bankIdx = 0; bankIdx < other.bank.length; bankIdx++ ) {{");
    let _ = writeln!(copy_extra, "            this.bank[bankIdx] = new {subty}(other.bank[bankIdx]);");
    let _ = writeln!(copy_extra, "         }}");
    // Same shape, in place: the bank a scratch already holds is the right
    // length unless a differently-parameterised handle borrowed it, in which
    // case it is rebuilt exactly as the copy constructor builds one.
    let mut restore_extra = String::new();
    let _ = writeln!(restore_extra, "         if( this.bank != null && this.bank.length == other.bank.length ) {{");
    let _ = writeln!(restore_extra, "            for( int bankIdx = 0; bankIdx < other.bank.length; bankIdx++ ) {{");
    let _ = writeln!(restore_extra, "               this.bank[bankIdx].copyFrom(other.bank[bankIdx]);");
    let _ = writeln!(restore_extra, "            }}");
    let _ = writeln!(restore_extra, "         }} else {{");
    let _ = writeln!(restore_extra, "            this.bank = new {subty}[other.bank.length];");
    let _ = writeln!(restore_extra, "            for( int bankIdx = 0; bankIdx < other.bank.length; bankIdx++ ) {{");
    let _ = writeln!(restore_extra, "               this.bank[bankIdx] = new {subty}(other.bank[bankIdx]);");
    let _ = writeln!(restore_extra, "            }}");
    let _ = writeln!(restore_extra, "         }}");
    // A bank is one handle per period in the span: unbounded by construction.
    let subs = SubMembers { copy: copy_extra, restore: restore_extra, subs: 1, unbounded: true };
    emit_handle_class_with_members(o, func, &fields, &subs, &extra_members);

    // --- step: advance ALL slots, output the clamped-period slot ------------
    emit_step_sig(o, func);
    let _ = writeln!(o, "      int cp = (int){period};");
    let _ = writeln!(o, "      if( cp < sp.{min} ) {{");
    let _ = writeln!(o, "         cp = sp.{min};");
    let _ = writeln!(o, "      }} else if( cp > sp.{max} ) {{");
    let _ = writeln!(o, "         cp = sp.{max};");
    let _ = writeln!(o, "      }}");
    let _ = writeln!(o, "      int slot = cp - sp.{min};");
    let _ = writeln!(o, "      for( int bankIdx = 0; bankIdx < sp.bank.length; bankIdx++ ) {{");
    let _ = writeln!(o, "         double subValue = sp.bank[bankIdx].update({price});");
    let _ = writeln!(o, "         if( bankIdx == slot ) {{");
    let _ = writeln!(o, "            sp.cur_{out} = subValue;");
    let _ = writeln!(o, "         }}");
    let _ = writeln!(o, "      }}");
    let _ = writeln!(o, "   }}");

    // --- open body (Scalar) -------------------------------------------------
    let own_lb_args: Vec<String> = func.optional_inputs.iter().map(|p| p.name.clone()).collect();
    let base = base_name(func);
    let own_lb_call = format!("{base}_Lookback({})", own_lb_args.join(", "));
    emit_open_body_sig(o, func, OutMode::Scalar);
    let _ = writeln!(o, "      int historyLen = {price}.length;");
    emit_open_validation(o, func, OutMode::Scalar, enums);
    let _ = writeln!(o, "      /* An inverted [min, max] period window is invalid (batch rejects). */");
    let _ = writeln!(o, "      if( {min} > {max} ) {{");
    let _ = writeln!(o, "         return RetCode.BadParam;");
    let _ = writeln!(o, "      }}");
    // Own-lookback precheck BEFORE opening the bank: a bank sub's reject would
    // carry the callee's message prefix, not this function's (stable-prefix
    // contract; the Fill body below has the equivalent check).
    let _ = writeln!(o, "      if( historyLen < {own_lb_call} + 1 ) {{");
    let _ = writeln!(o, "         return RetCode.InsufficientHistory;");
    let _ = writeln!(o, "      }}");
    let _ = writeln!(
        o,
        "      /* Seed EVERY sub at the SHARED max-period lookback, exactly as batch\n\
         \x20      * does: it clamps startIdx up to lookback(maxPeriod) and calls the callee\n\
         \x20      * with that same start for every period. Seeding each sub at its own\n\
         \x20      * (smaller) lookback would seed the recurrence from a different bar and\n\
         \x20      * diverge for every period < maxPeriod. */"
    );
    let _ = writeln!(o, "      int lookbackTotal = {callee_base}_Lookback({lb_args});");
    let _ = writeln!(o, "      int subStart = (startIdx < lookbackTotal)? lookbackTotal : startIdx;");
    // The bank is opened at `subStart`, so the history has to reach it.
    let _ = writeln!(o, "      if( historyLen < subStart + 1 ) {{");
    let _ = writeln!(o, "         return RetCode.InsufficientHistory;");
    let _ = writeln!(o, "      }}");
    let _ = writeln!(o, "      int nBank = {max} - {min} + 1;");
    let _ = writeln!(o, "      {subty}[] bank = new {subty}[nBank];");
    let _ = writeln!(o, "      for( int bankIdx = 0; bankIdx < nBank; bankIdx++ ) {{");
    let _ = writeln!(o, "         bank[bankIdx] = {callee_base}_OpenInternal({price}, subStart, {open_opts});");
    let _ = writeln!(o, "      }}");
    let _ = writeln!(o, "      int cp = (int){period}[historyLen - 1];");
    let _ = writeln!(o, "      if( cp < {min} ) {{");
    let _ = writeln!(o, "         cp = {min};");
    let _ = writeln!(o, "      }} else if( cp > {max} ) {{");
    let _ = writeln!(o, "         cp = {max};");
    let _ = writeln!(o, "      }}");
    for p in &func.optional_inputs {
        let _ = writeln!(o, "      sp.{0} = {0};", p.name);
    }
    let _ = writeln!(o, "      sp.bank = bank;");
    let _ = writeln!(o, "      sp.cur_{out} = bank[cp - {min}].cur_{callee_out0};");
    // `subStart` is the resolved max(startIdx, lookback) the whole bank was
    // opened at, which is the range's start by definition (issue #241).
    let _ = writeln!(o, "      sp.outRangeBegIdx = subStart;");
    let _ = writeln!(o, "      sp.outRangeCount = historyLen - subStart;");
    let _ = writeln!(o, "      return RetCode.Success;");
    let _ = writeln!(o, "   }}");

    // --- open body (Fill): no per-bar array exists to un-discard (the bank
    // yields one selected scalar per bar), so fill genuinely re-runs history:
    // seed the bank on the first-output-bar prefix, emit that bar, then replay
    // updates over the remaining history selecting per bar. ------------------
    emit_open_body_sig(o, func, OutMode::Fill);
    let _ = writeln!(o, "      int historyLen = {price}.length;");
    emit_open_validation(o, func, OutMode::Fill, enums);
    let _ = writeln!(o, "      /* An inverted [min, max] period window is invalid (batch rejects). */");
    let _ = writeln!(o, "      if( {min} > {max} ) {{");
    let _ = writeln!(o, "         return RetCode.BadParam;");
    let _ = writeln!(o, "      }}");
    let _ = writeln!(o, "      int lookbackTotal = {callee_base}_Lookback({lb_args});");
    let _ = writeln!(o, "      if( historyLen < lookbackTotal + 1 ) {{");
    let _ = writeln!(o, "         return RetCode.InsufficientHistory;");
    let _ = writeln!(o, "      }}");
    let _ = writeln!(o, "      int nBank = {max} - {min} + 1;");
    let _ = writeln!(o, "      /* Seed each sub at the first output bar (lookbackTotal), NOT the last. */");
    let _ = writeln!(o, "      {subty}[] bank = new {subty}[nBank];");
    let _ = writeln!(o, "      double[] scratch = new double[nBank];");
    let _ = writeln!(
        o,
        "      double[] seedPrefix = java.util.Arrays.copyOfRange({price}, 0, lookbackTotal + 1);"
    );
    let _ = writeln!(o, "      for( int bankIdx = 0; bankIdx < nBank; bankIdx++ ) {{");
    let _ = writeln!(o, "         {subty} sub = {callee_base}_OpenInternal(seedPrefix, lookbackTotal, {open_opts});");
    let _ = writeln!(o, "         bank[bankIdx] = sub;");
    let _ = writeln!(o, "         scratch[bankIdx] = sub.cur_{callee_out0};");
    let _ = writeln!(o, "      }}");
    let _ = writeln!(o, "      /* First output bar (lookbackTotal), then replay the remaining history. */");
    let _ = writeln!(o, "      int cp = (int){period}[lookbackTotal];");
    let _ = writeln!(o, "      if( cp < {min} ) {{");
    let _ = writeln!(o, "         cp = {min};");
    let _ = writeln!(o, "      }} else if( cp > {max} ) {{");
    let _ = writeln!(o, "         cp = {max};");
    let _ = writeln!(o, "      }}");
    let _ = writeln!(o, "      {out}[0] = scratch[cp - {min}];");
    let _ = writeln!(o, "      for( int t = lookbackTotal + 1; t < historyLen; t++ ) {{");
    let _ = writeln!(o, "         for( int bankIdx = 0; bankIdx < nBank; bankIdx++ ) {{");
    let _ = writeln!(o, "            scratch[bankIdx] = bank[bankIdx].update({price}[t]);");
    let _ = writeln!(o, "         }}");
    let _ = writeln!(o, "         cp = (int){period}[t];");
    let _ = writeln!(o, "         if( cp < {min} ) {{");
    let _ = writeln!(o, "            cp = {min};");
    let _ = writeln!(o, "         }} else if( cp > {max} ) {{");
    let _ = writeln!(o, "            cp = {max};");
    let _ = writeln!(o, "         }}");
    let _ = writeln!(o, "         {out}[t - lookbackTotal] = scratch[cp - {min}];");
    let _ = writeln!(o, "      }}");
    let _ = writeln!(o, "      outBegIdx.value = lookbackTotal;");
    let _ = writeln!(o, "      outNBElement.value = historyLen - lookbackTotal;");
    for p in &func.optional_inputs {
        let _ = writeln!(o, "      sp.{0} = {0};", p.name);
    }
    let _ = writeln!(o, "      sp.bank = bank;");
    let _ = writeln!(o, "      sp.cur_{out} = {out}[outNBElement.value - 1];");
    let _ = writeln!(o, "      return RetCode.Success;");
    let _ = writeln!(o, "   }}");

    emit_open_wrappers(o, func, false);
}

// ---------------------------------------------------------------------------
// Composed tier (STOCH class): producer transition + pipeline of owned public
// sub-handles, mirroring rust_stream's emit_composed with the managed-language
// simplifications: GC replaces every cleanup ladder and series-free replay,
// `free()` renders as a no-op so lag-ring seeding reads the still-live
// intermediate array, and copy-peek deletes peekMode entirely (sub handles
// deep-copy through their copy constructors).
// ---------------------------------------------------------------------------

/// Composed producer name map: identical to [`JavaStreamNames`] except the
/// intermediate series' "output" write lands in a `cur_<series>` local.
struct JavaComposedNames {
    series: String,
}

impl streaming::NameMap for JavaComposedNames {
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
            Expr::Var(format!("sp.cur_{name}"))
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
    fn extrema_mask(&self) -> String {
        "sp.xMask".to_string()
    }
}

/// The `cur_<name>` locals the composed step declares: the producer series,
/// each sub-call's destinations in tail order (dedup), and map-defined outputs.
fn composed_cur_scalars(
    cp: &streaming::ComposedPlan,
    bar_inputs: &[String],
    outputs: &[String],
) -> Vec<String> {
    let mut out: Vec<String> = Vec::new();
    let mut seen: BTreeSet<String> = BTreeSet::new();
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

/// Java twin of `c_stream::transform_map_step`: series reads/writes become the
/// per-bar `cur_*` locals (a lag-ring series' `[cursor]` read becomes the
/// ring's oldest slot), params read through `sp.`, `for` shells dropped.
fn transform_map_step(
    st: &Statement,
    cur: &std::collections::BTreeMap<String, String>,
    params: &BTreeSet<String>,
    sub_lag_rings: &[streaming::SubLagRing],
) -> Vec<Statement> {
    let cursor = map_cursor(st);
    let lag_series: BTreeSet<&str> = sub_lag_rings.iter().map(|r| r.series.as_str()).collect();
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

/// The composed step's locals: the producer's temps, the map temps, and one
/// `cur_` scalar per output and sub-call intermediate.
///
/// A `cur_` scalar is typed by what it stands for, as in C — an output's own
/// element type, `double` for the intermediates. Sizing them all as `double`
/// truncated an integer output on the way out.
fn emit_composed_step_decls(
    o: &mut String,
    func: &FuncDef,
    cp: &streaming::ComposedPlan,
    cur_scalars: &[String],
) {
    if let Some(model) = &cp.producer {
        for (name, ty) in &model.temps {
            let (jty, default) = field_type_and_default(ty);
            let _ = writeln!(o, "      {jty} {name} = {default};");
        }
    }
    for (name, ty) in &cp.map_temps {
        let (jty, default) = field_type_and_default(ty);
        let _ = writeln!(o, "      {jty} {name} = {default};");
    }
    for name in cur_scalars {
        let ty = out_java_type(func, name);
        let zero = if ty == "int" { "0" } else { "0.0" };
        let _ = writeln!(o, "      {ty} cur_{name} = {zero};");
    }
}

/// The composed StepImpl: producer transition (writing `cur_<series>`), then
/// the batch-tail pipeline through the owned sub handles, combine maps per
/// bar, lag-ring pushes, and the `sp.cur_*` output stores. No peek flag: peek
/// is the universal deep-copy of the whole tree.
#[allow(clippy::too_many_arguments)]
fn emit_composed_step(
    o: &mut String,
    func: &FuncDef,
    cp: &streaming::ComposedPlan,
    step_settings: &BTreeSet<String>,
    stream_fma: &FmaVarSets,
    registry: &Registry,
    inputs: &[String],
    outputs: &[String],
    enums: &HashMap<String, EnumDef>,
    helpers: &HelperRegistry,
    counter: &Cell<usize>,
) {
    emit_step_sig(o, func);
    let cur_scalars = composed_cur_scalars(cp, inputs, outputs);

    emit_composed_step_decls(o, func, cp, &cur_scalars);

    let empty = HashSet::new();
    let ctx = stream_ctx(&empty, counter, stream_fma);

    // The cur-map: bar inputs are the step's scalar parameters.
    let mut cur: std::collections::BTreeMap<String, String> = inputs
        .iter()
        .map(|b| (b.clone(), b.clone()))
        .collect();

    if let Some(model) = &cp.producer {
        emit_extrema_rebase(o, model, 6);
        for s in step_settings {
            let _ = writeln!(o, "      int {s}_rangeType = sp.cs_{s}_rangeType;");
            let _ = writeln!(o, "      int {s}_avgPeriod = sp.cs_{s}_avgPeriod;");
            let _ = writeln!(o, "      double {s}_factor = sp.cs_{s}_factor;");
        }
        let names = JavaComposedNames {
            series: cp.series.clone().expect("producer plan carries a series"),
        };
        let transition = streaming::build_transition(model, &names)
            .unwrap_or_else(|e| panic!("streaming transition: {e}"));
        for st in &transition {
            o.push_str(&render_statement_ctx(st, 6, &ctx, enums, registry, helpers));
        }
        let series = cp.series.clone().expect("producer plan carries a series");
        cur.insert(series.clone(), format!("cur_{series}"));
    }

    // Pipeline: the batch tail, one scalar per bar through the sub handles.
    let _ = writeln!(o, "      /* Pipeline the new bar through the sub-streams (batch tail order). */");
    let params: BTreeSet<String> = func.optional_inputs.iter().map(|p| p.name.clone()).collect();
    for step in &cp.steps {
        match step {
            streaming::UpdateStep::Sub { sub_idx } => {
                let sub = &cp.subs[*sub_idx];
                let callee_key = sub.callee.to_lowercase();
                let args: Vec<String> = sub
                    .srcs
                    .iter()
                    .map(|src| cur.get(src).expect("analyzer ordered sub srcs").clone())
                    .collect();
                let arg_str = args.join(", ");
                if sub.dsts.len() == 1 {
                    let d = &sub.dsts[0];
                    let _ = writeln!(o, "      cur_{d} = sp.sub{sub_idx}.update({arg_str});");
                } else {
                    let cls = callee_stream_class(registry, &callee_key);
                    let _ = writeln!(o, "      {{");
                    let _ = writeln!(o, "         {cls}.Value subOut{sub_idx} = sp.sub{sub_idx}.update({arg_str});");
                    for (k, d) in sub.dsts.iter().enumerate() {
                        let _ = writeln!(
                            o,
                            "         cur_{d} = subOut{sub_idx}.{}();",
                            callee_value_field(registry, &callee_key, k)
                        );
                    }
                    let _ = writeln!(o, "      }}");
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
                let _ = writeln!(o, "      /* Combine map (batch tail, per bar). */");
                for st in &transform_map_step(&cp.tail[*tail_idx], &cur, &params, &cp.sub_lag_rings) {
                    o.push_str(&render_statement_ctx(st, 6, &ctx, enums, registry, helpers));
                }
            }
        }
    }
    // Push the new sub-output value into each lag ring AFTER every read of the
    // oldest slot in the combine above (mirrors C, incl. the modulo advance).
    for ring in &cp.sub_lag_rings {
        let sn = &ring.series;
        let _ = writeln!(o, "      sp.lagRing_{sn}[sp.lagRingPos_{sn}] = cur_{sn};");
        let _ = writeln!(
            o,
            "      sp.lagRingPos_{sn} = (sp.lagRingPos_{sn} + 1) % sp.lagRingCap_{sn};"
        );
    }
    for out in outputs {
        let _ = writeln!(o, "      sp.cur_{out} = {};", cur.get(out).expect("analyzer gated output"));
    }
    let _ = writeln!(o, "   }}");
}

/// The transcribed (region, tail) for the composed open: output arrays renamed
/// to their `sc_` scratch arrays, early-success returns mapped to the
/// insufficient-history signal, final tail return dropped. No out-meta rewrite
/// (Scalar mode declares local MInteger boxes under the batch names).
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
                    Some(Expr::Var(v)) => Some(Expr::Var(map_open_return(&v))),
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

/// Composed open body (the merged `Core`, reached at stride 0 and stride 1):
/// scratch `sc_` output arrays + verbatim transcription of the batch body with
/// sub-streams opened at the exact consumption points, then capture.
#[allow(clippy::too_many_arguments, clippy::too_many_lines)]
fn emit_composed_open(
    o: &mut String,
    func: &FuncDef,
    cp: &streaming::ComposedPlan,
    step_settings: &BTreeSet<String>,
    stream_fma: &FmaVarSets,
    outputs: &[String],
    enums: &HashMap<String, EnumDef>,
    registry: &Registry,
    helpers: &HelperRegistry,
    counter: &Cell<usize>,
) {
    // The composed fill/scratch path hardcodes double arrays (mirrors C/Rust).
    let empty = HashSet::new();
    let ctx = stream_ctx(&empty, counter, stream_fma);

    emit_open_body_sig(o, func, OutMode::Core);
    let (region_stmts, tail_stmts) = build_composed_open_bodies(cp, outputs);
    let combined: Vec<Statement> = cleanup_open_body(
        &region_stmts.iter().cloned().chain(tail_stmts.iter().cloned()).collect::<Vec<_>>(),
        registry,
    );
    emit_body_decls(o, func, &combined);
    emit_open_head(o, func, &[]);
    emit_open_validation(o, func, OutMode::Core, enums);
    emit_anchor_guard(o);
    // Own-lookback precheck BEFORE opening any sub: a sub's reject would carry
    // the CALLEE's message prefix ("MA open:" for a BBANDS call), breaking the
    // stable "<NAME> open:" contract. Same check the dispatch and period-bank
    // shapes already emit, for the same reason.
    //
    // It also makes the contract testable in the other direction: past this
    // point a sub rejecting is a bug in THIS function's lookback, not a caller
    // error, so it surfaces as the IllegalStateException the reject-conversion
    // tail already maps InternalError to rather than being silently reported as
    // the sub's insufficient history.
    //
    // Cost: one lookback call per OPEN, on a path that already allocates
    // `historyLen` doubles per output; `update` is untouched. The rejecting path
    // is cheaper for it -- the scratch allocations below never happen.
    {
        let lb_args: Vec<String> =
            func.optional_inputs.iter().map(|p| p.name.clone()).collect();
        let lb_call = format!("{}_Lookback({})", base_name(func), lb_args.join(", "));
        let _ = writeln!(o, "      if( historyLen < {lb_call} + 1 ) {{");
        let _ = writeln!(o, "         return RetCode.InsufficientHistory;");
        let _ = writeln!(o, "      }}");
    }
    emit_extras_and_candle(o, func, &combined, registry, helpers, counter, stream_fma);
    // At stride 1 the caller's array is already the destination the transcribed
    // tail writes, so the scratch aliases it instead of being allocated and
    // copied back (issue #205). Only the scalar sink still needs its own
    // buffer. See `fill_scratch_may_alias_output` for when this is unsound.
    let alias_fill = cp.fill_scratch_may_alias_output(outputs);
    for out in outputs {
        let ty = out_java_type(func, out);
        if alias_fill {
            let _ = writeln!(
                o,
                "      {ty}[] sc_{out} = outStride == 1 ? {out} : new {ty}[historyLen];"
            );
        } else {
            let _ = writeln!(o, "      {ty}[] sc_{out} = new {ty}[historyLen];");
        }
    }

    // Sub-open inserts, keyed to combined region++tail indices. The sub reads
    // the produced series only up to the sub-call's endIdx, so pass a truncated
    // copy; a negative anchor clamps inside the callee exactly like batch.
    // Anchor/endIdx expressions may read MInteger locals of the transcribed
    // region (MACDEXT's outNbElement1), so render them with the region's
    // address-of sets.
    let mut ins_address_of = collect_address_of_vars(&combined);
    let ins_double_address_of = collect_double_address_of_vars(&combined, &ins_address_of);
    for name in &ins_double_address_of {
        ins_address_of.remove(name);
    }
    let ins_ctx = JavaRenderCtx {
        single_precision: false,
        nullable_outputs: &empty,
        nullable_shadow: false,
        address_of_vars: &ins_address_of,
        double_address_of_vars: &ins_double_address_of,
        float_input_params: &empty,
        inline_counter: counter,
        fma: Some(stream_fma),
        matype_map: HashMap::new(),
    };
    let region_len = region_stmts.len();
    // Own inputs are exactly `historyLen` long — `emit_open_validation` above
    // rejects any input whose length differs from the first one's, and
    // `endIdx` is `historyLen - 1` — so a copy of `[0, endIdx]` out of one of
    // them reproduces the array it was taken from. See the elision below.
    let own_inputs: HashSet<String> = streaming::input_array_names(func).into_iter().collect();
    let mut inserts: Vec<(usize, String)> = Vec::new();
    // Combined-body indices whose statement a fused sub-open replaced.
    let mut replaced: HashSet<usize> = HashSet::new();
    for (si, sub) in cp.subs.iter().enumerate() {
        let mut t = String::new();
        let callee_key = sub.callee.to_lowercase();
        let cls = callee_stream_class(registry, &callee_key);
        let callee_base = registry.name_of(&callee_key);
        let sc_rewrite = |e: &Expr| -> Expr {
            streaming::rewrite_expr(e, &|x| match x {
                Expr::Var(v) if outputs.contains(&v) => Expr::Var(format!("sc_{v}")),
                other => other,
            })
        };
        let anchor = render_expr(&sc_rewrite(&sub.s_arg), &ins_ctx, registry, helpers);
        let e_arg = render_expr(&sc_rewrite(&sub.e_arg), &ins_ctx, registry, helpers);
        let srcs: Vec<String> = sub
            .srcs
            .iter()
            .map(|src| {
                let name = if outputs.contains(src) {
                    format!("sc_{src}")
                } else {
                    src.clone()
                };
                // Java has no slice type, so the range the callee may read is
                // conveyed by materializing it. Where the range is the whole
                // array the copy conveys nothing the array does not already
                // say, and it is a `historyLen`-element allocate-copy-discard
                // per sub-open (issue #203). Both halves of the condition are
                // decidable here: the source is one of our own inputs (so its
                // length is `historyLen`, checked above) and the range ends at
                // `endIdx` (`historyLen - 1`). Nothing mutates the argument —
                // the same parameter is `const double[]` in the C prototype
                // and `&[f64]` in Rust — so the copy was never a defence.
                if own_inputs.contains(&name) && e_arg == "endIdx" {
                    name
                } else {
                    format!("java.util.Arrays.copyOfRange({name}, 0, ({e_arg}) + 1)")
                }
            })
            .collect();
        let opts: Vec<String> = sub
            .opt_args
            .iter()
            .map(|a| render_expr(a, &ins_ctx, registry, helpers))
            .collect();
        let opt_tail = if opts.is_empty() {
            String::new()
        } else {
            format!(", {}", opts.join(", "))
        };
        let _ = writeln!(
            t,
            "      /* Sub-stream {si}: {} over `{}`, warmed from bar 0 up to the\n\
             \x20      * sub-call's own startIdx (the seeding point). */",
            sub.callee,
            sub.srcs.join(", ")
        );
        // Fused (issue #192): one pass that BOTH warms the handle and fills this
        // sub-call's destination, so the batch sub-call transcribed next has
        // nothing left to compute and is dropped.
        let fused = sub.is_fusable()
            .then(|| streaming::batch_call_out_args(&tail_stmts[sub.tail_idx], sub))
            .flatten();
        if let Some((out_meta, dsts)) = fused {
            let rend = |e: &Expr| render_expr(&sc_rewrite(e), &ins_ctx, registry, helpers);
            let metas: Vec<String> = out_meta.iter().map(|e| rend(e)).collect();
            let dst_args: Vec<String> = dsts.iter().map(|e| rend(e)).collect();
            let _ = writeln!(
                t,
                "      {cls} sub{si} = {callee_base}_OpenAndFillInternal({}, {anchor}{opt_tail}, {}, {});",
                srcs.join(", "),
                metas.join(", "),
                dst_args.join(", ")
            );
            // Keep the assignment the transcribed error handling reads.
            if let Statement::Assign { target, .. } = &tail_stmts[sub.tail_idx] {
                let _ = writeln!(
                    t,
                    "      {} = RetCode.Success;",
                    render_expr(&sc_rewrite(target), &ins_ctx, registry, helpers)
                );
            }
            replaced.insert(region_len + sub.tail_idx);
        } else {
            let _ = writeln!(
                t,
                "      {cls} sub{si} = {callee_base}_OpenInternal({}, {anchor}{opt_tail});",
                srcs.join(", ")
            );
        }
        inserts.push((region_len + sub.tail_idx, t));
    }

    emit_open_region(o, func, &combined, enums, registry, helpers, counter, stream_fma, &inserts, &replaced);

    // --- capture ------------------------------------------------------------
    let _ = writeln!(o, "      /* Capture the live producer state + sub handles. */");
    let _ = writeln!(o, "      if( outNBElement.value < 1 ) {{");
    let _ = writeln!(o, "         return RetCode.InsufficientHistory;");
    let _ = writeln!(o, "      }}");
    // Lag rings: seed from the tail of the still-live intermediate array (its
    // batch `free()` renders as a no-op in Java, so no withheld-free dance).
    for ring in &cp.sub_lag_rings {
        let sr = &ring.series;
        let lag = render_expr(&ring.lag, &ctx, registry, helpers);
        let _ = writeln!(o, "      int lagCap_{sr} = (int)({lag});");
        let _ = writeln!(o, "      double[] lagRing_{sr} = new double[lagCap_{sr}];");
        let _ = writeln!(o, "      for( int lagI = 0; lagI < lagCap_{sr}; lagI++ ) {{");
        let _ = writeln!(o, "         lagRing_{sr}[lagI] = {sr}[outNBElement.value + lagI];");
        let _ = writeln!(o, "      }}");
    }
    let mut extra = String::new();
    for (si, _) in cp.subs.iter().enumerate() {
        let _ = writeln!(extra, "      sp.sub{si} = sub{si};");
    }
    for ring in &cp.sub_lag_rings {
        let sr = &ring.series;
        let _ = writeln!(extra, "      sp.lagRingPos_{sr} = 0;");
        let _ = writeln!(extra, "      sp.lagRingCap_{sr} = lagCap_{sr};");
        let _ = writeln!(extra, "      sp.lagRing_{sr} = lagRing_{sr};");
    }
    if let Some(model) = &cp.producer {
        // The producer's own "output" is the intermediate series, so its
        // cur seeding is suppressed; the real outputs seed from sc_ below.
        emit_capture(
            o, func, model, &model.state, step_settings, registry, helpers, stream_fma,
            counter, None, &extra,
        );
    } else {
        for p in &func.optional_inputs {
            let _ = writeln!(o, "      sp.{0} = {0};", p.name);
        }
        for (name, _) in &func.private_extra_params {
            let _ = writeln!(o, "      sp.{name} = {name};");
        }
        o.push_str(&extra);
    }
    emit_cur_capture(o, func, outputs, CurSource::Scratch);
    // Both modes compute into `sc_*` and seed `sp.cur_*` from it above; only the
    // hand-back differs, and it is the ONE place a stride multiply cannot express
    // the difference — a bulk copy takes a base array, not a subscript. At stride
    // 0 there is nothing to hand back: the scalar sink is one element and the
    // handle already carries the value.
    if !alias_fill {
        for out in outputs {
            let _ = writeln!(
                o,
                "      if( outStride == 1 ) System.arraycopy(sc_{out}, 0, {out}, 0, outNBElement.value);"
            );
        }
    }
    let _ = writeln!(o, "      return RetCode.Success;");
    let _ = writeln!(o, "   }}");
}

/// The whole composed stream section.
#[allow(clippy::too_many_arguments)]
fn emit_composed(
    o: &mut String,
    func: &FuncDef,
    cp: &streaming::ComposedPlan,
    stream_fma: &FmaVarSets,
    enums: &HashMap<String, EnumDef>,
    registry: &Registry,
    helpers: &HelperRegistry,
    counter: &Cell<usize>,
) {
    let inputs = streaming::input_array_names(func);
    let outputs: Vec<String> = func.outputs.iter().map(|x| x.name.clone()).collect();
    let step_settings = match &cp.producer {
        Some(model) => detect_candle_settings(&model.steady_stmts),
        None => BTreeSet::new(),
    };

    // --- handle class -------------------------------------------------------
    let mut fields: Vec<Field> = match &cp.producer {
        Some(model) => state_fields(func, model, &step_settings),
        None => base_fields(func),
    };
    // Lag rings ride the field list (primitive arrays auto-clone in the copy
    // constructor); sub handles need per-callee copy constructors (copy_extra).
    for ring in &cp.sub_lag_rings {
        let sr = &ring.series;
        fields.push((format!("lagRingPos_{sr}"), "int".into(), "0".into()));
        fields.push((format!("lagRingCap_{sr}"), "int".into(), "1".into()));
        fields.push((format!("lagRing_{sr}"), "double[]".into(), "new double[1]".into()));
    }
    let mut extra_members = String::new();
    let mut copy_extra = String::new();
    let mut restore_extra = String::new();
    for (si, sub) in cp.subs.iter().enumerate() {
        let callee_key = sub.callee.to_lowercase();
        let cls = callee_stream_class(registry, &callee_key);
        let _ = writeln!(extra_members, "      {cls} sub{si};");
        let _ = writeln!(copy_extra, "         this.sub{si} = new {cls}(other.sub{si});");
        // A sub's class is fixed by the plan, so a scratch always has one to
        // overwrite; the null arm covers a scratch built by the bare
        // `(Core)` constructor, which no path takes today.
        let _ = writeln!(restore_extra, "         if( this.sub{si} == null ) {{");
        let _ = writeln!(restore_extra, "            this.sub{si} = new {cls}(other.sub{si});");
        let _ = writeln!(restore_extra, "         }} else {{");
        let _ = writeln!(restore_extra, "            this.sub{si}.copyFrom(other.sub{si});");
        let _ = writeln!(restore_extra, "         }}");
    }
    let subs = SubMembers {
        copy: copy_extra,
        restore: restore_extra,
        subs: cp.subs.len(),
        unbounded: false,
    };
    emit_handle_class_with_members(o, func, &fields, &subs, &extra_members);

    emit_composed_step(
        o, func, cp, &step_settings, stream_fma, registry, &inputs, &outputs, enums, helpers,
        counter,
    );
    emit_composed_open(
        o, func, cp, &step_settings, stream_fma, &outputs, enums, registry, helpers, counter,
    );
    emit_open_and_fill_internal_wrapper(o, func, true);
    emit_open_wrappers(o, func, true);
}

/// `<base>_OpenAndFillInternal`: `OpenAndFill` anchored at the caller's
/// `startIdx` — the composed-open fusion seam (issue #192), so one pass both
/// warms the sub-handle and fills that sub-call's destination.
///
/// For a tier that owns a `Core` this IS the fill implementation: the public
/// `OpenAndFill` is the same call at anchor 0 with the aliasing guard in front,
/// so the seam is reachable for every function and none of them is emitted
/// unreachable. Carrying no guard of its own stays deliberate — the generator
/// emits a composed call here only for a sub-call whose destinations alias
/// neither its sources nor each other ([`streaming::SubCallStep::is_fusable`]),
/// and the public frame above answers for the caller-supplied case.
///
/// `merged` false is the Dispatch tier, which renders its own
/// `_OpenAndFillInternalImpl` because its anchored arm calls different callee
/// tiers than its public one and adds an anchor clamp.
fn emit_open_and_fill_internal_wrapper(o: &mut String, func: &FuncDef, merged: bool) {
    let base = base_name(func);
    let class = stream_class_name(func);
    let in_sig: Vec<String> = streaming::input_array_names(func)
        .iter()
        .map(|a| format!("double {a}[]"))
        .collect();
    let in_fwd: Vec<String> = streaming::input_array_names(func);
    let outs: Vec<String> = func.outputs.iter().map(|out| out.name.clone()).collect();
    let mut fi_sig: Vec<String> = in_sig.clone();
    fi_sig.push("int startIdx".to_string());
    for p in &func.optional_inputs {
        fi_sig.push(format!("{} {}", opt_param_java_type(&p.param_type), p.name));
    }
    fi_sig.push("MInteger outBegIdx".to_string());
    fi_sig.push("MInteger outNBElement".to_string());
    for out in &outs {
        fi_sig.push(format!("{} {}[]", out_java_type(func, out), out));
    }
    let _ = writeln!(
        o,
        "   /* {base}_OpenAndFill anchored at startIdx — the composed-open fusion seam. */"
    );
    let _ = writeln!(
        o,
        "   {class} {base}_OpenAndFillInternal( {} )\n   {{",
        fi_sig.join(", ")
    );
    let _ = writeln!(o, "      {class} sp = new {class}(this);");
    let mut fi_args: Vec<String> = vec!["sp".to_string()];
    fi_args.extend(in_fwd.iter().cloned());
    fi_args.push("startIdx".to_string());
    for p in &func.optional_inputs {
        fi_args.push(p.name.clone());
    }
    fi_args.push("outBegIdx".to_string());
    fi_args.push("outNBElement".to_string());
    fi_args.extend(outs.iter().cloned());
    if merged {
        fi_args.push("1".to_string());
        let _ = writeln!(o, "      RetCode retCode = {base}_OpenImpl({});", fi_args.join(", "));
    } else {
        let _ = writeln!(
            o,
            "      RetCode retCode = {base}_OpenAndFillInternalImpl({});",
            fi_args.join(", ")
        );
    }
    // The caller's pair holds the range the fill wrote — the same one every
    // composed sub-handle is opened through (issue #241).
    let _ = writeln!(o, "      sp.outRangeBegIdx = outBegIdx.value;");
    let _ = writeln!(o, "      sp.outRangeCount = outNBElement.value;");
    emit_reject_conversion(o, func, "openAndFill");
    let _ = writeln!(o, "   }}");
}
