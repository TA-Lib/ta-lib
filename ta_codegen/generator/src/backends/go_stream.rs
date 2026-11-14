//! Go stream emitter: the streaming tier of each streamable function's file.
//! It renders the same backend-neutral analysis as `ts_stream.rs`, through the
//! batch tier's lowering, so a streamed value is the batch value bit for bit.
//!
//! The handle's state is unexported fields of `<NAME>Stream`; the seams are
//! package functions (`<n>StepImpl`, the tape entries) and `*Core` methods
//! (`<n>OpenImpl` and the two `_Internal` variants), which a composed caller
//! reaches inside the package. A sub-stream's error surfaces from the step that
//! drove it.

use std::cell::Cell;
use std::collections::{BTreeSet, HashMap};

use crate::candle_settings::detect_candle_settings;
use crate::ir::{CircBuf, Expr, FuncDef, Lang, ParamType, Statement, Tier, VarType};
use crate::streaming::{self, StreamModel, StreamPlan};

use super::common;
use super::ctype::CType;
use super::fma::{self, FmaVarSets};
use super::go::{self, go_ident, Ctx, Ret, Scope, Services};
use super::go_doc::{self, StreamMethod};
use super::go_fmt::{GExpr, GOp};
use super::java_stream::{
    build_composed_open_bodies, build_open_body_java, dual_scalar_union, has_return, localize_state_writes,
    managed_lag_cap, managed_lag_open_locals,
};

/// Whether `func` gets a streaming tier: declared streamable, and the analysis
/// of its Go resolution accepts it.
pub fn emits_stream(func: &FuncDef, lookup: &dyn streaming::CalleeLookup) -> bool {
    func.streaming && streaming::validate_streamable(&func.resolved_for(Lang::Go), lookup).is_ok()
}

/// The names a function's streaming tier defines.
pub struct GoStreamNames {
    /// The handle type (`SMAStream`).
    pub stream: String,
    /// The public openers (`SMAOpen`, `SMAOpenAndFill`).
    pub open: String,
    pub open_and_fill: String,
    /// The anchored seams (`smaOpenInternal`, `smaOpenAndFillInternal`).
    pub open_internal: String,
    pub open_and_fill_internal: String,
    /// The transcribed open (`smaOpenImpl`) and transition (`smaStepImpl`).
    pub open_impl: String,
    pub step_impl: String,
}

#[must_use]
pub fn go_stream_names(func_name: &str) -> GoStreamNames {
    let camel = common::camel_words(func_name);
    GoStreamNames {
        stream: format!("{func_name}Stream"),
        open: format!("{func_name}Open"),
        open_and_fill: format!("{func_name}OpenAndFill"),
        open_internal: format!("{camel}OpenInternal"),
        open_and_fill_internal: format!("{camel}OpenAndFillInternal"),
        open_impl: format!("{camel}OpenImpl"),
        step_impl: format!("{camel}StepImpl"),
    }
}

fn multi(func: &FuncDef) -> bool {
    func.outputs.len() > 1
}

fn out_ctype(func: &FuncDef, name: &str) -> CType {
    if func.outputs.iter().any(|o| o.name == name && o.param_type == ParamType::Integer) {
        CType::Int32
    } else {
        CType::Real
    }
}

fn scalar_ty(t: CType) -> &'static str {
    go::go_type(t)
}

fn slice_ty(t: CType) -> &'static str {
    if t == CType::Int32 {
        "[]int32"
    } else {
        "[]float64"
    }
}

fn bars(func: &FuncDef) -> Vec<String> {
    streaming::input_array_names(func)
}

/// `inHigh, inLow float64`, the bar parameters of `Update` and `Peek`.
fn bar_sig(func: &FuncDef) -> String {
    go::param_list(&bars(func).iter().map(|b| (go_ident(b), "float64".to_string())).collect::<Vec<_>>())
}

fn bar_args(func: &FuncDef) -> String {
    bars(func).iter().map(|b| go_ident(b)).collect::<Vec<_>>().join(", ")
}

/// The result list of `Update`, `Peek` and `Value`: one value per output.
fn value_types(func: &FuncDef) -> Vec<&'static str> {
    func.outputs.iter().map(|o| scalar_ty(out_ctype(func, &o.name))).collect()
}

fn results(types: &[&str], with_error: bool) -> String {
    let mut t: Vec<&str> = types.to_vec();
    if with_error {
        t.push("error");
    }
    if t.len() == 1 {
        t[0].to_string()
    } else {
        format!("({})", t.join(", "))
    }
}

/// `return 0, 0, err`: every output's zero beside `err`.
fn fail_return(func: &FuncDef, err: &str) -> String {
    let mut v: Vec<&str> = func.outputs.iter().map(|_| "0").collect();
    v.push(err);
    format!("return {}", v.join(", "))
}

fn cur_list(func: &FuncDef, prefix: &str) -> String {
    func.outputs.iter().map(|o| format!("{prefix}cur_{}", o.name)).collect::<Vec<_>>().join(", ")
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
struct GoNames;

names_through!(GoNames, |_: &GoNames, name: &str| Expr::Var(format!("sp.cur_{name}")), |_: &GoNames| None);

/// [`GoNames`] for a tape frame.
struct GoTapeNames(streaming::TapeNames);

names_through!(
    GoTapeNames,
    |_: &GoTapeNames, name: &str| Expr::Var(format!("sp.cur_{name}")),
    |t: &GoTapeNames| Some(t.0.clone())
);

/// A composed producer: its intermediate series lands in a `cur_<series>` local.
struct GoComposedNames {
    series: String,
}

names_through!(
    GoComposedNames,
    |s: &GoComposedNames, name: &str| {
        if name == s.series {
            Expr::Var(format!("cur_{name}"))
        } else {
            Expr::Var(format!("sp.cur_{name}"))
        }
    },
    |_: &GoComposedNames| None
);

/// One handle field: its name, C type, Go type and identity-path value.
#[derive(Clone)]
struct Field {
    name: String,
    ty: CType,
    go_ty: String,
    default: String,
    /// A fixed-size array: copied with the handle, a slice only as a value.
    array: bool,
    code: bool,
}

impl Field {
    /// A slice the handle owns, which `Clone` copies.
    fn owned_slice(&self) -> bool {
        self.ty.is_pointer() && !self.array
    }
}

fn field(name: impl Into<String>, ty: CType, default: impl Into<String>) -> Field {
    let go_ty = match ty {
        CType::RealPtr => "[]float64".to_string(),
        CType::IntPtr => "[]int32".to_string(),
        t => scalar_ty(t).to_string(),
    };
    Field { name: name.into(), ty, go_ty, default: default.into(), array: false, code: false }
}

fn var_field(name: &str, ty: &VarType) -> Field {
    match ty {
        VarType::Real => field(name, CType::Real, "0"),
        VarType::Integer | VarType::Index => field(name, CType::Int32, "0"),
        VarType::RetCodeType => Field { go_ty: "RetCode".to_string(), code: true, ..field(name, CType::Int32, "Success") },
        VarType::RealPointer => field(name, CType::RealPtr, "make([]float64, 1)"),
        VarType::IntPointer => field(name, CType::IntPtr, "make([]int32, 1)"),
        VarType::RealArray(n) => Field { go_ty: format!("[{n}]float64"), array: true, ..field(name, CType::RealPtr, format!("[{n}]float64{{}}")) },
        VarType::IntArray(n) => Field { go_ty: format!("[{n}]int32"), array: true, ..field(name, CType::IntPtr, format!("[{n}]int32{{}}")) },
    }
}

/// A local's declaration at its zero value.
fn local_decl(name: &str, ty: &VarType) -> String {
    format!("var {} {}", go_ident(name), go::var_go_type(ty))
}

fn param_field(func: &FuncDef, p: &crate::ir::OptInput) -> Field {
    let _ = func;
    field(&p.name, CType::of_param(&p.param_type), go_ident(&p.name))
}

fn extra_ctype(c_type: &str) -> CType {
    match c_type {
        "double" => CType::Real,
        "int" => CType::Int32,
        other => panic!("unsupported private extra param type: {other}"),
    }
}

fn base_fields(func: &FuncDef) -> Vec<Field> {
    let mut fields = Vec::new();
    for p in &func.optional_inputs {
        fields.push(param_field(func, p));
    }
    for (name, c_type) in &func.private_extra_params {
        fields.push(field(name, extra_ctype(c_type), go_ident(name)));
    }
    for out in &func.outputs {
        fields.push(field(format!("cur_{}", out.name), out_ctype(func, &out.name), "0"));
    }
    fields
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
        fields.push(param_field(func, p));
    }
    for (name, c_type) in &func.private_extra_params {
        fields.push(field(name, extra_ctype(c_type), go_ident(name)));
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
            fields.push(field(format!("ring_{v}_{arr}"), CType::RealPtr, format!("make([]float64, {id_len})")));
        }
    }
    for win in model.windows() {
        let v = &win.var;
        fields.push(field(format!("winPos_{v}"), CType::Int32, "0"));
        fields.push(field(format!("winCap_{v}"), CType::Int32, "1"));
        for arr in &win.arrays {
            fields.push(field(format!("win_{v}_{arr}"), CType::RealPtr, "make([]float64, 1)"));
        }
    }
    for circ in model.circs() {
        fields.push(field(format!("cbSize_{}", circ.id), CType::Int32, "0"));
        for (storage, ty) in streaming::circ_storages(circ) {
            let f = if matches!(ty, VarType::Integer) {
                field(format!("cb_{storage}"), CType::IntPtr, "make([]int32, 1)")
            } else {
                field(format!("cb_{storage}"), CType::RealPtr, "make([]float64, 1)")
            };
            fields.push(f);
        }
    }
    if let Some(ex) = model.extrema() {
        fields.push(field("xMask", CType::Int32, "0"));
        for arr in &ex.arrays {
            fields.push(field(format!("x_{arr}"), CType::RealPtr, "make([]float64, 1)"));
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

/// A tier's members beyond the fields: their declarations (name, Go type) and
/// the lines of `Clone` that copy them into `cl`.
#[derive(Default)]
struct Members {
    decls: Vec<(String, String)>,
    copy: Vec<String>,
}

/// What every frame of one function shares.
struct Em<'a> {
    func: &'a FuncDef,
    svc: &'a Services<'a>,
    fma: &'a FmaVarSets,
    counter: &'a Cell<usize>,
    names: GoStreamNames,
    camel: String,
}

/// Line-building with Go's tab indentation.
#[derive(Default)]
struct B {
    lines: Vec<String>,
    depth: usize,
}

impl B {
    fn line(&mut self, s: impl AsRef<str>) {
        self.lines.push(format!("{}{}", "\t".repeat(self.depth), s.as_ref()));
    }
    fn open(&mut self, s: impl AsRef<str>) {
        self.line(s);
        self.depth += 1;
    }
    fn close(&mut self, s: impl AsRef<str>) {
        self.depth -= 1;
        self.line(s);
    }
    fn mid(&mut self, s: impl AsRef<str>) {
        self.depth -= 1;
        self.line(s);
        self.depth += 1;
    }
    fn extend(&mut self, lines: impl IntoIterator<Item = String>) {
        for l in lines {
            self.line(l);
        }
    }
    /// `if <cond> {` / `<body>` / `}` for a one-line body.
    fn guard(&mut self, cond: impl AsRef<str>, body: impl AsRef<str>) {
        self.open(format!("if {} {{", cond.as_ref()));
        self.line(body);
        self.close("}");
    }
}

impl<'a> Em<'a> {
    fn lookback_impl(&self) -> String {
        format!("c.{}", go::go_names(&self.func.name).lookback)
    }

    /// The C-shaped lookback over the open frame's own (int32) parameters.
    fn lookback_call(&self, via: &str) -> String {
        let args: Vec<String> = self.func.optional_inputs.iter().map(|p| go_ident(&p.name)).collect();
        format!("{via}.{}({})", go::go_names(&self.func.name).lookback, args.join(", "))
    }

    fn ctx(&self, body: &[Statement], ret: Ret) -> Ctx<'a> {
        Ctx::for_stream(self.svc, &self.func.name, body, self.fma, self.counter, ret)
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
        ret: Ret,
    ) -> Ctx<'a> {
        let mut ctx = self.ctx(body, ret);
        for f in fields {
            ctx.env.declare(&format!("sp.{}", f.name), f.ty);
            ctx.env.declare(&f.name, f.ty);
            if f.array {
                ctx.arrays.insert(format!("sp.{}", f.name));
                ctx.arrays.insert(f.name.clone());
            }
            if f.code {
                ctx.codes.insert(format!("sp.{}", f.name));
                ctx.codes.insert(f.name.clone());
            }
        }
        for b in bars(self.func) {
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
    fn open_ctx(&self, open_body: &[Statement]) -> Ctx<'a> {
        let ctx = self.ctx(open_body, Ret::Code);
        ctx.env.seed_signature(self.func);
        ctx.env.seed_candles(&detect_candle_settings(open_body));
        for n in ["outStride", "historyLen", "fillJ", "fillLb", "fillIdx"] {
            ctx.env.declare(n, CType::Int32);
        }
        ctx
    }
}

/// A function with its doc comment, as `go_doc` renders it, directly above it.
fn doc_fn(o: &mut String, doc: &str, sig: &str, lines: &[String]) {
    o.push_str(doc);
    o.push_str(&go::func_text(sig, lines));
}

fn render(ctx: &Ctx, body: &[Statement]) -> Vec<String> {
    ctx.block(body)
}

fn cond_text(ctx: &Ctx, e: &Expr) -> String {
    let mut sc = Scope::default();
    let g = ctx.cond(e, &mut sc);
    if !sc.is_empty() {
        ctx.fail("a stream condition that needs statements");
    }
    go::strip_paren(g).render()
}

fn int_g(ctx: &Ctx, e: &Expr) -> GExpr {
    ctx.pure(e, Some(CType::Int32))
}

fn name_g(s: &str) -> GExpr {
    GExpr::name(s)
}

fn bin(l: GExpr, op: GOp, r: GExpr) -> GExpr {
    GExpr::bin(l, op, r)
}

/// `arr[(*outNBElement-1)*outStride]`: the last value a strided fill wrote.
fn last_strided(arr: &str) -> String {
    let last = bin(GExpr::unary("*", name_g("outNBElement")), GOp::Sub, name_g("1")).paren();
    GExpr::index(name_g(arr), bin(last, GOp::Mul, name_g("outStride"))).render()
}

