//! Generates the C# streaming API section appended to each shipped
//! `Core_<NAME>.cs` — the managed .NET sibling of `java_stream.rs` /
//! `rust_stream.rs` / `c_stream.rs`.
//!
//! Like the other three it consumes the backend-neutral [`crate::streaming`]
//! layer (`StreamPlan`, `StreamModel`, `build_transition`, the `NameMap` trait)
//! and renders through the existing [`super::csharp`] statement/expression
//! walkers. `streaming.rs` and the three shipped stream emitters are not
//! touched by this module — that is what keeps the other backends byte-frozen
//! by construction while this one lands.
//!
//! # Pinned decisions
//!
//! - **The handle is a `public sealed class <NAME>_Stream` nested in
//!   `partial class Core`**, with a *sibling* nested
//!   `public readonly record struct <NAME>_Value` for multi-output functions.
//!   The `Value` type cannot itself be named `Value`: a nested type and a
//!   member of the same name is CS0102, and the member name is the one every
//!   language's documentation references.
//!
//! - **Every handle field is `internal`, every constructor `internal`.** A
//!   *sibling* nested type cannot reach another's `private` members, and
//!   `MAVP_Stream`'s copy constructor builds `MA_Stream` copies while
//!   `MA_Stream`'s step calls `SMA_Stream.Update`. One rule beats per-field
//!   analysis, and `internal` is invisible to consumers. Internal constructors
//!   additionally stop `System.Text.Json` from minting a half-built handle,
//!   which is the positive act C# needs where Java gets not-serializable free.
//!
//! - **The step stays a method on `Core`, not on the handle.** Transcribed
//!   bodies render unstable-period reads as
//!   `this.unstablePeriod[(int)FuncUnstId.X]`, which only compiles inside a
//!   `Core` instance method. Measured, `core.Step(sp, x)` versus
//!   `this.Step(x)` is 3.44–3.54 against 3.39–3.47 ns/bar — indistinguishable.
//!
//! - **No `cachedValue` field.** Java caches the boxed multi-output `Value` so
//!   that `value()` allocates nothing; a `readonly record struct` return is
//!   0 B/update by construction (measured against 40 B/update for the
//!   Java-shaped class). One fewer field, one fewer store per bar, and one
//!   fewer thing the copy path can get wrong.
//!
//! - **The `NameMap` prefixes are Java's, verbatim** (`sp.x`, `sp.cur_y`,
//!   `sp.ring_v_a`, ...). This is load-bearing, not cosmetic:
//!   [`super::fma::stream_base`] strips exactly `sp->`, `sp.` and `cur_` to
//!   decide integer-versus-float typing, so any other scheme needs `fma.rs`
//!   extended, and getting that wrong is a ~1 ULP cross-language divergence
//!   with nothing pointing at the cause.
//!
//! - **Double-only.** `single_precision` is always `false` and no `float[]`
//!   overload is emitted; the streaming contract is `double` in every language.
//!
//! # Emission rules that are measurements, not preferences
//!
//! Each of these was measured on the shipped shape (dotnet 10, pinned cores,
//! interleaved arms, min-of-N over 3–5 process launches). They are recorded so
//! that a later reader does not re-optimize on intuition in either direction.
//!
//! - No `MethodImpl` attributes. `Update` is 3.39–3.51 ns/bar inlinable against
//!   6.69–7.10 behind `NoInlining`; the JIT's IL-size heuristic already inlines
//!   the small steps and correctly declines the ~400-byte candlestick ones.
//! - `Update`/`Peek`/`Value` stay thin — no validation, no null checks (there
//!   are no array arguments), no logging. That is what keeps them inlinable.
//! - No unsafe indexing: `MemoryMarshal.GetArrayDataReference` + `Unsafe.Add`
//!   measured 4.26–4.55 against 3.44–3.47 ns/bar — a *regression*.
//! - No array-hoisting pass (3.35–3.48 against 3.39–3.47 — noise). Hoist only a
//!   *counted* loop bound, where it genuinely drops a check.
//! - No `[StructLayout]`: the CLR uses `LayoutKind.Auto` for reference types
//!   and packs them itself; `Sequential` would disable that.
//! - Rings stay `double[]`/`int[]` fields. `Span<T>` cannot be a field (CS8345)
//!   and `Memory<T>` costs a span materialization per access.
//! - The copy constructor uses `new T[n]` + `Array.Copy`, never
//!   `(double[])x.Clone()` — 2.3x, and it is on `Peek`'s path for ~86 functions.
//! - Dispatch is a `switch` + cast, but *not* because virtual calls are slow:
//!   an interface call measured 4.41–4.74 against the switch's 5.55–5.72
//!   ns/bar. The switch wins on cross-language parity and on not adding a type
//!   hierarchy across 172 handles, and that is the whole argument.

use std::cell::Cell;
use std::collections::{BTreeSet, HashMap, HashSet};
use std::fmt::Write;

use crate::candle_settings::detect_candle_settings;
use crate::helper_registry::{hoist_block_helpers, HelperRegistry};
use crate::ir::{CircBuf, DocDef, EnumDef, Expr, FuncDef, OptInput, ParamType, Statement, VarType};
use crate::registry::Registry;
use crate::streaming::{self, StreamModel, StreamPlan};

use super::common::CANDLE_FNS;
use super::csharp::{
    cs_series_in, cs_series_out, cs_type_str, emit_opt_param_validation, opt_param_type_str,
    render_csharp_switch_label, render_expr, render_hoisted_blocks, render_statement_ctx,
    CsRenderCtx,
};
use super::fma::{self, FmaVarSets};
use super::java::{
    build_matype_map, circbuf_arrays, collect_address_of_vars, collect_double_address_of_vars,
    collect_matype_vars, is_boolean_expr,
};

/// Marker heading the generated stream section (tests slice on it; mirrors the
/// C, Rust and Java emitters).
pub const SECTION_MARKER: &str = "/**** Streaming API *****/";

/// Whether a C# stream section is emitted for this function.
///
/// Resolves `PRAGMA TA_ALT` here rather than at the caller, exactly as the Rust
/// and Java twins do: six functions carry an `_ALT1` body claiming the STREAM
/// tier, and analyzing `func.body` would silently analyze the batch-only block
/// scan instead.
pub fn emits_stream(func: &FuncDef, lookup: &dyn streaming::CalleeLookup) -> bool {
    if !func.streaming {
        return false;
    }
    streaming::validate_streamable(&func.resolved_for(crate::ir::Lang::CSharp), lookup).is_ok()
}

/// The base every C# identifier for this function is spelled from: the YAML
/// `name:` verbatim (`SMA`, `MA`, `CDL2CROWS`), matching `Lang::CSharp` in
/// `registry.rs`.
fn base_name(func: &FuncDef) -> String {
    func.name.clone()
}

/// Public handle class name, nested in `Core`: `SMA_Stream`.
pub fn stream_class_name(func: &FuncDef) -> String {
    format!("{}_Stream", base_name(func))
}

/// Multi-output value type name, a *sibling* nested type: `BBANDS_Value`.
///
/// C#-only concern: a nested type named `Value` alongside the `Value` member
/// every other language exposes is CS0102, so the type carries the function
/// prefix and the member keeps the documented name.
pub fn value_type_name(func: &FuncDef) -> String {
    format!("{}_Value", base_name(func))
}

/// The `<NAME>_Value` member name for an output: `outSlowK` → `SlowK`,
/// `outMACDSignal` → `MACDSignal`, `outMinIdx` → `MinIdx`.
///
/// Unlike Java's `value_field_name` this does *not* lowercase the leading caps
/// run — C# members are PascalCase, so stripping `out` is the whole
/// transformation. Every one of the corpus's distinct output names is already a
/// valid PascalCase identifier after the strip.
pub(crate) fn value_member_name(out_name: &str) -> String {
    let stripped = out_name.strip_prefix("out").unwrap_or(out_name);
    if stripped.is_empty() {
        out_name.to_string()
    } else {
        stripped.to_string()
    }
}

/// Whether an output is integer-typed (`outInteger`, `outMinIdx`).
fn out_is_int(func: &FuncDef, name: &str) -> bool {
    func.outputs
        .iter()
        .any(|o| o.name == name && o.param_type == ParamType::Integer)
}

/// `double` / `int` element type of an output.
fn out_cs_type(func: &FuncDef, name: &str) -> &'static str {
    if out_is_int(func, name) {
        "int"
    } else {
        "double"
    }
}

/// The output-series PARAMETER type of an output. All three call sites are
/// signatures, so this is the span form; the handle's own buffers are fields
/// and stay arrays (see [`field_type_and_default`]).
fn out_cs_param_type(func: &FuncDef, name: &str) -> String {
    cs_series_out(if out_is_int(func, name) { "int" } else { "double" })
}

/// Whether `Update`/`Peek`/`Value` return the multi-output `<NAME>_Value`.
fn has_value_type(func: &FuncDef) -> bool {
    func.outputs.len() > 1
}

/// The surface type of `Update`/`Peek`/`Value`.
fn value_surface_type(func: &FuncDef) -> String {
    if has_value_type(func) {
        value_type_name(func)
    } else {
        out_cs_type(func, &func.outputs[0].name).to_string()
    }
}

/// C# type of a private extra param (EMA's k factor): C type string → C#.
fn extra_param_cs_type(c_type: &str) -> &'static str {
    match c_type {
        "double" => "double",
        "int" => "int",
        other => panic!("unsupported private extra param type: {other}"),
    }
}

/// The `", double inHigh, double inLow"`-style bar parameter list, and the
/// matching forwarding list.
fn bar_params(func: &FuncDef) -> (String, String) {
    let inputs = streaming::input_array_names(func);
    let sig: Vec<String> = inputs.iter().map(|a| format!("double {a}")).collect();
    (sig.join(", "), inputs.join(", "))
}

// ---------------------------------------------------------------------------
// NameMap: state through `sp.`, bars as same-named scalars, outputs as
// `sp.cur_<name>` field writes.
//
// The spellings are Java's VERBATIM and that is load-bearing:
// `fma::stream_base` strips exactly `sp->`, `sp.` and `cur_` to decide
// integer-versus-float typing, so any other scheme silently moves the fusion
// sites — a ~1 ULP cross-language divergence with nothing pointing here.
// ---------------------------------------------------------------------------

struct CsStreamNames;

impl streaming::NameMap for CsStreamNames {
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

/// One handle field: (name, `cs_type`, identity-path default expression).
/// Order mirrors the C stream struct / Rust state struct / Java handle.
type Field = (String, String, String);

/// C# type + identity-path default for a carried scalar / temp.
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

/// The params + `cur_<out>` fields every tier's handle carries
/// (dispatch/period-bank/loopless-composed build on exactly this base).
///
/// There is no `cachedValue`: the C# `Value` is a `readonly record struct`
/// returned by value, so caching it would cost a field and a store per bar and
/// buy nothing.
fn base_fields(func: &FuncDef) -> Vec<Field> {
    let mut fields: Vec<Field> = Vec::new();
    for p in &func.optional_inputs {
        fields.push((p.name.clone(), opt_param_type_str(p).to_string(), p.name.clone()));
    }
    for (name, c_type) in &func.private_extra_params {
        fields.push((name.clone(), extra_param_cs_type(c_type).to_string(), name.clone()));
    }
    for out in &func.outputs {
        fields.push((
            format!("cur_{}", out.name),
            out_cs_type(func, &out.name).to_string(),
            "0".into(),
        ));
    }
    fields
}

/// The full ordered field list of the handle's state (loop-tier shape).
/// `step_settings` = candle settings the transition reads (snapshotted).
fn state_fields(
    func: &FuncDef,
    model: &StreamModel,
    step_settings: &BTreeSet<String>,
) -> Vec<Field> {
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
        fields.push((p.name.clone(), opt_param_type_str(p).to_string(), p.name.clone()));
    }
    for (name, c_type) in &func.private_extra_params {
        fields.push((
            name.clone(),
            extra_param_cs_type(c_type).to_string(),
            name.clone(),
        ));
    }
    for (name, ty) in scalars {
        let (cty, default) = field_type_and_default(ty);
        fields.push((name.clone(), cty, default));
    }
    for name in &model.out_feedback {
        let t = out_cs_type(func, name);
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
            fields.push((format!("x_{arr}"), "double[]".into(), "new double[1]".into()));
        }
    }
    // Candle-settings snapshot: the step reads these primitives, never the live
    // (mutable) CandleSetting table — frozen-at-open like Rust and Java.
    for s in step_settings {
        fields.push((format!("cs_{s}_rangeType"), "int".into(), "0".into()));
        fields.push((format!("cs_{s}_avgPeriod"), "int".into(), "0".into()));
        fields.push((format!("cs_{s}_factor"), "double".into(), "0.0".into()));
    }
    // The last committed value per output — `Value` reads these; `Update`
    // returns them; open's capture seeds them (the value at the last history
    // bar).
    for out in &func.outputs {
        let t = out_cs_type(func, &out.name);
        fields.push((format!("cur_{}", out.name), t.to_string(), "0".to_string()));
    }
    fields
}

// ---------------------------------------------------------------------------
// Entry point
// ---------------------------------------------------------------------------

/// Generate the whole stream section for one function's `Core_<NAME>.cs`.
///
/// Panics on analysis failure: the declared-tier gate in `main.rs` validates
/// every function over `ir::ALL_LANGS` first, so a failure here means the gate
/// was bypassed — fail loudly rather than silently emit nothing.
#[allow(clippy::implicit_hasher)]
pub fn generate(
    func: &FuncDef,
    enums: &HashMap<String, EnumDef>,
    registry: &Registry,
    helpers: &HelperRegistry,
) -> String {
    // Resolve `PRAGMA TA_ALT` here too — `generate` has direct callers, and six
    // functions carry an `_ALT1` body claiming the STREAM tier. Nothing below
    // may read `func.body`.
    let resolved = func.resolved_for(crate::ir::Lang::CSharp);
    let func: &FuncDef = &resolved;
    assert!(
        func.streaming,
        "csharp_stream::generate called without a streaming declaration"
    );
    let plan = streaming::validate_streamable(func, registry)
        .unwrap_or_else(|e| panic!("streaming gate: {e}"));

    // FMA fusion sites: same detector recipe as the C/Rust/Java stream
    // emitters, so the streamed per-bar code fuses `a*b+c` at the same sites as
    // the batch body. Bar inputs become bare scalar params, so seed them into
    // real_vars explicitly.
    //
    // `stream_source()`, not `private_body` and not `body`: the fusion sets
    // must be derived from the very body this emitter renders, and a
    // `PRAGMA TA_ALT={STREAM,...}` alternate is exactly where the two stop
    // being the same slice.
    let mut stream_fma =
        fma::build_fma_var_sets(func.stream_source(), &func.outputs, &fma::INDEX_PARAM_SEEDS);
    for input in streaming::input_array_names(func) {
        stream_fma.real_vars.insert(input);
    }

    let counter = Cell::new(0usize);
    let mut o = String::new();

    let _ = writeln!(o, "   {SECTION_MARKER}\n");
    if let Some(m) = func.alt_marker(crate::ir::Tier::Stream, crate::ir::Lang::CSharp) {
        let _ = writeln!(o, "   /* {m} */\n");
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

// ---------------------------------------------------------------------------
// Loop tier lifecycle
// ---------------------------------------------------------------------------

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
/// loop tier (`model.body`) and (from stage 2) dual-mode.
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
        o, func, model, body, &fields, &step_settings, stream_fma, enums, registry, helpers,
        counter,
    );
    emit_open_and_fill_internal_wrapper(o, func, true);
    emit_open_wrappers(o, func, true);
}

