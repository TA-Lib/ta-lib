//! Detection and unpacking of candlestick settings variables.
//!
//! Candlestick indicators reference variables like `BodyLong_rangeType`,
//! `BodyLong_avgPeriod`, `BodyLong_factor`. This module scans statement bodies
//! to discover which settings are used and provides per-backend unpacking code.

use std::collections::BTreeSet;

use crate::ir::{Expr, Statement};

/// Known candlestick setting names.
const CANDLE_SETTINGS: &[&str] = &[
    "BodyLong",
    "BodyVeryLong",
    "BodyShort",
    "BodyDoji",
    "ShadowLong",
    "ShadowVeryLong",
    "ShadowShort",
    "ShadowVeryShort",
    "Near",
    "Far",
    "Equal",
];

/// The three properties each candle setting can reference.
const CANDLE_PROPERTIES: &[&str] = &["rangeType", "avgPeriod", "factor"];

/// Scan a slice of statements and return the set of candle setting names referenced.
///
/// Walks all expressions recursively looking for `Expr::Var("BodyLong_rangeType")` etc.
/// Returns e.g. `{"BodyLong", "ShadowShort"}`.
pub fn detect_candle_settings(stmts: &[Statement]) -> BTreeSet<String> {
    let mut found = BTreeSet::new();
    for stmt in stmts {
        scan_statement(stmt, &mut found);
    }
    found
}

fn scan_statement(stmt: &Statement, found: &mut BTreeSet<String>) {
    match stmt {
        Statement::VarDecl { init, .. } => {
            if let Some(expr) = init {
                scan_expr(expr, found);
            }
        }
        Statement::Assign { target, value, .. } => {
            scan_expr(target, found);
            scan_expr(value, found);
        }
        Statement::While { condition, body }
        | Statement::DoWhile { condition, body } => {
            scan_expr(condition, found);
            for s in body {
                scan_statement(s, found);
            }
        }
        Statement::For { count, body, .. } => {
            scan_expr(count, found);
            for s in body {
                scan_statement(s, found);
            }
        }
        Statement::ForC {
            init,
            condition,
            update,
            body,
        } => {
            scan_statement(init, found);
            scan_expr(condition, found);
            scan_statement(update, found);
            for s in body {
                scan_statement(s, found);
            }
        }
        Statement::If {
            condition,
            then_body,
            else_body,
            ..
        } => {
            scan_expr(condition, found);
            for s in then_body {
                scan_statement(s, found);
            }
            for s in else_body {
                scan_statement(s, found);
            }
        }
        Statement::Return { value } => {
            if let Some(expr) = value {
                scan_expr(expr, found);
            }
        }
        Statement::Switch {
            expr,
            cases,
            default,
        } => {
            scan_expr(expr, found);
            for (_, case_body) in cases {
                for s in case_body {
                    scan_statement(s, found);
                }
            }
            for s in default {
                scan_statement(s, found);
            }
        }
        Statement::Block { body } => {
            for s in body {
                scan_statement(s, found);
            }
        }
        Statement::Expr(e) => {
            scan_expr(e, found);
        }
        Statement::UnrollHint { .. }
        | Statement::Break
        | Statement::Continue
        | Statement::CircBuf(_)
        | Statement::Comment(_) => {}
    }
}

fn scan_expr(expr: &Expr, found: &mut BTreeSet<String>) {
    match expr {
        Expr::Var(name) => {
            check_candle_var(name, found);
        }
        Expr::ArrayAccess(name, idx) => {
            check_candle_var(name, found);
            scan_expr(idx, found);
        }
        Expr::BinOp(left, _, right) => {
            scan_expr(left, found);
            scan_expr(right, found);
        }
        Expr::Cast(_, inner)
        | Expr::Not(inner)
        | Expr::AddressOf(inner)
        | Expr::PostIncrement(inner)
        | Expr::PostDecrement(inner)
        | Expr::PreIncrement(inner)
        | Expr::PreDecrement(inner) => {
            scan_expr(inner, found);
        }
        Expr::FuncCall(_, args) => {
            for arg in args {
                scan_expr(arg, found);
            }
        }
        Expr::Ternary(cond, then_expr, else_expr) => {
            scan_expr(cond, found);
            scan_expr(then_expr, found);
            scan_expr(else_expr, found);
        }
        Expr::Literal(_) | Expr::IntLiteral(_) | Expr::PointerDeref(_) => {}
    }
}

/// Check if a variable name matches `{SettingName}_{property}` and record the setting.
fn check_candle_var(name: &str, found: &mut BTreeSet<String>) {
    for setting in CANDLE_SETTINGS {
        for prop in CANDLE_PROPERTIES {
            let pattern = format!("{setting}_{prop}");
            if name == pattern {
                found.insert((*setting).to_string());
                return;
            }
        }
    }
}

