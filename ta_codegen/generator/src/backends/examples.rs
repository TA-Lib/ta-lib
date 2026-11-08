//! The batch example behind every function page and every verified sample.
//!
//! One shape in every language: the input arrays named and sized, the call with every
//! parameter at its default, the output range, one printed value. A page shows the
//! `shown` lines; the copy a language's suite compiles carries the `hidden` lines too,
//! which fill the arrays with the synthetic series below and assert the result. One
//! emitter per language renders both from the same [`Snippet`], so a page cannot drift
//! from what compiled.

use super::doc_meta;
use crate::ir::{self, EnumDef, FuncDef, OptInput, ParamType};
use std::collections::HashMap;

/// Bars in every example series: one trading year, comfortably past the largest
/// default lookback (~64 for the Hilbert Transform family).
pub const LEN: usize = 252;

/// One line of a sample: on the page, or only in the compiled copy.
#[derive(Debug, Clone, PartialEq, Eq)]
pub struct Line {
    pub shown: bool,
    pub text: String,
}

pub fn shown(text: impl Into<String>) -> Line {
    Line { shown: true, text: text.into() }
}

pub fn hidden(text: impl Into<String>) -> Line {
    Line { shown: false, text: text.into() }
}

/// A language's sample: its import lines, then the body.
#[derive(Debug, Clone, PartialEq, Eq)]
pub struct Snippet {
    pub imports: Vec<String>,
    pub body: Vec<Line>,
}

impl Snippet {
    /// What the page shows: the imports, a blank line, the shown body lines.
    #[must_use]
    pub fn shown(&self) -> Vec<String> {
        let mut out = self.imports.clone();
        out.push(String::new());
        out.extend(self.body.iter().filter(|l| l.shown).map(|l| l.text.clone()));
        out
    }

    /// Every body line, for the compiled copy; the caller hoists the imports.
    #[must_use]
    pub fn body_text(&self) -> Vec<String> {
        self.body.iter().map(|l| l.text.clone()).collect()
    }
}

/// One term of a synthetic series in the bar index `i`.
#[derive(Debug, Clone, Copy, PartialEq)]
pub enum Term {
    Const(f64),
    /// `coef * i`
    Linear(f64),
    /// `amp * sin(freq * i + phase)`
    Sin { amp: f64, freq: f64, phase: f64 },
    /// `(i % m)`
    Mod(i64),
    /// `lo + (hi - lo) * i / (LEN - 1)`: `lo` on the first bar, `hi` on the last.
    Ramp { lo: f64, hi: f64 },
}

/// The synthetic series of one input and the variable that holds it.
#[derive(Debug, Clone, PartialEq)]
pub struct Series {
    pub var: &'static str,
    /// What the shown declaration says the array is.
    pub comment: &'static str,
    pub terms: Vec<Term>,
}

const SIN: Term = Term::Sin { amp: 10.0, freq: 0.1, phase: 0.0 };

