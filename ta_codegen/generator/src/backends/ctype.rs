//! The C type of every expression a TypeScript body renders (D10).
//!
//! JS has one number type, so each place where C's integer arithmetic differs
//! from float arithmetic has to be spelled out, and a wrong guess fails
//! silently. The verdict comes from C's own rules, seeded from declarations,
//! never from a name. An expression this pass cannot type stops generation.
//!
//! `Bool` is the 0/1 `int` a relational or logical operator yields; TS renders
//! it as a `boolean`, so it needs a conversion wherever C uses it as a number.

use std::cell::RefCell;
use std::collections::HashMap;

use crate::helper_registry::{try_inline_expr, HelperRegistry};
use crate::ir::{BinOp, CircBuf, CircBufLayout, EnumDef, Expr, FuncDef, HelperDef, ParamType, Statement, VarType};
use crate::registry::{Lang, Registry};

use super::builtins::{MathFn, SpecialBuiltin, StdlibFn};

#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum CType {
    Int32,
    Real,
    Bool,
    RealPtr,
    IntPtr,
}

impl CType {
    #[must_use]
    pub fn of_var_type(t: &VarType) -> Self {
        match t {
            VarType::Real => Self::Real,
            VarType::Integer | VarType::Index | VarType::RetCodeType => Self::Int32,
            VarType::RealPointer | VarType::RealArray(_) => Self::RealPtr,
            VarType::IntPointer | VarType::IntArray(_) => Self::IntPtr,
        }
    }

    #[must_use]
    pub fn of_param(t: &ParamType) -> Self {
        match t {
            ParamType::Real => Self::Real,
            ParamType::Integer | ParamType::Enum(_) => Self::Int32,
            ParamType::Price(_) => unreachable!("Price expanded during parsing"),
        }
    }

    #[must_use]
    pub fn is_pointer(self) -> bool {
        matches!(self, Self::RealPtr | Self::IntPtr)
    }

    /// The type C computes a value of this type in, once it is used as a number.
    #[must_use]
    pub fn numeric(self) -> Self {
        if self == Self::Bool {
            Self::Int32
        } else {
            self
        }
    }
}

fn arith(l: CType, r: CType, what: &dyn Fn() -> String) -> CType {
    assert!(!l.is_pointer() && !r.is_pointer(), "TS type pass: pointer arithmetic in {}", what());
    if l == CType::Real || r == CType::Real {
        CType::Real
    } else {
        CType::Int32
    }
}

/// What the TS emitter must add so a JS number behaves as the C `int` would.
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum IntFix {
    /// An `int / int`: `((a / b) | 0)`.
    TruncDiv,
    /// An `int % int`, which yields -0 in JS: `((a % b) | 0)`.
    TruncMod,
    /// An `int * int`, which yields -0 in JS: `((a * b) | 0)`.
    WrapMul,
    /// An integer negation, which yields -0 in JS: `(-a | 0)`.
    WrapNeg,
    /// An explicit `(int)` of a real: `(x | 0)`.
    TruncCast,
}

pub struct TypeEnv<'a> {
    vars: RefCell<HashMap<String, CType>>,
    helpers: &'a HelperRegistry,
    registry: &'a Registry,
}

impl<'a> TypeEnv<'a> {
    #[must_use]
    pub fn new(helpers: &'a HelperRegistry, registry: &'a Registry, enums: &HashMap<String, EnumDef>) -> Self {
        let mut vars = HashMap::new();
        for e in enums.values() {
            for v in &e.variants {
                vars.insert(v.c_name.clone(), CType::Int32);
            }
        }
        for c in [
            "SUCCESS",
            "BAD_PARAM",
            "ALLOC_ERR",
            "INTERNAL_ERROR",
            "BadParam",
            "InsufficientHistory",
            "OutOfRangeEndIndex",
            "OutOfRangeStartIndex",
        ] {
            vars.insert(c.to_string(), CType::Int32);
        }
        Self { vars: RefCell::new(vars), helpers, registry }
    }

    /// Declares `name`. A second declaration must agree: a flat scope is only
    /// sound while no two blocks give one name two types.
    pub fn declare(&self, name: &str, t: CType) {
        let mut vars = self.vars.borrow_mut();
        if let Some(prev) = vars.insert(name.to_string(), t) {
            assert!(prev == t, "TS type pass: `{name}` is declared both {prev:?} and {t:?}");
        }
    }

