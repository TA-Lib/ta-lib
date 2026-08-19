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

use super::doc_meta::{self, ensure_period, RangeMeta};
use crate::ir::{DocDef, EnumDef, FuncDef, OptInput, Output, ParamType};
use crate::registry::Registry;
use std::collections::HashMap;

/// Content width for wrapped doc lines: rustfmt max_width 100 minus `    /// `.
const WRAP: usize = 92;

// ---------------------------------------------------------------------------
// Public entry points
// ---------------------------------------------------------------------------

/// Full rustdoc block (+ `#[doc(alias)]` attributes) for the guarded public function.
#[allow(clippy::implicit_hasher, clippy::too_many_lines)]
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
        if let Some(note) = &doc.formula_note {
            d.blank();
            d.paragraph(&escape_prose(note));
        }
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
    for output in &func.outputs {
        d.bullet(&format!(
            "`{}` — {}",
            output.name,
            output_desc(&output.name, doc)
        ));
    }
    if let Some(sentence) = default_sentinel_sentence(func) {
        d.blank();
        d.paragraph(sentence);
    }

    // Where the values landed is the return value, not a pair of out-params, so it
    // belongs here rather than in `# Arguments` above.
    d.blank();
    d.paragraph("# Returns");
    d.blank();
    d.paragraph(
        "On success, an [`OutRange`]: `beg_idx` is the index of the first value written, \
         in the input series' coordinates, and `count` is how many were written. A range \
         shorter than the lookback succeeds with `count == 0`.",
    );

    d.blank();
    d.paragraph("# Errors");
    d.blank();
    if func.optional_inputs.is_empty() {
        d.paragraph(
            "Returns [`Err`] carrying [`RetCode::OutOfRangeStartIndex`] when `startIdx` \
             exceeds [`Core::MAX_INDEX`], and [`RetCode::OutOfRangeEndIndex`] when `endIdx` \
             exceeds it or is below `startIdx`. A range shorter than the lookback is not an \
             error: it is [`Ok`] with a zero [`OutRange::count`].",
        );
    } else {
        d.paragraph(
            "Returns [`Err`] carrying [`RetCode::OutOfRangeStartIndex`] when `startIdx` \
             exceeds [`Core::MAX_INDEX`], [`RetCode::OutOfRangeEndIndex`] when `endIdx` exceeds \
             it or is below `startIdx`, and [`RetCode::BadParam`] when an optional parameter is \
             outside its documented range. A range shorter than the lookback is not an error: \
             it is [`Ok`] with a zero [`OutRange::count`].",
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

    if let Some(example) = example_doctest(func, snake, enums) {
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
                    format!("[`Core::{}`]", registry.name_of(&n.to_lowercase()))
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
    // The site builds flat files (`dist/functions/sma.html`), so the slug is the
    // lower-cased name and carries no trailing slash: `/functions/SMA` and
    // `/functions/sma/` both 404. Same rule as `docs_site::generate`, which is
    // what names the page.
    let slug = func.name.to_lowercase();
    d.paragraph(&format!(
        "Further reading: [ta-lib.org/functions/{slug}](https://ta-lib.org/functions/{slug})"
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
        let mut sentence = String::from("Returns `usize::MAX` when a parameter is out of range.");
        if let Some(extra) = default_sentinel_sentence(func) {
            sentence.push(' ');
            sentence.push_str(extra);
        }
        d.paragraph(&sentence);
    }

    d.finish()
}

/// How a caller asks for a parameter's default value, phrased for whichever kinds
/// the function actually takes. Gated per kind because the two sentinels differ and
/// several functions take only one kind: SAR and MAMA have no integer optional
/// parameter at all, so an unconditional `i32::MIN` sentence documents an API they
/// do not have. An `enum:` parameter is NOT an integer here: it is typed as its
/// enum, so the sentinel is unrepresentable and its `DEFAULT` member is the
/// spelling instead — the same split Java has always had (issue #162).
fn default_sentinel_sentence(func: &FuncDef) -> Option<&'static str> {
    let takes = |want: fn(&ParamType) -> bool| {
        func.optional_inputs
            .iter()
            .any(|o| want(&o.param_type) && o.default.is_some())
    };
    let has_int = takes(|t| matches!(t, ParamType::Integer));
    let has_real = takes(|t| matches!(t, ParamType::Real));
    match (has_int, has_real) {
        (true, true) => Some(
            "Integer parameters accept [`Core::INTEGER_DEFAULT`], and real parameters \
             [`Core::REAL_DEFAULT`], to select their default value.",
        ),
        (true, false) => {
            Some("Integer parameters accept [`Core::INTEGER_DEFAULT`] to select their default value.")
        }
        (false, true) => {
            Some("Real parameters accept [`Core::REAL_DEFAULT`] to select their default value.")
        }
        (false, false) => None,
    }
}

/// Rustdoc block for the `_private` variant.
pub fn private_docs(func: &FuncDef, snake: &str) -> String {
    let mut d = DocWriter::new("    ");
    let params: Vec<String> = func
        .private_extra_params
        .iter()
        .map(|(name, _)| format!("`{name}`"))
        .collect();
    d.paragraph(&format!(
        "Internal variant of [`Core::{snake}`] taking the precomputed parameter{} {}. \
         Skips the validation prologue: its only callers are the guarded bodies, which \
         have already validated.",
        if params.len() == 1 { "" } else { "s" },
        params.join(", ")
    ));
    d.blank();
    d.paragraph(&format!(
        "Unlike [`Core::{snake}`] the bounds assertions here are unconditional: an \
         `endIdx` beyond the input slice panics even when the lookback clamp means \
         no element would be read."
    ));
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
/// The facts come from [`doc_meta::param_meta`], shared with the website renderer so
/// the two surfaces cannot drift; only the phrasing below is rustdoc's own.
fn param_doc(opt: &OptInput, doc: &DocDef, enums: &HashMap<String, EnumDef>) -> String {
    let desc = doc
        .params
        .iter()
        .find(|(n, _)| n == &opt.name)
        .map(|(_, d)| escape_prose(d))
        .or_else(|| opt.hint.clone())
        .or_else(|| opt.display_name.clone())
        .unwrap_or_else(|| "Optional parameter".to_string());

    let m = doc_meta::param_meta(opt, enums);
    let mut meta: Vec<String> = Vec::new();
    if matches!(opt.param_type, ParamType::Enum(_)) {
        // An enum's admissible set is its variant list; the default names a variant.
        if let (Some(d), Some(variant)) = (m.default.as_ref(), m.default_variant.as_ref()) {
            meta.push(format!("default {d} = {variant}"));
        }
        if !m.values.is_empty() {
            let values: Vec<String> = m.values.iter().map(|(v, n)| format!("{v}={n}")).collect();
            meta.push(format!("values: {}", values.join(", ")));
        }
        // The member is this parameter's spelling of "use the default": the
        // integer sentinel the primitive parameters take cannot be represented
        // at a typed enum.
        if let ParamType::Enum(name) = &opt.param_type {
            if let Some(v) = super::common::enum_default_variant(enums, name) {
                meta.push(format!("`{name}::{}` selects the default", v.name));
            }
        }
    } else {
        if let Some(d) = &m.default {
            meta.push(format!("default {d}"));
        }
        match &m.range {
            RangeMeta::Bounded(lo, hi) => meta.push(format!("range {lo}..={hi}")),
            RangeMeta::Min(lo) => meta.push(format!("minimum {lo}")),
            RangeMeta::Max(hi) => meta.push(format!("maximum {hi}")),
            RangeMeta::Unbounded => {}
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

/// `close` carries a second harmonic so it is never at the exact midpoint of the
/// bar: a midpoint close makes the money-flow multiplier
/// `((close-low) - (high-close)) / (high-low)` identically zero, which left the
/// AD/ADOSC/CMF examples unable to fail (issue #136). The harmonic amplitude keeps
/// `|close - midpoint| < 1.0`, so `close` stays inside `[low, high]`.
pub(super) const CLOSE_SERIES: &str =
    "100.0 + 10.0 * (0.1 * i as f64).sin() + 0.8 * (0.7 * i as f64).sin()";

/// `volume` rises overall but falls on some bars: a monotonically increasing volume
/// never takes the down-volume branch (NVI stays at its 1000.0 seed for every bar).
pub(super) const VOLUME_SERIES: &str =
    "10_000.0 + 100.0 * i as f64 + 2_000.0 * (0.3 * i as f64).sin()";

/// Input series for the functions that need small inputs: on the default ~100.0
/// price series ACOS/ASIN are out of domain (every output `NaN`) and TANH saturates
/// to a constant 1.0.
pub(super) const UNIT_SERIES: &str = "(0.1 * i as f64).sin()";

/// Functions whose example needs an input in `[-1, 1]` to be meaningful.
pub(super) fn unit_domain(func: &FuncDef) -> bool {
    matches!(func.name.to_uppercase().as_str(), "ACOS" | "ASIN" | "TANH")
}

/// The example input series for one input, as `(variable name, source lines)`.
/// `open` and `close` both stay inside `[low, high]`: their offset from the bar
/// midpoint never reaches 1.0. Returns `None` for an unknown input shape, which
/// drops the example.
fn example_input(func: &FuncDef, input: &str) -> Option<(&'static str, Vec<String>)> {
    Some(match input {
        "inReal" if unit_domain(func) => ("data", series_def("data", UNIT_SERIES)),
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
        "inClose" => ("close", series_def("close", CLOSE_SERIES)),
        "inVolume" => ("volume", series_def("volume", VOLUME_SERIES)),
        "inPeriods" => (
            "periods",
            vec![format!("let periods = vec![14.0; {EXAMPLE_LEN}];")],
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
        _ => return None,
    })
}

/// Build a runnable `# Examples` doctest that calls the guarded function on
/// deterministic synthetic data with every optional parameter at its default,
/// and asserts success. Returned lines are raw markdown (no `///` prefix).
/// `use ta_lib::X;` or `use ta_lib::{A, B};` — braces only when they are needed,
/// so a function with no enum parameter keeps the single-item form.
pub(crate) fn example_use_line(items: &[String]) -> String {
    if items.len() == 1 {
        format!("use ta_lib::{};", items[0])
    } else {
        format!("use ta_lib::{{{}}};", items.join(", "))
    }
}

/// The literal for an optional parameter in a generated example.
///
/// An `enum:` parameter is named rather than numbered: the example is the first
/// thing a reader copies, and a bare `1` would not compile against the typed
/// parameter. Shared with the streaming examples so both spell it the same way.
#[allow(clippy::cast_possible_truncation)]
#[allow(clippy::float_cmp)] // an enum default is an exact integer, not a measurement
pub(crate) fn example_opt_literal(
    opt: &crate::ir::OptInput,
    enums: &HashMap<String, EnumDef>,
) -> String {
    let default = opt.default.unwrap_or(0.0);
    match &opt.param_type {
        ParamType::Real => fmt_real_literal(default),
        ParamType::Enum(name) => enums
            .get(name)
            .and_then(|e| e.variants.iter().find(|v| f64::from(v.value) == default))
            .map_or_else(
                || format!("{}", default as i64),
                |v| format!("{name}::{}", v.name),
            ),
        _ => format!("{}", default as i64),
    }
}

/// The enum types a function's optional parameters use, for an example's `use`.
pub(crate) fn example_enum_imports(func: &FuncDef) -> Vec<String> {
    let mut v: Vec<String> = func
        .optional_inputs
        .iter()
        .filter_map(|o| match &o.param_type {
            ParamType::Enum(n) => Some(n.clone()),
            _ => None,
        })
        .collect();
    v.sort();
    v.dedup();
    v
}

fn example_doctest(
    func: &FuncDef,
    snake: &str,
    enums: &HashMap<String, EnumDef>,
) -> Option<Vec<String>> {
    let mut lines: Vec<String> = Vec::new();
    lines.push("```".to_string());
    // `RetCode` is not imported: the call returns `Result<OutRange, RetCode>` and
    // the example propagates with `?`, naming the error type once, fully
    // qualified, on the hidden trailing line that lets `?` work in a doctest.
    let mut imports = vec!["Core".to_string()];
    imports.extend(example_enum_imports(func));
    lines.push(example_use_line(&imports));
    lines.push(String::new());

    let mut first_series: Option<String> = None;
    let mut args: Vec<String> = Vec::new();
    for input in &func.inputs {
        let (var, def) = example_input(func, &input.name)?;
        lines.extend(def);
        if first_series.is_none() {
            first_series = Some(var.to_string());
        }
        args.push(format!("&{var}"));
    }
    let first = first_series?;

    // Optional parameters at their documented defaults. An `enum:` parameter is
    // named, not numbered -- the example is the first thing a reader copies, and
    // a bare `1` there would not even compile now that the parameter is typed.
    for opt in &func.optional_inputs {
        args.push(example_opt_literal(opt, enums));
    }

    lines.push(String::new());
    lines.push("let core = Core::new();".to_string());

    let mut out_args: Vec<String> = Vec::new();
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
        "let out_range = core.{snake}({range}, {}, {})?;",
        args.join(", "),
        out_args.join(", ")
    );
    if call_one_line.len() <= WRAP {
        lines.push(call_one_line);
    } else {
        lines.push(format!("let out_range = core.{snake}("));
        lines.push(format!("    {range}, {},", args.join(", ")));
        lines.push(format!("    {},", out_args.join(", ")));
        lines.push(")?;".to_string());
    }
    lines.push("assert!(out_range.count > 0);".to_string());
    // Assert on the values, not just the return code: a successful call never
    // emits NaN or infinity.
    if let Some(output) = func
        .outputs
        .iter()
        .find(|o| o.param_type != ParamType::Integer)
    {
        let var = output_var_name(output);
        lines.push(format!(
            "assert!({var}[..out_range.count].iter().all(|v| v.is_finite()));"
        ));
    }
    // Lets the example above use `?`; hidden from the rendered docs.
    lines.push("# Ok::<(), ta_lib::RetCode>(())".to_string());
    lines.push("```".to_string());
    Some(lines)
}

/// `let <name>: Vec<f64> = (0..252).map(|i| <expr>).collect();`
pub(super) fn series_def(name: &str, expr: &str) -> Vec<String> {
    let one_line = format!("let {name}: Vec<f64> = (0..{EXAMPLE_LEN}).map(|i| {expr}).collect();");
    if one_line.len() <= WRAP {
        return vec![one_line];
    }
    vec![
        format!("let {name}: Vec<f64> = (0..{EXAMPLE_LEN})"),
        format!("    .map(|i| {expr})"),
        "    .collect();".to_string(),
    ]
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

/// Format an f64 as a Rust literal (`2.0`, `0.02`). Distinct from `doc_meta::fmt_real`,
/// which drops the fraction: this one feeds generated doctest source, where `2` would
/// be an integer literal and fail to type-check as an `f64` argument.
fn fmt_real_literal(v: f64) -> String {
    if doc_meta::is_integral(v) {
        format!("{v:.1}")
    } else {
        format!("{v}")
    }
}

/// Escape canonical prose for rustdoc markdown: outside code spans, `[` would
/// start an intra-doc link and `<` an (unclosed) HTML tag — both draw rustdoc
/// lints on text like `inReal[i]` or `close<open`. Inside backtick code spans,
/// escapes would render literally, so leave those intact.
///
/// A genuine inline link, `[label](https://…)`, is passed through unescaped. Escaping its
/// opening bracket would strand the URL as plain text, which renders as the literal
/// `\[label](https://…)` *and* trips rustdoc's `bare_urls` lint — the docs are required to
/// build warning-free. Only a bracket whose matching `]` is immediately followed by
/// `(<scheme-or-path>)` qualifies, so `inReal[i]` and `close[i](t)`-style prose still escape.
fn escape_prose(text: &str) -> String {
    let mut out = String::with_capacity(text.len() + 8);
    let mut in_code = false;
    let chars: Vec<char> = text.chars().collect();
    let mut i = 0;
    while i < chars.len() {
        let c = chars[i];
        match c {
            '`' => {
                in_code = !in_code;
                out.push(c);
            }
            '[' if !in_code => {
                if let Some(end) = inline_link_end(&chars, i) {
                    let link: String = chars[i..end].iter().collect();
                    // A site-absolute destination (`/functions/sma`) is written for
                    // ta-lib.org, where the page is served from the site root. Rustdoc has
                    // no such root — docs.rs would resolve it against its own domain — so
                    // point it at the real page instead.
                    out.push_str(&link.replace("](/", "](https://ta-lib.org/"));
                    i = end;
                    continue;
                }
                out.push('\\');
                out.push(c);
            }
            '<' if !in_code => {
                out.push('\\');
                out.push(c);
            }
            '\n' => out.push(' '), // reflow: paragraphs re-wrap on emit
            _ => out.push(c),
        }
        i += 1;
    }
    out
}

/// If a well-formed inline link starts at `start` (`chars[start] == '['`), the index one
/// past its closing `)`. The label must not itself contain a bracket, and the destination
/// must look like a URL or a site-absolute path — a parenthesis that merely follows a
/// bracketed aside is not a link.
fn inline_link_end(chars: &[char], start: usize) -> Option<usize> {
    let close = chars[start + 1..]
        .iter()
        .position(|c| *c == ']' || *c == '[')
        .map(|p| start + 1 + p)
        .filter(|p| chars[*p] == ']')?;
    if chars.get(close + 1) != Some(&'(') {
        return None;
    }
    let paren = chars[close + 2..]
        .iter()
        .position(|c| *c == ')' || *c == '(')
        .map(|p| close + 2 + p)
        .filter(|p| chars[*p] == ')')?;
    let dest: String = chars[close + 2..paren].iter().collect();
    let is_url = dest.starts_with("http://")
        || dest.starts_with("https://")
        || dest.starts_with('/')
        || dest.starts_with('#');
    if is_url && !dest.contains(char::is_whitespace) {
        Some(paren + 1)
    } else {
        None
    }
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

    /// A real inline link survives intact: escaping it would strand the URL as plain text
    /// and trip rustdoc's `bare_urls` lint.
    #[test]
    fn inline_links_are_not_escaped() {
        assert_eq!(
            escape_prose("see [TradingView](https://tv.com/x) for more"),
            "see [TradingView](https://tv.com/x) for more"
        );
        // Site-absolute destinations are rebased: docs.rs has no ta-lib.org root.
        assert_eq!(
            escape_prose("the [`SMA`](/functions/sma) page"),
            "the [`SMA`](https://ta-lib.org/functions/sma) page"
        );
    }

    /// Bracketed prose that merely happens to be followed by parentheses is still escaped —
    /// only a URL-ish destination makes it a link.
    #[test]
    fn bracketed_prose_is_still_escaped() {
        assert_eq!(escape_prose("range [-1, 1]"), "range \\[-1, 1]");
        assert_eq!(escape_prose("close[i](t)"), "close\\[i](t)");
        assert_eq!(
            escape_prose("[label](not a url)"),
            "\\[label](not a url)"
        );
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

    /// Doctest arguments are Rust source: a whole-number `f64` default must keep its
    /// `.0` or the generated example fails to compile. (Prose formatting is
    /// `doc_meta::fmt_real`, tested there.)
    #[test]
    fn real_literal_formatting() {
        assert_eq!(fmt_real_literal(2.0), "2.0");
        assert_eq!(fmt_real_literal(0.3), "0.3");
    }
}
