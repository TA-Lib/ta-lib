pub mod abstract_rows;
pub mod builtins;
pub mod c;
pub mod c_stream;
pub mod cmake_lists;
pub mod common;
pub mod compat_fold;
pub mod ir_cleanup;
pub mod csharp;
pub mod csharp_doc;
pub mod csharp_enums;
pub mod csharp_metadata;
pub mod csharp_stream;
pub mod doc_meta;
pub mod docs_patch;
pub mod docs_site;
pub mod expr_walk;
pub mod fma;
pub mod func_api_xml;
pub mod func_list;
pub mod java;
pub mod java_abstract;
pub mod java_stream;
pub mod java_doc;
pub mod java_enums;
pub mod java_metadata;
pub mod java_shipped;
pub mod makefile_am;
pub mod price_bundle;
pub mod retcode;
pub mod rust_abstract;
pub mod rust_doc;
pub mod rust_enums;
pub mod rust_lang;
pub mod rust_phantom_io;
pub mod rust_stream;
pub mod stmt_walk;
pub mod stream_frame;
pub mod ta_abstract_c;
pub mod ta_defs;
pub mod variant_frame;

use crate::helper_registry::HelperRegistry;
use crate::ir::{EnumDef, FuncDef};
use crate::naming::NameKind;
use crate::registry::Registry;
use crate::server_gen;
use std::collections::HashMap;
use std::path::Path;

/// A per-indicator code-generation backend for one target language.
///
/// Each backend knows three things the generate/clean pipelines need: how to
/// render a single indicator, where its per-indicator file lives, and how to
/// recognise its own generated files for stale-cleanup. Routing those through a
/// registry of `LanguageBackend` implementations means a new language is
/// registered in exactly one place ([`all`]) instead of being threaded through
/// several `match backend { ... }` sites.
pub trait LanguageBackend {
    /// Short backend identifier used on the CLI (`c`, `rust`, `java`, `csharp`).
    fn name(&self) -> &'static str;

    /// Render the per-indicator source for `func`.
    fn generate(
        &self,
        func: &FuncDef,
        enums: &HashMap<String, EnumDef>,
        registry: &Registry,
        helpers: &HelperRegistry,
    ) -> String;

    /// Output directory for per-indicator files, relative to `ta_codegen/output/`.
    fn out_subdir(&self) -> &'static str;

    /// File name for the per-indicator file of `func` (e.g. `ta_SMA.c`).
    fn file_name(&self, func: &FuncDef) -> String;

    /// `(prefix, suffix)` identifying this backend's generated per-indicator
    /// files, used to remove stale artifacts before regenerating.
    fn clean_glob(&self) -> (&'static str, &'static str);

