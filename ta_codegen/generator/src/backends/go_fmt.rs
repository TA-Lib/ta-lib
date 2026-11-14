//! A Go expression tree and the printer that spells it exactly as `gofmt`
//! would, so the generator's output is gofmt-clean without running gofmt.
//!
//! Spacing around `+ - * /` follows go/printer's rule: it depends on the
//! precedence mix of the whole expression and on its nesting depth (a call with
//! several arguments, an index), not on the operator alone. The printer is a
//! transcription of `binaryExpr`, `cutoff`, `walkBinary` and `diffPrec` from
//! go/printer/nodes.go; a divergence shows up as a `gofmt -l` hit, never as a
//! different value.

#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub(crate) enum GOp {
    Add,
    Sub,
    Mul,
    Quo,
    Rem,
    Shl,
    Shr,
    And,
    Or,
    Xor,
    LAnd,
    LOr,
    Eq,
    Ne,
    Lt,
    Le,
    Gt,
    Ge,
}

impl GOp {
    pub(crate) fn token(self) -> &'static str {
        match self {
            Self::Add => "+",
            Self::Sub => "-",
            Self::Mul => "*",
            Self::Quo => "/",
            Self::Rem => "%",
            Self::Shl => "<<",
            Self::Shr => ">>",
            Self::And => "&",
            Self::Or => "|",
            Self::Xor => "^",
            Self::LAnd => "&&",
            Self::LOr => "||",
            Self::Eq => "==",
            Self::Ne => "!=",
            Self::Lt => "<",
            Self::Le => "<=",
            Self::Gt => ">",
            Self::Ge => ">=",
        }
    }

    pub(crate) fn prec(self) -> i32 {
        match self {
            Self::Mul | Self::Quo | Self::Rem | Self::Shl | Self::Shr | Self::And => 5,
            Self::Add | Self::Sub | Self::Or | Self::Xor => 4,
            Self::Eq | Self::Ne | Self::Lt | Self::Le | Self::Gt | Self::Ge => 3,
            Self::LAnd => 2,
            Self::LOr => 1,
        }
    }
}

const UNARY_PREC: i32 = 6;
const HIGHEST_PREC: i32 = 7;

#[derive(Clone, Debug, PartialEq)]
pub(crate) enum GExpr {
    /// An identifier, a literal or a qualified name: printed verbatim.
    Name(String),
    /// `-x`, `!x`, `^x`, `&x`, or `*x` (go/ast's StarExpr).
    Unary(&'static str, Box<GExpr>),
    Bin(Box<GExpr>, GOp, Box<GExpr>),
    Paren(Box<GExpr>),
    Call(String, Vec<GExpr>),
    Index(Box<GExpr>, Box<GExpr>),
    Slice(Box<GExpr>, Option<Box<GExpr>>, Option<Box<GExpr>>),
}

impl GExpr {
    pub(crate) fn name(s: impl Into<String>) -> Self {
        Self::Name(s.into())
    }

    pub(crate) fn call(f: impl Into<String>, args: Vec<GExpr>) -> Self {
        Self::Call(f.into(), args)
    }

    /// `l op r`, parenthesizing an operand Go's precedence would otherwise
    /// regroup: a right operand at the same level keeps C's left-to-right
    /// association, which floating point needs.
    pub(crate) fn bin(l: GExpr, op: GOp, r: GExpr) -> Self {
        let l = match &l {
            Self::Bin(_, lop, _) if lop.prec() < op.prec() => l.paren(),
            _ => l,
        };
        let r = match &r {
            Self::Bin(_, rop, _) if rop.prec() <= op.prec() => r.paren(),
            _ => r,
        };
        Self::Bin(Box::new(l), op, Box::new(r))
    }

    pub(crate) fn unary(op: &'static str, x: GExpr) -> Self {
        let x = match &x {
            Self::Bin(..) | Self::Unary(..) => x.paren(),
            _ => x,
        };
        Self::Unary(op, Box::new(x))
    }

    pub(crate) fn not(x: GExpr) -> Self {
        Self::unary("!", x)
    }

    pub(crate) fn index(x: GExpr, i: GExpr) -> Self {
        Self::Index(Box::new(x.primary()), Box::new(i))
    }

    pub(crate) fn slice(x: GExpr, lo: Option<GExpr>, hi: Option<GExpr>) -> Self {
        Self::Slice(Box::new(x.primary()), lo.map(Box::new), hi.map(Box::new))
    }

    pub(crate) fn paren(self) -> Self {
        match self {
            Self::Paren(_) => self,
            other => Self::Paren(Box::new(other)),
        }
    }

    /// `self` as the operand of an index or slice, which binds tighter than
    /// any operator.
    fn primary(self) -> Self {
        match self {
            Self::Bin(..) | Self::Unary(..) => self.paren(),
            other => other,
        }
    }

    pub(crate) fn is_name(&self, s: &str) -> bool {
        matches!(self, Self::Name(n) if n == s)
    }

    /// The text gofmt prints for this expression at statement level.
    pub(crate) fn render(&self) -> String {
        let mut out = String::new();
        expr1(self, 0, 1, &mut out);
        out
    }

