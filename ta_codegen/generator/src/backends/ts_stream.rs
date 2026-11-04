//! TypeScript stream emitter: the `/**** Streaming API *****/` section of each
//! streamable function's module. It renders the same backend-neutral analysis
//! as `java_stream.rs`, through the batch tier's statement renderer and type
//! pass, so a streamed value is the batch value bit for bit.
//!
//! The handle's state fields are public and `@internal`: the step, peek and
//! open frames are module-level functions, which cannot reach a `private` or
//! `#` member.

use std::cell::Cell;
use std::collections::{BTreeSet, HashMap};
use std::fmt::Write;

use crate::candle_settings::detect_candle_settings;
use crate::ir::{CircBuf, Expr, FuncDef, Lang, ParamType, Statement, Tier, VarType};
use crate::streaming::{self, StreamModel, StreamPlan};

use super::common;
use super::ctype::CType;
use super::fma::{self, FmaVarSets};
use super::java_stream::{
    build_composed_open_bodies, build_open_body_java, composed_cur_scalars, dual_scalar_union, has_return,
    localize_state_writes, transform_map_step, value_field_name,
};
use super::ts::{self, ts_names, Services, TsCtx};
use super::ts_doc::{self, StreamMethod};

pub const SECTION_MARKER: &str = "/**** Streaming API *****/";

/// Whether `func` gets a stream section: declared streamable, and the analysis
/// of its TypeScript resolution accepts it.
pub fn emits_stream(func: &FuncDef, lookup: &dyn streaming::CalleeLookup) -> bool {
    func.streaming && streaming::validate_streamable(&func.resolved_for(Lang::Ts), lookup).is_ok()
}

fn has_out_class(func: &FuncDef) -> bool {
    func.outputs.len() > 1
}

fn out_ctype(func: &FuncDef, name: &str) -> CType {
    if func.outputs.iter().any(|o| o.name == name && o.param_type == ParamType::Integer) {
        CType::Int32
    } else {
        CType::Real
    }
}

fn arr_ty(t: CType) -> &'static str {
    match t {
        CType::Int32 => "Int32Array",
        _ => "Float64Array",
    }
}

fn ts_ty(t: CType) -> &'static str {
    match t {
        CType::RealPtr => "Float64Array",
        CType::IntPtr => "Int32Array",
        _ => "number",
    }
}

fn bar_params(func: &FuncDef) -> (String, String) {
    let inputs = streaming::input_array_names(func);
    let sig: Vec<String> = inputs.iter().map(|a| format!("{a}: number")).collect();
    (sig.join(", "), inputs.join(", "))
}

fn opt_sig(func: &FuncDef) -> Vec<String> {
    func.optional_inputs.iter().map(|p| format!("{}: number", p.name)).collect()
}

fn opt_fwd(func: &FuncDef) -> Vec<String> {
    func.optional_inputs.iter().map(|p| p.name.clone()).collect()
}

/// A lookback call from a frame whose enum parameters are plain numbers.
fn lookback_call(func: &FuncDef, sym: &str) -> String {
    let mut args: Vec<String> = func
        .optional_inputs
        .iter()
        .map(|p| match &p.param_type {
            ParamType::Enum(e) => format!("{} as {e}", p.name),
            _ => p.name.clone(),
        })
        .collect();
    args.push("core".to_string());
    format!("{sym}({})", args.join(", "))
}

fn comma(list: &[String]) -> String {
    if list.is_empty() {
        String::new()
    } else {
        format!(", {}", list.join(", "))
    }
}

macro_rules! names_through {
    ($t:ty, $output:expr, $tape:expr) => {
        impl streaming::NameMap for $t {
            fn state(&self, name: &str) -> String {
                format!("sp.{name}")
            }
            fn bar(&self, array: &str) -> String {
                array.to_string()
            }
            fn output(&self, name: &str) -> Expr {
                #[allow(clippy::redundant_closure_call)]
                ($output)(self, name)
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
            fn tape(&self) -> Option<streaming::TapeNames> {
                #[allow(clippy::redundant_closure_call)]
                ($tape)(self)
            }
        }
    };
}

/// State through `sp.`, bars as same-named scalars, outputs as `sp.cur_<name>`.
/// `fma::stream_base` strips exactly these prefixes, so the fused sites match C.
struct TsStreamNames;

names_through!(
    TsStreamNames,
    |_: &TsStreamNames, name: &str| Expr::Var(format!("sp.cur_{name}")),
    |_: &TsStreamNames| None
);

/// [`TsStreamNames`] for a tape frame.
struct TsTapeNames(streaming::TapeNames);

names_through!(
    TsTapeNames,
    |_: &TsTapeNames, name: &str| Expr::Var(format!("sp.cur_{name}")),
    |t: &TsTapeNames| Some(t.0.clone())
);

/// A composed producer: its intermediate series lands in a `cur_<series>` local.
struct TsComposedNames {
    series: String,
}

names_through!(
    TsComposedNames,
    |s: &TsComposedNames, name: &str| {
        if name == s.series {
            Expr::Var(format!("cur_{name}"))
        } else {
            Expr::Var(format!("sp.cur_{name}"))
        }
    },
    |_: &TsComposedNames| None
);

/// One handle field: its name, C type and identity-path default.
#[derive(Clone)]
struct Field {
    name: String,
    ty: CType,
    default: String,
}

impl Field {
    /// The class initializer: the identity default, except that a parameter's
    /// is the parameter itself.
    fn init(&self) -> &str {
        if self.ty.is_pointer() || self.default.starts_with("RetCode.") || self.default.parse::<i64>().is_ok() {
            &self.default
        } else {
            "0"
        }
    }
}

fn field(name: impl Into<String>, ty: CType, default: impl Into<String>) -> Field {
    Field { name: name.into(), ty, default: default.into() }
}

fn var_field(name: &str, ty: &VarType) -> Field {
    let (t, d) = match ty {
        VarType::Real => (CType::Real, "0".to_string()),
        VarType::Integer | VarType::Index => (CType::Int32, "0".to_string()),
        VarType::RetCodeType => (CType::Int32, "RetCode.SUCCESS".to_string()),
        VarType::RealPointer => (CType::RealPtr, "new Float64Array(1)".to_string()),
        VarType::IntPointer => (CType::IntPtr, "new Int32Array(1)".to_string()),
        VarType::RealArray(n) => (CType::RealPtr, format!("new Float64Array({n})")),
        VarType::IntArray(n) => (CType::IntPtr, format!("new Int32Array({n})")),
    };
    field(name, t, d)
}

/// A local's declaration with its zero value.
fn local_decl(name: &str, ty: &VarType) -> String {
    if *ty == VarType::RetCodeType {
        return format!("let {name}: RetCode = RetCode.SUCCESS;");
    }
    let f = var_field(name, ty);
    if f.ty.is_pointer() {
        format!("let {name}: {} = {};", ts_ty(f.ty), f.default)
    } else {
        format!("let {name} = {};", f.default)
    }
}

fn base_fields(func: &FuncDef) -> Vec<Field> {
    let mut fields = Vec::new();
    for p in &func.optional_inputs {
        fields.push(field(&p.name, CType::of_param(&p.param_type), &p.name));
    }
    for (name, c_type) in &func.private_extra_params {
        fields.push(field(name, extra_ctype(c_type), name));
    }
    for out in &func.outputs {
        fields.push(field(format!("cur_{}", out.name), out_ctype(func, &out.name), "0"));
    }
    fields
}

fn extra_ctype(c_type: &str) -> CType {
    match c_type {
        "double" => CType::Real,
        "int" => CType::Int32,
        other => panic!("unsupported private extra param type: {other}"),
    }
}

fn state_fields(func: &FuncDef, model: &StreamModel, step_settings: &BTreeSet<String>) -> Vec<Field> {
    state_fields_from(func, model, &model.state, step_settings)
}

fn state_fields_from(
    func: &FuncDef,
    model: &StreamModel,
    scalars: &[(String, VarType)],
    step_settings: &BTreeSet<String>,
) -> Vec<Field> {
    let mut fields = Vec::new();
    for p in &func.optional_inputs {
        fields.push(field(&p.name, CType::of_param(&p.param_type), &p.name));
    }
    for (name, c_type) in &func.private_extra_params {
        fields.push(field(name, extra_ctype(c_type), name));
    }
    for (name, ty) in scalars {
        fields.push(var_field(name, ty));
    }
    for name in &model.out_feedback {
        fields.push(field(format!("lastOut_{name}"), out_ctype(func, name), "0"));
    }
    for lag in &model.lags {
        for k in 1..=lag.depth {
            fields.push(field(StreamModel::lag_field(&lag.array, k), CType::Real, "0"));
        }
    }
    for ring in model.rings() {
        let v = &ring.var;
        fields.push(field(format!("ringPos_{v}"), CType::Int32, "0"));
        let id_cap = if ring.back > 0 { format!("{}", ring.back + 1) } else { "0".into() };
        fields.push(field(format!("ringCap_{v}"), CType::Int32, id_cap));
        if ring.back > 0 {
            fields.push(field(format!("ringLag_{v}"), CType::Int32, "0"));
        }
        for arr in &ring.arrays {
            let id_len = if ring.back > 0 { format!("{}", ring.back + 1) } else { "1".into() };
            fields.push(field(format!("ring_{v}_{arr}"), CType::RealPtr, format!("new Float64Array({id_len})")));
        }
    }
    for win in model.windows() {
        let v = &win.var;
        fields.push(field(format!("winPos_{v}"), CType::Int32, "0"));
        fields.push(field(format!("winCap_{v}"), CType::Int32, "1"));
        for arr in &win.arrays {
            fields.push(field(format!("win_{v}_{arr}"), CType::RealPtr, "new Float64Array(1)"));
        }
    }
    for circ in model.circs() {
        fields.push(field(format!("cbSize_{}", circ.id), CType::Int32, "0"));
        for (storage, ty) in streaming::circ_storages(circ) {
            let f = if matches!(ty, VarType::Integer) {
                field(format!("cb_{storage}"), CType::IntPtr, "new Int32Array(1)")
            } else {
                field(format!("cb_{storage}"), CType::RealPtr, "new Float64Array(1)")
            };
            fields.push(f);
        }
    }
    if let Some(ex) = model.extrema() {
        fields.push(field("xMask", CType::Int32, "0"));
        for arr in &ex.arrays {
            fields.push(field(format!("x_{arr}"), CType::RealPtr, "new Float64Array(1)"));
        }
    }
    for s in step_settings {
        fields.push(field(format!("cs_{s}_rangeType"), CType::Int32, "0"));
        fields.push(field(format!("cs_{s}_avgPeriod"), CType::Int32, "0"));
        fields.push(field(format!("cs_{s}_factor"), CType::Real, "0"));
    }
    for out in &func.outputs {
        fields.push(field(format!("cur_{}", out.name), out_ctype(func, &out.name), "0"));
    }
    fields
}

/// Sees every IR body a frame renders, with the context that renders it.
pub(crate) type Visit<'v> = &'v dyn Fn(&TsCtx, &[Statement]);

/// What every frame of one function shares.
struct Em<'a> {
    func: &'a FuncDef,
    svc: &'a Services<'a>,
    fma: &'a FmaVarSets,
    counter: &'a Cell<usize>,
    visit: Option<Visit<'a>>,
}

impl<'a> Em<'a> {
    fn render(&self, ctx: &TsCtx, body: &[Statement], indent: usize) -> String {
        if let Some(visit) = self.visit {
            visit(ctx, body);
        }
        body.iter().map(|s| ctx.stmt(s, indent)).collect()
    }

    fn ctx(&self, body: &[Statement]) -> TsCtx<'a> {
        TsCtx::for_stream(self.svc, body, self.fma, self.counter)
    }

    fn name(&self) -> &str {
        &self.func.name
    }

    fn base(&self) -> String {
        common::camel_words(&self.func.name)
    }

    fn class(&self) -> String {
        ts_names(&self.func.name).stream_class
    }

    fn out_class(&self) -> String {
        ts_names(&self.func.name).out_class
    }

    fn lookback(&self) -> String {
        lookback_call(self.func, &ts_names(&self.func.name).lookback)
    }

    /// The function's own lookback where transcribed locals are in scope: a
    /// local of the same name is reached past through a self-import alias.
    fn lookback_in(&self, ctx: &TsCtx) -> String {
        let sym = ts::note_import(ctx, &self.func.name.to_lowercase(), &ts_names(&self.func.name).lookback);
        lookback_call(self.func, &sym)
    }

    /// A context for step and peek code: every handle field under `sp.` and
    /// bare (a peek carries a written field in a local of its own name), the
    /// bars, the outputs' `cur_` locals and the model's temps.
    fn frame_ctx(
        &self,
        body: &[Statement],
        fields: &[Field],
        model: Option<&StreamModel>,
        step_settings: &BTreeSet<String>,
        tape: bool,
    ) -> TsCtx<'a> {
        let ctx = self.ctx(body);
        ctx.inline_fma_per_bar(body);
        for f in fields {
            ctx.env.declare(&format!("sp.{}", f.name), f.ty);
            ctx.env.declare(&f.name, f.ty);
        }
        for b in streaming::input_array_names(self.func) {
            ctx.env.declare(&b, CType::Real);
        }
        for out in &self.func.outputs {
            ctx.env.declare(&format!("cur_{}", out.name), out_ctype(self.func, &out.name));
        }
        if let Some(m) = model {
            for (n, t) in &m.temps {
                ctx.env.declare(n, CType::of_var_type(t));
            }
        }
        if tape {
            ctx.env.declare("tape", CType::RealPtr);
            ctx.env.declare("tapeBase", CType::Int32);
            ctx.env.declare("tapeMask", CType::Int32);
        }
        ctx.env.seed_candles(step_settings);
        ctx
    }

    /// A context for a transcribed open body.
    fn open_ctx(&self, open_body: &[Statement]) -> TsCtx<'a> {
        let ctx = self.ctx(open_body);
        ctx.env.seed_signature(self.func);
        ctx.env.seed_candles(&detect_candle_settings(open_body));
        for n in ["outStride", "historyLen", "fillJ", "fillLb", "fillIdx"] {
            ctx.env.declare(n, CType::Int32);
        }
        ctx
    }
}

/// The stream section of one function's module.
#[must_use]
pub(crate) fn generate(func: &FuncDef, svc: &Services) -> String {
    generate_visiting(func, svc, None)
}

/// [`generate`], handing every rendered IR body to `visit`.
pub(crate) fn generate_visiting(func: &FuncDef, svc: &Services, visit: Option<Visit>) -> String {
    let resolved = func.resolved_for(Lang::Ts);
    let func: &FuncDef = &resolved;
    assert!(func.streaming, "ts_stream::generate called without a streaming declaration");
    let plan = streaming::validate_streamable(func, svc.registry).unwrap_or_else(|e| panic!("streaming gate: {e}"));
    // From the body these frames render, so the fused sites are the ones
    // every other backend's stream fuses.
    let mut stream_fma = fma::build_fma_var_sets(func.stream_source(), &func.outputs, &fma::INDEX_PARAM_SEEDS);
    for input in streaming::input_array_names(func) {
        stream_fma.real_vars.insert(input);
    }
    let counter = Cell::new(0usize);
    let em = Em { func, svc, fma: &stream_fma, counter: &counter, visit };

    let mut o = String::new();
    let _ = writeln!(o, "\n{SECTION_MARKER}");
    if let Some(m) = func.alt_marker(Tier::Stream, Lang::Ts) {
        let _ = writeln!(o, "\n/* {m} */");
    }
    match &plan {
        StreamPlan::Loop(model) => emit_loop_shape(&mut o, &em, model, model.body),
        StreamPlan::DualMode(dmp) => emit_dual_mode(&mut o, &em, dmp),
        StreamPlan::Dispatch(dp) => emit_dispatch(&mut o, &em, dp),
        StreamPlan::PeriodBank(pb) => emit_period_bank(&mut o, &em, pb),
        StreamPlan::Composed(cp) => emit_composed(&mut o, &em, cp),
    }
    o
}

