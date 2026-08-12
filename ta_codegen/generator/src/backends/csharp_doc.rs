//! XML documentation for the generated C# batch API, from each function's
//! canonical `ta_codegen/input/<name>/<name>.md` — the C# counterpart of
//! [`java_doc`](super::java_doc).
//!
//! The prose is the same source the rustdoc, the Javadoc, the C header comments
//! and the ta-lib.org pages render, so the five cannot describe the same
//! function differently. Numbers (defaults, ranges, enum value lists) come from
//! the YAML via [`doc_meta`](super::doc_meta), so the project's rule that
//! numbers live only in the YAML holds here too.
//!
//! These comments are load-bearing: the shipped `TALib.csproj` sets
//! `GenerateDocumentationFile` + `TreatWarningsAsErrors`, so a public member
//! without a `<summary>` (CS1591), a mis-named `<param>` (CS1572/CS1573) or
//! malformed XML (CS1570) fails the library build. Method cross-references are
//! deliberately plain `<c>` text, not `cref`: the wrapper and core methods are
//! overload sets (double/float inputs, plus the internal RetCode cores of the
//! same name), and an ambiguous `cref` is CS0419 — another error.

use std::collections::HashMap;
use std::fmt::Write as _;

use super::doc_meta::{self, ensure_period, RangeMeta};
use crate::ir::{DocDef, EnumDef, FuncDef, OptInput, Output, ParamType};

/// XML docs for the public guarded batch wrapper.
///
/// `single_precision` selects the `float[]`-input overload, whose only
/// difference is the input type and the note about it.
#[allow(clippy::implicit_hasher)]
pub fn guarded_docs(
    func: &FuncDef,
    cs_name: &str,
    single_precision: bool,
    enums: &HashMap<String, EnumDef>,
) -> String {
    let empty = DocDef::default();
    let doc = func.doc.as_ref().unwrap_or(&empty);
    let mut b = Block::new();

    b.open("summary");
    b.text(&summary_text(func, doc));
    b.close("summary");

    b.open("remarks");
    if let Some(formula) = &doc.formula {
        b.text("<b>Formula</b>");
        b.raw("<code>");
        for line in formula.lines() {
            let t = line.trim();
            if !t.is_empty() {
                b.raw(&xml_escape_raw(t));
            }
        }
        b.raw("</code>");
        if let Some(note) = &doc.formula_note {
            b.text(&csdoc(note));
        }
    }
    if !doc.notes.is_empty() {
        b.raw("<list type=\"bullet\">");
        for note in &doc.notes {
            b.raw(&format!("<item><description>{}</description></item>", csdoc(note)));
        }
        b.raw("</list>");
    }
    if single_precision {
        b.para(
            "This is the <c>float[]</c> overload: input elements are widened to \
             <c>double</c> as they are read and all arithmetic is performed in \
             <c>double</c>, so a result beyond <c>float</c> range is still \
             representable.",
        );
    }
    b.para(&format!(
        "Values are written only where the indicator is defined. The returned \
         <see cref=\"OutRange\"/> says where they start and how many there are; nothing \
         outside that range is touched, and the library never pads with NaN. A valid range \
         shorter than <c>{cs_name}_Lookback</c> is a <b>success with no values</b> \
         (<c>Count == 0</c>), not an error."
    ));
    b.close("remarks");

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
                "{} Must hold at least <c>endIdx - startIdx + 1</c> values.",
                output_desc(out, doc)
            ),
        );
    }

    b.tag(
        "returns",
        "The range written: <c>BegIdx</c> is the first bar with a value, <c>Count</c> \
         how many were written.",
    );
    b.exception(
        "System.ArgumentOutOfRangeException",
        "<c>startIdx</c> or <c>endIdx</c> is negative or above \
         <see cref=\"Core.MAX_INDEX\"/>, or <c>endIdx &lt; startIdx</c>.",
    );
    b.exception(
        "System.ArgumentException",
        "An optional parameter is outside its documented range, or two outputs share \
         one array.",
    );
    b.exception(
        "System.NullReferenceException",
        "An input or output array is null. (Unlike the C library, the managed \
         tier does not pre-validate nulls; the first array access throws.)",
    );
    b.render()
}