/// The streaming tier of one function's file.
#[must_use]
pub(super) fn generate(func: &FuncDef, svc: &Services) -> String {
    let resolved = func.resolved_for(Lang::Go);
    let func: &FuncDef = &resolved;
    assert!(func.streaming, "go_stream::generate called without a streaming declaration");
    let plan = streaming::validate_streamable(func, svc.registry).unwrap_or_else(|e| panic!("streaming gate: {e}"));
    // From the body these frames render, so the fused sites are the ones
    // every other backend's stream fuses.
    let mut stream_fma = fma::build_fma_var_sets(func.stream_source(), &func.outputs, &fma::INDEX_PARAM_SEEDS);
    for input in streaming::input_array_names(func) {
        stream_fma.real_vars.insert(input);
    }
    if let StreamPlan::Composed(cp) = &plan {
        stream_fma.real_vars.extend(streaming::lag_fma_reals(cp));
    }
    let counter = Cell::new(0usize);
    let em = Em {
        func,
        svc,
        fma: &stream_fma,
        counter: &counter,
        names: go_stream_names(&func.name),
        camel: common::camel_words(&func.name),
    };

    let mut o = String::new();
    if let Some(m) = func.alt_marker(Tier::Stream, Lang::Go) {
        o.push_str(&format!("\n// {m}\n"));
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
    let frame = peek_frame(em, model, &GoNames, &fields, &step_settings, &BTreeSet::new()).map(PeekFrame::falls_through);
    emit_handle(o, em, &fields, &Members::default(), frame.as_ref(), false);
    emit_step(o, em, model, &fields, &step_settings);
    emit_open_body(o, em, model, body, &fields, &step_settings);
    emit_open_and_fill_internal_wrapper(o, em, true);
    emit_open_wrappers(o, em, true, model.open_needs_last_bar_only());
    if em.svc.registry.in_tape_set(&em.func.name.to_lowercase()) {
        let tier = TapeTier { models: &[model], dual: None, fields: &fields, step_settings: &step_settings };
        emit_tape_entries(o, em, &tier);
    }
}

fn params_on_state(func: &FuncDef, e: &Expr) -> Expr {
    let params: Vec<String> = func.optional_inputs.iter().map(|p| p.name.clone()).collect();
    streaming::rewrite_expr(e, &|x| match x {
        Expr::Var(v) if params.contains(&v) => Expr::Var(format!("sp.{v}")),
        other => other,
    })
}

struct PeekFrame {
    lines: Vec<String>,
    /// The frame returns on every path; nothing follows it.
    terminal: bool,
}

impl PeekFrame {
    fn falls_through(lines: Vec<String>) -> Self {
        Self { lines, terminal: false }
    }
}

// ---------------------------------------------------------------------------
// The handle
// ---------------------------------------------------------------------------

/// `name<pad> type` lines, aligned as gofmt aligns a struct's fields.
fn struct_lines(fields: &[(String, String)]) -> Vec<String> {
    let w = fields.iter().map(|(n, _)| n.len()).max().unwrap_or(0);
    fields.iter().map(|(n, t)| format!("{n}{} {t}", " ".repeat(w - n.len()))).collect()
}

#[allow(clippy::too_many_lines)]
fn emit_handle(o: &mut String, em: &Em, fields: &[Field], members: &Members, frame: Option<&PeekFrame>, fallible: bool) {
    let func = em.func;
    let st = &em.names.stream;
    let mut decl: Vec<(String, String)> = vec![("core".to_string(), "*Core".to_string())];
    decl.extend(fields.iter().map(|f| (f.name.clone(), f.go_ty.clone())));
    decl.extend(members.decls.iter().cloned());
    decl.push(("outRangeBegIdx".to_string(), "int".to_string()));
    decl.push(("outRangeCount".to_string(), "int".to_string()));
    o.push_str(&go_doc::stream_type_docs(func));
    o.push_str(&format!("\ntype {st} struct {{\n"));
    for l in struct_lines(&decl) {
        o.push_str(&format!("\t{l}\n"));
    }
    o.push_str("}\n");

    doc_fn(
        o,
        &go_doc::stream_method_docs(func, StreamMethod::OutRange),
        &format!("func (sp *{st}) OutRange() OutRange"),
        &["return OutRange{BegIdx: sp.outRangeBegIdx, NBElement: sp.outRangeCount}".to_string()],
    );
    let mut b = B::default();
    b.guard("sp.outRangeBegIdx+sp.outRangeCount > IndexMax", "return OutOfRangeEndIndex");
    b.line("sp.outRangeCount++");
    b.line("return nil");
    doc_fn(
        o,
        &go_doc::stream_method_docs(func, StreamMethod::Advance),
        &format!("func (sp *{st}) Advance() error"), &b.lines);

    emit_update_method(o, em, fallible);
    emit_peek_method(o, em, frame);

    doc_fn(
        o,
        &go_doc::stream_method_docs(func, StreamMethod::Value),
        &format!("func (sp *{st}) Value() {}", results(&value_types(func), false)),
        &[format!("return {}", cur_list(func, "sp."))],
    );

    let mut b = B::default();
    b.line("cl := *sp");
    for f in fields.iter().filter(|f| f.owned_slice()) {
        b.line(format!("cl.{0} = append(sp.{0}[:0:0], sp.{0}...)", f.name));
    }
    b.extend(members.copy.iter().cloned());
    b.line("return &cl");
    doc_fn(
        o,
        &go_doc::stream_method_docs(func, StreamMethod::Clone),
        &format!("func (sp *{st}) Clone() *{st}"), &b.lines);
}

/// Every bar input finite, before anything is written: a handle keeps its
/// state, so one non-finite bar would poison every later value.
fn finite_check(b: &mut B, func: &FuncDef) {
    let conds: Vec<String> = bars(func).iter().map(|x| format!("!isFinite({})", go_ident(x))).collect();
    if conds.is_empty() {
        return;
    }
    b.guard(conds.join(" || "), fail_return(func, "BadParam"));
}

fn emit_update_method(o: &mut String, em: &Em, fallible: bool) {
    let func = em.func;
    let mut b = B::default();
    b.guard("sp.outRangeBegIdx+sp.outRangeCount > IndexMax", fail_return(func, "OutOfRangeEndIndex"));
    finite_check(&mut b, func);
    let sep = if bars(func).is_empty() { "" } else { ", " };
    let call = format!("{}(sp{sep}{})", em.names.step_impl, bar_args(func));
    if fallible {
        b.guard(format!("err := {call}; err != nil"), fail_return(func, "err"));
    } else {
        b.line(call);
    }
    b.line("sp.outRangeCount++");
    b.line(format!("return {}, nil", cur_list(func, "sp.")));
    doc_fn(
        o,
        &go_doc::stream_method_docs(func, StreamMethod::Update),
        &format!("func (sp *{}) Update({}) {}", em.names.stream, bar_sig(func), results(&value_types(func), true)),
        &b.lines,
    );
}

fn emit_peek_method(o: &mut String, em: &Em, frame: Option<&PeekFrame>) {
    let func = em.func;
    let frame = frame.expect("every tier emits a peek frame");
    let mut b = B::default();
    finite_check(&mut b, func);
    let mut body = with_core(frame.lines.clone());
    if !frame.terminal {
        body.push(format!("return {}, nil", cur_list(func, "")));
    }
    // A body local named as a bar parameter may shadow it inside a block,
    // never beside it.
    let bar_names: Vec<String> = bars(func).iter().map(|x| go_ident(x)).collect();
    if bar_names.iter().any(|n| declares_local(&body, n)) {
        b.open("{");
        b.extend(body);
        b.close("}");
    } else {
        b.extend(body);
    }
    doc_fn(
        o,
        &go_doc::stream_method_docs(func, StreamMethod::Peek),
        &format!("func (sp *{}) Peek({}) {}", em.names.stream, bar_sig(func), results(&value_types(func), true)),
        &b.lines,
    );
}

/// Whether `lines` declare `name` at some line's start.
fn declares_local(lines: &[String], name: &str) -> bool {
    lines.iter().any(|line| {
        let t = line.trim_start();
        let after = t.strip_prefix("var ").unwrap_or(t);
        after.strip_prefix(name).is_some_and(|rest| rest.starts_with(' ') && (t.starts_with("var ") || rest.starts_with(" :=")))
    })
}

/// The peek frame of one model: the transition with every store it would make
/// carried in a local of the field's own name, run against the live handle.
fn peek_frame(
    em: &Em,
    model: &StreamModel,
    names: &dyn streaming::NameMap,
    fields: &[Field],
    step_settings: &BTreeSet<String>,
    predeclared: &BTreeSet<String>,
) -> Option<Vec<String>> {
    peek_frame_with(em, model, names, fields, step_settings, predeclared, None)
}

/// [`peek_frame`] with the caller's own locals declared to the type pass.
#[allow(clippy::too_many_arguments)]
fn peek_frame_with(
    em: &Em,
    model: &StreamModel,
    names: &dyn streaming::NameMap,
    fields: &[Field],
    step_settings: &BTreeSet<String>,
    predeclared: &BTreeSet<String>,
    declare: Option<&dyn Fn(&Ctx)>,
) -> Option<Vec<String>> {
    let func = em.func;
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
        Statement::Return { value: None } => Some(Statement::Return { value: Some(Expr::Var(fresh_value_local(func))) }),
        other => Some(other),
    });
    let types: HashMap<&str, &Field> = fields.iter().map(|f| (f.name.as_str(), f)).collect();

    let mut out = Vec::new();
    for (name, ty) in &streaming::temps_used(&model.temps, &body_ir) {
        out.push(local_decl(name, ty));
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
        let local = go_ident(name);
        // A slice field taken plain would write the handle through it.
        if dead_seeds.contains(name) {
            out.push(format!("var {local} {}", f.go_ty));
        } else if f.owned_slice() {
            out.push(format!("{local} := append(sp.{name}[:0:0], sp.{name}...)"));
        } else {
            out.push(format!("{local} := sp.{name}"));
        }
    }
    for sh in &pt.shadows {
        out.push(format!("{} := int32(-1)", go_ident(&sh.slot_var)));
        out.push(format!("var {} {}", go_ident(&sh.val_var), if sh.int_elem { "int32" } else { "float64" }));
    }
    for t in &pt.slot_temps {
        out.push(format!("var {} int32", go_ident(t)));
    }
    out.extend(settings_snapshot(step_settings));
    let ret = if multi(func) { Ret::Code } else { Ret::Value(out_ctype(func, &func.outputs[0].name)) };
    let ctx = em.frame_ctx(&body_ir, fields, Some(model), step_settings, names.tape().is_some(), ret);
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
    out.extend(render(&ctx, &body_ir));
    Some(out)
}

fn fresh_value_local(func: &FuncDef) -> String {
    assert!(!multi(func), "{}: a peek frame's early exit has no multi-output spelling", func.name);
    format!("cur_{}", func.outputs[0].name)
}

fn settings_snapshot(step_settings: &BTreeSet<String>) -> Vec<String> {
    let mut out = Vec::new();
    for s in step_settings {
        out.push(format!("{s}_rangeType := sp.cs_{s}_rangeType"));
        out.push(format!("{s}_avgPeriod := sp.cs_{s}_avgPeriod"));
        out.push(format!("{s}_factor := sp.cs_{s}_factor"));
    }
    out
}

fn identity_branch_as_frame(func: &FuncDef, model: &StreamModel) -> Option<Vec<Statement>> {
    let st = streaming::identity_peek_branch(model, &GoNames)?;
    let bare: HashMap<String, String> =
        func.outputs.iter().map(|o| (format!("sp.cur_{}", o.name), format!("cur_{}", o.name))).collect();
    Some(streaming::rewrite_stmts(
        std::slice::from_ref(&st),
        &|e| match e {
            Expr::Var(ref v) => bare.get(v).map_or(e, |b| Expr::Var(b.clone())),
            other => other,
        },
        &|s| match s {
            Statement::Return { value: None } => Some(Statement::Return { value: Some(Expr::Var(fresh_value_local(func))) }),
            other => Some(other),
        },
    ))
}

fn step_sig(em: &Em, fallible: bool) -> String {
    let bs = bar_sig(em.func);
    let sep = if bs.is_empty() { "" } else { ", " };
    let ret = if fallible { " error" } else { "" };
    format!("func {}(sp *{}{sep}{bs}){ret}", em.names.step_impl, em.names.stream)
}

/// `c := sp.core` ahead of a frame that reaches the `Core`.
fn with_core(lines: Vec<String>) -> Vec<String> {
    let reads = lines.iter().any(|l| {
        let code = l.split("//").next().unwrap_or("");
        let b = code.as_bytes();
        code.match_indices("c.").any(|(i, _)| i == 0 || !(b[i - 1].is_ascii_alphanumeric() || b[i - 1] == b'_' || b[i - 1] == b'.'))
    });
    if reads {
        let mut v = vec!["c := sp.core".to_string()];
        v.extend(lines);
        v
    } else {
        lines
    }
}

fn emit_step(o: &mut String, em: &Em, model: &StreamModel, fields: &[Field], step_settings: &BTreeSet<String>) {
    let lines = step_body(em, model, fields, step_settings, None);
    o.push_str(&go::func_text(&step_sig(em, false), &with_core(lines)));
}

fn step_body(
    em: &Em,
    model: &StreamModel,
    fields: &[Field],
    step_settings: &BTreeSet<String>,
    tape: Option<&streaming::TapeNames>,
) -> Vec<String> {
    let mut out = Vec::new();
    for (name, ty) in &streaming::step_temps(model, tape) {
        out.push(local_decl(name, ty));
    }
    out.extend(settings_snapshot(step_settings));
    let tape_names = tape.map(|t| GoTapeNames(t.clone()));
    let names: &dyn streaming::NameMap = match &tape_names {
        Some(t) => t,
        None => &GoNames,
    };
    let transition = streaming::build_transition(model, names).unwrap_or_else(|e| panic!("streaming transition: {e}"));
    assert!(tape.is_none() || !has_return(&transition), "{}: a tape step cannot render an early exit", model.func.name);
    let ctx = em.frame_ctx(&transition, fields, Some(model), step_settings, tape.is_some(), Ret::Code);
    out.extend(render(&ctx, &transition));
    out
}

// ---------------------------------------------------------------------------
// Tape entries: the step, peek and detach a period bank drives each slot
// through, every slot reading its price history from the bank's one tape.
// ---------------------------------------------------------------------------

const TAPE_PARAMS: &str = "tape []float64, tapeBase, tapeMask int32";

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

