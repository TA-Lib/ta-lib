//! Per-function attributes: numeric metadata a flag bit cannot carry, published
//! as one canonical JSON string per function (spec `/spec/abstract/#attributes`).
//!
//! Adding a key is one row in [`KEYS`] plus its typed field in the YAML parser
//! and the IR. The string is public API in every language: never remove,
//! rename or reorder a row, and never change what a key means.
//!
//! Two renderers walk this registry, `abstract_rows` (Rust, Java, C#, TS) and
//! `ta_abstract_c` (C, and the XML), on purpose: `test_abstract.c` compares
//! their strings byte for byte, which a single renderer would make vacuous.

use std::fmt::Write as _;

use crate::ir::{FuncDef, Input, OptInput, Output};

/// A value an accessor found, in the shape its key's [`Type`] declares.
#[derive(Debug, Clone, Copy, PartialEq)]
pub enum Value<'a> {
    Reals(&'a [f64]),
}

impl Value<'_> {
    /// The canonical JSON text: no whitespace, each number in Rust's `f64`
    /// `Display` form (`30`, `26.5`), which never uses an exponent.
    pub fn write_json(&self, out: &mut String) {
        match self {
            Value::Reals(v) => {
                out.push('[');
                for (i, x) in v.iter().enumerate() {
                    if i > 0 {
                        out.push(',');
                    }
                    let _ = write!(out, "{x}");
                }
                out.push(']');
            }
        }
    }

    fn ty(&self) -> Type {
        match self {
            Value::Reals(_) => Type::AscendingReals,
        }
    }
}

/// Where a key lives, with the accessor that reads it off the IR.
#[derive(Clone, Copy)]
pub enum Scope {
    Function(fn(&FuncDef) -> Option<Value<'_>>),
    Input(fn(&Input) -> Option<Value<'_>>),
    OptInput(fn(&OptInput) -> Option<Value<'_>>),
    Output(fn(&Output) -> Option<Value<'_>>),
}

/// The JSON section of each argument scope. Sections follow the function's own
/// keys, in this order.
pub const INPUTS: &str = "inputs";
pub const OPT_INPUTS: &str = "optInputs";
pub const OUTPUTS: &str = "outputs";

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum Type {
    /// A non-empty list of finite numbers, strictly ascending.
    AscendingReals,
}

impl Type {
    pub fn check(self, v: &[f64]) -> Result<(), String> {
        match self {
            Type::AscendingReals => {
                if v.is_empty() {
                    return Err("an empty list".into());
                }
                if let Some(x) = v.iter().find(|x| !x.is_finite()) {
                    return Err(format!("{x} is not finite"));
                }
                if v.windows(2).any(|w| w[0] >= w[1]) {
                    return Err(format!("{v:?} is not strictly ascending"));
                }
                Ok(())
            }
        }
    }
}

pub struct Key {
    pub name: &'static str,
    pub scope: Scope,
    pub ty: Type,
}

impl Key {
    /// `None` both for an item of another scope and for an item with no value.
    pub fn of_function<'a>(&self, f: &'a FuncDef) -> Option<Value<'a>> {
        match self.scope {
            Scope::Function(get) => self.checked(get(f)),
            _ => None,
        }
    }

    pub fn of_input<'a>(&self, i: &'a Input) -> Option<Value<'a>> {
        match self.scope {
            Scope::Input(get) => self.checked(get(i)),
            _ => None,
        }
    }

    pub fn of_opt_input<'a>(&self, o: &'a OptInput) -> Option<Value<'a>> {
        match self.scope {
            Scope::OptInput(get) => self.checked(get(o)),
            _ => None,
        }
    }

    pub fn of_output<'a>(&self, o: &'a Output) -> Option<Value<'a>> {
        match self.scope {
            Scope::Output(get) => self.checked(get(o)),
            _ => None,
        }
    }

    fn checked<'a>(&self, v: Option<Value<'a>>) -> Option<Value<'a>> {
        if let Some(v) = &v {
            assert_eq!(v.ty(), self.ty, "attribute `{}`: the accessor returned another type", self.name);
        }
        v
    }
}

fn levels(o: &Output) -> Option<Value<'_>> {
    (!o.levels.is_empty()).then_some(Value::Reals(&o.levels))
}

/// Every attribute key, in the order a JSON object lists them.
pub const KEYS: &[Key] = &[Key {
    name: "levels",
    scope: Scope::Output(levels),
    ty: Type::AscendingReals,
}];

/// The row of a key the parser reads.
pub fn key(name: &str) -> &'static Key {
    KEYS.iter().find(|k| k.name == name).unwrap_or_else(|| panic!("attribute `{name}` has no row in KEYS"))
}

/// The string a function with no attribute carries.
pub const EMPTY: &str = "{}";

/// Both renderers' last step: the string is embedded verbatim in a CDATA
/// section of `ta_func_api.xml`.
pub fn assert_embeddable(name: &str, json: &str) {
    assert!(!json.contains("]]>"), "{name}: attributes {json} would close the XML CDATA section");
}
