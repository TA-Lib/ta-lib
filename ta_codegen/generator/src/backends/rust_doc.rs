//! Rustdoc renderer for the Rust backend.
//!
//! Renders each function's canonical documentation (`ta_codegen/input/<name>/<name>.md`,
//! parsed into [`DocDef`]) as idiomatic rustdoc on the generated `Core` methods:
//! summary, plain-text formula, notes, `# Arguments` with ranges/defaults injected
//! from the YAML metadata, `# Errors` / `# Panics`, a runnable `# Examples` doctest,
//! `# See also` intra-doc links, references, a ta-lib.org deep link, and
//! `#[doc(alias)]` attributes for docs.rs search.
//!
//! Prose is escaped for rustdoc's markdown: `[` and `<` outside code spans would
//! otherwise be parsed as intra-doc links / HTML tags (the canonical docs are full
//! of `inReal[i]` and `close<open`), and a wrapped line must not start with a list
//! or blockquote marker.

use crate::ir::{DocDef, EnumDef, FuncDef, OptInput, Output, ParamType};
use crate::registry::Registry;
use std::collections::HashMap;

/// Content width for wrapped doc lines: rustfmt max_width 100 minus `    /// `.
const WRAP: usize = 92;

// ---------------------------------------------------------------------------
// Public entry points
// ---------------------------------------------------------------------------

/// Full rustdoc block (+ `#[doc(alias)]` attributes) for the guarded public function.
#[allow(clippy::implicit_hasher)]
pub fn guarded_docs(
    func: &FuncDef,
    snake: &str,
    enums: &HashMap<String, EnumDef>,
    registry: &Registry,
) -> String {
    let empty = DocDef::default();
    let doc = func.doc.as_ref().unwrap_or(&empty);
    let mut d = DocWriter::new("    ");

    d.paragraph(&summary_text(func, doc));

    if let Some(formula) = &doc.formula {
        d.blank();
        d.paragraph("# Formula");
        d.blank();
        d.fenced_text(formula);
    }

    if !doc.notes.is_empty() {
        d.blank();
        d.paragraph("# Notes");
        d.blank();
        for note in &doc.notes {
            d.bullet(&escape_prose(note));
        }
    }

    d.blank();
    d.paragraph("# Arguments");
    d.blank();
    d.bullet("`startIdx` — Start index of the requested calculation range.");
    d.bullet("`endIdx` — End index of the requested calculation range (inclusive).");
    for input in &func.inputs {
        d.bullet(&format!(
            "`{}` — {}",
            input.name,
            input_desc(&input.name, doc)
        ));
    }
    for opt in &func.optional_inputs {
        d.bullet(&param_doc(opt, doc, enums));
    }
    d.bullet("`outBegIdx` — Set to the input index of the first output value.");
    d.bullet("`outNBElement` — Set to the number of output values written.");
    for output in &func.outputs {
        d.bullet(&format!(
            "`{}` — {}",
            output.name,
            output_desc(&output.name, doc)
        ));
    }
    if func
        .optional_inputs
        .iter()
        .any(|o| matches!(o.param_type, ParamType::Integer))
    {
        d.blank();
        d.paragraph("Integer parameters accept `i32::MIN` to select their default value.");
    }

    d.blank();
    d.paragraph("# Errors");
    d.blank();
    if func.optional_inputs.is_empty() {
        d.paragraph("Returns [`RetCode::OutOfRangeStartIndex`] when `endIdx < startIdx`.");
    } else {
        d.paragraph(
            "Returns [`RetCode::OutOfRangeStartIndex`] when `endIdx < startIdx`, and \
             [`RetCode::BadParam`] when an optional parameter is outside its documented range.",
        );
    }

    d.blank();
    d.paragraph("# Panics");
    d.blank();
    d.paragraph(
        "Input slices must cover `startIdx..=endIdx` and output slices must hold the \
         number of values produced for that range; an undersized slice panics. Sizing \
         every output slice to the input length is always sufficient.",
    );

    if let Some(example) = example_doctest(func, snake) {
        d.blank();
        d.paragraph("# Examples");
        d.blank();
        d.raw_lines(&example);
    }

    if !doc.see_also.is_empty() {
        d.blank();
        d.paragraph("# See also");
        d.blank();
        let links: Vec<String> = doc
            .see_also
            .iter()
            .map(|n| {
                if registry.contains(&n.to_lowercase()) {
                    format!("[`Core::{}`]", n.to_lowercase())
                } else {
                    escape_prose(n)
                }
            })
            .collect();
        d.paragraph(&links.join(" · "));
    }

    if !doc.references.is_empty() {
        d.blank();
        d.paragraph("# References");
        d.blank();
        for r in &doc.references {
            d.bullet(&escape_prose(r));
        }
    }

    d.blank();
    d.paragraph(&format!(
        "Further reading: [ta-lib.org/functions/{snake}](https://ta-lib.org/functions/{snake}/)"
    ));

    let mut out = d.finish();
    for alias in doc_aliases(func, doc) {
        out.push_str(&format!("    #[doc(alias = \"{alias}\")]\n"));
    }
    out
}

