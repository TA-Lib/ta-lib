//! `stream_verify`: each server replays the fuzz corpus through its own
//! streaming tiers and bit-compares every output and the reported range
//! against what its batch tier answered for the same bars. The other legs --
//! peek, state, clone -- compare within the streaming tiers, and which of them
//! a backend can express differs; each emitter says which it emits.
//!
//! One emitter per backend, four hand-written string builders, deliberately not
//! deduplicated -- what keeps them in step is the emitted-text suites. So a
//! helper ONE backend calls lives in that backend's file however generic its
//! name reads, and only what two or more call lives here -- except
//! `SV_RANGE_MASK_*`, four single-backend constants kept as one table because
//! each is only readable against the others.

use crate::ir::{EnumDef, FuncDef};
use crate::server_gen::func_unst_id;
use std::collections::HashMap;

pub(crate) mod c;
pub(crate) mod csharp;
pub(crate) mod java;
pub(crate) mod rust_lang;

/// The bare OHLCV/OI suffix an input maps to (shared by the per-server
/// `sv_*`/`fz_*` array-name helpers so the mapping can never drift).
fn sv_input_suffix(name: &str, generic_idx: &mut usize) -> &'static str {
    match name {
        "inOpen" => "o",
        "inHigh" => "h",
        "inLow" => "l",
        "inClose" => "c",
        "inVolume" => "v",
        "inOpenInterest" => "oi",
        _ => {
            let arr = if *generic_idx == 0 { "c" } else { "v" };
            *generic_idx += 1;
            arr
        }
    }
}

/// Unstable-period ids a function's stream values depend on: its own id
/// plus every unstable id reachable through the TRANSITIVE closure of
/// `<base>_lookback` calls starting from its lookback body (STOCH ->
/// ma_lookback -> ema_lookback -> EMA). Composed/dispatch functions honor
/// ambient K only through the callees' lookbacks, so the lookback closure
/// covers exactly the sub-stream selection space.
fn collect_pin_ids(func: &FuncDef, funcs: &[FuncDef], enums: &HashMap<String, EnumDef>) -> Vec<i32> {
    let mut pin_ids: Vec<i32> = Vec::new();
    let mut visited: std::collections::BTreeSet<String> = std::collections::BTreeSet::new();
    let mut queue: Vec<String> = vec![func.name.to_uppercase()];
    while let Some(cur) = queue.pop() {
        if !visited.insert(cur.clone()) {
            continue;
        }
        if let Some(id) = func_unst_id(&cur, enums) {
            if !pin_ids.contains(&id) {
                pin_ids.push(id);
            }
        }
        let Some(fd) = funcs.iter().find(|f| f.name.eq_ignore_ascii_case(&cur)) else {
            continue;
        };
        if let Some(crate::ir::LookbackExpr::Code(stmts)) = &fd.lookback {
            for st in stmts {
                crate::streaming::walk_stmt_exprs(st, &mut |e| {
                    crate::streaming::walk_expr(e, &mut |x| {
                        if let crate::ir::Expr::FuncCall(fname, _) = x {
                            if let Some(base) = fname.strip_suffix("_lookback") {
                                queue.push(base.to_uppercase());
                            }
                        }
                    });
                });
            }
        }
    }
    pin_ids
}

