//! The block scan's running extremes carried as `long` keys (#415).
//!
//! Java-only. HotSpot keeps a branch on a loop-carried `double` min or max;
//! over raw bits the same select is integer arithmetic. A key is only ever an
//! input read, a copy of a key, or a strict min/max of two keys, and the
//! rendered guard admits the keyed body only where bit order is value order.
//! Anything else assigned to a key must fail the plan: a wrong plan changes
//! output bits silently.

use std::collections::{BTreeSet, HashSet};

use crate::ir::{BinOp, CircBuf, CircBufLayout, Expr, FuncDef, ParamType, Statement, VarType};
use crate::streaming::{exprs_equal, rewrite_expr, walk_expr};

/// `double` to key. Rendered as `Double.doubleToRawLongBits(x)`.
pub(crate) const KEY_BITS: &str = "ta_key_bits";
/// Key to `double`. Rendered as `Double.longBitsToDouble(k)`.
pub(crate) const KEY_REAL: &str = "ta_key_real";
/// `(target, value)`. Rendered as `keyMin(target, value)`.
pub(crate) const KEY_MIN: &str = "ta_key_min";
/// `(target, value)`. Rendered as `keyMax(target, value)`.
pub(crate) const KEY_MAX: &str = "ta_key_max";

#[derive(Debug, Clone, PartialEq, Eq)]
pub(crate) struct KeyPlan {
    locals: BTreeSet<String>,
    pub(crate) arrays: BTreeSet<String>,
    /// The inputs read into a key, in declaration order: what the guard scans.
    pub(crate) sources: Vec<String>,
}

impl KeyPlan {
    #[must_use]
    pub(crate) fn slots(&self) -> HashSet<String> {
        self.locals.iter().chain(&self.arrays).cloned().collect()
    }

    fn slot_of<'e>(&self, e: &'e Expr) -> Option<&'e str> {
        match e {
            Expr::Var(n) if self.locals.contains(n) => Some(n),
            Expr::ArrayAccess(n, _) if self.arrays.contains(n) => Some(n),
            _ => None,
        }
    }
}

/// `Ok(None)`: not a block scan. `Err`: a block scan this module cannot key,
/// which the caller must not render unkeyed without saying so.
pub(crate) fn plan(func: &FuncDef, body: &[Statement]) -> Result<Option<KeyPlan>, String> {
    if func.has_explicit_private {
        return Ok(None);
    }
    let reals: Vec<String> = func
        .inputs
        .iter()
        .filter(|i| i.param_type == ParamType::Real)
        .map(|i| i.name.clone())
        .collect();
    plan_body(&reals, body)
}

