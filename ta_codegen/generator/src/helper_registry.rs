use std::collections::HashMap;
use std::path::Path;

use crate::ir::{Expr, HelperDef, Statement, VarType};
use crate::parser::c_source::parse_helper_file;

/// Registry of helper functions available for inlining.
pub struct HelperRegistry {
    helpers: HashMap<String, HelperDef>,
}

impl HelperRegistry {
    /// Build the registry by scanning `base_dir/helpers/*.c`.
    pub fn from_dir(base_dir: &Path) -> Self {
        let mut helpers = HashMap::new();
        let helpers_dir = base_dir.join("helpers");

        if let Ok(entries) = std::fs::read_dir(&helpers_dir) {
            for entry in entries.filter_map(Result::ok) {
                let path = entry.path();
                if path.extension().and_then(|e| e.to_str()) == Some("c") {
                    let parsed = parse_helper_file(&path);
                    for helper in parsed {
                        helpers.insert(helper.name.clone(), helper);
                    }
                }
            }
        }

        HelperRegistry { helpers }
    }

    /// Create an empty registry (for tests that don't need helper inlining).
    pub fn empty() -> Self {
        HelperRegistry {
            helpers: HashMap::new(),
        }
    }

    /// Look up a helper by function name.
    pub fn get(&self, name: &str) -> Option<&HelperDef> {
        self.helpers.get(name)
    }

    /// Build a registry from already-parsed helpers (tests, fixtures).
    pub fn from_defs(defs: Vec<HelperDef>) -> Self {
        HelperRegistry {
            helpers: defs.into_iter().map(|h| (h.name.clone(), h)).collect(),
        }
    }

    /// Every registered helper. Order is unspecified (the backing map is a
    /// `HashMap`), so callers must not let it decide anything — see
    /// `rust_lang::helper_local_ty`, which requires all matches to agree.
    pub fn iter(&self) -> impl Iterator<Item = &HelperDef> {
        self.helpers.values()
    }
}

/// Substitute helper parameters with actual arguments in an expression.
/// Recurses into ALL `Expr` variants. No `_ =>` fallback.
#[allow(clippy::implicit_hasher)]
pub fn substitute_expr(expr: &Expr, subs: &HashMap<String, Expr>) -> Expr {
    match expr {
        Expr::Var(name) => subs
            .get(name.as_str())
            .cloned()
            .unwrap_or_else(|| expr.clone()),
        Expr::Literal(_) | Expr::IntLiteral(_) | Expr::PointerDeref(_) => expr.clone(),
        Expr::BinOp(l, op, r) => Expr::BinOp(
            Box::new(substitute_expr(l, subs)),
            op.clone(),
            Box::new(substitute_expr(r, subs)),
        ),
        Expr::FuncCall(name, args) => Expr::FuncCall(
            name.clone(),
            args.iter().map(|a| substitute_expr(a, subs)).collect(),
        ),
        Expr::ArrayAccess(name, idx) => {
            Expr::ArrayAccess(name.clone(), Box::new(substitute_expr(idx, subs)))
        }
        Expr::Ternary(cond, then_e, else_e) => Expr::Ternary(
            Box::new(substitute_expr(cond, subs)),
            Box::new(substitute_expr(then_e, subs)),
            Box::new(substitute_expr(else_e, subs)),
        ),
        Expr::Cast(vt, inner) => Expr::Cast(vt.clone(), Box::new(substitute_expr(inner, subs))),
        Expr::Not(inner) => Expr::Not(Box::new(substitute_expr(inner, subs))),
        Expr::BitwiseNot(inner) => Expr::BitwiseNot(Box::new(substitute_expr(inner, subs))),
        Expr::AddressOf(inner) => Expr::AddressOf(Box::new(substitute_expr(inner, subs))),
        Expr::PostIncrement(inner) => {
            Expr::PostIncrement(Box::new(substitute_expr(inner, subs)))
        }
        Expr::PostDecrement(inner) => {
            Expr::PostDecrement(Box::new(substitute_expr(inner, subs)))
        }
        Expr::PreIncrement(inner) => {
            Expr::PreIncrement(Box::new(substitute_expr(inner, subs)))
        }
        Expr::PreDecrement(inner) => {
            Expr::PreDecrement(Box::new(substitute_expr(inner, subs)))
        }
    }
}

/// Try to inline a helper call as a single expression.
/// Returns `Some(inlined_expr)` for single-expression helpers (body is one `Return`),
/// `None` for multi-statement helpers.
pub fn try_inline_expr(helper: &HelperDef, args: &[Expr]) -> Option<Expr> {
    if helper.body.len() == 1 {
        if let Statement::Return {
            value: Some(ret_expr),
        } = &helper.body[0]
        {
            let subs: HashMap<String, Expr> = helper
                .params
                .iter()
                .zip(args.iter())
                .map(|(p, a)| (p.name.clone(), a.clone()))
                .collect();
            return Some(substitute_expr(ret_expr, &subs));
        }
    }
    None
}

// ---------------------------------------------------------------------------
// Block inlining for multi-statement helpers
// ---------------------------------------------------------------------------