/// Prefix every non-empty line of `s` with `extra` spaces — cosmetic re-indent
/// of a shared-emitter block nested inside an arm branch.
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

/// The caller's optional params rewritten onto the handle (`optInTimePeriod` →
/// `sp.optInTimePeriod`): steps re-derive their predicates from the stored
/// immutable param — no mode tag is ever stored.
fn params_on_state(func: &FuncDef, e: &Expr) -> Expr {
    let params: Vec<String> = func.optional_inputs.iter().map(|p| p.name.clone()).collect();
    streaming::rewrite_expr(e, &|x| match x {
        Expr::Var(v) if params.contains(&v) => Expr::Var(format!("sp.{v}")),
        other => other,
    })
}

/// Render a C-truthy predicate as a C# boolean condition (the same `!= 0` wrap
/// the shared statement walker applies to `if` conditions).
fn render_predicate(
    e: &Expr,
    ctx: &CsRenderCtx,
    registry: &Registry,
    helpers: &HelperRegistry,
) -> String {
    let s = render_expr(e, ctx, registry, helpers);
    if is_boolean_expr(e, helpers) {
        s
    } else {
        format!("({s}) != 0")
    }
}

// ---------------------------------------------------------------------------
// XML documentation
// ---------------------------------------------------------------------------

/// Accumulates `///` lines for one member.
///
/// `csharp_doc.rs` has a richer builder, but it is private to that module and
/// this emitter may not reach into it. Every public member the stream section
/// declares is documented through this one type, which is what makes the
/// documentation all-or-nothing *by construction*: `GenerateDocumentationFile`
/// plus `TreatWarningsAsErrors` turn CS1591 (no `<summary>`) and CS1573 (some
/// `<param>`s documented but not all) into build errors.
///
/// `cref` discipline: never `<see cref="Core.SMA"/>` — the batch method is an
/// overload set and that is CS0419. Only single-signature members
/// (`SMA_Open`, `SMA_Lookback`, `SMA_Stream`, `Update`, ...) are cref-able;
/// everything else goes in prose as `<c>SMA</c>`.
struct XmlDoc {
    lines: Vec<String>,
}

impl XmlDoc {
    fn new() -> Self {
        Self { lines: Vec::new() }
    }

    fn open(&mut self, tag: &str) {
        self.lines.push(format!("<{tag}>"));
    }

    fn close(&mut self, tag: &str) {
        self.lines.push(format!("</{tag}>"));
    }

    /// A `<para>`-wrapped paragraph (inside `<remarks>`).
    fn para(&mut self, text: &str) {
        self.element("<para>", text, "</para>");
    }

    fn summary(&mut self, text: &str) {
        self.element("<summary>", text, "</summary>");
    }

    fn param(&mut self, name: &str, text: &str) {
        self.element(&format!("<param name=\"{name}\">"), text, "</param>");
    }

    fn returns(&mut self, text: &str) {
        self.element("<returns>", text, "</returns>");
    }

    fn exception(&mut self, cref: &str, text: &str) {
        self.element(&format!("<exception cref=\"{cref}\">"), text, "</exception>");
    }

    /// Emit `<open>text</close>`, on one line when short, wrapped otherwise.
    fn element(&mut self, open: &str, text: &str, close: &str) {
        let one_line = format!("{open}{text}{close}");
        if one_line.chars().count() <= 78 {
            self.lines.push(one_line);
            return;
        }
        let wrapped = wrap_doc(text, 74);
        let mut first = true;
        for line in wrapped {
            if first {
                self.lines.push(format!("{open}{line}"));
                first = false;
            } else {
                self.lines.push(line);
            }
        }
        if let Some(last) = self.lines.last_mut() {
            last.push_str(close);
        }
    }

    fn render(&self, indent: usize) -> String {
        let pad = " ".repeat(indent);
        let mut s = String::new();
        for line in &self.lines {
            let _ = writeln!(s, "{pad}/// {line}");
        }
        s
    }
}

/// Greedy word wrap that never breaks inside an XML tag (`<see cref="X"/>` has
/// a space in it, and splitting there would be legal but unreadable).
fn wrap_doc(text: &str, width: usize) -> Vec<String> {
    let mut out: Vec<String> = Vec::new();
    let mut line = String::new();
    let mut word = String::new();
    let mut depth = 0usize;

    for c in text.chars() {
        match c {
            '<' => {
                depth += 1;
                word.push(c);
            }
            '>' => {
                depth = depth.saturating_sub(1);
                word.push(c);
            }
            ' ' | '\n' if depth == 0 => flush_word(&mut line, &mut word, &mut out, width),
            _ => word.push(c),
        }
    }
    flush_word(&mut line, &mut word, &mut out, width);
    if !line.is_empty() {
        out.push(line);
    }
    out
}

fn flush_word(line: &mut String, word: &mut String, out: &mut Vec<String>, width: usize) {
    if word.is_empty() {
        return;
    }
    if !line.is_empty() && line.chars().count() + 1 + word.chars().count() > width {
        out.push(std::mem::take(line));
    }
    if !line.is_empty() {
        line.push(' ');
    }
    line.push_str(word);
    word.clear();
}

/// Canonical prose → XML-doc-safe text: `&`/`<`/`>` become entities (CS1570
/// otherwise) and backtick spans become `<c>...</c>`. The batch tier's `csdoc`
/// is private to `csharp_doc`, and this emitter may not reach into it.
fn csdoc(text: &str) -> String {
    let mut out = String::new();
    let mut in_code = false;
    for c in text.chars() {
        match c {
            '`' => {
                out.push_str(if in_code { "</c>" } else { "<c>" });
                in_code = !in_code;
            }
            '&' => out.push_str("&amp;"),
            '<' => out.push_str("&lt;"),
            '>' => out.push_str("&gt;"),
            '\n' => out.push(' '),
            _ => out.push(c),
        }
    }
    if in_code {
        // Unbalanced backtick in the source — close it rather than emit bad XML.
        out.push_str("</c>");
    }
    out
}

/// Prose for one bar/history input, mirroring the batch tier's fallbacks.
fn input_desc(name: &str, doc: &DocDef) -> String {
    if let Some((_, desc)) = doc.inputs.iter().find(|(n, _)| n == name) {
        return super::doc_meta::ensure_period(&csdoc(desc));
    }
    match name {
        "inOpen" => "Open price per bar.",
        "inHigh" => "High price per bar.",
        "inLow" => "Low price per bar.",
        "inClose" => "Close price per bar.",
        "inVolume" => "Volume per bar.",
        "inOpenInterest" => "Open interest per bar.",
        _ => "Input data series.",
    }
    .to_string()
}

/// Whether any output of this function is an absolute bar index.
///
/// Detected from the documented prose rather than the type, because plenty of
/// integer outputs are flags (every `CDL*` pattern) and only these carry an
/// index basis that streaming changes the meaning of.
fn has_absolute_index_output(func: &FuncDef) -> bool {
    func.doc.as_ref().is_some_and(|d| {
        d.outputs
            .iter()
            .any(|(_, desc)| desc.to_lowercase().contains("absolute index"))
    })
}

/// Prose for one bar argument of `Update` / `Peek`.
///
/// Deliberately NOT [`input_desc`]: that describes the batch tier's *array*
/// parameter ("Source series to average"), and reusing it on a `double`
/// argument names the wrong thing — the caller is passing one bar, not a
/// series. The price components get their own bar-scoped wording; anything else
/// is described by naming the series it belongs to, which stays true for every
/// indicator without restating the batch prose.
fn bar_param_desc(name: &str) -> String {
    match name {
        "inOpen" => "This bar's open price.".to_string(),
        "inHigh" => "This bar's high price.".to_string(),
        "inLow" => "This bar's low price.".to_string(),
        "inClose" => "This bar's close price.".to_string(),
        "inVolume" => "This bar's volume.".to_string(),
        "inOpenInterest" => "This bar's open interest.".to_string(),
        "inPeriods" => "The period to use for this bar.".to_string(),
        other => format!("This bar's value for <c>{other}</c>."),
    }
}

/// Prose for one optional parameter on a stream opener. The batch tier's full
/// default/range machinery is private to `csharp_doc`, so the opener points at
/// the batch call rather than restating it — one place for the numbers.
fn opt_param_desc(base: &str, opt: &OptInput) -> String {
    let sentinel = match opt.param_type {
        ParamType::Real => "<c>-4e37</c>",
        _ => "<c>int.MinValue</c>",
    };
    format!(
        "As in the batch call; see <see cref=\"{base}_Lookback\"/> for its default \
         and range ({sentinel} selects the default)."
    )
}

// ---------------------------------------------------------------------------
// Handle class
// ---------------------------------------------------------------------------

/// The tier-owned members of a handle beyond `fields`: the statements that
/// deep-copy them into a fresh handle, and the statements that overwrite them
/// in an existing one without allocating. Both are raw C#; the loop tier owns
/// none and passes [`SubMembers::none`].
struct SubMembers {
    /// Deep-copy statements for the copy constructor.
    copy: String,
    /// In-place overwrite statements for `CopyFrom`.
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

/// Emit the sibling `<NAME>_Value` record struct and the nested handle class.
/// `subs` holds the tier-owned members (sub-handle copies); the loop tier passes
/// [`SubMembers::none`].
fn emit_handle_class(o: &mut String, func: &FuncDef, fields: &[Field], subs: &SubMembers) -> bool {
    emit_handle_class_with_members(o, func, fields, subs, "")
}

/// [`emit_handle_class`] with additional raw member declarations (dispatch's
/// `object? sub;`, composed/period-bank sub-handle fields). Returns whether the
/// handle owns enough on the heap for `Peek` to reuse a scratch.
#[allow(clippy::too_many_lines)]
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

    // The multi-output value type is a SIBLING of the handle, not a member of
    // it: `Value` as both a nested type and a member is CS0102, and the member
    // is the name every language's documentation references.
    emit_value_type(o, func);

    let mut d = XmlDoc::new();
    d.summary(&format!(
        "A live <c>{n}</c> stream: one value per closed bar, bit-identical to \
         <c>{n}</c> over the same series."
    ));
    d.open("remarks");
    d.para(&format!(
        "Open with <see cref=\"Core.{base}_Open\"/>. There is no close and nothing to \
         dispose — the handle is ordinary managed state, and an unreferenced handle is \
         simply collected."
    ));
    d.para(
        "Concurrency: a handle is single-writer — <see cref=\"Update\"/>, \
         <see cref=\"Peek\"/>, <see cref=\"Value\"/> and <see cref=\"Clone\"/> must not \
         race with an <c>Update</c> on the same handle. With no concurrent <c>Update</c>, \
         <c>Peek</c>, <c>Value</c> and <c>Clone</c> never write the handle. Independent \
         handles (a <c>Clone</c> result included) are fully independent.",
    );
    d.para(
        "Not serializable by design, and the constructors are internal so no partially \
         built handle can be minted: to checkpoint, retain the history and re-open — the \
         result is bit-identical by contract.",
    );
    // Absolute-index outputs need a streaming-specific caveat the batch prose
    // cannot carry: batch describes them as an index INTO the input array, and
    // in this tier there is no array — the bar argument is a scalar. The basis
    // is also rebased once the bar count passes 2^30, so the value is a window
    // position, never a durable bar id.
    if has_absolute_index_output(func) {
        d.para(
            "This indicator reports absolute bar indices. In the streaming tier they \
             count bars fed to this stream rather than positions in an array, and the \
             basis is shifted once that count passes 2^30 — so treat an index as a \
             position within the current window, not as an identifier you can store \
             and compare against one read much later.",
        );
    }
    d.close("remarks");
    o.push_str(&d.render(3));

    let _ = writeln!(o, "   public sealed class {class}");
    let _ = writeln!(o, "   {{");
    // Not readonly: `CopyFrom` retargets the peek scratch, which is one instance
    // per thread per class and so outlives any one handle's Core.
    let _ = writeln!(o, "      internal Core core;");
    for (name, cty, _) in fields {
        // `= []` on every array field is mandatory, not tidiness: CS8618
        // (non-nullable field not initialised by every constructor) is an error
        // under this csproj. It lowers to the cached `Array.Empty<T>()` and is
        // overwritten by `<N>_OpenImpl` before the handle escapes.
        let init = if cty.ends_with("[]") { " = []" } else { "" };
        let _ = writeln!(o, "      internal {cty} {name}{init};");
    }
    o.push_str(extra_members);
    // The bars this handle has produced a value for (issue #241). Two ints
    // rather than an `OutRange` field: `OutRange` is a readonly struct, so a
    // per-bar count bump would have to rebuild it, and `Update` is the hot path.
    let _ = writeln!(o, "      internal int outRangeBegIdx;");
    let _ = writeln!(o, "      internal int outRangeCount;");

    let _ = writeln!(o, "\n      internal {class}( Core core ) {{ this.core = core; }}");

    let mut d = XmlDoc::new();
    d.summary(
        "The bars this stream has produced a value for, in the input series' \
         coordinates: <c>[BegIdx, BegIdx + Count)</c>.",
    );
    d.open("remarks");
    d.para(&format!(
        "It is what <c>Core.{base}</c> reports over the same bars: the opener sets it to \
         <c>(lookback, historyLen - lookback)</c>, every accepted <c>Update</c> adds one to \
         the count, <c>Peek</c> leaves it alone, and <c>Clone</c> carries it verbatim. A \
         plain <c>Open</c> hands back only the last value, a subset of this range, because \
         the caller chose not to take the fill."
    ));
    d.close("remarks");
    o.push('\n');
    o.push_str(&d.render(6));
    // A property whose name is its own type: C#'s "Color Color" rule (§12.8.7.2)
    // makes this legal, and nothing inside the handle class names the type by a
    // bare simple name any more.
    let _ = writeln!(
        o,
        "      public OutRange OutRange => new OutRange(outRangeBegIdx, outRangeCount);"
    );