/// Every output paired with every input and every other output over the
/// converted inputs (H15), as two tests: one start address, which is refused
/// ahead of the history check, and any overlap, which C# and TS test only once
/// the history is long enough, so a short history answers
/// `TA_INSUFFICIENT_HISTORY` in every language.
fn alias_conditions(func: &FuncDef, input: &dyn Fn(&str) -> String) -> Option<(String, String)> {
    let inputs = streaming::input_array_names(func);
    let outs: Vec<&str> = func.outputs.iter().map(|out| out.name.as_str()).collect();
    let mut pairs: Vec<(String, String)> = Vec::new();
    for out in &outs {
        for i in &inputs {
            pairs.push(((*out).to_string(), input(i)));
        }
    }
    for i in 0..outs.len() {
        for b in &outs[i + 1..] {
            pairs.push((outs[i].to_string(), (*b).to_string()));
        }
    }
    if pairs.is_empty() {
        return None;
    }
    let same = pairs.iter().map(|(a, b)| format!("({a}.buffer === {b}.buffer && {a}.byteOffset === {b}.byteOffset)"));
    let partial = pairs.iter().map(|(a, b)| format!("overlaps({a}, {b})"));
    Some((same.collect::<Vec<_>>().join(" || "), partial.collect::<Vec<_>>().join(" || ")))
}

#[derive(Clone, Copy, PartialEq, Eq)]
enum OutMode {
    Scalar,
    Fill,
    FillInternal,
    Core,
}

fn emit_loop_shape(o: &mut String, em: &Em, model: &StreamModel, body: &[Statement]) {
    let step_settings = detect_candle_settings(&model.steady_stmts);
    let fields = state_fields(em.func, model, &step_settings);
    let frame = peek_frame(em, model, &TsStreamNames, &fields, &step_settings, 4, &BTreeSet::new())
        .map(PeekFrame::falls_through);
    emit_handle_class(o, em, &fields, &Members::default(), frame.as_ref());
    emit_step(o, em, model, &fields, &step_settings);
    emit_open_body(o, em, model, body, &fields, &step_settings);
    emit_open_and_fill_internal_wrapper(o, em, true);
    emit_open_wrappers(o, em, true);
    if em.svc.registry.in_tape_set(&em.func.name.to_lowercase()) {
        let tier = TapeTier { models: &[model], dual: None, fields: &fields, step_settings: &step_settings };
        emit_tape_entries(o, em, &tier);
    }
}

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

fn params_on_state(func: &FuncDef, e: &Expr) -> Expr {
    let params: Vec<String> = func.optional_inputs.iter().map(|p| p.name.clone()).collect();
    streaming::rewrite_expr(e, &|x| match x {
        Expr::Var(v) if params.contains(&v) => Expr::Var(format!("sp.{v}")),
        other => other,
    })
}

/// A tier's members beyond the fields: their declarations and the statements
/// that deep-copy them into `c`.
#[derive(Default)]
struct Members {
    decls: String,
    copy: String,
}

struct PeekFrame {
    body: String,
    terminal: bool,
}

impl PeekFrame {
    fn falls_through(body: String) -> Self {
        Self { body, terminal: false }
    }
}

fn emit_handle_class(o: &mut String, em: &Em, fields: &[Field], members: &Members, frame: Option<&PeekFrame>) {
    let class = em.class();
    let _ = write!(o, "\n{}", ts_doc::stream_class_docs(em.func));
    let _ = writeln!(o, "export class {class} {{");
    let _ = writeln!(o, "  /** @internal */\n  core: Core;");
    for f in fields {
        let _ = writeln!(o, "  /** @internal */\n  {}: {} = {};", f.name, ts_ty(f.ty), f.init());
    }
    o.push_str(&members.decls);
    let _ = writeln!(o, "  /** @internal */\n  outRangeBegIdx: number = 0;");
    let _ = writeln!(o, "  /** @internal */\n  outRangeCount: number = 0;");
    let _ = writeln!(o, "\n  private constructor(core: Core) {{\n    this.core = core;\n  }}");
    let _ = writeln!(o, "\n  /** @internal */\n  static of(core: Core): {class} {{\n    return new {class}(core);\n  }}");
    let _ = writeln!(
        o,
        "\n{}  outRange(): OutRange {{\n    return new OutRange(this.outRangeBegIdx, this.outRangeCount);\n  }}",
        ts_doc::stream_method_docs(em.func, StreamMethod::OutRange)
    );
    let _ = writeln!(
        o,
        "\n{}  advance(): void {{\n{}    this.outRangeCount++;\n  }}",
        ts_doc::stream_method_docs(em.func, StreamMethod::Advance),
        out_range_ceiling_guard(em.func, "    ", "advance")
    );
    emit_update_method(o, em);
    emit_peek_method(o, em, frame);
    emit_value_method(o, em);
    let _ = write!(o, "\n{}", ts_doc::stream_method_docs(em.func, StreamMethod::Clone));
    let _ = writeln!(o, "  clone(): {class} {{");
    let _ = writeln!(o, "    const c = new {class}(this.core);");
    for f in fields {
        if f.ty.is_pointer() {
            let _ = writeln!(o, "    c.{0} = this.{0}.slice();", f.name);
        } else {
            let _ = writeln!(o, "    c.{0} = this.{0};", f.name);
        }
    }
    o.push_str(&members.copy);
    let _ = writeln!(o, "    c.outRangeBegIdx = this.outRangeBegIdx;");
    let _ = writeln!(o, "    c.outRangeCount = this.outRangeCount;");
    let _ = writeln!(o, "    return c;\n  }}");
    let _ = writeln!(o, "}}");
    emit_out_class(o, em);
}

fn emit_out_class(o: &mut String, em: &Em) {
    let func = em.func;
    if !has_out_class(func) {
        return;
    }
    let _ = write!(o, "\n{}", ts_doc::out_class_docs(func));
    let _ = writeln!(o, "export class {} {{", em.out_class());
    for out in &func.outputs {
        o.push_str(&ts_doc::out_field_docs(func, out));
        let _ = writeln!(o, "  {}: number = 0;", value_field_name(&out.name));
    }
    let _ = writeln!(o, "}}");
}

fn write_out_stmts(func: &FuncDef, src: &str, indent: &str) -> String {
    let mut o = String::new();
    for out in &func.outputs {
        let rhs = if src.is_empty() { format!("cur_{}", out.name) } else { format!("{src}.cur_{}", out.name) };
        let _ = writeln!(o, "{indent}out.{} = {rhs};", value_field_name(&out.name));
    }
    o
}

fn assert_single_output(func: &FuncDef, site: &str) {
    assert!(!has_out_class(func), "{}: {site} has no multi-output spelling", func.name);
}

fn fresh_value_expr_local(func: &FuncDef) -> String {
    assert_single_output(func, "peek frame early exit");
    format!("cur_{}", func.outputs[0].name)
}

fn out_range_ceiling_guard(func: &FuncDef, indent: &str, verb: &str) -> String {
    format!(
        "{indent}if (this.outRangeBegIdx + this.outRangeCount > Core.INDEX_MAX) {{\n{indent}  throw failure('{} {verb}', RetCode.OUT_OF_RANGE_END_INDEX);\n{indent}}}\n",
        func.name
    )
}

/// Every bar input finite, before anything is written: a handle keeps its
/// state, so one non-finite bar would poison every later value.
fn finite_bar_check(func: &FuncDef, indent: &str, what: &str) -> String {
    let bars = streaming::input_array_names(func);
    if bars.is_empty() {
        return String::new();
    }
    let conds: Vec<String> = bars.iter().map(|b| format!("!Number.isFinite({b})")).collect();
    let (last, rest) = bars.split_last().expect("non-empty");
    let mut name = format!("'{last}'");
    for b in rest.iter().rev() {
        name = format!("!Number.isFinite({b}) ? '{b}' : {name}");
    }
    format!(
        "{indent}if ({}) {{\n{indent}  throw nonFiniteBar('{} {what}', {name});\n{indent}}}\n",
        conds.join(" || "),
        func.name
    )
}

/// A multi-output call's sink check, ahead of the finite-bar check as in Java:
/// an absent sink must be rejected before the step commits the bar. Not an
/// `instanceof`: a structurally typed object, or a sink from a second copy of
/// the package, is a valid sink.
fn sink_check(em: &Em, indent: &str, verb: &str) -> String {
    if !has_out_class(em.func) {
        return String::new();
    }
    let cls = em.out_class();
    format!(
        "{indent}if (typeof out !== 'object' || out === null) {{\n{indent}  throw badSink('{} {verb}', '{cls}');\n{indent}}}\n",
        em.func.name
    )
}

fn sink_param(em: &Em) -> String {
    if has_out_class(em.func) {
        format!(", out: {}", em.out_class())
    } else {
        String::new()
    }
}

fn value_type(em: &Em) -> &'static str {
    if has_out_class(em.func) {
        "void"
    } else {
        "number"
    }
}

fn emit_update_method(o: &mut String, em: &Em) {
    let func = em.func;
    let (sig_bars, fwd_bars) = bar_params(func);
    let sep = if sig_bars.is_empty() { "" } else { ", " };
    let _ = write!(o, "\n{}", ts_doc::stream_method_docs(func, StreamMethod::Update));
    let sink = sink_param(em);
    let sink = if sig_bars.is_empty() { sink.trim_start_matches(", ").to_string() } else { sink };
    let _ = writeln!(o, "  update({sig_bars}{sink}): {} {{", value_type(em));
    o.push_str(&out_range_ceiling_guard(func, "    ", "update"));
    o.push_str(&sink_check(em, "    ", "update"));
    o.push_str(&finite_bar_check(func, "    ", "update"));
    let _ = writeln!(o, "    {}StepImpl(this{sep}{fwd_bars});", em.base());
    let _ = writeln!(o, "    this.outRangeCount++;");
    if has_out_class(func) {
        o.push_str(&write_out_stmts(func, "this", "    "));
    } else {
        let _ = writeln!(o, "    return this.cur_{};", func.outputs[0].name);
    }
    let _ = writeln!(o, "  }}");
}

fn emit_peek_method(o: &mut String, em: &Em, frame: Option<&PeekFrame>) {
    let func = em.func;
    let (sig_bars, _) = bar_params(func);
    let sink = sink_param(em);
    let sink = if sig_bars.is_empty() { sink.trim_start_matches(", ").to_string() } else { sink };
    let _ = write!(o, "\n{}", ts_doc::stream_method_docs(func, StreamMethod::Peek));
    let _ = writeln!(o, "  peek({sig_bars}{sink}): {} {{", value_type(em));
    o.push_str(&sink_check(em, "    ", "peek"));
    o.push_str(&finite_bar_check(func, "    ", "peek"));
    let frame = frame.expect("every tier emits a peek frame");
    let _ = writeln!(o, "    const sp = this;");
    o.push_str(&frame.body);
    if frame.terminal {
        assert!(!has_out_class(func), "a terminal peek frame would skip the caller's sink write");
    } else if has_out_class(func) {
        o.push_str(&write_out_stmts(func, "", "    "));
    } else {
        let _ = writeln!(o, "    return {};", fresh_value_expr_local(func));
    }
    let _ = writeln!(o, "  }}");
}

fn emit_value_method(o: &mut String, em: &Em) {
    let func = em.func;
    let _ = write!(o, "\n{}", ts_doc::stream_method_docs(func, StreamMethod::Value));
    if has_out_class(func) {
        let _ = writeln!(o, "  value(out: {}): void {{", em.out_class());
        o.push_str(&sink_check(em, "    ", "value"));
        o.push_str(&write_out_stmts(func, "this", "    "));
    } else {
        let _ = writeln!(o, "  value(): number {{");
        let _ = writeln!(o, "    return this.cur_{};", func.outputs[0].name);
    }
    let _ = writeln!(o, "  }}");
}

/// The peek frame of one model: the transition with every store it would make
/// carried in a local of the field's own name, run against the live handle.
fn peek_frame(
    em: &Em,
    model: &StreamModel,
    names: &dyn streaming::NameMap,
    fields: &[Field],
    step_settings: &BTreeSet<String>,
    indent: usize,
    predeclared: &BTreeSet<String>,
) -> Option<String> {
    peek_frame_with(em, model, names, fields, step_settings, indent, predeclared, None)
}

/// [`peek_frame`] with the caller's own locals declared to the type pass.
#[allow(clippy::too_many_arguments)]
fn peek_frame_with(
    em: &Em,
    model: &StreamModel,
    names: &dyn streaming::NameMap,
    fields: &[Field],
    step_settings: &BTreeSet<String>,
    indent: usize,
    predeclared: &BTreeSet<String>,
    declare: Option<&dyn Fn(&TsCtx)>,
) -> Option<String> {
    let func = em.func;
    let pad = " ".repeat(indent);
    let transition = streaming::build_transition(model, names).ok()?;
    let pt = if names.tape().is_some() {
        streaming::tape_peek_transition(model, names, &transition, None)
    } else {
        streaming::peek_transition_widest(model, names, &transition, None)
    }
    .ok()?;
    let bufs = streaming::transition_buffers(model, names);
    let (locals, body_ir) = localize_state_writes(func, &pt.body, &[], &bufs)?;
    let body_ir = streaming::rewrite_stmts(&body_ir, &|e| e, &|st| match st {
        Statement::Return { value: None } => {
            Some(Statement::Return { value: Some(Expr::Var(fresh_value_expr_local(func))) })
        }
        other => Some(other),
    });
    let types: HashMap<&str, &Field> = fields.iter().map(|f| (f.name.as_str(), f)).collect();

    let mut out = String::new();
    for (name, ty) in &streaming::temps_used(&model.temps, &body_ir) {
        let _ = writeln!(out, "{pad}{}", local_decl(name, ty));
    }
    let dead_seeds: BTreeSet<String> = func
        .outputs
        .iter()
        .map(|o| format!("cur_{}", o.name))
        .filter(|n| locals.contains(n) && streaming::peek_seed_is_dead(&body_ir, n))
        .collect();
    for name in &locals {
        if predeclared.contains(name) {
            continue;
        }
        let f = types.get(name.as_str()).copied()?;
        // An array field taken plain would write the handle through it.
        let init = if dead_seeds.contains(name) {
            "0".to_string()
        } else if f.ty.is_pointer() {
            format!("sp.{name}.slice()")
        } else {
            format!("sp.{name}")
        };
        let _ = writeln!(out, "{pad}let {name} = {init};");
    }
    for sh in &pt.shadows {
        let _ = writeln!(out, "{pad}let {} = -1;", sh.slot_var);
        let _ = writeln!(out, "{pad}let {} = 0;", sh.val_var);
    }
    for t in &pt.slot_temps {
        let _ = writeln!(out, "{pad}let {t} = 0;");
    }
    out.push_str(&settings_snapshot(step_settings, &pad));
    let mut ctx = em.frame_ctx(&body_ir, fields, Some(model), step_settings, names.tape().is_some());
    for sh in &pt.shadows {
        ctx.env.declare(&sh.slot_var, CType::Int32);
        ctx.env.declare(&sh.val_var, if sh.int_elem { CType::Int32 } else { CType::Real });
    }
    for t in &pt.slot_temps {
        ctx.env.declare(t, CType::Int32);
    }
    if let Some(d) = declare {
        d(&ctx);
    }
    if !has_out_class(func) {
        ctx.ret = out_ctype(func, &func.outputs[0].name);
    }
    out.push_str(&em.render(&ctx, &body_ir, indent));
    Some(out)
}