/// Substitute params in a statement, renaming local vars to avoid collisions.
/// Handles all `Statement` variants exhaustively.
#[allow(clippy::too_many_lines, clippy::implicit_hasher)]
pub fn substitute_statement(
    stmt: &Statement,
    subs: &HashMap<String, Expr>,
    suffix: usize,
) -> Statement {
    match stmt {
        Statement::VarDecl {
            var_type,
            name,
            init,
        } => Statement::VarDecl {
            var_type: var_type.clone(),
            name: format!("{name}_{suffix}"),
            init: init.as_ref().map(|e| substitute_expr(e, subs)),
        },
        Statement::Assign {
            target,
            value,
            compound,
        } => {
            let new_target = substitute_expr(target, subs);
            Statement::Assign {
                target: new_target,
                value: substitute_expr(value, subs),
                compound: *compound,
            }
        }
        Statement::While { condition, body } => Statement::While {
            condition: substitute_expr(condition, subs),
            body: body
                .iter()
                .map(|s| substitute_statement(s, subs, suffix))
                .collect(),
        },
        Statement::DoWhile { condition, body } => Statement::DoWhile {
            condition: substitute_expr(condition, subs),
            body: body
                .iter()
                .map(|s| substitute_statement(s, subs, suffix))
                .collect(),
        },
        Statement::For { var, count, body } => Statement::For {
            var: format!("{var}_{suffix}"),
            count: substitute_expr(count, subs),
            body: body
                .iter()
                .map(|s| substitute_statement(s, subs, suffix))
                .collect(),
        },
        Statement::ForC {
            init,
            condition,
            update,
            body,
        } => Statement::ForC {
            init: Box::new(substitute_statement(init, subs, suffix)),
            condition: substitute_expr(condition, subs),
            update: Box::new(substitute_statement(update, subs, suffix)),
            body: body
                .iter()
                .map(|s| substitute_statement(s, subs, suffix))
                .collect(),
        },
        Statement::If {
            condition,
            then_body,
            else_body,
            cond_comments,
        } => Statement::If {
            condition: substitute_expr(condition, subs),
            then_body: then_body
                .iter()
                .map(|s| substitute_statement(s, subs, suffix))
                .collect(),
            else_body: else_body
                .iter()
                .map(|s| substitute_statement(s, subs, suffix))
                .collect(),
            cond_comments: cond_comments.clone(),
        },
        Statement::Return { value } => Statement::Return {
            value: value.as_ref().map(|e| {
                substitute_expr(e, subs)
            }),
        },
        Statement::Switch {
            expr,
            cases,
            default,
        } => Statement::Switch {
            expr: substitute_expr(expr, subs),
            cases: cases
                .iter()
                .map(|(label, stmts)| {
                    (
                        label.clone(),
                        stmts
                            .iter()
                            .map(|s| substitute_statement(s, subs, suffix))
                            .collect(),
                    )
                })
                .collect(),
            default: default
                .iter()
                .map(|s| substitute_statement(s, subs, suffix))
                .collect(),
        },
        Statement::Block { body } => Statement::Block {
            body: body
                .iter()
                .map(|s| substitute_statement(s, subs, suffix))
                .collect(),
        },
        Statement::Expr(e) => Statement::Expr(substitute_expr(e, subs)),
        Statement::UnrollHint { count } => Statement::UnrollHint { count: *count },
        Statement::Break => Statement::Break,
        Statement::Continue => Statement::Continue,
        Statement::CircBuf(op) => Statement::CircBuf(op.clone()),
        Statement::Comment(lines) => Statement::Comment(lines.clone()),
    }
}

/// Collect local variable names declared in a helper body (VarDecl names),
/// including those declared inside nested blocks, so that every helper-local
/// is renamed consistently when the helper is inlined more than once.
fn collect_local_names(body: &[Statement]) -> Vec<String> {
    let mut names = Vec::new();
    for stmt in body {
        match stmt {
            Statement::VarDecl { name, .. } => names.push(name.clone()),
            Statement::While { body, .. }
            | Statement::DoWhile { body, .. }
            | Statement::For { body, .. }
            | Statement::Block { body } => names.extend(collect_local_names(body)),
            Statement::If { then_body, else_body, .. } => {
                names.extend(collect_local_names(then_body));
                names.extend(collect_local_names(else_body));
            }
            Statement::ForC { init, update, body, .. } => {
                names.extend(collect_local_names(std::slice::from_ref(init)));
                names.extend(collect_local_names(std::slice::from_ref(update)));
                names.extend(collect_local_names(body));
            }
            Statement::Switch { cases, default, .. } => {
                for (_, stmts) in cases {
                    names.extend(collect_local_names(stmts));
                }
                names.extend(collect_local_names(default));
            }
            Statement::Assign { .. }
            | Statement::Expr(_)
            | Statement::Return { .. }
            | Statement::UnrollHint { .. }
            | Statement::Break
            | Statement::Continue
            | Statement::CircBuf(_)
            | Statement::Comment(_) => {}
        }
    }
    names
}