/// Rustdoc block for the `<snake>_lookback` function.
#[allow(clippy::implicit_hasher)]
pub fn lookback_docs(func: &FuncDef, snake: &str, enums: &HashMap<String, EnumDef>) -> String {
    let empty = DocDef::default();
    let doc = func.doc.as_ref().unwrap_or(&empty);
    let mut d = DocWriter::new("    ");

    d.paragraph(&format!(
        "Lookback period for [`Core::{snake}`]: the number of leading input values \
         consumed before the first output value can be produced."
    ));

    if !func.optional_inputs.is_empty() {
        d.blank();
        d.paragraph("# Arguments");
        d.blank();
        for opt in &func.optional_inputs {
            d.bullet(&param_doc(opt, doc, enums));
        }
        d.blank();
        d.paragraph(
            "Returns `usize::MAX` when a parameter is out of range. Integer parameters \
             accept `i32::MIN` to select their default value.",
        );
    }

    d.finish()
}

/// Rustdoc block for the `_unguarded` / `_private` variants.
pub fn unguarded_docs(func: &FuncDef, snake: &str, is_private: bool) -> String {
    let mut d = DocWriter::new("    ");

    if is_private {
        let params: Vec<String> = func
            .private_extra_params
            .iter()
            .map(|(name, _)| format!("`{name}`"))
            .collect();
        d.paragraph(&format!(
            "Internal variant of [`Core::{snake}_unguarded`] taking the precomputed \
             parameter{} {}. Same contract as [`Core::{snake}_unguarded`].",
            if params.len() == 1 { "" } else { "s" },
            params.join(", ")
        ));
    } else {
        d.paragraph(&format!(
            "Unguarded variant of [`Core::{snake}`], used for internal cross-indicator calls."
        ));
        d.blank();
        d.paragraph(&format!(
            "Skips parameter validation; indexing stays safe. Every argument must satisfy \
             the constraints documented on [`Core::{snake}`]; an out-of-range parameter, an \
             input slice not covering `startIdx..=endIdx`, or an undersized output slice \
             panics (never undefined behavior). Prefer [`Core::{snake}`]."
        ));
    }

    d.finish()
}

// ---------------------------------------------------------------------------
// Section builders
// ---------------------------------------------------------------------------

/// First doc paragraph: the canonical summary, falling back to YAML hint/group.
fn summary_text(func: &FuncDef, doc: &DocDef) -> String {
    if doc.summary.is_empty() {
        let title = func
            .description
            .as_deref()
            .or(func.hint.as_deref())
            .unwrap_or(&func.group);
        escape_prose(title)
    } else {
        escape_prose(&doc.summary)
    }
}

/// Description for an input parameter. Price bundles (`inPriceOHLC`) are expanded
/// to per-component slices in the signature, so components get standard texts.
fn input_desc(name: &str, doc: &DocDef) -> String {
    if let Some((_, desc)) = doc.inputs.iter().find(|(n, _)| n == name) {
        return ensure_period(&escape_prose(desc));
    }
    let fixed = match name {
        "inOpen" => "Open prices per bar.",
        "inHigh" => "High prices per bar.",
        "inLow" => "Low prices per bar.",
        "inClose" => "Close prices per bar.",
        "inVolume" => "Volume per bar.",
        _ => "Input data series.",
    };
    fixed.to_string()
}

