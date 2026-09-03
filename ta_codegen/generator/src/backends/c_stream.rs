//! C emitter for the streaming (incremental) API — docs/streaming-api-proposal.md.
//!
//! For every function whose YAML declares a `streaming:` tier, the generated
//! `src/ta_func/ta_<NAME>.c` gains a stream section after the batch variants:
//!
//! - `struct TA_<NAME>_Stream` — params, carried scalars, lag slots;
//! - `static void TA_<NAME>_StepImpl(...)` — the transition function (the batch
//!   steady-loop body on rewritten IR) that `Update` runs on the live state;
//!   `Peek` inlines the same transition rewritten to commit nothing, against a
//!   `const` binding of the caller's own handle;
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
use crate::ir::{EnumDef, Expr, FuncDef, ParamType, Statement, VarType};
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
    super::common::nullable_output_list(func)
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

/// `Open` and `OpenAndFill` are ONE emission: `TA_<N>_OpenImpl`, whose per-bar
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

/// The merged `<N>_OpenImpl` prototype (no trailing `;`): the union of both public
/// entry points' inputs — history arrays, `startIdx` (a parameter, as
/// `OpenInternal` needs for sub-stream composition), the batch output triplet,
/// and `outStride`. File-static: with two call sites the compiler decides per
/// function whether to share the body or inline it into both wrappers, and when
/// it inlines, `outStride` constant-folds to 0/1 and the arm is exactly what a
/// separate body per entry point would be. Forcing `noinline` measured LARGER.
fn open_core_signature(func: &FuncDef) -> String {
    let n = uname(func);
    let mut history = String::new();
    for a in streaming::input_array_names(func) {
        let _ = write!(history, "const double {a}[], ");
    }
    format!(
        "static TA_RetCode TA_{n}_OpenImpl( struct TA_{n}_Stream **stream, {}int startIdx, int historyLen, {}int *outBegIdx, int *outNBElement, {}, int {OUT_STRIDE} )",
        history,
        opt_params_sig(func),
        out_fill_arrays_sig(func)
    )
}

/// The argument list both wrappers pass to `<N>_OpenImpl`, up to (not including) the
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
/// point onto `<N>_OpenImpl`: it takes an extra `startIdx` — the bar within the
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
/// mutating any state, leaving the handle's accumulators exactly as they were.
fn finite_bar_check(func: &FuncDef, indent: &str, fail: &str, advance: Option<&str>) -> String {
    let bars = streaming::input_array_names(func);
    if bars.is_empty() {
        return String::new();
    }
    let conds: Vec<String> = bars.iter().map(|b| format!("!TA_IS_FINITE( {b} )")).collect();
    reject_on(&conds.join(" || "), indent, fail, advance)
}

/// The rejection a finite-bar check renders: a bare early return, or — pass the
/// handle in `advance` — the same return behind one advance of its produced-bar
/// count.
///
/// **Only rule U3 advances.** A non-finite bar still happened and still occupies
/// a position in the series, so an `Update` counts it and two handles driven off
/// one feed stay positionally aligned when one rejects a bar the other accepts
/// (`docs/error-handling-spec.md` §2.4). Every other streaming rejection — the
/// presence guards, and `UpdateAndFill`'s pre-loop checks — leaves the handle
/// untouched, and `Peek` never advances at all.
fn reject_on(cond: &str, indent: &str, fail: &str, advance: Option<&str>) -> String {
    match advance {
        None => format!("{indent}if( {cond} ) return {fail};\n"),
        Some(handle) => format!(
            "{indent}if( {cond} )\n{indent}{{\n{}{indent}   return {fail};\n{indent}}}\n",
            range_head_advance(&format!("{indent}   "), handle)
        ),
    }
}

/// Rules S1 and S2 — the opener's implied index pair — ahead of every presence
/// check, because an opener is a batch call over `[0, historyLen - 1]` and the
/// pair is B1 and B2 read on that range, answering the same two codes
/// (`docs/error-handling-spec.md` §2.3). Only `!stream` may precede it: the
/// "`*stream` is NULL on any failure" contract is published through that
/// pointer, so it is a precondition for reporting anything at all rather than an
/// argument competing with the pair.
///
/// The ceiling is `TA_MAX_INDEX + 1` because the implied `endIdx` is
/// `historyLen - 1`. Without it the streaming entry points would compute over
/// exactly the ranges the batch call refuses, and the two are required to agree
/// bit for bit (#180).
fn index_pair_guards() -> &'static str {
    concat!(
        "   if( historyLen < 1 ) return TA_OUT_OF_RANGE_START_INDEX;\n",
        "   if( historyLen > TA_MAX_INDEX + 1 ) return TA_OUT_OF_RANGE_END_INDEX;\n"
    )
}

/// Which frame is asking [`presence_guard`] what it must find present.
#[derive(Clone, Copy, PartialEq)]
enum Frame {
    /// `Open`, and `<N>_OpenImpl` behind it. The handle is checked separately,
    /// ahead of the `*stream = NULL` that publishes "no handle on any failure".
    Open,
    /// `OpenAndFill`: the same, plus the range out-parameters it writes through.
    OpenAndFill,
    /// `Update` / `Peek` over a transcribed `<N>_StepImpl`: bars arrive by value,
    /// so there are no input arrays, and a declined output is not required
    /// because the transcription guards its write (`if( outFAMA != NULL )`).
    Step,
    /// `Update` / `Peek` on a hand-rolled tier. Same shape, one clause less: the
    /// dispatch identity arm copies the bar with a bare `*out = in`, so there is
    /// no guard for a declined output to hide behind and EVERY output is
    /// required — declared `nullable` or not.
    StepEveryOutput,
    /// `UpdateAndFill`: bars arrive as arrays.
    StepAndFill,
}

/// Rule S4 / U1 / U2 — everything a C frame must find present, in one order:
/// the handle, the declared inputs, the range out-parameters, then the outputs a
/// caller is required to supply. It is the batch prologue's order too, so both
/// tiers state the same contract the same way.
///
/// One producer, because it is one decision and every frame makes it. Hand-rolled
/// it had already split three ways — the merged tier put the out-meta first, the
/// two exempt tiers put them last, and the dispatch tier dropped the clause
/// below. Nothing could see any of it: they all answer `TA_BAD_PARAM`.
///
/// **A `nullable` output is in the list only where nothing guards its write.**
/// Declining one is legal (Appendix F), and the transcribed bodies honour that
/// with an `if( out != NULL )` — so the opener and the transcribed step leave it
/// out, and only [`Frame::StepEveryOutput`] keeps it. Hand-rolling had both
/// halves wrong in the same tier: the dispatch OPENER dropped the clause it
/// needs (it would have required what its own batch call accepts), while the
/// dispatch STEP relied on it without the guard that earns it. Latent either
/// way, because `MA`'s single output is not declinable — which is exactly why
/// one producer is worth having.
fn required_args(func: &FuncDef, frame: Frame) -> Vec<String> {
    let nullable = nullable_out_names(func);
    let mut names: Vec<String> = Vec::new();
    if matches!(frame, Frame::Step | Frame::StepEveryOutput | Frame::StepAndFill) {
        names.push("stream".to_string());
    }
    if matches!(frame, Frame::Open | Frame::OpenAndFill | Frame::StepAndFill) {
        names.extend(streaming::input_array_names(func));
    }
    if frame == Frame::OpenAndFill {
        names.push("outBegIdx".to_string());
        names.push("outNBElement".to_string());
    }
    names.extend(
        func.outputs
            .iter()
            .map(|o| o.name.clone())
            .filter(|o| frame == Frame::StepEveryOutput || !nullable.contains(o)),
    );
    names
}

/// [`required_args`] as the single rejection every C frame emits for it.
///
/// `<N>_OpenImpl` makes the same test, so the public frames read like duplicates.
/// They are not: `Open` delegates through `OpenInternal`, which hands the core a
/// private `sink_outReal` and copies it out afterwards, so the core's `!outReal`
/// never sees the CALLER's pointer — without the check here,
/// `TA_<N>_Open( &s, data, n, p, NULL )` runs to completion and then dereferences
/// NULL on the copy-out. `OpenAndFill` does pass the caller's arrays straight
/// down, so there the repeat is real, and cheaper than teaching the frame which
/// of its arguments the callee will see.
/// One alias comparison, with the NULL guard a declinable operand needs.
///
/// A declined output aliases nothing, and two of them would otherwise compare
/// equal — `NULL == NULL` — rejecting a legal call. The batch emitter guards the
/// nullable operand for exactly this reason (rule B6a).
fn alias_term(func: &FuncDef, a: &str, b: &str) -> String {
    let nullable = super::common::nullable_output_names(func);
    let term = format!("(const void *){a} == (const void *){b}");
    match (nullable.contains(a), nullable.contains(b)) {
        (false, false) => term,
        (true, false) => format!("({a} != NULL && {term})"),
        (false, true) => format!("({b} != NULL && {term})"),
        (true, true) => format!("({a} != NULL && {b} != NULL && {term})"),
    }
}

