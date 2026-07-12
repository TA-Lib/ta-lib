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
use crate::streaming::{self, circ_storages, DispatchPlan, StreamModel, StreamPlan};

use super::c::{
    c_decl, emit_opt_param_validation, render_c_switch_label, render_expression,
    render_statement, render_statement_stream,
};

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
    fn ring_buf(&self, var: &str, array: &str) -> String {
        format!("sp->ring_{var}_{array}")
    }
    fn ring_pos(&self, var: &str) -> String {
        format!("sp->ringPos_{var}")
    }
    fn ring_lag(&self, var: &str) -> String {
        format!("sp->ringLag_{var}")
    }
    fn ring_cap(&self, var: &str) -> String {
        format!("sp->ringCap_{var}")
    }
    fn win_buf(&self, var: &str, array: &str) -> String {
        format!("sp->win_{var}_{array}")
    }
    fn win_pos(&self, var: &str) -> String {
        format!("sp->winPos_{var}")
    }
    fn win_cap(&self, var: &str) -> String {
        format!("sp->winCap_{var}")
    }
    fn circ_buf(&self, storage: &str) -> String {
        format!("sp->cb_{storage}")
    }
    fn extrema_buf(&self, array: &str) -> String {
        format!("sp->x_{array}")
    }
    fn extrema_cap(&self) -> String {
        "sp->xCap".to_string()
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

/// C element type of an output.
fn out_c_type(func: &FuncDef, name: &str) -> &'static str {
    let is_int = func
        .outputs
        .iter()
        .any(|o| o.name == name && o.param_type == ParamType::Integer);
    if is_int { "int" } else { "double" }
}

/// The per-output out-pointer piece: `double *outReal, int *outInteger, ...`.
fn out_params_sig(func: &FuncDef) -> String {
    func.outputs
        .iter()
        .map(|o| format!("{} *{}", out_c_type(func, &o.name), o.name))
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

/// Internal `OpenInternal` prototype (no trailing `;`). This is the real
/// worker: it takes an extra `startIdx` — the bar within the history buffer at
/// which warm-up begins (0 = warm from the very first bar). The public `Open`
/// is a thin wrapper that calls this with 0; only generated code (composed
/// functions opening a sub-stream) passes a non-zero startIdx, handing the sub
/// the FULL buffer from bar 0 so it seeds itself exactly as its batch would —
/// including MA types that seed from the absolute origin (`inReal[0]`) under
/// Metastock/Tradestation. The seeding stays inside each callee's own body; the
/// composer never reasons about MA types. Kept out of the public header so the
/// public API stays simple and this entry point can grow new knobs internally.
pub fn open_internal_signature(func: &FuncDef) -> String {
    let n = uname(func);
    let mut history = String::new();
    for a in streaming::input_array_names(func) {
        let _ = write!(history, "const double {a}[], ");
    }
    // Uses `struct TA_<n>_Stream` (not the typedef) so the internal header does
    // not depend on ta_func.h being included first. The tag is forward-declared
    // at file scope in the internal header, so this refers to the same struct as
    // the definition (a bare `struct X` first seen in a prototype would otherwise
    // get prototype scope and collide).
    format!(
        "TA_RetCode TA_{n}_OpenInternal( {}{}int startIdx, int historyLen, struct TA_{n}_Stream **stream, {} )",
        opt_params_sig(func),
        history,
        out_params_sig(func)
    )
}

/// Emit the public `Open` as a thin wrapper delegating to `OpenInternal` with
/// startIdx = 0 (the standalone/public default).
fn emit_open_wrapper(o: &mut String, func: &FuncDef) {
    let n = uname(func);
    let mut fwd = String::new();
    for p in &func.optional_inputs {
        let _ = write!(fwd, "{}, ", p.name);
    }
    for a in streaming::input_array_names(func) {
        let _ = write!(fwd, "{a}, ");
    }
    let outs: String = func
        .outputs
        .iter()
        .map(|out| out.name.clone())
        .collect::<Vec<_>>()
        .join(", ");
    let _ = writeln!(o, "{}\n{{", open_signature(func));
    let _ = writeln!(o, "   return TA_{n}_OpenInternal( {fwd}0, historyLen, stream, {outs} );");
    let _ = writeln!(o, "}}\n");
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
/// Dispatch functions with unsupported arms (MA while TRIMA/MAMA lack
/// streams) get a derived capability note: a batch-valid enum value being
/// stream-rejected is user-visible API behavior and must be documented at
/// the declaration, not only in the proposal. The note regenerates from the
/// plan, so it updates itself when a callee gains its stream.
pub fn header_decls(func: &FuncDef, lookup: &dyn streaming::CalleeLookup) -> String {
    let n = uname(func);
    let mut note = String::new();
    if let Ok(StreamPlan::Dispatch(dp)) = streaming::validate_streamable(func, lookup) {
        let unsupported = dp.unsupported_labels();
        if !unsupported.is_empty() {
            let consts: Vec<String> = unsupported
                .iter()
                .map(|l| {
                    if l.starts_with("TA_") {
                        (*l).to_string()
                    } else {
                        format!("TA_{l}")
                    }
                })
                .collect();
            let _ = write!(
                note,
                " * Note: {} values whose underlying function has no stream yet\n * ({}) are rejected at Open with TA_BAD_PARAM; they gain\n * streams automatically when the underlying function does.\n",
                dp.param,
                consts.join(", ")
            );
            if let Some(idp) = &dp.identity {
                if let Some(g) = identity_guard_text(&idp.condition) {
                    let _ = writeln!(
                        note,
                        " * The {g} identity path streams for every {} value.",
                        dp.param
                    );
                }
            }
        }
    }
    format!(
        "\n/*\n * Streaming API for TA_{n} — incremental per-bar evaluation.\n * Open consumes the warm-up history; Update commits one closed bar;\n * Peek evaluates a forming bar without committing; Close frees the handle.\n * A handle is single-writer: driving one handle from two threads\n * concurrently — Update or Peek, despite the latter's const — is\n * undefined behavior. Distinct handles are fully independent.\n{note} * See docs/streaming-api-proposal.md.\n */\ntypedef struct TA_{n}_Stream TA_{n}_Stream;\n\n{};\n\n{};\n\n{};\n\n{};\n",
        open_signature(func),
        update_signature(func),
        peek_signature(func),
        close_signature(func)
    )
}

/// Text form of a recognized identity guard (`<param> == 1` / `<param> <= 1`
/// — the closed shape the identity detector accepts).
fn identity_guard_text(cond: &Expr) -> Option<String> {
    use crate::ir::BinOp;
    if let Expr::BinOp(l, op, r) = cond {
        if let (Expr::Var(v), Expr::IntLiteral(k)) = (l.as_ref(), r.as_ref()) {
            let op_s = match op {
                BinOp::Eq => "==",
                BinOp::LessEq => "<=",
                _ => return None,
            };
            return Some(format!("{v} {op_s} {k}"));
        }
    }
    None
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
    let plan = streaming::validate_streamable(func, registry)
        .unwrap_or_else(|e| panic!("streaming gate: {e}"));

    let counter = Cell::new(0usize);
    let mut o = String::new();

    let _ = writeln!(o, "/**** Streaming API *****/\n");

    match &plan {
        StreamPlan::Loop(model) => {
            emit_state_struct(&mut o, func, model);
            emit_release(&mut o, func, model);
            emit_step(&mut o, func, model, enums, registry, helpers, &counter);
            emit_open(&mut o, func, model, enums, registry, helpers, &counter);
            emit_update(&mut o, func);
            emit_peek(&mut o, func, model);
            emit_close(&mut o, func, model);
        }
        StreamPlan::Dispatch(dp) => {
            emit_dispatch(&mut o, func, dp, enums, registry, helpers, &counter);
        }
        StreamPlan::Composed(cp) => {
            emit_composed(&mut o, func, cp, enums, registry, helpers, &counter);
        }
        StreamPlan::DualMode(dmp) => {
            emit_dual_mode(&mut o, func, dmp, enums, registry, helpers, &counter);
        }
        StreamPlan::FastPathSkip(gap) => {
            emit_fastpath_skip(&mut o, func, gap, enums, registry, helpers, &counter);
        }
    }

    o
}

// ---------------------------------------------------------------------------
// Composed emission (STOCH class): producer loop + pipeline over the
// callees' PUBLIC streams. See streaming::ComposedPlan.
// ---------------------------------------------------------------------------

/// C name mapping for the composed producer transition: identical to the
/// loop tier except the intermediate series' "output" write lands in a
/// local scalar (`cur_<series>`) the pipeline then consumes.
struct ComposedNames {
    series: String,
}

impl streaming::NameMap for ComposedNames {
    fn state(&self, name: &str) -> String {
        format!("sp->{name}")
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
        format!("sp->ring_{var}_{array}")
    }
    fn ring_pos(&self, var: &str) -> String {
        format!("sp->ringPos_{var}")
    }
    fn ring_lag(&self, var: &str) -> String {
        format!("sp->ringLag_{var}")
    }
    fn ring_cap(&self, var: &str) -> String {
        format!("sp->ringCap_{var}")
    }
    fn win_buf(&self, var: &str, array: &str) -> String {
        format!("sp->win_{var}_{array}")
    }
    fn win_pos(&self, var: &str) -> String {
        format!("sp->winPos_{var}")
    }
    fn win_cap(&self, var: &str) -> String {
        format!("sp->winCap_{var}")
    }
    fn circ_buf(&self, storage: &str) -> String {
        format!("sp->cb_{storage}")
    }
    fn extrema_buf(&self, array: &str) -> String {
        format!("sp->x_{array}")
    }
    fn extrema_cap(&self) -> String {
        "sp->xCap".to_string()
    }
}

/// Cleanup text for Open failure paths BEFORE the handle exists: close every
/// sub handle opened so far (Close(NULL) is a no-op) and free the scratch
/// output arrays. No trailing semicolon (rendered contexts add their own).
fn composed_cleanup(cp: &streaming::ComposedPlan, outputs: &[String]) -> String {
    let mut s = String::new();
    for (i, sub) in cp.subs.iter().enumerate() {
        let _ = write!(s, "{}_Close( sub{i} ); ", callee_prefix(&sub.callee));
    }
    for out in outputs {
        let _ = write!(s, "TA_Free( sc_{out} ); ");
    }
    s.trim_end().trim_end_matches(';').to_string()
}

/// The `cur_<name>` scalars the composed step declares: the producer's
/// intermediate series (if any), then each sub-call's destination series in
/// tail order (deduplicated; bar inputs are scalar parameters, not `cur_*`).
/// Align destinations alias an existing scalar and get no declaration of
/// their own.
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
    // Outputs a combine map DEFINES (ADXR's outReal, written from the ADX lag
    // ring rather than by a sub-call) also need a scalar.
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

/// Drop the shells of the map's `for` loops, keeping any inner param-selected
/// `if` structure. The per-bar step evaluates each element body exactly once,
/// so the loop cursor and bounds vanish (the array reads were already rewritten
/// to `cur_*` scalars by [`emit_composed_step`]).
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

/// The map loop's single cursor (the `for` init variable), needed to tell a
/// sub-output's current read (`series[cursor + lag]`) from its lagged read
/// (`series[cursor]`). None for non-`ForC` maps (which never carry lag rings).
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

/// Transform one combine-map tail statement into the per-bar scalar form:
/// rewrite every `series[cursor]` read/write into the series' current scalar
/// (`cur[series]`) and every optional-param read into `sp-><param>`, then drop
/// the `for` shells. A sub-output lag-ring series is index-AWARE: its
/// `series[cursor + lag]` read is the current scalar, but its `series[cursor]`
/// read is the value `lag` bars behind — the oldest slot of the ring.
/// `map_temps` stay as plain step locals.
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
                        format!("sp->lagRing_{name}"),
                        Box::new(Expr::Var(format!("sp->lagRingPos_{name}"))),
                    )
                } else {
                    // The current (newest) sub-output value.
                    Expr::Var(cur.get(&name).cloned().unwrap_or_else(|| format!("cur_{name}")))
                }
            }
            Expr::ArrayAccess(name, _) if cur.contains_key(&name) => {
                Expr::Var(cur.get(&name).expect("checked").clone())
            }
            Expr::Var(v) if params.contains(&v) => Expr::Var(format!("sp->{v}")),
            other => other,
        }
    };
    let rewritten = streaming::rewrite_stmts(std::slice::from_ref(st), &fe, &|s| Some(s));
    rewritten.iter().flat_map(drop_forc_shells).collect()
}