/// Description for an output parameter.
fn output_desc(name: &str, doc: &DocDef) -> String {
    doc.outputs.iter().find(|(n, _)| n == name).map_or_else(
        || "Output values.".to_string(),
        |(_, d)| ensure_period(&escape_prose(d)),
    )
}

/// One `# Arguments` bullet for an optional parameter: canonical prose plus the
/// default/range injected from YAML (numbers live only in the YAML — golden rule).
fn param_doc(opt: &OptInput, doc: &DocDef, enums: &HashMap<String, EnumDef>) -> String {
    let desc = doc
        .params
        .iter()
        .find(|(n, _)| n == &opt.name)
        .map(|(_, d)| escape_prose(d))
        .or_else(|| opt.hint.clone())
        .or_else(|| opt.display_name.clone())
        .unwrap_or_else(|| "Optional parameter".to_string());

    let mut meta: Vec<String> = Vec::new();
    match &opt.param_type {
        ParamType::Enum(enum_name) => {
            if let Some(def) = enums.get(enum_name) {
                #[allow(clippy::cast_possible_truncation)]
                let default = opt.default.unwrap_or(0.0) as i32;
                if let Some(v) = def.variants.iter().find(|v| v.value == default) {
                    meta.push(format!("default {} = {}", v.value, v.short_name));
                }
                let values: Vec<String> = def
                    .variants
                    .iter()
                    .map(|v| format!("{}={}", v.value, v.short_name))
                    .collect();
                meta.push(format!("values: {}", values.join(", ")));
            }
        }
        ParamType::Integer => {
            if let Some(default) = opt.default {
                #[allow(clippy::cast_possible_truncation)]
                meta.push(format!("default {}", default as i64));
            }
            if let Some((lo, hi)) = opt.range {
                #[allow(clippy::cast_possible_truncation)]
                meta.push(format!("range {}..={}", lo as i64, hi as i64));
            }
        }
        _ => {
            if let Some(default) = opt.default {
                meta.push(format!("default {}", fmt_real(default)));
            }
            if let Some((lo, hi)) = opt.range {
                let lo_bounded = lo.abs() < 1e15;
                let hi_bounded = hi.abs() < 1e15;
                match (lo_bounded, hi_bounded) {
                    (true, true) => meta.push(format!("range {}..={}", fmt_real(lo), fmt_real(hi))),
                    (true, false) => meta.push(format!("minimum {}", fmt_real(lo))),
                    (false, true) => meta.push(format!("maximum {}", fmt_real(hi))),
                    (false, false) => {}
                }
            }
        }
    }

    if meta.is_empty() {
        format!("`{}` — {}", opt.name, desc)
    } else {
        format!("`{}` — {} ({})", opt.name, desc, meta.join(", "))
    }
}

/// `#[doc(alias)]` values from the canonical `## Aliases`: whitespace/punctuation
/// removed (rustdoc forbids whitespace in aliases), deduplicated, and dropped when
/// the alias collapses to the function name itself.
fn doc_aliases(func: &FuncDef, doc: &DocDef) -> Vec<String> {
    let name_l = func.name.to_lowercase().replace('_', "");
    let mut out: Vec<String> = Vec::new();
    for alias in &doc.aliases {
        let cleaned: String = alias
            .chars()
            .filter(|c| c.is_ascii_alphanumeric() || *c == '_' || *c == '-' || *c == '.')
            .collect();
        if cleaned.is_empty() || cleaned.to_lowercase().replace('_', "") == name_l {
            continue;
        }
        if !out.iter().any(|a| a.eq_ignore_ascii_case(&cleaned)) {
            out.push(cleaned);
        }
    }
    out
}

// ---------------------------------------------------------------------------
// Example doctest generation
// ---------------------------------------------------------------------------

