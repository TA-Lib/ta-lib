//! The lower bound of every input index an extrema step reads.
//!
//! Accept-only: a read whose index the walk cannot bound refuses the body.
//! Each bound is an expression over the cursor and the window start as the
//! pass found them and names the step never writes, so the lag prover can
//! check it against the window start, the oldest bar the ring holds.

use std::collections::{BTreeMap, BTreeSet};

use super::{expr_var_names, for_each_stmt, own_writes, walk_expr, StreamError};
use crate::ir::{BinOp, Expr, Statement, VarType};

type Env = BTreeMap<String, Expr>;

struct Walk<'a> {
    inputs: &'a [String],
    written: BTreeSet<String>,
    reads: Vec<(String, Expr)>,
    unbounded: Option<String>,
    jumps: bool,
}

/// A variable keeps its bound past a merge only when both sides agree on it.
/// Bounds never change otherwise, so a loop's fixpoint only drops variables.
fn join(a: &Env, b: &Env) -> Env {
    a.iter()
        .filter(|(v, l)| b.get(*v) == Some(l))
        .map(|(v, l)| (v.clone(), l.clone()))
        .collect()
}

fn bin(l: Expr, op: BinOp, r: &Expr) -> Expr {
    Expr::BinOp(Box::new(l), op, Box::new(r.clone()))
}

impl Walk<'_> {
    fn invariant(&self, e: &Expr) -> bool {
        match e {
            Expr::IntLiteral(_) => true,
            Expr::Var(v) => !self.written.contains(v),
            Expr::Cast(_, a) | Expr::Neg(a) => self.invariant(a),
            Expr::BinOp(l, BinOp::Add | BinOp::Sub | BinOp::Mul, r) => {
                self.invariant(l) && self.invariant(r)
            }
            _ => false,
        }
    }

    /// Runs after the statement's decrements are applied, so an increment or
    /// decrement reads as its operand.
    fn eval(&self, e: &Expr, env: &Env) -> Option<Expr> {
        match e {
            Expr::Var(v) => env.get(v).cloned(),
            Expr::PostIncrement(b)
            | Expr::PreIncrement(b)
            | Expr::PostDecrement(b)
            | Expr::PreDecrement(b)
            | Expr::Cast(VarType::Integer | VarType::Index, b) => self.eval(b, env),
            Expr::BinOp(l, BinOp::Add, r) => {
                let (base, by) = if self.invariant(r) {
                    (l, r)
                } else if self.invariant(l) {
                    (r, l)
                } else {
                    return None;
                };
                Some(bin(self.eval(base, env)?, BinOp::Add, by))
            }
            Expr::BinOp(l, BinOp::Sub, r) if self.invariant(r) => {
                Some(bin(self.eval(l, env)?, BinOp::Sub, r))
            }
            Expr::Ternary(_, a, b) => {
                let a = self.eval(a, env)?;
                (self.eval(b, env)? == a).then_some(a)
            }
            _ => None,
        }
    }

    fn expr(&mut self, e: &Expr, env: &mut Env) {
        let mut down = Vec::new();
        let mut found = Vec::new();
        walk_expr(e, &mut |x| match x {
            Expr::PostDecrement(b) | Expr::PreDecrement(b) | Expr::AddressOf(b) => {
                if let Expr::Var(v) = b.as_ref() {
                    down.push((v.clone(), matches!(x, Expr::AddressOf(_))));
                }
            }
            Expr::ArrayAccess(name, idx) if self.inputs.contains(name) => {
                found.push((name.clone(), idx.as_ref().clone()));
            }
            _ => {}
        });
        for (v, lost) in down {
            match env.remove(&v) {
                Some(lb) if !lost => {
                    env.insert(v, bin(lb, BinOp::Sub, &Expr::IntLiteral(1)));
                }
                _ => {}
            }
        }
        for (name, idx) in found {
            let text = format!("{name}[{}]", index_text(&idx));
            match self.eval(&idx, env) {
                Some(lb) => {
                    if !self.reads.iter().any(|r| r.0 == text && r.1 == lb) {
                        self.reads.push((text, lb));
                    }
                }
                None => {
                    self.unbounded.get_or_insert(text);
                }
            }
        }
    }

    fn assign(&mut self, name: &str, value: Option<&Expr>, env: &mut Env) {
        // `v = v + k` keeps the bound as `v++` does, so a loop stepping `v`
        // this way still reaches its fixpoint with `v` bounded.
        if let Some(Expr::BinOp(l, BinOp::Add, r)) = value {
            let stepped = |a: &Expr, b: &Expr| {
                matches!(a, Expr::Var(x) if x == name)
                    && matches!(b, Expr::IntLiteral(k) if *k >= 0)
            };
            if stepped(l, r) || stepped(r, l) {
                return;
            }
        }
        let lb = value.and_then(|v| {
            self.expr(v, env);
            self.eval(v, env)
        });
        match lb {
            Some(lb) => env.insert(name.to_string(), lb),
            None => env.remove(name),
        };
    }

    fn block(&mut self, stmts: &[Statement], mut env: Env) -> Env {
        for s in stmts {
            env = self.stmt(s, env);
        }
        env
    }

    /// `cond` runs before `body` on every pass, after it when `body_first`.
    fn looped(
        &mut self,
        entry: Env,
        cond: Option<&Expr>,
        body: &[Statement],
        tail: Option<&Statement>,
        body_first: bool,
    ) -> Env {
        let mut head = entry;
        loop {
            let mut at = head.clone();
            if let (Some(c), false) = (cond, body_first) {
                self.expr(c, &mut at);
            }
            let mut after = self.block(body, at.clone());
            if let Some(t) = tail {
                after = self.stmt(t, after);
            }
            if let (Some(c), true) = (cond, body_first) {
                self.expr(c, &mut after);
            }
            let next = join(&head, &after);
            if next == head {
                return if body_first { after } else { at };
            }
            head = next;
        }
    }

    fn stmt(&mut self, s: &Statement, mut env: Env) -> Env {
        match s {
            Statement::VarDecl { name, init, .. } => self.assign(name, init.as_ref(), &mut env),
            Statement::Assign {
                target: Expr::Var(v),
                value,
                ..
            } => self.assign(v, Some(value), &mut env),
            Statement::Assign { target, value, .. } => {
                self.expr(target, &mut env);
                self.expr(value, &mut env);
            }
            Statement::Expr(e) => self.expr(e, &mut env),
            Statement::If {
                condition,
                then_body,
                else_body,
                ..
            } => {
                self.expr(condition, &mut env);
                let t = self.block(then_body, env.clone());
                return join(&t, &self.block(else_body, env));
            }
            Statement::Block { body } => return self.block(body, env),
            Statement::While { condition, body } => {
                return self.looped(env, Some(condition), body, None, false)
            }
            Statement::DoWhile { condition, body } => {
                return self.looped(env, Some(condition), body, None, true)
            }
            Statement::ForC {
                init,
                condition,
                update,
                body,
            } => {
                let env = self.stmt(init, env);
                return self.looped(env, Some(condition), body, Some(update), false);
            }
            Statement::For { var, count, body } => {
                self.expr(count, &mut env);
                env.remove(var);
                return self.looped(env, None, body, None, false);
            }
            Statement::Break
            | Statement::Continue
            | Statement::Return { .. }
            | Statement::Switch { .. } => self.jumps = true,
            Statement::CircBuf(_)
            | Statement::Comment(_)
            | Statement::UnrollHint { .. } => {}
        }
        env
    }
}

