---
title: "ta_codegen Input: Documentation (<name>.md) Reference"
---

# ta_codegen Input: Documentation (`<name>.md`) Reference

> **Status.** Every function has a `<name>.md`, and `ta_codegen` renders it to four
> targets: the **ta-lib.org** page (`backends/docs_site.rs` → `website/src/functions/`),
> the **rustdoc** in the Rust crate (`backends/rust_doc.rs`, with a runnable doctest per
> function), the **Javadoc** (`backends/java_doc.rs`) and the **C# XML doc**
> (`backends/csharp_doc.rs`). The YAML numbers for optional inputs come from the shared
> `backends/doc_meta.rs`. `generate` rejects a `.md` whose `## Parameters` or `## Inputs`
> are missing, malformed, or disagree with the YAML in name or order, one carrying a bare
> URL, and one carrying an authored `## Implementation`. Not built: YAML facts injected into `## Inputs` /
> `## Outputs`, a check of prose numbers against the YAML, a section-order check, a `.md`
> canonicaliser, and the npm/TSDoc render. See the sibling references for the two other
> input file kinds: [metadata](ta_codegen_input_yaml.md) and [code](ta_codegen_input_code.md).

`<name>.md` is the **third sibling** in each `ta_codegen/input/<name>/` directory,
alongside `<name>.yaml` (metadata) and `<name>.c` (logic). It is the **single canonical
source** for a function's human documentation. The generator fans it out — deterministically,
like every other backend — into each ecosystem's native format:

- the **ta-lib.org** per-function page (VuePress, `website/src/functions/`),
- **embedded rustdoc** in the generated Rust crate (docs.rs / IDE hover / offline `cargo doc`),
- **Javadoc** and the **C# XML doc** on the generated `Core` methods,
- later, **TSDoc** and the `ta_func_api.xml` description.

Documentation is **authored once**, then **embedded** into each package — never merely
linked. (A crate whose docs just link to ta-lib.org shows nothing on hover, nothing
offline, and is invisible to docs.rs search.) Hyperlinks out to ta-lib.org are reserved
for the *narrative* layer (charts, deeper prose) and for npm's README.

## The golden rule: numbers live in YAML, prose lives in `.md`

The single most important constraint, and the thing that keeps docs from drifting:

- **Numbers and structure** — parameter ranges, defaults, `suggested` triples, input/output
  arity, types, and flags — live **only** in [`<name>.yaml`](ta_codegen_input_yaml.md) and
  are **injected at render time**. Never restate them in prose.
- **Prose** — summary (with interpretation), brief formula, notes, per-argument meaning,
  references — lives **only** in `<name>.md`.

A consequence of the split: **most metadata changes (a widened range, a new default) need
no doc edit at all** — the render just picks up the new number.

