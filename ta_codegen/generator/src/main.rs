use ta_codegen_lib::backends;
use ta_codegen_lib::extractor::func_extractor::extract_function_source;
use ta_codegen_lib::extractor::table_parser::{parse_shared_defs, parse_table};
use ta_codegen_lib::extractor::TableFuncDef;
use ta_codegen_lib::formatter;
use ta_codegen_lib::helper_registry::HelperRegistry;
use ta_codegen_lib::ir;
use ta_codegen_lib::parser;
use ta_codegen_lib::registry::Registry;
use ta_codegen_lib::server_gen;

use std::collections::HashMap;
use std::path::{Path, PathBuf};

/// Find the repository root — the directory containing the `ta_codegen/input/`
/// marker — so the binary is callable from any working directory.
///
/// Resolved in order, first marker hit wins:
///   1. `TA_CODEGEN_ROOT` env var (explicit override; errors if it's wrong);
///   2. walking up from the running binary's own location (`current_exe`), which
///      is CWD-independent and survives the repo being moved/renamed (the binary
///      moves with it);
///   3. walking up from the current working directory (fallback for unusual
///      setups, e.g. the binary copied out of the repo tree).
fn repo_root() -> PathBuf {
    const MARKER: &str = "ta_codegen/input";

    // 1. Explicit override wins; if set but wrong, fail loudly rather than guess.
    if let Ok(root) = std::env::var("TA_CODEGEN_ROOT") {
        let root = PathBuf::from(root);
        if root.join(MARKER).is_dir() {
            return root;
        }
        eprintln!(
            "error: TA_CODEGEN_ROOT={} does not contain {MARKER}/",
            root.display()
        );
        std::process::exit(1);
    }

    // 2. From the binary's directory, then 3. the current working directory.
    let exe_dir = std::env::current_exe()
        .ok()
        .and_then(|p| p.parent().map(Path::to_path_buf));
    let candidates = [exe_dir, std::env::current_dir().ok()];
    for start in candidates.iter().flatten() {
        let mut dir: &Path = start;
        loop {
            if dir.join(MARKER).is_dir() {
                return dir.to_path_buf();
            }
            match dir.parent() {
                Some(parent) => dir = parent,
                None => break,
            }
        }
    }

    eprintln!("error: cannot find {MARKER}/ from the executable location or current directory.");
    eprintln!("       Run ta_codegen from within the ta-lib repository, or set TA_CODEGEN_ROOT.");
    std::process::exit(1);
}

fn main() {
    let args: Vec<String> = std::env::args().collect();
    let command = args.get(1).map(|s| s.as_str()).unwrap_or("generate");
    match command {
        "generate" => {
            let func_filter = find_arg(&args, &["--func", "--function"]);
            let backend_filter = find_arg(&args, &["--backend"]);
            generate(func_filter.as_deref(), backend_filter.as_deref());
        }
        "generate-servers" => {
            let func_filter = find_arg(&args, &["--func", "--function"]);
            let backend_filter = find_arg(&args, &["--backend"]);
            generate_servers(func_filter.as_deref(), backend_filter.as_deref());
        }
        "generate-bench" => {
            let backend_filter = find_arg(&args, &["--backend"]);
            generate_bench(backend_filter.as_deref());
        }
        "build" => {
            let backend_filter = find_arg(&args, &["--backend"]);
            build_servers(backend_filter.as_deref());
        }
        "extract" => {
            let func_filter = find_arg(&args, &["--function", "--func"]);
            extract(func_filter.as_deref());
        }
        "format" => {
            let func_filter = find_arg(&args, &["--func", "--function"]);
            let check_only = args.iter().any(|a| a == "--check");
            std::process::exit(format(func_filter.as_deref(), check_only));
        }
        "stream-census" => {
            let seed = args.iter().any(|a| a == "--seed-yaml");
            std::process::exit(stream_census(seed));
        }
        _ => {
            eprintln!("Usage: ta_codegen <command> [options]");
            eprintln!();
            eprintln!("Commands:");
            eprintln!("  generate         Generate code for all backends (default)");
            eprintln!("  generate-servers  Generate JSON-RPC server wrappers for each language");
            eprintln!("  generate-bench   Generate the direct-call C benchmark binary source");
            eprintln!("  build            Compile generated server source into executables");
            eprintln!("  extract          Extract indicators from C source to ta_codegen/input/");
            eprintln!("  format           Re-indent the ta_codegen/input/ C source of truth");
            eprintln!("  stream-census    Report the IR-derived streamability per function");
            eprintln!("                   (--seed-yaml writes `streaming: true` for clean functions)");
            eprintln!();
            eprintln!("Options for 'format':");
            eprintln!("  --func=NAME[,NAME,...]       Only format matching indicators (default: all)");
            eprintln!("  --check                      Report files that need formatting; write nothing");
            eprintln!();
            eprintln!("Options for 'generate' / 'generate-servers' / 'build':");
            eprintln!(
                "  --func=NAME[,NAME,...]      Only generate specified functions (default: all)"
            );
            eprintln!(
                "  --backend=NAME[,NAME,...]    Only generate specified backends (default: all)"
            );
            eprintln!("                               Backends: c, rust, java, dotnet");
            eprintln!();
            eprintln!("Options for 'extract':");
            eprintln!(
                "  --function=NAME[,NAME,...]   Only extract specified functions (default: all)"
            );
            std::process::exit(1);
        }
    }
}

/// Wire parsed C source (guarded + optional private) into a FuncDef.
/// Thin alias for the shared implementation in `parser::c_source` (also used
/// by integration tests so fixtures wire exactly like production loads).
fn wire_parsed_source(func_def: &mut ir::FuncDef, parsed: &parser::c_source::ParsedCSource) {
    parser::c_source::wire_parsed_source(func_def, parsed);
}

/// Look up a `--flag=value` argument, accepting any of the given flag spellings
/// (e.g. both `--func` and `--function`).
fn find_arg(args: &[String], prefixes: &[&str]) -> Option<String> {
    for prefix in prefixes {
        let prefix_eq = format!("{}=", prefix);
        if let Some(a) = args.iter().find(|a| a.starts_with(&prefix_eq)) {
            return Some(a[prefix_eq.len()..].to_string());
        }
    }
    None
}

/// True for non-indicator subdirectories of `ta_codegen/input/` (shared `helpers/`,
/// library scaffolding `lib/`) that never carry a `<name>.yaml` indicator
/// definition and therefore contribute nothing to generated output.
fn is_reserved_dir(name: &str) -> bool {
    matches!(name, "helpers" | "lib")
}

/// Recursively collect `*.c` files under `dir`.
fn collect_c_files(dir: &Path, out: &mut Vec<PathBuf>) {
    let Ok(entries) = std::fs::read_dir(dir) else {
        return;
    };
    for entry in entries.flatten() {
        let path = entry.path();
        if path.is_dir() {
            collect_c_files(&path, out);
        } else if path.extension().is_some_and(|e| e == "c") {
            out.push(path);
        }
    }
}

/// Re-indent the `ta_codegen/input/` C source of truth (see [`formatter`] for
/// the safety invariant). With `check_only`, nothing is written. Returns the
/// number of files scanned and the repo-relative paths that changed (or would
/// change), or an error message on an I/O or safety-check failure.
///
/// This is the shared core behind both the `format` subcommand and the
/// automatic pre-pass in [`generate`].
fn format_inputs(
    root: &Path,
    filters: Option<&[String]>,
    check_only: bool,
) -> Result<(usize, Vec<PathBuf>), String> {
    let base = root.join("ta_codegen/input");
    let mut files = Vec::new();
    collect_c_files(&base, &mut files);
    files.sort();

    let mut scanned = 0usize;
    let mut changed = Vec::new();
    for path in &files {
        // Optional --func filter: substring match on the (lowercased) path.
        if let Some(fs) = filters {
            let hay = path.to_string_lossy().to_lowercase();
            if !fs.iter().any(|f| hay.contains(f.as_str())) {
                continue;
            }
        }
        scanned += 1;

        let original = std::fs::read_to_string(path)
            .map_err(|e| format!("cannot read {}: {e}", path.display()))?;
        let formatted = formatter::reindent_source(&original);

        // Belt-and-suspenders: never write output that changed anything but
        // whitespace / blank-line runs, even if a formatter bug slipped through.
        if !formatter::content_preserved(&original, &formatted) {
            return Err(format!(
                "refusing to write {} — non-whitespace change detected (bug)",
                path.display()
            ));
        }
        if formatted == original {
            continue;
        }
        if !check_only {
            std::fs::write(path, &formatted)
                .map_err(|e| format!("cannot write {}: {e}", path.display()))?;
        }
        changed.push(path.strip_prefix(root).unwrap_or(path).to_path_buf());
    }
    Ok((scanned, changed))
}