/// Each input read of one pass of `steady`, by its text, with its index's
/// lower bound; `cursor` and `start` stand for their values at pass entry.
pub(super) fn extrema_reads(
    steady: &[Statement],
    cursor: &str,
    start: &str,
    inputs: &[String],
) -> Result<Vec<(String, Expr)>, StreamError> {
    let mut written = BTreeSet::new();
    for_each_stmt(steady, &mut |s| {
        own_writes(s, &mut |t| expr_var_names(t, &mut written));
        if let Statement::For { var: v, .. } | Statement::VarDecl { name: v, .. } = s {
            written.insert(v.clone());
        }
    });
    let mut w = Walk {
        inputs,
        written,
        reads: Vec::new(),
        unbounded: None,
        jumps: false,
    };
    let env = [cursor, start]
        .into_iter()
        .map(|v| (v.to_string(), Expr::Var(v.to_string())))
        .collect();
    w.block(steady, env);
    if w.jumps {
        return Err(StreamError::Unsupported(
            "extrema automaton: a `break`, `continue`, `return` or `switch` in the step".into(),
        ));
    }
    match w.unbounded {
        Some(read) => Err(StreamError::UnsupportedAccess(format!(
            "extrema automaton: nothing bounds how far back `{read}` reads; derive its index \
             in the same pass from `{start}` plus a constant, or from `{cursor}` minus a \
             constant or a parameter expression"
        ))),
        None => Ok(w.reads),
    }
}

fn index_text(e: &Expr) -> String {
    match e {
        Expr::Var(v) => v.clone(),
        Expr::IntLiteral(k) => k.to_string(),
        Expr::PostIncrement(b) => format!("{}++", index_text(b)),
        Expr::PostDecrement(b) => format!("{}--", index_text(b)),
        Expr::PreIncrement(b) => format!("++{}", index_text(b)),
        Expr::PreDecrement(b) => format!("--{}", index_text(b)),
        Expr::Cast(_, a) => index_text(a),
        Expr::BinOp(l, op @ (BinOp::Add | BinOp::Sub | BinOp::Mul), r) => {
            let o = match op {
                BinOp::Add => "+",
                BinOp::Sub => "-",
                _ => "*",
            };
            let r = if matches!(r.as_ref(), Expr::BinOp(..)) {
                format!("({})", index_text(r))
            } else {
                index_text(r)
            };
            format!("{} {o} {r}", index_text(l))
        }
        _ => format!("{e:?}"),
    }
}
