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
    let first = ts::result_key(func, &func.outputs[0]);
    b.para(&format!(
        "`{first}[k]` is the value at input bar `begIdx + k`; every output is exactly as long as the \
         number of values written, and nothing is padded. A range that ends before {{@link {}}} is a \
         success with empty outputs.",
        names.lookback
    ));
    b.blank();
    api_input_params(&mut b, func, doc, "");
    b.param(
        "options",
        &format!(
            "Parameters, the range `startIdx` to `endIdx` (inclusive), `core` and output buffers, \
             each optional; see {{@link {}}}.",
            names.options_type
        ),
    );
    b.tag(
        "returns",
        &format!(
            "`begIdx` and the values; see {{@link {}}}. Each output is a view of its buffer, the \
             caller's or a new one.",
            names.result_type
        ),
    );
    let mut causes = vec!["options that are not an object", "a `core` that is not a `Core`", "an absent input", "a range bound that is not an int32"];
    if func.optional_inputs.iter().any(|o| matches!(o.param_type, ParamType::Integer | ParamType::Enum(_))) {
        causes.push("an integer parameter that is not an int32");
    }
    if has_real_param(func) {
        causes.push("a real parameter that is not a number");
    }
    if !func.optional_inputs.is_empty() {
        causes.push("a parameter outside its range");
    }
    causes.extend(["an input too short for the range", "a buffer that is empty, too short or of the wrong type"]);
    if ts::shared_output_condition(func).is_some() {
        causes.push("two buffers sharing memory");
    }
    if !func.inputs.is_empty() {
        causes.push("a buffer sharing memory with a `Float64Array` input it is not");
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

/// `@param` lines for the idiomatic API's input parameters; `lead` prefixes
/// each text.
fn api_input_params(b: &mut Block, func: &FuncDef, doc: &DocDef, lead: &str) {
    for a in ts::api_inputs(func) {
        if a.is_price {
            let parts: Vec<String> = a
                .legs
                .iter()
                .map(|(i, expr)| format!("`{}`: {}", &expr[ts::PRICES_PARAM.len() + 1..], input_desc(&i.name, doc)))
                .collect();
            b.param(&a.name, &format!("{lead}One series per price component this function reads, all of one length; other components may be present and are ignored. {}", parts.join(" ")));
        } else {
            b.param(&a.name, &format!("{lead}{}", input_desc(&a.legs[0].0.name, doc)));
        }
    }
}

/// The `<N>Options` interface.
#[must_use]
pub fn options_docs(func: &FuncDef) -> String {
    let names = ts_names(&func.name);
    let mut calls = vec![names.public.clone(), names.lookback.clone(), names.display_shift.clone()];
    if func.streaming {
        calls.extend([names.open.clone(), names.open_and_fill.clone()]);
    }
    let links: Vec<String> = calls.iter().map(|c| format!("{{@link {c}}}")).collect();
    let refs: Vec<&str> = links.iter().map(String::as_str).collect();
    let mut b = Block::new("");
    b.para(&format!(
        "Options of {}. Every key is optional: an omitted or `undefined` key takes its default, and a \
         call ignores the keys it has no use for.",
        any_and(&refs)
    ));
    b.render()
}

/// The `<N>Result` interface.
#[must_use]
pub fn result_docs(func: &FuncDef) -> String {
    let names = ts_names(&func.name);
    let mut b = Block::new("");
    if func.streaming {
        b.para(&format!(
            "What {{@link {}}} returns, and {{@link {}}} with its stream.",
            names.public, names.open_and_fill
        ));
    } else {
        b.para(&format!("What {{@link {}}} returns.", names.public));
    }
    b.render()
}

/// A member of an interface, indented one level.
#[must_use]
pub fn member(text: &str) -> String {
    let mut b = Block::new("  ");
    b.para(text);
    b.render()
}

/// An optional parameter's options key.
#[allow(clippy::implicit_hasher)]
#[must_use]
pub fn param_text(func: &FuncDef, opt: &OptInput, enums: &HashMap<String, EnumDef>) -> String {
    param_doc(opt, doc_of(func), enums)
}

pub const START_IDX_KEY: &str =
    "First bar of the requested range (default 0). Bars before it are still read as warm-up. Batch calls only.";
pub const END_IDX_KEY: &str =
    "Last bar of the requested range, inclusive (default the last bar of the first input). Batch calls only.";
pub const CORE_KEY: &str = CORE_PARAM;
pub const BEG_IDX_FIELD: &str = "Input bar of the first value; 0 when there is none.";

/// An output buffer's options key.
#[must_use]
pub fn buffer_text(func: &FuncDef, out: &Output) -> String {
    let in_place = if out.param_type == ParamType::Real {
        "; a batch call may compute in place over a `Float64Array` input of the same length"
    } else {
        ""
    };
    format!(
        "Buffer for `{}`, written from index 0 and allocated when omitted. It must hold the values \
         produced (for a batch call the input length always suffices, for an `OpenAndFill` \
         `historyLen - lookback`), and share no memory with another buffer or with an input{in_place}.",
        ts::result_key(func, out)
    )
}

/// An output's field of `<N>Result`.
#[must_use]
pub fn result_field_text(func: &FuncDef, out: &Output) -> String {
    let key = ts::result_key(func, out);
    let desc = output_desc(out, doc_of(func));
    let stop = if desc.ends_with(['.', '!', '?']) { "" } else { "." };
    format!("{desc}{stop} `{key}[k]` is the value at input bar `begIdx + k`.")
}

const OPTIONS_IGNORED: &str = "other keys are ignored";

/// `<n>Lookback`.
#[must_use]
pub fn lookback_docs(func: &FuncDef) -> String {
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
    b.param(
        "options",
        &format!("The parameters and `core`; {OPTIONS_IGNORED}. See {{@link {}}}.", names.options_type),
    );
    b.tag("returns", "The lookback, or `Core.LOOKBACK_INVALID` (-1) if a parameter is out of range.");
    b.tag("throws", OPTIONS_THROW);
    b.render()
}

/// Lookback and display shift answer a rejected parameter with a value, but
/// not a caller error in the call's shape.
const OPTIONS_THROW: &str =
    "`TALibError` with code `TA_BAD_PARAM` when `options` is not an object or `core` is not a `Core`.";

/// `<n>DisplayShift`.
#[must_use]
pub fn display_shift_docs(func: &FuncDef) -> String {
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
    b.param(crate::ir::DISPLAY_SHIFT_INDEX_PARAM, "Position of the output among the result's outputs, from 0.");
    b.param(
        "options",
        &format!("The parameters and `core`; {OPTIONS_IGNORED}. See {{@link {}}}.", names.options_type),
    );
    b.tag(
        "returns",
        "The display shift, or `Core.DISPLAY_SHIFT_INVALID` (-2147483648) if a parameter is out of \
         range or the index names no output.",
    );
    b.tag("throws", OPTIONS_THROW);
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
    api_input_params(&mut b, func, doc, "Warm-up history. ");
    b.param(
        "options",
        &format!("The parameters and `core`; {OPTIONS_IGNORED}. See {{@link {}}}.", names.options_type),
    );
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
        "{{@link {}}} that also computes the outputs over the whole history in the same pass, \
         bit-identically to {{@link {}}}.",
        names.open, names.public
    ));
    b.blank();
    api_input_params(&mut b, func, doc, "Warm-up history. ");
    b.param(
        "options",
        &format!(
            "The parameters, `core` and output buffers; {OPTIONS_IGNORED}. See {{@link {}}}.",
            names.options_type
        ),
    );
    b.tag(
        "returns",
        &format!(
            "The stream, positioned on the last history bar, with `begIdx` and the values over the \
             history as {{@link {}}} returns them; the range is the stream's `outRange()`.",
            names.public
        ),
    );
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
                (true, true) => "Commits one closed bar and returns its outputs.",
                (false, _) => "What the next `update` with the same bar would return, committing nothing.",
            });
            b.blank();
            for bar in streaming::input_array_names(func) {
                let name = ts::input_param_name(&bar);
                b.param(&name, &format!("The bar's `{name}` value: {}", input_desc(&bar, doc)));
            }
            if multi {
                b.param("out", &sink_param(&names));
                b.tag("returns", if update { "`out`, or the new sink." } else { "`out`, or the new sink, as `update` would fill it." });
            } else {
                b.tag("returns", if update { "The new value." } else { "The value `update` would return." });
            }
            let sink = if multi { SINK_REJECTED } else { "" };
            if update {
                b.tag(
                    "throws",
                    &format!(
                        "`TALibError` with code `TA_BAD_PARAM` for a bar value that is not finite{sink}, \
                         before anything moves. {CEILING_THROWS}"
                    ),
                );
            } else {
                b.tag("throws", &format!("`TALibError` with code `TA_BAD_PARAM` for a bar value that is not finite{sink}."));
            }
        }
        StreamMethod::Value => {
            if multi {
                b.para("The outputs at the last bar this stream counted; `peek` does not change them.");
                b.blank();
                b.param("out", &sink_param(&names));
                b.tag("returns", "`out`, or the new sink.");
                b.tag("throws", &format!("`TALibError` with code `TA_BAD_PARAM` for{}.", SINK_REJECTED.trim_start_matches(", or")));
            } else {
                b.para("The value at the last bar this stream counted; `peek` does not change it.");
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

/// The sink clause of a multi-output method's `@throws`.
const SINK_REJECTED: &str = ", or a sink that is not an object, `null` included";

fn sink_param(names: &ts::TsNames) -> String {
    format!("Receives the outputs and is returned; a new {{@link {}}} when omitted.", names.out_class)
}

/// The `<N>Out` sink class of a multi-output stream.
#[must_use]
pub fn out_class_docs(func: &FuncDef) -> String {
    let mut b = Block::new("");
    b.para(&format!(
        "The outputs of one {} bar, as `update`, `peek` and `value` return them. Passed back to one \
         of them, it is filled and returned again, with no allocation; it holds its values until \
         the next call that writes it. A new one holds `NaN`.",
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

fn open_throws(func: &FuncDef, fill: bool) -> String {
    let names = ts_names(&func.name);
    let mut causes = vec!["options that are not an object", "a `core` that is not a `Core`", "an absent input"];
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
            "a buffer too short, of the wrong type or starting where another buffer or a `Float64Array` input \
             starts, or, on a history long enough to open, one sharing memory with another buffer or a \
             `Float64Array` input",
        );
    }
    let bad = format!(", and `TA_BAD_PARAM` for {}", any_of(&causes));
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

/// `a`, `a and b`, `a, b and c`.
fn any_and(items: &[&str]) -> String {
    match items {
        [] => String::new(),
        [one] => (*one).to_string(),
        [rest @ .., last] => format!("{} and {last}", rest.join(", ")),
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

/// The function names the prose mapper may spell in TypeScript, seeded by the
/// backend from the corpus it generates. Seeding only adds, so the set does not
/// depend on which caller seeded last; empty under a unit test that seeds nothing,
/// which then maps only the context-free forms.
static FUNCTION_NAMES: std::sync::RwLock<BTreeSet<String>> = std::sync::RwLock::new(BTreeSet::new());

pub fn seed_function_names<'a>(names: impl IntoIterator<Item = &'a str>) {
    FUNCTION_NAMES.write().expect("function names").extend(names.into_iter().map(str::to_string));
}

/// C spellings in the shared `.md` prose, in the TypeScript API's own terms:
/// `optInTimePeriod` is the option key `timePeriod`, `inHigh` the bar's `high`,
/// `outRealUpperBand` the result's `outUpperBand` and `outBegIdx` its `begIdx`
/// (a single output's `outReal` stays: it is also its buffer key), `TA_MAType_EMA`
/// is `MAType.EMA`, `TA_FUNC_UNST_EMA` is `FuncUnstId.EMA`, the settings calls are
/// the builder's, and a function `TA_MEDPRICE` (or its `_Lookback` /
/// `_DisplayShift`) is `medprice`. A `TA_` name that is none of these, a
/// `RetCode` for one, stays: it is the `code` a thrown error carries.
fn plain_terms(text: &str) -> String {
    use regex::Regex;
    use std::sync::OnceLock;
    static OPT: OnceLock<Regex> = OnceLock::new();
    static INP: OnceLock<Regex> = OnceLock::new();
    static OUT: OnceLock<Regex> = OnceLock::new();
    static TA: OnceLock<Regex> = OnceLock::new();
    let opt = OPT.get_or_init(|| Regex::new(r"\boptIn[A-Z][A-Za-z0-9_]*").expect("regex"));
    let inp = INP.get_or_init(|| Regex::new(r"\bin(Open|High|Low|Close|Volume|OpenInterest|Real[0-9]*|Periods)\b").expect("regex"));
    let out = OUT.get_or_init(|| Regex::new(r"\bout[A-Z][A-Za-z0-9_]*\b").expect("regex"));
    let ta = TA.get_or_init(|| Regex::new(r"\bTA_([A-Za-z][A-Za-z0-9_]*)\b").expect("regex"));
    let names = FUNCTION_NAMES.read().expect("function names");
    let text = opt.replace_all(text, |c: &regex::Captures| ts::param_key(&c[0]));
    let text = inp.replace_all(&text, |c: &regex::Captures| ts::input_param_name(&c[0]));
    let text = out.replace_all(&text, |c: &regex::Captures| if &c[0] == "outBegIdx" { "begIdx".to_string() } else { ts::output_key(&c[0]) });
    ta.replace_all(&text, |c: &regex::Captures| {
        let rest = &c[1];
        if let Some(m) = rest.strip_prefix("MAType_") {
            return format!("MAType.{m}");
        }
        if let Some(m) = rest.strip_prefix("FUNC_UNST_") {
            return format!("FuncUnstId.{m}");
        }
        match rest {
            "SetUnstablePeriod" => return "CoreBuilder.unstablePeriod".to_string(),
            "GetUnstablePeriod" => return "Core.unstablePeriod".to_string(),
            "SetCandleSettings" => return "CoreBuilder.candleSetting".to_string(),
            "RestoreCandleDefaultSettings" => return "CoreBuilder.restoreCandleDefault".to_string(),
            _ => {}
        }
        if let Some(func) = rest.strip_suffix("_Lookback").filter(|f| names.contains(*f)) {
            return ts::ts_names(func).lookback;
        }
        if let Some(func) = rest.strip_suffix("_DisplayShift").filter(|f| names.contains(*f)) {
            return ts::ts_names(func).display_shift;
        }
        if names.contains(rest) {
            return ts::func_ident(rest);
        }
        c[0].to_string()
    })
    .into_owned()
}

/// Canonical Markdown made safe inside a `/** */` block: rustdoc's escaping
/// (TSDoc renders the same Markdown), plus `@` outside code spans, which TSDoc
/// would read as a tag at the start of a wrapped line.
#[must_use]
pub fn tsdoc(text: &str) -> String {
    let escaped = rust_doc::escape_prose(&plain_terms(text));
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
fn series(func: &FuncDef, input: &str) -> Option<(&'static str, String, String)> {
    if let Some(domain) = rust_doc::input_domain(func, input) {
        let expr = rust_doc::domain_series(domain, "Math.sin(0.1 * i)", doc_meta::fmt_real);
        return Some(("data", expr, doc_meta::fmt_real(rust_doc::domain_bar(domain, 0.42))));
    }
    let (var, expr, bar) = match input {
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
    };
    Some((var, expr.to_string(), bar.to_string()))
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
    series: Vec<(&'static str, String)>,
    lines: Vec<String>,
    opts: Vec<String>,
    imports: BTreeSet<String>,
}

fn setup(func: &FuncDef, inputs: &[String], enums: &HashMap<String, EnumDef>) -> Option<Setup> {
    let mut s = Setup { series: Vec::new(), lines: Vec::new(), opts: Vec::new(), imports: BTreeSet::new() };
    for input in inputs {
        let (var, expr, bar) = series(func, input)?;
        s.lines.extend(series_lines(var, &expr));
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

/// The idiomatic call's input arguments over the example series: a price
/// group as one object of its series.
fn api_args(func: &FuncDef, s: &Setup) -> Vec<String> {
    let names: Vec<String> = func.inputs.iter().map(|i| i.name.clone()).collect();
    let var_of = |name: &str| s.series[names.iter().position(|n| n == name).expect("a series per input")].0;
    ts::api_inputs(func)
        .iter()
        .map(|a| {
            if a.is_price {
                let fields: Vec<String> = a
                    .legs
                    .iter()
                    .map(|(i, expr)| {
                        let key = &expr[ts::PRICES_PARAM.len() + 1..];
                        let var = var_of(&i.name);
                        if var == key { key.to_string() } else { format!("{key}: {var}") }
                    })
                    .collect();
                format!("{{ {} }}", fields.join(", "))
            } else {
                var_of(&a.legs[0].0.name).to_string()
            }
        })
        .collect()
}

/// `{lead}name(inputs.., { key: value, .. });`, the options object broken one
/// key per line when the call is long.
fn api_call(lead: &str, name: &str, inputs: &[String], opts: &[(String, String)]) -> Vec<String> {
    let fields: Vec<String> = opts.iter().map(|(k, v)| format!("{k}: {v}")).collect();
    let mut args = inputs.to_vec();
    if !fields.is_empty() {
        args.push(format!("{{ {} }}", fields.join(", ")));
    }
    let one = format!("{lead}{name}({});", args.join(", "));
    if one.len() <= WRAP || fields.is_empty() {
        return vec![one];
    }
    let mut lead_inputs = inputs.join(", ");
    if !lead_inputs.is_empty() {
        lead_inputs.push_str(", ");
    }
    let mut lines = vec![format!("{lead}{name}({lead_inputs}{{")];
    lines.extend(fields.iter().map(|f| format!("  {f},")));
    lines.push("});".to_string());
    lines
}

/// `const { a, b } = `, or the pattern one name per line when it is long.
fn destructure(names: &[String]) -> (Vec<String>, String) {
    let one = format!("const {{ {} }} = ", names.join(", "));
    if one.len() <= WRAP / 2 + 8 {
        return (Vec::new(), one);
    }
    let mut pre = vec!["const {".to_string()];
    pre.extend(names.iter().map(|n| format!("  {n},")));
    (pre, "} = ".to_string())
}

fn example_opts(func: &FuncDef, s: &Setup) -> Vec<(String, String)> {
    func.optional_inputs.iter().zip(&s.opts).map(|(o, v)| (ts::param_key(&o.name), v.clone())).collect()
}

/// The result keys of `func`, `begIdx` first.
fn result_names(func: &FuncDef) -> Vec<String> {
    let mut names = vec!["begIdx".to_string()];
    names.extend(func.outputs.iter().map(|o| ts::result_key(func, o)));
    names
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
    let outs: Vec<String> = func.outputs.iter().map(|o| ts::result_key(func, o)).collect();

    let mut body = s.lines.clone();
    let (pre, lead) = destructure(&result_names(func));
    body.extend(pre);
    body.extend(api_call(&lead, &names.public, &api_args(func, &s), &example_opts(func, &s)));
    body.push(format!("// {}[k] is the value at bar begIdx + k", outs[0]));

    let mut checks = vec![
        format!("const count = {}.length;", outs[0]),
        "assert.ok(count > 0);".to_string(),
        format!("assert.equal(begIdx + count, {first}.length);"),
    ];
    for var in &outs[1..] {
        checks.push(format!("assert.equal({var}.length, count);"));
    }
    if let Some((_, var)) = func.outputs.iter().zip(&outs).find(|(o, _)| o.param_type != ParamType::Integer) {
        // A `nan_inf_output` function may emit infinity on ordinary data.
        let check = if func.flags.iter().any(|fl| fl == "nan_inf_output") {
            "(v) => !Number.isNaN(v)"
        } else {
            "Number.isFinite"
        };
        checks.push(format!("assert.ok({var}.every({check}));"));
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
    let inputs: Vec<String> = func.inputs.iter().map(|i| i.name.clone()).collect();
    assert_eq!(inputs, streaming::input_array_names(func), "{}: the bars are the batch inputs", func.name);
    let s = setup(func, &inputs, enums)?;
    let first = s.series[0].0;
    let mut imports = s.imports.clone();
    imports.insert(names.open.clone());
    let bars: Vec<&str> = s.series.iter().map(|(_, b)| b.as_str()).collect();

    let mut body = s.lines.clone();
    body.extend(api_call("const stream = ", &names.open, &api_args(func, &s), &example_opts(func, &s)));
    body.push(format!("const peeked = stream.peek({}); // a forming bar: commits nothing", bars.join(", ")));
    body.push(format!("const updated = stream.update({}); // the bar closes", bars.join(", ")));
    let mut checks = Vec::new();
    if func.outputs.len() > 1 {
        for out in &func.outputs {
            let f = ts::output_key(&out.name);
            checks.push(format!("assert.ok(Object.is(peeked.{f}, updated.{f}));"));
        }
    } else {
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

/// `<n>OpenAndFill` over the history; the checks hold the fill to the stream's
/// range and to the batch call.
#[allow(clippy::implicit_hasher)]
#[must_use]
pub fn fill_example(func: &FuncDef, enums: &HashMap<String, EnumDef>) -> Option<Example> {
    if !func.streaming {
        return None;
    }
    let names = ts_names(&func.name);
    let inputs: Vec<String> = func.inputs.iter().map(|i| i.name.clone()).collect();
    let s = setup(func, &inputs, enums)?;
    let mut imports = s.imports.clone();
    imports.insert(names.open_and_fill.clone());
    let check_imports: BTreeSet<String> = [names.public.clone()].into_iter().collect();
    let outs: Vec<String> = func.outputs.iter().map(|o| ts::result_key(func, o)).collect();
    let args = api_args(func, &s);
    let options = example_opts(func, &s);

    let mut body = s.lines.clone();
    let mut bound = vec!["stream".to_string()];
    bound.extend(result_names(func));
    let (pre, lead) = destructure(&bound);
    body.extend(pre);
    body.extend(api_call(&lead, &names.open_and_fill, &args, &options));
    body.push(format!("// {}[k] is the value at bar begIdx + k", outs[0]));

    let mut checks = vec!["const range = stream.outRange();".to_string()];
    checks.extend(api_call("const batch = ", &names.public, &args, &options));
    checks.push(format!("assert.deepEqual([begIdx, {}.length], [range.begIdx, range.count]);", outs[0]));
    checks.push(format!("assert.deepEqual([begIdx, {0}.length], [batch.begIdx, batch.{0}.length]);", outs[0]));
    checks.push(format!("assert.ok({}.length > 0);", outs[0]));
    checks.push(format!("for (let k = 0; k < {}.length; k++) {{", outs[0]));
    for var in &outs {
        checks.push(format!("  assert.ok(Object.is({var}[k], batch.{var}[k]));"));
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
        let mut s = ts::ts_license();
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
