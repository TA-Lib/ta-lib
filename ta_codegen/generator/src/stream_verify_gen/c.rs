//! C `stream_verify`: `handle_stream_verify` runs batch (startIdx=0) and the
//! stream trajectory in-process on identical seeded inputs, compares BITWISE
//! per bar (memcmp on doubles), spot-asserts peek == update, and answers flat
//! JSON (`ok`, per-leg match flags, first divergence as %a on mismatch). See
//! docs/streaming-api-design.md, Verification. The whole handler is compiled
//! out under TA_REF_SERVE, frozen reference libraries having no stream
//! symbols.

use super::{
    collect_pin_ids, sv_input_suffix, sv_range_bit, sv_reject_condition, SvRangeSite,
    SV_RANGE_MASK_C,
};
use crate::ir::{EnumDef, FuncDef, ParamType};
use crate::server_gen::expand_input_names;
use std::collections::{BTreeMap, BTreeSet, HashMap};
use std::fmt::Write as _;

/// Emit the per-output bitwise (double) / exact (int) comparison lines of a
/// stream_verify leg.
fn emit_sv_compare(
    s: &mut String,
    out_is_int: &[bool],
    bbuf: &[String],
    pad: &str,
    idx: &str,
    bar: &str,
    pre: &str,
) {
    for (i, is_int) in out_is_int.iter().enumerate() {
        let b = &bbuf[i];
        if *is_int {
            let _ = std::fmt::Write::write_fmt(s, format_args!(
                "{pad}if( {pre} v{i} != {b}[{idx}] ) {{ ok = 0; badBar = {bar}; badOut = {i}; bv = (double){b}[{idx}]; sv = (double)v{i}; }}\n"
            ));
        } else {
            let _ = std::fmt::Write::write_fmt(s, format_args!(
                "{pad}if( {pre} sv_xtier_ne(v{i}, {b}[{idx}], &svZsign) ) {{ ok = 0; badBar = {bar}; badOut = {i}; bv = {b}[{idx}]; sv = v{i}; }}\n"
            ));
        }
    }
}

/// The fuzz-convention input array for one expanded input name: price
/// components map to their OHLCV series; generic reals map real0→close,
/// real1→volume (matches abstract_call/fuzz-064 and the driver).
fn sv_input_array(name: &str, generic_idx: &mut usize) -> &'static str {
    match sv_input_suffix(name, generic_idx) {
        "o" => "sv_o",
        "h" => "sv_h",
        "l" => "sv_l",
        "c" => "sv_c",
        "v" => "sv_v",
        _ => "sv_oi",
    }
}

/// Tail of the batch-failure branch: candle functions record the outcome and
/// continue to the next settings round (a failed round must not truncate the
/// sweep); non-candle functions respond and return as before.
fn emit_sv_batch_fail_tail(s: &mut String, candle: bool) {
    // Reject parity: whenever the batch leg produced nothing — an error
    // (bad params, e.g. an out-of-list enum hitting a dispatch default arm)
    // or an empty range — the stream's Open must reject too. Open mirrors
    // the batch validation and min-history by construction, so a stream
    // that opens where batch fails is always a contract break. Never force
    // ok=1 on a batch error -- that shields exactly this case.
    if candle {
        s.push_str("            if( !openRejects ) allOk = 0;\n");
        s.push_str("            if( rd + 1 < rounds ) continue;\n");
        s.push_str("            TA_RestoreCandleDefaultSettings( TA_AllCandleSettings );\n");
        // Reachable after earlier candle rounds already compared, so the benign
        // count travels with it — otherwise those cases vanish from the summary.
        s.push_str("            pos = json_appendf(resp, resp_size, pos, \",\\\"rrc\\\":%d,\\\"legs\\\":%d,\\\"nb\\\":%d,\\\"openRejects\\\":%d,\\\"ok\\\":%d,\\\"peek_checked\\\":%d,\\\"peek_ok\\\":%d,\\\"peek_reps\\\":%d,\\\"peek_rep_ok\\\":%d,\\\"peek_rejects\\\":%d,\\\"benign\\\":%d}\", (int)rc, lgi, svNb, openRejects, allOk ? 1 : 0, peekChecked, peekAll, peekReps, peekRepAll, peekRejects, svZsign);\n");
    } else {
        s.push_str("            snprintf(resp, resp_size, \"{\\\"retCode\\\":%d,\\\"legs\\\":0,\\\"nb\\\":%d,\\\"openRejects\\\":%d,\\\"ok\\\":%d,\\\"peek_ok\\\":1}\", (int)rc, svNb, openRejects, openRejects);\n");
    }
    s.push_str("            return;\n");
    s.push_str("        }\n");
}

/// Period-bank functions (MAVP): the fuzz period-selector input (mapped to a
/// generic real series ~volume, always >= 1000) would clamp to `maxPeriod` at
/// every bar, so the stream_verify would only ever exercise ONE bank slot and
/// pass vacuously for all others. Overwrite the selector with a ramp spanning
/// `[minPeriod-1, maxPeriod+1]` (fed identically to the batch and the stream),
/// so every bank slot AND both clamp directions are exercised. Regenerated per
/// request (fuzz_gen runs first), so this override does not leak to other funcs.
fn emit_sv_period_bank_input(
    s: &mut String,
    func: &FuncDef,
    funcs: &[FuncDef],
    input_arrays: &[&str],
) {
    let lookup = crate::streaming::FuncsLookup(funcs);
    let Ok(crate::streaming::StreamPlan::PeriodBank(pb)) =
        crate::streaming::validate_streamable(func, &lookup)
    else {
        return;
    };
    let inputs = crate::streaming::input_array_names(func);
    let Some(idx) = inputs.iter().position(|i| *i == pb.period_input) else {
        return;
    };
    let arr = input_arrays[idx];
    s.push_str(&format!(
        "        {{ int _pi; for( _pi = 0; _pi < svN; _pi++ ) {arr}[_pi] = (double)({min} + (_pi % ({max} - {min} + 3)) - 1); }}\n",
        min = pb.min_param,
        max = pb.max_param
    ));
}

/// Dispatch functions (MA): enum values whose arm has no sub-stream reject
/// at Open — a DOCUMENTED capability limitation, verified loudly here
/// (never a silent vacuous pass). The identity path (period==1) is exempt:
/// it streams for every arm value, exactly as the batch checks it before
/// dispatching. The unsupported set is derived from the callees' stream
/// flags at generation time, so a callee gaining the flag (TRIMA) flips its
/// legs from expect-reject to verified automatically on the next generate.
fn emit_sv_dispatch_precheck(
    s: &mut String,
    func: &FuncDef,
    funcs: &[FuncDef],
    input_arrays: &[&str],
    n_outs: usize,
    name: &str,
) {
    let Some(guard) = sv_reject_condition(func, funcs, None) else {
        return;
    };
    let mut pre_opt_args = String::new();
    for o in &func.optional_inputs {
        let _ = std::fmt::Write::write_fmt(&mut pre_opt_args, format_args!("{}, ", o.name));
    }
    let mut pre_in_args = String::new();
    for a in input_arrays {
        let _ = std::fmt::Write::write_fmt(&mut pre_in_args, format_args!("{a}, "));
    }
    let decls: String = func
        .outputs
        .iter()
        .enumerate()
        .map(|(i, ou)| {
            if ou.param_type == ParamType::Integer {
                format!("int v{i} = 0;")
            } else {
                format!("double v{i} = 0.0;")
            }
        })
        .collect::<Vec<_>>()
        .join(" ");
    let addrs = (0..n_outs)
        .map(|i| format!("&v{i}"))
        .collect::<Vec<_>>()
        .join(", ");
    // An unsupported arm (MAMA) must reject at OpenAndFill too, not just Open —
    // otherwise a regression could return SUCCESS with an unwritten output array
    // (silent garbage) on the exact param the header documents as rejected.
    // Every streamable function has an OpenAndFill, so this is unconditional.
    let fill_block = {
        let (mut ri, mut ii) = (0usize, 0usize);
        let fill_bufs: String = func
            .outputs
            .iter()
            .map(|ou| {
                if ou.param_type == ParamType::Integer {
                    let e = format!("sv_if{ii}");
                    ii += 1;
                    e
                } else {
                    let e = format!("sv_f{ri}");
                    ri += 1;
                    e
                }
            })
            .collect::<Vec<_>>()
            .join(", ");
        format!(
            "            {{ TA_{name}_Stream *stf = NULL; int fBeg = 0, fNb = 0;\n              TA_RetCode frc = TA_{name}_OpenAndFill( &stf, {pre_in_args}svN, {pre_opt_args}&fBeg, &fNb, {fill_bufs} );\n              if( !( frc != TA_SUCCESS && !stf ) ) rejected = 0;\n              if( stf ) TA_{name}_Close( stf ); }}\n"
        )
    };
    s.push_str(&format!(
        "        if( {guard} )\n        {{\n            TA_{name}_Stream *st = NULL; {decls} TA_RetCode orc;\n            int rejected;\n            orc = TA_{name}_Open( &st, {pre_in_args}svN, {pre_opt_args}{addrs} );\n            rejected = ( orc != TA_SUCCESS && !st ) ? 1 : 0;\n            if( st ) TA_{name}_Close( st );\n{fill_block}            snprintf(resp, resp_size, \"{{\\\"retCode\\\":0,\\\"legs\\\":0,\\\"unsupportedArm\\\":1,\\\"ok\\\":%d,\\\"peek_ok\\\":1}}\", rejected);\n            return;\n        }}\n"
    ));
}

/// Stamp the canary across the FULL width of the C fill buffers.
///
/// Not `svN`: a lookback-0 function fills the whole series, so `fNb == svN` and
/// a `[fNb, svN)` window is empty — the assert would be a no-op for exactly the
/// functions whose overrun has the furthest to travel. A one-past-the-range
/// write also lands at index `svN` itself. Stamp and assert must use the same
/// bound; widening only the assert would read whatever an earlier function in
/// the same request left in these `static` buffers and fail spuriously.
fn c_canary_stamp(fbuf: &[String], out_is_int: &[bool]) -> String {
    let mut s = String::from("            for( ft = 0; ft < SV_MAXN; ft++ ) {\n");
    for (i, is_int) in out_is_int.iter().enumerate() {
        let canary = if *is_int { "SV_FILL_CANARY_I" } else { "SV_FILL_CANARY" };
        let _ = writeln!(s, "               {}[ft] = {canary};", fbuf[i]);
    }
    s.push_str("            }\n");
    s
}

/// Assert the slack above the produced range still holds the canary. Nothing
/// else in the tree checks it: every gate sizes the fill buffer at full history
/// and compares only `[0, nb)`, so a write past `nb` lands in unread space.
fn c_canary_check(fbuf: &[String], out_is_int: &[bool]) -> String {
    let mut s = String::from("            if( frc == TA_SUCCESS )\n");
    s.push_str("               for( ft = fNb; fillOk && ft < SV_MAXN; ft++ ) {\n");
    for (i, is_int) in out_is_int.iter().enumerate() {
        let canary = if *is_int { "SV_FILL_CANARY_I" } else { "SV_FILL_CANARY" };
        let _ = writeln!(s, "                  if( {}[ft] != {canary} ) fillOk = 0;", fbuf[i]);
    }
    s.push_str("               }\n");
    s
}