fn plan_body(real_inputs: &[String], body: &[Statement]) -> Result<Option<KeyPlan>, String> {
    // Candidate: every scratch buffer is a plain array of doubles that is
    // never used as a ring, and some extreme is kept in a scratch slot.
    let mut eligible = KeyPlan { locals: BTreeSet::new(), arrays: BTreeSet::new(), sources: Vec::new() };
    let mut ring = false;
    for s in body {
        match s {
            Statement::VarDecl { var_type: VarType::Real, name, init: None } => {
                eligible.locals.insert(name.clone());
            }
            Statement::CircBuf(CircBuf::Prolog { id, layout: CircBufLayout::Plain(VarType::Real), .. }) => {
                eligible.arrays.insert(id.clone());
            }
            Statement::CircBuf(CircBuf::Prolog { .. }) => ring = true,
            _ => {}
        }
    }
    each_stmt(body, &mut |s| {
        ring |= matches!(s, Statement::CircBuf(CircBuf::Next { .. } | CircBuf::InitLocalOnly { .. }));
    });
    if ring || eligible.arrays.is_empty() {
        return Ok(None);
    }
    let mut keys: BTreeSet<String> = BTreeSet::new();
    each_stmt(body, &mut |s| {
        if let Some((t, v, _)) = extreme(s) {
            if let (Some(a), Some(b)) = (eligible.slot_of(t), eligible.slot_of(v)) {
                keys.insert(a.to_string());
                keys.insert(b.to_string());
            }
        }
    });
    if keys.is_empty() {
        return Ok(None);
    }
    // A plain copy between two scratch slots carries the key with it.
    loop {
        let before = keys.len();
        each_stmt(body, &mut |s| {
            if let Statement::Assign { target, value, compound: false } = s {
                if let (Some(a), Some(b)) = (eligible.slot_of(target), eligible.slot_of(value)) {
                    if keys.contains(a) || keys.contains(b) {
                        keys.insert(a.to_string());
                        keys.insert(b.to_string());
                    }
                }
            }
        });
        if keys.len() == before {
            break;
        }
    }
    let mut plan = KeyPlan {
        locals: eligible.locals.intersection(&keys).cloned().collect(),
        arrays: eligible.arrays.intersection(&keys).cloned().collect(),
        sources: Vec::new(),
    };
    if plan.arrays != eligible.arrays {
        return Err("a scratch array of the scan holds something other than a key".into());
    }

    let mut read: BTreeSet<String> = BTreeSet::new();
    let mut err: Option<String> = None;
    each_stmt(body, &mut |s| {
        if err.is_some() {
            return;
        }
        if let Statement::Assign { target, value, compound } = s {
            if let Some(slot) = plan.slot_of(target) {
                if *compound {
                    err = Some(format!("compound assignment to key `{slot}`"));
                } else if plan.slot_of(value).is_none() {
                    match value {
                        Expr::ArrayAccess(inp, _) if real_inputs.contains(inp) => {
                            read.insert(inp.clone());
                        }
                        _ => err = Some(format!("key `{slot}` is assigned something that is not an input read or a key")),
                    }
                }
            }
        }
        for e in exprs_of(s) {
            if let Err(why) = check_use(e, &plan) {
                err = Some(why);
            }
        }
    });
    if let Some(why) = err {
        return Err(why);
    }
    plan.sources = real_inputs.iter().filter(|n| read.contains(*n)).cloned().collect();
    if plan.sources.is_empty() {
        return Err("no input is read into a key".into());
    }
    guard_site(body, &plan)?;
    Ok(Some(plan))
}

/// The guard goes in front of the first key array's allocation and reads
/// `startIdx` and `endIdx` there, so they must already be clamped and never
/// move afterwards.
fn guard_site(body: &[Statement], plan: &KeyPlan) -> Result<(), String> {
    let init_of = |s: &Statement| match s {
        Statement::CircBuf(CircBuf::Init { id, .. }) if plan.arrays.contains(id) => Some(id.clone()),
        _ => None,
    };
    let inits: BTreeSet<String> = body.iter().filter_map(init_of).collect();
    if inits != plan.arrays {
        return Err("a key array is not allocated at the top level of the body".into());
    }
    let at = body.iter().position(|s| init_of(s).is_some()).unwrap_or(0);
    let empty_range_returns = body[..at].iter().any(|s| {
        let Statement::If { condition: Expr::BinOp(l, BinOp::Greater, r), then_body, .. } = s else {
            return false;
        };
        matches!((l.as_ref(), r.as_ref()), (Expr::Var(a), Expr::Var(b)) if a == "startIdx" && b == "endIdx")
            && matches!(then_body.last(), Some(Statement::Return { .. }))
    });
    if !empty_range_returns {
        return Err("no `if( startIdx > endIdx )` return before the first key array".into());
    }
    let mut moved = false;
    each_stmt(&body[at..], &mut |s| {
        if let Statement::Assign { target: Expr::Var(n), .. } = s {
            moved |= n == "startIdx" || n == "endIdx";
        }
    });
    if moved {
        return Err("startIdx or endIdx is assigned after the first key array".into());
    }
    Ok(())
}