/// XML docs for the public lookback method.
#[allow(clippy::implicit_hasher)]
pub fn lookback_docs(func: &FuncDef, cs_name: &str, enums: &HashMap<String, EnumDef>) -> String {
    let empty = DocDef::default();
    let doc = func.doc.as_ref().unwrap_or(&empty);
    let mut b = Block::new();

    b.open("summary");
    b.text(&format!(
        "Number of leading input bars <c>{cs_name}</c> consumes before it can produce \
         its first value."
    ));
    b.close("summary");
    b.open("remarks");
    b.text(
        "Equivalently, the index of the first bar with a value when the whole series is \
         requested. Feed at least <c>lookback + 1</c> bars to get any output.",
    );
    if func.flags.iter().any(|f| f == "unstable_period") {
        b.para(
            "This function is recursive, so the result also includes this <c>Core</c>'s \
             unstable-period setting — which is why it is an instance method.",
        );
    }
    b.close("remarks");
    for opt in &func.optional_inputs {
        b.param(&opt.name, &param_doc(opt, doc, enums));
    }
    b.tag("returns", "The lookback, or <c>-1</c> if a parameter is out of range.");
    b.render()
}

// ---------------------------------------------------------------------------
// Pieces
// ---------------------------------------------------------------------------

fn summary_text(func: &FuncDef, doc: &DocDef) -> String {
    if doc.summary.is_empty() {
        let title = func
            .description
            .as_deref()
            .or(func.hint.as_deref())
            .unwrap_or(&func.group);
        csdoc(title)
    } else {
        csdoc(&doc.summary)
    }
}