/// `format` subcommand: re-indent the input source of truth in place. Returns a
/// process exit code: 0 = success/clean; 1 = (with `--check`) files need
/// formatting, or an I/O / safety-check failure occurred.
fn format(func_filter: Option<&str>, check_only: bool) -> i32 {
    let root = repo_root();
    let filters: Option<Vec<String>> =
        func_filter.map(|f| f.split(',').map(|s| s.trim().to_lowercase()).collect());

    match format_inputs(&root, filters.as_deref(), check_only) {
        Ok((scanned, changed)) => {
            let verb = if check_only { "would reformat" } else { "reformatted" };
            for rel in &changed {
                println!("{verb} {}", rel.display());
            }
            if check_only && !changed.is_empty() {
                eprintln!(
                    "{} of {scanned} input file(s) need formatting (run `ta_codegen format`)",
                    changed.len()
                );
                return 1;
            }
            if check_only {
                println!("all {scanned} input file(s) already formatted");
            } else {
                println!("formatted {} of {scanned} input file(s)", changed.len());
            }
            0
        }
        Err(msg) => {
            eprintln!("error: {msg}");
            1
        }
    }
}

/// `stream-census`: print each function's IR-derived streamability (tier,
/// state size — all derived, never authored) and audit the `streaming: true`
/// declarations. This is the stage-0 tool from docs/streaming-api-proposal.md
/// and the audit trail when a batch rewrite changes a function's shape.
///
/// `--seed-yaml` inserts `streaming: true` (before the `inputs:` key) into
/// the YAML of every function that analyzes clean and has no declaration
/// yet. Exit code 1 when any declared function is no longer streamable.
fn stream_census(seed_yaml: bool) -> i32 {
    let root = repo_root();
    let funcs = load_func_defs(None, &root);
    let lookup = ta_codegen_lib::streaming::FuncsLookup(&funcs);
    let mut derived_t1 = 0usize;
    let mut derived_t2 = 0usize;
    let mut derived_t3 = 0usize;
    let mut derived_t4 = 0usize;
    let mut derived_tc = 0usize;
    let mut mismatches = 0usize;
    let mut seeded = 0usize;

    for func in &funcs {
        // Full validation (analysis + transition build) — the same gate
        // generate() enforces, so census can never seed a function the
        // emitter cannot actually build.
        match ta_codegen_lib::streaming::validate_streamable(func, &lookup) {
            Ok(plan) => {
                let status = if func.streaming { "streamed" } else { "candidate" };
                match &plan {
                    ta_codegen_lib::streaming::StreamPlan::Loop(m) => {
                        match m.tier {
                            ir::StreamTier::T1 => derived_t1 += 1,
                            ir::StreamTier::T2 => derived_t2 += 1,
                            ir::StreamTier::T3 => derived_t3 += 1,
                            ir::StreamTier::T4 => derived_t4 += 1,
                        }
                        println!(
                            "{:<10} {:<14} {} state={} lags={} outs={}",
                            status,
                            func.name,
                            m.tier.as_str(),
                            m.state.len(),
                            m.lags.len(),
                            m.outputs.len()
                        );
                    }
                    ta_codegen_lib::streaming::StreamPlan::Dispatch(dp) => {
                        derived_tc += 1;
                        println!(
                            "{:<10} {:<14} TC dispatch({}) arms={}/{} identity={}",
                            status,
                            func.name,
                            dp.param,
                            dp.arms.iter().filter(|a| a.supported).count(),
                            dp.arms.len(),
                            if dp.identity.is_some() { "yes" } else { "no" }
                        );
                    }
                }
                if seed_yaml && !func.streaming {
                    let yaml_path = root
                        .join("ta_codegen/input")
                        .join(func.name.to_lowercase())
                        .join(format!("{}.yaml", func.name.to_lowercase()));
                    match seed_streaming_flag(&yaml_path) {
                        Ok(()) => seeded += 1,
                        Err(e) => {
                            eprintln!("error: cannot seed {}: {e}", yaml_path.display());
                            return 1;
                        }
                    }
                }
            }
            Err(e) => {
                if func.streaming {
                    mismatches += 1;
                    println!("MISMATCH {:<14} declared streaming but: {e}", func.name);
                } else {
                    println!("none       {:<14} -- {e}", func.name);
                }
            }
        }
    }
    println!(
        "\n{} functions: {derived_t1} derive T1, {derived_t2} derive T2, {derived_t3} derive T3, {derived_t4} derive T4, {derived_tc} derive TC dispatch, {mismatches} declaration mismatch(es){}",
        funcs.len(),
        if seed_yaml { std::format!(", {seeded} YAML(s) seeded") } else { String::new() }
    );
    i32::from(mismatches > 0)
}

/// Append `stream` to the single-line `flags: [...]` list of a function
/// YAML (the flag maps to TA_FUNC_FLG_STREAM like every other entry).
fn seed_streaming_flag(yaml_path: &Path) -> Result<(), String> {
    let content = std::fs::read_to_string(yaml_path).map_err(|e| e.to_string())?;
    let start = content
        .find("\nflags: [")
        .map(|p| p + 1)
        .or_else(|| content.starts_with("flags: [").then_some(0))
        .ok_or_else(|| "no single-line `flags: [...]` found".to_string())?;
    let line_end = content[start..]
        .find(']')
        .map(|p| start + p)
        .ok_or_else(|| "unterminated flags list".to_string())?;
    let inner = &content[start + "flags: [".len()..line_end];
    if inner.split(',').any(|f| f.trim() == "stream") {
        return Ok(()); // already flagged
    }
    let new_inner = if inner.trim().is_empty() {
        "stream".to_string()
    } else {
        format!("{inner}, stream")
    };
    let mut out = String::with_capacity(content.len() + 16);
    out.push_str(&content[..start]);
    out.push_str("flags: [");
    out.push_str(&new_inner);
    out.push_str(&content[line_end..]);
    std::fs::write(yaml_path, out).map_err(|e| e.to_string())
}

