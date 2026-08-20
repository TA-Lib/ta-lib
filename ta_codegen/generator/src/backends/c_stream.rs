//! C emitter for the streaming (incremental) API — docs/streaming-api-proposal.md.
//!
//! For every function whose YAML declares a `streaming:` tier, the generated
//! `src/ta_func/ta_<NAME>.c` gains a stream section after the batch variants:
//!
//! - `struct TA_<NAME>_Stream` — params, carried scalars, lag slots;
//! - `static void TA_<NAME>_StepInternal(...)` — the ONE transition function
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
use crate::streaming::{self, circ_storages, CircState, DispatchPlan, StreamModel, StreamPlan};

use super::c::{
    c_decl, emit_opt_param_validation, render_c_switch_label, render_expression,
    render_statement, render_statement_stream,
};
use super::fma;

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
    fn extrema_mask(&self) -> String {
        "sp->xMask".to_string()
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

/// Names of `func`'s nullable outputs (see IR `Output::nullable`). Threaded into
/// the statement renderers so a stream's per-bar write to a discarded output
/// (`*outFAMA = …`) is NULL-guarded exactly as the batch body's write is — this
/// is what lets a dispatch pass NULL for a sub-stream output it doesn't want.
fn nullable_out_names(func: &FuncDef) -> Vec<String> {
    func.outputs
        .iter()
        .filter(|o| o.is_nullable())
        .map(|o| o.name.clone())
        .collect()
}

/// The callee output-argument list for one supported dispatch arm, built from
/// its [`streaming::OutSlot`] map: `Forward(k)` passes the dispatch func's own
/// output `k`, `Discard` passes NULL (a nullable callee output this dispatch
/// drops — MAMA's FAMA when MA routes only the MAMA line, issue #125). For a
/// same-arity arm (every single-output MA) this is exactly `outputs.join(", ")`.
fn dispatch_arm_out_args(arm: &streaming::DispatchArm, outputs: &[String]) -> String {
    arm.out_map
        .iter()
        .map(|slot| match slot {
            streaming::OutSlot::Forward(k) => outputs[*k].clone(),
            streaming::OutSlot::Discard => "NULL".to_string(),
        })
        .collect::<Vec<_>>()
        .join(", ")
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

/// `Open` and `OpenAndFill` are ONE emission: `TA_<N>_OpenPass`, whose per-bar
/// output writes are subscripted `out[(<idx>) * outStride]`.
///
/// `OpenAndFill` passes stride 1 and the caller's array, so the array is
/// bit-identical to `batch(startIdx=0, endIdx=len-1)`. `OpenInternal` passes
/// stride 0 and a one-element scalar sink: every write collapses onto slot 0, so
/// after the replay that slot holds the last history value — which is also what
/// makes the previous-output feedback reads (`out[outIdx - 1]`) and the capture
/// epilogue's `out[*outNBElement - 1]` resolve with no special case. Both leave
/// the same handle (one capture epilogue). See docs/streaming-api-design.md.
///
/// Exempt tiers keep two hand-written bodies and never used this machinery:
/// `Dispatch` (MA) hands the fill to a sub's public `OpenAndFill`, and
/// `PeriodBank` (MAVP) runs a genuinely different warm-up per mode.
const OUT_STRIDE: &str = "outStride";

/// The stride-scaled subscript for an output write/read inside the core.
fn stride_index(idx: &str) -> String {
    format!("({idx}) * {OUT_STRIDE}")
}

/// `<idx>` -> `<idx> * outStride`, as IR. Applied to EVERY output subscript in
/// the transcribed body — writes and previous-output feedback reads alike — so
/// the one body serves both entry points. The index may carry a side effect
/// (`outIdx++`); multiplying leaves it evaluated exactly once.
fn scale_by_stride(idx: Expr) -> Expr {
    Expr::BinOp(
        Box::new(idx),
        crate::ir::BinOp::Mul,
        Box::new(Expr::Var(OUT_STRIDE.to_string())),
    )
}

/// The per-output array piece of `OpenAndFill`: `double outReal[], int outInteger[], ...`.
fn out_fill_arrays_sig(func: &FuncDef) -> String {
    func.outputs
        .iter()
        .map(|o| format!("{} {}[]", out_c_type(func, &o.name), o.name))
        .collect::<Vec<_>>()
        .join(", ")
}

/// Public `OpenAndFill` prototype (no trailing `;`). Same input head as `Open`
/// (`stream**`, history arrays, `historyLen`, optional params — startIdx is
/// implicitly 0), then the batch API's own output tail (`int *outBegIdx`,
/// `int *outNBElement`, one caller-owned array per output). One pass fills the
/// full history AND leaves a live handle. Shared by the header emitter and the
/// definition so the two can never drift (MSVC C2375).
pub fn open_and_fill_signature(func: &FuncDef) -> String {
    let n = uname(func);
    let mut history = String::new();
    for a in streaming::input_array_names(func) {
        let _ = write!(history, "const double {a}[], ");
    }
    format!(
        "TA_LIB_API TA_RetCode TA_{n}_OpenAndFill( TA_{n}_Stream **stream, {}int historyLen, {}int *outBegIdx, int *outNBElement, {} )",
        history,
        opt_params_sig(func),
        out_fill_arrays_sig(func)
    )
}

/// `OpenAndFillInternal` prototype (no trailing `;`): `OpenAndFill` anchored at
/// a caller-supplied `startIdx` instead of an implicit 0.
///
/// This is what lets a COMPOSED Open warm a sub-handle and fill that sub-call's
/// destination in ONE pass (issue #192). Before it, the composed tier opened the
/// sub-stream over the history AND re-ran the transcribed batch sub-call over
/// the same range — the same numbers computed twice, which measured as
/// `TA_STDDEV_Open` costing 1.49x its own batch pass while every direct stream
/// sat at 1.0.
///
/// It carries NO aliasing rejection, unlike the public wrapper. That is not an
/// oversight: the generator emits a call to it only for a sub-call whose
/// destinations alias neither its sources nor each other
/// ([`streaming::SubCallStep::is_fusable`]), so the check could never fire, and the
/// one sub-call that DOES write in place (STOCH's slow-K `TA_MA` over
/// `tempBuffer`) keeps the unfused two-pass form precisely because this wrapper
/// would be unsound there.
pub fn open_and_fill_internal_signature(func: &FuncDef) -> String {
    let n = uname(func);
    let mut history = String::new();
    for a in streaming::input_array_names(func) {
        let _ = write!(history, "const double {a}[], ");
    }
    format!(
        "TA_RetCode TA_{n}_OpenAndFillInternal( struct TA_{n}_Stream **stream, {}int startIdx, int historyLen, {}int *outBegIdx, int *outNBElement, {} )",
        history,
        opt_params_sig(func),
        out_fill_arrays_sig(func)
    )
}

/// The merged `OpenCore` prototype (no trailing `;`): the union of both public
/// entry points' inputs — history arrays, `startIdx` (a parameter, as
/// `OpenInternal` needs for sub-stream composition), the batch output triplet,
/// and `outStride`. File-static: with two call sites the compiler decides per
/// function whether to share the body or inline it into both wrappers, and when
/// it inlines, `outStride` constant-folds to 0/1 and the arm is exactly what the
/// two separate bodies used to be. Forcing `noinline` measured LARGER.
fn open_core_signature(func: &FuncDef) -> String {
    let n = uname(func);
    let mut history = String::new();
    for a in streaming::input_array_names(func) {
        let _ = write!(history, "const double {a}[], ");
    }
    format!(
        "static TA_RetCode TA_{n}_OpenPass( struct TA_{n}_Stream **stream, {}int startIdx, int historyLen, {}int *outBegIdx, int *outNBElement, {}, int {OUT_STRIDE} )",
        history,
        opt_params_sig(func),
        out_fill_arrays_sig(func)
    )
}

/// The argument list both wrappers pass to `OpenCore`, up to (not including) the
/// output triplet: `stream, <inputs>, <startIdx>, historyLen, <opts>`.
fn open_core_call_head(func: &FuncDef, start_idx: &str) -> String {
    let mut s = String::from("stream, ");
    for a in streaming::input_array_names(func) {
        let _ = write!(s, "{a}, ");
    }
    let _ = write!(s, "{start_idx}, historyLen, ");
    for p in &func.optional_inputs {
        let _ = write!(s, "{}, ", p.name);
    }
    s
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
        "TA_LIB_API TA_RetCode TA_{n}_Open( TA_{n}_Stream **stream, {}int historyLen, {}{} )",
        history,
        opt_params_sig(func),
        out_params_sig(func)
    )
}

/// Internal `OpenInternal` prototype (no trailing `;`). The scalar-sink entry
/// point onto `OpenCore`: it takes an extra `startIdx` — the bar within the
/// history buffer at which warm-up begins (0 = warm from the very first bar).
/// The public `Open` is a thin wrapper that calls this with 0; only generated
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
        "TA_RetCode TA_{n}_OpenInternal( struct TA_{n}_Stream **stream, {}int startIdx, int historyLen, {}{} )",
        history,
        opt_params_sig(func),
        out_params_sig(func)
    )
}

/// The per-bar finite-input rejection for `Update`/`Peek`: one `TA_IS_FINITE`
/// per scalar bar input, before the handle is touched.
///
/// This is the streaming tier's half of the boundary contract (see
/// `docs/streaming-api-design.md`). Batch does not filter — it computes on
/// whatever it is handed and reports NaN back out. A stream handle cannot do
/// that, because its state is retained: one non-finite bar poisons every
/// recursive accumulator in it for the rest of the handle's life, long after the
/// feed recovers. So the streaming tier rejects instead, and rejects *before*
/// mutating anything, leaving the handle exactly as it was.
fn finite_bar_check(func: &FuncDef, indent: &str, fail: &str) -> String {
    let bars = streaming::input_array_names(func);
    if bars.is_empty() {
        return String::new();
    }
    let conds: Vec<String> = bars.iter().map(|b| format!("!TA_IS_FINITE( {b} )")).collect();
    format!("{indent}if( {} ) return {fail};\n", conds.join(" || "))
}

/// The null + range guards at the PUBLIC `Open` / `OpenAndFill` entry.
///
/// `OpenCore` repeats them, so this reads like a duplicate. It is not, and the
/// reason differs between the two entry points:
///
/// - `Open` delegates through `OpenInternal`, which hands `OpenCore` a private
///   `sink_outReal` and copies it out afterwards. `OpenCore`'s `!outReal` test
///   therefore never sees the CALLER's pointer, and without the check here
///   `TA_<N>_Open( &s, data, n, p, NULL )` would run to completion and then
///   dereference NULL on the copy-out. This wrapper is the only place that
///   pointer is checkable.
/// - `OpenAndFill` calls `OpenCore` directly, so the null checks are genuinely
///   repeated — but the alias guard emitted after these runs BEFORE `OpenCore`
///   sees anything, so dropping them would let an output aliasing its input on
///   an over-long history report `TA_BAD_PARAM` where it reports
///   `TA_OUT_OF_RANGE_END_INDEX` today.
fn public_open_guards(func: &FuncDef, fail: &str) -> String {
    let inputs = streaming::input_array_names(func);
    if inputs.is_empty() {
        return String::new();
    }
    // The OUTPUT pointers are checked here too, even though this wrapper does
    // not touch them: `OpenCore` rejects a NULL output BEFORE it range-checks
    // `historyLen`, so leaving them out would make a >TA_MAX_INDEX history with
    // a NULL output report TA_OUT_OF_RANGE_END_INDEX where it used to report
    // TA_BAD_PARAM. Same set, same order, same answer.
    let nullable = nullable_out_names(func);
    let nulls: Vec<String> = inputs
        .iter()
        .cloned()
        .chain(
            func.outputs
                .iter()
                .map(|o| o.name.clone())
                .filter(|o| !nullable.contains(o)),
        )
        .map(|i| format!("!{i}"))
        .collect();
    format!(
        "   if( {} ) return {fail};\n   if( historyLen < 1 ) return {fail};\n   if( historyLen > TA_MAX_INDEX + 1 ) return TA_OUT_OF_RANGE_END_INDEX;\n",
        nulls.join(" || ")
    )
}

/// Emit the public `Open` as a thin wrapper delegating to `OpenInternal` with
/// startIdx = 0 (the standalone/public default).
fn emit_open_wrapper(o: &mut String, func: &FuncDef) {
    let n = uname(func);
    let mut hist = String::new();
    for a in streaming::input_array_names(func) {
        let _ = write!(hist, "{a}, ");
    }
    let mut opts = String::new();
    for p in &func.optional_inputs {
        let _ = write!(opts, "{}, ", p.name);
    }
    let outputs: String = func
        .outputs
        .iter()
        .map(|out| out.name.clone())
        .collect::<Vec<_>>()
        .join(", ");
    let _ = writeln!(o, "{}\n{{", open_signature(func));
    // The handle is published as NULL before anything can reject, so the
    // documented "*stream is NULL on any failure" holds on these paths too —
    // OpenCore, which normally does it, is not reached.
    let _ = writeln!(o, "   if( !stream ) return TA_BAD_PARAM;");
    let _ = writeln!(o, "   *stream = NULL;");
    o.push_str(&public_open_guards(func, "TA_BAD_PARAM"));
    let _ = writeln!(o, "   return TA_{n}_OpenInternal( stream, {hist}0, historyLen, {opts}{outputs} );");
    let _ = writeln!(o, "}}\n");
}

