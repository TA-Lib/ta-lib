//! TSDoc for the generated TypeScript library, from each function's canonical
//! `<name>.md`, and the example tests generated into `library/test/gen/`.
//!
//! TSDoc renders Markdown, so prose passes through as authored, as in the
//! rustdoc. Numbers come from the YAML through [`doc_meta`]. Every `@example`
//! is also a test: [`render_example_tests`] runs the example verbatim and then
//! checks what its doc block claims, so an example that stops compiling or
//! stops being true fails `node --test`.
//!
//! The tests import the package by its own name ([`ts::PACKAGE_NAME`]), so
//! they run against `dist/` (or an installed tarball), never `src/`.

use std::collections::{BTreeMap, BTreeSet, HashMap};
use std::fmt::Write as _;
use std::path::Path;

use super::doc_meta::{self, ensure_period, RangeMeta};
use super::rust_doc::{self, IntClaim};
use super::ts::{self, ts_names};
use crate::ir::{DocDef, EnumDef, FuncDef, OptInput, Output, ParamType};
use crate::registry::Registry;
use crate::streaming;

/// Content width of a wrapped doc line, after ` * `.
const WRAP: usize = 80;

/// Bars in every example series: comfortably past the largest default lookback.
const EXAMPLE_LEN: usize = rust_doc::EXAMPLE_LEN;

// ---------------------------------------------------------------------------
// Declaration docs
// ---------------------------------------------------------------------------

/// The public batch function.
#[allow(clippy::implicit_hasher)]
#[must_use]
pub fn batch_docs(func: &FuncDef, enums: &HashMap<String, EnumDef>, registry: &Registry) -> String {
    let doc = doc_of(func);
    let names = ts_names(&func.name);
    let mut b = Block::new("");
    b.para(&summary_text(func, doc));
    b.para(&page_link(func));
    let notes = doc_meta::renderable_notes(&doc.notes);
    if !notes.is_empty() {
        b.para("**Notes**");
        for note in notes {
            b.bullet(&tsdoc(note));
        }
    }
    b.para(&format!(
        "Output `k` holds the value at input bar `begIdx + k`, for `k < count`; nothing \
         is padded. A range that ends before {{@link {}}} is a success with `count` 0.",
        names.lookback
    ));
    b.blank();
    b.param("startIdx", "First bar of the requested range (inclusive).");
    b.param("endIdx", "Last bar of the requested range (inclusive).");
    for input in &func.inputs {
        b.param(&input.name, &input_desc(&input.name, doc));
    }
    for opt in &func.optional_inputs {
        b.param(&opt.name, &param_doc(opt, doc, enums));
    }
    for out in &func.outputs {
        b.param(
            &out.name,
            &format!(
                "{} Must hold the values produced and never be empty; the input length always suffices.",
                output_desc(out, doc)
            ),
        );
    }
    b.param("core", CORE_PARAM);
    b.tag(
        "returns",
        "The range written: `begIdx` is the first bar with a value, `count` how many were written.",
    );
    let mut causes = vec!["an argument that is not an int32"];
    if has_real_param(func) {
        causes.push("a real parameter that is not a number");
    }
    if !func.optional_inputs.is_empty() {
        causes.push("a parameter outside its range");
    }
    causes.extend(["an array too short for the range", "an empty output"]);
    if ts::shared_output_condition(func).is_some() {
        causes.push("two outputs sharing memory");
    }
    if !func.inputs.is_empty() {
        causes.push("an output sharing memory with a `Float64Array` input it is not");
    }
    b.tag(
        "throws",
        &format!(
            "`TALibError` with code `TA_OUT_OF_RANGE_START_INDEX` or `TA_OUT_OF_RANGE_END_INDEX` \
             for a range outside `0..Core.INDEX_MAX` or ending before it starts, and \
             `TA_BAD_PARAM` for {}. Checked before anything is written.",
            any_of(&causes)
        ),
    );
    if let Some(example) = batch_example(func, enums) {
        b.example(&example);
    }
    for link in see_also(func, doc, registry) {
        b.tag("see", &link);
    }
    b.render()
}

/// `<n>Lookback`.
#[allow(clippy::implicit_hasher)]
#[must_use]
pub fn lookback_docs(func: &FuncDef, enums: &HashMap<String, EnumDef>) -> String {
    let doc = doc_of(func);
    let names = ts_names(&func.name);
    let mut b = Block::new("");
    b.para(&format!(
        "Number of leading input bars {{@link {}}} consumes before its first value: the \
         index of the first bar with a value when the whole series is requested.",
        names.public
    ));
    if func.flags.iter().any(|f| f == "unstable_period") {
        b.para("It includes `core`'s unstable period for this function.");
    }
    b.blank();
    for opt in &func.optional_inputs {
        b.param(&opt.name, &param_doc(opt, doc, enums));
    }
    b.param("core", CORE_PARAM);
    b.tag("returns", "The lookback, or -1 if a parameter is out of range.");
    b.render()
}

