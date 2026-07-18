//! Java stream emitter — the Java twin of `backends/rust_stream.rs` /
//! `backends/c_stream.rs`.
//!
//! For every YAML-declared streamable function this appends a
//! `/**** Streaming API *****/` section to the generated per-function Java
//! fragment (which the shipped `Core.java` splice and the JSON-RPC server
//! inline both pick up unchanged): a `public static final class <Base>Stream`
//! nested in `Core` (per-handle state as package-private fields, `update`/
//! `peek`/`value`/`copy` methods, a deep-copy constructor), a package-private
//! `<base>StreamStep(sp, bars...)` transition method on `Core` (so batch
//! rendering conventions — `this.compatibility`, cross-calls, `Math.fma`
//! sites — work verbatim), a `private RetCode <base>OpenBody(sp, ...)`
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
//!   `OpenBody` the batch body's reject returns stay plain `RetCode` (no throw
//!   statements ever cross the shared renderer — its `expr_stmt` hook skips
//!   bare identifiers); the early-SUCCESS no-data/seed-boundary returns are
//!   mapped in-band to `OutOfRangeEndIndex` so the thin wrapper can type the
//!   one routine, data-dependent condition as `InsufficientHistoryException`
//!   (an `IllegalArgumentException` subclass). `InternalError` (capture
//!   invariant) becomes `IllegalStateException`; every other reject a plain
//!   `IllegalArgumentException`. Messages carry the stable prefix
//!   `"TA_<NAME> open:"`. `update`/`peek` never throw after a successful open.
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
    collect_address_of_vars, collect_double_address_of_vars, collect_matype_vars,
    emit_opt_param_validation, java_type_str, render_expr, render_hoisted_blocks,
    render_statement_ctx, to_java_method_name, JavaRenderCtx, JAVA_CANDLE_FNS,
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
    streaming::validate_streamable(func, lookup).is_ok()
}

/// The Java method base name (`sma`, `movingAverage`, `cdl2Crows`) — always
/// derived from the Java naming authority, never the C name.
fn java_base(func: &FuncDef) -> String {
    to_java_method_name(&func.name, func.camel_case.as_deref())
}

/// First character uppercased (base name → class-name stem).
fn capitalize_first(s: &str) -> String {
    let mut c = s.chars();
    match c.next() {
        Some(f) => f.to_uppercase().collect::<String>() + c.as_str(),
        None => String::new(),
    }
}