// ---------------------------------------------------------------------------
// State-equivalence comparators (issue #240)
// ---------------------------------------------------------------------------
//
// A candlestick's only observable is a 3-valued integer, so an arithmetic error
// inside a `<Setting>PeriodTotal` stays invisible until it crosses a decision
// threshold. Measured on the #229 window fold: a permanent one-bar rotation of a
// folded ring read moved the OUTPUT of 3 of 14 functions and left 11 green in
// every language. It moves the STATE of all 14, on the first bar it is read.
//
// So compare state, not output: the handle after `Open(P)` plus `n - P` updates
// must equal the handle after `Open(n)`, bit for bit. That holds by
// construction — `Update` is the transcribed batch loop body, so both paths run
// the identical operation sequence over the identical bars, and the ring cursor
// is `historyLen % cap` on one side against the same number of `+1`s on the
// other. The property is independent of firing density and of decision margin,
// which is exactly what the output comparison is not.
//
// The comparator is derived from `c_stream::state_struct_text` — the same text
// the shipped struct is emitted from — so it cannot fall behind a new field, and
// a pointer field it cannot associate with a length is a hard generation
// failure, never a silent skip.

/// How the state comparator treats one pointer field of a stream struct.
enum SvPtr {
    /// `count` elements. With `phase`, the buffer is a ring and the compare is
    /// LOGICAL: slot `k` of one handle is `(phase + k) % count`, so two handles
    /// holding the same bars at different rotations compare equal.
    ///
    /// They legitimately do. The trailing-ring tiers capture in two different
    /// layouts: the absolute-mod one (`back > 0`) seeds `ringPos = historyLen %
    /// cap`, which the updates reproduce exactly, but the plain oldest-slot one
    /// re-bases every open to phase 0 (`memcpy` of the last `cap` bars,
    /// `ringPos = 0`) while an update just advances the cursor. Nothing can
    /// observe the difference — every read is relative to `ringPos` — so
    /// comparing raw slots would fail 90 of 175 functions on the rotation
    /// alone. Rotating by each handle's own phase still catches a cursor that
    /// advances wrongly: that misaligns the CONTENT, which is what is compared.
    Slots { count: String, is_int: bool, phase: Option<String> },
    /// The extrema automaton's absolute-index buffer. Only the `cap` bars
    /// `[trailing-1, trailing-1+cap)` are ever written, at `bar & mask`; the
    /// allocation is the next power of two, so the slack above the window holds
    /// whatever malloc returned and must not be read. (`trailing - 1`, not
    /// `trailing`: `xCap` is `today - trailing + 1` captured with `today`
    /// already advanced past the last bar, so the filled range is
    /// `[historyLen - xCap, historyLen)`. `+ phys` keeps the index
    /// non-negative when `trailing` is 0.)
    Extrema { cap: String, trailing: String, mask: String, phys: String },
    /// A typed sub-stream handle: recurse into the callee's comparator.
    Sub { callee: String },
    /// The dispatch tier's untyped handle, tagged by an enum param.
    DispatchSub { tag: String, arms: Vec<(String, String)> },
    /// MAVP's bank of `count` sub-handles.
    Bank { count: String, callee: String },
}

/// One parsed field declaration from a `struct TA_<N>_Stream` body.
struct SvDecl {
    name: String,
    /// `*` count on the declarator (0 for a scalar).
    ptr: usize,
    /// Array extent; 1 for a plain scalar.
    len: usize,
    is_int: bool,
}

/// Parse the emitted struct body into field declarations.
///
/// Every line is the opening/closing brace, a comment, or
/// `   <type> [*...]<name>[[N]];`. Anything else is a tier change this code has
/// not been taught: panic rather than silently drop a field from the compare.
fn sv_parse_state_struct(name: &str, text: &str) -> Vec<SvDecl> {
    let mut out = Vec::new();
    let mut in_comment = false;
    for raw in text.lines() {
        let line = raw.trim();
        // The opening line, not every line starting with `struct` — MAVP's bank
        // is declared `struct TA_MA_Stream **bank;` and skipping it here left
        // the bank uncompared (caught by the claimed-set assert below).
        if line.is_empty() || line.ends_with('{') || line == "};" {
            continue;
        }
        if in_comment {
            in_comment = !line.contains("*/");
            continue;
        }
        if line.starts_with("/*") {
            in_comment = !line.contains("*/");
            continue;
        }
        // A trailing comment on the declaration line (`int unused; /* ... */`).
        let line = match line.find("/*") {
            Some(at) => line[..at].trim_end(),
            None => line,
        };
        let decl = line
            .strip_suffix(';')
            .unwrap_or_else(|| panic!("{name}: unparsable stream state line `{line}`"));
        let mut tokens: Vec<&str> = decl.split_whitespace().collect();
        let mut declarator = tokens.pop().unwrap_or_default().to_string();
        let ptr = declarator.chars().take_while(|c| *c == '*').count();
        declarator = declarator.trim_start_matches('*').to_string();
        let mut len = 1usize;
        if let Some(open) = declarator.find('[') {
            let close = declarator
                .find(']')
                .unwrap_or_else(|| panic!("{name}: unparsable declarator `{declarator}`"));
            len = declarator[open + 1..close]
                .parse()
                .unwrap_or_else(|_| panic!("{name}: non-literal extent in `{declarator}`"));
            declarator.truncate(open);
        }
        let ty = tokens.join(" ");
        assert!(
            !declarator.is_empty() && !ty.is_empty(),
            "{name}: unparsable stream state line `{line}`"
        );
        // `double` and `int` are the only scalar storages the tiers emit; an
        // enum param (TA_MAType) compares like an int.
        out.push(SvDecl { name: declarator, ptr, len, is_int: ty != "double" });
    }
    out
}

/// Pointer roles contributed by one loop model's buffers.
fn sv_model_ptrs(
    model: &crate::streaming::StreamModel,
    roles: &mut BTreeMap<String, SvPtr>,
    phases: &mut BTreeSet<String>,
) {
    for r in model.rings() {
        let cap = format!("ringCap_{}", r.var);
        let pos = format!("ringPos_{}", r.var);
        phases.insert(pos.clone());
        for arr in &r.arrays {
            roles.insert(
                format!("ring_{}_{arr}", r.var),
                SvPtr::Slots { count: cap.clone(), is_int: false, phase: Some(pos.clone()) },
            );
        }
    }
    for w in model.windows() {
        let cap = format!("winCap_{}", w.var);
        let pos = format!("winPos_{}", w.var);
        phases.insert(pos.clone());
        for arr in &w.arrays {
            roles.insert(
                format!("win_{}_{arr}", w.var),
                SvPtr::Slots { count: cap.clone(), is_int: false, phase: Some(pos.clone()) },
            );
        }
    }
    for c in model.circs() {
        // No phase: a CIRCBUF is captured LIVE from the batch — contents AND
        // rotation — so both opens agree on the raw slots, and the rotation
        // index is an ordinary state scalar that is compared as one.
        let size = format!("cbSize_{}", c.id);
        for (storage, ty) in crate::streaming::circ_storages(c) {
            let is_int = matches!(ty, crate::ir::VarType::Integer);
            roles.insert(
                format!("cb_{storage}"),
                SvPtr::Slots { count: size.clone(), is_int, phase: None },
            );
        }
    }
    if let Some(ex) = model.extrema() {
        for arr in &ex.arrays {
            roles.insert(
                format!("x_{arr}"),
                SvPtr::Extrema {
                    cap: "xCap".to_string(),
                    trailing: ex.trailing.clone(),
                    mask: "xMask".to_string(),
                    phys: "xPhys".to_string(),
                },
            );
        }
    }
}

/// Pointer roles for one streaming function, keyed by field name.
fn sv_ptr_roles(
    func: &FuncDef,
    funcs: &[FuncDef],
) -> (BTreeMap<String, SvPtr>, BTreeSet<String>) {
    use crate::streaming::{FuncsLookup, StreamPlan};
    let resolved = func.resolved_for(crate::ir::Lang::C);
    let plan = crate::streaming::validate_streamable(&resolved, &FuncsLookup(funcs))
        .unwrap_or_else(|e| panic!("streaming gate: {e}"));
    let mut roles: BTreeMap<String, SvPtr> = BTreeMap::new();
    let mut phases: BTreeSet<String> = BTreeSet::new();
    match &plan {
        StreamPlan::Loop(model) => sv_model_ptrs(model, &mut roles, &mut phases),
        StreamPlan::DualMode(dmp) => {
            sv_model_ptrs(&dmp.mode_a, &mut roles, &mut phases);
            sv_model_ptrs(&dmp.mode_b, &mut roles, &mut phases);
        }
        StreamPlan::Composed(cp) => {
            if let Some(model) = &cp.producer {
                sv_model_ptrs(model, &mut roles, &mut phases);
            }
            for (i, sub) in cp.subs.iter().enumerate() {
                roles.insert(format!("sub{i}"), SvPtr::Sub { callee: sub.callee.to_uppercase() });
            }
            for ring in &cp.sub_lag_rings {
                let s = &ring.series;
                phases.insert(format!("lagRingPos_{s}"));
                roles.insert(
                    format!("lagRing_{s}"),
                    SvPtr::Slots {
                        count: format!("lagRingCap_{s}"),
                        is_int: false,
                        phase: Some(format!("lagRingPos_{s}")),
                    },
                );
            }
        }
        StreamPlan::Dispatch(dp) => {
            roles.insert(
                "sub".to_string(),
                SvPtr::DispatchSub {
                    tag: dp.param.clone(),
                    arms: dp
                        .arms
                        .iter()
                        .filter(|a| a.supported && !a.callee.is_empty())
                        .map(|a| (a.label.clone(), a.callee.to_uppercase()))
                        .collect(),
                },
            );
        }
        StreamPlan::PeriodBank(pbp) => {
            roles.insert(
                "bank".to_string(),
                SvPtr::Bank { count: "nBank".to_string(), callee: pbp.callee.to_uppercase() },
            );
            roles.insert(
                "scratch".to_string(),
                SvPtr::Slots { count: "nBank".to_string(), is_int: false, phase: None },
            );
        }
    }
    (roles, phases)
}

/// One function's comparator body plus the callees it recurses into.
struct SvComparator {
    body: String,
    deps: Vec<String>,
}

/// Bitwise compare of one scalar pair, as a C condition.
fn sv_ne(lhs: &str, rhs: &str, is_int: bool) -> String {
    if is_int {
        format!("{lhs} != {rhs}")
    } else {
        // Same rule the batch-vs-stream value legs use: differing bits that are
        // numerically equal can only be +0.0 vs -0.0, which max/min leave
        // unspecified. Counted as benign, never a mismatch (#147).
        format!("sv_xtier_ne({lhs}, {rhs}, z)")
    }
}