/// `<n>DisplayShift`.
#[allow(clippy::implicit_hasher)]
#[must_use]
pub fn display_shift_docs(func: &FuncDef, enums: &HashMap<String, EnumDef>) -> String {
    let doc = doc_of(func);
    let names = ts_names(&func.name);
    let mut b = Block::new("");
    b.para(&format!(
        "How many bars ahead (positive) or behind (negative) of the bar that computed it a \
         chart draws one output of {{@link {}}}.",
        names.public
    ));
    b.para(if func.has_display_shift() {
        "The values are never shifted: this describes the drawing only."
    } else {
        "Every output of this function is drawn at its own bar, so the answer is 0."
    });
    b.blank();
    for opt in &func.optional_inputs {
        b.param(&opt.name, &param_doc(opt, doc, enums));
    }
    b.param(crate::ir::DISPLAY_SHIFT_INDEX_PARAM, "Position of the output in the batch signature, from 0.");
    b.param("core", CORE_PARAM);
    b.tag(
        "returns",
        "The display shift, or -2147483648 if a parameter is out of range or the index names no output.",
    );
    b.render()
}

/// `<n>Open`.
#[allow(clippy::implicit_hasher)]
#[must_use]
pub fn open_docs(func: &FuncDef, enums: &HashMap<String, EnumDef>) -> String {
    let doc = doc_of(func);
    let names = ts_names(&func.name);
    let mut b = Block::new("");
    b.para(&format!(
        "Opens a live {} stream over the warm-up history: `value()` starts at the last \
         history bar, bit-identical to {{@link {}}} there.",
        func.name, names.public
    ));
    b.blank();
    history_params(&mut b, func, doc, enums);
    b.param("core", CORE_PARAM);
    b.tag("returns", "The stream, positioned on the last history bar.");
    b.tag("throws", &open_throws(func, false));
    if let Some(example) = open_example(func, enums) {
        b.example(&example);
    }
    b.render()
}

/// `<n>OpenAndFill`.
#[allow(clippy::implicit_hasher)]
#[must_use]
pub fn open_and_fill_docs(func: &FuncDef, enums: &HashMap<String, EnumDef>) -> String {
    let doc = doc_of(func);
    let names = ts_names(&func.name);
    let mut b = Block::new("");
    b.para(&format!(
        "{{@link {}}} that also fills the outputs over the whole history in the same pass, \
         bit-identically to {{@link {}}}. The range written is the stream's `outRange()`.",
        names.open, names.public
    ));
    b.blank();
    history_params(&mut b, func, doc, enums);
    for out in &func.outputs {
        b.param(
            &out.name,
            &format!(
                "{} Must hold `historyLen - lookback` values and share no memory with an input \
                 or another output.",
                output_desc(out, doc)
            ),
        );
    }
    b.param("core", CORE_PARAM);
    b.tag("returns", "The stream, positioned on the last history bar.");
    b.tag("throws", &open_throws(func, true));
    if let Some(example) = fill_example(func, enums) {
        b.example(&example);
    }
    b.render()
}

/// The `<N>Stream` class.
#[must_use]
pub fn stream_class_docs(func: &FuncDef) -> String {
    let names = ts_names(&func.name);
    let mut b = Block::new("");
    b.para(&format!(
        "A live {} stream: one value per closed bar, bit-identical to {{@link {}}} over the \
         same series. Open it with {{@link {}}} or {{@link {}}}; there is nothing to close.",
        func.name, names.public, names.open, names.open_and_fill
    ));
    b.para(&page_link(func));
    b.render()
}

/// One public method of `<N>Stream`.
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum StreamMethod {
    OutRange,
    Advance,
    Update,
    Peek,
    Value,
    Clone,
}

/// A public method of `<N>Stream`, indented as a class member.
#[must_use]
pub fn stream_method_docs(func: &FuncDef, method: StreamMethod) -> String {
    let doc = doc_of(func);
    let names = ts_names(&func.name);
    let multi = func.outputs.len() > 1;
    let mut b = Block::new("  ");
    match method {
        StreamMethod::OutRange => {
            b.para(&format!(
                "The bars this stream has an output for, `[begIdx, begIdx + count)`, as \
                 {{@link {}}} reports them over the same bars. Each accepted `update` and each \
                 `advance` adds one to `count`.",
                names.public
            ));
        }
        StreamMethod::Advance => {
            b.para(
                "Counts one bar this stream was not fed: `outRange()` advances by one and \
                 `value()` keeps answering the previous output.",
            );
            b.blank();
            b.tag("throws", CEILING_THROWS);
        }
        StreamMethod::Update | StreamMethod::Peek => {
            let update = method == StreamMethod::Update;
            b.para(match (update, multi) {
                (true, false) => "Commits one closed bar and returns the new value.",
                (true, true) => "Commits one closed bar, writing the new values into `out`.",
                (false, false) => "What the next `update` with the same bar would return, committing nothing.",
                (false, true) => "Writes into `out` what the next `update` with the same bar would write, committing nothing.",
            });
            b.blank();
            for bar in streaming::input_array_names(func) {
                b.param(&bar, &format!("The bar's `{bar}` value: {}", input_desc(&bar, doc)));
            }
            if multi {
                b.param("out", "Receives the bar's outputs.");
            } else {
                b.tag("returns", if update { "The new value." } else { "The value `update` would return." });
            }
            if update {
                b.tag(
                    "throws",
                    &format!(
                        "`TALibError` with code `TA_BAD_PARAM` for a bar value that is not finite, \
                         before anything moves. {CEILING_THROWS}"
                    ),
                );
            } else {
                b.tag("throws", "`TALibError` with code `TA_BAD_PARAM` for a bar value that is not finite.");
            }
        }
        StreamMethod::Value => {
            b.para("The value at the last bar this stream counted; `peek` does not change it.");
            if multi {
                b.blank();
                b.param("out", "Receives the outputs.");
            }
        }
        StreamMethod::Clone => {
            b.para(
                "An independent fork: both streams evolve separately from here. Buffers are \
                 copied and sub-streams cloned; the `Core` is shared.",
            );
            b.blank();
            b.tag("returns", "A stream at the same bar.");
        }
    }
    b.render()
}

