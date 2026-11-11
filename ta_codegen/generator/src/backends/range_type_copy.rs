//! A candlestick body's copies with its range types folded in.
//!
//! A range type cannot change during a call, yet each bar re-dispatches on it.
//! The rest of a body after its lookback clamp is copied once per folded case,
//! each copy reading the range types as constants, plus once as written; each
//! copy computes exactly what the written body would, so no value can change.
//! The wrapping guards are not IR: every backend writes them around the arms in
//! its own syntax.

use std::collections::{BTreeMap, BTreeSet};

use crate::ir::{BinOp, Expr, OptOut, Statement};
use crate::streaming::{rewrite_stmts, walk_expr, walk_stmt_exprs};

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum RangeType {
    RealBody,
    HighLow,
    Shadows,
}

impl RangeType {
    pub const ALL: [RangeType; 3] = [RangeType::RealBody, RangeType::HighLow, RangeType::Shadows];

    #[must_use]
    pub fn c_name(self) -> &'static str {
        match self {
            RangeType::RealBody => "TA_RangeType_RealBody",
            RangeType::HighLow => "TA_RangeType_HighLow",
            RangeType::Shadows => "TA_RangeType_Shadows",
        }
    }

    #[must_use]
    pub fn from_c_name(name: &str) -> Option<RangeType> {
        Self::ALL.into_iter().find(|t| t.c_name() == name)
    }

    /// The range type the library ships for `setting`, read from its own
    /// defaults table so no backend can assume one it does not ship.
    #[must_use]
    pub fn shipped_default(setting: &str) -> RangeType {
        const GLOBALS_C: &str = include_str!("../../../../src/ta_common/ta_global.c");
        let entry = format!("{{ TA_{setting}, ");
        let at = GLOBALS_C
            .find(&entry)
            .unwrap_or_else(|| panic!("no {setting} row in TA_CANDLE_DEFAULT_SETTINGS (src/ta_common/ta_global.c)"));
        let ty = &GLOBALS_C[at + entry.len()..];
        let ty = &ty[..ty.find(',').expect("a defaults row continues after its range type")];
        Self::from_c_name(ty).unwrap_or_else(|| panic!("{setting}: unknown default range type {ty}"))
    }
}

/// The local a body reads `setting`'s range type from.
#[must_use]
pub fn range_type_var(setting: &str) -> String {
    format!("{setting}_rangeType")
}

/// One copy: taken when every `(setting, range type)` of `guard` holds; `None`
/// is the last arm, the body as written.
#[derive(Debug, Clone)]
pub struct Arm {
    pub guard: Option<Vec<(String, RangeType)>>,
    pub body: Vec<Statement>,
}

#[derive(Debug, Clone)]
pub struct Copies {
    /// Up to and including the lookback clamp's exit, rendered once.
    pub head: Vec<Statement>,
    pub arms: Vec<Arm>,
}

fn is_clamp_exit(s: &Statement) -> bool {
    matches!(
        s,
        Statement::If { condition: Expr::BinOp(l, BinOp::Greater, r), .. }
            if matches!(l.as_ref(), Expr::Var(v) if v == "startIdx")
                && matches!(r.as_ref(), Expr::Var(v) if v == "endIdx")
    )
}

