//! C emitter for the streaming (incremental) API — docs/streaming-api-proposal.md.
//!
//! For every function whose YAML declares a `streaming:` tier, the generated
//! `src/ta_func/ta_<NAME>.c` gains a stream section after the batch variants:
//!
//! - `struct TA_<NAME>_Stream` — params, carried scalars, lag slots;
//! - `static void TA_<NAME>_StreamStep(...)` — the ONE transition function
//!   (the batch steady-loop body on rewritten IR); `Update` runs it on the
//!   live state and `Peek` on a stack copy, so peek == update bit-for-bit by
//!   construction;
//! - `TA_LIB_API TA_RetCode TA_<NAME>_Open/Update/Peek/Close` — the public
//!   lifecycle (proposal §"API shape per backend").
//!
//! `Open` transcribes the ENTIRE batch body (startIdx=0, endIdx=historyLen-1,
//! output writes redirected to `lastValue_*` scalars) and then captures the
//! still-live locals into the freshly allocated state struct — batch-equal
//! state by construction, seeding/compatibility/unstable-period handling
//! carried verbatim. Bit-exactness versus `batch(startIdx=0)` follows because
//! every rewritten statement renders through the same [`super::c`] renderer
//! in the same order.

use std::cell::Cell;
use std::collections::HashMap;
use std::fmt::Write as _;

use crate::helper_registry::HelperRegistry;
use crate::ir::{EnumDef, Expr, FuncDef, ParamType, Statement};
use crate::registry::Registry;
use crate::streaming::{self, StreamModel};

use super::c::{c_decl, emit_opt_param_validation, render_expression, render_statement};

/// C name mapping for the transition rewrite: state fields through the
/// handle pointer, current bars as same-named scalar params, outputs as
/// same-named out-pointers.
struct CNames;

impl streaming::NameMap for CNames {
    fn state(&self, name: &str) -> String {
        format!("sp->{name}")
    }
    fn bar(&self, array: &str) -> String {
        array.to_string()
    }
    fn output(&self, name: &str) -> Expr {
        Expr::PointerDeref(name.to_string())
    }
}

/// C type of an optional parameter.
fn opt_param_c_type(p: &ParamType) -> &'static str {
    match p {
        ParamType::Real => "double",
        ParamType::Integer => "int",
        ParamType::Enum(_) => "TA_MAType",
        ParamType::Price(_) => unreachable!("price optional params do not exist"),
    }
}

/// `TA_<NAME>` in upper case.
fn uname(func: &FuncDef) -> String {
    func.name.to_uppercase()
}

/// The optional-parameter piece of Open's signature: `int optInTimePeriod, `...
fn opt_params_sig(func: &FuncDef) -> String {
    let mut s = String::new();
    for p in &func.optional_inputs {
        let _ = write!(s, "{} {}, ", opt_param_c_type(&p.param_type), p.name);
    }
    s
}

/// The per-output out-pointer piece: `double *outReal` (comma-joined).
fn out_params_sig(func: &FuncDef) -> String {
    func.outputs
        .iter()
        .map(|o| format!("double *{}", o.name))
        .collect::<Vec<_>>()
        .join(", ")
}

/// The scalar bar-input piece of Update/Peek: `double inHigh, double inLow, `...
fn bar_params_sig(func: &FuncDef) -> String {
    let mut s = String::new();
    for a in streaming::input_array_names(func) {
        let _ = write!(s, "double {a}, ");
    }
    s
}

/// Public `Open` prototype (no trailing `;`). Shared by the header emitter
/// and the definition so the two can never drift (MSVC C2375).
pub fn open_signature(func: &FuncDef) -> String {
    let n = uname(func);
    let mut history = String::new();
    for a in streaming::input_array_names(func) {
        let _ = write!(history, "const double {a}[], ");
    }
    format!(
        "TA_LIB_API TA_RetCode TA_{n}_Open( {}{}int historyLen, TA_{n}_Stream **stream, {} )",
        opt_params_sig(func),
        history,
        out_params_sig(func)
    )
}

