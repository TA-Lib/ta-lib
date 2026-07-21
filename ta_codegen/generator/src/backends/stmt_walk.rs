//! Shared statement-tree walker for the language backends.
//!
//! The companion to [`ExprEmitter`](super::expr_walk::ExprEmitter): where that
//! owns the `Expr` dispatch, [`StatementEmitter`] owns the one exhaustive
//! `Statement`-variant dispatch in [`StatementEmitter::walk_stmt`] and threads the
//! current `indent` through it. Each backend implements the per-variant leaf hooks
//! (which are language-specific — brace style, indentation step, helper hoisting)
//! and recurses into child statements via [`walk_stmt`](StatementEmitter::walk_stmt).
//!
//! Three variants render identically across the C/Rust/Java backends — `break;`,
//! `continue;`, and a bare statement [`Block`](crate::ir::Statement::Block) (its
//! children rendered at the same indent) — so they have shared defaults here and no
//! backend repeats them. A backend wires itself in with a small emitter struct that
//! bundles its render context plus the enum/registry/helper services; its
//! `render_stmt` then becomes a thin `emitter.walk_stmt(stmt, indent)`.

use crate::ir::{BinOp, CircBuf, Expr, Statement, VarType};

/// Flatten a left-associative `&&` chain into its operands, in source order.
/// `a && b && c` (parsed as `((a && b) && c)`) yields `[a, b, c]`. A non-`&&`
/// expression yields a single-element vec. Used to render commented conditions
/// one operand per line.
pub(crate) fn flatten_and(e: &Expr) -> Vec<&Expr> {
    if let Expr::BinOp(l, BinOp::And, r) = e {
        let mut v = flatten_and(l);
        v.push(r);
        v
    } else {
        vec![e]
    }
}

/// Render an already-flattened, already-rendered `&&`-chain as multiple lines —
/// one operand per line joined by ` &&`, each with its inline comment aligned to
/// a shared column. The first operand is emitted with no leading pad (it follows
/// the caller's `if( ` / `if `); subsequent operands are indented by `line_pad`.
/// `last_suffix` is appended after the final operand (e.g. ` )` for C/Java, empty
/// for Rust). `block_style` picks `/* */` vs `//`. The result ends with a newline.
pub(crate) fn render_and_operands(
    operands: &[String],
    comments: &[Option<Vec<String>>],
    line_pad: &str,
    last_suffix: &str,
    block_style: bool,
) -> String {
    let n = operands.len();
    let code_lines: Vec<String> = operands
        .iter()
        .enumerate()
        .map(|(i, op)| {
            if i + 1 < n {
                format!("{op} &&")
            } else {
                format!("{op}{last_suffix}")
            }
        })
        .collect();
    // Align comments to a column, but cap it so one very long operand doesn't
    // push every comment far to the right; operands past the cap get a single
    // space before the comment.
    let align = code_lines
        .iter()
        .map(String::len)
        .filter(|&w| w <= 72)
        .max()
        .unwrap_or(0);
    let mut out = String::new();
    for (i, code) in code_lines.iter().enumerate() {
        if i > 0 {
            out.push_str(line_pad);
        }
        out.push_str(code);
        if let Some(Some(lines)) = comments.get(i) {
            let text = lines
                .iter()
                .filter(|l| !l.is_empty())
                .cloned()
                .collect::<Vec<_>>()
                .join(" ");
            if !text.is_empty() {
                let gap = if code.len() < align { align - code.len() + 1 } else { 1 };
                let gap = " ".repeat(gap);
                if block_style {
                    out.push_str(&format!("{gap}/* {text} */"));
                } else {
                    out.push_str(&format!("{gap}// {text}"));
                }
            }
        }
        out.push('\n');
    }
    out
}

/// Per-language leaf formatting for the shared [`walk_stmt`](StatementEmitter::walk_stmt)
/// dispatch. Implementors provide the per-variant hooks; the default `walk_stmt` owns
/// the exhaustive match over [`Statement`] variants and routes each to its hook,
/// passing the current `indent`. The three universally-identical variants
/// (`Break`/`Continue`/`Block`) have shared defaults.
pub trait StatementEmitter {
    /// Render a `Statement::VarDecl` (`type name [= init];`).
    fn var_decl(&self, var_type: &VarType, name: &str, init: &Option<Expr>, indent: usize)
        -> String;

    /// Render a `Statement::Assign` (`target = value;`, possibly compound/folded).
    fn assign(&self, target: &Expr, value: &Expr, compound: bool, indent: usize) -> String;

