---
title: "ta_codegen Input: Documentation (<name>.md) Reference"
---

# ta_codegen Input: Documentation (`<name>.md`) Reference

> **Status (2026-07-21).** All 166 `<name>.md` source files exist, and `ta_codegen`
> generates two render targets from them: the **ta-lib.org website**
> (`backends/docs_site.rs` → `website/src/functions/`, served at `/functions/<name>`) and the
> **embedded rustdoc** in the generated Rust crate (`backends/rust_doc.rs`, including a
> runnable doctest per function, verified by `cargo test --doc`). Both inject the YAML
> numbers for **optional inputs** from the shared `backends/doc_meta.rs` (issue #132), the
> website as a `## Parameters` table. `## Inputs` is now **gated** instead: it must name the
> arrays the function is actually called with, in signature order (issue #135) — the price
> bundle (`inPriceHLC`) is an `ta_abstract` descriptor, not an argument, so it never appears
> in prose. Still **planned**: injecting YAML facts into `## Inputs` / `## Outputs`, the
> npm/TSDoc render, Javadoc/.NET XML-doc, and the `docs-lint` gate (the "Rendering targets" /
> "Verification" sections below describe those intended targets). See the sibling references for the two other
> input file kinds: [metadata](ta_codegen_input_yaml.md) and [code](ta_codegen_input_code.md).

`<name>.md` is the **third sibling** in each `ta_codegen/input/<name>/` directory,
alongside `<name>.yaml` (metadata) and `<name>.c` (logic). It is the **single canonical
source** for a function's human documentation. The generator fans it out — deterministically,
like every other backend — into each ecosystem's native format:

- the **ta-lib.org** per-function page (mkdocs-material),
- **embedded rustdoc** in the generated Rust crate (docs.rs / IDE hover / offline `cargo doc`),
- later, **Javadoc / .NET XML-doc / TSDoc** and the `ta_func_api.xml` description.

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

A `docs-lint` gate fails the build if any `.md` prose hard-codes a number that contradicts
the YAML, names a parameter/output that does not exist in the YAML, or points a cross-reference
at a function that does not exist. A consequence of the split: **most metadata changes
(a widened range, a new default) need no doc edit at all** — the render just picks up the
new number.

Half of that gate exists today: `docs_site.rs` hard-errors when a function's `## Parameters`
bullets do not match its YAML `optional_inputs` in name and order, so a parameter renamed on
one side only cannot ship. The number-contradiction half is still unwritten, and it cannot be
a blanket "no digits in a structured section" rule — roughly 80 bullets carry legitimate
non-YAML numbers (the `CDL*` ±100 sign convention, output domains like WILLR's −100…0,
MACDFIX's hard-coded 12/26 constants that live in `macdfix.c`). It has to compare a number
against the specific YAML field it names.

## File format

A `<name>.md` file has **no frontmatter**. It opens with an `#` H1 = the TA-Lib function
name, then a fixed set of `##` sections (order enforced by `docs-lint`).

### Sections (in this order)

Numbers/ranges/defaults are **injected from YAML** at render — never restate them here.

| Section | Required | Content |
|---------|----------|---------|
| `# <NAME>` | yes | H1 = the TA-Lib function name (e.g. `# RSI`). |
| `## Summary` | yes | One prose block: what the function is + a brief intro **and** how to read its output. Interpretation is merged in — there is **no** separate `Interpretation` section. |
| `## Formula` | optional | A **brief, high-level** formula, only when the computation is expressible concisely. Omit for long detection lists (`CDL*`) or long DSP computations (`HT_*`) — that detail lives behind the Implementation source link. |
| `## Notes` | optional | Bullet list of ONLY variations / specification differences from the original indicator (e.g. rounding disabled, a compatibility-mode behavior, a pattern that does not verify the prior trend it classically assumes). **No** implementation mechanics or internal identifiers — those live behind the source link. |
| `## Inputs` | yes | One short line per **input name**, matching the call signature in name and order (arity/type come from YAML). A `type: price` bundle is documented as its **components** (`inHigh`, `inLow`, `inClose`) — the bundle name is an `ta_abstract` descriptor, never a parameter. Enforced by `docs_site::validate_inputs`. |
| `## Outputs` | yes | One short line per **output name**; for `CDL*` state the actual sign(s) emitted (+100 / −100 / 0). |
| `## Parameters` | if `optional_inputs` | Meaning per optional-input **name** (range / default / `suggested` come from YAML). |
| `## Implementation` | yes | A **TA-Lib Definition:** line (input `<name>.c` · `.yaml`), then a **Native** table of the generated **C / Rust / Java** files, then a pointer to language wrappers. |
| `## Aliases` | optional | Abbreviation expansions / alternative names for SEO, comma-separated. Drop any alias that merely repeats the function name — omit the whole section when nothing else qualifies (e.g. AROON, FLOOR). Feeds rustdoc `#[doc(alias)]` / site search. |
| `## See Also` | optional | Related TA-Lib function names (` · ` separated) → rustdoc intra-doc links / site links. |
| `## References` | optional | Books / sites, at the **bottom** of the file. |

The two **textual** sections are `## Summary` and `## Notes`; every other section is a brief
formula, a name→meaning list, or a link/name table. There is **no frontmatter**, **no
`## Example`**, and **no `## Interpretation`** section.

## The Implementation section

`## Implementation` is emitted by the renderer from the function's paths: a **TA-Lib
Definition:** line naming the source of truth, then a **Native** table of the generated
backends, then a pointer to the language wrappers.

```
TA-Lib Definition: <name>.c · <name>.yaml

| Native | File |
|--------|------|
| C      | src/ta_func/ta_<NAME>.c                      |
| Rust   | ta_codegen/output/rust/src/ta_func/<name>.rs |
| Java   | java/src/io/github/talib/Core.java      |

TA-Lib is also available for Python, R and more using a wrapper → ta-lib.org/wrappers/
```

Links resolve against the canonical repo (`github.com/TA-Lib/ta-lib`); the renderer rewrites
them per target as needed.

## Rendering targets

| Target | Mechanism | What is emitted |
|--------|-----------|-----------------|
| **ta-lib.org** (VuePress) | `backends/docs_site.rs` → `website/src/functions/<name>.md` (scoped prune) | SEO front matter; the authored prose passed through; a `## Parameters` **table joining** `.md` prose with live YAML numbers (type, default, accepted values) plus one italic legend per enum type; `## See Also` linkified to sibling pages. A cross-function emitter regenerates the grouped `functions/index.md`. Inputs/Outputs remain authored bullets — see the status note. |
| **crates.io / docs.rs** | inline `///` in the generated Rust (`backends/rust_doc.rs`) | Crate `//!` overview + quick-start doctest; per-function summary, ```` ```text ```` formula, notes; `# Arguments` joining `.md` prose with YAML defaults/ranges; `# Errors`/`# Panics`; a **generated runnable doctest** per function (synthetic data, defaults, asserts `Success` — run by `cargo test --doc`); `# See also` intra-doc links; `#[doc(alias)]` from Aliases; trailing ta-lib.org deep link. Plus Cargo.toml `description`/`license`/`homepage`/`keywords`/`categories` and a generated crate README.md. Prose is escaped for rustdoc (`[`, `<` outside code spans; list/quote markers at wrapped-line starts). |
| **Java / .NET / XML** (phase 3) | Javadoc / XML-doc / `<description>` | Summary + arguments prose. |
| **npm** (phase 4, when a JS/TS backend exists) | TSDoc `/** */` + README | Summary tier for IntelliSense; `package.json` `homepage`/`documentation` deep link. |

## Verification & CI

Everything a `<name>.md` produces is generated deterministically and is subject to the
existing **regeneration oracle** (`build.py generate` then `git status --porcelain` must be
empty). On top of that:

1. **`docs-lint`** — required sections present and ordered; every param/output named in prose
   exists in the YAML; no prose number contradicts the YAML; `see_also` and intra-doc links
   resolve; the Implementation paths exist.
2. **`mkdocs build --strict`** — broken nav / cross-references fail the docs PR.

`ta_codegen format` gains a deterministic `.md` canonicaliser (fixed section order,
trailing-whitespace strip, blank-line collapse) with the same whitespace-only safety guard
as the C reindenter, so the file is stable under the format gate.

> Note: the **formula, notes, and summary prose are not machine-checked against `<name>.c`** —
> nothing automated proves them correct. Human review of formulas, quirks, and citations is
> therefore load-bearing (see below).

## Authoring & review

Documentation is **AI-authored by default**, drafted from the function's `<name>.c` (the
implementation — the source of truth) and `<name>.yaml`, then **adversarially verified against
that same code**. A `/document-indicator` skill (mirroring `/convert-indicator`) can drive it.

- **Review gate:** every `<name>.md` is a `CODEOWNERS`-reviewed PR. Formulas, quirks, and
  citations are exactly where an AI can hallucinate, so maintainer sign-off is
  mandatory — distinct from the byte-oracle.
- **Staleness:** a soft CI gate flags a function whose `<name>.c`/`<name>.yaml` changed without
  a corresponding `<name>.md` update, prompting re-authoring.
- Generated outputs (`website/src/functions/*`, rustdoc, …) are **generator-owned and never hand-edited**.

## Related

- [ta_codegen Input: Metadata (`<name>.yaml`)](ta_codegen_input_yaml.md) — the metadata schema (numbers live here).
- [ta_codegen Input: Code (`<name>.c`)](ta_codegen_input_code.md) — the algorithm / `ta_defs.h` vocabulary.