/// Description for one input. Price bundles are expanded per component in the
/// signature, so the components fall back to standard texts.
fn input_desc(name: &str, doc: &DocDef) -> String {
    if let Some((_, desc)) = doc.inputs.iter().find(|(n, _)| n == name) {
        return ensure_period(&csdoc(desc));
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
        .map_or_else(|| "Output values.".to_string(), |(_, d)| ensure_period(&csdoc(d)))
}

/// One `<param>` for an optional parameter: canonical prose plus the default and
/// range injected from the YAML through the shared [`doc_meta`] resolver.
fn param_doc(opt: &OptInput, doc: &DocDef, enums: &HashMap<String, EnumDef>) -> String {
    let desc = doc
        .params
        .iter()
        .find(|(n, _)| n == &opt.name)
        .map(|(_, d)| csdoc(d))
        .or_else(|| opt.hint.as_deref().map(csdoc))
        .or_else(|| opt.display_name.as_deref().map(csdoc))
        .unwrap_or_else(|| "Optional parameter".to_string());

    let m = doc_meta::param_meta(opt, enums);
    let mut meta: Vec<String> = Vec::new();
    if matches!(opt.param_type, ParamType::Enum(_)) {
        if let (Some(d), Some(variant)) = (m.default.as_ref(), m.default_variant.as_ref()) {
            meta.push(format!("default {d} = {variant}"));
        }
        if !m.values.is_empty() {
            let values: Vec<String> = m.values.iter().map(|(v, n)| format!("{v}={n}")).collect();
            meta.push(format!("values: {}", values.join(", ")));
        }
        // Unlike Java's, a C# enum is an `int` with names, so the sentinel IS
        // expressible here and the typed API resolves it (issue #162). It needs
        // an explicit cast at this parameter though, so the member (#182) is
        // named first: it is the spelling that compiles as written.
        if let ParamType::Enum(name) = &opt.param_type {
            match super::common::enum_default_variant(enums, name) {
                Some(v) => meta.push(format!(
                    "<c>{name}.{}</c> (or <c>({name})int.MinValue</c>) selects the default",
                    v.name
                )),
                None => meta.push(format!("<c>({name})int.MinValue</c> selects the default")),
            }
        }
    } else {
        if let Some(d) = &m.default {
            meta.push(format!("default {d}"));
        }
        match &m.range {
            RangeMeta::Bounded(lo, hi) => meta.push(format!("range {lo}..{hi}")),
            RangeMeta::Min(lo) => meta.push(format!("minimum {lo}")),
            RangeMeta::Max(hi) => meta.push(format!("maximum {hi}")),
            RangeMeta::Unbounded => {}
        }
        // Every optional parameter accepts the cross-language default sentinel.
        meta.push(match opt.param_type {
            ParamType::Real => "<c>-4e37</c> selects the default".to_string(),
            _ => "<c>int.MinValue</c> selects the default".to_string(),
        });
    }

    if meta.is_empty() {
        ensure_period(&desc)
    } else {
        format!("{} ({}).", ensure_period(&desc).trim_end_matches('.'), meta.join("; "))
    }
}

// ---------------------------------------------------------------------------
// XML escaping + block assembly
// ---------------------------------------------------------------------------

/// Turn canonical Markdown-ish prose into XML-doc-safe text: `&`/`<`/`>` become
/// entities (CS1570 otherwise) and backtick spans become `<c>...</c>`.
fn csdoc(text: &str) -> String {
    let mut out = String::new();
    let mut in_code = false;
    for c in text.chars() {
        match c {
            '`' => {
                out.push_str(if in_code { "</c>" } else { "<c>" });
                in_code = !in_code;
            }
            '&' => out.push_str("&amp;"),
            '<' => out.push_str("&lt;"),
            '>' => out.push_str("&gt;"),
            '\n' => out.push(' '),
            _ => out.push(c),
        }
    }
    if in_code {
        // Unbalanced backtick in the source — close it rather than emit bad XML.
        out.push_str("</c>");
    }
    out
}

/// Escape a raw (formula) line for XML content — entities only, no backtick
/// handling (formulas use `*`/`<`/`>` as math, not markup).
pub(crate) fn xml_escape_raw(text: &str) -> String {
    text.replace('&', "&amp;").replace('<', "&lt;").replace('>', "&gt;")
}

/// Accumulates `///` lines and renders them at a 3-space indent, matching the
/// surrounding generated methods.
struct Block {
    lines: Vec<String>,
}

impl Block {
    fn new() -> Self {
        Block { lines: Vec::new() }
    }

    fn open(&mut self, tag: &str) {
        self.lines.push(format!("<{tag}>"));
    }

    fn close(&mut self, tag: &str) {
        self.lines.push(format!("</{tag}>"));
    }

    /// Wrapped body text at the current position.
    fn text(&mut self, text: &str) {
        for line in wrap(text, 74) {
            self.lines.push(line);
        }
    }

    /// A `<para>`-wrapped paragraph (inside `<remarks>`).
    fn para(&mut self, text: &str) {
        self.lines.push("<para>".to_string());
        self.text(text);
        self.lines.push("</para>".to_string());
    }

    fn raw(&mut self, line: &str) {
        self.lines.push(line.to_string());
    }

    /// A single-tag element with wrapped content: `<returns>...</returns>`.
    fn tag(&mut self, tag: &str, text: &str) {
        self.element(&format!("<{tag}>"), text, &format!("</{tag}>"));
    }

    fn param(&mut self, name: &str, text: &str) {
        self.element(&format!("<param name=\"{name}\">"), text, "</param>");
    }

    fn exception(&mut self, cref: &str, text: &str) {
        self.element(&format!("<exception cref=\"{cref}\">"), text, "</exception>");
    }

    /// Emit `<open>text</close>`, on one line when short, wrapped otherwise.
    fn element(&mut self, open: &str, text: &str, close: &str) {
        let one_line = format!("{open}{text}{close}");
        if one_line.chars().count() <= 78 {
            self.lines.push(one_line);
        } else {
            let wrapped = wrap(text, 74);
            let mut first = true;
            for line in wrapped {
                if first {
                    self.lines.push(format!("{open}{line}"));
                    first = false;
                } else {
                    self.lines.push(line);
                }
            }
            if let Some(last) = self.lines.last_mut() {
                last.push_str(close);
            }
        }
    }

    fn render(&self) -> String {
        let mut s = String::new();
        for line in &self.lines {
            if line.is_empty() {
                s.push_str("   ///\n");
            } else {
                let _ = writeln!(s, "   /// {line}");
            }
        }
        s
    }
}

/// Greedy word wrap that never breaks inside an XML tag or entity.
fn wrap(text: &str, width: usize) -> Vec<String> {
    let mut out: Vec<String> = Vec::new();
    let mut line = String::new();
    let mut word = String::new();
    let mut depth = 0usize;

    let flush_word = |line: &mut String, word: &mut String, out: &mut Vec<String>| {
        if word.is_empty() {
            return;
        }
        if !line.is_empty() && line.chars().count() + 1 + word.chars().count() > width {
            out.push(std::mem::take(line));
        }
        if !line.is_empty() {
            line.push(' ');
        }
        line.push_str(word);
        word.clear();
    };

    for c in text.chars() {
        match c {
            '<' => {
                depth += 1;
                word.push(c);
            }
            '>' => {
                depth = depth.saturating_sub(1);
                word.push(c);
            }
            ' ' if depth == 0 => flush_word(&mut line, &mut word, &mut out),
            _ => word.push(c),
        }
    }
    flush_word(&mut line, &mut word, &mut out);
    if !line.is_empty() {
        out.push(line);
    }
    if out.is_empty() {
        out.push(String::new());
    }
    out
}