    /// Render a `Statement::Expr` (an expression evaluated for its side effects).
    fn expr_stmt(&self, e: &Expr, indent: usize) -> String;

    /// Render a `Statement::While`; recurse into `body` via [`walk_stmt`](Self::walk_stmt).
    fn while_loop(&self, condition: &Expr, body: &[Statement], indent: usize) -> String;

    /// Render a `Statement::DoWhile`; recurse into `body` via [`walk_stmt`](Self::walk_stmt).
    fn do_while(&self, condition: &Expr, body: &[Statement], indent: usize) -> String;

    /// Render a `Statement::If`; recurse into the bodies via [`walk_stmt`](Self::walk_stmt).
    /// `cond_comments` holds the per-operand inline comments for a top-level
    /// `&&`-chain condition (empty for ordinary conditions).
    fn if_stmt(
        &self,
        condition: &Expr,
        then_body: &[Statement],
        else_body: &[Statement],
        cond_comments: &[Option<Vec<String>>],
        indent: usize,
    ) -> String;

    /// Render a `Statement::Return` (`return [value];`).
    fn return_stmt(&self, value: &Option<Expr>, indent: usize) -> String;

    /// Render a `Statement::For` (TA-Lib's countdown loop); recurse via [`walk_stmt`](Self::walk_stmt).
    fn for_loop(&self, var: &str, count: &Expr, body: &[Statement], indent: usize) -> String;

    /// Render a `Statement::ForC` (C-style `for(init; cond; update)`); recurse via
    /// [`walk_stmt`](Self::walk_stmt).
    fn for_c(
        &self,
        init: &Statement,
        condition: &Expr,
        update: &Statement,
        body: &[Statement],
        indent: usize,
    ) -> String;

    /// Render a `Statement::Switch`; recurse into the case/default bodies via
    /// [`walk_stmt`](Self::walk_stmt).
    fn switch(
        &self,
        expr: &Expr,
        cases: &[(String, Vec<Statement>)],
        default: &[Statement],
        indent: usize,
    ) -> String;

    /// Render a `Statement::CircBuf` (a `CIRCBUF_*` op). Language-specific lowering.
    fn circ_buf(&self, op: &CircBuf, indent: usize) -> String;

    /// Render a `Statement::Comment` — source trivia carried verbatim from the
    /// input `.c`. Language-specific comment syntax (block vs line).
    fn comment(&self, lines: &[String], indent: usize) -> String;

    /// Render a `Statement::UnrollHint` — the `TA_UNROLL(n)` advisory that
    /// precedes a loop. Only C has a place to put it (the macro in
    /// `ta_utility.h`); every other backend leaves the loop to its own
    /// optimizer, so the default is to emit nothing. Never affects semantics.
    fn unroll_hint(&self, _count: u32, _indent: usize) -> String {
        String::new()
    }

    /// Render a `Statement::Break` (`break;`). Identical across backends.
    fn break_stmt(&self, indent: usize) -> String {
        format!("{}break;\n", " ".repeat(indent))
    }

    /// Render a `Statement::Continue` (`continue;`). Identical across backends.
    fn continue_stmt(&self, indent: usize) -> String {
        format!("{}continue;\n", " ".repeat(indent))
    }

    /// Render a `Statement::Block` — its child statements at the same indent.
    /// Identical across backends.
    fn block(&self, body: &[Statement], indent: usize) -> String {
        let mut out = String::new();
        for s in body {
            out.push_str(&self.walk_stmt(s, indent));
        }
        out
    }

    /// The owned recursion: match `stmt`'s variant and dispatch to the corresponding
    /// leaf hook, threading `indent`. This is the single copy of the `Statement`
    /// tree-walk that every backend shares; the match is intentionally exhaustive (no
    /// wildcard arm) so a new IR variant forces every backend to handle it.
    fn walk_stmt(&self, stmt: &Statement, indent: usize) -> String {
        match stmt {
            Statement::VarDecl { var_type, name, init } => {
                self.var_decl(var_type, name, init, indent)
            }
            Statement::Assign { target, value, compound } => {
                self.assign(target, value, *compound, indent)
            }
            Statement::Expr(e) => self.expr_stmt(e, indent),
            Statement::While { condition, body } => self.while_loop(condition, body, indent),
            Statement::DoWhile { condition, body } => self.do_while(condition, body, indent),
            Statement::If { condition, then_body, else_body, cond_comments } => {
                self.if_stmt(condition, then_body, else_body, cond_comments, indent)
            }
            Statement::Return { value } => self.return_stmt(value, indent),
            Statement::UnrollHint { count } => self.unroll_hint(*count, indent),
            Statement::Break => self.break_stmt(indent),
            Statement::Continue => self.continue_stmt(indent),
            Statement::For { var, count, body } => self.for_loop(var, count, body, indent),
            Statement::ForC { init, condition, update, body } => {
                self.for_c(init, condition, update, body, indent)
            }
            Statement::Block { body } => self.block(body, indent),
            Statement::Switch { expr, cases, default } => self.switch(expr, cases, default, indent),
            Statement::CircBuf(op) => self.circ_buf(op, indent),
            Statement::Comment(lines) => self.comment(lines, indent),
        }
    }
}

