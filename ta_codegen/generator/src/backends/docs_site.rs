//! ta-lib.org website generator — writes the generated function pages directly into the
//! VuePress site source tree at `website/src/functions/` (real files served in-tree, no
//! symlink). This is the one `ta_codegen` output that lives under `website/` rather than
//! `ta_codegen/output/`.
//!
//! For each function it reads the canonical documentation source
//! `ta_codegen/input/<dir>/<dir>.md` and emits a website page at
//! `website/src/functions/<dir>.md` (served at `https://ta-lib.org/functions/<name>`), plus
//! a grouped `website/src/functions/index.md`. The page transform is deterministic (SEO
//! front matter + `## See Also` links + the `## Parameters` table), so the output stays
//! byte-stable under the regen oracle.
//!
//! The transform works on the **raw markdown text**, not on the parsed [`crate::ir::DocDef`].
//! That is deliberate: on a filtered run (`generate --func=SMA`) the `FuncDef`s come from
//! `load_all_yaml_defs`, which never attaches `doc`, so a `DocDef`-driven renderer would
//! blank all 166 pages whenever anyone regenerated a single function. Only the YAML
//! metadata — always present — is injected.

use super::doc_meta::{self, RangeMeta};
use crate::ir::{EnumDef, FuncDef, ParamType};
use crate::stability::{self, Stability};
use std::collections::{BTreeMap, HashMap, HashSet};
use std::path::Path;

/// Generate the per-function website pages + index into `website/src/functions/`.
#[allow(clippy::implicit_hasher)]
pub fn generate(funcs: &[FuncDef], enums: &HashMap<String, EnumDef>, root: &Path) {
    let input_base = root.join("ta_codegen/input");
    let out_dir = root.join("website/src/functions");
    std::fs::create_dir_all(&out_dir).expect("create website/src/functions");

    // Stability is derived from the whole call graph (see `crate::stability`), so it must be
    // computed over every function -- `all_funcs` is passed here even on a `--func=X` run.
    let stability = stability::classify(funcs);

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
        let page = transform_page(&body, f, enums, &known, &stability);
        super::write_if_changed_silent(&out_dir.join(format!("{dir}.md")), &page);
        paged.push(f);
    }

    let stability_page = build_stability_page(enums, &stability, &known);
    super::write_if_changed_silent(&out_dir.join("stability.md"), &stability_page);

    let index = build_index(&paged);
    super::write_if_changed(
        &out_dir.join("index.md"),
        &index,
        "website/src/functions",
        paged.len(),
    );

    // Prune stale pages (functions removed since the last run).
    let mut keep: HashSet<String> = paged
        .iter()
        .map(|f| format!("{}.md", f.name.to_lowercase()))
        .collect();
    keep.insert("index.md".to_string());
    keep.insert("stability.md".to_string());
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

/// Prepend SEO front matter (title + description), linkify `## See Also`, and replace the
/// `## Parameters` bullet list with a table carrying the YAML numbers.
fn transform_page(
    body: &str,
    func: &FuncDef,
    enums: &HashMap<String, EnumDef>,
    known: &HashSet<&str>,
    stability: &HashMap<String, Stability>,
) -> String {
    let name = &func.name;
    // Summary is extracted from the untransformed body: it precedes every rewrite.
    let desc = extract_summary(body);
    let injected = inject_parameters(body, func, enums);
    let with_flags = inject_flags(&injected, func, enums, stability);
    let linked = linkify_see_also(&with_flags, known);
    let pruned = strip_empty_sections(&linked);
    let mut out = String::from("---\n");
    out.push_str(&format!("title: {name}\n"));
    if !desc.is_empty() {
        out.push_str(&format!("description: {desc:?}\n"));
    }
    out.push_str("---\n\n");
    out.push_str(&pruned);
    out
}

/// Drop any `## Section` whose body is blank. The input pages carry `## Notes` as an
/// authoring placeholder whether or not there is anything to say (85 of 168 leave it
/// empty), and a bare heading with nothing under it reads as a rendering bug on the
/// site. Only `##` headings are considered — `###` and the `#` title are never sections
/// on these pages — and a section is kept the moment any non-blank line follows it.
fn strip_empty_sections(body: &str) -> String {
    let lines: Vec<&str> = body.lines().collect();
    let mut out: Vec<&str> = Vec::with_capacity(lines.len());
    let mut i = 0;
    while i < lines.len() {
        if lines[i].starts_with("## ") {
            let mut j = i + 1;
            while j < lines.len() && !lines[j].starts_with("## ") {
                j += 1;
            }
            if lines[i + 1..j].iter().all(|l| l.trim().is_empty()) {
                i = j;
                continue;
            }
        }
        out.push(lines[i]);
        i += 1;
    }
    let mut s = out.join("\n");
    s.push('\n');
    s
}

/// Escape a string for inclusion inside a double-quoted HTML attribute.
fn attr_escape(s: &str) -> String {
    s.replace('&', "&amp;").replace('"', "&quot;").replace('<', "&lt;").replace('>', "&gt;")
}

/// Resolve an MAType parameter default (a raw enum value) to its short name, e.g. `EMA`.
fn matype_name(enums: &HashMap<String, EnumDef>, default: Option<f64>) -> Option<String> {
    #[allow(clippy::cast_possible_truncation)]
    let want = default? as i32;
    let e = enums.get("MAType")?;
    e.variants.iter().find(|v| v.value == want).map(|v| v.short_name.clone())
}

/// `["EMA"]` -> `EMA`; `["ADX", "EMA"]` -> `ADX and EMA`.
fn join_and(names: &[String]) -> String {
    match names {
        [] => String::new(),
        [one] => one.clone(),
        [rest @ .., last] => format!("{} and {last}", rest.join(", ")),
    }
}