/// Public handle class name, nested in `Core`: `SmaStream`,
/// `MovingAverageStream` (from the Java base name — a batch `movingAverage`
/// user must find its stream under the same name).
pub fn stream_class_name(func: &FuncDef) -> String {
    format!("{}Stream", capitalize_first(&java_base(func)))
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
fn value_field_name(out_name: &str) -> String {
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
    fn extrema_cap(&self) -> String {
        "sp.xCap".to_string()
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

/// The full ordered field list of the handle's state (loop-tier shape).
/// `step_settings` = candle settings the transition reads (snapshotted).
fn state_fields(func: &FuncDef, model: &StreamModel, step_settings: &BTreeSet<String>) -> Vec<Field> {
    state_fields_from(func, model, &model.state, step_settings)
}

/// [`state_fields`] with the carried-scalar set supplied by the caller
/// (dual-mode: the union of both modes' scalars).
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
        fields.push(("xCap".into(), "int".into(), "1".into()));
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
    let mut stream_fma = fma::build_fma_var_sets(
        &func.private_body,
        &func.outputs,
        &fma::UNGUARDED_INDEX_SEEDS,
    );
    for input in streaming::input_array_names(func) {
        stream_fma.real_vars.insert(input);
    }

    let counter = Cell::new(0usize);
    let mut o = String::new();

    let _ = writeln!(o, "{SECTION_MARKER}\n");
    match &plan {
        StreamPlan::Loop(model) => {
            emit_loop(&mut o, func, model, &stream_fma, enums, registry, helpers, &counter);
        }
        StreamPlan::DualMode(dmp) => {
            emit_dual_mode(&mut o, func, dmp, &stream_fma, enums, registry, helpers, &counter);
        }
        StreamPlan::FastPathSkip(fp) => {
            emit_fastpath_skip(&mut o, func, fp, &stream_fma, enums, registry, helpers, &counter);
        }
        StreamPlan::Dispatch(dp) => {
            emit_dispatch(&mut o, func, dp, &stream_fma, enums, registry, helpers, &counter);
        }
        StreamPlan::PeriodBank(pb) => {
            emit_period_bank(&mut o, func, pb, registry, helpers);
        }
        StreamPlan::Composed(cp) => {
            emit_composed(&mut o, func, cp, &stream_fma, enums, registry, helpers, &counter);
        }
    }

    o
}

/// Output mode for the open family (mirrors `c_stream::OutMode`). `Scalar` is
/// the ordinary `OpenBody` path (per-bar output writes collapse to a
/// `lastValue_*` scalar); `Fill` is the `OpenAndFillBody` path (writes land in
/// the caller's arrays, out-meta kept, bit-identical to `batch(0, len-1)`).
#[derive(Clone, Copy, PartialEq, Eq)]
enum OutMode {
    Scalar,
    Fill,
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
/// loop tier (`model.body`) and fast-path-skip (`prologue ++ general arm ++
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
    emit_handle_class(o, func, &fields, "");
    emit_step(o, func, model, &step_settings, stream_fma, enums, registry, helpers, counter);
    emit_open_body(
        o, func, model, body, &fields, &step_settings, stream_fma, enums, registry,
        helpers, counter, OutMode::Scalar,
    );
    emit_open_body(
        o, func, model, body, &fields, &step_settings, stream_fma, enums, registry,
        helpers, counter, OutMode::Fill,
    );
    emit_open_wrappers(o, func);
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

/// Emit the nested handle class. `copy_extra` holds extra deep-copy statements
/// for tier-owned members (sub-handle copies); loop tier passes "".
fn emit_handle_class(o: &mut String, func: &FuncDef, fields: &[Field], copy_extra: &str) {
    emit_handle_class_with_members(o, func, fields, copy_extra, "");
}

/// [`emit_handle_class`] with additional raw member declarations (dispatch's
/// `Object sub;`, composed/period-bank sub-handle fields).
fn emit_handle_class_with_members(
    o: &mut String,
    func: &FuncDef,
    fields: &[Field],
    copy_extra: &str,
    extra_members: &str,
) {
    let class = stream_class_name(func);
    let base = java_base(func);
    let n = func.name.to_uppercase();

    let _ = writeln!(
        o,
        "   /**\n\
         \x20   * A live {n} stream (unrelated to {{@code java.util.stream}}): one value per\n\
         \x20   * closed bar, bit-identical to {{@link Core#{base}}} over the same series.\n\
         \x20   * Open with {{@link Core#{base}Open}}; there is no close — the handle is\n\
         \x20   * ordinary heap state, unreferenced handles are simply garbage-collected.\n\
         \x20   * <p>Concurrency: a handle is single-writer — {{@code update}}, {{@code peek}},\n\
         \x20   * {{@code value}} and {{@code copy}} must not race with an {{@code update}} on\n\
         \x20   * the same handle. With no concurrent {{@code update}}, {{@code peek}}/\n\
         \x20   * {{@code value}}/{{@code copy}} never write the handle and may be called\n\
         \x20   * concurrently after safe publication. Independent handles (including\n\
         \x20   * {{@code copy()}} results) are fully independent. Do not mutate the owning\n\
         \x20   * {{@link Core}}'s settings while streams opened from it are live.\n\
         \x20   * <p>Not serializable by design: to checkpoint, retain the history and\n\
         \x20   * re-open — the result is bit-identical by contract.\n\
         \x20   */"
    );
    let _ = writeln!(o, "   public static final class {class} {{");
    let _ = writeln!(o, "      final Core core;");
    for (name, jty, _) in fields {
        let _ = writeln!(o, "      {jty} {name};");
    }
    o.push_str(extra_members);
    let _ = writeln!(o, "\n      {class}( Core core ) {{ this.core = core; }}");

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
    o.push_str(copy_extra);
    let _ = writeln!(o, "      }}");

    emit_value_class(o, func);
    emit_update_peek_value_copy(o, func);

    let _ = writeln!(o, "   }}");
}

/// The immutable multi-output value class (batch output order, public final
/// fields named after the outputs: `outSlowK` → `slowK`).
fn emit_value_class(o: &mut String, func: &FuncDef) {
    if !has_value_class(func) {
        return;
    }
    let _ = writeln!(
        o,
        "\n      /** One output set, in batch output order. Immutable. */"
    );
    let _ = writeln!(o, "      public static final class Value {{");
    for out in &func.outputs {
        let t = out_java_type(func, &out.name);
        let _ = writeln!(o, "         public final {t} {};", value_field_name(&out.name));
    }
    let params: Vec<String> = func
        .outputs
        .iter()
        .map(|out| format!("{} {}", out_java_type(func, &out.name), value_field_name(&out.name)))
        .collect();
    let _ = writeln!(o, "         Value( {} ) {{", params.join(", "));
    for out in &func.outputs {
        let f = value_field_name(&out.name);
        let _ = writeln!(o, "            this.{f} = {f};");
    }
    let _ = writeln!(o, "         }}");
    // toString: "Value[slowK=…, slowD=…]".
    let fmt: Vec<String> = func
        .outputs
        .iter()
        .map(|out| {
            let f = value_field_name(&out.name);
            format!("\"{f}=\" + {f}")
        })
        .collect();
    let _ = writeln!(o, "         @Override public String toString() {{");
    let _ = writeln!(
        o,
        "            return \"Value[\" + {} + \"]\";",
        fmt.join(" + \", \" + ")
    );
    let _ = writeln!(o, "         }}");
    // equals/hashCode: bit-based double semantics (NaN/-0.0-safe, what a
    // record would generate).
    let eqs: Vec<String> = func
        .outputs
        .iter()
        .map(|out| {
            let f = value_field_name(&out.name);
            if out_is_int(func, &out.name) {
                format!("this.{f} == v.{f}")
            } else {
                format!("Double.doubleToLongBits(this.{f}) == Double.doubleToLongBits(v.{f})")
            }
        })
        .collect();
    let _ = writeln!(o, "         @Override public boolean equals( Object o ) {{");
    let _ = writeln!(o, "            if( !(o instanceof Value) ) return false;");
    let _ = writeln!(o, "            Value v = (Value) o;");
    let _ = writeln!(o, "            return {};", eqs.join(" && "));
    let _ = writeln!(o, "         }}");
    let _ = writeln!(o, "         @Override public int hashCode() {{");
    let _ = writeln!(o, "            int h = 17;");
    for out in &func.outputs {
        let f = value_field_name(&out.name);
        if out_is_int(func, &out.name) {
            let _ = writeln!(o, "            h = 31 * h + {f};");
        } else {
            let _ = writeln!(o, "            h = 31 * h + Double.hashCode({f});");
        }
    }
    let _ = writeln!(o, "            return h;");
    let _ = writeln!(o, "         }}");
    let _ = writeln!(o, "      }}");
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

fn emit_update_peek_value_copy(o: &mut String, func: &FuncDef) {
    let class = stream_class_name(func);
    let base = java_base(func);
    let vt = if has_value_class(func) {
        "Value".to_string()
    } else {
        out_java_type(func, &func.outputs[0].name).to_string()
    };
    let (sig_bars, fwd_bars) = bar_params(func);

    let _ = writeln!(
        o,
        "\n      /**\n\
         \x20      * Commit one closed bar; always produces the new current value.\n\
         \x20      * Never throws after a successful open; never allocates handle state.\n\
         \x20      */"
    );
    let _ = writeln!(o, "      public {vt} update( {sig_bars} ) {{");
    let _ = writeln!(o, "         core.{base}StreamStep(this, {fwd_bars});");
    if has_value_class(func) {
        let _ = writeln!(o, "         this.cachedValue = {};", fresh_value_expr(func, "this"));
        let _ = writeln!(o, "         return this.cachedValue;");
    } else {
        let _ = writeln!(o, "         return {};", fresh_value_expr(func, "this"));
    }
    let _ = writeln!(o, "      }}");

    let _ = writeln!(
        o,
        "\n      /**\n\
         \x20      * Evaluate a forming bar without committing — bit-identical to what the\n\
         \x20      * next {{@code update}} with the same bar would return (it is the same\n\
         \x20      * generated code, run on a throwaway copy). Deep-copies the handle state\n\
         \x20      * on every call: O(period) for windowed indicators — for hot loops,\n\
         \x20      * prefer {{@code update}} on a {{@code copy()}}.\n\
         \x20      */"
    );
    let _ = writeln!(o, "      public {vt} peek( {sig_bars} ) {{");
    let _ = writeln!(o, "         {class} scratch = new {class}(this);");
    let _ = writeln!(o, "         core.{base}StreamStep(scratch, {fwd_bars});");
    let _ = writeln!(o, "         return {};", fresh_value_expr(func, "scratch"));
    let _ = writeln!(o, "      }}");

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
// StreamStep
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
    }
}

/// `void <base>StreamStep( <Class> sp, double bar... )` — the one per-bar
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
    let base = java_base(func);
    let class = stream_class_name(func);
    let (sig_bars, _) = bar_params(func);
    let _ = writeln!(o, "   void {base}StreamStep( {class} sp, {sig_bars} )\n   {{");
}

/// One model's per-bar step body at a given indent: temp decls, the extrema
/// rebase, the candle-snapshot unpacking, and the rendered transition. Called
/// once by the loop tier (indent 6) and once per arm by the dual-mode step.
#[allow(clippy::too_many_arguments)]
fn emit_step_body(
    o: &mut String,
    func: &FuncDef,
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

/// Extrema automatons carry batch-absolute int indices; rebase them by a
/// multiple of the ring capacity long before Integer.MAX_VALUE (mirrors C
/// verbatim — index differences and `% cap` residues are invariant).
fn emit_extrema_rebase(o: &mut String, model: &StreamModel, indent: usize) {
    if let Some(ex) = model.extrema() {
        let pad = " ".repeat(indent);
        let inner = " ".repeat(indent + 3);
        let mut vars: Vec<String> = vec![model.cursor.clone(), ex.trailing.clone()];
        vars.extend(ex.index_vars.iter().cloned());
        let _ = writeln!(o, "{pad}if( sp.{} >= 1073741824 ) {{", model.cursor);
        let _ = writeln!(
            o,
            "{inner}int rebaseShift = (sp.{} / sp.xCap) * sp.xCap;",
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
/// (the no-data guard AND the Metastock seed-boundary return) become the
/// in-band insufficient-history signal `OutOfRangeEndIndex` — the wrapper
/// types it as `InsufficientHistoryException`. Everything else passes through
/// (BAD_PARAM / ALLOC_ERR / INTERNAL_ERROR render natively; `retCode` locals
/// propagate a failed cross-call).
fn map_open_return(v: &str) -> String {
    match v {
        "SUCCESS" | "TA_SUCCESS" => "OutOfRangeEndIndex".to_string(),
        other => other.to_string(),
    }
}

/// Transcribe a batch body region for the Java open: output-array writes to
/// `lastValue_*` scalars (Scalar) or kept (Fill), previous-output feedback
/// reads to `lastValue_*` (Scalar), early-success returns mapped, the final
/// top-level return dropped (capture + `return RetCode.Success` replace it),
/// and the body's own dead identity branch deleted (its whole-range copies
/// reference output arrays that do not exist in Scalar mode).
fn build_open_body_java(model: &StreamModel, body: &[Statement], mode: OutMode) -> Vec<Statement> {
    let outputs = model.outputs.clone();
    let fb_outputs = model.out_feedback.clone();
    let scalar = mode == OutMode::Scalar;
    let fe = move |e: Expr| -> Expr {
        match e {
            Expr::ArrayAccess(nm, idx)
                if scalar && fb_outputs.contains(&nm) && streaming::is_prev_output_read(&idx) =>
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

/// The open-body emitter: `private RetCode <base>OpenBody(sp, in..., startIdx,
/// opts...)` (Scalar) or `private RetCode <base>OpenAndFillBody(sp, in...,
/// opts..., outBegIdx, outNBElement, outs...)` (Fill). `body` is the
/// transcribed batch region (loop tier: `model.body`; fast-path-skip:
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
    mode: OutMode,
) {
    emit_open_body_sig(o, func, mode);
    let open_body = build_open_body_java(model, body, mode);
    emit_open_prologue(o, func, &open_body, model, enums, registry, helpers, counter, stream_fma, mode);
    emit_identity_fast_path(o, func, model, fields, registry, helpers, stream_fma, counter, mode);
    emit_open_region(o, func, &open_body, enums, registry, helpers, counter, stream_fma, &[]);
    let cur_source = match mode {
        OutMode::Scalar => CurSource::LastValue,
        OutMode::Fill => CurSource::FillArray,
    };
    emit_capture(
        o, func, model, &model.state, step_settings, registry, helpers, stream_fma, counter,
        mode, Some(cur_source), "",
    );
    let _ = writeln!(o, "      return RetCode.Success;");
    let _ = writeln!(o, "   }}");
}

/// The open-body signature. Scalar: sp + inputs + startIdx + opts. Fill: sp +
/// inputs + opts + batch output tail (no startIdx — pinning bar 0 is what
/// makes the fill bit-exact).
fn emit_open_body_sig(o: &mut String, func: &FuncDef, mode: OutMode) {
    let base = java_base(func);
    let class = stream_class_name(func);
    let mut params: Vec<String> = vec![format!("{class} sp")];
    for input in streaming::input_array_names(func) {
        params.push(format!("double {input}[]"));
    }
    if mode == OutMode::Scalar {
        params.push("int startIdx".to_string());
    }
    for p in &func.optional_inputs {
        params.push(format!("{} {}", opt_param_java_type(&p.param_type), p.name));
    }
    let name = match mode {
        OutMode::Scalar => format!("{base}OpenBody"),
        OutMode::Fill => {
            params.push("MInteger outBegIdx".to_string());
            params.push("MInteger outNBElement".to_string());
            for out in &func.outputs {
                params.push(format!("{} {}[]", out_java_type(func, &out.name), out.name));
            }
            format!("{base}OpenAndFillBody")
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
    mode: OutMode,
) {
    emit_body_decls(o, func, open_body);
    emit_open_head(o, func, &model.outputs, mode);
    emit_open_validation(o, func, mode);
    emit_extras_and_candle(o, func, open_body, registry, helpers, counter, stream_fma);
    let _ = enums;
}

/// The transcribed body's VarDecls (address-of / MAType aware, mirroring the
/// batch renderer's decl pass), including CIRCBUF prologs.
fn emit_body_decls(o: &mut String, func: &FuncDef, open_body: &[Statement]) {
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
fn emit_open_head(o: &mut String, func: &FuncDef, outputs: &[String], mode: OutMode) {
    let inputs = streaming::input_array_names(func);
    let first = &inputs[0];
    if mode == OutMode::Scalar {
        let _ = writeln!(o, "      MInteger outBegIdx = new MInteger();");
        let _ = writeln!(o, "      MInteger outNBElement = new MInteger();");
        for out in outputs {
            let (t, d) = if out_is_int(func, out) { ("int", "0") } else { ("double", "0.0") };
            let _ = writeln!(o, "      {t} lastValue_{out} = {d};");
        }
    }
    let _ = writeln!(o, "      int historyLen = {first}.length;");
    let _ = writeln!(o, "      int endIdx = historyLen - 1;");
    if mode == OutMode::Fill {
        let _ = writeln!(o, "      int startIdx = 0;");
    }
}

/// Input-length + optional-param validation, and the Fill-mode aliasing guards.
fn emit_open_validation(o: &mut String, func: &FuncDef, mode: OutMode) {
    let inputs = streaming::input_array_names(func);
    let first = &inputs[0];
    let mut checks: Vec<String> = vec![format!("historyLen < 1")];
    for extra in &inputs[1..] {
        checks.push(format!("{extra}.length != {first}.length"));
    }
    let _ = writeln!(o, "      if( {} ) {{", checks.join(" || "));
    let _ = writeln!(o, "         return RetCode.BadParam;");
    let _ = writeln!(o, "      }}");
    o.push_str(&emit_opt_param_validation(func, "RetCode.BadParam"));
    if mode == OutMode::Fill {
        // Output aliasing guards: OpenAndFill writes outputs then reads the
        // input tail to seed rings — the batch tier's in==out allowance is
        // exactly what a one-pass fill must revoke. Reference equality is
        // complete in Java (arrays are identical or disjoint).
        let outs: Vec<&str> = func.outputs.iter().map(|out| out.name.as_str()).collect();
        let mut pairs: Vec<String> = Vec::new();
        for out in &outs {
            for input in &inputs {
                pairs.push(format!("(Object){out} == (Object){input}"));
            }
        }
        for i in 0..outs.len() {
            for b in &outs[i + 1..] {
                pairs.push(format!("(Object){} == (Object){b}", outs[i]));
            }
        }
        let _ = writeln!(o, "      if( {} ) {{", pairs.join(" || "));
        let _ = writeln!(o, "         return RetCode.BadParam;");
        let _ = writeln!(o, "      }}");
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
    let ctx = JavaRenderCtx {
        single_precision: false,
        address_of_vars: &address_of_vars,
        double_address_of_vars: &double_address_of_vars,
        float_input_params: &empty,
        inline_counter: counter,
        fma: Some(stream_fma),
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
        if matches!(stmt, Statement::VarDecl { .. }) {
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
    mode: OutMode,
) {
    let Some(idp) = &model.identity else { return };
    let base = java_base(func);
    let empty = HashSet::new();
    let ctx = stream_ctx(&empty, counter, stream_fma);
    let cond = render_expr(&idp.condition, &ctx, registry, helpers);
    let lb_args: Vec<String> = func.optional_inputs.iter().map(|p| p.name.clone()).collect();
    let lb_call = format!("{base}Lookback({})", lb_args.join(", "));
    let _ = writeln!(o, "      if( {cond} ) {{");
    let _ = writeln!(o, "         if( historyLen < {lb_call} + 1 ) {{");
    let _ = writeln!(o, "            return RetCode.OutOfRangeEndIndex;");
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
    match mode {
        OutMode::Scalar => {
            for (out, inp) in &idp.pairs {
                let _ = writeln!(o, "         sp.cur_{out} = {inp}[historyLen - 1];");
            }
        }
        OutMode::Fill => {
            let _ = writeln!(o, "         int fillLb = {lb_call};");
            let _ = writeln!(o, "         outBegIdx.value = fillLb;");
            let _ = writeln!(o, "         outNBElement.value = historyLen - fillLb;");
            let _ = writeln!(o, "         for( int fillIdx = 0; fillIdx < historyLen - fillLb; fillIdx++ ) {{");
            for (out, inp) in &idp.pairs {
                let _ = writeln!(o, "            {out}[fillIdx] = {inp}[fillLb + fillIdx];");
            }
            let _ = writeln!(o, "         }}");
            for (out, inp) in &idp.pairs {
                let _ = inp;
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
    mode: OutMode,
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
            if back > 0 {
                // Absolute-mod layout: bar j lives at j % cap.
                let _ = writeln!(
                    o,
                    "      for( int fillJ = historyLen - cap_{v}; fillJ < historyLen; fillJ++ ) {{"
                );
                let _ = writeln!(o, "         capRing_{v}_{arr}[fillJ % cap_{v}] = {arr}[fillJ];");
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
        for arr in &ex.arrays {
            let _ = writeln!(o, "      double[] capX_{arr} = new double[capX];");
        }
        // Absolute slots: bar j lives at j % cap (a plain tail copy would
        // break the automaton's phase).
        let _ = writeln!(o, "      for( int fillJ = historyLen - capX; fillJ < historyLen; fillJ++ ) {{");
        for arr in &ex.arrays {
            let _ = writeln!(o, "         capX_{arr}[fillJ % capX] = {arr}[fillJ];");
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
        match mode {
            OutMode::Scalar => {
                let _ = writeln!(o, "      sp.lastOut_{name} = lastValue_{name};");
            }
            OutMode::Fill => {
                let _ = writeln!(o, "      sp.lastOut_{name} = {name}[outNBElement.value - 1];");
            }
        }
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
        let _ = writeln!(o, "      sp.xCap = capX;");
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
    /// The Scalar-mode `lastValue_<out>` sink.
    LastValue,
    /// The Fill-mode caller array's last valid element.
    FillArray,
    /// The composed tier's `sc_<out>` scratch array (both modes).
    Scratch,
}

/// Seed `sp.cur_*` (+ the cached Value) at the end of an open body.
fn emit_cur_capture(o: &mut String, func: &FuncDef, outputs: &[String], source: CurSource) {
    for out in outputs {
        let expr = match source {
            CurSource::LastValue => format!("lastValue_{out}"),
            CurSource::FillArray => format!("{out}[outNBElement.value - 1]"),
            CurSource::Scratch => format!("sc_{out}[outNBElement.value - 1]"),
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
    let _ = writeln!(o, "      if( retCode == RetCode.OutOfRangeEndIndex ) {{");
    let _ = writeln!(
        o,
        "         throw new InsufficientHistoryException(\"TA_{n} {what}: history shorter than lookback + 1\");"
    );
    let _ = writeln!(o, "      }}");
    let _ = writeln!(o, "      if( retCode == RetCode.InternalError ) {{");
    let _ = writeln!(o, "         throw new IllegalStateException(\"TA_{n} {what}: internal error\");");
    let _ = writeln!(o, "      }}");
    let _ = writeln!(o, "      throw new IllegalArgumentException(\"TA_{n} {what}: \" + retCode);");
}

/// `openInternal` (composition seam), the public `<base>Open`, and the public
/// `<base>OpenAndFill`. Shared by every tier (the bodies differ, these don't).
fn emit_open_wrappers(o: &mut String, func: &FuncDef) {
    let base = java_base(func);
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

    // openInternal: startIdx-anchored composition seam (package-private).
    let _ = writeln!(
        o,
        "   /* Internal startIdx-anchored open behind {base}Open (composition seam). */"
    );
    let _ = writeln!(
        o,
        "   {class} {base}OpenInternal( {}, int startIdx{opt_sig_str} )\n   {{",
        in_sig.join(", ")
    );
    let _ = writeln!(o, "      {class} sp = new {class}(this);");
    let _ = writeln!(
        o,
        "      RetCode retCode = {base}OpenBody(sp, {}, startIdx{opt_fwd_str});",
        in_fwd.join(", ")
    );
    emit_reject_conversion(o, func, "open");
    let _ = writeln!(o, "   }}");

    // Public open.
    let _ = writeln!(
        o,
        "   /**\n\
         \x20   * Open a live {n} stream over the warm-up history; the handle's\n\
         \x20   * {{@code value()}} starts at the last history bar's value — bit-identical\n\
         \x20   * to {{@link Core#{base}}} at that bar.\n\
         \x20   * <p>The history must hold at least {{@code {base}Lookback(...) + 1}} bars\n\
         \x20   * (unstable-period aware), or {{@link InsufficientHistoryException}} is\n\
         \x20   * thrown. Out-of-range parameters throw {{@link IllegalArgumentException}}\n\
         \x20   * ({{@code Integer.MIN_VALUE}} selects an integer parameter's documented\n\
         \x20   * default, as in the batch API).\n\
         \x20   */"
    );
    let _ = writeln!(
        o,
        "   public {class} {base}Open( {}{opt_sig_str} )\n   {{",
        in_sig.join(", ")
    );
    let _ = writeln!(
        o,
        "      return {base}OpenInternal({}, 0{opt_fwd_str});",
        in_fwd.join(", ")
    );
    let _ = writeln!(o, "   }}");

    // Public openAndFill.
    let mut fill_sig: Vec<String> = in_sig.clone();
    for p in &opt_sig {
        fill_sig.push(p.clone());
    }
    fill_sig.push("MInteger outBegIdx".to_string());
    fill_sig.push("MInteger outNBElement".to_string());
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
    let _ = writeln!(
        o,
        "   /**\n\
         \x20   * {{@link Core#{base}Open}} that also fills the output array(s) bit-identically\n\
         \x20   * to {{@link Core#{base}}} over the whole history in the same single pass\n\
         \x20   * (no separate batch call needed for the warm-up plot). Output arrays must\n\
         \x20   * not alias the inputs or each other, and must hold\n\
         \x20   * {{@code historyLen - lookback}} values.\n\
         \x20   */"
    );
    let _ = writeln!(
        o,
        "   public {class} {base}OpenAndFill( {} )\n   {{",
        fill_sig.join(", ")
    );
    let _ = writeln!(o, "      {class} sp = new {class}(this);");
    let _ = writeln!(
        o,
        "      RetCode retCode = {base}OpenAndFillBody(sp, {});",
        fill_fwd.join(", ")
    );
    emit_reject_conversion(o, func, "openAndFill");
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
    assert!(
        ma.identity.is_none() && mb.identity.is_none(),
        "{}: dual-mode arms must not carry an identity path",
        func.name
    );
    let mut step_settings = detect_candle_settings(&ma.steady_stmts);
    step_settings.extend(detect_candle_settings(&mb.steady_stmts));

    let union_scalars = dual_scalar_union(func, ma, mb);
    // Both modes must carry IDENTICAL non-scalar state (TRIMA's odd/even arms
    // share the very same rings; DI/DM have none) — mirror C/Rust's assertion.
    assert!(
        state_fields_from(func, ma, &[], &step_settings)
            == state_fields_from(func, mb, &[], &step_settings),
        "{}: dual-mode modes carry differing non-scalar state; a real per-arm \
         union is not supported",
        func.name
    );
    let fields = state_fields_from(func, ma, &union_scalars, &step_settings);
    emit_handle_class(o, func, &fields, "");

    // --- step: one function, the mode re-derived from the stored param ------
    emit_step_sig(o, func);
    let empty = HashSet::new();
    let ctx = stream_ctx(&empty, counter, stream_fma);
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
    for mode in [OutMode::Scalar, OutMode::Fill] {
        emit_open_body_sig(o, func, mode);
        emit_open_head(o, func, &ma.outputs, mode);
        emit_open_validation(o, func, mode);
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
            let open_body = build_open_body_java(arm, &body, mode);
            let mut s = String::new();
            emit_body_decls(&mut s, func, &open_body);
            emit_extras_and_candle(&mut s, func, &open_body, registry, helpers, counter, stream_fma);
            emit_open_region(&mut s, func, &open_body, enums, registry, helpers, counter, stream_fma, &[]);
            let cur_source = match mode {
                OutMode::Scalar => CurSource::LastValue,
                OutMode::Fill => CurSource::FillArray,
            };
            emit_capture(
                &mut s, func, arm, &union_scalars, &step_settings, registry, helpers,
                stream_fma, counter, mode, Some(cur_source), "",
            );
            let _ = writeln!(s, "      return RetCode.Success;");
            o.push_str(&indent_block(&s, 3));
        }
        let _ = writeln!(o, "      }}");
        let _ = writeln!(o, "   }}");
    }

    emit_open_wrappers(o, func);
}

// ---------------------------------------------------------------------------
// Fast-path-skip tier (MIDPRICE): the loop-tier lifecycle on the general arm.
// ---------------------------------------------------------------------------

#[allow(clippy::too_many_arguments)]
fn emit_fastpath_skip(
    o: &mut String,
    func: &FuncDef,
    fp: &streaming::FastPathSkipPlan,
    stream_fma: &FmaVarSets,
    enums: &HashMap<String, EnumDef>,
    registry: &Registry,
    helpers: &HelperRegistry,
    counter: &Cell<usize>,
) {
    // The fast-path `then` arm is a batch-only perf specialization skipped by
    // the stream (bit-identical by construction; the differential gates
    // enforce it across the threshold).
    let mut tbody: Vec<Statement> = fp.prologue.to_vec();
    tbody.extend_from_slice(fp.model.body);
    tbody.extend_from_slice(fp.epilogue);
    emit_loop_shape(o, func, &fp.model, &tbody, stream_fma, enums, registry, helpers, counter);
}

// ---------------------------------------------------------------------------
// Dispatch tier (MA): params + an `Object sub` tagged by the stored enum
// param (the C `void *sub` model — Java has no payload enums at release 9).
// ---------------------------------------------------------------------------

/// `SmaStream` for callee `sma` — from the callee's JAVA base name (the same
/// authority as the callee's own generated handle: `ma` -> `MovingAverageStream`).
fn callee_stream_class(registry: &Registry, callee: &str) -> String {
    format!("{}Stream", capitalize_first(&registry.java_base(callee)))
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
    let base = java_base(func);
    let empty = HashSet::new();
    let ctx = stream_ctx(&empty, counter, stream_fma);
    let lb_args: Vec<String> = func.optional_inputs.iter().map(|p| p.name.clone()).collect();
    let lb_call = format!("{base}Lookback({})", lb_args.join(", "));

    // --- handle class -------------------------------------------------------
    let mut fields: Vec<Field> = Vec::new();
    for p in &func.optional_inputs {
        fields.push((p.name.clone(), opt_param_java_type(&p.param_type), p.name.clone()));
    }
    for out in &outputs {
        fields.push((format!("cur_{out}"), out_java_type(func, out).to_string(), "0".into()));
    }
    if has_value_class(func) {
        fields.push(("cachedValue".into(), "Value".into(), "null".into()));
    }
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
    emit_handle_class_with_members(o, func, &fields, &copy_extra, &extra_members);

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
                        "         sp.cur_{} = subValue.{};",
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
    for mode in [OutMode::Scalar, OutMode::Fill] {
        emit_open_body_sig(o, func, mode);
        let first = &inputs[0];
        let _ = writeln!(o, "      int historyLen = {first}.length;");
        emit_open_validation(o, func, mode);
        if let Some(idp) = &dp.identity {
            // The identity path FIRST (batch order — it applies to every arm).
            let cond = render_predicate(&idp.condition, &ctx, registry, helpers);
            let _ = writeln!(o, "      if( {cond} ) {{");
            let _ = writeln!(o, "         if( historyLen < {lb_call} + 1 ) {{");
            let _ = writeln!(o, "            return RetCode.OutOfRangeEndIndex;");
            let _ = writeln!(o, "         }}");
            for p in &func.optional_inputs {
                let _ = writeln!(o, "         sp.{0} = {0};", p.name);
            }
            let _ = writeln!(o, "         sp.sub = null;");
            match mode {
                OutMode::Scalar => {
                    for (out, inp) in &idp.pairs {
                        let _ = writeln!(o, "         sp.cur_{out} = {inp}[historyLen - 1];");
                    }
                }
                OutMode::Fill => {
                    let _ = writeln!(o, "         int fillLb = {lb_call};");
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
                let callee_base = registry.java_base(&arm.callee);
                let opts: Vec<String> = arm
                    .opt_args
                    .iter()
                    .map(|e| render_expr(e, &ctx, registry, helpers))
                    .collect();
                let _ = writeln!(o, "      case {label}: {{");
                match mode {
                    OutMode::Scalar => {
                        let opts = if opts.is_empty() {
                            String::new()
                        } else {
                            format!(", {}", opts.join(", "))
                        };
                        let _ = writeln!(
                            o,
                            "         {cls} sub = {callee_base}OpenInternal({bar_args}, startIdx{opts});"
                        );
                    }
                    OutMode::Fill => {
                        // OutSlot-mapped fill tail: Forward(k) passes the
                        // dispatch func's own array, Discard materializes a
                        // throwaway buffer (Java's rendering of C's NULL for a
                        // nullable output — the batch discard-buffer idiom, #125).
                        let fill_outs: String = arm
                            .out_map
                            .iter()
                            .map(|slot| match slot {
                                streaming::OutSlot::Forward(k) => outputs[*k].clone(),
                                streaming::OutSlot::Discard => {
                                    format!("new double[historyLen]")
                                }
                            })
                            .collect::<Vec<_>>()
                            .join(", ");
                        let opts = if opts.is_empty() {
                            String::new()
                        } else {
                            format!("{}, ", opts.join(", "))
                        };
                        let _ = writeln!(
                            o,
                            "         {cls} sub = {callee_base}OpenAndFill({bar_args}, {opts}outBegIdx, outNBElement, {fill_outs});"
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

    emit_open_wrappers(o, func);
}

// ---------------------------------------------------------------------------
// Period-bank tier (MAVP): a bank of sub-MA streams advanced in lockstep,
// selected per bar by the clamped variable period.
// ---------------------------------------------------------------------------

fn emit_period_bank(
    o: &mut String,
    func: &FuncDef,
    plan: &streaming::PeriodBankPlan,
    registry: &Registry,
    helpers: &HelperRegistry,
) {
    let _ = helpers;
    let callee = plan.callee.as_str();
    let callee_base = registry.java_base(callee);
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
    let mut fields: Vec<Field> = Vec::new();
    for p in &func.optional_inputs {
        fields.push((p.name.clone(), opt_param_java_type(&p.param_type), p.name.clone()));
    }
    for o_ in &func.outputs {
        fields.push((format!("cur_{}", o_.name), out_java_type(func, &o_.name).to_string(), "0".into()));
    }
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
    emit_handle_class_with_members(o, func, &fields, &copy_extra, &extra_members);

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
    emit_open_body_sig(o, func, OutMode::Scalar);
    let _ = writeln!(o, "      int historyLen = {price}.length;");
    emit_open_validation(o, func, OutMode::Scalar);
    let _ = writeln!(o, "      /* An inverted [min, max] period window is invalid (batch rejects). */");
    let _ = writeln!(o, "      if( {min} > {max} ) {{");
    let _ = writeln!(o, "         return RetCode.BadParam;");
    let _ = writeln!(o, "      }}");
    let _ = writeln!(
        o,
        "      /* Seed EVERY sub at the SHARED max-period lookback, exactly as batch\n\
         \x20      * does: it clamps startIdx up to lookback(maxPeriod) and calls the callee\n\
         \x20      * with that same start for every period. Seeding each sub at its own\n\
         \x20      * (smaller) lookback would seed the recurrence from a different bar and\n\
         \x20      * diverge for every period < maxPeriod. */"
    );
    let _ = writeln!(o, "      int lookbackTotal = {callee_base}Lookback({lb_args});");
    let _ = writeln!(o, "      int subStart = (startIdx < lookbackTotal)? lookbackTotal : startIdx;");
    let _ = writeln!(o, "      int nBank = {max} - {min} + 1;");
    let _ = writeln!(o, "      {subty}[] bank = new {subty}[nBank];");
    let _ = writeln!(o, "      for( int bankIdx = 0; bankIdx < nBank; bankIdx++ ) {{");
    let _ = writeln!(o, "         bank[bankIdx] = {callee_base}OpenInternal({price}, subStart, {open_opts});");
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
    let _ = writeln!(o, "      return RetCode.Success;");
    let _ = writeln!(o, "   }}");

    // --- open body (Fill): no per-bar array exists to un-discard (the bank
    // yields one selected scalar per bar), so fill genuinely re-runs history:
    // seed the bank on the first-output-bar prefix, emit that bar, then replay
    // updates over the remaining history selecting per bar. ------------------
    emit_open_body_sig(o, func, OutMode::Fill);
    let _ = writeln!(o, "      int historyLen = {price}.length;");
    emit_open_validation(o, func, OutMode::Fill);
    let _ = writeln!(o, "      /* An inverted [min, max] period window is invalid (batch rejects). */");
    let _ = writeln!(o, "      if( {min} > {max} ) {{");
    let _ = writeln!(o, "         return RetCode.BadParam;");
    let _ = writeln!(o, "      }}");
    let _ = writeln!(o, "      int lookbackTotal = {callee_base}Lookback({lb_args});");
    let _ = writeln!(o, "      if( historyLen < lookbackTotal + 1 ) {{");
    let _ = writeln!(o, "         return RetCode.OutOfRangeEndIndex;");
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
    let _ = writeln!(o, "         {subty} sub = {callee_base}OpenInternal(seedPrefix, lookbackTotal, {open_opts});");
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

    emit_open_wrappers(o, func);
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
    fn extrema_cap(&self) -> String {
        "sp.xCap".to_string()
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

/// The composed StreamStep: producer transition (writing `cur_<series>`), then
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
    for name in &cur_scalars {
        let _ = writeln!(o, "      double cur_{name} = 0.0;");
    }

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
                            "         cur_{d} = subOut{sub_idx}.{};",
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

/// Composed open body (Scalar = `OpenBody`, Fill = `OpenAndFillBody`):
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
    mode: OutMode,
) {
    // The composed fill/scratch path hardcodes double arrays (mirrors C/Rust).
    assert!(
        func.outputs.iter().all(|out| !out_is_int(func, &out.name)),
        "composed open assumes real (double) outputs; {} has an integer output",
        func.name
    );
    let empty = HashSet::new();
    let ctx = stream_ctx(&empty, counter, stream_fma);

    emit_open_body_sig(o, func, mode);
    let (region_stmts, tail_stmts) = build_composed_open_bodies(cp, outputs);
    let combined: Vec<Statement> = region_stmts
        .iter()
        .cloned()
        .chain(tail_stmts.iter().cloned())
        .collect();
    emit_body_decls(o, func, &combined);
    emit_open_head(o, func, &[], mode);
    emit_open_validation(o, func, mode);
    emit_extras_and_candle(o, func, &combined, registry, helpers, counter, stream_fma);
    for out in outputs {
        let _ = writeln!(o, "      double[] sc_{out} = new double[historyLen];");
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
        address_of_vars: &ins_address_of,
        double_address_of_vars: &ins_double_address_of,
        float_input_params: &empty,
        inline_counter: counter,
        fma: Some(stream_fma),
    };
    let region_len = region_stmts.len();
    let mut inserts: Vec<(usize, String)> = Vec::new();
    for (si, sub) in cp.subs.iter().enumerate() {
        let mut t = String::new();
        let callee_key = sub.callee.to_lowercase();
        let cls = callee_stream_class(registry, &callee_key);
        let callee_base = registry.java_base(&callee_key);
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
                format!("java.util.Arrays.copyOfRange({name}, 0, ({e_arg}) + 1)")
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
        let _ = writeln!(
            t,
            "      {cls} sub{si} = {callee_base}OpenInternal({}, {anchor}{opt_tail});",
            srcs.join(", ")
        );
        inserts.push((region_len + sub.tail_idx, t));
    }

    emit_open_region(o, func, &combined, enums, registry, helpers, counter, stream_fma, &inserts);

    // --- capture ------------------------------------------------------------
    let _ = writeln!(o, "      /* Capture the live producer state + sub handles. */");
    let _ = writeln!(o, "      if( outNBElement.value < 1 ) {{");
    let _ = writeln!(o, "         return RetCode.OutOfRangeEndIndex;");
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
    match &cp.producer {
        Some(model) => {
            // The producer's own "output" is the intermediate series, so its
            // cur seeding is suppressed; the real outputs seed from sc_ below.
            emit_capture(
                o, func, model, &model.state, step_settings, registry, helpers, stream_fma,
                counter, OutMode::Fill, None, &extra,
            );
        }
        None => {
            for p in &func.optional_inputs {
                let _ = writeln!(o, "      sp.{0} = {0};", p.name);
            }
            for (name, _) in &func.private_extra_params {
                let _ = writeln!(o, "      sp.{name} = {name};");
            }
            o.push_str(&extra);
        }
    }
    emit_cur_capture(o, func, outputs, CurSource::Scratch);
    if mode == OutMode::Fill {
        for out in outputs {
            let _ = writeln!(
                o,
                "      System.arraycopy(sc_{out}, 0, {out}, 0, outNBElement.value);"
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
        None => {
            let mut f: Vec<Field> = Vec::new();
            for p in &func.optional_inputs {
                f.push((p.name.clone(), opt_param_java_type(&p.param_type), p.name.clone()));
            }
            for (name, c_type) in &func.private_extra_params {
                f.push((name.clone(), extra_param_java_type(c_type).to_string(), name.clone()));
            }
            for out in &func.outputs {
                f.push((format!("cur_{}", out.name), out_java_type(func, &out.name).to_string(), "0".into()));
            }
            if has_value_class(func) {
                f.push(("cachedValue".into(), "Value".into(), "null".into()));
            }
            f
        }
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
    for (si, sub) in cp.subs.iter().enumerate() {
        let callee_key = sub.callee.to_lowercase();
        let cls = callee_stream_class(registry, &callee_key);
        let _ = writeln!(extra_members, "      {cls} sub{si};");
        let _ = writeln!(copy_extra, "         this.sub{si} = new {cls}(other.sub{si});");
    }
    emit_handle_class_with_members(o, func, &fields, &copy_extra, &extra_members);

    emit_composed_step(
        o, func, cp, &step_settings, stream_fma, registry, &inputs, &outputs, enums, helpers,
        counter,
    );
    emit_composed_open(
        o, func, cp, &step_settings, stream_fma, &outputs, enums, registry, helpers, counter,
        OutMode::Scalar,
    );
    emit_composed_open(
        o, func, cp, &step_settings, stream_fma, &outputs, enums, registry, helpers, counter,
        OutMode::Fill,
    );
    emit_open_wrappers(o, func);
}