/// The composed StreamStep: the producer transition (when present) writes the
/// intermediate series' scalar, which pipelines through the sub handles;
/// combine maps run per-bar. `peekMode` selects sub-Peek over sub-Update so
/// the single step body serves both.
#[allow(clippy::too_many_lines)]
fn emit_composed_step(
    o: &mut String,
    func: &FuncDef,
    cp: &streaming::ComposedPlan,
    inputs: &[String],
    outputs: &[String],
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
    if let Some(model) = &cp.producer {
        for (name, ty) in &model.temps {
            let _ = writeln!(o, "   {};", c_decl(ty, name));
        }
    }
    for (name, ty) in &cp.map_temps {
        let _ = writeln!(o, "   {};", c_decl(ty, name));
    }
    let cur_scalars = composed_cur_scalars(cp, inputs, outputs);
    for name in &cur_scalars {
        let _ = writeln!(o, "   double cur_{name};");
    }
    let _ = writeln!(o);

    // The cur-map: bar inputs are the step's scalar parameters; the producer
    // series (when present) is written by the producer transition below.
    let mut cur: std::collections::BTreeMap<String, String> = inputs
        .iter()
        .map(|b| (b.clone(), b.clone()))
        .collect();

    if let Some(model) = &cp.producer {
        emit_extrema_rebase(o, model);
        let names = ComposedNames {
            series: cp.series.clone().expect("producer plan carries a series"),
        };
        let transition = streaming::build_transition(model, &names)
            .unwrap_or_else(|e| panic!("streaming transition: {e}"));
        let mut body_c = String::new();
        for s in &transition {
            body_c.push_str(&render_statement_stream(s, 3, enums, registry, helpers, counter));
        }
        let step_settings = crate::candle_settings::detect_candle_settings(&model.steady_stmts);
        if !step_settings.is_empty() {
            o.push_str(&emit_used_candle_unpacking(&step_settings, &body_c, 3));
        }
        o.push_str(&body_c);
        let series = cp.series.clone().expect("producer plan carries a series");
        cur.insert(series.clone(), format!("cur_{series}"));
    }

    // Pipeline: the batch tail, one scalar per bar through the sub handles.
    let _ = writeln!(o, "\n   /* Pipeline the new bar through the sub-streams (batch tail order). */");
    let params: std::collections::BTreeSet<String> =
        func.optional_inputs.iter().map(|p| p.name.clone()).collect();
    for step in &cp.steps {
        match step {
            streaming::UpdateStep::Sub { sub_idx } => {
                let sub = &cp.subs[*sub_idx];
                let cpfx = callee_prefix(&sub.callee);
                let mut args: Vec<String> = sub
                    .srcs
                    .iter()
                    .map(|s| cur.get(s).expect("analyzer ordered sub srcs").clone())
                    .collect();
                for d in &sub.dsts {
                    args.push(format!("&cur_{d}"));
                }
                let arg_str = args.join(", ");
                let _ = writeln!(o, "   if( sp->peekMode )");
                let _ = writeln!(
                    o,
                    "      {cpfx}_Peek( (const {cpfx}_Stream *)sp->sub{sub_idx}, {arg_str} );"
                );
                let _ = writeln!(o, "   else");
                let _ = writeln!(o, "      {cpfx}_Update( sp->sub{sub_idx}, {arg_str} );");
                for d in &sub.dsts {
                    cur.insert(d.clone(), format!("cur_{d}"));
                }
            }
            streaming::UpdateStep::Align { dst, src } => {
                let alias = cur.get(src).expect("analyzer ordered align src").clone();
                cur.insert(dst.clone(), alias);
            }
            streaming::UpdateStep::Map { tail_idx } => {
                // A map may DEFINE outputs (ADXR's outReal from the lag ring):
                // register them so the write becomes `cur_<out> = ...`.
                for o in streaming::map_output_writes(&cp.tail[*tail_idx], outputs) {
                    cur.entry(o.clone()).or_insert_with(|| format!("cur_{o}"));
                }
                let _ = writeln!(o, "   /* Combine map (batch tail, per bar). */");
                for st in &transform_map_step(&cp.tail[*tail_idx], &cur, &params, &cp.sub_lag_rings) {
                    o.push_str(&render_statement_stream(st, 3, enums, registry, helpers, counter));
                }
            }
        }
    }
    // Push the new sub-output value into each lag ring (after every read of the
    // oldest slot in the combine above). In peek mode the ring points at its
    // mirror, so this mutates the scratch copy, not the live handle.
    for ring in &cp.sub_lag_rings {
        let s = &ring.series;
        let _ = writeln!(o, "   sp->lagRing_{s}[sp->lagRingPos_{s}] = cur_{s};");
        let _ = writeln!(
            o,
            "   sp->lagRingPos_{s} = (sp->lagRingPos_{s} + 1) % sp->lagRingCap_{s};"
        );
    }
    for out in outputs {
        let _ = writeln!(o, "   *{out} = {};", cur.get(out).expect("analyzer gated output"));
    }
    let _ = writeln!(o, "}}\n");
}

/// Composed Close: release the sub handles, then the producer buffers + handle
/// (a loopless pipeline has no producer buffers, so a plain free suffices).
fn emit_composed_close(o: &mut String, func: &FuncDef, cp: &streaming::ComposedPlan) {
    let n = uname(func);
    let _ = writeln!(o, "{}\n{{", close_signature(func));
    let _ = writeln!(o, "   if( !stream ) return TA_SUCCESS;");
    for (i, sub) in cp.subs.iter().enumerate() {
        let _ = writeln!(o, "   {}_Close( stream->sub{i} );", callee_prefix(&sub.callee));
    }
    for ring in &cp.sub_lag_rings {
        let s = &ring.series;
        let _ = writeln!(o, "   TA_Free( stream->lagRing_{s} );");
        let _ = writeln!(o, "   TA_Free( stream->lagRingMirror_{s} );");
    }
    let has_buffers = cp.producer.as_ref().is_some_and(StreamModel::needs_release);
    if has_buffers {
        let _ = writeln!(o, "   TA_{n}_StreamRelease( stream );");
    } else {
        let _ = writeln!(o, "   TA_Free( stream );");
    }
    let _ = writeln!(o, "   return TA_SUCCESS;\n}}\n");
}

fn emit_composed(
    o: &mut String,
    func: &FuncDef,
    cp: &streaming::ComposedPlan,
    enums: &HashMap<String, EnumDef>,
    registry: &Registry,
    helpers: &HelperRegistry,
    counter: &Cell<usize>,
) {
    let n = uname(func);
    let inputs = streaming::input_array_names(func);
    let outputs: Vec<String> = func.outputs.iter().map(|x| x.name.clone()).collect();
    let cleanup = composed_cleanup(cp, &outputs);

    // --- state struct: producer fields (if any) + peek mode + sub handles ---
    let mut extra = String::new();
    let _ = writeln!(
        extra,
        "   /* Peek runs the SAME step body on a scratch copy; sub handles are\n    * heap pointers a struct copy cannot clone, so the copy carries this\n    * flag and the step calls sub-Peek instead of sub-Update. */"
    );
    let _ = writeln!(extra, "   int peekMode;");
    for (i, sub) in cp.subs.iter().enumerate() {
        let _ = writeln!(extra, "   {}_Stream *sub{i};", callee_prefix(&sub.callee));
    }
    // Sub-output lag rings (ADXR): a fixed-capacity ring of the last `lag`
    // sub-output values, plus a peek mirror.
    for ring in &cp.sub_lag_rings {
        let s = &ring.series;
        let _ = writeln!(extra, "   int lagRingPos_{s};");
        let _ = writeln!(extra, "   int lagRingCap_{s};");
        let _ = writeln!(extra, "   double *lagRing_{s};");
        let _ = writeln!(extra, "   double *lagRingMirror_{s};");
    }
    match &cp.producer {
        Some(model) => {
            emit_state_struct_ex(o, func, model, &extra);
            emit_release(o, func, model);
        }
        None => emit_composed_struct_noproducer(o, func, &extra),
    }

    // --- StreamStep -----------------------------------------------------------
    emit_composed_step(o, func, cp, &inputs, &outputs, enums, registry, helpers, counter);

    // --- Open ------------------------------------------------------------------
    emit_composed_open(o, func, cp, &outputs, &inputs, &cleanup, enums, registry, helpers, counter);

    // --- Update / Peek / Close ---------------------------------------------------
    emit_update(o, func);
    // Peek: scratch copy + (producer only) buffer mirrors + peekMode. A
    // loopless pipeline has no producer buffers, so the struct copy alone
    // (sub handles shared, peekMode routing sub-Peek) is const-correct.
    {
        let _ = writeln!(o, "{}\n{{", peek_signature(func));
        let _ = writeln!(o, "   struct TA_{n}_Stream scratch;");
        let checks: Vec<String> = std::iter::once("!stream".to_string())
            .chain(outputs.iter().map(|x| format!("!{x}")))
            .collect();
        let _ = writeln!(o, "\n   if( {} ) return TA_BAD_PARAM;", checks.join(" || "));
        let _ = writeln!(o, "   scratch = *stream;");
        if let Some(model) = &cp.producer {
            emit_peek_mirror_fixups(o, model);
        }
        // Point each lag ring at its mirror so the step's ring push mutates the
        // scratch copy, leaving the live handle untouched (peek is const).
        for ring in &cp.sub_lag_rings {
            let s = &ring.series;
            let _ = writeln!(
                o,
                "   memcpy( scratch.lagRingMirror_{s}, stream->lagRing_{s}, sizeof(double) * (size_t)stream->lagRingCap_{s} );"
            );
            let _ = writeln!(o, "   scratch.lagRing_{s} = scratch.lagRingMirror_{s};");
        }
        let _ = writeln!(o, "   scratch.peekMode = 1;");
        let args: Vec<String> = inputs
            .iter()
            .cloned()
            .chain(outputs.iter().cloned())
            .collect();
        let _ = writeln!(o, "   TA_{n}_StreamStep( &scratch, {} );", args.join(", "));
        let _ = writeln!(o, "   return TA_SUCCESS;\n}}\n");
    }
    emit_composed_close(o, func, cp);
}

/// State struct for a loopless composed pipeline (no producer loop): the
/// optional params (referenced by combine maps as `sp-><param>`), plus the
/// peek flag and typed sub handles. Dispatch-style — no ring/window/circ/
/// extrema fields, so no `StreamRelease`.
fn emit_composed_struct_noproducer(o: &mut String, func: &FuncDef, extra: &str) {
    let n = uname(func);
    let _ = writeln!(o, "struct TA_{n}_Stream {{");
    for p in &func.optional_inputs {
        let _ = writeln!(o, "   {} {};", opt_param_c_type(&p.param_type), p.name);
    }
    for (name, c_type) in &func.private_extra_params {
        let _ = writeln!(o, "   {c_type} {name};");
    }
    o.push_str(extra);
    let _ = writeln!(o, "}};\n");
}