/// One setting read after the clamp gets an arm per range type; several get one
/// arm with all of them at their shipped defaults. `None` when the function
/// opts out, has no top-level clamp, or reads no range type after it.
#[must_use]
pub fn split(body: &[Statement], opt_outs: &[OptOut], fold: &dyn Fn(RangeType) -> Expr) -> Option<Copies> {
    if opt_outs.contains(&OptOut::RangeTypeCopy) {
        return None;
    }
    let clamp = body.iter().position(is_clamp_exit)?;
    let (head, rest) = body.split_at(clamp + 1);
    let mut used = BTreeSet::new();
    for s in rest {
        walk_stmt_exprs(s, &mut |e| {
            walk_expr(e, &mut |x| {
                if let Expr::Var(v) = x {
                    if let Some(setting) = v.strip_suffix("_rangeType") {
                        used.insert(setting.to_string());
                    }
                }
            });
        });
    }
    let guards: Vec<Vec<(String, RangeType)>> = match used.iter().collect::<Vec<_>>().as_slice() {
        [] => return None,
        [one] => RangeType::ALL.iter().map(|&t| vec![((*one).clone(), t)]).collect(),
        several => vec![several.iter().map(|s| ((*s).clone(), RangeType::shipped_default(s))).collect()],
    };
    let mut arms: Vec<Arm> = guards
        .into_iter()
        .map(|guard| {
            let by_var: BTreeMap<String, RangeType> = guard.iter().map(|(s, t)| (range_type_var(s), *t)).collect();
            let fe = |e: Expr| match e {
                Expr::Var(v) if by_var.contains_key(&v) => fold(by_var[&v]),
                other => other,
            };
            Arm { body: rewrite_stmts(rest, &fe, &|s| Some(s)), guard: Some(guard) }
        })
        .collect();
    arms.push(Arm { guard: None, body: rest.to_vec() });
    Some(Copies { head: head.to_vec(), arms })
}

#[cfg(test)]
mod tests {
    use super::*;

    fn var(n: &str) -> Expr {
        Expr::Var(n.to_string())
    }

    fn read(n: &str) -> Statement {
        Statement::Expr(Expr::FuncCall("ta_candlerange".into(), vec![var(n)]))
    }

    fn body(after: Vec<Statement>) -> Vec<Statement> {
        let mut b = vec![
            read("Near_rangeType"),
            Statement::If {
                condition: Expr::BinOp(Box::new(var("startIdx")), BinOp::Greater, Box::new(var("endIdx"))),
                then_body: vec![Statement::Return { value: None }],
                else_body: vec![],
                cond_comments: vec![],
            },
        ];
        b.extend(after);
        b
    }

    fn fold(t: RangeType) -> Expr {
        var(t.c_name())
    }

    fn reads(stmts: &[Statement]) -> Vec<String> {
        let mut out = Vec::new();
        for s in stmts {
            walk_stmt_exprs(s, &mut |e| walk_expr(e, &mut |x| if let Expr::Var(v) = x { out.push(v.clone()) }));
        }
        out
    }

    #[test]
    fn folds_only_the_range_types_read_after_the_clamp() {
        let one = split(&body(vec![read("BodyDoji_rangeType")]), &[], &fold).expect("one setting copies");
        assert_eq!(one.head.len(), 2);
        let folded: Vec<_> = one.arms.iter().map(|a| (a.guard.clone(), reads(&a.body))).collect();
        assert_eq!(
            folded,
            vec![
                (Some(vec![("BodyDoji".into(), RangeType::RealBody)]), vec!["TA_RangeType_RealBody".to_string()]),
                (Some(vec![("BodyDoji".into(), RangeType::HighLow)]), vec!["TA_RangeType_HighLow".to_string()]),
                (Some(vec![("BodyDoji".into(), RangeType::Shadows)]), vec!["TA_RangeType_Shadows".to_string()]),
                (None, vec!["BodyDoji_rangeType".to_string()]),
            ]
        );

        let several = split(&body(vec![read("ShadowShort_rangeType"), read("BodyLong_rangeType")]), &[], &fold)
            .expect("several settings copy");
        assert_eq!(several.arms.len(), 2);
        assert_eq!(
            several.arms[0].guard,
            Some(vec![("BodyLong".into(), RangeType::RealBody), ("ShadowShort".into(), RangeType::Shadows)])
        );
        assert_eq!(reads(&several.arms[0].body), ["TA_RangeType_Shadows", "TA_RangeType_RealBody"]);

        assert!(split(&body(vec![read("BodyDoji_rangeType")]), &[OptOut::RangeTypeCopy], &fold).is_none());
        assert!(split(&body(vec![read("BodyDoji_avgPeriod")]), &[], &fold).is_none());
    }
}