fn tape_names(em: &Em) -> (String, String, String) {
    (format!("{}StepTape", em.camel), format!("{}PeekTape", em.camel), format!("{}TapeDetach", em.camel))
}

#[allow(clippy::too_many_lines)]
fn emit_tape_entries(o: &mut String, em: &Em, tier: &TapeTier) {
    let func = em.func;
    let st = &em.names.stream;
    let input = tape_input(func);
    let tape = streaming::TapeNames::new(&input);
    let (step_tape, peek_tape, detach_fn) = tape_names(em);
    let mut detach: Vec<(String, String, bool)> = Vec::new();
    for m in tier.models {
        streaming::check_tape_eligible(m, &input).unwrap_or_else(|e| panic!("{e}"));
        let n = &GoNames;
        for r in streaming::tape_covered_rings(m, &input) {
            detach.push((streaming::NameMap::ring_buf(n, &r.var, &input), streaming::NameMap::ring_cap(n, &r.var), false));
        }
        for w in streaming::tape_covered_windows(m, &input) {
            detach.push((streaming::NameMap::win_buf(n, &w.var, &input), streaming::NameMap::win_cap(n, &w.var), true));
        }
    }
    let mut seen = BTreeSet::new();
    detach.retain(|d| seen.insert(d.clone()));
    let pred = tier.dual.map(|d| {
        let p = params_on_state(func, &d.predicate);
        let ctx = em.frame_ctx(&[], tier.fields, None, tier.step_settings, false, Ret::Code);
        cond_text(&ctx, &p)
    });
    let in_sig = format!("{} float64", go_ident(&input));
    let in_arg = go_ident(&input);
    let single = !multi(func);

    let mut b = B::default();
    if detach.is_empty() {
        b.line(format!("{}(sp, {in_arg})", em.names.step_impl));
    } else if let Some(pred) = &pred {
        b.open(format!("if {pred} {{"));
        b.extend(step_body(em, tier.models[0], tier.fields, tier.step_settings, Some(&tape)));
        b.mid("} else {");
        b.extend(step_body(em, tier.models[1], tier.fields, tier.step_settings, Some(&tape)));
        b.close("}");
    } else {
        b.extend(step_body(em, tier.models[0], tier.fields, tier.step_settings, Some(&tape)));
    }
    b.line("sp.outRangeCount++");
    let ret = if single {
        b.line(format!("return sp.cur_{}", func.outputs[0].name));
        format!(" {}", scalar_ty(out_ctype(func, &func.outputs[0].name)))
    } else {
        String::new()
    };
    o.push_str(&go::func_text(&format!("func {step_tape}(sp *{st}, {TAPE_PARAMS}, {in_sig}){ret}"), &with_core(b.lines)));

    let mut b = B::default();
    if detach.is_empty() {
        b.line(format!("return sp.Peek({in_arg})"));
    } else {
        let names = GoTapeNames(tape.clone());
        let frame = |model: &StreamModel, predeclared: &BTreeSet<String>| {
            peek_frame(em, model, &names, tier.fields, tier.step_settings, predeclared)
                .unwrap_or_else(|| panic!("{}: no tape peek frame", func.name))
        };
        if let Some(pred) = &pred {
            let outs: BTreeSet<String> = func.outputs.iter().map(|x| format!("cur_{}", x.name)).collect();
            for out in &func.outputs {
                b.line(format!("var cur_{} {}", out.name, scalar_ty(out_ctype(func, &out.name))));
            }
            b.open(format!("if {pred} {{"));
            b.extend(frame(tier.models[0], &outs));
            b.mid("} else {");
            b.extend(frame(tier.models[1], &outs));
            b.close("}");
        } else {
            b.extend(frame(tier.models[0], &BTreeSet::new()));
        }
        b.line(format!("return {}, nil", cur_list(func, "")));
    }
    o.push_str(&go::func_text(
        &format!("func {peek_tape}(sp *{st}, {TAPE_PARAMS}, {in_sig}) {}", results(&value_types(func), true)),
        &with_core(b.lines),
    ));

    let mut b = B::default();
    if detach.is_empty() {
        b.line("return 0");
    } else {
        b.line("reach := int32(0)");
        for (buf, cap, window) in &detach {
            b.line(format!("{buf} = nil"));
            if *window {
                b.open(format!("if {cap}-1 > reach {{"));
                b.line(format!("reach = {cap} - 1"));
            } else {
                b.open(format!("if {cap} > reach {{"));
                b.line(format!("reach = {cap}"));
            }
            b.close("}");
        }
        b.line("return reach");
    }
    o.push_str(&go::func_text(&format!("func {detach_fn}(sp *{st}) int32"), &b.lines));
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
    let open_body = cleanup_open_body(&build_open_body_java(model, body), em);
    let ctx = em.open_ctx(&open_body);
    let mut b = B::default();
    emit_body_decls(&mut b, &open_body);
    emit_open_head(&mut b, em);
    emit_open_validation(&mut b, em, OutMode::Core);
    emit_anchor_guard(&mut b);
    emit_extras_and_candle(&mut b, em, &open_body, &ctx);
    emit_identity_fast_path(&mut b, em, model, fields, &ctx);
    emit_open_region(&mut b, &open_body, &ctx, &[], &BTreeSet::new());
    emit_capture(&mut b, em, model, &model.state, step_settings, &ctx, Some(CurSource::StridedArray), &[]);
    b.line("return Success");
    o.push_str(&go::func_text(&open_sig(em, OutMode::Core), &b.lines));
}

fn open_sig(em: &Em, mode: OutMode) -> String {
    let func = em.func;
    let mut params: Vec<(String, String)> = vec![("sp".to_string(), format!("*{}", em.names.stream))];
    for input in bars(func) {
        params.push((go_ident(&input), "[]float64".to_string()));
    }
    if matches!(mode, OutMode::Scalar | OutMode::Core | OutMode::FillInternal) {
        params.push(("startIdx".to_string(), "int32".to_string()));
    }
    for p in &func.optional_inputs {
        params.push((go_ident(&p.name), go::impl_opt_type(p).to_string()));
    }
    let name = match mode {
        OutMode::Scalar | OutMode::Core => em.names.open_impl.clone(),
        OutMode::Fill => format!("{}OpenAndFillImpl", em.camel),
        OutMode::FillInternal => format!("{}OpenAndFillInternalImpl", em.camel),
    };
    if mode != OutMode::Scalar {
        params.push(("outBegIdx".to_string(), "*int32".to_string()));
        params.push(("outNBElement".to_string(), "*int32".to_string()));
        for out in &func.outputs {
            params.push((go_ident(&out.name), slice_ty(out_ctype(func, &out.name)).to_string()));
        }
    }
    if mode == OutMode::Core {
        params.push(("outStride".to_string(), "int32".to_string()));
    }
    format!("func (c *Core) {name}({}) RetCode", go::param_list(&params))
}

fn emit_anchor_guard(b: &mut B) {
    b.open("if startIdx >= historyLen {");
    b.line("*outBegIdx = 0");
    b.line("*outNBElement = 0");
    b.line("return InsufficientHistory");
    b.close("}");
}

fn emit_body_decls(b: &mut B, open_body: &[Statement]) {
    for stmt in open_body {
        if let Statement::CircBuf(CircBuf::Prolog { id, layout, static_size }) = stmt {
            for (arr, t) in super::java::circbuf_arrays(id, layout) {
                b.line(format!("var {} {}", go_ident(&arr), go::circbuf_type(&t)));
            }
            b.line(format!("var {id}_Idx int32"));
            b.line(format!("var maxIdx_{id} int32"));
            b.line(format!("maxIdx_{id} = {}", static_size - 1));
            continue;
        }
        if let Statement::VarDecl { var_type, name, .. } = stmt {
            b.line(format!("var {} {}", go_ident(name), go::var_go_type(var_type)));
        }
    }
}

fn history(em: &Em) -> String {
    go_ident(&bars(em.func)[0])
}

/// The history length, checked against the index domain before it narrows.
fn emit_open_head(b: &mut B, em: &Em) {
    let h = history(em);
    b.guard(format!("len({h}) > IndexMax+1"), "return OutOfRangeEndIndex");
    b.line(format!("historyLen := int32(len({h}))"));
    b.line("endIdx := historyLen - 1");
}

fn alias_pairs(func: &FuncDef) -> Vec<(String, String)> {
    let mut pairs = Vec::new();
    for out in &func.outputs {
        for i in &bars(func) {
            if out_ctype(func, &out.name) == CType::Real {
                pairs.push((go_ident(&out.name), go_ident(i)));
            }
        }
    }
    for (k, a) in func.outputs.iter().enumerate() {
        for b in &func.outputs[k + 1..] {
            if out_ctype(func, &a.name) == out_ctype(func, &b.name) {
                pairs.push((go_ident(&a.name), go_ident(&b.name)));
            }
        }
    }
    pairs
}

/// Every output against every input and every other output of its type: one
/// start, refused outright, and any overlap once the history is long enough
/// to fill, so a short history answers `InsufficientHistory` first.
fn alias_conditions(func: &FuncDef) -> Option<(String, String)> {
    let pairs = alias_pairs(func);
    if pairs.is_empty() {
        return None;
    }
    let same: Vec<String> = pairs.iter().map(|(a, b)| format!("sameStart({a}, {b})")).collect();
    let part: Vec<String> = pairs.iter().map(|(a, b)| format!("overlaps({a}, {b})")).collect();
    Some((same.join(" || "), paren_or(&part)))
}

/// `a || b` as one operand of `&&`: parenthesized only when it is several.
fn paren_or(terms: &[String]) -> String {
    if terms.len() == 1 {
        terms[0].clone()
    } else {
        format!("({})", terms.join(" || "))
    }
}

fn emit_open_validation(b: &mut B, em: &Em, mode: OutMode) {
    let func = em.func;
    let inputs: Vec<String> = bars(func).iter().map(|x| go_ident(x)).collect();
    let first = &inputs[0];
    let mismatches: Vec<String> = inputs[1..].iter().map(|x| format!("len({x}) != len({first})")).collect();
    if !mismatches.is_empty() {
        b.guard(mismatches.join(" || "), "return BadParam");
    }
    b.extend(go::opt_validation(func, "BadParam", em.svc.enums));
    if mode == OutMode::Fill {
        if let Some((same, partial)) = alias_conditions(func) {
            b.guard(same, "return BadParam");
            b.guard(format!("historyLen > {} && {partial}", em.lookback_call("c")), "return BadParam");
        }
    }
}

fn emit_extras_and_candle(b: &mut B, em: &Em, open_body: &[Statement], ctx: &Ctx) {
    let func = em.func;
    for (name, _) in &func.private_extra_params {
        let init = func
            .private_param_init
            .iter()
            .find(|(pn, _)| pn == name)
            .map_or_else(|| panic!("{}: no init for private param {name}", func.name), |(_, e)| ctx.pure(e, None).render());
        b.line(format!("{} := {init}", go_ident(name)));
    }
    b.extend(go::emit_candle_unpacking(&detect_candle_settings(open_body)));
}

/// The transcribed region: initializers, then the statements, with a tier's
/// inserts spliced before the statement at their index.
fn emit_open_region(b: &mut B, open_body: &[Statement], ctx: &Ctx, inserts: &[(usize, Vec<String>)], replaced: &BTreeSet<usize>) {
    let mut init = Vec::new();
    ctx.initializers(open_body, &mut init);
    b.extend(init);
    for (i, stmt) in open_body.iter().enumerate() {
        for (at, text) in inserts {
            if *at == i {
                b.extend(text.iter().cloned());
            }
        }
        if matches!(stmt, Statement::VarDecl { .. }) || replaced.contains(&i) {
            continue;
        }
        let mut lines = Vec::new();
        ctx.stmt(stmt, &mut lines);
        b.extend(lines);
    }
    for (at, text) in inserts {
        if *at == open_body.len() {
            b.extend(text.iter().cloned());
        }
    }
}

/// The parameter-degenerate identity path: the whole range at stride 1, the
/// last bar alone at stride 0.
fn emit_identity_fast_path(b: &mut B, em: &Em, model: &StreamModel, fields: &[Field], ctx: &Ctx) {
    let Some(idp) = &model.identity else { return };
    b.open(format!("if {} {{", cond_text(ctx, &idp.condition)));
    b.line(format!("fillLb := {}", em.lookback_call("c")));
    b.guard("startIdx > fillLb", "fillLb = startIdx");
    b.guard("historyLen < fillLb+1", "return InsufficientHistory");
    for f in fields {
        if f.name.starts_with("cur_") {
            continue;
        }
        if model.parity.as_ref().is_some_and(|p| p.field == f.name) {
            b.line(format!("sp.{} = historyLen % 2", f.name));
        } else {
            b.line(format!("sp.{} = {}", f.name, f.default));
        }
    }
    b.line("*outBegIdx = fillLb");
    b.line("*outNBElement = historyLen - fillLb");
    b.open("if outStride == 0 {");
    for (out, inp) in &idp.pairs {
        b.line(format!("{}[0] = {}[historyLen-1]", go_ident(out), go_ident(inp)));
    }
    b.mid("} else {");
    b.open("for fillIdx := int32(0); fillIdx < historyLen-fillLb; fillIdx++ {");
    for (out, inp) in &idp.pairs {
        b.line(format!("{}[fillIdx] = {}[fillLb+fillIdx]", go_ident(out), go_ident(inp)));
    }
    b.close("}");
    b.close("}");
    for (out, _) in &idp.pairs {
        b.line(format!("sp.cur_{out} = {}", last_strided(&go_ident(out))));
    }
    b.line("return Success");
    b.close("}");
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
    b: &mut B,
    em: &Em,
    model: &StreamModel,
    scalars: &[(String, VarType)],
    step_settings: &BTreeSet<String>,
    ctx: &Ctx,
    cur_source: Option<CurSource>,
    extra_capture: &[String],
) {
    emit_capture_buffers(b, model, ctx);
    emit_capture_stores(b, em, model, scalars, step_settings);
    b.extend(extra_capture.iter().cloned());
    if let Some(cs) = cur_source {
        emit_cur_capture(b, &model.outputs, cs);
    }
}