    // Deep-copy constructor: scalars assign, arrays are allocated and copied
    // (never `(double[])x.Clone()` — 2.3x slower, and this is on Peek's path),
    // sub-handles copy recursively; the Core reference is shared (settings
    // identity is the contract).
    let _ = writeln!(o, "\n      internal {class}( {class} other )");
    let _ = writeln!(o, "      {{");
    let _ = writeln!(o, "         this.core = other.core;");
    for (name, cty, _) in fields {
        if let Some(elem) = cty.strip_suffix("[]") {
            let _ = writeln!(o, "         this.{name} = new {elem}[other.{name}.Length];");
            let _ = writeln!(o, "         Array.Copy( other.{name}, this.{name}, other.{name}.Length );");
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
    // Only `Peek`'s scratch calls it, and only where there is an allocation to
    // save (#201). No `!= null` test on the way in — every array field carries
    // an `= []` initialiser, so under `Nullable=enable` the length branch is
    // both sufficient and the only check the compiler accepts without a
    // diagnostic.
    let mut arrays = 0;
    let _ = writeln!(o, "\n      internal void CopyFrom( {class} other )");
    let _ = writeln!(o, "      {{");
    let _ = writeln!(o, "         this.core = other.core;");
    for (name, cty, _) in fields {
        if let Some(elem) = cty.strip_suffix("[]") {
            arrays += 1;
            let _ = writeln!(o, "         if( this.{name}.Length != other.{name}.Length ) {{");
            let _ = writeln!(o, "            this.{name} = new {elem}[other.{name}.Length];");
            let _ = writeln!(o, "         }}");
            let _ = writeln!(o, "         Array.Copy( other.{name}, this.{name}, other.{name}.Length );");
        } else {
            let _ = writeln!(o, "         this.{name} = other.{name};");
        }
    }
    o.push_str(&subs.restore);
    let _ = writeln!(o, "         this.outRangeBegIdx = other.outRangeBegIdx;");
    let _ = writeln!(o, "         this.outRangeCount = other.outRangeCount;");
    let _ = writeln!(o, "      }}");

    // What `Peek` trades away by reusing a scratch is one thread-static read,
    // and what it buys back is the allocation of a peer handle. For one small
    // array that is a wash — measured in Java, a slight loss — so the reuse is
    // for the shapes where the copy is several arrays or a sub-stream tree
    // (#201). The predicate is Java's, shipped unchanged: there is no C#
    // benchmark row to defend a different one with, and a re-derivation without
    // a control arm would be a guess wearing a number.
    let reuse = subs.unbounded || subs.subs >= 2 || arrays >= 2;
    if reuse {
        // `[ThreadStatic]`, not `ThreadLocal<T>`: `ThreadLocal<T>` is itself
        // `IDisposable`, the wrong signal inside the one tier whose thesis is
        // that it needs no `Dispose`. No inline initialiser — a thread-static
        // field initialiser runs on the first thread only, so the null check in
        // `Peek` is what creates it lazily, per thread.
        let _ = writeln!(
            o,
            "\n      /* Peek's reusable scratch — one per thread, see CopyFrom. */"
        );
        let _ = writeln!(o, "      [ThreadStatic] private static {class}? peekScratch;");
    }

    emit_update_peek_value_clone(o, func, reuse);

    let _ = writeln!(o, "   }}");
    reuse
}

/// The immutable multi-output value type: a *sibling* nested
/// `public readonly record struct` in batch output order, components named
/// after the outputs (`outSlowK` → `SlowK`).
///
/// A record struct, not Java's record class: the return is copied to the
/// caller's frame, so `Update` allocates nothing at all (measured 0 B/update
/// against 40 B/update for the Java-shaped cached class), which is also why
/// there is no `cachedValue` field to keep it free. `Deconstruct` comes free —
/// `var (up, mid, low) = s.Update(bar);`.
fn emit_value_type(o: &mut String, func: &FuncDef) {
    if !has_value_type(func) {
        return;
    }
    let vt = value_type_name(func);
    let n = func.name.to_uppercase();
    let empty = DocDef::default();
    let doc = func.doc.as_ref().unwrap_or(&empty);

    let components: Vec<String> = func
        .outputs
        .iter()
        .map(|out| {
            format!(
                "{} {}",
                out_cs_type(func, &out.name),
                value_member_name(&out.name)
            )
        })
        .collect();

    let mut d = XmlDoc::new();
    d.summary(&format!("One <c>{n}</c> output set, in batch output order."));
    d.open("remarks");
    d.para(
        "Equality is the compiler-generated record-struct equality, which compares the \
         components with <c>==</c>: <c>NaN</c> does not equal <c>NaN</c>, and <c>0.0</c> \
         equals <c>-0.0</c>. That is deliberately <em>not</em> the Java <c>Value</c> \
         contract, which compares bitwise — compare \
         <see cref=\"System.BitConverter.DoubleToInt64Bits(double)\"/> per component when \
         bit-level identity is what you mean.",
    );
    d.close("remarks");
    for out in &func.outputs {
        // Same prose the batch method's `<param name="out…">` carries, so an
        // output reads identically in both tiers.
        d.param(
            &value_member_name(&out.name),
            &super::csharp_doc::output_desc(out, doc),
        );
    }
    o.push_str(&d.render(3));
    let _ = writeln!(
        o,
        "   public readonly record struct {vt}( {} );\n",
        components.join(", ")
    );
}

/// The value expression reading the current outputs off a handle variable.
fn fresh_value_expr(func: &FuncDef, handle_var: &str) -> String {
    let prefix = if handle_var.is_empty() {
        String::new()
    } else {
        format!("{handle_var}.")
    };
    if has_value_type(func) {
        let args: Vec<String> = func
            .outputs
            .iter()
            .map(|out| format!("{prefix}cur_{}", out.name))
            .collect();
        format!("new {}({})", value_type_name(func), args.join(", "))
    } else {
        format!("{prefix}cur_{}", func.outputs[0].name)
    }
}

/// The per-bar finite-input rejection for `Update`/`Peek`: one `double.IsFinite`
/// per scalar bar input, before the handle is touched.
///
/// The streaming tier's half of the boundary contract (see
/// `docs/streaming-api-design.md`). Batch does not filter — it computes on
/// whatever it is handed. A handle cannot do that, because its state is
/// retained: one non-finite bar poisons every recursive accumulator in it for
/// the rest of its life, long after the feed recovers.
///
/// Routed through `Core.StreamFailure` so the message prefix and the exception
/// type match the open rejections exactly.
fn finite_bar_check(func: &FuncDef, indent: &str, what: &str) -> String {
    let bars = streaming::input_array_names(func);
    if bars.is_empty() {
        return String::new();
    }
    let n = base_name(func);
    let conds: Vec<String> = bars.iter().map(|b| format!("!double.IsFinite({b})")).collect();
    format!(
        "{indent}if( {} ) throw Core.StreamFailure(\"{n}\", \"{what}\", RetCode.BadParam);\n",
        conds.join(" || ")
    )
}


fn emit_update_peek_value_clone(o: &mut String, func: &FuncDef, reuse: bool) {
    emit_update_method(o, func);
    emit_peek_method(o, func, reuse);
    emit_update_and_fill_method(o, func);
    emit_value_property(o, func);
    emit_clone_method(o, func);
}

// --- Update ------------------------------------------------------------------
fn emit_update_method(o: &mut String, func: &FuncDef) {
    let base = base_name(func);
    let vt = value_surface_type(func);
    let (sig_bars, fwd_bars) = bar_params(func);
    let inputs = streaming::input_array_names(func);

    let mut d = XmlDoc::new();
    d.summary("Commit one closed bar, returning the new current value.");
    d.open("remarks");
    d.para(
        "Allocates nothing — neither handle state nor a return value.",
    );
    d.para(
        "Throws <see cref=\"System.ArgumentException\"/> if any bar value is not finite \
         (NaN or an infinity). That check runs before anything is written, so the handle \
         is left exactly as it was and the stream stays usable: skip the bar, or re-open \
         on a clean history. This is the one place the streaming tier is stricter than \
         the batch API, which computes on whatever it is given: a handle retains its \
         state, so a single non-finite bar would poison every later value it produces.",
    );
    d.close("remarks");
    for input in &inputs {
        d.param(input, &bar_param_desc(input));
    }
    d.returns("The value at the bar just committed.");
    o.push('\n');
    o.push_str(&d.render(6));
    let _ = writeln!(o, "      public {vt} Update( {sig_bars} )");
    let _ = writeln!(o, "      {{");
    o.push_str(&finite_bar_check(func, "         ", "update"));
    let _ = writeln!(o, "         core.{base}_StepImpl(this, {fwd_bars});");
    // After the step and after the finite-bar reject, so a rejected bar leaves
    // the range where it was. `Peek` runs the same step on a scratch copy and so
    // never reaches this. Saturating: nothing bounds how many bars a live stream
    // is fed, and past MAX_INDEX it has left the batch index domain anyway.
    let _ = writeln!(
        o,
        "         if( outRangeCount < Core.MAX_INDEX ) outRangeCount++;"
    );
    let _ = writeln!(o, "         return {};", fresh_value_expr(func, ""));
    let _ = writeln!(o, "      }}");
}

// --- Peek ----------------------------------------------------------------------
fn emit_peek_method(o: &mut String, func: &FuncDef, reuse: bool) {
    let class = stream_class_name(func);
    let base = base_name(func);
    let vt = value_surface_type(func);
    let (sig_bars, fwd_bars) = bar_params(func);
    let inputs = streaming::input_array_names(func);

    // State what the caller can observe, never a comparative performance claim.
    // The scratch-election predicate is Java's, shipped unchanged because no C#
    // A/B exists to justify a different one — so the emitted docs must not
    // assert to a .NET consumer that one arm is "cheaper", which is a
    // conclusion measured on another runtime. The allocation itself is a fact
    // and is worth telling them, because `Peek`-per-tick is the obvious usage.
    let alloc_note = if reuse {
        "It runs on a scratch handle held per thread and reused, so it allocates nothing \
         after this thread's first peek of this indicator. That scratch is retained for \
         the life of the thread."
    } else {
        "It runs on a fresh copy of this handle, so it allocates one — proportional to \
         the state this indicator carries. If you peek on every tick and that matters, \
         hold the value <see cref=\"Update\"/> returns instead."
    };
    let mut d = XmlDoc::new();
    d.summary("Evaluate a forming bar without committing it.");
    d.open("remarks");
    d.para(
        "Bit-identical to what the next <see cref=\"Update\"/> with the same bar would \
         return — it is the same generated code, run on a copy. Never writes this handle, \
         so peeks may run concurrently with each other.",
    );
    d.para(alloc_note);
    d.close("remarks");
    for input in &inputs {
        d.param(input, &bar_param_desc(input));
    }
    d.returns("What <see cref=\"Update\"/> would return for this bar.");
    o.push('\n');
    o.push_str(&d.render(6));
    let _ = writeln!(o, "      public {vt} Peek( {sig_bars} )");
    let _ = writeln!(o, "      {{");
    // Ahead of the scratch copy, not left to the step: a rejected bar must not
    // pay for a handle copy.
    o.push_str(&finite_bar_check(func, "         ", "peek"));
    if reuse {
        // Per thread, not per handle: `Peek` must not write the handle (two
        // threads may peek the same one), and a thread-static keeps the reuse
        // bounded — one scratch per thread per indicator, whatever the number of
        // live handles. `CopyFrom` retargets it, `core` included.
        let _ = writeln!(o, "         {class}? scratch = peekScratch;");
        let _ = writeln!(o, "         if( scratch is null ) {{");
        let _ = writeln!(o, "            scratch = new {class}(this);");
        let _ = writeln!(o, "            peekScratch = scratch;");
        let _ = writeln!(o, "         }} else {{");
        let _ = writeln!(o, "            scratch.CopyFrom(this);");
        let _ = writeln!(o, "         }}");
    } else {
        let _ = writeln!(o, "         {class} scratch = new {class}(this);");
    }
    let _ = writeln!(o, "         core.{base}_StepImpl(scratch, {fwd_bars});");
    let _ = writeln!(o, "         return {};", fresh_value_expr(func, "scratch"));
    let _ = writeln!(o, "      }}");
}

// --- UpdateAndFill ---------------------------------------------------------------
/// `UpdateAndFill`'s XML doc — hoisted so the emitter itself stays readable.
fn update_and_fill_doc(func: &FuncDef, inputs: &[String]) -> XmlDoc {
    let mut d = XmlDoc::new();
    d.summary(
        "Commit <c>n</c> closed bars and write their <c>n</c> values, in one call.",
    );
    d.open("remarks");
    d.para(
        "Exactly <c>n</c> back-to-back <see cref=\"Update\"/> calls, with one set of \
         argument checks instead of <c>n</c>. The outputs must hold at least <c>n</c> \
         values and must not overlap an input or each other.",
    );
    d.para(
        "<see cref=\"OutRange\"/> counts what was committed, which is what makes a \
         rejection readable: a non-finite bar <c>k</c> throws \
         <see cref=\"System.ArgumentException\"/> exactly as <see cref=\"Update\"/> would, \
         with bars <c>0..k</c> committed and written, bar <c>k</c> and everything after it \
         not, and the count advanced by <c>k</c>.",
    );
    // Rule U6a reads the same as S6a, and a caller of this tier needs telling in
    // the same place a caller of the opener is told.
    {
        let names = super::common::nullable_output_list(func);
        if !names.is_empty() {
            let list = names
                .iter()
                .map(|n| format!("<c>{n}</c>"))
                .collect::<Vec<_>>()
                .join(", ");
            d.para(&format!(
                "{list} may be declined with an empty span, per call and independently of \
                 what the opener was given: the value is still computed — \
                 <see cref=\"Value\"/> reports it — and nothing is written out."
            ));
        }
    }
    d.close("remarks");
    for input in inputs {
        d.param(input, &format!("Closed bars for <c>{input}</c>, oldest first."));
    }
    for out in &func.outputs {
        d.param(
            &out.name,
            &format!("Receives one <c>{}</c> value per bar committed.", out.name),
        );
    }
    d
}

fn emit_update_and_fill_method(o: &mut String, func: &FuncDef) {
    let base = base_name(func);
    let inputs = streaming::input_array_names(func);

    // One emitter for every tier: each owns a `<base>_StepImpl` with the same
    // surface, so the n-bar filler is that step in a loop (issue #246). No
    // `Value` cache to keep in step on the way out, unlike Java: a multi-output
    // `Value` here is a record struct built fresh from the handle's fields.
    let mut sig = String::new();
    for a in &inputs {
        let _ = write!(sig, "ReadOnlySpan<double> {a}, ");
    }
    for out in &func.outputs {
        let t = if out_is_int(func, &out.name) { "int" } else { "double" };
        let _ = write!(sig, "Span<{t}> {}, ", out.name);
    }
    let sig = sig.trim_end_matches(", ");
    let count_src = inputs
        .first()
        .map_or_else(|| "0".to_string(), |a| format!("{a}.Length"));
    o.push('\n');
    o.push_str(&update_and_fill_doc(func, &inputs).render(6));
    let _ = writeln!(o, "      public void UpdateAndFill( {sig} )");
    let _ = writeln!(o, "      {{");
    let _ = writeln!(o, "         int barCount = {count_src};");
    let mut checks: Vec<String> = inputs
        .iter()
        .skip(1)
        .map(|a| format!("{a}.Length != barCount"))
        .collect();
    // A `nullable` output may be declined here exactly as at the opener (rule
    // U6a), per call: bounded only where it was supplied, and its store guarded.
    // Nothing recorded at `Open` constrains what this call presents. An empty
    // span IS the declination, as it is at the opener — a span cannot be null.
    let nullable = super::common::nullable_output_names(func);
    for out in &func.outputs {
        if nullable.contains(&out.name) {
            checks.push(format!("(!{0}.IsEmpty && {0}.Length < barCount)", out.name));
        } else {
            checks.push(format!("{}.Length < barCount", out.name));
        }
    }
    if let Some(alias) = alias_condition(func, &inputs) {
        checks.push(alias);
    }
    if !checks.is_empty() {
        let _ = writeln!(
            o,
            "         if( {} ) throw Core.StreamFailure(\"{base}\", \"updateAndFill\", RetCode.BadParam);",
            checks.join(" || ")
        );
    }
    let _ = writeln!(o, "         for( int i = 0; i < barCount; i++ )");
    let _ = writeln!(o, "         {{");
    if !inputs.is_empty() {
        let conds: Vec<String> = inputs
            .iter()
            .map(|b| format!("!double.IsFinite({b}[i])"))
            .collect();
        let _ = writeln!(
            o,
            "            if( {} ) throw Core.StreamFailure(\"{base}\", \"updateAndFill\", RetCode.BadParam);",
            conds.join(" || ")
        );
    }
    let idx_bars: Vec<String> = inputs.iter().map(|a| format!("{a}[i]")).collect();
    let _ = writeln!(o, "            core.{base}_StepImpl(this, {});", idx_bars.join(", "));
    for out in &func.outputs {
        let name = &out.name;
        let guard = if nullable.contains(name) { format!("if( !{name}.IsEmpty ) ") } else { String::new() };
        let _ = writeln!(o, "            {guard}{name}[i] = cur_{name};");
    }
    let _ = writeln!(o, "            if( outRangeCount < Core.MAX_INDEX ) outRangeCount++;");
    let _ = writeln!(o, "         }}");
    let _ = writeln!(o, "      }}");
}

// --- Value -----------------------------------------------------------------------
fn emit_value_property(o: &mut String, func: &FuncDef) {
    let vt = value_surface_type(func);

    let mut d = XmlDoc::new();
    d.summary(
        "The value at the most recently committed bar — the last history bar right after \
         open, then whatever the latest <see cref=\"Update\"/> returned.",
    );
    d.open("remarks");
    d.para("<see cref=\"Peek\"/> does not change it.");
    d.close("remarks");
    o.push('\n');
    o.push_str(&d.render(6));
    let _ = writeln!(o, "      public {vt} Value => {};", fresh_value_expr(func, ""));
}

// --- Clone -----------------------------------------------------------------------
fn emit_clone_method(o: &mut String, func: &FuncDef) {
    let class = stream_class_name(func);

    let mut d = XmlDoc::new();
    d.summary(
        "An independent deep copy of this stream: both evolve separately from here on.",
    );
    d.returns("The new, independent handle.");
    o.push('\n');
    o.push_str(&d.render(6));
    let _ = writeln!(o, "      public {class} Clone()");
    let _ = writeln!(o, "      {{");
    let _ = writeln!(o, "         return new {class}(this);");
    let _ = writeln!(o, "      }}");
}

// ---------------------------------------------------------------------------
// StepImpl
// ---------------------------------------------------------------------------

/// Build the render context for stream-owned code (step bodies, captures).
///
/// `single_precision` is always false — the stream tier is double-only. The
/// double-address-of and float-input sets are empty (transitions carry no
/// out-params), and `matype_map` is empty because stream bodies dispatch
/// MA-type structurally rather than via `== TA_MAType_*`.
fn stream_ctx<'a>(
    empty: &'a HashSet<String>,
    counter: &'a Cell<usize>,
    fma_sets: &'a FmaVarSets,
) -> CsRenderCtx<'a> {
    CsRenderCtx {
        single_precision: false,
        // The streaming tier keeps every output required, so no store is guarded.
        nullable_outputs: empty,
        nullable_shadow: false,
        double_address_of_vars: empty,
        float_input_params: empty,
        inline_counter: counter,
        fma: Some(fma_sets),
        matype_map: HashMap::new(),
    }
}