/// Inline a multi-statement helper. Returns (substituted body, temp var name).
///
/// The body has parameter names replaced with argument expressions, local
/// variable names suffixed with `_N` to avoid collisions, and `Return`
/// statements replaced with assignments to the temp var.
pub fn inline_block(
    helper: &HelperDef,
    args: &[Expr],
    counter: &mut usize,
) -> (Vec<Statement>, String) {
    let suffix = *counter;
    *counter += 1;

    // Build parameter substitution map
    let subs: HashMap<String, Expr> = helper
        .params
        .iter()
        .zip(args.iter())
        .map(|(p, a)| (p.name.clone(), a.clone()))
        .collect();

    // Collect local variable names that need renaming
    let locals = collect_local_names(&helper.body);

    // Build a second substitution map for local variable renaming
    let mut local_subs: HashMap<String, Expr> = subs.clone();
    for local in &locals {
        if !subs.contains_key(local) {
            local_subs.insert(
                local.clone(),
                Expr::Var(format!("{local}_{suffix}")),
            );
        }
    }

    let temp_name = format!(
        "_{}_{suffix}",
        helper.name.trim_start_matches("ta_")
    );

    let mut body: Vec<Statement> = helper
        .body
        .iter()
        .map(|s| substitute_statement(s, &local_subs, suffix))
        .collect();

    replace_returns_with_assign(&mut body, &temp_name);

    (body, temp_name)
}

/// Replace `Return { value: Some(expr) }` with
/// `Assign { target: Var(temp_name), value: expr }` throughout a statement list.
/// Recurses into nested `If`, `Switch`, `While`, `DoWhile`, `For`, `ForC`, `Block`.
#[allow(clippy::match_same_arms)]
fn replace_returns_with_assign(body: &mut [Statement], temp_name: &str) {
    for stmt in body.iter_mut() {
        match stmt {
            Statement::Return { value: Some(expr) } => {
                *stmt = Statement::Assign {
                    target: Expr::Var(temp_name.to_string()),
                    value: expr.clone(),
                    compound: false,
                };
            }
            Statement::If {
                then_body,
                else_body,
                ..
            } => {
                replace_returns_with_assign(then_body, temp_name);
                replace_returns_with_assign(else_body, temp_name);
            }
            Statement::Switch {
                cases, default, ..
            } => {
                for (_, stmts) in cases.iter_mut() {
                    replace_returns_with_assign(stmts, temp_name);
                }
                replace_returns_with_assign(default, temp_name);
            }
            Statement::While { body, .. } | Statement::DoWhile { body, .. } => {
                replace_returns_with_assign(body, temp_name);
            }
            Statement::For { body, .. } | Statement::ForC { body, .. } => {
                replace_returns_with_assign(body, temp_name);
            }
            Statement::Block { body } => {
                replace_returns_with_assign(body, temp_name);
            }
            Statement::Return { value: None }
            | Statement::VarDecl { .. }
            | Statement::Assign { .. }
            | Statement::Expr(_)
            | Statement::UnrollHint { .. }
            | Statement::Break
            | Statement::Continue
            | Statement::CircBuf(_)
            | Statement::Comment(_) => {}
        }
    }
}