/// Each buffer's capacity from the still-live batch locals, and the buffer.
#[allow(clippy::too_many_lines)]
fn emit_capture_buffers(b: &mut B, model: &StreamModel, ctx: &Ctx) {
    let c = go_ident(&model.cursor);
    for ring in model.rings() {
        let v = &ring.var;
        let gv = go_ident(v);
        let back = ring.back;
        if back > 0 {
            b.line(format!("capLag_{v} := {c} - {gv}"));
            b.line(format!("cap_{v} := capLag_{v} + {}", back + 1));
            let mut served = String::new();
            for w in &ring.served {
                served.push_str(&format!(" || cap_{v} < {}", int_g(ctx, &w.cap).render_at(2)));
            }
            b.open(format!("if capLag_{v} < {} || cap_{v} > historyLen{served} {{", ring.fwd));
        } else {
            b.line(format!("cap_{v} := {c} - {gv}"));
            b.open(format!("if cap_{v} < {} || cap_{v} > historyLen {{", i32::from(ring.lag_ge1)));
        }
        b.line("return InternalError");
        b.close("}");
        for arr in &ring.arrays {
            let ga = go_ident(arr);
            let buf = format!("capRing_{v}_{arr}");
            b.line(format!("{buf} := make([]float64, max(cap_{v}, 1))"));
            let fill_rhs = ring.derived.as_ref().map(|dr| ctx.pure(&streaming::derived_fill_value(dr, "fillJ"), Some(CType::Real)).render());
            let from = bin(name_g("historyLen"), GOp::Sub, name_g(&format!("cap_{v}")));
            if back > 0 {
                let rhs = fill_rhs.clone().unwrap_or_else(|| format!("{ga}[fillJ]"));
                b.open(format!("for fillJ := historyLen - cap_{v}; fillJ < historyLen; fillJ++ {{"));
                b.line(format!("{buf}[fillJ%cap_{v}] = {rhs}"));
                b.close("}");
            } else if let Some(rhs) = fill_rhs {
                b.open(format!("for fillJ := historyLen - cap_{v}; fillJ < historyLen; fillJ++ {{"));
                let idx = bin(name_g("fillJ"), GOp::Sub, from.paren());
                b.line(format!("{} = {rhs}", GExpr::index(name_g(&buf), idx).render()));
                b.close("}");
            } else {
                b.line(GExpr::call("copy", vec![name_g(&buf), GExpr::slice(name_g(&ga), Some(from), Some(name_g("historyLen")))]).render());
            }
        }
    }
    for win in model.windows() {
        let v = &win.var;
        let cap = int_g(ctx, &win.cap).render();
        let cap = if cap.parse::<i64>().is_ok() { format!("int32({cap})") } else { cap };
        b.line(format!("cap_{v} := {cap}"));
        b.guard(format!("cap_{v} < 1 || cap_{v} > historyLen"), "return InternalError");
        for arr in &win.arrays {
            let buf = format!("capWin_{v}_{arr}");
            let from = bin(name_g("historyLen"), GOp::Sub, name_g(&format!("cap_{v}")));
            b.line(format!("{buf} := make([]float64, cap_{v})"));
            b.line(GExpr::call("copy", vec![name_g(&buf), GExpr::slice(name_g(&go_ident(arr)), Some(from), Some(name_g("historyLen")))]).render());
        }
    }
    if let Some(ex) = model.extrema() {
        let t = go_ident(&ex.trailing);
        b.line(format!("capX := {c} - {t} + 1"));
        b.guard("capX < 1 || capX > historyLen", "return InternalError");
        // A power of two, so `idx & xMask` maps any capX consecutive bars apart.
        b.line("physX := int32(1)");
        b.open("for physX < capX {");
        b.line("physX <<= 1");
        b.close("}");
        for arr in &ex.arrays {
            b.line(format!("capX_{arr} := make([]float64, physX)"));
        }
        b.open("for fillJ := historyLen - capX; fillJ < historyLen; fillJ++ {");
        for arr in &ex.arrays {
            b.line(format!("capX_{arr}[fillJ&(physX-1)] = {}[fillJ]", go_ident(arr)));
        }
        b.close("}");
    }
    for circ in model.circs() {
        let id = &circ.id;
        b.line(format!("capCb_{id} := maxIdx_{id} + 1"));
        b.guard(format!("capCb_{id} > historyLen+1"), "return InternalError");
    }
}

/// Every handle field.
fn emit_capture_stores(b: &mut B, em: &Em, model: &StreamModel, scalars: &[(String, VarType)], step_settings: &BTreeSet<String>) {
    let func = em.func;
    for p in &func.optional_inputs {
        b.line(format!("sp.{} = {}", p.name, go_ident(&p.name)));
    }
    for (name, _) in &func.private_extra_params {
        b.line(format!("sp.{name} = {}", go_ident(name)));
    }
    for (name, _) in scalars {
        if model.parity.as_ref().is_some_and(|p| &p.field == name) {
            b.line(format!("sp.{name} = historyLen % 2"));
        } else {
            b.line(format!("sp.{name} = {}", go_ident(name)));
        }
    }
    for name in &model.out_feedback {
        b.line(format!("sp.lastOut_{name} = {}", last_strided(&go_ident(name))));
    }
    for lag in &model.lags {
        for k in 1..=lag.depth {
            b.line(format!("sp.{} = {}[historyLen-{k}]", StreamModel::lag_field(&lag.array, k), go_ident(&lag.array)));
        }
    }
    for ring in model.rings() {
        let v = &ring.var;
        if ring.back > 0 {
            b.line(format!("sp.ringPos_{v} = historyLen % cap_{v}"));
            b.line(format!("sp.ringCap_{v} = cap_{v}"));
            b.line(format!("sp.ringLag_{v} = capLag_{v}"));
        } else {
            b.line(format!("sp.ringPos_{v} = 0"));
            b.line(format!("sp.ringCap_{v} = cap_{v}"));
        }
        for arr in &ring.arrays {
            b.line(format!("sp.ring_{v}_{arr} = capRing_{v}_{arr}"));
        }
    }
    for win in model.windows() {
        let v = &win.var;
        b.line(format!("sp.winPos_{v} = 0"));
        b.line(format!("sp.winCap_{v} = cap_{v}"));
        for arr in &win.arrays {
            b.line(format!("sp.win_{v}_{arr} = capWin_{v}_{arr}"));
        }
    }
    for circ in model.circs() {
        let id = &circ.id;
        b.line(format!("sp.cbSize_{id} = capCb_{id}"));
        for (storage, _) in streaming::circ_storages(circ) {
            b.line(format!("sp.cb_{storage} = {}", go_ident(&storage)));
        }
    }
    if let Some(ex) = model.extrema() {
        b.line("sp.xMask = physX - 1");
        for arr in &ex.arrays {
            b.line(format!("sp.x_{arr} = capX_{arr}"));
        }
    }
    for s in step_settings {
        b.line(format!("sp.cs_{s}_rangeType = {s}_rangeType"));
        b.line(format!("sp.cs_{s}_avgPeriod = {s}_avgPeriod"));
        b.line(format!("sp.cs_{s}_factor = {s}_factor"));
    }
}

fn emit_cur_capture(b: &mut B, outputs: &[String], source: CurSource) {
    for out in outputs {
        let expr = match source {
            CurSource::StridedArray => last_strided(&go_ident(out)),
            CurSource::Scratch => format!("sc_{out}[*outNBElement-1]"),
        };
        b.line(format!("sp.cur_{out} = {expr}"));
    }
}

// ---------------------------------------------------------------------------
// The seams and the public openers
// ---------------------------------------------------------------------------

fn internal_in_params(func: &FuncDef) -> Vec<(String, String)> {
    bars(func).iter().map(|i| (go_ident(i), "[]float64".to_string())).collect()
}

fn impl_opt_params(func: &FuncDef) -> Vec<(String, String)> {
    func.optional_inputs.iter().map(|p| (go_ident(&p.name), go::impl_opt_type(p).to_string())).collect()
}

fn opt_fwd(func: &FuncDef) -> Vec<String> {
    func.optional_inputs.iter().map(|p| go_ident(&p.name)).collect()
}

fn new_handle(em: &Em) -> String {
    format!("sp := &{}{{core: c}}", em.names.stream)
}

fn emit_open_internal_seam(o: &mut String, em: &Em, merged: bool, last_bar_only: bool) {
    let func = em.func;
    let st = &em.names.stream;
    let mut params = internal_in_params(func);
    params.push(("startIdx".to_string(), "int32".to_string()));
    params.extend(impl_opt_params(func));
    let h = history(em);
    let mut b = B::default();
    b.line(new_handle(em));
    let mut args: Vec<String> = vec!["sp".to_string()];
    args.extend(bars(func).iter().map(|x| go_ident(x)));
    if last_bar_only {
        b.line("anchor := startIdx");
        b.guard(format!("n := len({h}); n <= IndexMax+1 && int(startIdx)+1 < n"), "anchor = int32(n - 1)");
        args.push("anchor".to_string());
    } else {
        args.push("startIdx".to_string());
    }
    args.extend(opt_fwd(func));
    if merged {
        b.line("var outBegIdx, outNBElement int32");
        args.push("&outBegIdx".to_string());
        args.push("&outNBElement".to_string());
        for out in &func.outputs {
            b.line(format!("var sink_{} [1]{}", out.name, scalar_ty(out_ctype(func, &out.name))));
            args.push(format!("sink_{}[:]", out.name));
        }
        args.push("0".to_string());
    }
    b.guard(format!("retCode := c.{}({}); retCode != Success", em.names.open_impl, args.join(", ")), "return nil, retCode");
    if merged {
        if last_bar_only {
            b.line(format!("begIdx := {}", em.lookback_call("c")));
            b.guard("begIdx < startIdx", "begIdx = startIdx");
            b.line("sp.outRangeBegIdx = int(begIdx)");
            b.line(format!("sp.outRangeCount = len({h}) - sp.outRangeBegIdx"));
        } else {
            b.line("sp.outRangeBegIdx = int(outBegIdx)");
            b.line("sp.outRangeCount = int(outNBElement)");
        }
    }
    b.line("return sp, nil");
    o.push_str(&go::func_text(
        &format!("func (c *Core) {}({}) (*{st}, error)", em.names.open_internal, go::param_list(&params)),
        &b.lines,
    ));
}

/// The public parameters, as the batch call takes them.
fn public_params(func: &FuncDef, with_outputs: bool) -> Vec<(String, String)> {
    let mut params = internal_in_params(func);
    for p in &func.optional_inputs {
        params.push((go_ident(&p.name), go::public_opt_type(p)));
    }
    if with_outputs {
        for out in &func.outputs {
            params.push((go_ident(&out.name), slice_ty(out_ctype(func, &out.name)).to_string()));
        }
    }
    params
}

fn impl_args(func: &FuncDef) -> Vec<String> {
    func.optional_inputs
        .iter()
        .map(|p| match p.param_type {
            ParamType::Real => go_ident(&p.name),
            _ => format!("int32({})", go_ident(&p.name)),
        })
        .collect()
}

/// The checks a public opener runs ahead of the seam: each integer parameter
/// within `int32`, then for the fill the history's domain, the parameters,
/// every input's length and each output's capacity, then the aliasing rule.
fn emit_public_open_guards(b: &mut B, em: &Em, with_outputs: bool, fail: &str) {
    let func = em.func;
    for p in func.optional_inputs.iter().filter(|p| p.param_type == ParamType::Integer) {
        b.guard(format!("!fitsInt32({})", go_ident(&p.name)), format!("return {fail}, BadParam"));
    }
    if !with_outputs {
        return;
    }
    let h = history(em);
    b.guard(format!("len({h}) > IndexMax+1"), format!("return {fail}, OutOfRangeEndIndex"));
    b.line(format!("lookback := int({}({}))", em.lookback_impl(), impl_args(func).join(", ")));
    b.guard("lookback < 0", format!("return {fail}, BadParam"));
    for input in &bars(func)[1..] {
        b.guard(format!("len({}) != len({h})", go_ident(input)), format!("return {fail}, BadParam"));
    }
    b.line(format!("outLen := max(len({h})-lookback, 0)"));
    for out in &func.outputs {
        b.guard(format!("len({0}) == 0 || len({0}) < outLen", go_ident(&out.name)), format!("return {fail}, BadParam"));
    }
    let pairs = alias_pairs(func);
    if !pairs.is_empty() {
        let same: Vec<String> = pairs.iter().map(|(a, x)| format!("sameStart({a}, {x})")).collect();
        b.guard(same.join(" || "), format!("return {fail}, BadParam"));
        let part: Vec<String> = pairs.iter().map(|(a, x)| format!("overlaps({a}, {x})")).collect();
        b.guard(format!("outLen > 0 && {}", paren_or(&part)), format!("return {fail}, BadParam"));
    }
}

fn emit_open_wrappers(o: &mut String, em: &Em, merged: bool, last_bar_only: bool) {
    let func = em.func;
    let st = &em.names.stream;
    emit_open_internal_seam(o, em, merged, last_bar_only);

    let open_fail = format!("nil, {}", func.outputs.iter().map(|_| "0").collect::<Vec<_>>().join(", "));
    let mut b = B::default();
    emit_public_open_guards(&mut b, em, false, &open_fail);
    let mut args: Vec<String> = bars(func).iter().map(|x| go_ident(x)).collect();
    args.push("0".to_string());
    args.extend(impl_args(func));
    b.line(format!("sp, err := c.{}({})", em.names.open_internal, args.join(", ")));
    b.guard("err != nil", format!("return {open_fail}, err"));
    b.line(format!("return sp, {}, nil", cur_list(func, "sp.")));
    let mut ret: Vec<&str> = vec![];
    let star = format!("*{st}");
    ret.push(&star);
    ret.extend(value_types(func));
    doc_fn(
        o,
        &go_doc::open_docs(func),
        &format!("func (c *Core) {}({}) {}", em.names.open, go::param_list(&public_params(func, false)), results(&ret, true)),
        &b.lines,
    );

    let fill_fail = "nil, OutRange{}";
    let mut b = B::default();
    emit_public_open_guards(&mut b, em, true, fill_fail);
    b.line("var outBegIdx, outNBElement int32");
    let mut args: Vec<String> = bars(func).iter().map(|x| go_ident(x)).collect();
    if merged {
        args.push("0".to_string());
        args.extend(impl_args(func));
        args.push("&outBegIdx".to_string());
        args.push("&outNBElement".to_string());
        args.extend(func.outputs.iter().map(|x| go_ident(&x.name)));
        b.line(format!("sp, err := c.{}({})", em.names.open_and_fill_internal, args.join(", ")));
        b.guard("err != nil", format!("return {fill_fail}, err"));
    } else {
        b.line(new_handle(em));
        let mut fwd = vec!["sp".to_string()];
        fwd.extend(args);
        fwd.extend(impl_args(func));
        fwd.push("&outBegIdx".to_string());
        fwd.push("&outNBElement".to_string());
        fwd.extend(func.outputs.iter().map(|x| go_ident(&x.name)));
        b.guard(
            format!("retCode := c.{}OpenAndFillImpl({}); retCode != Success", em.camel, fwd.join(", ")),
            format!("return {fill_fail}, retCode"),
        );
        b.line("sp.outRangeBegIdx = int(outBegIdx)");
        b.line("sp.outRangeCount = int(outNBElement)");
    }
    b.line("return sp, OutRange{BegIdx: int(outBegIdx), NBElement: int(outNBElement)}, nil");
    doc_fn(
        o,
        &go_doc::open_and_fill_docs(func),
        &format!("func (c *Core) {}({}) (*{st}, OutRange, error)", em.names.open_and_fill, go::param_list(&public_params(func, true))),
        &b.lines,
    );
}