/// The `<N>Out` sink class of a multi-output stream.
#[must_use]
pub fn out_class_docs(func: &FuncDef) -> String {
    let mut b = Block::new("");
    b.para(&format!(
        "The outputs of one {} bar, written into an object the caller owns and reuses: \
         valid until the next call that writes it.",
        func.name
    ));
    b.render()
}

/// One field of `<N>Out`.
#[must_use]
pub fn out_field_docs(func: &FuncDef, output: &Output) -> String {
    let mut b = Block::new("  ");
    b.para(&output_desc(output, doc_of(func)));
    b.render()
}

const CORE_PARAM: &str = "Unstable periods and candle settings to compute with; `Core.DEFAULT` when omitted.";

const CEILING_THROWS: &str =
    "`TALibError` with code `TA_OUT_OF_RANGE_END_INDEX` once `outRange()` has reached bar `Core.INDEX_MAX`.";

fn history_params(b: &mut Block, func: &FuncDef, doc: &DocDef, enums: &HashMap<String, EnumDef>) {
    let bars = streaming::input_array_names(func);
    let same = if bars.len() > 1 { " Every history has the same length." } else { "" };
    for bar in &bars {
        b.param(bar, &format!("Warm-up history. {}{same}", input_desc(bar, doc)));
    }
    for opt in &func.optional_inputs {
        b.param(&opt.name, &param_doc(opt, doc, enums));
    }
}

fn open_throws(func: &FuncDef, fill: bool) -> String {
    let names = ts_names(&func.name);
    let mut causes = Vec::new();
    if func.optional_inputs.iter().any(|o| matches!(o.param_type, ParamType::Integer | ParamType::Enum(_))) {
        causes.push("an integer parameter that is not an int32");
    }
    if has_real_param(func) {
        causes.push("a real parameter that is not a number");
    }
    if !func.optional_inputs.is_empty() {
        causes.push("a parameter outside its range");
    }
    if streaming::input_array_names(func).len() > 1 {
        causes.push("histories of different lengths");
    }
    if fill {
        causes.push(
            "an output too short or starting where another output or a `Float64Array` input starts, or, on a \
             history long enough to open, one sharing memory with another output or a `Float64Array` input",
        );
    }
    let bad = if causes.is_empty() { String::new() } else { format!(", and `TA_BAD_PARAM` for {}", any_of(&causes)) };
    format!(
        "`TALibError` with code `TA_INSUFFICIENT_HISTORY` for a history shorter than \
         `{}(...) + 1` bars, an empty one included{bad}.",
        names.lookback
    )
}

fn has_real_param(func: &FuncDef) -> bool {
    func.optional_inputs.iter().any(|o| o.param_type == ParamType::Real)
}

/// `a`, `a or b`, `a, b or c`.
fn any_of(items: &[&str]) -> String {
    match items {
        [] => String::new(),
        [one] => (*one).to_string(),
        [rest @ .., last] => format!("{} or {last}", rest.join(", ")),
    }
}

// ---------------------------------------------------------------------------
// Prose pieces
// ---------------------------------------------------------------------------

fn doc_of(func: &FuncDef) -> &DocDef {
    static EMPTY: std::sync::OnceLock<DocDef> = std::sync::OnceLock::new();
    func.doc.as_ref().unwrap_or_else(|| EMPTY.get_or_init(DocDef::default))
}

fn summary_text(func: &FuncDef, doc: &DocDef) -> String {
    if doc.summary.is_empty() {
        tsdoc(func.description.as_deref().or(func.hint.as_deref()).unwrap_or(&func.group))
    } else {
        tsdoc(&doc.summary)
    }
}

fn page_link(func: &FuncDef) -> String {
    let url = doc_meta::function_page_url(&func.name);
    format!("Formula and more info at [{}]({url}).", url.trim_start_matches("https://"))
}

/// One input's prose. A price bundle's components fall back to standard texts.
fn input_desc(name: &str, doc: &DocDef) -> String {
    if let Some((_, desc)) = doc.inputs.iter().find(|(n, _)| n == name) {
        return ensure_period(&tsdoc(desc));
    }
    match name {
        "inOpen" => "Open price per bar.",
        "inHigh" => "High price per bar.",
        "inLow" => "Low price per bar.",
        "inClose" => "Close price per bar.",
        "inVolume" => "Volume per bar.",
        "inOpenInterest" => "Open interest per bar.",
        _ => "Input data series.",
    }
    .to_string()
}

fn output_desc(output: &Output, doc: &DocDef) -> String {
    doc.outputs
        .iter()
        .find(|(n, _)| n == &output.name)
        .map_or_else(|| "Output values.".to_string(), |(_, d)| ensure_period(&tsdoc(d)))
}

