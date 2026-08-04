//! Javadoc for the generated Java batch API, from each function's canonical
//! `ta_codegen/input/<name>/<name>.md` — the Java counterpart of
//! [`rust_doc`](super::rust_doc).
//!
//! The prose is the same source the rustdoc, the C header comments and the
//! ta-lib.org pages render, so the four cannot describe the same function
//! differently. Numbers (defaults, ranges, enum value lists) come from the YAML
//! via [`doc_meta`](super::doc_meta) — shared with the website renderer, so the
//! project's rule that numbers live only in the YAML holds here too.
//!
//! Until this existed, `Core.java`'s only `/**` blocks were the templated
//! streaming blurbs: 168 batch methods with no documentation at all, and no IDE
//! hover.

use std::collections::HashMap;
use std::fmt::Write as _;

use super::doc_meta::{self, ensure_period, RangeMeta};
use crate::ir::{DocDef, EnumDef, FuncDef, OptInput, Output, ParamType};
use crate::registry::Registry;

/// Javadoc for the public guarded batch wrapper.
///
/// `single_precision` selects the `float[]`-input overload, whose only difference
/// is the input type and the note about it.
#[allow(clippy::implicit_hasher)]
pub fn guarded_docs(
    func: &FuncDef,
    java_name: &str,
    single_precision: bool,
    enums: &HashMap<String, EnumDef>,
    registry: &Registry,
) -> String {
    let empty = DocDef::default();
    let doc = func.doc.as_ref().unwrap_or(&empty);
    let mut b = Block::new();

    b.para(&summary_text(func, doc));

    if let Some(formula) = &doc.formula {
        b.raw("<p><b>Formula</b>");
        b.raw("<pre>{@code");
        for line in formula.lines() {
            let t = line.trim();
            if !t.is_empty() {
                b.raw(format!(" * {t}").trim_start_matches(" * "));
            }
        }
        b.raw("}</pre>");
    }

    if !doc.notes.is_empty() {
        b.raw("<p><b>Notes</b>");
        b.raw("<ul>");
        for note in &doc.notes {
            b.raw(&format!("<li>{}</li>", jdoc(note)));
        }
        b.raw("</ul>");
    }

    if single_precision {
        b.para(
            "This is the {@code float[]} overload. The arithmetic is performed in \
             {@code double} before being written to the {@code double[]} output, so a \
             result beyond {@code float} range is still representable.",
        );
    }

    // Range / lookback contract — the thing callers get wrong most often.
    b.para(&format!(
        "Values are written only where the indicator is defined. The returned \
         {{@link OutRange}} says where they start and how many there are; nothing outside \
         that range is touched, and the library never pads with NaN. A valid range shorter \
         than {{@link Core#{java_name}Lookback}} is a <b>success with no values</b> \
         ({{@code count() == 0}}), not an error."
    ));

    b.blank();

    // @param, in the wrapper's exact parameter order.
    b.tag("param startIdx", "First bar of the requested range (inclusive).");
    b.tag("param endIdx", "Last bar of the requested range (inclusive).");
    for input in &func.inputs {
        b.tag(&format!("param {}", input.name), &input_desc(&input.name, doc));
    }
    for opt in &func.optional_inputs {
        b.tag(&format!("param {}", opt.name), &param_doc(opt, doc, enums));
    }
    for out in &func.outputs {
        b.tag(
            &format!("param {}", out.name),
            &format!(
                "{} Must hold at least {{@code endIdx - startIdx + 1}} values.",
                output_desc(out, doc)
            ),
        );
    }

    b.tag(
        "return",
        "The range written: {@code begIdx} is the first bar with a value, {@code count} \
         how many were written.",
    );
    b.tag(
        "throws IndexOutOfBoundsException",
        "if {@code startIdx} or {@code endIdx} is negative, or {@code endIdx < startIdx}.",
    );
    b.tag(
        "throws IllegalArgumentException",
        "if an optional parameter is outside its documented range, or two outputs share \
         one array.",
    );
    b.tag("throws NullPointerException", "if any input or output array is null.");

    if !doc.see_also.is_empty() {
        let links: Vec<String> = doc
            .see_also
            .iter()
            .filter_map(|n| see_also_link(n, func, registry))
            .collect();
        if !links.is_empty() {
            b.blank();
            for l in links {
                b.tag("see", &l);
            }
        }
    }

    b.render()
}


/// Javadoc for the public lookback method.
#[allow(clippy::implicit_hasher)]
pub fn lookback_docs(func: &FuncDef, java_name: &str, enums: &HashMap<String, EnumDef>) -> String {
    let empty = DocDef::default();
    let doc = func.doc.as_ref().unwrap_or(&empty);
    let mut b = Block::new();

    b.para(&format!(
        "Number of leading input bars {{@link Core#{java_name}}} consumes before it can \
         produce its first value."
    ));
    b.para(
        "Equivalently, the index of the first bar with a value when the whole series is \
         requested. Feed at least {@code lookback + 1} bars to get any output.",
    );
    if func.flags.iter().any(|f| f == "unstable_period") {
        b.para(
            "This function is recursive, so the result also includes this {@code Core}'s \
             unstable-period setting — which is why it is an instance method.",
        );
    }
    b.blank();
    for opt in &func.optional_inputs {
        b.tag(&format!("param {}", opt.name), &param_doc(opt, doc, enums));
    }
    b.tag("return", "The lookback, or {@code -1} if a parameter is out of range.");
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
        jdoc(title)
    } else {
        jdoc(&doc.summary)
    }
}