fn emit_open_and_fill_internal_wrapper(o: &mut String, em: &Em, merged: bool) {
    let func = em.func;
    let st = &em.names.stream;
    let mut params = internal_in_params(func);
    params.push(("startIdx".to_string(), "int32".to_string()));
    params.extend(impl_opt_params(func));
    params.push(("outBegIdx".to_string(), "*int32".to_string()));
    params.push(("outNBElement".to_string(), "*int32".to_string()));
    for out in &func.outputs {
        params.push((go_ident(&out.name), slice_ty(out_ctype(func, &out.name)).to_string()));
    }
    let mut b = B::default();
    b.line(new_handle(em));
    let mut args: Vec<String> = vec!["sp".to_string()];
    args.extend(bars(func).iter().map(|x| go_ident(x)));
    args.push("startIdx".to_string());
    args.extend(opt_fwd(func));
    args.push("outBegIdx".to_string());
    args.push("outNBElement".to_string());
    args.extend(func.outputs.iter().map(|x| go_ident(&x.name)));
    let callee = if merged {
        args.push("1".to_string());
        em.names.open_impl.clone()
    } else {
        format!("{}OpenAndFillInternalImpl", em.camel)
    };
    b.guard(format!("retCode := c.{callee}({}); retCode != Success", args.join(", ")), "return nil, retCode");
    b.line("sp.outRangeBegIdx = int(*outBegIdx)");
    b.line("sp.outRangeCount = int(*outNBElement)");
    b.line("return sp, nil");
    o.push_str(&go::func_text(
        &format!("func (c *Core) {}({}) (*{st}, error)", em.names.open_and_fill_internal, go::param_list(&params)),
        &b.lines,
    ));
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
    let dual_frame = peek_frame(em, ma, &GoNames, &fields, &step_settings, &outs)
        .zip(peek_frame(em, mb, &GoNames, &fields, &step_settings, &outs))
        .map(|(a, bb)| {
            let mut b = B::default();
            for out in &func.outputs {
                b.line(format!("var cur_{} {}", out.name, scalar_ty(out_ctype(func, &out.name))));
            }
            let ret = if multi(func) { Ret::Code } else { Ret::Value(out_ctype(func, &func.outputs[0].name)) };
            let ctx = em.frame_ctx(&[], &fields, Some(ma), &step_settings, false, ret);
            if let Some(ident) = identity_branch_as_frame(func, ma) {
                b.extend(render(&ctx, &ident));
            }
            let pred = cond_text(&ctx, &params_on_state(func, &dmp.predicate));
            b.open(format!("if {pred} {{"));
            b.extend(a);
            b.mid("} else {");
            b.extend(bb);
            b.close("}");
            PeekFrame::falls_through(b.lines)
        });
    emit_handle(o, em, &fields, &Members::default(), dual_frame.as_ref(), false);

    let mut b = B::default();
    {
        let ctx = em.frame_ctx(&[], &fields, Some(ma), &step_settings, false, Ret::Code);
        if let Some(s) = streaming::identity_step_branch(ma, &GoNames) {
            b.extend(render(&ctx, std::slice::from_ref(&s)));
        }
        let pred = cond_text(&ctx, &params_on_state(func, &dmp.predicate));
        b.open(format!("if {pred} {{"));
    }
    b.extend(step_body(em, ma, &fields, &step_settings, None));
    b.mid("} else {");
    b.extend(step_body(em, mb, &fields, &step_settings, None));
    b.close("}");
    o.push_str(&go::func_text(&step_sig(em, false), &with_core(b.lines)));

    let mut b = B::default();
    emit_open_head(&mut b, em);
    emit_open_validation(&mut b, em, OutMode::Core);
    {
        // The identity path is one property of the function; a context over
        // the prologue serves it and the predicate.
        let head_ctx = em.open_ctx(dmp.prologue);
        emit_identity_fast_path(&mut b, em, ma, &fields, &head_ctx);
        let pred = cond_text(&head_ctx, &dmp.predicate);
        b.open(format!("if {pred} {{"));
    }
    for (k, arm) in [ma, mb].into_iter().enumerate() {
        if k == 1 {
            b.mid("} else {");
        }
        let mut body: Vec<Statement> = dmp.prologue.to_vec();
        body.extend_from_slice(arm.body);
        body.extend_from_slice(dmp.epilogue);
        let open_body = cleanup_open_body(&build_open_body_java(arm, &body), em);
        let ctx = em.open_ctx(&open_body);
        emit_body_decls(&mut b, &open_body);
        emit_extras_and_candle(&mut b, em, &open_body, &ctx);
        emit_open_region(&mut b, &open_body, &ctx, &[], &BTreeSet::new());
        let (own, other) = if k == 0 { (&fields_a, &fields_b) } else { (&fields_b, &fields_a) };
        let own_names: BTreeSet<&String> = own.iter().map(|f| &f.name).collect();
        let complement: Vec<String> = other
            .iter()
            .filter(|f| !own_names.contains(&f.name) && f.ty.is_pointer())
            .map(|f| format!("sp.{} = {}", f.name, f.default))
            .collect();
        emit_capture(&mut b, em, arm, &union_scalars, &step_settings, &ctx, Some(CurSource::StridedArray), &complement);
        b.line("return Success");
    }
    b.close("}");
    o.push_str(&go::func_text(&open_sig(em, OutMode::Core), &b.lines));
    emit_open_and_fill_internal_wrapper(o, em, true);
    emit_open_wrappers(o, em, true, false);
    if em.svc.registry.in_tape_set(&func.name.to_lowercase()) {
        let tier = TapeTier { models: &[ma, mb], dual: Some(dmp), fields: &fields, step_settings: &step_settings };
        emit_tape_entries(o, em, &tier);
    }
}

// ---------------------------------------------------------------------------
// Dispatch tier (MA): a sub-stream picked by the stored enum parameter.
// ---------------------------------------------------------------------------

fn callee_names(em: &Em, callee: &str) -> GoStreamNames {
    go_stream_names(&em.svc.registry.name_of(callee))
}

fn callee_camel(em: &Em, callee: &str) -> String {
    common::camel_words(&em.svc.registry.name_of(callee))
}

fn callee_def<'r>(em: &'r Em, callee: &str) -> &'r FuncDef {
    em.svc.registry.def(callee).unwrap_or_else(|| panic!("no definition for `{callee}`"))
}

fn arm_tag(dp: &streaming::DispatchPlan, arm: &streaming::DispatchArm) -> usize {
    dp.arms.iter().filter(|a| a.supported).position(|a| a.label == arm.label).expect("a supported arm")
}

const IDENTITY_TAG: &str = "-1";

/// The callee's optional arguments, each converted to its slot in the
/// C-shaped tier.
fn callee_impl_opts(em: &Em, callee: &str, args: &[Expr], ctx: &Ctx) -> Vec<String> {
    let def = callee_def(em, callee);
    def.optional_inputs.iter().zip(args).map(|(o, a)| ctx.pure(a, Some(go::impl_param_ctype(o))).render_at(2)).collect()
}

/// The same arguments as the callee's public tier takes them.
fn callee_public_opts(em: &Em, callee: &str, args: &[Expr], ctx: &Ctx) -> Vec<String> {
    let def = callee_def(em, callee);
    def.optional_inputs
        .iter()
        .zip(args)
        .map(|(o, a)| {
            let g = ctx.pure(a, Some(go::impl_param_ctype(o))).render_at(2);
            match &o.param_type {
                ParamType::Real => g,
                ParamType::Enum(e) => format!("{e}({g})"),
                _ => format!("int({g})"),
            }
        })
        .collect()
}

/// `v0, _, err` for a call of `arm`'s callee: a name per forwarded slot.
fn arm_lhs(arm: &streaming::DispatchArm, err: &str) -> (Vec<String>, Vec<(usize, String)>) {
    let mut lhs = Vec::new();
    let mut fwd = Vec::new();
    for (i, slot) in arm.out_map.iter().enumerate() {
        match slot {
            streaming::OutSlot::Forward(k) => {
                let v = format!("v{i}");
                lhs.push(v.clone());
                fwd.push((*k, v));
            }
            streaming::OutSlot::Discard => lhs.push("_".to_string()),
        }
    }
    lhs.push(err.to_string());
    (lhs, fwd)
}

fn emit_dispatch(o: &mut String, em: &Em, dp: &streaming::DispatchPlan) {
    let func = em.func;
    let fields = base_fields(func);
    let ctx = em.frame_ctx(&[], &fields, None, &BTreeSet::new(), false, Ret::Code);
    let arms: Vec<&streaming::DispatchArm> = dp.arms.iter().filter(|a| a.supported).collect();
    let classes: Vec<String> = arms.iter().map(|a| callee_names(em, &a.callee).stream).collect();
    let mut union: Vec<String> = classes.clone();
    union.sort();
    union.dedup();
    let mut copy = vec!["switch sub := sp.sub.(type) {".to_string()];
    for cls in &union {
        copy.push(format!("case *{cls}:"));
        copy.push("\tcl.sub = sub.Clone()".to_string());
    }
    copy.push("}".to_string());
    let members = Members { decls: vec![("sub".to_string(), "any".to_string()), ("arm".to_string(), "int32".to_string())], copy };

    let frame = dispatch_peek_frame(em, dp, &arms, &classes);
    emit_handle(o, em, &fields, &members, Some(&frame), true);
    emit_dispatch_step(o, em, dp, &arms, &classes);
    for mode in [OutMode::Scalar, OutMode::Fill, OutMode::FillInternal] {
        emit_dispatch_open(o, em, dp, mode, &ctx);
    }
    emit_open_wrappers(o, em, false, false);
    emit_open_and_fill_internal_wrapper(o, em, false);
    if em.svc.registry.in_tape_set(&func.name.to_lowercase()) {
        emit_dispatch_tape(o, em, dp);
    }
}

fn dispatch_peek_frame(em: &Em, dp: &streaming::DispatchPlan, arms: &[&streaming::DispatchArm], classes: &[String]) -> PeekFrame {
    let func = em.func;
    let outputs: Vec<String> = func.outputs.iter().map(|x| x.name.clone()).collect();
    let args = bar_args(func);
    let terminal = func.outputs.len() == 1;
    let mut b = B::default();
    if !terminal {
        for out in &func.outputs {
            b.line(format!("var cur_{} {}", out.name, scalar_ty(out_ctype(func, &out.name))));
        }
    }
    b.line("switch sp.arm {");
    if let Some(idp) = &dp.identity {
        b.line(format!("case {IDENTITY_TAG}:"));
        b.depth += 1;
        if terminal {
            b.line(format!("return {}, nil", go_ident(&idp.pairs[0].1)));
        } else {
            for (out, inp) in &idp.pairs {
                b.line(format!("cur_{out} = {}", go_ident(inp)));
            }
        }
        b.depth -= 1;
    }
    for (arm, cls) in arms.iter().zip(classes) {
        b.line(format!("case {}:", arm_tag(dp, arm)));
        b.depth += 1;
        let call = format!("sp.sub.(*{cls}).Peek({args})");
        if terminal && arm.out_map.len() == 1 {
            b.line(format!("return {call}"));
        } else {
            let (lhs, fwd) = arm_lhs(arm, "err");
            b.line(format!("{} := {call}", lhs.join(", ")));
            if terminal {
                let (_, v) = fwd.first().unwrap_or_else(|| panic!("dispatch arm {} forwards no output", arm.label));
                b.line(format!("return {v}, err"));
            } else {
                b.guard("err != nil", fail_return(func, "err"));
                for (k, v) in &fwd {
                    b.line(format!("cur_{} = {v}", outputs[*k]));
                }
            }
        }
        b.depth -= 1;
    }
    b.line("default:");
    b.line(format!("\t{}", fail_return(func, "InternalError")));
    b.line("}");
    PeekFrame { lines: b.lines, terminal }
}

fn emit_dispatch_step(o: &mut String, em: &Em, dp: &streaming::DispatchPlan, arms: &[&streaming::DispatchArm], classes: &[String]) {
    let func = em.func;
    let outputs: Vec<String> = func.outputs.iter().map(|x| x.name.clone()).collect();
    let args = bar_args(func);
    let mut b = B::default();
    b.line("switch sp.arm {");
    if let Some(idp) = &dp.identity {
        b.line(format!("case {IDENTITY_TAG}:"));
        b.depth += 1;
        for (out, inp) in &idp.pairs {
            b.line(format!("sp.cur_{out} = {}", go_ident(inp)));
        }
        b.depth -= 1;
    }
    for (arm, cls) in arms.iter().zip(classes) {
        b.line(format!("case {}:", arm_tag(dp, arm)));
        b.depth += 1;
        let (lhs, fwd) = arm_lhs(arm, "err");
        b.line(format!("{} := sp.sub.(*{cls}).Update({args})", lhs.join(", ")));
        b.guard("err != nil", "return err");
        for (k, v) in &fwd {
            b.line(format!("sp.cur_{} = {v}", outputs[*k]));
        }
        b.depth -= 1;
    }
    b.line("}");
    b.line("return nil");
    o.push_str(&go::func_text(&step_sig(em, true), &b.lines));
}