/// An optional parameter's prose, with its default and range from the YAML.
fn param_doc(opt: &OptInput, doc: &DocDef, enums: &HashMap<String, EnumDef>) -> String {
    let desc = doc
        .params
        .iter()
        .find(|(n, _)| n == &opt.name)
        .map(|(_, d)| tsdoc(d))
        .or_else(|| opt.hint.as_deref().map(tsdoc))
        .or_else(|| opt.display_name.as_deref().map(tsdoc))
        .unwrap_or_else(|| "Optional parameter".to_string());

    let m = doc_meta::param_meta(opt, enums);
    let mut meta: Vec<String> = Vec::new();
    if let ParamType::Enum(name) = &opt.param_type {
        if let Some(variant) = &m.default_variant {
            meta.push(format!("default `{name}.{variant}`"));
        }
        if !m.values.is_empty() {
            let values: Vec<String> = m.values.iter().map(|(v, n)| format!("{v}={n}")).collect();
            meta.push(format!("values: {}", values.join(", ")));
        }
        let mut selectors = vec!["`Core.INTEGER_DEFAULT`".to_string()];
        if let Some(v) = super::common::enum_default_variant(enums, name) {
            selectors.push(format!("`{name}.{}`", v.name));
        }
        meta.push(format!("{} selects the default", selectors.join(" or ")));
    } else {
        if let Some(d) = &m.default {
            meta.push(format!("default {d}"));
        }
        match &m.range {
            RangeMeta::Bounded(lo, hi) => meta.push(format!("range {lo} to {hi}")),
            RangeMeta::Min(lo) => meta.push(format!("minimum {lo}")),
            RangeMeta::Max(hi) => meta.push(format!("maximum {hi}")),
            RangeMeta::Unbounded => {}
        }
        meta.push(match opt.param_type {
            ParamType::Real => "`Core.REAL_DEFAULT` selects the default".to_string(),
            _ => "`Core.INTEGER_DEFAULT` selects the default".to_string(),
        });
    }
    format!("{} ({}).", ensure_period(&desc).trim_end_matches('.'), meta.join("; "))
}

/// `## See Also` entries the registry knows, as links to their pages: a
/// `{@link}` would dangle, since the module does not import its neighbours.
fn see_also(func: &FuncDef, doc: &DocDef, registry: &Registry) -> Vec<String> {
    doc.see_also
        .iter()
        .map(|e| e.trim())
        .filter(|n| !n.is_empty() && !n.eq_ignore_ascii_case(&func.name))
        .filter(|n| n.chars().all(|c| c.is_ascii_alphanumeric() || c == '_'))
        .filter(|n| registry.contains(&n.to_lowercase()))
        .map(|n| {
            let name = registry.name_of(&n.to_lowercase());
            format!("[`{}`]({})", ts::func_ident(&name), doc_meta::function_page_url(&name))
        })
        .collect()
}

/// Canonical Markdown made safe inside a `/** */` block: rustdoc's escaping
/// (TSDoc renders the same Markdown), plus `@` outside code spans, which TSDoc
/// would read as a tag at the start of a wrapped line.
#[must_use]
pub fn tsdoc(text: &str) -> String {
    let escaped = rust_doc::escape_prose(text);
    let mut out = String::with_capacity(escaped.len());
    let mut in_code = false;
    for c in escaped.chars() {
        match c {
            '`' => in_code = !in_code,
            '@' if !in_code => out.push('\\'),
            _ => {}
        }
        out.push(c);
    }
    out.replace("*/", "*\u{200b}/")
}

// ---------------------------------------------------------------------------
// Block assembly
// ---------------------------------------------------------------------------

struct Block {
    indent: &'static str,
    lines: Vec<String>,
}

impl Block {
    fn new(indent: &'static str) -> Self {
        Block { indent, lines: Vec::new() }
    }

    fn blank(&mut self) {
        if self.lines.last().is_some_and(|l| !l.is_empty()) {
            self.lines.push(String::new());
        }
    }

    fn para(&mut self, text: &str) {
        self.blank();
        self.lines.extend(wrap(text, WRAP));
    }

    fn bullet(&mut self, text: &str) {
        for (i, line) in wrap(text, WRAP - 2).into_iter().enumerate() {
            self.lines.push(if i == 0 { format!("- {line}") } else { format!("  {line}") });
        }
    }

    fn tag(&mut self, tag: &str, text: &str) {
        for (i, line) in wrap(&format!("@{tag} {text}"), WRAP).into_iter().enumerate() {
            self.lines.push(if i == 0 { line } else { format!("  {line}") });
        }
    }

    fn param(&mut self, name: &str, text: &str) {
        self.tag(&format!("param {name} -"), text);
    }

    fn example(&mut self, ex: &Example) {
        self.blank();
        self.lines.push("@example".to_string());
        self.lines.push("```ts".to_string());
        self.lines.extend(ex.source());
        self.lines.push("```".to_string());
    }

    fn render(&self) -> String {
        let pad = self.indent;
        if let [line] = self.lines.as_slice() {
            if pad.len() + line.len() + 7 <= 100 {
                return format!("{pad}/** {line} */\n");
            }
        }
        let mut s = format!("{pad}/**\n");
        for line in &self.lines {
            if line.is_empty() {
                let _ = writeln!(s, "{pad} *");
            } else {
                let _ = writeln!(s, "{pad} * {line}");
            }
        }
        let _ = writeln!(s, "{pad} */");
        s
    }
}

/// Greedy word wrap that never breaks inside a code span or a `{@link}`, and
/// never starts a line with a word Markdown would read as a block marker.
fn wrap(text: &str, width: usize) -> Vec<String> {
    let mut words: Vec<String> = Vec::new();
    let mut word = String::new();
    let (mut code, mut brace) = (false, 0usize);
    for c in text.chars() {
        match c {
            '`' => code = !code,
            '{' if !code => brace += 1,
            '}' if !code => brace = brace.saturating_sub(1),
            _ => {}
        }
        if c == ' ' && !code && brace == 0 {
            if !word.is_empty() {
                words.push(std::mem::take(&mut word));
            }
        } else {
            word.push(c);
        }
    }
    if !word.is_empty() {
        words.push(word);
    }

    let mut lines: Vec<String> = Vec::new();
    let mut line = String::new();
    for w in words {
        let fits = line.chars().count() + 1 + w.chars().count() <= width;
        if line.is_empty() || fits || starts_block(&w) {
            if !line.is_empty() {
                line.push(' ');
            }
            line.push_str(&w);
        } else {
            lines.push(std::mem::replace(&mut line, w));
        }
    }
    if !line.is_empty() {
        lines.push(line);
    }
    lines
}