fn generate(func_filter: Option<&str>, backend_filter: Option<&str>) {
    let root = repo_root();
    let base = root.join("ta_codegen/input");

    // Keep the source of truth tidy: re-indent every input in place before
    // generating. This only ever changes indentation / whitespace / blank-line
    // runs (never semantics — the parser is whitespace-insensitive, so output is
    // unaffected), and only files that actually changed are rewritten.
    match format_inputs(&root, None, false) {
        Ok((_, changed)) if !changed.is_empty() => {
            println!("Formatted {} input file(s).", changed.len());
        }
        Ok(_) => {}
        Err(msg) => {
            eprintln!("error: input formatting failed: {msg}");
            std::process::exit(1);
        }
    }

    // Load indicator registry for cross-call resolution
    let registry = Registry::from_dir(&base);

    // Load helper registry for expression inlining
    let helper_registry = HelperRegistry::from_dir(&base);

    // Load enum definitions
    let enums_path = base.join("enums.yaml");
    let enums = if enums_path.exists() {
        parser::enums::load_enums(&enums_path)
    } else {
        HashMap::new()
    };

    // enums.yaml is the source of truth for FuncUnstId, but a couple of copies
    // are hand-maintained (not regenerated from it) and can silently drift —
    // notably the Rust crate enum template. Fail loudly here rather than let a
    // rename half-propagate (e.g. the MFI/IMI -> UNUSED reclassification, which
    // otherwise broke the Rust server build against a stale variant name).
    verify_hand_maintained_funcunstid(&enums, &root);

    // Discover all function definition directories
    let mut func_dirs: Vec<_> = std::fs::read_dir(&base)
        .expect("Cannot read ta_codegen/input directory")
        .filter_map(|e| e.ok())
        .filter(|e| e.path().is_dir())
        .collect();
    func_dirs.sort_by_key(|e| e.file_name());

    let filter_names: Option<Vec<String>> =
        func_filter.map(|f| f.split(',').map(|s| s.trim().to_uppercase()).collect());

    let backends_to_run: Vec<&str> = match backend_filter {
        Some(b) => b.split(',').map(|s| s.trim()).collect(),
        None => backends::all_names(),
    };

    // Clean stale per-function output files before generating.
    // This prevents generate-servers from picking up outdated artifacts
    // (e.g., stale Core_*.java files that were generated with old code).
    // Only clean when generating all functions (no filter) to avoid
    // accidentally removing files for functions not being regenerated.
    let out_base = root.join("ta_codegen/output");

    if func_filter.is_none() {
        for backend in &backends_to_run {
            clean_generated_files(&out_base, backend);
        }
    }

    // Phase 1: Load all FuncDefs
    let mut generated_funcs: Vec<ir::FuncDef> = Vec::new();

    for entry in &func_dirs {
        let dir = entry.path();
        let func_name_lower = entry.file_name().to_string_lossy().to_string();

        if let Some(ref names) = filter_names {
            if !names.iter().any(|n| n == &func_name_lower.to_uppercase()) {
                continue;
            }
        }

        if is_reserved_dir(&func_name_lower) {
            continue;
        }

        let yaml_path = dir.join(format!("{}.yaml", func_name_lower));
        let c_path = dir.join(format!("{}.c", func_name_lower));

        if !yaml_path.exists() {
            eprintln!("Skipping {}: missing .yaml file", func_name_lower);
            continue;
        }

        if !c_path.exists() {
            eprintln!("Skipping {}: missing .c file", func_name_lower);
            continue;
        }

        let mut func_def = parser::yaml::parse_yaml(&yaml_path);
        let parsed = parser::c_source::parse_c_source(&c_path);
        wire_parsed_source(&mut func_def, &parsed);

        // Streaming maintenance-coupling gate (docs/streaming-api-proposal.md):
        // a YAML-declared tier must match the IR-derived shape, so a batch
        // rewrite that breaks stream analyzability fails HERE, not at release.
        if func_def.streaming {
            if let Err(e) = ta_codegen_lib::streaming::validate_streamable(&func_def, &registry) {
                eprintln!("error: {e}");
                eprintln!("       (run `ta_codegen stream-census` for the full audit)");
                std::process::exit(1);
            }
        }

        // Canonical documentation (third sibling input file) — feeds the rustdoc
        // backend; the website backend reads the .md itself.
        let doc_path = dir.join(format!("{}.md", func_name_lower));
        func_def.doc = parser::doc_md::parse_doc_md(&doc_path);
        if func_def.doc.is_none() {
            eprintln!("Warning: {func_name_lower}: missing {func_name_lower}.md documentation");
        }

        generated_funcs.push(func_def);
    }

    // Phase 2: Generate output for each backend
    for func_def in &generated_funcs {
        for backend in &backends_to_run {
            generate_backend(func_def, backend, &enums, &registry, &helper_registry, &out_base);
        }
    }

    // For cross-function outputs (func_list, Makefile.am), use all definitions
    // regardless of --func filter. Reuse already-parsed data when unfiltered.
    let all_yaml_defs;
    let all_funcs: &[ir::FuncDef] = if func_filter.is_some() {
        all_yaml_defs = load_all_yaml_defs(&base);
        &all_yaml_defs
    } else {
        &generated_funcs
    };

    // The Rust crate scaffolding (Cargo.toml, lib.rs, README.md, mod.rs) is a
    // cross-function output: always built from ALL definitions so a filtered
    // `--func=X` run cannot rewrite mod.rs down to the filtered subset and
    // break the crate.
    if backends_to_run.contains(&"rust") {
        let types_src = root.join("ta_codegen/generator/templates/rust/types.rs");
        generate_rust_crate_scaffolding(&out_base, all_funcs, &types_src);
    }

    backends::func_list::generate(all_funcs, &root.join("ta_func_list.txt"));
    backends::func_api_xml::generate(all_funcs, &root.join("ta_func_api.xml"));

    // Website pages for ta-lib.org — generated into docs/ (NOT ta_codegen/output),
    // one page per function from its ta_codegen/input/<name>/<name>.md source.
    backends::docs_site::generate(all_funcs, &root);

    // Generate the Rust abstract/introspection registry from the full function set.
    if backends_to_run.contains(&"rust") {
        backends::rust_abstract::generate(all_funcs, &enums, &out_base);
    }

    // Generate Makefile.am and copy C library files when C is one of the backends
    if backends_to_run.contains(&"c") {
        backends::makefile_am::generate(all_funcs, &root.join("src/ta_func/Makefile.am"), &root);
        backends::cmake_lists::generate(all_funcs, &root.join("CMakeLists.txt"), &root);

        let c_lib_src = root.join("ta_codegen/generator/templates/c");
        let c_dir = root.join("ta_codegen/output/c");
        std::fs::create_dir_all(&c_dir).unwrap();
        // Single-entry file list, kept as a loop to match the sibling copy loops below.
        #[allow(clippy::single_element_loop)]
        for filename in &["ta_lib_types.h"] {
            let src = c_lib_src.join(filename);
            if src.exists() {
                let dest = c_dir.join(filename);
                std::fs::copy(&src, &dest).unwrap();
                println!("  Copied {filename} -> {}", dest.display());
            }
        }

        // ta_common (ta_global/ta_retcode/ta_version + headers) and
        // ta_func/ta_utility.{c,h} are hand-written and stay in `src/` (canonical
        // cutover option B). The generated C lib lives in `src/` too, so the
        // servers/unity build directly against `src/...` — no copy into output/c.

        // Generate ta_func_unguarded.h into include/ (public header)
        let unguarded_h = server_gen::generate_c_header_stub(all_funcs);
        let include_path = root.join("include").join("ta_func_unguarded.h");
        std::fs::write(&include_path, &unguarded_h).unwrap();
        println!("  ta_func_unguarded.h -> {}", include_path.display());

        // Generate ta_func_private.h into src/ta_func/ (alongside the generated indicators)
        let private_h = server_gen::generate_c_private_header(all_funcs);
        let private_path = root.join("src/ta_func").join("ta_func_private.h");
        std::fs::write(&private_path, &private_h).unwrap();
        println!("  ta_func_private.h -> {}", private_path.display());

        // Generate ta_abstract layer from YAML definitions
        backends::ta_abstract_c::generate(all_funcs, &enums, &out_base);

        // Take over gen_code's two remaining C-side scalar generators:
        //   - the FuncUnstId enum (GENCODE SECTION 1) in the public header ta_defs.h
        //   - the TA_SetRetCodeInfo table in ta_common/ta_retcode.c (from the csv)
        backends::ta_defs::generate(&enums, &root.join("include/ta_defs.h"));
        backends::retcode::generate(
            &root.join("ta_codegen/generator/templates/c/ta_retcode.c.template"),
            &root.join("src/ta_common/ta_retcode.csv"),
            &root.join("src/ta_common/ta_retcode.c"),
        );
    }

    // Take over gen_code's Java role: generate the shipped Java library files into
    // java/src/com/tictactec/ta/lib/ (the Rust/.NET bindings have no canonical home
    // and stay under ta_codegen/output/, but Java — like C — is a shipped product).
    if backends_to_run.contains(&"java") {
        let java_pkg = root.join("java/src/com/tictactec/ta/lib");
        // FuncUnstId.java depends only on enums.yaml — always safe to regenerate.
        backends::java_enums::generate(&enums, &java_pkg.join("FuncUnstId.java"));
        // Core.java's GENCODE section and CoreAnnotated.java splice ALL indicators
        // into a single file, so only regenerate on a full (unfiltered) run — a
        // --func subset would drop every other indicator's methods.
        if func_filter.is_none() {
            backends::java_shipped::generate_core(
                &generated_funcs,
                &enums,
                &registry,
                &helper_registry,
                &java_pkg.join("Core.java"),
            );
            backends::java_shipped::generate_annotated(
                &generated_funcs,
                &enums,
                &java_pkg.join("CoreAnnotated.java"),
            );
        } else {
            println!(
                "  (skipping shipped Core.java/CoreAnnotated.java — needs a full \
                 generate without --func)"
            );
        }
    }
}