/// Open one sub-stream on its source series at the anchor
/// `max(0, sArg − callee_lookback)`, IMMEDIATELY before the batch call that
/// consumes it. Multi-input callees receive one `&src[subOff]` per input (all
/// sharing the single anchor — every batch body is startIdx-relative after
/// clamping, and the anchor is time-invariant in composed bodies), and
/// multi-output callees get one `&subOpenDummy` per output. On failure, the
/// inserted return replays every intermediate free the batch performs LATER
/// than this call (`series_frees` with a greater tail index): those series are
/// live here, and only an inserted return — not the batch's own early returns —
/// must free them (LeakSanitizer caught the omission on honest-rejection legs).
#[allow(clippy::too_many_arguments)]
fn emit_composed_sub_open(
    o: &mut String,
    cp: &streaming::ComposedPlan,
    sub: &streaming::SubCallStep,
    si: usize,
    outputs: &[String],
    cleanup: &str,
    open_map: &dyn Fn(Expr) -> Expr,
    enums: &HashMap<String, EnumDef>,
    registry: &Registry,
    helpers: &HelperRegistry,
    counter: &Cell<usize>,
) {
    let cpfx = callee_prefix(&sub.callee);
    let opt_str: String = sub.opt_args.iter().fold(String::new(), |mut s, a| {
        let _ = write!(s, "{}, ", render_expression(a, registry, helpers, counter));
        s
    });
    let s_arg = render_expression(
        &streaming::rewrite_expr(&sub.s_arg, open_map),
        registry,
        helpers,
        counter,
    );
    let e_arg = render_expression(
        &streaming::rewrite_expr(&sub.e_arg, open_map),
        registry,
        helpers,
        counter,
    );
    // One source pointer per callee input, from bar 0 (caller outputs live in
    // the scratch arrays; materialized intermediates and bar inputs keep their
    // name). The sub sees the FULL history from the origin and warms up at the
    // sub-call's own startIdx, so it seeds exactly as its batch would — the
    // seeding (incl. absolute-origin MA types under Metastock) stays inside the
    // callee's own Open, no anchor arithmetic here.
    let src_ptrs: String = sub
        .srcs
        .iter()
        .map(|src| {
            if outputs.contains(src) {
                format!("sc_{src}")
            } else {
                src.clone()
            }
        })
        .collect::<Vec<_>>()
        .join(", ");
    // One initial-output dummy per callee output.
    let out_dummies: String = std::iter::repeat_n("&subOpenDummy", sub.dsts.len())
        .collect::<Vec<_>>()
        .join(", ");
    let _ = writeln!(
        o,
        "      /* Sub-stream {si}: {} over `{}`, warmed from bar 0 up to the",
        sub.callee,
        sub.srcs.join(", ")
    );
    let _ = writeln!(o, "       * sub-call's own startIdx (the seeding point). */");
    let _ = writeln!(o, "      {{");
    let _ = writeln!(
        o,
        "         subRc = {cpfx}_OpenInternal( {opt_str}{src_ptrs}, ({s_arg}), ({e_arg}) + 1, &sub{si}, {out_dummies} );"
    );
    let _ = writeln!(o, "         if( subRc != TA_SUCCESS )");
    let _ = writeln!(o, "         {{");
    for sf in &cp.series_frees {
        if sf.tail_idx > sub.tail_idx {
            o.push_str(&render_statement(&sf.stmt, 12, false, enums, registry, helpers, counter));
        }
    }
    let _ = writeln!(o, "            {cleanup};");
    let _ = writeln!(o, "            return subRc;");
    let _ = writeln!(o, "         }}");
    let _ = writeln!(o, "      }}");
}

/// True for a bare `free(<series>)` of a lag-ring series: it is WITHHELD from
/// the transcribed tail (the ring must be captured from the buffer's tail
/// first) and re-emitted after the capture epilogue.
fn is_lag_ring_free(stmt: &Statement, rings: &[streaming::SubLagRing]) -> bool {
    matches!(stmt,
        Statement::Expr(Expr::FuncCall(name, args))
            if name == "free"
                && matches!(args.first(), Some(Expr::Var(v))
                    if rings.iter().any(|r| &r.series == v)))
}

/// Composed Open: scratch output arrays + verbatim transcription of the
/// batch body with sub-streams opened on the materialized series at the
/// exact points batch consumes them, then producer-state capture.
#[allow(clippy::too_many_arguments, clippy::too_many_lines)]
fn emit_composed_open(
    o: &mut String,
    func: &FuncDef,
    cp: &streaming::ComposedPlan,
    outputs: &[String],
    inputs: &[String],
    cleanup: &str,
    enums: &HashMap<String, EnumDef>,
    registry: &Registry,
    helpers: &HelperRegistry,
    counter: &Cell<usize>,
) {
    let n = uname(func);
    let _ = writeln!(o, "{}\n{{", open_internal_signature(func));
    let _ = writeln!(o, "   struct TA_{n}_Stream *sp;");
    let _ = writeln!(o, "   int endIdx;");
    let _ = writeln!(o, "   int dummyBegIdx;");
    let _ = writeln!(o, "   int dummyNBElement;");
    let _ = writeln!(o, "   TA_RetCode subRc;");
    let _ = writeln!(o, "   double subOpenDummy;");
    for out in outputs {
        let _ = writeln!(o, "   double *sc_{out};");
    }
    for (i, sub) in cp.subs.iter().enumerate() {
        let _ = writeln!(o, "   {}_Stream *sub{i};", callee_prefix(&sub.callee));
    }

    emit_open_validation(o, func, outputs, inputs);

    // startIdx arrives as a parameter (0 for standalone opens).
    let _ = writeln!(o, "\n   endIdx = historyLen - 1;");
    let _ = writeln!(o, "   dummyBegIdx = 0;");
    let _ = writeln!(o, "   dummyNBElement = 0;");
    let _ = writeln!(o, "   subRc = TA_SUCCESS;");
    let _ = writeln!(o, "   subOpenDummy = 0.0;");
    for (i, _) in cp.subs.iter().enumerate() {
        let _ = writeln!(o, "   sub{i} = NULL;");
    }
    let _ = writeln!(
        o,
        "   (void)startIdx; (void)dummyBegIdx; (void)dummyNBElement; (void)subRc; (void)subOpenDummy;"
    );
    // Scratch output arrays: the batch tail writes REAL arrays (sub-call
    // out args, memmoves) — a last-value scalar cannot stand in here.
    for (k, out) in outputs.iter().enumerate() {
        let _ = writeln!(
            o,
            "   sc_{out} = (double *)TA_Malloc( sizeof(double) * (size_t)historyLen );"
        );
        let prior: String = outputs[..k]
            .iter()
            .fold(String::new(), |mut s, p| {
                let _ = write!(s, "TA_Free( sc_{p} ); ");
                s
            });
        let _ = writeln!(o, "   if( !sc_{out} ) {{ {prior}return TA_ALLOC_ERR; }}");
    }

    // --- transcription ---------------------------------------------------------
    let _ = writeln!(o, "\n   {{");
    let (region_stmts, tail_stmts) = build_composed_open_bodies(cp, outputs, cleanup);
    let mut region_c = String::new();
    for s in &region_stmts {
        region_c.push_str(&render_statement(s, 6, false, enums, registry, helpers, counter));
    }
    let open_settings = crate::candle_settings::detect_candle_settings(&cp.region);
    if !open_settings.is_empty() {
        o.push_str(&emit_used_candle_unpacking(&open_settings, &region_c, 6));
    }
    o.push_str(&region_c);

    // Tail: statement by statement, opening each sub-stream on its source
    // series IMMEDIATELY BEFORE the batch call that consumes it (in-place
    // smoothing overwrites the raw series right here — order is the whole
    // point; the spike's wrong-order sabotage fails 4,394 legs).
    let open_map = composed_open_expr_fn(outputs);
    for (i, stmt) in tail_stmts.iter().enumerate() {
        for (si, sub) in cp.subs.iter().enumerate() {
            if sub.tail_idx == i {
                emit_composed_sub_open(
                    o, cp, sub, si, outputs, cleanup, &open_map, enums, registry, helpers, counter,
                );
            }
        }
        // Withhold a lag-ring series' bare free: the ring seeds from its buffer
        // tail in the capture epilogue, so the buffer must outlive the tail.
        if is_lag_ring_free(stmt, &cp.sub_lag_rings) {
            continue;
        }
        o.push_str(&render_statement(stmt, 6, false, enums, registry, helpers, counter));
    }

    // --- capture ----------------------------------------------------------------
    // A lag-ring series' buffer free is WITHHELD from the tail (it is seeded
    // into the ring below), so it is still live through the capture epilogue:
    // every error return here must free it too, or an allocation failure leaks
    // it. Empty (== `cleanup`) for non-lag-ring functions, whose intermediate
    // buffers were already freed in the transcribed tail.
    let withheld_frees: String = cp.sub_lag_rings.iter().fold(String::new(), |mut s, r| {
        let _ = write!(s, "free( {} ); ", r.series);
        s
    });
    let epilogue_cleanup = format!("{withheld_frees}{cleanup}");
    let _ = writeln!(o, "\n      /* Capture the live producer state + sub handles. */");
    let _ = writeln!(
        o,
        "      if( dummyNBElement < 1 ) {{ {epilogue_cleanup}; return TA_BAD_PARAM; }}"
    );
    if let Some(model) = &cp.producer {
        o.push_str(&alloc_and_capture(
            func, model, "      ", /*with_state=*/ true, cleanup, registry, helpers, counter,
        ));
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
    } else {
        // Loopless pipeline: no producer state to capture, just the params.
        let _ = writeln!(o, "      sp = (struct TA_{n}_Stream *)TA_Malloc( sizeof(*sp) );");
        let _ = writeln!(o, "      if( !sp ) {{ {epilogue_cleanup}; return TA_ALLOC_ERR; }}");
        let _ = writeln!(o, "      memset( sp, 0, sizeof(*sp) );");
        for p in &func.optional_inputs {
            let _ = writeln!(o, "      sp->{0} = {0};", p.name);
        }
        for (name, _) in &func.private_extra_params {
            let _ = writeln!(o, "      sp->{name} = {name};");
        }
    }
    // Sub-output lag rings: allocate, then seed from the tail of the (still
    // live — its free was withheld) intermediate buffer. `dummyNBElement` here
    // is the caller's own output count; the buffer holds `lag` MORE elements
    // (its range starts `lag` bars earlier), so its tail is `buf[dummyNBElement
    // + k]` for k in 0..lag — exactly the last `lag` sub-output values.
    for ring in &cp.sub_lag_rings {
        let s = &ring.series;
        let lag = render_expression(&ring.lag, registry, helpers, counter);
        let _ = writeln!(o, "      sp->lagRingCap_{s} = {lag};");
        let _ = writeln!(
            o,
            "      sp->lagRing_{s} = (double *)TA_Malloc( sizeof(double) * (size_t)sp->lagRingCap_{s} );"
        );
        let _ = writeln!(
            o,
            "      if( !sp->lagRing_{s} ) {{ TA_Free( sp ); {epilogue_cleanup}; return TA_ALLOC_ERR; }}"
        );
        let _ = writeln!(
            o,
            "      sp->lagRingMirror_{s} = (double *)TA_Malloc( sizeof(double) * (size_t)sp->lagRingCap_{s} );"
        );
        let _ = writeln!(
            o,
            "      if( !sp->lagRingMirror_{s} ) {{ TA_Free( sp->lagRing_{s} ); TA_Free( sp ); {epilogue_cleanup}; return TA_ALLOC_ERR; }}"
        );
        let _ = writeln!(o, "      {{");
        let _ = writeln!(o, "         int lagI;");
        let _ = writeln!(o, "         for( lagI = 0; lagI < sp->lagRingCap_{s}; lagI++ )");
        let _ = writeln!(o, "            sp->lagRing_{s}[lagI] = {s}[dummyNBElement + lagI];");
        let _ = writeln!(o, "      }}");
        let _ = writeln!(o, "      sp->lagRingPos_{s} = 0;");
        let _ = writeln!(o, "      free( {s} );");
    }
    for (i, _) in cp.subs.iter().enumerate() {
        let _ = writeln!(o, "      sp->sub{i} = sub{i};");
    }
    for out in outputs {
        let _ = writeln!(o, "      *{out} = sc_{out}[dummyNBElement - 1];");
    }
    for out in outputs {
        let _ = writeln!(o, "      TA_Free( sc_{out} );");
    }
    let _ = writeln!(o, "      *stream = sp;");
    let _ = writeln!(o, "      return TA_SUCCESS;");
    let _ = writeln!(o, "   }}\n}}\n");
    emit_open_wrapper(o, func);
}