/// `OpenInternal`: the scalar wrapper. One stack slot per output stands in for
/// the caller's array; at stride 0 every per-bar write lands on slot 0, so after
/// the replay it holds the last history value. Copied out only on success, which
/// is what the two separate bodies did (an error path left `*out` untouched).
/// A nullable output passes NULL through — the core's writes are NULL-guarded
/// and a caller that discarded the output must not get it written.
fn emit_open_internal_wrapper(o: &mut String, func: &FuncDef) {
    let n = uname(func);
    let nullable = nullable_out_names(func);
    let _ = writeln!(o, "/* Private function, not in public API. */\n{}\n{{", open_internal_signature(func));
    let _ = writeln!(o, "   TA_RetCode retCode;");
    let _ = writeln!(o, "   int dummyBegIdx = 0;");
    let _ = writeln!(o, "   int dummyNBElement = 0;");
    for out in &func.outputs {
        let ty = out_c_type(func, &out.name);
        let init = if ty == "int" { "0" } else { "0.0" };
        let _ = writeln!(o, "   {ty} sink_{} = {init};", out.name);
    }
    let sinks: Vec<String> = func
        .outputs
        .iter()
        .map(|out| {
            if nullable.contains(&out.name) {
                format!("{0} ? &sink_{0} : NULL", out.name)
            } else {
                format!("&sink_{}", out.name)
            }
        })
        .collect();
    let _ = writeln!(
        o,
        "   retCode = TA_{n}_OpenPass( {}&dummyBegIdx, &dummyNBElement, {}, 0 );",
        open_core_call_head(func, "startIdx"),
        sinks.join(", ")
    );
    let _ = writeln!(o, "   if( retCode == TA_SUCCESS )\n   {{");
    for out in &func.outputs {
        let name = &out.name;
        if nullable.contains(name) {
            let _ = writeln!(o, "      if( {name} != NULL ) *{name} = sink_{name};");
        } else {
            let _ = writeln!(o, "      *{name} = sink_{name};");
        }
    }
    let _ = writeln!(o, "   }}");
    let _ = writeln!(o, "   return retCode;\n}}\n");
}

/// `OpenAndFill`: the fill wrapper. Carries the validation the scalar path has
/// no need of — the out-meta pointers, and the aliasing rejections (#108/#130).
/// Those are NOT stylistic: the capture epilogue reads the input tail AFTER
/// writing the outputs, so an output aliasing an input or another output would
/// corrupt the handle. `Open` writes only to its own stack slots and never has
/// that hazard, so making it pay the check would be pure cost.
fn emit_open_and_fill_wrapper(o: &mut String, func: &FuncDef) {
    let n = uname(func);
    let inputs = streaming::input_array_names(func);
    let nullable = nullable_out_names(func);
    let outs: Vec<String> = func.outputs.iter().map(|x| x.name.clone()).collect();
    let _ = writeln!(o, "{}\n{{", open_and_fill_signature(func));
    let _ = writeln!(o, "   if( !stream ) return TA_BAD_PARAM;");
    let _ = writeln!(o, "   *stream = NULL;");
    let mut null_checks: Vec<String> = vec!["!outBegIdx".into(), "!outNBElement".into()];
    null_checks.extend(
        outs.iter()
            .filter(|x| !nullable.contains(*x))
            .map(|x| format!("!{x}")),
    );
    let _ = writeln!(o, "   if( {} ) return TA_BAD_PARAM;", null_checks.join(" || "));
    o.push_str(&public_open_guards(func, "TA_BAD_PARAM"));
    // Cast to `const void *` so the comparison is well-typed for any output
    // element type (an integer output vs double inputs would otherwise warn
    // "comparison of distinct pointer types lacks a cast").
    let mut alias: Vec<String> = Vec::new();
    for out in &outs {
        for inp in &inputs {
            alias.push(format!("(const void *){out} == (const void *){inp}"));
        }
    }
    for (i, a) in outs.iter().enumerate() {
        for b in &outs[i + 1..] {
            alias.push(format!("(const void *){a} == (const void *){b}"));
        }
    }
    if !alias.is_empty() {
        let _ = writeln!(o, "   if( {} ) return TA_BAD_PARAM;", alias.join(" || "));
    }
    let _ = writeln!(
        o,
        "   return TA_{n}_OpenPass( {}outBegIdx, outNBElement, {}, 1 );",
        open_core_call_head(func, "0"),
        outs.join(", ")
    );
    let _ = writeln!(o, "}}\n");
}