    /// Hand-written / scaffolding files in [`out_subdir`](Self::out_subdir) that
    /// must be preserved by stale-cleanup.
    fn clean_keep(&self) -> &'static [&'static str] {
        &[]
    }

    /// Whether this backend emits per-indicator library source files. A backend
    /// that returns `false` produces only a JSON-RPC test server (no shipped
    /// library). Every current backend emits library files (C# was the last
    /// server-only holdout, until its managed backend landed); the hook stays
    /// for a future server-only bring-up.
    fn emits_lib_files(&self) -> bool {
        true
    }

    /// Absolute directory the per-indicator library files are written to.
    /// Defaults to `out_base/<out_subdir>`. The C backend overrides this to emit
    /// the shipped C library **directly into `src/ta_func`** (canonical cutover
    /// option B) instead of the `ta_codegen/output/c` staging tree.
    fn lib_output_dir(&self, out_base: &Path) -> std::path::PathBuf {
        out_base.join(self.out_subdir())
    }

    /// Words this backend cannot render as an identifier — its language's
    /// reserved keywords. Consulted by the default [`check_name`](Self::check_name).
    ///
    /// The lists live in each backend's own module (`c::RESERVED_WORDS`, ...) so
    /// adding a language adds its keywords in exactly one place, next to the
    /// renderer that would have to escape them.
    fn reserved_words(&self) -> &'static [&'static str] {
        &[]
    }

    /// How this backend spells an input-tree `name` of `kind` in its output.
    ///
    /// The default is verbatim, and only C overrides it: locals, signature
    /// arguments and function names all pass straight through, while C alone
    /// prefixes `TA_`. That prefix is what decides the answer for a function
    /// name — `loop` would be a fine C function (`TA_LOOP`) and an impossible
    /// Rust one.
    fn rendered_name(&self, name: &str, kind: NameKind) -> String {
        let _ = kind;
        name.to_string()
    }

    /// Reject `name` if this backend cannot render it as an identifier.
    ///
    /// The shared rule is "[`rendered_name`](Self::rendered_name) must not be a
    /// [`reserved_words`](Self::reserved_words) entry". A backend with a rule
    /// that a word list cannot express overrides this wholesale; the returned
    /// string is the reason, phrased to be readable after `name`.
    ///
    /// The comparison is **case-insensitive, and deliberately stricter than the
    /// compilers.** `NEW`, `Double`, `In` and `Out` are legal identifiers in all
    /// four targets, and all four are rejected here anyway.
    ///
    /// That is a house rule about legibility, not a legality constraint, so
    /// "but it compiles" is not a reason to relax it: these names are read in
    /// four languages at once, and one that reads as a keyword in any of them —
    /// `Core.NEW(..)`, a local called `Double` — is worse to read than the
    /// distinct abbreviation the author could have picked instead. An indicator
    /// is free to be called `SMA` or `HT_DCPERIOD`; nothing real needs to be
    /// called `In`.
    ///
    /// The cost is nil in practice: `shipped_input_tree_passes` sweeps every
    /// name, argument and local in `ta_codegen/input/` and none of them trips
    /// this. Contextual keywords stay legal, which is what keeps `VAR` — the
    /// variance function — spelling itself.
    fn check_name(&self, name: &str, kind: NameKind) -> Result<(), String> {
        let rendered = self.rendered_name(name, kind);
        if let Some(word) = self
            .reserved_words()
            .iter()
            .find(|w| w.eq_ignore_ascii_case(&rendered))
        {
            return Err(reserved_word_reason(self.name(), name, &rendered, word));
        }
        Ok(())
    }

    /// Generate and write this backend's JSON-RPC test server source under
    /// `out_base` (`ta_codegen/output/`). `enums` is passed so the server's
    /// `FuncUnstId` enum / id map can be emitted from enums.yaml (the source of
    /// truth) rather than hand-maintained — see `verify_hand_maintained_funcunstid`.
    fn generate_server(
        &self,
        funcs: &[FuncDef],
        enums: &HashMap<String, EnumDef>,
        out_base: &Path,
    );
}