/// Public `Update` prototype (no trailing `;`).
pub fn update_signature(func: &FuncDef) -> String {
    let n = uname(func);
    format!(
        "TA_LIB_API TA_RetCode TA_{n}_Update( TA_{n}_Stream *stream, {}{} )",
        bar_params_sig(func),
        out_params_sig(func)
    )
}

/// Public `Peek` prototype (no trailing `;`) — logically const handle.
pub fn peek_signature(func: &FuncDef) -> String {
    let n = uname(func);
    format!(
        "TA_LIB_API TA_RetCode TA_{n}_Peek( const TA_{n}_Stream *stream, {}{} )",
        bar_params_sig(func),
        out_params_sig(func)
    )
}

/// Public `Close` prototype (no trailing `;`).
pub fn close_signature(func: &FuncDef) -> String {
    let n = uname(func);
    format!("TA_LIB_API TA_RetCode TA_{n}_Close( TA_{n}_Stream *stream )")
}

/// Header declarations for one streamable function (opaque handle typedef +
/// the four lifecycle prototypes). Emitted into include/ta_func.h.
pub fn header_decls(func: &FuncDef) -> String {
    let n = uname(func);
    format!(
        "\n/*\n * Streaming API for TA_{n} — incremental per-bar evaluation.\n * Open consumes the warm-up history; Update commits one closed bar;\n * Peek evaluates a forming bar without committing; Close frees the handle.\n * See docs/streaming-api-proposal.md.\n */\ntypedef struct TA_{n}_Stream TA_{n}_Stream;\n\n{};\n\n{};\n\n{};\n\n{};\n",
        open_signature(func),
        update_signature(func),
        peek_signature(func),
        close_signature(func)
    )
}

/// Generate the whole stream section for one function's `.c` file.
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
        "c_stream::generate called without a streaming declaration"
    );
    let model =
        streaming::validate_streamable(func).unwrap_or_else(|e| panic!("streaming gate: {e}"));

    let counter = Cell::new(0usize);
    let mut o = String::new();

    let _ = writeln!(o, "/**** Streaming API *****/\n");

    emit_state_struct(&mut o, func, &model);
    emit_step(&mut o, func, &model, enums, registry, helpers, &counter);
    emit_open(&mut o, func, &model, enums, registry, helpers, &counter);
    emit_update(&mut o, func);
    emit_peek(&mut o, func);
    emit_close(&mut o, func);

    o
}

fn emit_state_struct(o: &mut String, func: &FuncDef, model: &StreamModel) {
    let n = uname(func);
    let _ = writeln!(o, "struct TA_{n}_Stream {{");
    for p in &func.optional_inputs {
        let _ = writeln!(o, "   {} {};", opt_param_c_type(&p.param_type), p.name);
    }
    for (name, c_type) in &func.private_extra_params {
        let _ = writeln!(o, "   {c_type} {name};");
    }
    for (name, ty) in &model.state {
        let _ = writeln!(o, "   {};", c_decl(ty, name));
    }
    for lag in &model.lags {
        for k in 1..=lag.depth {
            let _ = writeln!(o, "   double {};", StreamModel::lag_field(&lag.array, k));
        }
    }
    // A struct must have at least one member (T1 maps carry none).
    if func.optional_inputs.is_empty()
        && func.private_extra_params.is_empty()
        && model.state.is_empty()
        && model.lags.is_empty()
    {
        let _ = writeln!(o, "   int unused; /* T1: stateless map */");
    }
    let _ = writeln!(o, "}};\n");
}