/// The composed-Open expression mapping: out-meta pointers to the dummies —
/// in BOTH forms: `*outNBElement` reads/writes (deref) AND `outNBElement`
/// passed through as a pointer argument to the batch sub-calls — plus
/// output arrays renamed to their scratch names (`outX` -> `sc_outX`, both
/// bare Var pointer uses and ArrayAccess bases).
fn composed_open_expr_fn(outputs: &[String]) -> impl Fn(Expr) -> Expr + '_ {
    move |e: Expr| -> Expr {
        match e {
            Expr::PointerDeref(nm) if nm == "outBegIdx" => Expr::Var("dummyBegIdx".into()),
            Expr::PointerDeref(nm) if nm == "outNBElement" => {
                Expr::Var("dummyNBElement".into())
            }
            Expr::Var(v) if v == "outBegIdx" => {
                Expr::AddressOf(Box::new(Expr::Var("dummyBegIdx".into())))
            }
            Expr::Var(v) if v == "outNBElement" => {
                Expr::AddressOf(Box::new(Expr::Var("dummyNBElement".into())))
            }
            Expr::Var(v) if outputs.contains(&v) => Expr::Var(format!("sc_{v}")),
            Expr::ArrayAccess(name, idx) if outputs.contains(&name) => {
                Expr::ArrayAccess(format!("sc_{name}"), idx)
            }
            other => other,
        }
    }
}

/// `name = malloc(...); if (!name) { cleanup; return ALLOC_ERR; }` — the batch
/// bodies malloc intermediate series without a NULL check (a pre-existing batch
/// defect that surfaces as UB on this NEW API surface). The `= malloc` is
/// lowered to a plain assignment so the declaration-with-initializer form
/// (STOCHRSI's `double *tempRSIBuffer = malloc(...)`) does not re-declare a
/// series the body already declares elsewhere — matching what the batch
/// backend's decl-hoisting does.
fn malloc_null_check_block(name: &str, call: Expr, cleanup: &str) -> Statement {
    Statement::Block {
        body: vec![
            Statement::Assign {
                target: Expr::Var(name.to_string()),
                value: call,
                compound: false,
            },
            Statement::If {
                condition: Expr::Not(Box::new(Expr::Var(name.to_string()))),
                then_body: vec![
                    Statement::Expr(Expr::Var(cleanup.to_string())),
                    Statement::Return {
                        value: Some(Expr::Var("ALLOC_ERR".into())),
                    },
                ],
                else_body: vec![],
                cond_comments: vec![],
            },
        ],
    }
}