// ---------------------------------------------------------------------------
// Per-backend unpacking code generation
// ---------------------------------------------------------------------------

/// Emit C unpacking lines for the given candle settings.
///
/// ```c
/// int BodyLong_rangeType = TA_Globals->candleSettings[TA_BodyLong].rangeType;
/// int BodyLong_avgPeriod = TA_Globals->candleSettings[TA_BodyLong].avgPeriod;
/// double BodyLong_factor = TA_Globals->candleSettings[TA_BodyLong].factor;
/// ```
pub fn emit_c_unpacking(settings: &BTreeSet<String>, indent: usize) -> String {
    let pad = " ".repeat(indent);
    let mut out = String::new();
    for setting in settings {
        out.push_str(&format!(
            "{pad}int {setting}_rangeType = TA_Globals->candleSettings[TA_{setting}].rangeType;\n"
        ));
        out.push_str(&format!(
            "{pad}int {setting}_avgPeriod = TA_Globals->candleSettings[TA_{setting}].avgPeriod;\n"
        ));
        out.push_str(&format!(
            "{pad}double {setting}_factor = TA_Globals->candleSettings[TA_{setting}].factor;\n"
        ));
    }
    out
}

/// Emit Rust unpacking lines for the given candle settings.
///
/// ```rust,ignore
/// #[allow(non_snake_case)]
/// let BodyLong_rangeType: i32 = self.candle_settings.body_long.range_type;
/// ```
pub fn emit_rust_unpacking(settings: &BTreeSet<String>, indent: usize) -> String {
    let pad = " ".repeat(indent);
    let mut out = String::new();
    for setting in settings {
        let snake = pascal_to_snake_case(setting);
        out.push_str(&format!(
            "{pad}#[allow(non_snake_case)]\n\
             {pad}let {setting}_rangeType: i32 = self.candle_settings.{snake}.range_type;\n"
        ));
        out.push_str(&format!(
            "{pad}#[allow(non_snake_case)]\n\
             {pad}let {setting}_avgPeriod: i32 = self.candle_settings.{snake}.avg_period;\n"
        ));
        out.push_str(&format!(
            "{pad}#[allow(non_snake_case)]\n\
             {pad}let {setting}_factor: f64 = self.candle_settings.{snake}.factor;\n"
        ));
    }
    out
}

/// Emit Java unpacking lines for the given candle settings.
///
/// ```java
/// int BodyLong_rangeType = this.candleSettings[CandleSettingType.BodyLong.ordinal()].rangeType.ordinal();
/// int BodyLong_avgPeriod = this.candleSettings[CandleSettingType.BodyLong.ordinal()].avgPeriod;
/// double BodyLong_factor = this.candleSettings[CandleSettingType.BodyLong.ordinal()].factor;
/// ```
///
/// This is the canonical shipped-`Core.java` access form: `candleSettings` is a
/// `CandleSetting[]` indexed by `CandleSettingType` ordinals. `rangeType` is a
/// `RangeType` enum there, so it is read via `.ordinal()` into the `int` local the
/// candle ternaries compare against (`RealBody`=0, `HighLow`=1, `Shadows`=2). Shared
/// with the JSON-RPC Java server, whose `Core` mirrors this array + `RangeType` model.
pub fn emit_java_unpacking(settings: &BTreeSet<String>, indent: usize) -> String {
    let pad = " ".repeat(indent);
    let mut out = String::new();
    for setting in settings {
        out.push_str(&format!(
            "{pad}int {setting}_rangeType = this.candleSettings[CandleSettingType.{setting}.ordinal()].rangeType.ordinal();\n"
        ));
        out.push_str(&format!(
            "{pad}int {setting}_avgPeriod = this.candleSettings[CandleSettingType.{setting}.ordinal()].avgPeriod;\n"
        ));
        out.push_str(&format!(
            "{pad}double {setting}_factor = this.candleSettings[CandleSettingType.{setting}.ordinal()].factor;\n"
        ));
    }
    out
}