/// Build the comparator for one streaming function.
#[allow(clippy::too_many_lines)]
fn sv_comparator(func: &FuncDef, funcs: &[FuncDef], enums: &HashMap<String, EnumDef>) -> SvComparator {
    use crate::backends::c::render_c_switch_label;
    use crate::backends::c_stream;
    use crate::streaming::FuncsLookup;

    let name = &func.name;
    let text = c_stream::state_struct_text(func, &FuncsLookup(funcs));
    let decls = sv_parse_state_struct(name, &text);
    let (roles, phases) = sv_ptr_roles(func, funcs);
    let mut claimed: BTreeSet<&str> = BTreeSet::new();
    let mut body = String::new();
    let mut deps: Vec<String> = Vec::new();

    for d in &decls {
        let n = &d.name;
        if d.ptr == 0 {
            // A ring cursor is the buffer's phase, not state in its own right:
            // the two opens seed it differently by design and the rotated
            // buffer compare below is what actually pins the ring.
            if phases.contains(n.as_str()) {
                continue;
            }
            if d.len > 1 {
                let _ = writeln!(
                    body,
                    "   for( k = 0; k < {}; k++ ) if( {} ) {{ *w = \"{n}\"; return 1; }}",
                    d.len,
                    sv_ne(&format!("a->{n}[k]"), &format!("b->{n}[k]"), d.is_int)
                );
            } else {
                let _ = writeln!(
                    body,
                    "   if( {} ) {{ *w = \"{n}\"; return 1; }}",
                    sv_ne(&format!("a->{n}"), &format!("b->{n}"), d.is_int)
                );
            }
            continue;
        }
        let role = roles.get(n.as_str()).unwrap_or_else(|| {
            panic!(
                "TA_{name}: stream state pointer `{n}` has no state-equivalence rule. \
                 A new heap field must say how many elements it carries, or the #240 \
                 state leg would silently stop comparing it."
            )
        });
        claimed.insert(n.as_str());
        let guard = format!(
            "   if( (a->{n} == NULL) != (b->{n} == NULL) ) {{ *w = \"{n}\"; return 1; }}\n"
        );
        match role {
            SvPtr::Slots { count, is_int, phase } => {
                body.push_str(&guard);
                match phase {
                    None => {
                        let _ = writeln!(
                            body,
                            "   if( a->{n} ) for( k = 0; k < a->{count}; k++ ) if( {} ) {{ *w = \"{n}\"; return 1; }}",
                            sv_ne(&format!("a->{n}[k]"), &format!("b->{n}[k]"), *is_int)
                        );
                    }
                    Some(pos) => {
                        let _ = writeln!(body, "   if( a->{n} ) for( k = 0; k < a->{count}; k++ )");
                        let _ = writeln!(body, "   {{");
                        let _ = writeln!(body, "      ia = (a->{pos} + k) % a->{count};");
                        let _ = writeln!(body, "      ib = (b->{pos} + k) % b->{count};");
                        let _ = writeln!(
                            body,
                            "      if( {} ) {{ *w = \"{n}\"; return 1; }}",
                            sv_ne(&format!("a->{n}[ia]"), &format!("b->{n}[ib]"), *is_int)
                        );
                        let _ = writeln!(body, "   }}");
                    }
                }
            }
            SvPtr::Extrema { cap, trailing, mask, phys } => {
                body.push_str(&guard);
                let _ = writeln!(body, "   if( a->{n} ) for( k = 0; k < a->{cap}; k++ )");
                let _ = writeln!(body, "   {{");
                let _ = writeln!(body, "      ix = (a->{trailing} - 1 + a->{phys} + k) & a->{mask};");
                let _ = writeln!(
                    body,
                    "      if( {} ) {{ *w = \"{n}\"; return 1; }}",
                    sv_ne(&format!("a->{n}[ix]"), &format!("b->{n}[ix]"), false)
                );
                let _ = writeln!(body, "   }}");
            }
            SvPtr::Sub { callee } => {
                deps.push(callee.clone());
                body.push_str(&guard);
                let _ = writeln!(
                    body,
                    "   if( a->{n} && sv_steq_TA_{callee}( a->{n}, b->{n}, w, z ) ) return 1;"
                );
            }
            SvPtr::DispatchSub { tag, arms } => {
                body.push_str(&guard);
                let _ = writeln!(body, "   if( a->{n} )");
                let _ = writeln!(body, "      switch( a->{tag} )");
                let _ = writeln!(body, "      {{");
                for (label, callee) in arms {
                    deps.push(callee.clone());
                    let case = render_c_switch_label(label, enums);
                    let cty = format!("const struct TA_{callee}_Stream *");
                    let _ = writeln!(body, "      case {case}:");
                    let _ = writeln!(
                        body,
                        "         if( sv_steq_TA_{callee}( ({cty})a->{n}, ({cty})b->{n}, w, z ) ) return 1;"
                    );
                    let _ = writeln!(body, "         break;");
                }
                let _ = writeln!(body, "      default:");
                let _ = writeln!(body, "         *w = \"{n}\"; return 1;");
                let _ = writeln!(body, "      }}");
            }
            SvPtr::Bank { count, callee } => {
                deps.push(callee.clone());
                body.push_str(&guard);
                let _ = writeln!(body, "   if( a->{n} ) for( k = 0; k < a->{count}; k++ )");
                let _ = writeln!(body, "   {{");
                let _ = writeln!(
                    body,
                    "      if( (a->{n}[k] == NULL) != (b->{n}[k] == NULL) ) {{ *w = \"{n}\"; return 1; }}"
                );
                let _ = writeln!(
                    body,
                    "      if( a->{n}[k] && sv_steq_TA_{callee}( a->{n}[k], b->{n}[k], w, z ) ) return 1;"
                );
                let _ = writeln!(body, "   }}");
            }
        }
    }

    // Every pointer role the model produced must have matched a declared field.
    // The reverse direction (a declared pointer with no role) panics above; this
    // catches a spec whose field name the struct emitter spells differently,
    // which would leave the buffer uncompared without either side noticing.
    for key in roles.keys() {
        assert!(
            claimed.contains(key.as_str()),
            "TA_{name}: state-equivalence rule for `{key}` matches no field in \
             `struct TA_{name}_Stream` — the rule and the struct emitter have drifted."
        );
    }

    SvComparator { body, deps }
}

/// Every state-equivalence comparator, plus the names of the functions that
/// have one.
///
/// A comparator that recurses into a sub-handle needs its callee's comparator,
/// so the set closes under a fixpoint: drop any function whose dependency was
/// dropped, until nothing moves. The C server's leg is emitted only for what
/// survives, and `ta_regtest` ratchets the surviving count against the number
/// of streaming functions, so a shrinking set fails rather than going quiet.
fn generate_c_state_eq(
    funcs: &[FuncDef],
    enums: &HashMap<String, EnumDef>,
) -> (String, BTreeSet<String>) {
    let mut comps: BTreeMap<String, SvComparator> = BTreeMap::new();
    for f in funcs.iter().filter(|f| f.streaming) {
        comps.insert(f.name.clone(), sv_comparator(f, funcs, enums));
    }
    let mut have: BTreeSet<String> = comps.keys().cloned().collect();
    loop {
        let dropped: Vec<String> = have
            .iter()
            .filter(|n| comps[*n].deps.iter().any(|d| !have.contains(d)))
            .cloned()
            .collect();
        if dropped.is_empty() {
            break;
        }
        for d in dropped {
            have.remove(&d);
        }
    }

    let mut s = String::new();
    s.push_str("/* ---- state-equivalence comparators (issue #240) ----\n");
    s.push_str(" * `Open(P)` + (n-P) updates must leave the handle bit-identical to\n");
    s.push_str(" * `Open(n)`. Compares every carried field; skips the Peek scratch mirrors\n");
    s.push_str(" * (written only inside Peek) and, for the extrema automaton, the slack\n");
    s.push_str(" * above the live window (never written, so it holds malloc leftovers).\n");
    s.push_str(" * Returns 1 and names the field on the first difference. */\n");
    for n in &have {
        let _ = writeln!(
            s,
            "static int sv_steq_TA_{n}( const struct TA_{n}_Stream *a, const struct TA_{n}_Stream *b, const char **w, int *z );"
        );
    }
    s.push('\n');
    for n in &have {
        let _ = writeln!(
            s,
            "static int sv_steq_TA_{n}( const struct TA_{n}_Stream *a, const struct TA_{n}_Stream *b, const char **w, int *z )\n{{"
        );
        s.push_str("   int k = 0, ix = 0, ia = 0, ib = 0;\n");
        s.push_str("   (void)a; (void)b; (void)w; (void)z; (void)k; (void)ix; (void)ia; (void)ib;\n");
        s.push_str(&comps[n].body);
        s.push_str("   return 0;\n}\n\n");
    }
    (s, have)
}

// --- the state-equivalence leg's five emission points (issue #240) ----------
//
// Each takes the `steq` flag and returns early rather than being called under
// an `if`: `generate_c_stream_verify` is already at clippy's cognitive-
// complexity ceiling, and five more branches in it push it over.

/// The leg's per-function locals: one reference handle over the whole history,
/// plus the verdict it produces. Reopened per candle round, because the
/// settings a round installs are part of what the state encodes.
/// The range leg (issue #241): a handle's `OutRange` must equal what the batch
/// call reports over the same bars, whichever opener produced it and however
/// many updates followed. Unlike the state leg this needs no private struct and
/// no second reference handle — the range is public API in all four backends and
/// the batch pair is already in scope — so it runs in every language server.
fn emit_sv_range_decls(s: &mut String) {
    s.push_str("        int rangeChecked = 0, rangeOk = 1, rangeLegs = 0, rangeSites = 0;\n");
    s.push_str("        int rB = 0, rN = 0;\n");
}

/// One comparison: `handle`'s range against the `(beg, nb)` the batch reported
/// for the same bars. `guard` is the leg's own success condition — a leg that
/// already failed has a handle short of the bars it was supposed to consume.
fn emit_sv_range_check(
    s: &mut String, name: &str, indent: &str, handle: &str, guard: &str, beg: &str, nb: &str,
    site: SvRangeSite,
) {
    let _ = writeln!(s, "{indent}if( {guard} )");
    let _ = writeln!(s, "{indent}{{");
    let _ = writeln!(
        s,
        "{indent}    rangeChecked = 1; rangeLegs++; rangeSites |= {};",
        sv_range_bit(site, SV_RANGE_MASK_C)
    );
    let _ = writeln!(s, "{indent}    rB = -1; rN = -1;");
    let _ = writeln!(
        s,
        "{indent}    if( TA_{name}_OutRange( {handle}, &rB, &rN ) != TA_SUCCESS || rB != {beg} || rN != {nb} ) rangeOk = 0;"
    );
    let _ = writeln!(s, "{indent}}}");
}

/// Folded into `ok` like the fill and state legs, so a driver check that ever
/// regresses still fails the run.
fn emit_sv_range_report(s: &mut String) {
    s.push_str("        if( rangeChecked && !rangeOk ) allOk = 0;\n");
    let _ = writeln!(
        s,
        "        pos = json_appendf(resp, resp_size, pos, \",\\\"range_checked\\\":%d,\\\"range_legs\\\":%d,\\\"range_sites\\\":%d,\\\"range_sites_all\\\":{SV_RANGE_MASK_C},\\\"range_ok\\\":%d\", rangeChecked, rangeLegs, rangeSites, rangeOk);"
    );
}