/// The C reference-compatible backend.
pub struct CBackend;
impl LanguageBackend for CBackend {
    fn name(&self) -> &'static str {
        "c"
    }
    fn generate(
        &self,
        func: &FuncDef,
        enums: &HashMap<String, EnumDef>,
        registry: &Registry,
        helpers: &HelperRegistry,
    ) -> String {
        c::generate(func, enums, registry, helpers)
    }
    fn out_subdir(&self) -> &'static str {
        "c/ta_func"
    }
    fn file_name(&self, func: &FuncDef) -> String {
        format!("ta_{}.c", func.name)
    }
    fn clean_glob(&self) -> (&'static str, &'static str) {
        ("ta_", ".c")
    }
    /// `ta_utility.c` is a hand-written helper that lives in `src/ta_func`
    /// alongside the generated indicators — never delete it during cleanup.
    fn clean_keep(&self) -> &'static [&'static str] {
        &["ta_utility.c"]
    }
    fn reserved_words(&self) -> &'static [&'static str] {
        c::RESERVED_WORDS
    }
    /// `TA_SMA`, `TA_MOVING_AVERAGE` — the `TA_` prefix puts every function name
    /// out of the keywords' reach; everything else is verbatim.
    fn rendered_name(&self, name: &str, kind: NameKind) -> String {
        match kind {
            NameKind::Function => format!("TA_{}", name.to_uppercase()),
            _ => name.to_string(),
        }
    }
    /// Shipped C library is generated in place under `src/ta_func` (option B):
    /// `out_base` is `<root>/ta_codegen/output`, so `../../src/ta_func` = `<root>/src/ta_func`.
    fn lib_output_dir(&self, out_base: &Path) -> std::path::PathBuf {
        out_base
            .parent()
            .and_then(Path::parent)
            .expect("out_base is <root>/ta_codegen/output")
            .join("src/ta_func")
    }
    fn generate_server(
        &self,
        funcs: &[FuncDef],
        enums: &HashMap<String, EnumDef>,
        out_base: &Path,
    ) {
        // ta_func_stream_private.h is generated by `generate()`, not here.
        let dir = out_base.join("c/tools");
        std::fs::create_dir_all(&dir).unwrap();
        // C server uses the real FuncUnstId enum from ta_defs.h — no local copy.
        let output = server_gen::generate_c_server(funcs, enums);
        let path = dir.join("ta_codegen_serve.c");
        crate::emit::write_if_changed(&path, &output).unwrap();
        println!("  C server -> {}", path.display());
    }
}

/// The Rust crate backend.
pub struct RustBackend;
impl LanguageBackend for RustBackend {
    fn name(&self) -> &'static str {
        "rust"
    }
    fn generate(
        &self,
        func: &FuncDef,
        enums: &HashMap<String, EnumDef>,
        registry: &Registry,
        helpers: &HelperRegistry,
    ) -> String {
        rust_lang::generate(func, enums, registry, helpers)
    }
    fn out_subdir(&self) -> &'static str {
        "rust/library/src/ta_func"
    }
    fn file_name(&self, func: &FuncDef) -> String {
        format!("{}.rs", func.name.to_lowercase())
    }
    /// `SMA` is the method, but the module and file that carry it are `sma` /
    /// `sma.rs` — a lower-case path is the safe spelling on a case-insensitive
    /// filesystem. So the name has to clear Rust's keywords *lower-cased*: a
    /// function named `LOOP` would emit `mod loop;`, which rustc rejects.
    fn rendered_name(&self, name: &str, kind: NameKind) -> String {
        match kind {
            NameKind::Function => name.to_lowercase(),
            _ => name.to_string(),
        }
    }
    fn clean_glob(&self) -> (&'static str, &'static str) {
        ("", ".rs")
    }
    fn clean_keep(&self) -> &'static [&'static str] {
        // Hand-written modules copied from `templates/rust/` (see
        // `RUST_TEMPLATE_MODULES`) plus the generated `mod.rs`, and the
        // phantom-I/O sweep, which `generate` writes here but which names no
        // indicator (`RUST_GENERATED_TEST_MODULES`).
        &[
            "types.rs",
            "div_zero.rs",
            "scratch_election.rs",
            "stream_finite.rs",
            "stream_out_range.rs",
            "no_phantom_io.rs",
            "mod.rs",
        ]
    }
    fn reserved_words(&self) -> &'static [&'static str] {
        rust_lang::RESERVED_WORDS
    }
    /// Verbatim — Rust spells an indicator exactly as `input/` names it. Only
    /// the module/file path stays lower-cased, and that is not public API.
    fn generate_server(
        &self,
        funcs: &[FuncDef],
        enums: &HashMap<String, EnumDef>,
        out_base: &Path,
    ) {
        let dir = out_base.join("rust/tools/src/bin");
        std::fs::create_dir_all(&dir).unwrap();
        let output = server_gen::generate_rust_server(funcs, enums);
        let path = dir.join("ta_codegen_serve.rs");
        crate::emit::write_if_changed(&path, &output).unwrap();
        println!("  Rust server -> {}", path.display());
    }
}

