//! Pure IR-inspection helpers shared by the language backends.
//!
//! These walk the backend-agnostic [`Statement`]/[`Expr`] AST and contain no
//! language-specific output, so every backend shares one copy instead of
//! re-implementing them. New backends import from here rather than copy-pasting.

use crate::ir::{BinOp, Expr, Statement};

/// TA-Lib's real-value range sentinels (`TA_REAL_MIN` / `TA_REAL_MAX`).
pub const TA_REAL_MIN: f64 = -3e37;
pub const TA_REAL_MAX: f64 = 3e37;

/// Map the YAML parser's `f64::MIN`/`MAX` sentinels back to TA-Lib's
/// `-3e37`/`3e37` so every metadata backend agrees with C/XML semantics.
#[allow(clippy::float_cmp)]
pub fn ta_real_sentinel(v: f64) -> f64 {
    if v == f64::MIN {
        TA_REAL_MIN
    } else if v == f64::MAX {
        TA_REAL_MAX
    } else {
        v
    }
}

/// The candlestick helper functions (`ta_candlerange`, `ta_candleaverage`) whose
/// calls are unpacked/hoisted before the surrounding expression is rendered.
pub const CANDLE_FNS: &[&str] = &["ta_candlerange", "ta_candleaverage"];

/// True if `expr` directly contains a call to a candlestick helper, stopping the
/// walk at logical `&&`/`||` operators (which begin a separate condition in the
/// chain).
// The `And|Or => false` arm is intentionally separate from the catch-all: it must
// precede the general `BinOp` arm to stop the walk at logical operators.
#[allow(clippy::match_same_arms)]
pub fn expr_directly_contains_candle_call(expr: &Expr) -> bool {
    match expr {
        Expr::FuncCall(name, args) => {
            CANDLE_FNS.contains(&name.as_str())
                || args.iter().any(expr_directly_contains_candle_call)
        }
        // Stop at logical operators — those are separate conditions in the chain
        Expr::BinOp(_, BinOp::And | BinOp::Or, _) => false,
        Expr::BinOp(l, _, r) => {
            expr_directly_contains_candle_call(l) || expr_directly_contains_candle_call(r)
        }
        Expr::Ternary(c, t, e) => {
            expr_directly_contains_candle_call(c)
                || expr_directly_contains_candle_call(t)
                || expr_directly_contains_candle_call(e)
        }
        Expr::Cast(_, inner)
        | Expr::Not(inner)
        | Expr::AddressOf(inner)
        | Expr::PostIncrement(inner)
        | Expr::PostDecrement(inner)
        | Expr::PreIncrement(inner)
        | Expr::PreDecrement(inner) => expr_directly_contains_candle_call(inner),
        Expr::ArrayAccess(_, idx) => expr_directly_contains_candle_call(idx),
        Expr::Var(_) | Expr::Literal(_) | Expr::IntLiteral(_) | Expr::PointerDeref(_) => false,
    }
}

/// PascalCase a single word: lowercase it, then upper-case the first character
/// (`"SMA"` → `"Sma"`, `"rsi"` → `"Rsi"`). For multi-segment `snake_case` names,
/// backends use their own underscore-splitting `pascal_words` instead.
pub fn pascal_word(s: &str) -> String {
    let lower = s.to_lowercase();
    let mut chars = lower.chars();
    match chars.next() {
        None => String::new(),
        Some(c) => c.to_uppercase().collect::<String>() + chars.as_str(),
    }
}

/// True if any statement is `return ALLOC_ERR;`.
pub fn contains_alloc_err_return(stmts: &[Statement]) -> bool {
    // "Err(RetCode::AllocErr)" is the Rust stream tier's pre-mapped form of the
    // same return (`map_return_code` runs before rendering); no batch IR ever
    // carries it, so recognizing it is batch-invariant.
    stmts.iter().any(|s| matches!(s, Statement::Return { value: Some(Expr::Var(name)) }
        if name == "ALLOC_ERR" || name == "Err(RetCode::AllocErr)"))
}

/// If `expr` is (or recursively contains) a `sizeof(TYPE)`, return the type name.
pub fn find_sizeof_type(expr: &Expr) -> Option<String> {
    match expr {
        Expr::FuncCall(name, args) if name == "sizeof" => args.first().and_then(|a| match a {
            Expr::Var(type_name) => Some(type_name.clone()),
            _ => None,
        }),
        Expr::BinOp(left, _, right) => find_sizeof_type(left).or_else(|| find_sizeof_type(right)),
        Expr::Cast(_, inner) => find_sizeof_type(inner),
        _ => None,
    }
}
