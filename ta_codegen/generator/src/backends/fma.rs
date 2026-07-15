//! Shared fused-multiply-add (FMA) fusion detection — the single source of truth
//! for WHICH `a*b + c` sites every backend fuses.
//!
//! TA-Lib adopted an explicit-FMA numerical contract (PR #96,
//! `docs/fma-readiness-audit.md`): each function faithfully implements its
//! algorithm within a documented ~1e-9 relative tolerance, not bit-for-bit.
//! Explicit `fma` / `f64::mul_add` / `Math.fma` all compute the IEEE-754
//! correctly-rounded fused product-sum, identical on every platform — so fusing
//! the SAME sites in C, Rust and Java keeps the three backends bit-identical to
//! one another (the .NET binding P/Invokes the generated C, inheriting it).
//!
//! The regtest cross-language comparison only enforces a 1e-6 tolerance, so it
//! would NOT catch two backends fusing slightly different site sets (they would
//! silently drift ~1e-10). Routing all backends through this one detector makes
//! identical fusion a structural guarantee rather than something a tolerance gate
//! is trusted to police. Rust feeds its render context straight into a borrowing
//! [`FmaCtx`]; C and Java rebuild the same name-sets from the same IR body via
//! [`build_fma_var_sets`].

use std::collections::HashSet;

use crate::ir::{BinOp, Expr, Output, ParamType, Statement, VarType};

use super::rust_lang::{collect_sentinel_vars, collect_signed_int_vars, collect_var_types};

/// Master FMA emission gate. `true` = the FMA-era numerical contract is in force
/// and every backend fuses. Set it `false` to regenerate the pre-FMA output that
/// is bit-reproducible against the frozen v0.6.4 reference — the enumeration
/// oracle described in `docs/fma-readiness-audit.md` (diff the generated code to
/// see every fusion site, then revert).
pub const EMIT_FMA: bool = true;

/// Index-param seeds for the unguarded / `_private` variant (mirrors the Rust
/// `RustRenderCtx` construction). These range/count indices are never float
/// multiply operands, so the exact seed set does not change any fusion decision;
/// the seeds are kept faithful only so the derived name-sets match Rust's.
pub const UNGUARDED_INDEX_SEEDS: [&str; 4] = ["startIdx", "endIdx", "outBegIdx", "outNBElement"];
/// Index-param seeds for the guarded variant.
pub const GUARDED_INDEX_SEEDS: [&str; 2] = ["startIdx", "endIdx"];

/// Borrowing view of the variable-type name-sets the fusion predicate consults.
/// Rust builds this from its `RustRenderCtx`; C/Java from an owned [`FmaVarSets`].
pub struct FmaCtx<'a> {
    pub real_vars: &'a HashSet<String>,
    pub index_vars: &'a HashSet<String>,
    pub real_array_vars: &'a HashSet<String>,
    pub int_output_names: &'a HashSet<String>,
    pub sentinel_vars: &'a HashSet<String>,
}

/// Owned name-sets for the backends (C, Java) that do not otherwise track
/// per-variable types (they emit typed declarations and let the compiler infer
/// arithmetic types). Built by [`build_fma_var_sets`].
pub struct FmaVarSets {
    pub real_vars: HashSet<String>,
    pub index_vars: HashSet<String>,
    pub real_array_vars: HashSet<String>,
    pub int_output_names: HashSet<String>,
    pub sentinel_vars: HashSet<String>,
}

impl FmaVarSets {
    #[must_use]
    pub fn view(&self) -> FmaCtx<'_> {
        FmaCtx {
            real_vars: &self.real_vars,
            index_vars: &self.index_vars,
            real_array_vars: &self.real_array_vars,
            int_output_names: &self.int_output_names,
            sentinel_vars: &self.sentinel_vars,
        }
    }
}

