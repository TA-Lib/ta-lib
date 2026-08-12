//! Parser for the canonical per-function documentation file
//! (`ta_codegen/input/<name>/<name>.md`) into an [`ir::DocDef`].
//!
//! The file format is specified in docs/ta_codegen_input_doc.md: an `# <NAME>` H1
//! followed by a fixed set of `##` sections. This parser is deliberately lenient —
//! unknown sections are ignored (`## Implementation` is render-owned), missing
//! optional sections yield empty fields.

use crate::ir::DocDef;
use std::path::Path;

/// Parse `<name>.md` into a [`DocDef`]. Returns `None` when the file is absent
/// or unreadable (the caller decides whether that is worth a warning).
pub fn parse_doc_md(path: &Path) -> Option<DocDef> {
    let body = std::fs::read_to_string(path).ok()?;
    Some(parse_doc_str(&body))
}

/// Parse the markdown body of a canonical doc file.
pub fn parse_doc_str(body: &str) -> DocDef {
    let mut doc = DocDef::default();

    for (title, text) in sections(body) {
        match title.as_str() {
            "Summary" => doc.summary = text.trim().to_string(),
            "Formula" => {
                let t = text.trim();
                if !t.is_empty() {
                    let (formula, note) = split_formula_note(t);
                    doc.formula = Some(formula);
                    doc.formula_note = note;
                }
            }
            "Notes" => doc.notes = bullets(&text),
            "Inputs" => doc.inputs = named_bullets(&text),
            "Outputs" => doc.outputs = named_bullets(&text),
            "Parameters" => doc.params = named_bullets(&text),
            "Aliases" => {
                doc.aliases = text
                    .trim()
                    .split(',')
                    .map(str::trim)
                    .filter(|s| !s.is_empty())
                    .map(String::from)
                    .collect();
            }
            "See Also" => {
                doc.see_also = text
                    .trim()
                    .split('·')
                    .map(str::trim)
                    .filter(|s| !s.is_empty())
                    .map(String::from)
                    .collect();
            }
            "References" => doc.references = bullets(&text),
            _ => {} // `## Implementation` and anything unknown: render-owned / ignored
        }
    }

    doc
}

/// Split a `## Formula` body at its closing `$$` into the formula proper and the
/// prose that follows it — typically the sentence naming the symbols.
///
/// Every backend renders a formula as preformatted text, so a trailing sentence
/// left inside it renders as code rather than prose. Splitting here rather than
/// in each emitter keeps the three doc backends agreeing, and keeps the rule off
/// the authors: the `.md` stays plain markdown, where the sentence already reads
/// as a paragraph.
///
/// Bodies with no display-math delimiters are returned unchanged — that is most
/// functions, whose formulas are ASCII pseudo-math with load-bearing alignment.
fn split_formula_note(body: &str) -> (String, Option<String>) {
    let lines: Vec<&str> = body.lines().collect();
    let delims: Vec<usize> = lines
        .iter()
        .enumerate()
        .filter(|(_, l)| l.trim() == "$$")
        .map(|(i, _)| i)
        .collect();
    // Fewer than two is not a closed block; leave it alone rather than guess
    // which side of a lone delimiter is prose.
    if delims.len() < 2 {
        return (body.to_string(), None);
    }
    let close = delims[delims.len() - 1];
    // One logical paragraph, soft-wrapped in the `.md` as markdown allows. It has
    // to reach the emitters as a single line: each of them prefixes every line it
    // is handed (`/// `, ` * `), and an embedded newline slips a line past that —
    // which is malformed C#, not just ugly.
    let note = lines[close + 1..]
        .iter()
        .map(|l| l.trim())
        .filter(|l| !l.is_empty())
        .collect::<Vec<_>>()
        .join(" ");
    let formula = lines[..=close].join("\n").trim_end().to_string();
    (formula, (!note.is_empty()).then_some(note))
}