/// The series of the input named `input`, or `None` for a shape no example knows.
///
/// `open` and `close` stay inside `[low, high]`: their offset from the bar midpoint
/// never reaches 1.0. `close` carries a second harmonic so it is never at the exact
/// midpoint of the bar, which left the money-flow multiplier identically zero and the
/// AD/ADOSC/CMF examples unable to fail (#136). `volume` rises overall but falls on
/// some bars, or NVI never takes its down-volume branch. An input declaring a
/// synthetic `series` gets it over its `domain`: the default sine mapped affinely
/// onto it, or a ramp from `lo` to `hi`. `ta_refdata` feeds the same two formulas.
pub fn series(func: &FuncDef, input: &str) -> Option<Series> {
    const DATA: &str = "your data, oldest first";
    let decl = func.inputs.iter().find(|i| i.name == input);
    if let Some((kind, (lo, hi))) = decl.and_then(|i| i.domain.map(|d| (i.series, d))) {
        let var = match input {
            "inReal0" => "series0",
            "inReal1" => "series1",
            _ => "series",
        };
        let mut terms = Vec::new();
        match kind {
            ir::Series::Ramp => terms.push(Term::Ramp { lo, hi }),
            ir::Series::Sine | ir::Series::Price => {
                let (mid, half) = (f64::midpoint(lo, hi), (hi - lo) / 2.0);
                if mid != 0.0 {
                    terms.push(Term::Const(mid));
                }
                terms.push(Term::Sin { amp: half, freq: 0.1, phase: 0.0 });
            }
        }
        return Some(Series { var, comment: DATA, terms });
    }
    let (var, terms) = match input {
        "inOpen" => ("open", vec![Term::Const(100.0), Term::Sin { amp: 10.0, freq: 0.1, phase: -0.05 }]),
        "inHigh" => ("high", vec![Term::Const(101.0), SIN]),
        "inLow" => ("low", vec![Term::Const(99.0), SIN]),
        "inClose" => (
            "close",
            vec![Term::Const(100.0), SIN, Term::Sin { amp: 0.8, freq: 0.7, phase: 0.0 }],
        ),
        "inVolume" => (
            "volume",
            vec![Term::Const(10000.0), Term::Linear(100.0), Term::Sin { amp: 2000.0, freq: 0.3, phase: 0.0 }],
        ),
        "inPeriods" => {
            return Some(Series {
                var: "periods",
                comment: "one period per bar",
                terms: vec![Term::Const(5.0), Term::Mod(10)],
            })
        }
        "inReal" => ("series", vec![Term::Const(100.0), SIN]),
        "inReal0" => ("series0", vec![Term::Const(100.0), SIN]),
        "inReal1" => ("series1", vec![Term::Const(100.0), Term::Sin { amp: 10.0, freq: 0.1, phase: 0.7 }]),
        _ => return None,
    };
    Some(Series { var, comment: DATA, terms })
}

/// The series of every input, in call order; `None` when one has no example shape.
pub fn all_series(func: &FuncDef) -> Option<Vec<Series>> {
    func.inputs.iter().map(|i| series(func, &i.name)).collect()
}

/// How a language spells the pieces of a series expression.
pub struct Syntax<'a> {
    /// A real literal.
    pub real: fn(f64) -> String,
    /// The bar index as a real: `i as f64` in Rust, `i` where it widens.
    pub bar: &'a str,
    /// `sin(x)` of a rendered argument.
    pub sin: fn(&str) -> String,
    /// `(i % m)` as a real.
    pub modulo: fn(i64) -> String,
}

/// The element expression of `series` in one language.
#[allow(clippy::float_cmp)] // the amplitudes are literals, not measurements
pub fn render(series: &Series, syn: &Syntax) -> String {
    let mut out = String::new();
    for term in &series.terms {
        let text = match *term {
            Term::Const(c) => (syn.real)(c),
            Term::Linear(c) => format!("{} * {}", (syn.real)(c), syn.bar),
            Term::Sin { amp, freq, phase } => {
                let mut arg = format!("{} * {}", (syn.real)(freq), syn.bar);
                if phase != 0.0 {
                    let sign = if phase < 0.0 { '-' } else { '+' };
                    arg.push_str(&format!(" {sign} {}", (syn.real)(phase.abs())));
                }
                let s = (syn.sin)(&arg);
                if amp == 1.0 { s } else { format!("{} * {s}", (syn.real)(amp)) }
            }
            Term::Mod(m) => (syn.modulo)(m),
            // The divisor is spelled `251.0` in every language: `i` is an integer
            // in C, Java and C#, and `span * i / 251` would divide as one.
            Term::Ramp { lo, hi } => {
                let ramp = format!("{} * {} / {}.0", (syn.real)(hi - lo), syn.bar, LEN - 1);
                if lo == 0.0 { ramp } else { format!("{} + {ramp}", (syn.real)(lo)) }
            }
        };
        if !out.is_empty() {
            out.push_str(" + ");
        }
        out.push_str(&text);
    }
    out
}

/// The `Math.sin` family: TypeScript, Java and C# agree on this spelling but for case.
pub const MATH_SIN: Syntax<'static> = Syntax {
    real: doc_meta::fmt_real,
    bar: "i",
    sin: |x| format!("Math.sin({x})"),
    modulo: |m| format!("(i % {m})"),
};