/// The C condition under which a function's stream Open HONESTLY rejects a
/// param set the batch accepts (a documented capability limitation), or
/// None when no such set exists. Composes recursively:
/// - Dispatch (MA): `!identity && (param in unsupported labels)`.
/// - Composed (STOCH): OR over its sub-calls, with the sub's optional
///   argument EXPRESSIONS substituted for the callee's params — so MA's
///   `optInTimePeriod == 1` identity exemption becomes
///   `optInSlowK_Period == 1` at the STOCH level, and TRIMA landing later
///   narrows every dependent precheck automatically on regenerate.
/// - Loop tier: never (None).
///
/// `subst` maps the callee's param names to caller-level argument exprs
/// (None at the top level: the function's own params are in scope).
fn sv_reject_condition(
    func: &FuncDef,
    funcs: &[FuncDef],
    subst: Option<&std::collections::BTreeMap<String, crate::ir::Expr>>,
) -> Option<String> {
    use crate::ir::Expr;
    let lookup = crate::streaming::FuncsLookup(funcs);
    let render_arg = |e: &Expr| -> String {
        let mapped = match (e, subst) {
            (Expr::Var(v), Some(m)) => m.get(v).cloned().unwrap_or_else(|| e.clone()),
            _ => e.clone(),
        };
        sv_render_scalar(&mapped)
    };
    match crate::streaming::validate_streamable(func, &lookup) {
        Ok(crate::streaming::StreamPlan::Dispatch(dp)) => {
            let unsupported = dp.unsupported_labels();
            if unsupported.is_empty() {
                return None;
            }
            let param_c = render_arg(&Expr::Var(dp.param.clone()));
            let arm_match = unsupported
                .iter()
                .map(|l| {
                    let c_const = if l.starts_with("TA_") {
                        (*l).to_string()
                    } else {
                        format!("TA_{l}")
                    };
                    format!("{param_c} == {c_const}")
                })
                .collect::<Vec<_>>()
                .join(" || ");
            match dp.identity.as_ref().and_then(|i| {
                sv_identity_guard_subst(&i.condition, &render_arg)
            }) {
                Some(g) => Some(format!("( !({g}) && ( {arm_match} ) )")),
                None => Some(format!("( {arm_match} )")),
            }
        }
        Ok(crate::streaming::StreamPlan::Composed(cp)) => {
            let mut parts: Vec<String> = Vec::new();
            for sub in &cp.subs {
                let callee = funcs
                    .iter()
                    .find(|f| f.name.eq_ignore_ascii_case(&sub.callee))?;
                // Map the callee's params to the sub-call's argument exprs,
                // resolved through the CURRENT substitution.
                let mut m = std::collections::BTreeMap::new();
                for (p, a) in callee.optional_inputs.iter().zip(sub.opt_args.iter()) {
                    let resolved = match (a, subst) {
                        (Expr::Var(v), Some(outer)) => {
                            outer.get(v).cloned().unwrap_or_else(|| a.clone())
                        }
                        _ => a.clone(),
                    };
                    m.insert(p.name.clone(), resolved);
                }
                if let Some(cond) = sv_reject_condition(callee, funcs, Some(&m)) {
                    parts.push(cond);
                }
            }
            if parts.is_empty() {
                None
            } else {
                Some(format!("( {} )", parts.join(" || ")))
            }
        }
        Ok(crate::streaming::StreamPlan::PeriodBank(pb)) => {
            // The bank opens the callee (`ma`) at every period in [min,max], so
            // MAVP rejects when the callee rejects for the forwarded MAType at
            // ANY of those periods. The callee's period guard (its `period == 1`
            // identity path exempts MAType=MAMA) is resolved against the LARGEST
            // period in the bank: `ma(maxPeriod, MAMA)` rejects whenever
            // maxPeriod > 1, which is exactly when a non-identity slot exists.
            let callee = funcs
                .iter()
                .find(|f| f.name.eq_ignore_ascii_case(&pb.callee))?;
            let resolve = |name: &str| -> Expr {
                match subst {
                    Some(outer) => outer
                        .get(name)
                        .cloned()
                        .unwrap_or_else(|| Expr::Var(name.to_string())),
                    None => Expr::Var(name.to_string()),
                }
            };
            let mut m = std::collections::BTreeMap::new();
            for p in &callee.optional_inputs {
                match &p.param_type {
                    crate::ir::ParamType::Enum(e) if e == "MAType" => {
                        m.insert(p.name.clone(), resolve(&pb.matype_param));
                    }
                    crate::ir::ParamType::Integer => {
                        m.insert(p.name.clone(), resolve(&pb.max_param));
                    }
                    _ => {}
                }
            }
            sv_reject_condition(callee, funcs, Some(&m))
        }
        _ => None,
    }
}

/// Render a param-pure scalar expression for the verify precheck (the
/// analyzer guarantees purity; anything else is a generate-time panic so a
/// silently-omitted precheck can never ship).
fn sv_render_scalar(e: &crate::ir::Expr) -> String {
    use crate::ir::Expr;
    match e {
        Expr::Var(v) => v.clone(),
        Expr::IntLiteral(k) => k.to_string(),
        Expr::Literal(x) => format!("{x:?}"),
        _ => panic!("stream_verify precheck: unrenderable sub-call argument {e:?}"),
    }
}

/// The identity guard with the callee's params substituted through
/// `render_arg` (`optInTimePeriod == 1` -> `optInSlowK_Period == 1`).
fn sv_identity_guard_subst(
    cond: &crate::ir::Expr,
    render_arg: &dyn Fn(&crate::ir::Expr) -> String,
) -> Option<String> {
    use crate::ir::{BinOp, Expr};
    if let Expr::BinOp(l, op, r) = cond {
        if let (Expr::Var(_), Expr::IntLiteral(k)) = (l.as_ref(), r.as_ref()) {
            let op_s = match op {
                BinOp::Eq => "==",
                BinOp::LessEq => "<=",
                _ => return None,
            };
            let lhs = render_arg(l);
            return Some(format!("{lhs} {op_s} {k}"));
        }
    }
    None
}