/// Split the body into `(section_title, section_text)` pairs on `## ` headings.
fn sections(body: &str) -> Vec<(String, String)> {
    let mut out: Vec<(String, String)> = Vec::new();
    for line in body.lines() {
        if let Some(title) = line.strip_prefix("## ") {
            out.push((title.trim().to_string(), String::new()));
        } else if let Some((_, text)) = out.last_mut() {
            text.push_str(line);
            text.push('\n');
        }
    }
    out
}

/// Collect `- ` bullet items; a continuation line (no `- ` prefix) is appended
/// to the preceding bullet.
fn bullets(text: &str) -> Vec<String> {
    let mut items: Vec<String> = Vec::new();
    for line in text.lines() {
        let trimmed = line.trim();
        if let Some(item) = trimmed.strip_prefix("- ") {
            items.push(item.trim().to_string());
        } else if !trimmed.is_empty() {
            if let Some(last) = items.last_mut() {
                last.push(' ');
                last.push_str(trimmed);
            }
        }
    }
    items
}

/// Parse bullets of the form ``- `name` — description`` into `(name, description)`.
fn named_bullets(text: &str) -> Vec<(String, String)> {
    bullets(text)
        .iter()
        .filter_map(|item| {
            let rest = item.strip_prefix('`')?;
            let (name, after) = rest.split_once('`')?;
            // Separator is an em dash (` — `); tolerate a plain hyphen. Strip exactly
            // one separator char — the description itself may start with `-` (CDL signs).
            let after = after.trim_start();
            let desc = after
                .strip_prefix('—')
                .or_else(|| after.strip_prefix('-'))
                .unwrap_or(after)
                .trim_start();
            Some((name.to_string(), desc.to_string()))
        })
        .collect()
}

#[cfg(test)]
mod tests {
    use super::*;

    const SAMPLE: &str = "# BBANDS\n\n## Summary\n\nBollinger Bands: a moving-average middle band.\n\n## Formula\n\nmiddle = MA(inReal, period)\n\n## Notes\n\n- Uses the population form.\n- Always SMA for the stddev.\n\n## Inputs\n\n- `inReal` — Input data series\n\n## Outputs\n\n- `outRealUpperBand` — Middle band plus nbDevUp standard deviations\n\n## Parameters\n\n- `optInTimePeriod` — Periods for the MA and standard deviation\n\n## Implementation\n\nTA-Lib Definition: ignored\n\n## Aliases\n\nBollinger Bands\n\n## See Also\n\nMA · STDDEV · SMA\n\n## References\n\n- John A. Bollinger, *Bollinger on Bollinger Bands* (ISBN 0071373683)\n";

    #[test]
    fn parses_all_sections() {
        let doc = parse_doc_str(SAMPLE);
        assert_eq!(
            doc.summary,
            "Bollinger Bands: a moving-average middle band."
        );
        assert_eq!(doc.formula.as_deref(), Some("middle = MA(inReal, period)"));
        assert_eq!(doc.notes.len(), 2);
        assert_eq!(
            doc.inputs,
            vec![("inReal".into(), "Input data series".into())]
        );
        assert_eq!(doc.outputs.len(), 1);
        assert_eq!(
            doc.params,
            vec![(
                "optInTimePeriod".into(),
                "Periods for the MA and standard deviation".into()
            )]
        );
        assert_eq!(doc.aliases, vec!["Bollinger Bands"]);
        assert_eq!(doc.see_also, vec!["MA", "STDDEV", "SMA"]);
        assert_eq!(doc.references.len(), 1);
    }

    #[test]
    fn missing_optional_sections_are_empty() {
        let doc = parse_doc_str("# X\n\n## Summary\n\nOnly a summary.\n");
        assert_eq!(doc.summary, "Only a summary.");
        assert!(doc.formula.is_none());
        assert!(doc.notes.is_empty());
        assert!(doc.aliases.is_empty());
        assert!(doc.see_also.is_empty());
        assert!(doc.references.is_empty());
    }
}
