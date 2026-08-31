use ta_codegen_lib::backends;
use ta_codegen_lib::emit::{copy_if_changed, write_if_changed};
use ta_codegen_lib::formatter;
use ta_codegen_lib::helper_registry::HelperRegistry;
use ta_codegen_lib::internal_error_ids;
use ta_codegen_lib::ir;
use ta_codegen_lib::naming;
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
            let servers_only = args.iter().any(|a| a == "--servers-only");
            build_servers(backend_filter.as_deref(), servers_only);
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
            eprintln!("  generate         Everything, all backends (default): libraries,");
            eprintln!("                   JSON-RPC servers, benches. No compiler/JDK/SDK needed");
            eprintln!("  generate-servers  Only the JSON-RPC servers (narrowing, for `build`)");
            eprintln!("  generate-bench   Only the direct-call C benchmark binary source");
            eprintln!("  build            Compile generated server source into executables");
            eprintln!("                   --servers-only skips the C benchmark binaries");
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
            eprintln!("                               Backends: c, rust, java, csharp");
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
            write_if_changed(path, &formatted)
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
        // Reported for C. A `PRAGMA TA_ALT={STREAM,<lang>}` claim can give one
        // backend a different plan; the generate-time gate is what holds every
        // language to being streamable, so the census stays one line per function.
        let func = &func.resolved_for(ir::Lang::C);
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
                    ta_codegen_lib::streaming::StreamPlan::Composed(cmp) => {
                        derived_tc += 1;
                        let producer = cmp
                            .producer
                            .as_ref()
                            .map_or("loopless".to_string(), |m| {
                                format!("loop/{}", m.tier.as_str())
                            });
                        let series = cmp.series.as_deref().unwrap_or("-");
                        println!(
                            "{:<10} {:<14} TC composed({series}) producer={producer} subs={} inter={}",
                            status,
                            func.name,
                            cmp.subs.len(),
                            cmp.intermediates.len()
                        );
                    }
                    ta_codegen_lib::streaming::StreamPlan::DualMode(dm) => {
                        derived_tc += 1;
                        println!(
                            "{:<10} {:<14} TC dualmode modeA={}/state={} modeB={}/state={}",
                            status,
                            func.name,
                            dm.mode_a.tier.as_str(),
                            dm.mode_a.state.len(),
                            dm.mode_b.tier.as_str(),
                            dm.mode_b.state.len()
                        );
                    }
                    ta_codegen_lib::streaming::StreamPlan::PeriodBank(pb) => {
                        derived_tc += 1;
                        println!(
                            "{:<10} {:<14} TC period-bank callee={} min={} max={}",
                            status, func.name, pb.callee, pb.min_param, pb.max_param
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
    write_if_changed(yaml_path, out).map_err(|e| e.to_string())
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

    // Everything both up-front gates need, and the same list `all_funcs` uses on
    // the `--func` path below: every definition, fully wired from its .c.
    let all_defs = load_all_yaml_defs(&base);

    // Naming gate — before anything else, because a name no backend can render
    // is a defect in the source of truth rather than in any one output. Checked
    // against every backend at once, so a C-only build cannot pass a name that
    // Rust, Java or C# would choke on (`base` is a C# keyword and nothing
    // else's). See `naming.rs`.
    if let Err(errors) = naming::validate_all(&all_defs, &helper_registry) {
        for e in &errors {
            eprintln!("error: {e}");
        }
        eprintln!("       (a name must be a legal identifier in every backend: see ta_codegen/generator/src/naming.rs)");
        std::process::exit(1);
    }

    // Documentation gate — before the stale-file clean and before any backend
    // writes. The website renders every function's `## Parameters` from its .md
    // joined to the YAML, so a section that disagrees with the YAML is caught
    // while the tree is still untouched; failing later would abort with
    // `src/ta_func/*.c` already deleted by the clean below. Validates every function
    // regardless of `--func`, because the website is regenerated in full either way.
    // Same fail-fast shape as the streaming gate in Phase 1.
    if let Err(errors) = backends::docs_site::validate_docs(&all_defs, &root) {
        for e in &errors {
            eprintln!("error: {e}");
        }
        eprintln!("       (documentation input: see docs/ta_codegen_input_doc.md)");
        std::process::exit(1);
    }

    let out_base = root.join("ta_codegen/output");

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

        if parser::yaml::is_reserved_dir(&func_name_lower) {
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
        // Run it once per language: a `PRAGMA TA_ALT={STREAM,<lang>}` claim can
        // hand one backend a different body, so "streamable" is a per-language
        // property even though today every function resolves the same way for
        // all four.
        if func_def.streaming {
            for lang in ir::ALL_LANGS {
                let resolved = func_def.resolved_for(lang);
                if let Err(e) = ta_codegen_lib::streaming::validate_streamable(&resolved, &registry)
                {
                    eprintln!("error: [{}] {e}", lang.as_str());
                    eprintln!("       (run `ta_codegen stream-census` for the full audit)");
                    std::process::exit(1);
                }
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

    // For cross-function outputs (func_list, Makefile.am), use all definitions
    // regardless of --func filter. Reuse already-parsed data when unfiltered.
    let all_funcs: &[ir::FuncDef] = if func_filter.is_some() {
        &all_defs
    } else {
        &generated_funcs
    };

    // Phase 2: Generate output for each backend
    //
    // The C backend hands every internal-error guard it emits a site id from
    // this ledger, so it has to be in memory before the first C file and back
    // on disk after the last. See `internal_error_ids`.
    let site_ids_path = root.join("ta_codegen/input/internal_error_ids.yaml");
    internal_error_ids::load(&site_ids_path);
    for func_def in &generated_funcs {
        for backend in &backends_to_run {
            generate_backend(func_def, backend, &enums, &registry, &helper_registry, &out_base);
        }
    }
    // Prune only when this run actually re-emitted every C file: any other run
    // leaves most keys unasked-for, and dropping those would free ids that are
    // still live in the tree.
    internal_error_ids::save(
        &site_ids_path,
        func_filter.is_none() && backends_to_run.contains(&"c"),
    );

    // Drop per-function files for indicators that no longer exist. Only when
    // generating all functions (no filter), so a --func run cannot remove files
    // for functions it is not regenerating.
    //
    // Ordering is load-bearing on three sides; do not move this call. It sits:
    //
    // BELOW Phase 1, which only parses and validates, so every gate that can
    // `process::exit` (naming, docs, the per-function streaming gate) runs while
    // the tree is still intact. Above it, a gate that fires aborts with every
    // per-function file already deleted, `src/ta_func/*.c` among them, leaving
    // the shipped library gutted on a merely rejected input.
    //
    // ABOVE the server generation, which splices per-function fragments off
    // disk: a fragment for a dropped indicator has to be gone before anything
    // reads that directory.
    //
    // BELOW Phase 2, which is what lets an unchanged file keep its mtime -- see
    // `emit::write_if_changed`. Deleting first and rewriting tells cargo, CMake
    // and the gcc server build that every indicator changed on every run.
    if func_filter.is_none() {
        for backend in &backends_to_run {
            remove_stale_generated_files(&out_base, backend, &generated_funcs);
        }
    }

    // The Rust crate scaffolding (Cargo.toml, lib.rs, README.md, mod.rs) is a
    // cross-function output: always built from ALL definitions so a filtered
    // `--func=X` run cannot rewrite mod.rs down to the filtered subset and
    // break the crate.
    if backends_to_run.contains(&"rust") {
        let templates = root.join("ta_codegen/generator/templates/rust");
        generate_rust_crate_scaffolding(&out_base, all_funcs, &templates, &enums);
    }

    backends::func_list::generate(all_funcs, &root.join("ta_func_list.txt"));
    backends::func_api_xml::generate(all_funcs, &root.join("ta_func_api.xml"));

    // Website pages for ta-lib.org — generated into docs/ (NOT ta_codegen/output),
    // one page per function from its ta_codegen/input/<name>/<name>.md source.
    backends::docs_site::generate(all_funcs, &enums, &root);

    // Generated regions inside otherwise hand-written website pages (e.g. the
    // unstable-period function list, kept in step with the FuncUnstId enum).
    backends::docs_patch::generate(&enums, &root);

    // Generate the Rust abstract/introspection registry from the full function set.
    if backends_to_run.contains(&"rust") {
        backends::rust_abstract::generate(all_funcs, &enums, &out_base);
    }

    // The Rust phantom-I/O probe (issue #235). Whole-corpus like the registry
    // above: a filtered `--func=X` run must not shrink the sweep to one
    // function, which is the one way a negative-space gate fails open.
    if backends_to_run.contains(&"rust") {
        backends::rust_phantom_io::generate(all_funcs, &enums, &out_base);
    }

    // Generate Makefile.am and copy C library files when C is one of the backends
    if backends_to_run.contains(&"c") {
        backends::makefile_am::generate(all_funcs, &root.join("src/ta_func/Makefile.am"), &root);
        backends::cmake_lists::generate(all_funcs, &root.join("CMakeLists.txt"), &root);

        let c_lib_src = root.join("ta_codegen/generator/templates/c");
        let c_dir = root.join("ta_codegen/output/c/tools");
        std::fs::create_dir_all(&c_dir).unwrap();
        // Single-entry file list, kept as a loop to match the sibling copy loops below.
        #[allow(clippy::single_element_loop)]
        for filename in &["ta_lib_types.h"] {
            let src = c_lib_src.join(filename);
            if src.exists() {
                let dest = c_dir.join(filename);
                copy_if_changed(&src, &dest).unwrap();
                println!("  Copied {filename} -> {}", dest.display());
            }
        }

        // ta_common (ta_global/ta_retcode/ta_version + headers) and
        // ta_func/ta_utility.{c,h} are hand-written and stay in `src/` (canonical
        // cutover option B). The generated C lib lives in `src/` too, so the
        // servers/unity build directly against `src/...` — no copy into output/c.

        // Generate the private stream header into src/ta_func/ (alongside the
        // generated indicators). NOT installed. Remove the two headers it
        // superseded so a stale copy cannot satisfy an include.
        let stream_h = server_gen::generate_c_stream_private_header(all_funcs);
        let stream_path = root.join("src/ta_func").join("ta_func_stream_private.h");
        write_if_changed(&stream_path, &stream_h).unwrap();
        println!("  ta_func_stream_private.h -> {}", stream_path.display());
        for stale in [
            root.join("include").join("ta_func_unguarded.h"),
            root.join("src/ta_func").join("ta_func_private.h"),
        ] {
            if stale.exists() {
                std::fs::remove_file(&stale).unwrap();
                println!("  removed superseded header {}", stale.display());
            }
        }

        // Generate ta_abstract layer from YAML definitions
        backends::ta_abstract_c::generate(all_funcs, &enums, &out_base);

        // Generate the four-variant dispatcher ta_regtest's variant-parity gate
        // drives (issue #137). A header, so neither source list needs an entry.
        backends::variant_frame::generate(all_funcs, &enums, &root);

        // Generate the in-process streaming dispatcher (issue #256's L2/L3):
        // TA_<N>_Open / _OpenAndFill / _Close, native C types only, compiled
        // straight into ta_regtest — never a separate server process.
        backends::stream_frame::generate(all_funcs, &root);

        // Take over gen_code's two remaining C-side scalar generators:
        //   - the FuncUnstId enum (GENCODE SECTION 1) in the public header ta_defs.h
        //   - the TA_SetRetCodeInfo table in ta_common/ta_retcode.c (from the csv)
        backends::ta_defs::generate(all_funcs, &enums, &root.join("include/ta_defs.h"));
        backends::retcode::generate(
            &root.join("ta_codegen/generator/templates/c/ta_retcode.c.template"),
            &root.join("src/ta_common/ta_retcode.csv"),
            &root.join("src/ta_common/ta_retcode.c"),
        );
    }

    // Take over gen_code's Java role: generate the shipped Java library files into
    // java/library/src/main/java/io/github/talib/ (the Rust/C# bindings have no
    // canonical home and stay under ta_codegen/output/, but Java — like C — is a
    // shipped product). The Maven standard layout (src/main/java + src/test/java)
    // is what pom.xml publishes from: it keeps the test package out of the shipped
    // jar, the sources jar and the javadoc by directory, with no per-plugin
    // exclusion list to keep in step.
    if backends_to_run.contains(&"java") {
        let java_pkg = root.join("ta_codegen/output/java/library/src/main/java/io/github/talib");
        // FuncUnstId.java + MAType.java depend only on enums.yaml — always safe
        // to regenerate.
        backends::java_enums::generate(&enums, &java_pkg.join("FuncUnstId.java"));
        backends::java_enums::generate_matype(&enums, &java_pkg.join("MAType.java"));
        // Whole-corpus count regardless of --func: `all_funcs` is every
        // definition even under a filtered run (see its own comment above).
        sync_pom_indicator_count(&root, all_funcs.len());
        // Core.java's GENCODE section splices ALL indicators into a single file,
        // so only regenerate on a full (unfiltered) run — a --func subset would
        // drop every other indicator's methods.
        if func_filter.is_none() {
            backends::java_shipped::generate_core(
                &generated_funcs,
                &enums,
                &registry,
                &helper_registry,
                &java_pkg.join("Core.java"),
            );
            // The shipped introspection registry, from the same rows the
            // JSON-RPC server's table renders (so the two cannot disagree).
            backends::java_metadata::generate(
                &generated_funcs,
                &enums,
                &root.join("ta_codegen/output/java/library/src/main/java"),
            );
        } else {
            println!(
                "  (skipping shipped Core.java + metadata registry — needs a full \
                 generate without --func)"
            );
        }
    }

    // Shipped C# library enums. Everything generated lives under library/src/
    // (per-indicator Core_*.cs files land there via the backend's out_subdir);
    // the hand-written scaffolding stays in library/. The per-indicator partial
    // classes need no `--func` guard — one file per indicator, so a subset is
    // correct rather than destructive.
    if backends_to_run.contains(&"csharp") {
        let csharp_src = root.join("ta_codegen/output/csharp/library/src");
        std::fs::create_dir_all(&csharp_src).unwrap();
        backends::csharp_enums::generate(&enums, &csharp_src.join("FuncUnstId.cs"));
        backends::csharp_enums::generate_matype(all_funcs, &enums, &csharp_src.join("MAType.cs"));
        // The catalogue IS whole-corpus, so it renders `all_funcs` rather than
        // the filtered set — the same source `rust_abstract` uses. Java's
        // registry instead SKIPS under `--func` because its Core.java splice
        // would drop every other indicator; here there is nothing to drop, and
        // rendering the full corpus is strictly better than skipping: a
        // `generate --func=SMA` followed by `build.py servers` would otherwise
        // leave a catalogue whose entries the server's own
        // `FunctionCatalog.Default[name]` lookups no longer find.
        backends::csharp_metadata::generate(all_funcs, &enums, &csharp_src.join("metadata"));
    }

    // The JSON-RPC servers and the C benches, last: the Java server inlines the
    // per-function fragments this run just rewrote, so it has to read them after
    // the clean.
    //
    // They have their own subcommands (`generate-servers`, `generate-bench`)
    // because `build.py servers` / `regtest.py` regenerate them alone, without a
    // full `generate`. That is a narrowing, not a separate ownership: everything
    // committed under `output/` is written by `generate` and pinned by
    // "regenerate, then `git status` must be clean". Leaving them out is how a
    // stale `TaCodegenServe.java` reached dev (#211).
    //
    // Writing them needs no JDK and no .NET SDK — it is text emission from the
    // same IR the shipped libraries render. Those toolchains are what COMPILES a
    // server (`build`), and only for the backends actually requested.
    if func_filter.is_none() {
        generate_servers(None, backend_filter);
        if backends_to_run.contains(&"c") {
            generate_bench(Some("c"));
        }
    } else {
        // Same reason Core.java skips above: one file per corpus, so a subset
        // would drop every function the filter excluded.
        println!(
            "  (skipping the JSON-RPC servers + C benches — whole-corpus files, \
             need a full generate without --func)"
        );
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

        if parser::yaml::is_reserved_dir(&dir_name) {
            continue;
        }

        let yaml_path = dir.join(format!("{}.yaml", dir_name));
        if yaml_path.exists() {
            let mut func_def = parser::yaml::parse_yaml(&yaml_path);
            // Wire the parsed .c source too: cross-function artifacts written
            // from this list need the source-derived fields — has_explicit_private,
            // the private extra params, and `streaming`. A YAML-only def silently
            // dropped declarations from the shared header on every --func=X run.
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

        if parser::yaml::is_reserved_dir(&func_name_lower) {
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
    // The repo root, not the input dir: the callee appends the template's full
    // repo-relative path, so an input-relative root made it read a path that
    // never exists and return silently, leaving this call site inert (#144).
    verify_hand_maintained_funcunstid(&enums, &root);

    for backend in &backends_to_run {
        match backends::get(backend) {
            Some(b) => b.generate_server(&funcs, &enums, &out_base),
            // Hard error: a typo'd --backend must not read as "generated nothing, fine".
            None => {
                eprintln!(
                    "Error: unknown backend '{}' (valid: {}).",
                    backend,
                    backends::all_names().join(", ")
                );
                std::process::exit(1);
            }
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
            let dir = out_base.join("c/tools");
            std::fs::create_dir_all(&dir).unwrap();
            ta_codegen_lib::bench_gen::write_c_bench(&funcs, &dir);
            ta_codegen_lib::bench_gen::write_c_stream_bench(&funcs, &dir);
        } else {
            eprintln!("generate-bench: unsupported backend '{}' (only 'c' is supported)", backend);
        }
    }
}

/// Optimization/link flags shared by the C server, C bench, and shared-library
/// builds (centralized so they cannot drift). `-ffp-contract=off` is load-bearing
/// for the FMA contract (PR #96), matching the CMake/autotools library build:
/// without it the single-TU `target_clones` `.fma` clone auto-contracts the
/// un-fused `a*b+c` sites, breaking bitwise batch-vs-stream stream_verify.
///
/// `-fno-math-errno` (issue #192) also matches those builds. It frees `sqrt()`
/// from the ISO C obligation to set `errno`, which is the only reason GCC kept
/// it scalar and out of vectorized loops; the library never reads `errno`. It
/// cannot change a computed value, so it does not weaken the bitwise gates these
/// binaries feed — but it is not free of observable effects: see the FE_INVALID
/// note in CMakeLists.txt.
const COMMON_GCC_FLAGS: &[&str] = &[
    "-lm",
    "-O3",
    "-flto",
    "-DNDEBUG",
    "-ffp-contract=off",
    "-fno-math-errno",
    "-Wno-parentheses-equality",
];

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
    // The crate enum is the enums.yaml variants followed by the `ALL`
    // wildcard sentinel; keep it in the expected list so a misplaced/duplicated
    // sentinel (which would mis-size `[i32; ALL as usize]`) is caught.
    let mut expected: Vec<&str> = fu.variants.iter().map(|v| v.name.as_str()).collect();
    expected.push("ALL");

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
    // keep `ALL` in place (compared positionally against `expected`).
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

    // FuncUnstId::COUNT sizes the crate's unstable-period array. The template is
    // copied verbatim, so the literal cannot be interpolated -- check it here
    // instead, or adding an indicator would leave the array one slot short. The
    // needle is anchored on the impl block, not on `pub const COUNT` alone, which
    // is a spelling other types in the crate also use.
    let want_count = format!("pub const COUNT: usize = {};", fu.variants.len());
    let counted = src
        .find("impl FuncUnstId {")
        .and_then(|i| src[i..].find('}').map(|j| &src[i..i + j]))
        .is_some_and(|block| block.contains(&want_count));
    if !counted {
        eprintln!(
            "Error: FuncUnstId::COUNT in {} does not match enums.yaml.\n  expected: {}",
            path.display(),
            want_count
        );
        std::process::exit(1);
    }

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

/// `servers_only` skips the two C benchmark binaries. They are the only extra
/// artifacts any backend arm builds, they are two more whole-library `-flto`
/// compiles (~3x the C server alone), and they share this function's `failures`
/// counter -- so a caller that wants a server to talk to would otherwise pay for
/// them and fail on a break that has nothing to do with it.
fn build_servers(backend_filter: Option<&str>, servers_only: bool) {
    let root = repo_root();
    let backends_to_build: Vec<&str> = match backend_filter {
        Some(b) => b.split(',').map(|s| s.trim()).collect(),
        None => backends::all_names(),
    };

    let out_base = root.join("ta_codegen/output");
    let bin_dir = root.join("bin");

    // Track server-build failures so we can exit non-zero. Without this a
    // failed compile would still exit 0, and ta_regtest would silently reuse
    // the previously-built (stale) server binary — a real break reads as green.
    let mut failures: u32 = 0;

    for backend in &backends_to_build {
        match *backend {
            "c" => {
                print!("  Building C server... ");
                let c_dir = out_base.join("c/tools");
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
                // bench_corpus.h (shared benchmark input corpus) for the two
                // generated benchmark binaries below.
                let ta_bench_dir = src_dir.join("tools/ta_bench");
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
                let bench_src = out_base.join("c/tools/ta_bench_cg.c");
                if bench_src.exists() && !servers_only {
                    print!("  Building C bench... ");
                    let bench_dst = bin_dir.join("ta_bench_cg");
                    let bench_inc_c = out_base.join("c/tools");
                    match std::process::Command::new("gcc")
                        .args([
                            "-o",
                            bench_dst.to_str().unwrap(),
                            bench_src.to_str().unwrap(),
                            &format!("-I{}", bench_inc_c.to_str().unwrap()),
                            &format!("-I{}", ta_bench_dir.to_str().unwrap()),
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
                // Also build the streaming benchmark binary if source exists
                let sbench_src = out_base.join("c/tools/ta_bench_stream.c");
                if sbench_src.exists() && !servers_only {
                    print!("  Building C stream bench... ");
                    let sbench_dst = bin_dir.join("ta_bench_stream");
                    let bench_inc_c = out_base.join("c/tools");
                    match std::process::Command::new("gcc")
                        .args([
                            "-o",
                            sbench_dst.to_str().unwrap(),
                            sbench_src.to_str().unwrap(),
                            &format!("-I{}", bench_inc_c.to_str().unwrap()),
                            &format!("-I{}", ta_bench_dir.to_str().unwrap()),
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
                let java_dir = out_base.join("java/tools");
                let class_dir = bin_dir.join("ta_codegen_java");
                // Wipe first. javac's implicit compilation off --source-path does
                // NOT reliably refresh a class that is already here, so an edited
                // library source could leave the server running the previous
                // build's bytes -- and every Java gate would agree with it,
                // because they all drive this same classpath. Demonstrated by
                // corrupting FunctionDescription.java: the abstract gate passed
                // until this directory was removed by hand. The library build
                // below gets the same property from `mvnw clean`.
                let _ = std::fs::remove_dir_all(&class_dir);
                std::fs::create_dir_all(&class_dir).ok();
                // The server's ta_abstract RPCs answer from the SHIPPED registry
                // (io.github.talib.metadata), so the library's sources are on the
                // source path. Never a server-private table: the abstract gate
                // would then never touch what ships (issue #164).
                // The main source root only: under the Maven layout the test
                // package lives in a sibling root, so it is not on the server's
                // source path at all.
                let lib_src = out_base.join("java/library/src/main/java");
                match std::process::Command::new("javac")
                    .args([
                        // JDK 17 (LTS) floor: the spliced public wrappers return
                        // `record OutRange`. Pinning it here means a too-old JDK
                        // fails with a clear unsupported-release error.
                        "--release",
                        JAVA_RELEASE,
                        "-nowarn",
                        "--source-path",
                        lib_src.to_str().unwrap(),
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
                if !build_java_library(&root, &bin_dir) {
                    failures += 1;
                }
            }
            "csharp" => {
                print!("  Building C# server... ");
                let csharp_dir = out_base.join("csharp/tools");
                let csharp_out = bin_dir.join("ta_codegen_csharp");
                std::fs::create_dir_all(&csharp_out).ok();

                // The server csproj (generated by generate-servers) compiles the
                // shipped library sources directly, so one publish builds the
                // managed indicators + the server. No native shared library:
                // the P/Invoke harness was retired with the managed backend.
                match std::process::Command::new("dotnet")
                    .args([
                        "publish",
                        "-c",
                        "Release",
                        "-o",
                        csharp_out.to_str().unwrap(),
                        csharp_dir.to_str().unwrap(),
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
                        println!("FAILED (dotnet not found: {})", e);
                    }
                }
                // The shipped library itself (the artifact users get) — the
                // server build above proves nothing about its csproj or its
                // doc-comment gate (CS1591 via TreatWarningsAsErrors), so build
                // it too, like Java's library step — and it is what runs the
                // hand-written C# suites.
                if !build_csharp_library(&root) {
                    failures += 1;
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
            // Counted as a failure: an unrecognised backend built nothing, and
            // exiting 0 here lets ta_regtest reuse a stale binary and read green.
            _ => {
                failures += 1;
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

/// Compile the **shipped** Java library and run its junit-free tests.
///
/// The JSON-RPC server above is self-contained (it splices the same per-function
/// fragments into its own inline `Core`), so building it proves nothing about
/// `output/java/library/` — which is the artifact users get. Without this step the
/// shipped tree has no compile coverage at all in any gate, and a break in the
/// hand-written scaffolding (`Core.java`'s preserved region, `CoreBuilder`, the
/// shared types) surfaces only when someone opens an IDE.
///
/// The in-tree JUnit-3 tests are skipped: `junit.jar` is not in the tree and no
/// script or CI job ever ran them. The junit-free `main()` tests are compiled AND
/// executed — "compile + one call" is the project's standing Java rule. Skips are
/// printed, never silent.
///
/// Returns `true` on success.
fn build_java_library(root: &Path, bin_dir: &Path) -> bool {
    let lib_dir = root.join("ta_codegen/output/java/library");
    let src_root = lib_dir.join("src");
    if !src_root.exists() {
        println!("  Building Java library... SKIPPED (no {})", src_root.display());
        return true;
    }

    // Maven builds the artifact -- there is no second builder, and nothing here
    // tests a class directory. Everything downstream binds to the jar this
    // command just produced, which is the same file `./mvnw -Prelease deploy`
    // uploads.
    //
    // Through the committed wrapper, never a `mvn` off PATH. `./mvnw` is not a
    // different Maven: it downloads the exact Apache distribution pinned (and
    // SHA-256 verified) in .mvn/wrapper/maven-wrapper.properties and execs the
    // real `mvn` inside it -- same binary, same plugins, same ~/.m2. So the
    // backend needs no Maven installed -- a JDK and `unzip` -- and "which Maven
    // built the release" has one answer on every machine instead of one per distro.
    //
    // `clean` on purpose. maven-compiler-plugin's incremental check is
    // all-or-nothing on the sources it can see, but a class whose source was
    // DELETED survives in target/classes and would be packaged -- and this repo
    // has already been bitten twice by a stale Java artifact reading green
    // (a class directory javac would not refresh, and a stale server binary).
    // The jar is the artifact; it gets built from nothing, every time.
    //
    // Tests are skipped here, not run: the suites are junit-free `main()`
    // classes, so surefire discovers them and executes zero methods. They are
    // compiled and run below, against the jar. (The Maven-native way to bind
    // tests to the packaged artifact is maven-failsafe-plugin, which uses the
    // project artifact by default -- but it still needs a junit/testng provider
    // to find anything, so it would mean porting the suites first.)
    // Probe `unzip` first. The only-script wrapper prefers the pinned -bin.zip, but
    // silently falls back to the -bin.tar.gz distribution when unzip is absent -- and
    // then compares THAT file against the zip's distributionSha256Sum, so the failure
    // reads "your Maven distribution might be compromised". Reproduced; it is a missing
    // utility, not an attack, and nobody should have to learn that the hard way.
    if std::process::Command::new("unzip")
        .arg("-v")
        .stdout(std::process::Stdio::null())
        .stderr(std::process::Stdio::null())
        .status()
        .is_err()
    {
        println!(
            "  Building Java library... FAILED (no `unzip` on PATH). The Maven wrapper \
             needs it to unpack the pinned distribution; without it the download falls \
             back to .tar.gz and fails the checksum with a misleading \"might be \
             compromised\". Install unzip, or narrow the build to skip java."
        );
        return false;
    }

    print!("  Building Java library (./mvnw clean package)... ");
    // Absolute path on purpose: Command::new resolves a relative program against
    // the PARENT's cwd, not `current_dir`, so "./mvnw" would look in the wrong
    // place from anywhere but the library directory.
    let mvnw = lib_dir.join("mvnw");
    match std::process::Command::new(&mvnw)
        .args(["-B", "-q", "clean", "package", "-Dmaven.test.skip=true"])
        .current_dir(&lib_dir)
        .status()
    {
        Ok(s) if s.success() => println!("OK"),
        Ok(s) => {
            println!("FAILED (exit {})", s.code().unwrap_or(-1));
            return false;
        }
        Err(e) => {
            println!(
                "FAILED (cannot run {}: {e}). It is committed and executable; the java \
                 backend needs a JDK, and the wrapper's first run needs the network. \
                 Narrow the build (--backend=, build.py --language=) to skip java.",
                mvnw.display()
            );
            return false;
        }
    }

    // Where the three jars landed, named by the build itself rather than by a
    // literal or a re-parse of pom.xml: maven-archiver writes the coordinates it
    // actually used. A publish ships all three and Central is immutable, so all
    // three are checked.
    let target = lib_dir.join("target");
    let props_path = target.join("maven-archiver/pom.properties");
    let props = std::fs::read_to_string(&props_path).unwrap_or_default();
    let field = |k: &str| {
        props
            .lines()
            .find_map(|l| l.trim().strip_prefix(&format!("{k}=")))
            .map(|v| v.trim().to_string())
    };
    let (Some(artifact), Some(version)) = (field("artifactId"), field("version")) else {
        println!(
            "  Checking Java jars... FAILED (no artifactId/version in {})",
            props_path.display()
        );
        return false;
    };
    let main_jar = target.join(format!("{artifact}-{version}.jar"));
    let sources_jar = target.join(format!("{artifact}-{version}-sources.jar"));
    let javadoc_jar = target.join(format!("{artifact}-{version}-javadoc.jar"));
    for jar in [&main_jar, &sources_jar, &javadoc_jar] {
        if !jar.exists() {
            println!("  Checking Java jars... FAILED (the build produced no {})", jar.display());
            return false;
        }
    }

    if !check_java_jars(root, bin_dir, &main_jar, &sources_jar, &javadoc_jar) {
        return false;
    }

    if !check_java_doc_examples(&src_root, &main_jar, bin_dir) {
        return false;
    }

    // The suites: discovered, not listed. A hardcoded roster is how the retired
    // `AllTests` suite went vacuous (it named one class, a later change deleted
    // it, and `ant test` kept passing). Discovery walks BOTH Maven source roots,
    // so a suite misfiled under src/main/java is caught by the jar-content check
    // above -- its class leaks into the shipped jar and the `leaked` filter fires.
    // That check is the whole net: the run below launches each suite by the
    // package its own file declares, so a misfiled suite resolves and passes.
    let mut sources: Vec<std::path::PathBuf> = Vec::new();
    let mut junit_skipped: Vec<String> = Vec::new();
    collect_java_sources(&src_root, &mut sources, &mut junit_skipped);
    sources.sort();
    junit_skipped.sort();
    // Do NOT drop a junit-importing suite from `sources` with an informational
    // line: it is then neither compiled nor run and the build stays green -- the
    // exact AllTests vacuity the discovery below exists to prevent. Fail instead:
    // either add the dependency (Maven supplies it in one line) and run the
    // suite, or do not ship the file.
    if !junit_skipped.is_empty() {
        println!(
            "  Building Java tests... FAILED ({} junit-dependent file(s) would be \
             compiled by neither Maven nor this gate, so they would never run: {}). \
             Add a test-scoped junit dependency to pom.xml, or convert them to the \
             junit-free main() form the other suites use.",
            junit_skipped.len(),
            junit_skipped.join(", ")
        );
        return false;
    }
    let test_sources: Vec<_> = sources
        .iter()
        .filter(|p| p.to_string_lossy().contains("/test/"))
        .cloned()
        .collect();

    // Each entry is the FULLY QUALIFIED name: the package comes from the file's
    // own `package` line, not from an assumption that every suite lives in one
    // package. NoPhantomIoTest deliberately sits in `io.github.talib` so it can
    // reach the package-private cores and MInteger without `setAccessible`, and a
    // hardcoded package prefix would have launched it under the wrong name.
    let (tests, unrunnable) = discover_java_tests(&sources);
    if !unrunnable.is_empty() {
        println!(
            "  Building Java tests... FAILED ({} test class(es) with no main(): {})",
            unrunnable.len(),
            unrunnable.join(", ")
        );
        return false;
    }
    if tests.is_empty() {
        println!("  Building Java tests... FAILED (no test classes discovered)");
        return false;
    }

    print!("  Building Java tests ({} files, against the jar)... ", test_sources.len());
    let test_dir = bin_dir.join("ta_codegen_java_test");
    let _ = std::fs::remove_dir_all(&test_dir);
    std::fs::create_dir_all(&test_dir).ok();
    let mut tc = std::process::Command::new("javac");
    tc.arg("--release")
        .arg(JAVA_RELEASE)
        .arg("-nowarn")
        .arg("-cp")
        .arg(&main_jar)
        .arg("-d")
        .arg(&test_dir);
    for src in &test_sources {
        tc.arg(src);
    }
    match tc.status() {
        Ok(s) if s.success() => println!("OK"),
        Ok(s) => {
            println!("FAILED (exit {})", s.code().unwrap_or(-1));
            return false;
        }
        Err(e) => {
            println!("FAILED (javac not found: {})", e);
            return false;
        }
    }

    let Ok(run_cp) = std::env::join_paths([main_jar.as_os_str(), test_dir.as_os_str()]) else {
        println!("  Running Java tests... FAILED (cannot build a classpath)");
        return false;
    };
    for test in &tests {
        let short = test.rsplit('.').next().unwrap_or(test);
        print!("  Running Java {short} (on the jar)... ");
        match std::process::Command::new("java")
            .arg("-cp")
            .arg(&run_cp)
            .arg(test)
            .status()
        {
            Ok(s) if s.success() => println!("OK"),
            Ok(s) => {
                println!("FAILED (exit {})", s.code().unwrap_or(-1));
                return false;
            }
            Err(e) => {
                println!("FAILED (java not found: {})", e);
                return false;
            }
        }
    }
    true
}

/// Assert what the three published archives contain -- the half of a release
/// that no other gate sees.
///
/// Everything else about Java is proven from source: the suites, the servers, the
/// cross-language values, and (since Maven owns the build) the compile and the
/// doclint pass. Packaging is proven by nothing, and a Central release is
/// immutable -- a `<compilerArgs>`, an `<excludes>` or a plugin bump can change
/// what reaches an archive without changing a single class, and `./mvnw -Prelease
/// deploy` would still print BUILD SUCCESS.
///
/// Four questions, none of which a compile can answer:
///
/// 1. **Did every function get in?** Each definition under `ta_codegen/input/`
///    must appear on the jar's `Core` as exactly two public `OutRange` methods
///    (the `double[]` and `float[]` overloads). Read with `javap` off the
///    archive, so a function that failed to splice, spliced without its wrapper,
///    or lost one overload cannot hide. Pinned against the input tree rather
///    than a literal, for the reason `StreamSmokeTest` stopped hardcoding its
///    corpus size: a number here breaks the build the day an indicator lands,
///    and the pressure is then to bump it rather than to check it.
///
/// 2. **Did the suites stay out?** Of all three. `src/test/java` is excluded by
///    living in its own Maven source root -- one silent mechanism, and the reason
///    pom.xml warns against the flat layout.
///
/// 3. **Is the redistribution licence-complete?** BSD-3 clause 2 for the two
///    archives that carry code. The javadoc jar is exempt by design: the plugin
///    consumes no `<resources>`, so pom.xml renders the notice into every page
///    via `<bottom>` instead -- which is why this asserts a rendered page rather
///    than a `META-INF/LICENSE` that will never be there.
///
/// 4. **Will JPMS see the name we promised?** Without the manifest entry it
///    derives `ta.lib` from the artifactId, and a derived name is not a name a
///    library may promise.
///
/// The registry's own completeness is NOT re-checked here: `MetadataTest`
/// resolves every `Functions.all()` row against the jar's `Core` by reflection
/// and asserts an exact count, and C's `ta_abstract` compares its independently
/// derived function set against the Java server's metadata RPC, which since #164
/// answers out of this same shipped registry.
fn check_java_jars(
    root: &Path,
    bin_dir: &Path,
    main_jar: &Path,
    sources_jar: &Path,
    javadoc_jar: &Path,
) -> bool {
    print!("  Checking Java jars (main, sources, javadoc)... ");

    // The function set, straight from the source of truth. Same rule the
    // generator itself uses to decide a directory is a function.
    let input = root.join("ta_codegen/input");
    let mut expected: Vec<String> = Vec::new();
    let Ok(entries) = std::fs::read_dir(&input) else {
        println!("FAILED (cannot read {})", input.display());
        return false;
    };
    for entry in entries.flatten() {
        if !entry.path().is_dir() {
            continue;
        }
        let name = entry.file_name().to_string_lossy().to_string();
        if parser::yaml::is_reserved_dir(&name) {
            continue;
        }
        if entry.path().join(format!("{name}.yaml")).exists() {
            expected.push(name.to_uppercase());
        }
    }
    expected.sort();

    // 1. Every function on the jar's Core, with both overloads.
    let javap = match std::process::Command::new("javap")
        .arg("-cp")
        .arg(main_jar)
        .arg("io.github.talib.Core")
        .output()
    {
        Ok(o) if o.status.success() => String::from_utf8_lossy(&o.stdout).into_owned(),
        Ok(o) => {
            println!("FAILED (javap exit {})", o.status.code().unwrap_or(-1));
            print!("{}", String::from_utf8_lossy(&o.stderr));
            return false;
        }
        Err(e) => {
            println!("FAILED (javap not found: {})", e);
            return false;
        }
    };
    const RET: &str = "public io.github.talib.OutRange ";
    let mut found: std::collections::BTreeMap<String, usize> = std::collections::BTreeMap::new();
    for line in javap.lines() {
        let Some(rest) = line.trim_start().strip_prefix(RET) else {
            continue;
        };
        let Some(name) = rest.split('(').next() else {
            continue;
        };
        *found.entry(name.trim().to_string()).or_insert(0) += 1;
    }
    let missing: Vec<&String> = expected.iter().filter(|f| !found.contains_key(*f)).collect();
    let unexpected: Vec<&String> = found.keys().filter(|f| !expected.contains(f)).collect();
    let wrong_arity: Vec<String> = found
        .iter()
        .filter(|(_, n)| **n != 2)
        .map(|(f, n)| format!("{f} x{n}"))
        .collect();
    if !missing.is_empty() || !unexpected.is_empty() || !wrong_arity.is_empty() {
        println!("FAILED");
        if !missing.is_empty() {
            println!("    in ta_codegen/input/ but not on the jar's Core: {:?}", missing);
        }
        if !unexpected.is_empty() {
            println!("    on the jar's Core but not in ta_codegen/input/: {:?}", unexpected);
        }
        if !wrong_arity.is_empty() {
            println!(
                "    not exactly two public OutRange overloads (double[] + float[]): {:?}",
                wrong_arity
            );
        }
        return false;
    }

    // 2/3. Listings: what leaked in, and what must be there.
    let listing = |jar: &Path| -> Option<Vec<String>> {
        match std::process::Command::new("jar").arg("--list").arg("--file").arg(jar).output() {
            Ok(o) if o.status.success() => {
                Some(String::from_utf8_lossy(&o.stdout).lines().map(str::to_string).collect())
            }
            _ => None,
        }
    };
    let mut total = 0usize;
    for (label, jar, wants_license) in [
        ("main", main_jar, true),
        ("sources", sources_jar, true),
        ("javadoc", javadoc_jar, false),
    ] {
        let Some(entries) = listing(jar) else {
            println!("FAILED (cannot list the {label} jar: {})", jar.display());
            return false;
        };
        total += entries.len();
        let leaked: Vec<&String> = entries
            .iter()
            .filter(|l| {
                l.contains("/test/")
                    || l.ends_with("Test.class")
                    || l.ends_with("Test.java")
                    || l.ends_with("Test.html")
            })
            .collect();
        if !leaked.is_empty() {
            println!(
                "FAILED ({} test entr(ies) in the {label} jar: {:?})",
                leaked.len(),
                leaked
            );
            return false;
        }
        if wants_license && !entries.iter().any(|l| l.trim() == "META-INF/LICENSE") {
            println!(
                "FAILED (no META-INF/LICENSE in the {label} jar -- BSD-3 clause 2 requires \
                 the notice in a binary redistribution, and a Central release cannot be \
                 corrected)"
            );
            return false;
        }
    }
    // The javadoc jar's share of the same clause: pom.xml renders the notice into
    // every page's footer instead, so assert a rendered page carries it.
    if !javadoc_notice_present(bin_dir, javadoc_jar) {
        println!(
            "FAILED (the javadoc jar's pages carry no copyright notice -- pom.xml's \
             <bottom> is how the javadoc artifact meets BSD-3 clause 2)"
        );
        return false;
    }

    // 4. The module name JPMS will actually derive, read off the archive.
    let module_out = match std::process::Command::new("jar")
        .arg("--describe-module")
        .arg("--file")
        .arg(main_jar)
        .output()
    {
        Ok(o) if o.status.success() => String::from_utf8_lossy(&o.stdout).into_owned(),
        Ok(o) => {
            println!("FAILED (jar --describe-module exit {})", o.status.code().unwrap_or(-1));
            print!("{}", String::from_utf8_lossy(&o.stderr));
            return false;
        }
        Err(e) => {
            println!("FAILED (jar not found: {})", e);
            return false;
        }
    };
    let derived = module_out
        .lines()
        .find_map(|l| l.trim().strip_suffix(" automatic"))
        .map(|l| l.split('@').next().unwrap_or(l).to_string());
    if derived.as_deref() != Some(JAVA_MODULE_NAME) {
        println!(
            "FAILED (the jar derives module `{}`, not `{}` -- pom.xml's \
             <Automatic-Module-Name> is missing or wrong)",
            derived.as_deref().unwrap_or("<none>"),
            JAVA_MODULE_NAME
        );
        return false;
    }

    println!("OK ({} functions, {} entries across three jars)", expected.len(), total);
    true
}

/// Whether the javadoc jar's rendered pages carry the copyright notice.
///
/// Checked on `Core`'s own page rather than on any page: it is the one every
/// reader lands on, and naming it makes the failure legible.
fn javadoc_notice_present(bin_dir: &Path, javadoc_jar: &Path) -> bool {
    let Ok(listing) = std::process::Command::new("jar")
        .arg("--list")
        .arg("--file")
        .arg(javadoc_jar)
        .output()
    else {
        return false;
    };
    let listing = String::from_utf8_lossy(&listing.stdout);
    let Some(page) = listing.lines().find(|l| l.ends_with("/Core.html")) else {
        return false;
    };
    // `jar --extract` has no --dir option, so extract relative to a scratch cwd. That
    // cwd lives under bin/ like every other scratch path here, NOT under /tmp: a fixed
    // /tmp name is shared across concurrent runs (this repo is routinely worked in
    // several worktrees at once), and this function starts by deleting it.
    let tmp = bin_dir.join("ta_codegen_javadoc_notice");
    let _ = std::fs::remove_dir_all(&tmp);
    if std::fs::create_dir_all(&tmp).is_err() {
        return false;
    }
    let ok = std::process::Command::new("jar")
        .arg("--extract")
        .arg("--file")
        .arg(javadoc_jar.canonicalize().unwrap_or_else(|_| javadoc_jar.to_path_buf()))
        .arg(page)
        .current_dir(&tmp)
        .status()
        .map(|s| s.success())
        .unwrap_or(false);
    if !ok {
        return false;
    }
    let text = std::fs::read_to_string(tmp.join(page)).unwrap_or_default();
    let _ = std::fs::remove_dir_all(&tmp);
    text.contains("BSD 3-Clause License") && text.contains("Mario Fortier")
}

/// Compile the Java examples embedded in the shipped javadoc.
///
/// `-Xdoclint` does not look inside `<pre>{@code ...}</pre>`, so an example can
/// call a method that does not exist and every other gate stays green. That is
/// exactly how `setRealInput`/`setRealOutput` — names `ParamHolder` has never
/// had — reached the metadata package's landing page. These blocks are the first
/// thing a user copies, and a published javadoc jar cannot be corrected, so they
/// are compiled like any other source.
///
/// Scope is the hand-written scaffolding plus the two package pages, listed in
/// `DOC_EXAMPLE_FILES`. `Core.java` contributes only its hand-written region:
/// the 233 blocks inside the GENCODE markers are each function's **Formula**
/// from its canonical `.md`, which is algebra and deliberately not Java.
///
/// A snippet may use `close` and `out`; anything else fails, which is the point
/// — the alternative is a preamble that quietly grows until the gate compiles
/// something no reader could.
fn check_java_doc_examples(src_root: &Path, jar_path: &Path, bin_dir: &Path) -> bool {
    const DOC_EXAMPLE_FILES: &[&str] = &[
        "main/java/io/github/talib/package-info.java",
        "main/java/io/github/talib/CoreBuilder.java",
        "main/java/io/github/talib/metadata/package-info.java",
        "main/java/io/github/talib/metadata/Functions.java",
        "main/java/io/github/talib/metadata/ParamHolder.java",
    ];
    const CORE_GENCODE_START: &str = "/**** START GENCODE SECTION 1";

    let mut snippets: Vec<(String, String)> = Vec::new();
    for rel in DOC_EXAMPLE_FILES {
        let path = src_root.join(rel);
        let Ok(text) = std::fs::read_to_string(&path) else {
            println!("  Checking Java doc examples... FAILED (cannot read {})", path.display());
            return false;
        };
        for (i, body) in extract_doc_code_blocks(&text).into_iter().enumerate() {
            snippets.push((format!("{rel}#{i}"), body));
        }
    }
    // Core.java: everything before the generated section.
    let core_path = src_root.join("main/java/io/github/talib/Core.java");
    if let Ok(text) = std::fs::read_to_string(&core_path) {
        let head = text.split(CORE_GENCODE_START).next().unwrap_or("");
        for (i, body) in extract_doc_code_blocks(head).into_iter().enumerate() {
            snippets.push((format!("Core.java(hand-written)#{i}"), body));
        }
    }

    print!("  Checking Java doc examples ({})... ", snippets.len());
    if snippets.is_empty() {
        // The scaffolding always carries examples; none found means the
        // extractor stopped matching, not that the docs got simpler.
        println!("FAILED (no <pre>{{@code ...}}</pre> blocks found — extractor out of step?)");
        return false;
    }

    let dir = bin_dir.join("ta_codegen_java_docex");
    let _ = std::fs::remove_dir_all(&dir);
    if std::fs::create_dir_all(&dir).is_err() {
        println!("FAILED (cannot create {})", dir.display());
        return false;
    }
    let mut files: Vec<std::path::PathBuf> = Vec::new();
    for (n, (origin, body)) in snippets.iter().enumerate() {
        let indented: String =
            body.lines().map(|l| format!("         {l}\n")).collect::<String>();
        // The free variables are FIELDS, not parameters: a snippet that declares
        // its own `out` shadows a field legally, where a parameter of the same
        // name is a compile error the snippet is not responsible for.
        let src = format!(
            "import io.github.talib.*;\n\
             import io.github.talib.metadata.*;\n\
             /** From {origin} — generated by the doc-example gate, not shipped. */\n\
             public class DocExample{n} {{\n\
             \x20  static double[] close = new double[300];\n\
             \x20  static double[] out = new double[300];\n\
             \x20  @SuppressWarnings(\"unused\")\n\
             \x20  static void snippet() throws Exception {{\n\
             {indented}\
             \x20  }}\n\
             }}\n"
        );
        let f = dir.join(format!("DocExample{n}.java"));
        if write_if_changed(&f, src).is_err() {
            println!("FAILED (cannot write {})", f.display());
            return false;
        }
        files.push(f);
    }

    let mut cmd = std::process::Command::new("javac");
    cmd.arg("--release").arg(JAVA_RELEASE).arg("-nowarn").arg("-cp").arg(jar_path);
    cmd.arg("-d").arg(dir.join("classes"));
    for f in &files {
        cmd.arg(f);
    }
    match cmd.output() {
        Ok(o) if o.status.success() => {
            println!("OK");
            true
        }
        Ok(o) => {
            println!("FAILED");
            for (n, (origin, _)) in snippets.iter().enumerate() {
                println!("    DocExample{n} = {origin}");
            }
            print!("{}", String::from_utf8_lossy(&o.stderr));
            false
        }
        Err(e) => {
            println!("FAILED (javac not found: {e})");
            false
        }
    }
}

/// The bodies of every `<pre>{@code ... }</pre>` block in `text`, with the
/// javadoc `*` margin stripped.
fn extract_doc_code_blocks(text: &str) -> Vec<String> {
    let mut out: Vec<String> = Vec::new();
    let mut cur: Option<Vec<String>> = None;
    for line in text.lines() {
        let t = line.trim_start();
        let t = t.strip_prefix('*').map_or(t, str::trim_start);
        if cur.is_none() {
            if t.contains("<pre>{@code") {
                cur = Some(Vec::new());
            }
            continue;
        }
        if t.starts_with("}</pre>") {
            out.push(cur.take().unwrap_or_default().join("\n"));
            continue;
        }
        if let Some(buf) = cur.as_mut() {
            buf.push(t.to_string());
        }
    }
    out
}

/// Every `*Test.java` in the shipped library's test package, in a stable order,
/// paired with the ones that cannot be run.
///
/// Discovered rather than listed so a new suite cannot be compiled-but-never-run
/// (the `AllTests` trap). A `*Test.java` without a `main()` is returned in the
/// second vec and fails the build: silently skipping it would recreate exactly
/// the vacuity this replaces, and `tests.is_empty()` cannot catch it — one
/// surviving suite masks any number of skipped ones.
fn discover_java_tests(sources: &[std::path::PathBuf]) -> (Vec<String>, Vec<String>) {
    let mut out: Vec<String> = Vec::new();
    let mut unrunnable: Vec<String> = Vec::new();
    for src in sources {
        let Some(stem) = src.file_stem().map(|s| s.to_string_lossy().to_string()) else {
            continue;
        };
        if !stem.ends_with("Test") {
            continue;
        }
        let text = std::fs::read_to_string(src).unwrap_or_default();
        // Qualify with the file's declared package. A suite that declares none
        // would be launched by its bare name, which is also how `java` names a
        // class in the unnamed package -- so this stays correct either way.
        let pkg = text
            .lines()
            .find_map(|l| l.trim().strip_prefix("package "))
            .and_then(|l| l.split(';').next())
            .map(str::trim)
            .filter(|p| !p.is_empty());
        let fq = match pkg {
            Some(p) => format!("{p}.{stem}"),
            None => stem.clone(),
        };
        if has_java_main(&text) {
            out.push(fq);
        } else {
            unrunnable.push(fq);
        }
    }
    out.sort();
    unrunnable.sort();
    (out, unrunnable)
}

/// Whether `text` declares a `public static void main(...)`.
///
/// Tolerant of the modifier order and of whitespace before the parenthesis,
/// because the caller now FAILS the build on a miss rather than warning: an
/// exact-substring match would turn `static public void main(` — legal Java —
/// into a hard failure of the whole Java build, which is a worse bug than the
/// silent skip it replaced.
fn has_java_main(text: &str) -> bool {
    for line in text.lines() {
        let Some(head) = line.split("main").next().filter(|_| line.contains("main")) else {
            continue;
        };
        // The token right after `main` must open the parameter list.
        let after = &line[head.len() + "main".len()..];
        if !after.trim_start().starts_with('(') {
            continue;
        }
        let has = |kw: &str| head.split_whitespace().any(|w| w == kw);
        if has("public") && has("static") && has("void") {
            return true;
        }
    }
    false
}

/// JDK floor for everything Java this tool compiles, kept in one place so the
/// server and the shipped library cannot drift apart. Mirrors `pom.xml`'s
/// `<maven.compiler.release>`.
const JAVA_RELEASE: &str = "17";

/// The jar's JPMS automatic module name. `check_java_jars` asserts the built jar
/// derives exactly this: without pom.xml's `<Automatic-Module-Name>` entry JPMS falls
/// back to the artifactId and consumers get `ta.lib`, and a derived name is not a name
/// a library may promise. Equal to the root package, deliberately.
const JAVA_MODULE_NAME: &str = "io.github.talib";

/// Recursively collect `.java` sources under `dir`, partitioning out the files
/// that need junit (which is not in the tree).
fn collect_java_sources(
    dir: &Path,
    sources: &mut Vec<std::path::PathBuf>,
    junit_skipped: &mut Vec<String>,
) {
    let Ok(entries) = std::fs::read_dir(dir) else {
        return;
    };
    for entry in entries.flatten() {
        let path = entry.path();
        if path.is_dir() {
            collect_java_sources(&path, sources, junit_skipped);
        } else if path.extension().is_some_and(|e| e == "java") {
            let text = std::fs::read_to_string(&path).unwrap_or_default();
            if text.contains("import junit.") || text.contains("junit.framework") {
                junit_skipped.push(
                    path.file_name().unwrap_or_default().to_string_lossy().to_string(),
                );
            } else {
                sources.push(path);
            }
        }
    }
}

/// Compile the **shipped** C# library (`ta_codegen/output/csharp/library/`).
///
/// The JSON-RPC server compiles the same `.cs` sources into its own assembly,
/// so this step exists for what the server build cannot prove: the shipped
/// csproj itself and its doc-comment gate — `GenerateDocumentationFile` +
/// `TreatWarningsAsErrors` makes CS1591 an error, the C# analog of the Java
/// `-Xdoclint` step. Then runs the hand-written suites. Returns `true` on
/// success.
fn build_csharp_library(root: &Path) -> bool {
    let lib_dir = root.join("ta_codegen/output/csharp/library");
    if !lib_dir.exists() {
        println!("  Building C# library... SKIPPED (no {})", lib_dir.display());
        return true;
    }
    print!("  Building C# library... ");
    match std::process::Command::new("dotnet")
        .args(["build", "-c", "Release", "--nologo", "-v", "quiet"])
        .current_dir(&lib_dir)
        .status()
    {
        Ok(s) if s.success() => println!("OK"),
        Ok(s) => {
            println!("FAILED (exit {})", s.code().unwrap_or(-1));
            return false;
        }
        Err(e) => {
            println!("FAILED (dotnet not found: {})", e);
            return false;
        }
    }
    run_csharp_tests(&lib_dir)
}

/// Run the hand-written C# suites, once per target framework.
///
/// The TFM list is read from the test csproj rather than hardcoded, and the
/// loop runs every entry. Today that is just `net10.0`, so the loop looks like
/// overhead — it is not. The library briefly declared `net8.0;net10.0` while
/// every gate exercised net10.0 alone, which is precisely the failure this
/// shape prevents: a TFM that is claimed but never run is a promise nobody
/// checked. Add a TFM to both csprojs and it is executed here automatically.
///
/// A missing RUNTIME for a declared TFM is reported as SKIPPED rather than
/// failing the build: `dotnet build` only needs the reference assemblies, which
/// restore from NuGet, so a box can compile a TFM it cannot launch. It is
/// printed loudly so the gap is visible in the log instead of reading as
/// coverage — the same rule the compatibility skips in server_verify follow.
///
/// Skipping them ALL is a failure, though. The tolerance above is "the others
/// still ran"; with the library on a single TFM there are no others, so one
/// skip would mean the suite reported success having executed nothing.
fn run_csharp_tests(lib_dir: &Path) -> bool {
    let test_dir = lib_dir.join("test");
    if !test_dir.exists() {
        println!("  Running C# tests... SKIPPED (no {})", test_dir.display());
        return true;
    }

    // Parsed from the test csproj so this cannot drift from what is built.
    let tfms = csharp_test_tfms(&test_dir);
    if tfms.is_empty() {
        println!("  Running C# tests... FAILED (no TargetFrameworks in the test csproj)");
        return false;
    }

    print!("  Building C# tests... ");
    match std::process::Command::new("dotnet")
        .args(["build", "-c", "Release", "--nologo", "-v", "quiet"])
        .current_dir(&test_dir)
        .status()
    {
        Ok(s) if s.success() => println!("OK"),
        Ok(s) => {
            println!("FAILED (exit {})", s.code().unwrap_or(-1));
            return false;
        }
        Err(e) => {
            println!("FAILED (dotnet not found: {})", e);
            return false;
        }
    }

    let mut ran = 0;
    for tfm in &tfms {
        print!("  Running C# tests ({tfm})... ");
        let out = std::process::Command::new("dotnet")
            .args(["run", "-c", "Release", "--no-build", "-f", tfm])
            .current_dir(&test_dir)
            .output();
        match out {
            Ok(o) if o.status.success() => {
                print!("{}", String::from_utf8_lossy(&o.stdout));
                ran += 1;
            }
            Ok(o) => {
                let combined = format!(
                    "{}{}",
                    String::from_utf8_lossy(&o.stdout),
                    String::from_utf8_lossy(&o.stderr)
                );
                // "You must install or update .NET" — the TFM compiles but its
                // runtime is absent. Not a test failure; a coverage gap.
                if combined.contains("You must install or update .NET")
                    || combined.contains("not found and no additional frameworks")
                {
                    println!("SKIPPED (no {tfm} runtime installed — this TFM went UNTESTED)");
                } else {
                    println!("FAILED (exit {})", o.status.code().unwrap_or(-1));
                    print!("{combined}");
                    return false;
                }
            }
            Err(e) => {
                println!("FAILED (dotnet not found: {e})");
                return false;
            }
        }
    }

    // Skipping SOME target framework is a coverage gap worth tolerating (the
    // others still ran). Skipping them ALL means the suite reported success
    // having executed nothing, which is the failure mode every gate in this
    // tree exists to prevent — and with the library on a single TFM, one skip
    // is all of them.
    if ran == 0 {
        println!("  Running C# tests... FAILED (no target framework could be run — nothing was tested)");
        return false;
    }
    true
}

/// The `<TargetFrameworks>` (or singular `<TargetFramework>`) of the C# test
/// project, in declaration order.
fn csharp_test_tfms(test_dir: &Path) -> Vec<String> {
    let Ok(text) = std::fs::read_to_string(test_dir.join("TALib.Test.csproj")) else {
        return Vec::new();
    };
    for (open, close) in [
        ("<TargetFrameworks>", "</TargetFrameworks>"),
        ("<TargetFramework>", "</TargetFramework>"),
    ] {
        if let Some(start) = text.find(open) {
            let rest = &text[start + open.len()..];
            if let Some(end) = rest.find(close) {
                return rest[..end]
                    .split(';')
                    .map(|s| s.trim().to_string())
                    .filter(|s| !s.is_empty())
                    .collect();
            }
        }
    }
    Vec::new()
}

/// The hand-written Rust library sources that ship inside the generated crate,
/// copied verbatim from `ta_codegen/generator/templates/rust/`. `types.rs` holds
/// `Core`/`CoreBuilder` and its API tests (issue #144); the rest are
/// `#[cfg(test)]`-only modules — DIV's zero-divisor result (issue #249), the
/// batch bodies' scratch-buffer election (issue #146), the streaming tier's
/// non-finite input rejection, and a handle's `OutRange` against batch (issue
/// #241). All are listed in the Rust backend's `clean_keep`, so `generate` never
/// deletes them.
const RUST_TEMPLATE_MODULES: &[&str] =
    &["types", "div_zero", "scratch_election", "stream_finite", "stream_out_range"];

/// Of [`RUST_TEMPLATE_MODULES`], the ones that exist only for `cargo test` and so
/// are declared `#[cfg(test)]` in the generated `mod.rs`.
const RUST_TEST_ONLY_MODULES: &[&str] =
    &["div_zero", "scratch_election", "stream_finite", "stream_out_range"];

/// `#[cfg(test)]` modules that `generate` WRITES into `src/ta_func/` rather than
/// copying from `templates/rust/` — the phantom-I/O sweep, whose two probes per
/// indicator and their ~970 call sites are emitted from the IR
/// (`backends::rust_phantom_io`).
///
/// In `src/` rather than `tests/` because it probes `<N>_Impl`, which is
/// `pub(crate)` (#265). Also listed in the Rust backend's `clean_keep`, or the
/// stale-file sweep deletes it for naming no indicator.
const RUST_GENERATED_TEST_MODULES: &[&str] = &["no_phantom_io"];

/// Version of the `ta-lib-dispatch` support crate — deliberately decoupled from
/// the repo `VERSION` the other three members track, because it changes only
/// when its one macro does.
///
/// Bumped 0.1.1 -> 0.1.2 to carry a LICENSE file and crates.io
/// `keywords`/`categories` (#179 A3/A4): a published `.crate` is immutable, so
/// altering what 0.1.1 contains is only expressible as a new version. **The
/// consequence is a publish order, not just a number** — `ta-lib` pins
/// `=DISPATCH_VERSION`, so `cargo publish` of dispatch has to land first or
/// ta-lib's dependency does not resolve.
const DISPATCH_VERSION: &str = "0.1.2";

/// Rewrite the Java pom's `<description>` "<N> indicators" from the corpus
/// count -- the one field `generate` overwrites in an otherwise hand-written,
/// preserved file (see CLAUDE.md). It is published to Maven Central and
/// immutable per version, so a stale count cannot be fixed after release
/// (issue tracked in #317); tying it to `generate` means it can no longer go
/// stale between the indicator that added it and the release that ships it.
fn sync_pom_indicator_count(root: &Path, n_funcs: usize) {
    let pom_path = root.join("ta_codegen/output/java/library/pom.xml");
    let text = std::fs::read_to_string(&pom_path)
        .unwrap_or_else(|e| panic!("cannot read {}: {e}", pom_path.display()));
    let marker = " indicators";
    let marker_pos = text
        .find(marker)
        .unwrap_or_else(|| panic!("{}: no \"{marker}\" found in <description>", pom_path.display()));
    let digits_start = text[..marker_pos]
        .rfind(|c: char| !c.is_ascii_digit())
        .map_or(0, |i| i + 1);
    let new_text = format!("{}{n_funcs}{}", &text[..digits_start], &text[marker_pos..]);
    backends::write_if_changed(&pom_path, &new_text, "pom.xml", n_funcs);
}

fn generate_rust_crate_scaffolding(
    out_base: &Path,
    funcs: &[ir::FuncDef],
    templates: &Path,
    enums: &std::collections::HashMap<String, ir::EnumDef>,
) {
    // Single source of truth for the crate version: the VERSION file at the
    // repo root (kept in sync across all packaging by scripts/sync.py).
    // Hardcoding it here once made a release bump fail the regen-check gate.
    let repo_root = out_base
        .parent()
        .and_then(|p| p.parent())
        .expect("cannot derive repo root from output dir")
        .to_path_buf();
    let version_path = repo_root.join("VERSION");
    let crate_version = std::fs::read_to_string(&version_path)
        .unwrap_or_else(|e| panic!("cannot read {}: {e}", version_path.display()))
        .trim()
        .to_string();
    // The crate's shop window — the Cargo.toml description (the crates.io
    // search-result tagline), the lib.rs crate docs and README.md — quotes an
    // indicator count and an install requirement. Derive both, never re-type
    // them: three independent copies of "161" survived seven added indicators
    // (#179 A2), and the install line said `ta-lib = "0.6"`, which resolves to
    // nothing at all (#179 A1). `funcs` is every definition even under
    // `--func=`, so the counts are whole-corpus by construction.
    let n_funcs = funcs.len();
    let n_candles = funcs.iter().filter(|f| f.group == "Pattern Recognition").count();
    // The caret requirement a user should pin: the released major.minor.
    let install_req = crate_version
        .rsplit_once('.')
        .map_or_else(|| crate_version.clone(), |(major_minor, _)| major_minor.to_string());
    // The crate-docs category index (#179 D6): the grouping the registry has
    // always known, finally said in the docs. Built in `rust_doc` rather than
    // here so it is reachable from `tests/backend_suite.rs` — `funcs` is the
    // whole corpus, and the property worth pinning is that every one of them
    // reaches the page.
    let func_index = backends::rust_doc::category_index(funcs);
    // Applied to the two long prose literals below, which hold Rust and TOML
    // samples and so cannot be `format!` strings (every brace would need
    // doubling, in text that is read far more often than it is edited).
    let fill = |text: &str| {
        text.replace("$N_FUNCS", &n_funcs.to_string())
            .replace("$N_CANDLES", &n_candles.to_string())
            .replace("$INSTALL_REQ", &install_req)
            .replace("$FUNC_INDEX", &func_index)
    };
    // Two-crate Cargo workspace: `library/` is the published `ta-lib` crate;
    // `tools/` holds the JSON-RPC server/bench — a layer on top of the library.
    let rust_dir = out_base.join("rust"); // workspace root
    let lib_dir = rust_dir.join("library");
    let src_dir = lib_dir.join("src");
    let ta_func_dir = src_dir.join("ta_func");
    let tools_dir = rust_dir.join("tools");
    let bin_dir = tools_dir.join("src/bin");

    std::fs::create_dir_all(&ta_func_dir).unwrap();
    std::fs::create_dir_all(&bin_dir).unwrap();

    // --- LICENSE, one copy per published crate (#179 A3) ---
    // A `.crate` is a source redistribution, and BSD-3 clause 1 requires the
    // notice to travel with it; `license = "BSD-3-Clause"` is only an SPDX
    // label and cargo injects no file for it. Neither manifest has
    // include/exclude, so a copy at the package root is packaged as-is.
    // `tools/` is `publish = false` and is therefore not a redistribution.
    let license_text = std::fs::read_to_string(repo_root.join("LICENSE")).unwrap_or_else(|e| {
        panic!("cannot read {}: {e}", repo_root.join("LICENSE").display())
    });
    write_if_changed(lib_dir.join("LICENSE"), &license_text).unwrap();
    println!("  Scaffolding -> {}", lib_dir.join("LICENSE").display());

    // --- workspace Cargo.toml (virtual manifest — profiles apply at the root) ---
    let workspace_toml = "[workspace]\nmembers = [\"dispatch\", \"library\", \"tools\"]\nresolver = \"2\"\n\n\
        [profile.release]\nlto = \"thin\"\ncodegen-units = 1\n";
    write_if_changed(rust_dir.join("Cargo.toml"), workspace_toml).unwrap();

    // --- dispatch/ (issue #156): the runtime FMA-dispatch macro crate ---
    // The one `unsafe` in the Rust workspace lives here, next to the CPU-feature
    // check that justifies it, so the library crate keeps #![forbid(unsafe_code)]
    // (unsafe expanded from an external macro is exempt from the caller's lint).
    // The vendoring unit is therefore this workspace directory, not library/ alone.
    let dispatch_dir = rust_dir.join("dispatch");
    let dispatch_src = dispatch_dir.join("src");
    std::fs::create_dir_all(&dispatch_src).unwrap();
    write_if_changed(dispatch_dir.join("LICENSE"), &license_text).unwrap();
    let dispatch_toml = format!(
        "[package]\nname = \"ta-lib-dispatch\"\nversion = \"{DISPATCH_VERSION}\"\nedition = \"2021\"\nrust-version = \"1.86\"\n\
        description = \"Runtime CPU-feature dispatch macro for the ta-lib crate (internal support crate).\"\n\
        license = \"BSD-3-Clause\"\nhomepage = \"https://ta-lib.org\"\nrepository = \"https://github.com/TA-Lib/ta-lib\"\n\
        documentation = \"https://docs.rs/ta-lib-dispatch\"\nreadme = \"README.md\"\n\
        keywords = [\"fma\", \"target-feature\", \"cpu-features\", \"dispatch\", \"ta-lib\"]\n\
        categories = [\"hardware-support\", \"mathematics\"]\n\n\
        [lib]\nname = \"ta_lib_dispatch\"\npath = \"src/lib.rs\"\n"
    );
    write_if_changed(dispatch_dir.join("Cargo.toml"), dispatch_toml).unwrap();
    let dispatch_readme = r#"# ta-lib-dispatch

[![crates.io](https://img.shields.io/crates/v/ta-lib-dispatch.svg)](https://crates.io/crates/ta-lib-dispatch) [![docs.rs](https://docs.rs/ta-lib-dispatch/badge.svg)](https://docs.rs/ta-lib-dispatch)

Runtime CPU-feature dispatch for fused multiply-add (FMA), used internally
by the [`ta-lib`](https://crates.io/crates/ta-lib) crate — part of the
[TA-Lib](https://ta-lib.org) project.

## Why this exists

[Fused multiply-add](https://en.wikipedia.org/wiki/Multiply%E2%80%93accumulate_operation)
(FMA) computes `a * b + c` in one rounding step instead of two, and has
been a hardware x86-64 instruction since 2013 (Haswell). A published crate
has to run on any x86-64 CPU, though, so it can't require that instruction
at compile time — the safe fallback is a software `fma()` call through
libm, which costs up to ~7x in execution speed on FMA-heavy code.

This crate is one macro: check `is_x86_feature_detected!("fma")` once,
then call whichever of two compiled clones — one built with
`#[target_feature(enable = "fma")]`, one without — matches the CPU. Both
produce identical, correctly-rounded results; only speed changes.

## Not a novel trick

Runtime CPU-feature dispatch for numerical code is standard practice —
NumPy and OpenBLAS both select their FMA/SIMD kernels the same way, at
runtime, based on detected CPU features.

## Do you need this crate directly?

Almost certainly not. `ta-lib` already depends on an exact-pinned version
of this crate — the macro is an internal contract between the two, not a
public API.

## License

BSD-3-Clause — see
[LICENSE](https://github.com/TA-Lib/ta-lib/blob/main/LICENSE).
"#;
    write_if_changed(dispatch_dir.join("README.md"), dispatch_readme).unwrap();
    let dispatch_lib = r#"// Crate docs = README.md verbatim (single source for crates.io + docs.rs;
// see README.md for the actual text).
#![doc = include_str!("../README.md")]

/// Dispatch one indicator call to its hardware-FMA clone when the CPU supports
/// FMA, else to the portable implementation.
///
/// Both paths compute IEEE-754 correctly-rounded fused multiply-adds (`vfmadd`
/// vs libm `fma()`), so which one runs can change speed, never bits.
///
/// # Contract (enforced by ta_codegen, not checkable by a macro)
///
/// `$fma` must name a method whose only `#[target_feature]` requirement is
/// `fma`; the generator emits the clone and this dispatch call as a pair.
#[macro_export]
macro_rules! dispatch_fma {
    ($core:expr, $fma:ident, $plain:ident, ( $($arg:expr),* $(,)? )) => {
        if std::arch::is_x86_feature_detected!("fma") {
            // SAFETY: $fma's only target_feature requirement is "fma", proven
            // present on this CPU by the guard above.
            unsafe { $core.$fma($($arg),*) }
        } else {
            $core.$plain($($arg),*)
        }
    };
}
"#;
    write_if_changed(dispatch_src.join("lib.rs"), dispatch_lib).unwrap();
    println!("  Scaffolding -> {}", dispatch_dir.join("Cargo.toml").display());
    println!("  Scaffolding -> {}", dispatch_dir.join("README.md").display());

    // --- library/Cargo.toml (the published `ta-lib` crate — no bin; one
    //     internal dep: the dispatch macro crate, exact-pinned) ---
    // rust-version: safe #[target_feature] (the FMA dispatch clones)
    // stabilized in 1.86 — declare the floor so pre-1.86 toolchains get a
    // clear MSRV message instead of an opaque E0658.
    let lib_toml_head = format!(
        "[package]\nname = \"ta-lib\"\nversion = \"{crate_version}\"\nedition = \"2021\"\nrust-version = \"1.86\"\n\
         description = \"Technical analysis library: {n_funcs} indicators (SMA, EMA, RSI, MACD, \
         Bollinger Bands, ATR, Stochastic, candlestick patterns) — the official Rust port of \
         TA-Lib, verified against the C reference.\""
    );
    let lib_toml_tail = r#"
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

[dependencies]
# Exact pin (companion-crate pattern, like serde_derive): the macro is an
# internal contract, so a published ta-lib must never float onto a newer
# dispatch release. Publish order when releasing to crates.io: dispatch
# first, then ta-lib (cargo strips `path` and resolves by version).
#
# While the pinned dispatch version is not on crates.io yet, `cargo package -p
# ta-lib` on its own cannot resolve it. Package the pair — `cargo package -p
# ta-lib-dispatch -p ta-lib` — and cargo verifies ta-lib against a temporary
# registry built from the sibling.
"#;
    let lib_toml_dep = format!(
        "ta-lib-dispatch = {{ path = \"../dispatch\", version = \"={DISPATCH_VERSION}\" }}\n"
    );
    let lib_cargo_path = lib_dir.join("Cargo.toml");
    write_if_changed(
        &lib_cargo_path,
        format!("{lib_toml_head}{lib_toml_tail}{lib_toml_dep}"),
    )
    .unwrap();
    println!("  Scaffolding -> {}", lib_cargo_path.display());

    // --- tools/Cargo.toml (server/bench crate; depends on the library) ---
    //
    // `arbitrary_precision` is load-bearing, not a nicety. serde_json's default
    // number parser scales the significand by an f64 power of ten, which is not
    // correctly rounded: it returns 2.7755575615628914e-16 (the shortest repr of
    // 1.25 * 2^-52) ONE ULP LOW. strtod, Double.parseDouble and double.Parse are
    // all correctly rounded, so the Rust server alone was computing on different
    // inputs than the other three, and every cross-language comparison that went
    // over the decimal transport silently compared unequal data. The feature keeps
    // the original text and defers to std's parser, which is exact.
    //
    // Found via CDLLONGLINE on the DBL_EPSILON dataset (#164): there
    // `upperShadow` and `candleaverage(ShadowShort)` are EXACTLY equal, so one ULP
    // on inHigh flips `<` and the whole output from 0 to -100. The shipped crate
    // was always right — handed the same bits it answers 0, like C.
    let tools_toml = format!(
        "[package]\nname = \"ta-lib-tools\"\nversion = \"{crate_version}\"\nedition = \"2021\"\n\
         publish = false\n\n[[bin]]\nname = \"ta_codegen_serve\"\n\
         path = \"src/bin/ta_codegen_serve.rs\"\n\n[dependencies]\n\
         ta-lib = {{ path = \"../library\" }}\n\
         serde_json = {{ version = \"1\", features = [\"arbitrary_precision\"] }}\n"
    );
    let tools_cargo_path = tools_dir.join("Cargo.toml");
    write_if_changed(&tools_cargo_path, tools_toml).unwrap();
    println!("  Scaffolding -> {}", tools_cargo_path.display());

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
    write_if_changed(cargo_config_dir.join("config.toml"), cargo_config).unwrap();

    // --- src/lib.rs ---
    let lib_rs = r#"//! # TA-Lib: Technical Analysis Library
//!
//! $N_FUNCS technical-analysis indicators — moving averages, momentum oscillators,
//! volatility bands, volume studies, Hilbert Transform cycle analysis, statistics,
//! price transforms, and $N_CANDLES candlestick-pattern recognizers — as a pure-Rust crate.
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
//!
//! let out = core.SMA(0, close.len() - 1, &close, 3, &mut sma)?;
//!
//! // The first 3-period average lands at input index 2 (the lookback):
//! assert_eq!((out.beg_idx, out.count), (2, 8));
//! assert_eq!(sma[0], 12.0); // (11 + 12 + 13) / 3
//! # Ok::<(), ta_lib::RetCode>(())
//! ```
//!
//! # API shape
//!
//! Every indicator is a method on [`Core`] and follows the same pattern:
//!
//! * Inputs are `&[f64]` slices, computed over the range `startIdx..=endIdx`.
//! * Outputs are written into caller-provided `&mut` slices. An indicator consumes a
//!   number of leading values (its *lookback*) before producing output — query it with
//!   the matching `*_Lookback` method (e.g. [`Core::SMA_Lookback`]).
//! * Integer parameters accept [`Core::INTEGER_DEFAULT`], and real parameters
//!   [`Core::REAL_DEFAULT`], to select their default value; a moving-average type takes
//!   [`MAType::DEFAULT`] instead, the sentinel being unrepresentable at a typed enum.
//! * Every call returns [`Result`]`<`[`OutRange`]`, `[`RetCode`]`>`, so it composes with
//!   `?`. [`OutRange`] says where the values start ([`beg_idx`](OutRange::beg_idx), in the
//!   input series' coordinates) and how many there are ([`count`](OutRange::count)).
//!   A range shorter than the lookback is a **success with no values**, not an error.
//!
//! [`Core`] is immutable after construction: its per-instance settings — unstable
//! period and candlestick thresholds — are chosen up front with
//! [`Core::builder()`] and then frozen, so a `Core` is `Send + Sync` and
//! can be shared read-only across threads (e.g. via `Arc`) with no locking:
//!
//! ```
//! use ta_lib::{Core, FuncUnstId};
//!
//! let core = Core::builder()
//!     .unstable_period(FuncUnstId::EMA, 10)
//!     .build()?;
//! # Ok::<(), ta_lib::RetCode>(())
//! ```
//!
//! The setters are infallible so that they chain; a rejected argument is
//! reported once, by `build()`, as [`RetCode::BadParam`].
//!
//! To change a setting, build a new `Core` (cloning is cheap); [`Core::to_builder()`]
//! seeds a builder from an existing instance.
//!
//! The crate is `#![forbid(unsafe_code)]`: a bounds violation panics, it never
//! triggers undefined behavior. On x86-64, the batch entry
//! points of indicators built on fused multiply-adds are compiled twice and the
//! hardware-FMA clone is selected at runtime (the same dispatch the C library
//! performs via `target_clones`); both paths are correctly rounded, so results
//! are bit-identical either way. The streaming tier stays single-path.
//!
//! # Live data
//!
//! The calls above take a whole series at once. For a feed that arrives one bar
//! at a time, each indicator also has a *streaming* form: an `*_open` method
//! ([`Core::sma_open`], [`Core::rsi_open`], …) warms a handle up on the history
//! you already have, and from then on one bar in gives that bar's value out,
//! with no re-scan of the series and no allocation per bar.
//!
//! ```
//! use ta_lib::Core;
//!
//! let history = [11.0, 12.0, 13.0, 14.0, 15.0];
//! let core = Core::new();
//! let (mut sma, last) = core.sma_open(&history, 3)?;
//!
//! assert_eq!(last, 14.0); // (13 + 14 + 15) / 3, the last history bar
//! assert_eq!(sma.out_range().count, 3);
//!
//! // A bar that has not closed yet: ask without committing it.
//! assert_eq!(sma.peek(16.0)?, 15.0);
//! assert_eq!(sma.out_range().count, 3);
//!
//! // Once it closes, commit it — same value, and the range advances.
//! assert_eq!(sma.update(16.0)?, 15.0);
//! assert_eq!(sma.out_range().count, 4);
//!
//! // A non-finite bar is rejected, and still counted: the handle's output for
//! // it is the previous one, held, and its state is untouched.
//! assert!(sma.update(f64::NAN).is_err());
//! assert_eq!(sma.out_range().count, 5);
//! # Ok::<(), ta_lib::RetCode>(())
//! ```
//!
//! The handle's value at every bar is bit-identical to what the batch call
//! reports for that bar. [`SmaStream::out_range`] carries the same
//! [`OutRange`] the batch tier returns — the bars the handle has an output for
//! — and every bar handed to [`SmaStream::update`] advances it by one, a bar
//! rejected as non-finite included: its output is the previous one, held.
//! [`SmaStream::peek`] leaves it alone; cloning a handle forks an independent
//! stream, and dropping it closes the stream.
//!
//! The full function reference, grouped by category, is at
//! [ta-lib.org/functions](https://ta-lib.org/functions/); the guides are at
//! [ta-lib.org/api/rust](https://ta-lib.org/api/rust/) and, for the streaming
//! tier, [ta-lib.org/api/rust/stream](https://ta-lib.org/api/rust/stream/).
//!
//! # Indicators by category
//!
//! Every indicator is a method on [`Core`], and the methods are one flat
//! alphabetical list — so this is where the grouping lives. It is the same
//! grouping the registry answers at run time ([`abstract_api::Group`], reported
//! per function as [`FuncInfo::group`](abstract_api::FuncInfo::group)), and each
//! entry carries that row's own one-line hint. Follow a link for the function's
//! formula, arguments, ranges and a runnable example.
//!
$FUNC_INDEX

#![forbid(unsafe_code)]
// Every public item, and every public enum variant and struct field, carries its
// own documentation (#179 D7). `warn` rather than `deny` so that a future rustc
// widening the lint cannot break a downstream build; the nightly's
// `cargo clippy -- -D warnings` is what makes it a gate here.
#![warn(missing_docs)]
#![allow(non_snake_case, non_camel_case_types, unused_variables, unused_assignments, unused_mut, unused_parens, arithmetic_overflow)]
// Generated code: Clippy's style/complexity lints are noise on machine output, and
// several "fixes" would change numeric behavior — e.g. `neg_cmp_op_on_partial_ord`
// on C's `!(a < b)` NaN idiom, or De Morgan rewrites under `nonminimal_bool`. The
// crate is verified bit-exact against the C reference, so these are suppressed rather
// than applied. `too_many_arguments` is inherent to the C API arity.
#![allow(clippy::all, clippy::pedantic)]
#![allow(clippy::approx_constant)] // PI (180/3.141592653589793) is copied verbatim from the C source.
// Private, so every public type has exactly one path. `ta_func` is the C source
// directory's name, and `ta_lib::ta_func::Core` would stutter; the glob below is
// the only way in (#179 C5).
mod ta_func;
pub mod abstract_api;
pub use ta_func::*;

// The README is the crate's front page on crates.io and on GitHub, and its Rust
// sample is a claim about this API — yet nothing in the tree compiled it:
// `readme = "README.md"` is packaging metadata, and every other doctest here
// comes from a generated per-function page. So the front page was the one piece
// of Rust in this crate that could say anything, and twice it did: the install
// line resolved to no published version (#179 A1) and the indicator count was
// seven stale (#179 A2), both found by reading rather than by a gate. The counts
// and the install requirement are derived now; this covers the code.
//
// `cfg(doctest)` is what keeps it to `cargo test --doc`: the item does not exist
// during `cargo build`, `cargo clippy` or `cargo doc`, so the README's headings
// never appear in the rendered docs and its links are not resolved as intra-doc
// links (they are ordinary Markdown links, and must stay that way to render on
// crates.io).
#[cfg(doctest)]
#[doc = include_str!("../README.md")]
struct ReadmeExamples;
"#;
    let lib_path = src_dir.join("lib.rs");
    write_if_changed(&lib_path, fill(lib_rs)).unwrap();
    println!("  Scaffolding -> {}", lib_path.display());

    // --- README.md (crates.io / GitHub front page for the crate) ---
    let readme = r#"<!-- Generated by ta_codegen — do not edit. -->

# TA-Lib for Rust

[![crates.io](https://img.shields.io/crates/v/ta-lib.svg)](https://crates.io/crates/ta-lib) [![docs.rs](https://docs.rs/ta-lib/badge.svg)](https://docs.rs/ta-lib)

[TA-Lib](https://ta-lib.org) — the widely used technical-analysis library — as a
pure-Rust crate: $N_FUNCS indicators covering moving averages, momentum oscillators
(RSI, MACD, Stochastic), volatility (Bollinger Bands, ATR), volume, Hilbert
Transform cycle analysis, statistics, price transforms, and $N_CANDLES candlestick
patterns.

Every function is generated from the same canonical definitions as the C library
and verified against the C reference implementation.

## Install

```toml
[dependencies]
ta-lib = "$INSTALL_REQ"
```

## Quick start

```rust
use ta_lib::{Core, RetCode};

fn main() -> Result<(), RetCode> {
    let close = [11.0, 12.0, 13.0, 14.0, 15.0, 16.0, 17.0, 18.0, 19.0, 20.0];
    let core = Core::new();
    let mut sma = vec![0.0; close.len()];

    let out = core.SMA(0, close.len() - 1, &close, 3, &mut sma)?;

    // The first 3-period average lands at input index 2 (the lookback):
    assert_eq!((out.beg_idx, out.count), (2, 8));
    assert_eq!(sma[0], 12.0); // (11 + 12 + 13) / 3
    Ok(())
}
```

Every indicator is a method on `Core` with the same calling pattern: `&[f64]`
input slices, a `startIdx..=endIdx` range, caller-provided output slices, and a
`Result<OutRange, RetCode>`. On success the `OutRange` says where the values
start (`beg_idx`, in the input series' coordinates) and how many there are
(`count`); `*_Lookback` methods return how many leading values an indicator
consumes before the first one exists.

A range shorter than the lookback is a **success with no values** (`count == 0`),
not an error — the same contract as C, Java and C#.

## Configuration

`Core` is immutable after construction. The value-affecting settings — unstable
period and candlestick thresholds — are chosen up front with a builder and then
frozen:

```rust
use ta_lib::{Core, FuncUnstId, RetCode};

fn main() -> Result<(), RetCode> {
    let core = Core::builder()
        .unstable_period(FuncUnstId::EMA, 10)
        .build()?;

    assert_eq!(core.get_unstable_period(FuncUnstId::EMA)?, 10);
    Ok(())
}
```

The setters are infallible so that they chain; a rejected argument is reported
once, by `build()`, as `RetCode::BadParam`.

Because a configured `Core` only ever reads its settings, it is `Send + Sync` and
can be shared read-only across threads (e.g. an `Arc<Core>` with concurrent
indicator calls) without locking. To change a setting, build a new `Core`.

## Live data

The calls above take a whole series at once. For a feed that arrives one bar at
a time, each indicator also has a **streaming** form: an `*_open` method warms a
handle up on the history you already have, and from then on one bar in gives
that bar's value out — no re-scan of the series, no allocation per bar, and
bit-identical to what the batch call reports for the same bar.

```rust
use ta_lib::{Core, RetCode};

fn main() -> Result<(), RetCode> {
    let history = [11.0, 12.0, 13.0, 14.0, 15.0];
    let core = Core::new();
    let (mut sma, last) = core.sma_open(&history, 3)?;

    assert_eq!(last, 14.0); // (13 + 14 + 15) / 3, the last history bar
    assert_eq!(sma.out_range().count, 3);

    // A bar that has not closed yet: ask without committing it.
    assert_eq!(sma.peek(16.0)?, 15.0);
    assert_eq!(sma.out_range().count, 3);

    // Once it closes, commit it — same value, and the range advances.
    assert_eq!(sma.update(16.0)?, 15.0);
    assert_eq!(sma.out_range().count, 4);

    // A non-finite bar is rejected, and still counted: the handle's output for
    // it is the previous one, held, and its state is untouched.
    assert!(sma.update(f64::NAN).is_err());
    assert_eq!(sma.out_range().count, 5);
    Ok(())
}
```

`out_range()` carries the same `OutRange` the batch tier returns — the bars the
handle has an output for — and every bar handed to `update` advances it by one,
a bar rejected as non-finite included: its output is the previous one, held.
`peek` leaves it alone. Cloning a handle forks an independent stream, and
dropping it closes the stream.

## Documentation

- API reference: <https://docs.rs/ta-lib>
- Rust guide: <https://ta-lib.org/api/rust/> — and the streaming tier: <https://ta-lib.org/api/rust/stream/>
- Per-function reference (formulas, notes, sources): <https://ta-lib.org/functions/>
- Project home: <https://ta-lib.org>

## License

BSD-3-Clause — see [LICENSE](https://github.com/TA-Lib/ta-lib/blob/main/LICENSE).
"#;
    let readme_path = lib_dir.join("README.md");
    write_if_changed(&readme_path, fill(readme)).unwrap();
    println!("  Scaffolding -> {}", readme_path.display());

    // --- Copy the hand-written modules from ta_codegen/generator/templates/rust/ ---
    for module in RUST_TEMPLATE_MODULES {
        let src = templates.join(format!("{module}.rs"));
        if src.exists() {
            let dest = ta_func_dir.join(format!("{module}.rs"));
            copy_if_changed(&src, &dest).unwrap();
            println!("  Copied {module}.rs -> {}", dest.display());
        }
    }

    // --- src/ta_func/mod.rs (generated: imports types + declares indicator modules) ---
    let mut mod_rs = String::new();
    mod_rs.push_str(
        r#"//! Generated technical-analysis functions — one private module per indicator,
//! all exposed as methods on [`Core`].

// Types and Core struct are in types.rs (hand-written, not generated).
mod types;
pub use types::*;
"#,
    );

    // The MAType enum. Rendered by `backends::rust_enums` and spliced here so it
    // lands in an already-generated file -- see that module for why it carries
    // no `#[repr]` and why `#[non_exhaustive]` is load-bearing.
    mod_rs.push_str(&backends::rust_enums::render_matype(enums));

    // Hand-written test-only modules (not generated; see templates/rust/).
    if !RUST_TEST_ONLY_MODULES.is_empty() {
        mod_rs.push_str(
            "\n// Hand-written test-only modules (not generated; see templates/rust/).\n",
        );
        for module in RUST_TEST_ONLY_MODULES {
            mod_rs.push_str(&format!("#[cfg(test)]\nmod {module};\n"));
        }
    }

    // Generated test-only modules.
    if !RUST_GENERATED_TEST_MODULES.is_empty() {
        mod_rs.push_str("\n// Generated test-only modules.\n");
        for module in RUST_GENERATED_TEST_MODULES {
            mod_rs.push_str(&format!("#[cfg(test)]\nmod {module};\n"));
        }
    }

    mod_rs.push_str("\n// Generated indicator modules:\n");

    // Add mod declarations for each generated indicator
    let mut func_names: Vec<String> = funcs
        .iter()
        .map(|f| f.name.to_lowercase())
        .collect();
    func_names.sort();

    for name in &func_names {
        mod_rs.push_str(&format!("mod {};\n", name));
    }

    // Stream handle re-exports: modules are private, so each generated stream
    // handle type surfaces at the crate root (`ta_lib::SmaStream`) via mod.rs.
    let stream_lookup = ta_codegen_lib::streaming::FuncsLookup(funcs);
    let mut stream_exports: Vec<(String, String)> = funcs
        .iter()
        .filter(|f| ta_codegen_lib::backends::rust_stream::emits_stream(f, &stream_lookup))
        .map(|f| {
            (
                f.name.to_lowercase(),
                ta_codegen_lib::backends::rust_stream::stream_type_name(f),
            )
        })
        .collect();
    stream_exports.sort();
    if !stream_exports.is_empty() {
        mod_rs.push_str("\n// Generated stream handles (one per streamable indicator):\n");
        for (module, ty) in &stream_exports {
            mod_rs.push_str(&format!("pub use {module}::{ty};\n"));
        }
    }

    let mod_path = ta_func_dir.join("mod.rs");
    write_if_changed(&mod_path, &mod_rs).unwrap();
    println!("  Scaffolding -> {}", mod_path.display());

    // --- src/bin/ta_codegen_serve.rs (placeholder) ---
    let placeholder_bin = r#"fn main() {
    eprintln!("Server not yet generated — run: ta_codegen generate-servers --backend=rust");
}
"#;
    let bin_path = bin_dir.join("ta_codegen_serve.rs");
    // Only write placeholder if the server binary doesn't already exist
    if !bin_path.exists() {
        write_if_changed(&bin_path, placeholder_bin).unwrap();
        println!("  Scaffolding -> {} (placeholder)", bin_path.display());
    }

    println!("  Rust crate scaffolding generated ({} indicators)", func_names.len());
}

/// Remove per-function generated files for a backend so stale artifacts
/// from a previous run cannot leak into `generate-servers`.
fn remove_stale_generated_files(out_base: &Path, backend: &str, funcs: &[ir::FuncDef]) {
    let Some(backend) = backends::get(backend) else {
        return;
    };
    // Server-only backends emit no per-indicator files to clean.
    if !backend.emits_lib_files() {
        return;
    }
    let dir = backend.lib_output_dir(out_base);
    if !dir.exists() {
        return;
    }
    let (prefix, suffix) = backend.clean_glob();
    // Hand-written / scaffolding files (types.rs, mod.rs, ...) are preserved.
    let keep = backend.clean_keep();
    // `generate_backend` writes exactly one file per function, named by
    // `file_name`, so this is the complete set this run is entitled to leave
    // behind. Anything else matching the glob belonged to an indicator that is
    // gone.
    let written: std::collections::BTreeSet<String> =
        funcs.iter().map(|f| backend.file_name(f)).collect();
    let mut count = 0;
    if let Ok(entries) = std::fs::read_dir(&dir) {
        for entry in entries.flatten() {
            let name = entry.file_name().to_string_lossy().to_string();
            if name.starts_with(prefix)
                && name.ends_with(suffix)
                && !keep.contains(&name.as_str())
                && !written.contains(&name)
            {
                std::fs::remove_file(entry.path()).ok();
                count += 1;
            }
        }
    }
    if count > 0 {
        println!(
            "  Removed {count} stale {} file(s) from {}",
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
    // Server-only backends emit no per-indicator library files.
    if !backend.emits_lib_files() {
        return;
    }
    let output = backend.generate(func_def, enums, registry, helpers);
    let dir = backend.lib_output_dir(out_base);
    std::fs::create_dir_all(&dir).unwrap();
    let path = dir.join(backend.file_name(func_def));
    write_if_changed(&path, &output).unwrap();
    println!("  {} -> {}", func_def.name, path.display());
}