fn emit_sv_state_decls(s: &mut String, name: &str, steq: bool) {
    if !steq {
        return;
    }
    s.push_str("        int stateChecked = 0, stateOk = 1, stateLegs = 0;\n");
    s.push_str("        const char *stateWhat = \"-\";\n");
    let _ = writeln!(s, "        TA_{name}_Stream *stEq = NULL;");
}

/// `Open(svN)` -- the handle every prefix leg is compared against.
fn emit_sv_state_open(
    s: &mut String,
    name: &str,
    steq: bool,
    out_is_int: &[bool],
    in_args: &str,
    opt_args: &str,
) {
    if !steq {
        return;
    }
    let eouts: String = out_is_int
        .iter()
        .enumerate()
        .map(|(i, is_int)| if *is_int { format!("int e{i} = 0;") } else { format!("double e{i} = 0.0;") })
        .collect::<Vec<_>>()
        .join(" ");
    let eaddrs: String = (0..out_is_int.len())
        .map(|i| format!("&e{i}"))
        .collect::<Vec<_>>()
        .join(", ");
    s.push_str("        {\n");
    let _ = writeln!(s, "            {eouts}");
    let _ = writeln!(
        s,
        "            if( TA_{name}_Open( &stEq, {in_args}svN, {opt_args}{eaddrs} ) != TA_SUCCESS ) stEq = NULL;"
    );
    s.push_str("        }\n");
}

/// A repeated peek answers the same bits — `peek(t)`, `peek(t-1)`, `peek(t)`
/// with no update in between.
///
/// The middle call is the whole point. Every other peek leg hands `peek` the
/// arguments the `update` right after it gets, so a peek that committed what
/// that update was about to commit is invisible to all of them; here the
/// decoy moves the handle if anything moves it, and the third call reads the
/// damage back as a VALUE. That makes it the only cross-language observer of a
/// committing peek — the twin-handle state leg is C-only, and it is the C
/// server alone that owns a state comparator.
///
/// `t - 1` is in range: the sweep starts at `P >= lb + 1 >= 1`, and it is a real
/// bar of the same generated series, so the OHLC bundle stays coherent.
///
/// Its components being finite is NOT enough to make the peek succeed, which is
/// the trap this leg was written around and got wrong. A composed stream feeds a
/// DERIVED intermediate into a sub-stream's public peek, and that value can be
/// non-finite for finite inputs — measured: `MACDEXT` with `optInSlowMAType`
/// KAMA at period 2 on the wide-magnitude shape hands the signal sub-stream an
/// infinity, which it refuses. So the probe must expect a refusal, and a refusal
/// writes nothing: comparing anyway reads the PREVIOUS probe's values, which is
/// a false verdict in either direction.
///
/// A refusal is counted, never failed. Nothing here is a library defect — the
/// bar is one the batch never evaluates, and over 264 (shape, seed, MAType)
/// configurations no real bar was ever refused by `Peek` or `Update`. What keeps
/// the leg honest when refusals are skipped is the driver's own floor: every
/// streaming function must report a non-zero `peek_reps`.
fn emit_sv_peek_repeat_probe(
    s: &mut String,
    name: &str,
    out_is_int: &[bool],
    pad: &str,
    bar_args: &str,
    decoy_args: &str,
    rpout_args: &str,
) {
    let ne: Vec<String> = out_is_int
        .iter()
        .enumerate()
        .map(|(i, is_int)| {
            if *is_int { format!("(rp{i} != pk{i})") } else { format!("sv_bitne(rp{i}, pk{i})") }
        })
        .collect();
    let _ = writeln!(s, "{pad}if( (t % SV_PEEK_EVERY) == 0 )");
    let _ = writeln!(s, "{pad}{{");
    let _ = writeln!(
        s,
        "{pad}   if( TA_{name}_Peek(st, {decoy_args}{rpout_args}) != TA_SUCCESS ) peekRejects++;"
    );
    let _ = writeln!(
        s,
        "{pad}   if( pkRc == TA_SUCCESS && TA_{name}_Peek(st, {bar_args}{rpout_args}) == TA_SUCCESS )"
    );
    let _ = writeln!(s, "{pad}   {{");
    let _ = writeln!(s, "{pad}      peekReps++;");
    let _ = writeln!(s, "{pad}      if( {} ) peekRepAll = 0;", ne.join(" || "));
    let _ = writeln!(s, "{pad}   }}");
    let _ = writeln!(s, "{pad}   else peekRejects++;");
    let _ = writeln!(s, "{pad}}}");
}