/// Render one flag cell: checked -> a check plus **bold** label; unchecked -> an
/// outlined empty box (U+2610, not a filled square, so it does not read as a bullet)
/// plus a dimmed label. Each cell trails a focusable `.flag-tip` help badge carrying
/// `tip` in `data-tip`/`aria-label` (styled in `.vuepress/styles/index.scss` as a
/// hover/focus tooltip). Shared by both `## Stability` and `## Display Flags` so they
/// read identically; the dim uses `opacity` (not a fixed color) so it lightens
/// correctly against either the light or dark site theme.
fn flag_cell(label: &str, on: bool, tip: &str) -> String {
    if on {
        let esc = attr_escape(tip);
        let info = format!(
            "<span class=\"flag-tip\" tabindex=\"0\" role=\"note\" aria-label=\"{esc}\" data-tip=\"{esc}\">i</span>"
        );
        format!("<span class=\"flag-box\">✅</span> **{label}** {info}")
    } else {
        // No tooltip: its text would describe a property this function does not have.
        format!("<span class=\"flag-box\">☐</span> <span style=\"opacity:0.5\">{label}</span>")
    }
}

/// Inject the per-function `## Properties` table before `## Implementation` (present
/// on every page), computed from the raw YAML flags — a two-column table, checked
/// cells bold and the rest dimmed, each column's items stacked as rows:
///
/// * **`Numerical Stability`** column — how much the value at a bar depends on the
///   past, folded from two disjoint flags into three mutually-exclusive states
///   (exactly one checked): `Start-Independent` (neither flag; compare across any
///   window), `Initial Unstable Period` (`unstable_period`; converges after a
///   warm-up), `Path-Dependent` (`path_dependent`; a running accumulation or
///   path-dependent state machine that never converges — the behavior behind
///   ta-lib-python issues like #513).
/// * **`Display Flags`** column — `Overlap Input` (output shares the input price
///   scale, drawn over price) and its complement `Independent Y-Axis` (own pane) —
///   one of the two is always checked — plus `Candlestick` (integer pattern signal).
///
/// `stream` (internal codegen concern) and `volume` (in the ABI but set by no
/// function) are not surfaced.
fn inject_flags(
    body: &str,
    func: &FuncDef,
    enums: &HashMap<String, EnumDef>,
    all: &HashMap<String, Stability>,
) -> String {
    let has = |w: &str| func.flags.iter().any(|f| f == w);
    let overlap = has("overlap");

    // Display flags stay a table: they are a two-way pick plus a marker, and nothing about
    // them is stated by negation. Tooltips are emitted for checked cells only -- an
    // unchecked cell carrying its definition puts a sentence describing a property the
    // function does NOT have into the page text, where a crawler or a language model reads
    // it as a claim about this function.
    let display = [
        (
            "Overlap Input",
            overlap,
            "Output is on the same scale as the input price, so it is drawn over the price chart.",
        ),
        (
            "Independent Y-Axis",
            !overlap,
            "Output is on its own scale, drawn in a separate pane below the price chart.",
        ),
        (
            "Candlestick",
            has("candlestick"),
            "Output is an integer candlestick-pattern signal (e.g. -100 / 0 / +100).",
        ),
    ];

    let mut block = String::from("## Properties\n\n");
    block.push_str(&stability_line(func, enums, all));
    block.push_str("\n\n| Display<br>Flags |\n| :-- |\n");
    for (label, on, tip) in display {
        block.push_str(&format!("| {} |\n", flag_cell(label, on, tip)));
    }
    block.push('\n');

    match body.find("\n## Implementation") {
        Some(pos) => {
            let (before, after) = body.split_at(pos + 1);
            format!("{before}{block}{after}")
        }
        None => format!("{body}\n{block}"),
    }
}

/// The one-line numerical-stability statement: the state that applies, linked to its
/// section of the stability reference, plus a clause **only when there is something
/// specific to this function to say** — which inner function it inherits its period from,
/// or what its own MAType default implies. What a state *means* is written once, on the
/// reference page, rather than restated on 168 pages.
///
/// Deliberately prose rather than a checklist of four boxes. A checklist states three
/// facts by negation -- an unchecked box next to `Path-Dependent` -- and negation is
/// exactly what a search snippet, an embedding or a summarising model drops first. Only
/// what is true about this function appears on its page.
fn stability_line(
    func: &FuncDef,
    enums: &HashMap<String, EnumDef>,
    all: &HashMap<String, Stability>,
) -> String {
    let st = all.get(&func.name).cloned().unwrap_or_default();
    let link = |anchor: &str, label: &str| format!("[{label}](/functions/stability#{anchor})");

    let (state, mut reason) = if st.path_dependent {
        // Path-dependence subsumes an unstable period -- nothing converges either way -- but
        // an inherited period still moves the lookback, so ADOSC must not lose that it
        // responds to EMA's setting just because the stronger property won the headline.
        let why = if st.inherited_from.is_empty() {
            String::new()
        } else {
            let names = join_and(&st.inherited_from);
            format!(
                "it also computes {names} internally, so {names}'s unstable period governs how many leading values are discarded."
            )
        };
        (link("path-dependent", "Path-Dependent"), why)
    } else if st.unconditional() {
        // Owning the period is the plain case the reference page already describes; only
        // an inherited one, or an extra contributed by the MA type, is worth saying here.
        let mut why = if st.inherited_from.is_empty() {
            String::new()
        } else {
            let names = join_and(&st.inherited_from);
            format!(
                "inherited from {names}, which {} computes internally; tunable via {names}'s unstable period.",
                func.name
            )
        };
        if st.matype_dependent {
            if why.is_empty() {
                why.push_str("the MA type selected may add one of its own.");
            } else {
                why.push_str(" The MA type selected may add one of its own.");
            }
        }
        (link("initial-unstable-period", "Initial Unstable Period"), why)
    } else if st.matype_dependent {
        (link("depends-on-ma-type", "Depends on MA Type"), matype_reason(func, enums, all))
    } else {
        (link("start-independent", "Start-Independent"), String::new())
    };
    if reason.is_empty() {
        return format!("**Numerical Stability:** {state}");
    }
    if let Some(first) = reason.get_mut(0..1) {
        first.make_ascii_uppercase();
    }
    format!("**Numerical Stability:** {state} — {reason}")
}