/// Load all YAML function definitions (no C source parsing, no filter).
/// Used for cross-function outputs like `ta_func_list.txt`.
fn load_all_yaml_defs(base: &Path) -> Vec<ir::FuncDef> {
    let mut dirs: Vec<_> = std::fs::read_dir(base)
        .expect("Cannot read ta_codegen/input directory")
        .filter_map(|e| e.ok())
        .filter(|e| e.path().is_dir())
        .collect();
    dirs.sort_by_key(|e| e.file_name());

    let mut funcs = Vec::new();
    for entry in &dirs {
        let dir = entry.path();
        let dir_name = entry.file_name().to_string_lossy().to_string();

        if is_reserved_dir(&dir_name) {
            continue;
        }

        let yaml_path = dir.join(format!("{}.yaml", dir_name));
        if yaml_path.exists() {
            let mut func_def = parser::yaml::parse_yaml(&yaml_path);
            // Wire the parsed .c source too: cross-function artifacts written
            // from this list (ta_func_unguarded.h, ta_func_private.h) need the
            // source-derived fields — has_explicit_private and the private
            // extra params. A YAML-only def silently dropped the TA_*_Private
            // declarations from the shared headers on every --func=X run
            // (caught by clang: implicit declaration of TA_EMA_Private).
            let c_path = dir.join(format!("{}.c", dir_name));
            if c_path.exists() {
                let parsed = parser::c_source::parse_c_source(&c_path);
                wire_parsed_source(&mut func_def, &parsed);
            }
            funcs.push(func_def);
        }
    }

    funcs
}

fn load_func_defs(func_filter: Option<&str>, root: &Path) -> Vec<ir::FuncDef> {
    let base = root.join("ta_codegen/input");

    let mut func_dirs: Vec<_> = std::fs::read_dir(&base)
        .expect("Cannot read ta_codegen/input directory")
        .filter_map(|e| e.ok())
        .filter(|e| e.path().is_dir())
        .collect();
    func_dirs.sort_by_key(|e| e.file_name());

    let filter_names: Option<Vec<String>> =
        func_filter.map(|f| f.split(',').map(|s| s.trim().to_uppercase()).collect());

    let mut funcs = Vec::new();

    for entry in &func_dirs {
        let dir = entry.path();
        let func_name_lower = entry.file_name().to_string_lossy().to_string();

        if let Some(ref names) = filter_names {
            if !names.iter().any(|n| n == &func_name_lower.to_uppercase()) {
                continue;
            }
        }

        if is_reserved_dir(&func_name_lower) {
            continue;
        }

        let yaml_path = dir.join(format!("{}.yaml", func_name_lower));
        let c_path = dir.join(format!("{}.c", func_name_lower));

        if !yaml_path.exists() || !c_path.exists() {
            continue;
        }

        let mut func_def = parser::yaml::parse_yaml(&yaml_path);
        let parsed = parser::c_source::parse_c_source(&c_path);
        wire_parsed_source(&mut func_def, &parsed);

        funcs.push(func_def);
    }

    funcs
}

fn generate_servers(func_filter: Option<&str>, backend_filter: Option<&str>) {
    let root = repo_root();
    let funcs = load_func_defs(func_filter, &root);

    if funcs.is_empty() {
        eprintln!("No function definitions found");
        std::process::exit(1);
    }

    let backends_to_run: Vec<&str> = match backend_filter {
        Some(b) => b.split(',').map(|s| s.trim()).collect(),
        None => backends::all_names(),
    };

    let out_base = root.join("ta_codegen/output");

    // enums.yaml is the source of truth for FuncUnstId; pass it so the Java and
    // Rust servers emit their FuncUnstId enum / id map from it instead of a
    // hand-maintained copy that can silently drift.
    let enums = {
        let enums_path = root.join("ta_codegen/input/enums.yaml");
        if enums_path.exists() {
            parser::enums::load_enums(&enums_path)
        } else {
            HashMap::new()
        }
    };

    // The server FuncUnstId enums are emitted from enums.yaml below; the Rust
    // crate enum is a hand-maintained copy, so guard it here too (this command
    // can run without `generate`, e.g. `build.py servers`).
    verify_hand_maintained_funcunstid(&enums, &root.join("ta_codegen/input"));

    for backend in &backends_to_run {
        match backends::get(backend) {
            Some(b) => b.generate_server(&funcs, &enums, &out_base),
            None => eprintln!("Unknown backend: {}", backend),
        }
    }

    println!(
        "Server source files generated for {} function(s).",
        funcs.len()
    );
}

fn generate_bench(backend_filter: Option<&str>) {
    let root = repo_root();
    let funcs = load_func_defs(None, &root);
    if funcs.is_empty() {
        eprintln!("No function definitions found");
        std::process::exit(1);
    }
    let backends: Vec<&str> = match backend_filter {
        Some(b) => b.split(',').map(|s| s.trim()).collect(),
        None => vec!["c"],
    };
    let out_base = root.join("ta_codegen/output");
    for backend in &backends {
        if *backend == "c" {
            let dir = out_base.join("c");
            std::fs::create_dir_all(&dir).unwrap();
            ta_codegen_lib::bench_gen::write_c_bench(&funcs, &dir);
        } else {
            eprintln!("generate-bench: unsupported backend '{}' (only 'c' is supported)", backend);
        }
    }
}

/// Optimization/link flags shared by every gcc invocation in the build pipeline.
/// Centralized so the C server, C bench, and shared-library builds cannot drift.
const COMMON_GCC_FLAGS: &[&str] = &["-lm", "-O3", "-flto", "-DNDEBUG", "-Wno-parentheses-equality"];

/// Verify the hand-maintained Rust `FuncUnstId` enum matches enums.yaml.
///
/// enums.yaml is the source of truth for `FuncUnstId`; the C enum (`ta_defs.h`)
/// and the shipped Java enum are regenerated from it, but the Rust crate enum
/// lives in the hand-written template `ta_codegen/generator/templates/rust/types.rs`
/// and is copied verbatim. If it drifts, the Rust server references a variant that no
/// longer exists (build failure) and the shipped Rust crate's enum diverges from the C
/// header. Fail loudly at generate time rather than let a rename half-propagate.
fn verify_hand_maintained_funcunstid(
    enums: &HashMap<String, ir::EnumDef>,
    root: &std::path::Path,
) {
    let Some(fu) = enums.get("FuncUnstId") else {
        return;
    };
    // The crate enum is the enums.yaml variants followed by the `FuncUnstAll`
    // wildcard sentinel; keep it in the expected list so a misplaced/duplicated
    // sentinel (which would mis-size `[i32; FuncUnstAll as usize]`) is caught.
    let mut expected: Vec<&str> = fu.variants.iter().map(|v| v.pascal_name.as_str()).collect();
    expected.push("FuncUnstAll");

    let path = root.join("ta_codegen/generator/templates/rust/types.rs");
    let src = match std::fs::read_to_string(&path) {
        Ok(s) => s,
        Err(_) => return, // template absent in this checkout -- nothing to verify
    };

    let marker = "pub enum FuncUnstId {";
    let Some(start) = src.find(marker) else {
        eprintln!("Error: `{marker}` not found in {}", path.display());
        std::process::exit(1);
    };
    // Strip line/doc comments (`//`, `///`) BEFORE locating the closing brace, so
    // a `}` or stray token inside a comment cannot truncate the body or leak a
    // bogus variant.
    let stripped: String = src[start + marker.len()..]
        .lines()
        .map(|l| l.split("//").next().unwrap_or(l))
        .collect::<Vec<_>>()
        .join("\n");
    let Some(end) = stripped.find('}') else {
        eprintln!("Error: unterminated FuncUnstId enum in {}", path.display());
        std::process::exit(1);
    };

    // Variants are comma-separated; take each entry's leading identifier so an
    // explicit `= discriminant` or several variants on one line are handled, and
    // keep `FuncUnstAll` in place (compared positionally against `expected`).
    let found: Vec<&str> = stripped[..end]
        .split(',')
        .map(str::trim)
        .filter(|e| !e.is_empty())
        .map(|e| {
            e.split(|c: char| !(c.is_ascii_alphanumeric() || c == '_'))
                .next()
                .unwrap_or(e)
        })
        .filter(|s| !s.is_empty())
        .collect();

    if found != expected {
        eprintln!(
            "Error: the hand-maintained Rust FuncUnstId enum has drifted from \
             enums.yaml (the source of truth).\n  file:     {}\n  expected: {:?}\n  \
             found:    {:?}\nUpdate that template's `pub enum FuncUnstId` to match \
             enums.yaml -- the C and shipped-Java enums regenerate automatically, but \
             this Rust one does not.",
            path.display(),
            expected,
            found
        );
        std::process::exit(1);
    }
}