#[allow(clippy::too_many_lines)]
fn emit_dispatch_open(o: &mut String, em: &Em, dp: &streaming::DispatchPlan, mode: OutMode, ctx: &Ctx) {
    let func = em.func;
    let outputs: Vec<String> = func.outputs.iter().map(|x| x.name.clone()).collect();
    let in_args = bar_args(func);
    let lb_call = em.lookback_call("c");
    let mut b = B::default();
    emit_open_head(&mut b, em);
    emit_open_validation(&mut b, em, mode);
    b.guard(format!("historyLen < {lb_call}+1"), "return InsufficientHistory");
    if let Some(idp) = &dp.identity {
        b.open(format!("if {} {{", cond_text(ctx, &idp.condition)));
        for p in &func.optional_inputs {
            b.line(format!("sp.{} = {}", p.name, go_ident(&p.name)));
        }
        b.line("sp.sub = nil");
        b.line(format!("sp.arm = {IDENTITY_TAG}"));
        match mode {
            OutMode::Core => unreachable!("dispatch tier is exempt from the merge"),
            OutMode::Scalar => {
                for (out, inp) in &idp.pairs {
                    b.line(format!("sp.cur_{out} = {}[historyLen-1]", go_ident(inp)));
                }
                b.line(format!("fillLb := {lb_call}"));
                b.guard("startIdx > fillLb", "fillLb = startIdx");
                b.guard("historyLen < fillLb+1", "return InsufficientHistory");
                b.line("sp.outRangeBegIdx = int(fillLb)");
                b.line("sp.outRangeCount = int(historyLen - fillLb)");
            }
            OutMode::Fill | OutMode::FillInternal => {
                b.line(format!("fillLb := {lb_call}"));
                if mode == OutMode::FillInternal {
                    b.guard("startIdx > fillLb", "fillLb = startIdx");
                    b.guard("historyLen < fillLb+1", "return InsufficientHistory");
                }
                b.line("*outBegIdx = fillLb");
                b.line("*outNBElement = historyLen - fillLb");
                b.open("for fillIdx := int32(0); fillIdx < historyLen-fillLb; fillIdx++ {");
                for (out, inp) in &idp.pairs {
                    b.line(format!("{}[fillIdx] = {}[fillLb+fillIdx]", go_ident(out), go_ident(inp)));
                }
                b.close("}");
                for (out, _) in &idp.pairs {
                    b.line(format!("sp.cur_{out} = {}[*outNBElement-1]", go_ident(out)));
                }
            }
        }
        b.line("return Success");
        b.close("}");
    }
    b.line(format!("switch {} {{", go_ident(&dp.param)));
    for arm in &dp.arms {
        let label = match crate::parser::enums::lookup_variant(&arm.label, em.svc.enums) {
            Some((e, v)) => format!("int32({e}{})", v.name),
            None => arm.label.clone(),
        };
        b.line(format!("case {label}:"));
        b.depth += 1;
        if !arm.supported {
            b.line("return BadParam");
            b.depth -= 1;
            continue;
        }
        let cn = callee_names(em, &arm.callee);
        match mode {
            OutMode::Core => unreachable!("dispatch tier is exempt from the merge"),
            OutMode::Scalar => {
                let mut a = vec![in_args.clone(), "startIdx".to_string()];
                a.extend(callee_impl_opts(em, &arm.callee, &arm.opt_args, ctx));
                b.line(format!("sub, err := c.{}({})", cn.open_internal, a.join(", ")));
                b.guard("err != nil", "return err.(RetCode)");
                b.line("sp.outRangeBegIdx = sub.outRangeBegIdx");
                b.line("sp.outRangeCount = sub.outRangeCount");
            }
            OutMode::Fill | OutMode::FillInternal => {
                let callee_outs = &callee_def(em, &arm.callee).outputs;
                let fill_outs: Vec<String> = arm
                    .out_map
                    .iter()
                    .enumerate()
                    .map(|(slot_idx, slot)| match slot {
                        streaming::OutSlot::Forward(k) => go_ident(&outputs[*k]),
                        streaming::OutSlot::Discard => {
                            let ty = match callee_outs.get(slot_idx).map(|o| &o.param_type) {
                                Some(ParamType::Integer) => "[]int32",
                                _ => "[]float64",
                            };
                            format!("make({ty}, historyLen)")
                        }
                    })
                    .collect();
                if mode == OutMode::FillInternal {
                    let mut a = vec![in_args.clone(), "startIdx".to_string()];
                    a.extend(callee_impl_opts(em, &arm.callee, &arm.opt_args, ctx));
                    a.push("outBegIdx".to_string());
                    a.push("outNBElement".to_string());
                    a.extend(fill_outs);
                    b.line(format!("sub, err := c.{}({})", cn.open_and_fill_internal, a.join(", ")));
                    b.guard("err != nil", "return err.(RetCode)");
                } else {
                    let mut a = vec![in_args.clone()];
                    a.extend(callee_public_opts(em, &arm.callee, &arm.opt_args, ctx));
                    a.extend(fill_outs);
                    b.line(format!("sub, r, err := c.{}({})", cn.open_and_fill, a.join(", ")));
                    b.guard("err != nil", "return err.(RetCode)");
                    b.line("*outBegIdx = int32(r.BegIdx)");
                    b.line("*outNBElement = int32(r.NBElement)");
                }
            }
        }
        b.line("sp.sub = sub");
        b.line(format!("sp.arm = {}", arm_tag(dp, arm)));
        let callee_out_names = em.svc.registry.callee_outputs(&arm.callee);
        for (i, slot) in arm.out_map.iter().enumerate() {
            if let streaming::OutSlot::Forward(k) = slot {
                b.line(format!("sp.cur_{} = sub.cur_{}", outputs[*k], callee_out_names[i]));
            }
        }
        b.depth -= 1;
    }
    b.line("default:");
    b.line("\treturn BadParam");
    b.line("}");
    for p in &func.optional_inputs {
        b.line(format!("sp.{} = {}", p.name, go_ident(&p.name)));
    }
    b.line("return Success");
    o.push_str(&go::func_text(&open_sig(em, mode), &b.lines));
}

