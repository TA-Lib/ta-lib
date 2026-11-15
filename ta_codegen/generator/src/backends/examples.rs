//! The batch example behind every function page and every verified sample.
//!
//! One shape in every language: the input arrays named and sized, the call with every
//! parameter at its default, the output range, the first and last value of every output.
//! A page shows the `shown` lines; the copy a language's suite compiles carries the
//! `hidden` lines too, which fill the arrays with the synthetic series below and assert
//! the result. One emitter per language renders both from the same [`Snippet`], so a
//! page cannot drift from what compiled.

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

/// One integer term in the bar index `i`.
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum Int {
    Const(i64),
    /// `coef * i`
    Linear(i64),
    /// `i % m`
    Mod(i64),
    /// `coef * k * k` with `k = (i + shift) % period - period / 2`: a parabola every
    /// `period` bars, 0 on its middle bar and `coef * (period / 2)^2` on its first.
    Wave { coef: i64, period: i64, shift: i64 },
}

/// How a synthetic series is computed from the bar index `i`.
///
/// The inputs must be the same bits in every language, so no transcendental, and no
/// real product may be an addend: Go fuses `a*b + c` into an FMA on some targets.
#[derive(Debug, Clone, PartialEq)]
pub enum Shape {
    /// `offset + scale * sum / div`, `sum` the integer sum of `terms`: exact
    /// everywhere, so what follows is one division and at most two more roundings.
    Waves { offset: f64, scale: f64, terms: Vec<Int>, div: i64 },
    /// `lo + (hi - lo) * i / (LEN - 1)`: `lo` on the first bar, `hi` on the last.
    Ramp { lo: f64, hi: f64 },
}

/// The synthetic series of one input and the variable that holds it.
#[derive(Debug, Clone, PartialEq)]
pub struct Series {
    pub var: &'static str,
    /// What the shown declaration says the array is.
    pub comment: &'static str,
    pub shape: Shape,
}

/// The price wave: 0 to 400 over 40 bars.
const PRICE: Int = Int::Wave { coef: 1, period: 40, shift: 0 };

/// `offset + sum / 20`: the price wave spans 20 on the price scale.
fn price(offset: f64, terms: Vec<Int>) -> Shape {
    Shape::Waves { offset, scale: 1.0, terms, div: 20 }
}

fn integers(terms: Vec<Int>) -> Shape {
    Shape::Waves { offset: 0.0, scale: 1.0, terms, div: 1 }
}

/// The series of the input named `input`, or `None` for a shape no example knows.
///
/// `high` and `low` sit 2 to 3.8 either side of the price wave, the margin a 13-bar
/// wave, or a range-based function prints a constant. `open` is the previous bar's
/// midpoint, at most 1.95 from this one. `close` adds a 9-bar wave that keeps it within
/// 1.6 of the midpoint and never on it: a close on the midpoint leaves the money-flow
/// multiplier identically zero and the AD/ADOSC/CMF examples unable to fail (#136).
/// `volume` rises overall but falls on some bars, or NVI never takes its down-volume
/// branch.
/// An input declaring a synthetic `series` gets it over its `domain`: the price wave
/// mapped strictly inside it, so a function undefined at an end never meets that end,
/// or a ramp from `lo` to `hi`.
pub fn series(func: &FuncDef, input: &str) -> Option<Series> {
    const DATA: &str = "your data, oldest first";
    let decl = func.inputs.iter().find(|i| i.name == input);
    if let Some((kind, (lo, hi))) = decl.and_then(|i| i.domain.map(|d| (i.series, d))) {
        let var = match input {
            "inReal0" => "series0",
            "inReal1" => "series1",
            _ => "series",
        };
        let shape = match kind {
            ir::Series::Ramp => Shape::Ramp { lo, hi },
            ir::Series::Sine | ir::Series::Price => {
                // (PRICE - 200) / 201 stays within 200/201 of 0.
                let (mid, half) = (f64::midpoint(lo, hi), (hi - lo) / 2.0);
                if doc_meta::is_integral(half) {
                    #[allow(clippy::cast_possible_truncation)]
                    let h = half as i64;
                    let terms = vec![Int::Wave { coef: h, period: 40, shift: 0 }, Int::Const(-200 * h)];
                    Shape::Waves { offset: mid, scale: 1.0, terms, div: 201 }
                } else {
                    Shape::Waves { offset: mid, scale: half, terms: vec![PRICE, Int::Const(-200)], div: 201 }
                }
            }
        };
        return Some(Series { var, comment: DATA, shape });
    }
    let (var, shape) = match input {
        "inOpen" => ("open", price(90.0, vec![Int::Wave { coef: 1, period: 40, shift: 39 }])),
        "inHigh" => ("high", price(92.0, vec![PRICE, Int::Wave { coef: 1, period: 13, shift: 0 }])),
        "inLow" => ("low", price(88.0, vec![PRICE, Int::Wave { coef: -1, period: 13, shift: 0 }])),
        "inClose" => ("close", price(88.4, vec![PRICE, Int::Wave { coef: 4, period: 9, shift: 0 }])),
        "inVolume" => (
            "volume",
            integers(vec![Int::Const(10000), Int::Linear(100), Int::Wave { coef: 30, period: 20, shift: 0 }]),
        ),
        "inPeriods" => {
            return Some(Series {
                var: "periods",
                comment: "one period per bar",
                shape: integers(vec![Int::Const(5), Int::Mod(10)]),
            })
        }
        "inReal" => ("series", price(90.0, vec![PRICE])),
        "inReal0" => ("series0", price(90.0, vec![PRICE])),
        "inReal1" => ("series1", price(90.0, vec![Int::Wave { coef: 1, period: 40, shift: 7 }])),
        _ => return None,
    };
    Some(Series { var, comment: DATA, shape })
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
    /// An integer expression as a real, or `None` where arithmetic with a real widens it.
    pub widen: Option<fn(&str) -> String>,
}