/// A word that would open a list, heading, quote, table row or thematic break
/// at the start of a line.
fn starts_block(word: &str) -> bool {
    if matches!(word, "-" | "+" | "*" | ">" | "|" | "=" | "#") || word.starts_with(['#', '>', '|']) {
        return true;
    }
    if word.starts_with(['-', '+', '*', '=']) && word.chars().all(|c| matches!(c, '-' | '+' | '*' | '=')) {
        return true;
    }
    let digits = word.chars().take_while(char::is_ascii_digit).count();
    digits > 0 && matches!(&word[digits..], "." | ")")
}

// ---------------------------------------------------------------------------
// Examples
// ---------------------------------------------------------------------------

/// One `@example`: what the doc block shows, and what its test checks after
/// running that code verbatim.
#[derive(Debug, Clone)]
pub struct Example {
    /// The test's title.
    pub title: String,
    /// Names the example imports from the package.
    pub imports: BTreeSet<String>,
    /// The example's code, after its import line.
    pub body: Vec<String>,
    /// Names only the checks import.
    pub check_imports: BTreeSet<String>,
    /// Assertions over the variables the body defined.
    pub checks: Vec<String>,
}

impl Example {
    /// The example as the doc block shows it.
    #[must_use]
    pub fn source(&self) -> Vec<String> {
        let mut lines: Vec<String> = import_line(&self.imports).lines().map(String::from).collect();
        lines.push(String::new());
        lines.extend(self.body.iter().cloned());
        lines
    }
}

fn import_line(names: &BTreeSet<String>) -> String {
    let names: Vec<&str> = names.iter().map(String::as_str).collect();
    let one = format!("import {{ {} }} from '{}';", names.join(", "), ts::PACKAGE_NAME);
    if one.len() <= WRAP {
        return one;
    }
    let mut s = String::from("import {\n");
    for line in wrap(&names.join(", "), WRAP - 2) {
        let _ = writeln!(s, "  {line}");
    }
    let _ = write!(s, "}} from '{}';", ts::PACKAGE_NAME);
    s
}

/// One input's example series: `(variable, element expression in i, one more bar)`.
/// The batch, open and fill examples feed the same history, and `open` and
/// `close` stay inside `[low, high]`. See [`rust_doc::CLOSE_SERIES`] for why
/// `close` carries a second harmonic and `volume` falls on some bars.
fn series(func: &FuncDef, input: &str) -> Option<(&'static str, &'static str, &'static str)> {
    Some(match input {
        "inReal" if rust_doc::unit_domain(func) => ("data", "Math.sin(0.1 * i)", "0.42"),
        "inOpen" => ("open", "100 + 10 * Math.sin(0.1 * i - 0.05)", "100.2"),
        "inHigh" => ("high", "101 + 10 * Math.sin(0.1 * i)", "101.4"),
        "inLow" => ("low", "99 + 10 * Math.sin(0.1 * i)", "99.1"),
        "inClose" => ("close", "100 + 10 * Math.sin(0.1 * i) + 0.8 * Math.sin(0.7 * i)", "100.9"),
        "inVolume" => ("volume", "10000 + 100 * i + 2000 * Math.sin(0.3 * i)", "12345"),
        "inPeriods" => ("periods", "5 + (i % 10)", "14"),
        "inReal" => ("data", "100 + 10 * Math.sin(0.1 * i)", "100.9"),
        "inReal0" => ("data0", "100 + 10 * Math.sin(0.1 * i)", "100.9"),
        "inReal1" => ("data1", "100 + 10 * Math.sin(0.1 * i + 0.7)", "101.3"),
        _ => return None,
    })
}

fn series_lines(var: &str, expr: &str) -> Vec<String> {
    let one = format!("const {var} = Float64Array.from({{ length: {EXAMPLE_LEN} }}, (_, i) => {expr});");
    if one.len() <= WRAP {
        return vec![one];
    }
    vec![
        format!("const {var} = Float64Array.from({{ length: {EXAMPLE_LEN} }}, (_, i) =>"),
        format!("  {expr});"),
    ]
}

/// What every example of a function shares: its series and argument literals.
struct Setup {
    /// One `(variable, bar literal)` per bar input, in call order.
    series: Vec<(&'static str, &'static str)>,
    lines: Vec<String>,
    opts: Vec<String>,
    imports: BTreeSet<String>,
}

fn setup(func: &FuncDef, inputs: &[String], enums: &HashMap<String, EnumDef>) -> Option<Setup> {
    let mut s = Setup { series: Vec::new(), lines: Vec::new(), opts: Vec::new(), imports: BTreeSet::new() };
    for input in inputs {
        let (var, expr, bar) = series(func, input)?;
        s.lines.extend(series_lines(var, expr));
        s.series.push((var, bar));
    }
    if s.series.is_empty() {
        return None;
    }
    for opt in &func.optional_inputs {
        let (lit, import) = opt_literal(opt, enums);
        s.opts.push(lit);
        s.imports.extend(import);
    }
    Some(s)
}