/// Number of bars in every example input series: one trading year, comfortably
/// larger than the largest default lookback (~64 for the Hilbert Transform family).
const EXAMPLE_LEN: usize = 252;

/// Build a runnable `# Examples` doctest that calls the guarded function on
/// deterministic synthetic data with every optional parameter at its default,
/// and asserts success. Returned lines are raw markdown (no `///` prefix).
fn example_doctest(func: &FuncDef, snake: &str) -> Option<Vec<String>> {
    let mut lines: Vec<String> = Vec::new();
    lines.push("```".to_string());
    lines.push("use ta_lib::{Core, RetCode};".to_string());
    lines.push(String::new());

    // Input series, in signature order. `open` stays within [low, high] because
    // its phase shift keeps |open - close| < 1.0.
    let mut first_series: Option<String> = None;
    let mut args: Vec<String> = Vec::new();
    for input in &func.inputs {
        let (var, def) = match input.name.as_str() {
            "inOpen" => (
                "open",
                series_def("open", "100.0 + 10.0 * (0.1 * i as f64 - 0.05).sin()"),
            ),
            "inHigh" => (
                "high",
                series_def("high", "101.0 + 10.0 * (0.1 * i as f64).sin()"),
            ),
            "inLow" => (
                "low",
                series_def("low", "99.0 + 10.0 * (0.1 * i as f64).sin()"),
            ),
            "inClose" => (
                "close",
                series_def("close", "100.0 + 10.0 * (0.1 * i as f64).sin()"),
            ),
            "inVolume" => (
                "volume",
                series_def("volume", "10_000.0 + 100.0 * i as f64"),
            ),
            "inPeriods" => (
                "periods",
                format!("let periods = vec![14.0; {EXAMPLE_LEN}];"),
            ),
            "inReal" => (
                "data",
                series_def("data", "100.0 + 10.0 * (0.1 * i as f64).sin()"),
            ),
            "inReal0" => (
                "data0",
                series_def("data0", "100.0 + 10.0 * (0.1 * i as f64).sin()"),
            ),
            "inReal1" => (
                "data1",
                series_def("data1", "100.0 + 10.0 * (0.1 * i as f64 + 0.7).sin()"),
            ),
            _ => return None, // unknown input shape: skip the example
        };
        lines.push(def);
        if first_series.is_none() {
            first_series = Some(var.to_string());
        }
        args.push(format!("&{var}"));
    }
    let first = first_series?;

    // Optional parameters at their documented defaults.
    for opt in &func.optional_inputs {
        let default = opt.default.unwrap_or(0.0);
        #[allow(clippy::cast_possible_truncation)]
        let literal = match opt.param_type {
            ParamType::Real => fmt_real_literal(default),
            _ => format!("{}", default as i64),
        };
        args.push(literal);
    }

    lines.push(String::new());
    lines.push("let core = Core::new();".to_string());
    lines.push("let mut out_beg = 0;".to_string());
    lines.push("let mut out_nb = 0;".to_string());

    let mut out_args: Vec<String> = vec!["&mut out_beg".to_string(), "&mut out_nb".to_string()];
    for output in &func.outputs {
        let var = output_var_name(output);
        let zero = match output.param_type {
            ParamType::Integer => "0i32",
            _ => "0.0",
        };
        lines.push(format!("let mut {var} = vec![{zero}; {EXAMPLE_LEN}];"));
        out_args.push(format!("&mut {var}"));
    }

    lines.push(String::new());
    let range = format!("0, {first}.len() - 1");
    let call_one_line = format!(
        "let ret = core.{snake}({range}, {}, {});",
        args.join(", "),
        out_args.join(", ")
    );
    if call_one_line.len() <= WRAP {
        lines.push(call_one_line);
    } else {
        lines.push(format!("let ret = core.{snake}("));
        lines.push(format!("    {range}, {},", args.join(", ")));
        lines.push(format!("    {},", out_args.join(", ")));
        lines.push(");".to_string());
    }
    lines.push("assert_eq!(ret, RetCode::Success);".to_string());
    lines.push("assert!(out_nb > 0);".to_string());
    lines.push("```".to_string());
    Some(lines)
}

