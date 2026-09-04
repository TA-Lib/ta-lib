//! Shared expression-tree walker for the language backends.
//!
//! The recursive descent over [`Expr`] — matching each variant and recursing into
//! its sub-expressions — has the same *shape* in the C, Rust and Java backends;
//! only the leaf formatting (operator tokens, cast syntax, variable-name mapping,
//! call dispatch) differs per language. [`ExprEmitter`] captures that split: the
//! trait owns the one exhaustive variant dispatch in [`ExprEmitter::walk`], and each
//! backend implements only the language-specific leaf hooks. Variants that render
//! identically in every backend (numeric/integer literals, logical `!`) have default
//! hook implementations here so no backend repeats them.
//!
//! A backend wires itself in by defining a small emitter struct that bundles its
//! render context (e.g. `CRenderCtx` plus the registry/helper services) and
//! implementing the leaf hooks; its free `render_expr` then becomes a thin
//! `emitter.walk(expr)`. Hooks recurse by calling [`ExprEmitter::walk`] on
//! sub-expressions, so the dispatch stays in one place.

use crate::ir::{BinOp, Expr, VarType};

/// Binding strength of a binary operator (higher binds tighter), following C
/// operator precedence. Backends use this to emit the *minimal* parenthesization
/// that still preserves the IR's grouping, instead of wrapping every operand.
///
/// Rust and Java share this ordering for the operators the IR uses, so the same
/// table serves those backends too.
#[must_use]
pub fn binop_prec(op: &BinOp) -> u8 {
    match op {
        BinOp::Or => 2,
        BinOp::And => 3,
        BinOp::BitwiseOr => 4,
        BinOp::BitwiseXor => 5,
        BinOp::BitwiseAnd => 6,
        BinOp::Eq | BinOp::NotEq => 7,
        BinOp::Less | BinOp::LessEq | BinOp::Greater | BinOp::GreaterEq => 8,
        BinOp::Shl | BinOp::Shr => 9,
        BinOp::Add | BinOp::Sub => 10,
        BinOp::Mul | BinOp::Div | BinOp::Mod => 11,
    }
}

/// True for expressions whose value is integer-typed by construction — the
/// bitwise binaries, shifts, and `~`. In C these carry truthiness in condition
/// position; Java/C#/Rust require an explicit `!= 0` there, and Rust's `!` on
/// them is bitwise (not logical), so backends special-case exactly this set.
#[must_use]
pub fn is_int_bitwise(e: &Expr) -> bool {
    matches!(
        e,
        Expr::BinOp(
            _,
            BinOp::BitwiseAnd | BinOp::BitwiseOr | BinOp::BitwiseXor | BinOp::Shl | BinOp::Shr,
            _
        ) | Expr::BitwiseNot(_)
    )
}

/// Binding strength of an expression's top-level operator (higher binds
/// tighter). Atomic and postfix operands (literals, identifiers, calls,
/// indexing, `x++`) sit above every binary operator, so they never need
/// wrapping; prefix-unary forms sit just below postfix.
#[must_use]
pub fn expr_prec(e: &Expr) -> u8 {
    match e {
        Expr::Ternary(..) => 1,
        Expr::BinOp(_, op, _) => binop_prec(op),
        Expr::Not(_)
        | Expr::BitwiseNot(_)
        | Expr::Cast(..)
        | Expr::AddressOf(_)
        | Expr::PreIncrement(_)
        | Expr::PreDecrement(_)
        | Expr::PointerDeref(_) => 12,
        Expr::PostIncrement(_)
        | Expr::PostDecrement(_)
        | Expr::ArrayAccess(..)
        | Expr::FuncCall(..) => 13,
        Expr::Literal(_) | Expr::IntLiteral(_) | Expr::Var(_) => 14,
    }
}

/// Precedence of an atomic/postfix operand — the binding strength a bare call
/// result, identifier, or index has. Anything looser (a binary op or ternary,
/// precedence `< ATOMIC_PREC`) must be parenthesized when spliced where an atom
/// is expected.
pub const ATOMIC_PREC: u8 = 12;

/// Parenthesize a rendered operand only when its own operator binds looser than
/// the enclosing operator `parent_prec`, or ties with it on the right — the IR's
/// binary operators are all left-associative, so an equal-precedence *left*
/// operand needs no parens. Atomic and tighter-binding operands pass through.
///
/// Shared by the C-family backends: C, Java and C# share this exact operator
/// precedence, and the only operators Rust ranks differently (bitwise `| ^ &`)
/// are unused by the indicators, so the one table is correct for every backend.
///
/// One exception to "minimal": a bitwise operator's operands are wrapped as soon
/// as they are not atomic, even where precedence already groups them right. gcc
/// `-Wparentheses` rejects `a - 1 & mask` in the shipped library, and the
/// ring-slot rewrite produces exactly that from any `arr[i - 1]`.
#[must_use]
pub fn wrap_child(rendered: String, child: &Expr, parent_prec: u8, is_right: bool) -> String {
    let cp = expr_prec(child);
    if cp < parent_prec
        || (cp == parent_prec && is_right)
        || (cp < ATOMIC_PREC && is_bitwise_prec(parent_prec))
    {
        format!("({rendered})")
    } else {
        rendered
    }
}