/// The range-compare SITES a server emits. Each site owns one bit, and it is the
/// SAME bit in every language; each server declares the set it has as
/// `range_sites_all`; the driver ORs what actually fired across the run and
/// requires exactly that set.
///
/// The leg's other floor is a total — it counts functions — so a whole site
/// class going dead in one language leaves it far above its floor and green.
/// This is the ratchet that sees it. Corpus-wide rather than per function,
/// because a site can legitimately not run for a given function or vector: the
/// anchored compare needs `lb < Sidx < svN - 1`, which a large lookback denies.
///
/// **Why a mask and not a count.** It was a count, with the driver demanding
/// `(1 << n) - 1`. That is only expressible while every server's sites are a
/// prefix of one list, which `Copy` (#287) broke: `Copy` runs in Java, C# and
/// Rust, `Anchored` in C, Java and C#, so Rust's set skips a site in the middle
/// and a count cannot say WHICH sites ran. Renumbering per language could keep
/// the prefix, at the price of the same site meaning a different bit in each
/// server — so a mask printed in a diagnostic would no longer be readable
/// against any other language's.
///
/// One definition per language, and the bit and the declared set are read from
/// the SAME place, because the drift that fails OPEN is a site emitted but left
/// out of the set: the mask then carries a bit the ratchet never demands.
/// `sv_range_bit` is the only way to spell a bit, and it asserts membership.
#[derive(Clone, Copy, PartialEq, Eq)]
enum SvRangeSite {
    /// The `OpenAndFill` handle.
    Fill = 0,
    /// The `Open(P)` + updates handle.
    Prefix = 1,
    /// The `startIdx`-anchored `_OpenInternal` handle. Every server but Rust,
    /// whose server is a separate crate and cannot reach a `pub(crate)` seam.
    Anchored = 2,
    /// The handle forked mid-stream by `copy()` / `Clone()` / `.clone()` and
    /// driven to the end (#287). Every server, C included since it gained
    /// `TA_<N>_Clone`.
    Copy = 3,
    /// The prefix handle after one `TA_<N>_Advance` (#384) — the only call
    /// that moves the range without a bar, and the one place its cross-language
    /// contract is stated: it succeeds and reports exactly +1, in every backend.
    /// Runs everywhere. Its other answer, rule U4's ceiling, is 100 000 000 bars
    /// out of reach at `stream_verify` sizes and is probed per backend instead.
    Advance = 4,
}

/// The bit `site` sets, checked against the set the server will declare.
fn sv_range_bit(site: SvRangeSite, declared: u32) -> u32 {
    let bit = 1u32 << (site as u32);
    assert!(
        declared & bit != 0,
        "range site bit {bit:#x} is outside the {declared:#x} this server declares — the \
         mask would carry a bit the driver's ratchet never demands"
    );
    bit
}

/// The set `sites` spell, as the bit mask a server declares.
const fn sv_range_mask(sites: &[SvRangeSite]) -> u32 {
    let mut m = 0u32;
    let mut i = 0;
    while i < sites.len() {
        m |= 1u32 << (sites[i] as u32);
        i += 1;
    }
    m
}

/// C, Java and C# reach the anchored seam; Rust cannot — its server is a
/// separate crate and `_OpenInternal` is `pub(crate)`. All four can fork a live
/// stream since C gained `TA_<N>_Clone` (#287), so C is the only server that
/// reaches every site.
const SV_RANGE_MASK_C: u32 = sv_range_mask(&[
    SvRangeSite::Fill,
    SvRangeSite::Prefix,
    SvRangeSite::Anchored,
    SvRangeSite::Copy,
    SvRangeSite::Advance,
]);
const SV_RANGE_MASK_JAVA: u32 = sv_range_mask(&[
    SvRangeSite::Fill,
    SvRangeSite::Prefix,
    SvRangeSite::Anchored,
    SvRangeSite::Copy,
    SvRangeSite::Advance,
]);
const SV_RANGE_MASK_CSHARP: u32 = SV_RANGE_MASK_JAVA;
const SV_RANGE_MASK_RUST: u32 =
    sv_range_mask(&[SvRangeSite::Fill, SvRangeSite::Prefix, SvRangeSite::Copy, SvRangeSite::Advance]);

/// Substitute C enum constants (`TA_MAType_HMA`) with their integer values so
/// a `sv_reject_condition` guard renders as valid Rust or Java (both compare
/// the raw enum int).
fn sv_guard_enum_ints(guard: &str, enums: &HashMap<String, EnumDef>) -> String {
    let mut out = guard.to_string();
    for e in enums.values() {
        for v in &e.variants {
            out = out.replace(&v.c_name, &v.value.to_string());
        }
    }
    out
}