fn presence_guard(func: &FuncDef, frame: Frame) -> String {
    let nulls: Vec<String> = required_args(func, frame)
        .into_iter()
        .map(|a| format!("!{a}"))
        .collect();
    if nulls.is_empty() {
        return String::new();
    }
    format!("   if( {} ) return TA_BAD_PARAM;\n", nulls.join(" || "))
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
    // OpenImpl, which normally does it, is not reached.
    let _ = writeln!(o, "   if( !stream ) return TA_BAD_PARAM;");
    let _ = writeln!(o, "   *stream = NULL;");
    o.push_str(index_pair_guards());
    o.push_str(&presence_guard(func, Frame::Open));
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
        "   retCode = TA_{n}_OpenImpl( {}&dummyBegIdx, &dummyNBElement, {}, 0 );",
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
/// Those are NOT stylistic, but the reason is not the obvious one: the fill's
/// writes stop where the capture's warm-up seeds begin, or overlap them by the
/// single slot the next `Update` rewrites first, so in-place computes the right
/// answer today. It is forbidden because that margin is an accident of each
/// body's arithmetic that nothing asserts. `Open` writes only to its own stack
/// slots and never has the hazard, so making it pay the check would be pure cost.
fn emit_open_and_fill_wrapper(o: &mut String, func: &FuncDef) {
    let n = uname(func);
    let inputs = streaming::input_array_names(func);
    let outs: Vec<String> = func.outputs.iter().map(|x| x.name.clone()).collect();
    let _ = writeln!(o, "{}\n{{", open_and_fill_signature(func));
    let _ = writeln!(o, "   if( !stream ) return TA_BAD_PARAM;");
    let _ = writeln!(o, "   *stream = NULL;");
    o.push_str(index_pair_guards());
    o.push_str(&presence_guard(func, Frame::OpenAndFill));
    // Cast to `const void *` so the comparison is well-typed for any output
    // element type (an integer output vs double inputs would otherwise warn
    // "comparison of distinct pointer types lacks a cast").
    let mut alias: Vec<String> = Vec::new();
    for out in &outs {
        for inp in &inputs {
            alias.push(alias_term(func, out, inp));
        }
    }
    for (i, a) in outs.iter().enumerate() {
        for b in &outs[i + 1..] {
            alias.push(alias_term(func, a, b));
        }
    }
    if !alias.is_empty() {
        let _ = writeln!(o, "   if( {} ) return TA_BAD_PARAM;", alias.join(" || "));
    }
    // Straight to the anchored seam at 0, not to `_OpenImpl`, so the seam has a
    // caller for every function instead of only the sixteen something composes
    // over. The guard above is the difference between the two frames.
    let _ = writeln!(
        o,
        "   return TA_{n}_OpenAndFillInternal( {}outBegIdx, outNBElement, {} );",
        open_core_call_head(func, "0"),
        outs.join(", ")
    );
    let _ = writeln!(o, "}}\n");
}

/// `OpenAndFillInternal` for every tier that owns an `<N>_OpenImpl`: the same single
/// pass as the public `OpenAndFill`, at the caller's `startIdx`. See
/// [`open_and_fill_internal_signature`] for why it carries no aliasing guard.
fn emit_open_and_fill_internal_wrapper(o: &mut String, func: &FuncDef) {
    let n = uname(func);
    let outs: Vec<String> = func.outputs.iter().map(|x| x.name.clone()).collect();
    let _ = writeln!(o, "/* Private function, not in public API. */\n{}\n{{", open_and_fill_internal_signature(func));
    let _ = writeln!(
        o,
        "   return TA_{n}_OpenImpl( {}outBegIdx, outNBElement, {}, 1 );",
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

/// Public `UpdateAndFill` prototype (no trailing `;`): `barCount` closed bars
/// in, `barCount` values out, in one call (issue #246).
///
/// The shape is `Update`'s handle head with every scalar bar widened to an
/// array, `barCount` where `Open` puts `historyLen`, and the output tail from
/// `OpenAndFill` — one caller-owned array per output. There is no
/// `outBegIdx`/`outNBElement` pair: the range rides on the handle since #241,
/// so `TA_StreamOutRange` answers afterwards and answers it for a partial
/// commit too.
pub fn update_and_fill_signature(func: &FuncDef) -> String {
    let n = uname(func);
    let mut bars = String::new();
    for a in streaming::input_array_names(func) {
        let _ = write!(bars, "const double {a}[], ");
    }
    format!(
        "TA_LIB_API TA_RetCode TA_{n}_UpdateAndFill( TA_{n}_Stream *stream, {bars}int barCount, {} )",
        out_fill_arrays_sig(func)
    )
}

/// The guard prologue every `UpdateAndFill` shares, whatever its tier: NULL
/// arguments, a negative count, and the aliasing rejection.
///
/// The aliasing rule is `OpenAndFill`'s (S7) — but for its own reason, not for
/// symmetry. The loop writes output `i` and then reads input `i+1`, so an output
/// overlapping an input at a NON-ZERO offset feeds the next bar a value the
/// previous bar just wrote. Exact equality happens to be safe here (the step
/// takes bar `i` by value, so output `i` is written after every input `i` has
/// been read) — and is rejected anyway, because it is the only case C can see
/// and admitting it would advertise a guarantee whose immediate neighbourhood is
/// silent corruption. Java's reference equality and C#'s `Span.Overlaps` reject
/// the same call, so all four backends agree on everything each of them can
/// detect; a partial overlap in C stays rule N8, unspecified.
fn update_and_fill_guards(func: &FuncDef) -> String {
    let inputs = streaming::input_array_names(func);
    let outs: Vec<String> = func.outputs.iter().map(|x| x.name.clone()).collect();
    let mut o = String::new();
    o.push_str(&presence_guard(func, Frame::StepAndFill));
    // A zero count is a success no-op: a caller catching up over a gap should not
    // have to special-case an empty gap.
    let _ = writeln!(o, "   if( barCount < 0 ) return TA_BAD_PARAM;");
    let mut alias: Vec<String> = Vec::new();
    for out in &outs {
        for inp in &inputs {
            alias.push(alias_term(func, out, inp));
        }
    }
    for (i, a) in outs.iter().enumerate() {
        for b in &outs[i + 1..] {
            alias.push(alias_term(func, a, b));
        }
    }
    if !alias.is_empty() {
        let _ = writeln!(o, "   if( {} ) return TA_BAD_PARAM;", alias.join(" || "));
    }
    o
}

/// The per-bar output arguments inside an `UpdateAndFill` loop: `&outReal[i]`,
/// or `NULL` for a nullable output the caller declined — `&outFAMA[i]` on a NULL
/// `outFAMA` would be pointer arithmetic on NULL, which is undefined even
/// unread.
fn indexed_out_args(func: &FuncDef, idx: &str) -> Vec<String> {
    let nullable = nullable_out_names(func);
    func.outputs
        .iter()
        .map(|out| {
            let name = &out.name;
            if nullable.contains(name) {
                format!("{name} ? &{name}[{idx}] : NULL")
            } else {
                format!("&{name}[{idx}]")
            }
        })
        .collect()
}

/// The per-bar finite rejection inside an `UpdateAndFill` loop — the same test
/// `Update` makes, on `<array>[i]` instead of the scalar parameter.
///
/// Re-emitted rather than reached by calling `TA_<N>_Update` per bar: the check
/// lives in the public entry point, not in `<N>_StepImpl`, so routing through it
/// would buy the check at the price of a cross-TU call per bar — the one cost
/// this entry point exists to remove. It is a per-bar test in the loop, NOT a
/// pre-scan: the rejected bar is not committed, every bar before it is, and the
/// handle's count reaches the rejected bar — it is the last one counted, which
/// is how the caller locates where the loop stopped.
fn finite_bar_check_indexed(
    func: &FuncDef,
    indent: &str,
    idx: &str,
    fail: &str,
    advance: Option<&str>,
) -> String {
    let bars = streaming::input_array_names(func);
    if bars.is_empty() {
        return String::new();
    }
    let conds: Vec<String> = bars
        .iter()
        .map(|b| format!("!TA_IS_FINITE( {b}[{idx}] )"))
        .collect();
    reject_on(&conds.join(" || "), indent, fail, advance)
}

/// Public `Close` prototype (no trailing `;`).
pub fn close_signature(func: &FuncDef) -> String {
    let n = uname(func);
    format!("TA_LIB_API TA_RetCode TA_{n}_Close( TA_{n}_Stream *stream )")
}

/// Public `Clone` prototype (no trailing `;`). Handle leads, out-handle after —
/// the `Update`/`Peek` order; `Open` leads with its out-handle only because it
/// has no in-handle to lead with.
pub fn clone_signature(func: &FuncDef) -> String {
    let n = uname(func);
    format!(
        "TA_LIB_API TA_RetCode TA_{n}_Clone( const TA_{n}_Stream *stream, TA_{n}_Stream **clone )"
    )
}

/// The owned-heap inventory of one model as (disown, duplicate) line pairs.
///
/// Mirrors [`release_free_lines`] field for field, and that is the invariant:
/// a buffer freed there and not duplicated here is a fork that shares state
/// with its original. The disown half runs before the first allocation so a
/// mid-way failure frees the COPY's buffers and never the source's.
fn clone_buffer_lines(model: &StreamModel, n: &str) -> (Vec<String>, Vec<String>) {
    let mut disown: Vec<String> = Vec::new();
    let mut dup: Vec<String> = Vec::new();
    let one = |field: String, count: String, ty: &str, disown: &mut Vec<String>, dup: &mut Vec<String>| {
        disown.push(format!("   sp->{field} = NULL;"));
        // NULL-guarded, exactly as `release_free_lines` guards every free and
        // for the same reason: a buffer the active mode never allocated is NULL
        // (the dual-mode union carries both arms' fields, and a cap can be 0),
        // and a NULL source duplicates as NULL rather than as a read of nothing.
        dup.push(format!(
            "   if( stream->{field} )\n   {{ size_t copyN = (size_t)({count});\n     \
             sp->{field} = ({ty} *)TA_Malloc( sizeof({ty}) * copyN );\n     \
             if( !sp->{field} ) {{ TA_{n}_Close( sp ); return TA_ALLOC_ERR; }}\n     \
             memcpy( sp->{field}, stream->{field}, sizeof({ty}) * copyN ); }}"
        ));
    };
    for ring in model.rings() {
        let v = &ring.var;
        for arr in &ring.arrays {
            one(
                format!("ring_{v}_{arr}"),
                format!("sp->ringCap_{v} > 0 ? sp->ringCap_{v} : 1"),
                "double",
                &mut disown,
                &mut dup,
            );
        }
    }
    for win in model.windows() {
        let v = &win.var;
        for arr in &win.arrays {
            one(format!("win_{v}_{arr}"), format!("sp->winCap_{v}"), "double", &mut disown, &mut dup);
        }
    }
    for circ in model.circs() {
        let id = &circ.id;
        for (storage, ty) in circ_storages(circ) {
            let et = if matches!(ty, crate::ir::VarType::Integer) { "int" } else { "double" };
            one(format!("cb_{storage}"), format!("sp->cbSize_{id}"), et, &mut disown, &mut dup);
        }
    }
    if let Some(ex) = model.extrema() {
        for arr in &ex.arrays {
            one(format!("x_{arr}"), "sp->xPhys".to_string(), "double", &mut disown, &mut dup);
        }
    }
    (disown, dup)
}

/// `TA_<N>_Clone`: an independent fork of a live handle, at the same bar.
///
/// The mirror of `TA_<N>_Close`, NOT of `TA_<N>_ReleaseImpl` — Release is only
/// the leaf-buffer traversal and two tiers do not even have one, so Close is
/// the total inventory of what a handle owns and therefore of what a fork has
/// to duplicate. `*sp = *stream` carries the scalars and the fixed arrays; every
/// pointer it also carried is disowned before the first allocation, so the
/// failure path can hand the half-built copy to `Close` without touching the
/// source's buffers.
fn emit_clone(
    o: &mut String,
    func: &FuncDef,
    plan: &StreamPlan,
    enums: &HashMap<String, EnumDef>,
) {
    let n = uname(func);
    let (disown, dup) = clone_owned_lines(plan, &n, enums);
    let _ = writeln!(o, "{}\n{{", clone_signature(func));
    let _ = writeln!(o, "   struct TA_{n}_Stream *sp;\n");
    let _ = writeln!(o, "   if( !clone ) return TA_BAD_PARAM;");
    let _ = writeln!(o, "   *clone = NULL;");
    let _ = writeln!(o, "   if( !stream ) return TA_BAD_PARAM;");
    let _ = writeln!(o, "   sp = (struct TA_{n}_Stream *)TA_Malloc( sizeof(*sp) );");
    let _ = writeln!(o, "   if( !sp ) return TA_ALLOC_ERR;");
    let _ = writeln!(o, "   *sp = *stream;");
    for line in disown.iter().chain(dup.iter()) {
        let _ = writeln!(o, "{line}");
    }
    let _ = writeln!(o, "   *clone = sp;");
    let _ = writeln!(o, "   return TA_SUCCESS;\n}}\n");
}

/// Everything a handle of this tier owns, as (disown, duplicate) line lists.
fn clone_owned_lines(
    plan: &StreamPlan,
    n: &str,
    enums: &HashMap<String, EnumDef>,
) -> (Vec<String>, Vec<String>) {
    let mut disown: Vec<String> = Vec::new();
    let mut dup: Vec<String> = Vec::new();

    let add_model = |m: &StreamModel, disown: &mut Vec<String>, dup: &mut Vec<String>| {
        let (d, c) = clone_buffer_lines(m, n);
        disown.extend(d);
        dup.extend(c);
    };

    match plan {
        StreamPlan::Loop(model) => add_model(model, &mut disown, &mut dup),
        StreamPlan::DualMode(dmp) => {
            // One handle, the union of both arms' buffers — the inactive arm's
            // pointers are NULL, and a NULL source duplicates as NULL.
            let (d, c) = clone_buffer_lines(&dmp.mode_a, n);
            disown.extend(d);
            dup.extend(c);
            let seen: std::collections::BTreeSet<String> = disown.iter().cloned().collect();
            let (d2, c2) = clone_buffer_lines(&dmp.mode_b, n);
            for (i, line) in d2.iter().enumerate() {
                if !seen.contains(line) {
                    disown.push(line.clone());
                    dup.push(c2[i].clone());
                }
            }
        }
        StreamPlan::Composed(cp) => {
            if let Some(m) = &cp.producer {
                add_model(m, &mut disown, &mut dup);
            }
            for ring in &cp.sub_lag_rings {
                let sr = &ring.series;
                disown.push(format!("   sp->lagRing_{sr} = NULL;"));
                dup.push(format!(
                    "   if( stream->lagRing_{sr} )\n   {{ size_t copyN = (size_t)sp->lagRingCap_{sr};\n     \
                     sp->lagRing_{sr} = (double *)TA_Malloc( sizeof(double) * copyN );\n     \
                     if( !sp->lagRing_{sr} ) {{ TA_{n}_Close( sp ); return TA_ALLOC_ERR; }}\n     \
                     memcpy( sp->lagRing_{sr}, stream->lagRing_{sr}, sizeof(double) * copyN ); }}"
                ));
            }
            for (i, sub) in cp.subs.iter().enumerate() {
                let pre = callee_prefix(&sub.callee);
                disown.push(format!("   sp->sub{i} = NULL;"));
                dup.push(format!(
                    "   if( stream->sub{i} )\n   {{ TA_RetCode subRc = {pre}_Clone( stream->sub{i}, &sp->sub{i} );\n     \
                     if( subRc != TA_SUCCESS ) {{ TA_{n}_Close( sp ); return subRc; }} }}"
                ));
            }
        }
        StreamPlan::Dispatch(dp) => {
            disown.push("   sp->sub = NULL;".to_string());
            let mut sw = String::new();
            let _ = writeln!(sw, "   if( stream->sub )\n   {{");
            let _ = writeln!(sw, "      TA_RetCode subRc;");
            let _ = writeln!(sw, "      switch( stream->{} )\n      {{", dp.param);
            for arm in dp.arms.iter().filter(|a| a.supported) {
                let pre = callee_prefix(&arm.callee);
                let _ = writeln!(sw, "      case {}:", render_c_switch_label(&arm.label, enums));
                // Through a typed local, not `({pre}_Stream **)&sp->sub`:
                // writing a typed pointer through a cast `void **` is a
                // representation the standard does not promise, and the local
                // costs nothing.
                let _ = writeln!(sw, "         {{");
                let _ = writeln!(sw, "            {pre}_Stream *subClone = NULL;");
                let _ = writeln!(
                    sw,
                    "            subRc = {pre}_Clone( (const {pre}_Stream *)stream->sub, &subClone );"
                );
                let _ = writeln!(sw, "            sp->sub = subClone;");
                let _ = writeln!(sw, "         }}");
                let _ = writeln!(sw, "         break;");
            }
            let _ = writeln!(sw, "      default:");
            let _ = writeln!(sw, "         subRc = TA_SUCCESS; /* identity arm: no sub-stream */");
            let _ = writeln!(sw, "         break;");
            let _ = writeln!(sw, "      }}");
            let _ = writeln!(sw, "      if( subRc != TA_SUCCESS ) {{ TA_{n}_Close( sp ); return subRc; }}");
            let _ = write!(sw, "   }}");
            dup.push(sw);
        }
        StreamPlan::PeriodBank(pbp) => {
            let pre = callee_prefix(&pbp.callee);
            disown.push("   sp->bank = NULL;".to_string());
            disown.push("   sp->scratch = NULL;".to_string());
            dup.push(format!(
                "   {{ int k;\n     \
                 sp->bank = (struct {pre}_Stream **)TA_Malloc( sizeof(struct {pre}_Stream *) * (size_t)sp->nBank );\n     \
                 if( !sp->bank ) {{ TA_{n}_Close( sp ); return TA_ALLOC_ERR; }}\n     \
                 for( k = 0; k < sp->nBank; k++ ) sp->bank[k] = NULL;\n     \
                 for( k = 0; k < sp->nBank; k++ )\n     \
                 {{\n        if( !stream->bank[k] ) continue;\n        TA_RetCode subRc = {pre}_Clone( stream->bank[k], &sp->bank[k] );\n        \
                 if( subRc != TA_SUCCESS ) {{ TA_{n}_Close( sp ); return subRc; }}\n     }} }}"
            ));
            dup.push(format!(
                "   if( stream->scratch )\n   {{ size_t copyN = (size_t)sp->nBank;\n     \
                 sp->scratch = (double *)TA_Malloc( sizeof(double) * copyN );\n     \
                 if( !sp->scratch ) {{ TA_{n}_Close( sp ); return TA_ALLOC_ERR; }}\n     \
                 memcpy( sp->scratch, stream->scratch, sizeof(double) * copyN ); }}"
            ));
        }
    }

    (disown, dup)
}

/// Public `Value` prototype (no trailing `;`). Const source, one out-pointer
/// per output — the `Peek`/`Update` out-parameter shape, minus the bar.
pub fn value_signature(func: &FuncDef) -> String {
    let n = uname(func);
    format!(
        "TA_LIB_API TA_RetCode TA_{n}_Value( const TA_{n}_Stream *stream, {} )",
        out_params_sig(func)
    )
}

/// `TA_<N>_Value`: the value(s) at the last bar the stream counted, read back
/// without recomputing. Tier-independent — every tier retains into the same
/// `cur_` fields — so it is emitted once for all five rather than per arm.
///
/// Total on a live handle: an Open that returned `TA_SUCCESS` produced at least
/// one value and seeded these fields, so there is no "before the first update"
/// answer to invent. A DECLINABLE output may be passed NULL here exactly as it
/// may at `Update`.
fn emit_value(o: &mut String, func: &FuncDef) {
    let nullable = nullable_out_names(func);
    let required: Vec<String> = func
        .outputs
        .iter()
        .map(|out| out.name.clone())
        .filter(|name| !nullable.contains(name))
        .collect();
    let _ = writeln!(o, "{}\n{{", value_signature(func));
    let mut guard = String::from("   if( !stream");
    for name in &required {
        let _ = write!(guard, " || !{name}");
    }
    guard.push_str(" ) return TA_BAD_PARAM;");
    let _ = writeln!(o, "{guard}");
    for out in &func.outputs {
        let name = &out.name;
        if nullable.contains(name) {
            let _ = writeln!(o, "   if( {name} != NULL )");
            let _ = writeln!(o, "      *{name} = stream->cur_{name};");
        } else {
            let _ = writeln!(o, "   *{name} = stream->cur_{name};");
        }
    }
    let _ = writeln!(o, "   return TA_SUCCESS;\n}}\n");
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
    // UpdateAndFill: the same relationship to Update that OpenAndFill has to
    // Open, so it is declared unconditionally beside it for the same reason.
    let update_and_fill = format!(
        "\n/*\n * UpdateAndFill: commit barCount closed bars and write the barCount values,\n * in one call — barCount back-to-back TA_{n}_Update calls, including the\n * per-bar rejection. A rejected bar k leaves the bars before it committed and\n * written, itself uncommitted and its output slot untouched; TA_StreamOutRange\n * then reports k+1, the rejected bar being the last one counted. Outputs must\n * not alias the inputs or each other.\n */\n{};\n",
        update_and_fill_signature(func)
    );
    // Clone: an independent fork at the same bar. Declared unconditionally —
    // every tier can duplicate what it owns.
    let clone = format!(
        "\n/*\n * Clone: fork the stream — an independent stream at the same bar, owning its\n * own copy of everything the original owns. Both must be closed. The fork\n * carries the value and the range verbatim.\n */\n{};\n",
        clone_signature(func)
    );
    // Value: declared unconditionally beside the rest — every tier retains the
    // `cur_` fields, so there is no shape that could lack it.
    let value = format!(
        "\n/*\n * Value: the value(s) at the last bar the stream counted — the bar\n * TA_StreamOutRange ends on — without recomputing. Seeded by Open, refreshed by\n * every accepted Update and UpdateAndFill, left alone by Peek.\n */\n{};\n",
        value_signature(func)
    );
    format!(
        "\n/*\n * Streaming API for TA_{n} — incremental per-bar evaluation.\n * See docs/streaming-api-design.md.\n{note} */\ntypedef struct TA_{n}_Stream TA_{n}_Stream;\n\n{};\n\n{};\n\n{};\n\n{};\n{}{}{}{}",
        open_signature(func),
        update_signature(func),
        peek_signature(func),
        close_signature(func),
        open_and_fill,
        update_and_fill,
        value,
        clone
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
    // Same reason, for the same two callers: the internal-error site ids the
    // guards below ask for are keyed by function name. `c::generate` has
    // already named it; this nests inside that one and restores it.
    let _site_scope = crate::internal_error_ids::FuncScope::new(&func.name);
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
            emit_peek_loop(&mut o, func, model, enums, registry, helpers, &counter);
            emit_update_and_fill(&mut o, func, false);
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

    // Tier-independent: every tier retains into the same `cur_` fields, so one
    // call here covers all five and a new tier gets it without being asked.
    emit_value(&mut o, func);
    // Tier-DEPENDENT: what a handle owns differs per tier, so `emit_clone` takes
    // the plan. It is still emitted here rather than per arm so that a tier
    // cannot be added without one.
    emit_clone(&mut o, func, &plan, enums);

    mark_fma_multiversion(&mut o, func);

    o
}

/// FMA runtime CPU dispatch for `Peek` (#337) — the same rule the batch tiers
/// carry, applied to the one streaming tier where it pays.
///
/// `Peek` inlines its own copy of the step, so the fused arithmetic is already
/// inside the function being attributed and `target_clones` gives it a hardware
/// clone. The other four streaming tiers delegate to `static`
/// `_StepImpl`/`_OpenImpl` bodies that usually exceed
/// `--param max-inline-insns-auto` (30 at `-O3`), so the clone is emitted empty
/// and costs bytes for nothing; attributing the static instead would only make
/// it un-inlinable. Peek is 25% of the byte cost of attributing all five, for
/// every measured win and no measured regression.
fn mark_fma_multiversion(o: &mut String, func: &FuncDef) {
    if !fma::EMIT_FMA {
        return;
    }
    let sig = peek_signature(func);
    let Some(start) = o.find(&sig) else {
        return;
    };
    let line = o[..start].rfind('\n').map_or(0, |i| i + 1);
    // Every generated body indents, so `"\n}\n"` closes this definition.
    let Some(end) = o[start..].find("\n}\n").map(|e| start + e + 3) else {
        return;
    };
    if o[line..end].contains("fma(") {
        o.insert_str(line, "TA_FMA_MULTIVERSION\n");
    }
}

/// The `struct TA_<N>_Stream { ... };` text for one streaming function —
/// byte-identical to the block [`generate`] emits, because it calls the same
/// per-tier emitters.
///
/// Two consumers, one emitter: the shipped `.c`, and the C server's
/// state-equivalence comparators, which are generated by reading these field
/// declarations back (`server_gen`). A comparator built from a second,
/// hand-kept field model would drift silently the first time a tier grew a
/// field; this cannot. `backend_suite` pins the identity.
///
/// Needs no `Registry`/`HelperRegistry`: a state field's declaration is
/// derived from the stream model alone, never from a rendered expression.
pub fn state_struct_text(func: &FuncDef, lookup: &dyn streaming::CalleeLookup) -> String {
    let resolved = func.resolved_for(crate::ir::Lang::C);
    let func: &FuncDef = &resolved;
    let plan = streaming::validate_streamable(func, lookup)
        .unwrap_or_else(|e| panic!("streaming gate: {e}"));
    let mut o = String::new();
    match &plan {
        StreamPlan::Loop(model) => emit_state_struct(&mut o, func, model),
        StreamPlan::Dispatch(dp) => emit_dispatch_struct(&mut o, func, dp),
        StreamPlan::Composed(cp) => {
            let extra = composed_extra_fields(cp);
            match &cp.producer {
                Some(model) => emit_state_struct_ex(&mut o, func, model, &extra),
                None => emit_composed_struct_noproducer(&mut o, func, &extra),
            }
        }
        StreamPlan::DualMode(dmp) => emit_dual_state_struct(&mut o, func, &dmp.mode_a, &dmp.mode_b),
        StreamPlan::PeriodBank(pbp) => emit_period_bank_struct(&mut o, func, pbp),
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

/// The composed StepImpl / PeekImpl: the producer transition (when present)
/// writes the intermediate series' scalar, which pipelines through the sub
/// handles; combine maps run per-bar. The peek frame calls each sub's `Peek`,
/// which is why `update` no longer tests a routing flag per sub-call.
#[allow(clippy::too_many_lines, clippy::too_many_arguments)]
fn emit_composed_frame_body(
    decls: &mut String,
    o: &mut String,
    func: &FuncDef,
    cp: &streaming::ComposedPlan,
    inputs: &[String],
    outputs: &[String],
    enums: &HashMap<String, EnumDef>,
    registry: &Registry,
    helpers: &HelperRegistry,
    counter: &Cell<usize>,
    frame: StepFrame,
) {
    for (name, ty) in &cp.map_temps {
        let _ = writeln!(decls, "   {};", c_decl(ty, name));
    }
    let cur_scalars = composed_cur_scalars(cp, inputs, outputs);
    for name in &cur_scalars {
        // Typed by what the scalar STANDS FOR: an output's own element type when
        // the name is one of this function's outputs, `double` for the sub-call
        // intermediates that make up the rest. This is the one site here that
        // would not fail to compile if it stayed `double` — C narrows silently
        // on the `*{out} = cur_{out}` write, so an integer output would come
        // back truncated with nothing pointing at the cause.
        let cur_ty = out_c_type(func, name);
        // Initialized, because a sub-call can now leave one unwritten. Every
        // `cur_*` is filled by the sub-call that produces it, so the initializer
        // is dead on every path where that call succeeds — but `Update`/`Peek`
        // reject a non-finite bar, and the composed step feeds SUB-CALLS a
        // library-computed intermediate (STOCH, STOCHF, STOCHRSI, MACDEXT). If
        // that intermediate ever goes non-finite the sub returns without writing
        // its output, and reading an uninitialized double is undefined behaviour
        // — the read happened, and returned different stack garbage on two
        // identical calls. Rust, Java and C# already zero their equivalents.
        let zero = if cur_ty == "int" { "0" } else { "0.0" };
        let _ = writeln!(decls, "   {cur_ty} cur_{name} = {zero};");
    }

    // The cur-map: bar inputs are the step's scalar parameters; the producer
    // series (when present) is written by the producer transition below.
    let mut cur: std::collections::BTreeMap<String, String> = inputs
        .iter()
        .map(|b| (b.clone(), b.clone()))
        .collect();

    if let Some(model) = &cp.producer {
        let names = ComposedNames {
            series: cp.series.clone().expect("producer plan carries a series"),
        };
        let transition = streaming::build_transition(model, &names)
            .unwrap_or_else(|e| panic!("streaming transition: {e}"));
        let (transition, temps, shadow_decls, locals) = match frame {
            StepFrame::Commit => (transition, model.temps.clone(), String::new(), Vec::new()),
            StepFrame::Peek => {
                let pt = streaming::peek_transition_widest(model, &names, &transition, None)
                    .unwrap_or_else(|e| panic!("{}: {e}", func.name));
                let answered = answer_bare_returns(&pt.body);
                let (locals, pt) = peek_localized(model, &names, pt, &answered);
                let temps = streaming::temps_used(&model.temps, &pt.body);
                (pt.body, temps, peek_shadow_decls(&pt.shadows, &pt.slot_temps, 3), locals)
            }
        };
        // Into `decls`, both of them: the extrema rebase below is a STATEMENT,
        // and a declaration after it would be C99, which this tier's producers
        // would be the only place in the emitted library to need.
        for (name, ty) in &temps {
            let _ = writeln!(decls, "   {};", c_decl(ty, name));
        }
        for (name, ty) in &locals {
            let _ = writeln!(decls, "   {};", c_decl(ty, name));
        }
        decls.push_str(&shadow_decls);
        for (name, ty) in &locals {
            o.push_str(&peek_seed("   ", name, ty, &streaming::NameMap::state(&names, name)));
        }
        emit_extrema_rebase(o, model, frame);
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
                let call = match frame {
                    StepFrame::Commit => {
                        format!("{cpfx}_Update( sp->sub{sub_idx}, {arg_str} )")
                    }
                    StepFrame::Peek => format!(
                        "{cpfx}_Peek( (const {cpfx}_Stream *)sp->sub{sub_idx}, {arg_str} )"
                    ),
                };
                let _ = writeln!(o, "   {{");
                let _ = writeln!(o, "      TA_RetCode subRc = {call};");
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
    // Push the new sub-output value into each lag ring, after every read of the
    // oldest slot in the combine above — which is why the peek frame drops the
    // push outright rather than shadowing it: nothing below can load it back.
    if frame == StepFrame::Commit {
        for ring in &cp.sub_lag_rings {
            let s = &ring.series;
            let _ = writeln!(o, "   sp->lagRing_{s}[sp->lagRingPos_{s}] = cur_{s};");
            let _ = writeln!(
                o,
                "   sp->lagRingPos_{s} = (sp->lagRingPos_{s} + 1) % sp->lagRingCap_{s};"
            );
        }
    }
    for out in outputs {
        let _ = writeln!(o, "   *{out} = {};", cur.get(out).expect("analyzer gated output"));
    }
    let _ = writeln!(o, "   return TA_SUCCESS;");
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
    }
    let has_buffers = cp.producer.as_ref().is_some_and(StreamModel::needs_release);
    if has_buffers {
        let _ = writeln!(o, "   TA_{n}_ReleaseImpl( stream );");
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
    let extra = composed_extra_fields(cp);
    match &cp.producer {
        Some(model) => {
            emit_state_struct_ex(o, func, model, &extra);
            emit_release(o, func, model);
        }
        None => emit_composed_struct_noproducer(o, func, &extra),
    }

    // --- StepImpl -----------------------------------------------------------
    {
        let bars = bar_params_sig(func);
        let outs = out_params_sig(func);
        let _ = writeln!(
            o,
            "/* Private function, not in public API. */\nstatic TA_RetCode TA_{n}_StepImpl( struct TA_{n}_Stream *sp, {bars}{outs} )\n{{"
        );
        let (mut decls, mut body) = (String::new(), String::new());
        emit_composed_frame_body(
            &mut decls, &mut body, func, cp, &inputs, &outputs, enums, registry, helpers, counter,
            StepFrame::Commit,
        );
        o.push_str(&decls);
        if !decls.is_empty() {
            let _ = writeln!(o);
        }
        o.push_str(&body);
        let _ = writeln!(o, "}}\n");
    }

    // --- Open ------------------------------------------------------------------
    emit_composed_open(o, func, cp, &outputs, &cleanup, enums, registry, helpers, counter);
    emit_open_internal_wrapper(o, func);
    emit_open_wrapper(o, func);
    emit_open_and_fill_wrapper(o, func);
    emit_open_and_fill_internal_wrapper(o, func);

    // --- Update / Peek / Close ---------------------------------------------------
    emit_update(o, func, true);
    // Peek: the frame keeps every buffer behind the sub-handle pointers
    // read-only, and routes each sub-call to the callee's own `Peek`.
    {
        let (mut decls, mut body) = (String::new(), String::new());
        emit_composed_frame_body(
            &mut decls, &mut body, func, cp, &inputs, &outputs, enums, registry, helpers, counter,
            StepFrame::Peek,
        );
        emit_peek(o, func, &decls, &body, true);
    }
    emit_update_and_fill(o, func, true);
    emit_composed_close(o, func, cp);
}

/// The trailing struct fields every composed tier carries after the producer's
/// own: one typed handle per sub-stream, and the sub-output lag rings.
fn composed_extra_fields(cp: &streaming::ComposedPlan) -> String {
    let mut extra = String::new();
    for (i, sub) in cp.subs.iter().enumerate() {
        let _ = writeln!(extra, "   {}_Stream *sub{i};", callee_prefix(&sub.callee));
    }
    // Sub-output lag rings (ADXR): a fixed-capacity ring of the last `lag`
    // sub-output values.
    for ring in &cp.sub_lag_rings {
        let s = &ring.series;
        let _ = writeln!(extra, "   int lagRingPos_{s};");
        let _ = writeln!(extra, "   int lagRingCap_{s};");
        let _ = writeln!(extra, "   double *lagRing_{s};");
    }
    extra
}

/// State struct for a loopless composed pipeline (no producer loop): the
/// optional params (referenced by combine maps as `sp-><param>`), plus the
/// peek flag and typed sub handles. Dispatch-style — no ring/window/circ/
/// extrema fields, so no `ReleaseImpl`.
fn emit_composed_struct_noproducer(o: &mut String, func: &FuncDef, extra: &str) {
    let n = uname(func);
    let _ = writeln!(o, "struct TA_{n}_Stream {{");
    emit_range_head_fields(o);
    emit_cur_fields(o, func);
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
            o.push_str(&render_statement(&sf.stmt, 12, false, enums, registry, helpers, counter, &[], false));
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
    let _ = writeln!(o, "{}\n{{", open_core_signature(func));
    let _ = writeln!(o, "   struct TA_{n}_Stream *sp;");
    let _ = writeln!(o, "   int endIdx;");
    let _ = writeln!(o, "   int dummyBegIdx;");
    let _ = writeln!(o, "   int dummyNBElement;");
    let _ = writeln!(o, "   TA_RetCode subRc;");
    let _ = writeln!(o, "   double subOpenDummy;");
    for out in outputs {
        let _ = writeln!(o, "   {} *sc_{out};", out_c_type(func, out));
    }
    for (i, sub) in cp.subs.iter().enumerate() {
        let _ = writeln!(o, "   {}_Stream *sub{i};", callee_prefix(&sub.callee));
    }

    emit_open_validation(o, func, enums);

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
        let ty = out_c_type(func, out);
        if alias_fill {
            let _ = writeln!(o, "   if( {OUT_STRIDE} ) sc_{out} = {out};");
            let _ = writeln!(o, "   else");
            let _ = writeln!(o, "   {{");
            let _ = writeln!(
                o,
                "      sc_{out} = ({ty} *)TA_Malloc( sizeof({ty}) * (size_t)historyLen );"
            );
            let _ = writeln!(o, "      if( !sc_{out} ) {{ {prior}return TA_ALLOC_ERR; }}");
            let _ = writeln!(o, "   }}");
        } else {
            let _ = writeln!(
                o,
                "   sc_{out} = ({ty} *)TA_Malloc( sizeof({ty}) * (size_t)historyLen );"
            );
            let _ = writeln!(o, "   if( !sc_{out} ) {{ {prior}return TA_ALLOC_ERR; }}");
        }
    }

    // --- transcription ---------------------------------------------------------
    let _ = writeln!(o, "\n   {{");
    let (region_stmts, tail_stmts) = build_composed_open_bodies(cp, outputs, cleanup);
    let mut region_c = String::new();
    for s in &region_stmts {
        region_c.push_str(&render_statement(s, 6, false, enums, registry, helpers, counter, &nullable_out_names(func), false));
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
        o.push_str(&render_statement(stmt, 6, false, enums, registry, helpers, counter, &nullable_out_names(func), false));
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
                "      if( {OUT_STRIDE} ) memcpy( {out}, sc_{out}, sizeof({}) * (size_t)dummyNBElement );",
                out_c_type(func, out)
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
    emit_range_head_capture(o, "      ");
    emit_cur_capture(o, "      ", func, true);
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
    o.push_str(index_pair_guards());
    o.push_str(&presence_guard(
        func,
        if mode.fills() { Frame::OpenAndFill } else { Frame::Open },
    ));
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
                alias.push(alias_term(func, outp, inp));
            }
        }
        for (i, a) in outputs.iter().enumerate() {
            for b in &outputs[i + 1..] {
                alias.push(alias_term(func, a, b));
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
        // One anchor for every mode. batch( startIdx, .. ) begins at
        // max(startIdx, lookback), so the two variants that carry a startIdx
        // clamp to it — and then the history check has to be re-made against
        // the CLAMPED anchor, or an anchor past the history publishes a handle
        // whose count is negative (usize underflow in Rust). The public fill
        // has no startIdx parameter to clamp with.
        let _ = writeln!(o, "      int fillLb = {lb_call};");
        if mode != DispatchOpen::Fill {
            let _ = writeln!(o, "      if( startIdx > fillLb ) fillLb = startIdx;");
        }
        let _ = writeln!(
            o,
            "      if( historyLen < fillLb + 1 ) {{ TA_Free( sp ); return TA_INSUFFICIENT_HISTORY; }}"
        );
        if mode.fills() {
            let _ = writeln!(o, "      {{");
            let _ = writeln!(o, "         int fillIdx;");
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
        if mode.fills() {
            emit_range_head_capture(o, "      ");
            // The identity arm hands its input straight back, and this path has
            // no stride variable to index the output with — seed from the same
            // bar the fill above wrote.
            for (out, inp) in &idp.pairs {
                let _ = writeln!(o, "      sp->cur_{out} = {inp}[historyLen - 1];");
            }
        } else {
            // Scalar has no out-param pair to copy, so the range is the anchor
            // resolved above — the same one the fills report.
            let _ = writeln!(o, "      sp->outRangeBegIdx = fillLb;");
            let _ = writeln!(o, "      sp->outRangeCount = historyLen - fillLb;");
            for (out, inp) in &idp.pairs {
                let _ = writeln!(o, "      sp->cur_{out} = {inp}[historyLen - 1];");
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
    if mode.fills() {
        emit_range_head_capture(o, "   ");
        for out in &func.outputs {
            let name = &out.name;
            let _ = writeln!(o, "   sp->cur_{name} = {name}[*outNBElement - 1];");
        }
    } else {
        // The arm's own handle already carries the resolved range, and its
        // struct is private to the callee's translation unit — so read it back
        // through the one public accessor, which is exactly what it is for.
        let _ = writeln!(
            o,
            "   TA_StreamOutRange( sp->sub, &sp->outRangeBegIdx, &sp->outRangeCount );"
        );
        // The value has no generic accessor to read back through — the callee's
        // handle is untyped here — but the scalar open just wrote it into the
        // caller's out-pointer, which is the same value.
        for out in &func.outputs {
            let name = &out.name;
            let _ = writeln!(o, "   sp->cur_{name} = *{name};");
        }
    }
    let _ = writeln!(o, "   *stream = sp;");
    let _ = writeln!(o, "   return TA_SUCCESS;\n}}\n");
}

/// State struct for a dispatch tier: the optional params plus one untyped sub
/// handle, whose concrete type the discriminator param names.
fn emit_dispatch_struct(o: &mut String, func: &FuncDef, dp: &DispatchPlan) {
    let n = uname(func);
    let _ = writeln!(o, "struct TA_{n}_Stream {{");
    emit_range_head_fields(o);
    emit_cur_fields(o, func);
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
    let inputs = streaming::input_array_names(func);
    let outputs: Vec<String> = func.outputs.iter().map(|x| x.name.clone()).collect();
    let bar_args: String = inputs.join(", ");
    let case_of = |label: &str| render_c_switch_label(label, enums);
    // This tier hand-rolls every entry point, and all of them index the caller's
    // outputs unconditionally -- `Update` requires each one non-NULL, and
    // `UpdateAndFill` writes `out[i]` with no guard. The shared emitters instead
    // exempt a NULLABLE output from the check and write it through
    // `out ? &out[i] : NULL`, because `&out[i]` on a NULL `out` is undefined
    // even unread. Nothing in the corpus makes a dispatch output nullable, so
    // rather than emit a guard no call can reach, refuse the combination here:
    // a silently wrong body is what this tier would otherwise ship.
    assert!(
        nullable_out_names(func).is_empty(),
        "{}: the dispatch tier hand-rolls its bodies and indexes every output unguarded; \
         a nullable output needs the `out ? &out[i] : NULL` form the shared emitters use",
        uname(func)
    );

    // --- state struct -------------------------------------------------------
    emit_dispatch_struct(o, func, dp);

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
        // Update returns through one exit so the produced-bar count advances
        // once, for the identity path and every arm alike; Peek commits
        // nothing and keeps returning the arm's answer directly.
        if verb == "Update" {
            let _ = writeln!(o, "   TA_RetCode retCode;\n");
        }
        o.push_str(&presence_guard(func, Frame::StepEveryOutput));
        // Checked here rather than left to the sub-stream's own Update/Peek: the
        // identity arm below never reaches a sub-stream at all, it copies the bar
        // straight to the output.
        let advance = (verb == "Update").then_some("stream");
        o.push_str(&finite_bar_check(func, "   ", "TA_BAD_PARAM", advance));
        if let (Some(cond), Some(idp)) = (&identity_handle_cond, &dp.identity) {
            let _ = writeln!(o, "   if( {cond} )\n   {{");
            for (out, inp) in &idp.pairs {
                let _ = writeln!(o, "      *{out} = {inp};");
            }
            if verb == "Update" {
                emit_cur_retain(o, "      ", "stream", func, None);
                emit_range_head_advance(o, "      ", "stream");
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
            let keep = if verb == "Update" { "retCode =" } else { "return" };
            let _ = writeln!(
                o,
                "      {keep} {cp}_{verb}( ({const_qual}{cp}_Stream *)stream->sub, {bar_args}, {arm_out_args} );"
            );
            if verb == "Update" {
                let _ = writeln!(o, "      break;");
            }
        }
        let _ = writeln!(o, "   default:");
        let _ = writeln!(o, "      /* Unreachable: Open rejects arms without a sub-stream. */");
        let _ = writeln!(
            o,
            "      return TA_INTERNAL_ERROR({});",
            crate::internal_error_ids::site(&format!("dispatch.{verb}"))
        );
        let _ = writeln!(o, "   }}");
        if verb == "Update" {
            let _ = writeln!(o, "   if( retCode != TA_SUCCESS ) return retCode;");
            emit_cur_retain(o, "   ", "stream", func, None);
            emit_range_head_advance(o, "   ", "stream");
            let _ = writeln!(o, "   return TA_SUCCESS;");
        }
        let _ = writeln!(o, "}}\n");
    }

    // --- UpdateAndFill ---------------------------------------------------------
    // The dispatch tier hand-rolls this like it hand-rolls Update, and for the
    // same reason: it has no `<N>_StepImpl` to loop over, only a per-bar arm
    // selection. The identity test is loop-invariant (the handle's params are
    // fixed at Open) so it is hoisted out and gets its own loop; the arm switch
    // is left inside, where it is a perfectly predicted branch, rather than
    // duplicating the loop text once per supported MA type.
    //
    // It does NOT delegate the whole array to the sub's own `UpdateAndFill`,
    // which would amortise one level further. On a rejected bar it would then
    // have to recover how many bars the sub committed in order to advance its
    // own count by the same amount — reading the sub's range back through
    // `TA_StreamOutRange` before and after — where the per-bar form simply
    // stops with `i` bars committed on both handles.
    let _ = writeln!(o, "{}\n{{", update_and_fill_signature(func));
    let _ = writeln!(o, "   TA_RetCode retCode;");
    let _ = writeln!(o, "   int i;\n");
    o.push_str(&update_and_fill_guards(func));
    if let (Some(cond), Some(idp)) = (&identity_handle_cond, &dp.identity) {
        let _ = writeln!(o, "   if( {cond} )\n   {{");
        let _ = writeln!(o, "      for( i = 0; i < barCount; i++ )\n      {{");
        o.push_str(&finite_bar_check_indexed(func, "         ", "i", "TA_BAD_PARAM", Some("stream")));
        for (out, inp) in &idp.pairs {
            let _ = writeln!(o, "         {out}[i] = {inp}[i];");
        }
        emit_cur_retain(o, "         ", "stream", func, Some("i"));
        emit_range_head_advance(o, "         ", "stream");
        let _ = writeln!(o, "      }}");
        let _ = writeln!(o, "      return TA_SUCCESS;");
        let _ = writeln!(o, "   }}");
    }
    let _ = writeln!(o, "   for( i = 0; i < barCount; i++ )\n   {{");
    o.push_str(&finite_bar_check_indexed(func, "      ", "i", "TA_BAD_PARAM", Some("stream")));
    let _ = writeln!(o, "      switch( stream->{} )", dp.param);
    let _ = writeln!(o, "      {{");
    let indexed_bar_args: String = inputs
        .iter()
        .map(|b| format!("{b}[i]"))
        .collect::<Vec<_>>()
        .join(", ");
    for arm in dp.arms.iter().filter(|a| a.supported) {
        let cp = callee_prefix(&arm.callee);
        let arm_out_args = arm
            .out_map
            .iter()
            .map(|slot| match slot {
                streaming::OutSlot::Forward(k) => format!("&{}[i]", outputs[*k]),
                streaming::OutSlot::Discard => "NULL".to_string(),
            })
            .collect::<Vec<_>>()
            .join(", ");
        let _ = writeln!(o, "      case {}:", case_of(&arm.label));
        let _ = writeln!(
            o,
            "         retCode = {cp}_Update( ({cp}_Stream *)stream->sub, {indexed_bar_args}, {arm_out_args} );"
        );
        let _ = writeln!(o, "         break;");
    }
    let _ = writeln!(o, "      default:");
    let _ = writeln!(o, "         /* Unreachable: Open rejects arms without a sub-stream. */");
    let _ = writeln!(
        o,
        "         return TA_INTERNAL_ERROR({});",
        crate::internal_error_ids::site("dispatch.UpdateAndFill")
    );
    let _ = writeln!(o, "      }}");
    let _ = writeln!(o, "      if( retCode != TA_SUCCESS ) return retCode;");
    emit_cur_retain(o, "      ", "stream", func, Some("i"));
    emit_range_head_advance(o, "      ", "stream");
    let _ = writeln!(o, "   }}");
    let _ = writeln!(o, "   return TA_SUCCESS;\n}}\n");

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
    emit_range_head_fields(o);
    emit_cur_fields(o, func);
    for p in &func.optional_inputs {
        let _ = writeln!(o, "   {} {};", opt_param_c_type(&p.param_type), p.name);
    }
    for (name, c_type) in &func.private_extra_params {
        let _ = writeln!(o, "   {c_type} {name};");
    }
    // Union of the two modes' SCALAR state, mode-A order first, dedup by name.
    let mut seen: std::collections::BTreeMap<String, &crate::ir::VarType> =
        std::collections::BTreeMap::new();
    let mut order: Vec<(String, VarType)> = Vec::new();
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
/// branching StepImpl, one predicate-branching OpenInternal (+ public Open
/// wrapper), and Update/Peek/Close reused from the loop tier.
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
    // ReleaseImpl (frees the union of both modes' buffers) for a
    // buffer-carrying dual mode (TRIMA rings, HMA rings + circ); inert for a
    // scalar mode (DI/DM). Emitted before Open, whose malloc-failure paths
    // call it.
    emit_release_dual(o, func, ma, mb);

    // --- Step: one function, mode selected from the stored param ------------
    let bars = bar_params_sig(func);
    let outs = out_params_sig(func);
    let _ = writeln!(
        o,
        "/* Private function, not in public API. */\nstatic void TA_{n}_StepImpl( struct TA_{n}_Stream *sp, {bars}{outs} )\n{{"
    );
    emit_dual_frame_body(o, func, dmp, enums, registry, helpers, counter, StepFrame::Commit);
    let _ = writeln!(o, "}}\n");

    // --- OpenImpl: shared head, then a predicate branch per mode ------------
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
    let _ = writeln!(
        o,
        "\n   return TA_INTERNAL_ERROR({});\n}}\n",
        crate::internal_error_ids::site("dualmode")
    );
    emit_open_internal_wrapper(o, func);
    emit_open_wrapper(o, func);
    emit_open_and_fill_wrapper(o, func);
    emit_open_and_fill_internal_wrapper(o, func);

    // --- Update / Peek / Close (mode-fixed handle; Close releases the union of
    // both modes' buffers) ---------------------------------------------------
    emit_update(o, func, false);
    {
        let mut body = String::new();
        emit_dual_frame_body(&mut body, func, dmp, enums, registry, helpers, counter, StepFrame::Peek);
        emit_peek(o, func, "", &body, false);
    }
    emit_update_and_fill(o, func, false);
    emit_close_from(o, func, ma.needs_release() || mb.needs_release());
}

/// A dual-mode frame's statements: the identity short-circuit, then one arm per
/// mode. Every declaration it needs lives inside an arm's own block, so unlike
/// the single-model frame this emits statements only — which is what lets
/// [`emit_peek`] drop it in below the argument guards.
#[allow(clippy::too_many_arguments)]
fn emit_dual_frame_body(
    o: &mut String,
    func: &FuncDef,
    dmp: &streaming::DualModePlan,
    enums: &HashMap<String, EnumDef>,
    registry: &Registry,
    helpers: &HelperRegistry,
    counter: &Cell<usize>,
    frame: StepFrame,
) {
    let (ma, mb) = (&dmp.mode_a, &dmp.mode_b);
    // Identity (HMA period 1) short-circuits ahead of the predicate, as it does
    // in the batch and in Open: it is a property of the function, not of a mode.
    emit_identity_step_branch(o, ma, enums, registry, helpers, counter, 3, frame);
    let pred_h = render_dual_pred(&dmp.predicate, true, func, registry, helpers, counter);
    let _ = writeln!(o, "   if( {pred_h} )\n   {{");
    for (arm, model) in [(0, ma), (1, mb)] {
        if arm == 1 {
            let _ = writeln!(o, "   }}\n   else\n   {{");
        }
        let (mut decls, mut body) = (String::new(), String::new());
        emit_step_inner(&mut decls, &mut body, model, enums, registry, helpers, counter, 6, false, frame);
        o.push_str(&decls);
        if !decls.is_empty() {
            let _ = writeln!(o);
        }
        o.push_str(&body);
    }
    let _ = writeln!(o, "   }}");
}

/// The two leading members every `struct TA_<N>_Stream` carries: the range of
/// bars the handle has an output for (issue #241).
///
/// First, and in this order, in every tier — `TA_StreamOutRange` reads the pair
/// through a `const void *`, which is what lets ONE public accessor serve all
/// the streams instead of one typed accessor per function. Every tier's struct
/// emitter calls this immediately after opening the brace, so the layout cannot
/// drift between tiers; `c_stream_every_tier_leads_with_the_range_head` pins it.
///
/// Two ints rather than one: `begIdx` is what the opener resolved
/// (`max(startIdx, lookback)`), and neither `startIdx` nor the lookback is
/// otherwise on the handle, so there is nothing to derive it from at accessor
/// time.
fn emit_range_head_fields(o: &mut String) {
    let _ = writeln!(o, "   /* The bars this handle has an output for (see TA_StreamOutRange).");
    let _ = writeln!(o, "    * Kept first, and in this order, in every stream struct. */");
    for decl in RANGE_HEAD_FIELDS {
        let _ = writeln!(o, "   {decl}");
    }
}

/// The `cur_<output>` fields: the value(s) at the last bar the stream counted,
/// which `TA_<N>_Value` hands back without recomputing. One per output, on
/// every tier, emitted beside the range head so the two accessors' storage
/// cannot come apart tier by tier.
///
/// Distinct from `lastOut_<output>` even where both exist (DX): `lastOut_` is
/// the PREVIOUS bar's output, read by the body while computing this one, and
/// it is emitted only for the outputs a body actually reads back.
fn emit_cur_fields(o: &mut String, func: &FuncDef) {
    let _ = writeln!(o, "   /* The value(s) at the last bar the stream counted (see TA_{}_Value). */", uname(func));
    for out in &func.outputs {
        let _ = writeln!(o, "   {} cur_{};", out_c_type(func, &out.name), out.name);
    }
}

/// The C declarations of the range head, in struct order. `TA_StreamRangeHead`
/// (rendered into the private header by `server_gen`) is built from this same
/// list, so the layout the accessor reads through and the layout every stream
/// struct leads with cannot come apart.
pub const RANGE_HEAD_FIELDS: [&str; 2] = ["int outRangeBegIdx;", "int outRangeCount;"];

/// Advance the handle's count by one bar it has an output for (issue #241);
/// the U3 reject path calls this too, not only the committing steps.
/// Saturates at `TA_MAX_INDEX`: past that the stream has left the index domain
/// the batch tier addresses at all, and a signed overflow would be undefined.
fn emit_range_head_advance(o: &mut String, indent: &str, handle: &str) {
    o.push_str(&range_head_advance(indent, handle));
}

/// [`emit_range_head_advance`] as a statement, for the emitters that inline it
/// into a larger one. One spelling of the saturation guard, two shapes.
fn range_head_advance(indent: &str, handle: &str) -> String {
    format!("{indent}if( {handle}->outRangeCount < TA_MAX_INDEX ) {handle}->outRangeCount++;\n")
}

/// Retain the value(s) this committed bar produced, for `TA_<N>_Value`.
///
/// Emitted only where the tier's step has no transition tail to ride on: the
/// composed, dispatch and period-bank steps hand their outputs straight to the
/// caller's pointers. Sits with the range advance because the two describe the
/// same bar — a handle whose count moved but whose value did not is exactly the
/// split `TA_<N>_Value` must never show.
fn emit_cur_retain(o: &mut String, indent: &str, handle: &str, func: &FuncDef, idx: Option<&str>) {
    for out in &func.outputs {
        let n = &out.name;
        match idx {
            None => {
                let _ = writeln!(o, "{indent}{handle}->cur_{n} = *{n};");
            }
            Some(i) => {
                let _ = writeln!(o, "{indent}{handle}->cur_{n} = {n}[{i}];");
            }
        }
    }
}

/// Record on the handle the range this open produced (issue #241): the pair the
/// batch API reports for the same history, which every later `Update` extends.
/// Emitted immediately before the handle is published, where `*outBegIdx` /
/// `*outNBElement` hold their final values on every tier that carries them —
/// the loop tier writes them before the state capture, the composed tier after.
fn emit_range_head_capture(o: &mut String, indent: &str) {
    let _ = writeln!(o, "{indent}sp->outRangeBegIdx = *outBegIdx;");
    let _ = writeln!(o, "{indent}sp->outRangeCount = *outNBElement;");
}

/// Seed the value accessor at the publish point, where every tier's outputs
/// are written and `*outNBElement` is final. Sits with the range capture for
/// the same reason the retain sits with the range advance: the two describe the
/// same bar, and `Open(P)+updates` has to leave the handle where `Open(n)` does.
///
/// Declinable outputs are seeded earlier, from the body variable, because their
/// array may be NULL — `nullable_out_names` is the exemption list.
fn emit_cur_capture(o: &mut String, indent: &str, func: &FuncDef, strided: bool) {
    let nullable = nullable_out_names(func);
    for out in &func.outputs {
        let name = &out.name;
        if nullable.contains(name) {
            continue;
        }
        // The period-bank opener fills at stride 1 and takes no `outStride`
        // parameter, so its index is the bare count.
        let idx = if strided {
            stride_index("*outNBElement - 1")
        } else {
            "*outNBElement - 1".to_string()
        };
        let _ = writeln!(o, "{indent}sp->cur_{name} = {name}[{idx}];");
    }
}

fn emit_state_struct(o: &mut String, func: &FuncDef, model: &StreamModel) {
    emit_state_struct_ex(o, func, model, "");
}

/// State struct with extra trailing fields (composed tier: typed sub handles
/// appended after the producer's own fields).
/// The non-scalar handle fields (out-feedback, lag slots, ring/window/circ/
/// extrema buffers + their Peek mirrors) for one model. Shared by the loop-tier
/// struct and the dual-mode union struct (whose two modes carry identical
/// non-scalar state — TRIMA's odd/even arms share the same rings — so the union
/// emits one model's set).
fn emit_nonscalar_struct_fields(o: &mut String, func: &FuncDef, model: &StreamModel) {
    for (name, ty) in nonscalar_struct_fields(func, model) {
        let _ = writeln!(o, "   {};", c_decl(&ty, &name));
    }
}

/// The struct's non-`model.state` fields, in declaration order, with the type
/// each is declared with.
///
/// One list, two readers: [`emit_nonscalar_struct_fields`] renders it, and the
/// peek localizer types a local against it. A second hand-written type map
/// would be a silent miscompile the first time the two disagreed about an
/// `int`.
fn nonscalar_struct_fields(func: &FuncDef, model: &StreamModel) -> Vec<(String, VarType)> {
    let mut out: Vec<(String, VarType)> = Vec::new();
    let out_ty = |n: &str| {
        if out_c_type(func, n) == "int" { VarType::Integer } else { VarType::Real }
    };
    for name in &model.out_feedback {
        out.push((format!("lastOut_{name}"), out_ty(name)));
    }
    for lag in &model.lags {
        for k in 1..=lag.depth {
            out.push((StreamModel::lag_field(&lag.array, k), VarType::Real));
        }
    }
    for ring in model.rings() {
        let v = &ring.var;
        out.push((format!("ringPos_{v}"), VarType::Integer));
        out.push((format!("ringCap_{v}"), VarType::Integer));
        if ring.back > 0 {
            out.push((format!("ringLag_{v}"), VarType::Integer));
        }
        for arr in &ring.arrays {
            out.push((format!("ring_{v}_{arr}"), VarType::RealPointer));
        }
    }
    for win in model.windows() {
        let v = &win.var;
        out.push((format!("winPos_{v}"), VarType::Integer));
        out.push((format!("winCap_{v}"), VarType::Integer));
        for arr in &win.arrays {
            out.push((format!("win_{v}_{arr}"), VarType::RealPointer));
        }
    }
    for circ in model.circs() {
        out.push((format!("cbSize_{}", circ.id), VarType::Integer));
        for (storage, ty) in circ_storages(circ) {
            let elem = if matches!(ty, VarType::Integer) {
                VarType::IntPointer
            } else {
                VarType::RealPointer
            };
            out.push((format!("cb_{storage}"), elem));
        }
    }
    if let Some(ex) = model.extrema() {
        out.push(("xCap".to_string(), VarType::Integer));
        out.push(("xPhys".to_string(), VarType::Integer));
        out.push(("xMask".to_string(), VarType::Integer));
        for arr in &ex.arrays {
            out.push((format!("x_{arr}"), VarType::RealPointer));
        }
    }
    out
}

fn emit_state_struct_ex(o: &mut String, func: &FuncDef, model: &StreamModel, extra: &str) {
    let n = uname(func);
    let _ = writeln!(o, "struct TA_{n}_Stream {{");
    emit_range_head_fields(o);
    emit_cur_fields(o, func);
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
    let _ = writeln!(o, "}};\n");
}

/// Free-line list for one model's heap buffers (the `ReleaseImpl` body,
/// minus the trailing handle free). Every line is NULL-guarded, so a line
/// whose buffer the active mode never allocated is a no-op — which is what
/// lets the dual-mode union release line-dedup two models' lists.
fn release_free_lines(model: &StreamModel) -> Vec<String> {
    let mut lines: Vec<String> = Vec::new();
    for ring in model.rings() {
        for arr in &ring.arrays {
            lines.push(format!("   if( sp->ring_{0}_{arr} ) TA_Free( sp->ring_{0}_{arr} );", ring.var));
        }
    }
    for win in model.windows() {
        for arr in &win.arrays {
            lines.push(format!("   if( sp->win_{0}_{arr} ) TA_Free( sp->win_{0}_{arr} );", win.var));
        }
    }
    for circ in model.circs() {
        for (storage, _) in circ_storages(circ) {
            lines.push(format!("   if( sp->cb_{storage} ) TA_Free( sp->cb_{storage} );"));
        }
    }
    if let Some(ex) = model.extrema() {
        for arr in &ex.arrays {
            lines.push(format!("   if( sp->x_{arr} ) TA_Free( sp->x_{arr} );"));
        }
    }
    lines
}

fn emit_release_from(o: &mut String, func: &FuncDef, lines: &[String]) {
    let n = uname(func);
    let _ = writeln!(o, "/* Private function, not in public API. */
static void TA_{n}_ReleaseImpl( struct TA_{n}_Stream *sp )
{{");
    let _ = writeln!(o, "   if( !sp ) return;");
    for line in lines {
        let _ = writeln!(o, "{line}");
    }
    let _ = writeln!(o, "   TA_Free( sp );
}}
");
}

/// `static void TA_<N>_ReleaseImpl(...)`: frees every ring buffer and the
/// handle itself. Emitted only for ring models; safe on partially-allocated
/// handles (open memsets the struct, so unallocated buffers are NULL).
fn emit_release(o: &mut String, func: &FuncDef, model: &StreamModel) {
    if !model.needs_release() {
        return;
    }
    emit_release_from(o, func, &release_free_lines(model));
}

/// Dual-mode: one `ReleaseImpl` freeing the UNION of both modes' buffers
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

/// Which of the two transitions a step body renders. `Commit` stores into the
/// handle; `Peek` carries every store in a local — a buffer element in the
/// shadow pair [`streaming::peek_transition`] builds, a state field in a bind of
/// its own name — so that nothing it computes reaches the caller's handle.
#[derive(Clone, Copy, PartialEq, Eq)]
enum StepFrame {
    Commit,
    Peek,
}

/// `TA_<N>_StepImpl` — the committing transition. `Update` and `UpdateAndFill`
/// both call it, which is why it is a function; the peek frame has exactly one
/// caller, forever (a cross-indicator call enters the callee's PUBLIC `Peek`),
/// so it is emitted inline into [`emit_peek`] instead of as a second tier.
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
    let void_sp = model.state.is_empty()
        && func.optional_inputs.is_empty()
        && func.private_extra_params.is_empty()
        && model.lags.is_empty();
    let _ = writeln!(
        o,
        "/* Private function, not in public API. */\nstatic void TA_{n}_StepImpl( struct TA_{n}_Stream *sp, {bars}{outs} )\n{{"
    );
    let (mut decls, mut body) = (String::new(), String::new());
    emit_step_inner(&mut decls, &mut body, model, enums, registry, helpers, counter, 3, void_sp, StepFrame::Commit);
    o.push_str(&decls);
    if !decls.is_empty() {
        let _ = writeln!(o);
    }
    o.push_str(&body);
    let _ = writeln!(o, "}}\n");
}

/// The transition's own early exits are valueless — it is a `void` transition —
/// but a peek frame is emitted straight into `Peek`, which answers a `RetCode`.
/// The only one the corpus carries is the param-degenerate identity
/// short-circuit, which has already written the output when it fires.
fn answer_bare_returns(body: &[Statement]) -> Vec<Statement> {
    streaming::rewrite_stmts(
        body,
        &|e| e,
        &|s| {
            Some(match s {
                Statement::Return { value: None } => Statement::Return {
                    value: Some(Expr::Var("TA_SUCCESS".to_string())),
                },
                other => other,
            })
        },
    )
}

/// The locals a peek frame's shadowed stores live in. `pkSlot*` is seeded with
/// an index no load can produce, so a store that did not run never matches one.
fn peek_shadow_decls(
    shadows: &[streaming::PeekShadow],
    slot_temps: &[String],
    indent: usize,
) -> String {
    let pad = " ".repeat(indent);
    let mut s = String::new();
    for sh in shadows {
        let ty = if sh.int_elem { "int" } else { "double" };
        let zero = if sh.int_elem { "0" } else { "0.0" };
        let _ = writeln!(s, "{pad}int {} = -1;", sh.slot_var);
        let _ = writeln!(s, "{pad}{ty} {} = {zero};", sh.val_var);
    }
    for t in slot_temps {
        let _ = writeln!(s, "{pad}int {t} = 0;");
    }
    s
}

/// State scalars the compiler is FORCED to reload, bound to a local for the
/// length of the step body: loaded once at the top, stored back before every
/// exit.
///
/// `sp->cb_<id>[..]` is a `double *` store and `sp->sum` is a `double` —
/// COMPATIBLE types, so C permits them to alias. A store through a ring,
/// CIRCBUF or window buffer (or through a real output pointer) therefore
/// invalidates every `double` field the compiler was holding, and a later read
/// of one it had already written becomes a store-to-load round trip. That is
/// permitted aliasing, not a strict-aliasing violation, so no flag removes it,
/// and GCC does not exploit `restrict` on a pointer loaded out of a struct
/// (tested — member and local form both emit byte-identical code). The
/// generator knows what the compiler cannot derive: those buffers are separate
/// allocations, disjoint from the handle.
///
/// So the election is exactly `write -> clobber -> read`, not "read and
/// written": a field with no clobber between its write and its read already
/// lives in a register (ATR's `prevATR`, the Hilbert family's odd/even `prev_*`
/// shadows), and hoisting it would only add one load and one store per bar.
///
/// C only. Rust's `&mut`, and a Java/C# `double[]` element against a `double`
/// field, already forbid the alias, so those three backends never emit the
/// reload.
///
/// `double` only: an `int` field IS protected from a `double *` store, by the
/// strict-aliasing rule.
fn elect_step_scalars(model: &StreamModel, transition: &[Statement]) -> Vec<String> {
    let held: std::collections::BTreeMap<String, &str> = model
        .state
        .iter()
        .filter(|(_, t)| matches!(t, crate::ir::VarType::Real))
        .map(|(n, _)| (streaming::NameMap::state(&CNames, n), n.as_str()))
        .collect();
    if held.is_empty() {
        return Vec::new();
    }
    // A dual-mode arm renders inside a nested block, where a local sharing a
    // parameter's name would SHADOW it instead of failing to compile. The bar
    // and output names are the only ones that can reach here.
    let params: std::collections::BTreeSet<&str> = model
        .bar_inputs
        .iter()
        .chain(model.outputs.iter())
        .map(String::as_str)
        .collect();
    let int_outs: std::collections::BTreeSet<&str> = model
        .func
        .outputs
        .iter()
        .filter(|o| o.param_type == crate::ir::ParamType::Integer)
        .map(|o| o.name.as_str())
        .collect();

    let mut st = ReloadScan {
        held: &held,
        int_outs: &int_outs,
        written: std::collections::BTreeSet::new(),
        clobbered: std::collections::BTreeSet::new(),
        elected: std::collections::BTreeSet::new(),
    };
    st.stmts(transition);
    let elected = st.elected;
    model
        .state
        .iter()
        .map(|(n, _)| n.as_str())
        .filter(|n| !params.contains(n) && elected.contains(*n))
        .map(ToString::to_string)
        .collect()
}

/// Walks the transition in source order looking for `write -> clobber -> read`
/// on each carried double. Branch arms start from the state above them and
/// their effects are unioned below, so one arm's write never pairs with the
/// other arm's read.
struct ReloadScan<'a> {
    /// `sp->x` -> `x`, for the carried doubles only.
    held: &'a std::collections::BTreeMap<String, &'a str>,
    int_outs: &'a std::collections::BTreeSet<&'a str>,
    written: std::collections::BTreeSet<String>,
    clobbered: std::collections::BTreeSet<String>,
    elected: std::collections::BTreeSet<String>,
}

impl ReloadScan<'_> {
    /// Every store the compiler must assume can reach a `double` field: any
    /// write through a pointer or array whose element type is not integer.
    fn is_clobber(&self, target: &Expr) -> bool {
        match target {
            Expr::ArrayAccess(n, _) | Expr::PointerDeref(n) => !self.int_outs.contains(n.as_str()),
            _ => false,
        }
    }

    /// A read of a carried double that a clobber has invalidated since its
    /// last write is exactly the forced reload this election exists to remove.
    fn read_name(&mut self, n: &str) {
        if let Some(bare) = self.held.get(n) {
            if self.clobbered.contains(n) {
                self.elected.insert((*bare).to_string());
            }
        }
    }

    fn read(&mut self, e: &Expr) {
        let mut names = std::collections::BTreeSet::new();
        streaming::expr_var_names(e, &mut names);
        for n in names {
            self.read_name(&n);
        }
    }

    fn clobber(&mut self) {
        let w = std::mem::take(&mut self.written);
        self.clobbered.extend(w);
    }

    fn stmts(&mut self, list: &[Statement]) {
        for s in list {
            self.stmt(s);
        }
    }

    fn branches(&mut self, arms: &[&[Statement]]) {
        let entry_w = self.written.clone();
        let entry_c = self.clobbered.clone();
        let mut out_w = std::collections::BTreeSet::new();
        let mut out_c = std::collections::BTreeSet::new();
        for arm in arms {
            self.written.clone_from(&entry_w);
            self.clobbered.clone_from(&entry_c);
            self.stmts(arm);
            out_w.append(&mut self.written);
            out_c.append(&mut self.clobbered);
        }
        self.written = out_w;
        self.clobbered = out_c;
    }

    fn stmt(&mut self, s: &Statement) {
        match s {
            Statement::Assign {
                target: Expr::Var(v),
                value,
                compound,
            } => {
                self.read(value);
                if *compound {
                    // A compound assignment reads its target first.
                    self.read_name(v);
                }
                if self.held.contains_key(v) {
                    self.written.insert(v.clone());
                    self.clobbered.remove(v);
                }
            }
            Statement::Assign { target, value, .. } => {
                self.read(target);
                self.read(value);
                if self.is_clobber(target) {
                    self.clobber();
                }
            }
            Statement::VarDecl { init: Some(e), .. } => self.read(e),
            Statement::Return { value } => {
                if let Some(e) = value {
                    self.read(e);
                }
            }
            Statement::If {
                condition,
                then_body,
                else_body,
                ..
            } => {
                self.read(condition);
                self.branches(&[then_body, else_body]);
            }
            Statement::While { condition, body } | Statement::DoWhile { condition, body } => {
                self.read(condition);
                // A loop body can carry its own write into its next iteration's
                // read, so it is walked in place rather than as an arm.
                self.stmts(body);
                self.read(condition);
            }
            Statement::For { count, body, .. } => {
                self.read(count);
                self.stmts(body);
            }
            Statement::ForC {
                init,
                condition,
                update,
                body,
            } => {
                self.stmt(init);
                self.read(condition);
                self.stmts(body);
                self.stmt(update);
            }
            Statement::Block { body } => self.stmts(body),
            // Trivia and a bare declaration emit no code, so they cannot
            // clobber. They reach the catch-all below otherwise, where a
            // source comment would act as a memory barrier and elect every
            // field written above it (DX and ADX, whose bodies are commented
            // step by step, elected five apiece for nothing).
            Statement::Comment(_)
            | Statement::UnrollHint { .. }
            | Statement::VarDecl { init: None, .. } => {}
            Statement::Switch {
                expr,
                cases,
                default,
            } => {
                self.read(expr);
                let mut arms: Vec<&[Statement]> =
                    cases.iter().map(|(_, b)| b.as_slice()).collect();
                arms.push(default);
                self.branches(&arms);
            }
            // A bare call, or anything else not modelled, may store anywhere.
            other => {
                let mut names = std::collections::BTreeSet::new();
                streaming::stmt_var_names(other, &mut names);
                for n in names {
                    self.read_name(&n);
                }
                self.clobber();
            }
        }
    }
}

/// Rewrite `sp->x` to `x` for the elected scalars, and store each back before
/// every `return` the transition carries -- today only the param-degenerate
/// identity short-circuit, which [`streaming::insert_transition_prologue`]
/// places at index 0, so it can never follow a mutation. The arm is kept for
/// the transition that eventually carries a guarded mid-body return; dropping
/// a write-back is a lost state update, which `stream_verify` compares
/// bit-exactly. A path that did not modify
/// one writes back what it loaded, which is why this needs no per-path
/// analysis.
fn apply_step_scalars(transition: &[Statement], elected: &[String]) -> Vec<Statement> {
    if elected.is_empty() {
        return transition.to_vec();
    }
    let bare: std::collections::BTreeMap<String, String> = elected
        .iter()
        .map(|n| (streaming::NameMap::state(&CNames, n), n.clone()))
        .collect();
    let writeback: Vec<Statement> = elected
        .iter()
        .map(|n| Statement::Assign {
            target: Expr::Var(streaming::NameMap::state(&CNames, n)),
            value: Expr::Var(n.clone()),
            compound: false,
        })
        .collect();
    streaming::rewrite_stmts(
        transition,
        &|e| match e {
            Expr::Var(ref v) => bare.get(v).map_or(e, |b| Expr::Var(b.clone())),
            other => other,
        },
        &|s| match s {
            Statement::Return { .. } => {
                let mut body = writeback.clone();
                body.push(s);
                Some(Statement::Block { body })
            }
            other => Some(other),
        },
    )
}

/// What [`localize_peek_state_writes`] answers: the fields moved to a local, the
/// buffer bases bound beside them, and the rewritten frame.
type PeekLocals = (Vec<String>, Vec<(String, VarType)>, Vec<Statement>);

/// The state fields a peek frame writes, and the frame with every mention of
/// them moved to a bare local.
///
/// This is what lets `sp` point straight at the caller's handle: a frame that
/// stores nothing through it cannot commit, so there is nothing to copy first.
/// The local keeps the field's OWN name, because [`super::fma::stream_base`]
/// strips `sp->` before classifying an operand — any other spelling would
/// silently move an FMA site.
///
/// `None` where a written field's bare name would collide with a bar input or
/// an output: the local would shadow the parameter instead of failing to
/// compile.
fn localize_peek_state_writes(
    func: &FuncDef,
    transition: &[Statement],
    extra: &[String],
    buffers: &[(String, bool)],
) -> Option<PeekLocals> {
    fn note(e: &Expr, out: &mut std::collections::BTreeSet<String>) {
        if let Expr::Var(v) = e {
            if let Some(bare) = v.strip_prefix("sp->") {
                out.insert(bare.to_string());
            }
        }
    }
    fn targets(list: &[Statement], out: &mut std::collections::BTreeSet<String>) {
        for st in list {
            if let Statement::Assign { target, .. } = st {
                note(target, out);
                // A fixed-size array field is named through its subscript,
                // which the `Var` arm never sees. Heap buffers are excluded by
                // the caller — those the peek frame never writes at all.
                if let Expr::ArrayAccess(n, _) = target {
                    note(&Expr::Var(n.clone()), out);
                }
            }
            for b in nested_stmt_bodies(st) {
                targets(b, out);
            }
        }
    }
    let mut written: std::collections::BTreeSet<String> = extra.iter().cloned().collect();
    for st in transition {
        streaming::walk_stmt_exprs(st, &mut |top| {
            streaming::walk_expr(top, &mut |e| match e {
                Expr::PostIncrement(i)
                | Expr::PostDecrement(i)
                | Expr::PreIncrement(i)
                | Expr::PreDecrement(i) => note(i, &mut written),
                _ => {}
            });
        });
    }
    targets(transition, &mut written);
    let mut names_read: std::collections::BTreeSet<String> = std::collections::BTreeSet::new();
    for st in transition {
        streaming::walk_stmt_exprs(st, &mut |e| streaming::expr_var_names(e, &mut names_read));
    }
    for (b, _) in buffers {
        if let Some(bare) = b.strip_prefix("sp->") {
            written.remove(bare);
        }
    }

    // The handle's BUFFER BASES, which the frame reads and — by the shadow
    // rewrite's contract — never writes. The copy this replaces put them in
    // front of SRA, so each was a register for the whole frame; read through a
    // pointer instead, they are rematerialized once per unrolled arm of a
    // rescan.
    //
    // Bases only: binding every field the frame reads spills, and lands ABOVE
    // the instruction count the copy itself cost.
    let mut bases: Vec<(String, VarType)> = buffers
        .iter()
        .filter_map(|(b, int_elem)| {
            let bare = b.strip_prefix("sp->")?;
            let ty = if *int_elem { VarType::IntPointer } else { VarType::RealPointer };
            names_read.contains(b).then(|| (bare.to_string(), ty))
        })
        .collect();
    bases.retain(|(n, _)| !written.contains(n));

    let mut taken: std::collections::BTreeSet<String> =
        streaming::input_array_names(func).into_iter().collect();
    taken.extend(func.outputs.iter().map(|o| o.name.clone()));
    if written.iter().any(|w| taken.contains(w)) {
        return None;
    }
    bases.retain(|(n, _)| !taken.contains(n));
    let bare: std::collections::BTreeMap<String, String> = written
        .iter()
        .chain(bases.iter().map(|(n, _)| n))
        .map(|w| (format!("sp->{w}"), w.clone()))
        .collect();
    let out = streaming::rewrite_stmts(
        transition,
        &|e| match e {
            Expr::Var(ref v) => bare.get(v).map_or(e, |b| Expr::Var(b.clone())),
            Expr::ArrayAccess(ref v, ref i) => match bare.get(v) {
                Some(b) => Expr::ArrayAccess(b.clone(), i.clone()),
                None => e,
            },
            other => other,
        },
        &|st| Some(st),
    );
    Some((written.into_iter().collect(), bases, out))
}

/// The statement lists nested inside `st`.
fn nested_stmt_bodies(st: &Statement) -> Vec<&[Statement]> {
    match st {
        Statement::While { body, .. }
        | Statement::DoWhile { body, .. }
        | Statement::For { body, .. }
        | Statement::Block { body } => vec![body.as_slice()],
        Statement::ForC { init, update, body, .. } => vec![
            std::slice::from_ref(init.as_ref()),
            std::slice::from_ref(update.as_ref()),
            body.as_slice(),
        ],
        Statement::If { then_body, else_body, .. } => {
            vec![then_body.as_slice(), else_body.as_slice()]
        }
        Statement::Switch { cases, default, .. } => {
            let mut v: Vec<&[Statement]> = cases.iter().map(|(_, b)| b.as_slice()).collect();
            v.push(default.as_slice());
            v
        }
        _ => Vec::new(),
    }
}

/// One localized field's bind, at the top of the frame.
///
/// A fixed-size array field takes a `memcpy` because C cannot bind an array by
/// assignment. The copy is bounded — the dimension is the indicator's, never
/// the period's — which is the whole of what a peek frame promises about its
/// cost. A period-sized buffer never reaches here; the shadow rewrite has it.
fn peek_seed(pad: &str, name: &str, ty: &VarType, from: &str) -> String {
    match ty {
        VarType::RealArray(_) | VarType::IntArray(_) => {
            format!("{pad}memcpy( {name}, {from}, sizeof({name}) );\n")
        }
        _ => format!("{pad}{name} = {from};\n"),
    }
}

/// The declared type of one localizable field. The state struct is the only
/// authority: a field it does not declare has no local to move to, and reaching
/// here with one means the transition writes something the handle never held.
fn peek_field_type(model: &StreamModel, name: &str) -> Option<VarType> {
    if let Some((_, ty)) = model.state.iter().find(|(n, _)| n == name) {
        return Some(ty.clone());
    }
    if let Some(base) = name.strip_prefix("cur_") {
        if model.func.outputs.iter().any(|o| o.name == base) {
            return Some(if out_c_type(model.func, base) == "int" {
                VarType::Integer
            } else {
                VarType::Real
            });
        }
    }
    nonscalar_struct_fields(model.func, model).into_iter().find(|(n, _)| n == name).map(|(_, t)| t)
}

/// [`peek_field_type`] for a field the frame WRITES, where not knowing the type
/// is fatal: the write has to move to a local or the `const` binding rejects it,
/// and a name that reaches here is one the struct never declared.
fn peek_local_type(model: &StreamModel, name: &str) -> VarType {
    peek_field_type(model, name).unwrap_or_else(|| {
        panic!("{}: peek would localize {name}, which the stream struct does not declare", model.func.name)
    })
}

/// One peek frame localized: every state field it writes bound to a local, and
/// every store nothing reads dropped along with the declaration it would have
/// needed.
///
/// The purge is not cosmetic here. Localizing turns a store into the handle
/// that nothing read into a store to a local that nothing reads, which is
/// `-Wunused-but-set-variable`; and a shadow whose only store goes with it
/// leaves its `pkSlot`/`pkVal` pair unread in the same way.
fn peek_localized(
    model: &StreamModel,
    names: &dyn streaming::NameMap,
    pt: streaming::PeekTransition,
    body: &[Statement],
) -> (Vec<(String, VarType)>, streaming::PeekTransition) {

    // The extrema rebase moves the cursor before the first store, so its
    // targets are localized with the transition's own.
    let mut rebased: Vec<String> = Vec::new();
    if let Some(ex) = model.extrema() {
        rebased.push(model.cursor.clone());
        rebased.push(ex.trailing.clone());
        rebased.extend(ex.index_vars.iter().cloned());
    }
    let bufs = streaming::transition_buffers(model, names);
    let (written, bases, body) = localize_peek_state_writes(model.func, body, &rebased, &bufs)
        .unwrap_or_else(|| {
            panic!("{}: a peek local would shadow a bar input or an output", model.func.name)
        });
    let typed: Vec<(String, VarType)> =
        written.iter().map(|n| (n.clone(), peek_local_type(model, n))).collect();
    // Read-only binds carry no store, so the purge has nothing to say about
    // them; `temps_used` still drops one whose only reader the purge deleted.
    let bound = bases;
    // The rebase is emitted as text beside the frame, so no statement shows the
    // read that keeps its targets alive; hold them out of the purge entirely.
    let pinned: std::collections::BTreeSet<&str> = rebased.iter().map(String::as_str).collect();
    let mut purgeable: Vec<(String, VarType)> =
        typed.iter().filter(|(n, _)| !pinned.contains(n.as_str())).cloned().collect();
    for sh in &pt.shadows {
        purgeable.push((sh.slot_var.clone(), VarType::Integer));
        purgeable.push((
            sh.val_var.clone(),
            if sh.int_elem { VarType::Integer } else { VarType::Real },
        ));
    }
    for t in &pt.slot_temps {
        purgeable.push((t.clone(), VarType::Integer));
    }
    let body = streaming::purge_dead_temp_stores(&body, &purgeable);
    let kept: std::collections::BTreeSet<String> =
        streaming::temps_used(&purgeable, &body).into_iter().map(|(n, _)| n).collect();
    let read_kept: std::collections::BTreeSet<String> =
        streaming::temps_used(&bound, &body).into_iter().map(|(n, _)| n).collect();
    let live: Vec<(String, VarType)> = typed
        .into_iter()
        .filter(|(n, _)| pinned.contains(n.as_str()) || kept.contains(n))
        .chain(bound.into_iter().filter(|(n, _)| read_kept.contains(n)))
        .collect();
    let pt = streaming::PeekTransition {
        body,
        shadows: pt
            .shadows
            .into_iter()
            .filter(|sh| kept.contains(&sh.slot_var) || kept.contains(&sh.val_var))
            .collect(),
        slot_temps: pt.slot_temps.into_iter().filter(|t| kept.contains(t)).collect(),
    };
    (live, pt)
}

/// The per-bar step body for ONE model at a given indent, split into the block's
/// DECLARATIONS and its STATEMENTS. The peek frame is emitted straight into
/// `Peek`, whose own `sp` binding and argument guards sit between the two
/// halves, and C89 wants every declaration in a block ahead of every statement
/// in it. Shared by the single-model [`emit_step`] and the dual-mode step
/// (called once per arm inside the `if (sp->param ...)` branch, at a deeper
/// indent, with `void_sp = false` since a mode always has state).
///
/// A carried scalar's load is a STATEMENT here (`x = sp->x;`) rather than an
/// initializer, because `Peek` has not yet rejected a null handle where the
/// declarations go.
fn emit_step_inner(
    decls: &mut String,
    body: &mut String,
    model: &StreamModel,
    enums: &HashMap<String, EnumDef>,
    registry: &Registry,
    helpers: &HelperRegistry,
    counter: &Cell<usize>,
    indent: usize,
    void_sp: bool,
    frame: StepFrame,
) {
    let pad = " ".repeat(indent);
    let transition = streaming::build_transition(model, &CNames)
        .unwrap_or_else(|e| panic!("streaming transition: {e}"));
    // Peek localizes every field it writes instead, which is the stronger
    // property — the reload the election removes cannot survive a store that
    // never reaches the handle. `stream_base` makes `sp->x` and `x` classify
    // alike, so neither rewrite can move an FMA site.
    let elected = match frame {
        StepFrame::Commit => elect_step_scalars(model, &transition),
        StepFrame::Peek => Vec::new(),
    };
    let (transition, shadows, slot_temps, locals) = match frame {
        StepFrame::Commit => (transition, Vec::new(), Vec::new(), Vec::new()),
        StepFrame::Peek => {
            let pt = streaming::peek_transition_widest(model, &CNames, &transition, None)
                .unwrap_or_else(|e| panic!("{}: {e}", model.func.name));
            let answered = answer_bare_returns(&pt.body);
            let (locals, pt) = peek_localized(model, &CNames, pt, &answered);
            (pt.body, pt.shadows, pt.slot_temps, locals)
        }
    };
    let transition = apply_step_scalars(&transition, &elected);
    let temps = match frame {
        StepFrame::Commit => model.temps.clone(),
        StepFrame::Peek => streaming::temps_used(&model.temps, &transition),
    };
    for (name, ty) in &temps {
        let _ = writeln!(decls, "{pad}{};", c_decl(ty, name));
    }
    for name in &elected {
        let _ = writeln!(decls, "{pad}double {name};");
    }
    for (name, ty) in &locals {
        let _ = writeln!(decls, "{pad}{};", c_decl(ty, name));
    }
    decls.push_str(&peek_shadow_decls(&shadows, &slot_temps, indent));
    for name in &elected {
        let _ = writeln!(body, "{pad}{name} = {};", streaming::NameMap::state(&CNames, name));
    }
    for (name, ty) in &locals {
        body.push_str(&peek_seed(&pad, name, ty, &streaming::NameMap::state(&CNames, name)));
    }
    emit_extrema_rebase(body, model, frame);
    let mut body_c = String::new();
    for s in &transition {
        body_c.push_str(&render_statement_stream(s, indent, enums, registry, helpers, counter, &nullable_out_names(model.func)));
    }
    for name in &elected {
        let _ = writeln!(body_c, "{pad}{} = {name};", streaming::NameMap::state(&CNames, name));
    }
    // Read off the rendered text, not off the model: a peek frame can localize
    // its last written field and purge its last read, leaving `sp` unused where
    // the structural predicate still says it is needed.
    if void_sp || !(body.contains("sp->") || body_c.contains("sp->")) {
        body.insert_str(0, &format!("{pad}(void)sp;\n"));
    }
    // Candle settings are read where batch reads them (per step, from the
    // globals — the settings-stability rule). The TA_STREAM_CANDLE* macros
    // read the globals directly, so hoisted locals are emitted only when
    // the rendered body actually references them (no dead decls/-Wunused).
    let step_settings = crate::candle_settings::detect_candle_settings(&model.steady_stmts);
    if !step_settings.is_empty() {
        decls.push_str(&emit_used_candle_unpacking(&step_settings, &body_c, indent));
    }
    body.push_str(&body_c);
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
    frame: StepFrame,
) {
    // Peek commits nothing, so its arm carries no value retain.
    let built = match frame {
        StepFrame::Commit => streaming::identity_step_branch(model, &CNames),
        StepFrame::Peek => streaming::identity_peek_branch(model, &CNames),
    };
    if let Some(s) = built {
        // The short-circuit exits the transition, which is `void`; in a peek
        // frame it exits `Peek`, which answers a code.
        let s = match frame {
            StepFrame::Commit => s,
            StepFrame::Peek => answer_bare_returns(std::slice::from_ref(&s))
                .pop()
                .expect("one statement in, one out"),
        };
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
fn emit_extrema_rebase(o: &mut String, model: &StreamModel, frame: StepFrame) {
    if let Some(ex) = model.extrema() {
        // A peek frame has already moved every one of these to a local.
        let q = match frame {
            StepFrame::Commit => "sp->",
            StepFrame::Peek => "",
        };
        let mut vars: Vec<String> = vec![model.cursor.clone(), ex.trailing.clone()];
        vars.extend(ex.index_vars.iter().cloned());
        let _ = writeln!(o, "   if( {q}{} >= 1073741824 )", model.cursor);
        let _ = writeln!(o, "   {{");
        let _ = writeln!(
            o,
            "      int rebaseShift = {q}{} & ~sp->xMask;",
            ex.trailing
        );
        for v in &vars {
            let _ = writeln!(o, "      {q}{v} -= rebaseShift;");
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

    emit_open_validation(o, func, enums);

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
/// `emit_open_arm`: the merged `<N>_OpenImpl`, then `OpenInternal` (stride 0),
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

/// State struct for a period bank (MAVP): the optional params, the bank of
/// per-period sub handles, and the scratch the bank writes its outputs into.
fn emit_period_bank_struct(o: &mut String, func: &FuncDef, plan: &streaming::PeriodBankPlan) {
    let n = uname(func);
    let subty = format!("struct {}_Stream", callee_prefix(&plan.callee));
    let _ = writeln!(o, "struct TA_{n}_Stream {{");
    emit_range_head_fields(o);
    emit_cur_fields(o, func);
    for p in &func.optional_inputs {
        let _ = writeln!(o, "   {} {};", opt_param_c_type(&p.param_type), p.name);
    }
    let _ = writeln!(o, "   int nBank;");
    let _ = writeln!(o, "   {subty} **bank;");
    let _ = writeln!(o, "   double *scratch;");
    let _ = writeln!(o, "}};\n");
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
    // Same as the dispatch tier: hand-rolled bodies that index the output
    // unguarded, so a nullable output would be undefined behaviour rather than
    // the documented "pass NULL to skip".
    assert!(
        nullable_out_names(func).is_empty(),
        "{}: the period-bank tier hand-rolls its bodies and indexes its output unguarded; \
         a nullable output needs the `out ? &out[i] : NULL` form the shared emitters use",
        uname(func)
    );

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
    emit_period_bank_struct(o, func, plan);

    // --- OpenInternal -------------------------------------------------------
    let _ = writeln!(o, "/* Private function, not in public API. */\n{}\n{{", open_internal_signature(func));
    let _ = writeln!(o, "   struct TA_{n}_Stream *sp;");
    let _ = writeln!(o, "   int k, cp, lookbackTotal, subStart;");
    let _ = writeln!(o, "   double cpReal;");
    let _ = writeln!(o, "   TA_RetCode retCode;");
    let _ = writeln!(o, "\n   if( !stream ) return TA_BAD_PARAM;");
    let _ = writeln!(o, "   *stream = NULL;");
    o.push_str(index_pair_guards());
    o.push_str(&presence_guard(func, Frame::Open));
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
    // The bank is opened at `subStart`, so the history has to reach it. Without
    // this an anchor past the history publishes a negative count (and, where the
    // count is unsigned, underflows).
    let _ = writeln!(o, "   if( historyLen < subStart + 1 ) return TA_INSUFFICIENT_HISTORY;");

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
    // No out-param pair on the scalar open: `subStart` is the resolved
    // `max(startIdx, lookback)` the bank was opened at, which is the range's
    // start by definition.
    let _ = writeln!(o, "\n   sp->outRangeBegIdx = subStart;");
    let _ = writeln!(o, "   sp->outRangeCount = historyLen - subStart;");
    // The scalar open just wrote the clamped slot into the caller's
    // out-pointer; that is the value at the last committed bar.
    for out in &func.outputs {
        let name = &out.name;
        let _ = writeln!(o, "   sp->cur_{name} = *{name};");
    }
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
    o.push_str(index_pair_guards());
    o.push_str(&presence_guard(func, Frame::OpenAndFill));
    let mut alias: Vec<String> = Vec::new();
    for inp in &inputs {
        alias.push(alias_term(func, out, inp));
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
    emit_range_head_capture(o, "   ");
    emit_cur_capture(o, "   ", func, false);
    let _ = writeln!(o, "   *stream = sp;");
    let _ = writeln!(o, "   return TA_SUCCESS;\n}}\n");

    // --- Update -------------------------------------------------------------
    let _ = writeln!(o, "{}\n{{", update_signature(func));
    let _ = writeln!(o, "   int k, cp;");
    let _ = writeln!(o, "   double cpReal;");
    let _ = writeln!(o, "   if( !stream || !{out} ) return TA_BAD_PARAM;");
    // inPeriods is checked here too: a non-finite period would reach `(int)`, and
    // the conversion of NaN or an infinity to int is undefined behaviour.
    o.push_str(&finite_bar_check(func, "   ", "TA_BAD_PARAM", Some("stream")));
    let _ = writeln!(o, "   for( k = 0; k < stream->nBank; k++ )");
    let _ = writeln!(o, "      {pre}_Update( stream->bank[k], {price}, &stream->scratch[k] );");
    let _ = writeln!(o, "   cpReal = {period};");
    let _ = writeln!(o, "   if( !(cpReal >= stream->{min}) ) cp = stream->{min};");
    let _ = writeln!(o, "   else if( cpReal > stream->{max} ) cp = stream->{max};");
    let _ = writeln!(o, "   else cp = (int)cpReal;");
    let _ = writeln!(o, "   *{out} = stream->scratch[cp - stream->{min}];");
    emit_cur_retain(o, "   ", "stream", func, None);
    emit_range_head_advance(o, "   ", "stream");
    let _ = writeln!(o, "   return TA_SUCCESS;\n}}\n");

    // --- Peek ---------------------------------------------------------------
    // Only the SELECTED slot is peeked: the other slots' next values are not the
    // output for this bar, and peeking is non-committing per sub-handle.
    let _ = writeln!(o, "{}\n{{", peek_signature(func));
    let _ = writeln!(o, "   int cp;");
    let _ = writeln!(o, "   double cpReal;");
    let _ = writeln!(o, "   if( !stream || !{out} ) return TA_BAD_PARAM;");
    o.push_str(&finite_bar_check(func, "   ", "TA_BAD_PARAM", None));
    let _ = writeln!(o, "   cpReal = {period};");
    let _ = writeln!(o, "   if( !(cpReal >= stream->{min}) ) cp = stream->{min};");
    let _ = writeln!(o, "   else if( cpReal > stream->{max} ) cp = stream->{max};");
    let _ = writeln!(o, "   else cp = (int)cpReal;");
    let _ = writeln!(
        o,
        "   {pre}_Peek( stream->bank[cp - stream->{min}], {price}, {out} );"
    );
    let _ = writeln!(o, "   return TA_SUCCESS;\n}}\n");

    // --- UpdateAndFill --------------------------------------------------------
    // Inherently per-bar: every slot in the bank advances on every bar, and only
    // then does that bar's clamped period select which slot is the output. There
    // is no array-at-a-time form of that, so this is Update's body in a loop.
    let _ = writeln!(o, "{}\n{{", update_and_fill_signature(func));
    let _ = writeln!(o, "   int i, k, cp;");
    let _ = writeln!(o, "   double cpReal;\n");
    o.push_str(&update_and_fill_guards(func));
    let _ = writeln!(o, "   for( i = 0; i < barCount; i++ )\n   {{");
    o.push_str(&finite_bar_check_indexed(func, "      ", "i", "TA_BAD_PARAM", Some("stream")));
    let _ = writeln!(o, "      for( k = 0; k < stream->nBank; k++ )");
    let _ = writeln!(o, "         {pre}_Update( stream->bank[k], {price}[i], &stream->scratch[k] );");
    let _ = writeln!(o, "      cpReal = {period}[i];");
    let _ = writeln!(o, "      if( !(cpReal >= stream->{min}) ) cp = stream->{min};");
    let _ = writeln!(o, "      else if( cpReal > stream->{max} ) cp = stream->{max};");
    let _ = writeln!(o, "      else cp = (int)cpReal;");
    let _ = writeln!(o, "      {out}[i] = stream->scratch[cp - stream->{min}];");
    emit_cur_retain(o, "      ", "stream", func, Some("i"));
    emit_range_head_advance(o, "      ", "stream");
    let _ = writeln!(o, "   }}");
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
    let nullable = nullable_out_names(func);
    // The handle's `cur_<out>` has to hold what the guarded store *would* have
    // written even when the caller declined it — `TA_<N>_StepImpl` always
    // recomputes it (mama.c), so a capture that instead reads the (possibly
    // absent) array diverges from `Open(P)+updates`. Every guarded store below
    // also lands here, unconditionally; `alloc_and_capture` reads it.
    for name in &nullable {
        let _ = writeln!(o, "      double lastCur_{name} = 0.0;");
    }
    let open_body = build_open_body_from(model, body);
    let mut open_body_c = String::new();
    for s in &open_body {
        open_body_c.push_str(&render_statement(s, 6, false, enums, registry, helpers, counter, &nullable, true));
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
    emit_open_tail(o, func);
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
            "      if( sp->cbSize_{id} < 1 || sp->cbSize_{id} > historyLen + 1 ) {{ {free_batch}TA_{n}_ReleaseImpl( sp ); return TA_INTERNAL_ERROR({eid}); }}",
            eid = crate::internal_error_ids::site(&format!("cbsize.{id}"))
        );
        for (storage, ty) in circ_storages(circ) {
            let et = if matches!(ty, crate::ir::VarType::Integer) { "int" } else { "double" };
            let _ = writeln!(
                o,
                "      sp->cb_{storage} = ({et} *)TA_Malloc( sizeof({et}) * (size_t)sp->cbSize_{id} );"
            );
            let _ = writeln!(o, "      if( !sp->cb_{storage} ) {{ {free_batch}TA_{n}_ReleaseImpl( sp ); return TA_ALLOC_ERR; }}");
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
fn emit_open_tail(o: &mut String, func: &FuncDef) {
    emit_range_head_capture(o, "      ");
    emit_cur_capture(o, "      ", func, true);
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
/// and NULLs the ring pointers so `ReleaseImpl` is safe mid-allocation.
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
        // A DECLINABLE output has no output slot to read at the publish point
        // (its array may be NULL), so it is seeded here, from `lastCur_<out>`
        // — the transcribed open loop's own shadow of the guarded store
        // (`nullable_shadow`), computed every iteration regardless of decline
        // so it matches what `TA_<N>_StepImpl` always recomputes. Every other
        // output is seeded at the publish point, where all five tiers meet.
        for name in nullable_out_names(func) {
            let _ = writeln!(s, "{pad}sp->cur_{name} = lastCur_{name};");
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
                VarType::RealArray(_) | VarType::IntArray(_)
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
        format!("{{ {pre}TA_{n}_ReleaseImpl( sp ); return TA_ALLOC_ERR; }}")
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
                    "{pad}if( sp->ringLag_{v} < {fwd} || sp->ringCap_{v} > historyLen ) {{ {pre}TA_{n}_ReleaseImpl( sp ); return TA_INTERNAL_ERROR({eid}); }}",
                    fwd = ring.fwd,
                    eid = crate::internal_error_ids::site(&format!("ringlag.{v}"))
                );
            } else {
                let _ = writeln!(s, "{pad}sp->ringCap_{v} = (int)({} - {v});", model.cursor);
                let _ = writeln!(
                    s,
                    "{pad}if( sp->ringCap_{v} < 0 || sp->ringCap_{v} > historyLen ) {{ {pre}TA_{n}_ReleaseImpl( sp ); return TA_INTERNAL_ERROR({eid}); }}",
                    eid = crate::internal_error_ids::site(&format!("ringcap.{v}"))
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
            "{pad}if( sp->winCap_{v} < 1 || sp->winCap_{v} > historyLen ) {{ {pre}TA_{n}_ReleaseImpl( sp ); return TA_INTERNAL_ERROR({eid}); }}",
            eid = crate::internal_error_ids::site(&format!("wincap.{v}"))
        );
        for arr in &win.arrays {
            let _ = writeln!(
                s,
                "{pad}sp->win_{v}_{arr} = (double *)TA_Malloc( sizeof(double) * (size_t)sp->winCap_{v} );"
            );
            let _ = writeln!(s, "{pad}if( !sp->win_{v}_{arr} ) {{ {pre}TA_{n}_ReleaseImpl( sp ); return TA_ALLOC_ERR; }}");
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
            "{pad}if( sp->xCap < 1 || sp->xCap > historyLen ) {{ {pre}TA_{n}_ReleaseImpl( sp ); return TA_INTERNAL_ERROR({eid}); }}",
            eid = crate::internal_error_ids::site("extrema")
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
            let _ = writeln!(s, "{pad}if( !sp->x_{arr} ) {{ {pre}TA_{n}_ReleaseImpl( sp ); return TA_ALLOC_ERR; }}");
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
        // batch( startIdx, .. ) begins at max(startIdx, lookback), and the
        // anchored `_Open*Internal` variants are the batch call over that same
        // range. The public entry points pass 0, so this is a no-op for them —
        // it is the composition seams that were reporting (and filling) from
        // the raw lookback. The dispatch tier already clamped here; the loop
        // tier did not, and the #241 range leg is what first caught it.
        let _ = writeln!(o, "      int fillLb = {lb_call};");
        let _ = writeln!(o, "      if( startIdx > fillLb ) fillLb = startIdx;");
        let _ = writeln!(o, "      if( historyLen < fillLb + 1 ) return TA_INSUFFICIENT_HISTORY;");
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
        emit_range_head_capture(o, "      ");
    emit_cur_capture(o, "      ", func, true);
        let _ = writeln!(o, "      *stream = sp;");
        let _ = writeln!(o, "      return TA_SUCCESS;");
        let _ = writeln!(o, "   }}");
    }
}

/// Open's argument validation: NULL checks, minimum history, and the same
/// optional-parameter default-substitution/range checks the batch uses.
/// Fill mode additionally requires the batch output triplet non-NULL and forbids
/// any output aliasing an input or another output — not because it would compute
/// the wrong answer (measured: it does not), but because the margin between the
/// fill's writes and the capture's seed reads is unasserted (rule S6; #108). The
/// scalar path writes only caller scalars and never has the question.
fn emit_open_validation(o: &mut String, func: &FuncDef, enums: &HashMap<String, EnumDef>) {
    let _ = writeln!(o, "\n   if( !stream ) return TA_BAD_PARAM;");
    let _ = writeln!(o, "   *stream = NULL;");
    o.push_str(index_pair_guards());
    o.push_str(&presence_guard(func, Frame::Open));
    // The out-meta NULL checks and the output-aliasing rejections are FILL-only
    // and live in `emit_open_and_fill_wrapper`: only the fill path writes the
    // caller's arrays, and only it can therefore have an output alias an input
    // or another output (#108/#130). `Open`'s sinks are this call's own stack
    // slots.
    o.push_str(&emit_opt_param_validation(func, "TA_BAD_PARAM", enums));
    emit_anchor_guard(o);
}

/// The anchor has to land inside the history.
///
/// This is the batch prologue's `(endIdx < startIdx)` rejection, which the
/// streaming prologue never had. It matters because only 137 of the 174
/// transcribed `_OpenImpl` bodies carry TA-Lib's "make sure there is still
/// something to evaluate" preamble — a function with no lookback has nothing to
/// clamp `startIdx` up to, so the transcription has no such check and the batch
/// tier caught the case in its prologue instead. The remaining 37 compute
/// `nbBar = endIdx - startIdx + 1` and then run `while( nbBar != 0 )`, so a
/// negative count never reaches zero: the loop walks off the end of both the
/// inputs and the output (an ASan stack-buffer-overflow inside
/// `TA_AD_OpenImpl`, and a `usize` underflow panic in Rust).
///
/// Emitting it here rather than per-body is what makes it total, and it cannot
/// change any behaviour that was already defined: the preamble's clamp only
/// ever moves `startIdx` UP, so `startIdx > endIdx` before the clamp implies it
/// after — this fires exactly where that preamble would have, returns the same
/// code, and writes the same out-meta. For the public entry points it is inert
/// (they pass `startIdx = 0`, and `historyLen < 1` is already rejected above).
///
/// A negative `startIdx` is deliberately NOT rejected here: no caller in the
/// tree can produce one (every call site passes a literal `0`, a pass-through
/// of an already-validated anchor, or MAVP's `subStart`), and Rust types the
/// parameter `usize`, so a guard for it would be unreachable in C and
/// inexpressible in the backend it would have to match.
fn emit_anchor_guard(o: &mut String) {
    let _ = writeln!(o, "   if( startIdx > historyLen - 1 )");
    let _ = writeln!(o, "   {{");
    let _ = writeln!(o, "      *outBegIdx = 0;");
    let _ = writeln!(o, "      *outNBElement = 0;");
    let _ = writeln!(o, "      return TA_INSUFFICIENT_HISTORY;");
    let _ = writeln!(o, "   }}");
}

/// `if (buf != &local_buf[0]) TA_Free(buf);` for every batch circ storage —
/// the frees a failure path owes for the withheld top-level destroys.
fn free_batch_storages(model: &StreamModel) -> String {
    let mut s = String::new();
    for (storage, _) in model.circs().iter().flat_map(circ_storages) {
        // Braced: these are emitted several to a line, and an unbraced body
        // makes the next `if` read as guarded (-Wmisleading-indentation).
        let _ = write!(s, "if( {storage} != &local_{storage}[0] ) {{ TA_Free( {storage} ); }} ");
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
                    VarType::RealArray(_) | VarType::IntArray(_) => {
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
    let _ = writeln!(o, "{}\n{{", update_signature(func));
    if step_ret {
        let _ = writeln!(o, "   TA_RetCode retCode;\n");
    }
    o.push_str(&presence_guard(func, Frame::Step));
    o.push_str(&finite_bar_check(func, "   ", "TA_BAD_PARAM", Some("stream")));
    let args: Vec<String> = bars
        .iter()
        .cloned()
        .chain(outs.iter().cloned())
        .collect();
    // `step_ret` is the composed tier's fallible step (a sub-stream can reject a
    // non-finite intermediate); every other tier's step returns void.
    if step_ret {
        let _ = writeln!(o, "   retCode = TA_{n}_StepImpl( stream, {} );", args.join(", "));
        let _ = writeln!(o, "   if( retCode != TA_SUCCESS ) return retCode;");
        emit_cur_retain(o, "   ", "stream", func, None);
        emit_range_head_advance(o, "   ", "stream");
        let _ = writeln!(o, "   return TA_SUCCESS;\n}}\n");
    } else {
        let _ = writeln!(o, "   TA_{n}_StepImpl( stream, {} );", args.join(", "));
        emit_range_head_advance(o, "   ", "stream");
        let _ = writeln!(o, "   return TA_SUCCESS;\n}}\n");
    }
}

/// `UpdateAndFill` for every tier that owns a `<N>_StepImpl` (loop, dual-mode,
/// composed): the step in a loop, writing output `i` at index `i`.
///
/// Semantically `barCount` back-to-back `Update`s and nothing more — same
/// rejection, same order, same state. What it removes is the per-bar entry cost:
/// one set of argument checks and one call for the whole run instead of one of
/// each per bar.
fn emit_update_and_fill(o: &mut String, func: &FuncDef, step_ret: bool) {
    let n = uname(func);
    let bars: Vec<String> = streaming::input_array_names(func);
    let _ = writeln!(o, "{}\n{{", update_and_fill_signature(func));
    let _ = writeln!(o, "   int i;");
    if step_ret {
        let _ = writeln!(o, "   TA_RetCode retCode;");
    }
    let _ = writeln!(o);
    o.push_str(&update_and_fill_guards(func));
    let _ = writeln!(o, "   for( i = 0; i < barCount; i++ )\n   {{");
    o.push_str(&finite_bar_check_indexed(func, "      ", "i", "TA_BAD_PARAM", Some("stream")));
    let args: Vec<String> = bars
        .iter()
        .map(|b| format!("{b}[i]"))
        .chain(indexed_out_args(func, "i"))
        .collect();
    // `step_ret` is the composed tier's fallible step: a sub-stream can reject an
    // intermediate, and the bars already committed stay committed.
    if step_ret {
        let _ = writeln!(o, "      retCode = TA_{n}_StepImpl( stream, {} );", args.join(", "));
        let _ = writeln!(o, "      if( retCode != TA_SUCCESS ) return retCode;");
    } else {
        let _ = writeln!(o, "      TA_{n}_StepImpl( stream, {} );", args.join(", "));
    }
    if step_ret {
        emit_cur_retain(o, "      ", "stream", func, Some("i"));
    }
    emit_range_head_advance(o, "      ", "stream");
    let _ = writeln!(o, "   }}");
    let _ = writeln!(o, "   return TA_SUCCESS;\n}}\n");
}

/// `Peek` — the transition against the caller's own handle, inline.
///
/// `sp` is `const`, which is the whole contract: every store the frame would
/// make lives in a local (state scalars by [`peek_localized`], buffer elements
/// by the shadow rewrite), so a peek that commits is not a bug to be caught at
/// run time but a body that does not compile.
///
/// The frame is inline rather than a second `_Impl` because it has exactly one
/// caller and always will — a cross-indicator call enters the callee's PUBLIC
/// `Peek`, never its frame.
fn emit_peek(
    o: &mut String,
    func: &FuncDef,
    frame_decls: &str,
    frame_body: &str,
    fallible: bool,
) {
    let n = uname(func);
    let guard_frame = if fallible { Frame::StepEveryOutput } else { Frame::Step };
    let _ = writeln!(o, "{}\n{{", peek_signature(func));
    let _ = writeln!(o, "   const struct TA_{n}_Stream *sp = stream;");
    o.push_str(frame_decls);
    let _ = write!(o, "\n{}", presence_guard(func, guard_frame));
    o.push_str(&finite_bar_check(func, "   ", "TA_BAD_PARAM", None));
    o.push_str(frame_body);
    if !fallible {
        let _ = writeln!(o, "   return TA_SUCCESS;");
    }
    let _ = writeln!(o, "}}\n");
}

/// [`emit_peek`] for one model: build the peek frame, then wrap it.
#[allow(clippy::too_many_arguments)]
fn emit_peek_loop(
    o: &mut String,
    func: &FuncDef,
    model: &StreamModel,
    enums: &HashMap<String, EnumDef>,
    registry: &Registry,
    helpers: &HelperRegistry,
    counter: &Cell<usize>,
) {
    let void_sp = model.state.is_empty()
        && func.optional_inputs.is_empty()
        && func.private_extra_params.is_empty()
        && model.lags.is_empty();
    let (mut decls, mut body) = (String::new(), String::new());
    emit_step_inner(&mut decls, &mut body, model, enums, registry, helpers, counter, 3, void_sp, StepFrame::Peek);
    emit_peek(o, func, &decls, &body, false);
}

fn emit_close_from(o: &mut String, func: &FuncDef, needs_release: bool) {
    let n = uname(func);
    let _ = writeln!(o, "{}\n{{", close_signature(func));
    if needs_release {
        let _ = writeln!(o, "   TA_{n}_ReleaseImpl( stream );");
    } else {
        let _ = writeln!(o, "   if( stream ) TA_Free( stream );");
    }
    let _ = writeln!(o, "   return TA_SUCCESS;\n}}\n");
}

fn emit_close(o: &mut String, func: &FuncDef, model: &StreamModel) {
    emit_close_from(o, func, model.needs_release());
}

#[cfg(test)]
mod tests {
    use super::*;

    fn var(n: &str) -> Expr {
        Expr::Var(n.into())
    }

    /// The write-back-before-`return` arm. Today's corpus reaches it only
    /// vacuously — the sole return a transition carries is the param-degenerate
    /// identity short-circuit, which sits at index 0 and so never follows a
    /// mutation — so exercise it directly on a transition that DOES mutate
    /// first. Without the arm the returning path would leave `sp->sum` holding
    /// the previous bar's value.
    #[test]
    fn a_return_after_a_mutation_writes_the_local_back() {
        let elected = vec!["sum".to_string()];
        let transition = vec![
            Statement::Assign {
                target: var("sp->sum"),
                value: Expr::Literal(1.0),
                compound: false,
            },
            Statement::If {
                condition: Expr::BinOp(
                    Box::new(var("sp->sum")),
                    crate::ir::BinOp::Greater,
                    Box::new(Expr::Literal(0.0)),
                ),
                then_body: vec![Statement::Return { value: None }],
                else_body: vec![],
                cond_comments: vec![],
            },
        ];
        let out = apply_step_scalars(&transition, &elected);

        // The body now works in the bare local...
        let Statement::Assign { target, .. } = &out[0] else {
            panic!("first statement is the assignment")
        };
        assert_eq!(target, &var("sum"), "sp->sum rewritten to the local");

        // ...and the return is wrapped with the store back to the handle.
        let Statement::If { then_body, .. } = &out[1] else {
            panic!("second statement is the branch")
        };
        let Statement::Block { body } = &then_body[0] else {
            panic!("the return is wrapped in a block carrying the write-back")
        };
        assert_eq!(body.len(), 2, "one write-back, then the return");
        assert!(
            matches!(&body[0], Statement::Assign { target, value, compound: false }
                     if target == &var("sp->sum") && value == &var("sum")),
            "sp->sum = sum before returning, got {:?}", body[0]
        );
        assert!(matches!(body[1], Statement::Return { .. }), "return kept");
    }

    /// Electing nothing must leave the transition byte-identical — the 134
    /// functions that elect no scalar have to keep the emission they had.
    #[test]
    fn an_empty_election_does_not_touch_the_transition() {
        let transition = vec![Statement::Assign {
            target: var("sp->sum"),
            value: var("sp->sum"),
            compound: true,
        }];
        let out = apply_step_scalars(&transition, &[]);
        assert_eq!(format!("{out:?}"), format!("{transition:?}"));
    }
}