/// Words no sample may bind in C, Java, C# or Rust (`outRealLong` is CE's output).
const RESERVED: [&str; 14] =
    ["long", "short", "int", "double", "float", "char", "in", "do", "if", "for", "new", "const", "static", "void"];

/// The example variable of each output: `out` alone, else the TypeScript result key
/// without its `out` (`upperBand`, `macdSignal`), kept whole when the rest is a
/// keyword (`outLong`); Java, C# and C use it as is, Rust snake-cases it. TypeScript
/// itself destructures the result keys.
pub fn output_vars(func: &FuncDef) -> Vec<String> {
    func.outputs
        .iter()
        .map(|o| {
            let key = super::ts::result_key(func, o);
            if key == "values" {
                return "out".to_string();
            }
            let mut cs = key.strip_prefix("out").unwrap_or(&key).chars();
            let bare = cs.next().map_or_else(String::new, |f| f.to_ascii_lowercase().to_string() + cs.as_str());
            if RESERVED.contains(&bare.as_str()) { key } else { bare }
        })
        .collect()
}

/// An optional parameter at its default: an `enum:` one by name, through `variant`.
#[allow(clippy::cast_possible_truncation, clippy::implicit_hasher)]
pub fn opt_literal(
    opt: &OptInput,
    enums: &HashMap<String, EnumDef>,
    real: fn(f64) -> String,
    variant: impl Fn(&EnumDef, &str) -> String,
) -> String {
    let default = opt.default.unwrap_or(0.0);
    match &opt.param_type {
        ParamType::Real => real(default),
        ParamType::Enum(name) => enums
            .get(name)
            .and_then(|e| {
                e.variants.iter().find(|v| f64::from(v.value).to_bits() == default.to_bits()).map(|v| variant(e, &v.name))
            })
            .unwrap_or_else(|| format!("{}", default as i64)),
        _ => format!("{}", default as i64),
    }
}

/// Whether the function's real outputs may hold infinity on ordinary data, so a
/// sample may only assert them NaN-free.
pub fn allows_infinity(func: &FuncDef) -> bool {
    func.flags.iter().any(|fl| fl == "nan_inf_output")
}

/// `lead(args)tail`, or the arguments over two lines when the one-line form is long:
/// the inputs and parameters, then the outputs, the second line aligned under the first argument.
pub fn wrap_call(lead: &str, first: &[String], second: &[String], tail: &str, width: usize) -> Vec<String> {
    let mut all: Vec<String> = first.to_vec();
    all.extend(second.iter().cloned());
    let one = format!("{lead}{}{tail}", all.join(", "));
    if one.len() <= width || second.is_empty() {
        return vec![one];
    }
    let pad = " ".repeat(lead.len());
    vec![format!("{lead}{},", first.join(", ")), format!("{pad}{}{tail}", second.join(", "))]
}

#[cfg(test)]
mod tests {
    use super::*;