/// `let <name>: Vec<f64> = (0..252).map(|i| <expr>).collect();`
fn series_def(name: &str, expr: &str) -> String {
    format!("let {name}: Vec<f64> = (0..{EXAMPLE_LEN}).map(|i| {expr}).collect();")
}

/// Example variable name for an output: `outRealUpperBand` → `upper_band`,
/// `outMACDSignal` → `macd_signal`, bare `outReal`/`outInteger` → `out`.
fn output_var_name(output: &Output) -> String {
    let stripped = output.name.strip_prefix("out").unwrap_or(&output.name);
    let snake = camel_to_snake(stripped);
    let trimmed = snake
        .strip_prefix("real_")
        .or_else(|| snake.strip_prefix("integer_"))
        .unwrap_or(&snake);
    match trimmed {
        "" | "real" | "integer" => "out".to_string(),
        other => other.to_string(),
    }
}

/// Acronym-aware CamelCase → snake_case (`MACDSignal` → `macd_signal`).
fn camel_to_snake(s: &str) -> String {
    let chars: Vec<char> = s.chars().collect();
    let mut out = String::new();
    for (i, &c) in chars.iter().enumerate() {
        if c.is_uppercase()
            && i > 0
            && (chars[i - 1].is_lowercase()
                || (chars[i - 1].is_uppercase()
                    && chars.get(i + 1).is_some_and(|n| n.is_lowercase())))
        {
            out.push('_');
        }
        out.push(c.to_ascii_lowercase());
    }
    out
}

// ---------------------------------------------------------------------------
// Formatting / escaping helpers
// ---------------------------------------------------------------------------

/// True when `v` is an exactly-representable integer we can print without a fraction.
fn is_integral(v: f64) -> bool {
    (v - v.trunc()).abs() < f64::EPSILON && v.abs() < 1e15
}

/// Format an f64 for prose (`2` not `2.0`, `0.02` untouched).
fn fmt_real(v: f64) -> String {
    if is_integral(v) {
        format!("{v:.0}")
    } else {
        format!("{v}")
    }
}

/// Format an f64 as a Rust literal (`2.0`, `0.02`).
fn fmt_real_literal(v: f64) -> String {
    if is_integral(v) {
        format!("{v:.1}")
    } else {
        format!("{v}")
    }
}

fn ensure_period(s: &str) -> String {
    let t = s.trim_end();
    if t.is_empty() || t.ends_with(['.', '!', '?', ')']) {
        t.to_string()
    } else {
        format!("{t}.")
    }
}

/// Escape canonical prose for rustdoc markdown: outside code spans, `[` would
/// start an intra-doc link and `<` an (unclosed) HTML tag — both draw rustdoc
/// lints on text like `inReal[i]` or `close<open`. Inside backtick code spans,
/// escapes would render literally, so leave those intact.
fn escape_prose(text: &str) -> String {
    let mut out = String::with_capacity(text.len() + 8);
    let mut in_code = false;
    for c in text.chars() {
        match c {
            '`' => {
                in_code = !in_code;
                out.push(c);
            }
            '[' | '<' if !in_code => {
                out.push('\\');
                out.push(c);
            }
            '\n' => out.push(' '), // reflow: paragraphs re-wrap on emit
            _ => out.push(c),
        }
    }
    out
}

/// A wrapped-`///` doc-comment writer.
struct DocWriter {
    indent: &'static str,
    out: String,
}

impl DocWriter {
    fn new(indent: &'static str) -> Self {
        DocWriter {
            indent,
            out: String::new(),
        }
    }

    /// Emit a paragraph, word-wrapped to [`WRAP`] columns.
    fn paragraph(&mut self, text: &str) {
        for line in wrap_text(text, WRAP, 0) {
            self.push_line(&line);
        }
    }

    /// Emit one markdown list item, with continuation lines indented.
    fn bullet(&mut self, text: &str) {
        let full = format!("* {text}");
        for (i, line) in wrap_text(&full, WRAP, 2).into_iter().enumerate() {
            if i == 0 {
                self.push_line(&line);
            } else {
                self.push_line(&format!("  {line}"));
            }
        }
    }