/// An optional parameter at its documented default, an enum one by name.
#[allow(clippy::cast_possible_truncation)]
fn opt_literal(opt: &OptInput, enums: &HashMap<String, EnumDef>) -> (String, Option<String>) {
    let default = opt.default.unwrap_or(0.0);
    match &opt.param_type {
        ParamType::Enum(name) => {
            let variant = enums
                .get(name)
                .and_then(|e| e.variants.iter().find(|v| f64::from(v.value).to_bits() == default.to_bits()));
            match variant {
                Some(v) => (format!("{name}.{}", v.name), Some(name.clone())),
                None => ((default as i64).to_string(), None),
            }
        }
        ParamType::Real => (doc_meta::fmt_real(default), None),
        _ => ((default as i64).to_string(), None),
    }
}

/// The names an example or its checks bind besides its series and outputs.
const EXAMPLE_LOCALS: &[&str] =
    &["begIdx", "count", "stream", "peeked", "updated", "range", "batch", "k", "i", "v", "bar", "idx"];

/// One variable per output, distinct from each other, from [`EXAMPLE_LOCALS`]
/// and from every binding the example or its checks import.
fn output_vars(func: &FuncDef, taken: &BTreeSet<String>) -> Vec<String> {
    rust_doc::output_var_names(func)
        .iter()
        .map(|snake| {
            let camel = snake_to_camel(snake);
            if taken.contains(&camel)
                || EXAMPLE_LOCALS.contains(&camel.as_str())
                || ts::RESERVED_WORDS.contains(&camel.as_str())
            {
                format!("{camel}Out")
            } else {
                camel
            }
        })
        .collect()
}

fn snake_to_camel(s: &str) -> String {
    let mut out = String::new();
    let mut upper = false;
    for c in s.chars() {
        if c == '_' {
            upper = true;
        } else if upper {
            out.push(c.to_ascii_uppercase());
            upper = false;
        } else {
            out.push(c);
        }
    }
    out
}

fn array_ctor(output: &Output) -> &'static str {
    ts::array_ts_type(&output.param_type)
}

/// `name(a, b, ...)`, broken one argument group per line when it is long.
fn call(lead: &str, name: &str, groups: &[Vec<String>]) -> Vec<String> {
    let flat: Vec<String> = groups.iter().flatten().cloned().collect();
    let one = format!("{lead}{name}({});", flat.join(", "));
    if one.len() <= WRAP {
        return vec![one];
    }
    let mut lines = vec![format!("{lead}{name}(")];
    for g in groups.iter().filter(|g| !g.is_empty()) {
        lines.push(format!("  {},", g.join(", ")));
    }
    lines.push(");".to_string());
    lines
}

/// The batch call over the whole history.
#[allow(clippy::implicit_hasher)]
#[must_use]
pub fn batch_example(func: &FuncDef, enums: &HashMap<String, EnumDef>) -> Option<Example> {
    let names = ts_names(&func.name);
    let inputs: Vec<String> = func.inputs.iter().map(|i| i.name.clone()).collect();
    let s = setup(func, &inputs, enums)?;
    let first = s.series[0].0;
    let mut imports = s.imports.clone();
    imports.insert(names.public.clone());
    let outs = output_vars(func, &imports);

    let mut body = s.lines.clone();
    for (out, var) in func.outputs.iter().zip(&outs) {
        body.push(format!("const {var} = new {}({first}.length);", array_ctor(out)));
    }
    body.extend(call(
        "const { begIdx, count } = ",
        &names.public,
        &[
            vec!["0".to_string(), format!("{first}.length - 1")],
            s.series.iter().map(|(v, _)| (*v).to_string()).collect(),
            s.opts.clone(),
            outs.clone(),
        ],
    ));
    body.push(format!("// {}[k] is the value at bar begIdx + k, for k < count", outs[0]));

    let mut checks = vec!["assert.ok(count > 0);".to_string(), format!("assert.equal(begIdx + count, {first}.length);")];
    if let Some((_, var)) = func.outputs.iter().zip(&outs).find(|(o, _)| o.param_type != ParamType::Integer) {
        checks.push(format!("assert.ok({var}.subarray(0, count).every(Number.isFinite));"));
    }
    if func.outputs.iter().any(|o| o.param_type == ParamType::Integer) {
        let period = func
            .optional_inputs
            .iter()
            .position(|o| o.name == "optInTimePeriod")
            .map(|i| s.opts[i].clone());
        for (out, var) in func.outputs.iter().zip(&outs) {
            if out.param_type == ParamType::Integer {
                checks.extend(integer_claim_checks(func, out, var, first, period.as_deref()));
            }
        }
    }
    Some(Example {
        title: format!("{}: {} example", func.name, names.public),
        imports,
        body,
        check_imports: BTreeSet::new(),
        checks,
    })
}