/// `internal void <base>_StepImpl( <Class> sp, double bar... )` — the one
/// per-bar transition; `Update` runs it on live state, `Peek` on a copy.
///
/// It stays a method on `Core` rather than on the handle because transcribed
/// bodies render unstable-period reads as `this.unstablePeriod[(int)FuncUnstId.X]`,
/// which only compiles inside a `Core` instance method.
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
    let _ = writeln!(o, "\n   internal void {base}_StepImpl( {class} sp, {sig_bars} )");
    let _ = writeln!(o, "   {{");
}

/// One model's per-bar step body at a given indent: temp decls, the extrema
/// rebase, the candle-snapshot unpacking, and the rendered transition.
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
        let (cty, default) = field_type_and_default(ty);
        let _ = writeln!(o, "{pad}{cty} {name} = {default};");
    }
    emit_extrema_rebase(o, model, indent);
    // Candle settings from the open-time snapshot (never the live table). The
    // local NAMES are load-bearing: `fma::expr_is_float_typed` types an operand
    // float by the `_factor` SUFFIX, and these three are emitted as text, never
    // as IR VarDecls, so nothing else types them. Renaming one flips the FMA
    // fusion sites on 57 candlestick functions — ~1 ULP, invisible to every
    // structural gate.
    for s in step_settings {
        let _ = writeln!(o, "{pad}int {s}_rangeType = sp.cs_{s}_rangeType;");
        let _ = writeln!(o, "{pad}int {s}_avgPeriod = sp.cs_{s}_avgPeriod;");
        let _ = writeln!(o, "{pad}double {s}_factor = sp.cs_{s}_factor;");
    }

    let transition = streaming::build_transition(model, &CsStreamNames)
        .unwrap_or_else(|e| panic!("streaming transition: {e}"));
    let empty = HashSet::new();
    let ctx = stream_ctx(&empty, counter, stream_fma);
    for s in &transition {
        o.push_str(&render_statement_ctx(s, indent, &ctx, enums, registry, helpers));
    }
}

/// The identity short-circuit at the top of a dual-mode step, above the mode
/// predicate. Rendered from [`streaming::identity_step_branch`], never
/// hand-typed: the batch predicate is `optInTimePeriod <= 1 || ...`, and a
/// hand-typed `== 1` is a value divergence at period 0.
fn emit_identity_step_branch(
    o: &mut String,
    model: &StreamModel,
    ctx: &CsRenderCtx,
    enums: &HashMap<String, EnumDef>,
    registry: &Registry,
    helpers: &HelperRegistry,
    indent: usize,
) {
    if let Some(s) = streaming::identity_step_branch(model, &CsStreamNames) {
        o.push_str(&render_statement_ctx(&s, indent, ctx, enums, registry, helpers));
    }
}

/// Extrema automatons carry batch-absolute int indices; rebase them by a
/// multiple of the physical ring size long before `int.MaxValue` (mirrors C
/// verbatim — index differences and `& xMask` slots are invariant).
fn emit_extrema_rebase(o: &mut String, model: &StreamModel, indent: usize) {
    if let Some(ex) = model.extrema() {
        let pad = " ".repeat(indent);
        let inner = " ".repeat(indent + 3);
        let mut vars: Vec<String> = vec![model.cursor.clone(), ex.trailing.clone()];
        vars.extend(ex.index_vars.iter().cloned());
        let _ = writeln!(o, "{pad}if( sp.{} >= 1073741824 ) {{", model.cursor);
        let _ = writeln!(o, "{inner}int rebaseShift = sp.{} & ~sp.xMask;", ex.trailing);
        for v in &vars {
            let _ = writeln!(o, "{inner}sp.{v} -= rebaseShift;");
        }
        let _ = writeln!(o, "{pad}}}");
    }
}

// ---------------------------------------------------------------------------
// Open transcription
// ---------------------------------------------------------------------------

/// The output/input and output/output aliasing reject.
///
/// `MemoryExtensions.Overlaps`, not reference identity. Over arrays the two
/// agreed, because two arrays are identical or disjoint and nothing in
/// between. Spans made the in-between expressible: `buf.Slice(0, n)` and
/// `buf.Slice(1, n)` are different spans over overlapping memory, and identity
/// would call them unrelated.
///
/// This is not only about the reject. Several transcribed bodies branch on
/// series identity as part of the ALGORITHM — BBANDS elects its scratch with
/// `if (inReal == outRealUpperBand)` — and on a partially-overlapping pair that
/// test is false while the buffers do in fact collide, so the body would take
/// the wrong arm and write through its own input. Rejecting overlap up front is
/// what keeps those branches sound.
///
/// Cross-typed output pairs (`Span<double>` against `Span<int>`) cannot alias
/// and are skipped: `Overlaps` is not defined across element types, and the
/// runtime cannot place them on the same memory anyway.
fn alias_condition(func: &FuncDef, inputs: &[String]) -> Option<String> {
    let outs: Vec<&str> = func.outputs.iter().map(|out| out.name.as_str()).collect();
    let mut pairs: Vec<String> = Vec::new();
    for out in &outs {
        // An int output cannot overlap a double input series.
        if out_is_int(func, out) {
            continue;
        }
        for input in inputs {
            pairs.push(format!("{out}.Overlaps({input})"));
        }
    }
    for i in 0..outs.len() {
        for b in &outs[i + 1..] {
            if out_is_int(func, outs[i]) != out_is_int(func, b) {
                continue;
            }
            pairs.push(format!("{}.Overlaps({b})", outs[i]));
        }
    }
    if pairs.is_empty() { None } else { Some(pairs.join(" || ")) }
}

/// [`alias_condition`] as a reject that answers a code — what a hand-rolled
/// RetCode-returning fill body (the two exempt tiers) needs. The public frame
/// throws instead and builds on the condition directly, so neither emitter has
/// to read the other's text back out.
fn alias_reject(func: &FuncDef, inputs: &[String]) -> String {
    let Some(cond) = alias_condition(func, inputs) else {
        return String::new();
    };
    let mut s = String::new();
    let _ = writeln!(s, "      if( {cond} ) {{");
    let _ = writeln!(s, "         return RetCode.BadParam;");
    let _ = writeln!(s, "      }}");
    s
}

/// The `out int` seeding prologue. Every `out` parameter must be definitely
/// assigned on every return path (CS0177), and both bodies below reject before
/// the transcribed body assigns either. Zero matches the C server's
/// zero-initialised locals and Java's fresh `MInteger`s, so nothing observable
/// changes — the batch tier seeds identically.
fn emit_out_meta_seed(o: &mut String) {
    let _ = writeln!(o, "      outBegIdx = 0;");
    let _ = writeln!(o, "      outNBElement = 0;");
}

/// Output mode for the open family (mirrors `c_stream` / `java_stream`).
/// `Core` is the ONE transcription both entry points share: output writes are
/// subscripted `out[<idx> * outStride]`, so the filling entries pass stride 1
/// and the caller's arrays while `_OpenInternal` passes stride 0 and a
/// one-element sink whose slot 0 ends holding the last history value.
/// `Scalar`/`Fill`/`FillInternal` survive as signature selectors for the two
/// exempt tiers (`Dispatch`, `PeriodBank`), which hand-roll a body per entry.
#[derive(Clone, Copy, PartialEq, Eq)]
enum OutMode {
    Scalar,
    Fill,
    /// `Fill` anchored at a caller-supplied `startIdx` — the composed-open
    /// fusion seam (issue #192).
    FillInternal,
    Core,
}

/// `<idx>` → `<idx> * outStride`, as IR.
fn scale_by_stride(idx: Expr) -> Expr {
    Expr::BinOp(
        Box::new(idx),
        crate::ir::BinOp::Mul,
        Box::new(Expr::Var("outStride".to_string())),
    )
}

/// Map a batch return-code variable for the open body. Early SUCCESS returns
/// (the no-data guard AND the Metastock seed-boundary return) become
/// `InsufficientHistory`, which the wrapper types as
/// `InsufficientHistoryException`. Everything else passes through.
fn map_open_return(v: &str) -> String {
    match v {
        "SUCCESS" | "TA_SUCCESS" => "InsufficientHistory".to_string(),
        other => other.to_string(),
    }
}

/// Transcribe a batch body region for the C# open: output-array writes scaled by
/// `outStride`, previous-output feedback reads scaled the same way, early-success
/// returns mapped, the final top-level return dropped (capture +
/// `return RetCode.Success` replace it), and the body's own dead identity branch
/// deleted.
fn build_open_body_cs(model: &StreamModel, body: &[Statement]) -> Vec<Statement> {
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

/// The open-body emitter: the merged `private RetCode <base>_OpenImpl(...)`.
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
    emit_out_meta_seed(o);
    let open_body = cleanup_open_body(&build_open_body_cs(model, body), registry);
    emit_open_prologue(o, func, &open_body, model, enums, registry, helpers, counter, stream_fma);
    emit_identity_fast_path(o, func, model, fields, registry, helpers, stream_fma, counter);
    emit_open_region(
        o, func, &open_body, enums, registry, helpers, counter, stream_fma, &[], &HashSet::new(),
    );
    let cur_source = CurSource::StridedArray;
    emit_capture(
        o, func, model, &model.state, step_settings, registry, helpers, stream_fma, counter,
        Some(cur_source), "",
    );
    let _ = writeln!(o, "      return RetCode.Success;");
    let _ = writeln!(o, "   }}");
}

/// The open-body signature. `Scalar` takes sp + inputs + startIdx + opts;
/// `Fill` takes sp + inputs + opts + the batch output tail (no startIdx —
/// pinning bar 0 is what makes the fill bit-exact); `Core` takes both, plus the
/// stride.
fn emit_open_body_sig(o: &mut String, func: &FuncDef, mode: OutMode) {
    let base = base_name(func);
    let class = stream_class_name(func);
    let mut params: Vec<String> = vec![format!("{class} sp")];
    for input in streaming::input_array_names(func) {
        params.push(format!("{} {input}", cs_series_in("double")));
    }
    if mode == OutMode::Scalar || mode == OutMode::Core {
        params.push("int startIdx".to_string());
    }
    for p in &func.optional_inputs {
        params.push(format!("{} {}", opt_param_type_str(p), p.name));
    }
    let name = match mode {
        // Exempt tiers only: their plain-open body IS their numerics, so it
        // wears the same `_OpenImpl` name a merged tier's `Core` does. The two
        // are never emitted for the same function.
        OutMode::Scalar => format!("{base}_OpenImpl"),
        // The merged worker: both entry points' inputs, plus the stride that
        // selects where the per-bar writes land.
        OutMode::Core => {
            push_out_tail(&mut params, func);
            params.push("int outStride".to_string());
            format!("{base}_OpenImpl")
        }
        OutMode::Fill => {
            push_out_tail(&mut params, func);
            format!("{base}_OpenAndFillImpl")
        }
        OutMode::FillInternal => {
            params.insert(
                1 + streaming::input_array_names(func).len(),
                "int startIdx".to_string(),
            );
            push_out_tail(&mut params, func);
            format!("{base}_OpenAndFillInternalImpl")
        }
    };
    let _ = writeln!(o, "\n   private RetCode {name}( {} )", params.join(", "));
    let _ = writeln!(o, "   {{");
}