fn settings_snapshot(step_settings: &BTreeSet<String>, pad: &str) -> String {
    let mut out = String::new();
    for s in step_settings {
        let _ = writeln!(out, "{pad}const {s}_rangeType = sp.cs_{s}_rangeType;");
        let _ = writeln!(out, "{pad}const {s}_avgPeriod = sp.cs_{s}_avgPeriod;");
        let _ = writeln!(out, "{pad}const {s}_factor = sp.cs_{s}_factor;");
    }
    out
}

fn identity_branch_as_frame(func: &FuncDef, model: &StreamModel) -> Option<Vec<Statement>> {
    let st = streaming::identity_peek_branch(model, &TsStreamNames)?;
    let bare: HashMap<String, String> =
        func.outputs.iter().map(|o| (format!("sp.cur_{}", o.name), format!("cur_{}", o.name))).collect();
    Some(streaming::rewrite_stmts(
        std::slice::from_ref(&st),
        &|e| match e {
            Expr::Var(ref v) => bare.get(v).map_or(e, |b| Expr::Var(b.clone())),
            other => other,
        },
        &|s| match s {
            Statement::Return { value: None } => {
                Some(Statement::Return { value: Some(Expr::Var(fresh_value_expr_local(func))) })
            }
            other => Some(other),
        },
    ))
}

fn emit_step(o: &mut String, em: &Em, model: &StreamModel, fields: &[Field], step_settings: &BTreeSet<String>) {
    emit_step_sig(o, em);
    emit_step_body(o, em, model, fields, step_settings, 2, None);
    let _ = writeln!(o, "}}");
}

fn emit_step_sig(o: &mut String, em: &Em) {
    let (sig_bars, _) = bar_params(em.func);
    let sep = if sig_bars.is_empty() { "" } else { ", " };
    let _ = writeln!(o, "\nfunction {}StepImpl(sp: {}{sep}{sig_bars}): void {{", em.base(), em.class());
}

fn emit_step_body(
    o: &mut String,
    em: &Em,
    model: &StreamModel,
    fields: &[Field],
    step_settings: &BTreeSet<String>,
    indent: usize,
    tape: Option<&streaming::TapeNames>,
) {
    let pad = " ".repeat(indent);
    for (name, ty) in &streaming::step_temps(model, tape) {
        let _ = writeln!(o, "{pad}{}", local_decl(name, ty));
    }
    o.push_str(&settings_snapshot(step_settings, &pad));
    let tape_names = tape.map(|t| TsTapeNames(t.clone()));
    let names: &dyn streaming::NameMap = match &tape_names {
        Some(t) => t,
        None => &TsStreamNames,
    };
    let transition =
        streaming::build_transition(model, names).unwrap_or_else(|e| panic!("streaming transition: {e}"));
    assert!(
        tape.is_none() || !has_return(&transition),
        "{}: a tape step cannot render an early exit",
        model.func.name
    );
    let ctx = em.frame_ctx(&transition, fields, Some(model), step_settings, tape.is_some());
    o.push_str(&em.render(&ctx, &transition, indent));
}

// ---------------------------------------------------------------------------
// Tape entries: the step, peek and detach a period bank drives each slot
// through, every slot reading its price history from the bank's one tape.
// ---------------------------------------------------------------------------

const TAPE_PARAMS: &str = "tape: Float64Array, tapeBase: number, tapeMask: number";

struct TapeTier<'a> {
    models: &'a [&'a StreamModel<'a>],
    dual: Option<&'a streaming::DualModePlan<'a>>,
    fields: &'a [Field],
    step_settings: &'a BTreeSet<String>,
}

fn tape_input(func: &FuncDef) -> String {
    let inputs = streaming::input_array_names(func);
    assert!(inputs.len() == 1, "{}: a tape frame needs exactly one input, found {}", func.name, inputs.len());
    inputs[0].clone()
}

#[allow(clippy::too_many_lines)]
fn emit_tape_entries(o: &mut String, em: &Em, tier: &TapeTier) {
    let func = em.func;
    let base = em.base();
    let class = em.class();
    let (sig_bars, fwd_bars) = bar_params(func);
    let input = tape_input(func);
    let tape = streaming::TapeNames::new(&input);
    let mut detach: Vec<String> = Vec::new();
    for m in tier.models {
        streaming::check_tape_eligible(m, &input).unwrap_or_else(|e| panic!("{e}"));
        let n = &TsStreamNames;
        for r in streaming::tape_covered_rings(m, &input) {
            let (buf, cap) = (streaming::NameMap::ring_buf(n, &r.var, &input), streaming::NameMap::ring_cap(n, &r.var));
            detach.push(format!("  {buf} = new Float64Array(0);"));
            detach.push(format!("  if ({cap} > reach) {{\n    reach = {cap};\n  }}"));
        }
        for w in streaming::tape_covered_windows(m, &input) {
            let (buf, cap) = (streaming::NameMap::win_buf(n, &w.var, &input), streaming::NameMap::win_cap(n, &w.var));
            detach.push(format!("  {buf} = new Float64Array(0);"));
            detach.push(format!("  if ({cap} - 1 > reach) {{\n    reach = {cap} - 1;\n  }}"));
        }
    }
    let mut seen = BTreeSet::new();
    detach.retain(|l| seen.insert(l.clone()));
    let multi = has_out_class(func);
    let ret = if multi { "void" } else { "number" };
    let sink = if multi { format!(", out: {}", em.out_class()) } else { String::new() };
    let pred = tier.dual.map(|d| {
        let p = params_on_state(func, &d.predicate);
        let ctx = em.frame_ctx(&[], tier.fields, None, tier.step_settings, false);
        ctx.cond(&p)
    });

    let _ = writeln!(o, "\n/** @internal */\nexport function {base}StepTape(sp: {class}, {TAPE_PARAMS}, {sig_bars}): {ret} {{");
    if detach.is_empty() {
        let _ = writeln!(o, "  {base}StepImpl(sp, {fwd_bars});");
    } else if let Some(pred) = &pred {
        let _ = writeln!(o, "  if ({pred}) {{");
        emit_step_body(o, em, tier.models[0], tier.fields, tier.step_settings, 4, Some(&tape));
        let _ = writeln!(o, "  }} else {{");
        emit_step_body(o, em, tier.models[1], tier.fields, tier.step_settings, 4, Some(&tape));
        let _ = writeln!(o, "  }}");
    } else {
        emit_step_body(o, em, tier.models[0], tier.fields, tier.step_settings, 2, Some(&tape));
    }
    let _ = writeln!(o, "  sp.outRangeCount++;");
    if !multi {
        let _ = writeln!(o, "  return sp.cur_{};", func.outputs[0].name);
    }
    let _ = writeln!(o, "}}");

    let _ = writeln!(o, "\n/** @internal */\nexport function {base}PeekTape(sp: {class}, {TAPE_PARAMS}, {sig_bars}{sink}): {ret} {{");
    if detach.is_empty() {
        if multi {
            let _ = writeln!(o, "  sp.peek({fwd_bars}, out);");
        } else {
            let _ = writeln!(o, "  return sp.peek({fwd_bars});");
        }
    } else {
        let names = TsTapeNames(tape.clone());
        let frame = |model: &StreamModel, indent: usize, predeclared: &BTreeSet<String>| {
            peek_frame(em, model, &names, tier.fields, tier.step_settings, indent, predeclared)
                .unwrap_or_else(|| panic!("{}: no tape peek frame", func.name))
        };
        if let Some(pred) = &pred {
            let outs: BTreeSet<String> = func.outputs.iter().map(|x| format!("cur_{}", x.name)).collect();
            for out in &func.outputs {
                let _ = writeln!(o, "  let cur_{} = 0;", out.name);
            }
            let _ = writeln!(o, "  if ({pred}) {{");
            o.push_str(&frame(tier.models[0], 4, &outs));
            let _ = writeln!(o, "  }} else {{");
            o.push_str(&frame(tier.models[1], 4, &outs));
            let _ = writeln!(o, "  }}");
        } else {
            o.push_str(&frame(tier.models[0], 2, &BTreeSet::new()));
        }
        if multi {
            o.push_str(&write_out_stmts(func, "", "  "));
        } else {
            let _ = writeln!(o, "  return {};", fresh_value_expr_local(func));
        }
    }
    let _ = writeln!(o, "}}");

    let _ = writeln!(o, "\n/** @internal */\nexport function {base}TapeDetach(sp: {class}): number {{");
    if detach.is_empty() {
        let _ = writeln!(o, "  return 0;");
    } else {
        let _ = writeln!(o, "  let reach = 0;");
        for l in &detach {
            let _ = writeln!(o, "{l}");
        }
        let _ = writeln!(o, "  return reach;");
    }
    let _ = writeln!(o, "}}");
}

// ---------------------------------------------------------------------------
// Open transcription
// ---------------------------------------------------------------------------

fn cleanup_open_body(body: &[Statement], em: &Em) -> Vec<Statement> {
    let registry = em.svc.registry;
    let admits = |f: &str, a: &[Expr]| super::java::cross_call_split(f, a, registry).is_some();
    let folded = super::ir_cleanup::drop_answered_cross_call_guards(body, &admits, Some("InsufficientHistory"));
    let folded = super::ir_cleanup::drop_deallocation(&folded);
    super::ir_cleanup::drop_inert_guards(&folded)
}

fn emit_open_body(
    o: &mut String,
    em: &Em,
    model: &StreamModel,
    body: &[Statement],
    fields: &[Field],
    step_settings: &BTreeSet<String>,
) {
    emit_open_body_sig(o, em, OutMode::Core);
    let open_body = cleanup_open_body(&build_open_body_java(model, body), em);
    let ctx = em.open_ctx(&open_body);
    let mut s = String::new();
    emit_body_decls(&mut s, &open_body, &ctx);
    emit_open_head(&mut s, em);
    emit_open_validation(&mut s, em, OutMode::Core);
    emit_anchor_guard(&mut s);
    emit_extras_and_candle(&mut s, em, &open_body, &ctx);
    emit_identity_fast_path(&mut s, em, model, fields, &ctx);
    emit_open_region(&mut s, em, &open_body, &ctx, &[], &BTreeSet::new());
    emit_capture(&mut s, em, model, &model.state, step_settings, &ctx, Some(CurSource::StridedArray), "");
    let _ = writeln!(s, "  return RetCode.SUCCESS;");
    o.push_str(&with_core(&s));
    let _ = writeln!(o, "}}");
}

/// `const core = sp.core;` ahead of an open body that reads it.
fn with_core(body: &str) -> String {
    let code: String = body.lines().filter(|l| !l.trim_start().starts_with("/*") && !l.trim_start().starts_with('*')).collect();
    let reads = code.match_indices("core").any(|(i, _)| {
        let before = code[..i].chars().next_back();
        let after = code[i + 4..].chars().next();
        let ident = |c: char| c.is_ascii_alphanumeric() || c == '_' || c == '.' || c == '$';
        !before.is_some_and(ident) && !after.is_some_and(|c| c.is_ascii_alphanumeric() || c == '_' || c == '$')
    });
    if reads {
        format!("  const core = sp.core;\n{body}")
    } else {
        body.to_string()
    }
}

fn emit_open_body_sig(o: &mut String, em: &Em, mode: OutMode) {
    let func = em.func;
    let mut params: Vec<String> = vec![format!("sp: {}", em.class())];
    for input in streaming::input_array_names(func) {
        params.push(format!("{input}: Float64Array"));
    }
    if matches!(mode, OutMode::Scalar | OutMode::Core | OutMode::FillInternal) {
        params.push("startIdx: number".to_string());
    }
    params.extend(opt_sig(func));
    let base = em.base();
    let name = match mode {
        OutMode::Scalar | OutMode::Core => format!("{base}OpenImpl"),
        OutMode::Fill => format!("{base}OpenAndFillImpl"),
        OutMode::FillInternal => format!("{base}OpenAndFillInternalImpl"),
    };
    if mode != OutMode::Scalar {
        params.push("outBegIdx: MInteger".to_string());
        params.push("outNBElement: MInteger".to_string());
        for out in &func.outputs {
            params.push(format!("{}: {}", out.name, arr_ty(out_ctype(func, &out.name))));
        }
    }
    if mode == OutMode::Core {
        params.push("outStride: number".to_string());
    }
    let _ = writeln!(o, "\nfunction {name}(\n  {},\n): RetCode {{", params.join(",\n  "));
}

fn emit_anchor_guard(o: &mut String) {
    o.push_str(
        "  if (startIdx > endIdx) {\n    outBegIdx.value = 0;\n    outNBElement.value = 0;\n    return RetCode.INSUFFICIENT_HISTORY;\n  }\n",
    );
}

fn emit_body_decls(o: &mut String, open_body: &[Statement], ctx: &TsCtx) {
    for stmt in open_body {
        if let Statement::CircBuf(CircBuf::Prolog { id, layout, static_size }) = stmt {
            for (arr, t) in super::java::circbuf_arrays(id, layout) {
                let ty = if CType::of_var_type(&t) == CType::Real { "Float64Array" } else { "Int32Array" };
                let _ = writeln!(o, "  let {arr}!: {ty};");
            }
            let _ = writeln!(o, "  let {id}_Idx = 0;");
            let _ = writeln!(o, "  let maxIdx_{id} = ({static_size}) - 1;");
            continue;
        }
        if let Statement::VarDecl { var_type, name, .. } = stmt {
            let _ = writeln!(o, "  {}", ts::decl_line(name, var_type, ctx));
        }
    }
}

fn emit_open_head(o: &mut String, em: &Em) {
    let first = &streaming::input_array_names(em.func)[0];
    let _ = writeln!(o, "  const historyLen = {first}.length;");
    let _ = writeln!(o, "  let endIdx = historyLen - 1;");
}

fn emit_open_validation(o: &mut String, em: &Em, mode: OutMode) {
    let func = em.func;
    let inputs = streaming::input_array_names(func);
    let first = &inputs[0];
    o.push_str("  if (historyLen < 1) {\n    return RetCode.OUT_OF_RANGE_START_INDEX;\n  }\n");
    o.push_str("  if (historyLen > Core.INDEX_MAX + 1) {\n    return RetCode.OUT_OF_RANGE_END_INDEX;\n  }\n");
    let mismatches: Vec<String> = inputs[1..].iter().map(|extra| format!("{extra}.length !== {first}.length")).collect();
    if !mismatches.is_empty() {
        let _ = writeln!(o, "  if ({}) {{\n    return RetCode.BAD_PARAM;\n  }}", mismatches.join(" || "));
    }
    o.push_str(&ts::emit_opt_param_validation(func, "RetCode.BAD_PARAM", em.svc.enums));
    if mode == OutMode::Fill {
        if let Some((same, partial)) = alias_conditions(func, &str::to_string) {
            let _ = writeln!(o, "  if ({same}) {{\n    return RetCode.BAD_PARAM;\n  }}");
            let _ = writeln!(o, "  if (historyLen > {} && ({partial})) {{\n    return RetCode.BAD_PARAM;\n  }}", em.lookback());
        }
    }
}

fn emit_extras_and_candle(o: &mut String, em: &Em, open_body: &[Statement], ctx: &TsCtx) {
    let func = em.func;
    for (name, _) in &func.private_extra_params {
        let init = func
            .private_param_init
            .iter()
            .find(|(pn, _)| pn == name)
            .map_or_else(|| panic!("{}: no init for private param {name}", func.name), |(_, e)| ctx.expr(e));
        let _ = writeln!(o, "  const {name} = {init};");
    }
    o.push_str(&ts::emit_candle_unpacking(&detect_candle_settings(open_body)));
}