fn emit_step(
    o: &mut String,
    func: &FuncDef,
    model: &StreamModel,
    enums: &HashMap<String, EnumDef>,
    registry: &Registry,
    helpers: &HelperRegistry,
    counter: &Cell<usize>,
) {
    let n = uname(func);
    let bars = bar_params_sig(func);
    let outs = out_params_sig(func);
    let _ = writeln!(
        o,
        "static void TA_{n}_StreamStep( struct TA_{n}_Stream *sp, {bars}{outs} )\n{{"
    );
    for (name, ty) in &model.temps {
        let _ = writeln!(o, "   {};", c_decl(ty, name));
    }
    if !model.temps.is_empty() {
        let _ = writeln!(o);
    }
    if model.state.is_empty()
        && func.optional_inputs.is_empty()
        && func.private_extra_params.is_empty()
        && model.lags.is_empty()
    {
        let _ = writeln!(o, "   (void)sp;");
    }
    let transition = streaming::build_transition(model, &CNames)
        .unwrap_or_else(|e| panic!("streaming transition: {e}"));
    for s in &transition {
        o.push_str(&render_statement(s, 3, false, enums, registry, helpers, counter));
    }
    let _ = writeln!(o, "}}\n");
}

fn emit_open(
    o: &mut String,
    func: &FuncDef,
    model: &StreamModel,
    enums: &HashMap<String, EnumDef>,
    registry: &Registry,
    helpers: &HelperRegistry,
    counter: &Cell<usize>,
) {
    let n = uname(func);
    let inputs = streaming::input_array_names(func);
    let _ = writeln!(o, "{}\n{{", open_signature(func));

    // --- declarations -------------------------------------------------------
    let _ = writeln!(o, "   struct TA_{n}_Stream *sp;");
    let _ = writeln!(o, "   int startIdx;");
    let _ = writeln!(o, "   int endIdx;");
    let _ = writeln!(o, "   int dummyBegIdx;");
    let _ = writeln!(o, "   int dummyNBElement;");
    for out in &model.outputs {
        let _ = writeln!(o, "   double lastValue_{out};");
    }
    for (name, c_type) in &func.private_extra_params {
        let _ = writeln!(o, "   {c_type} {name};");
    }
    // Optional params arrive by value; validation may rewrite them (defaults).
    let mut opt_names: Vec<&str> = Vec::new();
    for p in &func.optional_inputs {
        opt_names.push(&p.name);
    }

    // --- validation ---------------------------------------------------------
    let _ = writeln!(o, "\n   if( !stream ) return TA_BAD_PARAM;");
    let _ = writeln!(o, "   *stream = NULL;");
    let null_checks: Vec<String> = inputs
        .iter()
        .map(|i| format!("!{i}"))
        .chain(model.outputs.iter().map(|out| format!("!{out}")))
        .collect();
    let _ = writeln!(o, "   if( {} ) return TA_BAD_PARAM;", null_checks.join(" || "));
    let _ = writeln!(o, "   if( historyLen < 1 ) return TA_BAD_PARAM;");
    o.push_str(&emit_opt_param_validation(func, "TA_BAD_PARAM"));

    // --- initialization (after defaults are substituted) ---------------------
    let _ = writeln!(o, "\n   startIdx = 0;");
    let _ = writeln!(o, "   endIdx = historyLen - 1;");
    let _ = writeln!(o, "   dummyBegIdx = 0;");
    let _ = writeln!(o, "   dummyNBElement = 0;");
    for out in &model.outputs {
        let _ = writeln!(o, "   lastValue_{out} = 0.0;");
    }
    for (name, _) in &func.private_extra_params {
        let init = func
            .private_param_init
            .iter()
            .find(|(pn, _)| pn == name)
            .map_or_else(
                || panic!("{}: no init for private param {name}", func.name),
                |(_, e)| render_expression(e, registry, helpers, counter),
            );
        let _ = writeln!(o, "   {name} = {init};");
    }
    let _ = writeln!(
        o,
        "   (void)startIdx; (void)dummyBegIdx; (void)dummyNBElement;"
    );

    // --- param==1 identity fast path (mirrors the batch's explicit path) ----
    if let Some(idp) = &model.identity {
        let cond = render_expression(&idp.condition, registry, helpers, counter);
        let lookback_args: Vec<String> =
            func.optional_inputs.iter().map(|p| p.name.clone()).collect();
        let _ = writeln!(o, "\n   if( {cond} )\n   {{");
        // min_history holds on this path too: lookback folds in the ambient
        // unstable period, so period==1 with K>0 still requires K+1 bars.
        let _ = writeln!(
            o,
            "      if( historyLen < TA_{n}_Lookback( {} ) + 1 ) return TA_BAD_PARAM;",
            lookback_args.join(", ")
        );
        o.push_str(&alloc_and_capture(func, model, "      ", /*with_state=*/ false));
        for (out, inp) in &idp.pairs {
            let _ = writeln!(o, "      *{out} = {inp}[historyLen - 1];");
        }
        let _ = writeln!(o, "      *stream = sp;");
        let _ = writeln!(o, "      return TA_SUCCESS;");
        let _ = writeln!(o, "   }}");
    }

    // --- transcribed batch body ----------------------------------------------
    let _ = writeln!(o, "\n   {{");
    let open_body = build_open_body(model);
    for s in &open_body {
        o.push_str(&render_statement(s, 6, false, enums, registry, helpers, counter));
    }

    // --- state capture --------------------------------------------------------
    let _ = writeln!(o, "\n      /* Capture the live batch state into the handle. */");
    o.push_str(&alloc_and_capture(func, model, "      ", /*with_state=*/ true));
    for lag in &model.lags {
        for k in 1..=lag.depth {
            let _ = writeln!(
                o,
                "      sp->{} = {}[historyLen - {k}];",
                StreamModel::lag_field(&lag.array, k),
                lag.array
            );
        }
    }
    for out in &model.outputs {
        let _ = writeln!(o, "      *{out} = lastValue_{out};");
    }
    let _ = writeln!(o, "      *stream = sp;");
    let _ = writeln!(o, "      return TA_SUCCESS;");
    let _ = writeln!(o, "   }}\n}}\n");
    let _ = opt_names; // (documentational; validation emitted above)
}