/// The body with every key slot holding raw bits. Length-preserving at every
/// nesting level.
#[must_use]
pub(crate) fn rewrite(body: &[Statement], plan: &KeyPlan) -> Vec<Statement> {
    let real = |e: &Expr| {
        rewrite_expr(e, &|x| {
            if plan.slot_of(&x).is_some() {
                Expr::FuncCall(KEY_REAL.into(), vec![x])
            } else {
                x
            }
        })
    };
    let pass = |b: &[Statement]| rewrite(b, plan);
    body.iter()
        .map(|s| {
            if let Some((target, value, is_min)) = extreme(s) {
                if plan.slot_of(target).is_some() && plan.slot_of(value).is_some() {
                    let f = if is_min { KEY_MIN } else { KEY_MAX };
                    return Statement::Assign {
                        target: target.clone(),
                        value: Expr::FuncCall(f.into(), vec![target.clone(), value.clone()]),
                        compound: false,
                    };
                }
            }
            match s {
                Statement::Assign { target, value, compound } => Statement::Assign {
                    target: target.clone(),
                    value: match (plan.slot_of(target), plan.slot_of(value)) {
                        (Some(_), Some(_)) => value.clone(),
                        (Some(_), None) => Expr::FuncCall(KEY_BITS.into(), vec![value.clone()]),
                        (None, _) => real(value),
                    },
                    compound: *compound,
                },
                Statement::VarDecl { var_type, name, init } => Statement::VarDecl {
                    var_type: var_type.clone(),
                    name: name.clone(),
                    init: init.as_ref().map(&real),
                },
                Statement::While { condition, body } => {
                    Statement::While { condition: real(condition), body: pass(body) }
                }
                Statement::DoWhile { condition, body } => {
                    Statement::DoWhile { condition: real(condition), body: pass(body) }
                }
                Statement::For { var, count, body } => {
                    Statement::For { var: var.clone(), count: real(count), body: pass(body) }
                }
                Statement::ForC { init, condition, update, body } => Statement::ForC {
                    init: Box::new(pass(std::slice::from_ref(init)).remove(0)),
                    condition: real(condition),
                    update: Box::new(pass(std::slice::from_ref(update)).remove(0)),
                    body: pass(body),
                },
                Statement::If { condition, then_body, else_body, cond_comments } => Statement::If {
                    condition: real(condition),
                    then_body: pass(then_body),
                    else_body: pass(else_body),
                    cond_comments: cond_comments.clone(),
                },
                Statement::Switch { expr, cases, default } => Statement::Switch {
                    expr: real(expr),
                    cases: cases.iter().map(|(v, b)| (v.clone(), pass(b))).collect(),
                    default: pass(default),
                },
                Statement::Block { body } => Statement::Block { body: pass(body) },
                Statement::Return { value } => Statement::Return { value: value.as_ref().map(&real) },
                Statement::Expr(e) => Statement::Expr(real(e)),
                other => other.clone(),
            }
        })
        .collect()
}

/// `if( v < t ) t = v;` or `if( v > t ) t = v;`, either operand order: the
/// target, the value, and whether it keeps the smaller.
fn extreme(s: &Statement) -> Option<(&Expr, &Expr, bool)> {
    let Statement::If { condition: Expr::BinOp(l, op, r), then_body, else_body, cond_comments } = s else {
        return None;
    };
    if !else_body.is_empty() || !cond_comments.is_empty() {
        return None;
    }
    let [Statement::Assign { target, value, compound: false }] = then_body.as_slice() else {
        return None;
    };
    let less = match op {
        BinOp::Less => true,
        BinOp::Greater => false,
        _ => return None,
    };
    let value_left = if exprs_equal(l, value) && exprs_equal(r, target) {
        true
    } else if exprs_equal(l, target) && exprs_equal(r, value) {
        false
    } else {
        return None;
    };
    Some((target, value, less == value_left))
}

fn check_use(e: &Expr, plan: &KeyPlan) -> Result<(), String> {
    let mut err = None;
    let mentions = |x: &Expr| {
        let mut hit = false;
        walk_expr(x, &mut |y| hit |= plan.slot_of(y).is_some());
        hit
    };
    walk_expr(e, &mut |x| match x {
        Expr::Var(n) if plan.arrays.contains(n) => err = Some(format!("key array `{n}` is used whole")),
        Expr::PointerDeref(n) if plan.locals.contains(n) || plan.arrays.contains(n) => {
            err = Some(format!("key `{n}` is dereferenced"));
        }
        Expr::AddressOf(i)
        | Expr::PostIncrement(i)
        | Expr::PostDecrement(i)
        | Expr::PreIncrement(i)
        | Expr::PreDecrement(i)
            if plan.slot_of(i).is_some() =>
        {
            err = Some("a key is stepped or has its address taken".into());
        }
        Expr::ArrayAccess(_, idx) if mentions(idx) => err = Some("a key is used as an index".into()),
        _ => {}
    });
    err.map_or(Ok(()), Err)
}