/// The transcribed region: initializers, then the statements, with a tier's
/// inserts spliced before the statement at their index.
fn emit_open_region(
    o: &mut String,
    em: &Em,
    open_body: &[Statement],
    ctx: &TsCtx,
    inserts: &[(usize, String)],
    replaced: &BTreeSet<usize>,
) {
    if let Some(visit) = em.visit {
        let emitted: Vec<Statement> =
            open_body.iter().enumerate().filter(|(i, _)| !replaced.contains(i)).map(|(_, s)| s.clone()).collect();
        visit(ctx, &emitted);
    }
    for stmt in open_body {
        if let Statement::VarDecl { var_type, name, init: Some(init) } = stmt {
            let (init, hoisted) = ctx.hoist(init);
            o.push_str(&ts::render_hoisted_blocks(&hoisted, 2, ctx));
            let target = if ctx.address_of_vars.contains(name) {
                format!("{name}.value")
            } else if ctx.double_address_of_vars.contains(name) {
                format!("{name}[0]")
            } else {
                name.clone()
            };
            let _ = writeln!(o, "  {target} = {};", ctx.value_for(CType::of_var_type(var_type), &init));
        }
    }
    for (i, stmt) in open_body.iter().enumerate() {
        for (at, text) in inserts {
            if *at == i {
                o.push_str(text);
            }
        }
        if matches!(stmt, Statement::VarDecl { .. }) || replaced.contains(&i) {
            continue;
        }
        o.push_str(&ctx.stmt(stmt, 2));
    }
}

/// The parameter-degenerate identity path: the whole range at stride 1, the
/// last bar alone at stride 0.
fn emit_identity_fast_path(o: &mut String, em: &Em, model: &StreamModel, fields: &[Field], ctx: &TsCtx) {
    let Some(idp) = &model.identity else { return };
    let cond = ctx.cond(&idp.condition);
    let _ = writeln!(o, "  if ({cond}) {{");
    let _ = writeln!(o, "    let fillLb = {};", em.lookback_in(ctx));
    let _ = writeln!(o, "    if (startIdx > fillLb) {{\n      fillLb = startIdx;\n    }}");
    let _ = writeln!(o, "    if (historyLen < fillLb + 1) {{\n      return RetCode.INSUFFICIENT_HISTORY;\n    }}");
    for f in fields {
        if f.name.starts_with("cur_") {
            continue;
        }
        if model.parity.as_ref().is_some_and(|p| p.field == f.name) {
            let _ = writeln!(o, "    sp.{} = historyLen % 2;", f.name);
        } else {
            let _ = writeln!(o, "    sp.{} = {};", f.name, f.default);
        }
    }
    let _ = writeln!(o, "    outBegIdx.value = fillLb;");
    let _ = writeln!(o, "    outNBElement.value = historyLen - fillLb;");
    let _ = writeln!(o, "    if (outStride === 0) {{");
    for (out, inp) in &idp.pairs {
        let _ = writeln!(o, "      {out}[0] = {inp}[historyLen - 1];");
    }
    let _ = writeln!(o, "    }} else {{");
    let _ = writeln!(o, "      for (let fillIdx = 0; fillIdx < historyLen - fillLb; fillIdx++) {{");
    for (out, inp) in &idp.pairs {
        let _ = writeln!(o, "        {out}[fillIdx] = {inp}[fillLb + fillIdx];");
    }
    let _ = writeln!(o, "      }}\n    }}");
    for (out, _) in &idp.pairs {
        let _ = writeln!(o, "    sp.cur_{out} = {out}[(outNBElement.value - 1) * outStride];");
    }
    let _ = writeln!(o, "    return RetCode.SUCCESS;\n  }}");
}

#[derive(Clone, Copy)]
enum CurSource {
    StridedArray,
    Scratch,
}

/// The capture epilogue: buffer capacities from the still-live batch locals,
/// the buffers, then every handle field. A CIRCBUF moves its batch storage,
/// rotation phase included.
#[allow(clippy::too_many_arguments)]
fn emit_capture(
    o: &mut String,
    em: &Em,
    model: &StreamModel,
    scalars: &[(String, VarType)],
    step_settings: &BTreeSet<String>,
    ctx: &TsCtx,
    cur_source: Option<CurSource>,
    extra_capture: &str,
) {
    emit_capture_buffers(o, model, ctx);
    emit_capture_stores(o, em, model, scalars, step_settings, ctx);
    o.push_str(extra_capture);
    if let Some(cs) = cur_source {
        emit_cur_capture(o, &model.outputs, cs);
    }
}

/// Each buffer's capacity from the still-live batch locals, and the buffer.
fn emit_capture_buffers(o: &mut String, model: &StreamModel, ctx: &TsCtx) {
    for ring in model.rings() {
        let v = &ring.var;
        let back = ring.back;
        let c = &model.cursor;
        if back > 0 {
            let _ = writeln!(o, "  const capLag_{v} = {c} - {v};");
            let _ = writeln!(o, "  const cap_{v} = capLag_{v} + {};", back + 1);
            let _ = writeln!(o, "  if (capLag_{v} < {} || cap_{v} > historyLen) {{", ring.fwd);
        } else {
            let _ = writeln!(o, "  const cap_{v} = {c} - {v};");
            let _ = writeln!(o, "  if (cap_{v} < {} || cap_{v} > historyLen) {{", i32::from(ring.lag_ge1));
        }
        let _ = writeln!(o, "    return RetCode.INTERNAL_ERROR;\n  }}");
        let _ = writeln!(o, "  const allocN_{v} = cap_{v} > 0 ? cap_{v} : 1;");
        for arr in &ring.arrays {
            let _ = writeln!(o, "  const capRing_{v}_{arr} = new Float64Array(allocN_{v});");
            let fill_rhs = ring.derived.as_ref().map(|dr| ctx.expr(&streaming::derived_fill_value(dr, "fillJ")));
            if back > 0 {
                let rhs = fill_rhs.clone().unwrap_or_else(|| format!("{arr}[fillJ]"));
                let _ = writeln!(o, "  for (let fillJ = historyLen - cap_{v}; fillJ < historyLen; fillJ++) {{");
                let _ = writeln!(o, "    capRing_{v}_{arr}[fillJ % cap_{v}] = {rhs};");
                let _ = writeln!(o, "  }}");
            } else if let Some(rhs) = fill_rhs {
                let _ = writeln!(o, "  for (let fillJ = historyLen - cap_{v}; fillJ < historyLen; fillJ++) {{");
                let _ = writeln!(o, "    capRing_{v}_{arr}[fillJ - (historyLen - cap_{v})] = {rhs};");
                let _ = writeln!(o, "  }}");
            } else {
                let _ = writeln!(o, "  capRing_{v}_{arr}.set({arr}.subarray(historyLen - cap_{v}, historyLen), 0);");
            }
        }
    }
    for win in model.windows() {
        let v = &win.var;
        let _ = writeln!(o, "  const cap_{v} = {};", ctx.value_for(CType::Int32, &win.cap));
        let _ = writeln!(o, "  if (cap_{v} < 1 || cap_{v} > historyLen) {{\n    return RetCode.INTERNAL_ERROR;\n  }}");
        for arr in &win.arrays {
            let _ = writeln!(o, "  const capWin_{v}_{arr} = {arr}.slice(historyLen - cap_{v}, historyLen);");
        }
    }
    if let Some(ex) = model.extrema() {
        let c = &model.cursor;
        let t = &ex.trailing;
        let _ = writeln!(o, "  const capX = {c} - {t} + 1;");
        let _ = writeln!(o, "  if (capX < 1 || capX > historyLen) {{\n    return RetCode.INTERNAL_ERROR;\n  }}");
        // A power of two, so `idx & xMask` maps any capX consecutive bars apart.
        let _ = writeln!(o, "  let physX = 1;\n  while (physX < capX) {{\n    physX <<= 1;\n  }}");
        for arr in &ex.arrays {
            let _ = writeln!(o, "  const capX_{arr} = new Float64Array(physX);");
        }
        let _ = writeln!(o, "  for (let fillJ = historyLen - capX; fillJ < historyLen; fillJ++) {{");
        for arr in &ex.arrays {
            let _ = writeln!(o, "    capX_{arr}[fillJ & (physX - 1)] = {arr}[fillJ];");
        }
        let _ = writeln!(o, "  }}");
    }
    for circ in model.circs() {
        let id = &circ.id;
        let _ = writeln!(o, "  const capCb_{id} = maxIdx_{id} + 1;");
        let _ = writeln!(o, "  if (capCb_{id} > historyLen + 1) {{\n    return RetCode.INTERNAL_ERROR;\n  }}");
    }
}

/// Every handle field.
fn emit_capture_stores(
    o: &mut String,
    em: &Em,
    model: &StreamModel,
    scalars: &[(String, VarType)],
    step_settings: &BTreeSet<String>,
    ctx: &TsCtx,
) {
    let func = em.func;
    for p in &func.optional_inputs {
        let _ = writeln!(o, "  sp.{0} = {0};", p.name);
    }
    for (name, _) in &func.private_extra_params {
        let _ = writeln!(o, "  sp.{name} = {name};");
    }
    for (name, _) in scalars {
        if model.parity.as_ref().is_some_and(|p| &p.field == name) {
            let _ = writeln!(o, "  sp.{name} = historyLen % 2;");
        } else if ctx.address_of_vars.contains(name) {
            let _ = writeln!(o, "  sp.{name} = {name}.value;");
        } else if ctx.double_address_of_vars.contains(name) {
            let _ = writeln!(o, "  sp.{name} = {name}[0];");
        } else {
            let _ = writeln!(o, "  sp.{name} = {name};");
        }
    }
    for name in &model.out_feedback {
        let _ = writeln!(o, "  sp.lastOut_{name} = {name}[(outNBElement.value - 1) * outStride];");
    }
    for lag in &model.lags {
        for k in 1..=lag.depth {
            let _ = writeln!(o, "  sp.{} = {}[historyLen - {k}];", StreamModel::lag_field(&lag.array, k), lag.array);
        }
    }
    for ring in model.rings() {
        let v = &ring.var;
        if ring.back > 0 {
            let _ = writeln!(o, "  sp.ringPos_{v} = historyLen % cap_{v};");
            let _ = writeln!(o, "  sp.ringCap_{v} = cap_{v};");
            let _ = writeln!(o, "  sp.ringLag_{v} = capLag_{v};");
        } else {
            let _ = writeln!(o, "  sp.ringPos_{v} = 0;");
            let _ = writeln!(o, "  sp.ringCap_{v} = cap_{v};");
        }
        for arr in &ring.arrays {
            let _ = writeln!(o, "  sp.ring_{v}_{arr} = capRing_{v}_{arr};");
        }
    }
    for win in model.windows() {
        let v = &win.var;
        let _ = writeln!(o, "  sp.winPos_{v} = 0;");
        let _ = writeln!(o, "  sp.winCap_{v} = cap_{v};");
        for arr in &win.arrays {
            let _ = writeln!(o, "  sp.win_{v}_{arr} = capWin_{v}_{arr};");
        }
    }
    for circ in model.circs() {
        let id = &circ.id;
        let _ = writeln!(o, "  sp.cbSize_{id} = capCb_{id};");
        for (storage, _) in streaming::circ_storages(circ) {
            let _ = writeln!(o, "  sp.cb_{storage} = {storage};");
        }
    }
    if let Some(ex) = model.extrema() {
        let _ = writeln!(o, "  sp.xMask = physX - 1;");
        for arr in &ex.arrays {
            let _ = writeln!(o, "  sp.x_{arr} = capX_{arr};");
        }
    }
    for s in step_settings {
        let _ = writeln!(o, "  sp.cs_{s}_rangeType = {s}_rangeType;");
        let _ = writeln!(o, "  sp.cs_{s}_avgPeriod = {s}_avgPeriod;");
        let _ = writeln!(o, "  sp.cs_{s}_factor = {s}_factor;");
    }
}

fn emit_cur_capture(o: &mut String, outputs: &[String], source: CurSource) {
    for out in outputs {
        let expr = match source {
            CurSource::StridedArray => format!("{out}[(outNBElement.value - 1) * outStride]"),
            CurSource::Scratch => format!("sc_{out}[outNBElement.value - 1]"),
        };
        let _ = writeln!(o, "  sp.cur_{out} = {expr};");
    }
}

// ---------------------------------------------------------------------------
// Public and @internal wrappers
// ---------------------------------------------------------------------------

/// Rule rS7 with its counts, then `streamFailure` for every other code.
fn emit_reject_conversion(o: &mut String, em: &Em, what: &str, anchor: Option<&str>) {
    let n = em.name();
    let history = &streaming::input_array_names(em.func)[0];
    let anchor = anchor.unwrap_or("0");
    let _ = writeln!(o, "  if (retCode === RetCode.SUCCESS) {{\n    return sp;\n  }}");
    let _ = writeln!(
        o,
        "  if (retCode === RetCode.INSUFFICIENT_HISTORY) {{\n    throw insufficientHistory('{n} {what}', {history}.length, {anchor}, {});\n  }}",
        em.lookback()
    );
    let _ = writeln!(o, "  throw streamFailure('{n} {what}', retCode);");
}

fn internal_in_sig(func: &FuncDef) -> Vec<String> {
    streaming::input_array_names(func).iter().map(|i| format!("{i}: Float64Array")).collect()
}

fn emit_open_internal_seam(o: &mut String, em: &Em, merged: bool) {
    let func = em.func;
    let base = em.base();
    let class = em.class();
    let in_fwd = streaming::input_array_names(func);
    let mut sig = vec!["core: Core".to_string()];
    sig.extend(internal_in_sig(func));
    sig.push("startIdx: number".to_string());
    sig.extend(opt_sig(func));
    let _ = writeln!(o, "\n/** @internal */\nexport function {base}OpenInternal(\n  {},\n): {class} {{", sig.join(",\n  "));
    let _ = writeln!(o, "  const sp = {class}.of(core);");
    let mut args: Vec<String> = vec!["sp".to_string()];
    args.extend(in_fwd.iter().cloned());
    args.push("startIdx".to_string());
    args.extend(opt_fwd(func));
    if merged {
        let _ = writeln!(o, "  const outBegIdx = new MInteger();\n  const outNBElement = new MInteger();");
        args.push("outBegIdx".to_string());
        args.push("outNBElement".to_string());
        for out in &func.outputs {
            let _ = writeln!(o, "  const sink_{} = new {}(1);", out.name, arr_ty(out_ctype(func, &out.name)));
            args.push(format!("sink_{}", out.name));
        }
        args.push("0".to_string());
        let _ = writeln!(o, "  const retCode = {base}OpenImpl({});", args.join(", "));
        let _ = writeln!(o, "  sp.outRangeBegIdx = outBegIdx.value;\n  sp.outRangeCount = outNBElement.value;");
    } else {
        let _ = writeln!(o, "  const retCode = {base}OpenImpl({});", args.join(", "));
    }
    emit_reject_conversion(o, em, "open", Some("startIdx"));
    let _ = writeln!(o, "}}");
}

/// The public opener's checks, in Java's order with the type tests first
/// (D13): rS1/rS2 on the history, rS3 through the fill width, rS5's input
/// half, then each output's capacity.
fn emit_public_open_guards(o: &mut String, em: &Em, verb: &str, with_outputs: bool) {
    let func = em.func;
    let n = em.name();
    let inputs = streaming::input_array_names(func);
    let history = &inputs[0];
    o.push_str(&ts::require_arg_types(func, &format!("{n} {verb}")));
    let _ = writeln!(o, "  requireArgument('{n} {verb}', '{history}', {history});");
    let _ = writeln!(o, "  requireHistory('{n} {verb}', {history}.length);");
    for input in &inputs[1..] {
        let _ = writeln!(o, "  requireArgument('{n} {verb}', '{input}', {input});");
    }
    if with_outputs {
        let _ = writeln!(o, "  const guardOutLen = openFillCount('{n} {verb}', {history}.length, {});", em.lookback());
    }
    for input in &inputs[1..] {
        let _ = writeln!(o, "  requireHistoryLength('{n} {verb}', '{input}', {input}.length, {history}.length);");
    }
    if with_outputs {
        for out in &func.outputs {
            let _ = writeln!(o, "  requireLength('{n} {verb}', '{0}', {0}, guardOutLen);", out.name);
        }
    }
}