/// `sp = TA_Malloc(...); memset; param/extra capture[; state capture]` at the
/// given indent. memset keeps unused fields (identity path) deterministic.
fn alloc_and_capture(func: &FuncDef, model: &StreamModel, pad: &str, with_state: bool) -> String {
    let n = uname(func);
    let mut s = String::new();
    let _ = writeln!(
        s,
        "{pad}sp = (struct TA_{n}_Stream *)TA_Malloc( sizeof(*sp) );"
    );
    let _ = writeln!(s, "{pad}if( !sp ) return TA_ALLOC_ERR;");
    let _ = writeln!(s, "{pad}memset( sp, 0, sizeof(*sp) );");
    for p in &func.optional_inputs {
        let _ = writeln!(s, "{pad}sp->{0} = {0};", p.name);
    }
    for (name, _) in &func.private_extra_params {
        let _ = writeln!(s, "{pad}sp->{name} = {name};");
    }
    if with_state {
        for (name, _) in &model.state {
            let _ = writeln!(s, "{pad}sp->{name} = {name};");
        }
    }
    s
}

/// The transcribed batch body for Open: out-param pointers redirected to the
/// dummies, output-array writes redirected to `lastValue_*`, early returns
/// mapped (no-data success -> TA_BAD_PARAM; error codes verbatim), final
/// return dropped so control falls through to the state capture.
fn build_open_body(model: &StreamModel) -> Vec<Statement> {
    let outputs = model.outputs.clone();
    // Carried-state locals must never be captured uninitialized: a local
    // assigned only inside a data-dependent branch (ADX's minusDI/plusDI on
    // flat-price history) would otherwise be UB at the capture epilogue.
    // Zero-init is bit-exact-safe: wherever the batch body assigns, the zero
    // is overwritten; wherever it does not, the transition is write-before-
    // read on that field and the zero is dead state.
    let state_names: std::collections::BTreeMap<String, crate::ir::VarType> =
        model.state.iter().cloned().collect();
    let fe = move |e: Expr| -> Expr {
        match e {
            Expr::PointerDeref(nm) if nm == "outBegIdx" => Expr::Var("dummyBegIdx".into()),
            Expr::PointerDeref(nm) if nm == "outNBElement" => Expr::Var("dummyNBElement".into()),
            other => other,
        }
    };
    let fs = move |s: Statement| -> Option<Statement> {
        match s {
            Statement::VarDecl {
                var_type,
                name,
                init: None,
            } if state_names.contains_key(&name) => {
                let zero = match var_type {
                    crate::ir::VarType::Real => Expr::Literal(0.0),
                    _ => Expr::IntLiteral(0),
                };
                Some(Statement::VarDecl {
                    var_type,
                    name,
                    init: Some(zero),
                })
            }
            Statement::Assign {
                target: Expr::ArrayAccess(nm, _),
                value,
                compound,
            } if outputs.contains(&nm) => Some(Statement::Assign {
                target: Expr::Var(format!("lastValue_{nm}")),
                value,
                compound,
            }),
            Statement::Return { value } => {
                let mapped = match value {
                    // No-data guard: not enough history for a first value.
                    Some(Expr::Var(v)) if matches!(v.as_str(), "SUCCESS" | "TA_SUCCESS") => {
                        Some(Expr::Var("BAD_PARAM".into()))
                    }
                    other => other, // error-code propagation, verbatim
                };
                Some(Statement::Return { value: mapped })
            }
            other => Some(other),
        }
    };

    // Drop the FINAL top-level return: the success path falls through to the
    // capture epilogue (early returns keep their mapped statements).
    let mut body: Vec<Statement> = model.body.to_vec();
    if matches!(body.last(), Some(Statement::Return { .. })) {
        body.pop();
    }
    streaming::rewrite_stmts(&body, &fe, &fs)
}