fn exprs_of(s: &Statement) -> Vec<&Expr> {
    match s {
        Statement::VarDecl { init, .. } => init.iter().collect(),
        Statement::Assign { target, value, .. } => vec![target, value],
        Statement::While { condition, .. }
        | Statement::DoWhile { condition, .. }
        | Statement::If { condition, .. }
        | Statement::ForC { condition, .. } => vec![condition],
        Statement::For { count, .. } => vec![count],
        Statement::Return { value } => value.iter().collect(),
        Statement::Switch { expr, .. } => vec![expr],
        Statement::Expr(e) => vec![e],
        Statement::CircBuf(CircBuf::Init { size, .. }) => vec![size],
        _ => Vec::new(),
    }
}

fn each_stmt(body: &[Statement], f: &mut dyn FnMut(&Statement)) {
    for s in body {
        f(s);
        match s {
            Statement::While { body, .. }
            | Statement::DoWhile { body, .. }
            | Statement::For { body, .. }
            | Statement::Block { body } => each_stmt(body, f),
            Statement::ForC { init, update, body, .. } => {
                each_stmt(std::slice::from_ref(init), f);
                each_stmt(std::slice::from_ref(update), f);
                each_stmt(body, f);
            }
            Statement::If { then_body, else_body, .. } => {
                each_stmt(then_body, f);
                each_stmt(else_body, f);
            }
            Statement::Switch { cases, default, .. } => {
                for (_, b) in cases {
                    each_stmt(b, f);
                }
                each_stmt(default, f);
            }
            _ => {}
        }
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    fn var(n: &str) -> Expr {
        Expr::Var(n.into())
    }
    fn at(a: &str, i: &str) -> Expr {
        Expr::ArrayAccess(a.into(), Box::new(var(i)))
    }
    fn set(t: Expr, v: Expr) -> Statement {
        Statement::Assign { target: t, value: v, compound: false }
    }
    fn keep(op: BinOp, v: Expr, t: Expr) -> Statement {
        Statement::If {
            condition: Expr::BinOp(Box::new(v.clone()), op, Box::new(t.clone())),
            then_body: vec![set(t, v)],
            else_body: vec![],
            cond_comments: vec![],
        }
    }
    fn real(n: &str) -> Statement {
        Statement::VarDecl { var_type: VarType::Real, name: n.into(), init: None }
    }
    fn scan(loop_body: Vec<Statement>) -> Vec<Statement> {
        vec![
            Statement::CircBuf(CircBuf::Prolog {
                id: "suf".into(),
                layout: CircBufLayout::Plain(VarType::Real),
                static_size: 30,
            }),
            real("lowest"),
            real("tmp"),
            real("other"),
            Statement::If {
                condition: Expr::BinOp(Box::new(var("startIdx")), BinOp::Greater, Box::new(var("endIdx"))),
                then_body: vec![Statement::Return { value: None }],
                else_body: vec![],
                cond_comments: vec![],
            },
            Statement::CircBuf(CircBuf::Init {
                id: "suf".into(),
                layout: CircBufLayout::Plain(VarType::Real),
                size: var("optInTimePeriod"),
            }),
            Statement::While { condition: var("i"), body: loop_body },
        ]
    }
    fn min_loop() -> Vec<Statement> {
        vec![
            set(var("tmp"), at("inReal", "i")),
            keep(BinOp::Less, var("tmp"), var("lowest")),
            set(at("suf", "i"), var("lowest")),
            set(at("outReal", "i"), var("lowest")),
        ]
    }
    fn inputs() -> Vec<String> {
        vec!["inReal".into()]
    }

    #[test]
    fn a_min_scan_plans_and_keys_exactly_its_chain() {
        let body = scan(min_loop());
        let p = plan_body(&inputs(), &body).unwrap().expect("a plan");
        assert_eq!(p.locals.iter().map(String::as_str).collect::<Vec<_>>(), ["lowest", "tmp"]);
        assert_eq!(p.arrays.iter().map(String::as_str).collect::<Vec<_>>(), ["suf"]);
        assert_eq!(p.sources, ["inReal"]);

        let out = rewrite(&body, &p);
        let Statement::While { body: l, .. } = &out[6] else { panic!("the loop") };
        let stored = |s: &Statement| match s {
            Statement::Assign { target, value, compound: false } => (target.clone(), value.clone()),
            other => panic!("not a plain assignment: {other:?}"),
        };
        assert_eq!(stored(&l[0]), (var("tmp"), Expr::FuncCall(KEY_BITS.into(), vec![at("inReal", "i")])));
        assert_eq!(
            stored(&l[1]),
            (var("lowest"), Expr::FuncCall(KEY_MIN.into(), vec![var("lowest"), var("tmp")]))
        );
        assert_eq!(stored(&l[2]), (at("suf", "i"), var("lowest")));
        assert_eq!(stored(&l[3]), (at("outReal", "i"), Expr::FuncCall(KEY_REAL.into(), vec![var("lowest")])));
    }

    #[test]
    fn a_greater_than_keeps_the_max_whichever_side_the_value_is_on() {
        let flipped = Statement::If {
            condition: Expr::BinOp(Box::new(var("lowest")), BinOp::Less, Box::new(var("tmp"))),
            then_body: vec![set(var("lowest"), var("tmp"))],
            else_body: vec![],
            cond_comments: vec![],
        };
        assert!(matches!(extreme(&flipped), Some((_, _, false))));
        assert!(matches!(extreme(&keep(BinOp::Greater, var("tmp"), var("lowest"))), Some((_, _, false))));
        assert!(matches!(extreme(&keep(BinOp::Less, var("tmp"), var("lowest"))), Some((_, _, true))));
        assert!(extreme(&keep(BinOp::LessEq, var("tmp"), var("lowest"))).is_none());
    }

    #[test]
    fn what_is_not_a_block_scan_has_no_plan() {
        // No extreme kept in a scratch slot.
        let mut l = min_loop();
        l[1] = keep(BinOp::LessEq, var("tmp"), var("lowest"));
        assert_eq!(plan_body(&inputs(), &scan(l)), Ok(None));
        // The buffer is a ring.
        let mut l = min_loop();
        l.push(Statement::CircBuf(CircBuf::Next { id: "suf".into() }));
        assert_eq!(plan_body(&inputs(), &scan(l)), Ok(None));
    }

    #[test]
    fn a_key_given_anything_but_a_read_or_a_key_fails_the_plan() {
        let arith = Expr::BinOp(Box::new(var("lowest")), BinOp::Add, Box::new(Expr::Literal(1.0)));
        let mut l = min_loop();
        l.push(set(var("lowest"), arith));
        assert!(plan_body(&inputs(), &scan(l)).is_err());

        let mut l = min_loop();
        l.push(Statement::Assign { target: var("lowest"), value: var("tmp"), compound: true });
        assert!(plan_body(&inputs(), &scan(l)).is_err());

        let mut l = min_loop();
        l.push(set(var("tmp"), at("outReal", "i")));
        assert!(plan_body(&inputs(), &scan(l)).is_err());

        // A copy drags a slot into the chain, and its arithmetic then fails.
        let mut l = min_loop();
        l.push(set(var("other"), var("lowest")));
        l.push(Statement::Assign { target: var("other"), value: Expr::Literal(2.0), compound: true });
        assert!(plan_body(&inputs(), &scan(l)).is_err());
    }

    #[test]
    fn the_guard_needs_a_settled_range() {
        let mut body = scan(min_loop());
        body.remove(4);
        assert!(plan_body(&inputs(), &body).is_err());

        let mut l = min_loop();
        l.push(set(var("endIdx"), var("i")));
        assert!(plan_body(&inputs(), &scan(l)).is_err());
    }
}