/// `k` of a [`Int::Wave`], parenthesised.
fn wave_k(period: i64, shift: i64) -> String {
    let at = if shift == 0 { "i".to_string() } else { format!("(i + {shift})") };
    format!("({at} % {period} - {})", period / 2)
}

/// The integer sum of `terms`, spelled the same in C, Java, C#, TypeScript and Rust.
fn int_sum(terms: &[Int]) -> String {
    let mut out = String::new();
    for term in terms {
        let (negative, text) = match *term {
            Int::Const(c) => (c < 0, c.abs().to_string()),
            Int::Linear(1) => (false, "i".to_string()),
            Int::Linear(c) => (false, format!("{c} * i")),
            Int::Mod(m) => (false, format!("i % {m}")),
            Int::Wave { coef, period, shift } => {
                let k = wave_k(period, shift);
                let c = coef.abs();
                (coef < 0, if c == 1 { format!("{k} * {k}") } else { format!("{c} * {k} * {k}") })
            }
        };
        match (out.is_empty(), negative) {
            (true, true) => out.push('-'),
            (true, false) => {}
            (false, true) => out.push_str(" - "),
            (false, false) => out.push_str(" + "),
        }
        out.push_str(&text);
    }
    out
}

/// The element expression of `series` in one language.
#[allow(clippy::float_cmp)] // the scales are literals, not measurements
pub fn render(series: &Series, syn: &Syntax) -> String {
    match &series.shape {
        // The divisor is spelled `251.0` in every language: `i` is an integer in C,
        // Java and C#, and `span * i / 251` would divide as one.
        Shape::Ramp { lo, hi } => {
            let ramp = format!("{} * {} / {}.0", (syn.real)(hi - lo), syn.bar, LEN - 1);
            if *lo == 0.0 { ramp } else { format!("{} + {ramp}", (syn.real)(*lo)) }
        }
        Shape::Waves { offset, scale, terms, div } => {
            let sum = int_sum(terms);
            let mut value = if *scale == 1.0 && *div == 1 {
                syn.widen.map_or_else(|| sum.clone(), |w| w(&sum))
            } else {
                let operand = match syn.widen {
                    Some(w) => w(&sum),
                    None if terms.len() > 1 || *scale != 1.0 => format!("({sum})"),
                    None => sum,
                };
                let scaled = if *scale == 1.0 { operand } else { format!("{} * {operand}", (syn.real)(*scale)) };
                if *div == 1 { scaled } else { format!("{scaled} / {div}.0") }
            };
            if *offset != 0.0 {
                value = format!("{} + {value}", (syn.real)(*offset));
            }
            value
        }
    }
}

/// C, Java, C# and TypeScript: one spelling, `i` an `int` in the first three.
pub const C_FAMILY: Syntax<'static> = Syntax { real: doc_meta::fmt_real, bar: "i", widen: None };

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

/// Which elements of each output a sample prints: the first and the last, `...`
/// between them once there are more than two.
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum Ends {
    One,
    Two,
    Many,
}