fn public_sig(func: &FuncDef, with_outputs: bool) -> Vec<String> {
    let mut sig: Vec<String> =
        streaming::input_array_names(func).iter().map(|i| format!("{i}: ArrayLike<number>")).collect();
    for p in &func.optional_inputs {
        sig.push(format!("{}: {}", p.name, ts::opt_ts_type(p)));
    }
    if with_outputs {
        for out in &func.outputs {
            sig.push(format!("{}: {}", out.name, arr_ty(out_ctype(func, &out.name))));
        }
    }
    sig.push("core: Core = Core.DEFAULT".to_string());
    sig
}

fn emit_open_wrappers(o: &mut String, em: &Em, merged: bool) {
    let func = em.func;
    let base = em.base();
    let class = em.class();
    let inputs = streaming::input_array_names(func);
    emit_open_internal_seam(o, em, merged);

    let _ = write!(o, "\n{}", ts_doc::open_docs(func, em.svc.enums));
    let _ = writeln!(o, "export function {base}Open(\n  {},\n): {class} {{", public_sig(func, false).join(",\n  "));
    emit_public_open_guards(o, em, "open", false);
    let conv: Vec<String> = inputs.iter().map(|i| format!("float64({i})")).collect();
    let _ = writeln!(o, "  return {base}OpenInternal(core, {}, 0{});", conv.join(", "), comma(&opt_fwd(func)));
    let _ = writeln!(o, "}}");

    let _ = write!(o, "\n{}", ts_doc::open_and_fill_docs(func, em.svc.enums));
    let _ = writeln!(o, "export function {base}OpenAndFill(\n  {},\n): {class} {{", public_sig(func, true).join(",\n  "));
    emit_public_open_guards(o, em, "openAndFill", true);
    for i in &inputs {
        let _ = writeln!(o, "  const {i}F64 = float64({i});");
    }
    let mut args: Vec<String> = inputs.iter().map(|i| format!("{i}F64")).collect();
    if merged {
        if let Some((same, partial)) = alias_conditions(func, &|i| format!("{i}F64")) {
            for cond in [same, format!("guardOutLen > 0 && ({partial})")] {
                let _ = writeln!(o, "  if ({cond}) {{\n    throw streamFailure('{} openAndFill', RetCode.BAD_PARAM);\n  }}", em.name());
            }
        }
        args.push("0".to_string());
        args.extend(opt_fwd(func));
        args.push("new MInteger()".to_string());
        args.push("new MInteger()".to_string());
        args.extend(func.outputs.iter().map(|out| out.name.clone()));
        let _ = writeln!(o, "  return {base}OpenAndFillInternal(core, {});", args.join(", "));
    } else {
        let _ = writeln!(o, "  const sp = {class}.of(core);");
        let _ = writeln!(o, "  const outBegIdx = new MInteger();\n  const outNBElement = new MInteger();");
        let mut fwd: Vec<String> = vec!["sp".to_string()];
        fwd.extend(inputs.iter().map(|i| format!("{i}F64")));
        fwd.extend(opt_fwd(func));
        fwd.push("outBegIdx".to_string());
        fwd.push("outNBElement".to_string());
        fwd.extend(func.outputs.iter().map(|out| out.name.clone()));
        let _ = writeln!(o, "  const retCode = {base}OpenAndFillImpl({});", fwd.join(", "));
        let _ = writeln!(o, "  sp.outRangeBegIdx = outBegIdx.value;\n  sp.outRangeCount = outNBElement.value;");
        emit_reject_conversion(o, em, "openAndFill", None);
    }
    let _ = writeln!(o, "}}");
}

fn emit_open_and_fill_internal_wrapper(o: &mut String, em: &Em, merged: bool) {
    let func = em.func;
    let base = em.base();
    let class = em.class();
    let mut sig = vec!["core: Core".to_string()];
    sig.extend(internal_in_sig(func));
    sig.push("startIdx: number".to_string());
    sig.extend(opt_sig(func));
    sig.push("outBegIdx: MInteger".to_string());
    sig.push("outNBElement: MInteger".to_string());
    for out in &func.outputs {
        sig.push(format!("{}: {}", out.name, arr_ty(out_ctype(func, &out.name))));
    }
    let _ = writeln!(o, "\n/** @internal */\nexport function {base}OpenAndFillInternal(\n  {},\n): {class} {{", sig.join(",\n  "));
    let _ = writeln!(o, "  const sp = {class}.of(core);");
    let mut args: Vec<String> = vec!["sp".to_string()];
    args.extend(streaming::input_array_names(func));
    args.push("startIdx".to_string());
    args.extend(opt_fwd(func));
    args.push("outBegIdx".to_string());
    args.push("outNBElement".to_string());
    args.extend(func.outputs.iter().map(|out| out.name.clone()));
    if merged {
        args.push("1".to_string());
        let _ = writeln!(o, "  const retCode = {base}OpenImpl({});", args.join(", "));
    } else {
        let _ = writeln!(o, "  const retCode = {base}OpenAndFillInternalImpl({});", args.join(", "));
    }
    let _ = writeln!(o, "  sp.outRangeBegIdx = outBegIdx.value;\n  sp.outRangeCount = outNBElement.value;");
    emit_reject_conversion(o, em, "openAndFill", Some("startIdx"));
    let _ = writeln!(o, "}}");
}

// ---------------------------------------------------------------------------
// Dual-mode tier: two parameter-selected arms over one union handle.
// ---------------------------------------------------------------------------

fn dual_union_fields(func: &FuncDef, fields_a: &[Field], fields_b: &[Field]) -> Vec<Field> {
    let mut fields: Vec<Field> = fields_a.to_vec();
    let a_types: HashMap<&String, CType> = fields_a.iter().map(|f| (&f.name, f.ty)).collect();
    for f in fields_b {
        if let Some(prev) = a_types.get(&f.name) {
            assert!(*prev == f.ty, "{}: dual-mode field `{}` has conflicting types across modes", func.name, f.name);
        } else {
            fields.push(f.clone());
        }
    }
    fields
}

#[allow(clippy::too_many_lines)]
fn emit_dual_mode(o: &mut String, em: &Em, dmp: &streaming::DualModePlan) {
    let func = em.func;
    let ma = &dmp.mode_a;
    let mb = &dmp.mode_b;
    let mut step_settings = detect_candle_settings(&ma.steady_stmts);
    step_settings.extend(detect_candle_settings(&mb.steady_stmts));
    let union_scalars = dual_scalar_union(func, ma, mb);
    let fields_a = state_fields_from(func, ma, &union_scalars, &step_settings);
    let fields_b = state_fields_from(func, mb, &union_scalars, &step_settings);
    let fields = dual_union_fields(func, &fields_a, &fields_b);

    let outs: BTreeSet<String> = func.outputs.iter().map(|o| format!("cur_{}", o.name)).collect();
    let dual_frame = peek_frame(em, ma, &TsStreamNames, &fields, &step_settings, 6, &outs)
        .zip(peek_frame(em, mb, &TsStreamNames, &fields, &step_settings, 6, &outs))
        .map(|(a, b)| {
            let mut f = String::new();
            for out in &func.outputs {
                let _ = writeln!(f, "    let cur_{} = 0;", out.name);
            }
            let mut ctx = em.frame_ctx(&[], &fields, Some(ma), &step_settings, false);
            if !has_out_class(func) {
                ctx.ret = out_ctype(func, &func.outputs[0].name);
            }
            if let Some(ident) = identity_branch_as_frame(func, ma) {
                f.push_str(&em.render(&ctx, &ident, 4));
            }
            let pred = ctx.cond(&params_on_state(func, &dmp.predicate));
            let _ = writeln!(f, "    if ({pred}) {{");
            f.push_str(&a);
            let _ = writeln!(f, "    }} else {{");
            f.push_str(&b);
            let _ = writeln!(f, "    }}");
            PeekFrame::falls_through(f)
        });
    emit_handle_class(o, em, &fields, &Members::default(), dual_frame.as_ref());

    emit_step_sig(o, em);
    {
        let ctx = em.frame_ctx(&[], &fields, Some(ma), &step_settings, false);
        if let Some(s) = streaming::identity_step_branch(ma, &TsStreamNames) {
            o.push_str(&em.render(&ctx, std::slice::from_ref(&s), 2));
        }
        let pred = ctx.cond(&params_on_state(func, &dmp.predicate));
        let _ = writeln!(o, "  if ({pred}) {{");
    }
    emit_step_body(o, em, ma, &fields, &step_settings, 4, None);
    let _ = writeln!(o, "  }} else {{");
    emit_step_body(o, em, mb, &fields, &step_settings, 4, None);
    let _ = writeln!(o, "  }}\n}}");

    emit_open_body_sig(o, em, OutMode::Core);
    let mut s = String::new();
    emit_open_head(&mut s, em);
    emit_open_validation(&mut s, em, OutMode::Core);
    {
        // The identity path is one property of the function; a context over
        // the prologue serves it and the predicate.
        let head_ctx = em.open_ctx(dmp.prologue);
        emit_identity_fast_path(&mut s, em, ma, &fields, &head_ctx);
        let pred = head_ctx.cond(&dmp.predicate);
        let _ = writeln!(s, "  if ({pred}) {{");
    }
    for (k, arm) in [ma, mb].into_iter().enumerate() {
        if k == 1 {
            let _ = writeln!(s, "  }} else {{");
        }
        let mut body: Vec<Statement> = dmp.prologue.to_vec();
        body.extend_from_slice(arm.body);
        body.extend_from_slice(dmp.epilogue);
        let open_body = cleanup_open_body(&build_open_body_java(arm, &body), em);
        let ctx = em.open_ctx(&open_body);
        let mut a = String::new();
        emit_body_decls(&mut a, &open_body, &ctx);
        emit_extras_and_candle(&mut a, em, &open_body, &ctx);
        emit_open_region(&mut a, em, &open_body, &ctx, &[], &BTreeSet::new());
        let (own, other) = if k == 0 { (&fields_a, &fields_b) } else { (&fields_b, &fields_a) };
        let own_names: BTreeSet<&String> = own.iter().map(|f| &f.name).collect();
        let mut complement = String::new();
        for f in other {
            if !own_names.contains(&f.name) && f.ty.is_pointer() {
                let _ = writeln!(complement, "  sp.{} = {};", f.name, f.default);
            }
        }
        emit_capture(
            &mut a,
            em,
            arm,
            &union_scalars,
            &step_settings,
            &ctx,
            Some(CurSource::StridedArray),
            &complement,
        );
        let _ = writeln!(a, "  return RetCode.SUCCESS;");
        s.push_str(&indent_block(&a, 2));
    }
    let _ = writeln!(s, "  }}");
    o.push_str(&with_core(&s));
    let _ = writeln!(o, "}}");
    emit_open_and_fill_internal_wrapper(o, em, true);
    emit_open_wrappers(o, em, true);
    if em.svc.registry.in_tape_set(&func.name.to_lowercase()) {
        let tier = TapeTier { models: &[ma, mb], dual: Some(dmp), fields: &fields, step_settings: &step_settings };
        emit_tape_entries(o, em, &tier);
    }
}

// ---------------------------------------------------------------------------
// Dispatch tier (MA): a sub-stream picked by the stored enum parameter.
// ---------------------------------------------------------------------------

fn callee_names(em: &Em, callee: &str) -> ts::TsNames {
    ts_names(&em.svc.registry.name_of(callee))
}

fn callee_value_field(em: &Em, callee: &str, slot: usize) -> String {
    value_field_name(&em.svc.registry.callee_outputs(callee)[slot])
}

/// The callee's stream class, imported.
fn callee_class(em: &Em, callee: &str) -> String {
    let c = callee_names(em, callee).stream_class;
    em.svc.import(callee, &c);
    c
}

/// A module-level sink a multi-output sub-stream writes, allocated once.
fn callee_scratch(em: &Em, callee: &str, scratch: &mut BTreeSet<String>) -> String {
    let ocls = callee_names(em, callee).out_class;
    em.svc.import(callee, &ocls);
    scratch.insert(ocls.clone());
    format!("(scratch{ocls} ??= new {ocls}())")
}

fn emit_scratch_decls(o: &mut String, scratch: &BTreeSet<String>) {
    for ocls in scratch {
        let _ = writeln!(o, "\nlet scratch{ocls}: {ocls} | null = null;");
    }
}

fn arm_tag(dp: &streaming::DispatchPlan, arm: &streaming::DispatchArm) -> usize {
    dp.arms.iter().filter(|a| a.supported).position(|a| a.label == arm.label).expect("a supported arm")
}

const IDENTITY_TAG: &str = "-1";

fn dispatch_store_sub(o: &mut String, em: &Em, arm: &streaming::DispatchArm, outputs: &[String], pad: &str, tag: usize) {
    let _ = writeln!(o, "{pad}sp.sub = sub;");
    let _ = writeln!(o, "{pad}sp.arm = {tag};");
    for (i, slot) in arm.out_map.iter().enumerate() {
        if let streaming::OutSlot::Forward(k) = slot {
            let _ = writeln!(o, "{pad}sp.cur_{} = sub.cur_{};", outputs[*k], em.svc.registry.callee_outputs(&arm.callee)[i]);
        }
    }
}

/// The callee's optional arguments, each converted to its slot.
fn callee_opts(em: &Em, callee: &str, args: &[Expr], ctx: &TsCtx) -> Vec<String> {
    let def = em.svc.registry.def(callee).unwrap_or_else(|| panic!("no definition for `{callee}`"));
    def.optional_inputs.iter().zip(args).map(|(o, a)| ts::opt_arg(o, a, ctx)).collect()
}

fn emit_dispatch(o: &mut String, em: &Em, dp: &streaming::DispatchPlan) {
    let func = em.func;
    let fields = base_fields(func);
    let ctx = em.frame_ctx(&[], &fields, None, &BTreeSet::new(), false);
    let mut scratch = BTreeSet::new();
    let arms: Vec<&streaming::DispatchArm> = dp.arms.iter().filter(|a| a.supported).collect();
    let classes: Vec<String> = arms.iter().map(|a| callee_class(em, &a.callee)).collect();
    let mut union: Vec<String> = classes.clone();
    union.dedup();

    let members = Members {
        decls: format!(
            "  /** @internal */\n  sub: {} | null = null;\n  /** @internal */\n  arm: number = 0;\n",
            union.join(" | ")
        ),
        copy: "    c.sub = this.sub === null ? null : this.sub.clone();\n    c.arm = this.arm;\n".to_string(),
    };

    let frame = dispatch_peek_frame(em, dp, &arms, &classes, &mut scratch);
    emit_handle_class(o, em, &fields, &members, Some(&frame));

    emit_dispatch_step(o, em, dp, &arms, &classes, &mut scratch);

    for mode in [OutMode::Scalar, OutMode::Fill, OutMode::FillInternal] {
        emit_dispatch_open(o, em, dp, mode, &ctx);
    }

    emit_open_wrappers(o, em, false);
    emit_open_and_fill_internal_wrapper(o, em, false);
    if em.svc.registry.in_tape_set(&func.name.to_lowercase()) {
        emit_dispatch_tape(o, em, dp, &mut scratch);
    }
    emit_scratch_decls(o, &scratch);
}