/// Peek commits nothing: a handle peeked over the history is bit-identical to a
/// twin that was not.
///
/// This is the leg that has to carry the property, and the reason is that the
/// value legs structurally CANNOT. They peek bar t and then update bar t with
/// the same arguments, so a peek that stored what the update was about to store
/// leaves the values and the end-of-run state compare alike — a sabotage
/// committing every shadow in 107 of 176 peek entry points passed the whole
/// sweep green. Peeking a handle nobody then updates is what makes the store
/// survive to be seen.
///
/// `stEq` is the untouched twin: `Open(svN)`, already opened for the state leg
/// and only ever read, so this costs one extra open per request rather than one
/// per leg.
fn emit_sv_peek_noncommit(
    s: &mut String,
    name: &str,
    steq: bool,
    out_is_int: &[bool],
    in_args: &str,
    opt_args: &str,
    input_arrays: &[&str],
) {
    if !steq {
        return;
    }
    let pouts: String = out_is_int
        .iter()
        .enumerate()
        .map(|(i, is_int)| if *is_int { format!("int q{i} = 0;") } else { format!("double q{i} = 0.0;") })
        .collect::<Vec<_>>()
        .join(" ");
    let paddrs: String = (0..out_is_int.len())
        .map(|i| format!("&q{i}"))
        .collect::<Vec<_>>()
        .join(", ");
    let mut bar_args = String::new();
    for a in input_arrays {
        bar_args.push_str(&format!("{a}[pi], "));
    }
    s.push_str("        if( stEq )
        {
");
    let _ = writeln!(s, "            TA_{name}_Stream *stPk = NULL; {pouts}");
    let _ = writeln!(
        s,
        "            if( TA_{name}_Open( &stPk, {in_args}svN, {opt_args}{paddrs} ) == TA_SUCCESS && stPk )"
    );
    s.push_str("            {
");
    s.push_str("                int pi;
");
    s.push_str("                for( pi = svBeg; pi < svN; pi += SV_PEEK_EVERY )
");
    s.push_str("                {
");
    let _ = writeln!(
        s,
        "                    if( TA_{name}_Peek(stPk, {bar_args}{paddrs}) == TA_SUCCESS ) peekChecked++;"
    );
    s.push_str("                    else peekRejects++;
");
    s.push_str("                }
");
    s.push_str("                {
");
    s.push_str("                    const char *pkWhat = \"-\";\n");
    let _ = writeln!(
        s,
        "                    if( sv_steq_TA_{name}( stPk, stEq, &pkWhat, &svZsign ) ) {{ peekAll = 0; peekBad = pkWhat; }}"
    );
    s.push_str("                }
");
    s.push_str("            }
");
    let _ = writeln!(s, "            if( stPk ) TA_{name}_Close(stPk);");
    s.push_str("        }
");
}

/// One `TA_<N>_Value` probe: the accessor must answer `expect[i]` for every
/// output, on the handle named by `handle`.
///
/// `TA_<N>_Value` retains a SEPARATE copy of the output — it is not the sink the
/// step writes — so every site that seeds or refreshes `cur_` needs a probe of
/// its own or it ships unread. The open-time seed, the strided `OpenAndFill`
/// seed and the `Update` retain each write a different expression, and none is
/// observable through any other leg.
fn emit_sv_value_probe(
    s: &mut String,
    name: &str,
    out_is_int: &[bool],
    pad: &str,
    handle: &str,
    expect: &[String],
    site: &str,
) {
    let n = out_is_int.len();
    let decls: String = out_is_int
        .iter()
        .enumerate()
        .map(|(i, b)| if *b { format!("int vq{i} = 0;") } else { format!("double vq{i} = 0.0;") })
        .collect::<Vec<_>>()
        .join(" ");
    let addrs: String = (0..n).map(|i| format!("&vq{i}")).collect::<Vec<_>>().join(", ");
    let _ = writeln!(s, "{pad}{{");
    let _ = writeln!(s, "{pad}   {decls}");
    let _ = writeln!(s, "{pad}   valueChecked = 1; valueLegs++;");
    let _ = writeln!(
        s,
        "{pad}   if( TA_{name}_Value( {handle}, {addrs} ) != TA_SUCCESS ) {{ valueOk = 0; valueBad = \"{site}: Value rejected a live stream\"; }}"
    );
    for (i, is_int) in out_is_int.iter().enumerate() {
        let cmp = if *is_int {
            format!("vq{i} != {}", expect[i])
        } else {
            format!("sv_bitne(vq{i}, {})", expect[i])
        };
        let _ = writeln!(
            s,
            "{pad}   if( {cmp} ) {{ valueOk = 0; valueBad = \"{site}\"; }}"
        );
    }
    let _ = writeln!(s, "{pad}}}");
}

/// Clone-independence leg (#287): open at the earliest prefix, advance to mid,
/// fork with `TA_<N>_Clone`, then drive BOTH to the end.
///
/// Two claims, and the second is the one that needs the whole run. Same-tier:
/// the fork and the original agree with each other bitwise — no `+/-0` fold,
/// they ran the identical arithmetic. Cross-tier: each agrees with batch.
///
/// **Why it drives to the end rather than a bar or two.** A fork that SHARES a
/// ring is latent for exactly one bar: the step reads the trailing slot before
/// it overwrites it, so the first update after the fork still answers correctly
/// on both handles and only the running total is left wrong. The divergence
/// surfaces on the next bar. A leg that stopped early would read green on the
/// defect it exists to catch.
///
/// It also checks `TA_<N>_Value` on the fork, because a fork is the one caller
/// that has no earlier call to have handed it a value — the case the accessor
/// was added for.
fn emit_sv_clone_leg(
    s: &mut String,
    name: &str,
    input_arrays: &[&str],
    in_args: &str,
    opt_args: &str,
    out_is_int: &[bool],
    bbuf: &[String],
) {
    let n = out_is_int.len();
    let decl_a: String = out_is_int
        .iter()
        .enumerate()
        .map(|(i, b)| if *b { format!("int ca{i} = 0;") } else { format!("double ca{i} = 0.0;") })
        .collect::<Vec<_>>()
        .join(" ");
    let decl_b: String = out_is_int
        .iter()
        .enumerate()
        .map(|(i, b)| if *b { format!("int cb{i} = 0;") } else { format!("double cb{i} = 0.0;") })
        .collect::<Vec<_>>()
        .join(" ");
    let decl_v: String = out_is_int
        .iter()
        .enumerate()
        .map(|(i, b)| if *b { format!("int cv{i} = 0;") } else { format!("double cv{i} = 0.0;") })
        .collect::<Vec<_>>()
        .join(" ");
    let addr_a: String = (0..n).map(|i| format!("&ca{i}")).collect::<Vec<_>>().join(", ");
    let addr_b: String = (0..n).map(|i| format!("&cb{i}")).collect::<Vec<_>>().join(", ");
    let addr_v: String = (0..n).map(|i| format!("&cv{i}")).collect::<Vec<_>>().join(", ");
    let mut bar_args = String::new();
    for a in input_arrays {
        bar_args.push_str(&format!("{a}[t], "));
    }

    s.push_str("        {\n");
    let _ = writeln!(s, "            TA_{name}_Stream *cA = NULL, *cB = NULL;");
    let _ = writeln!(s, "            {decl_a} {decl_b} {decl_v}");
    // The earliest prefix the opener accepts — the same one the prefix leg uses.
    s.push_str("            int cp0 = lb + 1, cmid, t, cOk = 1;\n");
    s.push_str("            if( cp0 <= svN - 1 )\n            {\n");
    let _ = writeln!(
        s,
        "                if( TA_{name}_Open(&cA, {in_args}cp0, {opt_args}{addr_a}) != TA_SUCCESS || !cA ) {{ cOk = 0; cloneBad = \"open rejected the fork leg's prefix\"; }}"
    );
    s.push_str("                cmid = (cp0 + svN) / 2;\n");
    s.push_str("                for( t = cp0; cOk && t < cmid; t++ )\n");
    let _ = writeln!(s, "                    TA_{name}_Update(cA, {bar_args}{addr_a});");
    s.push_str("                if( cOk )\n                {\n");
    let _ = writeln!(
        s,
        "                    if( TA_{name}_Clone(cA, &cB) != TA_SUCCESS || !cB ) {{ cOk = 0; cloneBad = \"clone rejected\"; }}"
    );
    s.push_str("                    else if( cB == cA ) { cOk = 0; cloneBad = \"clone returned the original\"; }\n");
    s.push_str("                }\n");
    // The fork must answer for the bar it was forked at, with no call of its own.
    s.push_str("                if( cOk )\n                {\n");
    let _ = writeln!(s, "                    if( TA_{name}_Value(cB, {addr_v}) != TA_SUCCESS ) {{ cOk = 0; cloneBad = \"Value rejected the fork\"; }}");
    for (i, is_int) in out_is_int.iter().enumerate() {
        let cmp = if *is_int { format!("cv{i} != ca{i}") } else { format!("sv_bitne(cv{i}, ca{i})") };
        let _ = writeln!(s, "                    if( cOk && ({cmp}) ) {{ cOk = 0; cloneBad = \"the fork's Value is not the bar it forked at\"; }}");
    }
    s.push_str("                }\n");
    // Drive both to the end. A shared buffer diverges here and nowhere earlier.
    s.push_str("                for( t = cmid; cOk && t < svN; t++ )\n                {\n");
    let _ = writeln!(s, "                    TA_{name}_Update(cA, {bar_args}{addr_a});");
    let _ = writeln!(s, "                    TA_{name}_Update(cB, {bar_args}{addr_b});");
    for (i, is_int) in out_is_int.iter().enumerate() {
        let same = if *is_int { format!("ca{i} != cb{i}") } else { format!("sv_bitne(ca{i}, cb{i})") };
        let _ = writeln!(s, "                    if( {same} ) {{ cOk = 0; cloneBad = \"the fork and the original disagree\"; }}");
        let b = &bbuf[i];
        let cross = if *is_int {
            format!("ca{i} != {b}[t - svBeg]")
        } else {
            format!("sv_xtier_ne(ca{i}, {b}[t - svBeg], &svZsign)")
        };
        let _ = writeln!(s, "                    if( {cross} ) {{ cOk = 0; cloneBad = \"the original left batch after the fork\"; }}");
        let cross_b = if *is_int {
            format!("cb{i} != {b}[t - svBeg]")
        } else {
            format!("sv_xtier_ne(cb{i}, {b}[t - svBeg], &svZsign)")
        };
        let _ = writeln!(s, "                    if( {cross_b} ) {{ cOk = 0; cloneBad = \"the fork left batch\"; }}");
    }
    s.push_str("                }\n");
    s.push_str("                cloneChecked = 1; cloneLegs++;\n");
    s.push_str("                if( !cOk ) cloneOk = 0;\n");
    // Both consumed bars [cp0-1, svN-1], so both report the batch range. The
    // original is the control: a failure on cA alone is the leg's own
    // bookkeeping, not the fork.
    s.push_str("                if( cOk )\n                {\n");
    s.push_str("                    int rbA = -1, rnA = -1, rbB = -1, rnB = -1;\n");
    let _ = writeln!(
        s,
        "                    rangeChecked = 1; rangeLegs++; rangeSites |= {};",
        sv_range_bit(SvRangeSite::Copy, SV_RANGE_MASK_C)
    );
    let _ = writeln!(s, "                    if( TA_{name}_OutRange( cA, &rbA, &rnA ) != TA_SUCCESS || rbA != svBeg || rnA != svNb ) {{ rangeOk = 0; cloneBad = \"the original's range moved\"; }}");
    let _ = writeln!(s, "                    if( TA_{name}_OutRange( cB, &rbB, &rnB ) != TA_SUCCESS || rbB != svBeg || rnB != svNb ) {{ rangeOk = 0; cloneBad = \"the fork's range is not the batch range\"; }}");
    s.push_str("                }\n");
    let _ = writeln!(s, "                if( cA ) TA_{name}_Close(cA);");
    let _ = writeln!(s, "                if( cB ) TA_{name}_Close(cB);");
    s.push_str("            }\n");
    s.push_str("        }\n");
}

/// The compare itself: the two handles have now consumed the same `svN` bars by
/// different routes. Only when the value leg passed -- its loop breaks early on
/// a mismatch, so the handle would be short of the reference.
fn emit_sv_state_compare(s: &mut String, name: &str, steq: bool) {
    if !steq {
        return;
    }
    s.push_str("            if( ok && st && stEq )\n");
    s.push_str("            {\n");
    s.push_str("                stateChecked = 1; stateLegs++;\n");
    let _ = writeln!(
        s,
        "                if( sv_steq_TA_{name}( st, stEq, &stateWhat, &svZsign ) ) stateOk = 0;"
    );
    s.push_str("            }\n");
}

fn emit_sv_state_close(s: &mut String, name: &str, steq: bool) {
    if !steq {
        return;
    }
    let _ = writeln!(s, "        if( stEq ) {{ TA_{name}_Close(stEq); stEq = NULL; }}");
}

/// Folded into `ok` as a safety net, exactly like the fill leg: the driver
/// reads `state_ok` for the specific message, and a run whose driver check ever
/// regresses still fails on the generic flag.
fn emit_sv_state_report(s: &mut String, steq: bool) {
    if !steq {
        return;
    }
    s.push_str("        if( stateChecked && !stateOk ) allOk = 0;\n");
    s.push_str("        if( cloneChecked && !cloneOk ) allOk = 0;\n");
    s.push_str("        if( valueChecked && !valueOk ) allOk = 0;\n");
    s.push_str("        pos = json_appendf(resp, resp_size, pos, \",\\\"state_checked\\\":%d,\\\"state_legs\\\":%d,\\\"state_ok\\\":%d,\\\"state_bad\\\":\\\"%s\\\"\", stateChecked, stateLegs, stateOk, stateWhat);\n");
}

// `cognitive_complexity`: the same allow the other whole-server emitters carry.
// This one crossed the threshold when #262's declined-output arm landed, and the
// shape it is complaining about is the sequence of `emit_sv_*` calls this
// function exists to order.
#[allow(clippy::too_many_lines, clippy::cognitive_complexity)]
pub(crate) fn generate_c_stream_verify(
    funcs: &[FuncDef],
    enums: &HashMap<String, EnumDef>,
) -> String {
    let mut s = String::new();
    s.push_str("/* ---- stream_verify: bitwise batch-vs-stream comparison ---- */\n");
    s.push_str("#ifndef TA_REF_SERVE\n");
    s.push_str("#define SV_MAXN 256\n");
    // Canary for the OpenAndFill slack. The fill buffers are SV_MAXN wide but
    // the call may only write `nb` elements; everything above that is stamped
    // before the call and asserted untouched after it, so a write past the
    // produced range fails instead of landing in unread space. Values no
    // indicator can produce from the generated series.
    s.push_str("#define SV_FILL_CANARY (-1.2345678901234e300)\n");
    s.push_str("#define SV_FILL_CANARY_I (-987654321)\n");
    // Stride for the peek work that costs more than one extra call: the
    // non-commit leg's sweep and the repeat probe. The plain peek-vs-update
    // compare runs on EVERY bar — striding it left a ring whose capacity is a
    // multiple of this reading only two of its slots.
    s.push_str("#define SV_PEEK_EVERY 7\n");
    s.push_str("static double sv_o[SV_MAXN], sv_h[SV_MAXN], sv_l[SV_MAXN];\n");
    s.push_str("static double sv_c[SV_MAXN], sv_v[SV_MAXN], sv_oi[SV_MAXN];\n");
    // Batch output buffers, then the OpenAndFill scratch it is compared against
    // bitwise. One per slot the widest function uses, counted from the corpus
    // for the reason `max_output_arity` gives.
    let (sv_n_real, sv_n_int) = crate::backends::common::max_output_arity(funcs);
    for (prefix, ty, n) in [
        ("sv_b", "double", sv_n_real),
        ("sv_f", "double", sv_n_real),
        ("sv_ib", "int", sv_n_int),
        ("sv_if", "int", sv_n_int),
    ] {
        if n == 0 {
            continue;
        }
        let decl: Vec<String> = (0..n).map(|k| format!("{prefix}{k}[SV_MAXN]")).collect();
        let _ = writeln!(s, "static {ty} {};", decl.join(", "));
    }
    s.push_str("static int sv_bitne(double a, double b) { return memcmp(&a, &b, sizeof(double)) != 0; }\n");
    // Cross-tier compare (stream vs batch, and OpenAndFill's array vs batch).
    // Differing bits that are numerically equal can only be +0.0 vs -0.0, which
    // max/min leave unspecified: counted, never a mismatch — the same benign
    // class --fuzz-064 carries (issue #147). Same-tier compares (peek vs
    // update) keep sv_bitne: one code path has no licence to differ at all.
    s.push_str("static int sv_xtier_ne(double a, double b, int *zsign) {\n");
    s.push_str("    if( !sv_bitne(a, b) ) return 0;\n");
    s.push_str("    if( a == b ) { (*zsign)++; return 0; }\n");
    s.push_str("    return 1;\n");
    s.push_str("}\n");
    // Candle-settings variation for CDL streams: rounds 1/2 re-run the
    // batch-vs-stream comparison with every setting's avgPeriod bumped (+3)
    // or zeroed (the instant-candle degenerate, runtime trailing lag 0).
    // mode 0: avgPeriod += 3; mode 1: avgPeriod = 0 (instant candle, runtime
    // trailing lag 0); mode 2: rangeType = Shadows everywhere (gates the
    // TA_STREAM Shadows arithmetic, which no default setting exercises).
    s.push_str("static void sv_candle_avg(int mode) {\n");
    s.push_str("    int i;\n");
    s.push_str("    for( i = 0; i < (int)TA_AllCandleSettings; i++ )\n");
    s.push_str("        TA_SetCandleSettings( (TA_CandleSettingType)i,\n");
    s.push_str("                              mode == 2 ? TA_RangeType_Shadows : TA_Globals->candleSettings[i].rangeType,\n");
    s.push_str("                              mode == 1 ? 0 : (mode == 0 ? TA_Globals->candleSettings[i].avgPeriod + 3 : TA_Globals->candleSettings[i].avgPeriod),\n");
    s.push_str("                              TA_Globals->candleSettings[i].factor );\n");
    s.push_str("}\n\n");
    // State-equivalence comparators, emitted before the handler that calls them.
    let (steq_code, steq_have) = generate_c_state_eq(funcs, enums);
    s.push_str(&steq_code);
    s.push_str("static void handle_stream_verify(const char *json, char *resp, int resp_size) {\n");
    s.push_str("    int fnLen = 0;\n");
    s.push_str("    const char *fn = json_find_string(json, \"funcName\", &fnLen);\n");
    s.push_str("    int svShape  = json_find_int(json, \"gen_shape\");\n");
    s.push_str("    int svSeed   = json_find_int(json, \"gen_seed\");\n");
    s.push_str("    int svN      = json_find_int(json, \"gen_n\");\n");
    s.push_str("    int svK      = json_find_int(json, \"unstablePeriod\");\n");
    s.push_str("    int svCandle = json_find_int(json, \"candleLegs\");\n");
    s.push_str("    (void)svCandle;\n");
    s.push_str("    (void)svK;\n");
    s.push_str("    if( !fn ) { snprintf(resp, resp_size, \"{\\\"error\\\":\\\"missing funcName\\\"}\"); return; }\n");
    s.push_str("    if( svN < 2 ) svN = 2;\n");
    s.push_str("    if( svN > SV_MAXN ) svN = SV_MAXN;\n");
    s.push_str("    fuzz_gen(svShape, svSeed, svN, sv_o, sv_h, sv_l, sv_c, sv_v, sv_oi);\n\n");

    let mut first = true;
    for func in funcs.iter().filter(|f| f.streaming) {
        let name = &func.name;
        let method = format!("TA_{name}");
        let cond = if first { "if" } else { "else if" };
        first = false;

        // Input arrays in fuzz convention, in signature order.
        let input_names = expand_input_names(&func.inputs);
        let mut generic_idx = 0usize;
        let input_arrays: Vec<&str> = input_names
            .iter()
            .map(|n| sv_input_array(n, &mut generic_idx))
            .collect();
        let n_outs = func.outputs.len();
        // Unstable ids to pin: the function's own, plus any unstable
        // dependency reachable TRANSITIVELY through its lookback body
        // (DEMA/TEMA/TRIX/MACD call ema_lookback directly; STOCH/STOCHF
        // reach EMA/KAMA/T3 only through ma_lookback — a non-transitive
        // scan left their K-legs running vacuously at ambient K=0).
        let pin_ids: Vec<i32> = collect_pin_ids(func, funcs, enums);


        s.push_str(&format!(
            "    {cond}( fnLen == {} && strncmp(fn, \"{method}\", {}) == 0 ) {{\n",
            method.len(),
            method.len()
        ));

        // Optional params from the request.
        for opt in &func.optional_inputs {
            if opt.param_type == ParamType::Real {
                s.push_str(&format!(
                    "        double {0} = json_find_double(json, \"{0}\");\n",
                    opt.name
                ));
            } else if matches!(&opt.param_type, ParamType::Enum(_)) {
                s.push_str(&format!(
                    "        TA_MAType {0} = (TA_MAType)json_find_int(json, \"{0}\");\n",
                    opt.name
                ));
            } else {
                s.push_str(&format!(
                    "        int {0} = json_find_int(json, \"{0}\");\n",
                    opt.name
                ));
            }
        }

        emit_sv_period_bank_input(&mut s, func, funcs, &input_arrays);
        emit_sv_dispatch_precheck(&mut s, func, funcs, &input_arrays, n_outs, name);

        let candle = func.flags.iter().any(|f| f == "candlestick");
        let steq = steq_have.contains(name);
        s.push_str("        TA_RetCode rc;\n");
        s.push_str("        int svBeg = 0, svNb = 0, lb, li, npref, pos, allOk = 1, peekAll = 1;\n");
        s.push_str("        int peekChecked = 0;\n");
        // The repeat probe's OWN counter. `peek_ok` cannot say the probe went
        // absent, and the leg it is the cross-language stand-in for
        // (`peek_checked`) is emitted by this server alone.
        s.push_str("        int peekReps = 0, peekRepAll = 1;\n");
        // A peek may legitimately refuse a bar, and the probe below feeds it one
        // the batch never visits. Counted rather than failed; see
        // `emit_sv_peek_repeat_probe`.
        s.push_str("        int peekRejects = 0;\n");
        s.push_str("        TA_RetCode pkRc = TA_SUCCESS;\n");
        // The fork leg's OWN counter: `range_ok` cannot say a fork leg died,
        // and the value legs sit far above any threshold worth setting.
        s.push_str("        int cloneChecked = 0, cloneOk = 1, cloneLegs = 0;\n");
        s.push_str("        int valueChecked = 0, valueOk = 1, valueLegs = 0;\n");
        s.push_str("        const char *valueBad = \"-\";\n");
        s.push_str("        const char *cloneBad = \"-\";\n");
        // Short-history reject leg. Rust, Java and C# have carried this since
        // the streaming tier landed; C never emitted it, so `ok` could not fall
        // for an Open that accepts a history no output is defined over.
        s.push_str("        int shortHistChecked = 0, shortHistOk = 1;\n");
        s.push_str("        const char *shortHistBad = \"-\";\n");
        s.push_str("        const char *peekBad = \"-\";\n");
        s.push_str("        int fillOk = 1, fillChecked = 0, fillBars = 0;\n");
        emit_sv_state_decls(&mut s, name, steq);
        emit_sv_range_decls(&mut s);
        // Benign +/-0 cases across every cross-tier compare in this request.
        s.push_str("        int svZsign = 0;\n");
        s.push_str("        int pref[4]; int pc[4];\n");
        if candle {
            // Candle functions honor "candleLegs": re-run the whole sweep
            // under bumped and zeroed avgPeriods (settings-stability rule:
            // settings are fixed per round; each round reopens its streams).
            s.push_str("        int rounds = svCandle ? 4 : 1; int rd, lgi = 0;\n");
        }
        for id in &pin_ids {
            s.push_str(&format!(
                "        TA_SetUnstablePeriod({id}, (unsigned int)svK);\n"
            ));
        }

        // Batch leg (startIdx=0, full range) + intrinsic-in-ambient-K lookback.
        let mut opt_args = String::new();
        for o in &func.optional_inputs {
            let _ = std::fmt::Write::write_fmt(&mut opt_args, format_args!("{}, ", o.name));
        }
        let mut in_args = String::new();
        for a in &input_arrays {
            let _ = std::fmt::Write::write_fmt(&mut in_args, format_args!("{a}, "));
        }
        let out_is_int: Vec<bool> = func
            .outputs
            .iter()
            .map(|ou| ou.param_type == ParamType::Integer)
            .collect();
        let mut out_args = String::new();
        {
            let (mut ri, mut ii) = (0usize, 0usize);
            for is_int in &out_is_int {
                if *is_int {
                    let _ = std::fmt::Write::write_fmt(&mut out_args, format_args!(", sv_ib{ii}"));
                    ii += 1;
                } else {
                    let _ = std::fmt::Write::write_fmt(&mut out_args, format_args!(", sv_b{ri}"));
                    ri += 1;
                }
            }
        }
        // Per-output batch buffer expression (indexed by output position).
        let bbuf: Vec<String> = {
            let (mut ri, mut ii) = (0usize, 0usize);
            out_is_int
                .iter()
                .map(|is_int| {
                    if *is_int {
                        let e = format!("sv_ib{ii}");
                        ii += 1;
                        e
                    } else {
                        let e = format!("sv_b{ri}");
                        ri += 1;
                        e
                    }
                })
                .collect()
        };
        if candle {
            s.push_str("        pos = json_appendf(resp, resp_size, 0, \"{\\\"retCode\\\":0\");\n");
            s.push_str("        for( rd = 0; rd < rounds; rd++ ) {\n");
            s.push_str("        if( rd > 0 ) TA_RestoreCandleDefaultSettings( TA_AllCandleSettings );\n");
            s.push_str("        if( rd > 0 ) sv_candle_avg(rd - 1);\n");
        }
        s.push_str(&format!(
            "        rc = {method}(0, svN - 1, {in_args}{opt_args}&svBeg, &svNb{out_args});\n"
        ));
        s.push_str(&format!("        lb = {method}_Lookback({});\n", {
            let a: Vec<String> = func.optional_inputs.iter().map(|o| o.name.clone()).collect();
            a.join(", ")
        }));
        // Batch failed or produced nothing: report and restore (a valid
        // stream cannot exist either — driver checks openRejects).
        s.push_str("        if( rc != TA_SUCCESS || svNb <= 0 ) {\n");
        s.push_str("            int openRejects = 0;\n");
        s.push_str(&format!(
            "            {{ TA_{name}_Stream *st = NULL; {} TA_RetCode orc = TA_{name}_Open(&st, {in_args}svN, {opt_args}{});\n",
            out_is_int
                .iter()
                .enumerate()
                .map(|(i, is_int)| if *is_int {
                    format!("int v{i} = 0;")
                } else {
                    format!("double v{i} = 0.0;")
                })
                .collect::<Vec<_>>()
                .join(" "),
            (0..n_outs).map(|i| format!("&v{i}")).collect::<Vec<_>>().join(", ")
        ));
        s.push_str(&format!(
            "              if( orc != TA_SUCCESS && !st ) openRejects = 1; else TA_{name}_Close(st); }}\n"
        ));
        for id in &pin_ids {
            s.push_str(&format!("            TA_SetUnstablePeriod({id}, 0);\n"));
        }
        emit_sv_batch_fail_tail(&mut s, candle);

        // OpenAndFill leg: the whole filled array must equal batch(0, svN-1)
        // bit-for-bit, and its begIdx/nb must match batch. Same seeded inputs +
        // same algorithm => same bits — an INVARIANT check (batch's numeric
        // correctness is owned by the reference-oracle tests, not re-checked
        // here). Runs where bbuf still holds batch(0,svN-1) at the current
        // K/(candle round) settings. Every streamable function has an
        // OpenAndFill, so the leg is unconditional (the driver's fill-coverage
        // floor asserts every streaming function reaches here).
        let fbuf_names: Vec<String> = {
            let (mut ri, mut ii) = (0usize, 0usize);
            out_is_int
                .iter()
                .map(|is_int| {
                    if *is_int {
                        let e = format!("sv_if{ii}");
                        ii += 1;
                        e
                    } else {
                        let e = format!("sv_f{ri}");
                        ri += 1;
                        e
                    }
                })
                .collect()
        };
        {
            let fbuf: Vec<String> = fbuf_names.clone();
            let fill_arrays = fbuf.join(", ");
            s.push_str("        {\n");
            s.push_str("            int fBeg = 0, fNb = 0, ft;\n");
            s.push_str(&format!("            TA_{name}_Stream *stf = NULL;\n"));
            // Declared, not initialised: the canary stamp has to run between the
            // declarations and the call, and these buffers are `static` (reused
            // across every function in the request), so a stale value left by an
            // earlier call would otherwise read as a write by this one.
            s.push_str("            TA_RetCode frc;\n");
            // Stamped over the FULL buffer width, not `svN`. Two reasons, and
            // both bounds must move together: a lookback-0 function has
            // `fNb == svN`, so a window of `[fNb, svN)` is empty and the check
            // is a no-op for it; and a one-past-the-range write lands at index
            // `svN` itself, in the tail beyond the request's series length.
            // Widening only the assert would instead read bytes an earlier
            // function in the same request left behind (these are `static`).
            s.push_str(&c_canary_stamp(&fbuf, &out_is_int));
            s.push_str(&format!(
                "            frc = TA_{name}_OpenAndFill(&stf, {in_args}svN, {opt_args}&fBeg, &fNb, {fill_arrays});\n"
            ));
            s.push_str("            fillChecked = 1;\n");
            s.push_str("            if( frc != TA_SUCCESS || !stf || fBeg != svBeg || fNb != svNb ) fillOk = 0;\n");
            // OpenAndFill seeds `cur_` through the STRIDED index, a different
            // expression from the scalar Open's, and read by nothing else.
            //
            // The array comparison below is a SEPARATE statement, not this
            // probe's `else`. It was one until #287 inserted this `if` above it:
            // the `else` silently re-bound from the shape check to this probe,
            // and since a NULL handle already forces `fillOk = 0`, reaching it
            // implied the loop's own `fillOk &&` guard would stop it. Both arms
            // went dead and every gate stayed green. The loop carries its own
            // guard; do not give it an `else`.
            s.push_str("            if( fillOk && stf )\n");
            emit_sv_value_probe(
                &mut s, name, &out_is_int, "            ", "stf",
                &fbuf.iter().map(|b| format!("{b}[svNb - 1]")).collect::<Vec<_>>(),
                "Value after OpenAndFill is not the last filled bar",
            );
            s.push_str("            for( ft = 0; fillOk && ft < svNb; ft++ ) {\n");
            for (i, is_int) in out_is_int.iter().enumerate() {
                if *is_int {
                    s.push_str(&format!(
                        "                if( {}[ft] != {}[ft] ) fillOk = 0;\n",
                        fbuf[i], bbuf[i]
                    ));
                } else {
                    s.push_str(&format!(
                        "                if( sv_xtier_ne({}[ft], {}[ft], &svZsign) ) fillOk = 0;\n",
                        fbuf[i], bbuf[i]
                    ));
                }
            }
            s.push_str("                fillBars++;\n");
            s.push_str("            }\n");
            // The slack above the produced range must still hold the canary.
            // Nothing else in the tree checks it: every gate sizes the fill
            // buffer at full history and reads only [0, nb), so a write past
            // `nb` lands in `lookback` elements of unread space.
            s.push_str(&c_canary_check(&fbuf, &out_is_int));
            emit_sv_range_check(&mut s, name, "            ", "stf", "frc == TA_SUCCESS && stf", "svBeg", "svNb", SvRangeSite::Fill);
            s.push_str(&format!("            if( stf ) TA_{name}_Close(stf);\n"));
            s.push_str("        }\n");

            // Aliasing-guard probe: an OpenAndFill output that aliases an input
            // MUST reject (out==in is forbidden — stricter than batch — because
            // the capture epilogue re-reads the input tail; a dropped guard would
            // silently corrupt). The gate's normal leg passes distinct buffers,
            // so without this the guard is unverified. Only when output 0 is
            // real: an integer output cannot alias a double input, and there is
            // no integer input to alias against.
            if !out_is_int.first().copied().unwrap_or(true) && !input_arrays.is_empty() {
                let alias_out: Vec<String> = fbuf
                    .iter()
                    .enumerate()
                    .map(|(i, b)| if i == 0 { input_arrays[0].to_string() } else { b.clone() })
                    .collect::<Vec<_>>();
                let alias_args = alias_out.join(", ");
                s.push_str("        {\n");
                s.push_str("            int alB = 0, alN = 0;\n");
                s.push_str(&format!("            TA_{name}_Stream *sal = NULL;\n"));
                s.push_str(&format!(
                    "            TA_RetCode alrc = TA_{name}_OpenAndFill(&sal, {in_args}svN, {opt_args}&alB, &alN, {alias_args});\n"
                ));
                s.push_str("            if( !( alrc == TA_BAD_PARAM && !sal ) ) fillOk = 0;\n");
                s.push_str(&format!("            if( sal ) TA_{name}_Close(sal);\n"));
                s.push_str("        }\n");
            }
            // Output-output aliasing probe (multi-output funcs): two outputs
            // sharing a buffer must reject (#108 class). Covers the mutual-
            // distinctness guard the input-output probe above never touches, and
            // exercises the integer-output multi-out funcs (MINMAXINDEX) whose
            // guard the input-output probe skips.
            //
            // The pair has to be SAME-TYPED, and is searched for rather than
            // assumed to be (0, 1): substituting a `double*` buffer into an
            // `int*` slot is an incompatible-pointer-type, i.e. the generated
            // server would not compile. Searching keeps the probe alive for a
            // mixed-type function whose same-typed pair is not adjacent —
            // SYNTH12's reals are outputs 0 and 2. C# already pairs by type
            // (below); Java gates on `!out_is_int[1]`.
            let aa_pair = (0..n_outs)
                .flat_map(|i| ((i + 1)..n_outs).map(move |j| (i, j)))
                .find(|&(i, j)| out_is_int[i] == out_is_int[j]);
            if let Some((ai, aj)) = aa_pair {
                let aa_out: Vec<String> = fbuf
                    .iter()
                    .enumerate()
                    .map(|(i, b)| if i == aj { fbuf[ai].clone() } else { b.clone() })
                    .collect::<Vec<_>>();
                let aa_args = aa_out.join(", ");
                s.push_str("        {\n");
                s.push_str("            int aaB = 0, aaN = 0;\n");
                s.push_str(&format!("            TA_{name}_Stream *saa = NULL;\n"));
                s.push_str(&format!(
                    "            TA_RetCode aarc = TA_{name}_OpenAndFill(&saa, {in_args}svN, {opt_args}&aaB, &aaN, {aa_args});\n"
                ));
                s.push_str("            if( !( aarc == TA_BAD_PARAM && !saa ) ) fillOk = 0;\n");
                s.push_str(&format!("            if( saa ) TA_{name}_Close(saa);\n"));
                s.push_str("        }\n");
            }
        }

        // Prefix sweep candidates (dedup, clamped to [lb+1, svN-1]).
        s.push_str("        npref = 0;\n");
        s.push_str("        pc[0] = lb + 1; pc[1] = lb + 13; pc[2] = svN / 2; pc[3] = svN - 1;\n");
        s.push_str("        for( li = 0; li < 4; li++ ) {\n");
        s.push_str("            int P = pc[li]; int seen = 0, k;\n");
        s.push_str("            if( P < lb + 1 ) P = lb + 1;\n");
        s.push_str("            if( P > svN - 1 ) P = svN - 1;\n");
        s.push_str("            if( P < 1 ) continue;\n");
        s.push_str("            for( k = 0; k < npref; k++ ) if( pref[k] == P ) seen = 1;\n");
        s.push_str("            if( !seen ) pref[npref++] = P;\n");
        s.push_str("        }\n");
        emit_sv_state_open(&mut s, name, steq, &out_is_int, &in_args, &opt_args);
        if !candle {
            s.push_str("        pos = json_appendf(resp, resp_size, 0, \"{\\\"retCode\\\":0,\\\"beg\\\":%d,\\\"nb\\\":%d,\\\"legs\\\":%d\", svBeg, svNb, npref);\n");
        }

        // Per-leg: open on prefix, update the rest, peek spot-asserts,
        // bitwise compare against the batch outputs at every bar.
        s.push_str("        for( li = 0; li < npref; li++ ) {\n");
        s.push_str("            int P = pref[li]; int t, ok = 1, pkOk = 1, badBar = -1, badOut = -1;\n");
        s.push_str("            double bv = 0.0, sv = 0.0;\n");
        s.push_str(&format!("            TA_{name}_Stream *st = NULL;\n"));
        for (i, is_int) in out_is_int.iter().enumerate() {
            if *is_int {
                s.push_str(&format!("            int v{i} = 0, pk{i} = 0, rp{i} = 0;\n"));
            } else {
                s.push_str(&format!("            double v{i} = 0.0, pk{i} = 0.0, rp{i} = 0.0;\n"));
            }
        }
        let vout_args: String = (0..n_outs)
            .map(|i| format!("&v{i}"))
            .collect::<Vec<_>>()
            .join(", ");
        let pkout_args: String = (0..n_outs)
            .map(|i| format!("&pk{i}"))
            .collect::<Vec<_>>()
            .join(", ");
        s.push_str(&format!(
            "            rc = TA_{name}_Open(&st, {in_args}P, {opt_args}{vout_args});\n"
        ));
        s.push_str("            if( rc != TA_SUCCESS || !st ) { ok = 0; badBar = P - 1; }\n");
        // Compare the open value (bar P-1).
        emit_sv_compare(&mut s, &out_is_int, &bbuf, "            ", "(P - 1) - svBeg", "P - 1", "ok &&");
        // The opener SEEDS `cur_`; nothing else observes that write.
        s.push_str("            if( ok && st )\n");
        emit_sv_value_probe(
            &mut s, name, &out_is_int, "            ", "st",
            &(0..out_is_int.len()).map(|i| format!("v{i}")).collect::<Vec<_>>(),
            "Value after Open is not the last history bar",
        );
        // Update the remaining bars.
        let mut bar_args = String::new();
        for a in &input_arrays {
            let _ = std::fmt::Write::write_fmt(&mut bar_args, format_args!("{a}[t], "));
        }
        let mut decoy_args = String::new();
        for a in &input_arrays {
            let _ = std::fmt::Write::write_fmt(&mut decoy_args, format_args!("{a}[t - 1], "));
        }
        let rpout_args: String = (0..n_outs)
            .map(|i| format!("&rp{i}"))
            .collect::<Vec<_>>()
            .join(", ");
        s.push_str("            for( t = P; ok && t < svN; t++ ) {\n");
        s.push_str(&format!(
            "                pkRc = TA_{name}_Peek(st, {bar_args}{pkout_args});\n"
        ));
        s.push_str("                if( pkRc != TA_SUCCESS ) peekRejects++;\n");
        emit_sv_peek_repeat_probe(
            &mut s, name, &out_is_int, "                ", &bar_args, &decoy_args, &rpout_args,
        );
        s.push_str(&format!(
            "                TA_{name}_Update(st, {bar_args}{vout_args});\n"
        ));
        let peek_ne: Vec<String> = (0..n_outs)
            .map(|i| {
                if out_is_int[i] {
                    format!("(pk{i} != v{i})")
                } else {
                    format!("sv_bitne(pk{i}, v{i})")
                }
            })
            .collect();
        s.push_str(&format!(
            "                if( pkRc == TA_SUCCESS && ({}) ) pkOk = 0;\n",
            peek_ne.join(" || ")
        ));
        emit_sv_compare(&mut s, &out_is_int, &bbuf, "                ", "t - svBeg", "t", "");
        // Every accepted Update refreshes `cur_`; a Peek in between must not.
        s.push_str("                if( ok )\n");
        emit_sv_value_probe(
            &mut s, name, &out_is_int, "                ", "st",
            &(0..out_is_int.len()).map(|i| format!("v{i}")).collect::<Vec<_>>(),
            "Value after Update is not the bar just committed",
        );
        s.push_str("            }\n");
        emit_sv_state_compare(&mut s, name, steq);
        // Open(P) + (svN - P) updates: whatever P was, the handle has consumed
        // svN bars and must report exactly what batch(0, svN-1) did.
        emit_sv_range_check(&mut s, name, "            ", "st", "ok && st", "svBeg", "svNb", SvRangeSite::Prefix);
        // One TA_<N>_Advance, LAST on this handle: it deliberately leaves the
        // batch range behind, so anything reading `st` after this reads a range
        // that is one ahead on purpose.
        s.push_str(&format!("            if( ok && st && TA_{name}_Advance( st ) != TA_SUCCESS ) rangeOk = 0;\n"));
        emit_sv_range_check(&mut s, name, "            ", "st", "ok && st", "svBeg", "svNb + 1", SvRangeSite::Advance);
        s.push_str(&format!("            if( st ) TA_{name}_Close(st);\n"));
        if candle {
            s.push_str("            pos = json_appendf(resp, resp_size, pos, \",\\\"p%d\\\":%d,\\\"match%d\\\":%d,\\\"peek%d\\\":%d\", lgi, P, lgi, ok, lgi, pkOk);\n");
            s.push_str("            if( !ok ) { allOk = 0; pos = json_appendf(resp, resp_size, pos, \",\\\"bar%d\\\":%d,\\\"out%d\\\":%d,\\\"batchv%d\\\":\\\"%a\\\",\\\"streamv%d\\\":\\\"%a\\\"\", lgi, badBar, lgi, badOut, lgi, bv, lgi, sv); }\n");
            s.push_str("            if( !pkOk ) peekAll = 0;\n");
            s.push_str("            lgi++;\n");
            s.push_str("        }\n");
        } else {
            s.push_str("            pos = json_appendf(resp, resp_size, pos, \",\\\"p%d\\\":%d,\\\"match%d\\\":%d,\\\"peek%d\\\":%d\", li, P, li, ok, li, pkOk);\n");
            s.push_str("            if( !ok ) { allOk = 0; pos = json_appendf(resp, resp_size, pos, \",\\\"bar%d\\\":%d,\\\"out%d\\\":%d,\\\"batchv%d\\\":\\\"%a\\\",\\\"streamv%d\\\":\\\"%a\\\"\", li, badBar, li, badOut, li, bv, li, sv); }\n");
            s.push_str("            if( !pkOk ) peekAll = 0;\n");
            s.push_str("        }\n");
        }
        emit_sv_peek_noncommit(&mut s, name, steq, &out_is_int, &in_args, &opt_args, &input_arrays);
        emit_sv_clone_leg(&mut s, name, &input_arrays, &in_args, &opt_args, &out_is_int, &bbuf);
        emit_sv_state_close(&mut s, name, steq);
        if candle {
            s.push_str("        }\n");
            s.push_str("        if( rounds > 1 ) TA_RestoreCandleDefaultSettings( TA_AllCandleSettings );\n");
        }

        // startIdx>0 coverage: the anchored internal open (OpenInternal at a
        // non-zero startIdx over the FULL history from bar 0) must equal
        // batch(S). This exercises the extra anchor parameter for EVERY stream
        // function — not just composed sub-callees — under the same K.
        // (Reuses the bbuf batch buffers, recomputed at startIdx=S; the prefix
        // sweep above is done with them.)
        {
            let aout: String = (0..n_outs).map(|i| format!("&v{i}")).collect::<Vec<_>>().join(", ");
            s.push_str("        {\n");
            s.push_str("            int Sidx = lb + (svN - lb) / 3;\n");
            s.push_str("            if( Sidx > lb && Sidx < svN - 1 ) {\n");
            s.push_str("                int svBegS = 0, svNbS = 0;\n");
            s.push_str(&format!(
                "                rc = {method}(Sidx, svN - 1, {in_args}{opt_args}&svBegS, &svNbS{out_args});\n"
            ));
            s.push_str("                if( rc == TA_SUCCESS && svNbS > 0 ) {\n");
            s.push_str("                    int ok = 1, badBar = -1, badOut = -1; double bv = 0.0, sv = 0.0;\n");
            for (i, is_int) in out_is_int.iter().enumerate() {
                let (ty, z) = if *is_int { ("int", "0") } else { ("double", "0.0") };
                s.push_str(&format!("                    {ty} v{i} = {z};\n"));
            }
            s.push_str(&format!("                    TA_{name}_Stream *stA = NULL;\n"));
            s.push_str(&format!(
                "                    TA_RetCode arc = TA_{name}_OpenInternal(&stA, {in_args}Sidx, svN, {opt_args}{aout});\n"
            ));
            s.push_str("                    if( arc != TA_SUCCESS || !stA ) ok = 0;\n");
            emit_sv_compare(&mut s, &out_is_int, &bbuf, "                    ", "(svN - 1) - svBegS", "svN - 1", "ok &&");
            emit_sv_range_check(&mut s, name, "                    ", "stA", "ok && stA", "svBegS", "svNbS", SvRangeSite::Anchored);

            s.push_str(&format!("                    if( stA ) TA_{name}_Close(stA);\n"));
            s.push_str("                    if( !ok ) allOk = 0;\n");
            s.push_str("                    (void)badBar; (void)badOut; (void)bv; (void)sv;\n");
            s.push_str("                }\n");
            s.push_str("            }\n");
            s.push_str("        }\n");
        }

        // At exactly `lb` bars no output is defined for ANY configuration, so
        // Open must reject -- and with TA_INSUFFICIENT_HISTORY specifically, the
        // one routine data-dependent failure a caller separates from a
        // programming error. Accepting is the defect; rejecting with the wrong
        // code is a second, distinguishable one, matching the typed-exception
        // arms Java and C# already carry.
        s.push_str("        if( lb >= 1 && lb < svN ) {\n");
        s.push_str("            shortHistChecked = 1;\n");
        let _ = writeln!(
            s,
            "            {{ TA_{name}_Stream *stSH = NULL; {} TA_RetCode shrc = TA_{name}_Open(&stSH, {in_args}lb, {opt_args}{});",
            out_is_int
                .iter()
                .enumerate()
                .map(|(i, is_int)| if *is_int { format!("int sh{i} = 0;") } else { format!("double sh{i} = 0.0;") })
                .collect::<Vec<_>>()
                .join(" "),
            (0..n_outs).map(|i| format!("&sh{i}")).collect::<Vec<_>>().join(", ")
        );
        let _ = writeln!(
            s,
            "              if( shrc == TA_SUCCESS ) {{ shortHistOk = 0; shortHistBad = \"open accepted a history shorter than one output\"; TA_{name}_Close(stSH); }}"
        );
        s.push_str("              else if( shrc != TA_INSUFFICIENT_HISTORY ) { shortHistOk = 0; shortHistBad = \"open rejected with the wrong retCode\"; }\n");
        s.push_str("              (void)stSH; }\n");
        s.push_str("        }\n");
        s.push_str("        if( shortHistChecked && !shortHistOk ) allOk = 0;\n");
        for id in &pin_ids {
            s.push_str(&format!("        TA_SetUnstablePeriod({id}, 0);\n"));
        }
        // Fold fill into ok as a safety net (the driver also checks fill_ok
        // explicitly for a clearer message), so a fill regression fails the run
        // even if the driver's fill check ever regresses.
        s.push_str("        if( fillChecked && !fillOk ) allOk = 0;\n");
        emit_sv_state_report(&mut s, steq);
        emit_sv_range_report(&mut s);
        if candle {
            s.push_str("        pos = json_appendf(resp, resp_size, pos, \",\\\"beg\\\":%d,\\\"nb\\\":%d,\\\"legs\\\":%d,\\\"fill_checked\\\":%d,\\\"fill_ok\\\":%d,\\\"fill_bars\\\":%d,\\\"ok\\\":%d,\\\"peek_checked\\\":%d,\\\"peek_ok\\\":%d,\\\"peek_reps\\\":%d,\\\"peek_rep_ok\\\":%d,\\\"peek_rejects\\\":%d,\\\"short_history_checked\\\":%d,\\\"short_history_ok\\\":%d,\\\"short_history_bad\\\":\\\"%s\\\",\\\"clone_checked\\\":%d,\\\"clone_legs\\\":%d,\\\"clone_ok\\\":%d,\\\"clone_bad\\\":\\\"%s\\\",\\\"value_checked\\\":%d,\\\"value_legs\\\":%d,\\\"value_ok\\\":%d,\\\"value_bad\\\":\\\"%s\\\",\\\"benign\\\":%d}\", svBeg, svNb, lgi, fillChecked, fillOk, fillBars, allOk, peekChecked, peekAll, peekReps, peekRepAll, peekRejects, shortHistChecked, shortHistOk, shortHistBad, cloneChecked, cloneLegs, cloneOk, cloneBad, valueChecked, valueLegs, valueOk, valueBad, svZsign);\n");
        } else {
            s.push_str("        pos = json_appendf(resp, resp_size, pos, \",\\\"fill_checked\\\":%d,\\\"fill_ok\\\":%d,\\\"fill_bars\\\":%d,\\\"ok\\\":%d,\\\"peek_checked\\\":%d,\\\"peek_ok\\\":%d,\\\"peek_reps\\\":%d,\\\"peek_rep_ok\\\":%d,\\\"peek_rejects\\\":%d,\\\"short_history_checked\\\":%d,\\\"short_history_ok\\\":%d,\\\"short_history_bad\\\":\\\"%s\\\",\\\"clone_checked\\\":%d,\\\"clone_legs\\\":%d,\\\"clone_ok\\\":%d,\\\"clone_bad\\\":\\\"%s\\\",\\\"value_checked\\\":%d,\\\"value_legs\\\":%d,\\\"value_ok\\\":%d,\\\"value_bad\\\":\\\"%s\\\",\\\"benign\\\":%d}\", fillChecked, fillOk, fillBars, allOk, peekChecked, peekAll, peekReps, peekRepAll, peekRejects, shortHistChecked, shortHistOk, shortHistBad, cloneChecked, cloneLegs, cloneOk, cloneBad, valueChecked, valueLegs, valueOk, valueBad, svZsign);\n");
        }
        s.push_str("        return;\n");
        s.push_str("    }\n");
    }

    // Unknown / non-streamable function.
    s.push_str("    snprintf(resp, resp_size, \"{\\\"error\\\":\\\"not_streamable\\\"}\");\n");
    s.push_str("}\n");
    s.push_str("#else /* TA_REF_SERVE: frozen libs have no stream symbols */\n");
    s.push_str("static void handle_stream_verify(const char *json, char *resp, int resp_size) {\n");
    s.push_str("    (void)json;\n");
    s.push_str("    snprintf(resp, resp_size, \"{\\\"error\\\":\\\"not supported\\\"}\");\n");
    s.push_str("}\n");
    s.push_str("#endif /* TA_REF_SERVE */\n\n");
    s
}