/// The Java `Core` bindings backend.
pub struct JavaBackend;
impl LanguageBackend for JavaBackend {
    fn name(&self) -> &'static str {
        "java"
    }
    fn generate(
        &self,
        func: &FuncDef,
        enums: &HashMap<String, EnumDef>,
        registry: &Registry,
        helpers: &HelperRegistry,
    ) -> String {
        java::generate(func, enums, registry, helpers)
    }
    /// A sibling of `library/` and `tools/`, not a child of either: nothing here
    /// ships. The fragments are method bodies with no package, class or imports,
    /// so they compile nowhere on their own — `java_shipped` re-renders them into
    /// `Core.java` from the IR and `generate_server` below inlines them from disk.
    /// Under `library/` they read as a second copy of the shipped source.
    fn out_subdir(&self) -> &'static str {
        "java/fragments"
    }
    fn file_name(&self, func: &FuncDef) -> String {
        format!("Core_{}.java", func.name)
    }
    fn clean_glob(&self) -> (&'static str, &'static str) {
        ("Core_", ".java")
    }
    fn reserved_words(&self) -> &'static [&'static str] {
        java::RESERVED_WORDS
    }
    /// Verbatim — Java spells an indicator exactly as `input/` names it.
    fn generate_server(
        &self,
        funcs: &[FuncDef],
        enums: &HashMap<String, EnumDef>,
        out_base: &Path,
    ) {
        // Per-function fragments (Core_<name>.java) are a generator intermediate
        // beside the library, not part of it; the server inlines them from disk.
        let frag_dir = out_base.join("java/fragments");
        let tools_dir = out_base.join("java/tools");
        std::fs::create_dir_all(&tools_dir).unwrap();
        let template = server_gen::generate_java_server(funcs, enums);
        let output = server_gen::inline_java_core_methods(&template, &frag_dir, funcs);
        let path = tools_dir.join("TaCodegenServe.java");
        crate::emit::write_if_changed(&path, &output).unwrap();
        println!("  Java server -> {}", path.display());
    }
}

/// The managed C# backend: one `Core_<NAME>.cs` per indicator (`public partial
/// class Core`) into the shipped library tree, plus a managed JSON-RPC server
/// whose csproj compiles those exact library sources — the same-text identity
/// proof Java gets by splicing, without the splice.
pub struct CSharpBackend;
impl LanguageBackend for CSharpBackend {
    fn name(&self) -> &'static str {
        "csharp"
    }
    fn generate(
        &self,
        func: &FuncDef,
        enums: &HashMap<String, EnumDef>,
        registry: &Registry,
        helpers: &HelperRegistry,
    ) -> String {
        csharp::generate(func, enums, registry, helpers)
    }
    /// Generated sources only (`Core_*.cs` + the enums); the hand-written
    /// scaffolding (`Core.cs`, `RetCode.cs`, `OutRange.cs`, ..., the csproj)
    /// lives one level up in `csharp/library/`, outside the clean sweep.
    fn out_subdir(&self) -> &'static str {
        "csharp/library/src"
    }
    fn file_name(&self, func: &FuncDef) -> String {
        format!("Core_{}.cs", func.name)
    }
    fn clean_glob(&self) -> (&'static str, &'static str) {
        ("Core_", ".cs")
    }
    fn reserved_words(&self) -> &'static [&'static str] {
        csharp::RESERVED_WORDS
    }
    /// Verbatim — C# spells an indicator exactly as `input/` names it.
    fn generate_server(
        &self,
        funcs: &[FuncDef],
        enums: &HashMap<String, EnumDef>,
        out_base: &Path,
    ) {
        let output = server_gen::generate_csharp_server(funcs, enums);
        let dir = out_base.join("csharp/tools");
        std::fs::create_dir_all(&dir).unwrap();
        let path = dir.join("TaCodegenServe.cs");
        crate::emit::write_if_changed(&path, &output).unwrap();
        // The csproj is generated too: it compiles the shipped library sources
        // into the server assembly (the same-text identity proof).
        let csproj = dir.join("TaCodegenServe.csproj");
        crate::emit::write_if_changed(&csproj, server_gen::csharp_server_csproj()).unwrap();
        println!("  C# server -> {}", path.display());
    }
}