fn emit_dispatch_tape(o: &mut String, em: &Em, dp: &streaming::DispatchPlan) {
    let func = em.func;
    assert!(!multi(func), "{}: a dispatcher's tape entries have no multi-output spelling", func.name);
    let st = &em.names.stream;
    let in_name = go_ident(&tape_input(func));
    let out = &func.outputs[0].name;
    let (step_tape, peek_tape, detach_fn) = tape_names(em);
    let identity = dp.identity.as_ref().map(|idp| go_ident(&idp.pairs[0].1));
    let arms: Vec<_> = dp.arms.iter().filter(|a| a.supported && !a.callee.is_empty()).collect();
    let tape_args = format!("tape, tapeBase, tapeMask, {in_name}");
    let sig_tail = format!("{TAPE_PARAMS}, {in_name} float64");

    let mut b = B::default();
    b.line("switch sp.arm {");
    if let Some(inp) = &identity {
        b.line(format!("case {IDENTITY_TAG}:"));
        b.line(format!("\tsp.cur_{out} = {inp}"));
    }
    for arm in &arms {
        let cc = callee_camel(em, &arm.callee);
        let cls = callee_names(em, &arm.callee).stream;
        b.line(format!("case {}:", arm_tag(dp, arm)));
        b.depth += 1;
        if arm.out_map.len() == 1 {
            b.line(format!("sp.cur_{out} = {cc}StepTape(sp.sub.(*{cls}), {tape_args})"));
        } else {
            let slot = arm
                .out_map
                .iter()
                .position(|s| matches!(s, streaming::OutSlot::Forward(_)))
                .unwrap_or_else(|| panic!("dispatch arm {} forwards no output", arm.label));
            let o_name = &em.svc.registry.callee_outputs(&arm.callee)[slot];
            b.line(format!("sub := sp.sub.(*{cls})"));
            b.line(format!("{cc}StepTape(sub, {tape_args})"));
            b.line(format!("sp.cur_{out} = sub.cur_{o_name}"));
        }
        b.depth -= 1;
    }
    b.line("}");
    b.line("sp.outRangeCount++");
    b.line(format!("return sp.cur_{out}"));
    o.push_str(&go::func_text(&format!("func {step_tape}(sp *{st}, {sig_tail}) float64"), &b.lines));

    let mut b = B::default();
    b.line("switch sp.arm {");
    if let Some(inp) = &identity {
        b.line(format!("case {IDENTITY_TAG}:"));
        b.line(format!("\treturn {inp}, nil"));
    }
    for arm in &arms {
        let cc = callee_camel(em, &arm.callee);
        let cls = callee_names(em, &arm.callee).stream;
        b.line(format!("case {}:", arm_tag(dp, arm)));
        b.depth += 1;
        let call = format!("{cc}PeekTape(sp.sub.(*{cls}), {tape_args})");
        if arm.out_map.len() == 1 {
            b.line(format!("return {call}"));
        } else {
            let (lhs, fwd) = arm_lhs(arm, "err");
            b.line(format!("{} := {call}", lhs.join(", ")));
            b.line(format!("return {}, err", fwd[0].1));
        }
        b.depth -= 1;
    }
    b.line("}");
    b.line("return 0, InternalError");
    o.push_str(&go::func_text(&format!("func {peek_tape}(sp *{st}, {sig_tail}) (float64, error)"), &b.lines));

    let mut b = B::default();
    b.line("switch sp.arm {");
    for arm in &arms {
        let cc = callee_camel(em, &arm.callee);
        let cls = callee_names(em, &arm.callee).stream;
        b.line(format!("case {}:", arm_tag(dp, arm)));
        b.line(format!("\treturn {cc}TapeDetach(sp.sub.(*{cls}))"));
    }
    b.line("}");
    b.line("return 0");
    o.push_str(&go::func_text(&format!("func {detach_fn}(sp *{st}) int32"), &b.lines));
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
    let cc = callee_camel(em, callee);
    let callee_lookback = format!("c.{}", go::go_names(&registry.name_of(callee)).lookback);
    let callee_public = registry.name_of(callee);
    let subty = cn.stream.clone();
    let callee_out0 = registry.callee_outputs(callee)[0].clone();
    let base = em.camel.clone();
    let st = em.names.stream.clone();
    let min = go_ident(&plan.min_param);
    let max = go_ident(&plan.max_param);
    let price = go_ident(&plan.price_input);
    let period = go_ident(&plan.period_input);
    let out = plan.output.as_str();
    let gout = go_ident(out);
    let matype = go_ident(&plan.matype_param);
    let callee_args = |per: &str, ty: &str, public: bool| -> String {
        plan.callee_opts
            .iter()
            .map(|a| match a {
                streaming::PeriodBankArg::Period => {
                    if public {
                        format!("int({per})")
                    } else {
                        per.to_string()
                    }
                }
                streaming::PeriodBankArg::MAType => {
                    if public {
                        format!("MAType({ty})")
                    } else {
                        ty.to_string()
                    }
                }
            })
            .collect::<Vec<_>>()
            .join(", ")
    };
    // Clamped as a double before it narrows, as C does: a period too large for
    // an int goes to the maximum.
    let clamp_to = |b: &mut B, var: &str, x: &str, h: &str| {
        b.line(format!("{var} := {h}{min}"));
        b.open(format!("if {x} > float64({h}{max}) {{"));
        b.line(format!("{var} = {h}{max}"));
        b.mid(format!("}} else if {x} >= float64({h}{min}) {{"));
        b.line(format!("{var} = int32({x})"));
        b.close("}");
    };
    let window_labels = registry.window_labels(&func.name.to_lowercase());
    let windowed = !window_labels.is_empty();
    let window_gate = |b: &mut B| {
        b.line(format!("window := {base}WindowMode({matype})"));
        b.guard(format!("{max}-{min}+1 < {}", crate::streaming::WINDOW_MIN_BAND), "window = false");
        b.open("if window {");
        b.open("for bankIdx := int32(0); bankIdx < nBank; bankIdx++ {");
        b.open(format!("if {callee_lookback}({}) > lookbackTotal {{", callee_args(&format!("{min}+bankIdx"), &matype, false)));
        b.line("window = false");
        b.line("break");
        b.close("}");
        b.close("}");
        b.close("}");
    };
    // Every slot's lookback is at most lookbackTotal, or the gate above
    // declines window mode, so the scratch never grows after Open.
    let store_window_handle = |b: &mut B| {
        for p in &func.optional_inputs {
            b.line(format!("sp.{} = {}", p.name, go_ident(&p.name)));
        }
        b.line("sp.bank = nil");
        b.line("sp.window = make([]float64, lookbackTotal+1)");
    };
    let open_bank = |b: &mut B, hist: &str, anchor: &str| {
        b.line(format!("bank := make([]*{subty}, 0, nBank)"));
        b.line("reach := int32(0)");
        b.open("for bankIdx := int32(0); bankIdx < nBank; bankIdx++ {");
        b.line(format!("slotSub, err := c.{}({hist}, {anchor}, {})", cn.open_internal, callee_args(&format!("{min}+bankIdx"), &matype, false)));
        b.guard("err != nil", "return err.(RetCode)");
        b.line("bank = append(bank, slotSub)");
        b.line(format!("slotReach := {cc}TapeDetach(slotSub)"));
        b.guard("slotReach > reach", "reach = slotReach");
        b.close("}");
    };
    let store_handle = |b: &mut B| {
        for p in &func.optional_inputs {
            b.line(format!("sp.{} = {}", p.name, go_ident(&p.name)));
        }
        b.line("sp.bank = bank");
    };

    let mut fields = base_fields(func);
    fields.push(field("tapeMask", CType::Int32, "0"));
    fields.push(field("tapePos", CType::Int32, "0"));
    fields.push(field("tape", CType::RealPtr, "nil"));
    let mut members = Members { decls: vec![("bank".to_string(), format!("[]*{subty}"))], copy: Vec::new() };
    members.copy.push(format!("cl.bank = make([]*{subty}, len(sp.bank))"));
    members.copy.push("for bankIdx, s := range sp.bank {".to_string());
    members.copy.push("\tcl.bank[bankIdx] = s.Clone()".to_string());
    members.copy.push("}".to_string());
    if windowed {
        // Scratch, not state: a clone takes its own.
        members.decls.push(("window".to_string(), "[]float64".to_string()));
        members.copy.push("cl.window = make([]float64, len(sp.window))".to_string());
    }

    let mut peek = B::default();
    clamp_to(&mut peek, "cp", &period, "sp.");
    peek.line(format!("slot := cp - sp.{min}"));
    peek.line(format!("var cur_{out} float64"));
    let tape_peek = |b: &mut B| {
        b.line(format!(
            "v, err := {cc}PeekTape(sp.bank[slot], sp.tape, ((sp.tapePos+1)&sp.tapeMask)+sp.tapeMask+1, sp.tapeMask, {price})"
        ));
        b.guard("err != nil", "return 0, err");
        b.line(format!("cur_{out} = v"));
    };
    if windowed {
        peek.open("if len(sp.bank) == 0 {");
        peek.line(format!("v, err := {base}EvalWindow(sp, {price}, cp)"));
        peek.guard("err != nil", "return 0, err");
        peek.line(format!("cur_{out} = v"));
        peek.mid("} else {");
        tape_peek(&mut peek);
        peek.close("}");
    } else {
        tape_peek(&mut peek);
    }
    emit_handle(o, em, &fields, &members, Some(&PeekFrame::falls_through(peek.lines)), true);

    let mut b = B::default();
    if windowed {
        b.open("if len(sp.bank) == 0 {");
        clamp_to(&mut b, "cp", &period, "sp.");
        b.line(format!("v, err := {base}EvalWindow(sp, {price}, cp)"));
        b.guard("err != nil", "return err");
        b.line("sp.tapePos = (sp.tapePos + 1) & sp.tapeMask");
        b.line(format!("sp.tape[sp.tapePos] = {price}"));
        b.line(format!("sp.cur_{out} = v"));
        b.line("return nil");
        b.close("}");
    }
    clamp_to(&mut b, "cp", &period, "sp.");
    b.line(format!("slot := cp - sp.{min}"));
    b.line("sp.tapePos = (sp.tapePos + 1) & sp.tapeMask");
    b.line(format!("sp.tape[sp.tapePos] = {price}"));
    b.line("tapeBase := sp.tapePos + sp.tapeMask + 1");
    b.open("for bankIdx, s := range sp.bank {");
    b.line(format!("subValue := {cc}StepTape(s, sp.tape, tapeBase, sp.tapeMask, {price})"));
    b.guard("int32(bankIdx) == slot", format!("sp.cur_{out} = subValue"));
    b.close("}");
    b.line("return nil");
    o.push_str(&go::func_text(&step_sig(em, true), &b.lines));

    // Strictly above: at size == reach the deepest read lands on the current
    // bar's slot and returns the wrong bar with no other symptom.
    let mut b = B::default();
    b.line("size := int32(1)");
    b.open("for size <= reach {");
    b.line("size <<= 1");
    b.close("}");
    b.line("sp.tape = make([]float64, size)");
    b.line("sp.tapeMask = size - 1");
    b.open("for bb := max(historyLen-size, 0); bb < historyLen; bb++ {");
    b.line(format!("sp.tape[bb&sp.tapeMask] = {price}[bb]"));
    b.close("}");
    b.line("sp.tapePos = (historyLen - 1) & sp.tapeMask");
    o.push_str(&go::func_text(&format!("func {base}TapeOpen(sp *{st}, {price} []float64, historyLen, reach int32)"), &b.lines));

    if windowed {
        let mut b = B::default();
        if window_labels.iter().any(|l| l == "*") {
            b.line("return true");
        } else {
            let labels: Vec<String> = window_labels
                .iter()
                .map(|l| match crate::parser::enums::lookup_variant(l, em.svc.enums) {
                    Some((e, v)) => format!("int32({e}{})", v.name),
                    None => l.clone(),
                })
                .collect();
            b.line("switch t {");
            b.line(format!("case {}:", labels.join(", ")));
            b.line("\treturn true");
            b.line("}");
            b.line("return false");
        }
        o.push_str(&go::func_text(&format!("func {base}WindowMode(t int32) bool"), &b.lines));
        let mut b = B::default();
        b.line(format!("lb := sp.core.{}({})", go::go_names(&callee_public).lookback, callee_args("cp", &format!("sp.{matype}"), false)));
        b.line("win := sp.window[:lb+1]");
        b.open("for i := int32(0); i < lb; i++ {");
        b.line("win[i] = sp.tape[(sp.tapePos+sp.tapeMask+2-lb+i)&sp.tapeMask]");
        b.close("}");
        b.line(format!("win[lb] = {price}"));
        b.line("var value [1]float64");
        b.guard(
            format!("_, err := sp.core.{callee_public}(int(lb), int(lb), win, {}, value[:]); err != nil", callee_args("cp", &format!("sp.{matype}"), true)),
            "return 0, err",
        );
        b.line("return value[0], nil");
        o.push_str(&go::func_text(&format!("func {base}EvalWindow(sp *{st}, {price} float64, cp int32) (float64, error)"), &b.lines));
    }

    let own_lb = em.lookback_call("c");
    let lb_args = callee_args(&max, &matype, false);
    let mut body = B::default();
    emit_open_head(&mut body, em);
    emit_open_validation(&mut body, em, OutMode::Scalar);
    body.guard(format!("{min} > {max}"), "return BadParam");
    body.guard(format!("historyLen < {own_lb}+1"), "return InsufficientHistory");
    // Every sub seeds at the shared max-period lookback, as batch does.
    body.line(format!("lookbackTotal := {callee_lookback}({lb_args})"));
    body.line("subStart := max(startIdx, lookbackTotal)");
    body.guard("historyLen < subStart+1", "return InsufficientHistory");
    body.line(format!("nBank := {max} - {min} + 1"));
    if windowed {
        window_gate(&mut body);
        body.open("if window {");
        clamp_to(&mut body, "cp", &format!("{period}[historyLen-1]"), "");
        store_window_handle(&mut body);
        body.line(format!("{base}TapeOpen(sp, {price}, historyLen, lookbackTotal)"));
        body.line("var out1 [1]float64");
        body.guard(
            format!("_, err := c.{callee_public}(int(historyLen-1), int(historyLen-1), {price}, {}, out1[:]); err != nil", callee_args("cp", &matype, true)),
            "return err.(RetCode)",
        );
        body.line(format!("sp.cur_{out} = out1[0]"));
        body.line("sp.outRangeBegIdx = int(subStart)");
        body.line("sp.outRangeCount = int(historyLen - subStart)");
        body.line("return Success");
        body.close("}");
    }
    open_bank(&mut body, &price, "subStart");
    clamp_to(&mut body, "cp", &format!("{period}[historyLen-1]"), "");
    store_handle(&mut body);
    body.line(format!("{base}TapeOpen(sp, {price}, historyLen, reach)"));
    body.line(format!("sp.cur_{out} = bank[cp-{min}].cur_{callee_out0}"));
    body.line("sp.outRangeBegIdx = int(subStart)");
    body.line("sp.outRangeCount = int(historyLen - subStart)");
    body.line("return Success");
    o.push_str(&go::func_text(&open_sig(em, OutMode::Scalar), &body.lines));

    let mut body = B::default();
    emit_open_head(&mut body, em);
    emit_open_validation(&mut body, em, OutMode::Fill);
    body.guard(format!("{min} > {max}"), "return BadParam");
    body.line(format!("lookbackTotal := {callee_lookback}({lb_args})"));
    body.guard("historyLen < lookbackTotal+1", "return InsufficientHistory");
    body.line(format!("nBank := {max} - {min} + 1"));
    if windowed {
        window_gate(&mut body);
        body.open("if window {");
        body.line("run := make([]float64, historyLen-lookbackTotal)");
        body.line("t1 := int32(0)");
        body.open("for t := lookbackTotal; t < historyLen; t = t1 + 1 {");
        clamp_to(&mut body, "cp", &format!("{period}[t]"), "");
        body.open("for t1 = t; t1+1 < historyLen; t1++ {");
        clamp_to(&mut body, "cp2", &format!("{period}[t1+1]"), "");
        body.guard("cp2 != cp", "break");
        body.close("}");
        body.guard(
            format!("_, err := c.{callee_public}(int(t), int(t1), {price}, {}, run); err != nil", callee_args("cp", &matype, true)),
            "return err.(RetCode)",
        );
        body.line(format!("copy({gout}[t-lookbackTotal:], run[:t1-t+1])"));
        body.close("}");
        store_window_handle(&mut body);
        body.line(format!("{base}TapeOpen(sp, {price}, historyLen, lookbackTotal)"));
        body.line("*outBegIdx = lookbackTotal");
        body.line("*outNBElement = historyLen - lookbackTotal");
        body.line(format!("sp.cur_{out} = {gout}[*outNBElement-1]"));
        body.line("return Success");
        body.close("}");
    }
    // Each sub seeds at the first output bar, then the step replays the rest.
    body.line(format!("seedPrefix := {price}[:lookbackTotal+1]"));
    open_bank(&mut body, "seedPrefix", "lookbackTotal");
    clamp_to(&mut body, "cp", &format!("{period}[lookbackTotal]"), "");
    store_handle(&mut body);
    body.line(format!("{base}TapeOpen(sp, seedPrefix, lookbackTotal+1, reach)"));
    body.line(format!("{gout}[0] = bank[cp-{min}].cur_{callee_out0}"));
    body.open("for t := lookbackTotal + 1; t < historyLen; t++ {");
    body.guard(format!("err := {}(sp, {price}[t], {period}[t]); err != nil", em.names.step_impl), "return err.(RetCode)");
    body.line(format!("{gout}[t-lookbackTotal] = sp.cur_{out}"));
    body.close("}");
    body.line("*outBegIdx = lookbackTotal");
    body.line("*outNBElement = historyLen - lookbackTotal");
    body.line(format!("sp.cur_{out} = {gout}[*outNBElement-1]"));
    body.line("return Success");
    o.push_str(&go::func_text(&open_sig(em, OutMode::Fill), &body.lines));

    emit_open_wrappers(o, em, false, false);
}

// ---------------------------------------------------------------------------
// Composed tier: a producer transition and a pipeline of owned sub-streams.
// ---------------------------------------------------------------------------

#[allow(clippy::too_many_arguments, clippy::too_many_lines)]
fn composed_step(
    em: &Em,
    cp: &streaming::ComposedPlan,
    step_settings: &BTreeSet<String>,
    outputs: &[String],
    fields: &[Field],
    frame: bool,
) -> Option<Vec<String>> {
    let func = em.func;
    let mut b = B::default();
    let cur_scalars = cp.cur_scalars();
    let cur_ty = |name: &str| out_ctype(func, name);
    let ret = if frame && !multi(func) { Ret::Value(out_ctype(func, &func.outputs[0].name)) } else if frame { Ret::Code } else { Ret::Fallible };
    let err_ret = |err: &str| if frame { fail_return(func, err) } else { format!("return {err}") };

    if let (Some(model), false) = (&cp.producer, frame) {
        for (name, ty) in &model.temps {
            b.line(local_decl(name, ty));
        }
    }
    for (name, ty) in &cp.map_temps {
        b.line(local_decl(name, ty));
    }
    for name in &cur_scalars {
        b.line(format!("var cur_{name} {}", scalar_ty(cur_ty(name))));
    }
    let (lag_reals, lag_ints) = streaming::lag_step_locals(cp, !frame);
    for name in &lag_reals {
        b.line(format!("var {} float64", go_ident(name)));
    }
    for name in &lag_ints {
        b.line(format!("var {} int32", go_ident(name)));
    }
    let mut aliased: Vec<String> = Vec::new();
    if frame {
        for out in outputs {
            if !cur_scalars.contains(out) {
                b.line(format!("var cur_{out} {}", scalar_ty(cur_ty(out))));
                aliased.push(out.clone());
            }
        }
    }
    if !cp.subs.is_empty() {
        b.line("var subErr error");
    }

    let declare_composed = |ctx: &Ctx| {
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
        for n in &lag_reals {
            ctx.env.declare(n, CType::Real);
        }
        for n in &lag_ints {
            ctx.env.declare(n, CType::Int32);
        }
    };

    if let Some(model) = &cp.producer {
        let names = GoComposedNames { series: cp.series.clone().expect("producer plan carries a series") };
        if frame {
            let declared: BTreeSet<String> = cur_scalars
                .iter()
                .chain(aliased.iter())
                .map(|n| format!("cur_{n}"))
                .chain(cp.map_temps.iter().map(|(n, _)| n.clone()))
                .chain(cp.map_state.iter().map(|st| st.name.clone()))
                .collect();
            b.extend(peek_frame_with(em, model, &names, fields, step_settings, &declared, Some(&declare_composed))?);
        } else {
            b.extend(settings_snapshot(step_settings));
            let transition = streaming::build_transition(model, &names).unwrap_or_else(|e| panic!("streaming transition: {e}"));
            let ctx = em.frame_ctx(&transition, fields, Some(model), step_settings, false, ret);
            declare_composed(&ctx);
            b.extend(render(&ctx, &transition));
        }
    }

    let verb = if frame { "Peek" } else { "Update" };
    let lag_names = GoComposedNames { series: cp.series.clone().unwrap_or_default() };
    let render_all = |b: &mut B, stmts: &[Statement]| {
        let ctx = em.frame_ctx(stmts, fields, cp.producer.as_ref(), step_settings, false, ret);
        declare_composed(&ctx);
        b.extend(render(&ctx, stmts));
    };
    render_all(&mut b, &streaming::lag_snapshots(cp, None, !frame));
    let mut state_loaded = false;
    for (si, step) in cp.steps.iter().enumerate() {
        match step {
            streaming::UpdateStep::Sub { sub_idx } => {
                let sub = &cp.subs[*sub_idx];
                let (loads, args) = streaming::lower_sub_sources(cp, *sub_idx, &lag_names);
                render_all(&mut b, &loads);
                let arg_str = args.iter().map(|a| go_ident(a)).collect::<Vec<_>>().join(", ");
                let mut lhs: Vec<String> = sub.dsts.iter().map(|d| format!("cur_{d}")).collect();
                lhs.push("subErr".to_string());
                b.line(format!("{} = sp.sub{sub_idx}.{verb}({arg_str})", lhs.join(", ")));
                b.guard("subErr != nil", err_ret("subErr"));
            }
            streaming::UpdateStep::Align { .. } => {
                render_all(&mut b, &streaming::lower_align_step(cp, si, &lag_names));
            }
            streaming::UpdateStep::Map { .. } => {
                if !std::mem::replace(&mut state_loaded, true) {
                    for st in &cp.map_state {
                        b.line(format!("{} := sp.{}", go_ident(&st.name), st.name));
                    }
                }
                let stmts = streaming::lower_map_step(cp, si, &lag_names);
                let ctx = em.frame_ctx(&stmts, fields, cp.producer.as_ref(), step_settings, false, ret);
                declare_composed(&ctx);
                b.extend(render(&ctx, &stmts));
            }
        }
        render_all(&mut b, &streaming::lag_snapshots(cp, Some(si), !frame));
    }
    if !frame {
        for k in 0..cp.lines.len() {
            render_all(&mut b, &streaming::lag_push(k, &lag_names));
        }
        for st in cp.map_state.iter().filter(|st| st.carried) {
            b.line(format!("sp.{} = {}", st.name, go_ident(&st.name)));
        }
    }
    let target = if frame { "" } else { "sp." };
    for out in outputs {
        let src = cp.output_storage(out);
        if frame && src == format!("cur_{out}") {
            continue;
        }
        b.line(format!("{target}cur_{out} = {}", go_ident(&src)));
    }
    if !frame {
        b.line("return nil");
    }
    Some(b.lines)
}