    #[must_use]
    pub fn lookup(&self, name: &str) -> Option<CType> {
        self.vars.borrow().get(name).copied()
    }

    pub fn seed_signature(&self, func: &FuncDef) {
        self.declare("startIdx", CType::Int32);
        self.declare("endIdx", CType::Int32);
        for i in &func.inputs {
            self.declare(&i.name, if CType::of_param(&i.param_type) == CType::Real { CType::RealPtr } else { CType::IntPtr });
        }
        self.seed_opts(func);
        for (name, c_type) in &func.private_extra_params {
            self.declare(name, if c_type == "int" { CType::Int32 } else { CType::Real });
        }
        self.declare("outBegIdx", CType::IntPtr);
        self.declare("outNBElement", CType::IntPtr);
        for o in &func.outputs {
            self.declare(&o.name, if o.param_type == ParamType::Real { CType::RealPtr } else { CType::IntPtr });
        }
    }

    pub fn seed_opts(&self, func: &FuncDef) {
        for o in &func.optional_inputs {
            self.declare(&o.name, CType::of_param(&o.param_type));
        }
    }

    /// Every declaration in `stmts`, nested blocks included, and the storage a
    /// `CIRCBUF` prolog declares.
    pub fn seed_decls(&self, stmts: &[Statement]) {
        for s in stmts {
            match s {
                Statement::VarDecl { var_type, name, .. } => self.declare(name, CType::of_var_type(var_type)),
                Statement::CircBuf(CircBuf::Prolog { id, layout, .. }) => {
                    let ptr = |t: &VarType| if CType::of_var_type(t) == CType::Real { CType::RealPtr } else { CType::IntPtr };
                    match layout {
                        CircBufLayout::Plain(t) => self.declare(id, ptr(t)),
                        CircBufLayout::Class(fields) => {
                            for (f, t) in fields {
                                self.declare(&format!("{id}_{f}"), ptr(t));
                            }
                        }
                    }
                    self.declare(&format!("{id}_Idx"), CType::Int32);
                    self.declare(&format!("maxIdx_{id}"), CType::Int32);
                }
                Statement::If { then_body, else_body, .. } => {
                    self.seed_decls(then_body);
                    self.seed_decls(else_body);
                }
                Statement::While { body, .. }
                | Statement::DoWhile { body, .. }
                | Statement::For { body, .. }
                | Statement::Block { body } => self.seed_decls(body),
                Statement::ForC { init, update, body, .. } => {
                    self.seed_decls(std::slice::from_ref(init));
                    self.seed_decls(std::slice::from_ref(update));
                    self.seed_decls(body);
                }
                Statement::Switch { cases, default, .. } => {
                    for (_, b) in cases {
                        self.seed_decls(b);
                    }
                    self.seed_decls(default);
                }
                Statement::Assign { .. }
                | Statement::Return { .. }
                | Statement::UnrollHint { .. }
                | Statement::Break
                | Statement::Continue
                | Statement::Expr(_)
                | Statement::CircBuf(_)
                | Statement::Comment(_) => {}
            }
        }
    }

    /// The `<Setting>_rangeType` / `_avgPeriod` / `_factor` locals a candle body unpacks.
    pub fn seed_candles(&self, settings: &std::collections::BTreeSet<String>) {
        for s in settings {
            self.declare(&format!("{s}_rangeType"), CType::Int32);
            self.declare(&format!("{s}_avgPeriod"), CType::Int32);
            self.declare(&format!("{s}_factor"), CType::Real);
        }
    }

    pub fn seed_helper(&self, helper: &HelperDef) {
        for p in &helper.params {
            self.declare(&p.name, CType::of_var_type(&p.var_type));
        }
        self.seed_decls(&helper.body);
    }

    /// The single-expression helper `helper(args)` with C's implicit
    /// conversions made explicit: an `int` parameter given a real, and an `int`
    /// return of a real. Substitution alone would carry the argument's type
    /// into the body.
    #[must_use]
    pub fn inline_helper(&self, helper: &HelperDef, args: &[Expr]) -> Option<Expr> {
        let converted: Vec<Expr> = helper
            .params
            .iter()
            .zip(args)
            .map(|(p, a)| self.convert_to(CType::of_var_type(&p.var_type), a))
            .collect();
        let inlined = try_inline_expr(helper, &converted)?;
        Some(self.convert_to(CType::of_var_type(&helper.return_type), &inlined))
    }