/// `out int outBegIdx, out int outNBElement, <out arrays...>`.
fn push_out_tail(params: &mut Vec<String>, func: &FuncDef) {
    params.push("out int outBegIdx".to_string());
    params.push("out int outNBElement".to_string());
    for out in &func.outputs {
        params.push(format!("{} {}", out_cs_param_type(func, &out.name), out.name));
    }
}

/// Declarations + validation head shared by both open bodies: the transcribed
/// body's VarDecls (address-of / MAType aware, mirroring the batch renderer),
/// history metadata, input-length validation, optional-param validation,
/// private-extra-param locals and candle unpacking.
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
}

/// The anchor has to land inside the history — see `c_stream::emit_anchor_guard`
/// for why the transcribed bodies cannot be relied on for this (only 137 of 174
/// carry TA-Lib's "make sure there is still something to evaluate" preamble, and
/// the rest run `while nbBar != 0` on a count that went negative).
fn emit_anchor_guard(o: &mut String) {
    let _ = writeln!(o, "      if( startIdx > endIdx ) {{");
    let _ = writeln!(o, "         outBegIdx = 0;");
    let _ = writeln!(o, "         outNBElement = 0;");
    let _ = writeln!(o, "         return RetCode.InsufficientHistory;");
    let _ = writeln!(o, "      }}");
}

/// The transcribed body's VarDecls, including CIRCBUF prologs.
///
/// Unlike Java there is no `MInteger` arm: an `int` taken by address stays a
/// plain `int` local and only its call-argument site changes (to `out name`).
/// `collect_address_of_vars` is still computed — it is what
/// `collect_double_address_of_vars` needs to tell a real out-param from an int
/// one.
fn emit_body_decls(o: &mut String, func: &FuncDef, open_body: &[Statement]) {
    // The handle caches every output's last value, and a declined output leaves
    // no span to read it back from — so the guarded store writes it here too.
    for name in super::common::nullable_output_list(func) {
        let _ = writeln!(o, "      {} lastCur_{name} = 0;", out_cs_type(func, &name));
    }
    let address_of_vars = collect_address_of_vars(open_body);
    let matype_params: HashSet<String> = func
        .optional_inputs
        .iter()
        .filter(|p| matches!(&p.param_type, ParamType::Enum(n) if n == "MAType"))
        .map(|p| p.name.clone())
        .collect();
    let matype_vars = collect_matype_vars(open_body, &matype_params);
    let double_address_of_vars = collect_double_address_of_vars(open_body, &address_of_vars);

    for stmt in open_body {
        if let Statement::CircBuf(CircBuf::Prolog { id, layout, static_size }) = stmt {
            for (arr, t) in circbuf_arrays(id, layout) {
                let elem = if matches!(t, VarType::Integer) { "int" } else { "double" };
                // `= []` rather than Java's bare declaration: the prolog only
                // declares, the allocation is a separate statement, and a
                // dual-mode arm can carry the prolog (it sits in the shared
                // prologue) without the allocation — HMA's short-period arm
                // never touches `dRing`, which is CS0168 (declared and never
                // used), an error under this csproj.
                let _ = writeln!(o, "      {elem}[] {arr} = [];");
            }
            let _ = writeln!(o, "      int {id}_Idx = 0;");
            let _ = writeln!(o, "      int maxIdx_{id} = ({static_size})-1;");
            continue;
        }
        if let Statement::VarDecl { var_type, name, .. } = stmt {
            let cs_decl = if matype_vars.contains(name) {
                format!("MAType {name}")
            } else if double_address_of_vars.contains(name) {
                format!("double[] {name} = new double[1]")
            } else {
                match var_type {
                    VarType::Real => format!("double {name} = 0"),
                    VarType::Integer | VarType::Index => format!("int {name} = 0"),
                    VarType::RealArray(size) => format!("double[] {name} = new double[{size}]"),
                    VarType::IntArray(size) => format!("int[] {name} = new int[{size}]"),
                    _ => format!("{} {name}", cs_type_str(var_type)),
                }
            };
            let _ = writeln!(o, "      {cs_decl};");
        }
    }
}

/// History metadata. `startIdx` and the out-meta pair are real parameters of the
/// core, so it declares neither.
fn emit_open_head(o: &mut String, func: &FuncDef, _outputs: &[String]) {
    let inputs = streaming::input_array_names(func);
    let first = &inputs[0];
    let _ = writeln!(o, "      int historyLen = {first}.Length;");
    let _ = writeln!(o, "      int endIdx = historyLen - 1;");
}

/// Input-length + optional-param validation, and the Fill-mode aliasing guards.
fn emit_open_validation(
    o: &mut String,
    func: &FuncDef,
    mode: OutMode,
    enums: &HashMap<String, EnumDef>,
) {
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
    // other and MAX_INDEX bounds it too (#180). Without this the streaming entry
    // points would compute over exactly the ranges the batch call refuses, and
    // the two are required to agree bit for bit.
    let _ = writeln!(o, "      if( historyLen > MAX_INDEX + 1 ) {{");
    let _ = writeln!(o, "         return RetCode.OutOfRangeEndIndex;");
    let _ = writeln!(o, "      }}");
    let mismatches: Vec<String> = inputs[1..]
        .iter()
        .map(|extra| format!("{extra}.Length != {first}.Length"))
        .collect();
    if !mismatches.is_empty() {
        let _ = writeln!(o, "      if( {} ) {{", mismatches.join(" || "));
        let _ = writeln!(o, "         return RetCode.BadParam;");
        let _ = writeln!(o, "      }}");
    }
    o.push_str(&emit_opt_param_validation(func, "RetCode.BadParam", enums));
    if mode == OutMode::Fill {
        o.push_str(&alias_reject(func, &inputs));
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
        let _ = writeln!(o, "      {} {name} = {init};", extra_param_cs_type(c_type));
    }
    let candle_used = detect_candle_settings(open_body);
    if !candle_used.is_empty() {
        o.push_str(&crate::candle_settings::emit_csharp_unpacking(&candle_used, 6));
    }
}

/// This backend's IR cleanup sequence — `java_stream::cleanup_open_body`'s twin,
/// with this backend's own admission test. See it for why the sequence runs
/// where the body is BUILT and what `INSUFFICIENT_HISTORY` answers; the C#
/// stake in the first is CS0219 on an orphaned local, which
/// `TreatWarningsAsErrors` makes a build failure.
fn cleanup_open_body(body: &[Statement], registry: &Registry) -> Vec<Statement> {
    let admits = |f: &str, a: &[Expr]| super::csharp::cross_call_split(f, a, registry).is_some();
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
    let address_of_vars = collect_address_of_vars(open_body);
    let double_address_of_vars = collect_double_address_of_vars(open_body, &address_of_vars);
    let empty = HashSet::new();
    let nullable = super::common::nullable_output_names(func);
    let ctx = CsRenderCtx {
        single_precision: false,
        nullable_outputs: &nullable,
        nullable_shadow: true,
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
            let new_init =
                hoist_block_helpers(init, helpers, &mut hoisted_vec, &mut cnt, CANDLE_FNS);
            counter.set(cnt);
            o.push_str(&render_hoisted_blocks(
                &hoisted_vec, 6, &ctx, enums, registry, helpers,
            ));
            let init_str = render_expr(&new_init, &ctx, registry, helpers);
            if double_address_of_vars.contains(name) {
                let _ = writeln!(o, "      {name}[0] = {init_str};");
            } else {
                let _ = writeln!(o, "      {name} = {init_str};");
            }
        }
    }

    for (i, stmt) in open_body.iter().enumerate() {
        // Composed tier: sub-stream opens splice in IMMEDIATELY before the batch
        // call that consumes their series (order is the contract — in-place
        // smoothing overwrites the raw series right after).
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

/// The param==1 identity fast path in the open head (mirrors C/Rust/Java).
///
/// The condition is rendered from `model.identity`, never hand-typed: the batch
/// body says `optInTimePeriod <= 1 || ...`, and a hand-typed `== 1` is a value
/// divergence at period 0.
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
        if name.starts_with("cur_") {
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
    let _ = writeln!(o, "         outBegIdx = fillLb;");
    let _ = writeln!(o, "         outNBElement = historyLen - fillLb;");
    let _ = writeln!(o, "         if( outStride == 0 ) {{");
    for (out, inp) in &idp.pairs {
        let _ = writeln!(o, "            {out}[0] = {inp}[historyLen - 1];");
    }
    let _ = writeln!(o, "         }} else {{");
    let _ = writeln!(
        o,
        "            for( int fillIdx = 0; fillIdx < historyLen - fillLb; fillIdx++ ) {{"
    );
    for (out, inp) in &idp.pairs {
        let _ = writeln!(o, "               {out}[fillIdx] = {inp}[fillLb + fillIdx];");
    }
    let _ = writeln!(o, "            }}");
    let _ = writeln!(o, "         }}");
    for (out, _inp) in &idp.pairs {
        let _ = writeln!(o, "         sp.cur_{out} = {out}[(outNBElement - 1) * outStride];");
    }
    let _ = writeln!(o, "         return RetCode.Success;");
    let _ = writeln!(o, "      }}");
}

// ---------------------------------------------------------------------------
// State capture
// ---------------------------------------------------------------------------

/// Derived ring (#229): `open` evaluates f(bar) over the history instead of
/// slicing a raw column that no longer exists under that name.
fn derived_fill_expr_cs(
    dr: &streaming::DerivedRing,
    idx_var: &str,
    ctx: &CsRenderCtx,
    registry: &Registry,
    helpers: &HelperRegistry,
) -> String {
    render_expr(&streaming::derived_fill_value(dr, idx_var), ctx, registry, helpers)
}

/// The capture epilogue: compute ring/window/extrema capacities NUMERICALLY from
/// the still-live batch locals, build the buffers, then store every handle
/// field. CIRCBUF capture MOVES the batch-materialized storage reference
/// (contents AND rotation phase — the CCI-class summation-order requirement).
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
                derived_fill_expr_cs(dr, "fillJ", &fill_ctx, registry, helpers)
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
                    "      {arr}.Slice(historyLen - cap_{v}, cap_{v}).CopyTo(capRing_{v}_{arr});"
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
                "      {arr}.Slice(historyLen - cap_{v}, cap_{v}).CopyTo(capWin_{v}_{arr});"
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
        let _ = writeln!(
            o,
            "      for( int fillJ = historyLen - capX; fillJ < historyLen; fillJ++ ) {{"
        );
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
            "      sp.lastOut_{name} = {name}[(outNBElement - 1) * outStride];"
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

/// Seed `sp.cur_*` at the end of an open body.
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
                CurSource::StridedArray => format!("{out}[(outNBElement - 1) * outStride]"),
                CurSource::Scratch => format!("sc_{out}[outNBElement - 1]"),
            }
        };
        let _ = writeln!(o, "      sp.cur_{out} = {expr};");
    }
}

// ---------------------------------------------------------------------------
// Public wrappers
// ---------------------------------------------------------------------------

/// The reject-conversion tail shared by `OpenInternal` / `OpenAndFill`.
///
/// One `throw StreamFailure(...)` per site rather than Java's four-line ladder:
/// the mapping (typed insufficient-history, `InvalidOperationException` for a
/// capture invariant, `ArgumentException` for everything else) is single-sourced
/// in `Core.StreamFailure`, which is also what makes the message prefix
/// `"<NAME> open: "` the gate greps for impossible to drift per function.
///
/// Deliberately not `Core.Failure`: that maps `OutOfRangeEndIndex` to
/// `ArgumentOutOfRangeException("endIdx")`, meaningless for a caller with no
/// `endIdx` parameter.
fn emit_reject_conversion(o: &mut String, func: &FuncDef, what: &str) {
    let n = func.name.to_uppercase();
    let _ = writeln!(o, "      if( retCode == RetCode.Success ) {{");
    let _ = writeln!(o, "         return sp;");
    let _ = writeln!(o, "      }}");
    let _ = writeln!(o, "      throw StreamFailure(\"{n}\", \"{what}\", retCode);");
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
    let in_fwd: Vec<String> = streaming::input_array_names(func);
    let outs: Vec<String> = func.outputs.iter().map(|out| out.name.clone()).collect();
    let mut fi_sig: Vec<String> = in_fwd
        .iter()
        .map(|a| format!("{} {a}", cs_series_in("double")))
        .collect();
    fi_sig.push("int startIdx".to_string());
    for p in &func.optional_inputs {
        fi_sig.push(format!("{} {}", opt_param_type_str(p), p.name));
    }
    fi_sig.push("out int outBegIdx".to_string());
    fi_sig.push("out int outNBElement".to_string());
    for out in &outs {
        fi_sig.push(format!("{} {out}", out_cs_param_type(func, out)));
    }
    let _ = writeln!(
        o,
        "\n   /* {base}_OpenAndFill anchored at startIdx — the composed-open fusion seam. */"
    );
    let _ = writeln!(
        o,
        "   internal {class} {base}_OpenAndFillInternal( {} )",
        fi_sig.join(", ")
    );
    let _ = writeln!(o, "   {{");
    let _ = writeln!(o, "      {class} sp = new {class}(this);");
    let mut fi_args: Vec<String> = vec!["sp".to_string()];
    fi_args.extend(in_fwd.iter().cloned());
    fi_args.push("startIdx".to_string());
    for p in &func.optional_inputs {
        fi_args.push(p.name.clone());
    }
    fi_args.push("out outBegIdx".to_string());
    fi_args.push("out outNBElement".to_string());
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
    let _ = writeln!(o, "      sp.outRangeBegIdx = outBegIdx;");
    let _ = writeln!(o, "      sp.outRangeCount = outNBElement;");
    emit_reject_conversion(o, func, "openAndFill");
    let _ = writeln!(o, "   }}");
}

/// The PUBLIC opener's input guards. A span cannot be null — a null array
/// converts to an empty one — so emptiness is the only absence C# can see, and
/// it means two different things by position: the FIRST input carries the
/// history, so empty there is rule S1, the implied `startIdx` of 0 naming no bar;
/// any other input is then a length disagreement, which is `BadParam` like every
/// other argument fault.
///
/// S1 is answered here rather than left to the core so the pair is evaluated
/// ahead of the aliasing guard below, which would otherwise see two empty spans
/// and name the wrong problem. `OpenInternal` is the composition seam and is
/// reached only with generator-created arrays, so it stays unchecked.
fn public_open_empty_guards(n: &str, verb: &str, inputs: &[String]) -> String {
    let mut s = String::new();
    let first = &inputs[0];
    let _ = writeln!(
        s,
        "      if( {first}.IsEmpty ) throw new TaLibArgumentOutOfRangeException(nameof({first}), \"{n} {verb}: history is empty\", RetCode.OutOfRangeStartIndex);"
    );
    let _ = writeln!(
        s,
        "      if( {first}.Length > MAX_INDEX + 1 ) throw new TaLibArgumentOutOfRangeException(nameof({first}), \"{n} {verb}: history is longer than MAX_INDEX + 1\", RetCode.OutOfRangeEndIndex);"
    );
    for input in &inputs[1..] {
        let _ = writeln!(
            s,
            "      if( {input}.IsEmpty ) throw new TaLibArgumentException(\"{n} {verb}: {input} is empty\", nameof({input}), RetCode.BadParam);"
        );
    }
    s
}