/// True for the precedence levels the bitwise binaries occupy. Derived from
/// [`binop_prec`] rather than written out, so the two cannot drift.
fn is_bitwise_prec(prec: u8) -> bool {
    prec == binop_prec(&BinOp::BitwiseAnd)
        || prec == binop_prec(&BinOp::BitwiseOr)
        || prec == binop_prec(&BinOp::BitwiseXor)
}

/// Parenthesize a rendered *inlined helper body* when it is non-atomic (a binary
/// op or ternary). Helper inlining splices the body where the caller expects an
/// atomic call result, so an unwrapped binary/ternary body could otherwise
/// mis-group with the surrounding operators.
#[must_use]
pub fn wrap_inlined(rendered: String, inlined: &Expr) -> String {
    if expr_prec(inlined) < ATOMIC_PREC {
        format!("({rendered})")
    } else {
        rendered
    }
}

/// Per-language leaf formatting for the shared [`walk`](ExprEmitter::walk) dispatch.
///
/// Implementors provide the language-specific hooks; the default `walk` owns the
/// exhaustive match over [`Expr`] variants and routes each to its hook. The three
/// universally-identical leaves ([`literal`](ExprEmitter::literal),
/// [`int_literal`](ExprEmitter::int_literal), [`not`](ExprEmitter::not)) have shared
/// defaults.
pub trait ExprEmitter {
    /// Render an `Expr::Var` reference (identifier or backend-specific constant mapping).
    fn var(&self, name: &str) -> String;

    /// Render an `Expr::ArrayAccess` (`name[idx]`); recurse into `idx` via [`walk`](Self::walk).
    fn array_access(&self, name: &str, idx: &Expr) -> String;

    /// Render an `Expr::BinOp`; recurse into `left`/`right` via [`walk`](Self::walk).
    fn binop(&self, left: &Expr, op: &BinOp, right: &Expr) -> String;

    /// Render an `Expr::Cast` to `ty`; recurse into `inner` via [`walk`](Self::walk).
    fn cast(&self, ty: &VarType, inner: &Expr) -> String;

    /// Render an `Expr::FuncCall` (builtin/helper/cross-indicator call dispatch).
    fn func_call(&self, name: &str, args: &[Expr]) -> String;

    /// Render an `Expr::PointerDeref` (`*name`).
    fn pointer_deref(&self, name: &str) -> String;

    /// Render an `Expr::AddressOf`; recurse into `inner` via [`walk`](Self::walk).
    fn address_of(&self, inner: &Expr) -> String;

    /// Render an `Expr::PostIncrement` (`inner++`).
    fn post_increment(&self, inner: &Expr) -> String;

    /// Render an `Expr::PostDecrement` (`inner--`).
    fn post_decrement(&self, inner: &Expr) -> String;

    /// Render an `Expr::PreIncrement` (`++inner`).
    fn pre_increment(&self, inner: &Expr) -> String;

    /// Render an `Expr::PreDecrement` (`--inner`).
    fn pre_decrement(&self, inner: &Expr) -> String;

    /// Render an `Expr::Ternary` (`cond ? then : else`).
    fn ternary(&self, cond: &Expr, then_expr: &Expr, else_expr: &Expr) -> String;

    /// Render an `Expr::Literal` (floating-point constant). Whole values gain a
    /// `.0` suffix (`3` → `3.0`); others use Rust's default `f64` formatting. This
    /// is byte-identical across the C, Rust and Java backends.
    ///
    /// One case the plain formatting gets *wrong*, reachable since the lexer
    /// learned exponents (`1e-13`): a whole value at or above 1e15 misses the
    /// `.0` branch above, and Rust's `Display` for `f64` never uses exponent
    /// notation, so what comes out is a run of digits carrying neither `.` nor
    /// `e`. In C that is an *integer* constant — a different type — and past
    /// `LLONG_MAX` an ill-formed one. Those switch to `{:e}` (`1e300`), spelled
    /// the same way in C, Rust, Java and C#.
    ///
    /// Deliberately corrective only, not cosmetic. A small literal keeps its
    /// positional spelling — `1e-13` in the input renders as
    /// `0.0000000000001`, the same double, long but valid everywhere. Emitting
    /// exponent form there reads better but re-spells constants already in the
    /// corpus (`0.000001` in `var.c` → `1e-6`, 10 generated files), which is a
    /// deliberate re-spelling to make on its own, not a side effect of teaching
    /// the lexer a token.
    fn literal(&self, f: f64) -> String {
        #[allow(clippy::float_cmp)]
        let is_whole = f == f.floor() && f.abs() < 1e15;
        if is_whole {
            #[allow(clippy::cast_possible_truncation)]
            let i = f as i64;
            return format!("{i}.0");
        }
        let s = format!("{f}");
        if s.contains(['.', 'e', 'E']) {
            s
        } else {
            format!("{f:e}")
        }
    }

    /// Render an `Expr::IntLiteral` (integer constant). Identical across backends.
    fn int_literal(&self, i: i64) -> String {
        format!("{i}")
    }