    /// `value` as C converts it on assignment to `dest`: only a real reaching
    /// an `int` changes, and it becomes an explicit cast.
    #[must_use]
    pub fn convert_to(&self, dest: CType, value: &Expr) -> Expr {
        if dest == CType::Int32 && self.type_of(value) == CType::Real {
            Expr::Cast(VarType::Integer, Box::new(value.clone()))
        } else {
            value.clone()
        }
    }

    /// True when storing `value` into a scalar of type `dest` truncates.
    #[must_use]
    pub fn implicit_trunc(&self, dest: CType, value: &Expr) -> bool {
        dest == CType::Int32 && self.type_of(value) == CType::Real
    }

    #[must_use]
    pub fn int_fix(&self, expr: &Expr) -> Option<IntFix> {
        match expr {
            Expr::BinOp(_, op @ (BinOp::Div | BinOp::Mod | BinOp::Mul), _) if self.type_of(expr) == CType::Int32 => {
                Some(match op {
                    BinOp::Div => IntFix::TruncDiv,
                    BinOp::Mod => IntFix::TruncMod,
                    _ => IntFix::WrapMul,
                })
            }
            Expr::Neg(inner) if self.type_of(inner).numeric() == CType::Int32 => Some(IntFix::WrapNeg),
            Expr::Cast(VarType::Integer | VarType::Index, inner) if self.type_of(inner) == CType::Real => {
                Some(IntFix::TruncCast)
            }
            _ => None,
        }
    }

    /// The C type of `expr`. Panics on anything it cannot type (H1).
    #[must_use]
    #[allow(clippy::too_many_lines)]
    pub fn type_of(&self, expr: &Expr) -> CType {
        let what = || format!("{expr:?}");
        match expr {
            Expr::Literal(_) => CType::Real,
            Expr::IntLiteral(_) => CType::Int32,
            Expr::Var(name) => self
                .lookup(name)
                .unwrap_or_else(|| panic!("TS type pass: `{name}` has no declared type")),
            Expr::ArrayAccess(name, idx) => {
                let it = self.type_of(idx);
                assert!(
                    it.numeric() == CType::Int32,
                    "TS type pass: index of `{name}` is {it:?}, not an int: {}",
                    what()
                );
                match self.lookup(name) {
                    Some(CType::RealPtr) => CType::Real,
                    Some(CType::IntPtr) => CType::Int32,
                    other => panic!("TS type pass: `{name}` is indexed but is {other:?}"),
                }
            }
            Expr::PointerDeref(name) => match self.lookup(name) {
                Some(CType::RealPtr) => CType::Real,
                Some(CType::IntPtr) => CType::Int32,
                other => panic!("TS type pass: `*{name}` dereferences {other:?}"),
            },
            Expr::AddressOf(inner) => match inner.as_ref() {
                Expr::Var(n) | Expr::ArrayAccess(n, _) => match self.lookup(n) {
                    Some(CType::Real | CType::RealPtr) => CType::RealPtr,
                    Some(CType::Int32 | CType::IntPtr) => CType::IntPtr,
                    other => panic!("TS type pass: `&{n}` of {other:?}"),
                },
                _ => panic!("TS type pass: address of a non-lvalue: {}", what()),
            },
            Expr::BinOp(l, op, r) => {
                let (lt, rt) = (self.type_of(l), self.type_of(r));
                match op {
                    BinOp::Add | BinOp::Sub | BinOp::Mul | BinOp::Div => arith(lt.numeric(), rt.numeric(), &what),
                    BinOp::Mod | BinOp::BitwiseAnd | BinOp::BitwiseOr | BinOp::BitwiseXor | BinOp::Shl | BinOp::Shr => {
                        assert!(
                            lt.numeric() == CType::Int32 && rt.numeric() == CType::Int32,
                            "TS type pass: integer operator over {lt:?}, {rt:?}: {}",
                            what()
                        );
                        CType::Int32
                    }
                    BinOp::Eq | BinOp::NotEq => {
                        assert!(
                            lt.is_pointer() == rt.is_pointer(),
                            "TS type pass: pointer compared with a number: {}",
                            what()
                        );
                        CType::Bool
                    }
                    BinOp::Less | BinOp::LessEq | BinOp::Greater | BinOp::GreaterEq => {
                        assert!(!lt.is_pointer() && !rt.is_pointer(), "TS type pass: pointer ordering: {}", what());
                        CType::Bool
                    }
                    BinOp::And | BinOp::Or => CType::Bool,
                }
            }
            Expr::Cast(t, inner) => {
                let it = self.type_of(inner);
                let out = CType::of_var_type(t);
                assert!(
                    out.is_pointer() == it.is_pointer(),
                    "TS type pass: cast between a pointer and a number: {}",
                    what()
                );
                out
            }
            Expr::Not(_) => CType::Bool,
            Expr::Neg(inner) => {
                let t = self.type_of(inner).numeric();
                assert!(!t.is_pointer(), "TS type pass: negated pointer: {}", what());
                t
            }
            Expr::BitwiseNot(inner) => {
                let t = self.type_of(inner).numeric();
                assert!(t == CType::Int32, "TS type pass: `~` of {t:?}: {}", what());
                t
            }
            Expr::PostIncrement(inner)
            | Expr::PostDecrement(inner)
            | Expr::PreIncrement(inner)
            | Expr::PreDecrement(inner) => self.type_of(inner),
            Expr::Ternary(c, t, e) => {
                let _ = self.type_of(c);
                if let (Expr::IntLiteral(a), Expr::IntLiteral(b)) = (t.as_ref(), e.as_ref()) {
                    if (*a, *b) == (1, 0) || (*a, *b) == (0, 1) {
                        return CType::Bool;
                    }
                }
                let (tt, et) = (self.type_of(t), self.type_of(e));
                if tt == CType::Bool && et == CType::Bool {
                    CType::Bool
                } else if tt.is_pointer() || et.is_pointer() {
                    assert!(tt == et, "TS type pass: ternary over {tt:?} and {et:?}: {}", what());
                    tt
                } else {
                    arith(tt.numeric(), et.numeric(), &what)
                }
            }
            Expr::FuncCall(name, args) => self.call_type(name, args),
        }
    }