impl Ends {
    /// From the count `bless-go-examples` recorded, which is every language's. An
    /// unrecorded function prints `Many`; each compiled copy asserts [`Self::claim`].
    #[must_use]
    pub fn of(func: &FuncDef) -> Self {
        match super::go_examples::recorded_count(&func.name) {
            Some(1) => Self::One,
            Some(2) => Self::Two,
            _ => Self::Many,
        }
    }

    /// `[first, ..., last]`, `[first, last]` or `[first]`.
    #[must_use]
    pub fn list(self, first: &str, last: &str) -> String {
        match self {
            Self::One => format!("[{first}]"),
            Self::Two => format!("[{first}, {last}]"),
            Self::Many => format!("[{first}, ..., {last}]"),
        }
    }

    /// The values [`Self::list`] prints, in order.
    #[must_use]
    pub fn args<T>(self, first: T, last: T) -> Vec<T> {
        if self == Self::One { vec![first] } else { vec![first, last] }
    }

    /// What the count must satisfy for this form, written after the count.
    #[must_use]
    pub fn claim(self) -> &'static str {
        match self {
            Self::One => "== 1",
            Self::Two => "== 2",
            Self::Many => "> 2",
        }
    }
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

    use super::super::rust_doc::RUST_SYNTAX;

    /// What a rendering computes, term by term in the same order.
    #[allow(clippy::cast_precision_loss)]
    fn eval(s: &Series, i: i64) -> f64 {
        match &s.shape {
            Shape::Ramp { lo, hi } => lo + (hi - lo) * i as f64 / 251.0,
            Shape::Waves { offset, scale, terms, div } => {
                let sum: i64 = terms
                    .iter()
                    .map(|t| match *t {
                        Int::Const(c) => c,
                        Int::Linear(c) => c * i,
                        Int::Mod(m) => i % m,
                        Int::Wave { coef, period, shift } => {
                            let k = (i + shift) % period - period / 2;
                            coef * k * k
                        }
                    })
                    .sum();
                offset + scale * sum as f64 / *div as f64
            }
        }
    }

    fn values(s: &Series) -> Vec<f64> {
        (0..i64::try_from(LEN).unwrap()).map(|i| eval(s, i)).collect()
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
            render(&close, &RUST_SYNTAX),
            "88.4 + ((i % 40 - 20) * (i % 40 - 20) + 4 * (i % 9 - 4) * (i % 9 - 4)) as f64 / 20.0"
        );
        assert_eq!(
            render(&close, &C_FAMILY),
            "88.4 + ((i % 40 - 20) * (i % 40 - 20) + 4 * (i % 9 - 4) * (i % 9 - 4)) / 20.0"
        );
        let open = series(&fixture("inOpen", None), "inOpen").unwrap();
        assert_eq!(render(&open, &C_FAMILY), "90 + ((i + 39) % 40 - 20) * ((i + 39) % 40 - 20) / 20.0");
        let low = series(&fixture("inLow", None), "inLow").unwrap();
        assert_eq!(
            render(&low, &C_FAMILY),
            "88 + ((i % 40 - 20) * (i % 40 - 20) - (i % 13 - 6) * (i % 13 - 6)) / 20.0"
        );
        let volume = series(&fixture("inVolume", None), "inVolume").unwrap();
        assert_eq!(render(&volume, &C_FAMILY), "10000 + 100 * i + 30 * (i % 20 - 10) * (i % 20 - 10)");
        assert_eq!(render(&volume, &RUST_SYNTAX), "(10000 + 100 * i + 30 * (i % 20 - 10) * (i % 20 - 10)) as f64");
        let periods = series(&fixture("inPeriods", None), "inPeriods").unwrap();
        assert_eq!(render(&periods, &RUST_SYNTAX), "(5 + i % 10) as f64");
        assert_eq!(render(&periods, &C_FAMILY), "5 + i % 10");
    }

    /// Every bar a valid OHLCV bar, with a close off the midpoint, a range that is not
    /// constant and a falling volume.
    #[test]
    #[allow(clippy::float_cmp)]
    fn the_price_series_are_valid_bars() {
        let of = |input: &str| values(&series(&fixture(input, None), input).unwrap());
        let (open, high, low, close, volume) = (of("inOpen"), of("inHigh"), of("inLow"), of("inClose"), of("inVolume"));
        let range: Vec<f64> = (0..LEN).map(|i| high[i] - low[i]).collect();
        assert!(range.iter().any(|&r| r != range[0]), "the bar range is constant");
        for i in 0..LEN {
            for (what, v) in [("open", open[i]), ("close", close[i])] {
                assert!(low[i] < v && v < high[i], "bar {i}: {what} {v} outside [{}, {}]", low[i], high[i]);
            }
            assert!(close[i] != f64::midpoint(low[i], high[i]), "bar {i}: close on the midpoint");
            assert!(volume[i] > 0.0);
        }
        assert!(volume.windows(2).any(|w| w[1] < w[0]), "volume never falls");
        assert!(volume[LEN - 1] > volume[0], "volume does not rise overall");
    }

    #[test]
    fn a_domain_maps_the_wave_strictly_inside() {
        let unit = series(&fixture("inReal", Some((-1.0, 1.0))), "inReal").unwrap();
        assert_eq!(unit.var, "series");
        assert_eq!(render(&unit, &RUST_SYNTAX), "((i % 40 - 20) * (i % 40 - 20) - 200) as f64 / 201.0");
        let wide = series(&fixture("inReal", Some((-3.0, 3.0))), "inReal").unwrap();
        assert_eq!(render(&wide, &C_FAMILY), "(3 * (i % 40 - 20) * (i % 40 - 20) - 600) / 201.0");
        let offset = series(&fixture("inReal", Some((0.0, 5.0))), "inReal").unwrap();
        assert_eq!(render(&offset, &C_FAMILY), "2.5 + 2.5 * ((i % 40 - 20) * (i % 40 - 20) - 200) / 201.0");
        assert_eq!(render(&offset, &RUST_SYNTAX), "2.5 + 2.5 * ((i % 40 - 20) * (i % 40 - 20) - 200) as f64 / 201.0");
        for (s, (lo, hi)) in [(&unit, (-1.0, 1.0)), (&wide, (-3.0, 3.0)), (&offset, (0.0, 5.0))] {
            let v = values(s);
            assert!(v.iter().all(|&x| lo < x && x < hi), "{v:?} leaves ({lo}, {hi})");
        }
        assert!(series(&fixture("inNonesuch", None), "inNonesuch").is_none());
    }

    #[test]
    fn a_ramp_runs_from_lo_to_hi_with_a_real_divisor() {
        let unit = series(&fixture_series("inReal", ir::Series::Ramp, Some((-1.0, 1.0))), "inReal").unwrap();
        assert_eq!(unit.var, "series");
        assert_eq!(render(&unit, &RUST_SYNTAX), "-1.0 + 2.0 * i as f64 / 251.0");
        assert_eq!(render(&unit, &C_FAMILY), "-1 + 2 * i / 251.0");
        let from_zero = series(&fixture_series("inReal", ir::Series::Ramp, Some((0.0, 100.0))), "inReal").unwrap();
        assert_eq!(render(&from_zero, &C_FAMILY), "100 * i / 251.0");
        let second = series(&fixture_series("inReal1", ir::Series::Ramp, Some((95.0, 105.0))), "inReal1").unwrap();
        assert_eq!(second.var, "series1");
        assert_eq!(render(&second, &C_FAMILY), "95 + 10 * i / 251.0");
        let first = series(&fixture_series("inReal0", ir::Series::Sine, Some((90.0, 110.0))), "inReal0").unwrap();
        assert_eq!(first.var, "series0");
        assert_eq!(render(&first, &C_FAMILY), "100 + (10 * (i % 40 - 20) * (i % 40 - 20) - 2000) / 201.0");
    }

    #[test]
    fn the_printed_list_drops_the_ellipsis_for_one_or_two_values() {
        assert_eq!(Ends::Many.list("a", "b"), "[a, ..., b]");
        assert_eq!(Ends::Two.list("a", "b"), "[a, b]");
        assert_eq!(Ends::One.list("a", "b"), "[a]");
        assert_eq!(Ends::One.args(0, 9), [0]);
        assert_eq!(Ends::Many.args(0, 9), [0, 9]);
    }

    #[test]
    fn wrap_call_breaks_before_the_outputs() {
        let a = vec!["0".to_string(), "n - 1".to_string()];
        let b = vec!["out".to_string()];
        assert_eq!(wrap_call("f(", &a, &b, ");", 80), vec!["f(0, n - 1, out);"]);
        assert_eq!(wrap_call("f(", &a, &b, ");", 10), vec!["f(0, n - 1,", "  out);"]);
    }
}
