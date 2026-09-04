//! Every candlestick condition comment sits on the operand it annotates.
//!
//! Read off DISK, in all four backends, because the defect this pins was
//! invisible to every value gate: the comments were never lost, only moved --
//! concatenated into one blob at the end of a line, or stranded as a standalone
//! line at the head of the `if` body. Both spellings compile and compute the
//! right answer, so only a shape test sees them.
//!
//! The residue is an input-authoring ambiguity, not an emitter one, so it is
//! pinned by exact set equality: a function whose input gets fixed must be
//! deleted from `UNPLACED` here, and one that regresses must be added.

#[path = "common/mod.rs"]
mod common;

use std::path::{Path, PathBuf};

fn repo_root() -> PathBuf {
    Path::new(env!("CARGO_MANIFEST_DIR")).join("../..")
}

/// Where a comment ends up relative to the leaf it was written against.
#[derive(PartialEq, Eq, PartialOrd, Ord, Debug, Clone, Copy)]
enum Placement {
    /// Concatenated with at least one other comment into a single block.
    Blob,
    /// Alone on its line, so it annotates the body rather than an operand.
    Hoisted,
}

/// Candlestick inputs whose own comment position does not say which operand a
/// comment belongs to. Empty, and meant to stay that way: every such comment was
/// moved in `ta_codegen/input/<name>/<name>.c` so the source says what it means --
/// a comment describing the operand above it now trails that operand, one written
/// after the condition's `)` now sits inside, and a sentence split over two lines
/// is one line. An entry here is a comment the generator cannot place, so adding
/// one is a claim that no input spelling would have worked.
const UNPLACED: &[(&str, Placement)] = &[];

/// The `//` comments an input body carries, longest first so the greedy match
/// below never stops on a prefix of a longer one.
fn input_comments(path: &Path) -> Vec<String> {
    let Ok(src) = std::fs::read_to_string(path) else { return Vec::new() };
    let mut seen: Vec<String> = src
        .lines()
        .filter_map(|l| l.split_once("//"))
        .map(|(_, c)| c.trim().to_string())
        .filter(|c| !c.is_empty())
        .collect();
    seen.sort();
    seen.dedup();
    seen.sort_by_key(|c| std::cmp::Reverse(c.len()));
    seen
}

/// How many input comments this emitted comment was built from, or `None` if it
/// did not come from the condition at all (a file header, a statement comment).
fn parts(text: &str, inputs: &[String]) -> Option<usize> {
    let (mut rest, mut used) = (text.trim(), 0);
    while !rest.is_empty() {
        let hit = inputs.iter().find(|c| rest.starts_with(c.as_str()))?;
        rest = rest[hit.len()..].trim();
        used += 1;
    }
    (used > 0).then_some(used)
}

/// Split one generated line into `(code before the comment, comment text)`.
fn comments_on(line: &str, block_style: bool) -> Vec<(&str, &str)> {
    if !block_style {
        return line
            .split_once("//")
            .map(|(code, c)| vec![(code, c)])
            .unwrap_or_default();
    }
    let mut out = Vec::new();
    let mut at = 0;
    while let Some(open) = line[at..].find("/*") {
        let open = at + open;
        let Some(close) = line[open..].find("*/") else { break };
        out.push((&line[..open], &line[open + 2..open + close]));
        at = open + close + 2;
    }
    out
}

/// One generated file per backend for `name`, plus that backend's comment style.
fn backend_files(root: &Path, name: &str) -> Vec<(String, PathBuf, bool)> {
    let upper = name.to_uppercase();
    vec![
        ("c".into(), root.join(format!("src/ta_func/ta_{upper}.c")), true),
        (
            "rust".into(),
            root.join(format!("ta_codegen/output/rust/library/src/ta_func/{name}.rs")),
            false,
        ),
        (
            "java".into(),
            root.join(format!("ta_codegen/output/java/fragments/Core_{upper}.java")),
            true,
        ),
        (
            "csharp".into(),
            root.join(format!("ta_codegen/output/csharp/library/src/Core_{upper}.cs")),
            true,
        ),
    ]
}