fn build_servers(backend_filter: Option<&str>) {
    let root = repo_root();
    let backends_to_build: Vec<&str> = match backend_filter {
        Some(b) => b.split(',').map(|s| s.trim()).collect(),
        None => backends::all_names(),
    };

    let out_base = root.join("ta_codegen/output");
    let bin_dir = root.join("bin");

    // Remove stale shared-lib marker so it rebuilds fresh each invocation.
    let _ = std::fs::remove_file(bin_dir.join(".shared_lib_built"));

    // Track server-build failures so we can exit non-zero. Without this a
    // failed compile would still exit 0, and ta_regtest would silently reuse
    // the previously-built (stale) server binary — a real break reads as green.
    let mut failures: u32 = 0;

    for backend in &backends_to_build {
        match *backend {
            "c" => {
                print!("  Building C server... ");
                let c_dir = out_base.join("c");
                let include_dir = root.join("include");
                let src_dir = root.join("src");
                // Option B: the whole C library (indicators + ta_common + the generated
                // ta_abstract layer) lives in src/; output/c holds only the
                // server/unity wrappers.
                let ta_func_dir = src_dir.join("ta_func");
                let ta_common_dir = src_dir.join("ta_common");
                let ta_abstract_dir = src_dir.join("ta_abstract");
                let ta_frames_dir = ta_abstract_dir.join("frames");
                let ta_abstract_serve_dir = root.join("ta_codegen/generator/templates/c");
                // fuzz_data.h (shared seed-generator/hasher) for stream_verify.
                let ta_regtest_dir = src_dir.join("tools/ta_regtest");
                let src = c_dir.join("ta_codegen_serve.c");
                let dst = bin_dir.join("ta_codegen_serve_c");
                match std::process::Command::new("gcc")
                    .args([
                        "-o",
                        dst.to_str().unwrap(),
                        src.to_str().unwrap(),
                        &format!("-I{}", c_dir.to_str().unwrap()),
                        &format!("-I{}", ta_abstract_dir.to_str().unwrap()),
                        &format!("-I{}", ta_frames_dir.to_str().unwrap()),
                        &format!("-I{}", include_dir.to_str().unwrap()),
                        &format!("-I{}", src_dir.to_str().unwrap()),
                        &format!("-I{}", ta_func_dir.to_str().unwrap()),
                        &format!("-I{}", ta_common_dir.to_str().unwrap()),
                        &format!("-I{}", ta_abstract_serve_dir.to_str().unwrap()),
                        &format!("-I{}", ta_regtest_dir.to_str().unwrap()),
                    ])
                    .args(COMMON_GCC_FLAGS)
                    .status()
                {
                    Ok(s) if s.success() => println!("OK"),
                    Ok(s) => {
                        failures += 1;
                        println!("FAILED (exit {})", s.code().unwrap_or(-1));
                    }
                    Err(e) => {
                        failures += 1;
                        println!("FAILED (gcc not found: {})", e);
                    }
                }
                // Also build direct-call benchmark binary if source exists
                let bench_src = out_base.join("c/ta_bench_cg.c");
                if bench_src.exists() {
                    print!("  Building C bench... ");
                    let bench_dst = bin_dir.join("ta_bench_cg");
                    let bench_inc_c = out_base.join("c");
                    match std::process::Command::new("gcc")
                        .args([
                            "-o",
                            bench_dst.to_str().unwrap(),
                            bench_src.to_str().unwrap(),
                            &format!("-I{}", bench_inc_c.to_str().unwrap()),
                            &format!("-I{}", include_dir.to_str().unwrap()),
                            &format!("-I{}", src_dir.to_str().unwrap()),
                            &format!("-I{}", ta_func_dir.to_str().unwrap()),
                            &format!("-I{}", ta_common_dir.to_str().unwrap()),
                        ])
                        .args(COMMON_GCC_FLAGS)
                        .status()
                    {
                        Ok(s) if s.success() => println!("OK"),
                        Ok(s) => {
                            failures += 1;
                            println!("FAILED (exit {})", s.code().unwrap_or(-1));
                        }
                        Err(e) => {
                            failures += 1;
                            println!("FAILED (gcc not found: {})", e);
                        }
                    }
                }
            }
            "java" => {
                print!("  Building Java server... ");
                let java_dir = out_base.join("java");
                let class_dir = bin_dir.join("ta_codegen_java");
                std::fs::create_dir_all(&class_dir).ok();
                match std::process::Command::new("javac")
                    .args([
                        "-d",
                        class_dir.to_str().unwrap(),
                        java_dir.join("TaCodegenServe.java").to_str().unwrap(),
                    ])
                    .status()
                {
                    Ok(s) if s.success() => println!("OK"),
                    Ok(s) => {
                        failures += 1;
                        println!("FAILED (exit {})", s.code().unwrap_or(-1));
                    }
                    Err(e) => {
                        failures += 1;
                        println!("FAILED (javac not found: {})", e);
                    }
                }
            }
            "dotnet" => {
                // Build shared library from generated C files (needed by .NET P/Invoke)
                if !build_shared_lib(&out_base, &bin_dir) {
                    failures += 1;
                }

                print!("  Building .NET server... ");
                let dotnet_dir = out_base.join("dotnet");
                let dotnet_out = bin_dir.join("ta_codegen_dotnet");
                std::fs::create_dir_all(&dotnet_out).ok();

                // Create a minimal .csproj if not present
                let csproj_path = dotnet_dir.join("TaCodegenServe.csproj");
                if !csproj_path.exists() {
                    let csproj = r#"<Project Sdk="Microsoft.NET.Sdk">
  <PropertyGroup>
    <OutputType>Exe</OutputType>
    <TargetFramework>net10.0</TargetFramework>
    <Nullable>enable</Nullable>
  </PropertyGroup>
</Project>"#;
                    std::fs::write(&csproj_path, csproj).unwrap();
                }

                match std::process::Command::new("dotnet")
                    .args([
                        "publish",
                        "-c",
                        "Release",
                        "-o",
                        dotnet_out.to_str().unwrap(),
                        dotnet_dir.to_str().unwrap(),
                    ])
                    .status()
                {
                    Ok(s) if s.success() => {
                        // Copy shared lib into dotnet output dir for P/Invoke discovery
                        let lib_name = if cfg!(target_os = "macos") {
                            "libta_codegen_funcs.dylib"
                        } else {
                            "libta_codegen_funcs.so"
                        };
                        let lib_src = bin_dir.join(lib_name);
                        let lib_dst = dotnet_out.join(lib_name);
                        if lib_src.exists() {
                            std::fs::copy(&lib_src, &lib_dst).ok();
                        }
                        println!("OK");
                    }
                    Ok(s) => {
                        failures += 1;
                        println!("FAILED (exit {})", s.code().unwrap_or(-1));
                    }
                    Err(e) => {
                        failures += 1;
                        println!("FAILED (dotnet not found: {})", e);
                    }
                }
            }
            "rust" => {
                print!("  Building Rust server... ");
                let rust_dir = out_base.join("rust");
                match std::process::Command::new("cargo")
                    .args(["build", "--release", "--bin", "ta_codegen_serve"])
                    .current_dir(&rust_dir)
                    .status()
                {
                    Ok(s) if s.success() => {
                        let src = rust_dir.join("target/release/ta_codegen_serve");
                        let dst = bin_dir.join("ta_codegen_serve_rust");
                        if let Err(e) = std::fs::copy(&src, &dst) {
                            failures += 1;
                            println!("OK (build), FAILED (copy: {})", e);
                        } else {
                            println!("OK");
                        }
                    }
                    Ok(s) => {
                        failures += 1;
                        println!("FAILED (exit {})", s.code().unwrap_or(-1));
                    }
                    Err(e) => {
                        failures += 1;
                        println!("FAILED (cargo not found: {})", e);
                    }
                }
            }
            _ => {
                eprintln!("  Unknown backend: {}", backend);
            }
        }
    }

    if failures > 0 {
        eprintln!(
            "\nError: {failures} server build step(s) FAILED (see above). Refusing to \
             exit 0 -- otherwise ta_regtest silently reuses stale server binaries and a \
             real break reads as green."
        );
        std::process::exit(1);
    }
}