/// Build the FMA name-sets for a rendered body, mirroring the Rust
/// `RustRenderCtx` construction step-for-step (`rust_lang.rs`) so C and Java
/// identify the byte-for-byte identical fusion sites Rust does.
#[must_use]
pub fn build_fma_var_sets(
    body: &[Statement],
    outputs: &[Output],
    extra_index_params: &[&str],
) -> FmaVarSets {
    let mut index_vars = HashSet::new();
    let mut real_vars = HashSet::new();
    let mut vec_vars = HashSet::new();
    let mut real_array_vars = HashSet::new();
    let mut int_vec_vars = HashSet::new();
    collect_var_types(
        body,
        &mut index_vars,
        &mut real_vars,
        &mut vec_vars,
        &mut real_array_vars,
        &mut int_vec_vars,
    );
    for p in extra_index_params {
        index_vars.insert((*p).to_string());
    }
    let mut sentinel_vars = HashSet::new();
    collect_sentinel_vars(body, &mut sentinel_vars);
    collect_signed_int_vars(body, &index_vars, &mut sentinel_vars);
    for sv in &sentinel_vars {
        index_vars.remove(sv);
    }
    let int_output_names = outputs
        .iter()
        .filter(|o| o.param_type == ParamType::Integer)
        .map(|o| o.name.clone())
        .collect();
    FmaVarSets {
        real_vars,
        index_vars,
        real_array_vars,
        int_output_names,
        sentinel_vars,
    }
}

/// The underlying batch name of a stream-qualified operand. The C stream
/// transition rewrite qualifies state fields as `sp->X` and renames per-bar
/// series reads to `cur_X`; classifying by the base `X` makes the streamed
/// recurrence fuse the identical sites the batch body does (else a renamed
/// operand is misclassified non-float and the site is silently left unfused,
/// diverging ~1 ULP from the fused batch — e.g. BBANDS's `cur_tempBuffer2 *
/// sp->optInNbDevUp` with unequal deviations). A no-op for batch names.
fn stream_base(name: &str) -> &str {
    name.strip_prefix("sp->")
        .or_else(|| name.strip_prefix("cur_"))
        .unwrap_or(name)
}

/// True when `expr` is provably integer-typed (an index/counter, a sentinel, or
/// an integer output name) — such an operand must never be treated as a float
/// multiply operand. Moved verbatim from `rust_lang.rs`, retargeted to [`FmaCtx`].
pub(crate) fn is_definitely_integer(expr: &Expr, ctx: &FmaCtx) -> bool {
    match expr {
        Expr::Var(name) => {
            let name = stream_base(name);
            ctx.index_vars.contains(name)
                || ctx.sentinel_vars.contains(name)
                || ctx.int_output_names.contains(name)
        }
        Expr::BinOp(a, _, b) => is_definitely_integer(a, ctx) || is_definitely_integer(b, ctx),
        // IntLiteral and everything else: not definitely integer (literals coerce to f64).
        _ => false,
    }
}

