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

/// What the aliasing guard rejects, which differs by overload: only the
/// `double` one can express computing wholly in place, since a `float` input and
/// a `double` output are never the same span.
fn aliasing_exception_text(single_precision: bool) -> &'static str {
    if single_precision {
        "Two output buffers overlap, or an output overlaps an input. An output and a \
         real input never share an element type in this overload, so the two can never \
         be the same span: there is no in-place case to allow, and any overlap of their \
         byte ranges is rejected."
    } else {
        "Two output buffers overlap, or an output partially overlaps an input. \
         Computing wholly in place (an output that IS an input) is allowed."
    }
}

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
        // A nullable output may be declined; C# spells "declined" as an empty
        // span, which the signature cannot say on its own.
        let sizing = if out.is_nullable() {
            "Pass an empty span to decline it: it is still computed where the \
             algorithm needs it, but nothing is written out. Supplied, it must \
             hold at least <c>endIdx - startIdx + 1</c> values."
        } else {
            "Must hold at least <c>endIdx - startIdx + 1</c> values."
        };
        b.param(&out.name, &format!("{} {sizing}", output_desc(out, doc)));
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
        "System.ArgumentException",
        "A span is too short for the range requested: any input this function \
         <i>declares</i> that does not reach <c>endIdx</c>, or an output that cannot hold \
         the values produced. Checked before anything is written, so a rejected call \
         leaves every buffer untouched. Declared, not read: a few candlestick patterns \
         take an OHLC series they never index, and it is required all the same. An empty \
         span — which is what a null array becomes, since a span cannot be null — is \
         rejected on the same terms and no others: it is too short whenever the range \
         produces a value, and fine when it produces none, and on an output this function \
         documents as declinable it is how you decline.",
    );
    b.exception("System.ArgumentException", aliasing_exception_text(single_precision));
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

pub(super) fn output_desc(output: &Output, doc: &DocDef) -> String {
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
            ParamType::Real => "<see cref=\"Core.REAL_DEFAULT\"/> selects the default".to_string(),
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
/// entities (CS1570 otherwise), backtick spans become `<c>...</c>`, and a
/// Markdown inline link becomes a `<see href>`.
pub(super) fn csdoc(text: &str) -> String {
    let mut out = String::new();
    let mut in_code = false;
    let chars: Vec<char> = text.chars().collect();
    let mut i = 0;
    while i < chars.len() {
        let c = chars[i];
        match c {
            '`' => {
                out.push_str(if in_code { "</c>" } else { "<c>" });
                in_code = !in_code;
            }
            '[' if !in_code => {
                if let Some((label, dest, end)) = inline_link(&chars, i) {
                    let _ = write!(
                        out,
                        "<see href=\"{}\">{}</see>",
                        attr_escape(&dest),
                        csdoc(&label)
                    );
                    i = end;
                    continue;
                }
                out.push('[');
            }
            '&' => out.push_str("&amp;"),
            '<' => out.push_str("&lt;"),
            '>' => out.push_str("&gt;"),
            '\n' => out.push(' '),
            _ => out.push(c),
        }
        i += 1;
    }
    if in_code {
        // Unbalanced backtick in the source — close it rather than emit bad XML.
        out.push_str("</c>");
    }
    out
}

/// A URL destination as an attribute value. `&` and `"` are the only characters
/// that can end the attribute early; everything else in a URL is literal.
fn attr_escape(dest: &str) -> String {
    dest.replace('&', "&amp;").replace('"', "&quot;")
}

/// A well-formed Markdown inline link starting at `chars[start] == '['`: its
/// label, its destination and the index one past the closing `)`. The label must
/// not itself contain a bracket, and the destination must look like a URL or a
/// site-absolute path — a parenthesis that merely follows a bracketed aside is
/// not a link. A site-absolute destination is written for ta-lib.org, which an
/// XML doc has no root for, so it is resolved against the real site.
fn inline_link(chars: &[char], start: usize) -> Option<(String, String, usize)> {
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
    let is_url =
        dest.starts_with("http://") || dest.starts_with("https://") || dest.starts_with('/');
    if !is_url || dest.contains(char::is_whitespace) {
        return None;
    }
    let label: String = chars[start + 1..close].iter().collect();
    let dest = match dest.strip_prefix('/') {
        Some(rest) => format!("https://ta-lib.org/{rest}"),
        None => dest,
    };
    Some((label, dest, paren + 1))
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

#[cfg(test)]
mod tests {
    use super::csdoc;

    /// A Markdown link renders as a `<see href>`. The C# compiler has no lint
    /// for a stranded one, so nothing but this says the conversion still happens.
    #[test]
    fn inline_links_become_see_href() {
        assert_eq!(
            csdoc("see [TradingView](https://tv.com/x) for more"),
            "see <see href=\"https://tv.com/x\">TradingView</see> for more"
        );
        // Site-absolute destinations are rebased: an XML doc has no ta-lib.org root.
        assert_eq!(
            csdoc("the [`SMA`](/functions/sma) page"),
            "the <see href=\"https://ta-lib.org/functions/sma\"><c>SMA</c></see> page"
        );
    }

    /// Bracketed prose that merely happens to be followed by parentheses is not
    /// a link, and a bracket inside a code span is literal.
    #[test]
    fn bracketed_prose_is_left_alone() {
        assert_eq!(csdoc("range [-1, 1]"), "range [-1, 1]");
        assert_eq!(csdoc("close[i](t)"), "close[i](t)");
        assert_eq!(csdoc("[label](not a url)"), "[label](not a url)");
        assert_eq!(csdoc("`a[i](x)`"), "<c>a[i](x)</c>");
        // A bare fragment resolves to the type page, where no such anchor is.
        assert_eq!(csdoc("[Rules](#rules)"), "[Rules](#rules)");
    }

    /// A raw `&` in an attribute is malformed XML, which CS1570 makes an error.
    #[test]
    fn link_destinations_are_attribute_escaped() {
        assert_eq!(
            csdoc("[x](https://a.b/?p=1&q=2)"),
            "<see href=\"https://a.b/?p=1&amp;q=2\">x</see>"
        );
    }
}
