//! Go doc comments for the generated package, from each function's canonical
//! `<name>.md` and its YAML.
//!
//! A Go doc comment is plain text: Markdown emphasis, backticks and inline links
//! would reach `go doc` and pkg.go.dev as punctuation, so [`plain`] removes them,
//! turns a link to another function's page into a `[Core.X]` doc link and spells
//! the C names in the prose as the Go API does. gofmt rewrites a doc comment that
//! is not in its canonical form and `generate` never runs gofmt, so every block is
//! emitted canonical: no indented paragraph line (a code block), lists as
//! `//   - item`, a blank line after each list, and no one-line paragraph without
//! punctuation (gofmt makes it a heading).

use std::collections::HashMap;

use super::doc_meta::{self, ensure_period, RangeMeta};
use super::go::go_names;
use super::go_stream::go_stream_names;
use crate::ir::{DocDef, EnumDef, FuncDef, OptInput, Output, ParamType};
use crate::registry::Registry;
use crate::streaming;

/// Text columns after `// `.
const WRAP: usize = 77;

/// One doc comment's text lines; an empty line is a blank `//`.
struct Block {
    lines: Vec<String>,
}

impl Block {
    fn new() -> Self {
        Block { lines: Vec::new() }
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

    /// A lead-in line and its list, closed by a blank line as gofmt closes it.
    fn list(&mut self, lead: &str, items: &[String]) {
        self.para(lead);
        for item in items {
            for (i, line) in wrap(item, WRAP - 4).into_iter().enumerate() {
                self.lines.push(if i == 0 { format!("  - {line}") } else { format!("    {line}") });
            }
        }
        self.lines.push(String::new());
    }

    /// `\n// line` per line, the form the emitters place above a declaration.
    fn render(&self) -> String {
        let mut lines = self.lines.as_slice();
        while let [rest @ .., last] = lines {
            if !last.is_empty() {
                break;
            }
            lines = rest;
        }
        let mut s = String::new();
        for l in lines {
            s.push_str(if l.is_empty() { "\n//" } else { "\n// " });
            s.push_str(l);
        }
        s
    }
}

fn wrap(text: &str, width: usize) -> Vec<String> {
    let mut lines: Vec<String> = Vec::new();
    let mut line = String::new();
    for w in text.split_whitespace() {
        if !line.is_empty() && line.chars().count() + 1 + w.chars().count() > width {
            lines.push(std::mem::take(&mut line));
        }
        if !line.is_empty() {
            line.push(' ');
        }
        line.push_str(w);
    }
    if !line.is_empty() {
        lines.push(line);
    }
    lines
}

// ---------------------------------------------------------------------------
// Prose
// ---------------------------------------------------------------------------

fn doc_of(func: &FuncDef) -> &DocDef {
    static EMPTY: std::sync::OnceLock<DocDef> = std::sync::OnceLock::new();
    func.doc.as_ref().unwrap_or_else(|| EMPTY.get_or_init(DocDef::default))
}

/// RetCode spellings: C's enumerator, then the Go constant.
const RETCODES: &[(&str, &str)] = &[
    ("SUCCESS", "Success"),
    ("BAD_PARAM", "BadParam"),
    ("ALLOC_ERR", "AllocErr"),
    ("OUT_OF_RANGE_START_INDEX", "OutOfRangeStartIndex"),
    ("OUT_OF_RANGE_END_INDEX", "OutOfRangeEndIndex"),
    ("INSUFFICIENT_HISTORY", "InsufficientHistory"),
    ("INTERNAL_ERROR", "InternalError"),
];

/// C spellings in the shared `.md` prose, in the Go API's terms. A `TA_` name the
/// package has no counterpart for stays as written.
fn go_terms(text: &str, registry: &Registry) -> String {
    use regex::Regex;
    use std::sync::OnceLock;
    static TA: OnceLock<Regex> = OnceLock::new();
    static META: OnceLock<Regex> = OnceLock::new();
    let ta = TA.get_or_init(|| Regex::new(r"\bTA_([A-Za-z][A-Za-z0-9_]*)\b").expect("regex"));
    let meta = META.get_or_init(|| Regex::new(r"\bout(BegIdx|NBElement)\b").expect("regex"));
    let text = meta.replace_all(text, "OutRange.$1");
    ta.replace_all(&text, |c: &regex::Captures| {
        let rest = &c[1];
        if let Some(m) = rest.strip_prefix("MAType_") {
            return format!("MAType{m}");
        }
        if let Some(m) = rest.strip_prefix("FUNC_UNST_") {
            return format!("FuncUnstId{m}");
        }
        let fixed = match rest {
            "SetUnstablePeriod" => Some("Core.SetUnstablePeriod"),
            "GetUnstablePeriod" => Some("Core.UnstablePeriod"),
            "SetCandleSettings" => Some("Core.SetCandleSetting"),
            "RestoreCandleDefaultSettings" => Some("Core.RestoreCandleDefault"),
            "INTEGER_DEFAULT" => Some("IntegerDefault"),
            "REAL_DEFAULT" => Some("RealDefault"),
            "REAL_MIN" => Some("RealMin"),
            "REAL_MAX" => Some("RealMax"),
            "INTEGER_MIN" => Some("IntegerMin"),
            "INTEGER_MAX" => Some("IntegerMax"),
            _ => RETCODES.iter().find(|(c, _)| *c == rest).map(|(_, g)| *g),
        };
        if let Some(go) = fixed {
            return go.to_string();
        }
        if let Some(func) = rest.strip_suffix("_Lookback").filter(|f| registry.contains(&f.to_lowercase())) {
            return format!("Core.{}", go_names(&registry.name_of(&func.to_lowercase())).lookback_public);
        }
        if registry.contains(&rest.to_lowercase()) {
            return format!("Core.{}", registry.name_of(&rest.to_lowercase()));
        }
        c[0].to_string()
    })
    .into_owned()
}

/// Canonical Markdown as Go doc text: code spans and emphasis lose their
/// delimiters, a link to a function's page becomes a doc link to its method and
/// any other link reads `label (URL)`.
fn plain(text: &str, registry: &Registry) -> String {
    let text = go_terms(text, registry);
    let chars: Vec<char> = text.chars().collect();
    let mut out = String::new();
    let mut in_code = false;
    let mut emphasis: Option<usize> = None;
    let mut i = 0;
    while i < chars.len() {
        let c = chars[i];
        if !in_code {
            if let Some(len) = emphasis {
                if super::common::asterisks_at(&chars, i, len) {
                    emphasis = None;
                    i += len;
                    continue;
                }
            } else if let Some(len) = super::common::emphasis_open(&chars, i) {
                emphasis = Some(len);
                i += len;
                continue;
            }
        }
        match c {
            '`' => in_code = !in_code,
            '[' if !in_code => {
                if let Some((label, dest, end)) = super::java_doc::inline_link(&chars, i) {
                    let page = dest
                        .strip_prefix("https://ta-lib.org/functions/")
                        .map(|s| s.trim_end_matches('/').trim_end_matches(".md").to_lowercase())
                        .filter(|key| registry.contains(key));
                    match page {
                        Some(key) => out.push_str(&format!("[Core.{}]", registry.name_of(&key))),
                        None => out.push_str(&format!("{} ({dest})", plain(&label, registry))),
                    }
                    i = end;
                    continue;
                }
                out.push('[');
            }
            '\n' => out.push(' '),
            _ => out.push(c),
        }
        i += 1;
    }
    out
}

/// The summary's first paragraph: a tooltip, the page holds the rest.
fn summary_text(func: &FuncDef, registry: &Registry) -> String {
    let doc = doc_of(func);
    let text = if doc.summary.is_empty() {
        func.description.as_deref().or(func.hint.as_deref()).unwrap_or(&func.group)
    } else {
        doc.summary.split("\n\n").next().unwrap_or_default()
    };
    ensure_period(&plain(text, registry))
}

fn input_desc(name: &str, registry: &Registry, doc: &DocDef) -> String {
    if let Some((_, desc)) = doc.inputs.iter().find(|(n, _)| n == name) {
        return ensure_period(&plain(desc, registry));
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

fn output_desc(output: &Output, registry: &Registry, doc: &DocDef) -> String {
    doc.outputs
        .iter()
        .find(|(n, _)| n == &output.name)
        .map_or_else(|| "Output values.".to_string(), |(_, d)| ensure_period(&plain(d, registry)))
}

/// An optional parameter's prose, with its default and range from the YAML.
fn param_doc(opt: &OptInput, registry: &Registry, doc: &DocDef, enums: &HashMap<String, EnumDef>) -> String {
    let desc = doc
        .params
        .iter()
        .find(|(n, _)| n == &opt.name)
        .map(|(_, d)| plain(d, registry))
        .or_else(|| opt.hint.clone())
        .or_else(|| opt.display_name.clone())
        .unwrap_or_else(|| "Optional parameter".to_string());
    let m = doc_meta::param_meta(opt, enums);
    let mut facts: Vec<String> = Vec::new();
    if let ParamType::Enum(name) = &opt.param_type {
        if let Some(variant) = &m.default_variant {
            facts.push(format!("Default {name}{variant}"));
        }
        if let Some(v) = super::common::enum_default_variant(enums, name) {
            facts.push(format!("{name}{} selects the default", v.name));
        }
    } else {
        if let Some(d) = &m.default {
            facts.push(format!("Default {d}"));
        }
        match &m.range {
            RangeMeta::Bounded(lo, hi) => facts.push(format!("range {lo} to {hi}")),
            RangeMeta::Min(lo) => facts.push(format!("minimum {lo}")),
            RangeMeta::Max(hi) => facts.push(format!("maximum {hi}")),
            RangeMeta::Unbounded => {}
        }
        if opt.default.is_some() {
            facts.push(if opt.param_type == ParamType::Real {
                "RealDefault selects the default".to_string()
            } else {
                "IntegerDefault selects the default".to_string()
            });
        }
    }
    let head = format!("{}: {}", opt.name, ensure_period(&desc));
    if facts.is_empty() {
        head
    } else {
        format!("{head} {}.", facts.join("; "))
    }
}

/// `a`, `a and b`, `a, b and c`.
fn and_list(items: &[String]) -> String {
    joined(items, "and")
}

/// `a`, `a or b`, `a, b or c`.
fn or_list(items: &[String]) -> String {
    joined(items, "or")
}

fn joined(items: &[String], word: &str) -> String {
    match items {
        [] => String::new(),
        [one] => one.clone(),
        [rest @ .., last] => format!("{} {word} {last}", rest.join(", ")),
    }
}

fn unstable_sentence(func: &FuncDef, enums: &HashMap<String, EnumDef>) -> Option<String> {
    if !func.flags.iter().any(|f| f == "unstable_period") {
        return None;
    }
    let id = enums
        .get("FuncUnstId")
        .and_then(|e| e.variants.iter().find(|v| v.name == func.name && !v.reserved))
        .map_or_else(|| "this function".to_string(), |v| format!("FuncUnstId{}", v.name));
    Some(format!("The lookback includes the unstable period the Core holds for {id} (see [Core.SetUnstablePeriod])."))
}

// ---------------------------------------------------------------------------
// Batch tier
// ---------------------------------------------------------------------------

fn first_sentence(func: &FuncDef) -> String {
    let name = &func.name;
    let hint = func.hint.as_deref().map_or_else(|| name.clone(), |h| h.trim_end_matches('.').to_string());
    if func.is_candlestick() {
        let pattern = hint.strip_suffix(" Pattern").unwrap_or(&hint);
        format!("{name} recognizes the {pattern} pattern over bars startIdx through endIdx.")
    } else {
        format!("{name} computes the {hint} over bars startIdx through endIdx.")
    }
}

/// The public batch method.
#[allow(clippy::implicit_hasher)]
#[must_use]
pub fn batch_docs(func: &FuncDef, enums: &HashMap<String, EnumDef>, registry: &Registry) -> String {
    let doc = doc_of(func);
    let names = go_names(&func.name);
    let lookback = format!("[Core.{}]", names.lookback_public);
    let mut b = Block::new();
    b.para(&first_sentence(func));
    b.para(&summary_text(func, registry));
    b.para(&format!("Formula and more info at {}.", doc_meta::function_page_url(&func.name)));
    let inputs: Vec<String> =
        func.inputs.iter().map(|i| format!("{}: {}", i.name, input_desc(&i.name, registry, doc))).collect();
    b.list("Inputs, each at least endIdx+1 long:", &inputs);
    if !func.optional_inputs.is_empty() {
        let params: Vec<String> = func.optional_inputs.iter().map(|o| param_doc(o, registry, doc, enums)).collect();
        b.list("Parameters:", &params);
    }
    let outputs: Vec<String> =
        func.outputs.iter().map(|o| format!("{}: {}", o.name, output_desc(o, registry, doc))).collect();
    b.list("Outputs, each at least as long as the values written (the input length always suffices):", &outputs);
    b.para(&format!(
        "Element k of each output is the value at bar BegIdx+k of the returned [OutRange], and BegIdx is at \
         least {lookback}: a range that ends before it returns NBElement 0."
    ));
    if let Some(s) = unstable_sentence(func, enums) {
        b.para(&s);
    }
    let real_out = func.outputs.iter().any(|o| o.param_type == ParamType::Real);
    let real_in = func.inputs.iter().any(|i| i.param_type == ParamType::Real);
    let sharing = if real_out && real_in {
        "A real output may be one of the inputs, the same slice, to compute in place; outputs may not otherwise \
         share memory with an input or with each other."
    } else if func.outputs.len() > 1 {
        "Outputs may not share memory with each other."
    } else {
        ""
    };
    if !sharing.is_empty() {
        b.para(sharing);
    }
    let mut causes = Vec::new();
    if !func.optional_inputs.is_empty() {
        causes.push("a parameter out of range".to_string());
    }
    causes.push("an input too short".to_string());
    causes.push("an output empty or too short".to_string());
    if !sharing.is_empty() {
        causes.push("memory shared otherwise".to_string());
    }
    let mut errors = format!(
        "The error is [OutOfRangeStartIndex] or [OutOfRangeEndIndex] for a range outside 0 to [IndexMax] or \
         ending before it starts, and [BadParam] for {}.",
        or_list(&causes)
    );
    if !super::go::batch_overlap_checks(func).is_empty() {
        errors.push_str(CLIPPED);
    }
    b.para(&errors);
    b.render()
}

/// The limit of the runtime's `overlaps`, said wherever a doc promises the check.
const CLIPPED: &str = " A slice whose capacity a three-index expression cut, as slices.Clip does, can escape the \
                       sharing check.";

/// `<N>Lookback`.
#[allow(clippy::implicit_hasher)]
#[must_use]
pub fn lookback_docs(func: &FuncDef, enums: &HashMap<String, EnumDef>) -> String {
    let names = go_names(&func.name);
    let mut b = Block::new();
    b.para(&format!(
        "{} returns how many leading bars [Core.{}] consumes before its first value: called with startIdx 0, \
         its first value is at that bar.",
        names.lookback_public, names.public
    ));
    if let Some(s) = unstable_sentence(func, enums) {
        b.para(&s);
    }
    if !func.optional_inputs.is_empty() {
        b.para(&format!(
            "It takes the parameters of [Core.{}] and returns [BadParam] for one out of range.",
            names.public
        ));
    }
    b.render()
}

/// `<N>DisplayShift`.
#[must_use]
pub fn display_shift_docs(func: &FuncDef) -> String {
    let names = go_names(&func.name);
    let idx = crate::ir::DISPLAY_SHIFT_INDEX_PARAM;
    let mut b = Block::new();
    b.para(&format!(
        "{} returns how many bars ahead (positive) or behind (negative) of the bar that computed it a chart \
         draws output {idx} of [Core.{}]. The values are never shifted.",
        names.display_shift_public, names.public
    ));
    if !func.has_display_shift() {
        b.para("Every output of this function is drawn at its own bar, so the answer is 0.");
    }
    let lead = if func.optional_inputs.is_empty() {
        format!("It takes the output's position among the outputs of [Core.{}], from 0", names.public)
    } else {
        format!(
            "It takes the parameters of [Core.{}] followed by the output's position among its outputs, from 0",
            names.public
        )
    };
    let rejects = if func.optional_inputs.is_empty() {
        "returns [BadParam] for an index that names no output".to_string()
    } else {
        format!(
            "returns [BadParam] for a parameter [Core.{}] rejects or an index that names no output",
            names.lookback_public
        )
    };
    b.para(&format!("{lead}, and {rejects}."));
    b.render()
}

// ---------------------------------------------------------------------------
// Streaming tier
// ---------------------------------------------------------------------------

fn bar_inputs(func: &FuncDef) -> Vec<String> {
    streaming::input_array_names(func).iter().map(|n| super::go::go_ident(n)).collect()
}

/// The outputs in the order a stream method returns them.
fn returned_outputs(func: &FuncDef) -> String {
    and_list(&func.outputs.iter().map(|o| super::go::go_ident(&o.name)).collect::<Vec<_>>())
}

/// `<N>Open`.
#[must_use]
pub fn open_docs(func: &FuncDef) -> String {
    let names = go_names(&func.name);
    let st = go_stream_names(&func.name);
    let bars = bar_inputs(func);
    let multi = func.outputs.len() > 1;
    let mut b = Block::new();
    let values = if multi {
        format!("its values at the last history bar ({}), the values", returned_outputs(func))
    } else {
        "its value at the last history bar, the value".to_string()
    };
    b.para(&format!(
        "{} opens a live [{}] on a warm-up history, oldest bar first, and returns it with {values} [Core.{}] \
         writes there.",
        st.open, st.stream, names.public
    ));
    let mut what = if bars.len() > 1 {
        format!("{} are the histories, of one length", and_list(&bars))
    } else {
        format!("{} is the history", bars.join(""))
    };
    if !func.optional_inputs.is_empty() {
        what.push_str(&format!("; the parameters are those of [Core.{}]", names.public));
    }
    b.para(&format!("{what}."));
    let mut causes = Vec::new();
    if !func.optional_inputs.is_empty() {
        causes.push("a parameter out of range".to_string());
    }
    if bars.len() > 1 {
        causes.push("histories of different lengths".to_string());
    }
    let bad = if causes.is_empty() { String::new() } else { format!(", and [BadParam] for {}", or_list(&causes)) };
    let ceiling = if causes.is_empty() { " and" } else { "," };
    b.para(&format!(
        "The error is [InsufficientHistory] for a history of at most [Core.{}] bars{ceiling} \
         [OutOfRangeEndIndex] for one longer than [IndexMax]+1 bars{bad}.",
        names.lookback_public
    ));
    b.render()
}

/// `<N>OpenAndFill`.
#[must_use]
pub fn open_and_fill_docs(func: &FuncDef) -> String {
    let names = go_names(&func.name);
    let st = go_stream_names(&func.name);
    let bars = bar_inputs(func);
    let history = bars.first().cloned().unwrap_or_default();
    let mut b = Block::new();
    b.para(&format!(
        "{} opens a stream as [Core.{}] does and also writes the outputs over the whole history, as \
         [Core.{}] writes them for bars 0 through len({history})-1, returning that range.",
        st.open_and_fill, st.open, names.public
    ));
    b.para(&format!(
        "Each output must hold len({history}) minus [Core.{}] values and share no memory with an input or \
         another output.",
        names.lookback_public
    ));
    let mut errors = format!(
        "The errors are those of [Core.{}], and [BadParam] for an output that is empty, too short or sharing \
         memory.",
        st.open
    );
    if !super::go_stream::alias_pairs(func).is_empty() {
        errors.push_str(CLIPPED);
    }
    b.para(&errors);
    b.render()
}

/// The `<N>Stream` type.
#[must_use]
pub fn stream_type_docs(func: &FuncDef) -> String {
    let names = go_names(&func.name);
    let st = go_stream_names(&func.name);
    let what = if func.outputs.len() > 1 { "values" } else { "value" };
    let mut b = Block::new();
    b.para(&format!(
        "{0} is a live {1} over a growing series: [{0}.Update] takes each new closed bar and returns the {what} \
         [Core.{1}] computes at that bar over the same series. Open one with [Core.{2}] or [Core.{3}].",
        st.stream, names.public, st.open, st.open_and_fill
    ));
    b.para(&format!("A stream belongs to one goroutine; [{}.Clone] forks it.", st.stream));
    b.render()
}

/// One exported method of `<N>Stream`.
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum StreamMethod {
    OutRange,
    Advance,
    Update,
    Peek,
    Value,
    Clone,
}

const CEILING: &str = "[OutOfRangeEndIndex] once OutRange reaches bar [IndexMax]";

/// A method of `<N>Stream`.
#[must_use]
pub fn stream_method_docs(func: &FuncDef, method: StreamMethod) -> String {
    let names = go_names(&func.name);
    let bars = bar_inputs(func);
    let multi = func.outputs.len() > 1;
    let bar = match bars.as_slice() {
        [] => "one closed bar".to_string(),
        [one] => format!("the closed bar {one}"),
        many => format!("one closed bar ({})", and_list(many)),
    };
    let returns = if multi { format!("its outputs: {}", returned_outputs(func)) } else { "the new value".to_string() };
    let finite = if bars.is_empty() { None } else { Some("[BadParam] for a bar value that is not finite") };
    let text = match method {
        StreamMethod::OutRange => format!(
            "OutRange returns the bars the stream has a value for, as [Core.{}] reports them over the same bars; \
             each Update and each Advance adds one to NBElement.",
            names.public
        ),
        StreamMethod::Advance => format!(
            "Advance counts one bar the stream was not fed: OutRange grows by one and Value keeps its previous \
             answer. It returns {CEILING}."
        ),
        StreamMethod::Update => {
            let errs = match finite {
                Some(f) => format!("It returns {f} and {CEILING}, both before anything changes."),
                None => format!("It returns {CEILING}, before anything changes."),
            };
            format!("Update commits {bar} and returns {returns}. {errs}")
        }
        StreamMethod::Peek => {
            let bar = match bars.as_slice() {
                [] => "one bar".to_string(),
                [one] => format!("the bar {one}"),
                many => format!("the bar ({})", and_list(many)),
            };
            let errs = finite.map_or_else(String::new, |f| format!(" It returns {f}."));
            format!("Peek returns what Update would return for {bar}, changing nothing.{errs}")
        }
        StreamMethod::Value => {
            if multi {
                "Value returns the outputs at the last bar of OutRange, in Update's order; Peek does not change \
                 them."
                    .to_string()
            } else {
                "Value returns the value at the last bar of OutRange; Peek does not change it.".to_string()
            }
        }
        StreamMethod::Clone => "Clone returns an independent copy of the stream at the same bar: the two evolve \
                                separately and share only the [Core]."
            .to_string(),
    };
    let mut b = Block::new();
    b.para(&text);
    b.render()
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn a_list_closes_with_a_blank_line_and_trailing_blanks_go() {
        let mut b = Block::new();
        b.para("X does things.");
        b.list("Inputs:", &["inReal: a series.".to_string()]);
        assert_eq!(b.render(), "\n// X does things.\n//\n// Inputs:\n//   - inReal: a series.");
        b.para("More.");
        assert_eq!(b.render(), "\n// X does things.\n//\n// Inputs:\n//   - inReal: a series.\n//\n// More.");
    }

    #[test]
    fn wrap_breaks_at_the_width_and_never_indents() {
        let lines = wrap("aaa bbb ccc ddd", 7);
        assert_eq!(lines, ["aaa bbb", "ccc ddd"]);
        assert!(wrap("  lead  ", 10).iter().all(|l| !l.starts_with(' ')));
    }
}