/// Build a shared library from the generated C files.
/// This is used by both the Python (ctypes) and .NET (P/Invoke) servers.
/// The shared lib exports all TA_* functions and is placed in bin/.
/// Returns `true` on success so the caller can count a failure (the .NET server
/// needs this native lib; a silent failure here used to still exit 0).
fn build_shared_lib(out_base: &Path, bin_dir: &Path) -> bool {
    let marker = bin_dir.join(".shared_lib_built");
    if marker.exists() {
        return true; // Already built this run
    }

    print!("  Building shared library... ");
    // out_base is `<root>/ta_codegen/output`, so the repo root is two levels up.
    let root = out_base.parent().unwrap().parent().unwrap();
    // The generated C library lives in src/ (option B); only the unity wrapper is
    // a codegen artifact under output/c.
    let c_lib_dir = root.join("src/ta_func");
    let lib_name = if cfg!(target_os = "macos") {
        "libta_codegen_funcs.dylib"
    } else {
        "libta_codegen_funcs.so"
    };
    let dst = bin_dir.join(lib_name);

    let shared_flag = if cfg!(target_os = "macos") {
        "-dynamiclib"
    } else {
        "-shared"
    };

    // Generate a unified source file that includes all individual C files.
    // This handles forward declarations (e.g., MA calls SMA/EMA/WMA). Library
    // sources come from src/ via -I; ta_utility.c is the hand-written helper.
    let mut unity_src = String::new();
    unity_src.push_str("/* Unity build for shared library */\n");
    unity_src.push_str("#include \"ta_func_unguarded.h\"\n");
    unity_src.push_str("#include \"ta_func_private.h\"\n");
    unity_src.push_str("#include \"ta_common/ta_global.c\"\n");
    unity_src.push_str("#include \"ta_func/ta_utility.c\"\n");
    unity_src.push_str("#include \"ta_common/ta_version.c\"\n");
    unity_src.push_str("#include \"ta_common/ta_retcode.c\"\n\n");

    let mut c_names: Vec<String> = Vec::new();
    if let Ok(entries) = std::fs::read_dir(&c_lib_dir) {
        let mut sorted: Vec<_> = entries.filter_map(|e| e.ok()).collect();
        sorted.sort_by_key(|e| e.file_name());
        for entry in sorted {
            let name = entry.file_name().to_string_lossy().to_string();
            if name.starts_with("ta_")
                && name.ends_with(".c")
                // ta_utility.c is the hand-written helper, included explicitly above.
                && name != "ta_utility.c"
            {
                c_names.push(name);
            }
        }
    }

    if c_names.is_empty() {
        println!("FAILED (no C source files found)");
        return false;
    }

    // Sort alphabetically, but move MA to end (it calls SMA/EMA/WMA)
    c_names.sort();
    if let Some(pos) = c_names.iter().position(|n| n == "ta_MA.c") {
        let ma = c_names.remove(pos);
        c_names.push(ma);
    }
    for name in &c_names {
        unity_src.push_str(&format!("#include \"{}\"\n", name));
    }

    let unity_path = out_base.join("c").join("ta_codegen_funcs.c");
    std::fs::write(&unity_path, &unity_src).unwrap();

    let include_dir = root.join("include");
    let src_dir = root.join("src");
    let ta_common_dir = src_dir.join("ta_common");

    let args = vec![
        shared_flag.to_string(),
        "-fPIC".to_string(),
        "-o".to_string(),
        dst.to_str().unwrap().to_string(),
        unity_path.to_str().unwrap().to_string(),
        format!("-I{}", include_dir.to_str().unwrap()),
        format!("-I{}", src_dir.to_str().unwrap()),
        format!("-I{}", c_lib_dir.to_str().unwrap()),
        format!("-I{}", ta_common_dir.to_str().unwrap()),
    ];

    match std::process::Command::new("gcc")
        .args(&args)
        .args(COMMON_GCC_FLAGS)
        .status()
    {
        Ok(s) if s.success() => {
            println!("OK");
            std::fs::write(&marker, "").ok();
            true
        }
        Ok(s) => {
            println!("FAILED (exit {})", s.code().unwrap_or(-1));
            false
        }
        Err(e) => {
            println!("FAILED (gcc not found: {})", e);
            false
        }
    }
}

fn extract(func_filter: Option<&str>) {
    let base = repo_root();
    let tables_dir = base.join("src/ta_abstract/tables");
    let def_ui_path = base.join("src/ta_abstract/ta_def_ui.c");
    let func_dir = base.join("src/ta_func");
    let out_dir = base.join("ta_codegen/input");

    // 1. Parse shared definitions
    let def_ui_source = std::fs::read_to_string(&def_ui_path).expect("Cannot read ta_def_ui.c");
    let shared = parse_shared_defs(&def_ui_source);

    // 2. Parse all table files
    let mut all_funcs: Vec<TableFuncDef> = Vec::new();
    let mut table_files: Vec<_> = std::fs::read_dir(&tables_dir)
        .expect("Cannot read tables directory")
        .filter_map(|e| e.ok())
        .filter(|e| {
            let name = e.file_name().to_string_lossy().to_string();
            name.starts_with("table_") && name.ends_with(".c")
        })
        .collect();
    table_files.sort_by_key(|e| e.file_name());

    for entry in &table_files {
        let source = std::fs::read_to_string(entry.path()).unwrap();
        let funcs = parse_table(&source, &shared);
        all_funcs.extend(funcs);
    }

    println!("Found {} indicators in abstract tables", all_funcs.len());

    // 3. Apply function filter
    let filter_names: Option<Vec<String>> =
        func_filter.map(|f| f.split(',').map(|s| s.trim().to_uppercase()).collect());

    let mut succeeded = 0;
    let mut failed = 0;
    let mut skipped = 0;

    for func in &all_funcs {
        if let Some(ref names) = filter_names {
            if !names.iter().any(|n| n == &func.name) {
                continue;
            }
        }

        // Read corresponding source file
        let src_path = func_dir.join(format!("ta_{}.c", func.name));
        if !src_path.exists() {
            eprintln!("  SKIP {}: source file not found", func.name);
            skipped += 1;
            continue;
        }

        let source = match std::fs::read_to_string(&src_path) {
            Ok(s) => s,
            Err(e) => {
                eprintln!("  FAIL {}: cannot read source: {}", func.name, e);
                failed += 1;
                continue;
            }
        };

        // Extract C logic (catch panics and timeouts from parser)
        let name_lower = func.name.to_lowercase();
        let source_clone = source.clone();
        let name_clone = name_lower.clone();
        let (tx, rx) = std::sync::mpsc::channel();
        std::thread::spawn(move || {
            let result = std::panic::catch_unwind(std::panic::AssertUnwindSafe(|| {
                extract_function_source(&source_clone, &name_clone)
            }));
            let _ = tx.send(result);
        });

        let extracted_c = match rx.recv_timeout(std::time::Duration::from_secs(10)) {
            Ok(Ok(c)) => c,
            Ok(Err(_)) => {
                eprintln!("  FAIL {}: extractor panicked", func.name);
                failed += 1;
                continue;
            }
            Err(_) => {
                eprintln!("  FAIL {}: extractor timed out", func.name);
                failed += 1;
                continue;
            }
        };

        if extracted_c.trim().is_empty() {
            eprintln!("  FAIL {}: extractor produced empty output", func.name);
            failed += 1;
            continue;
        }

        // Generate YAML
        let yaml = func_to_yaml(func);

        // Write output
        let indicator_dir = out_dir.join(&name_lower);
        std::fs::create_dir_all(&indicator_dir).unwrap();

        let yaml_path = indicator_dir.join(format!("{}.yaml", name_lower));
        let c_path = indicator_dir.join(format!("{}.c", name_lower));

        std::fs::write(&yaml_path, &yaml).unwrap();
        std::fs::write(&c_path, &extracted_c).unwrap();

        println!("  OK   {}", func.name);
        succeeded += 1;
    }

    println!();
    println!(
        "Extracted {} indicators ({} succeeded, {} failed, {} skipped)",
        succeeded + failed + skipped,
        succeeded,
        failed,
        skipped
    );
}

