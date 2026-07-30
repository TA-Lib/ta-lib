//! Pure IR-inspection helpers shared by the language backends.
//!
//! These walk the backend-agnostic [`Statement`]/[`Expr`] AST and contain no
//! language-specific output, so every backend shares one copy instead of
//! re-implementing them. New backends import from here rather than copy-pasting.

use crate::ir::{BinOp, Expr, Statement};

// TA-Lib's parameter sentinels, mirroring include/ta_defs.h. The "use the
// default" value deliberately sits one step OUTSIDE the legal range so
// `x == DEFAULT` can never collide with real data — hence -4e37 vs -3e37, and
// i32::MIN vs i32::MIN+1. DBL_MIN/DBL_MAX cannot express that scheme (there is
// no representable double outside them to spare), which is why these are fixed
// decimals. Values are pinned by the ta_defs_sentinels_match test below.
pub const TA_REAL_MIN: f64 = -3e37;
pub const TA_REAL_MAX: f64 = 3e37;
pub const TA_REAL_DEFAULT: f64 = -4e37;
pub const TA_INTEGER_MIN: i32 = i32::MIN + 1;
pub const TA_INTEGER_MAX: i32 = i32::MAX;
pub const TA_INTEGER_DEFAULT: i32 = i32::MIN;

/// Render a real range bound: the `REAL_MIN`/`REAL_MAX` sentinels by name, anything
/// else as a literal. `prefix` is the backend's namespace (`"TA_"` for C and Java,
/// empty for Rust, whose crate already namespaces them) — so the generated sources
/// name the constant instead of repeating ±3e37.
#[must_use]
#[allow(clippy::float_cmp)] // these are exact sentinel values, not measurements
pub fn real_bound_literal(v: f64, prefix: &str) -> String {
    if v == TA_REAL_MIN {
        format!("{prefix}REAL_MIN")
    } else if v == TA_REAL_MAX {
        format!("{prefix}REAL_MAX")
    } else {
        format!("{v:e}")
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

#[cfg(test)]
mod tests {
    use super::*;

    /// The sentinels above are a copy of `include/ta_defs.h`, which is hand-written
    /// public ABI. Nothing else notices when the two drift: a wrong value silently
    /// widens or narrows every generated range check (that is exactly how
    /// `TA_REAL_MAX` came to be emitted as `f64::MAX`). Pin them to the header.
    #[test]
    #[allow(clippy::float_cmp)] // pinning exact literals is the point
    fn ta_defs_sentinels_match() {
        let hdr = std::fs::read_to_string(
            std::path::Path::new(env!("CARGO_MANIFEST_DIR")).join("../../include/ta_defs.h"),
        )
        .expect("include/ta_defs.h");
        for (macro_name, expected) in [
            ("TA_REAL_MIN", "(-3e+37)"),
            ("TA_REAL_MAX", "(3e+37)"),
            ("TA_REAL_DEFAULT", "(-4e+37)"),
            ("TA_INTEGER_MIN", "(INT_MIN+1)"),
            ("TA_INTEGER_MAX", "(INT_MAX)"),
            ("TA_INTEGER_DEFAULT", "(INT_MIN)"),
        ] {
            let needle = format!("#define {macro_name} ");
            let line = hdr
                .lines()
                .find(|l| l.starts_with(&needle))
                .unwrap_or_else(|| panic!("{macro_name} not found in ta_defs.h"));
            assert_eq!(
                line[needle.len()..].trim(),
                expected,
                "{macro_name} changed in ta_defs.h; update backends::common to match"
            );
        }
        // And that our Rust values are those literals.
        assert_eq!(TA_REAL_MIN, -3e37);
        assert_eq!(TA_REAL_MAX, 3e37);
        assert_eq!(TA_REAL_DEFAULT, -4e37);
        assert_eq!(TA_INTEGER_MIN, i32::MIN + 1);
        assert_eq!(TA_INTEGER_MAX, i32::MAX);
        assert_eq!(TA_INTEGER_DEFAULT, i32::MIN);
    }
}