/// Rule S5's input half, the same at both openers: the history's own length IS
/// the range, so every other declared input must AGREE with it rather than
/// merely reach it.
///
/// At the plain open it is the whole of S5 — nothing is written, so there is no
/// capacity to bound — and it belongs on this frame for the same reason the
/// fill's does: the core makes the test, but answers it as a bare `BadParam`
/// naming nothing, where the same fault at `OpenAndFill` named the leg (issue
/// #271 item 1).
fn history_length_guards(func: &FuncDef, n: &str, verb: &str) -> String {
    let inputs = streaming::input_array_names(func);
    let history = &inputs[0];
    let mut s = String::new();
    for input in &inputs[1..] {
        let _ = writeln!(
            s,
            "      RequireHistoryLength(\"{n}\", \"{verb}\", \"{input}\", {input}.Length, {history}.Length);"
        );
    }
    s
}

/// Rule S5 at the PUBLIC `OpenAndFill`.
///
/// An opener is a batch call over `[0, historyLen - 1]`, so B5's produced count
/// collapses to `historyLen - lookback`. B5 reads its two halves in one rule,
/// inputs first, so the input series' agreement with the history is checked here
/// too — the core makes that test, but only after this frame would have answered,
/// which reported a short input as an output-capacity fault.
///
/// `Core.OpenFillCount` floors a short history at 0 so that it reaches S7, and
/// raises on the `-1` a rejected parameter returns so that S3 stays ahead of the
/// buffer rules.
/// `<N>_Lookback` does its own default substitution, so the raw parameters the
/// frame was handed are the right ones to pass.
///
/// **The PUBLIC frame, never `<N>_OpenAndFillInternal`.** That seam takes an
/// anchor and writes `historyLen - max(lookback, startIdx)` — fewer — so the
/// same bound there would reject the composed sub-calls that pass a non-zero
/// anchor, and would be redundant: those destinations are proved disjoint and
/// sized by construction.
///
/// An output marked `nullable` is bounded only where it was supplied: declining
/// it is legal here, exactly as in the batch tier (rule B6a).
fn public_open_fill_capacity(func: &FuncDef, n: &str, history: &str) -> String {
    let lb_args: Vec<String> = func.optional_inputs.iter().map(|p| p.name.clone()).collect();
    let mut s = String::new();
    // Rule S3 first, in the shape the buffer rules need it: `<N>_Lookback`
    // answers `-1` for an out-of-domain parameter and `OpenFillCount` raises on
    // it, so a bad parameter is reported as one rather than as whatever the
    // buffer rules would have said about a call it made no sense to size.
    let _ = writeln!(
        s,
        "      int guardOutLen = OpenFillCount(\"{n}\", \"openAndFill\", {history}.Length, {n}_Lookback({}));",
        lb_args.join(", ")
    );
    // Then S5's input half, ahead of its output half — the order B5 states.
    s.push_str(&history_length_guards(func, n, "openAndFill"));
    // A `nullable` output may be declined with an empty span (rule B6a read on
    // this tier), so its bound is conditional — the shape the batch wrapper uses.
    let nullable = super::common::nullable_output_names(func);
    for out in &func.outputs {
        let guard = if nullable.contains(&out.name) {
            format!("if( !{0}.IsEmpty ) ", out.name)
        } else {
            String::new()
        };
        let _ = writeln!(
            s,
            "      {guard}RequireFillLength(\"{n}\", \"openAndFill\", \"{0}\", {0}.Length, guardOutLen);",
            out.name
        );
    }
    s
}