    /// As a list element of a statement with several operands on one side
    /// (`a, b = x, y`), which go/printer prints one level deeper.
    pub(crate) fn render_at(&self, depth: i32) -> String {
        let mut out = String::new();
        expr1(self, 0, depth, &mut out);
        out
    }
}

fn reduce_depth(depth: i32) -> i32 {
    (depth - 1).max(1)
}

fn diff_prec(e: &GExpr, prec: i32) -> i32 {
    match e {
        GExpr::Bin(_, op, _) if op.prec() == prec => 0,
        _ => 1,
    }
}

fn walk_binary(e: &GExpr) -> (bool, bool, i32) {
    let GExpr::Bin(l, op, r) = e else { return (false, false, 0) };
    let mut has4 = op.prec() == 4;
    let mut has5 = op.prec() == 5;
    let mut max_problem = 0;
    if let GExpr::Bin(_, lop, _) = l.as_ref() {
        if lop.prec() >= op.prec() {
            let (h4, h5, mp) = walk_binary(l);
            has4 |= h4;
            has5 |= h5;
            max_problem = max_problem.max(mp);
        }
    }
    match r.as_ref() {
        GExpr::Bin(_, rop, _) => {
            if rop.prec() > op.prec() {
                let (h4, h5, mp) = walk_binary(r);
                has4 |= h4;
                has5 |= h5;
                max_problem = max_problem.max(mp);
            }
        }
        GExpr::Unary("*", _) => {
            if *op == GOp::Quo {
                max_problem = 5;
            }
        }
        GExpr::Unary(u, _) => match format!("{}{u}", op.token()).as_str() {
            "/*" | "&&" | "&^" => max_problem = 5,
            "++" | "--" => max_problem = max_problem.max(4),
            _ => {}
        },
        _ => {}
    }
    (has4, has5, max_problem)
}

fn cutoff(e: &GExpr, depth: i32) -> i32 {
    let (has4, has5, max_problem) = walk_binary(e);
    if max_problem > 0 {
        return max_problem + 1;
    }
    if has4 && has5 {
        return if depth == 1 { 5 } else { 4 };
    }
    if depth == 1 {
        6
    } else {
        4
    }
}

fn expr1(e: &GExpr, prec1: i32, depth: i32, out: &mut String) {
    match e {
        GExpr::Name(n) => out.push_str(n),
        GExpr::Bin(l, op, r) => {
            let prec = op.prec();
            assert!(prec >= prec1, "go_fmt: a binary operand was not parenthesized: {e:?}");
            let blank = prec < cutoff(e, depth);
            expr1(l, prec, depth + diff_prec(l, prec), out);
            if blank {
                out.push(' ');
            }
            out.push_str(op.token());
            if blank {
                out.push(' ');
            }
            expr1(r, prec + 1, depth + 1, out);
        }
        GExpr::Unary(op, x) => {
            assert!(UNARY_PREC >= prec1, "go_fmt: a unary operand was not parenthesized: {e:?}");
            out.push_str(op);
            expr1(x, UNARY_PREC, depth, out);
        }
        GExpr::Paren(x) => {
            out.push('(');
            expr1(x, 0, reduce_depth(depth), out);
            out.push(')');
        }
        GExpr::Call(f, args) => {
            let depth = if args.len() > 1 { depth + 1 } else { depth };
            out.push_str(f);
            out.push('(');
            for (i, a) in args.iter().enumerate() {
                if i > 0 {
                    out.push_str(", ");
                }
                expr1(a, 0, depth, out);
            }
            out.push(')');
        }
        GExpr::Index(x, i) => {
            expr1(x, HIGHEST_PREC, 1, out);
            out.push('[');
            expr1(i, 0, depth + 1, out);
            out.push(']');
        }
        GExpr::Slice(x, lo, hi) => {
            expr1(x, HIGHEST_PREC, 1, out);
            out.push('[');
            let count = usize::from(lo.is_some()) + usize::from(hi.is_some());
            let has_bin = [lo, hi].iter().any(|b| matches!(b.as_deref(), Some(GExpr::Bin(..))));
            let blanks = depth <= 1 && count > 1 && has_bin;
            if let Some(lo) = lo {
                expr1(lo, 0, depth + 1, out);
                if blanks {
                    out.push(' ');
                }
            }
            out.push(':');
            if let Some(hi) = hi {
                if blanks {
                    out.push(' ');
                }
                expr1(hi, 0, depth + 1, out);
            }
            out.push(']');
        }
    }
}

#[cfg(test)]
mod tests {
    use super::{GExpr, GOp};

    fn n(s: &str) -> GExpr {
        GExpr::name(s)
    }

    #[test]
    fn spacing_follows_gofmt() {
        let mul = GExpr::bin(n("a"), GOp::Mul, n("b"));
        assert_eq!(mul.render(), "a * b");
        assert_eq!(GExpr::bin(mul.clone(), GOp::Add, n("c")).render(), "a*b + c");
        assert_eq!(GExpr::call("float64", vec![mul.clone()]).render(), "float64(a * b)");
        assert_eq!(GExpr::call("f", vec![mul.clone(), n("c")]).render(), "f(a*b, c)");
        let idx = GExpr::index(n("x"), GExpr::bin(n("i"), GOp::Sub, n("1")));
        assert_eq!(idx.render(), "x[i-1]");
        let cmp = GExpr::bin(n("i"), GOp::Lt, GExpr::bin(n("n"), GOp::Sub, n("1")));
        assert_eq!(cmp.render(), "i < n-1");
        let neg = GExpr::bin(n("a"), GOp::Sub, GExpr::unary("-", n("b")));
        assert_eq!(neg.render(), "a - -b");
        let assoc = GExpr::bin(n("a"), GOp::Sub, GExpr::bin(n("b"), GOp::Sub, n("c")));
        assert_eq!(assoc.render(), "a - (b - c)");
        let sl = GExpr::slice(n("s"), Some(n("a")), Some(GExpr::bin(n("a"), GOp::Add, n("k"))));
        assert_eq!(sl.render(), "s[a : a+k]");
        assert_eq!(GExpr::call("copy", vec![n("d"), sl]).render(), "copy(d, s[a:a+k])");
    }
}