    fn call_type(&self, name: &str, args: &[Expr]) -> CType {
        if let Some(helper) = self.helpers.get(name) {
            let declared = CType::of_var_type(&helper.return_type);
            if let Some(inlined) = self.inline_helper(helper, args) {
                let t = self.type_of(&inlined);
                if declared == CType::Int32 && t == CType::Bool {
                    return CType::Bool;
                }
            }
            return declared;
        }
        if let Some(b) = SpecialBuiltin::from_name(name) {
            return match b {
                SpecialBuiltin::IsZero | SpecialBuiltin::IsZeroScaled | SpecialBuiltin::IsZeroOrNeg | SpecialBuiltin::IsFinite => {
                    CType::Bool
                }
                SpecialBuiltin::UnstablePeriod | SpecialBuiltin::UnstableAuto => CType::Int32,
                SpecialBuiltin::PerToK => CType::Real,
                SpecialBuiltin::Unlikely => self.type_of(&args[0]),
                SpecialBuiltin::ArrayCopy => panic!("TS type pass: ARRAY_COPY has no value"),
            };
        }
        if let Some(m) = MathFn::from_name(name) {
            return match m {
                // C renders max/min as its branch macros, whose type is the
                // usual conversion of the two operands.
                MathFn::Max | MathFn::Min => {
                    let [a, b] = args else { panic!("TS type pass: {name} takes two arguments") };
                    arith(self.type_of(a).numeric(), self.type_of(b).numeric(), &|| name.to_string())
                }
                _ => CType::Real,
            };
        }
        if let Some(s) = StdlibFn::from_name(name) {
            return match s {
                StdlibFn::Sizeof => CType::Int32,
                StdlibFn::Malloc => {
                    if super::common::find_sizeof_type(&args[0]).as_deref() == Some("int") {
                        CType::IntPtr
                    } else {
                        CType::RealPtr
                    }
                }
                _ => panic!("TS type pass: {name} has no value"),
            };
        }
        if self.registry.contains(name) || self.registry.resolve_call(name, Lang::Ts) != name {
            return CType::Int32;
        }
        panic!("TS type pass: call to `{name}` has no known type")
    }
}