/// The test lines asserting one integer output's domain.
fn integer_claim_checks(func: &FuncDef, out: &Output, var: &str, series: &str, period: Option<&str>) -> Vec<String> {
    let Some(claim) = rust_doc::integer_claim(func, out) else {
        return Vec::new();
    };
    let all = |pred: &str| format!("assert.ok({var}.subarray(0, count).every((v) => {pred}));");
    match claim {
        IntClaim::Pattern(values) => {
            let list: Vec<String> = values.iter().map(i32::to_string).collect();
            vec![
                "// a pattern output writes only the values its flags declare".to_string(),
                all(&format!("[{}].includes(v)", list.join(", "))),
            ]
        }
        IntClaim::Either(a, b, note) => vec![format!("// {note}"), all(&format!("v === {a} || v === {b}"))],
        IntClaim::Within(lo, hi, note) => vec![format!("// {note}"), all(&format!("v >= {lo} && v <= {hi}"))],
        IntClaim::WindowExtremum { is_max } => {
            let period = period.expect("an index function carries optInTimePeriod");
            let (cmp, word) = if is_max { ("<=", "highest") } else { (">=", "lowest") };
            vec![
                format!("// every reported index locates the {word} value of its {period}-bar window"),
                "for (let k = 0; k < count; k++) {".to_string(),
                format!("  const bar = begIdx + k, idx = {var}[k];"),
                format!("  assert.ok(bar + 1 - {period} <= idx && idx <= bar);"),
                format!("  assert.ok({series}.subarray(bar + 1 - {period}, bar + 1).every((v) => v {cmp} {series}[idx]));"),
                "}".to_string(),
            ]
        }
        IntClaim::PivotIdx(note) => vec![
            format!("// {note}"),
            "for (let k = 0; k < count; k++) {".to_string(),
            format!("  assert.ok({var}[k] <= begIdx + k);"),
            format!("  if (k > 0) assert.ok({var}[k] >= {var}[k - 1]);"),
            "}".to_string(),
        ],
    }
}

/// `<n>Open`, then one forming bar peeked and the same bar committed.
#[allow(clippy::implicit_hasher)]
#[must_use]
pub fn open_example(func: &FuncDef, enums: &HashMap<String, EnumDef>) -> Option<Example> {
    if !func.streaming {
        return None;
    }
    let names = ts_names(&func.name);
    let s = setup(func, &streaming::input_array_names(func), enums)?;
    let first = s.series[0].0;
    let mut imports = s.imports.clone();
    imports.insert(names.open.clone());
    let mut args: Vec<String> = s.series.iter().map(|(v, _)| (*v).to_string()).collect();
    args.extend(s.opts.iter().cloned());
    let bars: Vec<&str> = s.series.iter().map(|(_, b)| *b).collect();

    let mut body = s.lines.clone();
    body.extend(call("const stream = ", &names.open, &[args]));
    let mut checks = Vec::new();
    if func.outputs.len() > 1 {
        imports.insert(names.out_class.clone());
        body.push(format!("const peeked = new {}();", names.out_class));
        body.push(format!("const updated = new {}();", names.out_class));
        body.push(format!("stream.peek({}, peeked); // a forming bar: commits nothing", bars.join(", ")));
        body.push(format!("stream.update({}, updated); // the bar closes", bars.join(", ")));
        for out in &func.outputs {
            let f = super::java_stream::value_field_name(&out.name);
            checks.push(format!("assert.ok(Object.is(peeked.{f}, updated.{f}));"));
        }
    } else {
        body.push(format!("const peeked = stream.peek({}); // a forming bar: commits nothing", bars.join(", ")));
        body.push(format!("const updated = stream.update({}); // the bar closes", bars.join(", ")));
        checks.push("assert.ok(Object.is(peeked, updated));".to_string());
        checks.push("assert.ok(Object.is(stream.value(), updated));".to_string());
    }
    checks.push("const range = stream.outRange();".to_string());
    checks.push(format!("assert.equal(range.begIdx + range.count, {first}.length + 1);"));
    Some(Example {
        title: format!("{}: {} example", func.name, names.open),
        imports,
        body,
        check_imports: BTreeSet::new(),
        checks,
    })
}

/// `<n>OpenAndFill` over the history; the checks hold the fill to the batch call.
#[allow(clippy::implicit_hasher)]
#[must_use]
pub fn fill_example(func: &FuncDef, enums: &HashMap<String, EnumDef>) -> Option<Example> {
    if !func.streaming {
        return None;
    }
    let names = ts_names(&func.name);
    let s = setup(func, &streaming::input_array_names(func), enums)?;
    let first = s.series[0].0;
    let mut imports = s.imports.clone();
    imports.insert(names.open_and_fill.clone());
    let check_imports: BTreeSet<String> = [names.public.clone()].into_iter().collect();
    let taken: BTreeSet<String> = imports.union(&check_imports).cloned().collect();
    let outs = output_vars(func, &taken);
    let series_args: Vec<String> = s.series.iter().map(|(v, _)| (*v).to_string()).collect();

    let mut body = s.lines.clone();
    for (out, var) in func.outputs.iter().zip(&outs) {
        body.push(format!("const {var} = new {}({first}.length);", array_ctor(out)));
    }
    body.extend(call(
        "const stream = ",
        &names.open_and_fill,
        &[series_args.clone(), s.opts.clone(), outs.clone()],
    ));
    body.push("const { begIdx, count } = stream.outRange();".to_string());
    body.push(format!("// {}[k] is the value at bar begIdx + k, for k < count", outs[0]));

    let batch_outs: Vec<String> = outs.iter().map(|v| format!("batch{}{}", v[..1].to_ascii_uppercase(), &v[1..])).collect();
    let mut checks = Vec::new();
    for (out, var) in func.outputs.iter().zip(&batch_outs) {
        checks.push(format!("const {var} = new {}({first}.length);", array_ctor(out)));
    }
    checks.extend(call(
        "const batch = ",
        &names.public,
        &[
            vec!["0".to_string(), format!("{first}.length - 1")],
            series_args,
            s.opts.clone(),
            batch_outs.clone(),
        ],
    ));
    checks.push("assert.deepEqual([begIdx, count], [batch.begIdx, batch.count]);".to_string());
    checks.push("assert.ok(count > 0);".to_string());
    checks.push("for (let k = 0; k < count; k++) {".to_string());
    for (var, batch) in outs.iter().zip(&batch_outs) {
        checks.push(format!("  assert.ok(Object.is({var}[k], {batch}[k]));"));
    }
    checks.push("}".to_string());
    Some(Example {
        title: format!("{}: {} example", func.name, names.open_and_fill),
        imports,
        body,
        check_imports,
        checks,
    })
}