    /// Emit a ```text fenced block, lines verbatim (no wrapping, no escaping).
    fn fenced_text(&mut self, body: &str) {
        self.push_line_raw("```text");
        for line in body.lines() {
            self.push_line_raw(line);
        }
        self.push_line_raw("```");
    }

    /// Emit pre-built raw markdown lines verbatim (e.g. a doctest).
    fn raw_lines(&mut self, lines: &[String]) {
        for line in lines {
            self.push_line_raw(line);
        }
    }

    fn blank(&mut self) {
        self.out.push_str(self.indent);
        self.out.push_str("///\n");
    }

    /// Push one content line, escaping accidental markdown block markers at the
    /// start of the line (a wrapped `-DI` or `>70` must not become a list/quote).
    fn push_line(&mut self, line: &str) {
        let needs_escape =
            matches!(line.chars().next(), Some('-' | '+' | '>' | '#')) && !line.starts_with("# "); // our own section headings
        if needs_escape {
            self.out.push_str(self.indent);
            self.out.push_str("/// \\");
            self.out.push_str(line);
            self.out.push('\n');
        } else {
            self.push_line_raw(line);
        }
    }

    fn push_line_raw(&mut self, line: &str) {
        self.out.push_str(self.indent);
        if line.is_empty() {
            self.out.push_str("///\n");
        } else {
            self.out.push_str("/// ");
            self.out.push_str(line);
            self.out.push('\n');
        }
    }

    fn finish(self) -> String {
        self.out
    }
}

/// Greedy word wrap. `hang` reduces the width of continuation lines (they get
/// indented by the caller). Words longer than the width stay on their own line.
fn wrap_text(text: &str, width: usize, hang: usize) -> Vec<String> {
    let mut lines: Vec<String> = Vec::new();
    let mut current = String::new();
    let mut current_width = width;
    for word in text.split_whitespace() {
        if current.is_empty() {
            current = word.to_string();
        } else if current.len() + 1 + word.len() <= current_width {
            current.push(' ');
            current.push_str(word);
        } else {
            lines.push(std::mem::take(&mut current));
            current_width = width.saturating_sub(hang);
            current = word.to_string();
        }
    }
    if !current.is_empty() {
        lines.push(current);
    }
    lines
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn escapes_brackets_and_tags_outside_code() {
        assert_eq!(escape_prose("a[i] < b"), "a\\[i] \\< b");
        assert_eq!(escape_prose("`a[i] < b`"), "`a[i] < b`");
    }

    #[test]
    fn output_var_names() {
        let out = |name: &str, pt: ParamType| Output {
            name: name.to_string(),
            param_type: pt,
            flags: vec![],
        };
        assert_eq!(output_var_name(&out("outReal", ParamType::Real)), "out");
        assert_eq!(
            output_var_name(&out("outInteger", ParamType::Integer)),
            "out"
        );
        assert_eq!(
            output_var_name(&out("outRealUpperBand", ParamType::Real)),
            "upper_band"
        );
        assert_eq!(
            output_var_name(&out("outMACDSignal", ParamType::Real)),
            "macd_signal"
        );
        assert_eq!(
            output_var_name(&out("outMinIdx", ParamType::Integer)),
            "min_idx"
        );
    }

    #[test]
    fn camel_snake_acronyms() {
        assert_eq!(camel_to_snake("MACDSignal"), "macd_signal");
        assert_eq!(camel_to_snake("UpperBand"), "upper_band");
        assert_eq!(camel_to_snake("MAMA"), "mama");
        assert_eq!(camel_to_snake("SlowK"), "slow_k");
    }

    #[test]
    fn real_formatting() {
        assert_eq!(fmt_real(2.0), "2");
        assert_eq!(fmt_real(0.02), "0.02");
        assert_eq!(fmt_real_literal(2.0), "2.0");
        assert_eq!(fmt_real_literal(0.3), "0.3");
    }
}