#[test]
fn every_candlestick_condition_comment_trails_its_own_operand() {
    let root = repo_root();
    let input_dir = root.join("ta_codegen/input");
    let mut offenders: Vec<(String, Placement)> = Vec::new();
    let mut missing: Vec<String> = Vec::new();
    let mut placed = 0usize;
    let mut functions = 0usize;

    let mut dirs: Vec<PathBuf> = std::fs::read_dir(&input_dir)
        .expect("ta_codegen/input is readable")
        .flatten()
        .map(|e| e.path())
        .filter(|p| p.is_dir() && p.file_name().is_some_and(|n| n.to_string_lossy().starts_with("cdl")))
        .collect();
    dirs.sort();

    for dir in dirs {
        let name = dir.file_name().unwrap().to_string_lossy().to_string();
        let inputs = input_comments(&dir.join(format!("{name}.c")));
        if inputs.is_empty() {
            continue;
        }
        functions += 1;
        let listed = UNPLACED.iter().any(|(n, _)| *n == name);
        for (backend, path, block_style) in backend_files(&root, &name) {
            let src = std::fs::read_to_string(&path)
                .unwrap_or_else(|e| panic!("{}: {e}", path.display()));
            let mut seen: Vec<&str> = Vec::new();
            for line in src.lines() {
                for (code, text) in comments_on(line, block_style) {
                    let Some(used) = parts(text, &inputs) else { continue };
                    if used > 1 {
                        offenders.push((name.clone(), Placement::Blob));
                    } else if code.trim().is_empty() {
                        offenders.push((name.clone(), Placement::Hoisted));
                    } else {
                        placed += 1;
                        seen.push(text.trim());
                    }
                }
            }
            // A condition whose rendering falls back to the flat one-liner loses
            // every comment on it at once, and drops no comment into either
            // offender bucket — so only asking which comments ARRIVED sees it.
            for want in &inputs {
                if !seen.iter().any(|s| s == want) && !listed {
                    missing.push(format!("{name}/{backend}: `{want}`"));
                }
            }
        }
    }

    // A corpus that emitted no inline condition comments at all would satisfy the
    // sweep by saying nothing.
    assert!(
        functions > 40 && placed > 3000,
        "only {placed} placed comment(s) across {functions} candlestick(s) — the sweep has nothing to judge"
    );

    assert!(
        missing.is_empty(),
        "condition comment(s) that reached no operand — a fallback to the flat \
         rendering drops them silently:\n  {}",
        missing.join("\n  ")
    );

    offenders.sort();
    offenders.dedup();
    let expected: Vec<(String, Placement)> =
        UNPLACED.iter().map(|(n, p)| ((*n).to_string(), *p)).collect();
    assert_eq!(
        offenders, expected,
        "the set of comments the input's own position cannot place has changed; \
         a fixed input must be dropped from UNPLACED, a regression added"
    );
}

/// The three conditions that lost every inline comment before the spine collector
/// existed: each is rooted at `||`, which the old collector refused outright.
#[test]
fn an_or_rooted_condition_carries_its_comments_inline() {
    let root = repo_root();
    for name in ["cdl3outside", "cdlengulfing", "cdltasukigap"] {
        let inputs = input_comments(&root.join(format!("ta_codegen/input/{name}/{name}.c")));
        assert!(!inputs.is_empty(), "{name} carries no input comments");
        for (backend, path, block_style) in backend_files(&root, name) {
            let src = std::fs::read_to_string(&path)
                .unwrap_or_else(|e| panic!("{}: {e}", path.display()));
            let mut inline = 0usize;
            for line in src.lines() {
                for (code, text) in comments_on(line, block_style) {
                    let Some(used) = parts(text, &inputs) else { continue };
                    assert_eq!(used, 1, "{name}/{backend}: blobbed comment `{text}`");
                    assert!(
                        !code.trim().is_empty(),
                        "{name}/{backend}: comment `{text}` stranded on its own line"
                    );
                    inline += 1;
                }
            }
            assert!(
                inline >= inputs.len(),
                "{name}/{backend}: only {inline} inline comment(s) for {} distinct input comment(s)",
                inputs.len()
            );
        }
    }
}

/// A condition that is a single non-boolean leaf still gets Java's whole-condition
/// `!= 0` coercion when it carries a comment.
///
/// That coercion lives outside the per-operand rules, so rebuilding the line from
/// the operand hooks silently dropped it and emitted `if( intVar )` — a Java type
/// error, and invisible to the renderer's token-identity check, which was
/// measuring against the *uncoerced* string. No shipped input has this shape yet;
/// the first one to arrive would have broken the build, not the values.
#[test]
fn a_lone_non_boolean_leaf_keeps_its_boolean_coercion() {
    use std::cell::Cell;
    use std::collections::{HashMap, HashSet};
    use ta_codegen_lib::backends::java::render_statement;
    use ta_codegen_lib::helper_registry::HelperRegistry;
    use ta_codegen_lib::ir::Statement;

    let src = "\
TA_RetCode f( int isUptrend, int startIdx, int endIdx, int *outBegIdx, int *outNBElement, double outReal[] )
{
    if( isUptrend   // trend is up
      )
    {
        isUptrend = 0;
    }
}
";
    let parsed = ta_codegen_lib::parser::c_source::parse_c_source_str(src);
    let stmt = &parsed.functions[0].body[0];
    let Statement::If { cond_comments, .. } = stmt else { panic!("expected If") };
    assert_eq!(
        cond_comments.len(),
        1,
        "fixture must reach the commented-condition path: {cond_comments:?}"
    );

    let (enums, empty) = (HashMap::new(), HashSet::new());
    let rendered = render_statement(
        stmt,
        0,
        false,
        &enums,
        &common::make_registry(),
        &HelperRegistry::empty(),
        &Cell::new(0),
        &empty,
        &empty,
        &empty,
    );
    assert!(
        rendered.contains("(isUptrend) != 0"),
        "a bare int in Java's condition position needs `!= 0`: {rendered}"
    );
    assert!(
        rendered.contains("trend is up"),
        "the comment must survive the coercion: {rendered}"
    );
}