/// Every example of one function, in doc order.
#[allow(clippy::implicit_hasher)]
#[must_use]
pub fn examples(func: &FuncDef, enums: &HashMap<String, EnumDef>) -> Vec<Example> {
    [batch_example(func, enums), open_example(func, enums), fill_example(func, enums)]
        .into_iter()
        .flatten()
        .collect()
}

// ---------------------------------------------------------------------------
// Example tests
// ---------------------------------------------------------------------------

/// The `library/test/gen/` file a function's examples land in: one per group.
fn test_file(func: &FuncDef) -> String {
    let mut stem = String::new();
    for (i, word) in func.group.split_whitespace().enumerate() {
        let word = word.to_ascii_lowercase();
        if i == 0 {
            stem.push_str(&word);
        } else {
            let mut cs = word.chars();
            if let Some(c) = cs.next() {
                stem.push(c.to_ascii_uppercase());
                stem.push_str(cs.as_str());
            }
        }
    }
    format!("{stem}.test.ts")
}

/// Every example test file, by file name.
#[allow(clippy::implicit_hasher)]
#[must_use]
pub fn render_example_tests(funcs: &[FuncDef], enums: &HashMap<String, EnumDef>) -> BTreeMap<String, String> {
    let mut by_file: BTreeMap<String, Vec<&FuncDef>> = BTreeMap::new();
    for f in funcs {
        by_file.entry(test_file(f)).or_default().push(f);
    }
    let mut files = BTreeMap::new();
    for (file, mut fs) in by_file {
        fs.sort_by(|a, b| a.name.cmp(&b.name));
        let exs: Vec<Example> = fs.iter().flat_map(|f| examples(f, enums)).collect();
        if exs.is_empty() {
            continue;
        }
        let mut imports = BTreeSet::new();
        for ex in &exs {
            imports.extend(ex.imports.iter().cloned());
            imports.extend(ex.check_imports.iter().cloned());
        }
        let mut s = String::from(super::ta_abstract_c::LICENSE);
        s.push_str("\n/* Generated by ta_codegen from the TSDoc examples \u{2014} do not edit. */\n\n");
        s.push_str("import assert from 'node:assert/strict';\nimport { test } from 'node:test';\n");
        let _ = writeln!(s, "{}", import_line(&imports));
        for ex in &exs {
            let _ = writeln!(s, "\ntest('{}', () => {{", ex.title);
            for line in ex.body.iter().chain(std::iter::once(&String::new())).chain(&ex.checks) {
                if line.is_empty() {
                    s.push('\n');
                } else {
                    let _ = writeln!(s, "  {line}");
                }
            }
            s.push_str("});\n");
        }
        files.insert(file, s);
    }
    files
}

/// Writes the example tests into `dir` and removes anything else there:
/// `generate` owns the directory.
#[allow(clippy::implicit_hasher)]
pub fn write_example_tests(funcs: &[FuncDef], enums: &HashMap<String, EnumDef>, dir: &Path) {
    let files = render_example_tests(funcs, enums);
    std::fs::create_dir_all(dir).unwrap();
    for entry in std::fs::read_dir(dir).unwrap().flatten() {
        let name = entry.file_name().to_string_lossy().to_string();
        if !files.contains_key(&name) {
            let path = entry.path();
            if path.is_dir() {
                std::fs::remove_dir_all(&path).unwrap();
            } else {
                std::fs::remove_file(&path).unwrap();
            }
        }
    }
    for (name, text) in &files {
        crate::emit::write_if_changed(dir.join(name), text).unwrap();
    }
    println!("  TS example tests -> {}", dir.display());
}

#[cfg(test)]
mod tests {
    use super::{starts_block, tsdoc, wrap};

    #[test]
    fn tsdoc_escapes_tags_and_comment_ends_but_not_code() {
        assert_eq!(tsdoc("mail a@b, `x@y`"), "mail a\\@b, `x@y`");
        assert_eq!(tsdoc("a */ b"), "a *\u{200b}/ b");
        assert_eq!(tsdoc("close<open and in[i]"), "close\\<open and in\\[i]");
        assert_eq!(tsdoc("the [`SMA`](/functions/sma) page"), "the [`SMA`](https://ta-lib.org/functions/sma) page");
        assert_eq!(tsdoc("**bold** and *it*"), "**bold** and *it*");
    }

    #[test]
    fn wrap_keeps_code_spans_and_block_markers_off_line_starts() {
        let lines = wrap("aaaa `b c d` e", 6);
        assert_eq!(lines, vec!["aaaa", "`b c d`", "e"]);
        let lines = wrap("aaaa - bbbb", 5);
        assert_eq!(lines, vec!["aaaa -", "bbbb"]);
        assert!(starts_block("1."));
        assert!(starts_block("12)"));
        assert!(starts_block("---"));
        assert!(!starts_block("-1"));
        assert!(!starts_block("1.5"));
    }
}