/// The reason string a backend's default [`LanguageBackend::check_name`]
/// returns, spelling out the mangling when the rendered form differs from the
/// authored one (`LOOP` is not itself a keyword; `loop` is), and the case fold
/// when it does not (`NEW` appears in no keyword list; `new` does).
pub(crate) fn reserved_word_reason(
    backend: &str,
    name: &str,
    rendered: &str,
    word: &str,
) -> String {
    if rendered != name {
        format!(
            "it renders as `{rendered}` in the {backend} backend, which is the reserved \
             word `{word}` there"
        )
    } else if rendered == word {
        format!("`{name}` is a reserved word in the {backend} backend")
    } else {
        // Case-only match. Say so plainly — `NEW` is in no keyword list, so
        // "`NEW` is a reserved word" reads as a contradiction and invites the
        // reader to go looking for a bug that is not there.
        format!(
            "`{name}` differs only by case from `{word}`, a reserved word in the \
             {backend} backend; pick a distinct abbreviation"
        )
    }
}

/// All per-indicator backends, in canonical order. Registering a new language
/// is a one-line addition here.
pub fn all() -> Vec<Box<dyn LanguageBackend>> {
    vec![
        Box::new(CBackend),
        Box::new(RustBackend),
        Box::new(JavaBackend),
        Box::new(CSharpBackend),
    ]
}

/// The canonical backend names (`["c", "rust", "java", "csharp"]`).
pub fn all_names() -> Vec<&'static str> {
    all().iter().map(|b| b.name()).collect()
}

/// Look up a backend by its CLI name.
pub fn get(name: &str) -> Option<Box<dyn LanguageBackend>> {
    all().into_iter().find(|b| b.name() == name)
}

/// Write `content` to `path` only if it differs from the current file contents.
/// Prints a one-line status message using `label` and `count`.
pub fn write_if_changed(path: &std::path::Path, content: &str, label: &str, count: usize) {
    let existing = std::fs::read_to_string(path).unwrap_or_default();
    if existing == content {
        println!("  {label} is up to date ({count} functions)");
    } else {
        crate::emit::write_if_changed(path, content).unwrap();
        println!("  {label} updated ({count} functions)");
    }
}

/// Like [`write_if_changed`] but without the status message — for high-volume
/// callers (e.g. the per-file `ta_abstract` writers) that report progress themselves.
pub fn write_if_changed_silent(path: &std::path::Path, content: &str) {
    let existing = std::fs::read_to_string(path).unwrap_or_default();
    if existing != content {
        crate::emit::write_if_changed(path, content).unwrap();
    }
}

/// The sorted `ta_<STEM>.c` source stems for the generated C library, shared by the
/// CMake (`LIB_SOURCES`) and autotools source-list generators so the two lists cannot
/// drift apart. Every function in `ta_codegen/input/`, upper-cased and sorted.
///
/// Returns `(sorted_stems, extras)`; `extras` is always empty (kept for the callers'
/// signature) — every shipped `.c` comes from `ta_codegen/input/`, nothing is picked up
/// by scanning `src/ta_func/`.
pub fn sorted_source_stems(funcs: &[FuncDef], _root: &Path) -> (Vec<String>, Vec<String>) {
    let mut names: Vec<String> = funcs.iter().map(|f| f.name.to_uppercase()).collect();
    names.sort();
    (names, Vec::new())
}