    fn rust_syntax() -> Syntax<'static> {
        Syntax {
            real: super::super::rust_doc::fmt_real_literal,
            bar: "i as f64",
            sin: |x| format!("({x}).sin()"),
            modulo: |m| format!("(i % {m}) as f64"),
        }
    }

    fn fixture(input: &str, domain: Option<(f64, f64)>) -> FuncDef {
        fixture_series(input, if domain.is_some() { ir::Series::Sine } else { ir::Series::Price }, domain)
    }

    fn fixture_series(input: &str, series: ir::Series, domain: Option<(f64, f64)>) -> FuncDef {
        FuncDef {
            name: "X".to_string(),
            group: String::new(),
            description: None,
            hint: None,
            flags: vec![],
            inputs: vec![crate::ir::Input { name: input.to_string(), param_type: ParamType::Real, price: None, series, domain }],
            optional_inputs: vec![],
            outputs: vec![],
            lookback: None,
            display_shift: None,
            body: vec![],
            private_body: vec![],
            private_extra_params: vec![],
            private_param_init: vec![],
            has_explicit_private: false,
            opt_outs: vec![],
            header_comments: vec![],
            doc: None,
            streaming: false,
            alternates: vec![],
            resolved_stream_body: None,
        }
    }

    #[test]
    fn series_render_in_each_syntax() {
        let close = series(&fixture("inClose", None), "inClose").unwrap();
        assert_eq!(close.var, "close");
        assert_eq!(
            render(&close, &rust_syntax()),
            "100.0 + 10.0 * (0.1 * i as f64).sin() + 0.8 * (0.7 * i as f64).sin()"
        );
        assert_eq!(render(&close, &MATH_SIN), "100 + 10 * Math.sin(0.1 * i) + 0.8 * Math.sin(0.7 * i)");
        let open = series(&fixture("inOpen", None), "inOpen").unwrap();
        assert_eq!(render(&open, &MATH_SIN), "100 + 10 * Math.sin(0.1 * i - 0.05)");
        let volume = series(&fixture("inVolume", None), "inVolume").unwrap();
        assert_eq!(render(&volume, &MATH_SIN), "10000 + 100 * i + 2000 * Math.sin(0.3 * i)");
        let periods = series(&fixture("inPeriods", None), "inPeriods").unwrap();
        assert_eq!(render(&periods, &rust_syntax()), "5.0 + (i % 10) as f64");
        assert_eq!(render(&periods, &MATH_SIN), "5 + (i % 10)");
    }

    #[test]
    fn a_domain_maps_the_sine_and_drops_the_unit_factors() {
        let unit = series(&fixture("inReal", Some((-1.0, 1.0))), "inReal").unwrap();
        assert_eq!(unit.var, "series");
        assert_eq!(render(&unit, &rust_syntax()), "(0.1 * i as f64).sin()");
        let wide = series(&fixture("inReal", Some((-3.0, 3.0))), "inReal").unwrap();
        assert_eq!(render(&wide, &MATH_SIN), "3 * Math.sin(0.1 * i)");
        let offset = series(&fixture("inReal", Some((0.0, 5.0))), "inReal").unwrap();
        assert_eq!(render(&offset, &MATH_SIN), "2.5 + 2.5 * Math.sin(0.1 * i)");
        assert!(series(&fixture("inNonesuch", None), "inNonesuch").is_none());
    }

    #[test]
    fn a_ramp_runs_from_lo_to_hi_with_a_real_divisor() {
        let unit = series(&fixture_series("inReal", ir::Series::Ramp, Some((-1.0, 1.0))), "inReal").unwrap();
        assert_eq!(unit.var, "series");
        assert_eq!(render(&unit, &rust_syntax()), "-1.0 + 2.0 * i as f64 / 251.0");
        assert_eq!(render(&unit, &MATH_SIN), "-1 + 2 * i / 251.0");
        let from_zero = series(&fixture_series("inReal", ir::Series::Ramp, Some((0.0, 100.0))), "inReal").unwrap();
        assert_eq!(render(&from_zero, &MATH_SIN), "100 * i / 251.0");
        let second = series(&fixture_series("inReal1", ir::Series::Ramp, Some((95.0, 105.0))), "inReal1").unwrap();
        assert_eq!(second.var, "series1");
        assert_eq!(render(&second, &MATH_SIN), "95 + 10 * i / 251.0");
        let first = series(&fixture_series("inReal0", ir::Series::Sine, Some((90.0, 110.0))), "inReal0").unwrap();
        assert_eq!(first.var, "series0");
        assert_eq!(render(&first, &MATH_SIN), "100 + 10 * Math.sin(0.1 * i)");
    }

    #[test]
    fn wrap_call_breaks_before_the_outputs() {
        let a = vec!["0".to_string(), "n - 1".to_string()];
        let b = vec!["out".to_string()];
        assert_eq!(wrap_call("f(", &a, &b, ");", 80), vec!["f(0, n - 1, out);"]);
        assert_eq!(wrap_call("f(", &a, &b, ");", 10), vec!["f(0, n - 1,", "  out);"]);
    }
}