/// True when `expr` is likely float/Real-typed. Moved verbatim from
/// `rust_lang.rs` (`expr_is_float_typed_ctx`), retargeted to [`FmaCtx`]. The
/// heuristics fire on canonical IR variable names shared by all backends, so the
/// predicate identifies the same sites regardless of which backend calls it.
pub(crate) fn expr_is_float_typed(expr: &Expr, ctx: Option<&FmaCtx>) -> bool {
    match expr {
        Expr::Literal(_) | Expr::Cast(VarType::Real, _) => true,
        Expr::Var(name) => {
            // Classify by the underlying batch name so the streamed recurrence
            // (which qualifies operands as `sp->X` / `cur_X`) fuses the same
            // sites the batch does. No-op for un-prefixed batch names.
            let name = stream_base(name);
            // Check context's real_vars set first
            if let Some(c) = ctx {
                if c.real_vars.contains(name) {
                    return true;
                }
                // A `cur_X` scalar is a renamed per-bar read of the real array X:
                // classify it float exactly as the batch `X[i]` ArrayAccess would.
                if c.real_array_vars.contains(name) {
                    return true;
                }
                // If declared as index/integer in VarDecl, it's NOT float
                // Only use explicit declarations, not naming heuristics (which overlap)
                if c.index_vars.contains(name) {
                    return false;
                }
            }
            // optIn Real params are f64 in the function signature
            if name.starts_with("optIn") && !is_i32_opt_in_param(name) {
                return true;
            }
            // Only match known float-typed variable patterns.
            // Exclude anything that could be an integer/index variable.
            name.starts_with("temp")
                || name.starts_with("prev")
                || name.starts_with("sum")
                || name.starts_with("diff")
                || name.ends_with("PeriodTotal")
                || name == "k"
                || name == "k1"
                || name.starts_with("factor")
                || name.ends_with("_factor")
                || name.starts_with("_true_range")
                || name.starts_with("_candlerange")
                || name.starts_with("_periodTotal")
                || name.starts_with("_meanValue")
                || name.starts_with("_tempReal")
        }
        Expr::ArrayAccess(name, _) => {
            // Check context's real_array_vars set
            if let Some(c) = ctx {
                if c.real_array_vars.contains(name) {
                    return true;
                }
            }
            // Real arrays: input arrays, temp buffers, output Real arrays
            name.starts_with("in")
                || name.starts_with("temp")
                || (name.starts_with("out") && !name.contains("Int") && !name.contains("integer"))
                || name.contains("_Odd")
                || name.contains("_Even")
                || name.starts_with("detrender")
                || name.starts_with("Q1")
                || name.starts_with("jI")
                || name.starts_with("jQ")
        }
        Expr::FuncCall(name, _) => {
            // Math functions and ta_ methods return T, but NOT integer-returning helpers
            if is_integer_returning_helper(name) {
                return false;
            }
            name.starts_with("ta_")
                || name.contains("_from_")
                || matches!(
                    name.as_str(),
                    "sqrt"
                        | "sin"
                        | "cos"
                        | "tan"
                        | "asin"
                        | "acos"
                        | "atan"
                        | "exp"
                        | "log"
                        | "log10"
                        | "ceil"
                        | "floor"
                        | "abs"
                        | "fabs"
                        | "cosh"
                        | "sinh"
                        | "tanh"
                        | "max"
                        | "fmax"
                        | "min"
                        | "fmin"
                        | "IS_ZERO"
                        | "IS_ZERO_OR_NEG"
                        | "PER_TO_K"
                )
        }
        Expr::BinOp(left, op, right) => {
            matches!(op, BinOp::Add | BinOp::Sub | BinOp::Mul | BinOp::Div)
                && (expr_is_float_typed(left, ctx) || expr_is_float_typed(right, ctx))
        }
        Expr::Ternary(_, then_expr, _) => expr_is_float_typed(then_expr, ctx),
        _ => false,
    }
}

/// Known Real (f64) optIn params — the rest of `optIn*` are i32. Moved verbatim
/// from `rust_lang.rs`; also re-imported there so its existing callers are
/// unchanged.
pub(crate) fn is_i32_opt_in_param(name: &str) -> bool {
    if !name.starts_with("optIn") {
        return false;
    }
    // Known Real optIn params that are f64
    !matches!(
        name,
        "optInAcceleration"
            | "optInMaximum"
            | "optInOffsetOnReverse"
            | "optInFastLimit"
            | "optInSlowLimit"
            | "optInNbDevUp"
            | "optInNbDevDn"
            | "optInNbDev"
            | "optInPenetration"
            | "optInVFactor"
            | "optInStartValue"
            | "optInPercentage"
            | "optInAccelerationInitLong"
            | "optInAccelerationLong"
            | "optInAccelerationMaxLong"
            | "optInAccelerationInitShort"
            | "optInAccelerationShort"
            | "optInAccelerationMaxShort"
    )
}

/// Candle helpers that return an integer (not a Real). Moved verbatim from
/// `rust_lang.rs`; also re-imported there.
pub(crate) fn is_integer_returning_helper(name: &str) -> bool {
    matches!(
        name,
        "ta_candlecolor"
            | "ta_realbodygapup"
            | "ta_realbodygapdown"
            | "ta_candlegapup"
            | "ta_candlegapdown"
    )
}