/// Convert `PascalCase` to `snake_case`.
///
/// `"BodyLong"` -> `"body_long"`, `"ShadowVeryShort"` -> `"shadow_very_short"`
fn pascal_to_snake_case(s: &str) -> String {
    let mut result = String::new();
    for (i, ch) in s.chars().enumerate() {
        if ch.is_uppercase() && i > 0 {
            result.push('_');
        }
        result.push(ch.to_ascii_lowercase());
    }
    result
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn pascal_to_snake_case_converts_correctly() {
        assert_eq!(pascal_to_snake_case("BodyLong"), "body_long");
        assert_eq!(pascal_to_snake_case("BodyVeryLong"), "body_very_long");
        assert_eq!(pascal_to_snake_case("ShadowVeryShort"), "shadow_very_short");
        assert_eq!(pascal_to_snake_case("Near"), "near");
        assert_eq!(pascal_to_snake_case("Equal"), "equal");
    }

    #[test]
    fn detect_finds_settings_in_var_refs() {
        let stmts = vec![
            Statement::Assign {
                target: Expr::Var("x".to_string()),
                value: Expr::Var("BodyLong_avgPeriod".to_string()),
                compound: false,
            },
            Statement::Assign {
                target: Expr::Var("y".to_string()),
                value: Expr::BinOp(
                    Box::new(Expr::Var("ShadowShort_rangeType".to_string())),
                    crate::ir::BinOp::Add,
                    Box::new(Expr::IntLiteral(1)),
                ),
                compound: false,
            },
        ];
        let found = detect_candle_settings(&stmts);
        assert!(found.contains("BodyLong"));
        assert!(found.contains("ShadowShort"));
        assert_eq!(found.len(), 2);
    }

    #[test]
    fn detect_finds_settings_in_func_call_args() {
        let stmts = vec![Statement::Assign {
            target: Expr::Var("x".to_string()),
            value: Expr::FuncCall(
                "ta_candlerange".to_string(),
                vec![
                    Expr::Var("BodyLong_rangeType".to_string()),
                    Expr::Var("a".to_string()),
                ],
            ),
            compound: false,
        }];
        let found = detect_candle_settings(&stmts);
        assert!(found.contains("BodyLong"));
        assert_eq!(found.len(), 1);
    }

    #[test]
    fn detect_returns_empty_for_non_candle_vars() {
        let stmts = vec![Statement::Assign {
            target: Expr::Var("x".to_string()),
            value: Expr::Var("periodTotal".to_string()),
            compound: false,
        }];
        let found = detect_candle_settings(&stmts);
        assert!(found.is_empty());
    }

    #[test]
    fn detect_recurses_into_nested_structures() {
        let stmts = vec![Statement::If {
            condition: Expr::Var("x".to_string()),
            then_body: vec![Statement::While {
                condition: Expr::Var("y".to_string()),
                body: vec![Statement::Assign {
                    target: Expr::Var("z".to_string()),
                    value: Expr::Var("Far_factor".to_string()),
                    compound: false,
                }],
            }],
            else_body: vec![Statement::Assign {
                target: Expr::Var("w".to_string()),
                value: Expr::Var("Equal_avgPeriod".to_string()),
                compound: false,
            }],
            cond_comments: vec![],
        }];
        let found = detect_candle_settings(&stmts);
        assert!(found.contains("Far"));
        assert!(found.contains("Equal"));
        assert_eq!(found.len(), 2);
    }

    #[test]
    fn c_unpacking_emits_correct_code() {
        let mut settings = BTreeSet::new();
        settings.insert("BodyLong".to_string());
        let code = emit_c_unpacking(&settings, 3);
        assert!(code.contains("int BodyLong_rangeType = TA_Globals->candleSettings[TA_BodyLong].rangeType;"));
        assert!(code.contains("int BodyLong_avgPeriod = TA_Globals->candleSettings[TA_BodyLong].avgPeriod;"));
        assert!(code.contains("double BodyLong_factor = TA_Globals->candleSettings[TA_BodyLong].factor;"));
    }

    #[test]
    fn rust_unpacking_emits_correct_code() {
        let mut settings = BTreeSet::new();
        settings.insert("BodyLong".to_string());
        let code = emit_rust_unpacking(&settings, 8);
        assert!(code.contains("self.candle_settings.body_long.range_type"));
        assert!(code.contains("self.candle_settings.body_long.avg_period"));
        assert!(code.contains("self.candle_settings.body_long.factor"));
        assert!(code.contains("#[allow(non_snake_case)]"));
    }

    #[test]
    fn java_unpacking_emits_correct_code() {
        let mut settings = BTreeSet::new();
        settings.insert("BodyLong".to_string());
        let code = emit_java_unpacking(&settings, 6);
        assert!(code.contains("this.candleSettings[CandleSettingType.BodyLong.ordinal()].rangeType.ordinal();"));
        assert!(code.contains("this.candleSettings[CandleSettingType.BodyLong.ordinal()].avgPeriod;"));
        assert!(code.contains("this.candleSettings[CandleSettingType.BodyLong.ordinal()].factor;"));
    }

    #[test]
    fn only_referenced_settings_emitted() {
        let mut settings = BTreeSet::new();
        settings.insert("ShadowVeryShort".to_string());
        let code = emit_c_unpacking(&settings, 3);
        assert!(code.contains("ShadowVeryShort_rangeType"));
        assert!(!code.contains("BodyLong"));
    }
}