/// The transcribed (region, tail) statement lists for the composed Open:
/// out-meta pointers to dummies, output arrays renamed to scratch, early
/// returns mapped (success -> BAD_PARAM) with the cleanup prepended, final
/// tail return dropped.
fn build_composed_open_bodies(
    cp: &streaming::ComposedPlan,
    outputs: &[String],
    cleanup: &str,
) -> (Vec<Statement>, Vec<Statement>) {
    let fe = composed_open_expr_fn(outputs);
    let cleanup_owned = cleanup.to_string();
    let intermediates: std::collections::BTreeSet<String> =
        cp.intermediates.iter().cloned().collect();
    // Each intermediate's malloc-failure cleanup must free every intermediate
    // allocated BEFORE it (BBANDS allocates tempBuffer1 then tempBuffer2 — if
    // tempBuffer2's malloc fails, tempBuffer1 must be freed or it leaks). Track
    // them in the order the region allocates them; a malloc's cleanup prepends
    // `free()` of the ones already live. The base cleanup (close subs + free
    // scratch) is enough for a plain early `Return`, whose source already frees
    // its own intermediates explicitly.
    let cleanup_for_malloc = cleanup_owned.clone();
    let allocated_before: std::cell::RefCell<Vec<String>> = std::cell::RefCell::new(Vec::new());
    let malloc_cleanup = move |name: &str| -> String {
        let prior: String =
            allocated_before
                .borrow()
                .iter()
                .fold(String::new(), |mut s, n: &String| {
                    let _ = std::fmt::Write::write_fmt(&mut s, format_args!("free( {n} ); "));
                    s
                });
        allocated_before.borrow_mut().push(name.to_string());
        format!("{prior}{cleanup_for_malloc}")
    };
    let fs = move |s: Statement| -> Option<Statement> {
        match s {
            // Assignment form (`tempBuffer = malloc(...)`, STOCH). A
            // cast-wrapped or TA_Malloc allocation is recognized too.
            Statement::Assign {
                target: Expr::Var(v),
                value,
                ..
            } if intermediates.contains(&v) && streaming::expr_allocates(&value) => {
                let cu = malloc_cleanup(&v);
                Some(malloc_null_check_block(&v, value, &cu))
            }
            // Declaration-with-initializer form
            // (`double *tempRSIBuffer = malloc(...)`, STOCHRSI).
            Statement::VarDecl {
                name,
                init: Some(init),
                ..
            } if intermediates.contains(&name) && streaming::expr_allocates(&init) => {
                let cu = malloc_cleanup(&name);
                Some(malloc_null_check_block(&name, init, &cu))
            }
            Statement::Return { value } => {
                let mapped = match value {
                    Some(Expr::Var(v)) if matches!(v.as_str(), "SUCCESS" | "TA_SUCCESS") => {
                        Some(Expr::Var("BAD_PARAM".into()))
                    }
                    other => other,
                };
                // Close the subs opened so far and free the scratch arrays
                // on every early exit (Close(NULL) is a no-op, so one
                // uniform cleanup text is safe on every path).
                Some(Statement::Block {
                    body: vec![
                        Statement::Expr(Expr::Var(cleanup_owned.clone())),
                        Statement::Return { value: mapped },
                    ],
                })
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

// ---------------------------------------------------------------------------
// Dispatch emission (MA): a tagged handle over the callees' PUBLIC streams.
// ---------------------------------------------------------------------------

/// `TA_<CALLEE>` for an input-level callee name (`sma` -> `TA_SMA`).
fn callee_prefix(callee: &str) -> String {
    format!("TA_{}", callee.to_uppercase())
}

/// The identity condition with the caller's optional params redirected
/// through the handle (`optInTimePeriod == 1` -> `stream->optInTimePeriod == 1`).
fn dispatch_identity_cond_on_handle(
    func: &FuncDef,
    dp: &DispatchPlan,
    registry: &Registry,
    helpers: &HelperRegistry,
    counter: &Cell<usize>,
) -> Option<String> {
    let idp = dp.identity.as_ref()?;
    let params: Vec<String> = func.optional_inputs.iter().map(|p| p.name.clone()).collect();
    let cond = streaming::rewrite_expr(&idp.condition, &|e| match e {
        Expr::Var(v) if params.contains(&v) => Expr::Var(format!("stream->{v}")),
        other => other,
    });
    Some(render_expression(&cond, registry, helpers, counter))
}

/// Per-arm dispatch bodies for Update/Peek/Close, plus the shared open
/// switch. All labels render through the batch's own `ENUM_CASE` mapping so
/// the arms read exactly like the batch dispatch they mirror.
#[allow(clippy::too_many_lines)]
fn emit_dispatch(
    o: &mut String,
    func: &FuncDef,
    dp: &DispatchPlan,
    enums: &HashMap<String, EnumDef>,
    registry: &Registry,
    helpers: &HelperRegistry,
    counter: &Cell<usize>,
) {
    let n = uname(func);
    let inputs = streaming::input_array_names(func);
    let outputs: Vec<String> = func.outputs.iter().map(|x| x.name.clone()).collect();
    let bar_args: String = inputs.join(", ");
    let out_args: String = outputs.join(", ");
    let case_of = |label: &str| render_c_switch_label(label, enums);

    // --- state struct -------------------------------------------------------
    let _ = writeln!(o, "struct TA_{n}_Stream {{");
    for p in &func.optional_inputs {
        let _ = writeln!(o, "   {} {};", opt_param_c_type(&p.param_type), p.name);
    }
    let _ = writeln!(
        o,
        "   /* Sub-stream handle, tagged by {}; NULL on the identity path. */",
        dp.param
    );
    let _ = writeln!(o, "   void *sub;");
    let _ = writeln!(o, "}};\n");

    // --- Open ----------------------------------------------------------------
    let _ = writeln!(o, "{}\n{{", open_internal_signature(func));
    let _ = writeln!(o, "   struct TA_{n}_Stream *sp;");
    let _ = writeln!(o, "   TA_RetCode retCode;");
    let _ = writeln!(o, "\n   if( !stream ) return TA_BAD_PARAM;");
    let _ = writeln!(o, "   *stream = NULL;");
    let null_checks: Vec<String> = inputs
        .iter()
        .chain(outputs.iter())
        .map(|x| format!("!{x}"))
        .collect();
    let _ = writeln!(o, "   if( {} ) return TA_BAD_PARAM;", null_checks.join(" || "));
    let _ = writeln!(o, "   if( historyLen < 1 ) return TA_BAD_PARAM;");
    let _ = writeln!(o, "   (void)startIdx;");
    o.push_str(&emit_opt_param_validation(func, "TA_BAD_PARAM"));
    let _ = writeln!(
        o,
        "\n   sp = (struct TA_{n}_Stream *)TA_Malloc( sizeof(*sp) );"
    );
    let _ = writeln!(o, "   if( !sp ) return TA_ALLOC_ERR;");
    let _ = writeln!(o, "   memset( sp, 0, sizeof(*sp) );");
    for p in &func.optional_inputs {
        let _ = writeln!(o, "   sp->{0} = {0};", p.name);
    }
    if let Some(idp) = &dp.identity {
        // The batch checks the identity path BEFORE the dispatch, for every
        // arm value — mirror the order (min_history holds: the lookback is 0
        // on this path for every arm).
        let cond = render_expression(&idp.condition, registry, helpers, counter);
        let lookback_args: Vec<String> =
            func.optional_inputs.iter().map(|p| p.name.clone()).collect();
        let _ = writeln!(o, "\n   if( {cond} )\n   {{");
        let _ = writeln!(
            o,
            "      if( historyLen < TA_{n}_Lookback( {} ) + 1 ) {{ TA_Free( sp ); return TA_BAD_PARAM; }}",
            lookback_args.join(", ")
        );
        for (out, inp) in &idp.pairs {
            let _ = writeln!(o, "      *{out} = {inp}[historyLen - 1];");
        }
        let _ = writeln!(o, "      *stream = sp;");
        let _ = writeln!(o, "      return TA_SUCCESS;");
        let _ = writeln!(o, "   }}");
    }
    let _ = writeln!(o, "\n   retCode = TA_BAD_PARAM;");
    let _ = writeln!(o, "   switch( {} )", dp.param);
    let _ = writeln!(o, "   {{");
    for arm in dp.arms.iter().filter(|a| a.supported) {
        let cp = callee_prefix(&arm.callee);
        let opt_args: Vec<String> = arm
            .opt_args
            .iter()
            .map(|e| render_expression(e, registry, helpers, counter))
            .collect();
        let opt_str = opt_args
            .iter()
            .fold(String::new(), |mut s, a| {
                let _ = write!(s, "{a}, ");
                s
            });
        let _ = writeln!(o, "   case {}:", case_of(&arm.label));
        let _ = writeln!(o, "      {{");
        let _ = writeln!(o, "         {cp}_Stream *sub = NULL;");
        let _ = writeln!(
            o,
            "         retCode = {cp}_OpenInternal( {opt_str}{bar_args}, startIdx, historyLen, &sub, {out_args} );",
        );
        let _ = writeln!(o, "         sp->sub = sub;");
        let _ = writeln!(o, "      }}");
        let _ = writeln!(o, "      break;");
    }
    // Unsupported arms reject at Open — a documented capability limitation
    // (the callee has no stream yet). They regenerate as supported arms the
    // moment the callee's YAML gains the stream flag.
    for arm in dp.arms.iter().filter(|a| !a.supported) {
        let _ = writeln!(
            o,
            "   case {}: /* no {} stream */",
            case_of(&arm.label),
            if arm.callee.is_empty() { "delegation" } else { &arm.callee }
        );
    }
    let _ = writeln!(o, "   default:");
    let _ = writeln!(o, "      retCode = TA_BAD_PARAM;");
    let _ = writeln!(o, "      break;");
    let _ = writeln!(o, "   }}");
    let _ = writeln!(o, "\n   if( retCode != TA_SUCCESS )");
    let _ = writeln!(o, "   {{");
    let _ = writeln!(o, "      TA_Free( sp );");
    let _ = writeln!(o, "      return retCode;");
    let _ = writeln!(o, "   }}");
    let _ = writeln!(o, "   *stream = sp;");
    let _ = writeln!(o, "   return TA_SUCCESS;\n}}\n");
    emit_open_wrapper(o, func);

    // --- Update / Peek ---------------------------------------------------------
    let identity_handle_cond =
        dispatch_identity_cond_on_handle(func, dp, registry, helpers, counter);
    for verb in ["Update", "Peek"] {
        let sig = if verb == "Update" {
            update_signature(func)
        } else {
            peek_signature(func)
        };
        let const_qual = if verb == "Peek" { "const " } else { "" };
        let _ = writeln!(o, "{sig}\n{{");
        let checks: Vec<String> = std::iter::once("!stream".to_string())
            .chain(outputs.iter().map(|x| format!("!{x}")))
            .collect();
        let _ = writeln!(o, "   if( {} ) return TA_BAD_PARAM;", checks.join(" || "));
        if let (Some(cond), Some(idp)) = (&identity_handle_cond, &dp.identity) {
            let _ = writeln!(o, "   if( {cond} )\n   {{");
            for (out, inp) in &idp.pairs {
                let _ = writeln!(o, "      *{out} = {inp};");
            }
            let _ = writeln!(o, "      return TA_SUCCESS;");
            let _ = writeln!(o, "   }}");
        }
        let _ = writeln!(o, "   switch( stream->{} )", dp.param);
        let _ = writeln!(o, "   {{");
        for arm in dp.arms.iter().filter(|a| a.supported) {
            let cp = callee_prefix(&arm.callee);
            let _ = writeln!(o, "   case {}:", case_of(&arm.label));
            let _ = writeln!(
                o,
                "      return {cp}_{verb}( ({const_qual}{cp}_Stream *)stream->sub, {bar_args}, {out_args} );"
            );
        }
        let _ = writeln!(o, "   default:");
        let _ = writeln!(o, "      /* Unreachable: Open rejects arms without a sub-stream. */");
        let _ = writeln!(o, "      return TA_INTERNAL_ERROR;");
        let _ = writeln!(o, "   }}\n}}\n");
    }

    // --- Close -----------------------------------------------------------------
    let _ = writeln!(o, "{}\n{{", close_signature(func));
    let _ = writeln!(o, "   if( !stream ) return TA_SUCCESS;");
    let _ = writeln!(o, "   switch( stream->{} )", dp.param);
    let _ = writeln!(o, "   {{");
    for arm in dp.arms.iter().filter(|a| a.supported) {
        let cp = callee_prefix(&arm.callee);
        let _ = writeln!(o, "   case {}:", case_of(&arm.label));
        let _ = writeln!(
            o,
            "      {cp}_Close( ({cp}_Stream *)stream->sub );"
        );
        let _ = writeln!(o, "      break;");
    }
    let _ = writeln!(o, "   default:");
    let _ = writeln!(o, "      break; /* identity-only or rejected arm: no sub-stream */");
    let _ = writeln!(o, "   }}");
    let _ = writeln!(o, "   TA_Free( stream );");
    let _ = writeln!(o, "   return TA_SUCCESS;\n}}\n");
}

// ---------------------------------------------------------------------------
// Dual-mode emission (DI/DM class): two param-selected inline steady loops
// sharing one handle. See streaming::DualModePlan.
// ---------------------------------------------------------------------------

/// Render the arm predicate (`optInTimePeriod <= 1`) either bare (Open, where
/// the param is a local) or handle-qualified (`sp->optInTimePeriod <= 1`, for
/// the Step which re-selects the mode from the immutable stored param).
fn render_dual_pred(
    pred: &Expr,
    on_handle: bool,
    func: &FuncDef,
    registry: &Registry,
    helpers: &HelperRegistry,
    counter: &Cell<usize>,
) -> String {
    let params: Vec<String> = func.optional_inputs.iter().map(|p| p.name.clone()).collect();
    let e = if on_handle {
        streaming::rewrite_expr(pred, &|x| match x {
            Expr::Var(v) if params.contains(&v) => Expr::Var(format!("sp->{v}")),
            other => other,
        })
    } else {
        pred.clone()
    };
    render_expression(&e, registry, helpers, counter)
}

/// The dual-mode state struct: optional params (incl. the discriminator param),
/// then the TYPE-CHECKED UNION of both modes' scalar state (a name shared by
/// the two modes — DI/DM's `prevHigh`/`prevLow`/`prevClose` — is one field;
/// mode-B-only fields sit zeroed under mode A). No `mode` tag is stored: the
/// step re-derives it from the immutable `sp->optInTimePeriod` (the Dispatch
/// precedent). M6a modes are pure scalar; a mode carrying rings/windows/circs/
/// extrema/out-feedback/lags is rejected loudly (extend this when TRIMA lands).
fn emit_dual_state_struct(o: &mut String, func: &FuncDef, ma: &StreamModel, mb: &StreamModel) {
    for m in [ma, mb] {
        assert!(
            m.out_feedback.is_empty()
                && m.lags.is_empty()
                && m.rings().is_empty()
                && m.windows().is_empty()
                && m.circs().is_empty()
                && m.extrema().is_none(),
            "{}: dual-mode with non-scalar state (rings/windows/circs/extrema/feedback/lags) \
             is not supported yet — extend emit_dual_mode",
            func.name
        );
    }
    let n = uname(func);
    let _ = writeln!(o, "struct TA_{n}_Stream {{");
    for p in &func.optional_inputs {
        let _ = writeln!(o, "   {} {};", opt_param_c_type(&p.param_type), p.name);
    }
    for (name, c_type) in &func.private_extra_params {
        let _ = writeln!(o, "   {c_type} {name};");
    }
    // Union of the two modes' state, mode-A order first, dedup by name.
    let mut seen: std::collections::BTreeMap<String, &crate::ir::VarType> =
        std::collections::BTreeMap::new();
    let mut order: Vec<(String, crate::ir::VarType)> = Vec::new();
    for (name, ty) in ma.state.iter().chain(mb.state.iter()) {
        if let Some(prev) = seen.get(name) {
            assert!(
                *prev == ty,
                "{}: dual-mode state `{name}` has conflicting types across modes",
                func.name
            );
        } else {
            seen.insert(name.clone(), ty);
            order.push((name.clone(), ty.clone()));
        }
    }
    for (name, ty) in &order {
        let _ = writeln!(o, "   {};", c_decl(ty, name));
    }
    let _ = writeln!(o, "}};\n");
}

/// Remove top-level `VarDecl`s whose variable is never referenced elsewhere in
/// `body`. Used only for the dual-mode Open arms: each arm is `shared prologue
/// ++ its own arm body`, and the prologue declares the UNION of both modes'
/// function-top scalars, so the degenerate arm would otherwise carry (and
/// -Wunused-warn on) the Wilder-path accumulators and warm-up counter it never
/// touches. A decl's own name is not a "use" (only its initializer is walked),
/// and a decl kept here is exactly one the arm reads or writes — so dropping
/// the rest is behavior-preserving.
fn drop_unused_decls(body: Vec<Statement>) -> Vec<Statement> {
    let mut used: std::collections::BTreeSet<String> = std::collections::BTreeSet::new();
    for s in &body {
        streaming::walk_stmt_exprs(s, &mut |e| {
            streaming::walk_expr(e, &mut |x| {
                if let Expr::Var(v) = x {
                    used.insert(v.clone());
                }
            });
        });
    }
    body.into_iter()
        .filter(|s| !matches!(s, Statement::VarDecl { name, .. } if !used.contains(name)))
        .collect()
}

/// Emit the full dual-mode stream section: one union struct, one predicate-
/// branching StreamStep, one predicate-branching OpenInternal (+ public Open
/// wrapper), and Update/Peek/Close reused from the loop tier (mode-independent
/// for scalar modes — the stored param rides the struct copy through Peek).
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
    let n = uname(func);
    let ma = &dmp.mode_a;
    let mb = &dmp.mode_b;

    // --- state struct -------------------------------------------------------
    emit_dual_state_struct(o, func, ma, mb);

    // --- Step: one function, mode selected from the stored param ------------
    let bars = bar_params_sig(func);
    let outs = out_params_sig(func);
    let _ = writeln!(
        o,
        "static void TA_{n}_StreamStep( struct TA_{n}_Stream *sp, {bars}{outs} )\n{{"
    );
    let pred_h = render_dual_pred(&dmp.predicate, true, func, registry, helpers, counter);
    let _ = writeln!(o, "   if( {pred_h} )\n   {{");
    emit_step_inner(o, ma, enums, registry, helpers, counter, 6, false);
    let _ = writeln!(o, "   }}\n   else\n   {{");
    emit_step_inner(o, mb, enums, registry, helpers, counter, 6, false);
    let _ = writeln!(o, "   }}\n}}\n");

    // --- OpenInternal: shared head, then a predicate branch per mode --------
    let inputs = streaming::input_array_names(func);
    let _ = writeln!(o, "{}\n{{", open_internal_signature(func));
    let _ = writeln!(o, "   struct TA_{n}_Stream *sp;");
    let _ = writeln!(o, "   int endIdx;");
    let _ = writeln!(o, "   int dummyBegIdx;");
    let _ = writeln!(o, "   int dummyNBElement;");
    for out in &ma.outputs {
        let _ = writeln!(o, "   {} lastValue_{out};", out_c_type(func, out));
    }
    for (name, c_type) in &func.private_extra_params {
        let _ = writeln!(o, "   {c_type} {name};");
    }
    emit_open_validation(o, func, &ma.outputs, &inputs);
    let _ = writeln!(o, "\n   endIdx = historyLen - 1;");
    let _ = writeln!(o, "   dummyBegIdx = 0;");
    let _ = writeln!(o, "   dummyNBElement = 0;");
    for out in &ma.outputs {
        let init = if out_c_type(func, out) == "int" { "0" } else { "0.0" };
        let _ = writeln!(o, "   lastValue_{out} = {init};");
    }
    let _ = writeln!(
        o,
        "   (void)startIdx; (void)dummyBegIdx; (void)dummyNBElement;"
    );

    // Each mode transcribes the SHARED PROLOGUE then its own arm body, seeding
    // its own state and returning. The prologue computes the mode-appropriate
    // lookback/clamp, so min-history is per-mode correct by construction. The
    // shared prologue declares the UNION of both modes' function-top locals, so
    // a per-arm dead-decl drop is applied: the degenerate arm never touches the
    // Wilder accumulators or the warm-up counter, and shipping their decls would
    // emit -Wunused-variable in the generated C.
    let compose = |arm_body: &[Statement]| -> Vec<Statement> {
        let mut v = dmp.prologue.to_vec();
        v.extend_from_slice(arm_body);
        drop_unused_decls(v)
    };
    let pred_bare = render_dual_pred(&dmp.predicate, false, func, registry, helpers, counter);
    let body_a = compose(ma.body);
    let body_b = compose(mb.body);
    let _ = writeln!(o, "\n   if( {pred_bare} )\n   {{");
    emit_open_arm(o, func, ma, &body_a, enums, registry, helpers, counter);
    let _ = writeln!(o, "   }}\n   else\n   {{");
    emit_open_arm(o, func, mb, &body_b, enums, registry, helpers, counter);
    let _ = writeln!(o, "   }}");
    // Both arms return; keep the compiler happy about the fall-through.
    let _ = writeln!(o, "\n   return TA_INTERNAL_ERROR;\n}}\n");
    emit_open_wrapper(o, func);

    // --- Update / Peek / Close (mode-independent for scalar modes) ----------
    emit_update(o, func);
    emit_peek(o, func, ma);
    emit_close(o, func, ma);
}

fn emit_state_struct(o: &mut String, func: &FuncDef, model: &StreamModel) {
    emit_state_struct_ex(o, func, model, "");
}

/// State struct with extra trailing fields (composed tier: peekMode + typed
/// sub handles appended after the producer's own fields).
fn emit_state_struct_ex(o: &mut String, func: &FuncDef, model: &StreamModel, extra: &str) {
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
    for name in &model.out_feedback {
        let _ = writeln!(o, "   {} lastOut_{name};", out_c_type(func, name));
    }
    for lag in &model.lags {
        for k in 1..=lag.depth {
            let _ = writeln!(o, "   double {};", StreamModel::lag_field(&lag.array, k));
        }
    }
    for ring in model.rings() {
        let v = &ring.var;
        let _ = writeln!(o, "   int ringPos_{v};");
        let _ = writeln!(o, "   int ringCap_{v};");
        if ring.back > 0 {
            let _ = writeln!(o, "   int ringLag_{v};");
        }
        for arr in &ring.arrays {
            let _ = writeln!(o, "   double *ring_{v}_{arr};");
            // Scratch mirror for Peek: pre-allocated at open so Peek stays
            // allocation-free (proposal: forming-bar evaluation).
            let _ = writeln!(o, "   double *ringMirror_{v}_{arr};");
        }
    }
    for win in model.windows() {
        let v = &win.var;
        let _ = writeln!(o, "   int winPos_{v};");
        let _ = writeln!(o, "   int winCap_{v};");
        for arr in &win.arrays {
            let _ = writeln!(o, "   double *win_{v}_{arr};");
            let _ = writeln!(o, "   double *winMirror_{v}_{arr};");
        }
    }
    for circ in model.circs() {
        let _ = writeln!(o, "   int cbSize_{};", circ.id);
        for (storage, ty) in circ_storages(circ) {
            let et = if matches!(ty, crate::ir::VarType::Integer) { "int" } else { "double" };
            let _ = writeln!(o, "   {et} *cb_{storage};");
            let _ = writeln!(o, "   {et} *cbMirror_{storage};");
        }
    }
    if let Some(ex) = model.extrema() {
        let _ = writeln!(o, "   int xCap;");
        for arr in &ex.arrays {
            let _ = writeln!(o, "   double *x_{arr};");
            let _ = writeln!(o, "   double *xMirror_{arr};");
        }
    }
    o.push_str(extra);
    // A struct must have at least one member (T1 maps carry none).
    if extra.is_empty()
        && func.optional_inputs.is_empty()
        && func.private_extra_params.is_empty()
        && model.state.is_empty()
        && model.lags.is_empty()
    {
        let _ = writeln!(o, "   int unused; /* T1: stateless map */");
    }
    let _ = writeln!(o, "}};\n");
}

/// `static void TA_<N>_StreamRelease(...)`: frees every ring buffer and the
/// handle itself. Emitted only for ring models; safe on partially-allocated
/// handles (open memsets the struct, so unallocated buffers are NULL).
fn emit_release(o: &mut String, func: &FuncDef, model: &StreamModel) {
    if !model.needs_release() {
        return;
    }
    let n = uname(func);
    let _ = writeln!(o, "static void TA_{n}_StreamRelease( struct TA_{n}_Stream *sp )
{{");
    let _ = writeln!(o, "   if( !sp ) return;");
    for ring in model.rings() {
        for arr in &ring.arrays {
            let _ = writeln!(o, "   if( sp->ring_{0}_{arr} ) TA_Free( sp->ring_{0}_{arr} );", ring.var);
            let _ = writeln!(o, "   if( sp->ringMirror_{0}_{arr} ) TA_Free( sp->ringMirror_{0}_{arr} );", ring.var);
        }
    }
    for win in model.windows() {
        for arr in &win.arrays {
            let _ = writeln!(o, "   if( sp->win_{0}_{arr} ) TA_Free( sp->win_{0}_{arr} );", win.var);
            let _ = writeln!(o, "   if( sp->winMirror_{0}_{arr} ) TA_Free( sp->winMirror_{0}_{arr} );", win.var);
        }
    }
    for circ in model.circs() {
        for (storage, _) in circ_storages(circ) {
            let _ = writeln!(o, "   if( sp->cb_{storage} ) TA_Free( sp->cb_{storage} );");
            let _ = writeln!(o, "   if( sp->cbMirror_{storage} ) TA_Free( sp->cbMirror_{storage} );");
        }
    }
    if let Some(ex) = model.extrema() {
        for arr in &ex.arrays {
            let _ = writeln!(o, "   if( sp->x_{arr} ) TA_Free( sp->x_{arr} );");
            let _ = writeln!(o, "   if( sp->xMirror_{arr} ) TA_Free( sp->xMirror_{arr} );");
        }
    }
    let _ = writeln!(o, "   TA_Free( sp );
}}
");
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
    let void_sp = model.state.is_empty()
        && func.optional_inputs.is_empty()
        && func.private_extra_params.is_empty()
        && model.lags.is_empty();
    emit_step_inner(o, model, enums, registry, helpers, counter, 3, void_sp);
    let _ = writeln!(o, "}}\n");
}

/// The per-bar step body for ONE model at a given indent: temp decls, an
/// optional `(void)sp`, the extrema rebase, the rendered transition, and
/// candle-settings unpacking. Shared by the single-model [`emit_step`] and the
/// dual-mode step (called once per arm inside the `if (sp->param ...)` branch,
/// at a deeper indent, with `void_sp = false` since a mode always has state).
fn emit_step_inner(
    o: &mut String,
    model: &StreamModel,
    enums: &HashMap<String, EnumDef>,
    registry: &Registry,
    helpers: &HelperRegistry,
    counter: &Cell<usize>,
    indent: usize,
    void_sp: bool,
) {
    let pad = " ".repeat(indent);
    for (name, ty) in &model.temps {
        let _ = writeln!(o, "{pad}{};", c_decl(ty, name));
    }
    if !model.temps.is_empty() {
        let _ = writeln!(o);
    }
    if void_sp {
        let _ = writeln!(o, "{pad}(void)sp;");
    }
    emit_extrema_rebase(o, model);
    let transition = streaming::build_transition(model, &CNames)
        .unwrap_or_else(|e| panic!("streaming transition: {e}"));
    let mut body_c = String::new();
    for s in &transition {
        body_c.push_str(&render_statement_stream(s, indent, enums, registry, helpers, counter));
    }
    // Candle settings are read where batch reads them (per step, from the
    // globals — the settings-stability rule). The TA_STREAM_CANDLE* macros
    // read the globals directly, so hoisted locals are emitted only when
    // the rendered body actually references them (no dead decls/-Wunused).
    let step_settings = crate::candle_settings::detect_candle_settings(&model.steady_stmts);
    if !step_settings.is_empty() {
        o.push_str(&emit_used_candle_unpacking(&step_settings, &body_c, indent));
    }
    o.push_str(&body_c);
}

/// Extrema automatons carry batch-absolute int indices that grow by one
/// per bar. Rebase them by a multiple of the ring capacity long before
/// INT_MAX: index differences and `% cap` residues are invariant, so the
/// automaton (and bit-exactness vs any batch-comparable range, which is
/// itself bounded by int) is untouched. Index-observable outputs
/// (MININDEX...) report the rebased position beyond ~2^30 bars — the
/// batch contract is inherently vacuous past INT_MAX bars.
fn emit_extrema_rebase(o: &mut String, model: &StreamModel) {
    if let Some(ex) = model.extrema() {
        let mut vars: Vec<String> = vec![model.cursor.clone(), ex.trailing.clone()];
        vars.extend(ex.index_vars.iter().cloned());
        let _ = writeln!(o, "   if( sp->{} >= 1073741824 )", model.cursor);
        let _ = writeln!(o, "   {{");
        let _ = writeln!(
            o,
            "      int rebaseShift = ( sp->{} / sp->xCap ) * sp->xCap;",
            ex.trailing
        );
        for v in &vars {
            let _ = writeln!(o, "      sp->{v} -= rebaseShift;");
        }
        let _ = writeln!(o, "   }}");
    }
}

/// Emit candle-settings unpacking lines only for the `<Set>_<prop>` locals
/// the rendered code actually references.
fn emit_used_candle_unpacking(
    settings: &std::collections::BTreeSet<String>,
    rendered: &str,
    indent: usize,
) -> String {
    let pad = " ".repeat(indent);
    let mut out = String::new();
    for set in settings {
        for (prop, cty) in [("rangeType", "int"), ("avgPeriod", "int"), ("factor", "double")] {
            let local = format!("{set}_{prop}");
            if rendered.contains(&local) {
                let _ = writeln!(
                    out,
                    "{pad}{cty} {local} = TA_Globals->candleSettings[TA_{set}].{prop};"
                );
            }
        }
    }
    out
}

/// The `OpenInternal` head shared by the loop tier and the fast-path-skip tier:
/// signature, declarations, param validation, initialization, and the identity
/// fast path. The caller then emits the transcribed body arm(s) and closes the
/// function.
fn emit_open_head(
    o: &mut String,
    func: &FuncDef,
    model: &StreamModel,
    registry: &Registry,
    helpers: &HelperRegistry,
    counter: &Cell<usize>,
) {
    let n = uname(func);
    let inputs = streaming::input_array_names(func);
    let _ = writeln!(o, "{}\n{{", open_internal_signature(func));

    // --- declarations -------------------------------------------------------
    let _ = writeln!(o, "   struct TA_{n}_Stream *sp;");
    emit_circ_hoist(o, func, model);
    let _ = writeln!(o, "   int endIdx;");
    let _ = writeln!(o, "   int dummyBegIdx;");
    let _ = writeln!(o, "   int dummyNBElement;");
    for out in &model.outputs {
        let _ = writeln!(o, "   {} lastValue_{out};", out_c_type(func, out));
    }
    for (name, c_type) in &func.private_extra_params {
        let _ = writeln!(o, "   {c_type} {name};");
    }

    emit_open_validation(o, func, &model.outputs, &inputs);

    // --- initialization (after defaults are substituted) ---------------------
    // startIdx arrives as a parameter (0 for standalone opens; the sub-call's
    // own startIdx when a composed function opens this as a sub-stream).
    let _ = writeln!(o, "\n   endIdx = historyLen - 1;");
    let _ = writeln!(o, "   dummyBegIdx = 0;");
    let _ = writeln!(o, "   dummyNBElement = 0;");
    for out in &model.outputs {
        let init = if out_c_type(func, out) == "int" { "0" } else { "0.0" };
        let _ = writeln!(o, "   lastValue_{out} = {init};");
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

    emit_identity_fast_path(o, func, model, registry, helpers, counter);
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
    emit_open_head(o, func, model, registry, helpers, counter);
    emit_open_arm(o, func, model, model.body, enums, registry, helpers, counter);
    let _ = writeln!(o, "}}\n");
    emit_open_wrapper(o, func);
}

/// Emit the fast-path-skip stream section (MIDPRICE): the standard loop-tier
/// lifecycle for the general (else) arm's model, except the OpenInternal
/// transcribes `prologue ++ general-arm body ++ epilogue` — the fast-path
/// `then` arm is skipped (a batch-only perf specialization). Struct / Step /
/// Update / Peek / Close are the ordinary single-model emitters.
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
    emit_state_struct(o, func, model);
    emit_release(o, func, model);
    emit_step(o, func, model, enums, registry, helpers, counter);

    emit_open_head(o, func, model, registry, helpers, counter);
    // prologue ++ general arm ++ epilogue: the Open seeds the general path for
    // every param (the skipped fast-path arm is bit-identical by construction).
    // drop_unused_decls prunes any fast-path-only locals the skipped arm owned.
    let mut body = plan.prologue.to_vec();
    body.extend_from_slice(model.body);
    body.extend_from_slice(plan.epilogue);
    let body = drop_unused_decls(body);
    emit_open_arm(o, func, model, &body, enums, registry, helpers, counter);
    let _ = writeln!(o, "}}\n");
    emit_open_wrapper(o, func);

    emit_update(o, func);
    emit_peek(o, func, model);
    emit_close(o, func, model);
}

/// One Open arm: the transcribed batch body region + live state capture,
/// wrapped in a `{ ... }` block ending in `emit_open_tail` (publish + return).
/// The single-model [`emit_open`] calls it once on `model.body`; the dual-mode
/// Open calls it once per arm on `prologue ++ selected-arm-body`, inside the
/// predicate `if/else`. Does NOT close the enclosing `OpenInternal` (the caller
/// owns that and the public wrapper).
#[allow(clippy::too_many_arguments)]
fn emit_open_arm(
    o: &mut String,
    func: &FuncDef,
    model: &StreamModel,
    body: &[Statement],
    enums: &HashMap<String, EnumDef>,
    registry: &Registry,
    helpers: &HelperRegistry,
    counter: &Cell<usize>,
) {
    let n = uname(func);
    // --- transcribed batch body ----------------------------------------------
    let _ = writeln!(o, "\n   {{");
    let open_body = build_open_body_from(model, body);
    let mut open_body_c = String::new();
    for s in &open_body {
        open_body_c.push_str(&render_statement(s, 6, false, enums, registry, helpers, counter));
    }
    let open_settings = crate::candle_settings::detect_candle_settings(body);
    if !open_settings.is_empty() {
        o.push_str(&emit_used_candle_unpacking(&open_settings, &open_body_c, 6));
    }
    o.push_str(&open_body_c);

    // --- state capture --------------------------------------------------------
    let _ = writeln!(o, "\n      /* Capture the live batch state into the handle. */");
    o.push_str(&alloc_and_capture(
        func, model, "      ", /*with_state=*/ true, "", registry, helpers, counter,
    ));
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
    emit_circ_capture(o, model, &n);
    emit_open_tail(o, func, model);
    let _ = writeln!(o, "   }}");
}

/// Circ capture: allocate + copy the live batch buffers (contents AND
/// rotation phase), freeing them on every path. Failure returns must ALSO
/// free the still-live batch buffers (their top-level CIRCBUF_DESTROY was
/// withheld so the capture below can read them).
fn emit_circ_capture(o: &mut String, model: &StreamModel, n: &str) {
    let free_batch = free_batch_storages(model);
    for circ in model.circs() {
        let id = &circ.id;
        let _ = writeln!(o, "      sp->cbSize_{id} = maxIdx_{id} + 1;");
        let _ = writeln!(
            o,
            "      if( sp->cbSize_{id} < 1 || sp->cbSize_{id} > historyLen + 1 ) {{ {free_batch}TA_{n}_StreamRelease( sp ); return TA_INTERNAL_ERROR; }}"
        );
        for (storage, ty) in circ_storages(circ) {
            let et = if matches!(ty, crate::ir::VarType::Integer) { "int" } else { "double" };
            let _ = writeln!(
                o,
                "      sp->cb_{storage} = ({et} *)TA_Malloc( sizeof({et}) * (size_t)sp->cbSize_{id} );"
            );
            let _ = writeln!(o, "      if( !sp->cb_{storage} ) {{ {free_batch}TA_{n}_StreamRelease( sp ); return TA_ALLOC_ERR; }}");
            let _ = writeln!(
                o,
                "      sp->cbMirror_{storage} = ({et} *)TA_Malloc( sizeof({et}) * (size_t)sp->cbSize_{id} );"
            );
            let _ = writeln!(o, "      if( !sp->cbMirror_{storage} ) {{ {free_batch}TA_{n}_StreamRelease( sp ); return TA_ALLOC_ERR; }}");
            // Live copy: contents AND rotation phase, straight from the
            // batch's own buffer (ring-ORDER constraint by construction).
            let _ = writeln!(
                o,
                "      memcpy( sp->cb_{storage}, {storage}, sizeof({et}) * (size_t)sp->cbSize_{id} );"
            );
        }
    }
    if !model.circs().is_empty() {
        let _ = writeln!(o, "      {free_batch}");
    }
}

/// Final lines of Open's success path: out values, handle publish, return.
fn emit_open_tail(o: &mut String, func: &FuncDef, model: &StreamModel) {
    for out in &model.outputs {
        let _ = writeln!(o, "      *{out} = lastValue_{out};");
        let _ = out_c_type(func, out);
    }
    let _ = writeln!(o, "      *stream = sp;");
    let _ = writeln!(o, "      return TA_SUCCESS;");
}

/// Cleanup prefix for a capture-failure return (composed Open: close subs +
/// free scratch before releasing the half-built handle). Formatted as
/// statements or empty.
fn pre_fail_stmt(pre_fail: &str) -> String {
    if pre_fail.is_empty() {
        String::new()
    } else {
        format!("{pre_fail}; ")
    }
}

/// `sp = TA_Malloc(...); memset; param/extra capture[; state capture]` at the
/// given indent. memset keeps unused fields (identity path) deterministic
/// and NULLs the ring pointers so `StreamRelease` is safe mid-allocation.
///
/// Rings: `with_state == true` is the normal path — capacity is captured
/// NUMERICALLY from the still-live batch locals (`cursor - var`,
/// loop-invariant), buffers are filled from the history tail (phase-free
/// trailing reads only; CIRCBUF-order functions are a later tranche), and
/// Peek's scratch mirrors are pre-allocated. On the identity path
/// (`with_state == false`) capacities are zero and 1-slot buffers keep the
/// transition's cap-0 guard and Peek's mirror copy well-defined.
#[allow(clippy::too_many_lines)]
fn alloc_and_capture(
    func: &FuncDef,
    model: &StreamModel,
    pad: &str,
    with_state: bool,
    pre_fail: &str,
    registry: &Registry,
    helpers: &HelperRegistry,
    counter: &Cell<usize>,
) -> String {
    let n = uname(func);
    let pre = pre_fail_stmt(pre_fail);
    let mut s = String::new();
    let _ = writeln!(
        s,
        "{pad}sp = (struct TA_{n}_Stream *)TA_Malloc( sizeof(*sp) );"
    );
    // Circ models: the batch's own circular buffer is still live here (its
    // top-level destroy was withheld for the capture) — free it on failure.
    let sp_fail: String = if with_state && !model.circs().is_empty() {
        free_batch_storages(model)
    } else {
        String::new()
    };
    let _ = writeln!(s, "{pad}if( !sp ) {{ {pre}{sp_fail}return TA_ALLOC_ERR; }}");
    let _ = writeln!(s, "{pad}memset( sp, 0, sizeof(*sp) );");
    for p in &func.optional_inputs {
        let _ = writeln!(s, "{pad}sp->{0} = {0};", p.name);
    }
    for (name, _) in &func.private_extra_params {
        let _ = writeln!(s, "{pad}sp->{name} = {name};");
    }
    if with_state {
        for name in &model.out_feedback {
            let _ = writeln!(s, "{pad}sp->lastOut_{name} = lastValue_{name};");
        }
        for (name, ty) in &model.state {
            if matches!(
                ty,
                crate::ir::VarType::RealArray(_) | crate::ir::VarType::IntArray(_)
            ) {
                let _ = writeln!(
                    s,
                    "{pad}memcpy( sp->{name}, {name}, sizeof( sp->{name} ) );"
                );
            } else {
                let _ = writeln!(s, "{pad}sp->{name} = {name};");
            }
        }
    }
    let fail = if model.rings().is_empty() {
        String::new()
    } else {
        format!("{{ {pre}TA_{n}_StreamRelease( sp ); return TA_ALLOC_ERR; }}")
    };
    for ring in model.rings() {
        let v = &ring.var;
        let back = ring.back;
        if with_state {
            if back > 0 {
                let _ = writeln!(s, "{pad}sp->ringLag_{v} = (int)({} - {v});", model.cursor);
                let _ = writeln!(
                    s,
                    "{pad}sp->ringCap_{v} = sp->ringLag_{v} + {};",
                    back + 1
                );
                let _ = writeln!(
                    s,
                    "{pad}if( sp->ringLag_{v} < {fwd} || sp->ringCap_{v} > historyLen ) {{ {pre}TA_{n}_StreamRelease( sp ); return TA_INTERNAL_ERROR; }}",
                    fwd = ring.fwd
                );
            } else {
                let _ = writeln!(s, "{pad}sp->ringCap_{v} = (int)({} - {v});", model.cursor);
                let _ = writeln!(
                    s,
                    "{pad}if( sp->ringCap_{v} < 0 || sp->ringCap_{v} > historyLen ) {{ {pre}TA_{n}_StreamRelease( sp ); return TA_INTERNAL_ERROR; }}"
                );
            }
        } else if back > 0 {
            let _ = writeln!(s, "{pad}sp->ringLag_{v} = 0;");
            let _ = writeln!(s, "{pad}sp->ringCap_{v} = {};", back + 1);
        } else {
            let _ = writeln!(s, "{pad}sp->ringCap_{v} = 0;");
        }
        let _ = writeln!(
            s,
            "{pad}{{ size_t allocN = (size_t)(sp->ringCap_{v} > 0 ? sp->ringCap_{v} : 1);"
        );
        for arr in &ring.arrays {
            let _ = writeln!(
                s,
                "{pad}  sp->ring_{v}_{arr} = (double *)TA_Malloc( sizeof(double) * allocN );"
            );
            let _ = writeln!(s, "{pad}  if( !sp->ring_{v}_{arr} ) {fail}");
            let _ = writeln!(
                s,
                "{pad}  sp->ringMirror_{v}_{arr} = (double *)TA_Malloc( sizeof(double) * allocN );"
            );
            let _ = writeln!(s, "{pad}  if( !sp->ringMirror_{v}_{arr} ) {fail}");
            if with_state {
                if ring.back > 0 {
                    let _ = writeln!(s, "{pad}  {{ int fillJ;");
                    let _ = writeln!(
                        s,
                        "{pad}    for( fillJ = historyLen - sp->ringCap_{v}; fillJ < historyLen; fillJ++ )"
                    );
                    let _ = writeln!(
                        s,
                        "{pad}       sp->ring_{v}_{arr}[fillJ % sp->ringCap_{v}] = {arr}[fillJ];"
                    );
                    let _ = writeln!(s, "{pad}  }}");
                } else {
                    let _ = writeln!(
                        s,
                        "{pad}  memcpy( sp->ring_{v}_{arr}, {arr} + (historyLen - sp->ringCap_{v}), sizeof(double) * (size_t)sp->ringCap_{v} );"
                    );
                }
            } else {
                // Identity path never reads the ring, but Peek's mirror
                // memcpy must not copy uninitialized heap (MSan).
                let _ = writeln!(
                    s,
                    "{pad}  memset( sp->ring_{v}_{arr}, 0, sizeof(double) * allocN );"
                );
            }
        }
        let _ = writeln!(s, "{pad}}}");
        if ring.back > 0 && with_state {
            let _ = writeln!(s, "{pad}sp->ringPos_{v} = historyLen % sp->ringCap_{v};");
        } else {
            let _ = writeln!(s, "{pad}sp->ringPos_{v} = 0;");
        }
    }
    for win in model.windows() {
        let v = &win.var;
        if with_state {
            let cap = render_expression(&win.cap, registry, helpers, counter);
            let _ = writeln!(s, "{pad}sp->winCap_{v} = (int)({cap});");
        } else {
            // Identity path: window untouched by the transition's identity
            // branch; keep a deterministic 1-slot buffer.
            let _ = writeln!(s, "{pad}sp->winCap_{v} = 1;");
        }
        let _ = writeln!(
            s,
            "{pad}if( sp->winCap_{v} < 1 || sp->winCap_{v} > historyLen ) {{ {pre}TA_{n}_StreamRelease( sp ); return TA_INTERNAL_ERROR; }}"
        );
        for arr in &win.arrays {
            let _ = writeln!(
                s,
                "{pad}sp->win_{v}_{arr} = (double *)TA_Malloc( sizeof(double) * (size_t)sp->winCap_{v} );"
            );
            let _ = writeln!(s, "{pad}if( !sp->win_{v}_{arr} ) {{ {pre}TA_{n}_StreamRelease( sp ); return TA_ALLOC_ERR; }}");
            let _ = writeln!(
                s,
                "{pad}sp->winMirror_{v}_{arr} = (double *)TA_Malloc( sizeof(double) * (size_t)sp->winCap_{v} );"
            );
            let _ = writeln!(s, "{pad}if( !sp->winMirror_{v}_{arr} ) {{ {pre}TA_{n}_StreamRelease( sp ); return TA_ALLOC_ERR; }}");
            // Fill with the history tail: slot cap-1 = last bar, so the next
            // update writes the new bar at pos 0 and (pos+cap-w)%cap walks
            // back w bars.
            if with_state {
                let _ = writeln!(
                    s,
                    "{pad}memcpy( sp->win_{v}_{arr}, {arr} + (historyLen - sp->winCap_{v}), sizeof(double) * (size_t)sp->winCap_{v} );"
                );
            } else {
                let _ = writeln!(s, "{pad}sp->win_{v}_{arr}[0] = 0.0;");
            }
        }
        let _ = writeln!(s, "{pad}sp->winPos_{v} = 0;");
    }
    if let Some(ex) = model.extrema() {
        if with_state {
            let _ = writeln!(
                s,
                "{pad}sp->xCap = (int)({} - {}) + 1;",
                model.cursor, ex.trailing
            );
        } else {
            let _ = writeln!(s, "{pad}sp->xCap = 1;");
        }
        let _ = writeln!(
            s,
            "{pad}if( sp->xCap < 1 || sp->xCap > historyLen ) {{ {pre}TA_{n}_StreamRelease( sp ); return TA_INTERNAL_ERROR; }}"
        );
        for arr in &ex.arrays {
            let _ = writeln!(
                s,
                "{pad}sp->x_{arr} = (double *)TA_Malloc( sizeof(double) * (size_t)sp->xCap );"
            );
            let _ = writeln!(s, "{pad}if( !sp->x_{arr} ) {{ {pre}TA_{n}_StreamRelease( sp ); return TA_ALLOC_ERR; }}");
            let _ = writeln!(
                s,
                "{pad}sp->xMirror_{arr} = (double *)TA_Malloc( sizeof(double) * (size_t)sp->xCap );"
            );
            let _ = writeln!(s, "{pad}if( !sp->xMirror_{arr} ) {{ {pre}TA_{n}_StreamRelease( sp ); return TA_ALLOC_ERR; }}");
        }
        if with_state {
            // Absolute slots: bar j lives at j % cap (matches the automaton's
            // absolute-index reads; a plain tail memcpy would break phase).
            let _ = writeln!(s, "{pad}{{ int fillJ;");
            let _ = writeln!(
                s,
                "{pad}  for( fillJ = historyLen - sp->xCap; fillJ < historyLen; fillJ++ )"
            );
            let _ = writeln!(s, "{pad}  {{");
            for arr in &ex.arrays {
                let _ = writeln!(s, "{pad}     sp->x_{arr}[fillJ % sp->xCap] = {arr}[fillJ];");
            }
            let _ = writeln!(s, "{pad}  }}");
            let _ = writeln!(s, "{pad}}}");
        }
    }
    s
}

/// CIRCBUF storage is a hoisted declaration in batch (the Prolog renders
/// empty in statement position) — replicate the hoist in Open so the
/// transcribed Init/uses compile.
/// The param==1 identity fast path in Open (mirrors the batch's explicit
/// path; min_history holds here too — lookback folds in the ambient
/// unstable period, so period==1 with K>0 still requires K+1 bars).
fn emit_identity_fast_path(
    o: &mut String,
    func: &FuncDef,
    model: &StreamModel,
    registry: &Registry,
    helpers: &HelperRegistry,
    counter: &Cell<usize>,
) {
    let n = uname(func);
    if let Some(idp) = &model.identity {
        let cond = render_expression(&idp.condition, registry, helpers, counter);
        let lookback_args: Vec<String> =
            func.optional_inputs.iter().map(|p| p.name.clone()).collect();
        let _ = writeln!(o, "\n   if( {cond} )\n   {{");
        let _ = writeln!(
            o,
            "      if( historyLen < TA_{n}_Lookback( {} ) + 1 ) return TA_BAD_PARAM;",
            lookback_args.join(", ")
        );
        o.push_str(&alloc_and_capture(
            func, model, "      ", /*with_state=*/ false, "", registry, helpers, counter,
        ));
        for (out, inp) in &idp.pairs {
            let _ = writeln!(o, "      *{out} = {inp}[historyLen - 1];");
        }
        let _ = writeln!(o, "      *stream = sp;");
        let _ = writeln!(o, "      return TA_SUCCESS;");
        let _ = writeln!(o, "   }}");
    }
}

/// Open's argument validation: NULL checks, minimum history, and the same
/// optional-parameter default-substitution/range checks the batch uses.
fn emit_open_validation(o: &mut String, func: &FuncDef, outputs: &[String], inputs: &[String]) {
    let _ = writeln!(o, "\n   if( !stream ) return TA_BAD_PARAM;");
    let _ = writeln!(o, "   *stream = NULL;");
    let null_checks: Vec<String> = inputs
        .iter()
        .map(|i| format!("!{i}"))
        .chain(outputs.iter().map(|out| format!("!{out}")))
        .collect();
    let _ = writeln!(o, "   if( {} ) return TA_BAD_PARAM;", null_checks.join(" || "));
    let _ = writeln!(o, "   if( historyLen < 1 ) return TA_BAD_PARAM;");
    o.push_str(&emit_opt_param_validation(func, "TA_BAD_PARAM"));
}

/// `if (buf != &local_buf[0]) TA_Free(buf);` for every batch circ storage —
/// the frees a failure path owes for the withheld top-level destroys.
fn free_batch_storages(model: &StreamModel) -> String {
    let mut s = String::new();
    for (storage, _) in model.circs().iter().flat_map(circ_storages) {
        let _ = write!(s, "if( {storage} != &local_{storage}[0] ) TA_Free( {storage} ); ");
    }
    s
}

fn emit_circ_hoist(o: &mut String, func: &FuncDef, model: &StreamModel) {
    for circ in model.circs() {
        for (storage, ty) in circ_storages(circ) {
            let et = if matches!(ty, crate::ir::VarType::Integer) { "int" } else { "double" };
            let _ = writeln!(o, "   {et} local_{storage}[{}];", circ_static_size(func, &circ.id));
            let _ = writeln!(o, "   {et} *{storage};");
        }
        let _ = writeln!(o, "   int {}_Idx;", circ.id);
        let _ = writeln!(o, "   int maxIdx_{};", circ.id);
    }
}

/// Static stack capacity of a CIRCBUF, from its Prolog in the batch body.
fn circ_static_size(func: &FuncDef, id: &str) -> i64 {
    fn find(stmts: &[Statement], id: &str) -> Option<i64> {
        for st in stmts {
            match st {
                Statement::CircBuf(crate::ir::CircBuf::Prolog {
                    id: pid,
                    static_size,
                    ..
                }) if pid == id => return Some(*static_size),
                Statement::While { body, .. }
                | Statement::DoWhile { body, .. }
                | Statement::For { body, .. }
                | Statement::Block { body } => {
                    if let Some(v) = find(body, id) {
                        return Some(v);
                    }
                }
                Statement::If {
                    then_body,
                    else_body,
                    ..
                } => {
                    if let Some(v) = find(then_body, id).or_else(|| find(else_body, id)) {
                        return Some(v);
                    }
                }
                _ => {}
            }
        }
        None
    }
    let body: &[Statement] = if func.has_explicit_private {
        &func.private_body
    } else {
        &func.body
    };
    find(body, id).expect("circbuf prolog present for referenced id")
}

/// Transcribe a batch body region for Open: out-param pointers → dummies,
/// output-array writes → `lastValue_*`, early returns mapped (no-data success →
/// TA_BAD_PARAM; error codes verbatim), final return dropped so control falls
/// through to the state capture. The loop tier passes `model.body`; dual-mode
/// passes `prologue ++ selected-arm-body` (not `model.body`), so the region is
/// an explicit parameter. Output redirection / early-return mapping / state
/// zero-init use `model`'s outputs, out-feedback, and state.
fn build_open_body_from(model: &StreamModel, body: &[Statement]) -> Vec<Statement> {
    let outputs = model.outputs.clone();
    // Carried-state locals must never be captured uninitialized: a local
    // assigned only inside a data-dependent branch (ADX's minusDI/plusDI on
    // flat-price history) would otherwise be UB at the capture epilogue.
    // Zero-init is bit-exact-safe: wherever the batch body assigns, the zero
    // is overwritten; wherever it does not, the transition is write-before-
    // read on that field and the zero is dead state.
    let state_names: std::collections::BTreeMap<String, crate::ir::VarType> =
        model.state.iter().cloned().collect();
    let fb_outputs = model.out_feedback.clone();
    let fe = move |e: Expr| -> Expr {
        match e {
            Expr::PointerDeref(nm) if nm == "outBegIdx" => Expr::Var("dummyBegIdx".into()),
            Expr::PointerDeref(nm) if nm == "outNBElement" => Expr::Var("dummyNBElement".into()),
            // Previous-output feedback read: in the transcription the output
            // array does not exist; lastValue_<out> still holds the previous
            // bar's value at the point of the read.
            Expr::ArrayAccess(nm, idx)
                if fb_outputs.contains(&nm)
                    && crate::streaming::is_prev_output_read(&idx) =>
            {
                Expr::Var(format!("lastValue_{nm}"))
            }
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
                    // Renders as `= {0}` — aggregate zero-init for carried
                    // fixed-size array state.
                    crate::ir::VarType::RealArray(_) | crate::ir::VarType::IntArray(_) => {
                        Expr::Var("{0}".into())
                    }
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
                    // Any early success return maps to BAD_PARAM. This is
                    // not just the no-data guard: a mid-body seed return
                    // (RSI/CMO under Metastock) exits with state the batch
                    // would REWIND and rebuild before continuing, so no
                    // bit-exact continuation exists — the stream honestly
                    // asks for one more bar instead (strict min-history).
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
    // capture epilogue (early returns keep their mapped statements). Also
    // drop TOP-LEVEL CIRCBUF_DESTROYs — the capture epilogue must still read
    // those buffers, and it frees them itself afterwards. Destroys NESTED in
    // early-return guards are kept verbatim: they are the batch's own
    // leak-free error paths (dropping them leaked MFI's heap buffers on the
    // insufficient-history return).
    let mut body: Vec<Statement> = body.to_vec();
    if matches!(body.last(), Some(Statement::Return { .. })) {
        body.pop();
    }
    body.retain(|st| !matches!(st, Statement::CircBuf(crate::ir::CircBuf::Destroy { .. })));
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

fn emit_peek(o: &mut String, func: &FuncDef, model: &StreamModel) {
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
    emit_peek_mirror_fixups(o, model);
    let args: Vec<String> = bars
        .iter()
        .cloned()
        .chain(outs.iter().cloned())
        .collect();
    let _ = writeln!(o, "   TA_{n}_StreamStep( &scratch, {} );", args.join(", "));
    let _ = writeln!(o, "   return TA_SUCCESS;\n}}\n");
}

/// Rings/windows/circs/extrema: run the step against the handle's
/// pre-allocated scratch mirrors so the live buffers are never touched (the
/// handle is logically const; single-writer covers the mirror — see the
/// proposal).
fn emit_peek_mirror_fixups(o: &mut String, model: &StreamModel) {
    for ring in model.rings() {
        let v = &ring.var;
        for arr in &ring.arrays {
            let _ = writeln!(o, "   scratch.ring_{v}_{arr} = stream->ringMirror_{v}_{arr};");
            let _ = writeln!(
                o,
                "   memcpy( scratch.ring_{v}_{arr}, stream->ring_{v}_{arr}, sizeof(double) * (size_t)(stream->ringCap_{v} > 0 ? stream->ringCap_{v} : 1) );"
            );
        }
    }
    for win in model.windows() {
        let v = &win.var;
        for arr in &win.arrays {
            let _ = writeln!(o, "   scratch.win_{v}_{arr} = stream->winMirror_{v}_{arr};");
            let _ = writeln!(
                o,
                "   memcpy( scratch.win_{v}_{arr}, stream->win_{v}_{arr}, sizeof(double) * (size_t)stream->winCap_{v} );"
            );
        }
    }
    for circ in model.circs() {
        let id = &circ.id;
        for (storage, ty) in circ_storages(circ) {
            let et = if matches!(ty, crate::ir::VarType::Integer) { "int" } else { "double" };
            let _ = writeln!(o, "   scratch.cb_{storage} = stream->cbMirror_{storage};");
            let _ = writeln!(
                o,
                "   memcpy( scratch.cb_{storage}, stream->cb_{storage}, sizeof({et}) * (size_t)stream->cbSize_{id} );"
            );
        }
    }
    if let Some(ex) = model.extrema() {
        for arr in &ex.arrays {
            let _ = writeln!(o, "   scratch.x_{arr} = stream->xMirror_{arr};");
            let _ = writeln!(
                o,
                "   memcpy( scratch.x_{arr}, stream->x_{arr}, sizeof(double) * (size_t)stream->xCap );"
            );
        }
    }
}

fn emit_close(o: &mut String, func: &FuncDef, model: &StreamModel) {
    let n = uname(func);
    let _ = writeln!(o, "{}\n{{", close_signature(func));
    if model.needs_release() {
        let _ = writeln!(o, "   TA_{n}_StreamRelease( stream );");
    } else {
        let _ = writeln!(o, "   if( stream ) TA_Free( stream );");
    }
    let _ = writeln!(o, "   return TA_SUCCESS;\n}}\n");
}