/// True if `e` references the variable `name` anywhere.
fn expr_references(e: &Expr, name: &str) -> bool {
    match e {
        Expr::Var(n) => n == name,
        Expr::ArrayAccess(n, idx) => n == name || expr_references(idx, name),
        Expr::BinOp(l, _, r) => expr_references(l, name) || expr_references(r, name),
        Expr::Cast(_, i)
        | Expr::Not(i)
        | Expr::AddressOf(i)
        | Expr::PostIncrement(i)
        | Expr::PostDecrement(i)
        | Expr::PreIncrement(i)
        | Expr::PreDecrement(i) => expr_references(i, name),
        Expr::FuncCall(_, args) => args.iter().any(|a| expr_references(a, name)),
        Expr::Ternary(c, t, f) => {
            expr_references(c, name) || expr_references(t, name) || expr_references(f, name)
        }
        _ => false,
    }
}

/// Canonicalize an accumulator recurrence `target = P1 + P2` (both products) so
/// the product that references `target` — the accumulator — is the FUSED product
/// (the left operand, which [`fuse_operands`] selects). Two operand orders occur
/// in the sources: `(1-k)*prev + k*x` (accumulator first, e.g. the Hilbert
/// smoothers, MAMA) and `k*x + (1-k)*prev` (accumulator last, e.g. T3, ADOSC);
/// the streaming transition rewrite can additionally reorder them. Pre-FMA the
/// order was invisible (`+` commutes), but under FMA `fuse_operands` fuses the
/// LEFT product, so an order difference makes two paths fuse different products
/// and diverge (a ULP — within the 1e-9 contract, but breaking the bitwise
/// batch-vs-stream gate, and making the choice depend on incidental source
/// order). Pinning the accumulator product as the fused one makes every path and
/// backend fuse identically. Accumulator-first is chosen over -last so the
/// Hilbert family (whose sources are accumulator-first and whose HT_TRENDMODE
/// integer output the readiness audit certified flip-free) is left byte-for-byte
/// unchanged; only the accumulator-last continuous-output functions re-fuse. A
/// no-op unless the pattern matches exactly (both operands products, exactly one
/// referencing a `Var` target).
#[must_use]
pub fn canonicalize_accumulator_add(target: &Expr, value: &Expr) -> Expr {
    if let (Expr::Var(t), Expr::BinOp(l, BinOp::Add, r)) = (target, value) {
        if matches!(l.as_ref(), Expr::BinOp(_, BinOp::Mul, _))
            && matches!(r.as_ref(), Expr::BinOp(_, BinOp::Mul, _))
        {
            let l_has = expr_references(l, t);
            let r_has = expr_references(r, t);
            if r_has && !l_has {
                // Accumulator product is on the right — swap so it is the fused
                // (left) product.
                return Expr::BinOp(r.clone(), BinOp::Add, l.clone());
            }
        }
    }
    value.clone()
}

/// The single fusion detector. For `a*b + c` in either operand order — with both
/// multiply operands provably float and neither definitely-integer (excludes
/// `i32 * f64` index math) — returns the `(a, b, c)` a backend should emit as
/// `fma(a, b, c)` / `a.mul_add(b, c)` / `Math.fma(a, b, c)`. A left-hand `Mul`
/// takes priority over a right-hand one, matching the historical Rust detector
/// (this matters for `a*b + c*d`, where only the first product fuses).
///
/// Does not itself consult [`EMIT_FMA`]; callers gate on it so the (cheap) name-set
/// construction can be skipped entirely when fusion is off.
pub fn fuse_operands<'e>(
    left: &'e Expr,
    op: &BinOp,
    right: &'e Expr,
    ctx: &FmaCtx,
) -> Option<(&'e Expr, &'e Expr, &'e Expr)> {
    if !matches!(op, BinOp::Add) {
        return None;
    }
    let both_float = |a: &Expr, b: &Expr| {
        expr_is_float_typed(a, Some(ctx))
            && !is_definitely_integer(a, ctx)
            && expr_is_float_typed(b, Some(ctx))
            && !is_definitely_integer(b, ctx)
    };
    if let Expr::BinOp(a, BinOp::Mul, b) = left {
        if both_float(a, b) {
            return Some((a, b, right));
        }
    }
    if let Expr::BinOp(a, BinOp::Mul, b) = right {
        if both_float(a, b) {
            return Some((a, b, left));
        }
    }
    None
}