#[allow(clippy::too_many_lines)]
fn emit_composed_open(o: &mut String, em: &Em, cp: &streaming::ComposedPlan, step_settings: &BTreeSet<String>, outputs: &[String]) {
    let func = em.func;
    let (region_stmts, tail_stmts) = build_composed_open_bodies(cp, outputs);
    let combined: Vec<Statement> =
        cleanup_open_body(&region_stmts.iter().cloned().chain(tail_stmts.iter().cloned()).collect::<Vec<_>>(), em);
    let ctx = em.open_ctx(&combined);
    for out in outputs {
        ctx.env.declare(&format!("sc_{out}"), if out_ctype(func, out) == CType::Int32 { CType::IntPtr } else { CType::RealPtr });
    }
    let mut b = B::default();
    emit_body_decls(&mut b, &combined);
    emit_open_head(&mut b, em);
    emit_open_validation(&mut b, em, OutMode::Core);
    emit_anchor_guard(&mut b);
    // A sub's rejection would carry the callee's name; past this point one is
    // a bug in this function's lookback.
    b.guard(format!("historyLen < {}+1", em.lookback_call("c")), "return InsufficientHistory");
    emit_extras_and_candle(&mut b, em, &combined, &ctx);
    let alias_fill = cp.fill_scratch_may_alias_output(outputs);
    for out in outputs {
        let ty = slice_ty(out_ctype(func, out));
        if alias_fill {
            b.line(format!("var sc_{out} {ty}"));
            b.open("if outStride == 1 {");
            b.line(format!("sc_{out} = {}", go_ident(out)));
            b.mid("} else {");
            b.line(format!("sc_{out} = make({ty}, historyLen)"));
            b.close("}");
        } else {
            b.line(format!("sc_{out} := make({ty}, historyLen)"));
        }
    }
    for (_, name, init) in managed_lag_open_locals(cp, "", "") {
        if name.starts_with("lagBuf") {
            b.line(format!("var {} []float64", go_ident(&name)));
        } else {
            b.line(format!("{} := int32({init})", go_ident(&name)));
        }
    }

    let region_len = region_stmts.len();
    let own_inputs: BTreeSet<String> = streaming::input_array_names(func).into_iter().collect();
    let lag = GoLagOpen { cp, outputs, ctx: &ctx };
    let mut inserts: Vec<(usize, u8, String)> = streaming::lag_inserts(cp, region_len, &|t, a| {
        let mut bb = B::default();
        lag.emit(&mut bb, a);
        t.push_str(&bb.lines.join("\n"));
    });
    let mut replaced: BTreeSet<usize> = BTreeSet::new();
    for (si, sub) in cp.subs.iter().enumerate() {
        let mut t = B::default();
        let callee_key = sub.callee.to_lowercase();
        let cn = callee_names(em, &callee_key);
        let sc_rewrite = |e: &Expr| -> Expr {
            streaming::rewrite_expr(e, &|x| match x {
                Expr::Var(v) if outputs.contains(&v) => Expr::Var(format!("sc_{v}")),
                other => other,
            })
        };
        let anchor = int_g(&ctx, &sc_rewrite(&sub.s_arg)).render_at(2);
        let e_arg = int_g(&ctx, &sc_rewrite(&sub.e_arg));
        let e_is_end = e_arg.is_name("endIdx");
        let srcs: Vec<String> = sub
            .srcs
            .iter()
            .map(|src| {
                let name = if outputs.contains(src) { format!("sc_{src}") } else { go_ident(src) };
                // A whole own input is already exactly the range; anything
                // else is cut to what the callee may read.
                if own_inputs.contains(src) && e_is_end {
                    name
                } else {
                    GExpr::slice(name_g(&name), None, Some(bin(e_arg.clone(), GOp::Add, name_g("1")))).render_at(2)
                }
            })
            .collect();
        let opts = callee_impl_opts(em, &callee_key, &sub.opt_args, &ctx);
        let err = ctx.temp("E");
        let fused = sub.is_fusable().then(|| streaming::batch_call_out_args(&tail_stmts[sub.tail_idx], sub)).flatten();
        let mut args = srcs;
        args.push(anchor);
        args.extend(opts);
        if let Some((out_meta, dsts)) = fused {
            args.extend(out_meta.iter().map(|e| meta_ref(&sc_rewrite(e), &ctx)));
            args.extend(dsts.iter().map(|e| ctx.pure(&sc_rewrite(e), None).render_at(2)));
            t.line(format!("sub{si}, {err} := c.{}({})", cn.open_and_fill_internal, args.join(", ")));
            t.guard(format!("{err} != nil"), format!("return {err}.(RetCode)"));
            if let Statement::Assign { target, .. } = &tail_stmts[sub.tail_idx] {
                t.line(format!("{} = Success", ctx.pure(&sc_rewrite(target), None).render()));
            }
            replaced.insert(region_len + sub.tail_idx);
        } else {
            t.line(format!("sub{si}, {err} := c.{}({})", cn.open_internal, args.join(", ")));
            t.guard(format!("{err} != nil"), format!("return {err}.(RetCode)"));
        }
        inserts.push((region_len + sub.tail_idx, 2, t.lines.join("\n")));
    }
    let inserts: Vec<(usize, Vec<String>)> = streaming::order_lag_inserts(inserts)
        .into_iter()
        .map(|(at, t)| (at, t.lines().filter(|l| !l.is_empty()).map(str::to_string).collect()))
        .collect();

    emit_open_region(&mut b, &combined, &ctx, &inserts, &replaced);

    b.guard("*outNBElement < 1", "return InsufficientHistory");
    for (at, action) in &cp.open_actions {
        if *at == streaming::OpenAt::Epilogue {
            lag.emit(&mut b, action);
        }
    }
    let mut extra = Vec::new();
    for (si, _) in cp.subs.iter().enumerate() {
        extra.push(format!("sp.sub{si} = sub{si}"));
    }
    for name in streaming::lag_handle_fields(cp) {
        extra.push(format!("sp.{name} = {}", go_ident(&name)));
    }
    for st in &cp.map_state {
        extra.push(format!("sp.{} = {}", st.name, go_ident(&st.name)));
    }
    if let Some(model) = &cp.producer {
        emit_capture(&mut b, em, model, &model.state, step_settings, &ctx, None, &extra);
    } else {
        for p in &func.optional_inputs {
            b.line(format!("sp.{} = {}", p.name, go_ident(&p.name)));
        }
        for (name, _) in &func.private_extra_params {
            b.line(format!("sp.{name} = {}", go_ident(name)));
        }
        b.extend(extra);
    }
    emit_cur_capture(&mut b, outputs, CurSource::Scratch);
    if !alias_fill {
        for out in outputs {
            b.guard("outStride == 1", format!("copy({}, sc_{out}[:*outNBElement])", go_ident(out)));
        }
    }
    b.line("return Success");
    o.push_str(&go::func_text(&open_sig(em, OutMode::Core), &b.lines));
}

/// Renders the composed Open's delay-line actions in Go.
struct GoLagOpen<'a, 'c> {
    cp: &'a streaming::ComposedPlan<'a>,
    outputs: &'a [String],
    ctx: &'a Ctx<'c>,
}

impl GoLagOpen<'_, '_> {
    fn scratch(&self, e: &Expr) -> Expr {
        let outs = self.outputs;
        streaming::rewrite_expr(e, &|x| match x {
            Expr::Var(v) if outs.contains(&v) => Expr::Var(format!("sc_{v}")),
            other => other,
        })
    }

    fn formula(&self, edge: usize) -> GExpr {
        let mut acc: Option<GExpr> = None;
        for (sign, t) in &self.cp.edge_delay_formula(edge).terms {
            let g = match t {
                streaming::LagTerm::Nb(s) => name_g(&streaming::lag_nb_local(*s)),
                streaming::LagTerm::Const(k) => name_g(&k.to_string()),
                streaming::LagTerm::Batch(e) => int_g(self.ctx, &self.scratch(e)),
            };
            acc = Some(match acc {
                None if *sign < 0 => GExpr::unary("-", g),
                None => g,
                Some(a) => bin(a, if *sign < 0 { GOp::Sub } else { GOp::Add }, g),
            });
        }
        acc.unwrap_or_else(|| name_g("0"))
    }

    fn emit(&self, b: &mut B, action: &streaming::OpenAction) {
        use streaming::{lag_line_field, LineField, OpenAction};
        let cp = self.cp;
        match action {
            OpenAction::Snapshot { def } => {
                let s = cp.snapshot_of(*def).expect("snapshotted definition");
                b.line(format!("{} = {}", streaming::lag_nb_local(s), int_g(self.ctx, &self.scratch(&cp.snapshot_count(*def))).render()));
            }
            OpenAction::EmptyGuard { count, floor } => {
                let count = int_g(self.ctx, &self.scratch(count));
                b.guard(bin(count, GOp::Lt, name_g(&floor.to_string())).render(), "return InsufficientHistory");
            }
            OpenAction::Presume { edge } => {
                b.guard(bin(self.formula(*edge), GOp::Ne, name_g("0")).render(), "return InternalError");
            }
            OpenAction::Capture { line, tap } => {
                let t = &cp.lines[*line].taps[*tap];
                let d = streaming::lag_depth_local(*line, *tap);
                let nb = streaming::lag_nb_local(cp.snapshot_of(cp.lines[*line].def).expect("snapshotted line"));
                b.line(format!("{d} = {}", self.formula(t.edge).render()));
                let low = bin(name_g(&d), GOp::Lt, name_g(&t.capture_floor().to_string()));
                let high = bin(name_g(&d), GOp::Gt, bin(name_g(&nb), GOp::Sub, name_g("1")));
                b.guard(bin(low, GOp::LOr, high).render(), "return InternalError");
            }
            OpenAction::Seed { line } => {
                let l = &cp.lines[*line];
                let (buf, cap, pos) = (
                    lag_line_field(*line, LineField::Buf),
                    lag_line_field(*line, LineField::Cap),
                    lag_line_field(*line, LineField::Pos),
                );
                b.line(format!("{cap} = {}", managed_lag_cap(cp, *line, Some("max"))));
                for (j, tap) in l.taps.iter().enumerate() {
                    if tap.back == streaming::TapBack::Field {
                        let d = streaming::lag_depth_local(*line, j);
                        let back = lag_line_field(*line, LineField::Back(j));
                        b.open(format!("if {d} < 0 {{"));
                        b.line(format!("{back} = 0"));
                        b.mid("} else {");
                        b.line(format!("{back} = {cap} - {d}"));
                        b.close("}");
                    }
                }
                let nb = streaming::lag_nb_local(cp.snapshot_of(l.def).expect("snapshotted line"));
                let src = self.ctx.pure(&self.scratch(&Expr::Var(cp.defs[l.def].name.clone())), None).render();
                b.line(format!("{buf} = make([]float64, max({cap}, 1))"));
                b.line(GExpr::call("copy", vec![name_g(&buf), GExpr::slice(name_g(&src), Some(bin(name_g(&nb), GOp::Sub, name_g(&cap))), Some(name_g(&nb)))]).render());
                b.line(format!("{pos} = 0"));
            }
            OpenAction::Mirror { cond, then_, else_ } => {
                b.open(format!("if {} {{", cond_text(self.ctx, &self.scratch(cond))));
                for a in then_ {
                    self.emit(b, a);
                }
                b.mid("} else {");
                for a in else_ {
                    self.emit(b, a);
                }
                b.close("}");
            }
        }
    }
}

/// An out-meta argument of a fused sub-open: a pointer to the count.
fn meta_ref(e: &Expr, ctx: &Ctx) -> String {
    let is_ptr = |n: &str| ctx.env.lookup(n) == Some(CType::IntPtr);
    match e {
        Expr::AddressOf(inner) => match inner.as_ref() {
            Expr::Var(n) if is_ptr(n) => go_ident(n),
            Expr::Var(n) => format!("&{}", go_ident(n)),
            other => ctx.fail(&format!("an out-meta argument {other:?}")),
        },
        Expr::Var(n) if is_ptr(n) => go_ident(n),
        other => ctx.fail(&format!("an out-meta argument {other:?}")),
    }
}

fn emit_composed(o: &mut String, em: &Em, cp: &streaming::ComposedPlan) {
    let func = em.func;
    let outputs: Vec<String> = func.outputs.iter().map(|x| x.name.clone()).collect();
    let step_settings = match &cp.producer {
        Some(model) => detect_candle_settings(&model.steady_stmts),
        None => BTreeSet::new(),
    };
    let mut fields: Vec<Field> = match &cp.producer {
        Some(model) => state_fields(func, model, &step_settings),
        None => base_fields(func),
    };
    for name in streaming::lag_handle_fields(cp) {
        if name.starts_with("lagBuf") {
            fields.push(field(name, CType::RealPtr, "make([]float64, 1)"));
        } else {
            fields.push(field(name, CType::Int32, "0"));
        }
    }
    for st in &cp.map_state {
        fields.push(var_field(&st.name, &st.ty));
    }
    let mut members = Members::default();
    for (si, sub) in cp.subs.iter().enumerate() {
        let cls = callee_names(em, &sub.callee.to_lowercase()).stream;
        members.decls.push((format!("sub{si}"), format!("*{cls}")));
        members.copy.push(format!("cl.sub{si} = sp.sub{si}.Clone()"));
    }
    let frame = composed_step(em, cp, &step_settings, &outputs, &fields, true).map(PeekFrame::falls_through);
    emit_handle(o, em, &fields, &members, frame.as_ref(), true);
    let step = composed_step(em, cp, &step_settings, &outputs, &fields, false).expect("a composed step always renders");
    o.push_str(&go::func_text(&step_sig(em, true), &with_core(step)));
    emit_composed_open(o, em, cp, &step_settings, &outputs);
    emit_open_and_fill_internal_wrapper(o, em, true);
    emit_open_wrappers(o, em, true, false);
}