fn emit_update(o: &mut String, func: &FuncDef) {
    let n = uname(func);
    let bars: Vec<String> = streaming::input_array_names(func);
    let outs: Vec<String> = func.outputs.iter().map(|x| x.name.clone()).collect();
    let _ = writeln!(o, "{}\n{{", update_signature(func));
    let checks: Vec<String> = std::iter::once("!stream".to_string())
        .chain(outs.iter().map(|x| format!("!{x}")))
        .collect();
    let _ = writeln!(o, "   if( {} ) return TA_BAD_PARAM;", checks.join(" || "));
    let args: Vec<String> = bars
        .iter()
        .cloned()
        .chain(outs.iter().cloned())
        .collect();
    let _ = writeln!(o, "   TA_{n}_StreamStep( stream, {} );", args.join(", "));
    let _ = writeln!(o, "   return TA_SUCCESS;\n}}\n");
}

fn emit_peek(o: &mut String, func: &FuncDef) {
    let n = uname(func);
    let bars: Vec<String> = streaming::input_array_names(func);
    let outs: Vec<String> = func.outputs.iter().map(|x| x.name.clone()).collect();
    let _ = writeln!(o, "{}\n{{", peek_signature(func));
    let _ = writeln!(o, "   struct TA_{n}_Stream scratch;");
    let checks: Vec<String> = std::iter::once("!stream".to_string())
        .chain(outs.iter().map(|x| format!("!{x}")))
        .collect();
    let _ = writeln!(o, "\n   if( {} ) return TA_BAD_PARAM;", checks.join(" || "));
    let _ = writeln!(o, "   scratch = *stream;");
    let args: Vec<String> = bars
        .iter()
        .cloned()
        .chain(outs.iter().cloned())
        .collect();
    let _ = writeln!(o, "   TA_{n}_StreamStep( &scratch, {} );", args.join(", "));
    let _ = writeln!(o, "   return TA_SUCCESS;\n}}\n");
}

fn emit_close(o: &mut String, func: &FuncDef) {
    let _ = writeln!(o, "{}\n{{", close_signature(func));
    let _ = writeln!(o, "   if( stream ) TA_Free( stream );");
    let _ = writeln!(o, "   return TA_SUCCESS;\n}}\n");
}