fn dispatch_peek_frame(
    em: &Em,
    dp: &streaming::DispatchPlan,
    arms: &[&streaming::DispatchArm],
    classes: &[String],
    scratch: &mut BTreeSet<String>,
) -> PeekFrame {
    let func = em.func;
    let outputs: Vec<String> = func.outputs.iter().map(|x| x.name.clone()).collect();
    let bar_args = streaming::input_array_names(func).join(", ");
    let terminal = func.outputs.len() == 1;
    let mut f = String::new();
    if !terminal {
        for out in &func.outputs {
            let _ = writeln!(f, "    let cur_{} = 0;", out.name);
        }
    }
    let _ = writeln!(f, "    const sub = sp.sub;\n    switch (sp.arm) {{");
    if let Some(idp) = &dp.identity {
        let _ = writeln!(f, "      case {IDENTITY_TAG}:");
        if terminal {
            let _ = writeln!(f, "        return {};", idp.pairs[0].1);
        } else {
            for (out, inp) in &idp.pairs {
                let _ = writeln!(f, "        cur_{out} = {inp};");
            }
            let _ = writeln!(f, "        break;");
        }
    }
    for (arm, cls) in arms.iter().zip(classes) {
        let label = arm_tag(dp, arm);
        let _ = writeln!(f, "      case {label}: {{");
        let mut returned = false;
        if arm.out_map.len() == 1 {
            let streaming::OutSlot::Forward(k) = arm.out_map[0] else {
                panic!("single-output arm cannot discard its only slot")
            };
            let call = format!("(sub as {cls}).peek({bar_args})");
            if terminal {
                let _ = writeln!(f, "        return {call};");
                returned = true;
            } else {
                let _ = writeln!(f, "        cur_{} = {call};", outputs[k]);
            }
        } else {
            let sink = callee_scratch(em, &arm.callee, scratch);
            let _ = writeln!(f, "        const subValue = {sink};\n        (sub as {cls}).peek({bar_args}, subValue);");
            for (i, slot) in arm.out_map.iter().enumerate() {
                if let streaming::OutSlot::Forward(k) = slot {
                    let fld = callee_value_field(em, &arm.callee, i);
                    if terminal {
                        let _ = writeln!(f, "        return subValue.{fld};");
                        returned = true;
                    } else {
                        let _ = writeln!(f, "        cur_{} = subValue.{fld};", outputs[*k]);
                    }
                }
            }
        }
        if terminal {
            assert!(returned, "dispatch arm {label} forwards no output");
        } else {
            let _ = writeln!(f, "        break;");
        }
        let _ = writeln!(f, "      }}");
    }
    let _ = writeln!(f, "      default:\n        throw new Error('unreachable: open rejects arms without a sub-stream');\n    }}");
    PeekFrame { body: f, terminal }
}

fn emit_dispatch_step(
    o: &mut String,
    em: &Em,
    dp: &streaming::DispatchPlan,
    arms: &[&streaming::DispatchArm],
    classes: &[String],
    scratch: &mut BTreeSet<String>,
) {
    let func = em.func;
    let outputs: Vec<String> = func.outputs.iter().map(|x| x.name.clone()).collect();
    let bar_args = streaming::input_array_names(func).join(", ");
    emit_step_sig(o, em);
    let _ = writeln!(o, "  const sub = sp.sub;\n  switch (sp.arm) {{");
    if let Some(idp) = &dp.identity {
        let _ = writeln!(o, "    case {IDENTITY_TAG}:");
        for (out, inp) in &idp.pairs {
            let _ = writeln!(o, "      sp.cur_{out} = {inp};");
        }
        let _ = writeln!(o, "      return;");
    }
    for (arm, cls) in arms.iter().zip(classes) {
        let label = arm_tag(dp, arm);
        let _ = writeln!(o, "    case {label}: {{");
        if arm.out_map.len() == 1 {
            let streaming::OutSlot::Forward(k) = arm.out_map[0] else {
                panic!("single-output arm cannot discard its only slot");
            };
            let _ = writeln!(o, "      sp.cur_{} = (sub as {cls}).update({bar_args});", outputs[k]);
        } else {
            let sink = callee_scratch(em, &arm.callee, scratch);
            let _ = writeln!(o, "      const subOut = {sink};\n      (sub as {cls}).update({bar_args}, subOut);");
            for (i, slot) in arm.out_map.iter().enumerate() {
                if let streaming::OutSlot::Forward(k) = slot {
                    let _ = writeln!(o, "      sp.cur_{} = subOut.{};", outputs[*k], callee_value_field(em, &arm.callee, i));
                }
            }
        }
        let _ = writeln!(o, "      return;\n    }}");
    }
    let _ = writeln!(o, "    default:\n      return;\n  }}\n}}");
}

#[allow(clippy::too_many_lines)]
fn emit_dispatch_open(o: &mut String, em: &Em, dp: &streaming::DispatchPlan, mode: OutMode, ctx: &TsCtx) {
    let func = em.func;
    let outputs: Vec<String> = func.outputs.iter().map(|x| x.name.clone()).collect();
    let inputs = streaming::input_array_names(func);
    let bar_args = inputs.join(", ");
    let lb_call = em.lookback();
    emit_open_body_sig(o, em, mode);
    let mut s = String::new();
    let _ = writeln!(s, "  const historyLen = {}.length;", inputs[0]);
    emit_open_validation(&mut s, em, mode);
    let _ = writeln!(s, "  if (historyLen < {lb_call} + 1) {{\n    return RetCode.INSUFFICIENT_HISTORY;\n  }}");
    if let Some(idp) = &dp.identity {
        let _ = writeln!(s, "  if ({}) {{", ctx.cond(&idp.condition));
        let _ = writeln!(s, "    if (historyLen < {lb_call} + 1) {{\n      return RetCode.INSUFFICIENT_HISTORY;\n    }}");
        for p in &func.optional_inputs {
            let _ = writeln!(s, "    sp.{0} = {0};", p.name);
        }
        let _ = writeln!(s, "    sp.sub = null;\n    sp.arm = {IDENTITY_TAG};");
        match mode {
            OutMode::Core => unreachable!("dispatch tier is exempt from the merge"),
            OutMode::Scalar => {
                for (out, inp) in &idp.pairs {
                    let _ = writeln!(s, "    sp.cur_{out} = {inp}[historyLen - 1];");
                }
                let _ = writeln!(s, "    let fillLb = {lb_call};");
                let _ = writeln!(s, "    if (startIdx > fillLb) {{\n      fillLb = startIdx;\n    }}");
                let _ = writeln!(s, "    if (historyLen < fillLb + 1) {{\n      return RetCode.INSUFFICIENT_HISTORY;\n    }}");
                let _ = writeln!(s, "    sp.outRangeBegIdx = fillLb;\n    sp.outRangeCount = historyLen - fillLb;");
            }
            OutMode::Fill | OutMode::FillInternal => {
                let _ = writeln!(s, "    let fillLb = {lb_call};");
                if mode == OutMode::FillInternal {
                    let _ = writeln!(s, "    if (startIdx > fillLb) {{\n      fillLb = startIdx;\n    }}");
                    let _ = writeln!(s, "    if (historyLen < fillLb + 1) {{\n      return RetCode.INSUFFICIENT_HISTORY;\n    }}");
                }
                let _ = writeln!(s, "    outBegIdx.value = fillLb;\n    outNBElement.value = historyLen - fillLb;");
                let _ = writeln!(s, "    for (let fillIdx = 0; fillIdx < historyLen - fillLb; fillIdx++) {{");
                for (out, inp) in &idp.pairs {
                    let _ = writeln!(s, "      {out}[fillIdx] = {inp}[fillLb + fillIdx];");
                }
                let _ = writeln!(s, "    }}");
                for (out, _) in &idp.pairs {
                    let _ = writeln!(s, "    sp.cur_{out} = {out}[outNBElement.value - 1];");
                }
            }
        }
        let _ = writeln!(s, "    return RetCode.SUCCESS;\n  }}");
    }
    let _ = writeln!(s, "  switch ({}) {{", dp.param);
    for arm in &dp.arms {
        let label = ts::render_switch_label(&arm.label, em.svc.enums);
        if !arm.supported {
            let what = if arm.callee.is_empty() { "delegation" } else { arm.callee.as_str() };
            let _ = writeln!(s, "    case {label}:\n      return RetCode.BAD_PARAM; /* no {what} stream */");
            continue;
        }
        let cn = callee_names(em, &arm.callee);
        let opts = callee_opts(em, &arm.callee, &arm.opt_args, ctx);
        let _ = writeln!(s, "    case {label}: {{");
        match mode {
            OutMode::Core => unreachable!("dispatch tier is exempt from the merge"),
            OutMode::Scalar => {
                em.svc.import(&arm.callee, &cn.open_internal);
                let _ = writeln!(s, "      const sub = {}(core, {bar_args}, startIdx{});", cn.open_internal, comma(&opts));
                let _ = writeln!(s, "      sp.outRangeBegIdx = sub.outRangeBegIdx;\n      sp.outRangeCount = sub.outRangeCount;");
            }
            OutMode::Fill | OutMode::FillInternal => {
                let callee_outs = em.svc.registry.def(&arm.callee).map(|d| &d.outputs);
                let fill_outs: Vec<String> = arm
                    .out_map
                    .iter()
                    .enumerate()
                    .map(|(slot_idx, slot)| match slot {
                        streaming::OutSlot::Forward(k) => outputs[*k].clone(),
                        streaming::OutSlot::Discard => {
                            let ty = match callee_outs.and_then(|o| o.get(slot_idx)).map(|o| &o.param_type) {
                                Some(ParamType::Integer) => "Int32Array",
                                _ => "Float64Array",
                            };
                            format!("new {ty}(historyLen)")
                        }
                    })
                    .collect();
                if mode == OutMode::FillInternal {
                    em.svc.import(&arm.callee, &cn.open_and_fill_internal);
                    let _ = writeln!(
                        s,
                        "      const sub = {}(core, {bar_args}, startIdx{}, outBegIdx, outNBElement, {});",
                        cn.open_and_fill_internal,
                        comma(&opts),
                        fill_outs.join(", ")
                    );
                } else {
                    em.svc.import(&arm.callee, &cn.open_and_fill);
                    let _ = writeln!(
                        s,
                        "      const sub = {}({bar_args}{}, {}, core);",
                        cn.open_and_fill,
                        comma(&opts),
                        fill_outs.join(", ")
                    );
                    let _ = writeln!(s, "      outBegIdx.value = sub.outRangeBegIdx;\n      outNBElement.value = sub.outRangeCount;");
                }
            }
        }
        dispatch_store_sub(&mut s, em, arm, &outputs, "      ", arm_tag(dp, arm));
        let _ = writeln!(s, "      break;\n    }}");
    }
    let _ = writeln!(s, "    default:\n      return RetCode.BAD_PARAM;\n  }}");
    for p in &func.optional_inputs {
        let _ = writeln!(s, "  sp.{0} = {0};", p.name);
    }
    let _ = writeln!(s, "  return RetCode.SUCCESS;");
    o.push_str(&with_core(&s));
    let _ = writeln!(o, "}}");
}

#[allow(clippy::too_many_lines)]
fn emit_dispatch_tape(o: &mut String, em: &Em, dp: &streaming::DispatchPlan, scratch: &mut BTreeSet<String>) {
    let func = em.func;
    assert_single_output(func, "a dispatcher's tape entries");
    let base = em.base();
    let class = em.class();
    let (sig_bars, bar_args) = bar_params(func);
    let out = &func.outputs[0].name;
    let identity = dp.identity.as_ref().map(|idp| idp.pairs[0].1.clone());
    let arms: Vec<_> = dp.arms.iter().filter(|a| a.supported && !a.callee.is_empty()).collect();
    let tape_args = format!("tape, tapeBase, tapeMask, {bar_args}");
    let params = format!("sp: {class}, {TAPE_PARAMS}, {sig_bars}");
    let mut arm_value = |arm: &streaming::DispatchArm, verb: &str, var: &str| -> (String, String) {
        let cb = common::camel_words(&em.svc.registry.name_of(&arm.callee));
        let cls = callee_class(em, &arm.callee);
        let fname = format!("{cb}{verb}Tape");
        em.svc.import(&arm.callee, &fname);
        let call = format!("{fname}(sp.sub as {cls}, {tape_args}");
        if arm.out_map.len() == 1 {
            return (String::new(), format!("{call})"));
        }
        let slot = arm
            .out_map
            .iter()
            .position(|s| matches!(s, streaming::OutSlot::Forward(_)))
            .unwrap_or_else(|| panic!("dispatch arm {} forwards no output", arm.label));
        if verb == "Step" {
            let o = &em.svc.registry.callee_outputs(&arm.callee)[slot];
            return (format!("const sub = sp.sub as {cls};\n{fname}(sub, {tape_args});\n"), format!("sub.cur_{o}"));
        }
        let sink = callee_scratch(em, &arm.callee, scratch);
        let fld = callee_value_field(em, &arm.callee, slot);
        (format!("const {var} = {sink};\n{call}, {var});\n"), format!("{var}.{fld}"))
    };
    let mut cases = |verb: &str, var: &str, tail: &dyn Fn(&str) -> String| -> String {
        let mut s = String::new();
        for arm in &arms {
            let (pre, value) = arm_value(arm, verb, var);
            let _ = writeln!(s, "    case {}: {{", arm_tag(dp, arm));
            for l in pre.lines().chain(tail(&value).lines()) {
                let _ = writeln!(s, "      {l}");
            }
            let _ = writeln!(s, "    }}");
        }
        s
    };

    let step_cases = cases("Step", "subOut", &|v| format!("sp.cur_{out} = {v};\nbreak;"));
    let peek_cases = cases("Peek", "subValue", &|v| format!("return {v};"));

    let _ = writeln!(o, "\n/** @internal */\nexport function {base}StepTape({params}): number {{");
    let _ = writeln!(o, "  switch (sp.arm) {{");
    if let Some(inp) = &identity {
        let _ = writeln!(o, "    case {IDENTITY_TAG}:\n      sp.cur_{out} = {inp};\n      break;");
    }
    o.push_str(&step_cases);
    let _ = writeln!(o, "    default:\n      break;\n  }}");
    let _ = writeln!(o, "  sp.outRangeCount++;\n  return sp.cur_{out};\n}}");

    let _ = writeln!(o, "\n/** @internal */\nexport function {base}PeekTape({params}): number {{");
    let _ = writeln!(o, "  switch (sp.arm) {{");
    if let Some(inp) = &identity {
        let _ = writeln!(o, "    case {IDENTITY_TAG}:\n      return {inp};");
    }
    o.push_str(&peek_cases);
    let _ = writeln!(o, "    default:\n      throw new Error('unreachable: open rejects arms without a sub-stream');\n  }}\n}}");

    let _ = writeln!(o, "\n/** @internal */\nexport function {base}TapeDetach(sp: {class}): number {{");
    let _ = writeln!(o, "  switch (sp.arm) {{");
    for arm in &arms {
        let cb = common::camel_words(&em.svc.registry.name_of(&arm.callee));
        let cls = callee_class(em, &arm.callee);
        let fname = format!("{cb}TapeDetach");
        em.svc.import(&arm.callee, &fname);
        let _ = writeln!(o, "    case {}:\n      return {fname}(sp.sub as {cls});", arm_tag(dp, arm));
    }
    let _ = writeln!(o, "    default:\n      return 0;\n  }}\n}}");
}

// ---------------------------------------------------------------------------
// Period-bank tier (MAVP): one sub-stream per period, advanced in lockstep,
// every slot reading its price history from one tape the bank owns.
// ---------------------------------------------------------------------------

