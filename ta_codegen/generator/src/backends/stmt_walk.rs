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

/// One line of a rendered condition: the code, carrying its indent *relative to*
/// the condition's first column but not the caller's base padding, plus the
/// comment annotating the leaf the line ends on.
struct CondLine {
    text: String,
    comment: Option<Vec<String>>,
}

/// Per-backend operand spelling for [`render_cond_tree`]. Each hook must answer
/// exactly as that backend's own `binop` hook does in the same operand position,
/// so the multi-line form differs from the flat one in whitespace and comments
/// only.
pub(crate) struct CondHooks<'a> {
    /// Render `child` as the `is_right` operand of `parent`, parens included.
    pub operand: &'a dyn Fn(&Expr, &BinOp, bool) -> String,
    /// Does this backend parenthesize `child` there? Consulted only for a child
    /// the spine descends into.
    pub wraps: &'a dyn Fn(&Expr, &BinOp, bool) -> bool,
}

/// True for a child the spine descends into rather than spelling as one operand.
/// The IR keeps no parenthesis node, so an `&&`/`||` node is the only thing a
/// source-level boolean group can become -- which is what makes this predicate
/// and the parser's descent decision two spellings of one rule.
fn descends(child: &Expr) -> bool {
    matches!(child, Expr::BinOp(_, BinOp::And | BinOp::Or, _))
}

/// Render a commented condition across multiple lines: one boolean-spine leaf per
/// line with its comment trailing, each `&&`/`||` group indented under the paren
/// that opens it. The first line carries no pad (it follows the caller's `if( `);
/// later lines get `line_pad`, and `last_suffix` closes the final one.
///
/// `flat` is the backend's own one-line rendering of the same condition. The
/// result is returned only when the two agree token for token, so a construct the
/// hooks spell differently falls back to the flat form instead of moving a token.
pub(crate) fn render_cond_tree(
    condition: &Expr,
    comments: &[Option<Vec<String>>],
    hooks: &CondHooks,
    flat: &str,
    line_pad: &str,
    last_suffix: &str,
    block_style: bool,
) -> Option<String> {
    let mut lines = vec![CondLine { text: String::new(), comment: None }];
    let mut idx = 0;
    if descends(condition) {
        build_spine(condition, 0, comments, hooks, &mut lines, &mut idx);
    } else {
        // A condition that is not an `&&`/`||` node is one line with no operator
        // to break at, so the flat rendering IS that line. Taking it verbatim also
        // keeps whatever whole-condition rule a backend applies outside its
        // operand rules -- Java and C# coerce a non-boolean condition with `!= 0`
        // there, and rebuilding the line from the operand hooks would drop it.
        if let Some(line) = lines.last_mut() {
            line.text.push_str(flat);
            line.comment = comments.first().cloned().flatten();
        }
        idx = 1;
    }
    // The one check that keeps slots paired with leaves: the parser's spine
    // collector and `build_operand` must agree on what a leaf is, and a
    // disagreement (or a helper inlined into the condition) lands here.
    if idx != comments.len() {
        return None;
    }
    lines.last_mut()?.text.push_str(last_suffix);
    let rebuilt: String = lines.iter().map(|l| l.text.as_str()).collect();
    if squeeze(&rebuilt) != squeeze(&format!("{flat}{last_suffix}")) {
        return None;
    }
    Some(align_cond_lines(&lines, line_pad, block_style))
}

/// Whitespace carries no meaning between two renderings of one condition, so the
/// token-identity check compares what is left once it is gone.
fn squeeze(s: &str) -> String {
    s.chars().filter(|c| !c.is_whitespace()).collect()
}

/// Emit one `&&`/`||` node at column `col`: its left operand, the operator closing
/// that line, and its right operand opening a new line back at `col`.
fn build_spine(
    e: &Expr,
    col: usize,
    comments: &[Option<Vec<String>>],
    hooks: &CondHooks,
    lines: &mut Vec<CondLine>,
    idx: &mut usize,
) {
    let Expr::BinOp(l, op, r) = e else { return };
    build_operand(l, op, false, col, comments, hooks, lines, idx);
    let sep = if matches!(op, BinOp::And) { " &&" } else { " ||" };
    if let Some(line) = lines.last_mut() {
        line.text.push_str(sep);
    }
    lines.push(CondLine { text: " ".repeat(col), comment: None });
    build_operand(r, op, true, col, comments, hooks, lines, idx);
}

/// Emit one operand of a `&&`/`||` node that starts at `col`: a nested group
/// descended into, indented one column in, or a leaf claiming a comment slot.
fn build_operand(
    child: &Expr,
    op: &BinOp,
    is_right: bool,
    col: usize,
    comments: &[Option<Vec<String>>],
    hooks: &CondHooks,
    lines: &mut Vec<CondLine>,
    idx: &mut usize,
) {
    if descends(child) {
        let wrap = (hooks.wraps)(child, op, is_right);
        if wrap {
            if let Some(line) = lines.last_mut() {
                line.text.push('(');
            }
        }
        // A nested group's lines sit one column in from where it starts, so the
        // grouping shows even in the three backends that emit no parens around an
        // `&&` inside an `||`; a wrapped group takes that column from its own `(`.
        // A same-operator child is the *same chain*, so it keeps `col` -- deriving
        // it from the line length instead would re-indent every left-assoc rung.
        let inner = if matches!(child, Expr::BinOp(_, o, _) if o == op) && !wrap {
            col
        } else {
            lines.last().map_or(0, |line| line.text.len()) + usize::from(!wrap)
        };
        build_spine(child, inner, comments, hooks, lines, idx);
        if wrap {
            if let Some(line) = lines.last_mut() {
                line.text.push(')');
            }
        }
        return;
    }
    let text = (hooks.operand)(child, op, is_right);
    if let Some(line) = lines.last_mut() {
        line.text.push_str(&text);
        line.comment = comments.get(*idx).cloned().flatten();
    }
    *idx += 1;
}

/// Attach each line's comment at a shared column and join. The column is measured
/// over the unpadded text, so it lands where the caller's `if( ` prefix and
/// `line_pad` agree; one very long line is excluded from the measurement rather
/// than pushing every comment off to the right.
fn align_cond_lines(lines: &[CondLine], line_pad: &str, block_style: bool) -> String {
    let align = lines
        .iter()
        .map(|l| l.text.len())
        .filter(|&w| w <= 72)
        .max()
        .unwrap_or(0);
    let mut out = String::new();
    for (i, line) in lines.iter().enumerate() {
        if i > 0 {
            out.push_str(line_pad);
        }
        out.push_str(&line.text);
        if let Some(lines) = &line.comment {
            let text = lines
                .iter()
                .filter(|l| !l.is_empty())
                .cloned()
                .collect::<Vec<_>>()
                .join(" ");
            if !text.is_empty() {
                let width = line.text.len();
                let gap = " ".repeat(if width < align { align - width + 1 } else { 1 });
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
    /// `cond_comments` holds one inline comment slot per boolean-spine leaf of the
    /// condition (empty for ordinary conditions).
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
    // A comment whose content reduces to nothing — `/*  */`, or `/* * */`,
    // whose lone `*` is eaten as a continuation prefix — arrives here with no
    // lines at all. Both are ordinary C, and indexing `lines[1..]` here is an
    // empty-slice panic that aborts the whole `generate`.
    if lines.is_empty() {
        return format!("{pad}/* */\n");
    }
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
/// precision (`TA_S_*`) and `_private` variants which share the
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