fn func_to_yaml(func: &TableFuncDef) -> String {
    let mut out = String::new();

    out.push_str(&format!("name: {}\n", func.name));
    out.push_str(&format!("camel_case: {}\n", func.camel_case));
    out.push_str(&format!("group: {}\n", func.group));
    out.push_str(&format!("hint: {}\n", func.hint));

    // flags
    if func.flags.is_empty() {
        out.push_str("flags: []\n");
    } else {
        out.push_str(&format!("flags: [{}]\n", func.flags.join(", ")));
    }

    // inputs
    out.push_str("inputs:\n");
    for input in &func.inputs {
        out.push_str(&format!("  - name: {}\n", input.name));
        out.push_str(&format!("    type: {}\n", input.param_type));
        if input.param_type == "price" && !input.price_flags.is_empty() {
            out.push_str(&format!(
                "    price_components: [{}]\n",
                input.price_flags.join(", ")
            ));
        }
    }

    // optional_inputs
    if !func.optional_inputs.is_empty() {
        out.push_str("optional_inputs:\n");
        for opt in &func.optional_inputs {
            out.push_str(&format!("  - name: {}\n", opt.name));
            out.push_str(&format!("    type: {}\n", opt.param_type));
            if !opt.display_name.is_empty() {
                out.push_str(&format!("    display_name: {}\n", opt.display_name));
            }
            if !opt.hint.is_empty() {
                out.push_str(&format!("    hint: {}\n", opt.hint));
            }
            if let Some((min, max)) = opt.range {
                out.push_str(&format!(
                    "    range: [{}, {}]\n",
                    format_yaml_num(min),
                    format_yaml_num(max)
                ));
            }
            if let Some(default) = opt.default {
                out.push_str(&format!("    default: {}\n", format_yaml_num(default)));
            }
            if let Some((start, end, inc)) = opt.suggested {
                out.push_str(&format!(
                    "    suggested: [{}, {}, {}]\n",
                    format_yaml_num(start),
                    format_yaml_num(end),
                    format_yaml_num(inc)
                ));
            }
            if !opt.flags.is_empty() {
                out.push_str(&format!("    flags: [{}]\n", opt.flags.join(", ")));
            }
        }
    }

    // outputs
    out.push_str("outputs:\n");
    for output in &func.outputs {
        out.push_str(&format!("  - name: {}\n", output.name));
        out.push_str(&format!("    type: {}\n", output.param_type));
        if !output.flags.is_empty() {
            out.push_str(&format!("    flags: [{}]\n", output.flags.join(", ")));
        }
    }

    out
}

/// Format a number for YAML output: integers without decimal, reals with decimal.
fn format_yaml_num(v: f64) -> String {
    if v == f64::MIN {
        return "TA_REAL_MIN".to_string();
    }
    if v == f64::MAX {
        return "TA_REAL_MAX".to_string();
    }
    if v == v.floor() && v.abs() < 1e15 {
        format!("{}", v as i64)
    } else {
        format!("{}", v)
    }
}