/// `OpenInternal` (the anchored plain open), the public `<base>_Open`, and the
/// public `<base>_OpenAndFill`.
///
/// `merged` says whether this function owns a `Core` — one stride-parameterized
/// `<base>_OpenImpl` every entry point reaches. When it does, the two internal
/// seams ARE the implementation: `_OpenInternal` synthesizes a one-element sink
/// per output and calls the numerics at stride 0, and the public `_OpenAndFill`
/// hoists the aliasing guard and delegates to `_OpenAndFillInternal`, exactly as
/// `_Open` delegates to `_OpenInternal`. That symmetry is what makes the
/// anchored fill seam reachable for every function rather than only the sixteen
/// something composes over. Mirrors `java_stream::emit_open_wrappers`.
#[allow(clippy::too_many_lines)]
fn emit_open_wrappers(o: &mut String, func: &FuncDef, merged: bool) {
    let base = base_name(func);
    let class = stream_class_name(func);
    let n = func.name.to_uppercase();
    let empty = DocDef::default();
    let doc = func.doc.as_ref().unwrap_or(&empty);

    let mut in_sig: Vec<String> = Vec::new();
    let mut in_fwd: Vec<String> = Vec::new();
    for input in streaming::input_array_names(func) {
        in_sig.push(format!("{} {input}", cs_series_in("double")));
        in_fwd.push(input.clone());
    }
    let mut opt_sig: Vec<String> = Vec::new();
    let mut opt_fwd: Vec<String> = Vec::new();
    for p in &func.optional_inputs {
        opt_sig.push(format!("{} {}", opt_param_type_str(p), p.name));
        opt_fwd.push(p.name.clone());
    }
    let opt_sig_str = if opt_sig.is_empty() {
        String::new()
    } else {
        format!(", {}", opt_sig.join(", "))
    };
    let opt_fwd_str = if opt_fwd.is_empty() {
        String::new()
    } else {
        format!(", {}", opt_fwd.join(", "))
    };

    // --- OpenInternal: startIdx-anchored composition seam --------------------
    let _ = writeln!(
        o,
        "\n   /* Internal startIdx-anchored open behind {base}_Open (composition seam). */"
    );
    let _ = writeln!(
        o,
        "   internal {class} {base}_OpenInternal( {}, int startIdx{opt_sig_str} )",
        in_sig.join(", ")
    );
    let _ = writeln!(o, "   {{");
    let _ = writeln!(o, "      {class} sp = new {class}(this);");
    if merged {
        let mut args: Vec<String> = vec!["sp".to_string()];
        args.extend(in_fwd.iter().cloned());
        args.push("startIdx".to_string());
        args.extend(opt_fwd.iter().cloned());
        args.push("out int outBegIdx".to_string());
        args.push("out int outNBElement".to_string());
        for out in &func.outputs {
            let t = out_cs_type(func, &out.name);
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
        let _ = writeln!(o, "      sp.outRangeBegIdx = outBegIdx;");
        let _ = writeln!(o, "      sp.outRangeCount = outNBElement;");
    } else {
        let _ = writeln!(
            o,
            "      RetCode retCode = {base}_OpenImpl(sp, {}, startIdx{opt_fwd_str});",
            in_fwd.join(", ")
        );
    }
    emit_reject_conversion(o, func, "open");
    let _ = writeln!(o, "   }}");

    // --- public Open ---------------------------------------------------------
    let mut d = XmlDoc::new();
    d.summary(&format!(
        "Open a live <c>{n}</c> stream over the warm-up history."
    ));
    d.open("remarks");
    d.para(&format!(
        "The handle's <see cref=\"{class}.Value\"/> starts at the last history bar's \
         value — bit-identical to what <c>{n}</c> reports for that bar."
    ));
    d.para(&format!(
        "The history must hold at least <c>{base}_Lookback(...) + 1</c> bars \
         (unstable-period aware). Nothing is written to any caller array; use \
         <c>{base}_OpenAndFill</c> to get the warm-up values as well."
    ));
    d.close("remarks");
    for input in &in_fwd {
        d.param(
            input,
            &format!("{} The warm-up history, oldest bar first.", input_desc(input, doc)),
        );
    }
    for p in &func.optional_inputs {
        d.param(&p.name, &opt_param_desc(&base, p));
    }
    d.returns("The open stream handle.");
    d.exception(
        "InsufficientHistoryException",
        &format!("The history holds fewer than <c>{base}_Lookback(...) + 1</c> bars."),
    );
    d.exception(
        "System.ArgumentException",
        "An optional parameter is outside its documented range, or the input series have \
         different lengths.",
    );
    d.exception(
        "System.ArgumentOutOfRangeException",
        "The history is empty — which is what a null array becomes, since a span cannot be \
         null — or it is longer than <see cref=\"Core.MAX_INDEX\"/> + 1, the two index \
         faults an opener can have (rules S1 and S2).",
    );
    o.push('\n');
    o.push_str(&d.render(3));
    let _ = writeln!(
        o,
        "   public {class} {base}_Open( {}{opt_sig_str} )",
        in_sig.join(", ")
    );
    let _ = writeln!(o, "   {{");
    o.push_str(&public_open_empty_guards(&n, "open", &in_fwd));
    o.push_str(&history_length_guards(func, &n, "open"));
    let _ = writeln!(
        o,
        "      return {base}_OpenInternal({}, 0{opt_fwd_str});",
        in_fwd.join(", ")
    );
    let _ = writeln!(o, "   }}");

    // --- public OpenAndFill --------------------------------------------------
    // The filled range is reported on the handle (`OutRange`), not through a
    // pair of caller-supplied out-params.
    let mut fill_sig: Vec<String> = in_sig.clone();
    for p in &opt_sig {
        fill_sig.push(p.clone());
    }
    let mut fill_fwd: Vec<String> = in_fwd.clone();
    for p in &opt_fwd {
        fill_fwd.push(p.clone());
    }
    fill_fwd.push("out int outBegIdx".to_string());
    fill_fwd.push("out int outNBElement".to_string());
    for out in &func.outputs {
        fill_sig.push(format!("{} {}", out_cs_param_type(func, &out.name), out.name));
        fill_fwd.push(out.name.clone());
    }

    let mut d = XmlDoc::new();
    d.summary(&format!(
        "<c>{base}_Open</c> that also fills the output array(s) over the whole history in \
         the same single pass."
    ));
    d.open("remarks");
    d.para(&format!(
        "The values written are bit-identical to what <c>{n}</c> produces over the same \
         series, so no separate batch call is needed for the warm-up plot."
    ));
    d.para(&format!(
        "Output arrays must hold <c>historyLen - {base}_Lookback(...)</c> values and must \
         not alias the inputs or each other — this path writes the outputs and then reads \
         the input tail to seed its rings, so the batch tier's in-place allowance does not \
         carry over here. Both are checked before anything is written, so an undersized \
         span is an <c>ArgumentException</c> naming it rather than a fault from inside \
         the fill."
    ));
    d.para(&format!(
        "The range written is reported on the returned handle: \
         <see cref=\"{class}.OutRange\"/>."
    ));
    d.close("remarks");
    for input in &in_fwd {
        d.param(
            input,
            &format!("{} The warm-up history, oldest bar first.", input_desc(input, doc)),
        );
    }
    for p in &func.optional_inputs {
        d.param(&p.name, &opt_param_desc(&base, p));
    }
    for out in &func.outputs {
        d.param(
            &out.name,
            &format!(
                "{}{} Must hold at least <c>historyLen - {base}_Lookback(...)</c> values.",
                super::csharp_doc::output_desc(out, doc),
                if out.is_nullable() {
                    " Pass an empty span to decline it: the value is still computed \
                     — the handle's <c>Value</c> reports it — and nothing is written out."
                } else {
                    ""
                }
            ),
        );
    }
    d.returns("The open stream handle, with its fill range set.");
    d.exception(
        "InsufficientHistoryException",
        &format!("The history holds fewer than <c>{base}_Lookback(...) + 1</c> bars."),
    );
    d.exception(
        "System.ArgumentException",
        "An optional parameter is outside its documented range, the input series have \
         different lengths, an output is shorter than the values the fill writes, or an \
         output array aliases an input or another output.",
    );
    d.exception(
        "System.ArgumentOutOfRangeException",
        "The history is empty — which is what a null array becomes, since a span cannot be \
         null — or it is longer than <see cref=\"Core.MAX_INDEX\"/> + 1, the two index \
         faults an opener can have (rules S1 and S2).",
    );
    o.push('\n');
    o.push_str(&d.render(3));
    let _ = writeln!(
        o,
        "   public {class} {base}_OpenAndFill( {} )",
        fill_sig.join(", ")
    );
    let _ = writeln!(o, "   {{");
    o.push_str(&public_open_empty_guards(&n, "openAndFill", &in_fwd));
    o.push_str(&public_open_fill_capacity(func, &n, &in_fwd[0]));
    if merged {
        // The guard the anchored seam deliberately omits: every composed
        // sub-call passes a destination that overlaps neither its sources nor
        // each other, so it belongs on the public frame, not the hot one. It
        // throws here rather than answering a code, producing the identical
        // exception the shared ladder produced when the deleted fill body
        // returned BadParam into it.
        if let Some(cond) = alias_condition(func, &in_fwd) {
            let _ = writeln!(o, "      if( {cond} ) {{");
            let _ = writeln!(
                o,
                "         throw StreamFailure(\"{n}\", \"openAndFill\", RetCode.BadParam);"
            );
            let _ = writeln!(o, "      }}");
        }
        let mut args: Vec<String> = in_fwd.clone();
        args.push("0".to_string());
        args.extend(opt_fwd.iter().cloned());
        args.push("out _".to_string());
        args.push("out _".to_string());
        for out in &func.outputs {
            args.push(out.name.clone());
        }
        let _ = writeln!(o, "      return {base}_OpenAndFillInternal({});", args.join(", "));
    } else {
        let _ = writeln!(o, "      {class} sp = new {class}(this);");
        let _ = writeln!(
            o,
            "      RetCode retCode = {base}_OpenAndFillImpl(sp, {});",
            fill_fwd.join(", ")
        );
        let _ = writeln!(o, "      sp.outRangeBegIdx = outBegIdx;");
        let _ = writeln!(o, "      sp.outRangeCount = outNBElement;");
        emit_reject_conversion(o, func, "openAndFill");
    }
    let _ = writeln!(o, "   }}");
}

// ---------------------------------------------------------------------------
// Dual-mode tier (DI/DM scalar, TRIMA ring): two param-selected arms sharing
// one union handle. Mirrors java_stream::emit_dual_mode.
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
/// array). A name both lists carry must agree on type (mirrors Rust/Java).
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

/// `sp.<field> = <default>;` for the OTHER mode's exclusive ARRAY fields.
///
/// Scalars are already at their zero default. An array is not: C# initialises
/// every array field to the empty `[]`, and the deep-copy constructor allocates
/// `new T[other.<field>.Length]` — so an inactive mode's array left at `[]`
/// would survive a copy as a zero-length array where the field list says one
/// slot. Seeding the declared default keeps the two handles' shapes identical
/// whichever arm opened them (Java seeds here to avoid an NPE instead; same
/// statement, different failure it is preventing).
fn dual_complement_capture(own: &[Field], other: &[Field]) -> String {
    let own_names: HashSet<&String> = own.iter().map(|(n, _, _)| n).collect();
    let mut s = String::new();
    for (name, cty, default) in other {
        if !own_names.contains(name) && cty.ends_with("[]") {
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
    // the inactive mode's arrays are seeded to their 1-slot defaults in the arm
    // capture (the deep-copy constructor copies every array field — mirrors
    // C/Rust's union struct).
    let fields_a = state_fields_from(func, ma, &union_scalars, &step_settings);
    let fields_b = state_fields_from(func, mb, &union_scalars, &step_settings);
    let fields = dual_union_fields(func, &fields_a, &fields_b);
    emit_handle_class(o, func, &fields, &SubMembers::none());

    // --- step: one method, the mode re-derived from the stored param --------
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

    // --- open: shared head, then one predicate branch per mode, each
    // transcribing `prologue ++ its arm ++ epilogue` and capturing into the
    // union (both branches return; nothing follows the if/else) -------------
    {
        emit_open_body_sig(o, func, OutMode::Core);
        emit_out_meta_seed(o);
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
            // defaults == C's memset zeros). The two arms are sibling blocks,
            // so both may declare the same locals.
            let mut body: Vec<Statement> = dmp.prologue.to_vec();
            body.extend_from_slice(arm.body);
            body.extend_from_slice(dmp.epilogue);
            let open_body = cleanup_open_body(&build_open_body_cs(arm, &body), registry);
            let mut s = String::new();
            emit_body_decls(&mut s, func, &open_body);
            emit_extras_and_candle(&mut s, func, &open_body, registry, helpers, counter, stream_fma);
            emit_open_region(
                &mut s, func, &open_body, enums, registry, helpers, counter, stream_fma, &[],
                &HashSet::new(),
            );
            let (own, other) = if k == 0 { (&fields_a, &fields_b) } else { (&fields_b, &fields_a) };
            let complement = dual_complement_capture(own, other);
            emit_capture(
                &mut s, func, arm, &union_scalars, &step_settings, registry, helpers, stream_fma,
                counter, Some(CurSource::StridedArray), &complement,
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
// Dispatch tier (MA): params + an `object? sub` tagged by the stored enum
// param (the C `void *sub` model — C# has no payload enums either).
//
// The dispatch is a `switch` plus a cast, and NOT because a virtual call would
// be slow: measured on .NET 10, a monomorphic interface call is 4.41–4.74
// ns/bar against the switch's 5.55–5.72. The switch wins on cross-language
// parity, on profile-independence, and on not adding a type hierarchy across
// 172 handle classes — that is the whole argument, and the number is here so
// nobody re-optimizes on the "virtual calls are slow" intuition.
// ---------------------------------------------------------------------------

/// `SMA_Stream` for callee `sma` — from the callee's own base name, the same
/// authority as the callee's generated handle.
fn callee_stream_class(registry: &Registry, callee: &str) -> String {
    format!("{}_Stream", registry.name_of(callee))
}

/// `MAMA_Value` for callee `mama` — the callee's multi-output record struct.
fn callee_value_type(registry: &Registry, callee: &str) -> String {
    format!("{}_Value", registry.name_of(callee))
}

/// `sp.cur_<out>` / `Value` member routing for one forwarded callee slot.
fn callee_value_member(registry: &Registry, callee: &str, slot: usize) -> String {
    value_member_name(&registry.callee_outputs(callee)[slot])
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
    // (TA_MAType_DISABLED, #93); resolve those to their qualified member like
    // batch. This is the ONE tier that populates the map — every other stream
    // body dispatches structurally.
    ctx.matype_map = build_matype_map(enums);
    let lb_args: Vec<String> = func.optional_inputs.iter().map(|p| p.name.clone()).collect();
    let lb_call = format!("{base}_Lookback({})", lb_args.join(", "));

    // --- handle class -------------------------------------------------------
    let fields = base_fields(func);
    let extra_members = format!(
        "      // Sub-stream, tagged by {}; null on the identity path.\n      internal object? sub;\n",
        dp.param
    );
    // Deep copy of the tagged sub: switch on the stored enum param, invoke the
    // callee's copy constructor. All three tables below — this one, CopyFrom's
    // and the step's — walk the SAME `dp.arms`, so a new MAType cannot be
    // handled in one and missed in another.
    let mut copy_extra = String::new();
    let _ = writeln!(copy_extra, "         if( other.sub is null ) {{");
    let _ = writeln!(copy_extra, "            this.sub = null;");
    let _ = writeln!(copy_extra, "         }} else {{");
    let _ = writeln!(copy_extra, "            switch( this.{} )", dp.param);
    let _ = writeln!(copy_extra, "            {{");
    for arm in dp.arms.iter().filter(|a| a.supported) {
        let label = render_csharp_switch_label(&arm.label, enums);
        let cls = callee_stream_class(registry, &arm.callee);
        let _ = writeln!(copy_extra, "            case {label}:");
        let _ = writeln!(copy_extra, "               this.sub = new {cls}(({cls}) other.sub!);");
        let _ = writeln!(copy_extra, "               break;");
    }
    let _ = writeln!(copy_extra, "            default:");
    // InvalidOperationException here, ArgumentException in the STEP's default
    // arm — the two differ deliberately. The step is reachable from a caller's
    // bad enum, so it must throw the type the gate catches and reports as a
    // reject-parity failure. A copy constructor has no caller-supplied argument
    // to blame: this arm is unreachable by construction (open rejects any arm
    // without a sub-stream), so the honest type is the one for "this object is
    // in a state that should not exist".
    let _ = writeln!(
        copy_extra,
        "               throw new InvalidOperationException(\"unreachable: open rejects arms without a sub-stream\");"
    );
    let _ = writeln!(copy_extra, "            }}");
    let _ = writeln!(copy_extra, "         }}");
    // The same switch in place: the scratch keeps the sub it already holds when
    // the arm matches. It is only the same arm when the source handle's param is
    // the same, which is why the tag is read off `this` after the field copy,
    // exactly as the copy constructor reads it after its own.
    let mut restore_extra = String::new();
    let _ = writeln!(restore_extra, "         if( other.sub is null ) {{");
    let _ = writeln!(restore_extra, "            this.sub = null;");
    let _ = writeln!(restore_extra, "         }} else {{");
    let _ = writeln!(restore_extra, "            switch( this.{} )", dp.param);
    let _ = writeln!(restore_extra, "            {{");
    for arm in dp.arms.iter().filter(|a| a.supported) {
        let label = render_csharp_switch_label(&arm.label, enums);
        let cls = callee_stream_class(registry, &arm.callee);
        let _ = writeln!(restore_extra, "            case {label}:");
        let _ = writeln!(restore_extra, "               if( this.sub is {cls} ) {{");
        let _ = writeln!(
            restore_extra,
            "                  (({cls}) this.sub!).CopyFrom(({cls}) other.sub!);"
        );
        let _ = writeln!(restore_extra, "               }} else {{");
        let _ = writeln!(
            restore_extra,
            "                  this.sub = new {cls}(({cls}) other.sub!);"
        );
        let _ = writeln!(restore_extra, "               }}");
        let _ = writeln!(restore_extra, "               break;");
    }
    let _ = writeln!(restore_extra, "            default:");
    let _ = writeln!(
        restore_extra,
        "               throw new InvalidOperationException(\"unreachable: open rejects arms without a sub-stream\");"
    );
    let _ = writeln!(restore_extra, "            }}");
    let _ = writeln!(restore_extra, "         }}");
    // The dispatch arm is an enum value chosen at run time, so what the sub owns
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
        let label = render_csharp_switch_label(&arm.label, enums);
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
                "         sp.cur_{} = (({cls}) sp.sub!).Update({bar_args});",
                outputs[k]
            );
        } else {
            let vt = callee_value_type(registry, &arm.callee);
            let _ = writeln!(
                o,
                "         {vt} subValue = (({cls}) sp.sub!).Update({bar_args});"
            );
            for (i, slot) in arm.out_map.iter().enumerate() {
                if let streaming::OutSlot::Forward(k) = slot {
                    let _ = writeln!(
                        o,
                        "         sp.cur_{} = subValue.{};",
                        outputs[*k],
                        callee_value_member(registry, &arm.callee, i)
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

    // --- open bodies (Scalar delegates to OpenInternal; Fill to OpenAndFill) -
    for mode in [OutMode::Scalar, OutMode::Fill, OutMode::FillInternal] {
        emit_open_body_sig(o, func, mode);
        if mode != OutMode::Scalar {
            emit_out_meta_seed(o);
        }
        let first = &inputs[0];
        let _ = writeln!(o, "      int historyLen = {first}.Length;");
        emit_open_validation(o, func, mode, enums);
        // Own-lookback precheck BEFORE delegating: the callee's open would
        // reject too, but with ITS message prefix ("SMA open:" for a MA call) —
        // the documented stable "<NAME> open:" contract requires the reject to
        // carry this function's name.
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
                // The dispatch tier is exempt from the OpenImpl merge (it hands
                // the fill to a sub's public OpenAndFill), so it only ever
                // renders the three signature modes, never the merged core.
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
                    let _ = writeln!(o, "         outBegIdx = fillLb;");
                    let _ = writeln!(o, "         outNBElement = historyLen - fillLb;");
                    let _ = writeln!(
                        o,
                        "         for( int fillIdx = 0; fillIdx < historyLen - fillLb; fillIdx++ ) {{"
                    );
                    for (out, inp) in &idp.pairs {
                        let _ = writeln!(o, "            {out}[fillIdx] = {inp}[fillLb + fillIdx];");
                    }
                    let _ = writeln!(o, "         }}");
                    for (out, _) in &idp.pairs {
                        let _ = writeln!(o, "         sp.cur_{out} = {out}[outNBElement - 1];");
                    }
                }
            }
            let _ = writeln!(o, "         return RetCode.Success;");
            let _ = writeln!(o, "      }}");
        }
        let _ = writeln!(o, "      switch( {} )", dp.param);
        let _ = writeln!(o, "      {{");
        for arm in &dp.arms {
            let label = render_csharp_switch_label(&arm.label, enums);
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
                        // dispatch func's own array, Discard materializes a
                        // throwaway buffer (the managed rendering of C's NULL
                        // for a nullable output — the batch discard-buffer
                        // idiom, #125).
                        let fill_outs: String = arm
                            .out_map
                            .iter()
                            .map(|slot| match slot {
                                streaming::OutSlot::Forward(k) => outputs[*k].clone(),
                                streaming::OutSlot::Discard => "default".to_string(),
                            })
                            .collect::<Vec<_>>()
                            .join(", ");
                        let opts = if opts.is_empty() {
                            String::new()
                        } else {
                            format!("{}, ", opts.join(", "))
                        };
                        if mode == OutMode::FillInternal {
                            // The internal variant takes the out-meta directly,
                            // so there is no range to copy back.
                            let _ = writeln!(
                                o,
                                "         {cls} sub = {callee_base}_OpenAndFillInternal({bar_args}, startIdx, {opts}out outBegIdx, out outNBElement, {fill_outs});"
                            );
                            dispatch_store_sub(o, registry, arm, &outputs, "         ");
                            let _ = writeln!(o, "         break;");
                            let _ = writeln!(o, "      }}");
                            continue;
                        }
                        // The callee's public OpenAndFill reports its filled
                        // range on the handle; this body still owes its caller
                        // the out-meta pair, so copy it back out.
                        let _ = writeln!(
                            o,
                            "         {cls} sub = {callee_base}_OpenAndFill({bar_args}, {opts}{fill_outs});"
                        );
                        let _ = writeln!(o, "         outBegIdx = sub.outRangeBegIdx;");
                        let _ = writeln!(o, "         outNBElement = sub.outRangeCount;");
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
        "      // One sub-{} stream per period in [{min}, {max}], advanced in lockstep.\n      internal {subty}[] bank = [];\n",
        callee.to_uppercase()
    );
    // An array copy is SHALLOW whatever spelling it uses: the bank must copy
    // element-wise or a peek would advance the LIVE handle's sub-streams
    // through the aliased slots.
    let mut copy_extra = String::new();
    let _ = writeln!(copy_extra, "         this.bank = new {subty}[other.bank.Length];");
    let _ = writeln!(copy_extra, "         for( int bankIdx = 0; bankIdx < other.bank.Length; bankIdx++ ) {{");
    let _ = writeln!(copy_extra, "            this.bank[bankIdx] = new {subty}(other.bank[bankIdx]);");
    let _ = writeln!(copy_extra, "         }}");
    // Same shape, in place: the bank a scratch already holds is the right
    // length unless a differently-parameterised handle borrowed it, in which
    // case it is rebuilt exactly as the copy constructor builds one. The branch
    // is on LENGTH alone — the field is a non-nullable array initialised to
    // `[]`, so Java's `!= null` test would be a nullable-analysis diagnostic
    // here, and the zero length it starts at already takes the rebuild arm.
    let mut restore_extra = String::new();
    let _ = writeln!(restore_extra, "         if( this.bank.Length == other.bank.Length ) {{");
    let _ = writeln!(restore_extra, "            for( int bankIdx = 0; bankIdx < other.bank.Length; bankIdx++ ) {{");
    let _ = writeln!(restore_extra, "               this.bank[bankIdx].CopyFrom(other.bank[bankIdx]);");
    let _ = writeln!(restore_extra, "            }}");
    let _ = writeln!(restore_extra, "         }} else {{");
    let _ = writeln!(restore_extra, "            this.bank = new {subty}[other.bank.Length];");
    let _ = writeln!(restore_extra, "            for( int bankIdx = 0; bankIdx < other.bank.Length; bankIdx++ ) {{");
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
    // The one array hoist this emitter does. There is no general hoisting pass
    // (measured noise, 3.35–3.48 against 3.39–3.47 ns/bar); a COUNTED loop
    // bound is the exception, where it genuinely drops a bounds check per bar.
    let _ = writeln!(o, "      {subty}[] bank = sp.bank;");
    let _ = writeln!(o, "      for( int bankIdx = 0; bankIdx < bank.Length; bankIdx++ ) {{");
    let _ = writeln!(o, "         double subValue = bank[bankIdx].Update({price});");
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
    let _ = writeln!(o, "      int historyLen = {price}.Length;");
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
    emit_out_meta_seed(o);
    let _ = writeln!(o, "      int historyLen = {price}.Length;");
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
    let _ = writeln!(o, "      double[] seedPrefix = new double[lookbackTotal + 1];");
    let _ = writeln!(o, "      {price}.Slice(0, lookbackTotal + 1).CopyTo(seedPrefix);");
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
    let _ = writeln!(o, "            scratch[bankIdx] = bank[bankIdx].Update({price}[t]);");
    let _ = writeln!(o, "         }}");
    let _ = writeln!(o, "         cp = (int){period}[t];");
    let _ = writeln!(o, "         if( cp < {min} ) {{");
    let _ = writeln!(o, "            cp = {min};");
    let _ = writeln!(o, "         }} else if( cp > {max} ) {{");
    let _ = writeln!(o, "            cp = {max};");
    let _ = writeln!(o, "         }}");
    let _ = writeln!(o, "         {out}[t - lookbackTotal] = scratch[cp - {min}];");
    let _ = writeln!(o, "      }}");
    let _ = writeln!(o, "      outBegIdx = lookbackTotal;");
    let _ = writeln!(o, "      outNBElement = historyLen - lookbackTotal;");
    for p in &func.optional_inputs {
        let _ = writeln!(o, "      sp.{0} = {0};", p.name);
    }
    let _ = writeln!(o, "      sp.bank = bank;");
    let _ = writeln!(o, "      sp.cur_{out} = {out}[outNBElement - 1];");
    let _ = writeln!(o, "      return RetCode.Success;");
    let _ = writeln!(o, "   }}");

    emit_open_wrappers(o, func, false);
}

// ---------------------------------------------------------------------------
// Composed tier (STOCH class): producer transition + pipeline of owned public
// sub-handles, mirroring java_stream's emit_composed with the same managed
// simplifications: GC replaces every cleanup ladder and series-free replay,
// `free()` renders as a no-op so lag-ring seeding reads the still-live
// intermediate array, and copy-peek deletes peekMode entirely (sub handles
// deep-copy through their copy constructors).
// ---------------------------------------------------------------------------

/// Composed producer name map: identical to [`CsStreamNames`] except the
/// intermediate series' "output" write lands in a BARE `cur_<series>` local
/// rather than on the handle — that value is consumed by the pipeline in the
/// same step and never survives the bar.
struct CsComposedNames {
    series: String,
}

impl streaming::NameMap for CsComposedNames {
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

/// C# twin of `c_stream::transform_map_step`: series reads/writes become the
/// per-bar `cur_*` locals (a lag-ring series' `[cursor]` read becomes the ring's
/// oldest slot), params read through `sp.`, `for` shells dropped.
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
            let (cty, default) = field_type_and_default(ty);
            let _ = writeln!(o, "      {cty} {name} = {default};");
        }
    }
    for (name, ty) in &cp.map_temps {
        let (cty, default) = field_type_and_default(ty);
        let _ = writeln!(o, "      {cty} {name} = {default};");
    }
    for name in cur_scalars {
        let ty = out_cs_type(func, name);
        let zero = if ty == "int" { "0" } else { "0.0" };
        let _ = writeln!(o, "      {ty} cur_{name} = {zero};");
    }
}

/// The composed `StepImpl`: producer transition (writing `cur_<series>`), then
/// the batch-tail pipeline through the owned sub handles, combine maps per bar,
/// lag-ring pushes, and the `sp.cur_*` output stores. No peek flag: peek is the
/// universal deep-copy of the whole tree.
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
        // Same load-bearing local names as the loop tier — `fma::expr_is_float_typed`
        // types an operand float by the `_factor` SUFFIX and these are emitted
        // as text, never as IR VarDecls.
        for s in step_settings {
            let _ = writeln!(o, "      int {s}_rangeType = sp.cs_{s}_rangeType;");
            let _ = writeln!(o, "      int {s}_avgPeriod = sp.cs_{s}_avgPeriod;");
            let _ = writeln!(o, "      double {s}_factor = sp.cs_{s}_factor;");
        }
        let names = CsComposedNames {
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
                    let _ = writeln!(o, "      cur_{d} = sp.sub{sub_idx}.Update({arg_str});");
                } else {
                    let vt = callee_value_type(registry, &callee_key);
                    let _ = writeln!(o, "      {{");
                    let _ = writeln!(o, "         {vt} subOut{sub_idx} = sp.sub{sub_idx}.Update({arg_str});");
                    for (k, d) in sub.dsts.iter().enumerate() {
                        let _ = writeln!(
                            o,
                            "         cur_{d} = subOut{sub_idx}.{};",
                            callee_value_member(registry, &callee_key, k)
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
/// (the out-meta pair is a real `out int` parameter under its batch name).
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
    // `cp.region`, NEVER `func.body[..tail_start]`: the region is an owned Vec
    // precisely because any leading parameter-guarded fast-path block is
    // filtered out of it. The stream composes the general path, not the
    // specialization, and a bypass here reintroduces the block.
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

/// Composed open body: the merged `<N>_OpenImpl` — scratch `sc_` output arrays plus
/// a verbatim transcription of the batch body with sub-streams opened at the
/// exact consumption points, then capture.
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
    // The composed fill/scratch path hardcodes double arrays (mirrors C/Rust/Java).
    let empty = HashSet::new();
    let ctx = stream_ctx(&empty, counter, stream_fma);

    emit_open_body_sig(o, func, OutMode::Core);
    emit_out_meta_seed(o);
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
    // error, so it surfaces as the InvalidOperationException the reject
    // conversion already maps InternalError to rather than being silently
    // reported as the sub's insufficient history.
    //
    // Cost: one lookback call per OPEN, on a path that already allocates
    // `historyLen` doubles per output; `Update` is untouched. The rejecting path
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
    // copied back (issue #205). Only the scalar sink still needs its own buffer.
    // See `fill_scratch_may_alias_output` for when this is unsound — and do not
    // widen its pinned destination set on the strength of it passing here.
    let alias_fill = cp.fill_scratch_may_alias_output(outputs);
    for out in outputs {
        let ty = out_cs_type(func, out);
        if alias_fill {
            let _ = writeln!(
                o,
                "      Span<{ty}> sc_{out} = outStride == 1 ? {out} : new {ty}[historyLen];"
            );
        } else {
            let _ = writeln!(o, "      Span<{ty}> sc_{out} = new {ty}[historyLen];");
        }
    }

    // Sub-open inserts, keyed to combined region++tail indices. The sub reads
    // the produced series only up to the sub-call's endIdx, so pass a truncated
    // copy; a negative anchor clamps inside the callee exactly like batch.
    // Anchor/endIdx expressions may read out-meta locals of the transcribed
    // region (MACDEXT's outNbElement1; APO/PPO/PVO read fastNb; STOCHRSI mixes
    // outBegIdx2 with dummyNBElement), so render them with the region's
    // address-of sets. `collect_address_of_vars` is computed only to feed
    // `collect_double_address_of_vars`: C# has no `address_of_vars` context
    // field, because an int taken by address stays a plain int local.
    let ins_address_of = collect_address_of_vars(&combined);
    let ins_double_address_of = collect_double_address_of_vars(&combined, &ins_address_of);
    let ins_ctx = CsRenderCtx {
        single_precision: false,
        nullable_outputs: &empty,
        nullable_shadow: false,
        double_address_of_vars: &ins_double_address_of,
        float_input_params: &empty,
        inline_counter: counter,
        fma: Some(stream_fma),
        matype_map: HashMap::new(),
    };
    let region_len = region_stmts.len();
    // Own inputs are exactly `historyLen` long — `emit_open_validation` above
    // rejects any input whose length differs from the first one's, and `endIdx`
    // is `historyLen - 1` — so a copy of `[0, endIdx]` out of one of them
    // reproduces the array it was taken from. See the elision below.
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
        // C# has no slice type either, so the range the callee may read is
        // conveyed by materializing it — but as STATEMENTS, because there is no
        // expression-shaped `Arrays.copyOfRange` twin. (Never `x.Clone()`: it
        // copies the WHOLE array, which is both the wrong range and 2.3x
        // slower.) Where the range is the whole array the copy conveys nothing
        // the array does not already say, and it is a `historyLen`-element
        // allocate-copy-discard per sub-open (issue #203). Both halves of the
        // condition are decidable here: the source is one of our own inputs (so
        // its length is `historyLen`, checked above) and the range ends at
        // `endIdx` (`historyLen - 1`). Nothing mutates the argument — the same
        // parameter is `const double[]` in the C prototype and `&[f64]` in Rust
        // — so the copy was never a defence, which is also why one materialized
        // range serves a callee that is handed the same series more than once
        // (STOCHRSI feeds one RSI buffer to all three of STOCHF's price inputs,
        // exactly as the batch call does).
        //
        // THAT DEDUP RESTS ON AN INVARIANT WORTH STATING, because it is the one
        // thing Java gets structurally and C# gets only by argument. Java hands
        // each price input its own `copyOfRange` result, so two of a callee's
        // sources can never be the same array; here they can. The fusion seam
        // also drops the #108/#130 aliasing guard on the strength of
        // `SubCallStep::is_fusable()`, and that predicate compares DESTINATIONS
        // against sources and against each other — never sources against each
        // other. So sharing one buffer across a callee's inputs is sound only
        // while no `_OpenImpl` ever writes through an input array. That holds
        // for all 172 today. A future body that wrote into an input would break
        // this silently, and no value gate would see it: both arms of the
        // stream-vs-batch compare would read the same corrupted buffer.
        //
        // Sizing note for the same reason: `Array.Copy` throws where Java's
        // `copyOfRange` would zero-pad a short source, and that throw is inside
        // `_OpenImpl`, so it would escape the RetCode -> `Core.StreamFailure`
        // mapping and reach the caller without the stable "<NAME> open: "
        // prefix. Unreachable as long as every `subLen` is bounded by the
        // buffer it copies from, which is true at all five sites (STOCH x2,
        // STOCHF, STOCHRSI, MACDEXT).
        let mut src_locals = String::new();
        let mut materialized: std::collections::BTreeMap<String, String> =
            std::collections::BTreeMap::new();
        let mut src_names: Vec<String> = Vec::new();
        for src in &sub.srcs {
            let name = if outputs.contains(src) {
                format!("sc_{src}")
            } else {
                src.clone()
            };
            if own_inputs.contains(&name) && e_arg == "endIdx" {
                src_names.push(name);
                continue;
            }
            if let Some(existing) = materialized.get(&name) {
                src_names.push(existing.clone());
                continue;
            }
            if materialized.is_empty() {
                let _ = writeln!(src_locals, "      int subLen{si} = ({e_arg}) + 1;");
            }
            let local = format!("subSrc{si}_{}", materialized.len());
            let _ = writeln!(src_locals, "      double[] {local} = new double[subLen{si}];");
            let _ = writeln!(src_locals, "      {name}.Slice(0, subLen{si}).CopyTo({local});");
            materialized.insert(name.clone(), local.clone());
            src_names.push(local);
        }
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
        t.push_str(&src_locals);
        // Fused (issue #192): one pass that BOTH warms the handle and fills this
        // sub-call's destination, so the batch sub-call transcribed next has
        // nothing left to compute and is dropped.
        let fused = sub.is_fusable()
            .then(|| streaming::batch_call_out_args(&tail_stmts[sub.tail_idx], sub))
            .flatten();
        if let Some((out_meta, dsts)) = fused {
            let rend = |e: &Expr| render_expr(&sc_rewrite(e), &ins_ctx, registry, helpers);
            // The callee's out-meta pair is `out int` here, so the ARGUMENT
            // needs the keyword too (CS1620). `&localBegIdx` already renders
            // `out localBegIdx`; a bare `Var` — this body's own out-parameter
            // threaded straight through, which is what STDDEV/BBANDS/STOCH pass
            // — does not, and gets it added.
            let metas: Vec<String> = out_meta
                .iter()
                .map(|e| match &sc_rewrite(e) {
                    Expr::Var(name) => format!("out {name}"),
                    other => render_expr(other, &ins_ctx, registry, helpers),
                })
                .collect();
            let dst_args: Vec<String> = dsts.iter().map(|e| rend(e)).collect();
            let _ = writeln!(
                t,
                "      {cls} sub{si} = {callee_base}_OpenAndFillInternal({}, {anchor}{opt_tail}, {}, {});",
                src_names.join(", "),
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
                src_names.join(", ")
            );
        }
        inserts.push((region_len + sub.tail_idx, t));
    }

    emit_open_region(
        o, func, &combined, enums, registry, helpers, counter, stream_fma, &inserts, &replaced,
    );

    // --- capture ------------------------------------------------------------
    let _ = writeln!(o, "      /* Capture the live producer state + sub handles. */");
    let _ = writeln!(o, "      if( outNBElement < 1 ) {{");
    let _ = writeln!(o, "         return RetCode.InsufficientHistory;");
    let _ = writeln!(o, "      }}");
    // Lag rings: seed from the tail of the still-live intermediate array (its
    // batch `free()` renders as nothing in a managed backend, so no
    // withheld-free dance).
    for ring in &cp.sub_lag_rings {
        let sr = &ring.series;
        let lag = render_expr(&ring.lag, &ctx, registry, helpers);
        let _ = writeln!(o, "      int lagCap_{sr} = (int)({lag});");
        let _ = writeln!(o, "      double[] lagRing_{sr} = new double[lagCap_{sr}];");
        let _ = writeln!(o, "      for( int lagI = 0; lagI < lagCap_{sr}; lagI++ ) {{");
        let _ = writeln!(o, "         lagRing_{sr}[lagI] = {sr}[outNBElement + lagI];");
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
        // The producer's own "output" is the intermediate series, so its cur
        // seeding is suppressed; the real outputs seed from `sc_` below.
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
    // The composed `cur_*` seed from the FUNCTION outputs, never the producer
    // series/model — the producer's "output" is the intermediate.
    emit_cur_capture(o, func, outputs, CurSource::Scratch);
    // Both modes compute into `sc_*` and seed `sp.cur_*` from it above; only the
    // hand-back differs, and it is the ONE place a stride multiply cannot
    // express the difference — a bulk copy takes a base array, not a subscript.
    // At stride 0 there is nothing to hand back: the scalar sink is one element
    // and the handle already carries the value.
    if !alias_fill {
        for out in outputs {
            let _ = writeln!(
                o,
                "      if( outStride == 1 ) sc_{out}.Slice(0, outNBElement).CopyTo({out});"
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
    // Lag rings ride the field list (a plain array field copies with the rest);
    // sub handles need per-callee copy constructors (copy_extra).
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
        // `= null!`: the field is a non-nullable reference the constructors do
        // not set, which is CS8618 (an error here); `<N>_OpenImpl` overwrites it
        // before the handle escapes and every constructor is internal.
        let _ = writeln!(extra_members, "      internal {cls} sub{si} = null!;");
        let _ = writeln!(copy_extra, "         this.sub{si} = new {cls}(other.sub{si});");
        // A sub's class is fixed by the plan, so a scratch always has one to
        // overwrite; the null arm covers a scratch built by the bare `(Core)`
        // constructor, which no path takes today.
        let _ = writeln!(restore_extra, "         if( this.sub{si} is null ) {{");
        let _ = writeln!(restore_extra, "            this.sub{si} = new {cls}(other.sub{si});");
        let _ = writeln!(restore_extra, "         }} else {{");
        let _ = writeln!(restore_extra, "            this.sub{si}.CopyFrom(other.sub{si});");
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