    /// Render an `Expr::Not` (`!(inner)`). Identical across backends.
    fn not(&self, inner: &Expr) -> String {
        format!("!({})", self.walk(inner))
    }

    /// Render an `Expr::BitwiseNot` (`~(inner)`). Identical across the
    /// C-family backends (C, Java, C# all spell bitwise complement `~`).
    fn bitwise_not(&self, inner: &Expr) -> String {
        format!("~({})", self.walk(inner))
    }

    /// The owned recursion: match `expr`'s variant and dispatch to the
    /// corresponding leaf hook. This is the single copy of the `Expr` tree-walk
    /// that every backend shares; the match is intentionally exhaustive (no
    /// wildcard arm) so a new IR variant forces every backend to handle it.
    fn walk(&self, expr: &Expr) -> String {
        match expr {
            Expr::Literal(f) => self.literal(*f),
            Expr::IntLiteral(i) => self.int_literal(*i),
            Expr::Var(name) => self.var(name),
            Expr::ArrayAccess(name, idx) => self.array_access(name, idx),
            Expr::BinOp(left, op, right) => self.binop(left, op, right),
            Expr::Cast(ty, inner) => self.cast(ty, inner),
            Expr::Not(inner) => self.not(inner),
            Expr::BitwiseNot(inner) => self.bitwise_not(inner),
            Expr::FuncCall(name, args) => self.func_call(name, args),
            Expr::PointerDeref(name) => self.pointer_deref(name),
            Expr::AddressOf(inner) => self.address_of(inner),
            Expr::PostIncrement(inner) => self.post_increment(inner),
            Expr::PostDecrement(inner) => self.post_decrement(inner),
            Expr::PreIncrement(inner) => self.pre_increment(inner),
            Expr::PreDecrement(inner) => self.pre_decrement(inner),
            Expr::Ternary(cond, then_expr, else_expr) => self.ternary(cond, then_expr, else_expr),
        }
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    /// `literal` is a provided method that reads nothing off `self`, so the
    /// required half of the trait only has to exist, not work.
    struct Probe;

    macro_rules! unused {
        ($($name:ident($($arg:ident: $ty:ty),*)),* $(,)?) => {
            $(fn $name(&self $(, $arg: $ty)*) -> String { unimplemented!() })*
        };
    }

    impl ExprEmitter for Probe {
        unused! {
            var(_n: &str),
            array_access(_n: &str, _i: &Expr),
            binop(_l: &Expr, _o: &BinOp, _r: &Expr),
            cast(_t: &VarType, _i: &Expr),
            func_call(_n: &str, _a: &[Expr]),
            pointer_deref(_n: &str),
            address_of(_i: &Expr),
            post_increment(_i: &Expr),
            post_decrement(_i: &Expr),
            pre_increment(_i: &Expr),
            pre_decrement(_i: &Expr),
            ternary(_c: &Expr, _t: &Expr, _e: &Expr),
        }
    }

    /// Every spelling below has to be a valid FLOATING constant in C, Rust,
    /// Java and C# at once -- the four backends share this one renderer, so a
    /// form only three of them accept breaks the fourth's build, not a value
    /// gate. Reachable since the lexer learned exponents; no shipped input
    /// body has a literal outside the middle band.
    #[test]
    fn literal_spellings() {
        let p = Probe;

        // Unchanged middle band: the whole corpus lives here.
        assert_eq!(p.literal(0.0), "0.0");
        assert_eq!(p.literal(3.0), "3.0");
        assert_eq!(p.literal(-2.0), "-2.0");
        assert_eq!(p.literal(0.5), "0.5");
        assert_eq!(p.literal(0.015), "0.015");
        assert_eq!(p.literal(100.0), "100.0");

        // A small literal keeps its positional spelling. Long, but the same
        // double, and re-spelling it would move constants already in the
        // corpus (var.c's 0.000001) for no semantic gain.
        assert_eq!(p.literal(1e-13), "0.0000000000001");
        assert_eq!(p.literal(0.000_001), "0.000001");
        assert_eq!(p.literal(0.00012), "0.00012");

        // A whole value past 1e15 misses the `.0` branch; positional form
        // would emit an integer constant, and above LLONG_MAX an invalid one.
        assert_eq!(p.literal(1e300), "1e300");
        assert_eq!(p.literal(1e16), "1e16");

        // Whatever the path, the result is never bare digits.
        for v in [0.0, 3.0, 1e-13, 1e300, 1e16, 0.5, -2.5e-7, 1e15] {
            let s = p.literal(v);
            assert!(
                s.contains(['.', 'e', 'E']),
                "literal({v}) rendered as {s:?}, an integer constant in C"
            );
        }

        // And it round-trips: the spelling names the same double it was given.
        for v in [1e-13, -2.5e-7, 1e300, 1e16, 0.015, 0.5, 123.456, 1e15] {
            let s = p.literal(v);
            assert_eq!(
                s.parse::<f64>().unwrap().to_bits(),
                v.to_bits(),
                "literal({v}) rendered {s:?}, which is a different double"
            );
        }
    }
}