fn generate_rust_crate_scaffolding(out_base: &Path, funcs: &[ir::FuncDef], types_src: &Path) {
    // Single source of truth for the crate version: the VERSION file at the
    // repo root (kept in sync across all packaging by scripts/sync.py).
    // Hardcoding it here once made a release bump fail the regen-check gate.
    let version_path = out_base
        .parent()
        .and_then(|p| p.parent())
        .map(|root| root.join("VERSION"))
        .expect("cannot derive repo root from output dir");
    let crate_version = std::fs::read_to_string(&version_path)
        .unwrap_or_else(|e| panic!("cannot read {}: {e}", version_path.display()))
        .trim()
        .to_string();
    let rust_dir = out_base.join("rust");
    let src_dir = rust_dir.join("src");
    let ta_func_dir = src_dir.join("ta_func");
    let bin_dir = src_dir.join("bin");

    std::fs::create_dir_all(&ta_func_dir).unwrap();
    std::fs::create_dir_all(&bin_dir).unwrap();

    // --- Cargo.toml ---
    let cargo_toml_head = format!(
        "[package]\nname = \"ta-lib\"\nversion = \"{crate_version}\"\nedition = \"2021\""
    );
    let cargo_toml_tail = r#"
description = "Technical analysis library: 161 indicators (SMA, EMA, RSI, MACD, Bollinger Bands, ATR, Stochastic, candlestick patterns) — the official Rust port of TA-Lib, verified against the C reference."
license = "BSD-3-Clause"
homepage = "https://ta-lib.org"
repository = "https://github.com/TA-Lib/ta-lib"
documentation = "https://docs.rs/ta-lib"
readme = "README.md"
keywords = ["technical-analysis", "finance", "trading", "indicators", "candlestick"]
categories = ["finance", "mathematics", "algorithms"]

[lib]
name = "ta_lib"
path = "src/lib.rs"

[[bin]]
name = "ta_codegen_serve"
path = "src/bin/ta_codegen_serve.rs"

[dependencies]
serde_json = "1"

[profile.release]
lto = "thin"
codegen-units = 1
"#;
    let cargo_path = rust_dir.join("Cargo.toml");
    std::fs::write(&cargo_path, format!("{cargo_toml_head}{cargo_toml_tail}")).unwrap();
    println!("  Scaffolding -> {}", cargo_path.display());

    // --- .cargo/config.toml ---
    let cargo_config_dir = rust_dir.join(".cargo");
    std::fs::create_dir_all(&cargo_config_dir).unwrap();
    let cargo_config = r#"# Build with the default target CPU (baseline x86-64 / aarch64).
# This is what crates.io users get by default, so it is what the local
# servers and benchmarks must measure — no native-tuning bias vs the
# baseline-built C library and third-party comparison servers.
# Opt into native tuning explicitly: RUSTFLAGS="-C target-cpu=native"
[build]
"#;
    std::fs::write(cargo_config_dir.join("config.toml"), cargo_config).unwrap();

    // --- src/lib.rs ---
    let lib_rs = r#"//! # TA-Lib: Technical Analysis Library
//!
//! 161 technical-analysis indicators — moving averages, momentum oscillators,
//! volatility bands, volume studies, Hilbert Transform cycle analysis, statistics,
//! price transforms, and 61 candlestick-pattern recognizers — as a pure-Rust crate.
//!
//! This is the official Rust port of [TA-Lib](https://ta-lib.org): every function is
//! generated from the same canonical definitions as the C library and verified
//! against the C reference implementation.
//!
//! # Quick start
//!
//! ```
//! use ta_lib::{Core, RetCode};
//!
//! let close = [11.0, 12.0, 13.0, 14.0, 15.0, 16.0, 17.0, 18.0, 19.0, 20.0];
//! let core = Core::new();
//! let mut sma = vec![0.0; close.len()];
//! let (mut out_beg, mut out_nb) = (0, 0);
//!
//! let ret = core.sma(0, close.len() - 1, &close, 3, &mut out_beg, &mut out_nb, &mut sma);
//! assert_eq!(ret, RetCode::Success);
//!
//! // The first 3-period average lands at input index 2 (the lookback):
//! assert_eq!((out_beg, out_nb), (2, 8));
//! assert_eq!(sma[0], 12.0); // (11 + 12 + 13) / 3
//! ```
//!
//! # API shape
//!
//! Every indicator is a method on [`Core`] and follows the same pattern:
//!
//! * Inputs are `&[f64]` slices, computed over the range `startIdx..=endIdx`.
//! * Outputs are written into caller-provided `&mut` slices; `outBegIdx` receives the
//!   input index of the first output value and `outNBElement` the number of values
//!   written. An indicator consumes a number of leading values (its *lookback*)
//!   before producing output — query it with the matching `*_lookback` method
//!   (e.g. [`Core::sma_lookback`]).
//! * Integer parameters accept `i32::MIN` to select their default value.
//! * Every call returns a [`RetCode`]; anything other than [`RetCode::Success`]
//!   means no output was produced.
//!
//! [`Core`] is immutable after construction: its per-instance settings — unstable
//! period, Metastock [`Compatibility`], and candlestick thresholds — are chosen up
//! front with [`Core::builder()`] and then frozen, so a `Core` is `Send + Sync` and
//! can be shared read-only across threads (e.g. via `Arc`) with no locking:
//!
//! ```
//! use ta_lib::{Core, Compatibility, FuncUnstId};
//!
//! let core = Core::builder()
//!     .compatibility(Compatibility::Metastock)
//!     .unstable_period(FuncUnstId::Ema, 10)
//!     .build();
//! ```
//!
//! To change a setting, build a new `Core` (cloning is cheap); [`Core::to_builder()`]
//! seeds a builder from an existing instance.
//!
//! Every indicator also has an `*_unguarded` variant that skips parameter
//! validation for internal cross-indicator calls — prefer the checked methods.
//! The crate is `#![forbid(unsafe_code)]`: misuse of an `*_unguarded` variant
//! panics, it never triggers undefined behavior.
//!
//! The full function reference, grouped by category, is at
//! [ta-lib.org/functions](https://ta-lib.org/functions/).

#![forbid(unsafe_code)]
#![allow(non_snake_case, unused_variables, unused_assignments, unused_mut, unused_parens, arithmetic_overflow)]
// Generated code: Clippy's style/complexity lints are noise on machine output, and
// several "fixes" would change numeric behavior — e.g. `neg_cmp_op_on_partial_ord`
// on C's `!(a < b)` NaN idiom, or De Morgan rewrites under `nonminimal_bool`. The
// crate is verified bit-exact against the C reference, so these are suppressed rather
// than applied. `too_many_arguments` is inherent to the C API arity.
#![allow(clippy::all, clippy::pedantic)]
#![allow(clippy::approx_constant)] // PI (180/3.141592653589793) is copied verbatim from the C source.
pub mod ta_func;
pub mod abstract_api;
pub use ta_func::*;
"#;
    let lib_path = src_dir.join("lib.rs");
    std::fs::write(&lib_path, lib_rs).unwrap();
    println!("  Scaffolding -> {}", lib_path.display());

    // --- README.md (crates.io / GitHub front page for the crate) ---
    let readme = r#"<!-- Generated by ta_codegen — do not edit. -->

# TA-Lib for Rust

[TA-Lib](https://ta-lib.org) — the widely used technical-analysis library — as a
pure-Rust crate: 161 indicators covering moving averages, momentum oscillators
(RSI, MACD, Stochastic), volatility (Bollinger Bands, ATR), volume, Hilbert
Transform cycle analysis, statistics, price transforms, and 61 candlestick
patterns.

Every function is generated from the same canonical definitions as the C library
and verified against the C reference implementation.

## Install

```toml
[dependencies]
ta-lib = "0.6"
```

## Quick start

```rust
use ta_lib::{Core, RetCode};

let close = [11.0, 12.0, 13.0, 14.0, 15.0, 16.0, 17.0, 18.0, 19.0, 20.0];
let core = Core::new();
let mut sma = vec![0.0; close.len()];
let (mut out_beg, mut out_nb) = (0, 0);

let ret = core.sma(0, close.len() - 1, &close, 3, &mut out_beg, &mut out_nb, &mut sma);
assert_eq!(ret, RetCode::Success);
assert_eq!(sma[0], 12.0); // (11 + 12 + 13) / 3, at input index `out_beg` = 2
```

Every indicator is a method on `Core` with the same calling pattern: `&[f64]`
input slices, a `startIdx..=endIdx` range, caller-provided output slices, and a
`RetCode` result. `outBegIdx` reports the input index of the first output value;
`*_lookback` methods return how many leading values an indicator consumes.

## Configuration

`Core` is immutable after construction. The value-affecting settings — unstable
period, Metastock compatibility, and candlestick thresholds — are chosen up front
with a builder and then frozen:

```rust
use ta_lib::{Core, Compatibility, FuncUnstId};

let core = Core::builder()
    .compatibility(Compatibility::Metastock)
    .unstable_period(FuncUnstId::Ema, 10)
    .build();
```

Because a configured `Core` only ever reads its settings, it is `Send + Sync` and
can be shared read-only across threads (e.g. an `Arc<Core>` with concurrent
indicator calls) without locking. To change a setting, build a new `Core`.

## Documentation

- API reference: <https://docs.rs/ta-lib>
- Per-function reference (formulas, notes, sources): <https://ta-lib.org/functions/>
- Project home: <https://ta-lib.org>

## License

BSD-3-Clause — see [LICENSE](https://github.com/TA-Lib/ta-lib/blob/main/LICENSE).
"#;
    let readme_path = rust_dir.join("README.md");
    std::fs::write(&readme_path, readme).unwrap();
    println!("  Scaffolding -> {}", readme_path.display());

    // --- Copy hand-written types.rs from ta_codegen/generator/templates/rust/ ---
    if types_src.exists() {
        let types_dest = ta_func_dir.join("types.rs");
        std::fs::copy(types_src, &types_dest).unwrap();
        println!("  Copied types.rs -> {}", types_dest.display());
    }

    // --- src/ta_func/mod.rs (generated: imports types + declares indicator modules) ---
    let mut mod_rs = String::new();
    mod_rs.push_str(
        r#"//! Generated technical-analysis functions — one private module per indicator,
//! all exposed as methods on [`Core`].

// Types and Core struct are in types.rs (hand-written, not generated).
mod types;
pub use types::*;

// Generated indicator modules:
"#,
    );

    // Add mod declarations for each generated indicator
    let mut func_names: Vec<String> = funcs
        .iter()
        .map(|f| f.name.to_lowercase())
        .collect();
    func_names.sort();

    for name in &func_names {
        mod_rs.push_str(&format!("mod {};\n", name));
    }

    let mod_path = ta_func_dir.join("mod.rs");
    std::fs::write(&mod_path, &mod_rs).unwrap();
    println!("  Scaffolding -> {}", mod_path.display());

    // --- src/bin/ta_codegen_serve.rs (placeholder) ---
    let placeholder_bin = r#"fn main() {
    eprintln!("Server not yet generated — run: ta_codegen generate-servers --backend=rust");
}
"#;
    let bin_path = bin_dir.join("ta_codegen_serve.rs");
    // Only write placeholder if the server binary doesn't already exist
    if !bin_path.exists() {
        std::fs::write(&bin_path, placeholder_bin).unwrap();
        println!("  Scaffolding -> {} (placeholder)", bin_path.display());
    }

    println!("  Rust crate scaffolding generated ({} indicators)", func_names.len());
}

/// Remove per-function generated files for a backend so stale artifacts
/// from a previous run cannot leak into `generate-servers`.
fn clean_generated_files(out_base: &Path, backend: &str) {
    let Some(backend) = backends::get(backend) else {
        return;
    };
    let dir = backend.lib_output_dir(out_base);
    if !dir.exists() {
        return;
    }
    let (prefix, suffix) = backend.clean_glob();
    // Hand-written / scaffolding files (types.rs, mod.rs, ...) are preserved.
    let keep = backend.clean_keep();
    let mut count = 0;
    if let Ok(entries) = std::fs::read_dir(&dir) {
        for entry in entries.flatten() {
            let name = entry.file_name().to_string_lossy().to_string();
            if name.starts_with(prefix) && name.ends_with(suffix) && !keep.contains(&name.as_str())
            {
                std::fs::remove_file(entry.path()).ok();
                count += 1;
            }
        }
    }
    if count > 0 {
        println!(
            "  Cleaned {count} stale {} files from {}",
            backend.name(),
            dir.display()
        );
    }
}

fn generate_backend(
    func_def: &ir::FuncDef,
    backend: &str,
    enums: &HashMap<String, ir::EnumDef>,
    registry: &Registry,
    helpers: &HelperRegistry,
    out_base: &Path,
) {
    let Some(backend) = backends::get(backend) else {
        eprintln!("Unknown backend: {}", backend);
        return;
    };
    let output = backend.generate(func_def, enums, registry, helpers);
    let dir = backend.lib_output_dir(out_base);
    std::fs::create_dir_all(&dir).unwrap();
    let path = dir.join(backend.file_name(func_def));
    std::fs::write(&path, &output).unwrap();
    println!("  {} -> {}", func_def.name, path.display());
}