`generate` rejects a `## Parameters` list that does not match the YAML `optional_inputs` in
name and order, so a parameter renamed on one side only cannot ship. Nothing checks a prose
number against the YAML, and that check cannot be a blanket "no digits in a structured
section" rule — roughly 80 bullets carry legitimate
non-YAML numbers (the `CDL*` ±100 sign convention, output domains like WILLR's −100…0,
MACDFIX's hard-coded 12/26 constants that live in `macdfix.c`). It has to compare a number
against the specific YAML field it names.

## File format

A `<name>.md` file has **no frontmatter**. It opens with an `#` H1 = the TA-Lib function
name, then a fixed set of `##` sections, in the order below.

### Sections (in this order)

Numbers/ranges/defaults are **injected from YAML** at render — never restate them here.

| Section | Required | Content |
|---------|----------|---------|
| `# <NAME>` | yes | H1 = the TA-Lib function name (e.g. `# RSI`). |
| `## Summary` | yes | One prose block: what the function is + a brief intro **and** how to read its output. Interpretation is merged in — there is **no** separate `Interpretation` section. |
| `## Formula` | optional | A **brief, high-level** formula, only when the computation is expressible concisely. Omit for long detection lists (`CDL*`) or long DSP computations (`HT_*`) — that detail lives behind the Implementation source link. **Renders on ta-lib.org only**; the API doc targets link to that page in its place, so the formula may grow notation the four doc renderers could never agree on. |
| `## Notes` | optional | Bullet list of ONLY variations / specification differences from the original indicator (e.g. rounding disabled, a pattern that does not verify the prior trend it classically assumes). **No** implementation mechanics or internal identifiers — those live behind the source link. **No TeX**: javadoc and the C# XML doc render markup, not notation, so a bullet carrying `$…$` or a `\command` is dropped from those two targets (per bullet — the others still ship). Notation goes in `## Formula`. |
| `## Inputs` | yes | One short line per **input name**, matching the call signature in name and order (arity/type come from YAML). A `type: price` bundle is documented as its **components** (`inHigh`, `inLow`, `inClose`) — the bundle name is an `ta_abstract` descriptor, never a parameter. Enforced by `docs_site::validate_inputs`. |
| `## Outputs` | yes | One short line per **output name**; for `CDL*` state the actual sign(s) emitted (+100 / −100 / 0). |
| `## Parameters` | if `optional_inputs` | Meaning per optional-input **name** (range / default / `suggested` come from YAML). |
| `## Implementation` | never | Generated on the website page; see below. `generate` rejects an authored one. |
| `## Aliases` | optional | Abbreviation expansions / alternative names for SEO, comma-separated. Drop any alias that merely repeats the function name — omit the whole section when nothing else qualifies (e.g. AROON, FLOOR). Feeds rustdoc `#[doc(alias)]` / site search. |
| `## See Also` | optional | Related TA-Lib function names (` · ` separated) → rustdoc intra-doc links / site links. |
| `## References` | optional | Books / sites, at the **bottom** of the file. A URL must be written as `[label](url)`, never bare: the prose is rendered verbatim into the rustdoc, where a bare URL trips `rustdoc::bare_urls` and fails the crate's warning-free `cargo doc` gate. `generate` rejects one. |

The two **textual** sections are `## Summary` and `## Notes`; every other section is a brief
formula, a name→meaning list, or a link/name table. There is **no frontmatter**, **no
`## Example`**, and **no `## Interpretation`** section.

## The Implementation section

`## Implementation` is **generated**, never authored: `docs_site.rs` writes it into the
website page as the input `<name>.c` and `<name>.yaml`, then one row per backend in the
backend registry naming the file that backend writes for the function, then a pointer to
the language wrappers. A new backend gets its row on every page without an edit here.

## Rendering targets

| Target | Mechanism | What is emitted |
|--------|-----------|-----------------|
| **ta-lib.org** (VuePress) | `backends/docs_site.rs` → `website/src/functions/<name>.md` (scoped prune) | SEO front matter; the authored prose passed through; a `## Parameters` **table joining** `.md` prose with live YAML numbers (type, default, accepted values) plus one italic legend per enum type; the `## Properties` and `## Implementation` sections; `## See Also` linkified to sibling pages. A cross-function emitter regenerates the grouped `functions/index.md`. Inputs/Outputs remain authored bullets — see the status note. |
| **crates.io / docs.rs** | inline `///` in the generated Rust (`backends/rust_doc.rs`) | Crate `//!` overview + quick-start doctest; per-function summary then the ta-lib.org deep link; `# Arguments` joining `.md` prose with YAML defaults/ranges; `# Errors`; a **generated runnable doctest** per function (synthetic data, defaults, asserts `Success` — run by `cargo test --doc`); `# See also` intra-doc links; `#[doc(alias)]` from Aliases. Plus Cargo.toml `description`/`license`/`homepage`/`keywords`/`categories` and a generated crate README.md. Prose is escaped for rustdoc (`[`, `<` outside code spans; list/quote markers at wrapped-line starts). |
| **Java / .NET** | Javadoc / XML doc in the generated `Core` (`backends/java_doc.rs`, `backends/csharp_doc.rs`) | Summary, the ta-lib.org deep link, notes, the range contract; `@param` / `<param>` joining `.md` prose with YAML defaults/ranges; `@throws` / `<exception>`; `@see` / `<seealso>`. Prose is converted to the target's markup — javadoc renders HTML and the XML doc renders XML, so nothing authored may reach either as literal Markdown. |
| **npm** (phase 4, when a JS/TS backend exists) | TSDoc `/** */` + README | Summary tier for IntelliSense; `package.json` `homepage`/`documentation` deep link. |

## Verification & CI

Everything a `<name>.md` produces is generated deterministically and is subject to the
existing **regeneration oracle** (`build.py generate` then `git status --porcelain` must be
empty). On top of that:

1. **`generate`** rejects the `.md` faults listed in the status note.
2. **`pnpm docs:build`** (VuePress, from `website/`) — broken nav / cross-references fail the docs PR.
3. The Rust crate's warning-free **`cargo doc`** and the C# library build fail on a broken
   intra-doc link or `cref`, so a `## See Also` entry that renders reaches a real method.

> Note: the **formula, notes, and summary prose are not machine-checked against `<name>.c`** —
> nothing automated proves them correct. Human review of formulas, quirks, and citations is
> therefore load-bearing (see below).

## Authoring & review

Documentation is **AI-authored by default**, drafted from the function's `<name>.c` (the
implementation — the source of truth) and `<name>.yaml`, then **adversarially verified against
that same code**. A `/document-indicator` skill (mirroring `/new-ta-func`) can drive it.

- **Review gate:** every `<name>.md` is a `CODEOWNERS`-reviewed PR. Formulas, quirks, and
  citations are exactly where an AI can hallucinate, so maintainer sign-off is
  mandatory — distinct from the byte-oracle.
- **Staleness:** a soft CI gate flags a function whose `<name>.c`/`<name>.yaml` changed without
  a corresponding `<name>.md` update, prompting re-authoring.
- Generated outputs (`website/src/functions/*`, rustdoc, …) are **generator-owned and never hand-edited**.

## Related

- [ta_codegen Input: Metadata (`<name>.yaml`)](ta_codegen_input_yaml.md) — the metadata schema (numbers live here).
- [ta_codegen Input: Code (`<name>.c`)](ta_codegen_input_code.md) — the algorithm / `ta_defs.h` vocabulary.