#[allow(clippy::too_many_lines)]
fn emit_period_bank(o: &mut String, em: &Em, plan: &streaming::PeriodBankPlan) {
    let func = em.func;
    let registry = em.svc.registry;
    let callee = plan.callee.as_str();
    let cn = callee_names(em, callee);
    let callee_camel = common::camel_words(&registry.name_of(callee));
    let subty = callee_class(em, callee);
    let callee_out0 = registry.callee_outputs(callee)[0].clone();
    let base = em.base();
    let class = em.class();
    let min = plan.min_param.as_str();
    let max = plan.max_param.as_str();
    let price = plan.price_input.as_str();
    let period = plan.period_input.as_str();
    let out = plan.output.as_str();
    let step_tape = format!("{callee_camel}StepTape");
    let peek_tape = format!("{callee_camel}PeekTape");
    let detach = format!("{callee_camel}TapeDetach");
    for sym in [&cn.lookback, &cn.open_internal, &step_tape, &peek_tape, &detach] {
        em.svc.import(callee, sym);
    }
    let callee_args = |per: &str, ty: &str| -> String {
        plan.callee_opts
            .iter()
            .map(|a| match a {
                streaming::PeriodBankArg::Period => per.to_string(),
                streaming::PeriodBankArg::MAType => format!("{ty} as MAType"),
            })
            .collect::<Vec<_>>()
            .join(", ")
    };
    let matype = plan.matype_param.as_str();
    let lb_args = callee_args(max, matype);
    let open_opts = callee_args(&format!("{min} + bankIdx"), matype);
    // Clamped as a double before it narrows, as C does: `| 0` wraps a period
    // too large for an int where C's order sends it to the maximum.
    let clamp_to = |o: &mut String, pad: &str, var: &str, x: &str, h: &str| {
        let _ = writeln!(o, "{pad}let {var} = {h}{min};");
        let _ = writeln!(o, "{pad}if ({x} > {h}{max}) {{\n{pad}  {var} = {h}{max};\n{pad}}} else if ({x} >= {h}{min}) {{\n{pad}  {var} = {x} | 0;\n{pad}}}");
    };
    let clamp = |o: &mut String, pad: &str, x: &str, h: &str| clamp_to(o, pad, "cp", x, h);
    let window_labels = registry.window_labels(&func.name.to_lowercase());
    let windowed = !window_labels.is_empty();
    if windowed {
        em.svc.import(callee, &cn.public);
    }
    let window_gate = |o: &mut String| {
        let _ = writeln!(o, "  let window = {base}WindowMode({matype});");
        let _ = writeln!(o, "  if ({max} - {min} + 1 < {}) {{\n    window = false;\n  }}", crate::streaming::WINDOW_MIN_BAND);
        let _ = writeln!(o, "  if (window) {{");
        let _ = writeln!(o, "    for (let bankIdx = 0; bankIdx < nBank; bankIdx++) {{");
        let _ = writeln!(
            o,
            "      if ({}({}, core) > lookbackTotal) {{\n        window = false;\n        break;\n      }}",
            cn.lookback,
            callee_args(&format!("{min} + bankIdx"), matype)
        );
        let _ = writeln!(o, "    }}\n  }}");
    };
    let store_window_handle = |o: &mut String| {
        for p in &func.optional_inputs {
            let _ = writeln!(o, "    sp.{0} = {0};", p.name);
        }
        let _ = writeln!(o, "    sp.bank = [];");
    };
    let open_bank = |o: &mut String, hist: &str, anchor: &str| {
        let _ = writeln!(o, "  const bank: {subty}[] = [];");
        let _ = writeln!(o, "  let reach = 0;");
        let _ = writeln!(o, "  for (let bankIdx = 0; bankIdx < nBank; bankIdx++) {{");
        let _ = writeln!(o, "    const slotSub = {}(core, {hist}, {anchor}, {open_opts});", cn.open_internal);
        let _ = writeln!(o, "    bank.push(slotSub);");
        let _ = writeln!(o, "    const slotReach = {detach}(slotSub);");
        let _ = writeln!(o, "    if (slotReach > reach) {{\n      reach = slotReach;\n    }}");
        let _ = writeln!(o, "  }}");
    };
    let store_handle = |o: &mut String| {
        for p in &func.optional_inputs {
            let _ = writeln!(o, "  sp.{0} = {0};", p.name);
        }
        let _ = writeln!(o, "  sp.bank = bank;");
    };

    let mut fields = base_fields(func);
    fields.push(field("tapeMask", CType::Int32, "0"));
    fields.push(field("tapePos", CType::Int32, "0"));
    fields.push(field("tape", CType::RealPtr, "new Float64Array(0)"));
    let members = Members {
        decls: format!("  /** @internal */\n  bank: {subty}[] = [];\n"),
        copy: "    c.bank = [];\n    for (let bankIdx = 0; bankIdx < this.bank.length; bankIdx++) {\n      c.bank.push(this.bank[bankIdx].clone());\n    }\n".to_string(),
    };
    let mut bank_frame = String::new();
    clamp(&mut bank_frame, "    ", period, "sp.");
    let _ = writeln!(bank_frame, "    const slot = cp - sp.{min};");
    let window_peek = if windowed { format!("sp.bank.length === 0 ? {base}EvalWindow(sp, {price}, cp) : ") } else { String::new() };
    let _ = writeln!(
        bank_frame,
        "    const cur_{out} = {window_peek}{peek_tape}(sp.bank[slot], sp.tape, ((sp.tapePos + 1) & sp.tapeMask) + sp.tapeMask + 1, sp.tapeMask, {price});"
    );
    emit_handle_class(o, em, &fields, &members, Some(&PeekFrame::falls_through(bank_frame)));

    emit_step_sig(o, em);
    if windowed {
        let _ = writeln!(o, "  if (sp.bank.length === 0) {{");
        clamp(o, "    ", period, "sp.");
        let _ = writeln!(o, "    const v = {base}EvalWindow(sp, {price}, cp);");
        let _ = writeln!(o, "    sp.tapePos = (sp.tapePos + 1) & sp.tapeMask;");
        let _ = writeln!(o, "    sp.tape[sp.tapePos] = {price};");
        let _ = writeln!(o, "    sp.cur_{out} = v;\n    return;\n  }}");
    }
    clamp(o, "  ", period, "sp.");
    let _ = writeln!(o, "  const slot = cp - sp.{min};");
    let _ = writeln!(o, "  sp.tapePos = (sp.tapePos + 1) & sp.tapeMask;");
    let _ = writeln!(o, "  sp.tape[sp.tapePos] = {price};");
    let _ = writeln!(o, "  const tapeBase = sp.tapePos + sp.tapeMask + 1;");
    let _ = writeln!(o, "  for (let bankIdx = 0; bankIdx < sp.bank.length; bankIdx++) {{");
    let _ = writeln!(o, "    const subValue = {step_tape}(sp.bank[bankIdx], sp.tape, tapeBase, sp.tapeMask, {price});");
    let _ = writeln!(o, "    if (bankIdx === slot) {{\n      sp.cur_{out} = subValue;\n    }}");
    let _ = writeln!(o, "  }}\n}}");

    // Strictly above: at size == reach the deepest read lands on the current
    // bar's slot and returns the wrong bar with no other symptom.
    let _ = writeln!(o, "\nfunction {base}TapeOpen(sp: {class}, {price}: Float64Array, historyLen: number, reach: number): void {{");
    let _ = writeln!(o, "  let size = 1;\n  while (size <= reach) {{\n    size <<= 1;\n  }}");
    let _ = writeln!(o, "  sp.tape = new Float64Array(size);\n  sp.tapeMask = size - 1;");
    let _ = writeln!(o, "  for (let b = historyLen > size ? historyLen - size : 0; b < historyLen; b++) {{");
    let _ = writeln!(o, "    sp.tape[b & sp.tapeMask] = {price}[b];\n  }}");
    let _ = writeln!(o, "  sp.tapePos = (historyLen - 1) & sp.tapeMask;\n}}");

    if windowed {
        let _ = writeln!(o, "\nfunction {base}WindowMode(t: number): boolean {{");
        if window_labels.iter().any(|l| l == "*") {
            let _ = writeln!(o, "  return true;");
        } else {
            let _ = writeln!(o, "  switch (t) {{");
            for label in window_labels {
                let _ = writeln!(o, "    case {}:", ts::render_switch_label(label, em.svc.enums));
            }
            let _ = writeln!(o, "      return true;\n    default:\n      return false;\n  }}");
        }
        let _ = writeln!(o, "}}");
        // Module-level scratch: the step and the peek both evaluate here once
        // per bar, and nothing they call re-enters this module.
        let _ = writeln!(o, "\nlet {base}Window = new Float64Array(0);\nconst {base}Value = new Float64Array(1);");
        let _ = writeln!(o, "\nfunction {base}EvalWindow(sp: {class}, {price}: number, cp: number): number {{");
        let _ = writeln!(o, "  const lb = {}({}, sp.core);", cn.lookback, callee_args("cp", &format!("sp.{matype}")));
        let _ = writeln!(o, "  if ({base}Window.length < lb + 1) {{\n    {base}Window = new Float64Array(lb + 1);\n  }}");
        let _ = writeln!(o, "  const win = {base}Window;");
        let _ = writeln!(o, "  for (let i = 0; i < lb; i++) {{");
        let _ = writeln!(o, "    win[i] = sp.tape[(sp.tapePos + sp.tapeMask + 2 - lb + i) & sp.tapeMask];\n  }}");
        let _ = writeln!(o, "  win[lb] = {price};");
        let _ = writeln!(o, "  {}(lb, lb, win, {}, {base}Value, sp.core);", cn.public, callee_args("cp", &format!("sp.{matype}")));
        let _ = writeln!(o, "  return {base}Value[0];\n}}");
    }

    let own_lb = em.lookback();
    emit_open_body_sig(o, em, OutMode::Scalar);
    let mut s = String::new();
    let _ = writeln!(s, "  const historyLen = {price}.length;");
    emit_open_validation(&mut s, em, OutMode::Scalar);
    let _ = writeln!(s, "  if ({min} > {max}) {{\n    return RetCode.BAD_PARAM;\n  }}");
    let _ = writeln!(s, "  if (historyLen < {own_lb} + 1) {{\n    return RetCode.INSUFFICIENT_HISTORY;\n  }}");
    // Every sub seeds at the shared max-period lookback, as batch does.
    let _ = writeln!(s, "  const lookbackTotal = {}({lb_args}, core);", cn.lookback);
    let _ = writeln!(s, "  const subStart = startIdx < lookbackTotal ? lookbackTotal : startIdx;");
    let _ = writeln!(s, "  if (historyLen < subStart + 1) {{\n    return RetCode.INSUFFICIENT_HISTORY;\n  }}");
    let _ = writeln!(s, "  const nBank = {max} - {min} + 1;");
    if windowed {
        window_gate(&mut s);
        let _ = writeln!(s, "  if (window) {{");
        clamp(&mut s, "    ", &format!("{period}[historyLen - 1]"), "");
        store_window_handle(&mut s);
        let _ = writeln!(s, "    {base}TapeOpen(sp, {price}, historyLen, lookbackTotal);");
        let _ = writeln!(s, "    const out1 = new Float64Array(1);");
        let _ = writeln!(s, "    {}(historyLen - 1, historyLen - 1, {price}, {}, out1, core);", cn.public, callee_args("cp", matype));
        let _ = writeln!(s, "    sp.cur_{out} = out1[0];");
        let _ = writeln!(s, "    sp.outRangeBegIdx = subStart;\n    sp.outRangeCount = historyLen - subStart;");
        let _ = writeln!(s, "    return RetCode.SUCCESS;\n  }}");
    }
    open_bank(&mut s, price, "subStart");
    clamp(&mut s, "  ", &format!("{period}[historyLen - 1]"), "");
    store_handle(&mut s);
    let _ = writeln!(s, "  {base}TapeOpen(sp, {price}, historyLen, reach);");
    let _ = writeln!(s, "  sp.cur_{out} = bank[cp - {min}].cur_{callee_out0};");
    let _ = writeln!(s, "  sp.outRangeBegIdx = subStart;\n  sp.outRangeCount = historyLen - subStart;");
    let _ = writeln!(s, "  return RetCode.SUCCESS;");
    o.push_str(&with_core(&s));
    let _ = writeln!(o, "}}");

    emit_open_body_sig(o, em, OutMode::Fill);
    let mut s = String::new();
    let _ = writeln!(s, "  const historyLen = {price}.length;");
    emit_open_validation(&mut s, em, OutMode::Fill);
    let _ = writeln!(s, "  if ({min} > {max}) {{\n    return RetCode.BAD_PARAM;\n  }}");
    let _ = writeln!(s, "  const lookbackTotal = {}({lb_args}, core);", cn.lookback);
    let _ = writeln!(s, "  if (historyLen < lookbackTotal + 1) {{\n    return RetCode.INSUFFICIENT_HISTORY;\n  }}");
    let _ = writeln!(s, "  const nBank = {max} - {min} + 1;");
    if windowed {
        window_gate(&mut s);
        let _ = writeln!(s, "  if (window) {{");
        let _ = writeln!(s, "    const run = new Float64Array(historyLen - lookbackTotal);");
        let _ = writeln!(s, "    let t1 = 0;");
        let _ = writeln!(s, "    for (let t = lookbackTotal; t < historyLen; t = t1 + 1) {{");
        clamp(&mut s, "      ", &format!("{period}[t]"), "");
        let _ = writeln!(s, "      for (t1 = t; t1 + 1 < historyLen; t1++) {{");
        clamp_to(&mut s, "        ", "cp2", &format!("{period}[t1 + 1]"), "");
        let _ = writeln!(s, "        if (cp2 !== cp) {{\n          break;\n        }}\n      }}");
        let _ = writeln!(s, "      {}(t, t1, {price}, {}, run, core);", cn.public, callee_args("cp", matype));
        let _ = writeln!(s, "      {out}.set(run.subarray(0, t1 - t + 1), t - lookbackTotal);");
        let _ = writeln!(s, "    }}");
        store_window_handle(&mut s);
        let _ = writeln!(s, "    {base}TapeOpen(sp, {price}, historyLen, lookbackTotal);");
        let _ = writeln!(s, "    outBegIdx.value = lookbackTotal;\n    outNBElement.value = historyLen - lookbackTotal;");
        let _ = writeln!(s, "    sp.cur_{out} = {out}[outNBElement.value - 1];");
        let _ = writeln!(s, "    return RetCode.SUCCESS;\n  }}");
    }
    // Each sub seeds at the first output bar, then the step replays the rest.
    let _ = writeln!(s, "  const seedPrefix = {price}.slice(0, lookbackTotal + 1);");
    open_bank(&mut s, "seedPrefix", "lookbackTotal");
    clamp(&mut s, "  ", &format!("{period}[lookbackTotal]"), "");
    store_handle(&mut s);
    let _ = writeln!(s, "  {base}TapeOpen(sp, seedPrefix, lookbackTotal + 1, reach);");
    let _ = writeln!(s, "  {out}[0] = bank[cp - {min}].cur_{callee_out0};");
    let _ = writeln!(s, "  for (let t = lookbackTotal + 1; t < historyLen; t++) {{");
    let _ = writeln!(s, "    {base}StepImpl(sp, {price}[t], {period}[t]);");
    let _ = writeln!(s, "    {out}[t - lookbackTotal] = sp.cur_{out};\n  }}");
    let _ = writeln!(s, "  outBegIdx.value = lookbackTotal;\n  outNBElement.value = historyLen - lookbackTotal;");
    let _ = writeln!(s, "  sp.cur_{out} = {out}[outNBElement.value - 1];");
    let _ = writeln!(s, "  return RetCode.SUCCESS;");
    o.push_str(&with_core(&s));
    let _ = writeln!(o, "}}");

    emit_open_wrappers(o, em, false);
}

