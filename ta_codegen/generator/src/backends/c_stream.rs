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
use crate::streaming::{self, circ_storages, StreamModel};

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
    fn ring_buf(&self, var: &str, array: &str) -> String {
        format!("sp->ring_{var}_{array}")
    }
    fn ring_pos(&self, var: &str) -> String {
        format!("sp->ringPos_{var}")
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
        "\n/*\n * Streaming API for TA_{n} — incremental per-bar evaluation.\n * Open consumes the warm-up history; Update commits one closed bar;\n * Peek evaluates a forming bar without committing; Close frees the handle.\n * A handle is single-writer: driving one handle from two threads\n * concurrently — Update or Peek, despite the latter's const — is\n * undefined behavior. Distinct handles are fully independent.\n * See docs/streaming-api-proposal.md.\n */\ntypedef struct TA_{n}_Stream TA_{n}_Stream;\n\n{};\n\n{};\n\n{};\n\n{};\n",
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
    emit_release(&mut o, func, &model);
    emit_step(&mut o, func, &model, enums, registry, helpers, &counter);
    emit_open(&mut o, func, &model, enums, registry, helpers, &counter);
    emit_update(&mut o, func);
    emit_peek(&mut o, func, &model);
    emit_close(&mut o, func, &model);

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
    for ring in &model.rings {
        let v = &ring.var;
        let _ = writeln!(o, "   int ringPos_{v};");
        let _ = writeln!(o, "   int ringCap_{v};");
        for arr in &ring.arrays {
            let _ = writeln!(o, "   double *ring_{v}_{arr};");
            // Scratch mirror for Peek: pre-allocated at open so Peek stays
            // allocation-free (proposal: forming-bar evaluation).
            let _ = writeln!(o, "   double *ringMirror_{v}_{arr};");
        }
    }
    for win in &model.windows {
        let v = &win.var;
        let _ = writeln!(o, "   int winPos_{v};");
        let _ = writeln!(o, "   int winCap_{v};");
        for arr in &win.arrays {
            let _ = writeln!(o, "   double *win_{v}_{arr};");
            let _ = writeln!(o, "   double *winMirror_{v}_{arr};");
        }
    }
    for circ in &model.circs {
        let _ = writeln!(o, "   int cbSize_{};", circ.id);
        for (storage, ty) in circ_storages(circ) {
            let et = if matches!(ty, crate::ir::VarType::Integer) { "int" } else { "double" };
            let _ = writeln!(o, "   {et} *cb_{storage};");
            let _ = writeln!(o, "   {et} *cbMirror_{storage};");
        }
    }
    if let Some(ex) = &model.extrema {
        let _ = writeln!(o, "   int xCap;");
        for arr in &ex.arrays {
            let _ = writeln!(o, "   double *x_{arr};");
            let _ = writeln!(o, "   double *xMirror_{arr};");
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

/// `static void TA_<N>_StreamRelease(...)`: frees every ring buffer and the
/// handle itself. Emitted only for ring models; safe on partially-allocated
/// handles (open memsets the struct, so unallocated buffers are NULL).
fn emit_release(o: &mut String, func: &FuncDef, model: &StreamModel) {
    if model.rings.is_empty()
        && model.windows.is_empty()
        && model.circs.is_empty()
        && model.extrema.is_none()
    {
        return;
    }
    let n = uname(func);
    let _ = writeln!(o, "static void TA_{n}_StreamRelease( struct TA_{n}_Stream *sp )
{{");
    let _ = writeln!(o, "   if( !sp ) return;");
    for ring in &model.rings {
        for arr in &ring.arrays {
            let _ = writeln!(o, "   if( sp->ring_{0}_{arr} ) TA_Free( sp->ring_{0}_{arr} );", ring.var);
            let _ = writeln!(o, "   if( sp->ringMirror_{0}_{arr} ) TA_Free( sp->ringMirror_{0}_{arr} );", ring.var);
        }
    }
    for win in &model.windows {
        for arr in &win.arrays {
            let _ = writeln!(o, "   if( sp->win_{0}_{arr} ) TA_Free( sp->win_{0}_{arr} );", win.var);
            let _ = writeln!(o, "   if( sp->winMirror_{0}_{arr} ) TA_Free( sp->winMirror_{0}_{arr} );", win.var);
        }
    }
    for circ in &model.circs {
        for (storage, _) in circ_storages(circ) {
            let _ = writeln!(o, "   if( sp->cb_{storage} ) TA_Free( sp->cb_{storage} );");
            let _ = writeln!(o, "   if( sp->cbMirror_{storage} ) TA_Free( sp->cbMirror_{storage} );");
        }
    }
    if let Some(ex) = &model.extrema {
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
    // Extrema automatons carry batch-absolute int indices that grow by one
    // per bar. Rebase them by a multiple of the ring capacity long before
    // INT_MAX: index differences and `% cap` residues are invariant, so the
    // automaton (and bit-exactness vs any batch-comparable range, which is
    // itself bounded by int) is untouched. Index-observable outputs
    // (MININDEX...) report the rebased position beyond ~2^30 bars — the
    // batch contract is inherently vacuous past INT_MAX bars.
    if let Some(ex) = &model.extrema {
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
    emit_circ_hoist(o, func, model);
    let _ = writeln!(o, "   int startIdx;");
    let _ = writeln!(o, "   int endIdx;");
    let _ = writeln!(o, "   int dummyBegIdx;");
    let _ = writeln!(o, "   int dummyNBElement;");
    for out in &model.outputs {
        let _ = writeln!(o, "   {} lastValue_{out};", out_c_type(func, out));
    }
    for (name, c_type) in &func.private_extra_params {
        let _ = writeln!(o, "   {c_type} {name};");
    }
    // Optional params arrive by value; validation may rewrite them (defaults).
    let mut opt_names: Vec<&str> = Vec::new();
    for p in &func.optional_inputs {
        opt_names.push(&p.name);
    }

    emit_open_validation(o, func, model, &inputs);

    // --- initialization (after defaults are substituted) ---------------------
    let _ = writeln!(o, "\n   startIdx = 0;");
    let _ = writeln!(o, "   endIdx = historyLen - 1;");
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

    // --- transcribed batch body ----------------------------------------------
    let _ = writeln!(o, "\n   {{");
    let open_body = build_open_body(model);
    for s in &open_body {
        o.push_str(&render_statement(s, 6, false, enums, registry, helpers, counter));
    }

    // --- state capture --------------------------------------------------------
    let _ = writeln!(o, "\n      /* Capture the live batch state into the handle. */");
    o.push_str(&alloc_and_capture(
        func, model, "      ", /*with_state=*/ true, registry, helpers, counter,
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
    let _ = writeln!(o, "   }}\n}}\n");
}

/// Circ capture: allocate + copy the live batch buffers (contents AND
/// rotation phase), freeing them on every path. Failure returns must ALSO
/// free the still-live batch buffers (their top-level CIRCBUF_DESTROY was
/// withheld so the capture below can read them).
fn emit_circ_capture(o: &mut String, model: &StreamModel, n: &str) {
    let free_batch = free_batch_storages(model);
    for circ in &model.circs {
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
    if !model.circs.is_empty() {
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
    registry: &Registry,
    helpers: &HelperRegistry,
    counter: &Cell<usize>,
) -> String {
    let n = uname(func);
    let mut s = String::new();
    let _ = writeln!(
        s,
        "{pad}sp = (struct TA_{n}_Stream *)TA_Malloc( sizeof(*sp) );"
    );
    // Circ models: the batch's own circular buffer is still live here (its
    // top-level destroy was withheld for the capture) — free it on failure.
    let sp_fail: String = if with_state && !model.circs.is_empty() {
        free_batch_storages(model)
    } else {
        String::new()
    };
    let _ = writeln!(s, "{pad}if( !sp ) {{ {sp_fail}return TA_ALLOC_ERR; }}");
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
    let fail = if model.rings.is_empty() {
        String::new()
    } else {
        format!("{{ TA_{n}_StreamRelease( sp ); return TA_ALLOC_ERR; }}")
    };
    for ring in &model.rings {
        let v = &ring.var;
        if with_state {
            let _ = writeln!(s, "{pad}sp->ringCap_{v} = (int)({} - {v});", model.cursor);
            let _ = writeln!(
                s,
                "{pad}if( sp->ringCap_{v} < 0 || sp->ringCap_{v} > historyLen ) {{ TA_{n}_StreamRelease( sp ); return TA_INTERNAL_ERROR; }}"
            );
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
                let _ = writeln!(
                    s,
                    "{pad}  memcpy( sp->ring_{v}_{arr}, {arr} + (historyLen - sp->ringCap_{v}), sizeof(double) * (size_t)sp->ringCap_{v} );"
                );
            } else {
                let _ = writeln!(s, "{pad}  sp->ring_{v}_{arr}[0] = 0.0;");
            }
        }
        let _ = writeln!(s, "{pad}}}");
        let _ = writeln!(s, "{pad}sp->ringPos_{v} = 0;");
    }
    for win in &model.windows {
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
            "{pad}if( sp->winCap_{v} < 1 || sp->winCap_{v} > historyLen ) {{ TA_{n}_StreamRelease( sp ); return TA_INTERNAL_ERROR; }}"
        );
        for arr in &win.arrays {
            let _ = writeln!(
                s,
                "{pad}sp->win_{v}_{arr} = (double *)TA_Malloc( sizeof(double) * (size_t)sp->winCap_{v} );"
            );
            let _ = writeln!(s, "{pad}if( !sp->win_{v}_{arr} ) {{ TA_{n}_StreamRelease( sp ); return TA_ALLOC_ERR; }}");
            let _ = writeln!(
                s,
                "{pad}sp->winMirror_{v}_{arr} = (double *)TA_Malloc( sizeof(double) * (size_t)sp->winCap_{v} );"
            );
            let _ = writeln!(s, "{pad}if( !sp->winMirror_{v}_{arr} ) {{ TA_{n}_StreamRelease( sp ); return TA_ALLOC_ERR; }}");
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
    if let Some(ex) = &model.extrema {
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
            "{pad}if( sp->xCap < 1 || sp->xCap > historyLen ) {{ TA_{n}_StreamRelease( sp ); return TA_INTERNAL_ERROR; }}"
        );
        for arr in &ex.arrays {
            let _ = writeln!(
                s,
                "{pad}sp->x_{arr} = (double *)TA_Malloc( sizeof(double) * (size_t)sp->xCap );"
            );
            let _ = writeln!(s, "{pad}if( !sp->x_{arr} ) {{ TA_{n}_StreamRelease( sp ); return TA_ALLOC_ERR; }}");
            let _ = writeln!(
                s,
                "{pad}sp->xMirror_{arr} = (double *)TA_Malloc( sizeof(double) * (size_t)sp->xCap );"
            );
            let _ = writeln!(s, "{pad}if( !sp->xMirror_{arr} ) {{ TA_{n}_StreamRelease( sp ); return TA_ALLOC_ERR; }}");
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
            func, model, "      ", /*with_state=*/ false, registry, helpers, counter,
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
fn emit_open_validation(o: &mut String, func: &FuncDef, model: &StreamModel, inputs: &[String]) {
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
}

/// `if (buf != &local_buf[0]) TA_Free(buf);` for every batch circ storage —
/// the frees a failure path owes for the withheld top-level destroys.
fn free_batch_storages(model: &StreamModel) -> String {
    let mut s = String::new();
    for (storage, _) in model.circs.iter().flat_map(circ_storages) {
        let _ = write!(s, "if( {storage} != &local_{storage}[0] ) TA_Free( {storage} ); ");
    }
    s
}

fn emit_circ_hoist(o: &mut String, func: &FuncDef, model: &StreamModel) {
    for circ in &model.circs {
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
    // capture epilogue (early returns keep their mapped statements). Also
    // drop TOP-LEVEL CIRCBUF_DESTROYs — the capture epilogue must still read
    // those buffers, and it frees them itself afterwards. Destroys NESTED in
    // early-return guards are kept verbatim: they are the batch's own
    // leak-free error paths (dropping them leaked MFI's heap buffers on the
    // insufficient-history return).
    let mut body: Vec<Statement> = model.body.to_vec();
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
    // Rings: run the step against the handle's pre-allocated scratch
    // mirrors so the live window is never touched (the handle is logically
    // const; single-writer covers the mirror — see the proposal).
    for ring in &model.rings {
        let v = &ring.var;
        for arr in &ring.arrays {
            let _ = writeln!(o, "   scratch.ring_{v}_{arr} = stream->ringMirror_{v}_{arr};");
            let _ = writeln!(
                o,
                "   memcpy( scratch.ring_{v}_{arr}, stream->ring_{v}_{arr}, sizeof(double) * (size_t)(stream->ringCap_{v} > 0 ? stream->ringCap_{v} : 1) );"
            );
        }
    }
    for win in &model.windows {
        let v = &win.var;
        for arr in &win.arrays {
            let _ = writeln!(o, "   scratch.win_{v}_{arr} = stream->winMirror_{v}_{arr};");
            let _ = writeln!(
                o,
                "   memcpy( scratch.win_{v}_{arr}, stream->win_{v}_{arr}, sizeof(double) * (size_t)stream->winCap_{v} );"
            );
        }
    }
    for circ in &model.circs {
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
    if let Some(ex) = &model.extrema {
        for arr in &ex.arrays {
            let _ = writeln!(o, "   scratch.x_{arr} = stream->xMirror_{arr};");
            let _ = writeln!(
                o,
                "   memcpy( scratch.x_{arr}, stream->x_{arr}, sizeof(double) * (size_t)stream->xCap );"
            );
        }
    }
    let args: Vec<String> = bars
        .iter()
        .cloned()
        .chain(outs.iter().cloned())
        .collect();
    let _ = writeln!(o, "   TA_{n}_StreamStep( &scratch, {} );", args.join(", "));
    let _ = writeln!(o, "   return TA_SUCCESS;\n}}\n");
}

fn emit_close(o: &mut String, func: &FuncDef, model: &StreamModel) {
    let n = uname(func);
    let _ = writeln!(o, "{}\n{{", close_signature(func));
    if model.rings.is_empty()
        && model.windows.is_empty()
        && model.circs.is_empty()
        && model.extrema.is_none()
    {
        let _ = writeln!(o, "   if( stream ) TA_Free( stream );");
    } else {
        let _ = writeln!(o, "   TA_{n}_StreamRelease( stream );");
    }
    let _ = writeln!(o, "   return TA_SUCCESS;\n}}\n");
}