/// Walk an expression tree, replacing multi-statement helper `FuncCall`s with
/// `Var(temp_name)`. The hoisted blocks (declaration + body) are collected in
/// `hoisted` and should be emitted before the containing statement.
///
/// Inner calls are hoisted first (depth-first), so nested helpers like
/// `ta_candleaverage` calling `ta_candlerange` work correctly.
///
/// Functions listed in `skip_fns` are left as `FuncCall` nodes (not hoisted).
/// The C backend uses this to emit candle macros instead of expanded code.
pub fn hoist_block_helpers(
    expr: &Expr,
    helpers: &HelperRegistry,
    hoisted: &mut Vec<(String, VarType, Vec<Statement>)>,
    counter: &mut usize,
    skip_fns: &[&str],
) -> Expr {
    match expr {
        Expr::FuncCall(name, args) => {
            // First recurse into args (inner calls get hoisted first)
            let new_args: Vec<Expr> = args
                .iter()
                .map(|a| hoist_block_helpers(a, helpers, hoisted, counter, skip_fns))
                .collect();
            // Skip hoisting for specified functions (C backend emits macros)
            if skip_fns.contains(&name.as_str()) {
                return Expr::FuncCall(name.clone(), new_args);
            }
            // Check if this is a multi-statement helper
            if let Some(helper) = helpers.get(name) {
                if try_inline_expr(helper, &new_args).is_none() {
                    let (body, temp_name) =
                        inline_block(helper, &new_args, counter);
                    hoisted.push((
                        temp_name.clone(),
                        helper.return_type.clone(),
                        body,
                    ));
                    return Expr::Var(temp_name);
                }
            }
            Expr::FuncCall(name.clone(), new_args)
        }
        Expr::BinOp(l, op, r) => Expr::BinOp(
            Box::new(hoist_block_helpers(l, helpers, hoisted, counter, skip_fns)),
            op.clone(),
            Box::new(hoist_block_helpers(r, helpers, hoisted, counter, skip_fns)),
        ),
        Expr::Ternary(cond, then_e, else_e) => Expr::Ternary(
            Box::new(hoist_block_helpers(cond, helpers, hoisted, counter, skip_fns)),
            Box::new(hoist_block_helpers(then_e, helpers, hoisted, counter, skip_fns)),
            Box::new(hoist_block_helpers(else_e, helpers, hoisted, counter, skip_fns)),
        ),
        Expr::Cast(vt, inner) => Expr::Cast(
            vt.clone(),
            Box::new(hoist_block_helpers(inner, helpers, hoisted, counter, skip_fns)),
        ),
        Expr::Not(inner) => {
            Expr::Not(Box::new(hoist_block_helpers(inner, helpers, hoisted, counter, skip_fns)))
        }
        Expr::BitwiseNot(inner) => Expr::BitwiseNot(Box::new(hoist_block_helpers(
            inner, helpers, hoisted, counter, skip_fns,
        ))),
        Expr::AddressOf(inner) => Expr::AddressOf(Box::new(hoist_block_helpers(
            inner, helpers, hoisted, counter, skip_fns,
        ))),
        Expr::PostIncrement(inner) => Expr::PostIncrement(Box::new(
            hoist_block_helpers(inner, helpers, hoisted, counter, skip_fns),
        )),
        Expr::PostDecrement(inner) => Expr::PostDecrement(Box::new(
            hoist_block_helpers(inner, helpers, hoisted, counter, skip_fns),
        )),
        Expr::PreIncrement(inner) => Expr::PreIncrement(Box::new(
            hoist_block_helpers(inner, helpers, hoisted, counter, skip_fns),
        )),
        Expr::PreDecrement(inner) => Expr::PreDecrement(Box::new(
            hoist_block_helpers(inner, helpers, hoisted, counter, skip_fns),
        )),
        Expr::ArrayAccess(name, idx) => Expr::ArrayAccess(
            name.clone(),
            Box::new(hoist_block_helpers(idx, helpers, hoisted, counter, skip_fns)),
        ),
        Expr::Var(name) => {
            // A bare Var cannot be a helper call; just clone.
            Expr::Var(name.clone())
        }
        Expr::Literal(_) | Expr::IntLiteral(_) | Expr::PointerDeref(_) => {
            expr.clone()
        }
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::ir::{BinOp, Expr, HelperDef, HelperParam, Statement, VarType};
    use std::collections::HashMap;
    use std::path::Path;

    /// Helper: build a substitution map from a slice of (name, expr) pairs.
    fn subs(pairs: &[(&str, Expr)]) -> HashMap<String, Expr> {
        pairs
            .iter()
            .map(|(k, v)| (k.to_string(), v.clone()))
            .collect()
    }

    /// Helper: build a simple HelperDef with the given body.
    fn make_helper(
        name: &str,
        params: &[(&str, VarType)],
        body: Vec<Statement>,
    ) -> HelperDef {
        HelperDef {
            name: name.to_string(),
            return_type: VarType::Real,
            params: params
                .iter()
                .map(|(n, vt)| HelperParam {
                    name: n.to_string(),
                    var_type: vt.clone(),
                })
                .collect(),
            body,
        }
    }

    // -----------------------------------------------------------------------
    // HelperRegistry tests
    // -----------------------------------------------------------------------

    #[test]
    fn test_empty_registry_returns_none() {
        let reg = HelperRegistry::empty();
        assert!(reg.get("ta_nonexistent").is_none());
    }

    #[test]
    fn test_from_dir_loads_helpers() {
        // Use the real ta_codegen/input directory (relative to CARGO_MANIFEST_DIR).
        let base = Path::new(env!("CARGO_MANIFEST_DIR")).join("../../ta_codegen/input");
        if !base.join("helpers").exists() {
            // Skip if not available in CI
            return;
        }
        let reg = HelperRegistry::from_dir(&base);
        // The candlestick helpers file should contain at least one helper.
        // We just verify the registry is non-empty and can look up something.
        assert!(
            reg.get("ta_candlerange").is_some()
                || reg.get("ta_realbody").is_some()
                || reg.get("ta_round").is_some(),
            "Expected at least one known helper to be loaded"
        );
    }

    #[test]
    fn test_from_dir_nonexistent_is_empty() {
        let reg = HelperRegistry::from_dir(Path::new("/nonexistent/path"));
        assert!(reg.get("anything").is_none());
    }

    // -----------------------------------------------------------------------
    // substitute_expr tests — cover every Expr variant
    // -----------------------------------------------------------------------

    #[test]
    fn test_substitute_expr_var_replacement() {
        let map = subs(&[("x", Expr::IntLiteral(42))]);
        let result = substitute_expr(&Expr::Var("x".into()), &map);
        assert!(matches!(result, Expr::IntLiteral(42)));
    }

    #[test]
    fn test_substitute_expr_var_no_match() {
        let map = subs(&[("x", Expr::IntLiteral(1))]);
        let result = substitute_expr(&Expr::Var("y".into()), &map);
        assert!(matches!(result, Expr::Var(ref n) if n == "y"));
    }

    #[test]
    fn test_substitute_expr_literal_passthrough() {
        let map = subs(&[]);
        let result = substitute_expr(&Expr::Literal(7.77), &map);
        assert!(matches!(result, Expr::Literal(v) if (v - 7.77).abs() < 1e-15));
    }

    #[test]
    fn test_substitute_expr_int_literal_passthrough() {
        let map = subs(&[]);
        let result = substitute_expr(&Expr::IntLiteral(99), &map);
        assert!(matches!(result, Expr::IntLiteral(99)));
    }

    #[test]
    fn test_substitute_expr_pointer_deref_passthrough() {
        let map = subs(&[]);
        let result = substitute_expr(&Expr::PointerDeref("ptr".into()), &map);
        assert!(matches!(result, Expr::PointerDeref(ref n) if n == "ptr"));
    }

    #[test]
    fn test_substitute_expr_binop() {
        let map = subs(&[("a", Expr::IntLiteral(1)), ("b", Expr::IntLiteral(2))]);
        let expr = Expr::BinOp(
            Box::new(Expr::Var("a".into())),
            BinOp::Add,
            Box::new(Expr::Var("b".into())),
        );
        let result = substitute_expr(&expr, &map);
        match result {
            Expr::BinOp(l, _, r) => {
                assert!(matches!(*l, Expr::IntLiteral(1)));
                assert!(matches!(*r, Expr::IntLiteral(2)));
            }
            other => panic!("Expected BinOp, got {other:?}"),
        }
    }

    #[test]
    fn test_substitute_expr_func_call() {
        let map = subs(&[("x", Expr::Literal(5.0))]);
        let expr = Expr::FuncCall(
            "my_func".into(),
            vec![Expr::Var("x".into()), Expr::IntLiteral(10)],
        );
        let result = substitute_expr(&expr, &map);
        match result {
            Expr::FuncCall(name, args) => {
                assert_eq!(name, "my_func");
                assert_eq!(args.len(), 2);
                assert!(matches!(args[0], Expr::Literal(v) if (v - 5.0).abs() < 1e-15));
                assert!(matches!(args[1], Expr::IntLiteral(10)));
            }
            other => panic!("Expected FuncCall, got {other:?}"),
        }
    }

    #[test]
    fn test_substitute_expr_array_access() {
        let map = subs(&[("i", Expr::IntLiteral(7))]);
        let expr = Expr::ArrayAccess("arr".into(), Box::new(Expr::Var("i".into())));
        let result = substitute_expr(&expr, &map);
        match result {
            Expr::ArrayAccess(name, idx) => {
                assert_eq!(name, "arr");
                assert!(matches!(*idx, Expr::IntLiteral(7)));
            }
            other => panic!("Expected ArrayAccess, got {other:?}"),
        }
    }

    #[test]
    fn test_substitute_expr_ternary() {
        let map = subs(&[("c", Expr::IntLiteral(1))]);
        let expr = Expr::Ternary(
            Box::new(Expr::Var("c".into())),
            Box::new(Expr::Literal(10.0)),
            Box::new(Expr::Literal(20.0)),
        );
        let result = substitute_expr(&expr, &map);
        match result {
            Expr::Ternary(cond, _, _) => {
                assert!(matches!(*cond, Expr::IntLiteral(1)));
            }
            other => panic!("Expected Ternary, got {other:?}"),
        }
    }

    #[test]
    fn test_substitute_expr_cast() {
        let map = subs(&[("x", Expr::Literal(3.0))]);
        let expr = Expr::Cast(VarType::Integer, Box::new(Expr::Var("x".into())));
        let result = substitute_expr(&expr, &map);
        match result {
            Expr::Cast(vt, inner) => {
                assert_eq!(vt, VarType::Integer);
                assert!(matches!(*inner, Expr::Literal(v) if (v - 3.0).abs() < 1e-15));
            }
            other => panic!("Expected Cast, got {other:?}"),
        }
    }

    #[test]
    fn test_substitute_expr_not() {
        let map = subs(&[("x", Expr::IntLiteral(0))]);
        let expr = Expr::Not(Box::new(Expr::Var("x".into())));
        let result = substitute_expr(&expr, &map);
        match result {
            Expr::Not(inner) => assert!(matches!(*inner, Expr::IntLiteral(0))),
            other => panic!("Expected Not, got {other:?}"),
        }
    }

    #[test]
    fn test_substitute_expr_address_of() {
        let map = subs(&[("x", Expr::Var("replaced".into()))]);
        let expr = Expr::AddressOf(Box::new(Expr::Var("x".into())));
        let result = substitute_expr(&expr, &map);
        match result {
            Expr::AddressOf(inner) => {
                assert!(matches!(*inner, Expr::Var(ref n) if n == "replaced"));
            }
            other => panic!("Expected AddressOf, got {other:?}"),
        }
    }

    #[test]
    fn test_substitute_expr_post_increment() {
        let map = subs(&[("i", Expr::Var("j".into()))]);
        let expr = Expr::PostIncrement(Box::new(Expr::Var("i".into())));
        let result = substitute_expr(&expr, &map);
        match result {
            Expr::PostIncrement(inner) => {
                assert!(matches!(*inner, Expr::Var(ref n) if n == "j"));
            }
            other => panic!("Expected PostIncrement, got {other:?}"),
        }
    }

    #[test]
    fn test_substitute_expr_post_decrement() {
        let map = subs(&[("i", Expr::Var("j".into()))]);
        let expr = Expr::PostDecrement(Box::new(Expr::Var("i".into())));
        let result = substitute_expr(&expr, &map);
        match result {
            Expr::PostDecrement(inner) => {
                assert!(matches!(*inner, Expr::Var(ref n) if n == "j"));
            }
            other => panic!("Expected PostDecrement, got {other:?}"),
        }
    }

    #[test]
    fn test_substitute_expr_pre_increment() {
        let map = subs(&[("i", Expr::Var("k".into()))]);
        let expr = Expr::PreIncrement(Box::new(Expr::Var("i".into())));
        let result = substitute_expr(&expr, &map);
        match result {
            Expr::PreIncrement(inner) => {
                assert!(matches!(*inner, Expr::Var(ref n) if n == "k"));
            }
            other => panic!("Expected PreIncrement, got {other:?}"),
        }
    }

    #[test]
    fn test_substitute_expr_pre_decrement() {
        let map = subs(&[("i", Expr::Var("k".into()))]);
        let expr = Expr::PreDecrement(Box::new(Expr::Var("i".into())));
        let result = substitute_expr(&expr, &map);
        match result {
            Expr::PreDecrement(inner) => {
                assert!(matches!(*inner, Expr::Var(ref n) if n == "k"));
            }
            other => panic!("Expected PreDecrement, got {other:?}"),
        }
    }

    // -----------------------------------------------------------------------
    // try_inline_expr tests
    // -----------------------------------------------------------------------

    #[test]
    fn test_try_inline_single_return_expr() {
        // Helper: double add(double a, double b) { return a + b; }
        let helper = make_helper(
            "ta_add",
            &[("a", VarType::Real), ("b", VarType::Real)],
            vec![Statement::Return {
                value: Some(Expr::BinOp(
                    Box::new(Expr::Var("a".into())),
                    BinOp::Add,
                    Box::new(Expr::Var("b".into())),
                )),
            }],
        );
        let args = vec![Expr::Literal(1.0), Expr::Literal(2.0)];
        let result = try_inline_expr(&helper, &args);
        assert!(result.is_some());
        match result.unwrap() {
            Expr::BinOp(l, _, r) => {
                assert!(matches!(*l, Expr::Literal(v) if (v - 1.0).abs() < 1e-15));
                assert!(matches!(*r, Expr::Literal(v) if (v - 2.0).abs() < 1e-15));
            }
            other => panic!("Expected BinOp, got {other:?}"),
        }
    }

    #[test]
    fn test_try_inline_multi_statement_returns_none() {
        // Multi-statement body => cannot inline as expression
        let helper = make_helper(
            "ta_complex",
            &[("x", VarType::Real)],
            vec![
                Statement::VarDecl {
                    var_type: VarType::Real,
                    name: "tmp".into(),
                    init: Some(Expr::Var("x".into())),
                },
                Statement::Return {
                    value: Some(Expr::Var("tmp".into())),
                },
            ],
        );
        let args = vec![Expr::Literal(1.0)];
        assert!(try_inline_expr(&helper, &args).is_none());
    }

    #[test]
    fn test_try_inline_return_without_value_returns_none() {
        let helper = make_helper(
            "ta_noop",
            &[],
            vec![Statement::Return { value: None }],
        );
        assert!(try_inline_expr(&helper, &[]).is_none());
    }

    // -----------------------------------------------------------------------
    // substitute_statement tests — cover While, DoWhile, For, ForC, Block,
    //                              Break, Continue
    // -----------------------------------------------------------------------

    #[test]
    fn test_substitute_statement_while() {
        let map = subs(&[("x", Expr::IntLiteral(10))]);
        let stmt = Statement::While {
            condition: Expr::Var("x".into()),
            body: vec![Statement::Assign {
                target: Expr::Var("x".into()),
                value: Expr::IntLiteral(0),
                compound: false,
            }],
        };
        let result = substitute_statement(&stmt, &map, 1);
        match result {
            Statement::While { condition, body } => {
                assert!(matches!(condition, Expr::IntLiteral(10)));
                assert_eq!(body.len(), 1);
            }
            other => panic!("Expected While, got {other:?}"),
        }
    }

    #[test]
    fn test_substitute_statement_do_while() {
        let map = subs(&[("n", Expr::Var("count".into()))]);
        let stmt = Statement::DoWhile {
            condition: Expr::Var("n".into()),
            body: vec![Statement::Break],
        };
        let result = substitute_statement(&stmt, &map, 2);
        match result {
            Statement::DoWhile { condition, body } => {
                assert!(matches!(condition, Expr::Var(ref n) if n == "count"));
                assert_eq!(body.len(), 1);
                assert!(matches!(body[0], Statement::Break));
            }
            other => panic!("Expected DoWhile, got {other:?}"),
        }
    }

    #[test]
    fn test_substitute_statement_for() {
        let map = subs(&[("limit", Expr::IntLiteral(100))]);
        let stmt = Statement::For {
            var: "i".into(),
            count: Expr::Var("limit".into()),
            body: vec![Statement::Continue],
        };
        let result = substitute_statement(&stmt, &map, 3);
        match result {
            Statement::For { var, count, body } => {
                assert_eq!(var, "i_3");
                assert!(matches!(count, Expr::IntLiteral(100)));
                assert_eq!(body.len(), 1);
                assert!(matches!(body[0], Statement::Continue));
            }
            other => panic!("Expected For, got {other:?}"),
        }
    }

    #[test]
    fn test_substitute_statement_for_c() {
        let map = subs(&[("n", Expr::IntLiteral(5))]);
        let stmt = Statement::ForC {
            init: Box::new(Statement::VarDecl {
                var_type: VarType::Index,
                name: "i".into(),
                init: Some(Expr::IntLiteral(0)),
            }),
            condition: Expr::BinOp(
                Box::new(Expr::Var("i".into())),
                BinOp::Less,
                Box::new(Expr::Var("n".into())),
            ),
            update: Box::new(Statement::Assign {
                target: Expr::Var("i".into()),
                value: Expr::BinOp(
                    Box::new(Expr::Var("i".into())),
                    BinOp::Add,
                    Box::new(Expr::IntLiteral(1)),
                ),
                compound: true,
            }),
            body: vec![Statement::Break],
        };
        let result = substitute_statement(&stmt, &map, 7);
        match result {
            Statement::ForC {
                init,
                condition,
                update,
                body,
            } => {
                // init should be a renamed VarDecl
                assert!(matches!(*init, Statement::VarDecl { ref name, .. } if name == "i_7"));
                // condition should have substituted 'n' -> 5
                assert!(matches!(condition, Expr::BinOp(_, BinOp::Less, ref r)
                    if matches!(**r, Expr::IntLiteral(5))));
                // update should exist
                assert!(matches!(*update, Statement::Assign { .. }));
                assert_eq!(body.len(), 1);
            }
            other => panic!("Expected ForC, got {other:?}"),
        }
    }

    #[test]
    fn test_substitute_statement_block() {
        let map = subs(&[("x", Expr::IntLiteral(42))]);
        let stmt = Statement::Block {
            body: vec![
                Statement::Assign {
                    target: Expr::Var("y".into()),
                    value: Expr::Var("x".into()),
                    compound: false,
                },
                Statement::Break,
            ],
        };
        let result = substitute_statement(&stmt, &map, 0);
        match result {
            Statement::Block { body } => {
                assert_eq!(body.len(), 2);
                assert!(matches!(body[1], Statement::Break));
            }
            other => panic!("Expected Block, got {other:?}"),
        }
    }

    #[test]
    fn test_substitute_statement_break() {
        let map = subs(&[]);
        let result = substitute_statement(&Statement::Break, &map, 0);
        assert!(matches!(result, Statement::Break));
    }

    #[test]
    fn test_substitute_statement_continue() {
        let map = subs(&[]);
        let result = substitute_statement(&Statement::Continue, &map, 0);
        assert!(matches!(result, Statement::Continue));
    }

    // -----------------------------------------------------------------------
    // replace_returns_with_assign — covers While, DoWhile, For, ForC, Block
    // -----------------------------------------------------------------------

    #[test]
    fn test_replace_returns_in_while() {
        let mut body = vec![Statement::While {
            condition: Expr::IntLiteral(1),
            body: vec![Statement::Return {
                value: Some(Expr::Literal(99.0)),
            }],
        }];
        replace_returns_with_assign(&mut body, "_result_0");
        match &body[0] {
            Statement::While {
                body: inner_body, ..
            } => match &inner_body[0] {
                Statement::Assign {
                    target, compound, ..
                } => {
                    assert!(matches!(target, Expr::Var(ref n) if n == "_result_0"));
                    assert!(!compound);
                }
                other => panic!("Expected Assign, got {other:?}"),
            },
            other => panic!("Expected While, got {other:?}"),
        }
    }

    #[test]
    fn test_replace_returns_in_do_while() {
        let mut body = vec![Statement::DoWhile {
            condition: Expr::IntLiteral(0),
            body: vec![Statement::Return {
                value: Some(Expr::IntLiteral(1)),
            }],
        }];
        replace_returns_with_assign(&mut body, "_tmp");
        match &body[0] {
            Statement::DoWhile {
                body: inner_body, ..
            } => {
                assert!(matches!(inner_body[0], Statement::Assign { .. }));
            }
            other => panic!("Expected DoWhile, got {other:?}"),
        }
    }

    #[test]
    fn test_replace_returns_in_for() {
        let mut body = vec![Statement::For {
            var: "i".into(),
            count: Expr::IntLiteral(10),
            body: vec![Statement::Return {
                value: Some(Expr::Var("i".into())),
            }],
        }];
        replace_returns_with_assign(&mut body, "_out");
        match &body[0] {
            Statement::For {
                body: inner_body, ..
            } => {
                assert!(matches!(inner_body[0], Statement::Assign { .. }));
            }
            other => panic!("Expected For, got {other:?}"),
        }
    }

    #[test]
    fn test_replace_returns_in_for_c() {
        let mut body = vec![Statement::ForC {
            init: Box::new(Statement::VarDecl {
                var_type: VarType::Index,
                name: "i".into(),
                init: Some(Expr::IntLiteral(0)),
            }),
            condition: Expr::IntLiteral(1),
            update: Box::new(Statement::Assign {
                target: Expr::Var("i".into()),
                value: Expr::IntLiteral(1),
                compound: true,
            }),
            body: vec![Statement::Return {
                value: Some(Expr::Literal(0.0)),
            }],
        }];
        replace_returns_with_assign(&mut body, "_fc_out");
        match &body[0] {
            Statement::ForC {
                body: inner_body, ..
            } => {
                assert!(matches!(inner_body[0], Statement::Assign { .. }));
            }
            other => panic!("Expected ForC, got {other:?}"),
        }
    }

    #[test]
    fn test_replace_returns_in_block() {
        let mut body = vec![Statement::Block {
            body: vec![Statement::Return {
                value: Some(Expr::IntLiteral(7)),
            }],
        }];
        replace_returns_with_assign(&mut body, "_blk");
        match &body[0] {
            Statement::Block {
                body: inner_body, ..
            } => {
                assert!(matches!(inner_body[0], Statement::Assign { .. }));
            }
            other => panic!("Expected Block, got {other:?}"),
        }
    }

    // -----------------------------------------------------------------------
    // inline_block tests
    // -----------------------------------------------------------------------

    #[test]
    fn test_inline_block_renames_locals_and_replaces_returns() {
        // Helper: double compute(double x) { double tmp = x * 2; return tmp; }
        let helper = make_helper(
            "ta_compute",
            &[("x", VarType::Real)],
            vec![
                Statement::VarDecl {
                    var_type: VarType::Real,
                    name: "tmp".into(),
                    init: Some(Expr::BinOp(
                        Box::new(Expr::Var("x".into())),
                        BinOp::Mul,
                        Box::new(Expr::IntLiteral(2)),
                    )),
                },
                Statement::Return {
                    value: Some(Expr::Var("tmp".into())),
                },
            ],
        );
        let args = vec![Expr::Literal(5.0)];
        let mut counter = 0_usize;
        let (body, temp_name) = inline_block(&helper, &args, &mut counter);

        assert_eq!(counter, 1);
        assert_eq!(temp_name, "_compute_0");
        assert_eq!(body.len(), 2);

        // First statement: VarDecl with renamed local "tmp_0"
        match &body[0] {
            Statement::VarDecl { name, .. } => assert_eq!(name, "tmp_0"),
            other => panic!("Expected VarDecl, got {other:?}"),
        }

        // Second statement: Return replaced with Assign to "_compute_0"
        match &body[1] {
            Statement::Assign { target, .. } => {
                assert!(matches!(target, Expr::Var(ref n) if n == "_compute_0"));
            }
            other => panic!("Expected Assign (from Return), got {other:?}"),
        }
    }

    #[test]
    fn test_inline_block_counter_increments() {
        let helper = make_helper(
            "ta_noop",
            &[],
            vec![Statement::Return {
                value: Some(Expr::IntLiteral(0)),
            }],
        );
        let mut counter = 5_usize;
        let (_, name1) = inline_block(&helper, &[], &mut counter);
        let (_, name2) = inline_block(&helper, &[], &mut counter);

        assert_eq!(name1, "_noop_5");
        assert_eq!(name2, "_noop_6");
        assert_eq!(counter, 7);
    }

    // -----------------------------------------------------------------------
    // hoist_block_helpers tests
    // -----------------------------------------------------------------------

    #[test]
    fn test_hoist_block_helpers_no_helpers() {
        let reg = HelperRegistry::empty();
        let mut hoisted = Vec::new();
        let mut counter = 0_usize;
        let expr = Expr::BinOp(
            Box::new(Expr::Var("a".into())),
            BinOp::Add,
            Box::new(Expr::Var("b".into())),
        );
        let result = hoist_block_helpers(&expr, &reg, &mut hoisted, &mut counter, &[]);
        assert!(hoisted.is_empty());
        assert!(matches!(result, Expr::BinOp(..)));
    }

    #[test]
    fn test_hoist_block_helpers_single_return_stays_func_call() {
        // Single-return helpers are inlined as expressions by try_inline_expr,
        // so hoist_block_helpers should NOT hoist them.
        let mut helpers = HashMap::new();
        helpers.insert(
            "ta_add".into(),
            make_helper(
                "ta_add",
                &[("a", VarType::Real), ("b", VarType::Real)],
                vec![Statement::Return {
                    value: Some(Expr::BinOp(
                        Box::new(Expr::Var("a".into())),
                        BinOp::Add,
                        Box::new(Expr::Var("b".into())),
                    )),
                }],
            ),
        );
        let reg = HelperRegistry { helpers };
        let mut hoisted = Vec::new();
        let mut counter = 0_usize;
        let expr = Expr::FuncCall(
            "ta_add".into(),
            vec![Expr::Literal(1.0), Expr::Literal(2.0)],
        );
        let result = hoist_block_helpers(&expr, &reg, &mut hoisted, &mut counter, &[]);
        // Single-return: not hoisted, stays as FuncCall (the caller does try_inline_expr)
        assert!(hoisted.is_empty());
        assert!(matches!(result, Expr::FuncCall(..)));
    }

    #[test]
    fn test_hoist_block_helpers_multi_statement_hoisted() {
        let mut helpers = HashMap::new();
        helpers.insert(
            "ta_complex".into(),
            make_helper(
                "ta_complex",
                &[("x", VarType::Real)],
                vec![
                    Statement::VarDecl {
                        var_type: VarType::Real,
                        name: "tmp".into(),
                        init: Some(Expr::Var("x".into())),
                    },
                    Statement::Return {
                        value: Some(Expr::Var("tmp".into())),
                    },
                ],
            ),
        );
        let reg = HelperRegistry { helpers };
        let mut hoisted = Vec::new();
        let mut counter = 0_usize;
        let expr = Expr::FuncCall("ta_complex".into(), vec![Expr::Literal(5.0)]);
        let result = hoist_block_helpers(&expr, &reg, &mut hoisted, &mut counter, &[]);

        // Should be hoisted: result replaced with Var(_complex_0)
        assert_eq!(hoisted.len(), 1);
        assert_eq!(hoisted[0].0, "_complex_0");
        assert_eq!(hoisted[0].1, VarType::Real);
        assert!(matches!(result, Expr::Var(ref n) if n == "_complex_0"));
    }
}