/// `OpenAndFillInternal` for every tier that owns an `OpenCore`: the same single
/// pass as the public `OpenAndFill`, at the caller's `startIdx`. See
/// [`open_and_fill_internal_signature`] for why it carries no aliasing guard.
fn emit_open_and_fill_internal_wrapper(o: &mut String, func: &FuncDef) {
    let n = uname(func);
    let outs: Vec<String> = func.outputs.iter().map(|x| x.name.clone()).collect();
    let _ = writeln!(o, "/* Private function, not in public API. */\n{}\n{{", open_and_fill_internal_signature(func));
    let _ = writeln!(
        o,
        "   return TA_{n}_OpenPass( {}outBegIdx, outNBElement, {}, 1 );",
        open_core_call_head(func, "startIdx"),
        outs.join(", ")
    );
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
    // Every streamable function has an OpenAndFill (all StreamPlan tiers emit
    // one). A new tier that could not would fail loudly in `generate`, never
    // silently skip — so the declaration is unconditional, not gated.
    let open_and_fill = format!(
        "\n/*\n * OpenAndFill: like Open, but a single pass ALSO fills the caller's arrays\n * with the whole warm-up history — bit-identical to TA_{n}( 0, historyLen-1,\n * ... ).\n */\n{};\n",
        open_and_fill_signature(func)
    );
    format!(
        "\n/*\n * Streaming API for TA_{n} — incremental per-bar evaluation.\n * See docs/streaming-api-design.md.\n{note} */\ntypedef struct TA_{n}_Stream TA_{n}_Stream;\n\n{};\n\n{};\n\n{};\n\n{};\n{}",
        open_signature(func),
        update_signature(func),
        peek_signature(func),
        close_signature(func),
        open_and_fill
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
    // Resolve `PRAGMA TA_ALT` here as well as at the language backend's own
    // entry: `generate` is called directly by tests and tools, and resolving
    // twice is idempotent while forgetting once is silent.
    let resolved = func.resolved_for(crate::ir::Lang::C);
    let func: &FuncDef = &resolved;
    assert!(
        func.streaming,
        "c_stream::generate called without a streaming declaration"
    );
    let plan = streaming::validate_streamable(func, registry)
        .unwrap_or_else(|e| panic!("streaming gate: {e}"));

    // Install this function's FMA fusion sets for the crate-public render entry
    // points (render_statement*/render_expression) used throughout this call, so
    // the streamed per-bar code fuses `a*b+c` at the same sites as the batch body
    // (keeps the bitwise batch-vs-stream stream_verify gate green under FMA). The
    // detector strips the transition rewrite's `sp->`/`cur_` qualifiers (see
    // fma::stream_base), so state/series operands classify by their batch name;
    // the per-bar bar inputs become bare scalar params (`inClose[i]` -> `inClose`)
    // that carry no prefix, so seed them into real_vars explicitly — else a
    // non-power-of-two input weight would fuse in the batch/Open replay but not in
    // Update. Cleared when the guard drops.
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
    let _stream_fma_guard = super::c::StreamFmaGuard::new(stream_fma);

    let counter = Cell::new(0usize);
    let mut o = String::new();

    let _ = writeln!(o, "/**** Streaming API *****/\n");
    if let Some(m) = func.alt_marker(crate::ir::Tier::Stream, crate::ir::Lang::C) {
        let _ = writeln!(o, "/* {m} */\n");
    }

    match &plan {
        StreamPlan::Loop(model) => {
            emit_state_struct(&mut o, func, model);
            emit_release(&mut o, func, model);
            emit_step(&mut o, func, model, enums, registry, helpers, &counter);
            emit_open_core_body(&mut o, func, model, model.body, enums, registry, helpers, &counter);
            emit_update(&mut o, func, false);
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
        StreamPlan::PeriodBank(pbp) => {
            emit_period_bank(&mut o, func, pbp, registry, helpers, &counter, enums);
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
    fn extrema_mask(&self) -> String {
        "sp->xMask".to_string()
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
    let alias_fill = cp.fill_scratch_may_alias_output(outputs);
    for out in outputs {
        // `sc_<out>` is the caller's own output array when OUT_STRIDE (#205) —
        // only free it when it was actually allocated (the scalar-sink mode).
        if alias_fill {
            let _ = write!(s, "if( !{OUT_STRIDE} ) TA_Free( sc_{out} ); ");
        } else {
            let _ = write!(s, "TA_Free( sc_{out} ); ");
        }
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

/// The composed StepInternal: the producer transition (when present) writes the
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
        "/* Private function, not in public API. */\nstatic TA_RetCode TA_{n}_StepInternal( struct TA_{n}_Stream *sp, {bars}{outs} )\n{{"
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
        // Initialized, because a sub-call can now leave one unwritten. Every
        // `cur_*` is filled by the sub-call that produces it, so the initializer
        // is dead on every path where that call succeeds — but `Update`/`Peek`
        // reject a non-finite bar, and the composed step feeds SUB-CALLS a
        // library-computed intermediate (STOCH, STOCHF, STOCHRSI, MACDEXT). If
        // that intermediate ever goes non-finite the sub returns without writing
        // its output, and reading an uninitialized double is undefined behaviour
        // — the read happened, and returned different stack garbage on two
        // identical calls. Rust, Java and C# already zero their equivalents.
        let _ = writeln!(o, "   double cur_{name} = 0.0;");
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
            body_c.push_str(&render_statement_stream(s, 3, enums, registry, helpers, counter, &nullable_out_names(func)));
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
                // The return code is CHECKED, not discarded. A sub-stream is
                // fed a library-computed intermediate here, and `Update`/`Peek`
                // now reject a non-finite bar — so this call can fail without
                // the caller having done anything wrong. Swallowing it would
                // leave `cur_*` at its initializer and report TA_SUCCESS on a
                // value that was never computed; C is the only backend where
                // that was expressible, since Rust propagates with `?` and
                // Java/C# throw. Reachable only where an intermediate overflows
                // to +/-Inf, i.e. input magnitudes the library already declares
                // out of scope (#191) -- but silently wrong is not an option.
                let _ = writeln!(o, "   {{");
                let _ = writeln!(o, "      TA_RetCode subRc;");
                let _ = writeln!(o, "      if( sp->peekMode )");
                let _ = writeln!(
                    o,
                    "         subRc = {cpfx}_Peek( (const {cpfx}_Stream *)sp->sub{sub_idx}, {arg_str} );"
                );
                let _ = writeln!(o, "      else");
                let _ = writeln!(o, "         subRc = {cpfx}_Update( sp->sub{sub_idx}, {arg_str} );");
                let _ = writeln!(o, "      if( subRc != TA_SUCCESS ) return subRc;");
                let _ = writeln!(o, "   }}");
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
                    o.push_str(&render_statement_stream(st, 3, enums, registry, helpers, counter, &nullable_out_names(func)));
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
    let _ = writeln!(o, "   return TA_SUCCESS;");
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
        let _ = writeln!(o, "   TA_{n}_ReleaseInternal( stream );");
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

    // --- StepInternal -----------------------------------------------------------
    emit_composed_step(o, func, cp, &inputs, &outputs, enums, registry, helpers, counter);

    // --- Open ------------------------------------------------------------------
    emit_composed_open(o, func, cp, &outputs, &inputs, &cleanup, enums, registry, helpers, counter);
    emit_open_internal_wrapper(o, func);
    emit_open_wrapper(o, func);
    emit_open_and_fill_wrapper(o, func);
    emit_open_and_fill_internal_wrapper(o, func);

    // --- Update / Peek / Close ---------------------------------------------------
    emit_update(o, func, true);
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
        o.push_str(&finite_bar_check(func, "   ", "TA_BAD_PARAM"));
        let _ = writeln!(o, "   scratch = *stream;");
        if let Some(model) = &cp.producer {
            for (_, text) in peek_fixup_groups(model) {
                o.push_str(&text);
            }
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
        let _ = writeln!(o, "   return TA_{n}_StepInternal( &scratch, {} );\n}}\n", args.join(", "));
    }
    emit_composed_close(o, func, cp);
}

/// State struct for a loopless composed pipeline (no producer loop): the
/// optional params (referenced by combine maps as `sp-><param>`), plus the
/// peek flag and typed sub handles. Dispatch-style — no ring/window/circ/
/// extrema fields, so no `ReleaseInternal`.
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
    batch_stmt: &Statement,
    enums: &HashMap<String, EnumDef>,
    registry: &Registry,
    helpers: &HelperRegistry,
    counter: &Cell<usize>,
) -> bool {
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
    // Fused form (issue #192): one pass that BOTH warms the handle and fills
    // this sub-call's destination, so the batch sub-call the caller transcribed
    // next has nothing left to compute. The out-meta and destination arguments
    // are taken from that very statement rather than re-derived — they are not
    // uniformly the dummies (MACDEXT reads `outNbElement1`, APO/PPO/PVO read
    // `fastNb`, STOCHRSI mixes `outBegIdx2` with `dummyNBElement`), and getting
    // them from anywhere else would silently feed the wrong lengths downstream.
    let fused = sub.is_fusable() && streaming::batch_call_out_args(batch_stmt, sub).is_some();
    if fused {
        let (out_meta, dsts) = streaming::batch_call_out_args(batch_stmt, sub).unwrap();
        let rend = |e: &Expr| render_expression(e, registry, helpers, counter);
        let out_args: String = out_meta
            .iter()
            .chain(dsts.iter())
            .map(|e| rend(e))
            .collect::<Vec<_>>()
            .join(", ");
        let _ = writeln!(
            o,
            "         subRc = {cpfx}_OpenAndFillInternal( &sub{si}, {src_ptrs}, ({s_arg}), ({e_arg}) + 1, {opt_str}{out_args} );"
        );
    } else {
        let _ = writeln!(
            o,
            "         subRc = {cpfx}_OpenInternal( &sub{si}, {src_ptrs}, ({s_arg}), ({e_arg}) + 1, {opt_str}{out_dummies} );"
        );
    }
    let _ = writeln!(o, "         if( subRc != TA_SUCCESS )");
    let _ = writeln!(o, "         {{");
    for sf in &cp.series_frees {
        if sf.tail_idx > sub.tail_idx {
            o.push_str(&render_statement(&sf.stmt, 12, false, enums, registry, helpers, counter, &[]));
        }
    }
    let _ = writeln!(o, "            {cleanup};");
    let _ = writeln!(o, "            return subRc;");
    let _ = writeln!(o, "         }}");
    let _ = writeln!(o, "      }}");
    fused
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
    // The composed fill path hardcodes `double` scratch arrays + memcpy (sc_<out>
    // is `double *`, the fill copy is sizeof(double)). Every composed function is
    // real-output today; fail LOUD at generation time if that ever changes, so the
    // sc_/memcpy element type gets threaded through out_c_type rather than
    // silently truncating an integer output.
    assert!(
        func.outputs.iter().all(|out| out_c_type(func, &out.name) == "double"),
        "composed OpenAndFill assumes real (double) outputs; {} has a non-double output \
         — thread out_c_type through the sc_ scratch malloc + memcpy in emit_composed_open",
        func.name
    );
    let _ = writeln!(o, "{}\n{{", open_core_signature(func));
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

    emit_open_validation(o, func, outputs, inputs, enums);

    // startIdx arrives as a parameter: 0 for both public entry points, the
    // caller's own startIdx when a composed function opens this as a sub-stream.
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
    // out args, memmoves) — a last-value scalar cannot stand in here. When
    // the caller wants the whole range (OpenAndFill), its own output array
    // IS the historyLen-sized destination the batch tail needs, so `sc_<out>`
    // aliases it directly instead of allocating a throwaway copy that would
    // only be memcpy'd back at the end (issue #205: 938 KB / 6 mmap'd blocks,
    // about half of TA_BBANDS_OpenAndFill's own time). Only the scalar-sink
    // mode (`!outStride`, the caller's array is a single `double`) still
    // needs its own history-sized scratch.
    let alias_fill = cp.fill_scratch_may_alias_output(outputs);
    for (k, out) in outputs.iter().enumerate() {
        let prior: String = outputs[..k]
            .iter()
            .fold(String::new(), |mut s, p| {
                let _ = write!(s, "TA_Free( sc_{p} ); ");
                s
            });
        if alias_fill {
            let _ = writeln!(o, "   if( {OUT_STRIDE} ) sc_{out} = {out};");
            let _ = writeln!(o, "   else");
            let _ = writeln!(o, "   {{");
            let _ = writeln!(
                o,
                "      sc_{out} = (double *)TA_Malloc( sizeof(double) * (size_t)historyLen );"
            );
            let _ = writeln!(o, "      if( !sc_{out} ) {{ {prior}return TA_ALLOC_ERR; }}");
            let _ = writeln!(o, "   }}");
        } else {
            let _ = writeln!(
                o,
                "   sc_{out} = (double *)TA_Malloc( sizeof(double) * (size_t)historyLen );"
            );
            let _ = writeln!(o, "   if( !sc_{out} ) {{ {prior}return TA_ALLOC_ERR; }}");
        }
    }

    // --- transcription ---------------------------------------------------------
    let _ = writeln!(o, "\n   {{");
    let (region_stmts, tail_stmts) = build_composed_open_bodies(cp, outputs, cleanup);
    let mut region_c = String::new();
    for s in &region_stmts {
        region_c.push_str(&render_statement(s, 6, false, enums, registry, helpers, counter, &nullable_out_names(func)));
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
        let mut fused = false;
        for (si, sub) in cp.subs.iter().enumerate() {
            if sub.tail_idx == i {
                fused |= emit_composed_sub_open(
                    o, cp, sub, si, outputs, cleanup, &open_map, stmt, enums, registry, helpers,
                    counter,
                );
            }
        }
        // Withhold a lag-ring series' bare free: the ring seeds from its buffer
        // tail in the capture epilogue, so the buffer must outlive the tail.
        if is_lag_ring_free(stmt, &cp.sub_lag_rings) {
            continue;
        }
        // A fused sub-open already produced this statement's outputs. Keep only
        // its assignment, so the error handling the batch transcribed right
        // after it still reads a retCode — and reads the SAME one, since the
        // fused call returns what the batch call would have.
        if fused {
            if let Statement::Assign { target, .. } = stmt {
                let _ = writeln!(
                    o,
                    "      {} = subRc;",
                    render_expression(target, registry, helpers, counter)
                );
            }
            continue;
        }
        o.push_str(&render_statement(stmt, 6, false, enums, registry, helpers, counter, &nullable_out_names(func)));
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
        "      if( dummyNBElement < 1 ) {{ {epilogue_cleanup}; return TA_INSUFFICIENT_HISTORY; }}"
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
    // Fill mode: `sc_<out>` already IS the caller's `<out>` (aliased above,
    // #205), so the batch tail's writes landed there directly — nothing left
    // to hand back. Scalar-sink mode: `sc_<out>` is the owned history-sized
    // scratch; take its last element and free it.
    let _ = writeln!(o, "      *outBegIdx = dummyBegIdx;");
    let _ = writeln!(o, "      *outNBElement = dummyNBElement;");
    for out in outputs {
        if alias_fill {
            let _ = writeln!(o, "      if( !{OUT_STRIDE} ) {out}[0] = sc_{out}[dummyNBElement - 1];");
        } else {
            let _ = writeln!(
                o,
                "      if( {OUT_STRIDE} ) memcpy( {out}, sc_{out}, sizeof(double) * (size_t)dummyNBElement );"
            );
            let _ = writeln!(o, "      else {out}[0] = sc_{out}[dummyNBElement - 1];");
        }
    }
    for out in outputs {
        if alias_fill {
            let _ = writeln!(o, "      if( !{OUT_STRIDE} ) TA_Free( sc_{out} );");
        } else {
            let _ = writeln!(o, "      TA_Free( sc_{out} );");
        }
    }
    let _ = writeln!(o, "      *stream = sp;");
    let _ = writeln!(o, "      return TA_SUCCESS;");
    let _ = writeln!(o, "   }}\n}}\n");
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

/// The pointer a NULL check tests: `!x`, `x == NULL`, `x == 0`. An unrecognized
/// spelling (`NULL == x`, a cast) yields `None`, which keeps both blocks — the
/// pre-existing duplicate, never a dropped check.
fn null_check_var(cond: &Expr) -> Option<&str> {
    match cond {
        Expr::Not(inner) => match inner.as_ref() {
            Expr::Var(v) => Some(v.as_str()),
            _ => None,
        },
        Expr::BinOp(lhs, crate::ir::BinOp::Eq, rhs) => match (lhs.as_ref(), rhs.as_ref()) {
            (Expr::Var(v), Expr::Var(n)) if n == "NULL" => Some(v.as_str()),
            (Expr::Var(v), Expr::IntLiteral(0)) => Some(v.as_str()),
            _ => None,
        },
        _ => None,
    }
}

/// Whether a rewritten `then` body exits with `TA_ALLOC_ERR`. Both shapes are
/// accepted: the bare `Return`, and the cleanup `Block` the early-return arm
/// wraps it in (children are rewritten before their parent, so in practice the
/// wrapped one is what arrives here). Only `Block` is descended into — a return
/// nested inside a further conditional is not an unconditional exit.
fn returns_alloc_err(body: &[Statement]) -> bool {
    body.iter().any(|s| match s {
        Statement::Return {
            value: Some(Expr::Var(v)),
        } => matches!(v.as_str(), "ALLOC_ERR" | "TA_ALLOC_ERR"),
        Statement::Block { body } => returns_alloc_err(body),
        _ => false,
    })
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
    // "Control cannot reach here with this pointer NULL." Set ONLY by the
    // injection below, so an allocation form the alloc arms do not recognize
    // leaves the pointer unproven and its source check is kept. Killed by any
    // other write to the pointer, and by every statement that is not a leaf
    // (see the default arm), so a proof established inside a branch cannot
    // escape it. Every kill errs toward keeping a redundant block; only the
    // reverse could drop a live check.
    let proven: std::cell::RefCell<std::collections::BTreeSet<String>> =
        std::cell::RefCell::new(std::collections::BTreeSet::new());
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
                proven.borrow_mut().insert(v.clone());
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
                proven.borrow_mut().insert(name.clone());
                Some(malloc_null_check_block(&name, init, &cu))
            }
            // Any OTHER write to a pointer already proven non-NULL invalidates
            // the proof (`tempBuffer = someOtherBuffer;` re-points it). Below
            // both alloc arms, so an allocating write still injects and re-proves.
            Statement::Assign {
                target: Expr::Var(ref v),
                ..
            }
            | Statement::VarDecl { name: ref v, .. }
                if proven.borrow().contains(v) =>
            {
                proven.borrow_mut().remove(v);
                Some(s)
            }
            // The source's OWN NULL check for a pointer already proven
            // non-NULL — every composed input that allocates an intermediate
            // writes one today. The injected check carries the cascading
            // `free()` of everything allocated before it that the source's does
            // not, so the source's is redundant on every path reaching it and
            // renders as a second, unreachable copy of the same block. Keyed on
            // the proof, not on adjacency: an intervening comment or an
            // `x == NULL` spelling must not resurrect the duplicate.
            //
            // An `else` arm is required to be empty — dropping the `If` drops
            // the `else` with it, and that would be a silent loss rather than a
            // redundancy. No input has one.
            //
            // ORDER: must precede the non-leaf default at the bottom, which
            // would otherwise swallow every `If` before this arm is reached.
            Statement::If {
                ref condition,
                ref then_body,
                ref else_body,
                ..
            } if else_body.is_empty()
                && null_check_var(condition).is_some_and(|v| proven.borrow().contains(v))
                && returns_alloc_err(then_body) =>
            {
                None
            }
            Statement::Return { value } => {
                let mapped = match value {
                    Some(Expr::Var(v)) if matches!(v.as_str(), "SUCCESS" | "TA_SUCCESS") => {
                        Some(Expr::Var("INSUFFICIENT_HISTORY".into()))
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
            // Statements that cannot contain a nested body: the proof carries
            // straight across them.
            Statement::Assign { .. }
            | Statement::VarDecl { .. }
            | Statement::UnrollHint { .. }
            | Statement::Break
            | Statement::Continue
            | Statement::Expr(_)
            | Statement::CircBuf(_)
            | Statement::Comment(_) => Some(s),
            // Everything else has a body, and `rewrite_stmts` hands `fs` such a
            // statement only AFTER its children — so anything proven inside was
            // proven on a single path and must not outlive it. Blunt: it also
            // discards proofs established BEFORE the statement, which do
            // dominate it. That is the safe direction (a kept redundant block,
            // never a dropped live check), and writing it as the DEFAULT rather
            // than as a list of body-bearing variants is what keeps it safe
            // when a new `Statement` variant is added — an unknown statement
            // clears rather than silently letting a proof leak out of a branch.
            other => {
                proven.borrow_mut().clear();
                Some(other)
            }
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

/// Which of the dispatch tier's three open entry points a body is being
/// emitted for.
///
/// The three differ in exactly four places — the signature, which pointers are
/// checked, what the identity path hands back, and which callee entry point
/// each arm delegates to. Everything else (the `TA_MAX_INDEX` bound, the
/// optional-param validation, the handle allocation, the arm switch and the
/// cleanup tail) is one text emitted once, so a fourth mode costs a variant
/// and four arms rather than a fourth copy of the body.
#[derive(Clone, Copy, PartialEq, Eq)]
enum DispatchOpen {
    /// `OpenInternal` (plus the public `Open` wrapper over it): warm the
    /// handle and hand back the last bar only.
    Scalar,
    /// `OpenAndFill`: public, anchored at bar 0, fills the caller's arrays.
    Fill,
    /// `OpenAndFillInternal`: the same fill anchored at the caller's `startIdx`
    /// and without the aliasing rejection — what a composed `Open` fuses into
    /// (issue #192). MA is the callee of 13 of the 18 shipped composed
    /// sub-calls, so without this variant the fusion would reach almost none
    /// of them.
    ///
    /// Dispatching to the arm's *public* `OpenAndFill` here would be wrong
    /// twice over: it has no `startIdx`, and it carries the aliasing guard the
    /// internal path deliberately drops.
    FillInternal,
}

impl DispatchOpen {
    /// Whether this mode writes the caller's output arrays over the whole
    /// history (and so carries the batch API's `outBegIdx`/`outNBElement`
    /// pair) rather than handing back one value per output.
    fn fills(self) -> bool {
        self != Self::Scalar
    }
}

/// One of the dispatch tier's open bodies (MA): dispatch to the selected arm's
/// matching entry point, with the identity path served in place; unsupported
/// arms (a callee with no stream) reject. Handle layout is the same in every
/// mode, so Update/Peek/Close are shared.
#[allow(clippy::too_many_lines, clippy::too_many_arguments)]
fn emit_dispatch_open(
    o: &mut String,
    func: &FuncDef,
    dp: &DispatchPlan,
    mode: DispatchOpen,
    enums: &HashMap<String, EnumDef>,
    registry: &Registry,
    helpers: &HelperRegistry,
    counter: &Cell<usize>,
) {
    let n = uname(func);
    let inputs = streaming::input_array_names(func);
    let outputs: Vec<String> = func.outputs.iter().map(|x| x.name.clone()).collect();
    let bar_args: String = inputs.join(", ");
    let case_of = |label: &str| render_c_switch_label(label, enums);

    if mode != DispatchOpen::Fill {
        let _ = writeln!(o, "/* Private function, not in public API. */");
    }
    let _ = writeln!(
        o,
        "{}\n{{",
        match mode {
            DispatchOpen::Scalar => open_internal_signature(func),
            DispatchOpen::Fill => open_and_fill_signature(func),
            DispatchOpen::FillInternal => open_and_fill_internal_signature(func),
        }
    );
    let _ = writeln!(o, "   struct TA_{n}_Stream *sp;");
    let _ = writeln!(o, "   TA_RetCode retCode;");
    let _ = writeln!(o, "\n   if( !stream ) return TA_BAD_PARAM;");
    let _ = writeln!(o, "   *stream = NULL;");
    let mut null_checks: Vec<String> = inputs
        .iter()
        .chain(outputs.iter())
        .map(|x| format!("!{x}"))
        .collect();
    if mode.fills() {
        null_checks.push("!outBegIdx".into());
        null_checks.push("!outNBElement".into());
    }
    let _ = writeln!(o, "   if( {} ) return TA_BAD_PARAM;", null_checks.join(" || "));
    let _ = writeln!(o, "   if( historyLen < 1 ) return TA_BAD_PARAM;");
    // The warm-up covers bars 0..historyLen-1, so its last bar is an index like
    // any other and TA_MAX_INDEX bounds it too (#180). Without this the
    // streaming entry points would compute over exactly the ranges the batch
    // call refuses, and the two are required to agree bit for bit.
    let _ = writeln!(
        o,
        "   if( historyLen > TA_MAX_INDEX + 1 ) return TA_OUT_OF_RANGE_END_INDEX;"
    );
    if mode == DispatchOpen::Scalar {
        // The arms forward it, but an identity-only or all-rejecting dispatch
        // would leave it unread.
        let _ = writeln!(o, "   (void)startIdx;");
    }
    if mode == DispatchOpen::Fill {
        // Aliasing: fill writes the caller's arrays, so they must be distinct
        // from every input and from each other (the callee OpenAndFill also
        // guards, but the identity path below fills directly). The internal
        // variant deliberately carries no such guard — see [`DispatchOpen`].
        let mut alias: Vec<String> = Vec::new();
        for outp in &outputs {
            for inp in &inputs {
                alias.push(format!("(const void *){outp} == (const void *){inp}"));
            }
        }
        for (i, a) in outputs.iter().enumerate() {
            for b in &outputs[i + 1..] {
                alias.push(format!("(const void *){a} == (const void *){b}"));
            }
        }
        if !alias.is_empty() {
            let _ = writeln!(o, "   if( {} ) return TA_BAD_PARAM;", alias.join(" || "));
        }
    }
    o.push_str(&emit_opt_param_validation(func, "TA_BAD_PARAM", enums));
    let _ = writeln!(o, "\n   sp = (struct TA_{n}_Stream *)TA_Malloc( sizeof(*sp) );");
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
        let lb_call = format!("TA_{n}_Lookback( {} )", lookback_args.join(", "));
        let _ = writeln!(o, "\n   if( {cond} )\n   {{");
        let _ = writeln!(
            o,
            "      if( historyLen < {lb_call} + 1 ) {{ TA_Free( sp ); return TA_INSUFFICIENT_HISTORY; }}"
        );
        if mode.fills() {
            let _ = writeln!(o, "      {{");
            let _ = writeln!(o, "         int fillLb = {lb_call};");
            let _ = writeln!(o, "         int fillIdx;");
            if mode == DispatchOpen::FillInternal {
                // batch( startIdx, .. ) begins at max(startIdx, lookback); the
                // public entry point's startIdx is 0, so only this variant has
                // to clamp.
                let _ = writeln!(o, "         if( startIdx > fillLb ) fillLb = startIdx;");
                let _ = writeln!(
                    o,
                    "         if( historyLen < fillLb + 1 ) {{ TA_Free( sp ); return TA_INSUFFICIENT_HISTORY; }}"
                );
            }
            let _ = writeln!(o, "         *outBegIdx = fillLb;");
            let _ = writeln!(o, "         *outNBElement = historyLen - fillLb;");
            let _ = writeln!(o, "         for( fillIdx = 0; fillIdx < historyLen - fillLb; fillIdx++ )");
            let _ = writeln!(o, "         {{");
            for (out, inp) in &idp.pairs {
                let _ = writeln!(o, "            {out}[fillIdx] = {inp}[fillLb + fillIdx];");
            }
            let _ = writeln!(o, "         }}");
            let _ = writeln!(o, "      }}");
        } else {
            for (out, inp) in &idp.pairs {
                let _ = writeln!(o, "      *{out} = {inp}[historyLen - 1];");
            }
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
        let opt_str = arm.opt_args.iter().fold(String::new(), |mut s, e| {
            let _ = write!(s, "{}, ", render_expression(e, registry, helpers, counter));
            s
        });
        let arm_out_args = dispatch_arm_out_args(arm, &outputs);
        // Each mode delegates to the callee's matching entry point: the fills
        // hand the caller's arrays and out-meta straight down, and only the
        // startIdx-anchored modes forward a startIdx.
        let call = match mode {
            DispatchOpen::Scalar => format!(
                "{cp}_OpenInternal( &sub, {bar_args}, startIdx, historyLen, {opt_str}{arm_out_args} )"
            ),
            DispatchOpen::Fill => format!(
                "{cp}_OpenAndFill( &sub, {bar_args}, historyLen, {opt_str}outBegIdx, outNBElement, {arm_out_args} )"
            ),
            DispatchOpen::FillInternal => format!(
                "{cp}_OpenAndFillInternal( &sub, {bar_args}, startIdx, historyLen, {opt_str}outBegIdx, outNBElement, {arm_out_args} )"
            ),
        };
        let _ = writeln!(o, "   case {}:", case_of(&arm.label));
        let _ = writeln!(o, "      {{");
        let _ = writeln!(o, "         {cp}_Stream *sub = NULL;");
        let _ = writeln!(o, "         retCode = {call};");
        let _ = writeln!(o, "         sp->sub = sub;");
        let _ = writeln!(o, "      }}");
        let _ = writeln!(o, "      break;");
    }
    // Unsupported arms reject at open — a documented capability limitation
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
}

/// Per-arm dispatch bodies for Update/Peek/Close, plus the shared open
/// switch. All labels render through the batch's own switch-label mapping so
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
    // One body emitter, three modes. `Open` itself is the public one-liner over
    // `OpenInternal` and is emitted next to the body it wraps; the two fill
    // modes are their own public surface.
    for mode in [DispatchOpen::Scalar, DispatchOpen::Fill, DispatchOpen::FillInternal] {
        emit_dispatch_open(o, func, dp, mode, enums, registry, helpers, counter);
        if mode == DispatchOpen::Scalar {
            emit_open_wrapper(o, func);
        }
    }

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
        // Checked here rather than left to the sub-stream's own Update/Peek: the
        // identity arm below never reaches a sub-stream at all, it copies the bar
        // straight to the output.
        o.push_str(&finite_bar_check(func, "   ", "TA_BAD_PARAM"));
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
            let arm_out_args = dispatch_arm_out_args(arm, &outputs);
            let _ = writeln!(o, "   case {}:", case_of(&arm.label));
            let _ = writeln!(
                o,
                "      return {cp}_{verb}( ({const_qual}{cp}_Stream *)stream->sub, {bar_args}, {arm_out_args} );"
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
/// the TYPE-CHECKED UNION of both modes' SCALAR state (a name shared by the two
/// modes — DI/DM's `prevHigh`/`prevLow`, TRIMA's `numerator` — is one field;
/// mode-B-only fields sit zeroed under mode A), then the UNION of both modes'
/// NON-SCALAR state (rings/windows/circs/extrema/feedback/lags): mode-A fields
/// first, then mode-B-only fields (HMA: the general arm's half-period ring and
/// d-CIRCBUF). The mode is fixed at Open and re-derived from the immutable
/// discriminator param each step (the Dispatch precedent — no `mode` tag), so
/// each arm touches only its own fields; Open's memset leaves the inactive
/// mode's buffer pointers NULL (Release/Peek guard on them).
fn emit_dual_state_struct(o: &mut String, func: &FuncDef, ma: &StreamModel, mb: &StreamModel) {
    let mut a_nonscalar = String::new();
    emit_nonscalar_struct_fields(&mut a_nonscalar, func, ma);
    let mut b_nonscalar = String::new();
    emit_nonscalar_struct_fields(&mut b_nonscalar, func, mb);
    // Line-level union: every field line is `   <type> <name>;` with the name
    // derived from its spec, so a spec both modes share renders the identical
    // line and dedups away. A same-named field rendering DIFFERENTLY across
    // modes is a type conflict — caught by the member-name check below.
    let a_lines: std::collections::BTreeSet<&str> = a_nonscalar.lines().collect();
    let mut union_nonscalar = a_nonscalar.clone();
    for line in b_nonscalar.lines() {
        if !a_lines.contains(line) {
            union_nonscalar.push_str(line);
            union_nonscalar.push('\n');
        }
    }
    let mut member_names = std::collections::BTreeSet::new();
    for line in union_nonscalar.lines() {
        let name = line
            .trim()
            .trim_end_matches(';')
            .split_whitespace()
            .last()
            .unwrap_or("")
            .trim_start_matches('*')
            .to_string();
        assert!(
            member_names.insert(name.clone()),
            "{}: dual-mode non-scalar field `{name}` renders differently across modes",
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
    // Union of the two modes' SCALAR state, mode-A order first, dedup by name.
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
    o.push_str(&union_nonscalar);
    let _ = writeln!(o, "}};\n");
}

/// Union of both modes' circs (mode-A order first, dedup by id). A shared id
/// must expose identical storages — they name struct fields, hoisted Open
/// locals, release frees and Peek mirrors that both arms address.
fn dual_union_circs(func: &FuncDef, ma: &StreamModel, mb: &StreamModel) -> Vec<CircState> {
    let mut v: Vec<CircState> = ma.circs().to_vec();
    for c in mb.circs() {
        if let Some(prev) = v.iter().find(|p| p.id == c.id) {
            assert!(
                circ_storages(prev) == circ_storages(c),
                "{}: dual-mode circ `{}` differs across modes",
                func.name,
                c.id
            );
        } else {
            v.push(c.clone());
        }
    }
    v
}

/// Remove top-level `VarDecl`s whose variable is never READ in `body`, together
/// with the top-level assignments that only ever wrote it.
///
/// Used only for the dual-mode Open arms: each arm is `shared prologue ++ its
/// own arm body`, and the prologue both declares and INITIALIZES the union of
/// both modes' function-top scalars. Dropping only the unreferenced decls
/// leaves the ones the shared prologue assigns — HMA's degenerate arm
/// (`optInTimePeriod` 2 or 3) inherits `halfPeriod = optInTimePeriod / 2` from
/// the prologue and then never reads it, because at that period the formula
/// collapses and the half-period WMA disappears. That is a
/// `-Wunused-but-set-variable` in the consumer's build, so a write alone must
/// not count as a use.
///
/// Behavior-preserving because the three conditions are checked together: the
/// variable is never read anywhere in the arm, every assignment to it sits at
/// the top level (nothing conditional or looped is removed), and every such
/// right-hand side is call-free, so evaluating it can be observed only through
/// the variable being dropped.
fn drop_unused_decls(body: Vec<Statement>) -> Vec<Statement> {
    // A plain `x = <expr>` writes x; it does not read it. Every other mention
    // — a compound assign, an index, a deref, any rvalue — is a read.
    let mut read: std::collections::BTreeSet<String> = std::collections::BTreeSet::new();
    let note_reads = |e: &Expr, out: &mut std::collections::BTreeSet<String>| {
        streaming::walk_expr(e, &mut |x| {
            if let Expr::Var(v) = x {
                out.insert(v.clone());
            }
        });
    };
    for s in &body {
        match s {
            Statement::Assign { target, value, compound } => {
                note_reads(value, &mut read);
                if *compound || !matches!(target, Expr::Var(_)) {
                    note_reads(target, &mut read);
                }
            }
            other => streaming::walk_stmt_exprs(other, &mut |e| note_reads(e, &mut read)),
        }
        // `walk_stmt_exprs` does not descend into CircBuf, but CIRCBUF_INIT's
        // size IS an expression and reading it is a real use — HMA's general
        // arm sizes its de-lag ring with `ringSize`, which is otherwise only
        // ever assigned. Missing this drops a live declaration.
        collect_circbuf_size_reads(s, &mut |e| note_reads(e, &mut read));
    }

    // Only names the arm writes exclusively from the top level, with a
    // call-free RHS, are eligible; anything assigned deeper stays untouched.
    let mut nested_assigned: std::collections::BTreeSet<String> = std::collections::BTreeSet::new();
    for s in &body {
        if matches!(s, Statement::Assign { .. }) {
            continue;
        }
        collect_assigned_targets(s, &mut nested_assigned);
    }

    let impure = |e: &Expr| {
        let mut found = false;
        streaming::walk_expr(e, &mut |x| {
            if matches!(x, Expr::FuncCall(..)) {
                found = true;
            }
        });
        found
    };

    let droppable = |name: &String, body: &[Statement]| {
        if read.contains(name) || nested_assigned.contains(name) {
            return false;
        }
        body.iter().all(|s| match s {
            Statement::Assign { target, value, compound } => {
                !matches!(target, Expr::Var(v) if v == name) || (!*compound && !impure(value))
            }
            _ => true,
        })
    };

    let dead: std::collections::BTreeSet<String> = body
        .iter()
        .filter_map(|s| match s {
            Statement::VarDecl { name, .. } if droppable(name, &body) => Some(name.clone()),
            _ => None,
        })
        .collect();

    body.into_iter()
        .filter(|s| match s {
            Statement::VarDecl { name, .. } => !dead.contains(name),
            Statement::Assign { target: Expr::Var(v), .. } => !dead.contains(v),
            _ => true,
        })
        .collect()
}

/// Visit the `size` expression of every `CIRCBUF_INIT` reachable from `s`.
/// [`streaming::walk_stmt_exprs`] treats `Statement::CircBuf` as opaque, so this
/// is the one expression a use-analysis over it would otherwise miss.
fn collect_circbuf_size_reads(s: &Statement, f: &mut dyn FnMut(&Expr)) {
    match s {
        Statement::CircBuf(crate::ir::CircBuf::Init { size, .. }) => f(size),
        Statement::CircBuf(_) | Statement::VarDecl { .. } | Statement::Assign { .. }
        | Statement::Comment(_) | Statement::UnrollHint { .. } | Statement::Break
        | Statement::Continue | Statement::Return { .. } | Statement::Expr(_) => {}
        Statement::While { body, .. } | Statement::DoWhile { body, .. }
        | Statement::For { body, .. } | Statement::Block { body } => {
            for st in body {
                collect_circbuf_size_reads(st, f);
            }
        }
        Statement::If { then_body, else_body, .. } => {
            for st in then_body.iter().chain(else_body) {
                collect_circbuf_size_reads(st, f);
            }
        }
        Statement::Switch { cases, default, .. } => {
            for st in cases.iter().flat_map(|(_, b)| b).chain(default) {
                collect_circbuf_size_reads(st, f);
            }
        }
        Statement::ForC { init, update, body, .. } => {
            collect_circbuf_size_reads(init, f);
            collect_circbuf_size_reads(update, f);
            for st in body {
                collect_circbuf_size_reads(st, f);
            }
        }
    }
}

/// Every variable a statement assigns to, at any depth.
fn collect_assigned_targets(s: &Statement, out: &mut std::collections::BTreeSet<String>) {
    match s {
        Statement::Assign { target: Expr::Var(v), .. } => {
            out.insert(v.clone());
        }
        Statement::Assign { .. } | Statement::VarDecl { .. } | Statement::CircBuf(_)
        | Statement::Comment(_) | Statement::UnrollHint { .. } | Statement::Break
        | Statement::Continue | Statement::Return { .. } | Statement::Expr(_) => {}
        Statement::While { body, .. } | Statement::DoWhile { body, .. }
        | Statement::For { body, .. } | Statement::Block { body } => {
            for st in body {
                collect_assigned_targets(st, out);
            }
        }
        Statement::If { then_body, else_body, .. } => {
            for st in then_body.iter().chain(else_body) {
                collect_assigned_targets(st, out);
            }
        }
        Statement::Switch { cases, default, .. } => {
            for st in cases.iter().flat_map(|(_, b)| b).chain(default) {
                collect_assigned_targets(st, out);
            }
        }
        Statement::ForC { init, update, body, .. } => {
            collect_assigned_targets(init, out);
            collect_assigned_targets(update, out);
            for st in body {
                collect_assigned_targets(st, out);
            }
        }
    }
}

/// Emit the full dual-mode stream section: one union struct, one predicate-
/// branching StepInternal, one predicate-branching OpenInternal (+ public Open
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
    let union_circs = dual_union_circs(func, ma, mb);

    // --- state struct -------------------------------------------------------
    emit_dual_state_struct(o, func, ma, mb);
    // ReleaseInternal (frees the union of both modes' buffers) for a
    // buffer-carrying dual mode (TRIMA rings, HMA rings + circ); inert for a
    // scalar mode (DI/DM). Emitted before Open, whose malloc-failure paths
    // call it.
    emit_release_dual(o, func, ma, mb);

    // --- Step: one function, mode selected from the stored param ------------
    let bars = bar_params_sig(func);
    let outs = out_params_sig(func);
    let _ = writeln!(
        o,
        "/* Private function, not in public API. */\nstatic void TA_{n}_StepInternal( struct TA_{n}_Stream *sp, {bars}{outs} )\n{{"
    );
    // Identity (HMA period 1) short-circuits ahead of the predicate, as it does
    // in the batch and in Open: it is a property of the function, not of a mode.
    emit_identity_step_branch(o, ma, enums, registry, helpers, counter, 3);
    let pred_h = render_dual_pred(&dmp.predicate, true, func, registry, helpers, counter);
    let _ = writeln!(o, "   if( {pred_h} )\n   {{");
    emit_step_inner(o, ma, enums, registry, helpers, counter, 6, false);
    let _ = writeln!(o, "   }}\n   else\n   {{");
    emit_step_inner(o, mb, enums, registry, helpers, counter, 6, false);
    let _ = writeln!(o, "   }}\n}}\n");

    // --- OpenCore: shared head, then a predicate branch per mode ------------
    // The head is `emit_open_head` over the UNION circ hoist: a mode-B-only
    // CIRCBUF's locals (HMA's dRing) are declared once at function scope and
    // only the owning arm touches them. Its identity fast path leaves the whole
    // union memset, including the buffers only the general arm dereferences;
    // what keeps that arm from running is the step's guard, hoisted above the
    // mode predicate.
    emit_open_head(o, func, ma, &union_circs, registry, helpers, counter, enums);

    // Each mode transcribes the SHARED PROLOGUE, then its own arm body, then the
    // SHARED EPILOGUE (empty for the early-return form; the out-meta + return tail
    // for the if/else form). The prologue computes the mode-appropriate lookback/
    // clamp, so min-history is per-mode correct by construction. The shared
    // prologue declares the UNION of both modes' function-top locals, so a per-arm
    // dead-decl drop is applied: a mode that never touches the other mode's
    // accumulators or warm-up counter would otherwise emit -Wunused-variable.
    let compose = |arm_body: &[Statement]| -> Vec<Statement> {
        let mut v = dmp.prologue.to_vec();
        v.extend_from_slice(arm_body);
        v.extend_from_slice(dmp.epilogue);
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
    emit_open_internal_wrapper(o, func);
    emit_open_wrapper(o, func);
    emit_open_and_fill_wrapper(o, func);
    emit_open_and_fill_internal_wrapper(o, func);

    // --- Update / Peek / Close (mode-fixed handle: Peek mirrors the union of
    // both modes' buffers, guarding mode-exclusive groups; Close releases the
    // union) -----------------------------------------------------------------
    emit_update(o, func, false);
    emit_peek_dual(o, func, ma, mb);
    emit_close_from(o, func, ma.needs_release() || mb.needs_release());
}

fn emit_state_struct(o: &mut String, func: &FuncDef, model: &StreamModel) {
    emit_state_struct_ex(o, func, model, "");
}

/// State struct with extra trailing fields (composed tier: peekMode + typed
/// sub handles appended after the producer's own fields).
/// The non-scalar handle fields (out-feedback, lag slots, ring/window/circ/
/// extrema buffers + their Peek mirrors) for one model. Shared by the loop-tier
/// struct and the dual-mode union struct (whose two modes carry identical
/// non-scalar state — TRIMA's odd/even arms share the same rings — so the union
/// emits one model's set).
fn emit_nonscalar_struct_fields(o: &mut String, func: &FuncDef, model: &StreamModel) {
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
        let _ = writeln!(o, "   int xPhys;");
        let _ = writeln!(o, "   int xMask;");
        for arr in &ex.arrays {
            let _ = writeln!(o, "   double *x_{arr};");
            let _ = writeln!(o, "   double *xMirror_{arr};");
        }
    }
}

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
    emit_nonscalar_struct_fields(o, func, model);
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

/// Free-line list for one model's heap buffers (the `ReleaseInternal` body,
/// minus the trailing handle free). Every line is NULL-guarded, so a line
/// whose buffer the active mode never allocated is a no-op — which is what
/// lets the dual-mode union release line-dedup two models' lists.
fn release_free_lines(model: &StreamModel) -> Vec<String> {
    let mut lines: Vec<String> = Vec::new();
    for ring in model.rings() {
        for arr in &ring.arrays {
            lines.push(format!("   if( sp->ring_{0}_{arr} ) TA_Free( sp->ring_{0}_{arr} );", ring.var));
            lines.push(format!("   if( sp->ringMirror_{0}_{arr} ) TA_Free( sp->ringMirror_{0}_{arr} );", ring.var));
        }
    }
    for win in model.windows() {
        for arr in &win.arrays {
            lines.push(format!("   if( sp->win_{0}_{arr} ) TA_Free( sp->win_{0}_{arr} );", win.var));
            lines.push(format!("   if( sp->winMirror_{0}_{arr} ) TA_Free( sp->winMirror_{0}_{arr} );", win.var));
        }
    }
    for circ in model.circs() {
        for (storage, _) in circ_storages(circ) {
            lines.push(format!("   if( sp->cb_{storage} ) TA_Free( sp->cb_{storage} );"));
            lines.push(format!("   if( sp->cbMirror_{storage} ) TA_Free( sp->cbMirror_{storage} );"));
        }
    }
    if let Some(ex) = model.extrema() {
        for arr in &ex.arrays {
            lines.push(format!("   if( sp->x_{arr} ) TA_Free( sp->x_{arr} );"));
            lines.push(format!("   if( sp->xMirror_{arr} ) TA_Free( sp->xMirror_{arr} );"));
        }
    }
    lines
}

fn emit_release_from(o: &mut String, func: &FuncDef, lines: &[String]) {
    let n = uname(func);
    let _ = writeln!(o, "/* Private function, not in public API. */
static void TA_{n}_ReleaseInternal( struct TA_{n}_Stream *sp )
{{");
    let _ = writeln!(o, "   if( !sp ) return;");
    for line in lines {
        let _ = writeln!(o, "{line}");
    }
    let _ = writeln!(o, "   TA_Free( sp );
}}
");
}

/// `static void TA_<N>_ReleaseInternal(...)`: frees every ring buffer and the
/// handle itself. Emitted only for ring models; safe on partially-allocated
/// handles (open memsets the struct, so unallocated buffers are NULL).
fn emit_release(o: &mut String, func: &FuncDef, model: &StreamModel) {
    if !model.needs_release() {
        return;
    }
    emit_release_from(o, func, &release_free_lines(model));
}

/// Dual-mode: one `ReleaseInternal` freeing the UNION of both modes' buffers
/// (mode-A lines first, dedup by line). Open memsets the handle, so the
/// inactive mode's pointers are NULL and their guarded frees no-op.
fn emit_release_dual(o: &mut String, func: &FuncDef, ma: &StreamModel, mb: &StreamModel) {
    if !ma.needs_release() && !mb.needs_release() {
        return;
    }
    let mut lines = release_free_lines(ma);
    let seen: std::collections::BTreeSet<String> = lines.iter().cloned().collect();
    for line in release_free_lines(mb) {
        if !seen.contains(&line) {
            lines.push(line);
        }
    }
    emit_release_from(o, func, &lines);
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
        "/* Private function, not in public API. */\nstatic void TA_{n}_StepInternal( struct TA_{n}_Stream *sp, {bars}{outs} )\n{{"
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
        body_c.push_str(&render_statement_stream(s, indent, enums, registry, helpers, counter, &nullable_out_names(model.func)));
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

/// The identity short-circuit at the top of a dual-mode step, above the mode
/// predicate — the one place it belongs, since it holds for the whole function
/// (the arms are marked `identity_hoisted`, so they no longer carry a copy).
fn emit_identity_step_branch(
    o: &mut String,
    model: &StreamModel,
    enums: &HashMap<String, EnumDef>,
    registry: &Registry,
    helpers: &HelperRegistry,
    counter: &Cell<usize>,
    indent: usize,
) {
    if let Some(s) = streaming::identity_step_branch(model, &CNames) {
        o.push_str(&render_statement_stream(
            &s,
            indent,
            enums,
            registry,
            helpers,
            counter,
            &nullable_out_names(model.func),
        ));
    }
}

/// Extrema automatons carry batch-absolute int indices that grow by one
/// per bar. Rebase them by a multiple of the physical ring size long before
/// INT_MAX: index differences and `& xMask` slots are invariant, so the
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
            "      int rebaseShift = sp->{} & ~sp->xMask;",
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

/// The `OpenInternal` head shared by the loop tier and dual-mode:
/// signature, declarations, param validation, initialization, and the identity
/// fast path. The caller then emits the transcribed body arm(s) and closes the
/// function.
fn emit_open_head(
    o: &mut String,
    func: &FuncDef,
    model: &StreamModel,
    hoist_circs: &[CircState],
    registry: &Registry,
    helpers: &HelperRegistry,
    counter: &Cell<usize>,
    enums: &HashMap<String, EnumDef>,
) {
    let n = uname(func);
    let inputs = streaming::input_array_names(func);
    let _ = writeln!(o, "{}\n{{", open_core_signature(func));

    // --- declarations -------------------------------------------------------
    let _ = writeln!(o, "   struct TA_{n}_Stream *sp;");
    emit_circ_hoist(o, func, hoist_circs);
    let _ = writeln!(o, "   int endIdx;");
    // Kept as locals even though the core always has real out-meta pointers:
    // the transcribed body writes them on paths the fill contract does not
    // publish, and the composed tier reads them back as plain ints.
    let _ = writeln!(o, "   int dummyBegIdx;");
    let _ = writeln!(o, "   int dummyNBElement;");
    for (name, c_type) in &func.private_extra_params {
        let _ = writeln!(o, "   {c_type} {name};");
    }

    emit_open_validation(o, func, &model.outputs, &inputs, enums);

    // --- initialization (after defaults are substituted) ---------------------
    // startIdx arrives as a parameter: 0 for both standalone public entry
    // points, the sub-call's own startIdx when a composed function opens this
    // as a sub-stream.
    let _ = writeln!(o, "\n   endIdx = historyLen - 1;");
    let _ = writeln!(o, "   dummyBegIdx = 0;");
    let _ = writeln!(o, "   dummyNBElement = 0;");
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

/// The whole Open family for any tier whose core is `emit_open_head` + a single
/// `emit_open_arm`: the merged `OpenCore`, then `OpenInternal` (stride 0),
/// the public `Open`, and `OpenAndFill` (stride 1). `body` is the transcribed
/// batch region — loop: `model.body`; dual-mode: `prologue ++ arm body ++
/// epilogue`.
#[allow(clippy::too_many_arguments)]
fn emit_open_core_body(
    o: &mut String,
    func: &FuncDef,
    model: &StreamModel,
    body: &[Statement],
    enums: &HashMap<String, EnumDef>,
    registry: &Registry,
    helpers: &HelperRegistry,
    counter: &Cell<usize>,
) {
    emit_open_head(o, func, model, model.circs(), registry, helpers, counter, enums);
    emit_open_arm(o, func, model, body, enums, registry, helpers, counter);
    let _ = writeln!(o, "}}\n");
    emit_open_internal_wrapper(o, func);
    emit_open_wrapper(o, func);
    emit_open_and_fill_wrapper(o, func);
    emit_open_and_fill_internal_wrapper(o, func);
}

/// Emit the period-bank stream section (MAVP): a moving average whose period
/// varies per bar. Open builds a bank of `maxPeriod - minPeriod + 1` sub-MA
/// streams (one per possible period, each seeded from history via the callee's
/// `OpenInternal`); Update advances the whole bank in lockstep and outputs the
/// slot the current bar's clamped period selects; Peek previews only the
/// selected slot; Close frees the bank. The bank inherits the callee's
/// per-MAType streamability (MAType_MAMA rejects at the first sub-open).
#[allow(clippy::too_many_lines)]
fn emit_period_bank(
    o: &mut String,
    func: &FuncDef,
    plan: &streaming::PeriodBankPlan,
    registry: &Registry,
    helpers: &HelperRegistry,
    counter: &Cell<usize>,
    enums: &HashMap<String, EnumDef>,
) {
    let n = uname(func);
    let pre = callee_prefix(&plan.callee); // e.g. TA_MA
    let subty = format!("struct {pre}_Stream");
    let min = &plan.min_param;
    let max = &plan.max_param;
    let price = &plan.price_input;
    let period = &plan.period_input;
    let out = &plan.output;
    let inputs = streaming::input_array_names(func);

    // Sub-open opt args in the callee's signature order.
    let open_opts: String = plan
        .callee_opts
        .iter()
        .map(|a| match a {
            streaming::PeriodBankArg::Period => format!("{min} + k"),
            streaming::PeriodBankArg::MAType => plan.matype_param.clone(),
        })
        .collect::<Vec<_>>()
        .join(", ");

    // --- state struct -------------------------------------------------------
    let _ = writeln!(o, "struct TA_{n}_Stream {{");
    for p in &func.optional_inputs {
        let _ = writeln!(o, "   {} {};", opt_param_c_type(&p.param_type), p.name);
    }
    let _ = writeln!(o, "   int nBank;");
    let _ = writeln!(o, "   {subty} **bank;");
    let _ = writeln!(o, "   double *scratch;");
    let _ = writeln!(o, "}};\n");

    // --- OpenInternal -------------------------------------------------------
    let _ = writeln!(o, "/* Private function, not in public API. */\n{}\n{{", open_internal_signature(func));
    let _ = writeln!(o, "   struct TA_{n}_Stream *sp;");
    let _ = writeln!(o, "   int k, cp, lookbackTotal, subStart;");
    let _ = writeln!(o, "   double cpReal;");
    let _ = writeln!(o, "   TA_RetCode retCode;");
    let _ = writeln!(o, "\n   if( !stream ) return TA_BAD_PARAM;");
    let _ = writeln!(o, "   *stream = NULL;");
    let null_checks: Vec<String> = inputs
        .iter()
        .cloned()
        .chain(std::iter::once(out.clone()))
        .map(|x| format!("!{x}"))
        .collect();
    let _ = writeln!(o, "   if( {} ) return TA_BAD_PARAM;", null_checks.join(" || "));
    let _ = writeln!(o, "   if( historyLen < 1 ) return TA_BAD_PARAM;");
    // The fill covers bars 0..historyLen-1, so its last bar is an index like any
    // other and TA_MAX_INDEX bounds it too (#180). Without this the streaming
    // entry points would compute over exactly the ranges the batch call refuses,
    // and the two are required to agree bit for bit.
    let _ = writeln!(
        o,
        "   if( historyLen > TA_MAX_INDEX + 1 ) return TA_OUT_OF_RANGE_END_INDEX;"
    );
    o.push_str(&emit_opt_param_validation(func, "TA_BAD_PARAM", enums));
    // MAVP's own guard: an inverted [min,max] window is invalid (batch rejects).
    let _ = writeln!(o, "   if( {min} > {max} ) return TA_BAD_PARAM;");
    // Seed EVERY sub-MA at the SHARED max-period lookback, exactly as the batch
    // does: it clamps startIdx up to lookback(maxPeriod) and calls the callee
    // with that same start for every period. Seeding each sub at its OWN (smaller)
    // lookback would seed the recurrence from a different bar and diverge for
    // every period < maxPeriod (order-1 for recursive MAs, running-sum residue
    // for stable ones). This is the OpenInternal start-anchor seam (MACDEXT).
    let _ = writeln!(
        o,
        "   lookbackTotal = {pre}_Lookback( {max}, {matype} );",
        matype = plan.matype_param
    );
    let _ = writeln!(
        o,
        "   subStart = startIdx < lookbackTotal ? lookbackTotal : startIdx;"
    );

    let _ = writeln!(o, "\n   sp = (struct TA_{n}_Stream *)TA_Malloc( sizeof(*sp) );");
    let _ = writeln!(o, "   if( !sp ) return TA_ALLOC_ERR;");
    let _ = writeln!(o, "   memset( sp, 0, sizeof(*sp) );");
    for p in &func.optional_inputs {
        let _ = writeln!(o, "   sp->{0} = {0};", p.name);
    }
    let _ = writeln!(o, "   sp->nBank = {max} - {min} + 1;");
    let _ = writeln!(
        o,
        "   sp->bank = ({subty} **)TA_Malloc( sizeof({subty} *) * (size_t)sp->nBank );"
    );
    let _ = writeln!(o, "   if( !sp->bank ) {{ TA_Free( sp ); return TA_ALLOC_ERR; }}");
    let _ = writeln!(
        o,
        "   memset( sp->bank, 0, sizeof({subty} *) * (size_t)sp->nBank );"
    );
    let _ = writeln!(
        o,
        "   sp->scratch = (double *)TA_Malloc( sizeof(double) * (size_t)sp->nBank );"
    );
    let _ = writeln!(
        o,
        "   if( !sp->scratch ) {{ TA_Free( sp->bank ); TA_Free( sp ); return TA_ALLOC_ERR; }}"
    );
    // Open one sub-MA per possible period, seeded from the full history.
    let _ = writeln!(o, "\n   for( k = 0; k < sp->nBank; k++ )");
    let _ = writeln!(o, "   {{");
    let _ = writeln!(
        o,
        "      retCode = {pre}_OpenInternal( &sp->bank[k], {price}, subStart, historyLen, {open_opts}, &sp->scratch[k] );"
    );
    let _ = writeln!(o, "      if( retCode != TA_SUCCESS )");
    let _ = writeln!(o, "      {{");
    let _ = writeln!(o, "         int j;");
    let _ = writeln!(o, "         for( j = 0; j < k; j++ ) {pre}_Close( sp->bank[j] );");
    let _ = writeln!(
        o,
        "         TA_Free( sp->scratch ); TA_Free( sp->bank ); TA_Free( sp );"
    );
    let _ = writeln!(o, "         return retCode;");
    let _ = writeln!(o, "      }}");
    let _ = writeln!(o, "   }}");
    // Current output: the last history bar's clamped period selects the slot.
    let _ = writeln!(o, "\n   cpReal = {period}[historyLen - 1];");
    let _ = writeln!(o, "   if( !(cpReal >= {min}) ) cp = {min};");
    let _ = writeln!(o, "   else if( cpReal > {max} ) cp = {max};");
    let _ = writeln!(o, "   else cp = (int)cpReal;");
    let _ = writeln!(o, "   *{out} = sp->scratch[cp - {min}];");
    let _ = writeln!(o, "\n   *stream = sp;");
    let _ = writeln!(o, "   return TA_SUCCESS;\n}}\n");
    let _ = registry;
    let _ = helpers;
    let _ = counter;
    emit_open_wrapper(o, func);

    // --- OpenAndFill --------------------------------------------------------
    // No per-bar output array exists to un-discard (the bank yields one selected
    // scalar per bar), so fill genuinely re-runs history: seed the bank at the
    // FIRST output bar (lookbackTotal), emit that bar, then replay Update over
    // the rest, selecting the clamped-period slot each bar. Each sub-MA's
    // (seed-on-prefix + Update) trajectory is bit-exact to its own batch, so the
    // filled array equals batch(0, historyLen-1) by construction.
    let _ = writeln!(o, "{}\n{{", open_and_fill_signature(func));
    let _ = writeln!(o, "   struct TA_{n}_Stream *sp;");
    let _ = writeln!(o, "   int k, cp, lookbackTotal, t;");
    let _ = writeln!(o, "   double cpReal;");
    let _ = writeln!(o, "   TA_RetCode retCode;");
    let _ = writeln!(o, "\n   if( !stream ) return TA_BAD_PARAM;");
    let _ = writeln!(o, "   *stream = NULL;");
    let mut fill_nulls: Vec<String> = inputs
        .iter()
        .cloned()
        .chain(std::iter::once(out.clone()))
        .map(|x| format!("!{x}"))
        .collect();
    fill_nulls.push("!outBegIdx".into());
    fill_nulls.push("!outNBElement".into());
    let _ = writeln!(o, "   if( {} ) return TA_BAD_PARAM;", fill_nulls.join(" || "));
    let _ = writeln!(o, "   if( historyLen < 1 ) return TA_BAD_PARAM;");
    // The fill covers bars 0..historyLen-1, so its last bar is an index like any
    // other and TA_MAX_INDEX bounds it too (#180). Without this the streaming
    // entry points would compute over exactly the ranges the batch call refuses,
    // and the two are required to agree bit for bit.
    let _ = writeln!(
        o,
        "   if( historyLen > TA_MAX_INDEX + 1 ) return TA_OUT_OF_RANGE_END_INDEX;"
    );
    let mut alias: Vec<String> = Vec::new();
    for inp in &inputs {
        alias.push(format!("(const void *){out} == (const void *){inp}"));
    }
    if !alias.is_empty() {
        let _ = writeln!(o, "   if( {} ) return TA_BAD_PARAM;", alias.join(" || "));
    }
    o.push_str(&emit_opt_param_validation(func, "TA_BAD_PARAM", enums));
    let _ = writeln!(o, "   if( {min} > {max} ) return TA_BAD_PARAM;");
    let _ = writeln!(
        o,
        "   lookbackTotal = {pre}_Lookback( {max}, {matype} );",
        matype = plan.matype_param
    );
    let _ = writeln!(o, "   if( historyLen < lookbackTotal + 1 ) return TA_INSUFFICIENT_HISTORY;");
    let _ = writeln!(o, "\n   sp = (struct TA_{n}_Stream *)TA_Malloc( sizeof(*sp) );");
    let _ = writeln!(o, "   if( !sp ) return TA_ALLOC_ERR;");
    let _ = writeln!(o, "   memset( sp, 0, sizeof(*sp) );");
    for p in &func.optional_inputs {
        let _ = writeln!(o, "   sp->{0} = {0};", p.name);
    }
    let _ = writeln!(o, "   sp->nBank = {max} - {min} + 1;");
    let _ = writeln!(
        o,
        "   sp->bank = ({subty} **)TA_Malloc( sizeof({subty} *) * (size_t)sp->nBank );"
    );
    let _ = writeln!(o, "   if( !sp->bank ) {{ TA_Free( sp ); return TA_ALLOC_ERR; }}");
    let _ = writeln!(
        o,
        "   memset( sp->bank, 0, sizeof({subty} *) * (size_t)sp->nBank );"
    );
    let _ = writeln!(
        o,
        "   sp->scratch = (double *)TA_Malloc( sizeof(double) * (size_t)sp->nBank );"
    );
    let _ = writeln!(
        o,
        "   if( !sp->scratch ) {{ TA_Free( sp->bank ); TA_Free( sp ); return TA_ALLOC_ERR; }}"
    );
    // Seed each sub-MA at the first output bar (lookbackTotal), NOT the last.
    let _ = writeln!(o, "\n   for( k = 0; k < sp->nBank; k++ )");
    let _ = writeln!(o, "   {{");
    let _ = writeln!(
        o,
        "      retCode = {pre}_OpenInternal( &sp->bank[k], {price}, lookbackTotal, lookbackTotal + 1, {open_opts}, &sp->scratch[k] );"
    );
    let _ = writeln!(o, "      if( retCode != TA_SUCCESS )");
    let _ = writeln!(o, "      {{");
    let _ = writeln!(o, "         int j;");
    let _ = writeln!(o, "         for( j = 0; j < k; j++ ) {pre}_Close( sp->bank[j] );");
    let _ = writeln!(
        o,
        "         TA_Free( sp->scratch ); TA_Free( sp->bank ); TA_Free( sp );"
    );
    let _ = writeln!(o, "         return retCode;");
    let _ = writeln!(o, "      }}");
    let _ = writeln!(o, "   }}");
    // First output bar (lookbackTotal), then replay the remaining history.
    let _ = writeln!(o, "\n   cpReal = {period}[lookbackTotal];");
    let _ = writeln!(o, "   if( !(cpReal >= {min}) ) cp = {min};");
    let _ = writeln!(o, "   else if( cpReal > {max} ) cp = {max};");
    let _ = writeln!(o, "   else cp = (int)cpReal;");
    let _ = writeln!(o, "   {out}[0] = sp->scratch[cp - {min}];");
    let _ = writeln!(o, "\n   for( t = lookbackTotal + 1; t < historyLen; t++ )");
    let _ = writeln!(o, "   {{");
    let _ = writeln!(o, "      for( k = 0; k < sp->nBank; k++ )");
    let _ = writeln!(o, "         {pre}_Update( sp->bank[k], {price}[t], &sp->scratch[k] );");
    let _ = writeln!(o, "      cpReal = {period}[t];");
    let _ = writeln!(o, "      if( !(cpReal >= {min}) ) cp = {min};");
    let _ = writeln!(o, "      else if( cpReal > {max} ) cp = {max};");
    let _ = writeln!(o, "      else cp = (int)cpReal;");
    let _ = writeln!(o, "      {out}[t - lookbackTotal] = sp->scratch[cp - {min}];");
    let _ = writeln!(o, "   }}");
    let _ = writeln!(o, "\n   *outBegIdx = lookbackTotal;");
    let _ = writeln!(o, "   *outNBElement = historyLen - lookbackTotal;");
    let _ = writeln!(o, "   *stream = sp;");
    let _ = writeln!(o, "   return TA_SUCCESS;\n}}\n");

    // --- Update -------------------------------------------------------------
    let _ = writeln!(o, "{}\n{{", update_signature(func));
    let _ = writeln!(o, "   int k, cp;");
    let _ = writeln!(o, "   double cpReal;");
    let _ = writeln!(o, "   if( !stream || !{out} ) return TA_BAD_PARAM;");
    // inPeriods is checked here too: a non-finite period would reach `(int)`, and
    // the conversion of NaN or an infinity to int is undefined behaviour.
    o.push_str(&finite_bar_check(func, "   ", "TA_BAD_PARAM"));
    let _ = writeln!(o, "   for( k = 0; k < stream->nBank; k++ )");
    let _ = writeln!(o, "      {pre}_Update( stream->bank[k], {price}, &stream->scratch[k] );");
    let _ = writeln!(o, "   cpReal = {period};");
    let _ = writeln!(o, "   if( !(cpReal >= stream->{min}) ) cp = stream->{min};");
    let _ = writeln!(o, "   else if( cpReal > stream->{max} ) cp = stream->{max};");
    let _ = writeln!(o, "   else cp = (int)cpReal;");
    let _ = writeln!(o, "   *{out} = stream->scratch[cp - stream->{min}];");
    let _ = writeln!(o, "   return TA_SUCCESS;\n}}\n");

    // --- Peek ---------------------------------------------------------------
    // Only the SELECTED slot is peeked: the other slots' next values are not the
    // output for this bar, and peeking is non-committing per sub-handle.
    let _ = writeln!(o, "{}\n{{", peek_signature(func));
    let _ = writeln!(o, "   int cp;");
    let _ = writeln!(o, "   double cpReal;");
    let _ = writeln!(o, "   if( !stream || !{out} ) return TA_BAD_PARAM;");
    o.push_str(&finite_bar_check(func, "   ", "TA_BAD_PARAM"));
    let _ = writeln!(o, "   cpReal = {period};");
    let _ = writeln!(o, "   if( !(cpReal >= stream->{min}) ) cp = stream->{min};");
    let _ = writeln!(o, "   else if( cpReal > stream->{max} ) cp = stream->{max};");
    let _ = writeln!(o, "   else cp = (int)cpReal;");
    let _ = writeln!(
        o,
        "   {pre}_Peek( stream->bank[cp - stream->{min}], {price}, {out} );"
    );
    let _ = writeln!(o, "   return TA_SUCCESS;\n}}\n");

    // --- Close --------------------------------------------------------------
    let _ = writeln!(o, "{}\n{{", close_signature(func));
    let _ = writeln!(o, "   int k;");
    let _ = writeln!(o, "   if( stream )");
    let _ = writeln!(o, "   {{");
    let _ = writeln!(o, "      if( stream->bank )");
    let _ = writeln!(o, "      {{");
    let _ = writeln!(o, "         for( k = 0; k < stream->nBank; k++ )");
    let _ = writeln!(o, "            if( stream->bank[k] ) {pre}_Close( stream->bank[k] );");
    let _ = writeln!(o, "         TA_Free( stream->bank );");
    let _ = writeln!(o, "      }}");
    let _ = writeln!(o, "      if( stream->scratch ) TA_Free( stream->scratch );");
    let _ = writeln!(o, "      TA_Free( stream );");
    let _ = writeln!(o, "   }}");
    let _ = writeln!(o, "   return TA_SUCCESS;\n}}\n");
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
        open_body_c.push_str(&render_statement(s, 6, false, enums, registry, helpers, counter, &nullable_out_names(func)));
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
    emit_open_tail(o);
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
            "      if( sp->cbSize_{id} < 1 || sp->cbSize_{id} > historyLen + 1 ) {{ {free_batch}TA_{n}_ReleaseInternal( sp ); return TA_INTERNAL_ERROR; }}"
        );
        for (storage, ty) in circ_storages(circ) {
            let et = if matches!(ty, crate::ir::VarType::Integer) { "int" } else { "double" };
            let _ = writeln!(
                o,
                "      sp->cb_{storage} = ({et} *)TA_Malloc( sizeof({et}) * (size_t)sp->cbSize_{id} );"
            );
            let _ = writeln!(o, "      if( !sp->cb_{storage} ) {{ {free_batch}TA_{n}_ReleaseInternal( sp ); return TA_ALLOC_ERR; }}");
            let _ = writeln!(
                o,
                "      sp->cbMirror_{storage} = ({et} *)TA_Malloc( sizeof({et}) * (size_t)sp->cbSize_{id} );"
            );
            let _ = writeln!(o, "      if( !sp->cbMirror_{storage} ) {{ {free_batch}TA_{n}_ReleaseInternal( sp ); return TA_ALLOC_ERR; }}");
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
/// Scalar returns the last history value per output; Fill has already written
/// the whole array plus `*outBegIdx`/`*outNBElement` in the transcribed body,
/// so it only publishes the handle.
fn emit_open_tail(o: &mut String) {
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

/// A derived ring stores one scalar per bar, so `open` cannot memcpy a raw
/// column into it -- it has to evaluate the expression over the history. The
/// expression is rendered with every array read re-indexed to `idx_var`, the
/// fill loop's counter (#229).
fn derived_fill_expr(
    dr: &streaming::DerivedRing,
    idx_var: &str,
    registry: &Registry,
    helpers: &HelperRegistry,
    counter: &Cell<usize>,
) -> String {
    super::c::render_expression_stream_candles(&streaming::derived_fill_value(dr, idx_var), registry, helpers, counter)
}

/// Emit one ring's per-slot allocation and its open-time fill. Split out of
/// `alloc_and_capture` because the derived case (#229) turned the fill from a
/// single `memcpy` into three shapes and pushed that function past the
/// line limit.
fn emit_ring_slots(
    s: &mut String,
    ring: &streaming::RingSpec,
    v: &str,
    pad: &str,
    fail: &str,
    with_state: bool,
    registry: &Registry,
    helpers: &HelperRegistry,
    counter: &Cell<usize>,
) {
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
            // A derived ring holds f(bar), not a raw column, so both fill
            // shapes evaluate the expression per bar instead of copying.
            let fill_val = ring
                .derived
                .as_ref()
                .map(|dr| derived_fill_expr(dr, "fillJ", registry, helpers, counter));
            if ring.back > 0 {
                let rhs = fill_val.clone().unwrap_or_else(|| format!("{arr}[fillJ]"));
                let _ = writeln!(s, "{pad}  {{ int fillJ;");
                let _ = writeln!(
                    s,
                    "{pad}    for( fillJ = historyLen - sp->ringCap_{v}; fillJ < historyLen; fillJ++ )"
                );
                let _ = writeln!(
                    s,
                    "{pad}       sp->ring_{v}_{arr}[fillJ % sp->ringCap_{v}] = {rhs};"
                );
                let _ = writeln!(s, "{pad}  }}");
            } else if let Some(rhs) = fill_val {
                let _ = writeln!(s, "{pad}  {{ int fillJ;");
                let _ = writeln!(
                    s,
                    "{pad}    for( fillJ = historyLen - sp->ringCap_{v}; fillJ < historyLen; fillJ++ )"
                );
                let _ = writeln!(
                    s,
                    "{pad}       sp->ring_{v}_{arr}[fillJ - (historyLen - sp->ringCap_{v})] = {rhs};"
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
}

/// `sp = TA_Malloc(...); memset; param/extra capture[; state capture]` at the
/// given indent. memset keeps unused fields (identity path) deterministic
/// and NULLs the ring pointers so `ReleaseInternal` is safe mid-allocation.
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
            // The previous-output carry reads the last element just written
            // (outNBElement is set to the output count immediately above). At
            // stride 0 that resolves to slot 0 -- the scalar sink -- so this is
            // one expression for both callers.
            let idx = stride_index("*outNBElement - 1");
            let _ = writeln!(s, "{pad}sp->lastOut_{name} = {name}[{idx}];");
        }
        for (name, ty) in &model.state {
            if model.parity.as_ref().is_some_and(|p| &p.field == name) {
                // Synthetic parity field: SEEDED (not captured from a batch
                // local) to the next bar's parity — the batch replay processed
                // bars 0..historyLen-1, so the next update handles bar
                // historyLen. Flipped each update (see build_transition).
                let _ = writeln!(s, "{pad}sp->{name} = historyLen % 2;");
            } else if matches!(
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
        format!("{{ {pre}TA_{n}_ReleaseInternal( sp ); return TA_ALLOC_ERR; }}")
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
                    "{pad}if( sp->ringLag_{v} < {fwd} || sp->ringCap_{v} > historyLen ) {{ {pre}TA_{n}_ReleaseInternal( sp ); return TA_INTERNAL_ERROR; }}",
                    fwd = ring.fwd
                );
            } else {
                let _ = writeln!(s, "{pad}sp->ringCap_{v} = (int)({} - {v});", model.cursor);
                let _ = writeln!(
                    s,
                    "{pad}if( sp->ringCap_{v} < 0 || sp->ringCap_{v} > historyLen ) {{ {pre}TA_{n}_ReleaseInternal( sp ); return TA_INTERNAL_ERROR; }}"
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
        emit_ring_slots(
            &mut s, ring, v, pad, &fail, with_state, registry, helpers, counter,
        );
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
            "{pad}if( sp->winCap_{v} < 1 || sp->winCap_{v} > historyLen ) {{ {pre}TA_{n}_ReleaseInternal( sp ); return TA_INTERNAL_ERROR; }}"
        );
        for arr in &win.arrays {
            let _ = writeln!(
                s,
                "{pad}sp->win_{v}_{arr} = (double *)TA_Malloc( sizeof(double) * (size_t)sp->winCap_{v} );"
            );
            let _ = writeln!(s, "{pad}if( !sp->win_{v}_{arr} ) {{ {pre}TA_{n}_ReleaseInternal( sp ); return TA_ALLOC_ERR; }}");
            let _ = writeln!(
                s,
                "{pad}sp->winMirror_{v}_{arr} = (double *)TA_Malloc( sizeof(double) * (size_t)sp->winCap_{v} );"
            );
            let _ = writeln!(s, "{pad}if( !sp->winMirror_{v}_{arr} ) {{ {pre}TA_{n}_ReleaseInternal( sp ); return TA_ALLOC_ERR; }}");
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
            "{pad}if( sp->xCap < 1 || sp->xCap > historyLen ) {{ {pre}TA_{n}_ReleaseInternal( sp ); return TA_INTERNAL_ERROR; }}"
        );
        // The slot map is a mask, so the ring is allocated at the next power of
        // two at or above the logical capacity: `idx & xMask` then equals
        // `idx % xPhys`, still injective over any xCap consecutive bars.
        let _ = writeln!(s, "{pad}sp->xPhys = 1;");
        let _ = writeln!(s, "{pad}while( sp->xPhys < sp->xCap ) sp->xPhys <<= 1;");
        let _ = writeln!(s, "{pad}sp->xMask = sp->xPhys - 1;");
        for arr in &ex.arrays {
            let _ = writeln!(
                s,
                "{pad}sp->x_{arr} = (double *)TA_Malloc( sizeof(double) * (size_t)sp->xPhys );"
            );
            let _ = writeln!(s, "{pad}if( !sp->x_{arr} ) {{ {pre}TA_{n}_ReleaseInternal( sp ); return TA_ALLOC_ERR; }}");
            let _ = writeln!(
                s,
                "{pad}sp->xMirror_{arr} = (double *)TA_Malloc( sizeof(double) * (size_t)sp->xPhys );"
            );
            let _ = writeln!(s, "{pad}if( !sp->xMirror_{arr} ) {{ {pre}TA_{n}_ReleaseInternal( sp ); return TA_ALLOC_ERR; }}");
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
                let _ = writeln!(s, "{pad}     sp->x_{arr}[fillJ & sp->xMask] = {arr}[fillJ];");
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
        let lb_call = format!("TA_{n}_Lookback( {} )", lookback_args.join(", "));
        let _ = writeln!(o, "\n   if( {cond} )\n   {{");
        let _ = writeln!(o, "      if( historyLen < {lb_call} + 1 ) return TA_INSUFFICIENT_HISTORY;");
        o.push_str(&alloc_and_capture(
            func, model, "      ", /*with_state=*/ false, "", registry, helpers, counter,
        ));
        // Fill the whole identity range: output j maps to input bar
        // (lookback + j), 0 <= j < historyLen - lookback — batch(0,len-1) for the
        // identity param (a shifted copy; bit-exact by construction).
        //
        // Stride 0 short-circuits to the last bar instead of running the loop.
        // It would be CORRECT to let it run (every iteration rewrites slot 0 and
        // the last one leaves the right value), but the scalar Open would then
        // be O(history) where it is O(1) — a whole-history loop whose only
        // surviving effect is its final store. `outStride` is a literal at both
        // call sites, so this branch folds away in each.
        let _ = writeln!(o, "      {{");
        let _ = writeln!(o, "         int fillLb = {lb_call};");
        let _ = writeln!(o, "         int fillIdx;");
        let _ = writeln!(o, "         *outBegIdx = fillLb;");
        let _ = writeln!(o, "         *outNBElement = historyLen - fillLb;");
        let _ = writeln!(o, "         if( {OUT_STRIDE} )");
        let _ = writeln!(o, "         {{");
        let _ = writeln!(o, "            for( fillIdx = 0; fillIdx < historyLen - fillLb; fillIdx++ )");
        let _ = writeln!(o, "            {{");
        for (out, inp) in &idp.pairs {
            let _ = writeln!(o, "               {out}[fillIdx] = {inp}[fillLb + fillIdx];");
        }
        let _ = writeln!(o, "            }}");
        let _ = writeln!(o, "         }}");
        let _ = writeln!(o, "         else");
        let _ = writeln!(o, "         {{");
        for (out, inp) in &idp.pairs {
            let _ = writeln!(o, "            {out}[0] = {inp}[historyLen - 1];");
        }
        let _ = writeln!(o, "         }}");
        let _ = writeln!(o, "      }}");
        let _ = writeln!(o, "      *stream = sp;");
        let _ = writeln!(o, "      return TA_SUCCESS;");
        let _ = writeln!(o, "   }}");
    }
}

/// Open's argument validation: NULL checks, minimum history, and the same
/// optional-parameter default-substitution/range checks the batch uses.
/// Fill mode additionally requires the batch output triplet non-NULL and, since
/// its state-capture epilogue reads the input tail AFTER writing the output
/// arrays, forbids any output aliasing an input or another output (a hazard the
/// scalar path — which writes only to caller scalars — never has; cf. #108).
fn emit_open_validation(
    o: &mut String,
    func: &FuncDef,
    outputs: &[String],
    inputs: &[String],
    enums: &HashMap<String, EnumDef>,
) {
    let _ = writeln!(o, "\n   if( !stream ) return TA_BAD_PARAM;");
    let _ = writeln!(o, "   *stream = NULL;");
    // A nullable output may be NULL (discarded) — its writes are NULL-guarded,
    // so it is not required.
    let nullable = nullable_out_names(func);
    let null_checks: Vec<String> = inputs
        .iter()
        .map(|i| format!("!{i}"))
        .chain(
            outputs
                .iter()
                .filter(|out| !nullable.contains(*out))
                .map(|out| format!("!{out}")),
        )
        .collect();
    let _ = writeln!(o, "   if( {} ) return TA_BAD_PARAM;", null_checks.join(" || "));
    let _ = writeln!(o, "   if( historyLen < 1 ) return TA_BAD_PARAM;");
    // The fill covers bars 0..historyLen-1, so its last bar is an index like any
    // other and TA_MAX_INDEX bounds it too (#180). Without this the streaming
    // entry points would compute over exactly the ranges the batch call refuses,
    // and the two are required to agree bit for bit.
    let _ = writeln!(
        o,
        "   if( historyLen > TA_MAX_INDEX + 1 ) return TA_OUT_OF_RANGE_END_INDEX;"
    );
    // The out-meta NULL checks and the output-aliasing rejections are FILL-only
    // and live in `emit_open_and_fill_wrapper`: only the fill path writes the
    // caller's arrays, and only it can therefore have an output alias an input
    // or another output (#108/#130). `Open`'s sinks are this call's own stack
    // slots.
    o.push_str(&emit_opt_param_validation(func, "TA_BAD_PARAM", enums));
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

fn emit_circ_hoist(o: &mut String, func: &FuncDef, circs: &[CircState]) {
    for circ in circs {
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
    find(func.stream_source(), id).expect("circbuf prolog present for referenced id")
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
            // Previous-output feedback read (`out[outIdx - 1]`). Scaled like the
            // writes: at stride 1 it reads the array element the previous bar
            // wrote; at stride 0 it reads slot 0, which still holds exactly that
            // value. One expression, no mode.
            Expr::ArrayAccess(nm, idx)
                if fb_outputs.contains(&nm) && crate::streaming::is_prev_output_read(&idx) =>
            {
                Expr::ArrayAccess(nm, Box::new(scale_by_stride(*idx)))
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
            // The per-bar output write, scaled by the stride. At stride 1 this is
            // the batch's own write and the array ends up bit-identical to
            // batch(0, len-1); at stride 0 every bar rewrites slot 0, so the
            // caller's one-element sink ends holding the last history value.
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
                    // Any early success return maps to INSUFFICIENT_HISTORY.
                    // This is not just the no-data guard: a mid-body seed
                    // return (RSI/CMO under Metastock) exits with state the
                    // batch would REWIND and rebuild before continuing, so no
                    // bit-exact continuation exists — the stream honestly
                    // asks for one more bar instead (strict min-history).
                    Some(Expr::Var(v)) if matches!(v.as_str(), "SUCCESS" | "TA_SUCCESS") => {
                        Some(Expr::Var("INSUFFICIENT_HISTORY".into()))
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
    let body = streaming::strip_identity_branch(&body, model.identity.as_ref());
    streaming::rewrite_stmts(&body, &fe, &fs)
}

fn emit_update(o: &mut String, func: &FuncDef, step_ret: bool) {
    let n = uname(func);
    let bars: Vec<String> = streaming::input_array_names(func);
    let outs: Vec<String> = func.outputs.iter().map(|x| x.name.clone()).collect();
    let nullable = nullable_out_names(func);
    let _ = writeln!(o, "{}\n{{", update_signature(func));
    let checks: Vec<String> = std::iter::once("!stream".to_string())
        .chain(
            outs.iter()
                .filter(|x| !nullable.contains(*x))
                .map(|x| format!("!{x}")),
        )
        .collect();
    let _ = writeln!(o, "   if( {} ) return TA_BAD_PARAM;", checks.join(" || "));
    o.push_str(&finite_bar_check(func, "   ", "TA_BAD_PARAM"));
    let args: Vec<String> = bars
        .iter()
        .cloned()
        .chain(outs.iter().cloned())
        .collect();
    // `step_ret` is the composed tier's fallible step (a sub-stream can reject a
    // non-finite intermediate); every other tier's step returns void.
    if step_ret {
        let _ = writeln!(o, "   return TA_{n}_StepInternal( stream, {} );\n}}\n", args.join(", "));
    } else {
        let _ = writeln!(o, "   TA_{n}_StepInternal( stream, {} );", args.join(", "));
        let _ = writeln!(o, "   return TA_SUCCESS;\n}}\n");
    }
}

fn emit_peek_from(o: &mut String, func: &FuncDef, fixups: &str) {
    let n = uname(func);
    let bars: Vec<String> = streaming::input_array_names(func);
    let outs: Vec<String> = func.outputs.iter().map(|x| x.name.clone()).collect();
    let nullable = nullable_out_names(func);
    let _ = writeln!(o, "{}\n{{", peek_signature(func));
    let _ = writeln!(o, "   struct TA_{n}_Stream scratch;");
    let checks: Vec<String> = std::iter::once("!stream".to_string())
        .chain(
            outs.iter()
                .filter(|x| !nullable.contains(*x))
                .map(|x| format!("!{x}")),
        )
        .collect();
    let _ = writeln!(o, "\n   if( {} ) return TA_BAD_PARAM;", checks.join(" || "));
    o.push_str(&finite_bar_check(func, "   ", "TA_BAD_PARAM"));
    let _ = writeln!(o, "   scratch = *stream;");
    o.push_str(fixups);
    let args: Vec<String> = bars
        .iter()
        .cloned()
        .chain(outs.iter().cloned())
        .collect();
    let _ = writeln!(o, "   TA_{n}_StepInternal( &scratch, {} );", args.join(", "));
    let _ = writeln!(o, "   return TA_SUCCESS;\n}}\n");
}

fn emit_peek(o: &mut String, func: &FuncDef, model: &StreamModel) {
    let fixups: String = peek_fixup_groups(model)
        .into_iter()
        .map(|(_, text)| text)
        .collect();
    emit_peek_from(o, func, &fixups);
}

/// Dual-mode Peek: mirror fixups for the UNION of both modes' buffers. A
/// group both modes carry is emitted bare (TRIMA: both arms share the same
/// rings — byte-identical to the single-model Peek); a mode-exclusive group
/// is guarded on its live buffer pointer, which Open's memset leaves NULL
/// under the other mode (an unguarded memcpy would dereference it).
fn emit_peek_dual(o: &mut String, func: &FuncDef, ma: &StreamModel, mb: &StreamModel) {
    let ga = peek_fixup_groups(ma);
    let gb = peek_fixup_groups(mb);
    let a_keys: std::collections::BTreeSet<&String> = ga.iter().map(|(k, _)| k).collect();
    let b_map: std::collections::BTreeMap<&String, &String> =
        gb.iter().map(|(k, t)| (k, t)).collect();
    let mut fixups = String::new();
    let push_guarded = |out: &mut String, guard: &String, text: &String| {
        let _ = writeln!(out, "   if( {guard} )\n   {{");
        for line in text.lines() {
            let _ = writeln!(out, "   {line}");
        }
        let _ = writeln!(out, "   }}");
    };
    for (k, t) in &ga {
        if let Some(bt) = b_map.get(k) {
            assert!(
                *bt == t,
                "{}: dual-mode Peek fixup for shared buffer `{k}` differs across modes",
                func.name
            );
            fixups.push_str(t);
        } else {
            push_guarded(&mut fixups, k, t);
        }
    }
    for (k, t) in &gb {
        if !a_keys.contains(k) {
            push_guarded(&mut fixups, k, t);
        }
    }
    emit_peek_from(o, func, &fixups);
}

/// Rings/windows/circs/extrema: run the step against the handle's
/// pre-allocated scratch mirrors so the live buffers are never touched (the
/// handle is logically const; single-writer covers the mirror — see the
/// proposal). One `(live-buffer expr, fixup text)` group per buffer: the key
/// identifies a buffer across the dual-mode arms and doubles as the NULL
/// guard for a mode-exclusive group.
fn peek_fixup_groups(model: &StreamModel) -> Vec<(String, String)> {
    let mut groups: Vec<(String, String)> = Vec::new();
    for ring in model.rings() {
        let v = &ring.var;
        for arr in &ring.arrays {
            let mut t = String::new();
            let _ = writeln!(t, "   scratch.ring_{v}_{arr} = stream->ringMirror_{v}_{arr};");
            let _ = writeln!(
                t,
                "   memcpy( scratch.ring_{v}_{arr}, stream->ring_{v}_{arr}, sizeof(double) * (size_t)(stream->ringCap_{v} > 0 ? stream->ringCap_{v} : 1) );"
            );
            groups.push((format!("stream->ring_{v}_{arr}"), t));
        }
    }
    for win in model.windows() {
        let v = &win.var;
        for arr in &win.arrays {
            let mut t = String::new();
            let _ = writeln!(t, "   scratch.win_{v}_{arr} = stream->winMirror_{v}_{arr};");
            let _ = writeln!(
                t,
                "   memcpy( scratch.win_{v}_{arr}, stream->win_{v}_{arr}, sizeof(double) * (size_t)stream->winCap_{v} );"
            );
            groups.push((format!("stream->win_{v}_{arr}"), t));
        }
    }
    for circ in model.circs() {
        let id = &circ.id;
        for (storage, ty) in circ_storages(circ) {
            let et = if matches!(ty, crate::ir::VarType::Integer) { "int" } else { "double" };
            let mut t = String::new();
            let _ = writeln!(t, "   scratch.cb_{storage} = stream->cbMirror_{storage};");
            let _ = writeln!(
                t,
                "   memcpy( scratch.cb_{storage}, stream->cb_{storage}, sizeof({et}) * (size_t)stream->cbSize_{id} );"
            );
            groups.push((format!("stream->cb_{storage}"), t));
        }
    }
    if let Some(ex) = model.extrema() {
        for arr in &ex.arrays {
            let mut t = String::new();
            let _ = writeln!(t, "   scratch.x_{arr} = stream->xMirror_{arr};");
            let _ = writeln!(
                t,
                "   memcpy( scratch.x_{arr}, stream->x_{arr}, sizeof(double) * (size_t)stream->xPhys );"
            );
            groups.push((format!("stream->x_{arr}"), t));
        }
    }
    groups
}

fn emit_close_from(o: &mut String, func: &FuncDef, needs_release: bool) {
    let n = uname(func);
    let _ = writeln!(o, "{}\n{{", close_signature(func));
    if needs_release {
        let _ = writeln!(o, "   TA_{n}_ReleaseInternal( stream );");
    } else {
        let _ = writeln!(o, "   if( stream ) TA_Free( stream );");
    }
    let _ = writeln!(o, "   return TA_SUCCESS;\n}}\n");
}

fn emit_close(o: &mut String, func: &FuncDef, model: &StreamModel) {
    emit_close_from(o, func, model.needs_release());
}