/// Description for one input. Price bundles are expanded per component in the
/// signature, so the components fall back to standard texts.
fn input_desc(name: &str, doc: &DocDef) -> String {
    if let Some((_, desc)) = doc.inputs.iter().find(|(n, _)| n == name) {
        return ensure_period(&jdoc(desc));
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
        .map_or_else(|| "Output values.".to_string(), |(_, d)| ensure_period(&jdoc(d)))
}

/// One `@param` for an optional parameter: canonical prose plus the default and
/// range injected from the YAML through the shared [`doc_meta`] resolver.
fn param_doc(opt: &OptInput, doc: &DocDef, enums: &HashMap<String, EnumDef>) -> String {
    let desc = doc
        .params
        .iter()
        .find(|(n, _)| n == &opt.name)
        .map(|(_, d)| jdoc(d))
        .or_else(|| opt.hint.as_deref().map(jdoc))
        .or_else(|| opt.display_name.as_deref().map(jdoc))
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
        // Every optional parameter Java surfaces as a primitive accepts the
        // cross-language default sentinel. An `enum:` param does not, and the
        // branch above says nothing about it on purpose: `MAType` is a real enum
        // and `Integer.MIN_VALUE` cannot be made into one (issue #162 — C, Rust
        // and C# all type it as an integer and must substitute).
        meta.push(match opt.param_type {
            ParamType::Real => "{@code -4e37} selects the default".to_string(),
            _ => "{@code Integer.MIN_VALUE} selects the default".to_string(),
        });
    }

    if meta.is_empty() {
        ensure_period(&desc)
    } else {
        format!("{} ({}).", ensure_period(&desc).trim_end_matches('.'), meta.join("; "))
    }
}

/// A `@see` target for a canonical `## See Also` entry, skipping anything that is
/// not a plain function name (the source separates them with `·`).
///
/// The method name comes from the [`Registry`], NOT from lower-casing the
/// canonical name: several functions have irregular Java names (`MA` is
/// `movingAverage`, `WILLR` is `willR`), so a naive mapping emits `@see Core#ma`
/// — a link that silently resolves to nothing. An unknown name is dropped rather
/// than guessed at.
fn see_also_link(entry: &str, func: &FuncDef, registry: &Registry) -> Option<String> {
    let name = entry.trim();
    if name.is_empty() || name.eq_ignore_ascii_case(&func.name) {
        return None;
    }
    if !name.chars().all(|c| c.is_ascii_alphanumeric() || c == '_') {
        return None;
    }
    let key = name.to_lowercase();
    if !registry.contains(&key) {
        return None;
    }
    Some(format!("Core#{}", registry.java_base(&key)))
}

// ---------------------------------------------------------------------------
// Javadoc escaping + block assembly
// ---------------------------------------------------------------------------

/// Turn canonical Markdown-ish prose into Javadoc-safe HTML.
///
/// `&`, `<` and `>` become entities (a stray `<` would silently swallow text as a
/// bogus tag), backtick spans become `{@code ...}`, and a literal `@` is escaped
/// so it cannot be read as a block tag.
fn jdoc(text: &str) -> String {
    let mut out = String::new();
    let mut in_code = false;
    for c in text.chars() {
        match c {
            '`' => {
                out.push_str(if in_code { "}" } else { "{@code " });
                in_code = !in_code;
            }
            '&' => out.push_str("&amp;"),
            '<' => out.push_str("&lt;"),
            '>' => out.push_str("&gt;"),
            '@' => out.push_str("&#64;"),
            '\n' => out.push(' '),
            _ => out.push(c),
        }
    }
    if in_code {
        // Unbalanced backtick in the source — close it rather than emit `{@code`.
        out.push('}');
    }
    out
}

/// Accumulates Javadoc lines and renders the `/** ... */` block at a 3-space
/// indent, matching the surrounding generated methods.
struct Block {
    lines: Vec<String>,
}

impl Block {
    fn new() -> Self {
        Block { lines: Vec::new() }
    }

    /// A `<p>`-led paragraph (the first one omits the tag, as Javadoc convention).
    fn para(&mut self, text: &str) {
        let tag = if self.lines.is_empty() { "" } else { "<p>" };
        for (i, line) in wrap(&format!("{tag}{text}"), 74).into_iter().enumerate() {
            let _ = i;
            self.lines.push(line);
        }
    }

    fn raw(&mut self, line: &str) {
        self.lines.push(line.to_string());
    }

    fn blank(&mut self) {
        if !self.lines.last().is_none_or(String::is_empty) {
            self.lines.push(String::new());
        }
    }

    /// A `@tag description`, continuation lines hanging-indented.
    fn tag(&mut self, tag: &str, text: &str) {
        let wrapped = wrap(&format!("@{tag} {text}"), 74);
        for (i, line) in wrapped.into_iter().enumerate() {
            if i == 0 {
                self.lines.push(line);
            } else {
                self.lines.push(format!("       {line}"));
            }
        }
    }

    fn render(&self) -> String {
        let mut s = String::from("   /**\n");
        for line in &self.lines {
            if line.is_empty() {
                s.push_str("    *\n");
            } else {
                let _ = writeln!(s, "    * {line}");
            }
        }
        s.push_str("    */\n");
        s
    }
}

/// Greedy word wrap that never breaks inside a `{@code ...}` / `{@link ...}` span.
fn wrap(text: &str, width: usize) -> Vec<String> {
    let mut out: Vec<String> = Vec::new();
    let mut line = String::new();
    let mut depth = 0usize;
    let mut word = String::new();

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
            '{' => {
                depth += 1;
                word.push(c);
            }
            '}' => {
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