/// The reason clause for `Depends on MA Type`. Names no MA type as the stable one — a
/// future MA type need not be recursive — but does state this function's own default, so a
/// reader who never touches the parameter gets an answer without following the link.
fn matype_reason(
    func: &FuncDef,
    enums: &HashMap<String, EnumDef>,
    all: &HashMap<String, Stability>,
) -> String {
    let mut defaults: Vec<String> = Vec::new();
    for opt in &func.optional_inputs {
        if !matches!(&opt.param_type, ParamType::Enum(e) if e == "MAType") {
            continue;
        }
        if let Some(name) = matype_name(enums, opt.default) {
            if !defaults.contains(&name) {
                defaults.push(name);
            }
        }
    }
    let tail = match defaults.as_slice() {
        [] => String::new(),
        [one] => {
            let unstable = all.get(one.as_str()).is_some_and(Stability::unconditional);
            if unstable {
                format!("this function's default, {one}, has an initial unstable period.")
            } else {
                format!("this function's default, {one}, is start-independent.")
            }
        }
        many => {
            let unstable: Vec<String> = many
                .iter()
                .filter(|m| all.get(m.as_str()).is_some_and(Stability::unconditional))
                .cloned()
                .collect();
            if unstable.is_empty() {
                format!("this function's defaults ({}) are start-independent.", many.join(", "))
            } else {
                format!(
                    "of this function's defaults ({}), {} has an initial unstable period.",
                    many.join(", "),
                    join_and(&unstable)
                )
            }
        }
    };
    tail.trim_start().to_string()
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

/// Replace the `## Parameters` bullet list with a table joining the authored prose to the
/// YAML metadata — the render-time injection `docs/ta_codegen_input_doc.md` specifies, so a
/// reader learns a parameter's type, default and accepted values without opening the header.
///
/// A no-op for the 89 functions with no `optional_inputs` (they have no such section).
fn inject_parameters(body: &str, func: &FuncDef, enums: &HashMap<String, EnumDef>) -> String {
    // Validated up-front by `validate_docs`, before any backend has written a file.
    match try_inject_parameters(body, func, enums) {
        Ok(page) => page,
        Err(e) => panic!("{e}"),
    }
}

/// Check that every function's `## Parameters` section can be rendered, without writing
/// anything. Returns every problem found rather than the first, so one run reports the
/// whole backlog. Called before Phase 2 so a documentation typo cannot abort a `generate`
/// that has already rewritten `src/ta_func/*.c`.
///
/// # Errors
/// One message per function whose documentation and YAML disagree.
#[allow(clippy::implicit_hasher)]
pub fn validate_docs(funcs: &[FuncDef], root: &Path) -> Result<(), Vec<String>> {
    let input_base = root.join("ta_codegen/input");
    let enums = HashMap::new();
    let mut errors = Vec::new();
    for f in funcs {
        let dir = f.name.to_lowercase();
        let src = input_base.join(&dir).join(format!("{dir}.md"));
        // A missing .md is not an error here: `generate` warns and skips the page.
        let Ok(body) = std::fs::read_to_string(&src) else {
            continue;
        };
        if let Err(e) = try_inject_parameters(&body, f, &enums) {
            errors.push(e);
        }
        if let Err(e) = validate_inputs(&body, f) {
            errors.push(e);
        }
    }
    if errors.is_empty() {
        Ok(())
    } else {
        Err(errors)
    }
}

/// Check that `## Inputs` names the arrays the function is actually called with.
///
/// The YAML folds a price bar into one named input (`inPriceHLC` + `price_components`)
/// because `ta_abstract` — and through it every wrapper that reads the abstract API —
/// needs a single `TA_Input_Price` descriptor. That fold is an introspection detail: the
/// C, Rust and Java signatures all take the components as separate arrays
/// (`TA_STOCH(startIdx, endIdx, inHigh, inLow, inClose, ...)`), so a page documenting
/// `inPriceHLC` names a parameter the reader cannot pass. `parser/yaml.rs` expands the
/// bundle, so `func.inputs` is already the real argument list — require the prose to
/// match it in name and order, exactly as `## Parameters` must match `optional_inputs`.
///
/// Unlike `## Parameters` this section is passed through to the page verbatim; the check
/// exists because nothing else joins it to the IR. `rust_doc::input_desc` looks the
/// authored prose up by expanded name and silently falls back to canned text on a miss,
/// which is what let 94 pages drift onto the bundle name unnoticed.
fn validate_inputs(body: &str, func: &FuncDef) -> Result<(), String> {
    let lines: Vec<&str> = body.lines().collect();
    let Some((heading, end)) = section_span(&lines, "## Inputs") else {
        return Err(format!(
            "{}: {}.md has no `## Inputs` section, but the function takes {} input(s)",
            func.name,
            func.name.to_lowercase(),
            func.inputs.len()
        ));
    };

    let section = &lines[heading + 1..end];
    let items = bullet_items(section);
    let prose = named_bullets(section);
    if items.len() != prose.len() {
        return Err(format!(
            "{}: `## Inputs` has {} bullet(s) that are not `- `name` — description`",
            func.name,
            items.len() - prose.len()
        ));
    }

    let names: Vec<&str> = prose.iter().map(|(n, _)| n.as_str()).collect();
    let expected: Vec<&str> = func.inputs.iter().map(|i| i.name.as_str()).collect();
    if names != expected {
        return Err(format!(
            "{}: `## Inputs` bullets {names:?} do not match the call signature {expected:?} \
             — the two must agree in name and order. A price bundle (`inPriceHLC`) is an \
             abstract-API descriptor, not an argument: document its components separately.",
            func.name
        ));
    }
    Ok(())
}

/// The fallible core of [`inject_parameters`], so the same rules can gate a run before
/// anything is written.
fn try_inject_parameters(
    body: &str,
    func: &FuncDef,
    enums: &HashMap<String, EnumDef>,
) -> Result<String, String> {
    if func.optional_inputs.is_empty() {
        return Ok(body.to_string());
    }
    let lines: Vec<&str> = body.lines().collect();
    let Some((heading, end)) = section_span(&lines, "## Parameters") else {
        return Err(format!(
            "{}: YAML declares {} optional input(s) but {}.md has no `## Parameters` section",
            func.name,
            func.optional_inputs.len(),
            func.name.to_lowercase()
        ));
    };

    let section = &lines[heading + 1..end];
    // The section is replaced wholesale by the table, so any block in it that is not a
    // parameter bullet would vanish (before the first bullet) or be folded into the
    // preceding description (after one). `## Parameters` is specified as a
    // name-to-meaning list (docs/ta_codegen_input_doc.md), so reject the rest loudly
    // rather than delete an author's sentence without telling them. A *wrapped* bullet
    // is fine and stays supported: it continues the line above with no blank line
    // between, whereas a stray paragraph or sub-heading starts its own block.
    let strays: Vec<&str> = section
        .iter()
        .enumerate()
        .filter(|(i, l)| {
            let starts_block = *i == 0 || section[i - 1].trim().is_empty();
            starts_block && !l.trim().is_empty() && !is_bullet_line(l)
        })
        .map(|(_, l)| *l)
        .collect();
    if !strays.is_empty() {
        return Err(format!(
            "{}: `## Parameters` opens a block that is not a `- `name` — description` \
             bullet, which the rendered table would drop: {strays:?}",
            func.name
        ));
    }

    // Every bullet must be a named one. An item that fails the backtick parse is dropped
    // by `named_bullets`, and dropping it silently would both delete it from the page and
    // slip past the name check below, which only sees what parsed.
    let items = bullet_items(section);
    let prose = named_bullets(section);
    if items.len() != prose.len() {
        let unparsed: Vec<&String> = items
            .iter()
            .filter(|i| !i.starts_with('`') || !i[1..].contains('`'))
            .collect();
        return Err(format!(
            "{}: `## Parameters` has {} bullet(s) that are not \
             `- `name` — description` and would be dropped: {unparsed:?}",
            func.name,
            items.len() - prose.len()
        ));
    }

    // The join is by position *and* name: the authored bullets must mirror the YAML
    // exactly. They do for all 120 parameters today, so a mismatch means a parameter was
    // renamed or added on one side only — say so rather than silently drop a description.
    let names: Vec<&str> = prose.iter().map(|(n, _)| n.as_str()).collect();
    let expected: Vec<&str> = func
        .optional_inputs
        .iter()
        .map(|o| o.name.as_str())
        .collect();
    if names != expected {
        return Err(format!(
            "{}: `## Parameters` bullets {names:?} do not match the YAML optional_inputs \
             {expected:?} — the two must agree in name and order",
            func.name
        ));
    }

    let mut table = vec![
        "| Parameter | Type | Default | Accepted values | Description |".to_string(),
        "| --- | --- | --- | --- | --- |".to_string(),
    ];
    for (opt, (_, desc)) in func.optional_inputs.iter().zip(prose.iter()) {
        let m = doc_meta::param_meta(opt, enums);
        let default = match (&m.default_variant, &m.default) {
            (Some(variant), Some(value)) => format!("{variant} ({value})"),
            (_, Some(value)) => value.clone(),
            (_, None) => "—".to_string(),
        };
        table.push(format!(
            "| `{}` | {} | {} | {} | {} |",
            opt.name,
            m.type_label,
            default,
            accepted_values(&m),
            escape_cell(desc)
        ));
    }

    let mut out: Vec<String> = lines[..=heading].iter().map(|s| (*s).to_string()).collect();
    out.push(String::new());
    out.extend(table);
    for legend in enum_legends(func, enums) {
        out.push(String::new());
        out.push(legend);
    }
    out.push(String::new());
    out.extend(lines[end..].iter().map(|s| (*s).to_string()));
    let mut s = out.join("\n");
    s.push('\n');
    Ok(s)
}

/// A line that [`named_bullets`] will turn into a `(name, description)` pair.
fn is_bullet_line(line: &str) -> bool {
    let Some(item) = line.trim().strip_prefix("- ") else {
        return false;
    };
    item.trim()
        .strip_prefix('`')
        .is_some_and(|rest| rest.contains('`'))
}

/// The `Accepted values` cell: a numeric domain for integer/real parameters, `any <type>`
/// for an enum (whose admissible set is spelled out once per page by [`enum_legends`]).
/// `TA_REAL_MIN`/`TA_REAL_MAX` bounds constrain nothing, so they read as `any real` or a
/// one-sided `≥` rather than as ±1.8e308.
fn accepted_values(m: &doc_meta::ParamMeta) -> String {
    match &m.range {
        RangeMeta::Bounded(lo, hi) => format!("{lo}–{hi}"),
        RangeMeta::Min(lo) => format!("≥ {lo}"),
        RangeMeta::Max(hi) => format!("≤ {hi}"),
        RangeMeta::Unbounded => format!("any {}", m.type_label),
    }
}

/// One italic legend per distinct enum type used on the page, spelling out its admissible
/// values with their numeric codes. Emitted once rather than per row because MACDEXT takes
/// three `MAType` parameters and STOCH two — repeating nine names in every row would
/// dominate the table's width to no benefit.
fn enum_legends(func: &FuncDef, enums: &HashMap<String, EnumDef>) -> Vec<String> {
    let mut seen: Vec<&str> = Vec::new();
    let mut out = Vec::new();
    for opt in &func.optional_inputs {
        let ParamType::Enum(name) = &opt.param_type else {
            continue;
        };
        if seen.contains(&name.as_str()) {
            continue;
        }
        seen.push(name);
        let Some(def) = enums.get(name) else { continue };
        let values: Vec<String> = def
            .variants
            .iter()
            .map(|v| format!("{} {}", v.value, v.short_name))
            .collect();
        out.push(format!("*`{name}` values: {}*", values.join(" · ")));
    }
    out
}

/// Escape prose for a GFM table cell: an unescaped `|` would end the cell early
/// (SAREXT's `optInStartValue` description contains `|value|`).
fn escape_cell(text: &str) -> String {
    text.replace('|', "\\|")
}

/// The half-open line span of a `## ` section: `(heading index, index of the next `## `)`.
fn section_span(lines: &[&str], heading: &str) -> Option<(usize, usize)> {
    let start = lines.iter().position(|l| l.trim() == heading)?;
    let end = lines[start + 1..]
        .iter()
        .position(|l| l.starts_with("## "))
        .map_or(lines.len(), |i| start + 1 + i);
    Some((start, end))
}

/// Collect `- ` items, folding a wrapped continuation line into the item above.
///
/// Deliberately staged exactly like `parser::doc_md::bullets`: continuations are merged
/// **before** any name parsing, so an item that later fails to parse takes its own
/// continuation lines with it instead of donating them to the previous parameter. Merging
/// after the parse — the obvious shortcut — makes this renderer disagree with the rustdoc
/// one on malformed input, which is the single thing sharing `doc_meta` exists to prevent.
fn bullet_items(lines: &[&str]) -> Vec<String> {
    let mut items: Vec<String> = Vec::new();
    for line in lines {
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

/// Parse ``- `name` — description`` bullets. Mirrors `parser::doc_md::named_bullets`,
/// including its single-separator strip: a CDL output description legitimately begins with
/// `-` (`-100 on a bearish pattern`), so exactly one separator character comes off.
fn named_bullets(lines: &[&str]) -> Vec<(String, String)> {
    bullet_items(lines)
        .iter()
        .filter_map(|item| {
            let rest = item.strip_prefix('`')?;
            let (name, after) = rest.split_once('`')?;
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

/// Turn `## See Also` entries (`ADX · DX · …`) into source-root-absolute, extensionless
/// links (`/functions/<name>`) to sibling pages, leaving any non-function token untouched.
/// Absolute (not bare-relative) so VuePress resolves them even though the pages are served
/// from a symlink outside the site source root; extensionless (not `.md`/`.html`) so
/// VuePress renders a real `<a>` server-side — the `.md` form only hydrates client-side
/// (empty `<!---->` in SSR), and the `.html` form does the same.
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
                            format!("[{n}](/functions/{})", n.to_lowercase())
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

/// The `/functions/stability` reference: the four numerical-stability categories, each with a
/// stable anchor every function page links to.
///
/// Headings are phrased conditionally and trail off ("If Initial Unstable Period, then...")
/// so a reader landing mid-page sees that a section describes one case out of four, not a
/// property of every function, and that the answer is the section body. Their anchors are pinned with `{#id}` (markdown-it-attrs, enabled by the
/// theme) rather than left to the slugifier, so the ids stay short and, more importantly,
/// survive any future rewording of the headings -- 168 generated pages link to them.
///
/// Generated, never hand-written: the MA-type table under `#depends-on-ma-type` is not
/// readable off the YAML (DEMA and TEMA declare no unstable period of their own yet inherit
/// EMA's), and a new MA type — HMA in 0.8.1, `DISABLED` in #93 — must not require anyone to
/// remember to edit prose.
fn build_stability_page(
    enums: &HashMap<String, EnumDef>,
    stability: &HashMap<String, Stability>,
    known: &HashSet<&str>,
) -> String {
    let mut s = String::from(
        "---\ntitle: Numerical Stability\ndescription: \"What it means for an indicator to be start-independent, to carry an initial unstable period, to depend on the MA type selected, or to be path-dependent.\"\n---\n\n",
    );
    s.push_str("# Numerical Stability\n\n");
    s.push_str(
        "The [Function Documentation](/functions/) specifies which of the four categories below \
         applies to each function. They answer a single \
         practical question: **does the value at a given bar depend on how much history you \
         passed in?**\n\n",
    );

    s.push_str("## If Start-Independent, then... {#start-independent}\n\n");
    s.push_str(
        "The value at a bar does not depend on where your data starts. Feed the function a \
         year or a decade and the value it reports for a given bar is identical. These \
         functions read a bounded window — a fixed number of bars — and ignore everything \
         older.\n\n",
    );

    s.push_str("## If Initial Unstable Period, then... {#initial-unstable-period}\n\n");
    s.push_str(
        "Early values depend on how much history precedes them, and converge as more bars are \
         supplied. These functions are defined recursively: each value folds in the previous \
         one, so the series never entirely forgets where it began — though the influence decays \
         until it is lost in floating-point rounding.\n\n\
         See [Unstable Period](/api/unstable-period/) for what to do about it: when to \
         ignore it, when to supply extra history, and how to have TA-Lib drop the \
         unstable values for you.\n\n",
    );

    s.push_str("## If Depends on MA Type, then... {#depends-on-ma-type}\n\n");
    s.push_str(
        "Some functions take an `optInMAType` parameter selecting how their moving average is \
         computed. That choice decides which of the properties above applies: a recursive MA \
         type gives the function an initial unstable period, a windowed one leaves it \
         start-independent.\n\n",
    );
    s.push_str("| MA Type | Value | Numerical Stability | Why |\n| :-- | --: | :-- | :-- |\n");
    if let Some(e) = enums.get("MAType") {
        let mut variants = e.variants.clone();
        variants.sort_by_key(|v| v.value);
        for v in &variants {
            let name = &v.short_name;
            let (state, why) = match stability.get(name.as_str()) {
                None => (
                    "Start-Independent",
                    "Not a moving average: the input is copied through unchanged.".to_string(),
                ),
                Some(st) if st.intrinsic => (
                    "Initial Unstable Period",
                    format!("Recursive: each value folds in the previous one. Tunable via {name}'s own unstable period."),
                ),
                Some(st) if !st.inherited_from.is_empty() => (
                    "Initial Unstable Period",
                    format!("Built from {}, and inherits its unstable period.", join_and(&st.inherited_from)),
                ),
                Some(_) => (
                    "Start-Independent",
                    "A windowed average: it reads a fixed number of bars and forgets everything older.".to_string(),
                ),
            };
            let linked = if known.contains(name.as_str()) {
                format!("[{name}](/functions/{})", name.to_lowercase())
            } else {
                format!("`{name}`")
            };
            s.push_str(&format!("| {linked} | {} | {state} | {why} |\n", v.value));
        }
    }
    s.push('\n');

    s.push_str("## If Path-Dependent, then... {#path-dependent}\n\n");
    s.push_str(
        "The value is built up from the first bar — a running accumulation or a state machine \
         that tracks the path prices took — so it depends on where your data begins and never \
         converges. Unlike an unstable period, there is no warm-up you can discard: the \
         difference persists for the whole series.\n\n\
         Two Examples:\n\n\
         - [AD](/functions/ad) adds each bar's money-flow volume to a running total that begins \
         at zero on your first bar. Only the differences between bars carry meaning; the \
         absolute level is an artifact of the start date.\n\
         - [SAR](/functions/sar) is a state machine: it reads the first two bars to decide \
         whether the trend starts long or short, then carries that direction, the extreme \
         price, and an acceleration factor forward. Start a day earlier and it can pick the \
         opposite direction, putting the stop on the other side of price for the rest of the \
         run.\n\n\
         Do not compare these values across differently-sized windows, and expect a backtest \
         starting at a different date to produce different numbers.\n",
    );
    s
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
            s.push_str(&format!("- [{}](/functions/{dir}) — {hint}\n", f.name));
        }
    }
    s
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::ir::{EnumVariant, Input, OptInput, PriceComponent, PriceRef};

    fn func(name: &str, opts: Vec<OptInput>) -> FuncDef {
        FuncDef {
            name: name.to_string(),
            group: "Overlap Studies".to_string(),
            description: None,
            camel_case: None,
            hint: None,
            flags: vec![],
            inputs: vec![Input::new("inReal", ParamType::Real)],
            optional_inputs: opts,
            outputs: vec![],
            lookback: None,
            body: vec![],
            private_body: vec![],
            private_extra_params: vec![],
            private_param_init: vec![],
            has_explicit_private: false,
            header_comments: vec![],
            doc: None,
            streaming: false,
        }
    }

    /// A function taking a price bar, i.e. after `parser/yaml.rs` has expanded
    /// `inPriceHLC` into the three arrays the signature really has.
    fn hlc_func(name: &str) -> FuncDef {
        let mut f = func(name, vec![]);
        f.inputs = [
            PriceComponent::High,
            PriceComponent::Low,
            PriceComponent::Close,
        ]
        .iter()
        .map(|c| Input {
            name: c.input_name().to_string(),
            param_type: ParamType::Real,
            price: Some(PriceRef {
                group: 0,
                component: *c,
            }),
        })
        .collect();
        f
    }

    /// The bundle name is an `ta_abstract` descriptor, never a parameter. Documenting it
    /// (as 94 pages did) tells the reader to pass an array that does not exist.
    #[test]
    fn the_price_bundle_name_is_rejected_in_inputs() {
        let body = "# X\n\n## Inputs\n\n- `inPriceHLC` — High/Low/Close price series\n\n## Outputs\n";
        let err = validate_inputs(body, &hlc_func("X")).unwrap_err();
        assert!(err.contains("do not match the call signature"), "{err}");
        assert!(err.contains("inHigh"), "{err}");
    }

    /// The components, in signature order, are what the caller actually passes.
    #[test]
    fn expanded_components_in_signature_order_are_accepted() {
        let body = "# X\n\n## Inputs\n\n- `inHigh` — High price\n- `inLow` — Low price\n\
                    - `inClose` — Close price\n\n## Outputs\n";
        assert!(validate_inputs(body, &hlc_func("X")).is_ok());
    }

    /// Order is part of the contract: the bullets document positional arguments.
    #[test]
    fn components_out_of_signature_order_are_rejected() {
        let body = "# X\n\n## Inputs\n\n- `inClose` — Close price\n- `inHigh` — High price\n\
                    - `inLow` — Low price\n\n## Outputs\n";
        assert!(validate_inputs(body, &hlc_func("X")).is_err());
    }

    /// A partially documented bar (NVI/PVI/PVO authored components while their YAML
    /// declared a bundle — nothing caught the disagreement) must not pass.
    #[test]
    fn a_missing_component_is_rejected() {
        let body = "# X\n\n## Inputs\n\n- `inHigh` — High price\n- `inLow` — Low price\n\n## Outputs\n";
        assert!(validate_inputs(body, &hlc_func("X")).is_err());
    }

    #[test]
    fn a_missing_inputs_section_is_rejected() {
        let body = "# X\n\n## Outputs\n\n- `outReal` — Values\n";
        let err = validate_inputs(body, &hlc_func("X")).unwrap_err();
        assert!(err.contains("no `## Inputs` section"), "{err}");
    }

    fn opt(name: &str, pt: ParamType, range: Option<(f64, f64)>, default: f64) -> OptInput {
        OptInput {
            name: name.to_string(),
            param_type: pt,
            range,
            default: Some(default),
            display_name: None,
            hint: None,
            flags: vec![],
            suggested: None,
            precision: None,
        }
    }

    fn ma_type() -> HashMap<String, EnumDef> {
        let variants = [("SMA", 0), ("EMA", 1), ("WMA", 2)]
            .iter()
            .map(|(n, v)| EnumVariant {
                c_name: format!("TA_MAType_{n}"),
                pascal_name: (*n).to_string(),
                short_name: (*n).to_string(),
                value: *v,
            })
            .collect();
        let mut m = HashMap::new();
        m.insert(
            "MAType".to_string(),
            EnumDef {
                name: "MAType".to_string(),
                variants,
            },
        );
        m
    }

    const PAGE: &str = "# X\n\n## Parameters\n\n- `optInTimePeriod` — Window length\n\n## Implementation\n\nkeep me\n";

    #[test]
    fn parameters_become_a_table_with_the_yaml_numbers() {
        let f = func(
            "X",
            vec![opt(
                "optInTimePeriod",
                ParamType::Integer,
                Some((1.0, 100_000.0)),
                30.0,
            )],
        );
        let out = inject_parameters(PAGE, &f, &HashMap::new());
        assert!(out.contains("| `optInTimePeriod` | integer | 30 | 1–100000 | Window length |"));
        // Surrounding sections survive untouched.
        assert!(out.contains("## Implementation\n\nkeep me"));
        assert!(!out.contains("- `optInTimePeriod`"));
    }

    /// The 89 functions with no optional inputs have no `## Parameters` section at all.
    #[test]
    fn no_optional_inputs_is_a_no_op() {
        let f = func("X", vec![]);
        let body = "# X\n\n## Inputs\n\n- `inReal` — Series\n";
        assert_eq!(inject_parameters(body, &f, &HashMap::new()), body);
    }

    /// BBANDS' `optInNbDevUp`: both bounds are `TA_REAL_MIN`/`TA_REAL_MAX` sentinels, which
    /// constrain nothing — the cell must say so rather than print ±1.8e308.
    #[test]
    fn sentinel_range_reads_as_any_of_the_type() {
        let f = func(
            "X",
            vec![opt(
                "optInTimePeriod",
                ParamType::Real,
                Some((f64::MIN, f64::MAX)),
                2.0,
            )],
        );
        let out = inject_parameters(PAGE, &f, &HashMap::new());
        assert!(out.contains("| `optInTimePeriod` | real | 2 | any real |"));
    }

    /// APO/PPO/PVO default to `1 = EMA`: the variant is looked up by value, not by index.
    /// The admissible set goes in a single per-page legend, not into every row.
    #[test]
    fn enum_names_the_default_by_value_and_legends_the_variants_once() {
        let f = func(
            "X",
            vec![
                opt(
                    "optInTimePeriod",
                    ParamType::Enum("MAType".to_string()),
                    None,
                    1.0,
                ),
                opt(
                    "optInSecond",
                    ParamType::Enum("MAType".to_string()),
                    None,
                    0.0,
                ),
            ],
        );
        let body = "# X\n\n## Parameters\n\n- `optInTimePeriod` — Window length\n- `optInSecond` — Second\n";
        let out = inject_parameters(body, &f, &ma_type());
        assert!(out.contains("| `optInTimePeriod` | MAType | EMA (1) | any MAType |"));
        assert!(out.contains("| `optInSecond` | MAType | SMA (0) | any MAType |"));
        // One legend for the shared enum type, not one per parameter.
        assert_eq!(out.matches("`MAType` values:").count(), 1);
        assert!(out.contains("*`MAType` values: 0 SMA · 1 EMA · 2 WMA*"));
    }

    /// SAREXT's `optInStartValue` prose contains `|value|`, which would end the cell early.
    #[test]
    fn pipes_in_prose_are_escaped() {
        let f = func(
            "X",
            vec![opt("optInStartValue", ParamType::Real, Some((0.0, f64::MAX)), 0.0)],
        );
        let body = "# X\n\n## Parameters\n\n- `optInStartValue` — start short at |value|\n";
        let out = inject_parameters(body, &f, &HashMap::new());
        assert!(out.contains(r"start short at \|value\| |"));
        assert!(out.contains("| ≥ 0 |"));
    }

    /// A parameter renamed in the YAML but not in the prose would otherwise silently lose
    /// its description; the golden rule only holds if the two stay in lockstep.
    #[test]
    #[should_panic(expected = "do not match the YAML optional_inputs")]
    fn a_name_mismatch_fails_loudly() {
        let f = func(
            "X",
            vec![opt("optInRenamed", ParamType::Integer, Some((1.0, 9.0)), 3.0)],
        );
        inject_parameters(PAGE, &f, &HashMap::new());
    }

    /// The table replaces the whole section, so a stray paragraph would be deleted (before
    /// the first bullet) or swallowed into the last cell (after one). Refuse instead.
    #[test]
    fn stray_prose_in_the_section_is_rejected_not_deleted() {
        let f = func(
            "X",
            vec![opt("optInTimePeriod", ParamType::Integer, Some((1.0, 9.0)), 3.0)],
        );
        let leading = "# X\n\n## Parameters\n\nAll periods are in bars.\n\n- `optInTimePeriod` — Window length\n";
        let err = try_inject_parameters(leading, &f, &HashMap::new()).unwrap_err();
        assert!(err.contains("All periods are in bars."), "{err}");

        let trailing = "# X\n\n## Parameters\n\n- `optInTimePeriod` — Window length\n\n### Notes on tuning\n";
        let err = try_inject_parameters(trailing, &f, &HashMap::new()).unwrap_err();
        assert!(err.contains("Notes on tuning"), "{err}");
    }

    fn stability_of(name: &str, flags: &[&str], opts: Vec<crate::ir::OptInput>) -> FuncDef {
        let mut f = func(name, opts);
        f.flags = flags.iter().map(|s| (*s).to_string()).collect();
        f
    }

    /// The stability statement must assert only what is true of this function: no unchecked
    /// boxes, no definitions of states that do not apply. A crawler or a language model
    /// reading the page text must not find `Path-Dependent` prose on a page that is not.
    #[test]
    fn stability_line_states_only_the_applicable_property() {
        let enums = HashMap::new();
        let f = stability_of("SMA", &[], vec![]);
        let mut all = HashMap::new();
        all.insert("SMA".to_string(), Stability::default());
        let line = stability_line(&f, &enums, &all);
        assert_eq!(
            line,
            "**Numerical Stability:** [Start-Independent](/functions/stability#start-independent)",
            "the plain case is the bare state: what it means belongs on the reference page"
        );

        // Transitive: names the inner function, twice (what it inherits, what to tune).
        let f = stability_of("DEMA", &[], vec![]);
        let mut all = HashMap::new();
        all.insert(
            "DEMA".to_string(),
            Stability { inherited_from: vec!["EMA".into()], ..Stability::default() },
        );
        let line = stability_line(&f, &enums, &all);
        assert!(line.contains("Inherited from EMA, which DEMA computes internally"), "{line}");
        assert!(line.contains("tunable via EMA's unstable period"), "{line}");

        // An intrinsically-unstable function says nothing extra: owning the period is the
        // plain case the reference page describes.
        let f = stability_of("EMA", &["unstable_period"], vec![]);
        let mut all = HashMap::new();
        all.insert("EMA".to_string(), Stability { intrinsic: true, ..Stability::default() });
        assert_eq!(
            stability_line(&f, &enums, &all),
            "**Numerical Stability:** [Initial Unstable Period](/functions/stability#initial-unstable-period)"
        );

        // Combination: unstable regardless *and* MA-type dependent -> both stated.
        let f = stability_of("STOCHRSI", &[], vec![]);
        let mut all = HashMap::new();
        all.insert(
            "STOCHRSI".to_string(),
            Stability {
                inherited_from: vec!["RSI".into()],
                matype_dependent: true,
                ..Stability::default()
            },
        );
        let line = stability_line(&f, &enums, &all);
        assert!(line.contains("Inherited from RSI"), "{line}");
        assert!(line.contains("may add one of its own"), "{line}");

        // Path-dependent wins over everything: it never converges.
        let f = stability_of("OBV", &["path_dependent"], vec![]);
        let mut all = HashMap::new();
        all.insert(
            "OBV".to_string(),
            Stability { path_dependent: true, ..Stability::default() },
        );
        let line = stability_line(&f, &enums, &all);
        assert_eq!(
            line,
            "**Numerical Stability:** [Path-Dependent](/functions/stability#path-dependent)",
            "no trailing prose when there is nothing specific to this function to say"
        );

        // ...but a path-dependent function that also inherits a period keeps that clause,
        // because the period still moves its lookback (ADOSC through EMA).
        let f = stability_of("ADOSC", &["path_dependent"], vec![]);
        let mut all = HashMap::new();
        all.insert(
            "ADOSC".to_string(),
            Stability {
                path_dependent: true,
                inherited_from: vec!["EMA".into()],
                ..Stability::default()
            },
        );
        let line = stability_line(&f, &enums, &all);
        assert!(line.contains("computes EMA internally"), "{line}");
    }

    /// An unchecked display cell must not carry a tooltip: its text would describe a
    /// property the function does not have.
    #[test]
    fn only_checked_cells_carry_their_definition() {
        let on = flag_cell("Candlestick", true, "Output is an integer pattern signal.");
        assert!(on.contains("data-tip=\"Output is an integer pattern signal.\""), "{on}");
        let off = flag_cell("Candlestick", false, "Output is an integer pattern signal.");
        assert!(!off.contains("data-tip"), "{off}");
        assert!(!off.contains("integer pattern signal"), "{off}");
        assert!(off.contains("Candlestick"), "the label still shows: {off}");
    }

    /// An empty `## Notes` placeholder must not reach the site, while a section with any
    /// content — including one whose body is only the `###` subheadings — is untouched.
    #[test]
    fn empty_sections_are_dropped_and_populated_ones_kept() {
        let body = "# X\n\n## Formula\n\n$$a = b$$\n\n## Notes\n\n\n\n## Inputs\n\n- `inReal`\n";
        let out = strip_empty_sections(body);
        assert!(!out.contains("## Notes"), "{out}");
        assert!(out.contains("## Formula") && out.contains("$$a = b$$"), "{out}");
        assert!(out.contains("## Inputs") && out.contains("- `inReal`"), "{out}");
        assert!(out.contains("# X"), "the title is not a section: {out}");

        // Trailing empty section (no following heading to close it) is dropped too.
        assert!(!strip_empty_sections("# X\n\n## Notes\n\n").contains("## Notes"));
        // A section holding only a subheading is content, not empty.
        let sub = "# X\n\n## Notes\n\n### Tuning\n\ntext\n";
        assert!(strip_empty_sections(sub).contains("## Notes"), "{sub}");
    }

    /// A bullet wrapped over two lines is legitimate authoring — it must keep working, and
    /// the continuation belongs to the description.
    #[test]
    fn a_wrapped_bullet_is_joined_not_rejected() {
        let f = func(
            "X",
            vec![opt("optInTimePeriod", ParamType::Integer, Some((1.0, 9.0)), 3.0)],
        );
        let body = "# X\n\n## Parameters\n\n- `optInTimePeriod` — Window length,\n  measured in bars\n";
        let out = try_inject_parameters(body, &f, &HashMap::new()).unwrap();
        assert!(out.contains("| Window length, measured in bars |"), "{out}");
    }

    /// The exact case the review reproduced: a non-named bullet tucked directly under a
    /// parameter bullet (no blank line, so the stray-block check does not see it). Its
    /// wrapped line used to be grafted onto the preceding parameter's description on the
    /// website while rustdoc dropped the whole item — the two surfaces silently disagreeing.
    #[test]
    fn an_unparseable_bullet_cannot_graft_itself_onto_the_previous_parameter() {
        let f = func(
            "X",
            vec![opt("optInTimePeriod", ParamType::Integer, Some((1.0, 9.0)), 3.0)],
        );
        let body = "# X\n\n## Parameters\n\n- `optInTimePeriod` — Window length\n- Note: interacts with\n  the unstable period\n";
        let err = try_inject_parameters(body, &f, &HashMap::new()).unwrap_err();
        assert!(err.contains("would be dropped"), "{err}");
        assert!(err.contains("Note: interacts with"), "{err}");
    }

    /// `named_bullets` must stage exactly like `parser::doc_md::named_bullets`: a failed
    /// item swallows its own continuation rather than donating it to the item above.
    #[test]
    fn continuations_are_merged_before_the_name_parse() {
        let lines = vec![
            "- `optInA` — First",
            "- Note: stray",
            "  continuation of the stray",
        ];
        let parsed = named_bullets(&lines);
        assert_eq!(parsed.len(), 1);
        assert_eq!(parsed[0].1, "First");
    }

    #[test]
    #[should_panic(expected = "has no `## Parameters` section")]
    fn a_missing_section_fails_loudly() {
        let f = func(
            "X",
            vec![opt("optInTimePeriod", ParamType::Integer, Some((1.0, 9.0)), 3.0)],
        );
        inject_parameters("# X\n\n## Inputs\n\n- `inReal` — Series\n", &f, &HashMap::new());
    }
}