/// Format comment content lines as a C/Java block comment at `indent`. A single
/// content line renders as `/* text */`; multiple lines render as an opening
/// `/*`, ` * `-prefixed body lines, and a closing ` */` — matching TA-Lib's
/// house style and preserving interior alignment (e.g. ADX's ASCII-art diagram).
pub(crate) fn block_comment(lines: &[String], indent: usize) -> String {
    let pad = " ".repeat(indent);
    if lines.len() == 1 {
        return if lines[0].is_empty() {
            format!("{pad}/* */\n")
        } else {
            format!("{pad}/* {} */\n", lines[0])
        };
    }
    let mut out = String::new();
    match lines.first() {
        Some(first) if !first.is_empty() => out.push_str(&format!("{pad}/* {first}\n")),
        _ => out.push_str(&format!("{pad}/*\n")),
    }
    for l in &lines[1..] {
        if l.is_empty() {
            out.push_str(&format!("{pad} *\n"));
        } else {
            out.push_str(&format!("{pad} * {l}\n"));
        }
    }
    out.push_str(&format!("{pad} */\n"));
    out
}

/// Recursively drop [`Statement::Comment`] nodes from a statement tree. Used to
/// keep the carried source comments in only the primary (double-precision,
/// guarded) function variant, so they are not duplicated across the single-
/// precision (`TA_S_*`) and unguarded/logic/private variants which share the
/// same body.
pub(crate) fn strip_comments(stmts: &[Statement]) -> Vec<Statement> {
    stmts
        .iter()
        .filter(|s| !matches!(s, Statement::Comment(_)))
        .map(|s| match s {
            Statement::While { condition, body } => Statement::While {
                condition: condition.clone(),
                body: strip_comments(body),
            },
            Statement::DoWhile { condition, body } => Statement::DoWhile {
                condition: condition.clone(),
                body: strip_comments(body),
            },
            Statement::For { var, count, body } => Statement::For {
                var: var.clone(),
                count: count.clone(),
                body: strip_comments(body),
            },
            Statement::ForC { init, condition, update, body } => Statement::ForC {
                init: Box::new(strip_comments(std::slice::from_ref(init)).pop().unwrap()),
                condition: condition.clone(),
                update: Box::new(strip_comments(std::slice::from_ref(update)).pop().unwrap()),
                body: strip_comments(body),
            },
            Statement::If { condition, then_body, else_body, .. } => Statement::If {
                condition: condition.clone(),
                then_body: strip_comments(then_body),
                else_body: strip_comments(else_body),
                // Inline condition comments dropped in stripped (non-primary) variants.
                cond_comments: Vec::new(),
            },
            Statement::Switch { expr, cases, default } => Statement::Switch {
                expr: expr.clone(),
                cases: cases
                    .iter()
                    .map(|(label, body)| (label.clone(), strip_comments(body)))
                    .collect(),
                default: strip_comments(default),
            },
            Statement::Block { body } => Statement::Block { body: strip_comments(body) },
            // Leaf statements (no nested statement bodies) — cloned as-is. Comment
            // was already filtered out above.
            Statement::VarDecl { .. }
            | Statement::Assign { .. }
            | Statement::Return { .. }
            | Statement::UnrollHint { .. }
            | Statement::Break
            | Statement::Continue
            | Statement::Expr(_)
            | Statement::CircBuf(_)
            | Statement::Comment(_) => s.clone(),
        })
        .collect()
}

/// Format comment content lines as Rust `//` line comments at `indent`.
pub(crate) fn line_comment(lines: &[String], indent: usize) -> String {
    let pad = " ".repeat(indent);
    let mut out = String::new();
    for l in lines {
        if l.is_empty() {
            out.push_str(&format!("{pad}//\n"));
        } else {
            out.push_str(&format!("{pad}// {l}\n"));
        }
    }
    out
}
