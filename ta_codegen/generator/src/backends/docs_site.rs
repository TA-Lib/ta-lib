//! ta-lib.org website generator — the one `ta_codegen` output that lives under `docs/`
//! (the mkdocs site tree) instead of `ta_codegen/output/`.
//!
//! For each function it reads the canonical documentation source
//! `ta_codegen/input/<dir>/<dir>.md` and emits a website page at
//! `docs/functions/<dir>.md` (served at `https://ta-lib.org/functions/<name>`), plus a
//! grouped `docs/functions/index.md`. The page transform is deterministic (SEO front
//! matter + `## See Also` links), so the output stays byte-stable under the regen oracle.

use crate::ir::FuncDef;
use std::collections::{BTreeMap, HashSet};
use std::path::Path;

/// Generate the per-function website pages + index into `docs/functions/`.
pub fn generate(funcs: &[FuncDef], root: &Path) {
    let input_base = root.join("ta_codegen/input");
    let out_dir = root.join("docs/functions");
    std::fs::create_dir_all(&out_dir).expect("create docs/functions");

    let mut funcs: Vec<&FuncDef> = funcs.iter().collect();
    funcs.sort_by(|a, b| a.name.cmp(&b.name));

    // Known function names, for linkifying `## See Also` to sibling pages.
    let known: HashSet<&str> = funcs.iter().map(|f| f.name.as_str()).collect();

    let mut paged: Vec<&FuncDef> = Vec::new();
    for f in &funcs {
        let dir = f.name.to_lowercase();
        let src = input_base.join(&dir).join(format!("{dir}.md"));
        let Ok(body) = std::fs::read_to_string(&src) else {
            eprintln!("  docs: no source {dir}/{dir}.md — skipping page");
            continue;
        };
        let page = transform_page(&body, &f.name, &known);
        super::write_if_changed_silent(&out_dir.join(format!("{dir}.md")), &page);
        paged.push(f);
    }

    let index = build_index(&paged);
    super::write_if_changed(&out_dir.join("index.md"), &index, "docs/functions", paged.len());

    // Prune stale pages (functions removed since the last run).
    let mut keep: HashSet<String> = paged
        .iter()
        .map(|f| format!("{}.md", f.name.to_lowercase()))
        .collect();
    keep.insert("index.md".to_string());
    if let Ok(rd) = std::fs::read_dir(&out_dir) {
        for e in rd.flatten() {
            let p = e.path();
            let is_md = p.extension().is_some_and(|x| x.eq_ignore_ascii_case("md"));
            let fname = e.file_name().to_string_lossy().to_string();
            if is_md && !keep.contains(&fname) {
                let _ = std::fs::remove_file(p);
            }
        }
    }
}

/// Prepend SEO front matter (title + description) and linkify `## See Also`.
fn transform_page(body: &str, name: &str, known: &HashSet<&str>) -> String {
    let desc = extract_summary(body);
    let linked = linkify_see_also(body, known);
    let mut out = String::from("---\n");
    out.push_str(&format!("title: {name}\n"));
    if !desc.is_empty() {
        out.push_str(&format!("description: {desc:?}\n"));
    }
    out.push_str("---\n\n");
    out.push_str(&linked);
    out
}

/// Pull the `## Summary` paragraph as a single line for the page meta description.
fn extract_summary(body: &str) -> String {
    let Some(start) = body.find("## Summary") else {
        return String::new();
    };
    let after = &body[start + "## Summary".len()..];
    let rest = match after.find('\n') {
        Some(i) => &after[i + 1..],
        None => "",
    };
    let end = rest.find("\n## ").unwrap_or(rest.len());
    rest[..end].split_whitespace().collect::<Vec<_>>().join(" ")
}

/// Turn `## See Also` entries (`ADX · DX · …`) into source-root-absolute links
/// (`/functions/<name>.md`) to sibling pages, leaving any non-function token untouched.
/// Absolute (not bare-relative) so VuePress resolves them even though the pages are
/// served from a symlink that lives outside the site source root.
fn linkify_see_also(body: &str, known: &HashSet<&str>) -> String {
    let mut lines: Vec<String> = body.lines().map(String::from).collect();
    for i in 0..lines.len() {
        if lines[i].trim() == "## See Also" {
            let mut j = i + 1;
            while j < lines.len() && lines[j].trim().is_empty() {
                j += 1;
            }
            if j < lines.len() {
                lines[j] = lines[j]
                    .split('·')
                    .map(|tok| {
                        let n = tok.trim();
                        if known.contains(n) {
                            format!("[{n}](/functions/{}.md)", n.to_lowercase())
                        } else {
                            n.to_string()
                        }
                    })
                    .collect::<Vec<_>>()
                    .join(" · ");
            }
            break;
        }
    }
    let mut out = lines.join("\n");
    out.push('\n');
    out
}

/// The `/functions/` landing page: every function linked, grouped by category.
fn build_index(funcs: &[&FuncDef]) -> String {
    let mut by_group: BTreeMap<&str, Vec<&FuncDef>> = BTreeMap::new();
    for f in funcs {
        by_group.entry(f.group.as_str()).or_default().push(f);
    }
    let mut s = String::from(
        "---\ntitle: Functions\ndescription: \"All TA-Lib technical analysis functions, grouped by category.\"\n---\n\n",
    );
    s.push_str("# TA-Lib Functions\n\n");
    s.push_str(
        "All technical-analysis functions, grouped by category. Each page documents the \
         formula, inputs, outputs, and links to the C / Rust / Java source.\n",
    );
    for (group, fns) in &by_group {
        s.push_str(&format!("\n## {group}\n\n"));
        for f in fns {
            let dir = f.name.to_lowercase();
            let hint = f.hint.as_deref().unwrap_or("");
            s.push_str(&format!("- [{}](/functions/{dir}.md) — {hint}\n", f.name));
        }
    }
    s
}