// ---------------------------------------------------------------------------
// Composed tier: a producer transition and a pipeline of owned sub-streams.
// ---------------------------------------------------------------------------

#[allow(clippy::too_many_arguments, clippy::too_many_lines)]
fn emit_composed_step(
    o: &mut String,
    em: &Em,
    cp: &streaming::ComposedPlan,
    step_settings: &BTreeSet<String>,
    inputs: &[String],
    outputs: &[String],
    fields: &[Field],
    frame: bool,
    scratch: &mut BTreeSet<String>,
) -> Option<String> {
    let func = em.func;
    let indent = if frame { 4 } else { 2 };
    let pad = " ".repeat(indent);
    let mut sink = String::new();
    let o: &mut String = if frame { &mut sink } else { o };
    if !frame {
        emit_step_sig(o, em);
    }
    let cur_scalars = composed_cur_scalars(cp, inputs, outputs);
    let cur_ty = |name: &str| out_ctype(func, name);

    if let (Some(model), false) = (&cp.producer, frame) {
        for (name, ty) in &model.temps {
            let _ = writeln!(o, "{pad}{}", local_decl(name, ty));
        }
    }
    for (name, ty) in &cp.map_temps {
        let _ = writeln!(o, "{pad}{}", local_decl(name, ty));
    }
    for name in &cur_scalars {
        let _ = writeln!(o, "{pad}let cur_{name} = 0;");
    }
    let mut aliased: Vec<String> = Vec::new();
    if frame {
        for out in outputs {
            if !cur_scalars.contains(out) {
                let _ = writeln!(o, "{pad}let cur_{out} = 0;");
                aliased.push(out.clone());
            }
        }
    }

    let mut cur: std::collections::BTreeMap<String, String> = inputs.iter().map(|b| (b.clone(), b.clone())).collect();
    let declare_composed = |ctx: &TsCtx| {
        for n in &cur_scalars {
            if !outputs.contains(n) {
                ctx.env.declare(&format!("cur_{n}"), cur_ty(n));
            }
        }
        for (n, t) in &cp.map_temps {
            ctx.env.declare(n, CType::of_var_type(t));
        }
        for st in &cp.map_state {
            ctx.env.declare(&st.name, CType::of_var_type(&st.ty));
        }
    };

    if let Some(model) = &cp.producer {
        let names = TsComposedNames { series: cp.series.clone().expect("producer plan carries a series") };
        if frame {
            let declared: BTreeSet<String> = cur_scalars
                .iter()
                .chain(aliased.iter())
                .map(|n| format!("cur_{n}"))
                .chain(cp.map_temps.iter().map(|(n, _)| n.clone()))
                .chain(cp.map_state.iter().map(|st| st.name.clone()))
                .collect();
            o.push_str(&peek_frame_with(em, model, &names, fields, step_settings, indent, &declared, Some(&declare_composed))?);
        } else {
            o.push_str(&settings_snapshot(step_settings, &pad));
            let transition =
                streaming::build_transition(model, &names).unwrap_or_else(|e| panic!("streaming transition: {e}"));
            let ctx = em.frame_ctx(&transition, fields, Some(model), step_settings, false);
            declare_composed(&ctx);
            o.push_str(&em.render(&ctx, &transition, indent));
        }
        let series = cp.series.clone().expect("producer plan carries a series");
        cur.insert(series.clone(), format!("cur_{series}"));
    }

    let verb = if frame { "peek" } else { "update" };
    let params: BTreeSet<String> = func.optional_inputs.iter().map(|p| p.name.clone()).collect();
    let mut state_loaded = false;
    for step in &cp.steps {
        match step {
            streaming::UpdateStep::Sub { sub_idx } => {
                let sub = &cp.subs[*sub_idx];
                let callee_key = sub.callee.to_lowercase();
                let args: Vec<String> =
                    sub.srcs.iter().map(|src| cur.get(src).expect("analyzer ordered sub srcs").clone()).collect();
                let arg_str = args.join(", ");
                if sub.dsts.len() == 1 {
                    let d = &sub.dsts[0];
                    let _ = writeln!(o, "{pad}cur_{d} = sp.sub{sub_idx}.{verb}({arg_str});");
                } else {
                    let sink_expr = callee_scratch(em, &callee_key, scratch);
                    let _ = writeln!(o, "{pad}{{");
                    let _ = writeln!(o, "{pad}  const subOut{sub_idx} = {sink_expr};");
                    let _ = writeln!(o, "{pad}  sp.sub{sub_idx}.{verb}({arg_str}, subOut{sub_idx});");
                    for (k, d) in sub.dsts.iter().enumerate() {
                        let _ = writeln!(o, "{pad}  cur_{d} = subOut{sub_idx}.{};", callee_value_field(em, &callee_key, k));
                    }
                    let _ = writeln!(o, "{pad}}}");
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
                if !std::mem::replace(&mut state_loaded, true) {
                    for st in &cp.map_state {
                        let _ = writeln!(o, "{pad}let {0} = sp.{0};", st.name);
                    }
                }
                for out in streaming::map_output_writes(&cp.tail[*tail_idx], outputs) {
                    cur.entry(out.clone()).or_insert_with(|| format!("cur_{out}"));
                }
                let stmts = transform_map_step(&cp.tail[*tail_idx], &cur, &params, &cp.sub_lag_rings);
                let ctx = em.frame_ctx(&stmts, fields, cp.producer.as_ref(), step_settings, false);
                ctx.inline_fma_per_bar(cp.tail);
                declare_composed(&ctx);
                o.push_str(&em.render(&ctx, &stmts, indent));
            }
        }
    }
    if !frame {
        for ring in &cp.sub_lag_rings {
            let sn = &ring.series;
            let _ = writeln!(o, "{pad}sp.lagRing_{sn}[sp.lagRingPos_{sn}] = cur_{sn};");
            let _ = writeln!(o, "{pad}sp.lagRingPos_{sn} = (sp.lagRingPos_{sn} + 1) % sp.lagRingCap_{sn};");
        }
        for st in cp.map_state.iter().filter(|st| st.carried) {
            let _ = writeln!(o, "{pad}sp.{0} = {0};", st.name);
        }
    }
    let target = if frame { "" } else { "sp." };
    for out in outputs {
        let src = cur.get(out).expect("analyzer gated output");
        if frame && src == &format!("cur_{out}") {
            continue;
        }
        let _ = writeln!(o, "{pad}{target}cur_{out} = {src};");
    }
    if frame {
        return Some(sink);
    }
    let _ = writeln!(o, "}}");
    None
}

#[allow(clippy::too_many_lines)]
fn emit_composed_open(
    o: &mut String,
    em: &Em,
    cp: &streaming::ComposedPlan,
    step_settings: &BTreeSet<String>,
    outputs: &[String],
) {
    let func = em.func;
    emit_open_body_sig(o, em, OutMode::Core);
    let (region_stmts, tail_stmts) = build_composed_open_bodies(cp, outputs);
    let combined: Vec<Statement> =
        cleanup_open_body(&region_stmts.iter().cloned().chain(tail_stmts.iter().cloned()).collect::<Vec<_>>(), em);
    let ctx = em.open_ctx(&combined);
    for out in outputs {
        ctx.env.declare(&format!("sc_{out}"), if out_ctype(func, out) == CType::Int32 { CType::IntPtr } else { CType::RealPtr });
    }
    let mut s = String::new();
    emit_body_decls(&mut s, &combined, &ctx);
    emit_open_head(&mut s, em);
    emit_open_validation(&mut s, em, OutMode::Core);
    emit_anchor_guard(&mut s);
    // A sub's rejection would carry the callee's name; past this point one is
    // a bug in this function's lookback.
    let _ = writeln!(s, "  if (historyLen < {} + 1) {{\n    return RetCode.INSUFFICIENT_HISTORY;\n  }}", em.lookback_in(&ctx));
    emit_extras_and_candle(&mut s, em, &combined, &ctx);
    let alias_fill = cp.fill_scratch_may_alias_output(outputs);
    for out in outputs {
        let ty = arr_ty(out_ctype(func, out));
        if alias_fill {
            let _ = writeln!(s, "  const sc_{out} = outStride === 1 ? {out} : new {ty}(historyLen);");
        } else {
            let _ = writeln!(s, "  const sc_{out} = new {ty}(historyLen);");
        }
    }

    let region_len = region_stmts.len();
    let own_inputs: BTreeSet<String> = streaming::input_array_names(func).into_iter().collect();
    let mut inserts: Vec<(usize, String)> = Vec::new();
    let mut replaced: BTreeSet<usize> = BTreeSet::new();
    for (si, sub) in cp.subs.iter().enumerate() {
        let mut t = String::new();
        let callee_key = sub.callee.to_lowercase();
        let cn = callee_names(em, &callee_key);
        let sc_rewrite = |e: &Expr| -> Expr {
            streaming::rewrite_expr(e, &|x| match x {
                Expr::Var(v) if outputs.contains(&v) => Expr::Var(format!("sc_{v}")),
                other => other,
            })
        };
        let anchor = ctx.value_for(CType::Int32, &sc_rewrite(&sub.s_arg));
        let e_arg = ctx.value_for(CType::Int32, &sc_rewrite(&sub.e_arg));
        let srcs: Vec<String> = sub
            .srcs
            .iter()
            .map(|src| {
                let name = if outputs.contains(src) { format!("sc_{src}") } else { src.clone() };
                // A whole own input is already exactly the range; anything
                // else is cut to what the callee may read.
                if own_inputs.contains(&name) && e_arg == "endIdx" {
                    name
                } else {
                    format!("{name}.slice(0, ({e_arg}) + 1)")
                }
            })
            .collect();
        let opts = callee_opts(em, &callee_key, &sub.opt_args, &ctx);
        let opt_tail = comma(&opts);
        let fused = sub.is_fusable().then(|| streaming::batch_call_out_args(&tail_stmts[sub.tail_idx], sub)).flatten();
        if let Some((out_meta, dsts)) = fused {
            em.svc.import(&callee_key, &cn.open_and_fill_internal);
            let rend = |e: &Expr| ctx.expr(&sc_rewrite(e));
            let metas: Vec<String> = out_meta.iter().map(|e| meta_box(&sc_rewrite(e), &ctx)).collect();
            let dst_args: Vec<String> = dsts.iter().map(|e| rend(e)).collect();
            let _ = writeln!(
                t,
                "  const sub{si} = {}(core, {}, {anchor}{opt_tail}, {}, {});",
                cn.open_and_fill_internal,
                srcs.join(", "),
                metas.join(", "),
                dst_args.join(", ")
            );
            if let Statement::Assign { target, .. } = &tail_stmts[sub.tail_idx] {
                let _ = writeln!(t, "  {} = RetCode.SUCCESS;", ctx.expr(&sc_rewrite(target)));
            }
            replaced.insert(region_len + sub.tail_idx);
        } else {
            em.svc.import(&callee_key, &cn.open_internal);
            let _ = writeln!(t, "  const sub{si} = {}(core, {}, {anchor}{opt_tail});", cn.open_internal, srcs.join(", "));
        }
        inserts.push((region_len + sub.tail_idx, t));
    }

    emit_open_region(&mut s, em, &combined, &ctx, &inserts, &replaced);

    let _ = writeln!(s, "  if (outNBElement.value < 1) {{\n    return RetCode.INSUFFICIENT_HISTORY;\n  }}");
    for ring in &cp.sub_lag_rings {
        let sr = &ring.series;
        let lag = ctx.value_for(CType::Int32, &ring.lag);
        let _ = writeln!(s, "  const lagCap_{sr} = {lag};");
        let _ = writeln!(s, "  const lagRing_{sr} = new Float64Array(lagCap_{sr});");
        let _ = writeln!(s, "  for (let lagI = 0; lagI < lagCap_{sr}; lagI++) {{");
        let _ = writeln!(s, "    lagRing_{sr}[lagI] = {sr}[outNBElement.value + lagI];\n  }}");
    }
    let mut extra = String::new();
    for (si, _) in cp.subs.iter().enumerate() {
        let _ = writeln!(extra, "  sp.sub{si} = sub{si};");
    }
    for ring in &cp.sub_lag_rings {
        let sr = &ring.series;
        let _ = writeln!(extra, "  sp.lagRingPos_{sr} = 0;\n  sp.lagRingCap_{sr} = lagCap_{sr};\n  sp.lagRing_{sr} = lagRing_{sr};");
    }
    for st in &cp.map_state {
        let _ = writeln!(extra, "  sp.{0} = {0};", st.name);
    }
    if let Some(model) = &cp.producer {
        emit_capture(&mut s, em, model, &model.state, step_settings, &ctx, None, &extra);
    } else {
        for p in &func.optional_inputs {
            let _ = writeln!(s, "  sp.{0} = {0};", p.name);
        }
        for (name, _) in &func.private_extra_params {
            let _ = writeln!(s, "  sp.{name} = {name};");
        }
        s.push_str(&extra);
    }
    emit_cur_capture(&mut s, outputs, CurSource::Scratch);
    if !alias_fill {
        for out in outputs {
            let _ = writeln!(s, "  if (outStride === 1) {{\n    {out}.set(sc_{out}.subarray(0, outNBElement.value), 0);\n  }}");
        }
    }
    let _ = writeln!(s, "  return RetCode.SUCCESS;");
    o.push_str(&with_core(&s));
    let _ = writeln!(o, "}}");
}

/// An out-meta argument of a fused sub-open: the `MInteger` box itself.
fn meta_box(e: &Expr, ctx: &TsCtx) -> String {
    match e {
        Expr::AddressOf(inner) => match inner.as_ref() {
            Expr::Var(n) => n.clone(),
            other => ctx.expr(other),
        },
        Expr::Var(n) => n.clone(),
        other => ctx.expr(other),
    }
}

fn emit_composed(o: &mut String, em: &Em, cp: &streaming::ComposedPlan) {
    let func = em.func;
    let inputs = streaming::input_array_names(func);
    let outputs: Vec<String> = func.outputs.iter().map(|x| x.name.clone()).collect();
    let step_settings = match &cp.producer {
        Some(model) => detect_candle_settings(&model.steady_stmts),
        None => BTreeSet::new(),
    };
    let mut fields: Vec<Field> = match &cp.producer {
        Some(model) => state_fields(func, model, &step_settings),
        None => base_fields(func),
    };
    for ring in &cp.sub_lag_rings {
        let sr = &ring.series;
        fields.push(field(format!("lagRingPos_{sr}"), CType::Int32, "0"));
        fields.push(field(format!("lagRingCap_{sr}"), CType::Int32, "1"));
        fields.push(field(format!("lagRing_{sr}"), CType::RealPtr, "new Float64Array(1)"));
    }
    for st in &cp.map_state {
        fields.push(var_field(&st.name, &st.ty));
    }
    let mut members = Members::default();
    for (si, sub) in cp.subs.iter().enumerate() {
        let cls = callee_class(em, &sub.callee.to_lowercase());
        let _ = writeln!(members.decls, "  /** @internal */\n  sub{si}!: {cls};");
        let _ = writeln!(members.copy, "    c.sub{si} = this.sub{si}.clone();");
    }
    let mut scratch = BTreeSet::new();
    let frame = emit_composed_step(&mut String::new(), em, cp, &step_settings, &inputs, &outputs, &fields, true, &mut scratch)
        .map(PeekFrame::falls_through);
    emit_handle_class(o, em, &fields, &members, frame.as_ref());
    emit_composed_step(o, em, cp, &step_settings, &inputs, &outputs, &fields, false, &mut scratch);
    emit_composed_open(o, em, cp, &step_settings, &outputs);
    emit_open_and_fill_internal_wrapper(o, em, true);
    emit_open_wrappers(o, em, true);
    emit_scratch_decls(o, &scratch);
}
